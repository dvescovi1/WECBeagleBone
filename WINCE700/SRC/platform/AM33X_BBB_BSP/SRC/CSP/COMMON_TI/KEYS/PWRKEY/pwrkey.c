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
//  File: pwrkey.c
//
//  This file implements device driver for pwrkey. The driver isn't implemented
//  as classical keyboard driver. Instead implementation uses stream driver
//  model and it calls SetSystemPowerState to suspend the system.
//
#include "omap.h"
#include "ceddkex.h"
#include "sdk_gpio.h"
#include <nkintr.h>


#define DEFAULT_DEBOUNCE_PERIOD (50)
#define DEFAULT_THREAD_PRIORITY (250)

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

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"pwrkey", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
            L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif


//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    HANDLE hGpio;
    DWORD pinId;
    DWORD dwSysintr;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    BOOL fTerminateThread;
    DWORD threadPriority;
    DWORD debouncePeriod;
} T_PWRKEY_DEVICE;

//------------------------------------------------------------------------------

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"pinId", PARAM_DWORD, TRUE, offset(T_PWRKEY_DEVICE, pinId),
            fieldsize(T_PWRKEY_DEVICE, pinId), (VOID*)0
    }, {
        L"debouncePeriod", PARAM_DWORD, FALSE, offset(T_PWRKEY_DEVICE, debouncePeriod),
            fieldsize(T_PWRKEY_DEVICE, debouncePeriod), (VOID*)DEFAULT_DEBOUNCE_PERIOD
    }, {
        L"Priority", PARAM_DWORD, FALSE, offset(T_PWRKEY_DEVICE, threadPriority),
            fieldsize(T_PWRKEY_DEVICE, threadPriority), (VOID*)DEFAULT_THREAD_PRIORITY
        },
};



BOOL PKD_Deinit(DWORD context);
DWORD PKD_IntrThread(VOID *pContext);

//------------------------------------------------------------------------------
//
//  Function:  PKD_Init
//
//  Called by device manager to initialize device.
//
DWORD
PKD_Init(
         LPCTSTR szContext,
         LPCVOID pBusContext
         )
{
    DWORD rc = (DWORD)NULL;    
    T_PWRKEY_DEVICE* pDevice = malloc(sizeof(T_PWRKEY_DEVICE));

    UNREFERENCED_PARAMETER(pBusContext);

    if (pDevice==NULL) return 0;
    memset(pDevice,0,sizeof(T_PWRKEY_DEVICE));
    pDevice->dwSysintr = (DWORD) SYSINTR_UNDEFINED;

    // Read parameters from registry
    if (GetDeviceRegistryParams(
        szContext, 
        pDevice, 
        dimof(s_deviceRegParams), 
        s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: PKD_Init: Error reading from Registry.\r\n")));
        goto cleanUp;
    }

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: Failed create interrupt event\r\n"));
        goto cleanUp;
    }

    pDevice->hIntrThread = CreateThread(NULL, 0, PKD_IntrThread, pDevice, 0, NULL);
    if (!pDevice->hIntrThread)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: PKD_Init: Failed create interrupt thread\r\n"));
        goto cleanUp;
    }


    pDevice->hGpio = GPIOOpen();        
    GPIOSetMode(pDevice->hGpio,pDevice->pinId,GPIO_DIR_INPUT | GPIO_INT_LOW);
    if (!GPIOInterruptInitialize(pDevice->hGpio,pDevice->pinId,&pDevice->dwSysintr,pDevice->hIntrEvent))
    {
        goto cleanUp;
    }    
    GPIOInterruptDone(pDevice->hGpio,pDevice->pinId,pDevice->dwSysintr);

    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread,pDevice->threadPriority);

    // Return non-null value
    rc = (DWORD)pDevice;
cleanUp:
    if (rc == (DWORD)NULL)
    {
        PKD_Deinit((DWORD) pDevice);
    }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
PKD_Deinit(
           DWORD context
           )
{
    BOOL rc = FALSE;
    T_PWRKEY_DEVICE *pDevice = (T_PWRKEY_DEVICE*) context;

    if (pDevice == NULL)
    {
        goto cleanUp;
    }
    if (pDevice->hIntrThread)
    {
        pDevice->fTerminateThread = TRUE;
        SetEvent(pDevice->hIntrEvent);
        WaitForSingleObject(pDevice->hIntrThread,INFINITE);
    }
    if (pDevice->dwSysintr != SYSINTR_UNDEFINED)
    {
        GPIOInterruptDisable(pDevice->hGpio,pDevice->pinId,pDevice->dwSysintr);
    }
    if (pDevice->hGpio)
    {
        GPIOClose(pDevice->hGpio);
    }
    if (pDevice->hIntrEvent)
    {
        CloseHandle(pDevice->hIntrEvent);
    }
    LocalFree(pDevice);

    rc = TRUE;

cleanUp:

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
PKD_Open(
         DWORD context, 
         DWORD accessCode, 
         DWORD shareMode
         )
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);
    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_Close
//
//  This function closes the device context.
//
BOOL
PKD_Close(
          DWORD context
          )
{
    UNREFERENCED_PARAMETER(context);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_PowerUp
//
//  Called on resume of device.  Current implementation of pwrkey driver
//  will disable the pwrkey interrupts before suspend.  Make sure the
//  pwrkey interrupts are re-enabled on resume.
//
void
PKD_PowerUp(
            DWORD context
            )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_PowerDown
//
//  Called on suspend of device.
//
void
PKD_PowerDown(
              DWORD context
              )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_IOControl
//
//  This function sends a command to a device.
//
BOOL
PKD_IOControl(
              DWORD context, 
              DWORD code, 
              UCHAR *pInBuffer, 
              DWORD inSize, 
              UCHAR *pOutBuffer,
              DWORD outSize, 
              DWORD *pOutSize
              )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(context);    
    UNREFERENCED_PARAMETER(code);    
    UNREFERENCED_PARAMETER(pInBuffer);    
    UNREFERENCED_PARAMETER(inSize);    
    UNREFERENCED_PARAMETER(pOutBuffer);    
    UNREFERENCED_PARAMETER(outSize);    
    UNREFERENCED_PARAMETER(pOutSize);    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PKD_InterruptThread
//
//  This function acts as the IST for the pwrkey.
//
//  Note: This function is more complex than it would be if there were any way to
//        directly read the state of the TPS659XX PWRON key status.
//
DWORD PKD_IntrThread(VOID *pContext)
{
    T_PWRKEY_DEVICE *pDevice = (T_PWRKEY_DEVICE*) pContext;
    while (!pDevice->fTerminateThread)
    {
        // Wait for pwrkey interrupt
        if (WaitForSingleObject(pDevice->hIntrEvent, INFINITE) == WAIT_OBJECT_0)
        {
            GPIOInterruptDone(pDevice->hGpio,pDevice->pinId,pDevice->dwSysintr);            
            while (WaitForSingleObject(pDevice->hIntrEvent, pDevice->debouncePeriod) == WAIT_OBJECT_0)
            {
                GPIOInterruptDone(pDevice->hGpio,pDevice->pinId,pDevice->dwSysintr);
            }            
            if (pDevice->fTerminateThread)
            {
                break;
            }
            if (GPIOGetBit(pDevice->hGpio,pDevice->pinId) == 0)
            {
                // Key is pressed for more than xxx ms
                RETAILMSG(1,(TEXT("Power Key pressed!!!\r\n")));
                // suspend system
                SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);
            }
        }
        if (pDevice->fTerminateThread)
        {
            break;
        }

    }

    DEBUGMSG(ZONE_IST, (L"-PKD_IntrThread\r\n"));
    return ERROR_SUCCESS;
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

