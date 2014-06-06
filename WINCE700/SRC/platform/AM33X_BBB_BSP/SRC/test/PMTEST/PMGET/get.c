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

    get.c

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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    WCHAR  state[1024] = {0};
    LPWSTR pState = &state[0];
    DWORD dwBufChars = (sizeof(state) / sizeof(state[0]));
    DWORD  dwStateFlags = 0;
    DWORD dwErr;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);
	UNREFERENCED_PARAMETER(lpCmdLine);
    
    dwErr = GetSystemPowerState(pState, dwBufChars, &dwStateFlags);
    if (ERROR_SUCCESS != dwErr) {
        RetailPrint (TEXT("PMGET!GetSystemPowerState:ERROR:%d\n"), dwErr);
    } else {
        RetailPrint (TEXT("PMGET! System Power state is '%s', flags 0x%08x\n"), state, dwStateFlags);
        switch (POWER_STATE(dwStateFlags)) {
        case POWER_STATE_ON:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_ON\n"));
            break;
        case POWER_STATE_OFF:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_OFF\n"));
            break;
        case POWER_STATE_CRITICAL:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_CRITICAL\n"));
            break;
        case POWER_STATE_BOOT:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_BOOT\n"));
            break;
        case POWER_STATE_IDLE:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_IDLE\n"));
            break;
        case POWER_STATE_SUSPEND:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_SUSPEND\n"));
            break;
        case POWER_STATE_RESET:
            RetailPrint (TEXT("PMGET!\tPOWER_STATE_RESET\n"));
            break;
        case 0:
            break;
        default:
            RetailPrint(TEXT("PMGET!\tUnknown Power State Flags:0x%x\n"),dwStateFlags);
            ASSERT(0);
            break;
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