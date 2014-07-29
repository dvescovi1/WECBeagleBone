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
#include <omap_dvfs.h>
#include <DefaultBusContext.h>

//------------------------------------------------------------------------------

class omap35xxBusContext_t : public DefaultBusContext_t
{
    friend class  omap35xxBus_t;

protected:
    omap35xxBusContext_t(
        omap35xxBus_t* pBus
        );
        
public:

    // There are additional IOCTL codes for omap35xx Bus Driver
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

