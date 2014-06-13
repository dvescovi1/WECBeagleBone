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
//  File: _constants.h
//

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <pnp.h>
#include <ceddkex.h>
#include <omap_pmext.h>

//------------------------------------------------------------------------------
//
//  GUID:  power management GUID's
//
DEFINE_GUID(
    NULL_GUID, 0x00000000, 0x0000, 0x0000, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    );

//-----------------------------------------------------------------------------
// device notification queue parameters
//
#define MAX_NOTIFY_INTERFACES       (5)
#define PNP_QUEUE_ENTRIES           (3)
#define PNP_MAX_NAMELEN             (128)
#define PNP_DEFAULT_THREAD_PRIORITY (96)
#define PNP_THREAD_PRIORITY         (L"priority256")
#define PNP_QUEUE_SIZE              (PNP_QUEUE_ENTRIES * \
                                        (sizeof(DEVDETAIL) + \
                                        (PNP_MAX_NAMELEN * sizeof(TCHAR))))

//-----------------------------------------------------------------------------
// device mediator constants
//
// 
#define BUSNAMESPACE_CLASS          (100)
#define STREAMDEVICE_CLASS          (101)
#define DISPLAYDEVICE_CLASS         (102)

#define MAX_ASYNC_TIMEOUT           (100)
#define DVFS_MAX_ASYNC_EVENTS       (256)

#define REGEDIT_DVFS_FLAGS          (L"DVFSFlags")
#define REGEDIT_DVFS_ORDER          (L"DVFSOrder")
#define REGEDIT_DVFS_ASYNCEVENT     (L"DVFSAsyncEventName")


//-----------------------------------------------------------------------------
//  registry key names
//
#define REGEDIT_CONSTRAINT_ROOT     (L"ConstraintRoot")

#define REGEDIT_CONSTRAINT_NAME     (L"Name")
#define REGEDIT_CONSTRAINT_DLL      (L"Dll")
#define REGEDIT_CONSTRAINT_ORDER    (L"Order")
#define REGEDIT_CONSTRAINT_CLASSES  (L"ConstraintClass")

//-----------------------------------------------------------------------------
//  registry key names
//
#define REGEDIT_POLICY_ROOT         (L"PowerPolicyRoot")

#define REGEDIT_POLICY_NAME         (L"Name")
#define REGEDIT_POLICY_DLL          (L"Dll")
#define REGEDIT_POLICY_ORDER        (L"Order")


//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

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
    );

#ifdef __cplusplus
}
#endif

#endif //__CONSTANTS_H__
//-----------------------------------------------------------------------------
