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
//  File:  devicemediator.cpp
//

#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include "_constants.h"
#include "devicemediator.h"
#include "streamdevice.h"
#include "dvfslist.h"
#pragma warning(push)
#pragma warning(disable:4127)
#include "devicelist.h"
#pragma warning(pop)

//-----------------------------------------------------------------------------
DeviceMediator* 
DeviceMediator::CreateDeviceMediator()
{
    static DeviceMediator *_pDeviceMediator = NULL;

    // device mediator is a singleton
    if (_pDeviceMediator == NULL)
        {
        _pDeviceMediator = new DeviceMediator();

        if (_pDeviceMediator)
            _pDeviceMediator->Initialize();
        }
    return _pDeviceMediator;
}


//-----------------------------------------------------------------------------
void 
DeviceMediator::DeleteDeviceMediator(
    DeviceMediator *pDeviceMediaton
    )
{
    // for right now don't delete device mediator
    UNREFERENCED_PARAMETER(pDeviceMediaton);
}

//-----------------------------------------------------------------------------
BOOL 
DeviceMediator::Initialize()
{
    BOOL rc = FALSE;
    ContextList *pDVFSContextList;
    ContextList *pDeviceContextList;
    
    ::InitializeCriticalSection(&m_cs);    
    m_pContextListHead = NULL;

    // initialize device base list.  Need 1 dummy stream device in the list
    DeviceBase* pDevice = new StreamDevice();
    if (pDevice == NULL) goto cleanUp;
    InitializeListHead(pDevice);
    m_pDeviceBaseHead = pDevice;

    // instantiate DVFS device list
    pDVFSContextList = new DVFSList();
    InitializeListHead(pDVFSContextList);
    m_pContextListHead = pDVFSContextList;
    pDVFSContextList->Initialize();

    // initialize the device list
    pDeviceContextList = new DeviceList();
#pragma warning(push)
#pragma warning(disable:4127)
    InsertTailList(m_pContextListHead, pDeviceContextList);
#pragma warning(pop)
    pDeviceContextList->Initialize();
        
    rc = TRUE;
cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
BOOL 
DeviceMediator::Uninitialize()
{
    ::DeleteCriticalSection(&m_cs);

    // delete dummy stream device
    delete m_pDeviceBaseHead;
    m_pDeviceBaseHead = NULL;
    return TRUE;
}
    
//-----------------------------------------------------------------------------
BOOL 
DeviceMediator::InsertDevice(
    LPCTSTR szDeviceName, 
    DeviceBase *pDevice
    )
{
    BOOL rc = FALSE;
    HKEY hKey = NULL;
    DEVMGR_DEVICE_INFORMATION di;
    IOCTL_TIPMX_CONTEXTPATH_OUT outData;

    // get information about the driver
    di.dwSize = sizeof(di);
    if (GetDeviceInformationByFileHandle(pDevice->GetDeviceHandle(), &di) == TRUE)
        {
            RegOpenKeyEx(HKEY_LOCAL_MACHINE, di.szDeviceKey, 0, 0, &hKey);
        }
    else if (pDevice->SendIoControl(IOCTL_TIPMX_CONTEXTPATH, NULL, 0, &outData, 
                sizeof(outData), NULL) == TRUE)
        {
        if (_tcslen(outData.szContext) > 0)
            {
            RegOpenKeyEx(HKEY_LOCAL_MACHINE, outData.szContext, 0, 0, &hKey);
            }
        }

    // if a valid context key is retrieved get information from registry
    // and insert device driver wrapper if a list is using it.
    if (hKey != NULL)
        {
        ContextList *pContextList = m_pContextListHead;
        do
            {
            rc |= pContextList->InsertDevice(szDeviceName, pDevice, hKey);
            pContextList = (ContextList*)pContextList->Flink;
            }
            while (pContextList != m_pContextListHead);

        // if there is a list wanting the device then add it the list
        if (rc != FALSE)
            {
            Lock();
#pragma warning(push)
#pragma warning(disable:4127)
            InsertHeadList(m_pDeviceBaseHead, pDevice);
#pragma warning(pop)
            Unlock();
            }

        RegCloseKey(hKey);
        }

    return rc;
}

//-----------------------------------------------------------------------------
DeviceBase* 
DeviceMediator::FindDevice(
    LPCTSTR szDeviceName
    )
{
    Lock();
    DeviceBase *pDevice = NULL;
    DeviceBase *pCurrent = (DeviceBase*)m_pDeviceBaseHead->Flink;
    while (pCurrent != m_pDeviceBaseHead)
        {
        if (*pCurrent == szDeviceName)
            {
            pDevice = pCurrent;
            break;
            }
        pCurrent = (DeviceBase*)pCurrent->Flink;
        }

    Unlock();
    return pDevice;
}


//-----------------------------------------------------------------------------
DeviceBase* 
DeviceMediator::RemoveDevice(
    LPCTSTR szDeviceName
    )
{
    DeviceBase *pDevice;

    // check for valid items
    if (*szDeviceName == NULL) return NULL;

    Lock();
    pDevice = FindDevice(szDeviceName);
    if (pDevice != NULL)
        {
        ContextList *pContextList = m_pContextListHead;
        do
            {
            pContextList->RemoveDevice(szDeviceName, pDevice);
            pContextList = (ContextList*)pContextList->Flink;
            }
            while (pContextList != m_pContextListHead);
#pragma warning(push)
#pragma warning(disable:4127)
        RemoveEntryList(pDevice);
#pragma warning(pop)
        }
    Unlock();
    
    return pDevice;
}

//-----------------------------------------------------------------------------

BOOL 
DeviceMediator::SendMessage(
    UINT listId, 
    UINT dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    LPDWORD lpBytesReturned
    )
{
    BOOL rc = FALSE;
    
    // find list
    Lock();
    ContextList *pContextList = m_pContextListHead;
    do
        {
        if (pContextList->GetContextListId() == listId)
            {
             rc = pContextList->SendIoControl(dwParam, 
                    dwIoControlCode, 
                    lpInBuf, 
                    nInBufSize, 
                    lpOutBuf, 
                    nOutBufSize, 
                    lpBytesReturned
                    );
             break;
            }
        pContextList = (ContextList*)pContextList->Flink;
        }
        while (pContextList != m_pContextListHead);
    Unlock();
    
    return rc;
}

//-----------------------------------------------------------------------------
