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
//  File: dvfslist.h
//

#if !defined(EA_4F1909C5_9CDC_42f9_BA06_E5EDC4EC1193__INCLUDED_)
#define EA_4F1909C5_9CDC_42f9_BA06_E5EDC4EC1193__INCLUDED_


#pragma warning(push)
#pragma warning(disable:4127)
#include <linklist.h>
#include "contextlist.h"
#pragma warning(pop)

//-----------------------------------------------------------------------------
//
// class: DVFSElement
//
// desc: represents a single DVFS aware device driver
//
class DVFSElement : public LIST_ENTRY
{
public:
    //-------------------------------------------------------------------------
    // member variables
    //------------------------------------------------------------------------- 

    DWORD                                   m_dwOrder;
    HANDLE                                  m_AsyncEvent;
    DWORD                                   m_ffDVFSNotificationType;
    DeviceBase                             *m_pDevice;

public:
    DVFSElement(DeviceBase *pDevice, HANDLE AsyncEvent, DWORD dwOrder,
        DWORD ffDVFSNotificationType) : m_pDevice(pDevice),
                                        m_AsyncEvent(AsyncEvent),
                                        m_dwOrder(dwOrder),
                                        m_ffDVFSNotificationType(ffDVFSNotificationType)
        {        
        }
};


//-----------------------------------------------------------------------------
//
// class: DVFSList
//
// desc: contains an array of asynchronous event handles
//
//
class DVFSList : public ContextList
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
    DVFSElement                            *m_pDVFSElementHead;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //------------------------------------------------------------------------- 

    DVFSList() : m_pDVFSElementHead(NULL)
        {
        m_idContextList = DEVICEMEDIATOR_DVFS_LIST;
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

protected:
    //-------------------------------------------------------------------------
    // protected methods
    //------------------------------------------------------------------------- 

    void InsertElementByOrder(DVFSElement *pElement);
    BOOL WaitForAcknowledgements(HANDLE *rgEvents, UINT count);
    
    
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

#ifndef InsertEntryList
#define InsertEntryList(PrevEntry, Entry, NextEntry) {\
      (Entry)->Flink = NextEntry; \
      (Entry)->Blink = PrevEntry; \
      (PrevEntry)->Flink = Entry; \
      (NextEntry)->Blink = Entry; \
      };
#endif

#endif // !defined(EA_4F1909C5_9CDC_42f9_BA06_E5EDC4EC1193__INCLUDED_)
//-----------------------------------------------------------------------------
