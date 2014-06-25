// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//

// Includes
#include <windows.h>
#include <tchar.h>
#include <cpl.h>
#include "resource.h"

// Defines
#define lengthof(exp) ((sizeof((exp))) / sizeof((*(exp))))
#define UNUSED_PARAMETER(x) x
#define EXE_PATH L"\\Windows\\bsppnl.exe"

// Globasl variables
HMODULE g_hModule = NULL;

extern "C" BOOL WINAPI DllEntry(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNUSED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = (HMODULE) hinstDLL;
        }
        break;
    }

    return TRUE;
}

extern "C" LONG CALLBACK CPlApplet(HWND hwndCPL, UINT  message, LPARAM lParam1, LPARAM lParam2)
{
    UNUSED_PARAMETER(hwndCPL);
    UNUSED_PARAMETER(lParam1);

    DWORD ret = 1;

    switch (message)
    {
        case CPL_INIT:
            ret = 1;
            break;

        case CPL_GETCOUNT:
            ret = 1;
            break;

        case CPL_NEWINQUIRE:
        {
            ASSERT(0 == lParam1);
            ASSERT(lParam2);

            NEWCPLINFO *lpNewCplInfo = (NEWCPLINFO *)lParam2;
            if (lpNewCplInfo)
            {
                lpNewCplInfo->dwSize = sizeof(NEWCPLINFO);
                lpNewCplInfo->dwFlags = 0;
                lpNewCplInfo->dwHelpContext = 0;
                lpNewCplInfo->lData = IDI_BSPPNLICON;
                lpNewCplInfo->hIcon = LoadIcon(g_hModule, MAKEINTRESOURCE(IDI_BSPPNLICON));
                LoadString(g_hModule, IDS_TITLE, lpNewCplInfo->szName, lengthof(lpNewCplInfo->szName));
                LoadString(g_hModule, IDS_INFO, lpNewCplInfo->szInfo, lengthof(lpNewCplInfo->szInfo));
                _tcscpy_s(lpNewCplInfo->szHelpFile,_countof(lpNewCplInfo->szHelpFile), _T(""));
                ret = 0;
                break;
            }
            ret = 1; // Failure
            break;
        }

        case CPL_DBLCLK:
        {
            PROCESS_INFORMATION pi = {0};
			STARTUPINFO si = {0};
            if (CreateProcess(EXE_PATH, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                ret = 0;
                break;
            }
            ret = 1;
            break;
        }

        case CPL_STOP:
        case CPL_EXIT:
        default:
            ret = 0;
            break;
    }

    return ret;
}
