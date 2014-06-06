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
//  File: devicelist.h
//

#ifndef __DEVICELIST_H__
#define __DEVICELIST_H__

#pragma warning(push)
#pragma warning(disable:4127)
#include <linklist.h>
#include "contextlist.h"
#pragma warning(pop)

//-----------------------------------------------------------------------------
//
// class: DeviceList
//
// desc: contains an array of asynchronous event handles
//
//
class DeviceList : public ContextList
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------

protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------

    CRITICAL_SECTION                        m_cs;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //------------------------------------------------------------------------- 

    DeviceList()
        {
        m_idContextList = DEVICEMEDIATOR_DEVICE_LIST;
        }

protected:
    //-------------------------------------------------------------------------
    // protected methods
    //------------------------------------------------------------------------- 

    void Lock()
        {
        ::EnterCriticalSection(&m_cs);
        }

    void Unlock()
        {
        ::LeaveCriticalSection(&m_cs);
        }

public:
    //-------------------------------------------------------------------------
    // virtual methods for DeviceList
    //------------------------------------------------------------------------- 

    virtual BOOL Uninitialize();
    
    virtual BOOL Initialize();

    virtual BOOL InsertDevice(_TCHAR const* szDeviceName,
                              DeviceBase *pDevice,
                              HKEY hKey
                              );
    
    virtual BOOL RemoveDevice(_TCHAR const* szDeviceName,
                              DeviceBase *pDevice
                              );
    
    virtual BOOL SendIoControl(DWORD dwParam, DWORD dwIoControlCode, 
                               LPVOID lpInBuf, DWORD nInBufSize, 
                               LPVOID lpOutBuf, DWORD nOutBufSize, 
                               DWORD *lpBytesReturned
                               );    

};


#endif // !defined(__DEVICELIST_H__)
//-----------------------------------------------------------------------------
