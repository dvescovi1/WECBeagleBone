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
//  File: reglib.h
//

//------------------------------------------------------------------------------
//
//  Function:  WriteRegistryParams
//
//  This function writes registery parameters for self registering drivers
//  and/or service modules
//
typedef struct {
    LPTSTR name;
    DWORD  type;
    BOOL   overwrite;
    DWORD  offset;
    DWORD  size;
    PVOID  pValue;
} DEVICE_WRITEREGISTRY_PARAM;

//------------------------------------------------------------------------------
    
HKEY 
CreateRegKey(
    HKEY hKeyRoot, 
    WCHAR const *szPath
    );
    
//------------------------------------------------------------------------------

/*
Usage Example:

typedef struct {
    WCHAR       szPrefix[16];
    WCHAR       szDLL[32];
    DWORD       nIndex;
    DWORD       nOrder;
} RegInit_t;

static const DEVICE_WRITEREGISTRY_PARAM s_deviceWriteRegParams[] = 
{
    {
        L"Prefix", REG_SZ, FALSE, offset(RegInit_t, szPrefix), 
        fieldsize(RegInit_t, szPrefix)
    }, {
        L"Dll", REG_SZ, FALSE, offset(RegInit_t, szDLL), 
        fieldsize(RegInit_t, szDLL)
    }, {
        L"Index", REG_DWORD, FALSE, offset(RegInit_t, nIndex), 
        fieldsize(RegInit_t, nIndex)
    }, {
        L"Order", REG_DWORD, FALSE, offset(RegInit_t, nOrder), 
        fieldsize(RegInit_t, nOrder)
    } 
};

//------------------------------------------------------------------------------
//
//  Function:  DllRegisterDriver
//
//  Self registration mechanism.
//
DWORD DllRegisterDriver()
{    
    RegInit_t *pRegInit = new RegInit_t();

    pRegInit->nOrder = 25;
    pRegInit->nIndex = 1;
    wcscpy(pRegInit->szDLL, L"some_svc.dll");
    wcscpy(pRegInit->szPrefix, L"SVC");

    DWORD dw = WriteRegistryParams(HKEY_LOCAL_MACHINE, L"\\Drivers\\SomeService",
        pRegInit, dimof(s_deviceWriteRegParams), s_deviceWriteRegParams);
    
    delete pRegInit;
    return dw;
}
*/

DWORD
WriteRegistryParams(
    HKEY hKeyRoot,
    WCHAR const *szPath,
    VOID *pBase,
    DWORD count, 
    const DEVICE_WRITEREGISTRY_PARAM params[]
    );

//------------------------------------------------------------------------------




