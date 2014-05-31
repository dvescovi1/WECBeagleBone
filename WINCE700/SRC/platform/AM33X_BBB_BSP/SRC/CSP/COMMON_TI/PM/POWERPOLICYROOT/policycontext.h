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
//  File:  policytcontext.h
//

#ifndef __POLICYCONTEXT_H__
#define __POLICYCONTEXT_H__

class PolicyAdapter;

//------------------------------------------------------------------------------
//
//  Class:  PolicyContext
//
class PolicyContext
{
public:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------

    DWORD               m_cookie;
    HANDLE              m_hContext;
    PolicyAdapter      *m_pPolicyAdapter;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    
    PolicyContext(
        PolicyAdapter  *pPolicyAdapter
        ) : m_pPolicyAdapter(pPolicyAdapter), m_hContext(NULL)
    {
        m_cookie = (DWORD)this;
    }   

    PolicyContext()
    {
        m_cookie = 0;
    }
};

#endif //__POLICYCONTEXT_H__
//------------------------------------------------------------------------------