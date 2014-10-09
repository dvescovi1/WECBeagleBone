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
//  model and it calls SetSystemPowerState to suspend/turn off the system.
//
#include "bsp.h"
#include "pm.h"
#include "ceddkex.h"
#include "tps65217.h"

#include <initguid.h>
#include "twl.h"
#include "triton.h"


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

#define IOCTL_PWRKEY_GETSTAT            \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x300, METHOD_BUFFERED, FILE_ANY_ACCESS)

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
    0x000F
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define PKD_DEVICE_COOKIE           'KPGG'

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD Cookie;
    DWORD priority256;
    DWORD enableWake;
    DWORD DebounceTimeMs;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    DWORD irq;
    DWORD dwSysintr;
	BYTE oldStatus;
    HANDLE hTWL;
    BOOL bIntrThreadExit;
    DWORD powerMask;
    CEDEVICE_POWER_STATE powerState;
    volatile DWORD bWakeFromSuspend;
} PwrkeyDevice_t;

// To Do:
// - Find out how to read current state of PWRON pin.

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE, 
        offset(PwrkeyDevice_t, priority256),
        fieldsize(PwrkeyDevice_t, priority256), (VOID*)100
    }, {
        L"EnableWake", PARAM_DWORD, FALSE, 
        offset(PwrkeyDevice_t, enableWake),
        fieldsize(PwrkeyDevice_t, enableWake), (VOID*)1
    }, {
        L"DebounceTimeMs", PARAM_DWORD, FALSE, 
        offset(PwrkeyDevice_t, DebounceTimeMs),
        fieldsize(PwrkeyDevice_t, DebounceTimeMs), (VOID*)50
    }
};

//------------------------------------------------------------------------------

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
    PwrkeyDevice_t *pDevice = NULL;
    BYTE data;
    
    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"+PKD_Init(%s, 0x%08x)\r\n", szContext, pBusContext));

    // Create device structure
    pDevice = (PwrkeyDevice_t *)LocalAlloc(LPTR, sizeof(PwrkeyDevice_t));
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: Failed allocate PKD driver structure\r\n"));
        goto cleanUp;
    }

    memset(pDevice, 0, sizeof(PwrkeyDevice_t));

    // Set Cookie
    pDevice->Cookie = PKD_DEVICE_COOKIE;

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: Failed read PKD driver registry parameters\r\n"));
        goto cleanUp;
    }

    // Open T2 driver
    pDevice->hTWL = TWLOpen();
    if (pDevice->hTWL == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: Failed open TWL bus driver\r\n"));
        goto cleanUp;
    }

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: Failed create interrupt event\r\n"));
        goto cleanUp;
    }

	pDevice->irq = IRQ_NMI;
	
    if (KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,&pDevice->irq,sizeof(pDevice->irq),&pDevice->dwSysintr,sizeof(pDevice->dwSysintr),NULL)== FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: unable to request a SYSINTR\r\n"));
        goto cleanUp;
    }

    // initialize interrupt
    if (InterruptInitialize(pDevice->dwSysintr,pDevice->hIntrEvent,NULL,0)==FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PKD_Init: unable to initialize the interrupt\r\n"));
        goto cleanUp;
    }

	data = 0;
	TWLReadByteReg(pDevice->hTWL, PMIC_REG_CHIPID, &data);

    // Start interrupt service thread
    pDevice->bIntrThreadExit = FALSE;
    pDevice->hIntrThread = CreateThread(NULL, 0, PKD_IntrThread, pDevice, 0, NULL);
    if (!pDevice->hIntrThread)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: PKD_Init: Failed create interrupt thread\r\n"));
        goto cleanUp;
    }
    
    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256);

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0)
    {
        PKD_Deinit((DWORD)pDevice);
    }
    DEBUGMSG(ZONE_FUNCTION, (L"-PKD_Init(rc = %d\r\n", rc));
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
    PwrkeyDevice_t *pDevice = (PwrkeyDevice_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+PKD_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->Cookie != PKD_DEVICE_COOKIE))
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: PKD_Deinit: Incorrect context parameter\r\n"));
        goto cleanUp;
    }

    // Signal stop to threads
    pDevice->bIntrThreadExit = TRUE;
        
    // Close interrupt thread
    if (pDevice->hIntrThread != NULL)
    {
        // Set event to wake it
        SetEvent(pDevice->hIntrEvent);
        // Wait until thread exits
        WaitForSingleObject(pDevice->hIntrThread, INFINITE);
        // Close handle
        CloseHandle(pDevice->hIntrThread);
    }

    // Close interrupt handle
    if (pDevice->hIntrEvent != NULL) 
        CloseHandle(pDevice->hIntrEvent);

    if (pDevice->hTWL)
    {
        // close T2 driver
        TWLClose(pDevice->hTWL);
    }
    
    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-PKD_Deinit(rc = %d)\r\n", rc));
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
//  Function:  PKD_Open
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
    PwrkeyDevice_t *pDevice = (PwrkeyDevice_t*)context;

    pDevice->bWakeFromSuspend = TRUE;
    if (pDevice->hIntrEvent)
        SetEvent(pDevice->hIntrEvent);
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
    PwrkeyDevice_t *pDevice = (PwrkeyDevice_t*)context;
	BYTE data;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+PKD_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));
        
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->Cookie != PKD_DEVICE_COOKIE))
    {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PKD_IOControl: Incorrect context parameter\r\n"));
        goto cleanUp;
    }
    
	switch (code)
	{
		case IOCTL_PWRKEY_GETSTAT:
			// sanity check parameters
			if (pOutBuffer != NULL && outSize == sizeof(DWORD)) 
			{
				// pass back return values
				__try {
					data = 0;
					TWLReadByteReg(pDevice->hTWL, PMIC_REG_CHIPID, &data);
					*((PDWORD) pOutBuffer) = (DWORD)data;
					rc = TRUE;
				}
				__except(EXCEPTION_EXECUTE_HANDLER) {
					DEBUGMSG(ZONE_WARN, 
						(_T("exception in IOCTL_PWRKEY_GETSTAT\r\n")));
					rc = ERROR_INVALID_PARAMETER;
				}
			}
		break;
	}

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-PKD_IOControl(rc = %d)\r\n", rc));
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
    PwrkeyDevice_t *pDevice = (PwrkeyDevice_t*)pContext;
    BYTE data, change;
    
    DEBUGMSG(ZONE_IST, (L"+PKD_IntrThread\r\n"));

    // Loop until we are stopped...
    while (!pDevice->bIntrThreadExit)
    {

        // Wait for pwrkey interrupt or timeout
        WaitForSingleObject(pDevice->hIntrEvent, pDevice->DebounceTimeMs);

        if (pDevice->bIntrThreadExit) break;
        
        DEBUGMSG(ZONE_IST, (L"+PKD_IntrThread interrupt!\r\n"));

		data = 0;
		TWLReadByteReg(pDevice->hTWL, PMIC_REG_INTERRUPT, &data);
		data = 0;
		TWLReadByteReg(pDevice->hTWL, PMIC_REG_STATUS, &data);
		change = pDevice->oldStatus ^ data;

		if ((change & PMIC_STATUS_PB) && (data & PMIC_STATUS_PB))
		{
			RETAILMSG(1, (L"Power off button pressed!\r\n"));
			// full OFF on PWR_EN pin low
			TWLWriteByteReg(pDevice->hTWL, PMIC_REG_STATUS, PMIC_STATUS_OFF);
            // suspend system
            SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);
		}

		pDevice->oldStatus = data;

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

