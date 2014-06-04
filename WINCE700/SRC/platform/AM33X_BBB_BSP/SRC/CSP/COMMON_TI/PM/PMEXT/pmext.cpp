// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  pmext.cpp
//
//  This file implements CE DDK DLL entry function.
//
#include <windows.h>
#include <ceddk.h>
#include <pm.h>
#include <pmext.h>
#include "_constants.h"
#include <_debug.h>

//-----------------------------------------------------------------------------
// singleton identifier
#define PMEXT_IDENTIFIER    (L"{A575EC68-6081-4f44-87C8-8AB41A9C2F2E}")

//-----------------------------------------------------------------------------
DWORD InitializeDeviceMediator(HKEY hKey, LPCTSTR lpRegistryPath);
VOID UninitializeDeviceMediator(DWORD dwExtContext);
DWORD InitializeConstraintRoot(HKEY hKey, LPCTSTR lpRegistryPath);
VOID UninitializeConstraintRoot(DWORD dwExtContext);
VOID UninitializePolicyRoot(DWORD dwExtContext);
DWORD InitializePolicyRoot(HKEY hKey, LPCTSTR lpRegistryPath);
DWORD PreSystemStateChange(DWORD dwExtContext, LPCTSTR lpNewStateName, DWORD dwFlags);
DWORD PostSystemStateChange(DWORD dwExtContext, LPCTSTR lpNewStateName, DWORD dwFlags);
    
//-----------------------------------------------------------------------------
HANDLE _hIdentifier = NULL;

//-----------------------------------------------------------------------------
// 
//  Function:  OmapPmExtensions
//
//  Entry point to retrieve omap power pm extension api's
//
BOOL
WINAPI
OmapPmExtensions(
    OMAP_PMEXT_IFC *pIfc,
    DWORD           size
    )
{
    // parameter validation
    if (pIfc == NULL || size != sizeof(OMAP_PMEXT_IFC)) return FALSE;
    
    // this module is a singleton and shouldn't be loaded more than once
    _hIdentifier = CreateEvent(NULL, FALSE, FALSE, PMEXT_IDENTIFIER);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
        CloseHandle(_hIdentifier);
        _hIdentifier = NULL;
        return FALSE;
        }
    
    pIfc->PmxSendDeviceNotification = PmxSendDeviceNotification;
    pIfc->PmxSetConstraintByClass = PmxSetConstraintByClass;
    pIfc->PmxSetConstraintById = PmxSetConstraintById;
    pIfc->PmxUpdateConstraint = PmxUpdateConstraint;
    pIfc->PmxRegisterConstraintCallback = (fnPmxRegisterConstraintCallback)PmxRegisterConstraintCallback;
    pIfc->PmxUnregisterConstraintCallback = PmxUnregisterConstraintCallback;
    pIfc->PmxOpenPolicy = PmxOpenPolicy;
    pIfc->PmxNotifyPolicy = PmxNotifyPolicy;
    pIfc->PmxClosePolicy = PmxClosePolicy;
    pIfc->PmxReleaseConstraint = PmxReleaseConstraint;
    pIfc->PmxLoadConstraint = PmxLoadConstraint;
    pIfc->PmxLoadPolicy = PmxLoadPolicy;
    return TRUE;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
VOID
WINAPI
PMExt_DeInit(
    DWORD dwExtContext
)
{
    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_DeInit(dwExtContext=0x%08X)", dwExtContext));

    UninitializePolicyRoot(dwExtContext);
    UninitializeConstraintRoot(dwExtContext);
    UninitializeDeviceMediator(dwExtContext);

    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_DeInit()"));
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
DWORD
WINAPI
PMExt_Init(
    HKEY hKey,
    LPCTSTR lpRegistryPath
    )
{
    DWORD rc = FALSE;
    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_Init(hKey=0x%08X, "
        L"lpRegistryPath=\"%s\")", hKey, lpRegistryPath)
        );

    InitializeDeviceMediator(hKey, lpRegistryPath);
    InitializeConstraintRoot(hKey, lpRegistryPath);
    InitializePolicyRoot(hKey, lpRegistryPath);
    
    rc = TRUE;
    
    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_Init()=%d", rc));
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
DWORD
WINAPI
PMExt_GetNotificationHandle(
    DWORD dwExtContext
    )
{
    DWORD rc = FALSE;

	UNREFERENCED_PARAMETER(dwExtContext);

    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_GetNotificationHandle("
        L"dwExtContext=0x%08X)", dwExtContext)
        );

    rc = (DWORD)CreateEvent(NULL, FALSE, FALSE, NULL);

    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_GetNotificationHandle()=0x%08X", rc));
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
DWORD
WINAPI
PMExt_EventNotification(
    DWORD dwExtContext, 
    PLATFORM_ACTIVITY_EVENT platformActivityEvent
    )
{
    DWORD rc = FALSE;

	UNREFERENCED_PARAMETER(dwExtContext);
	UNREFERENCED_PARAMETER(platformActivityEvent);

    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_EventNotification(dwExtContext=0x%08X, "
        L"platformActivityEvent=%d)", dwExtContext, platformActivityEvent)
        );


    rc = TRUE;
    
    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_EventNotification()=%d", rc));
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
DWORD
WINAPI
PMExt_PMBeforeNewSystemState(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    DWORD rc;
    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_PMBeforeNewSystemState(dwExtContext=0x%08X, "
        L"lpNewStateName=%d, dwFlags=0x%08X)", 
        dwExtContext, lpNewStateName, dwFlags)
        );

    rc = PreSystemStateChange(dwExtContext, lpNewStateName, dwFlags);
    
    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_PMBeforeNewSystemState()=%d", rc));
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
DWORD
WINAPI
PMExt_PMAfterNewSystemState(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    DWORD rc;
    RETAILMSG(ZONE_FUNCTION, (L"+PMExt_PMAfterNewSystemState(dwExtContext=0x%08X, "
        L"lpNewStateName=%d, dwFlags=0x%08X)", 
        dwExtContext, lpNewStateName, dwFlags)
        );

    rc = PostSystemStateChange(dwExtContext, lpNewStateName, dwFlags);
    
    RETAILMSG(ZONE_FUNCTION, (L"-PMExt_PMAfterNewSystemState()=%d", rc));
    return rc;
}
       
//-----------------------------------------------------------------------------
// 
//  Function:  DllEntry
//
//  The DLL entry function
//
extern "C"
BOOL WINAPI
DllEntry(
    HANDLE hInstance,
    DWORD reason,
    LPVOID pvReserved
    )
{
    UNREFERENCED_PARAMETER(pvReserved);
    if (reason == DLL_PROCESS_ATTACH)
        {
        RETAILREGISTERZONES((HMODULE)hInstance);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        }
    return TRUE;
}

//------------------------------------------------------------------------------
