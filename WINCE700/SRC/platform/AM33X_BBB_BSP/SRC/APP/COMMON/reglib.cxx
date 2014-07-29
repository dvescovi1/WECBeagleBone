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
//  File: reglib.cxx
//
#include <windows.h>
#include <devload.h>
#include <ceddkex.h>
#include "reglib.h"

//------------------------------------------------------------------------------

typedef enum RegToken
{
    kKeyName,
    kBackSlash,
    kEnd,
    kError
};

//------------------------------------------------------------------------------

static RegToken GetToken(WCHAR const* pch, WCHAR *pchDest, UINT *pnTokenLen)
{
    UINT nOffset = 0;
    RegToken token;
    
    // check for single character tokens
    if (*pch == L'\\')
        {
        token = kBackSlash;
        nOffset = 1;
        }
    else if (*pch == L'\0')
        {
        token = kEnd;
        nOffset = 0;
        }
    else
        {
        int i;
        token = kError;
        BOOL bCharFound = FALSE;
        for (i = 0; i < MAX_PATH; ++i)
            {
            if (*pch == L'\\')
                {
                // hit end of token
                *pchDest = L'\0';
                break;
                }
            else if (*pch == L'\0')
                {
                // hit end of string
                *pchDest = L'\0';
                break;                
                }
            else if (*pch != L' ')
                {
                bCharFound = TRUE;                
                }
            *pchDest++ = *pch++;
            nOffset++;
            }
        
        if (bCharFound == TRUE) token = kKeyName;
        }
    *pnTokenLen = nOffset;
    return token;
}

//------------------------------------------------------------------------------

static HKEY CreateRegKeyHelper(HKEY hKey, WCHAR const *lpSubKey)
{
    DWORD dwDisp;
    HKEY hNewKey;

    LONG nRet = RegCreateKeyEx(hKey, lpSubKey, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, 0, NULL, &hNewKey, &dwDisp);

    if (nRet != ERROR_SUCCESS) hNewKey = NULL;
    return hNewKey;
}

//------------------------------------------------------------------------------

static DWORD
SetStringParam(
    HKEY hKey,
    VOID *pBase,
    const DEVICE_WRITEREGISTRY_PARAM *pParam
    )
{
    DWORD status, size, type;
    WCHAR *pName;
    UCHAR *pValue;

    // check if no overwrite then check if key already exists
    pName = pParam->name;
    if (pParam->overwrite == FALSE)
        {
        status = RegQueryValueEx(hKey, pName, NULL, &type, NULL, &size);
        if (status == ERROR_SUCCESS) goto cleanUp;
        }
    
    pValue = (UCHAR*)pBase + pParam->offset;
    size = wcslen((WCHAR*)pValue) * sizeof(WCHAR);
    type = pParam->type;

    status = RegSetValueEx(hKey, pName, 0, type, pValue, size);

cleanUp:
    return status;
}

//------------------------------------------------------------------------------

static DWORD
SetBlobParam(
    HKEY hKey,
    VOID *pBase,
    const DEVICE_WRITEREGISTRY_PARAM *pParam
    )
{
    DWORD status, size, type;
    WCHAR *pName;
    UCHAR *pValue;

    // check if no overwrite then check if key already exists
    pName = pParam->name;
    if (pParam->overwrite == FALSE)
        {
        status = RegQueryValueEx(hKey, pName, NULL, &type, NULL, &size);
        if (status == ERROR_SUCCESS) goto cleanUp;
        }
    
    type = pParam->type;
    pValue = (UCHAR*)pBase + pParam->offset;
    size = pParam->size;
    status = RegSetValueEx(hKey, pName, 0, type, pValue, size);

cleanUp:
    return status;
}

//------------------------------------------------------------------------------

HKEY CreateRegKey(HKEY hKeyRoot, WCHAR const *szPath)
{
    HKEY hKeyNew = NULL;
    HKEY hKey = hKeyRoot;
    RegToken token = kBackSlash;
    UINT nNextToken;
    WCHAR szBuffer[MAX_PATH];
    WCHAR const *pch = szPath;
    LONG status = ERROR_INVALID_DATA;
    
    for(;;)
        {
        token = GetToken(pch, szBuffer, &nNextToken);
        switch (token)
            {
            case kBackSlash:
                pch++;
                break;

            case kKeyName:
                // create key using the key name
                hKeyNew = CreateRegKeyHelper(hKey, szBuffer);
                if (hKeyNew == NULL) goto cleanUp; 

                // close old key and use new one
                RegCloseKey(hKey);
                hKey = hKeyNew;

                // flag indicating key was created
                status = ERROR_SUCCESS;
                pch += nNextToken;
                break;

            case kEnd:
                goto cleanUp;
                
            case kError:
                status = ERROR_INVALID_DATA;
                goto cleanUp;
            }
        }

cleanUp:
    if (status != ERROR_SUCCESS) 
        {
        RegCloseKey(hKey);
        hKey = NULL;
        }
    
    return hKey;
}

//------------------------------------------------------------------------------

DWORD
WriteRegistryParams(
    HKEY hKeyRoot,
    WCHAR const*szPath,
    VOID *pBase,
    DWORD count, 
    const DEVICE_WRITEREGISTRY_PARAM params[]
    )
{
    DWORD status = ERROR_SUCCESS;
    HKEY hKey;
    DWORD i;

    // Open registry context to read parameters
    if ((hKey = CreateRegKey(hKeyRoot, szPath)) == NULL)
        {
        return ERROR_OUTOFMEMORY;
        }

    // For all members of array
    for (i = 0; i < count && status == ERROR_SUCCESS; i++)
        {
            switch (params[i].type)
            {
            case REG_SZ:
                status = SetStringParam(hKey, pBase, &params[i]);
                break;

            default:
                status = SetBlobParam(hKey, pBase, &params[i]);
            }
        }

    // Close key
    RegCloseKey(hKey);
    return status;
}

//------------------------------------------------------------------------------

