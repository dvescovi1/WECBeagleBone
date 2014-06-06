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
//  File: hdq.cpp
//
#include "omap.h"
#include "ceddkex.h"
#include "omap_hdq_1wire_regs.h"

#include "soc_cfg.h"
#include "oal_clock.h"
#include "sdk_gpio.h"


#include <initguid.h>
#include "omap_hdq.h"

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD


//************* uncommented 6/23/08  MGJ******************

//#define ZONE_INIT           DEBUGZONE(0)
//#define ZONE_ERROR          DEBUGZONE(1)
#define ZONE_WARN           DEBUGZONE(2)
#define ZONE_FUNCTION       DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5) 
#define ZONE_HDQ            DEBUGZONE(15)
//******************END commented ************************

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"HDQ", {
        L"Init",        L"Errors",      L"Warnings",    L"Function",
        L"Info",        L"IST",         L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"HDQ_log"
    },
    0x0003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define HDQ_DEVICE_COOKIE       'hdqD'
#define HDQ_INSTANCE_COOKIE     'hdqI'
#define TIMERTHREAD_TIMEOUT     1
#define TIMERTHREAD_PRIORITY    152

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD cookie;
/*    DWORD memBase;
    DWORD memLen;
    DWORD irq;
*/
    DWORD dwHDQBusIndex;
    DWORD dwDeviceId;
    DWORD breakTimeout;
    DWORD txTimeout;
    DWORD rxTimeout;
    OMAP_HDQ_1WIRE_REGS *pHDQRegs;
    HANDLE hParent;
    LONG instances;
    CRITICAL_SECTION cs;
    DWORD sysIntr;
    HANDLE hIntrEvent;
    DWORD  nPowerState;
    DWORD  nActivityTimeout;
    HANDLE hTimerThread;
    HANDLE hTimerEvent;
    BOOL   bDisablePower;
    BOOL   bExitThread;
    int    nPowerCounter;
    HANDLE hGpio;
} Device_t;


typedef struct {
    DWORD cookie;
    DWORD mode;
    Device_t *pDevice;
} Instance_t;

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    /*{
        L"MemBase", PARAM_DWORD, TRUE, offset(Device_t, memBase),
        fieldsize(Device_t, memBase), NULL
    }, {
        L"MemLen", PARAM_DWORD, TRUE, offset(Device_t, memLen),
        fieldsize(Device_t, memLen), NULL
    }, {
        L"Irq", PARAM_DWORD, TRUE, offset(Device_t, irq),
        fieldsize(Device_t, irq), NULL
    },
    */{
        L"BusIndex", PARAM_DWORD, FALSE, offset(Device_t, dwHDQBusIndex),
        fieldsize(Device_t, dwHDQBusIndex), (VOID*)0
    }, {
        L"BreakTimeout", PARAM_DWORD, FALSE, offset(Device_t, breakTimeout),
        fieldsize(Device_t, breakTimeout), (VOID*)1000
    }, {
        L"TxTimeout", PARAM_DWORD, FALSE, offset(Device_t, txTimeout),
        fieldsize(Device_t, txTimeout), (VOID*)1000
    }, {
        L"RxTimeout", PARAM_DWORD, FALSE, offset(Device_t, rxTimeout),
        fieldsize(Device_t, rxTimeout), (VOID*)1000
    }, {
        L"ActivityTimeout", PARAM_DWORD, FALSE, offset(Device_t, nActivityTimeout),
        fieldsize(Device_t, nActivityTimeout), (VOID*)TIMERTHREAD_TIMEOUT
    }
};

//------------------------------------------------------------------------------
//  Local Functions

BOOL HDQ_Deinit(DWORD context);
EXTERN_C DWORD HDQ_Write(DWORD context, void* pBuffer, DWORD size);
EXTERN_C DWORD HDQ_Read(DWORD context, void* pBuffer, DWORD size);

//------------------------------------------------------------------------------
//
//  Function:  HDQPowerTimerThread
//
//  timeout thread, checks to see if the power can be disabled.
//
DWORD 
HDQPowerTimerThread(
    void *pv
    )
{
    DWORD nTimeout = INFINITE;
    Device_t *pDevice = (Device_t*)(pv);

    for(;;)
        {
        WaitForSingleObject(pDevice->hTimerEvent, nTimeout);

        if (pDevice->bExitThread == TRUE) break;

        // serialize access to power state changes
        EnterCriticalSection(&pDevice->cs);

        // by the time this thread got the cs hTimerEvent may 
        // have gotten resignaled.  Clear the event to  make
        // sure the activity timer thread isn't awaken prematurely
        //
        ResetEvent(pDevice->hTimerEvent);

        // check if we need to reset the timer
        if (pDevice->nPowerCounter == 0)
            {
            // We disable the power only when this thread
            // wakes-up twice in a row with no power state
            // change to D0.  This is achieved by using the
            // bDisablePower flag to determine if power state
            // changed since the last time this thread woke-up
            //
            if (pDevice->bDisablePower == TRUE)
                {
                //SetDevicePowerState(pDevice->hParent, D4, 0);
                EnableDeviceClocks(pDevice->dwDeviceId,FALSE);
                pDevice->nPowerState = D4;
                DEBUGMSG(ZONE_FUNCTION, (L"HDQ SetDevicePowerState(pDevice->hParent, D4, NULL) in the Thread\r\n"));
                nTimeout = INFINITE;
                }
            else
                {
                // wait for activity time-out before shutting off power.
                pDevice->bDisablePower = TRUE;
                nTimeout = pDevice->nActivityTimeout;
                }
            }
        else
            {
            nTimeout = INFINITE;
            }
        LeaveCriticalSection(&pDevice->cs);
        }

    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  SetHDQPower
//
//  centeral function to enable power to hdq bus.
//
inline BOOL
SetHDQPower(
    Device_t               *pDevice,
    CEDEVICE_POWER_STATE    state
    )
{
    // enable power when the power state request is D0-D2
    EnterCriticalSection(&pDevice->cs);

    if (state < D3)
        {
        if (pDevice->nPowerState >= D3)
            {
                //SetDevicePowerState(pDevice->hParent, D0, NULL);
                EnableDeviceClocks(pDevice->dwDeviceId,TRUE);
                pDevice->nPowerState = D0;
            }
        pDevice->bDisablePower = FALSE;
        pDevice->nPowerCounter++;
        }
    else 
        {
        pDevice->nPowerCounter--;
        if (pDevice->nPowerCounter == 0)
            {
            if (pDevice->hTimerEvent != NULL)
                {
                SetEvent(pDevice->hTimerEvent);
                }
            else
                {
                //SetDevicePowerState(pDevice->hParent, D4, NULL);
                EnableDeviceClocks(pDevice->dwDeviceId,FALSE);
                pDevice->nPowerState = D4;
                DEBUGMSG(ZONE_FUNCTION, (L"HDQ SetDevicePowerState(pDevice->hParent, D4, NULL)\r\n"));
                }
            }
        }
    
    LeaveCriticalSection(&pDevice->cs);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  TransmitBreak
//
//  sends a break command to the slave device.
//
void 
TransmitBreak(
    Instance_t *pInstance
    )
{
    DWORD status;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+TransmitBreak(x%08x)\r\n", pInstance));

    // set break command to slave
    Device_t *pDevice = pInstance->pDevice;
    OUTREG32(&pDevice->pHDQRegs->CTRL_STATUS, 
        HDQ_CTRL_INITIALIZE | HDQ_CTRL_GO | HDQ_CTRL_CLOCK_ENABLE | 
        pInstance->mode);

    // wait for timeout
    WaitForSingleObject(pDevice->hIntrEvent, pDevice->breakTimeout);
        
    // Clear interrupt.
    status = INREG32(&pDevice->pHDQRegs->INT_STATUS);
    InterruptDone(pDevice->sysIntr);

    DEBUGMSG(ZONE_INFO, (L"TransmitBreak: status = 0x%02X\r\n", status));

    // disable clock and reset to mode
    OUTREG32(&pDevice->pHDQRegs->CTRL_STATUS, pInstance->mode);

    DEBUGMSG(ZONE_FUNCTION, (L"-TransmitBreak()\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  TransmitCommand
//
//  sends a command to the slave device.
//
BOOL 
TransmitCommand(
    Instance_t *pInstance,
    UINT8 cmd,
    DWORD *pStatus
    )
{
    DWORD mask;
    BOOL rc = FALSE;
    Device_t *pDevice = pInstance->pDevice;
    OMAP_HDQ_1WIRE_REGS *pHDQRegs = pDevice->pHDQRegs;

    DEBUGMSG(ZONE_FUNCTION, (L"+TransmitCommand(cmd=0x%02X)\r\n", cmd));

    // setup mask
    mask = HDQ_CTRL_GO | HDQ_CTRL_WRITE | 
           HDQ_CTRL_CLOCK_ENABLE | pInstance->mode;
    
    // write the command
    OUTREG32(&pHDQRegs->TX_DATA, cmd);

    // send signal to write
    OUTREG32(&pHDQRegs->CTRL_STATUS, mask);
            
    // Wait on TX complete interrupt.
    if (WaitForSingleObject(pDevice->hIntrEvent, 
        pDevice->txTimeout) == WAIT_TIMEOUT) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: TransmitCommand: "
            L"Timeout in Tx\r\n"
            ));
        TransmitBreak(pInstance);
        goto cleanUp;
        }
    
    // Clear interrupt.
    *pStatus = INREG32(&pHDQRegs->INT_STATUS);
    InterruptDone(pDevice->sysIntr);

    DEBUGMSG(ZONE_INFO, (L"TransmitCommand: *pStatus = 0x%02X\r\n", *pStatus));
    
    // Verify interrupt source
    if ((*pStatus & HDQ_INT_TX_COMPLETE) == 0) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: TransmitCommand: "
            L"TX complete expected (0x%x)\r\n", *pStatus
            ));
        TransmitBreak(pInstance);
        goto cleanUp;
        }
    rc = TRUE;
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TransmitCommand(cmd=0x%02X)\r\n", cmd));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_Init
//
//  Called by device manager to initialize device.
//
DWORD 
HDQ_Init(
    LPCTSTR szContext, 
    LPCVOID pBusContext
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Init(%s, 0x%08x)\r\n", 
        szContext, pBusContext
        ));

    UNREFERENCED_PARAMETER(pBusContext);    
    
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = NULL;
    PHYSICAL_ADDRESS pa;

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed allocate HDQ controller structure\r\n"
            ));
        goto cleanUp;
        }
    memset(pDevice, 0, sizeof(Device_t));

    // Set cookie
    pDevice->cookie = HDQ_DEVICE_COOKIE;
    pDevice->nPowerState = D4;

    // set device reference count to 1
    InterlockedIncrement(&pDevice->instances);

    // Initalize critical section
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(g_deviceRegParams), 
        g_deviceRegParams) != ERROR_SUCCESS) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed read HDQ driver registry parameters\r\n"
            ));
        goto cleanUp;
        }

    // Open parent bus
    pDevice->hParent = CreateBusAccessHandle(szContext);
    if (pDevice->hParent == NULL) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed open parent bus driver\r\n"
            ));
        goto cleanUp;
        }
    
    // start timer thread
    pDevice->hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hTimerEvent != NULL)
        {
        pDevice->hTimerThread = CreateThread(NULL, 0, HDQPowerTimerThread, 
            pDevice, 0, NULL
            );
        
        if (pDevice->hTimerThread != NULL)
            {
            CeSetThreadPriority(pDevice->hTimerThread, TIMERTHREAD_PRIORITY);
            }
        }

    pDevice->dwDeviceId = SOCGetHDQDevice(pDevice->dwHDQBusIndex);

    // Set hardware to full power
    SetHDQPower(pDevice, D0);

    // Map HDQ_1WIRE controller
    pa.QuadPart = GetAddressByDevice(pDevice->dwDeviceId);
    pDevice->pHDQRegs = (OMAP_HDQ_1WIRE_REGS*)MmMapIoSpace(pa, 
                            sizeof(OMAP_HDQ_1WIRE_REGS), FALSE
                            );
    
    if (pDevice->pHDQRegs == NULL) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed map HDQ controller registers\r\n"
            ));
        goto cleanUp;
        }

    DWORD irq = GetIrqByDevice(pDevice->dwDeviceId,NULL);
    // Map HDQ_1WIRE interrupt
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &irq, 
        sizeof(irq), &pDevice->sysIntr, 
        sizeof(pDevice->sysIntr), NULL
        )) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed map HDQ/1WIRE controller interrupt\r\n"
            ));
        goto cleanUp;
        }

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Init: "
            L"Failed create interrupt event\r\n"
            ));
        goto cleanUp;
        }
    
    if (InterruptInitialize(pDevice->sysIntr, pDevice->hIntrEvent, NULL, 0) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDQ_Init: "
            L"InterruptInitialize failed\r\n"
            ));
        goto cleanUp;
        }

    // Enable the clock.
    SETREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_CTRL_CLOCK_ENABLE);
    
    // Set to HDQ mode as default
    CLRREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_MODE_1WIRE);
    
    // Clear the interrupt
    INREG32(&pDevice->pHDQRegs->INT_STATUS);
    
    // Enable interrupt
    SETREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_CTRL_INTERRUPT_MASK);

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (pDevice != NULL) SetHDQPower(pDevice, D4);
    if (rc == 0) HDQ_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Init(rc = 0x%x)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL 
HDQ_Deinit(
    DWORD context
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Deinit(0x%08x)\r\n", context));
    
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: HDQ_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    InterlockedDecrement(&pDevice->instances);
    ASSERT(pDevice->instances == 0);

    // Disable interrupt.
    CLRREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_CTRL_INTERRUPT_MASK);

    // Clear the interrupt.
    INREG32(&pDevice->pHDQRegs->INT_STATUS);

    // stop interrupt thread
    if (pDevice->hTimerThread != NULL)
        {
        pDevice->bExitThread = TRUE;
        SetEvent(pDevice->hTimerEvent);
        WaitForSingleObject(pDevice->hTimerThread, INFINITE);
        CloseHandle(pDevice->hTimerThread);
        pDevice->hTimerThread = NULL;
        }

    if (pDevice->hTimerEvent != NULL)
        {
        CloseHandle(pDevice->hTimerEvent);
        }

    // Set hardware to D4 and close parent bus driver
    if (pDevice->hParent != NULL) 
        {
        SetHDQPower(pDevice, D4);
        CloseBusAccessHandle(pDevice->hParent);
        }

    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);

    // Unmap HDQ_1Wire controller registers
    if (pDevice->pHDQRegs != NULL) 
        {
        MmUnmapIoSpace((VOID*)pDevice->pHDQRegs, sizeof(OMAP_HDQ_1WIRE_REGS));
        }

    // Release HDQ_1Wire controller interrupt
    if (pDevice->sysIntr != 0) 
        {
        InterruptDisable(pDevice->sysIntr);
        }

    // Close interrupt handler
    if (pDevice->hIntrEvent != NULL) CloseHandle(pDevice->hIntrEvent);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Deinit(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD 
HDQ_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    Instance_t *pInstance = NULL;
    Device_t *pDevice = (Device_t *)context;

    UNREFERENCED_PARAMETER(shareMode);
    UNREFERENCED_PARAMETER(accessCode);

    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Open(0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, accessCode, shareMode
        ));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Open: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    pInstance = (Instance_t*)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Open: "
            L"Unable to allocate instance (err=0x%08X)",
            GetLastError()
            ));
        goto cleanUp;
        }

    // initialize variable
    pInstance->cookie = HDQ_INSTANCE_COOKIE;
    pInstance->mode = HDQ_CTRL_INTERRUPT_MASK;
    pInstance->pDevice = pDevice;

    // increment reference count
    InterlockedIncrement(&pDevice->instances);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Open(context=0x%08X)\r\n", context));
    return (DWORD)pInstance;
}


void ContextRestore(Device_t *pDevice)
{
    // Enable the clock.
    SETREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_CTRL_CLOCK_ENABLE);
    
    // Set to HDQ mode as default
    CLRREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_MODE_1WIRE);
    
    // Clear the interrupt
    INREG32(&pDevice->pHDQRegs->INT_STATUS);
    
    // Enable interrupt
    SETREG32(&pDevice->pHDQRegs->CTRL_STATUS, HDQ_CTRL_INTERRUPT_MASK);
}

//------------------------------------------------------------------------------
//
//  Function:  HDQ_Close
//
//  This function closes the device context.
//
BOOL 
HDQ_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice;
    Instance_t *pInstance = (Instance_t*)context;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Close(0x%08x)\r\n", context));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != HDQ_INSTANCE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Close: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
        }

    pDevice = pInstance->pDevice;
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Close: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
        }

    InterlockedDecrement(&pDevice->instances);
    ASSERT(pDevice->instances > 0);
    LocalFree(pInstance);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Close(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_IOControl
//
//  called during power down of device.
//
VOID
HDQ_PowerDown(
    DWORD dwContext
    )
{
    Device_t *pDevice = (Device_t*)dwContext;
    if (pDevice->nPowerState == D0)
        {
        //SetDevicePowerState(pDevice->hParent, D4, NULL);
        EnableDeviceClocks(pDevice->dwDeviceId,FALSE);
        }
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_PowerUp
//
//  called during power up device.
//
VOID
HDQ_PowerUp(
    DWORD dwContext
    )
{
    Device_t *pDevice = (Device_t*)dwContext;
    if (pDevice->nPowerState == D0)
        {
        //SetDevicePowerState(pDevice->hParent, D0, NULL);
        EnableDeviceClocks(pDevice->dwDeviceId,TRUE);
        }
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_IOControl
//
//  This function sends a command to a device.
//
BOOL 
HDQ_IOControl(
    DWORD context, 
    DWORD code, 
    BYTE *pInBuffer, 
    DWORD inSize, 
    BYTE *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice;
    DEVICE_IFC_HDQ ifc;
    Instance_t *pInstance = (Instance_t*)context;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_IOControl"
        L"(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != HDQ_INSTANCE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Check if we get correct context
    pDevice = pInstance->pDevice;
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_IOControl: "
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
                DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                ));
                SetLastError(ERROR_ACCESS_DENIED);
                break;
                }
            
            if (pInBuffer == NULL || inSize < sizeof(GUID)) 
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            
            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_HDQ_GUID)) 
                {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_HDQ);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_HDQ)) 
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                
                ifc.context = context;
                ifc.pfnRead = HDQ_Read;
                ifc.pfnWrite = HDQ_Write;           
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_HDQ))) 
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }
            
            SetLastError(ERROR_INVALID_PARAMETER);
            break;

    case IOCTL_CONTEXT_RESTORE:
            DEBUGMSG(ZONE_FUNCTION, (L"HDQ_IOControl: IOCTL_CONTEXT_RESTORE_NOTIFY\r\n"));
            ContextRestore(pDevice);
            break;
      
    default:
            DEBUGMSG(ZONE_WARN, (L"WARN: HDQ_IOControl: "
                L"Unsupported code 0x%08x\r\n", code
                ));
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_Write
//
//  This function writes a byte of data in 8 bit mode to the specified address.
//
DWORD 
HDQ_Write(
    DWORD context, 
    VOID *pBuffer, 
    DWORD size
    )
{
    DWORD mask;
    DWORD status, i;
    BOOL rc = FALSE;
    Device_t *pDevice=NULL;
    BOOL bLocked = FALSE;
    OMAP_HDQ_1WIRE_REGS *pHDQRegs;
    UINT8 *pData = (UINT8*)pBuffer;
    Instance_t *pInstance = (Instance_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Write(0x%08x, 0x%02x, 0x%x)\r\n",
        context, pBuffer, size
        ));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != HDQ_INSTANCE_COOKIE || 
        pBuffer == NULL || size <= 1) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Write: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Check if we get correct context
    pDevice = pInstance->pDevice;
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Write: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Get hardware
    pHDQRegs = pDevice->pHDQRegs;
    EnterCriticalSection(&pDevice->cs);

    // set flag so we properly release critical section
    bLocked = TRUE;

    // Make sure that clock is present
    SetHDQPower(pDevice, D0);

    // Clear the interrupt.
    status = INREG32(&pHDQRegs->INT_STATUS);
    InterruptDone(pDevice->sysIntr);
    DEBUGMSG(ZONE_INFO, (L"HDQ_Write: "
        L"Interrupt status (1) was 0x%02x\r\n", status
        ));

    // send command to slave device
    TransmitBreak(pInstance);
    if (TransmitCommand(pInstance, (UINT8)(pData[0] | HDQ_WRITE_CMD), &status) == FALSE)
	    {
        goto cleanUp;
        }

    // setup mask for read
    mask = HDQ_CTRL_GO | HDQ_CTRL_WRITE | 
           HDQ_CTRL_CLOCK_ENABLE | pInstance->mode; 

    // Two write cycles required
    for (i = 1; i < size; ++i) 
        {
        // Write address or data to TX write register
        OUTREG32(&pHDQRegs->TX_DATA, pData[i]);

        // send signal to write
        OUTREG32(&pHDQRegs->CTRL_STATUS, mask);
                
        // Wait on TX complete interrupt.
        if (WaitForSingleObject(pDevice->hIntrEvent, 
            pDevice->txTimeout) == WAIT_TIMEOUT) 
            {
            DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Write: "
                L"Timeout in Tx\r\n"
                ));
            
            TransmitBreak(pInstance);
            goto cleanUp;
            }
        
        // Clear interrupt.
        status = INREG32(&pHDQRegs->INT_STATUS);
        InterruptDone(pDevice->sysIntr);
        DEBUGMSG(ZONE_INFO, (L"HDQ_Write: "
            L"Interrupt status (%d) was 0x%x\r\n", 3 + i, status
            ));
        
        // Verify interrupt source
        if ((status & HDQ_INT_TX_COMPLETE) == 0) 
            {
            DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Write: "
                L"TX complete expected (0x%x)\r\n", status
                ));
            
            TransmitBreak(pInstance);
            goto cleanUp;
            }
        }

    // disable clock
    OUTREG32(&pHDQRegs->CTRL_STATUS, pInstance->mode);

    // Done
    rc = TRUE;

cleanUp:
    if(pDevice != NULL) 
    {
       SetHDQPower(pDevice, D4);   
    
       if (bLocked == TRUE) LeaveCriticalSection(&pDevice->cs);
       DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Write(rc = %d)\r\n", rc));
    }   
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  HDQ_Read
//
//  This function reads from the specified address data in HDQ mode.
//
DWORD 
HDQ_Read(
    DWORD context, 
    void* pBuffer, 
    DWORD size
    )
{
    DWORD mask;
    DWORD status, i;
    BOOL rc = FALSE;
    Device_t *pDevice = NULL;
    BOOL bLocked = FALSE;
    OMAP_HDQ_1WIRE_REGS *pHDQRegs;
    UINT8 *pData = (UINT8*)pBuffer;
    Instance_t *pInstance = (Instance_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+HDQ_Read"
        L"(0x%08x, 0x%02x, 0x%08x)\r\n", context, pBuffer, size
        ));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != HDQ_INSTANCE_COOKIE || 
        pBuffer == NULL || size <= 1) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR! HDQ_Read: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Check if we get correct context
    pDevice = pInstance->pDevice;
    if (pDevice == NULL || pDevice->cookie != HDQ_DEVICE_COOKIE) 
        {
        DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Read: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Get hardware
    pHDQRegs = pDevice->pHDQRegs;
    EnterCriticalSection(&pDevice->cs);
    bLocked = TRUE;

    // Make sure that clock is present
    SetHDQPower(pDevice, D0);

    // Clear the interrupt.
    status = INREG32(&pHDQRegs->INT_STATUS);
    InterruptDone(pDevice->sysIntr);
    DEBUGMSG(ZONE_INFO, (L"HDQ_Read: "
        L"Interrupt status (1) was 0x%02x\r\n", status
        ));

    // send command to slave device
    TransmitBreak(pInstance);
    if (TransmitCommand(pInstance, pData[0], &status) == FALSE)
        {
        goto cleanUp;
        }

    // setup mask for read
    mask = HDQ_CTRL_GO | HDQ_CTRL_READ | 
           HDQ_CTRL_CLOCK_ENABLE | pInstance->mode; 

    for (i = 1; i < size; ++i)
        {
        // it's possible to receive data soon after transmit resulting
        // in both receive and transmit flags being set.  So need to
        // check if data was recieved.
        if ((status & HDQ_INT_RX_COMPLETE) == 0)
            {
            // initiate receive
            OUTREG32(&pHDQRegs->CTRL_STATUS, mask);
            if (WaitForSingleObject(pDevice->hIntrEvent, 
                pDevice->rxTimeout) == WAIT_TIMEOUT)
                {
                DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Read: "
                    L"Timeout in Rx\r\n"
                    ));

                TransmitBreak(pInstance);
                goto cleanUp;
                
                }

            // Clear interrupt
            status = INREG32(&pHDQRegs->INT_STATUS);
            InterruptDone(pDevice->sysIntr);
            }
        
        // Verify interrupt source
        if ((status & HDQ_INT_RX_COMPLETE) == 0) 
            {
            DEBUGMSG(ZONE_ERROR && ZONE_HDQ, (L"ERROR: HDQ_Read: "
                L"RX complete expected (0x%02x)\r\n", status
                ));

            TransmitBreak(pInstance);
            goto cleanUp;
            }
            
        DEBUGMSG(ZONE_INFO, (L"INFO: HDQ_Read: "
            L"Read status = 0x%02X\r\n", status
            ));
        
        // read data
        pData[i] = (UINT8)INREG32(&pHDQRegs->RX_DATA);
        DEBUGMSG(ZONE_INFO, (L"INFO: HDQ_Read: "
            L"Read data[i] = 0x%02X\r\n", i, pData[i]
            ));
        
        // reset status to force read request
        status = 0;
        }

    // disable clock
    OUTREG32(&pHDQRegs->CTRL_STATUS, pInstance->mode);
    
    // Done
    rc = TRUE;

cleanUp:
    if(pDevice != NULL) 
    {
       SetHDQPower(pDevice, D4);
    
       if (bLocked == TRUE) LeaveCriticalSection(&pDevice->cs);
       DEBUGMSG(ZONE_FUNCTION, (L"-HDQ_Read(rc = %d)\r\n", rc));
    }
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

//------------------------------------------------------------------------------
