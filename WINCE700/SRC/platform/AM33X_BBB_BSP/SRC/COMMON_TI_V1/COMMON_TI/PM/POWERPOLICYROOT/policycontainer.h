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
//  File:  policycontainer.h
//

#ifndef __POLICYCONTAINER_H__
#define __POLICYCONTAINER_H__

#include <list.hxx>
#include <policyadapter.h>

//-----------------------------------------------------------------------------
class PolicyRoot;

//-----------------------------------------------------------------------------
//
//  Class: PolicyContainer
//
class PolicyContainer : public ce::list<PolicyAdapter*>
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    typedef ce::list<PolicyAdapter*>        PolicyList;
    
public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    
    PolicyContainer() : PolicyList() {};
    
public:
    //-------------------------------------------------------------------------
    // public routines
    //-------------------------------------------------------------------------
    
    //-------------------------------------------------------------------------
    // Initializes all constraint adapters in the container
    //        
    BOOL
    Initialize(
        LPCWSTR szContext
        );

    //-------------------------------------------------------------------------
    // Performs final initialization on all adapters in the container
    //        
    BOOL
    PostInitialize(
        PolicyRoot *pPolicyRoot
        );


    //-------------------------------------------------------------------------
    // Uninitialize all constraint adapters in the container
    //        
    void
    Uninitialize(
        );

    //-------------------------------------------------------------------------
    // FindAdapterByName function finds adapter object based on adapter name.
    //
    PolicyAdapter*
    FindAdapterById(
        LPCWSTR szName
        );
};

#endif //__POLICYCONTAINER_H__
//------------------------------------------------------------------------------
