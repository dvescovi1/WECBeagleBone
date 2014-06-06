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
//  File: driverreg.cxx
//
#include <windows.h>
#include <devload.h>
#include <ceddkex.h>

//------------------------------------------------------------------------------
typedef DWORD (*pDllRegisterDriver)();

//------------------------------------------------------------------------------
// Dynamically registers a driver
//
DWORD RegisterDriver(WCHAR *szDriver)
{
    DWORD dwRet;
    pDllRegisterDriver fnDllRegisterDriver;
    HMODULE hLib = (HMODULE)LoadLibrary(szDriver);
    if (hLib == INVALID_HANDLE_VALUE)
        {
        dwRet = GetLastError();
        goto cleanUp;
        }

    fnDllRegisterDriver = (pDllRegisterDriver)GetProcAddress(hLib, L"DllRegisterDriver");
    if (fnDllRegisterDriver != NULL)
        {
        fnDllRegisterDriver();
        }

    dwRet = GetLastError();
    FreeLibrary(hLib);

cleanUp:
    return dwRet;
}

