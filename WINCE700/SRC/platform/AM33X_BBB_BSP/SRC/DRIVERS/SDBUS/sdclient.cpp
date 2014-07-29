//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDClient.cpp
Abstract:
    SDBus Client Driver Interface Implementation.

Notes: 
--*/
#include <windows.h>
#include <bldver.h>
#include <cesdbus.h>
#include <marshal.hpp>

#include <sdhcd.h>

#include "sdbus.hpp"


///////////////////////////////////////////////////////////////////////////////
//  SDC_Init - the init entry point for the bus driver
//  Input:  dwContext - the context for this init
//  Output:
//  Return:  returns a non-zero value (pointer to the bus driver instance)
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD SDC_Init(DWORD dwContext)
{
    LPCTSTR         pszActiveKey = (LPCTSTR) dwContext;
    DBG_UNREFERENCED_LOCAL_VARIABLE(pszActiveKey);

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: +SDC_Init\n")));
    DWORD dwRet = (DWORD) CSDHostContainer::DriverInit((LPCTSTR) dwContext);
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: Bus Driver instance created : 0x%08X ! -SDC_Init \n"), dwRet));

    // return pointer to bus driver object as a context
    return (DWORD)dwRet;
}

///////////////////////////////////////////////////////////////////////////////
//  SDC_PreDeinit - the predeinit entry point for the SD bus driver
//  Input:  hDeviceContext - the context returned from SDC_Init
//  Output:
//  Notes:  always returns TRUE.
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SDC_PreDeinit(DWORD /*hDeviceContext*/)
{
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: +SDC_PreDeinit\n")));

    // Nothing to do here. Just make sure that the device manager won't
    // call Deinit until everyone is out of SDC_IOControl.

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: -SDC_PreDeinit\n")));
    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
//  SDC_Deinit - the deinit entry point for the SD bus driver
//  Input:  hDeviceContext - the context returned from SDC_Init
//  Output:
//  Notes:  always returns TRUE.
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SDC_Deinit(DWORD hDeviceContext)
{
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: +SDC_Deinit\n")));
    CSDHostContainer::DriverDeInit((PVOID)hDeviceContext);
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: -SDC_Deinit\n")));
    return TRUE;
}

static DWORD dwBusAccess = 1;
static DWORD dwDeviceAccess = 0;

///////////////////////////////////////////////////////////////////////////////
//  SDC_Open - the open entry point for the bus driver
//  Input:  hDeviceContext - the device context from SDC_Init
//          AccessCode - the desired access
//          ShareMode - the desired share mode
//  Output:
//  Return: returns open context (in this case, just the context from SDC_Init)
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C"
HANDLE SDC_Open(HANDLE    /*hDeviceContext*/,
               DWORD    AccessCode,
               DWORD    /*ShareMode*/)
{
    HANDLE hRet = NULL ;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDBusDriver: +-SDC_Open\n")));
    
    if (CSDHostContainer::GetHostContainer ()) {
        if (AccessCode & DEVACCESS_BUSNAMESPACE) {
            // This is a bus access
            hRet = (HANDLE)&dwBusAccess;
        }
        else {
            // Standard access
            hRet = (HANDLE)&dwDeviceAccess;
        }
    }

    return hRet;
}
///////////////////////////////////////////////////////////////////////////////
//  SDC_Close - the close entry point for the bus driver
//  Input:  hOpenContext - the context returned from SDC_Open
//  Output:
//  Notes:  always returns TRUE
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SDC_Close(DWORD /*hOpenContext*/)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDBusDriver: +-SDC_Close\n")));

    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
//  SDC_IOControl - the I/O control entry point for the bus driver
//  Input:  hOpenContext - the context returned from SDC_Open
//          dwCode - the ioctl code
//          pBufIn - the input buffer from the user
//          dwLenIn - the length of the input buffer
//          pBufOut - the output buffer from the user
//          dwLenOut - the length of the output buffer
//          pdwActualOut - the size of the transfer
//  Output:
//  Return: TRUE if ioctl was successfully handled
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SDC_IOControl(HANDLE   hOpenContext,
                   DWORD   dwCode,
                   PBYTE   pBufIn,
                   DWORD   dwLenIn,
                   PBYTE   pBufOut,
                   DWORD   dwLenOut,
                   PDWORD  pdwActualOut)
{

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDBusDriver: +SDC_IOControl\n")));
    BOOL fRet = FALSE;
    if (CSDHostContainer::GetHostContainer()!=NULL && hOpenContext!=NULL) {        
        if ((*(PDWORD)hOpenContext)!= 0 ) {
            fRet = CSDHostContainer::GetHostContainer()->IOControl(dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        else {
            fRet = CSDHostContainer::GetHostContainer()->SDC_IOControl(dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
            
    } 
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
        fRet = FALSE;
    }
    if (fRet == FALSE) {
        DEBUGMSG(SDCARD_ZONE_ERROR, 
            (TEXT("ERROR: SDC_IOControl failed processing of IOCTL:%08X error:%u\n"),
            dwCode, GetLastError()));
    }
    return (fRet);
}
    


// Fill in the host controller's function table.
extern "C"
SD_API_STATUS
SDHCDGetHCFunctions(
                    PSDHOST_API_FUNCTIONS pFunctions
                    )
{
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHCDGetHCFunctions: +Init\n")));

    if ( pFunctions && (pFunctions->dwSize == sizeof(*pFunctions)) ) {
        pFunctions->pAllocateContext = &CSDHostContainer::SDHCDAllocateContext__X;
        pFunctions->pDeleteContext = &CSDHostContainer::SDHCDDeleteContext__X;
        pFunctions->pRegisterHostController = &CSDHostContainer::SDHCDRegisterHostController__X;
        pFunctions->pDeregisterHostController = &CSDHostContainer::SDHCDDeregisterHostController__X;
        pFunctions->pIndicateSlotStateChange = &CSDHostContainer::SDHCDIndicateSlotStateChange__X;
        pFunctions->pIndicateBusRequestComplete = &CSDHostContainer::SDHCDIndicateBusRequestComplete__X;
        pFunctions->pUnlockRequest = &CSDHostContainer::SDHCDUnlockRequest__X;
        pFunctions->pGetAndLockCurrentRequest = &CSDHostContainer::SDHCDGetAndLockCurrentRequest__X;
        pFunctions->pPowerUpDown = &CSDHostContainer::SDHCDPowerUpDown__X;

        status = SD_API_STATUS_SUCCESS;
    }
    else {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDGetHCFunctions: Invalid parameter\n")));
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHCDGetHCFunctions: -Init\n")));

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  DllMain - the main dll entry point
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output:
//  Return: Always returns TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL WINAPI DllEntry(HANDLE     hInstance,
              DWORD      Reason,
              LPVOID     /*pReserved*/)
{
    BOOL fRet = TRUE;

    if ( Reason == DLL_PROCESS_ATTACH ) {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: PROCESS_ATTACH \n")));
        DisableThreadLibraryCalls((HMODULE) hInstance);
        SD_DEBUG_ZONE_REGISTER((HMODULE)hInstance, NULL);
    }
    else if ( Reason == DLL_PROCESS_DETACH ) {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDBusDriver: PROCESS_DETACH \n")));
    }

    return fRet;
}


