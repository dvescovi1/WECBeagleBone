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

    set.c

Abstract:

    Power Manager util to query/set the system power state.

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
    WCHAR  state[1024] = {0};
    LPWSTR pState = &state[0];
    TCHAR  *argv[20];
    int     argc;
    DWORD  dwErr=0, dwState=0;
    DWORD  dwStateFlags = 0;    
    DWORD  dwOptions = 0;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);
    
    //
    // parse command line
    //
    argc = CreateArgvArgc(TEXT( "PMSET" ), argv, lpCmdLine);
    if (argc > 1) {
        if (!swscanf(argv[1], TEXT("%d"), &dwState))
            dwState = 1;
        if (dwState == 0)
            pState = NULL;
        else if (!swscanf(argv[1], TEXT("%s"), &state))
            goto _error;
            
        if (argc > 2) {
            if (!swscanf(argv[2], TEXT("%x"), &dwStateFlags))
                goto _error;
        }
        
        if (argc > 3) {        
            if (!swscanf(argv[3], TEXT("%x"), &dwOptions))
                goto _error;
        }
    } else {
_error:    
        RetailPrint(TEXT("PMSET <State> <HexStateFlags> <HexOptions>\n"));
        RetailPrint(TEXT("  where <State> is system power state defined in the registry, e.g. RunAC\n"));
        RetailPrint(TEXT("    optional <StateFlags> specify state flags, in hex, e.g. 200000 (POWER_STATE_SUSPEND)\n"));
        RetailPrint(TEXT("    optional <Options> specify options, in hex, e.g. 1000 (POWER_FORCE)\n"));
        RetailPrint(TEXT("  Example: PMSET RunAC 1, sets the system state by name by name\n"));        
        RetailPrint(TEXT("  Example: PMSET 0 200000 1000, will force a suspend without knowing the state name\n"));
        exit(1);
    }

    dwErr = SetSystemPowerState(pState, dwStateFlags, dwOptions);
    if (ERROR_SUCCESS != dwErr) {
        RetailPrint(TEXT("SetSystemPowerState:'%s' ERROR:%d\n"), state, dwErr);
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
    swprintf(szTempFormat, L"PMSET: %s", pszFormat);
    pszFormat = szTempFormat;
    vswprintf(szTemp, pszFormat, al);
    RETAILMSG(1, (szTemp));

    va_end(al);
}

// EOF

#pragma warning(pop)