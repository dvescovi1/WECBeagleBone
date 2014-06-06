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
//  File:  constraintroot.h
//

#ifndef __CONSTRAINTROOT_H__
#define __CONSTRAINTROOT_H__

#include "constraintcontainer.h"

//------------------------------------------------------------------------------

class ConstraintAdapter;
class ConstraintContext;

//------------------------------------------------------------------------------
//
//  Class:  ConstraintRoot
//
class ConstraintRoot 
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    
protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------
    
    _TCHAR                          m_szConstraintKey[MAX_PATH];
    ConstraintContainer             m_ConstraintContainer;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------

    ConstraintRoot() : m_ConstraintContainer()
    {
        *m_szConstraintKey = NULL;
    }

public:
    //-------------------------------------------------------------------------
    // inline public routines
    //-------------------------------------------------------------------------

    ConstraintAdapter*
    FindConstraintAdapterById(
        LPCWSTR szId
        )
    {
        return m_ConstraintContainer.FindAdapterById(szId);
    }

    ConstraintAdapter*
    FindConstraintAdapterByClass(
        DWORD classId
        )
    {
        return m_ConstraintContainer.FindAdapterByClass(classId);
    }

    BOOL
    PostInitialize()
    {
        return m_ConstraintContainer.PostInitialize();
    }

    void
    Uninitialize()
    {
        m_ConstraintContainer.Uninitialize();
    }
    
    _TCHAR const*
    GetConstraintPath() const
    {
        return m_szConstraintKey;
    }    
    
public:
    //-------------------------------------------------------------------------
    // public routines
    //-------------------------------------------------------------------------

    BOOL
    Initialize(
        LPCWSTR context
        );

    ConstraintAdapter*
    LoadConstraintAdapter(
        LPCWSTR szPath
        );
};

//-----------------------------------------------------------------------------
#endif //__CONSTRAINTROOT_H__