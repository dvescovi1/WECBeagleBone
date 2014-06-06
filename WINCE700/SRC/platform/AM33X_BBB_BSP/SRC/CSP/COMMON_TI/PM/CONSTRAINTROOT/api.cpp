// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  api.cpp
//
//
#include <windows.h>
#include <pnp.h>
#include <pm.h>
#include <cregedit.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <omap_pmext.h>
#include "_constants.h"
#include "constraintadapter.h"
#include "constraintcontext.h"
#include "constraintroot.h"

//-----------------------------------------------------------------------------
// global variables
ConstraintRoot      *g_pConstraintRoot = NULL;

//-----------------------------------------------------------------------------
// 
//  Function:  UninitializeConstraintRoot
//
//  Power management extension uninitialization routine
//
VOID
UninitializeConstraintRoot(
    DWORD dwExtContext
    )
{
    UNREFERENCED_PARAMETER(dwExtContext);
    // delete constraint root if initialization failed
    if (g_pConstraintRoot != NULL)
        {
        g_pConstraintRoot->Uninitialize();
        delete g_pConstraintRoot;
        }
    
    g_pConstraintRoot = NULL;    
}

//-----------------------------------------------------------------------------
// 
//  Function:  InitializeConstraintRoot
//
//  Power management extension initialization routine
//
DWORD
InitializeConstraintRoot(
    HKEY hKey,
    LPCTSTR lpRegistryPath
    )
{   
    DWORD rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    CRegistryEdit Registry(hKey, lpRegistryPath);

    // initialize the constraint root object
    g_pConstraintRoot = new ConstraintRoot();
    if (g_pConstraintRoot == NULL) goto cleanUp;

    // Read device parameters
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(szBuffer);
    if (TRUE == Registry.RegQueryValueEx(REGEDIT_CONSTRAINT_ROOT,
            &dwType, (BYTE*)szBuffer, &dwSize))
        {        
        if (g_pConstraintRoot->Initialize(szBuffer) == FALSE)
            {
            goto cleanUp;
            }
        }

    // indicate success by returning a reference to global
    //
    rc = g_pConstraintRoot->PostInitialize();

cleanUp:
    if (rc == FALSE) UninitializeConstraintRoot((DWORD)g_pConstraintRoot);
    
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxSetConstraintByClass
//
//  Called by external components to impose a constraint
//
HANDLE
PmxSetConstraintByClass(
    DWORD classId,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    BOOL rc = FALSE;
    ConstraintContext *pContext = NULL;
    ConstraintAdapter *pAdapter;
    
    // validate
    if (g_pConstraintRoot == NULL) goto cleanUp;

    // find a constraint adapter with the matching class
    pAdapter = g_pConstraintRoot->FindConstraintAdapterByClass(classId);
    if (pAdapter == NULL) goto cleanUp;
    
    // create a constraint context
    pContext = new ConstraintContext(pAdapter);

    // apply constraint
    pContext->m_hContext = pAdapter->CreateConstraint();
    if (pContext->m_hContext == NULL) goto cleanUp;
    if (pAdapter->UpdateConstraint(pContext->m_hContext, msg, pParam, size) == NULL)
        {
        pAdapter->CloseConstraint(pContext->m_hContext);
        goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    if (rc == FALSE)
        {
        if (pContext != NULL)
            {
            delete pContext;
            pContext = NULL;
            }
        }
    
    return (HANDLE)pContext;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxSetConstraintById
//
//  Called by external components to impose a constraint
//
HANDLE
PmxSetConstraintById(
    LPCWSTR szId,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    BOOL rc = FALSE;
    ConstraintContext *pContext = NULL;
    ConstraintAdapter *pAdapter;
    
    // validate
    if (g_pConstraintRoot == NULL) goto cleanUp;

    // find a constraint adapter with the matching class
    pAdapter = g_pConstraintRoot->FindConstraintAdapterById(szId);
    if (pAdapter == NULL) goto cleanUp;
    
    // create a constraint context
    pContext = new ConstraintContext(pAdapter);

    // apply constraint
    pContext->m_hContext = pAdapter->CreateConstraint();
    if (pContext->m_hContext == NULL) goto cleanUp;
    if (pAdapter->UpdateConstraint(pContext->m_hContext, msg, pParam, size) == FALSE)
        {
        pAdapter->CloseConstraint(pContext->m_hContext);
        goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    if (rc == FALSE)
        {
        if (pContext != NULL)
            {
            delete pContext;
            pContext = NULL;
            }
        }
    
    return (HANDLE)pContext;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxUpdateConstraint
//
//  Called by external components to impose a constraint
//
BOOL
PmxUpdateConstraint(
    HANDLE hConstraintContext,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    BOOL rc = FALSE;
    ConstraintContext *pContext = (ConstraintContext*)hConstraintContext;
    ConstraintAdapter *pAdapter;
    
    // validate
    if (g_pConstraintRoot == NULL || pContext == NULL) goto cleanUp;
    if (pContext->m_cookie != (DWORD)hConstraintContext) goto cleanUp;

    // check if request succeeded
    pAdapter = pContext->m_pConstraintAdapter;
    if (pAdapter->UpdateConstraint(pContext->m_hContext, msg, pParam, size) == FALSE)
        {
        goto cleanUp;
        }
    
    rc = TRUE;    
cleanUp:    
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxReleaseConstraint
//
//  Called by external components to impose a constraint
//
void
PmxReleaseConstraint(
    HANDLE hConstraintContext
    )
{
    BOOL rc = FALSE;
    ConstraintContext *pContext = (ConstraintContext*)hConstraintContext;
    ConstraintAdapter *pAdapter;

    // validate
    if (g_pConstraintRoot == NULL || pContext == NULL) return;
    if (pContext->m_cookie != (DWORD)hConstraintContext) return;

    // apply constraint
    pAdapter = pContext->m_pConstraintAdapter;
    rc = pAdapter->CloseConstraint(pContext->m_hContext);

    delete pContext;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxGetConstraintInfo
//
//  Called by external components to impose a constraint
//
HANDLE
PmxGetConstraintInfo(
    HANDLE hConstraintContext,
    void  *pParam,
    UINT   size
    )
{
    // UNDONE:
    // need to better define this once more of the infrastructure is built
    
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(pParam);
    UNREFERENCED_PARAMETER(hConstraintContext);

    // validate
    if (g_pConstraintRoot == NULL) goto cleanUp;

cleanUp:
    return NULL;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxRegisterConstraintCallback
//
//  Called by external components to register a callback
//
HANDLE
PmxRegisterConstraintCallback(
    HANDLE hConstraintContext,
    ConstraintCallback fnCallback, 
    void *pParam, 
    UINT  size, 
    HANDLE hRefContext
    )
{    
    // validate
    HANDLE rc = NULL;
    ConstraintContext *pContext = (ConstraintContext*)hConstraintContext;
    ConstraintAdapter *pAdapter;
    
    // validate
    if (g_pConstraintRoot == NULL || pContext == NULL) goto cleanUp;
    if (pContext->m_cookie != (DWORD)hConstraintContext) goto cleanUp;

    // check if request succeeded
    pAdapter = pContext->m_pConstraintAdapter;
    rc = pAdapter->InsertConstraintCallback(
                pContext->m_hContext,
                fnCallback, 
                pParam, 
                size, 
                hRefContext
                );
    
cleanUp:    
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxUnregisterConstraintCallback
//
//  Called by external components to release a callback
//
BOOL
PmxUnregisterConstraintCallback(
    HANDLE hConstraintContext,
    HANDLE hConstraintCallback
    )
{
    // validate
    BOOL rc = FALSE;
    ConstraintContext *pContext = (ConstraintContext*)hConstraintContext;
    ConstraintAdapter *pAdapter;
    
    // validate
    if (g_pConstraintRoot == NULL || pContext == NULL) goto cleanUp;
    if (pContext->m_cookie != (DWORD)hConstraintContext) goto cleanUp;

    // check if request succeeded
    pAdapter = pContext->m_pConstraintAdapter;
    rc = pAdapter->RemoveConstraintCallback(hConstraintCallback);

cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraint
//
//  loads a constraint adapter post initialization
//
HANDLE
PmxLoadConstraint(
    void *context
    )
{
    HKEY hKey = NULL;
    DWORD rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    ConstraintContext *pContext = NULL;
    ConstraintAdapter *pAdapter;
    

    // check for global root object
    if (g_pConstraintRoot == NULL || context == NULL) goto cleanUp;

    // try opening context via registry key
    _tcscpy_s(szBuffer,_countof(szBuffer), (_TCHAR*)context);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, &hKey ) != ERROR_SUCCESS)
        {
        // check if context is a subkey
        _stprintf_s(szBuffer, _countof(szBuffer),L"%s\\%s", 
            g_pConstraintRoot->GetConstraintPath(),
            (_TCHAR*)context
            );

        // there isn't a member function to update internal 
        // variables, do it manually
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, &hKey) != ERROR_SUCCESS)
            {
            hKey = NULL;
            goto cleanUp;
            }
        }

    // At this point we know the registry path of the constraint adapter
    // pass it down to the adapter
    pAdapter = g_pConstraintRoot->LoadConstraintAdapter(szBuffer);
    if (pAdapter == NULL) goto cleanUp;

    // create a constraint context
    pContext = new ConstraintContext(pAdapter);
    pContext->m_hContext = pAdapter->CreateConstraint();
    if (pContext->m_hContext == NULL) goto cleanUp;

    rc = TRUE;

cleanUp:
    if (hKey != NULL)
        {
        RegCloseKey(hKey);
        }
    
    if (rc == FALSE)
        {
        if (pContext != NULL)
            {
            delete pContext;
            pContext = NULL;
            }
        }
    
    return pContext;
}
      
//------------------------------------------------------------------------------
