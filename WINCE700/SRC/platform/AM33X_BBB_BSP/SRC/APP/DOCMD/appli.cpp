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
// include files section
//
#include <windows.h>

//
// pound defines section
//
#define SIZE_CMD    255

//
// types defintions section
//
typedef VOID (*PFN_FmtPuts)(WCHAR *s, ...);
typedef WCHAR* (*PFN_Gets)(WCHAR *s, int cch);

typedef BOOL (*ParseCmd)(LPWSTR, LPWSTR, PFN_FmtPuts, PFN_Gets);

//
// Main
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    PFN_FmtPuts pfnPut;
    PFN_Gets    pfnGet = NULL;
    HINSTANCE   hinstLib = NULL;
    HINSTANCE   hcoreLib = NULL;
    ParseCmd    ProcAddr;
    TCHAR       Command[20]; // Ususally, a command is only a few characters.
    TCHAR       Options[SIZE_CMD]; // rest of the command line
    TCHAR       *token;
    int         position;
    int         cmdLen = 0;
    
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

    NKDbgPrintfW(TEXT ("do it!: application built on %a at %a.\r\n" ), __DATE__, __TIME__ );

    SecureZeroMemory(Command, sizeof(Command));
    SecureZeroMemory(Options, sizeof(Options));
    
    cmdLen  = wcslen(lpCmdLine);
    if(0 == cmdLen)
    {
        NKDbgPrintfW(TEXT ("do it!: failed: no command!\r\n"));
        return -1;
    }
        
    token   = wcsstr(lpCmdLine, L" ");
    if(token == NULL)
    {
        wcsncpy_s(Command, _countof(Command), lpCmdLine, cmdLen);
    }
    else
    {
        position = (int)(token - lpCmdLine);
        if(position)
        {
            wcsncpy_s(Command, _countof(Command), lpCmdLine, position);
        }
        else
        {
            NKDbgPrintfW(TEXT ("do it!: failed: no valid command!\r\n"));
            return -1;
        }

        if(wcslen(&lpCmdLine[position]))
        {
            wcsncpy_s(Options, &lpCmdLine[position+1], wcslen(lpCmdLine)-position);
        }
        else
        {
            NKDbgPrintfW(TEXT ("do it!: failed: no valid command parameter!\r\n"));
            return -1;
        }
    }
    
    hinstLib = LoadLibrary(L"\\windows\\omap_shell.dll");
    if(hinstLib == NULL)
    {
        NKDbgPrintfW(TEXT ("do it!: failed to load omap_shell.dll\r\n"));
        return -1;
    }
    hcoreLib = LoadLibrary(L"\\windows\\coredll.dll");
    if(hcoreLib == NULL)
    {
        NKDbgPrintfW(TEXT ("do it!: failed to load coredll.dll\r\n"));
        FreeLibrary(hinstLib);
        return -1;
    }
    
    pfnPut = (PFN_FmtPuts)GetProcAddress(hcoreLib, TEXT("wprintf")); 
    
    ProcAddr = (ParseCmd)GetProcAddress(hinstLib, TEXT("ParseCommand")); 
    (ProcAddr)(Command, Options, pfnPut, pfnGet);
    
    FreeLibrary(hinstLib); 
    FreeLibrary(hcoreLib);

    return 0;
}