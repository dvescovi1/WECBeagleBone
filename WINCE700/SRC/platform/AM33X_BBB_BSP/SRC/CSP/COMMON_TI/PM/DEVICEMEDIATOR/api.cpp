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
//  File:  api.cpp
//
//  This file implements CE DDK DLL entry function.
//

// causes GUIDs defined in _constants.h and omap_pmext.h to be instantiated and initialized
#define INITGUID

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4100)
#include <windows.h>
#include <pnp.h>
#include <pm.h>
#include <ceddk.h>
#include "omap_bus.h"
//#include <initguid.h>
#include <omap_pmext.h>
#include "_constants.h"
#include "devicemediator.h"
#include "streamdevice.h"
#include "displaydevice.h"
#pragma warning(pop)
//------------------------------------------------------------------------------
//  prototypes

BOOL 
CALLBACK
PreDeviceStateChange(
    UINT dev, 
    UINT oldState, 
    UINT newState
    );

BOOL 
CALLBACK
PostDeviceStateChange(
    UINT dev, 
    UINT oldState, 
    UINT newState
    );


//------------------------------------------------------------------------------
//  Local Structures
    
typedef struct {
    UINT            priority256;
    HANDLE          hListenerThread;
    HANDLE          hMsgSync;
    BOOL            bShutdown;
    DeviceMediator *pDeviceMediator;
} Device_t;

typedef struct {
    DWORD            iid;
    const GUID      *pguid;
} AdvertisedInfo_t;


//------------------------------------------------------------------------------
//  local variables


static Device_t         s_Device;

//------------------------------------------------------------------------------
//  Device registry parameters

static const AdvertisedInfo_t    s_rgAdvertisedInfo[MAX_NOTIFY_INTERFACES] = {
    { 
        StreamDevice::iid, &PMCLASS_PMEXT_GUID 
    }, { 
        DisplayDevice::iid, &PMCLASS_DISPLAY_GUID 
    }, {
        BUSNAMESPACE_CLASS, &PMCLASS_BUSNAMESPACE_GUID,
    }, {
        (DWORD) -1, &NULL_GUID 
    }
};

//------------------------------------------------------------------------------
//
//  Function:  RegisterBusPowerCallbacks
//
//  this routine registers for power state change callbacks to an 
//  advertised bus namespace
// 
BOOL
RegisterBusPowerCallbacks(
    WCHAR const* szBusNamespace
    )
{
    HANDLE hBus;
    BOOL rc = FALSE;
    CE_BUS_DEVICESTATE_CALLBACKS deviceStateCallbacks;
 
    // Open bus for later calls
    hBus = CreateFile(szBusNamespace, GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
            );

    if (hBus != INVALID_HANDLE_VALUE)
        {
        // initialize callback structure
        deviceStateCallbacks.size = sizeof(CE_BUS_DEVICESTATE_CALLBACKS);
        deviceStateCallbacks.PreDeviceStateChange = PreDeviceStateChange;
        deviceStateCallbacks.PostDeviceStateChange = PostDeviceStateChange;

            // Call bus driver
        rc = DeviceIoControl(hBus, 
                             IOCTL_BUS_DEVICESTATE_CALLBACKS, 
                             &deviceStateCallbacks,
                             sizeof(CE_BUS_DEVICESTATE_CALLBACKS), 
                             NULL, 
                             0, 
                             NULL, 
                             NULL
                             ); 

        CloseHandle(hBus);
        }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ProcessMsgQueueSignal
//
//  this routine reads device notifications from a message queue and updates
//  registers the device driver
// 
BOOL
ProcessMsgQueueSignal(
    int idx,
    HANDLE hMsgQ,
    DWORD iid
    )
{
    BOOL rc = FALSE;    
    DWORD dwFlags = 0;
    DeviceBase *pDevice;
    DWORD iBytesInQueue = 0;
    UCHAR deviceBuf[PNP_QUEUE_SIZE];
    PDEVDETAIL pDevDetail = (PDEVDETAIL)deviceBuf;


    UNREFERENCED_PARAMETER(idx);

    // read a message from the message queue -- it should be a device advertisement
    memset(deviceBuf, 0, PNP_QUEUE_SIZE);
    BOOL bRes = ::ReadMsgQueue(hMsgQ, deviceBuf, PNP_QUEUE_SIZE, 
                    &iBytesInQueue, 0, &dwFlags);
    
    if (bRes == TRUE && iBytesInQueue >= sizeof(DEVDETAIL)) 
        {
        // process the message
        //
        
        // check for overlarge names
        if(pDevDetail->cbName > ((PNP_MAX_NAMELEN - 1) * sizeof(pDevDetail->szName[0]))) 
            {
            goto cleanUp;
            }
            
        // add or remove the device -- note that a particular interface 
        // may be advertised more than once, so these routines must 
        // handle that possibility.
        //
        if(pDevDetail->fAttached) 
            {
            pDevice = s_Device.pDeviceMediator->FindDevice(pDevDetail->szName);
            if (pDevice == NULL)
                {
                // Create DVFSDevice wrapper for device driver
                //
                switch (iid)
                    {
                    case StreamDevice::iid:
                        pDevice = new StreamDevice();
                        break;

                    case DisplayDevice::iid:
                        pDevice = new DisplayDevice();
                        break;

                    case BUSNAMESPACE_CLASS:
                        // special case to open a handle to bus namespace
                        RegisterBusPowerCallbacks(pDevDetail->szName);
                        goto cleanUp;

                    default:
                        goto cleanUp;
                    }

                pDevice->Initialize(pDevDetail->szName);
                if (s_Device.pDeviceMediator->InsertDevice((LPCTSTR)pDevDetail->szName, pDevice) == FALSE)
                    {
                    pDevice->Uninitialize();
                    delete pDevice;
                    }
                }
            }
        else 
            {
            // detach
            //
            pDevice = s_Device.pDeviceMediator->RemoveDevice((LPCTSTR)pDevDetail->szName);
            if (pDevice != NULL)
                {
                pDevice->Uninitialize();
                delete pDevice;
                }
            }
        }

    rc = TRUE;
    
cleanUp:    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  Device_ListenerThread
//
//  Listener thread for devices supporting a particular class interface.
//
DWORD
Device_ListenerThread(
    void *pData
    )
{
    int             idx;
    DWORD           code;
    MSGQUEUEOPTIONS msgOptions;
    int             queueCount = 0;
    HANDLE          rgAdvertiseClass[MAX_NOTIFY_INTERFACES];
    HANDLE          rgMsgQueues[MAX_NOTIFY_INTERFACES + 1];

    UNREFERENCED_PARAMETER(pData);

    // Set thread priority
    //
    CeSetThreadPriority(GetCurrentThread(), s_Device.priority256);

    // loop through and create the msgQueue's and register for 
    // advertised interfaces
    //
    memset(&msgOptions, 0, sizeof(msgOptions));
    msgOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags = 0;
    msgOptions.cbMaxMessage = PNP_QUEUE_SIZE;
    msgOptions.bReadAccess = TRUE;
    memset(rgMsgQueues, 0, sizeof(HANDLE) * (MAX_NOTIFY_INTERFACES + 1));
    memset(rgAdvertiseClass, 0, sizeof(HANDLE) * (MAX_NOTIFY_INTERFACES));    
    
    for (queueCount = 0; queueCount < MAX_NOTIFY_INTERFACES; ++queueCount)
        {
        if (s_rgAdvertisedInfo[queueCount].iid == -1) break;

        // Get handle to msg queues
        //
        rgMsgQueues[queueCount] = ::CreateMsgQueue(NULL, &msgOptions);
        if (rgMsgQueues[queueCount] == NULL)
            {
            goto cleanUp;
            }

        // Get handle to advertised interfaces
        rgAdvertiseClass[queueCount] = ::RequestDeviceNotifications(
            s_rgAdvertisedInfo[queueCount].pguid, rgMsgQueues[queueCount],
            TRUE
            );

        if (rgAdvertiseClass[queueCount] == NULL)
            {
            goto cleanUp;
            }
        }

    // insert private event at end of array
    //
    rgMsgQueues[queueCount] = s_Device.hMsgSync;

    // done with initialiation
    //
    for(;;)
        {
        code = ::WaitForMultipleObjects(queueCount + 1, rgMsgQueues, FALSE, INFINITE);
        idx = code - WAIT_OBJECT_0;
        if (idx < queueCount)
            {
            ProcessMsgQueueSignal(idx, rgMsgQueues[idx], s_rgAdvertisedInfo[idx].iid);
            continue;
            }
        else if (idx == queueCount)
            {
            break;
            }
        else 
            {
            continue;
            }
        }

cleanUp:

    // free all allocated handles
    //
    for (idx = 0; idx < MAX_NOTIFY_INTERFACES; ++idx)
        {
        if (rgAdvertiseClass[idx] == NULL)   break;
        ::StopDeviceNotifications(rgAdvertiseClass[idx]);
        ::CloseMsgQueue(rgMsgQueues[idx]);
        }  
    return 0;
}

//-----------------------------------------------------------------------------
// 
//  Function:  UninitializeDeviceMediator
//
//  Power management extension uninitialization routine
//
VOID
UninitializeDeviceMediator(
    DWORD dwExtContext
    )
{
    UNREFERENCED_PARAMETER(dwExtContext);

    // wait for listener thread to shut-down
    //
    if (s_Device.hListenerThread != NULL)
        {
        // signal listner thread to shutdown
        s_Device.bShutdown = TRUE;
        ::SetEvent(s_Device.hMsgSync);

        // wait for thread to shutdown
        ::WaitForSingleObject(s_Device.hListenerThread, INFINITE);        
        ::CloseHandle(s_Device.hListenerThread);
        s_Device.hListenerThread = NULL;
        }

    if (s_Device.pDeviceMediator != NULL)
        {
        DeviceMediator::DeleteDeviceMediator(s_Device.pDeviceMediator);
        s_Device.pDeviceMediator = NULL;
        }

    if (s_Device.hMsgSync != NULL)
        {
        ::CloseHandle(s_Device.hMsgSync);
        s_Device.hMsgSync = NULL;
        }
}

//-----------------------------------------------------------------------------
// 
//  Function:  InitializeDeviceMediator
//
//  Power management extension initialization routine
//
DWORD
InitializeDeviceMediator(
    HKEY hKey,
    LPCTSTR lpRegistryPath
    )
{    
    DWORD rc = FALSE;
    CRegistryEdit Registry(hKey, lpRegistryPath);
    memset(&s_Device, 0, sizeof(Device_t));

    // Read device parameters
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(s_Device.priority256);
    if (FALSE == Registry.RegQueryValueEx(PNP_THREAD_PRIORITY,
            &dwType, (BYTE*)&s_Device.priority256, &dwSize))
        {
        s_Device.priority256 = PNP_DEFAULT_THREAD_PRIORITY;
        }            

    // Setup event handle to signal listener thread
    s_Device.hMsgSync = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (s_Device.hMsgSync == NULL)
        {
        goto cleanUp;
        }

    // Instantiate Device mediator object
    s_Device.pDeviceMediator = DeviceMediator::CreateDeviceMediator();
    if (s_Device.pDeviceMediator == NULL)
        {
        goto cleanUp;
        }

    // Listener thread
    s_Device.hListenerThread = ::CreateThread(NULL, 0, 
        Device_ListenerThread, NULL, 0, NULL
        );
    
    if (s_Device.hListenerThread == NULL) 
        {
        goto cleanUp;
        }

    // indicate success by returning a reference to global
    //
    rc = TRUE;

cleanUp:
    if (rc == FALSE) UninitializeDeviceMediator((DWORD)&s_Device);
    
    return rc;
}


//-----------------------------------------------------------------------------
// 
//  Function:  PmxSendDeviceNotification
//
//  Power management extension API to send a notification to device drivers
//
DWORD
WINAPI
PmxSendDeviceNotification(
    UINT listId, 
    UINT dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    LPDWORD pdwBytesRet
    )
{
    DWORD rc = FALSE;

    if (s_Device.pDeviceMediator != NULL)
        {
        rc = s_Device.pDeviceMediator->SendMessage(listId, dwParam, 
                dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, nOutBufSize, 
                pdwBytesRet
                );
        }
    return rc;
}       
//------------------------------------------------------------------------------
