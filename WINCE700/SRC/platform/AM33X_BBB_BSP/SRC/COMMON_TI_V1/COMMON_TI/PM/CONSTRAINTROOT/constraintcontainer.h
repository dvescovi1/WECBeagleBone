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
//  File:  constraintcontainer.h
//

#ifndef __CONSTRAINTCONTAINER_H__
#define __CONSTRAINTCONTAINER_H__

#include <list.hxx>
#include <constraintadapter.h>

//------------------------------------------------------------------------------
//
//  Class: ConstraintContainer
//
class ConstraintContainer : public ce::list<ConstraintAdapter*>
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    typedef ce::list<ConstraintAdapter*>    ContainerList;
    
public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    
    ConstraintContainer() : ContainerList() {};
    
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
    PostInitialize();


    //-------------------------------------------------------------------------
    // Uninitialize all constraint adapters in the container
    //        
    void
    Uninitialize(
        );

    //-------------------------------------------------------------------------
    // FindAdapterByName function finds adapter object based on adapter name.
    //
    ConstraintAdapter*
    FindAdapterById(
        LPCWSTR szName
        );

    //-------------------------------------------------------------------------
    // FindAdapterByClass function finds adapter object based on adapter 
    // classification.
    //    
    ConstraintAdapter*
    FindAdapterByClass(
        UINT adapterClass
        );
};

#endif
//------------------------------------------------------------------------------
