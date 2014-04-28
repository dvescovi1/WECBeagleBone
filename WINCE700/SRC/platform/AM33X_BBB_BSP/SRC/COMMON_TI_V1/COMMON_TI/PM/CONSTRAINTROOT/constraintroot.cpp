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
//  File:  constraintroot.cpp
//

#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <_debug.h>
#include <constraintcontext.h>
#include <constraintadapter.h>
#include <constraintcontainer.h>
#include <constraintroot.h>

//------------------------------------------------------------------------------
//
//  Method: ConstraintRoot::Initialize
//
BOOL
ConstraintRoot::
Initialize(
    LPCWSTR context
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"ConstraintRoot::"
        L"+Init('%s', 0x%08x)\r\n", context, this
        ));
    
    m_ConstraintContainer.Initialize(context);

    _tcscpy_s(m_szConstraintKey,_countof(m_szConstraintKey), context);
    
    DEBUGMSG(ZONE_FUNCTION, (L"ConstraintRoot::"
        L"-Init(%d)\r\n", TRUE
        ));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: ConstraintRoot::LoadConstraintAdapter
//
ConstraintAdapter*
ConstraintRoot::
LoadConstraintAdapter(
    LPCWSTR szContext
    )
{
    BOOL bSuccess = FALSE;
    ConstraintAdapter *pAdapterOld;
    
    // create an instance of a ConstraintAdapter
    ConstraintAdapter *pAdapter = new ConstraintAdapter();
    if (pAdapter == NULL) goto cleanUp;

    // perform initialization steps
    if (pAdapter->Initialize(szContext) == FALSE) goto cleanUp;

    // we need to preserve the uniqueness of constraint adapter id's
    // check if the constraint adapter is already in the collection
    pAdapterOld = m_ConstraintContainer.FindAdapterById(pAdapter->GetAdapterId());
    if (pAdapterOld == NULL)
        {
        if (pAdapter->PostInitialize() == FALSE)
            {
            pAdapter->Uninitialize();
            goto cleanUp;        
            }

        // insert into list
        m_ConstraintContainer.push_back(pAdapter);
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
