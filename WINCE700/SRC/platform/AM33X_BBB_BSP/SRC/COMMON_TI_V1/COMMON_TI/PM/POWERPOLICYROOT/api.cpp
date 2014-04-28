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
#include <omap_pmext.h>
#include "_constants.h"
#include "policyadapter.h"
#include "policycontext.h"
#include "policyroot.h"

//-----------------------------------------------------------------------------
// global variables
PolicyRoot      *g_pPolicyRoot = NULL;

//-----------------------------------------------------------------------------
// 
//  Function:  UninitializePolicyRoot
//
//  Power management extension uninitialization routine
//
VOID
UninitializePolicyRoot(
    DWORD dwExtContext
    )
{
    UNREFERENCED_PARAMETER(dwExtContext);

    // delete policy root if initialization failed
    if (g_pPolicyRoot != NULL)
        {
        g_pPolicyRoot->Uninitialize();
        delete g_pPolicyRoot;
        }
    
    g_pPolicyRoot = NULL;    
}

//-----------------------------------------------------------------------------
// 
//  Function:  InitializePolicyRoot
//
//  Power management extension initialization routine
//
DWORD
InitializePolicyRoot(
    HKEY hKey,
    LPCTSTR lpRegistryPath
    )
{   
    DWORD rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    CRegistryEdit Registry(hKey, lpRegistryPath);

    // initialize the policy root object
    g_pPolicyRoot = new PolicyRoot();
    if (g_pPolicyRoot == NULL) goto cleanUp;

    // Read device parameters
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(szBuffer);
    if (TRUE == Registry.RegQueryValueEx(REGEDIT_POLICY_ROOT,
            &dwType, (BYTE*)szBuffer, &dwSize))
        {        
        if (g_pPolicyRoot->Initialize(szBuffer) == FALSE)
            {
            goto cleanUp;
            }
        }

    // indicate success by returning a reference to global
    //
    rc = g_pPolicyRoot->PostInitialize();

cleanUp:
    if (rc == FALSE) UninitializePolicyRoot((DWORD)g_pPolicyRoot);
    
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PreSystemStateChange
//
//  called by rootbus before device power state changes
//
BOOL 
CALLBACK
PreDeviceStateChange(
    UINT dev, 
    UINT oldState,
    UINT newState
    )
{
    // UNDONE:
    //  Need to encapsulate w/ mutex
    if (g_pPolicyRoot != NULL)
        {
        g_pPolicyRoot->PreDeviceStateChange(dev, oldState, newState);
        }

    return TRUE;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PostDeviceStateChange
//
//  called by rootbus after device power state changes
//
BOOL 
CALLBACK
PostDeviceStateChange(
    UINT dev, 
    UINT oldState,
    UINT newState
    )
{
    // UNDONE:
    //  Need to encapsulate w/ mutex
    if (g_pPolicyRoot != NULL)
        {
        g_pPolicyRoot->PostDeviceStateChange(dev, oldState, newState);
        }

    return TRUE;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PreSystemStateChange
//
//  Power management extension notification before system state change
//
DWORD 
PreSystemStateChange(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    if (g_pPolicyRoot != NULL)
        {
        g_pPolicyRoot->PreSystemStateChange(dwExtContext, 
                lpNewStateName, dwFlags
                );
        }
    return 1;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PostSystemStateChange
//
//  Power management extension notification after system state change
//
DWORD 
PostSystemStateChange(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    if (g_pPolicyRoot != NULL)
        {
        g_pPolicyRoot->PostSystemStateChange(dwExtContext, 
                lpNewStateName, dwFlags
                );
        }
    
    return 1;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxOpenPolicy
//
//  Called by external components to open an access to a policy
//
HANDLE
PmxOpenPolicy(
    LPCWSTR szId
    )
{
    BOOL rc = FALSE;
    PolicyContext *pContext = NULL;
    PolicyAdapter *pAdapter;
    
    // validate
    if (g_pPolicyRoot == NULL) goto cleanUp;

    // find a policy adapter with the matching class
    pAdapter = g_pPolicyRoot->FindPolicy(szId);
    if (pAdapter == NULL) goto cleanUp;
    
    // create a policy context
    pContext = new PolicyContext(pAdapter);

    // apply policy
    pContext->m_hContext = pAdapter->OpenPolicy();
    if (pContext->m_hContext == NULL) goto cleanUp;

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
//  Function:  PmxNotifyPolicy
//
//  Called by external components to notify a policy
//
BOOL
PmxNotifyPolicy(
    HANDLE hPolicyContext,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    BOOL rc = FALSE;
    PolicyContext *pContext = (PolicyContext*)hPolicyContext;
    PolicyAdapter *pAdapter;
    
    // validate
    if (g_pPolicyRoot == NULL || pContext == NULL) goto cleanUp;
    if (pContext->m_cookie != (DWORD)hPolicyContext) goto cleanUp;

    // check if request succeeded
    pAdapter = pContext->m_pPolicyAdapter;
    if (pAdapter->NotifyPolicy(pContext->m_hContext, msg, pParam, size) == FALSE)
        {
        goto cleanUp;
        }
    
    rc = TRUE;    
cleanUp:    
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxClosePolicy
//
//  Called by external components to close a policy handle
//
void
PmxClosePolicy(
    HANDLE hPolicyContext
    )
{
    BOOL rc = FALSE;
    PolicyContext *pContext = (PolicyContext*)hPolicyContext;
    PolicyAdapter *pAdapter;

    // validate
    if (g_pPolicyRoot == NULL || pContext == NULL) return;
    if (pContext->m_cookie != (DWORD)hPolicyContext) return;

    // apply policy
    pAdapter = pContext->m_pPolicyAdapter;
    rc = pAdapter->ClosePolicy(pContext->m_hContext);

    delete pContext;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxGetPolicyInfo
//
//  Called by external components to impose a policy
//
HANDLE
PmxGetPolicyInfo(
    HANDLE hPolicyContext,
    void  *pParam,
    UINT   size
    )
{
    // UNDONE:
    // need to better define this once more of the infrastructure is built
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(pParam);
    UNREFERENCED_PARAMETER(hPolicyContext);
    // validate
    if (g_pPolicyRoot == NULL) goto cleanUp;

cleanUp:
    return NULL;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadPolicy
//
//  loads a power policy adapter post initialization
//
HANDLE
PmxLoadPolicy(
    void *context
    )
{
    HKEY hKey = NULL;
    DWORD rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    PolicyContext *pContext = NULL;
    PolicyAdapter *pAdapter;
    

    // check for global root object
    if (g_pPolicyRoot == NULL || context == NULL) goto cleanUp;

    // try opening context via registry key
    _tcscpy_s(szBuffer,_countof(szBuffer), (_TCHAR*)context);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, &hKey ) != ERROR_SUCCESS)
        {
        // check if context is a subkey
        _stprintf_s(szBuffer,_countof(szBuffer), L"%s\\%s", 
            g_pPolicyRoot->GetPolicyPath(),
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
    pAdapter = g_pPolicyRoot->LoadPolicy(szBuffer);
    if (pAdapter == NULL) goto cleanUp;

    // create a constraint context
    pContext = new PolicyContext(pAdapter);
    pContext->m_hContext = pAdapter->OpenPolicy();
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
