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
//  File: TCA6416_gpio.cpp
//
#include "omap.h"
#include "ceddkex.h"
#include "oalex.h"
#include "soc_cfg.h"
#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include <initguid.h>
#include "gpio_ioctls.h"
#include <nkintr.h>

#define IST_TIMEOUT     (5000)

#define NB_PIN_PER_BANK     (8)
#define NB_BANK             (2)

//------------------------------------------------------------------------------
//  Register definitons

#define INPUT_PORT_0        (0)     //Input Port 0
#define INPUT_PORT_1        (1)     //Input Port 1
#define OUTPUT_PORT_0       (2)     //Output Port 0
#define OUTPUT_PORT_1       (3)     //Output Port 1
#define POLINV_PORT_0       (4)     //Polarity Inversion Port 0
#define POLINV_PORT_1       (5)     //Polarity Inversion Port 1
#define CONFIG_PORT_0       (6)     //Configuration Port 0
#define CONFIG_PORT_1       (7)     //Configuration Port 1

#define INPUT_PORT(x)  (INPUT_PORT_0 + (x))
#define OUTPUT_PORT(x) (OUTPUT_PORT_0 + (x))
#define POLINV_PORT(x) (POLINV_PORT_0 + (x))
#define CONFIG_PORT(x) (CONFIG_PORT_0 + (x))


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
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IST            DEBUGZONE(4)
#ifdef DEVICE
DBGPARAM dpCurSettings = {
    L"TCA6416 GPIO", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
            L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};
#endif
#endif


typedef struct {
    DWORD I2CBusIndex;
    DWORD I2CAddr;
    DWORD I2CBaudrateIndex;
    DWORD IntrGpio;
    HANDLE hI2C;
    HANDLE hGpio;
    DWORD dwSysintr;
    HANDLE hIntrEvent;
    HANDLE hIntrthread;
    BOOL fTerminateThread;
    CRITICAL_SECTION bankCs[NB_BANK];
    CRITICAL_SECTION wakeupStateCs;
    int wakeupStateCounter;
    UCHAR intMask[NB_BANK];
    HANDLE hPinEvent[NB_BANK][NB_PIN_PER_BANK];
    LONG   enabledIntCount;
} TCA6416_DEVICE;

//------------------------------------------------------------------------------
//  Local Functions

// Init function
static BOOL Tca6416GpioInit(LPCTSTR szContext, HANDLE *phContext, UINT *pGpioCount);
static BOOL Tca6416GpioDeinit(HANDLE hContext);
static BOOL Tca6416GpioSetMode(HANDLE hContext, UINT id, UINT mode);
static DWORD Tca6416GpioGetMode(HANDLE hContext, UINT id);
static BOOL Tca6416GpioPullup(HANDLE hcontext, UINT id, UINT enable);
static BOOL Tca6416GpioPulldown(HANDLE hcontext, UINT id, UINT enable);
static DWORD Tca6416GpioGetSystemIrq(HANDLE hcontext, UINT id);
static BOOL Tca6416GpioInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent);
static VOID Tca6416GpioInterruptDone(HANDLE hcontext,DWORD id, DWORD sysintr);
static VOID Tca6416GpioInterruptDisable(HANDLE hcontext,DWORD id, DWORD sysintr);
static VOID Tca6416GpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable);
static BOOL Tca6416GpioInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnable);
static BOOL Tca6416GpioSetBit(HANDLE hContext, UINT id);
static BOOL Tca6416GpioClrBit(HANDLE hContext, UINT id);
static DWORD Tca6416GpioGetBit(HANDLE hContext, UINT id);
#ifdef DEVICE
static void Tca6416GpioPowerUp(HANDLE hContext);
static void Tca6416GpioPowerDown(HANDLE hContext);
#endif
static BOOL Tca6416GpioIoControl(HANDLE hContext, UINT code,
                              UCHAR *pinVal, UINT inSize, UCHAR *poutVal,
                              UINT outSize, DWORD *pOutSize);

//------------------------------------------------------------------------------
//  exported function table
extern "C" DEVICE_IFC_GPIO Tca6416_Gpio;

DEVICE_IFC_GPIO Tca6416_Gpio = {
    0,
    Tca6416GpioInit,
    NULL,
    Tca6416GpioDeinit,
    Tca6416GpioSetBit,   
    Tca6416GpioClrBit,     
    Tca6416GpioGetBit,    
    Tca6416GpioSetMode,
    Tca6416GpioGetMode,
    Tca6416GpioPullup,
    Tca6416GpioPulldown,
    Tca6416GpioIoControl,
    NULL,NULL,NULL,NULL,
};


//------------------------------------------------------------------------------
//  Device registry parameters
static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    {
        L"I2CBus", PARAM_DWORD, TRUE, offset(TCA6416_DEVICE, I2CBusIndex),
        fieldsize(TCA6416_DEVICE, I2CBusIndex), 0
	}, {
        L"I2CAddr", PARAM_DWORD, TRUE, offset(TCA6416_DEVICE, I2CAddr),
        fieldsize(TCA6416_DEVICE, I2CAddr), 0
	}, {
        L"I2CBaudrateIndex", PARAM_DWORD, FALSE, offset(TCA6416_DEVICE, I2CBaudrateIndex),
        fieldsize(TCA6416_DEVICE, I2CBaudrateIndex), (void*)0
    },
    {
        L"IntrGpio", PARAM_DWORD, FALSE, offset(TCA6416_DEVICE, IntrGpio),
        fieldsize(TCA6416_DEVICE, IntrGpio), (PVOID)-1
    }, 
};

void SendExtraSCLCycle(HANDLE hI2C)
{    
    I2CSetManualDriveMode(hI2C,TRUE);
    I2CDriveSCL(hI2C,FALSE);
    I2CDriveSCL(hI2C,TRUE);
    I2CSetManualDriveMode(hI2C,FALSE);    
}

UINT
TCA6416I2CWrite(
    void            *hCtx,
    UINT32          subAddr,
    const VOID      *pBuffer,
    UINT32          size
    )
{
    UINT result = I2CWrite(hCtx,subAddr,pBuffer,size);
    SendExtraSCLCycle(hCtx);
    return result;
}

UINT
TCA6416I2CRead(
    void           *hCtx,
    UINT32          subAddr,
    VOID           *pBuffer,
    UINT32          size
    )
{
    UINT result = I2CRead(hCtx,subAddr,pBuffer,size);
    SendExtraSCLCycle(hCtx);
    return result;
}

DWORD IST(LPVOID lpParam)
{
    int i,j;
    UCHAR currentpinstate[NB_BANK];
    UCHAR oldpinstate[NB_BANK];
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE *)lpParam;
    for (i=0;i<NB_BANK;i++)
    {
        TCA6416I2CRead(pDevice->hI2C,INPUT_PORT(i),&oldpinstate[i],sizeof(oldpinstate[i]));
    }
    while (!pDevice->fTerminateThread)
    {
        if (WaitForSingleObject(pDevice->hIntrEvent,IST_TIMEOUT) == WAIT_TIMEOUT)
        {
            // we haven't detected any interrupt in the last xxx seconds
            // Send a clock cycle on the I2C in case someboday made an I2C transfer that didn't 
            // include this extra clock cycle. If don't do it, then the interrupt may never be triggerred again
            SendExtraSCLCycle(pDevice->hI2C);
            continue;
        }
        if (pDevice->fTerminateThread)
        {
            break;
        }
        
        for (i=0;i<NB_BANK;i++)
        {
            UCHAR diff;
            UCHAR bankstatus;
            TCA6416I2CRead(pDevice->hI2C,INPUT_PORT(i),&currentpinstate[i],sizeof(currentpinstate[i]));
            EnterCriticalSection(&pDevice->bankCs[i]);
            diff = currentpinstate[i] ^ oldpinstate[i];     
            DEBUGMSG(ZONE_IST,(TEXT("bank %d : 0%x 0%x 0x%x\r\n"),i,currentpinstate[i],oldpinstate[i],diff));
            bankstatus = diff & pDevice->intMask[i];
            for (j=0;j<NB_PIN_PER_BANK;j++)
            {
                
                if (bankstatus & (1<<j))
                {
                    pDevice->intMask[i] &= (UCHAR) ~(1<<j);
                    pDevice->enabledIntCount--;
                    if (pDevice->enabledIntCount == 0)
                    {
                        GPIOInterruptMask(pDevice->hGpio, pDevice->IntrGpio,pDevice->dwSysintr,TRUE);
                    }
                    if (pDevice->hPinEvent[i][j])
                    {
                        SetEvent(pDevice->hPinEvent[i][j]);
                    }
                }
            }
            oldpinstate[i] = currentpinstate[i];
            LeaveCriticalSection(&pDevice->bankCs[i]);
        }
        GPIOInterruptDone(pDevice->hGpio, pDevice->IntrGpio, pDevice->dwSysintr);     

    }
    
    return 0;
}
//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInit
//
//  Called by client to initialize device.
//
BOOL
Tca6416GpioInit(
             LPCTSTR szContext,
             HANDLE *phContext,
             UINT   *pGpioCount
             )
{
    BOOL rc = FALSE;  
    UCHAR dummy;
    TCA6416_DEVICE *pDevice = NULL;


    DEBUGMSG(ZONE_FUNCTION, (
        L"+Tca6416GpioInit(%s)\r\n", szContext
        ));

    if (phContext == NULL)
    {
        goto cleanUp;
    }
    // Create device structure
    pDevice = (TCA6416_DEVICE *)LocalAlloc(LPTR, sizeof(TCA6416_DEVICE));
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Tca6416GpioInit: "
            L"Failed allocate GPIO driver structure\r\n"
            ));
        goto cleanUp;
    }
    memset(pDevice, 0, sizeof(TCA6416_DEVICE));
    pDevice->dwSysintr = (DWORD) SYSINTR_UNDEFINED;
    *phContext = pDevice;
    if (pGpioCount)
    {
        *pGpioCount = NB_BANK * NB_PIN_PER_BANK;
    }

   // Read device parameters
    if (GetDeviceRegistryParams(
        szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams
        ) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Tca6416GpioInit: "
            L"Failed read FMD registry parameters\r\n"
            ));
        goto cleanUp;
    }
    pDevice->hI2C = I2COpen(SOCGetI2CDeviceByBus(pDevice->I2CBusIndex));
    if (pDevice->hI2C == NULL)
    {
        goto cleanUp;
    }
    I2CSetBaudIndex(pDevice->hI2C,pDevice->I2CBaudrateIndex);
    I2CSetSlaveAddress(pDevice->hI2C, (UINT16)pDevice->I2CAddr);
	I2CSetSubAddressMode(pDevice->hI2C, I2C_SUBADDRESS_MODE_8);

    for (int i=0;i<NB_BANK;i++)
    {
        InitializeCriticalSection(&pDevice->bankCs[i]);
    }
    InitializeCriticalSection(&pDevice->wakeupStateCs);    

    // Do a dummy read to chek if the expander is present on the bus.
    if (TCA6416I2CRead(pDevice->hI2C,CONFIG_PORT_0,&dummy,sizeof(dummy)) != sizeof(dummy))
    {
        goto cleanUp;
    }
    
    rc = TRUE;

cleanUp:
    if (rc == FALSE) Tca6416GpioDeinit((HANDLE)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-Tca6416GpioInit()\r\n"));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioDeinit
//
//  Called by device manager to uninitialize device.
//
BOOL
Tca6416GpioDeinit(
               HANDLE context
               )
{
    BOOL rc = FALSE;
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;
    
    if (pDevice == NULL)
    {
        goto cleanUp;
    }
    
    pDevice->fTerminateThread = TRUE;
    if (pDevice->hIntrEvent)
    {
        SetEvent(pDevice->hIntrEvent);        
    }
    if (pDevice->hIntrthread)
    {
        WaitForSingleObject(pDevice->hIntrthread,INFINITE);
    }    
    if (pDevice->dwSysintr != SYSINTR_UNDEFINED)
    {
        GPIOInterruptDisable(pDevice->hGpio, pDevice->IntrGpio, pDevice->dwSysintr);
    }
    if (pDevice->hGpio)
    {
        GPIOClose(pDevice->hGpio);
    }
    if (pDevice->hI2C)
    {
        I2CClose(pDevice->hI2C);
    }
    for (int i=0;i<NB_BANK;i++)
    {
        DeleteCriticalSection(&pDevice->bankCs[i]);
    }
    DeleteCriticalSection(&pDevice->wakeupStateCs);
    
    if (pDevice->hIntrEvent)
    {
        CloseHandle(pDevice->hIntrEvent);
    }

    // Free device structure
    LocalFree(pDevice);
    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-Tca6416GpioDeinit()\r\n"));
    return rc;
}

static BOOL  TCA6416SetRegMaskedValue(
               HANDLE hI2C,
               DWORD address,
               UCHAR value,
               UCHAR mask,
               BOOL fForceWrite
               )
{
    UCHAR regval,oldregval;
    BOOL rc = FALSE;

    if (TCA6416I2CRead(hI2C, (UCHAR)address, &oldregval, sizeof(oldregval)) != sizeof(oldregval))
    {
        goto cleanUp;
    }   
    
    regval = (oldregval & ~mask) | value;

    if (fForceWrite || (oldregval != regval))
    {
        if (TCA6416I2CWrite(hI2C, (UCHAR)address, &regval, sizeof(regval)) != sizeof(regval))
        {
            goto cleanUp;
        }   
    }

    // We succceded
    rc = TRUE;

cleanUp:

    return rc;    
}

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioSetMode
//
BOOL
Tca6416GpioSetMode(
                HANDLE context,
                UINT id,
                UINT mode
                )
{
    BOOL rc = FALSE;
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    UCHAR mask = (UCHAR) (1<<pin);
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    if ((pDevice == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))
    {
        goto cleanUp;
    }    
    
    EnterCriticalSection(&pDevice->bankCs[bank]);
    rc = TCA6416SetRegMaskedValue(
        pDevice->hI2C,
        CONFIG_PORT(bank),
        (mode & GPIO_DIR_INPUT) ? mask : 0,
        mask,
        FALSE);
    LeaveCriticalSection(&pDevice->bankCs[bank]);
        
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioGetMode
//
DWORD
Tca6416GpioGetMode(
                HANDLE context,
                UINT id
                )
{
    DWORD mode = (DWORD) -1;       
    UCHAR config;
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    UCHAR mask = (UCHAR) (1<<pin);

    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    if ((pDevice == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))    
    {
        goto cleanUp;
    }    
    EnterCriticalSection(&pDevice->bankCs[bank]);
    if (TCA6416I2CRead(pDevice->hI2C,CONFIG_PORT(bank),&config,sizeof(config)) == sizeof(config))
    {
        if (config & mask)
        {
            mode = GPIO_DIR_INPUT;
        }
        else
        {
            mode = GPIO_DIR_OUTPUT;
        }
    }
    LeaveCriticalSection(&pDevice->bankCs[bank]);
cleanUp:
    return mode;
}




//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioPullup
//
BOOL
Tca6416GpioPullup(
               HANDLE context,
               UINT id,
               UINT enable
               )
{
    BOOL    rc = FALSE;
    if (context == NULL)
    {
        goto cleanUp;
    }  
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enable);

    //Not available on the TCA6416

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioPulldown
//
BOOL
Tca6416GpioPulldown(
                 HANDLE context,
                 UINT id,
                 UINT enable
                 )
{
    BOOL    rc = FALSE;
    if (context == NULL)
    {
        goto cleanUp;
    }  
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enable);

    //Not available on the TCA6416

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioIntrEnable
//
BOOL
Tca6416GpioInterruptDone(
                      HANDLE context,
                      UINT intrId
                      )
{
    BOOL    rc = FALSE;
    if (context == NULL)
    {
        goto cleanUp;
    }  
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(intrId); 
cleanUp:
    return rc;   
}


//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioIntrDisable
//
BOOL
Tca6416GpioInterruptDisable(
                         HANDLE context,
                         UINT intrId
                         )
{
    BOOL    rc = FALSE;
    if (context == NULL)
    {
        goto cleanUp;
    }  
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(intrId); 
cleanUp:
    return rc;   
}

//***************************************END ADDITION********************************************

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioSetBit - Set the value of the GPIO output pin
//
BOOL
Tca6416GpioSetBit(
               HANDLE context,
               UINT id
               )
{
    BOOL rc = FALSE;
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    UCHAR mask = (UCHAR) (1<<pin);

    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    if ((pDevice == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))    
    {
        goto cleanUp;
    }    
    
    EnterCriticalSection(&pDevice->bankCs[bank]);
    rc = TCA6416SetRegMaskedValue(
        pDevice->hI2C,
        OUTPUT_PORT(bank),
        mask,
        mask,
        FALSE);
    LeaveCriticalSection(&pDevice->bankCs[bank]);

cleanUp:
    return rc;   
}

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioClrBit
//
BOOL
Tca6416GpioClrBit(
               HANDLE context,
               UINT id
               )
{
    BOOL rc = FALSE;
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    UCHAR mask = (UCHAR) (1<<pin);

    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    if ((pDevice == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))    
    {
        goto cleanUp;
    }    
    
    EnterCriticalSection(&pDevice->bankCs[bank]);
    rc = TCA6416SetRegMaskedValue(
        pDevice->hI2C,
        OUTPUT_PORT(bank),
        0,
        mask,
        FALSE);
    LeaveCriticalSection(&pDevice->bankCs[bank]);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Tca6416GpioGetBit
//
DWORD
Tca6416GpioGetBit(
               HANDLE context,
               UINT id
               )
{
    DWORD value = (DWORD) -1;
    UCHAR input;
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);

    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    if ((pDevice == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))    
    {
        goto cleanUp;
    }        
    if (TCA6416I2CRead(pDevice->hI2C,INPUT_PORT(bank),&input,sizeof(input)) == sizeof(input))
    {
        if (input & (1<<pin))
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
    }    

cleanUp:
    return value;
}

//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioIoControl
//
//  This function sends a command to a device.
//
BOOL
Tca6416GpioIoControl(
                  HANDLE  context,
                  UINT    code,
                  UCHAR  *pInBuffer,
                  UINT    inSize,
                  UCHAR  *pOutBuffer,
                  UINT    outSize,
                  DWORD   *pOutSize
                  )
{
#ifdef DEVICE
    UINT id;
    DWORD value,mode;
    DEVICE_IFC_GPIO ifc;
    BOOL rc = FALSE;
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)context;

    UNREFERENCED_PARAMETER(pOutSize);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+Tca6416GpioIOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Tca6416GpioIOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
    }
    switch (code)
    {
    case IOCTL_DDK_GET_DRIVER_IFC:
        // We can give interface only to our peer in device process
        if (GetCurrentProcessId() != (DWORD)GetCallerProcess())
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: GIO_IOControl: "
                L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                L"device process (caller process id 0x%08x)\r\n",
                GetCurrentProcessId()
                ));
            SetLastError(ERROR_ACCESS_DENIED);
            goto cleanUp;
        }
        if ((pInBuffer == NULL) || (inSize < sizeof(GUID)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_GPIO_GUID))
        {
            if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_GPIO);
            if ((pOutBuffer == NULL) || (outSize < sizeof(DEVICE_IFC_GPIO)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            ifc.context                 = context;
            ifc.pfnSetBit               = Tca6416GpioSetBit;
            ifc.pfnClrBit               = Tca6416GpioClrBit;
            ifc.pfnGetBit               = Tca6416GpioGetBit;
            ifc.pfnSetMode              = Tca6416GpioSetMode;
            ifc.pfnGetMode              = Tca6416GpioGetMode;
            ifc.pfnPullup               = Tca6416GpioPullup;
            ifc.pfnPulldown             = Tca6416GpioPulldown;
            ifc.pfnInterruptInitialize  = Tca6416GpioInterruptInitialize;
            ifc.pfnInterruptMask        = Tca6416GpioInterruptMask;
            ifc.pfnInterruptDisable     = Tca6416GpioInterruptDisable;
            ifc.pfnInterruptDone        = Tca6416GpioInterruptDone;
			ifc.pfnGetSystemIrq			= Tca6416GpioGetSystemIrq;
            ifc.pfnInterruptWakeUp      = Tca6416GpioInterruptWakeUp;
            ifc.pfnIoControl            = Tca6416GpioIoControl;

            if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(ifc))) break;
            rc = TRUE;
            break;
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        break;

    case IOCTL_GPIO_SETBIT:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Tca6416GpioSetBit(context, id);
        rc = TRUE;
        break;

    case IOCTL_GPIO_CLRBIT:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Tca6416GpioClrBit(context, id);
        rc = TRUE;
        break;

    case IOCTL_GPIO_GETBIT:
        if (pOutSize != 0) *pOutSize = sizeof(DWORD);
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)) ||
            (pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        value = Tca6416GpioGetBit(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &value, sizeof(value)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;

    case IOCTL_GPIO_SETMODE:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < 2 * sizeof(DWORD)) ||
            !CeSafeCopyMemory(
            &id, &((DWORD*)pInBuffer)[0], sizeof(DWORD)
            ) ||
            !CeSafeCopyMemory(
            &mode, &((DWORD*)pInBuffer)[1], sizeof(DWORD)
            ))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Tca6416GpioSetMode(context, id, mode);
        rc = TRUE;
        break;

    case IOCTL_GPIO_GETMODE:
        if (pOutSize != 0) *pOutSize = sizeof(DWORD);
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)) ||
            (pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        mode = Tca6416GpioGetMode(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &mode, sizeof(mode)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;
    case IOCTL_GPIO_SET_POWER_STATE:
        {
            if ((pInBuffer == NULL) || inSize != sizeof(IOCTL_GPIO_POWER_STATE_IN))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ERRORMSG(ZONE_ERROR, (L"ERROR: Tca6416GpioIOControl: "
                    L"IOCTL_GPIO_SET_POWER_STATE - Invalid parameter\r\n"
                    ));
                break;
            }
            rc = TRUE;            
        }
        break;

    case IOCTL_GPIO_GET_POWER_STATE:
        {
            if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                inSize != sizeof(IOCTL_GPIO_POWER_STATE_IN) ||
                outSize != sizeof(IOCTL_GPIO_GET_POWER_STATE_OUT))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ERRORMSG(ZONE_ERROR, (L"ERROR: Tca6416GpioIOControl: "
                    L"IOCTL_GPIO_SET_POWER_STATE - Invalid parameter\r\n"
                    ));
                break;
            }

            IOCTL_GPIO_POWER_STATE_IN *pPowerIn;
            IOCTL_GPIO_GET_POWER_STATE_OUT *pPowerOut;
            pPowerIn = (IOCTL_GPIO_POWER_STATE_IN*)pInBuffer;
            pPowerOut = (IOCTL_GPIO_GET_POWER_STATE_OUT*)pOutBuffer;
            
            pPowerOut->gpioState = D0;
            pPowerOut->bankState = D0;

            rc = TRUE;
        }
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-Tca6416GpioIOControl(rc = %d)\r\n", rc));
    return rc;
#endif

#ifdef OAL
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);        
    return FALSE;
#endif
}

//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioPowerUp
//
//  This function restores power to a device.
//
VOID
Tca6416GpioPowerUp(
                HANDLE hContext
                )
{
    UNREFERENCED_PARAMETER(hContext);
}

//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioPowerDown
//
//  This function suspends power to the device.
//
VOID
Tca6416GpioPowerDown(
                  HANDLE hContext
                  )
{
    UNREFERENCED_PARAMETER(hContext);
}

DWORD Tca6416GpioGetSystemIrq(HANDLE hContext, UINT id)
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(id);
    return (DWORD) SYSINTR_UNDEFINED;
}
//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInterruptInitialize
//
//  This function intialize the interrupt for the pin
//
BOOL Tca6416GpioInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent)
{
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)hContext;

    if ((hContext == NULL) || (sysintr==NULL) || (hEvent==NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))
    {
        goto cleanUp;
    }
    // if needed start the thread (done once only)
    if ((pDevice->dwSysintr == SYSINTR_UNDEFINED) && (pDevice->IntrGpio != (DWORD)-1))
    {
        pDevice->dwSysintr = (DWORD) SYSINTR_UNDEFINED;
        pDevice->hIntrEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (pDevice->hIntrEvent == NULL)
        {
            goto cleanUp;
        }
        pDevice->hIntrthread = CreateThread(NULL,0,IST,pDevice,CREATE_SUSPENDED,NULL);
        if (pDevice->hIntrthread == NULL)
        {
            goto cleanUp;
        }
        pDevice->hGpio = GPIOOpen();
        if (pDevice->hGpio == NULL)
        {
            goto cleanUp;
        }
        if (GPIOInterruptInitialize(pDevice->hGpio,pDevice->IntrGpio,&pDevice->dwSysintr,pDevice->hIntrEvent)== FALSE)
        {
            goto cleanUp;
        }
        ResumeThread(pDevice->hIntrthread);
    }


    EnterCriticalSection(&pDevice->bankCs[bank]);
    pDevice->hPinEvent[bank][pin] = hEvent;
    pDevice->intMask[bank] |= (UCHAR) (1<<pin);
    pDevice->enabledIntCount++;
    if (pDevice->enabledIntCount > 0)
    {
        GPIOInterruptMask(pDevice->hGpio, pDevice->IntrGpio,pDevice->dwSysintr,FALSE);
    }
    LeaveCriticalSection(&pDevice->bankCs[bank]);
    // compute a fake sysintr (unique for each pin)
    *sysintr = (pDevice->I2CAddr << 16) | (pDevice->I2CBusIndex << 8) | id;
        

    
    return TRUE;

cleanUp:
    return FALSE;
}
//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInterruptMask
//
//  This function suspends power to the device.
//
VOID Tca6416GpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable)
{    
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)hContext;

    UNREFERENCED_PARAMETER(sysintr);

    if ((hContext == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))
    {
        goto cleanUp;
    }     

    EnterCriticalSection(&pDevice->bankCs[bank]);    
    if (fDisable)
    {
        pDevice->intMask[bank] &= (UCHAR) (~(1<<pin));
        pDevice->enabledIntCount--;
        if (pDevice->enabledIntCount == 0)
        {
            GPIOInterruptMask(pDevice->hGpio, pDevice->IntrGpio,pDevice->dwSysintr,TRUE);
        }
    }
    else
    {
        pDevice->intMask[bank] |= (UCHAR) (1<<pin);
        pDevice->enabledIntCount++;
        if (pDevice->enabledIntCount > 0)
        {
            GPIOInterruptMask(pDevice->hGpio, pDevice->IntrGpio,pDevice->dwSysintr,FALSE);
        }
    }
    LeaveCriticalSection(&pDevice->bankCs[bank]);

cleanUp:
    return;
}
//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInterruptDisable
//
//  This function suspends power to the device.
//
VOID Tca6416GpioInterruptDisable(HANDLE hContext,DWORD id, DWORD sysintr)
{
    int bank = id / NB_PIN_PER_BANK;
    int pin = id - (bank*NB_PIN_PER_BANK);
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)hContext;

    UNREFERENCED_PARAMETER(sysintr);

    if ((hContext == NULL) || (bank >= NB_BANK) || (pin >= NB_PIN_PER_BANK))
    {
        goto cleanUp;
    }     

    EnterCriticalSection(&pDevice->bankCs[bank]);
    pDevice->hPinEvent[bank][pin] = NULL;
    pDevice->intMask[bank] &= (UCHAR) ~(1<<pin);
    LeaveCriticalSection(&pDevice->bankCs[bank]);

cleanUp:
    return;
}
//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInterruptDone
//
//  This function acknowledges the interrupt;
//
VOID Tca6416GpioInterruptDone(HANDLE hContext, DWORD id, DWORD sysintr)
{
    Tca6416GpioInterruptMask(hContext,id, sysintr,FALSE);
}


//------------------------------------------------------------------------------
//
//  Function:  Tca6416GpioInterruptWakeUp
//
//  This function enable the interrupt to wake up the processor from suspend
//
//  Note : we cannot select which pio is enabled so all of them will wakeup the system
//		   A ref counter is used
//

BOOL Tca6416GpioInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnable)
{    
    TCA6416_DEVICE *pDevice = (TCA6416_DEVICE*)hContext;
    int oldCounter = pDevice->wakeupStateCounter;
    BOOL rc = TRUE;

    UNREFERENCED_PARAMETER(sysintr);
    UNREFERENCED_PARAMETER(id);

    EnterCriticalSection(&pDevice->wakeupStateCs);
    
    if (fEnable)
        pDevice->wakeupStateCounter++;
    else
        pDevice->wakeupStateCounter--;

    if (pDevice->wakeupStateCounter < 0)
    {
        pDevice->wakeupStateCounter = 0;
    }

    if ((oldCounter>0) && (pDevice->wakeupStateCounter == 0))
    {
        rc = GPIOInterruptWakeUp(pDevice->hGpio, pDevice->IntrGpio, pDevice->dwSysintr,FALSE);     
    }
    else if ((oldCounter<=0) && (pDevice->wakeupStateCounter > 0))
    {
        rc = GPIOInterruptWakeUp(pDevice->hGpio, pDevice->IntrGpio, pDevice->dwSysintr,TRUE);     
    }

    LeaveCriticalSection(&pDevice->wakeupStateCs);

    return rc;
}

DWORD GIO_Init(
               LPCTSTR pContext,
               DWORD dwBusContext
               )
{
    HANDLE devCtxt;
    UINT count;

    UNREFERENCED_PARAMETER(dwBusContext);

    if (Tca6416GpioInit(pContext,&devCtxt,&count))
    {
        return (DWORD) devCtxt;                
    }
    return NULL;
}

BOOL GIO_Deinit(
                DWORD hDeviceContext 
                )
{
    return Tca6416GpioDeinit((HANDLE)hDeviceContext);
}

DWORD GIO_Open(
               DWORD hDeviceContext,
               DWORD AccessCode,
               DWORD ShareMode 
               )
{
    UNREFERENCED_PARAMETER(AccessCode);
    UNREFERENCED_PARAMETER(ShareMode);
    return hDeviceContext;
}

BOOL GIO_Close(
               DWORD hOpenContext 
               )
{
    UNREFERENCED_PARAMETER(hOpenContext);
    return TRUE;
}

void GIO_PowerDown(
                   DWORD hDeviceContext 
                   )
{
    Tca6416GpioPowerDown((HANDLE) hDeviceContext);
}
void GIO_PowerUp(
                 DWORD hDeviceContext 
                 )
{
    Tca6416GpioPowerUp((HANDLE) hDeviceContext);
}



BOOL GIO_IOControl(
                   DWORD hOpenContext,
                   DWORD dwCode,
                   PBYTE pBufIn,
                   DWORD dwLenIn,
                   PBYTE pBufOut,
                   DWORD dwLenOut,
                   PDWORD pdwActualOut 
                   )
{
    return Tca6416GpioIoControl(
        (HANDLE)hOpenContext,
        dwCode,
        pBufIn,
        dwLenIn,
        pBufOut,
        dwLenOut,
        pdwActualOut 
        );
}
