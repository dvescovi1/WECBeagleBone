// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: tps659xx.cpp
//
#include "omap.h"
#include <pm.h>
#include "ceddkex.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"
#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include "soc_cfg.h"
#include "bsp_cfg.h"
#include "triton.h"
#include <initguid.h>
#include "twl.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// disable zero size array warning
#pragma warning(disable:4200)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)
#define ZONE_RTC            DEBUGZONE(15)


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"Triton (TWL)", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"IST",         L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"RTC"
    },
    0x8003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define TWL_DEVICE_COOKIE       'twlD'
#define TWL_INSTANCE_COOKIE     'twlI'
#define MAX_SIH_COUNT           (6)
#define MATRIX_QUEUE_SIZE       (16)

//------------------------------------------------------------------------------
//  Local enumerations

// defines which triton interrupts should be enabled
typedef enum {
    kTritonIntrEnabled,
    kTritonIntrWakeup,
    kTritonIntrDisabled
} InterruptMode_e;


//------------------------------------------------------------------------------
//  Local Structures

// contains information about an interrupt status register
typedef struct {
    UINT    statusSubaddress;
    UINT    maskSubaddress;
    UINT8   ffEnable;
    UINT8   ffWakeupEnable;
    UINT    interruptCount;
} StatusRegister_t;

// contains information about SIH
typedef struct {
    UINT                ctrlSubaddress;
    StatusRegister_t    StatusRegisters[];
} SIHEntry_t;

typedef struct {
    DWORD cookie;
    DWORD irq;    
    DWORD gpio;
    DWORD priority256;
    HANDLE hI2C;
    HANDLE hGpio;
    CRITICAL_SECTION cs;
    DWORD slaveAddress;
    DWORD sysIntr;
    DWORD wakeupCount;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    BOOL intrThreadExit;
    CEDEVICE_POWER_STATE powerState;
    HANDLE hSetIntrEvent[TWL_MAX_INTR];
} Device_t;

typedef struct {
    DWORD cookie;
    DWORD address;
    Device_t *pDevice;
} Instance_t;

typedef BOOL (*pfnSIH)(Device_t *pDevice, HANDLE *rgEvents, SIHEntry_t const *pSIHEntry);

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Gpio", PARAM_DWORD, TRUE, offset(Device_t, gpio),
        fieldsize(Device_t, gpio), NULL
    }, {
        L"Priority256", PARAM_DWORD, FALSE, offset(Device_t, priority256),
        fieldsize(Device_t, priority256), (VOID*)100
    }
};

SIHEntry_t _SIHEntry_GPIO = {
    TWL_GPIO_SIH_CTRL,
    {
        {(TWL_GPIO_ISR1A), (TWL_GPIO_IMR1A), 0xFF, 0xFF, 8},
        {(TWL_GPIO_ISR2A), (TWL_GPIO_IMR2A), 0xFF, 0xFF, 8},
        {(TWL_GPIO_ISR3A), (TWL_GPIO_IMR3A), 0x03, 0x03, 2},
        {(0), (0), 0, 0}
    }
};

SIHEntry_t _SIHEntry_Keypad = {
    TWL_KEYP_SIH_CTRL,
    {
        {(TWL_KEYP_ISR1), (TWL_KEYP_IMR1), 0x0F, 0x0F, 4},
        {(0), (0), 0, 0, 0}
    }
};

SIHEntry_t _SIHEntry_BCI = {
    TWL_BCISIHCTRL,
    {
        {(TWL_BCIISR1A), (TWL_BCIIMR1A), 0xFF, 0xFF, 8},
        {(TWL_BCIISR2A), (TWL_BCIIMR2A), 0x0F, 0x0F, 4},
        {(0), (0), 0, 0}
    }
};

SIHEntry_t _SIHEntry_MADC = {
    TWL_MADC_SIH_CTRL,
    {
        {(TWL_MADC_ISR1), (TWL_MADC_IMR1), 0x0F, 0x0F, 4},
        {(0), (0), 0, 0, 0}
    }
};

SIHEntry_t _SIHEntry_USB = {
    0,
    {
        {(TWL_USB_INT_STS), (TWL_USB_INT_EN_RISE), 0x1E, 0x00, 8},
        {(TWL_USB_INT_STS), (TWL_USB_INT_EN_FALL), 0x1E, 0x00, 8},
        {(TWL_OTHER_INT_STS), (TWL_OTHER_INT_EN_RISE), 0x00, 0x00, 8},
        {(TWL_OTHER_INT_STS), (TWL_OTHER_INT_EN_FALL), 0x00, 0x00, 8},
        {(0), (0), 0, 0, 0}
    }
};

SIHEntry_t _SIHEntry_Power = {
    TWL_PWR_SIH_CTRL,
    {
        {(TWL_PWR_ISR1), (TWL_PWR_IMR1), 0xFF, 0xFF, 8},
        {0, 0, 0, 0}
    }
};

static SIHEntry_t const *s_pSIHEntries [] = {
    &_SIHEntry_GPIO,
    &_SIHEntry_Keypad,
    &_SIHEntry_BCI,
    &_SIHEntry_MADC,
    &_SIHEntry_USB,
    &_SIHEntry_Power,
    NULL
};

typedef enum {
    SIHEntry_GPIO = 0,
    SIHEntry_Keypad,
    SIHEntry_BCI,
    SIHEntry_MADC,
    SIHEntry_USB,
    SIHEntry_Power,
    SIHEntry_MAX,
} t_SIHEntry;

static pfnSIH s_pSIHRoutines[SIHEntry_MAX];

UINT8 _rgKeypadMatrixQueue[MATRIX_QUEUE_SIZE][TWL_KEYPAD_ROWS];
DWORD _nKeypadMatrixQueueHead = 0;
DWORD _nKeypadMatrixQueueTail = 0;
LONG _nKeypadMatrixQueueCount = 0;

//------------------------------------------------------------------------------
//  prototype

BOOL TWL_Deinit(DWORD context);
static BOOL InitializeInterrupts(Device_t *pDevice);
DWORD TWL_InterruptThread(void *pContext);
static BOOL TWL_ReadRegs(DWORD c, DWORD addr, void *pBuffer, DWORD size);
static BOOL TWL_WriteRegs(DWORD c, DWORD addr, const void *pBuffer, DWORD size);

//------------------------------------------------------------------------------
//
//  Function:  GetFirstEventByGroup
//
//  Returns a pointer to the first set of a given group.
//
HANDLE*
GetFirstEventByGroup(
    Device_t *pDevice,
    DWORD     group
    )
{
    HANDLE *pEvent = NULL;
    switch (group)
        {
        case 0:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_GPIO_0);
            break;
        case 1:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_ITKPI);
            break;
        case 2:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_WOVF);
            break;
        case 3:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_MADC_RT);
            break;
        case 4:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_USB_RISE_IDGND);
            break;
        case 5:
            pEvent = pDevice->hSetIntrEvent + TWL_ARRAYINDEX(TWL_INTR_PWRON);
            break;
        }
    
    return pEvent;
}


//------------------------------------------------------------------------------
//
//  Function:  ReadRegs
//
//  Internal routine to read triton registers.
//
BOOL
ReadRegs(
    Device_t *pDevice,
    DWORD address,
    VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;

    EnterCriticalSection(&pDevice->cs);
    // set slave address if necessary
    if (pDevice->slaveAddress != HIWORD(address))
        {
        I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress() | HIWORD(address));        
        pDevice->slaveAddress = HIWORD(address);
        }

    if (I2CRead(pDevice->hI2C, (UCHAR)address, pBuffer, size) != size)
        {
        goto cleanUp;
        }
    
    // We succceded
    rc = TRUE;

cleanUp:    
    LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  WriteRegs
//
//  Internal routine to write triton registers.
//
BOOL
WriteRegs(
    Device_t *pDevice,
    DWORD address,
    const VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;

    EnterCriticalSection(&pDevice->cs);

    // set slave address if necessary
    if (pDevice->slaveAddress != HIWORD(address))
        {
        I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress() | HIWORD(address));        
        pDevice->slaveAddress = HIWORD(address);
        }

    if (I2CWrite(pDevice->hI2C, (UCHAR)address, pBuffer, size) != size)
        {
        goto cleanUp;
        }   

    // We succceded
    rc = TRUE;

cleanUp:  
    LeaveCriticalSection(&pDevice->cs);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  WriteKeypadMatrixQueue
//
//  This function acts as the IST for the keypad.
//
BOOL 
WriteKeypadMatrixQueue(
    UINT8 matrix[TWL_KEYPAD_ROWS],
    HANDLE hKeypadEvent
    )
{    
    DEBUGMSG(ZONE_INFO, 
        (L"WriteKeypadMatrixQueue:_nKeypadMatrixQueueCount=%d, "
         L"0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n",
         _nKeypadMatrixQueueCount, matrix[0], matrix[1], matrix[2], 
         matrix[3], matrix[4], matrix[5], matrix[6], matrix[7]
        ));
    
    if (_nKeypadMatrixQueueCount < MATRIX_QUEUE_SIZE)
        {
        memcpy(_rgKeypadMatrixQueue[_nKeypadMatrixQueueTail], matrix,  TWL_KEYPAD_ROWS);
        _nKeypadMatrixQueueTail = (_nKeypadMatrixQueueTail + 1) % MATRIX_QUEUE_SIZE;
        
        InterlockedIncrement(&_nKeypadMatrixQueueCount);
        
        SetEvent(hKeypadEvent);
        return TRUE;
        }
    else if (hKeypadEvent != NULL)
        {
        // keypad matrix queue is full so just signal keypad listener
        // and throw-away the character
        SetEvent(hKeypadEvent);
        return TRUE;
        }

    
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  ReadKeypadMatrixQueue
//
//  This function acts as the IST for the keypad.
//
BOOL 
ReadKeypadMatrixQueue(
    UINT8 matrix[TWL_KEYPAD_ROWS],
    HANDLE hKeypadEvent
    )
{
    DEBUGMSG(ZONE_INFO, 
        (L"ReadKeypadMatrixQueue:_nKeypadMatrixQueueCount=%d, "
         L"0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n",
         _nKeypadMatrixQueueCount, matrix[0], matrix[1], matrix[2], 
         matrix[3], matrix[4], matrix[5], matrix[6], matrix[7]
        ));
    
    if (_nKeypadMatrixQueueCount > 0)
        {
        memcpy(matrix, _rgKeypadMatrixQueue[_nKeypadMatrixQueueHead], TWL_KEYPAD_ROWS);
        _nKeypadMatrixQueueHead = (_nKeypadMatrixQueueHead + 1) % MATRIX_QUEUE_SIZE;
        
        InterlockedDecrement(&_nKeypadMatrixQueueCount);

        // indicate more data is available
        if (_nKeypadMatrixQueueCount > 0) SetEvent(hKeypadEvent);
        return TRUE;
        }
    
    // return number of items in the queue
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  SendSingularPBMessage
//
//  Initializes T2 power.
//
BOOL 
SendPBMessage(
    Device_t   *pDevice,
    UCHAR power_res_id,UCHAR res_state
    )
{
    UINT16 pb_message;
    UINT8 data[5];
    
    data[0] = 0x02; // Enable I2C access to the Power Bus 
    if(!WriteRegs(pDevice, TWL_PB_CFG, data, 1))
        goto cleanUp;
    
    // Form the message for VDAC 
    pb_message = TwlTargetMessage(TWL_PROCESSOR_GRP1, power_res_id, res_state);
    
    // Extract the Message MSB 
    data[0] = (UINT8) (pb_message >> 8);
    if(!WriteRegs(pDevice, TWL_PB_WORD_MSB, data, 1))
        goto cleanUp;
    
    // Extract the Message LSB 
    data[0] = (UINT8) (pb_message & 0x00FF);
    if(!WriteRegs(pDevice, TWL_PB_WORD_LSB, data, 1))
        goto cleanUp;
    
    return (TRUE);
    
cleanUp:
    return FALSE;    
}


//------------------------------------------------------------------------------
//
//  Function:  InitializeT2
//
//  Initializes T2.
//
BOOL
InitializeHardware(
    Device_t *pDevice
    )
{
    // Enable LDO for T2 PHY
    SendPBMessage(pDevice, TWL_VUSB_3V1_RES_ID, TWL_RES_ACTIVE);
    SendPBMessage(pDevice, TWL_VUSB_1V5_RES_ID, TWL_RES_ACTIVE);
    SendPBMessage(pDevice, TWL_VUSB_1V8_RES_ID, TWL_RES_ACTIVE);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ProcessSecondaryInterrupts
//
//  generic routine which processes the n.
//
BOOL
ProcessSecondaryInterrupts_USB(
    Device_t         *pDevice,
    HANDLE           *rgEvents,
    SIHEntry_t const *pSIHEntry
    )
{    
    // PIH and SIH will be cleared by reading the LATCH registers.
    // Since USB driver will handle all the possible interrupts,
    // the interrupts bits are passed to usb driver by event data.
    BYTE data[] = {0, 0, 0, 0, 0, 0};

    UNREFERENCED_PARAMETER(pSIHEntry);

    // Debounce for 5 ms
    Sleep(5);
    DWORD ed = 0;

    // Clear all intrrupts
    ReadRegs(pDevice, TWL_USB_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_OTHER_INT_LATCH, &data[1], 1);
    ReadRegs(pDevice, TWL_CARKIT_INT_LATCH, &data[2], 1);
    ReadRegs(pDevice, TWL_CARKIT_SM_1_INT_LATCH, &data[3], 1);
    ReadRegs(pDevice, TWL_CARKIT_SM_2_INT_LATCH, &data[4], 1);
    ReadRegs(pDevice, TWL_REG_CTRL_ERROR, &data[5], 1);

    //RETAILMSG(1, (TEXT("ProcessSecondaryInterrupts USB_INT (0x%x), OTHER_INT (0x%x)\r\n"),
    //    data[0], data[1]));
    // Currently only USB and OTHER intrs are handled
    ed = (data[1] << 8) + data[0];
    if ((*rgEvents != NULL) && (*rgEvents != INVALID_HANDLE_VALUE))
        {
        SetEventData(*rgEvents, ed);
        SetEvent(*rgEvents);
        }
    else
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ProcessSecondaryInterrupts_USB: "
            L"Invalid event handle."
            ));
        }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  ProcessSecondaryInterrupts_Keypad
//
//  generic routine which processes the n.
//
BOOL
ProcessSecondaryInterrupts_Keypad(
    Device_t         *pDevice,
    HANDLE           *rgEvents,
    SIHEntry_t const *pSIHEntry
    )
{
    UINT8 status;
    UINT8 matrix[TWL_KEYPAD_ROWS];
    
    // loop through all status registers for the secondary interrupt handler
    StatusRegister_t const* pStatusRegister = &pSIHEntry->StatusRegisters[0];
    
    // get status of secondary interrupt
    if (ReadRegs(pDevice, pStatusRegister->statusSubaddress, &status, 
        sizeof(status)) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: ProcessSecondaryInterrupts: "
            L"Failed to read status register(0x%08X)",
            pStatusRegister->statusSubaddress
            ));
        return FALSE;
        };

    DEBUGMSG(ZONE_INFO | ZONE_IST, (L"INFO: ProcessSecondaryInterrupts "
        L"secondary status=0x%02X; tick=%d\r\n", status, GetTickCount()
        ));

    // read TWL_KEYPAD_ROWS amount of rows...
    ReadRegs(pDevice, TWL_FULL_CODE_7_0, matrix, TWL_KEYPAD_ROWS);
    WriteKeypadMatrixQueue(matrix, *rgEvents);
    WriteRegs(pDevice, pStatusRegister->statusSubaddress, &status, sizeof(status));

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  ProcessSecondaryInterrupts
//
//  generic routine which processes the n.
//
BOOL
ProcessSecondaryInterrupts(
    Device_t         *pDevice,
    HANDLE           *rgEvents,
    SIHEntry_t const *pSIHEntry
    )
{
    UINT8 status;
    UINT8 clearMask;    
    int interruptCount;
    int statusRegisterIndex = 0;
    StatusRegister_t const* pStatusRegister;
    
    // loop through all status registers for the secondary interrupt handler
    pStatusRegister = &pSIHEntry->StatusRegisters[statusRegisterIndex];
    while (pStatusRegister->interruptCount > 0)
        {
        // get status of secondary interrupt
        if (ReadRegs(pDevice, pStatusRegister->statusSubaddress, &status, 
            sizeof(status)) == FALSE)
            {
            RETAILMSG(ZONE_ERROR, (L"ERROR: ProcessSecondaryInterrupts: "
                L"Failed to read status register(0x%08X)",
                pStatusRegister->statusSubaddress
                ));
            break;
            };

        DEBUGMSG(ZONE_INFO | ZONE_IST, (L"INFO: ProcessSecondaryInterrupts "
            L"secondary status=0x%02X; tick=%d\r\n", status, GetTickCount()
            ));

        // loop through and signal relevant events
        clearMask = status;
        interruptCount = pStatusRegister->interruptCount;
        while (interruptCount-- && status != 0)
        {
            if ((status & 0x01) && *rgEvents)
            {
                SetEvent(*rgEvents);
            }
            status >>= 1;
            ++rgEvents;
        }
        
        // clear secondary interrupt handler for more events
        if(clearMask)
            {
            WriteRegs(pDevice, pStatusRegister->statusSubaddress, 
                &clearMask, sizeof(clearMask)
                );
            }

        // next status register
        ++statusRegisterIndex;
        pStatusRegister = &pSIHEntry->StatusRegisters[statusRegisterIndex];
        }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  SetInterruptMode
//
//  enables/disables triton interrupts based on the requested mode
//
BOOL
SetInterruptMode(
    Device_t               *pDevice,
    InterruptMode_e         intrMode
    )
{  
    int i = 0;
    int offset = 0;
    UINT8 mask = 0xFF;
    StatusRegister_t const *pStatusReg;
    SIHEntry_t const *pEntry = s_pSIHEntries[0];
    switch (intrMode)
        {
        case kTritonIntrEnabled:
            do
                {
                pStatusReg = pEntry->StatusRegisters;

                // For USB, there are separate set/clr registers.
                // So we need to make sure all interrupts which
                // aren't part of the interrupts being enabled are
                // in fact disabled before we start enabling interrupts
                // which need to be enabled
                if (pEntry->ctrlSubaddress == 0)
                    {
                    while (pStatusReg->interruptCount)
                        {
                        mask = 0xFF & ~(pStatusReg->ffEnable);
                        WriteRegs(pDevice, pStatusReg->maskSubaddress + 2,
                            &mask, sizeof(UINT8));
                        pStatusReg++;
                        }

                    // point back to beginning of status register array
                    // to start enabling interrupts
                    pStatusReg = pEntry->StatusRegisters;
                    }

                // USB has specific set/clr registers
                offset = (pEntry->ctrlSubaddress == 0) ? 1 : 0; 
                while (pStatusReg->interruptCount)
                    {
                    WriteRegs(pDevice, pStatusReg->maskSubaddress + offset, 
                        &pStatusReg->ffEnable, sizeof(UINT8)
                        );
                    pStatusReg++;
                    }
                pEntry = s_pSIHEntries[++i];
                }
                
                while(pEntry);        
            break;
            
        case kTritonIntrWakeup:
            do
                {
                pStatusReg = pEntry->StatusRegisters;

                // For USB, there are separate set/clr registers.
                // So we need to make sure all interrupts which
                // aren't part of the wakeup interrupts are
                // in fact disabled before we start enabling wakeup
                // interrupts 
                if (pEntry->ctrlSubaddress == 0)
                    {
                    while (pStatusReg->interruptCount)
                        {
                        // first clear interrupts which isn't part of wakeup
                        mask = 0xFF & ~(pStatusReg->ffWakeupEnable);
                        WriteRegs(pDevice, pStatusReg->maskSubaddress + 2,
                            &mask, sizeof(UINT8));
                        pStatusReg++;
                        }

                    // point back to beginning of status register array
                    // to start enabling wakeup interrupts
                    pStatusReg = pEntry->StatusRegisters;
                    }

                // USB has specific set/clr registers
                offset = (pEntry->ctrlSubaddress == 0) ? 1 : 0;
                while (pStatusReg->interruptCount)
                    {
                    DEBUGMSG(ZONE_INFO, (L"TWL:Setting Interrupt mode: "
                        L"0x%02X, 0x%02X\r\n", 
                        pStatusReg->maskSubaddress, pStatusReg->ffWakeupEnable
                        ));
                    
                    WriteRegs(pDevice, pStatusReg->maskSubaddress + offset, 
                        &pStatusReg->ffWakeupEnable, sizeof(UINT8)
                        );
                    pStatusReg++;
                    }
                
                pEntry = s_pSIHEntries[++i];
                }
                while(pEntry);
            break;

        case kTritonIntrDisabled:
            do
                {
                pStatusReg = pEntry->StatusRegisters;
                
                // USB has specific set/clr registers
                offset = (pEntry->ctrlSubaddress == 0) ? 2 : 0;
                while (pStatusReg->interruptCount)
                    {
                    WriteRegs(pDevice, pStatusReg->maskSubaddress + offset, 
                        &mask, sizeof(UINT8)
                        );
                    pStatusReg++;
                    }
                pEntry = s_pSIHEntries[++i];
                }
                while(pEntry);
            break;
        }
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  TWL_Init
//
//  Called by device manager to initialize device.
//
DWORD
TWL_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = NULL;

    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+TWL_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed allocate TWL controller structure\r\n"
            ));
        goto cleanUp;
        }

    // clear memory
    memset(pDevice, 0, sizeof(Device_t));

    // Set cookie and initial power state
    pDevice->cookie = TWL_DEVICE_COOKIE;
    pDevice->powerState = D0;

    // Initalize critical section
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed read TWL driver registry parameters\r\n"
            ));
        goto cleanUp;
        }

    // Open i2c bus
    pDevice->hI2C = I2COpen(BSPGetTritonBusID());
    if (pDevice->hI2C == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed open I2C bus driver\r\n"
            ));
        goto cleanUp;
        }
    I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress());

    // Open gpio driver
    pDevice->hGpio = GPIOOpen();
    if (pDevice->hGpio == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed to open Gpio driver \r\n"
            ));
        goto cleanUp;
        }

    InitializeHardware(pDevice);   

    // initialze callback arrays
    s_pSIHRoutines[0] = ProcessSecondaryInterrupts;
    s_pSIHRoutines[1] = ProcessSecondaryInterrupts_Keypad;
    s_pSIHRoutines[2] = ProcessSecondaryInterrupts;
    s_pSIHRoutines[3] = ProcessSecondaryInterrupts;
    s_pSIHRoutines[4] = ProcessSecondaryInterrupts_USB;
    s_pSIHRoutines[5] = ProcessSecondaryInterrupts;

    // set triton interrupts to initial settings
    if (!InitializeInterrupts(pDevice))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed initialize triton interrupts\r\n"
            ));
        goto cleanUp;
        }

    // NOTE:
    //  Triton generates a low level interrupt.
    GPIOSetMode(pDevice->hGpio, pDevice->gpio, GPIO_DIR_INPUT | GPIO_INT_LOW);

    //---------------------------------------------------------------------
    
    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed create interrupt event\r\n"
            ));
        goto cleanUp;
        }

    // Initialize interrupt
    if (!GPIOInterruptInitialize(pDevice->hGpio, pDevice->gpio, &pDevice->sysIntr, pDevice->hIntrEvent))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"InterruptInitialize failed\r\n"
            ));
        goto cleanUp;
        }

    // Start interrupt service thread
    pDevice->intrThreadExit = FALSE;
    pDevice->hIntrThread = CreateThread(
                                NULL, 0, TWL_InterruptThread, pDevice, 0,NULL
                                );
    if (!pDevice->hIntrThread)
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed create interrupt thread\r\n"
            ));
        goto cleanUp;
        }

    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256);

    // Return non-null value
    rc = (DWORD)pDevice;
    
cleanUp:
    if (rc == 0) TWL_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
TWL_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;


    DEBUGMSG(ZONE_FUNCTION, (L"+TWL_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Close interrupt thread
    if (pDevice->hIntrThread != NULL)
        {
        // Signal stop to thread
        pDevice->intrThreadExit = TRUE;
        // Set event to wake it
        SetEvent(pDevice->hIntrEvent);
        // Wait until thread exits
        WaitForSingleObject(pDevice->hIntrThread, INFINITE);
        // Close handle
        CloseHandle(pDevice->hIntrThread);
        }

    // Disable interrupt
    if (pDevice->sysIntr != 0)
        {
        GPIOInterruptDisable(pDevice->hGpio, pDevice->gpio, pDevice->sysIntr);
        }

    // Close interrupt handler
    if (pDevice->hIntrEvent != NULL) CloseHandle(pDevice->hIntrEvent);

    // Close I2C bus
    if (pDevice->hI2C != NULL) I2CClose(pDevice->hI2C);

    // close Gpio 
    if (pDevice->hGpio != NULL) GPIOClose(pDevice->hGpio);

    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
TWL_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    Instance_t *pInstance = (Instance_t*)LocalAlloc(LPTR, sizeof(Instance_t));

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    if (pInstance == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Open: "
            L"Failed allocate TWL instance structure\r\n"
            ));
        return NULL;
    }

    pInstance->cookie = TWL_INSTANCE_COOKIE;
    pInstance->pDevice = (Device_t*)context;
    pInstance->address = 0;

    return (DWORD)pInstance;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function closes the device context.
//
BOOL
TWL_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance != NULL && pInstance->cookie == TWL_INSTANCE_COOKIE)
        {
        LocalFree(pInstance);
        rc = TRUE;
        }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Read
//
//  This function closes the device context.
//
DWORD
TWL_Read(
    DWORD   context,
    LPVOID  pBuffer,
    DWORD   count
    )
{
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        count = 0;
        goto cleanUp;
        }

    count = TWL_ReadRegs(context, pInstance->address, pBuffer, count);

cleanUp:
    return count;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Write
//
//  This function closes the device context.
//
DWORD
TWL_Write(
    DWORD   context,
    LPCVOID pBuffer,
    DWORD   count
    )
{
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        count = 0;
        goto cleanUp;
        }

    if (TWL_WriteRegs(context, pInstance->address, pBuffer, count) == FALSE)
        {
        count = 0;
        }

cleanUp:
    return count;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Seek
//
//  This function closes the device context.
//
BOOL
TWL_Seek(
    DWORD context,
    long  amount,
    WORD  type
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;

    UNREFERENCED_PARAMETER(type);

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        goto cleanUp;
        }

    pInstance->address = amount;
    
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_PowerUp
//
//  This function wakes-up driver from power down.
//
void
TWL_PowerUp(
    DWORD context
    )
{
    Device_t *pDevice = (Device_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        return;
        }
    
    GPIOSetMode(pDevice->hGpio, pDevice->gpio, GPIO_DIR_INPUT | GPIO_INT_LOW);
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function prepares driver to power down.
//
void
TWL_PowerDown(
    DWORD context
    )
{
    Device_t *pDevice = (Device_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        return;
        }

    // OMAP only wakes-up from edge detections but we enable both edge and 
    // level to make sure we don't drop an interrupt
    GPIOSetMode(pDevice->hGpio, pDevice->gpio, 
        GPIO_DIR_INPUT | GPIO_INT_HIGH_LOW | GPIO_INT_LOW
        );
}

//------------------------------------------------------------------------------

static
BOOL
TWL_ReadRegs(
    DWORD context, 
    DWORD address,
    void *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;
    UINT8 matrix[TWL_KEYPAD_ROWS];
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_ReadRegs: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (size > 0xFF) goto cleanUp;

    // check for cached keypad state values
    if (address == TWL_LOGADDR_FULL_CODE_7_0)
        {
        // if null buffer or zero count return back number of bytes
        // in queue which needs to be read out
        if (pBuffer == NULL)
            {
            rc = _nKeypadMatrixQueueCount;
            goto cleanUp;
            }

        // get cached keypad scan keys from internal queue
        size = min(size, TWL_KEYPAD_ROWS);
        if (ReadKeypadMatrixQueue(matrix, GetFirstEventByGroup(pDevice, 1)) == FALSE)
            {
            // read TWL_KEYPAD_ROWS amount of rows...
            memset(matrix, 0, sizeof(matrix));
            }
        memcpy(pBuffer, matrix, size);
        rc = TRUE;
        }
    else
        {
        rc = ReadRegs(pDevice, address, pBuffer, size);
        }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static
BOOL
TWL_WriteRegs(
    DWORD context, 
    DWORD address,
    const void *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_WriteRegs: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (size > 0xFF) goto cleanUp;
    if (address & 0xFF000000) goto cleanUp;

    rc = WriteRegs(pDevice, address, pBuffer, size);
    
cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  TWL_InterruptThread
//
//  This function acts as the IST for the keypad.
//
DWORD
TWL_InterruptThread(
    VOID *pContext
    )
{
    Device_t *pDevice = (Device_t*)pContext;
    DWORD timeout = INFINITE;
    UINT8 status;
    DWORD id;
    HANDLE *pEvents = NULL;


    // Loop until we are not stopped...
    while (!pDevice->intrThreadExit)
        {
        // Wait for event
        WaitForSingleObject(pDevice->hIntrEvent, timeout);
        if (pDevice->intrThreadExit) break;

        // NOTE:
        //   We can't encapsulate this here since the clients may
        // call into the triton driver causing a deadlock
        //
        //EnterCriticalSection(&pDevice->cs);
        // Get and clear interrupt status register
        ReadRegs(pDevice, TWL_PIH_ISR_P1, &status, sizeof(status));

        DEBUGMSG(ZONE_IST, (L"INFO: TWL_InterruptThread "
            L"Triton2 interrupt status=0x%02X\r\n", status
            ));

        // Process each interrupt
        id = 0;
        status &= 0x3F;
        while (status != 0)
            {
            if ((status & 0x1) != 0)
                {
                pEvents = GetFirstEventByGroup(pDevice, id);
                s_pSIHRoutines[id](pDevice, pEvents, s_pSIHEntries[id]); 
                }
            status >>= 1;
            id++;
            }
        

        //LeaveCriticalSection(&pDevice->cs);
        
        GPIOInterruptDone(pDevice->hGpio, pDevice->gpio, pDevice->sysIntr);
        }

    return ERROR_SUCCESS;
}


//------------------------------------------------------------------------------

static
BOOL 
TWL_InterruptMask(
    DWORD context, 
    DWORD intrId,
    BOOL  bEnable
    )
{
    BOOL rc = FALSE;
    BOOL bLocked = FALSE;    
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
    UINT8 mask;
    SIHEntry_t const *pSIHEntry;
    StatusRegister_t const *pStatusRegister;
    int nArrayIndex;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IntrEnable: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (TWL_ARRAYINDEX(intrId) >= TWL_MAX_INTR)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptMask: "
            L"Incorrect interrupt Id %d\r\n", intrId
            ));
        goto cleanUp;
        }

    // We have take critical section there to avoid concurrent
    // enable register modification
    bLocked = TRUE;
    EnterCriticalSection(&pDevice->cs);

    // get secondary interrupt handler info
    nArrayIndex = TWL_SIHINDEX(intrId);
    if (nArrayIndex >= MAX_SIH_COUNT)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptMask: "
            L"Invalid SIH index %d\r\n", nArrayIndex
            ));
        goto cleanUp;
        }
    
    pSIHEntry = s_pSIHEntries[nArrayIndex];

    // get mask register value
    pStatusRegister = pSIHEntry->StatusRegisters + TWL_REGISTERINDEX(intrId);
   
    int delta = 0;
    // Enable interrupt
    // USB register is an enable register
    if (SIHEntry_USB == TWL_SIHINDEX(intrId))
    {
        mask = (UINT8) ((1 << TWL_MASKBIT(intrId)));
        // Move to the Set address;
        delta = (bEnable == TRUE) ? 2 : 1;
    }
    else
    {
        // Get actual mask
        if (!ReadRegs(pDevice, pStatusRegister->maskSubaddress, 
            &mask, sizeof(mask)))
            {
            goto cleanUp;
            }
        if (bEnable == TRUE)
            {
            mask |= (1 << TWL_MASKBIT(intrId));
            }
        else
            {
            mask &= ~(1 << TWL_MASKBIT(intrId));
            }
        ((StatusRegister_t*)pStatusRegister)->ffEnable = mask;
    }
    
    // Write it back
    if (!WriteRegs(pDevice, pStatusRegister->maskSubaddress + delta, 
        &mask, sizeof(mask)))
        {        
        goto cleanUp;
        }
    
    rc = TRUE;
    
cleanUp:
    if (bLocked == TRUE) LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------

static
BOOL 
TWL_InterruptSetEvent(
    DWORD context, 
    DWORD intrId,
    HANDLE hEvent
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
    int nArrayIndex;

    // We have take critical section there to avoid concurrent
    // enable register modification
    EnterCriticalSection(&pDevice->cs);


    nArrayIndex = TWL_ARRAYINDEX(intrId);

    if ((nArrayIndex > TWL_MAX_INTR))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptSetEvent: "
            L"Incorrect interrupt Id 0x%08X\r\n", intrId
            ));
        goto cleanUp;
        }
    
    // Special caes.  If keypad interrupt clear cached
    // states
    if (intrId == TWL_INTR_ITKPI)
    {
        _nKeypadMatrixQueueCount = 0;
        _nKeypadMatrixQueueHead = 0;
        _nKeypadMatrixQueueTail = 0;
    }

    // If handle isn't NULL we set new association, 
    // otherwise we delete it....
    if (hEvent != NULL)
    {
        if (pDevice->hSetIntrEvent[nArrayIndex] != NULL)
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptSetEvent: "
                L"Interrupt Id %d already associated with event\r\n"
                ));
            goto cleanUp;
            }
        rc = DuplicateHandle(
            GetCurrentProcess(), hEvent, GetCurrentProcess(),
            &pDevice->hSetIntrEvent[nArrayIndex], 0, FALSE, 
            DUPLICATE_SAME_ACCESS
            );
        if (!rc)
        {        
            DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptSetEvent: "
                L"Event handler duplication failed\r\n"
                ));
        goto cleanUp;
        }
        }
    else
    {
        if (pDevice->hSetIntrEvent[nArrayIndex] == NULL)
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_InterruptSetEvent: "
                L"Interrupt Id 0x%08X isn't associated with event\r\n",
                intrId
                ));
            goto cleanUp;
            }
        rc = CloseHandle(pDevice->hSetIntrEvent[nArrayIndex]);
        pDevice->hSetIntrEvent[nArrayIndex] = NULL;
        TWL_InterruptMask(context, intrId, TRUE);
    }
    
cleanUp:
    LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------

static
BOOL
TWL_InterruptInitialize(
    DWORD context,
    DWORD intrId,
    HANDLE hEvent
    )
{
    BOOL rc = FALSE;
    
    if (hEvent != NULL)
        {
        rc = TWL_InterruptSetEvent(context, intrId, hEvent);
        }

    return rc;
}

//------------------------------------------------------------------------------

static
BOOL 
TWL_InterruptDisable(
    DWORD context, 
    DWORD intrId
    )
        {
    return TWL_InterruptSetEvent(context, intrId, NULL);
        }

//------------------------------------------------------------------------------

static
BOOL
TWL_InterruptInitializeProxy(
    DWORD context,
    UINT  procId,
    DWORD intrId,
    HANDLE hEventOrig
    )
{   
    HANDLE hProc;
    HANDLE hEvent = NULL;
    BOOL rc = FALSE;    
    
    // If handle isn't NULL we set new association, 
    // otherwise we delete it....
    if (hEventOrig != NULL)
        {
        hProc = OpenProcess(0, FALSE, procId);
        if (hProc == NULL)
            {
            RETAILMSG(ZONE_ERROR, (L"ERROR! TWL_InterruptInitializeProxy: "
                L"Unable to get process handle (err=0x%08X)\r\n",
                GetLastError()
                ));
            goto cleanUp;
            }
        
        // create a duplicate handle
        rc = DuplicateHandle(
            hProc, hEventOrig, GetCurrentProcess(),
            &hEvent, 0, FALSE, 
            DUPLICATE_SAME_ACCESS
            );

        // release resources
        CloseHandle(hProc);
        if (rc == FALSE)
        {
            RETAILMSG(ZONE_ERROR, (L"ERROR! TWL_InterruptInitializeProxy: "
                L"Unable to duplicate handle (err=0x%08X)\r\n",
                GetLastError()
                ));
            goto cleanUp;
            }
        }
            
    // call local routine to map event with interrupt
    rc = TWL_InterruptInitialize(context, intrId, hEvent);
    
    // release this handle
    CloseHandle(hEvent);
            
cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
BOOL
TWL_EnableWakeup(
    DWORD context, 
    DWORD intrId, 
    BOOL bEnable
    )
{
    BOOL rc = FALSE;
    BOOL bLocked = FALSE;    
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
    SIHEntry_t const *pSIHEntry;
    StatusRegister_t const *pStatusRegister;
    int nArrayIndex;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IntrEnable: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (TWL_ARRAYINDEX(intrId) >= TWL_MAX_INTR)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_EnableWakeup: "
            L"Incorrect interrupt Id %d\r\n", intrId
            ));
        goto cleanUp;
        }

    // We have take critical section there to avoid concurrent
    // enable register modification
    bLocked = TRUE;
    EnterCriticalSection(&pDevice->cs);

    // get secondary interrupt handler info
    nArrayIndex = TWL_SIHINDEX(intrId);
    if (nArrayIndex > MAX_SIH_COUNT)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_EnableWakeup: "
            L"Invalid SIH index %d\r\n", nArrayIndex
            ));
        goto cleanUp;
        }
    
    pSIHEntry = s_pSIHEntries[nArrayIndex];

    // get mask register value
    pStatusRegister = pSIHEntry->StatusRegisters + TWL_REGISTERINDEX(intrId);

    // NOTE:
    //  The bit must be cleared to indicate the interrupt is unmasked
    // set enable/disable mask only if the request doesn't match the current state
    if (!!((pStatusRegister->ffWakeupEnable) & (1 << TWL_MASKBIT(intrId))) != !bEnable)
        {
        if (bEnable)
            {
            ((StatusRegister_t*)pStatusRegister)->ffWakeupEnable &= 
                                            ~(1 << TWL_MASKBIT(intrId));
            pDevice->wakeupCount++;
            }
        else
            {
            ((StatusRegister_t*)pStatusRegister)->ffWakeupEnable |= 
                                            (1 << TWL_MASKBIT(intrId));
            pDevice->wakeupCount--;
            }

        // (un)register as wakeup interrupt, as necessary
        if (pDevice->wakeupCount == 1)
            {
                if (!GPIOInterruptWakeUp(pDevice->hGpio, pDevice->gpio, pDevice->sysIntr,TRUE))
                {
                DEBUGMSG(ZONE_ERROR, (L"WARN: TWL_EnableWakeup: "
                    L"Failed enable as wakeup source\r\n"
                    ));
                }
            }
        else if (pDevice->wakeupCount == 0)
            {
            if (!GPIOInterruptWakeUp(pDevice->hGpio, pDevice->gpio, pDevice->sysIntr,FALSE))
                {
                DEBUGMSG(ZONE_ERROR, (L"WARN: TWL_EnableWakeup: "
                    L"Failed to disable as wakeup source\r\n"
                    ));
                }
            }
        }
    
    rc = TRUE;
    
cleanUp:
    if (bLocked == TRUE) LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  TWL_IOControl
//
//  This function sends a command to a device.
//
BOOL
TWL_IOControl(
    DWORD context, 
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"+TWL_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));
    
    BOOL rc = FALSE;
    const void* pBuffer;
    Device_t *pDevice;
    DEVICE_IFC_TWL ifc;
    DWORD address, size;
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        goto cleanUp;
        }

    pDevice = pInstance->pDevice;


    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_DDK_GET_DRIVER_IFC:
            // We can give interface only to our peer in device process
            if (GetCurrentProcessId() != (DWORD)GetCallerProcess())
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                    ));
                SetLastError(ERROR_ACCESS_DENIED);
                break;
                }
            // Check input parameters
            if ((pInBuffer == NULL) || (inSize < sizeof(GUID)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_TWL_GUID))
                {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_TWL);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_TWL))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                ifc.context = context;
                ifc.pfnReadRegs = TWL_ReadRegs;
                ifc.pfnWriteRegs = TWL_WriteRegs;
                ifc.pfnInterruptInitialize = TWL_InterruptInitialize;
                ifc.pfnInterruptMask = TWL_InterruptMask;
                ifc.pfnInterruptDisable = TWL_InterruptDisable;
                ifc.pfnEnableWakeup = TWL_EnableWakeup;
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_TWL)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
            
        case IOCTL_TWL_READREGS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(IOCTL_TWL_READREGS_IN)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            address = ((IOCTL_TWL_READREGS_IN*)pInBuffer)->address;
            size = ((IOCTL_TWL_READREGS_IN*)pInBuffer)->size;
            if (pOutSize != NULL) *pOutSize = size;
            if ((pOutBuffer == NULL) || (outSize < size))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            rc = TWL_ReadRegs(context, address, pOutBuffer, size);

        case IOCTL_TWL_WRITEREGS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(IOCTL_TWL_WRITEREGS_IN)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            address = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->address;
            pBuffer = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->pBuffer;
            size = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->size;
            /*
            if (inSize < (sizeof(IOCTL_TWL_WRITEREGS_IN) + size))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            */	
            rc = TWL_WriteRegs(context, address, pBuffer, size);
            break;

        case IOCTL_POWER_CAPABILITIES: 
            DEBUGMSG(ZONE_INFO, (L"TWL: Received IOCTL_POWER_CAPABILITIES\r\n"));
            if (pOutBuffer && outSize >= sizeof (POWER_CAPABILITIES) && 
                pOutSize) 
                {
                    __try 
                        {
                        PPOWER_CAPABILITIES PowerCaps;
                        PowerCaps = (PPOWER_CAPABILITIES)pOutBuffer;
         
                        // Only supports D0 (permanently on) and D4(off.         
                        memset(PowerCaps, 0, sizeof(*PowerCaps));
                        PowerCaps->DeviceDx = (DX_MASK(D0) | 
                                               DX_MASK(D2) | 
                                               DX_MASK(D3) | 
                                               DX_MASK(D4));
                        *pOutSize = sizeof(*PowerCaps);                        
                        rc = TRUE;
                        }
                    __except(EXCEPTION_EXECUTE_HANDLER) 
                        {
                        RETAILMSG(ZONE_ERROR, (L"exception in ioctl\r\n"));
                        }
                }
            break;
#if (_WINCEOSVER<700)
        // deprecated
        case IOCTL_POWER_QUERY: 
            rc = TRUE;
            DEBUGMSG(ZONE_INFO,(L"TWL: Received IOCTL_POWER_QUERY\r\n"));
            break;
#endif
        // requests a change from one device power state to another
        case IOCTL_POWER_SET: 
            DEBUGMSG(ZONE_INFO,(L"TWL: Received IOCTL_POWER_SET\r\n"));
            if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pOutBuffer;
                    switch (ReqDx)
                        {
                        case D0:
                            SetInterruptMode(pDevice, kTritonIntrEnabled);
                            pDevice->powerState = D0;
                            break;
                            
                        case D1:
                        case D2:
                            SetInterruptMode(pDevice, kTritonIntrEnabled);
                            pDevice->powerState = D2;
                            break;

                        case D3:
                            SetInterruptMode(pDevice, kTritonIntrWakeup);
                            pDevice->powerState = D3;
                            break;

                        case D4:
                            SetInterruptMode(pDevice, kTritonIntrDisabled);
                            pDevice->powerState = D4;
                            break;
                        }
                    *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
                    *pOutSize = sizeof(CEDEVICE_POWER_STATE);
                    DEBUGMSG(ZONE_INFO, (L"TWL: IOCTL_POWER_SET to D%u \r\n",
                        pDevice->powerState
                        ));

                    rc = TRUE;
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
                    }
            }
            break;

        // gets the current device power state
        case IOCTL_POWER_GET: 
            DEBUGMSG(ZONE_INFO, (L"TWL: Received IOCTL_POWER_GET\r\n"));
            if (pOutBuffer != NULL && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
 
                    rc = TRUE;

                    DEBUGMSG(ZONE_INFO, (L"TWL: "
                            L"IOCTL_POWER_GET to D%u \r\n",
                            pDevice->powerState
                            ));
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
                    }
                }     
            break;

        case IOCTL_TWL_INTRINIT:
            if (pInBuffer != NULL && inSize == sizeof(IOCTL_TWL_INTRINIT_IN))
                {
                IOCTL_TWL_INTRINIT_IN *pInParam = (IOCTL_TWL_INTRINIT_IN*)pInBuffer;
                rc = TWL_InterruptInitializeProxy(context, pInParam->procId, pInParam->intrId, pInParam->hEvent);
                }
            break;

        case IOCTL_TWL_WAKEENABLE:
            if (pInBuffer != NULL && inSize == sizeof(IOCTL_TWL_WAKEENABLE_IN))
                {
                IOCTL_TWL_WAKEENABLE_IN *pInParam = (IOCTL_TWL_WAKEENABLE_IN*)pInBuffer;
                rc = TWL_EnableWakeup(context, pInParam->intrId, pInParam->bEnable);
                }
            break;

        case IOCTL_TWL_INTRMASK:
            if (pInBuffer != NULL && inSize == sizeof(IOCTL_TWL_INTRMASK_IN))
                {
                IOCTL_TWL_INTRMASK_IN *pInParam = (IOCTL_TWL_INTRMASK_IN*)pInBuffer;
                rc = TWL_InterruptMask(context, pInParam->procId, pInParam->bEnable);
                }
            break;

        case IOCTL_TWL_INTRDISABLE:
            if (pInBuffer != NULL && inSize == sizeof(DWORD))
                {
                rc = TWL_InterruptDisable(context, *(DWORD*)pInBuffer);
                }
            break;
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  InitializeInterrupts
//
//  initializes all the interrupts; edge detect, fall, rise, etc.
//
BOOL
InitializeInterrupts(
    Device_t   *pDevice
    )
{
    BOOL rc = TRUE;
    UINT8 data[5];

    // mask all interrupts
    data[0] = 0xFF;
    WriteRegs(pDevice, TWL_PWR_IMR1, data, 1);
    WriteRegs(pDevice, TWL_BCIIMR1A, data, 1);
    WriteRegs(pDevice, TWL_BCIIMR2A, data, 1);
    WriteRegs(pDevice, TWL_MADC_IMR1, data, 1);    
    WriteRegs(pDevice, TWL_KEYP_IMR1, data, 1);
    WriteRegs(pDevice, TWL_GPIO_IMR1A, data, 1);
    WriteRegs(pDevice, TWL_GPIO_IMR2A, data, 1);
    WriteRegs(pDevice, TWL_GPIO_IMR3A, data, 1);

    // Clear all USB interrupts
    ReadRegs(pDevice, TWL_USB_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_OTHER_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_CARKIT_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_CARKIT_SM_1_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_CARKIT_SM_2_INT_LATCH, &data[0], 1);
    ReadRegs(pDevice, TWL_REG_CTRL_ERROR, &data[0], 1);

    // Disable all USB interrupts
    data[0] = 0xFF;
    WriteRegs(pDevice, TWL_USB_INT_EN_FALL_CLR, data, 1);
    WriteRegs(pDevice, TWL_USB_INT_EN_RISE_CLR, data, 1);
    WriteRegs(pDevice, TWL_OTHER_INT_EN_RISE_CLR, data, 1);
    WriteRegs(pDevice, TWL_OTHER_INT_EN_FALL_CLR, data, 1);
    WriteRegs(pDevice, TWL_CARKIT_INT_EN_CLR, data, 1);
    WriteRegs(pDevice, TWL_CARKIT_SM_1_INT_EN_CLR, data, 1);
    WriteRegs(pDevice, TWL_CARKIT_SM_2_INT_EN_CLR, data, 1);
    WriteRegs(pDevice, TWL_REG_CTRL_EN_CLR, data, 1);

    // put power and rtc interrupts in exclusive mode
    data[0] = TWL_SIH_CTRL_EXCLEN;
    WriteRegs(pDevice, TWL_PWR_SIH_CTRL, data, 1);

    // set edge interrupt and ctrl for power

    // RTC_IT_RISING
    // USB_PRES_RISING | USB_PRES_FALLING
    // CHG_PRES_RISING | CHG_PRES_FALLING
    data[0] = TWL_RTC_IT_RISING | TWL_USB_PRES_RISING | TWL_USB_PRES_FALLING |
              TWL_CHG_PRES_RISING | TWL_CHG_PRES_FALLING;
    
    WriteRegs(pDevice, TWL_PWR_EDR1, data, 1);

    // set edge interrupt and ctrl for keypad

    // set edge interrupt and ctrl for bci

    // interrupt on both edges for all interrupts
    data[0] = 0xFF;
    data[1] = 0xFF;
    data[2] = 0xFF;
    WriteRegs(pDevice, TWL_BCIEDR1, data, 3);

    // set edge interrupt and ctrl for madc

    // set edge interrupt and ctrl for gpio

    // set interrupts for USB

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

#pragma warning(default:4200)

//------------------------------------------------------------------------------

