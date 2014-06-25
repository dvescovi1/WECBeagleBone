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
//  File:  constraintcontext.h
//

#ifndef __CONSTRAINTCONTEXT_H__
#define __CONSTRAINTCONTEXT_H__

class ConstraintAdapter;

//------------------------------------------------------------------------------
//
//  Class:  ConstraintContext
//
class ConstraintContext
{
public:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------

    DWORD               m_cookie;
    HANDLE              m_hContext;
    ConstraintAdapter  *m_pConstraintAdapter;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    
    ConstraintContext(
        ConstraintAdapter  *pConstraintAdapter
        ) : m_pConstraintAdapter(pConstraintAdapter), m_hContext(NULL)
    {
        m_cookie = (DWORD)this;
    }   

    ~ConstraintContext()
    {
        m_cookie = 0;
    }
};

#endif //__CONSTRAINTCONTEXT_H__
//------------------------------------------------------------------------------