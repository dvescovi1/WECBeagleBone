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
//  File: DisplayDevice.h
//

#if !defined(EA_FB261B9C_80E7_42e1_B0AE_D684443582D5__INCLUDED_)
#define EA_FB261B9C_80E7_42e1_B0AE_D684443582D5__INCLUDED_

#include "DeviceBase.h"

//-----------------------------------------------------------------------------
//
// class: DisplayDevice
//
// desc: Wrapper class for display drivers.
//
//
class DisplayDevice : public DeviceBase
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    
    enum { iid = DISPLAYDEVICE_CLASS };
    
public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    
    DisplayDevice() : DeviceBase()
        {
        }

public:
    //-------------------------------------------------------------------------
    // virtual methods
    //------------------------------------------------------------------------- 

    virtual BOOL Initialize(_TCHAR const* szDeviceName);
    virtual BOOL Uninitialize();
    virtual BOOL SendIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, 
                                DWORD nInBufSize, LPVOID lpOutBuf, 
                                DWORD nOutBufSize, DWORD *lpBytesReturned
                                );

};
#endif // !defined(EA_FB261B9C_80E7_42e1_B0AE_D684443582D5__INCLUDED_)
//-----------------------------------------------------------------------------