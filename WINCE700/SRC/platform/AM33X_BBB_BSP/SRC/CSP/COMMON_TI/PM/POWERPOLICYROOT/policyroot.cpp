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
//  File:  policyroot.cpp
//

#include <windows.h>
#include <_debug.h>
#include <policycontext.h>
#include <policyadapter.h>
#include <policycontainer.h>
#include <policyroot.h>

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// disable PREFAST warning for empty _except block
#pragma warning (disable: 6322)

//------------------------------------------------------------------------------
//
//  Method: policyRoot::Initialize
//
BOOL
PolicyRoot::
Initialize(
    LPCWSTR context
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"PolicyRoot::"
        L"+Init('%s', 0x%08x)\r\n", context, this
        ));
    
    m_PolicyContainer.Initialize(context);

    _tcscpy_s(m_szPolicyKey,_countof(m_szPolicyKey), context);
    
    DEBUGMSG(ZONE_FUNCTION, (L"PolicyRoot::"
        L"-Init(%d)\r\n", TRUE
        ));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::AddDeviceChangeObserver
//
void
PolicyRoot::
AddDeviceChangeObserver(
    PolicyAdapter   *pPolicy
    )
{
    PolicyAdapter *pPolicyCurrent;
    
    // initialize policy
    pPolicy->m_pNextDeviceChangeObserver = NULL;

    // insert into list
    if (m_pDeviceChangeList == NULL)
        {
        m_pDeviceChangeList = pPolicy;
        }
    else
        {
        // go to end of list
        pPolicyCurrent = m_pDeviceChangeList;
        while (pPolicyCurrent->m_pNextDeviceChangeObserver != NULL)
            {
            pPolicyCurrent = pPolicyCurrent->m_pNextDeviceChangeObserver;
            }

        // append to list
        pPolicyCurrent->m_pNextDeviceChangeObserver = pPolicy;
        }
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::AddSystemChangeObserver
//
void
PolicyRoot::
AddSystemChangeObserver(
    PolicyAdapter   *pPolicy
    )
{
    PolicyAdapter *pPolicyCurrent;
    
    // initialize policy
    pPolicy->m_pNextSystemChangeObserver = NULL;

    // insert into list
    if (m_pSystemChangeList == NULL)
        {
        m_pSystemChangeList = pPolicy;
        }
    else
        {
        // go to end of list
        pPolicyCurrent = m_pSystemChangeList;
        while (pPolicyCurrent->m_pNextSystemChangeObserver != NULL)
            {
            pPolicyCurrent = pPolicyCurrent->m_pNextSystemChangeObserver;
            }

        // append to list
        pPolicyCurrent->m_pNextSystemChangeObserver = pPolicy;
        }
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::PreSystemStateChange
//
DWORD 
PolicyRoot::
PreSystemStateChange(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    PolicyAdapter *pPolicyCurrent = m_pSystemChangeList;

    // go through and notify all observers
    while (pPolicyCurrent != NULL)
        {
        __try
            {
            pPolicyCurrent->PreSystemStateChange(
                dwExtContext, lpNewStateName, dwFlags
                );
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }

        pPolicyCurrent = pPolicyCurrent->m_pNextSystemChangeObserver;
        }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::PostSystemStateChange
//
DWORD 
PolicyRoot::
PostSystemStateChange(
    DWORD dwExtContext, 
    LPCTSTR lpNewStateName, 
    DWORD dwFlags
    )
{
    PolicyAdapter *pPolicyCurrent = m_pSystemChangeList;

    // go through and notify all observers
    while (pPolicyCurrent != NULL)
        {
        __try
            {
            pPolicyCurrent->PostSystemStateChange(
                dwExtContext, lpNewStateName, dwFlags
                );
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }

        pPolicyCurrent = pPolicyCurrent->m_pNextSystemChangeObserver;
        }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::PreDeviceStateChange
//
BOOL 
PolicyRoot::
PreDeviceStateChange(
    UINT dev, 
    UINT oldState, 
    UINT newState
    )
{
    PolicyAdapter *pPolicyCurrent = m_pDeviceChangeList;

    // go through and notify all observers
    while (pPolicyCurrent != NULL)
        {
        __try
            {
            pPolicyCurrent->PreDeviceStateChange(
                dev, oldState, newState
                );
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }

        pPolicyCurrent = pPolicyCurrent->m_pNextDeviceChangeObserver;
        }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: policyRoot::PostDeviceStateChange
//
BOOL 
PolicyRoot::
PostDeviceStateChange(
    UINT dev, 
    UINT oldState, 
    UINT newState
    )
{
    PolicyAdapter *pPolicyCurrent = m_pDeviceChangeList;

    // go through and notify all observers
    while (pPolicyCurrent != NULL)
        {
        __try
            {
            pPolicyCurrent->PostDeviceStateChange(
                dev, oldState, newState
                );
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }

        pPolicyCurrent = pPolicyCurrent->m_pNextDeviceChangeObserver;
        }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: PolicyRoot::LoadPolicy
//
PolicyAdapter*
PolicyRoot::
LoadPolicy(
    LPCWSTR szContext
    )
{
    BOOL bSuccess = FALSE;
    PolicyAdapter *pAdapterOld;
    
    // create an instance of a ConstraintAdapter
    PolicyAdapter *pAdapter = new PolicyAdapter();
    if (pAdapter == NULL) goto cleanUp;

    // perform initialization steps
    if (pAdapter->Initialize(szContext) == FALSE) goto cleanUp;

    // we need to preserve the uniqueness of constraint adapter id's
    // check if the constraint adapter is already in the collection
    pAdapterOld = m_PolicyContainer.FindAdapterById(pAdapter->GetAdapterId());
    if (pAdapterOld == NULL)
        {
        if (pAdapter->PostInitialize(this) == FALSE)
            {
            pAdapter->Uninitialize();
            goto cleanUp;        
            }

        // insert into list
        m_PolicyContainer.push_back(pAdapter);
        }
    else
        {
        // if adapter is already in the collection then delete new one
        // and use the old one.
        pAdapter->Uninitialize();
        delete pAdapter;
        pAdapter = pAdapterOld;        
        }

    bSuccess = TRUE;

cleanUp:
    if (bSuccess == FALSE)
        {
        if (pAdapter != NULL) delete pAdapter;
        pAdapter = NULL;
        }
    return pAdapter;
}


//------------------------------------------------------------------------------
