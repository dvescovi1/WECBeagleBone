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
//  File:  constraintcontainer.cpp
//

#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <constraintadapter.h>
#include <constraintcontainer.h>
#include <vector.hxx>


//------------------------------------------------------------------------------
//
//  Method:  ConstraintContainer::Uninitialize
//
void
ConstraintContainer::
Uninitialize()
{
    // Look for device by walking through list
    while (empty() == FALSE)
    {
        (*begin())->Uninitialize();
        delete (*begin());
        erase(begin());

    }
}

//------------------------------------------------------------------------------
//
//  Method:  ConstraintContainer::Initialize
//
BOOL
ConstraintContainer::
Initialize(
    LPCWSTR szContext
    )
{
    BOOL rc = FALSE;
    HKEY hKey = NULL;
    LONG code;
    DWORD size;
    DWORD subKeys;    
    int nRootKeyLen;
    iterator iAdapter;
    ConstraintAdapter *pAdapter;
    _TCHAR szBuffer[MAX_PATH];
    _TCHAR szSubKeyBuffer[MAX_PATH * 2];


    // Open template registry key
    code = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szContext, 0, 0, &hKey);
    if (code != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    // copy text into buffer
    nRootKeyLen = _tcslen(szContext);
    memcpy(szSubKeyBuffer, szContext, nRootKeyLen * sizeof(_TCHAR));
    szSubKeyBuffer[nRootKeyLen]='\0';

    // append '\' to end of string

    _tcscat_s(szSubKeyBuffer, _countof(szSubKeyBuffer), L"\\");

    ++nRootKeyLen;

    // Find number of subkeys
    code = RegQueryInfoKey(hKey, 0, 0, 0, &subKeys, 0, 0, 0, 0, 0, 0, 0);
    if (code != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    for (DWORD index = 0; index < subKeys; index++)
        {
        // Get subkey name
        size = MAX_PATH;
        code = RegEnumKeyEx(hKey, index, szBuffer, &size, 0, 0, 0, 0);
        if (code != ERROR_SUCCESS)
            {
            continue;
            }

        // Create child device object
        pAdapter = new ConstraintAdapter();
        if (pAdapter == NULL)
            {
            continue;
            }

        // create a copy of key

        //_tcscat_s(szSubKeyBuffer,_countof(szSubKeyBuffer), szBuffer);
        _tcscpy_s(szSubKeyBuffer + nRootKeyLen, MAX_PATH*2, szBuffer);


        // Initialize child device object
        if (!pAdapter->Initialize(szSubKeyBuffer))
            {
            // This will result in delete
            delete pAdapter;
            continue;
            }
        
        // Add child object to array based on order 
        for (iAdapter = begin(); 
             iAdapter != end() && (*iAdapter)->GetLoadOrder() < pAdapter->GetLoadOrder();
             ++iAdapter
             );

        // Insert 
        insert(iAdapter, pAdapter);        
    }
    
    rc = TRUE;
    
cleanUp:
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method:  ConstraintContainer::PostInitialize
//
BOOL
ConstraintContainer::
PostInitialize()
{
    // Look for device by walking through list
    int nCount = size();
    iterator iAdapterErase;
    iterator iAdapter = begin();    
    while (nCount > 0)
        {
        if ((*iAdapter)->PostInitialize() == FALSE)
            {
            iAdapterErase = iAdapter;
            iAdapter++;

            (*iAdapterErase)->Uninitialize();
            delete *iAdapterErase;
            erase(iAdapterErase);
            }
        else
            {
            iAdapter++;
            }
        --nCount;
        }
        
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method:  ConstraintContainer::FindAdapterById
//
ConstraintAdapter*
ConstraintContainer::
FindAdapterById(
    LPCWSTR szId
    )
{
    ConstraintAdapter *pAdapter = NULL;
    if (szId == NULL) return NULL;

    // Look for device by walking through list
    for (iterator iAdapter = begin(); iAdapter != end(); iAdapter++)
        {
        if ((*iAdapter)->IsAdapterId(szId) != FALSE)
            {
            pAdapter = *iAdapter;
            break;
            }
        }
        
    return pAdapter;
}

//------------------------------------------------------------------------------
//
//  Method:  ConstraintContainer::FindChildByDeviceId
//
ConstraintAdapter*
ConstraintContainer::
FindAdapterByClass(
    UINT adapterClass
    )
{
    ConstraintAdapter *pAdapter = NULL;

    // Look for device by walking through list
    for (iterator iAdapter = begin(); iAdapter != end(); iAdapter++)
        {
        if ((*iAdapter)->IsAdapterClass(adapterClass) != FALSE)
            {
            pAdapter = *iAdapter;
            break;
            }
        }
        
    return pAdapter;
}

//------------------------------------------------------------------------------

