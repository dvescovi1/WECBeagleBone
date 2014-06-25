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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/*++


Module Name:

    getd.c

Abstract:

    Power Manager util to query device power state.

Notes:

Revision History:

--*/

#pragma warning(push)
#pragma warning(disable : 4115 6067)
#include <windows.h>
#include <msgqueue.h>
#include <pm.h>

void RetailPrint(wchar_t *pszFormat, ...);
extern int	CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    WCHAR  device[1024] = {0};
    TCHAR  *argv[20];
    int     argc;
    DWORD  dwErr=0;
    DWORD  dwFlags = 0;
    
    CEDEVICE_POWER_STATE dwState=PwrDeviceUnspecified;
    
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);
    
    //
    // parse command line
    //
    argc = CreateArgvArgc(TEXT( "PMGETD" ), argv, lpCmdLine);

    switch (argc) {
        case 3:
            if (!swscanf(argv[1], TEXT("%s"), &device))
                dwErr = 1;
            if (!swscanf(argv[2], TEXT("%x"), &dwFlags))
                dwErr = 1;
            break;

        default:
            dwErr=1;
            break;
    }

    if (dwErr) {    
        RetailPrint(TEXT("PMGETD <Device> <HexDeviceFlags>\n"));
        RetailPrint(TEXT("  where <Device> describes the device, e.g. DDI1:\n"));
        RetailPrint(TEXT("  where <DeviceFlags> specify device flags, in hex, e.g. 1 (POWER_NAME)\n"));
        RetailPrint(TEXT("Device names may be qualified with the device's class GUID.  This is\n"));
        RetailPrint(TEXT("required for devices that are not members of the default power-manageable\n"));
        RetailPrint(TEXT("device class\n"));
        RetailPrint(TEXT("  Example: PMGETD DDI1: 1001 (gets DDI1: device state, forcing an IOCTL to the driver)\n"));        
        RetailPrint(TEXT("           PMGETD DDI1: 1 (gets DDI1: device state)\n"));
        RetailPrint(TEXT("           PMGETD {A32942B7-920C-486b-B0E6-92A702A99B35}\\DDI1: 1 (gets DDI1: device state)\n"));
        RetailPrint(TEXT("           PMGETD {98C5250D-C29A-4985-AE5F-AFE5367E5006}\\NE20001 1 (gets the NDIS miniport NE20001 device state)\n"));
        exit(1);
    }
    
    dwErr = GetDevicePower(device, dwFlags, &dwState);
    if(dwErr != ERROR_SUCCESS) {
        RetailPrint(TEXT("GetDevicePower('%s', 0x%x) ERROR:%d\n"), device, dwFlags, dwErr);
    } else {
        RetailPrint(TEXT("GetDevicePower('%s', 0x%x) succeeded, device state is %d\n"), device, dwFlags, dwState);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    wchar_t szTemp[2048];
    wchar_t szTempFormat[2048];

    va_start(al, pszFormat);

    // Show message on console
    vwprintf(pszFormat, al);

    // Show message on RETAILMSG
    swprintf(szTempFormat, L"PMGETD: %s", pszFormat);
    pszFormat = szTempFormat;
    vswprintf(szTemp, pszFormat, al);
    RETAILMSG(1, (szTemp));

    va_end(al);
}

#pragma warning(pop)

// EOF
