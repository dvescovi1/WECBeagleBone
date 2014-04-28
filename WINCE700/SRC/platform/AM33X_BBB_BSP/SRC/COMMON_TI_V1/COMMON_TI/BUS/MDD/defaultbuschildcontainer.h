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

#pragma once
#include <BusDriver.hxx>
#include <string.hxx>
#include <list.hxx>

//------------------------------------------------------------------------------

class DefaultBusChild_t;

//------------------------------------------------------------------------------
//
//  Class: DefaultBusChildContainer_t
//
class DefaultBusChildContainer_t : public ce::list<BusDriverChild_t*>
{
    friend class DefaultBus_t;

protected:
    BusDriver_t* m_pBus;
        
protected:
    DefaultBusChildContainer_t(
        BusDriver_t* pBus
        );

    virtual
    ~DefaultBusChildContainer_t(
        );

public:
    
    //----------------------------------------------------------------------
    // Init function creates child objects based on bus requirements. For
    // most buses registry sub-tree will be used. Function is called in
    // chain XXX_Init->BusDriver_t::Init->BusDriverChildContainer_t::Init.
    //    
    virtual
    BOOL
    Init(
        LPCWSTR deviceRegistryKey
        );

    //----------------------------------------------------------------------
    // FindChildByName function finds child object based on child name.
    //
    virtual
    BusDriverChild_t*
    FindChildByName(
        LPCWSTR childName
        );

    //----------------------------------------------------------------------
    // FindChildByName function finds child object based on child name.
    //
    virtual
    BusDriverChild_t*
    FindChildByDriverId(
        UINT driverId
        );

    //----------------------------------------------------------------------
    // RemoveChild finds child and removes object from list.  Also
    // performing appropriate release on the child bus.
    //
    virtual
    void
    RemoveChild(
        BusDriverChild_t *pChild
        );

};

//------------------------------------------------------------------------------


