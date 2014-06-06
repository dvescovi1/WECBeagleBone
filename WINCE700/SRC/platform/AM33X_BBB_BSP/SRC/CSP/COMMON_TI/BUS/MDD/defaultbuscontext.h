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
#include <omap_bus.h>

//------------------------------------------------------------------------------

class DefaultBusContext_t : public BusDriverContext_t
{
    friend class DefaultBus_t;
    
protected:
    BusDriver_t* m_pBus;
    BusDriverChild_t *m_pChild;

protected:
    
    BusDriverChild_t* 
        FindChildByName(
        LPCWSTR childName
        );

protected:
    
    DefaultBusContext_t(
        BusDriver_t *pBus
        );

    virtual
    ~DefaultBusContext_t(
        );

public:
    //----------------------------------------------------------------------
    // BusDriverContext_t interface, see BusDriver.h for comments

    virtual
    BOOL
    Close(
        );

    virtual
    BOOL
    IOControl(
        DWORD code, 
        UCHAR *pInBuffer, 
        DWORD inSize, 
        UCHAR *pOutBuffer,
        DWORD outSize, 
        DWORD *pOutSize
        );
        
};

//------------------------------------------------------------------------------

