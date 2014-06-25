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

    req.c

Abstract:

    Power Manager (PM) util to set a device power requirement.

Notes:

    Use in conjunction with the other PM utils.

Revision History:

--*/
#pragma warning(push)
#pragma warning(disable : 4115 6067)


#include <windows.h>
#include <msgqueue.h>
#include <pm.h>

extern int	CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);

void RetailPrint(wchar_t *pszFormat, ...);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    WCHAR  device[1024] = {0};
    //LPWSTR pDevice = &device[0];
    DWORD  dwDeviceState = 0;
    DWORD  dwDeviceFlags = 0;    
    
    WCHAR  state[1024] = {0};
    //LPWSTR pState = &state[0];
    DWORD  dwStateFlags = 0;  

    HANDLE h = NULL;
    int    release = 0;

    WCHAR  szEventName[MAX_PATH];

    TCHAR  *argv[20];   
    int     argc;
    DWORD  dwErr=0;

    BOOL fGotState = FALSE;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);
    
    //
    // parse command line
    //
    argc = CreateArgvArgc(TEXT( "PMREQ" ), argv, lpCmdLine);
    switch (argc) {
        case 2:
            if (!swscanf(argv[1], TEXT("%p"), &h))
                dwErr = 1;
            release = 1;
            break;
            
        case 4:
            if (!swscanf(argv[1], TEXT("%s"), &device))
                dwErr = 1;
            if (!swscanf(argv[2], TEXT("%x"), &dwDeviceState))
                dwErr = 1;
            if (!swscanf(argv[3], TEXT("%x"), &dwDeviceFlags))
                dwErr = 1;
            break;
            
        case 6:
            if (!swscanf(argv[1], TEXT("%s"), &device))
                dwErr = 1;
            if (!swscanf(argv[2], TEXT("%x"), &dwDeviceState))
                dwErr = 1;
            if (!swscanf(argv[3], TEXT("%x"), &dwDeviceFlags))
                dwErr = 1;
            if (!swscanf(argv[4], TEXT("%s"), &state))
                dwErr = 1;;
            if (!swscanf(argv[5], TEXT("%x"), &dwStateFlags))
                dwErr = 1;
            fGotState = TRUE;
            break;
            
        default:
            dwErr = 1;
            break;
    }

    if (dwErr) {
        RetailPrint  (TEXT("PMREQ <Device> <DeviceState> <DeviceFlags> [[<SystemState>] <SystemFlags>]\n"));
        RetailPrint  (TEXT("  Places a specific power requirement on a device.\n"));                
        RetailPrint  (TEXT("    <Device> names the deive, e.g. PDX1:\n"));        
        RetailPrint  (TEXT("    <DeviceState> specifies the device state (Dx), e.g. 4\n"));
        RetailPrint  (TEXT("    <DeviceFlags> specifies the device flags in hex, e.g. POWER_NAME = 0x1\n"));
        RetailPrint  (TEXT("    <SystemState> (OPTIONAL) names the system power state, e.g. LowPowerAC\n"));
        RetailPrint  (TEXT("    <SystemFlags> (OPTIONAL) specifies the system power state flags, e.g. POWER_FORCE = 0x1000\n"));                
        RetailPrint  (TEXT("Device names may be qualified with the device's class GUID.  This is\n"));
        RetailPrint  (TEXT("required for devices that are not members of the default power-manageable\n"));
        RetailPrint  (TEXT("device class\n"));

        RetailPrint  (TEXT("PMREQ <handle>\n"));
        RetailPrint  (TEXT("  Releases a previously established requirement\n"));
        RetailPrint  (TEXT("    where <handle> is the return HANDLE from a previous invocation of PMREQ\n\n"));
        
        RetailPrint  (TEXT("  Example: PMREQ PDX1: 1 1 (requires that PDX1: be at D1 or higher in all system power states)\n"));
        RetailPrint  (TEXT("           PMREQ PDX1: 1 1 SystemIdle 0 (requires that PDX1: be at D1 or higher in the SystemIdle system power state\n"));
        RetailPrint  (TEXT("           PMREQ PDX1: 1 1001 (requires that PDX1: be at D1 or higher in all system power states,\n"));
        RetailPrint  (TEXT("                              even when the system is suspended; not all drivers actually stay powered\n"));
        RetailPrint  (TEXT("                              during suspend)\n"));
        RetailPrint  (TEXT("           PMREQ {A32942B7-920C-486b-B0E6-92A702A99B35}\\PDX1: 1 1 (requires that PDX1: be at D1 or higher in all system power states)\n"));
        RetailPrint  (TEXT("           PMREQ {98C5250D-C29A-4985-AE5F-AFE5367E5006}\\NE20001 0 1 (requires that NDIS miniport NE20001 be at D0 in all system power states)\n"));
        exit(1);
    }

    if (!release) {
        // set the requirement
        if(!fGotState) {
            h = SetPowerRequirement(device, dwDeviceState, dwDeviceFlags, NULL, 0);
        } else {
            h = SetPowerRequirement(device, dwDeviceState, dwDeviceFlags, state, dwStateFlags);
        }
        if (h) {
            HANDLE hevWakeEvent;
            DWORD dwProcess = GetCurrentProcessId();
            
            // create an event so we can be signaled to wake up and release the requirement
            wsprintf(szEventName, _T("PMREQ_Wake_Event_%x"), dwProcess);
            hevWakeEvent = CreateEvent(NULL, FALSE, FALSE, szEventName);
            if(hevWakeEvent != NULL) {
                RetailPrint  (TEXT("Use 'PMREQ %x' to release the requirement\n"), dwProcess);
                WaitForSingleObject(hevWakeEvent, INFINITE);
                CloseHandle(hevWakeEvent);
            } else {
                RetailPrint  (TEXT("PMREQ!CreateEvent('%s') failed %d\n"), szEventName);
            }

            // release the requirement
            dwErr = ReleasePowerRequirement(h);
            if(dwErr != ERROR_SUCCESS) {
                RetailPrint  (TEXT("PMREQ!ReleasePowerRequirement(0x%08x) failed %d\n"), h, dwErr);
            } else {
                RetailPrint  (TEXT("PMREQ!ReleasePowerRequirement(0x%08x) succeeded\n"), h);
            }
        } else {
            dwErr = GetLastError();
            RetailPrint  (TEXT("PMREQ!SetPowerRequirement(%s, %x, 0x%x, %s, 0x%x) ERROR:%d\n"), 
                device, dwDeviceState, dwDeviceFlags, state, dwStateFlags, dwErr);
        }
    } else {
        HANDLE hevWakeEvent;

        // signal an event to wake up the process holding a requirement
        wsprintf(szEventName, _T("PMREQ_Wake_Event_%p"), h);
        hevWakeEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName);
        if(hevWakeEvent == NULL) {
            RetailPrint  (TEXT("PMREQ!OpenEvent('%s') failed %d\r\n"), szEventName, GetLastError());
        } else {
            SetEvent(hevWakeEvent);
            CloseHandle(hevWakeEvent);
        }
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


// EOF

#pragma warning(pop)
