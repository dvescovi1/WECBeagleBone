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
//  File:  dvfslist.cpp
//

#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <cregedit.h>
#include "_constants.h"
#include "dvfslist.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// 
// insert EntryToInsert just after Entry 
// 
#ifdef  InsertEntryList
#undef  InsertEntryList
#endif

#define InsertEntryList( Entry, EntryToInsert, NotUsed ) {      \
(EntryToInsert)->Flink = (Entry)->Flink;                \
(Entry)->Flink = (EntryToInsert);                       \
(EntryToInsert)->Blink = (EntryToInsert)->Flink->Blink; \
(EntryToInsert)->Flink->Blink = (EntryToInsert);        \
}

//-----------------------------------------------------------------------------
BOOL
DVFSList::Initialize()
{
    ::InitializeCriticalSection(&m_cs);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
DVFSList::Uninitialize()
{
    // iterate through and release all elements
    DVFSElement *pCurrent = m_pDVFSElementHead;
    while (pCurrent != NULL)
        {
        // move head pointer to next element
        if (m_pDVFSElementHead == pCurrent->Flink) 
            {
            m_pDVFSElementHead = NULL;
            }
        else
            {
            m_pDVFSElementHead = (DVFSElement*)pCurrent->Flink;
            }

        if (pCurrent->m_AsyncEvent != NULL) ::CloseHandle(pCurrent->m_AsyncEvent);
        delete pCurrent;
        pCurrent = m_pDVFSElementHead;
        }

    ::DeleteCriticalSection(&m_cs);
    return TRUE;
}

//-----------------------------------------------------------------------------
void
DVFSList::InsertElementByOrder(DVFSElement *pElement)
{
    Lock();
    if (m_pDVFSElementHead != NULL)
        {        
        // code is optimized on the assumption that higher priority
        // elements will get loaded before lower priority elements
        //
        DVFSElement *pCurrent = m_pDVFSElementHead;
        do
            {
            pCurrent = (DVFSElement*)pCurrent->Blink;
            if (pCurrent->m_dwOrder < pElement->m_dwOrder) break;            
            }
            while (pCurrent != m_pDVFSElementHead);

        // update head if necessary
        if (pElement->m_dwOrder < m_pDVFSElementHead->m_dwOrder) 
            {
#pragma warning(push)
#pragma warning(disable:4127)
            InsertTailList(m_pDVFSElementHead, pElement);
#pragma warning(pop)

            m_pDVFSElementHead = pElement;
            }
        else
            {
            // insert into list
            InsertEntryList(pCurrent, pElement, pCurrent->Flink);
            }
        }
    else
        {
        InitializeListHead(pElement);
        m_pDVFSElementHead = pElement;
        }

    Unlock();
}

//-----------------------------------------------------------------------------
BOOL 
DVFSList::InsertDevice(
    _TCHAR const* szDeviceName,
    DeviceBase *pDevice,
    HKEY hKey
    )
{
    BOOL rc = FALSE;
    
    UNREFERENCED_PARAMETER(szDeviceName);

    // Get Notification flags
    DWORD dwType = REG_DWORD;
    DWORD ffDVFSNotificationType;
    DWORD dwSize = sizeof(ffDVFSNotificationType);
    if (::RegQueryValueEx(hKey, REGEDIT_DVFS_FLAGS, NULL, &dwType, 
            (BYTE*)&ffDVFSNotificationType, &dwSize) != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    // Get Notification Order
    DWORD dwOrder;
    dwType = REG_DWORD;
    dwSize = sizeof(dwOrder);
    if (::RegQueryValueEx(hKey, REGEDIT_DVFS_ORDER, NULL, &dwType, 
            (BYTE*)&dwOrder, &dwSize) != ERROR_SUCCESS)
        {
        
        goto cleanUp;
        }

    // Get ansync event name
    HANDLE hEvent = NULL;
    _TCHAR szEventName[MAX_PATH];
    dwType = REG_SZ;
    dwSize = sizeof(szEventName);
    if (::RegQueryValueEx(hKey, REGEDIT_DVFS_ASYNCEVENT, NULL, &dwType,
            (BYTE*)szEventName, &dwSize) == ERROR_SUCCESS)
        {
        if (_tcslen(szEventName) > 0)
            {
            hEvent = CreateEvent(NULL, TRUE, FALSE, szEventName);
            if (hEvent == NULL)
                {
                goto cleanUp;
                }
            }
        }

    // create DVFS Element
    DVFSElement *pElement = new DVFSElement(pDevice, hEvent, dwOrder, ffDVFSNotificationType);
    if (pElement == NULL)
        {
        CloseHandle(hEvent);
        goto cleanUp;
        }
        
    // add it to list
    InsertElementByOrder(pElement);
    rc = TRUE;

cleanUp:
    return rc;    
}

//-----------------------------------------------------------------------------
BOOL 
DVFSList::RemoveDevice(
    _TCHAR const* szDeviceName,
    DeviceBase *pDevice
    )
{   
    BOOL rc = TRUE;
    DVFSElement *pRemove = NULL;
    DVFSElement *pCurrent = m_pDVFSElementHead;

    UNREFERENCED_PARAMETER(szDeviceName);

    Lock();

    if(m_pDVFSElementHead == NULL)
    {
        goto cleanUp;
    }
    
    // iterate through list and find element to remove
    //
    do
        {
        pCurrent = (DVFSElement*)pCurrent->Blink;
        if (pCurrent->m_pDevice == pDevice)
            {
            pRemove = pCurrent;
            break;
            }
        }
        while (pCurrent != m_pDVFSElementHead);

    // check for head ptr
    //
    if (pRemove == m_pDVFSElementHead)
        {
        if (pRemove->Flink == pRemove)
            {
            // previously only 1 element in the list.
            m_pDVFSElementHead = NULL;
            }
        else
            {
            m_pDVFSElementHead = (DVFSElement*)pRemove->Flink;
            }
        }

    // free all allocated resources
    if (pRemove != NULL)
        {
#pragma warning(push)
#pragma warning(disable:4127)
        RemoveEntryList(pRemove);
#pragma warning(pop)
        if (pRemove->m_AsyncEvent != NULL) ::CloseHandle(pRemove->m_AsyncEvent);
        delete pRemove;
        rc = TRUE;
        }
cleanUp:
    Unlock();
    return rc;
}

//-----------------------------------------------------------------------------
BOOL 
DVFSList::WaitForAcknowledgements(HANDLE *rgEvents, UINT count)
{
    UINT idx;
    DWORD code;
    BOOL rc = TRUE;

    Lock();
    
    // wait for all events to get signaled
    while (rc != DVFS_FAIL_TRANSITION && (count > 0))
        {
        code = ::WaitForMultipleObjects(count, rgEvents, 0, MAX_ASYNC_TIMEOUT);
        switch (code)
            {
            case WAIT_TIMEOUT:
                rc = FALSE;
                goto cleanUp;

            default:
                // find which event got signaled
                //
                idx = code - WAIT_OBJECT_0;

                // check if successful 
                if (GetEventData(rgEvents[idx]) == DVFS_FAIL_TRANSITION)
                    {
                    rc = FALSE;
                    goto cleanUp;
                    }

                --count;
                if (idx < count)
                    {
                    // replace signaled event with the one in the back
                    rgEvents[idx] = rgEvents[count];
                    }
                break;
            }
        }    

cleanUp:
    Unlock();
    return rc;
}

//-----------------------------------------------------------------------------
BOOL 
DVFSList::SendIoControl(
    DWORD dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    DWORD *lpBytesReturned
    )
{ 
    BOOL rc = TRUE;
    DWORD nEvents = 0;
    DVFSElement *pElement;
    HANDLE rgEvents[DVFS_MAX_ASYNC_EVENTS];
    
    if (m_pDVFSElementHead == NULL) return rc;

    Lock();
         
    // iterate through all the objects and send notifications
    // to all DVFS listeners
    pElement = m_pDVFSElementHead;
    if (dwParam & (DVFS_CORE1_PRE_NOTICE | DVFS_MPU1_PRE_NOTICE))
        {
        // mask out post notifications
        dwParam &= (DVFS_CORE1_PRE_NOTICE | DVFS_MPU1_PRE_NOTICE);

        // loop within exception handler so we handle failures gracefully
        _try 
            {   
            do
                {
                // check if a notification should be sent for this element
                if (pElement->m_ffDVFSNotificationType & dwParam)
                    {
                    // save off async events
                    if (pElement->m_AsyncEvent != NULL)
                        {
                        ::SetEventData(pElement->m_AsyncEvent, DVFS_RESET_TRANSITION);
                        ::ResetEvent(pElement->m_AsyncEvent);
                        rgEvents[nEvents] = pElement->m_AsyncEvent;
                        ++nEvents;
                        }

                    // Send IOCTL to device
                    rc = pElement->m_pDevice->SendIoControl(dwIoControlCode, lpInBuf, 
                                nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned
                                );

                    if (rc == FALSE || rc == DVFS_FAIL_TRANSITION) break;                
                    }

                // next element
                pElement = (DVFSElement*)pElement->Flink;
                }
                while (pElement != m_pDVFSElementHead);
            }
        __except(EXCEPTION_EXECUTE_HANDLER) 
            {
            rc = FALSE;
            }

        // if successful so far wait for all async events
        if (rc != FALSE && nEvents > 0)
            {
            // wait for all async objects to signal
            rc = WaitForAcknowledgements(rgEvents, nEvents);
            }
        }

    // post notifications are sent in reverse order
    //
    if (dwParam & (DVFS_CORE1_POST_NOTICE | DVFS_MPU1_POST_NOTICE))
        {
        _try 
            { 
            do
                {
                // next element
                pElement = (DVFSElement*)pElement->Blink;
                
                // check if a notification should be sent for this element
                if (pElement->m_ffDVFSNotificationType & dwParam)
                    {
                    pElement->m_pDevice->SendIoControl(dwIoControlCode, lpInBuf, 
                                nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned
                                );            
                    }
                }
                while (pElement != m_pDVFSElementHead);
            }
        __except(EXCEPTION_EXECUTE_HANDLER) 
            {
            rc = FALSE;
            }
        }
    
    Unlock();
    return rc;
}

//-----------------------------------------------------------------------------
