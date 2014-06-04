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
//  File: DeviceList.h
//


#if !defined(EA_9985FD86_8E02_4720_A41B_992D216CAB76__INCLUDED_)
#define EA_9985FD86_8E02_4720_A41B_992D216CAB76__INCLUDED_

#include <linklist.h>
#include <cregedit.h>
#include "devicebase.h"

//-----------------------------------------------------------------------------
//
// class: ContextList
//
// desc: Base class of all device list classes.  Defines a set of common
//       abstract methods implemented by all device lists.
//
//
class ContextList : public LIST_ENTRY
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------

protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------

    DWORD                                   m_idContextList;

public:
    //-------------------------------------------------------------------------
    // virtual methods
    //------------------------------------------------------------------------- 

    DWORD GetContextListId() const
        {
        return m_idContextList;
        }

public:
    //-------------------------------------------------------------------------
    // virtual methods
    //------------------------------------------------------------------------- 

    virtual BOOL Uninitialize() = 0;
    
    virtual BOOL Initialize() = 0;

    virtual BOOL InsertDevice(_TCHAR const* szDeviceName,
                              DeviceBase *pDevice,
                              HKEY hKey
                              ) = 0;
    
    virtual BOOL RemoveDevice(_TCHAR const* szDeviceName,
                              DeviceBase *pDevice
                              ) = 0;
    
    virtual BOOL SendIoControl(DWORD dwParam, DWORD dwIoControlCode, 
                               LPVOID lpInBuf, DWORD nInBufSize, 
                               LPVOID lpOutBuf, DWORD nOutBufSize, 
                               DWORD *lpBytesReturned
                               ) = 0;
};

#endif // !defined(EA_9985FD86_8E02_4720_A41B_992D216CAB76__INCLUDED_)
//-----------------------------------------------------------------------------

