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
//  File:  pmxproxy.h
//
#ifndef __PMXPROXY_H__
#define __PMXPROXY_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PMXPROXY_DRIVER             (L"PMX1:")

//-----------------------------------------------------------------------------
//  Definitions

#define IOCTL_PMX_LOADPOLICY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 101, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_OPENPOLICY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_CLOSEPOLICY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_NOTIFYPOLICY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_LOADCONSTRAINT    \
    CTL_CODE(FILE_DEVICE_STREAMS, 105, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_SETCONSTRAINTBYID   \
    CTL_CODE(FILE_DEVICE_STREAMS, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_UPDATECONSTRAINT    \
    CTL_CODE(FILE_DEVICE_STREAMS, 107, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PMX_RELEASECONSTRAINT    \
    CTL_CODE(FILE_DEVICE_STREAMS, 108, METHOD_BUFFERED, FILE_ANY_ACCESS)

//-----------------------------------------------------------------------------

typedef struct {
    LPCWSTR szId;
    DWORD   strlen;
    DWORD   msg;
    void*   pvParam;
    DWORD   size;
} IOCTL_PMX_PARAMETERS;

//-----------------------------------------------------------------------------

#ifdef USE_PMX_WRAPPER

//------------------------------------------------------------------------------
//
//  Function:  PmxSendDeviceNotification
//
//  Sends a notification to a given device.
//
__inline
DWORD
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
    return 0;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintByClass
//
//  Imposes a constraint using an adapter identified by class.
//
__inline
HANDLE
PmxSetConstraintByClass(
    DWORD classId,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    return NULL;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintById
//
//  Imposes a constraint using an adapter identified by id.
//
__inline
HANDLE
PmxSetConstraintById(
    LPCWSTR szId,
    DWORD msg,
    void *pvParam,
    UINT  size
    )
{
    BOOL rc;
    HANDLE hPmx;
    IOCTL_PMX_PARAMETERS pmxInfo;

    pmxInfo.szId = szId;
    pmxInfo.strlen = _tcslen(szId);
    pmxInfo.msg = msg;
    pmxInfo.pvParam = pvParam;
    pmxInfo.size = size;

    // open handle to pmx proxy as that will act as a handle to the
    // constraint adapter
    hPmx = CreateFile(PMXPROXY_DRIVER, 
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE, 
                      NULL, 
                      OPEN_EXISTING, 
                      0, 
                      NULL
                      );
    
    if (hPmx == INVALID_HANDLE_VALUE) return NULL;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_SETCONSTRAINTBYID, 
                         &pmxInfo, 
                         sizeof(pmxInfo), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    if (rc == FALSE)
        {
        CloseHandle(hPmx);
        hPmx = NULL;
        }

    return hPmx;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxUpdateConstraint
//
//  Updates a constraint context.
//
__inline
BOOL
PmxUpdateConstraint(
    HANDLE hPmx,
    DWORD msg,
    void *pvParam,
    UINT  size
    )
{
    BOOL rc;
    IOCTL_PMX_PARAMETERS pmxInfo;

    pmxInfo.msg = msg;
    pmxInfo.pvParam = pvParam;
    pmxInfo.size = size;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_UPDATECONSTRAINT, 
                         &pmxInfo, 
                         sizeof(pmxInfo), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxReleaseConstraint
//
//  Releases a constraint.
//
__inline
void
PmxReleaseConstraint(
    HANDLE hPmx
    )
{
    DeviceIoControl(hPmx, 
                    IOCTL_PMX_RELEASECONSTRAINT, 
                    NULL, 
                    0, 
                    NULL, 
                    0, 
                    NULL, 
                    NULL
                    );

    CloseHandle(hPmx);
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxRegisterConstraintCallback
//
//  Called by external components to register a callback
//
__inline
HANDLE
PmxRegisterConstraintCallback(
    HANDLE hConstraintContext,
    ConstraintCallback fnCallback, 
    void *pParam, 
    UINT  size, 
    HANDLE hRefContext
    )
{
    return NULL;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxUnregisterConstraintCallback
//
//  Called by external components to release a callback
//
__inline
BOOL
PmxUnregisterConstraintCallback(
    HANDLE hConstraintContext,
    HANDLE hConstraintCallback
    )
{
    return NULL;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxOpenPolicy
//
//  Called by external components to open an access to a policy
//
__inline
HANDLE
PmxOpenPolicy(
    LPCWSTR szId
    )
{
    BOOL rc;
    HANDLE hPmx;

    // open handle to pmx proxy as that will act as a handle to the
    // constraint adapter
    hPmx = CreateFile(PMXPROXY_DRIVER, 
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE, 
                      NULL, 
                      OPEN_EXISTING, 
                      0, 
                      NULL
                      );
    
    if (hPmx == INVALID_HANDLE_VALUE) return NULL;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_OPENPOLICY, 
                         (void*)szId, 
                         sizeof(WCHAR)*wcslen(szId), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    if (rc == FALSE)
        {
        CloseHandle(hPmx);
        hPmx = NULL;
        }

    return hPmx;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxNotifyPolicy
//
//  Called by external components to notify a policy
//
__inline
BOOL
PmxNotifyPolicy(
    HANDLE hPmx,
    DWORD msg,
    void *pvParam,
    UINT  size
    )
{
    BOOL rc;
    IOCTL_PMX_PARAMETERS pmxInfo;

    pmxInfo.msg = msg;
    pmxInfo.pvParam = pvParam;
    pmxInfo.size = size;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_NOTIFYPOLICY, 
                         &pmxInfo, 
                         sizeof(pmxInfo), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxClosePolicy
//
//  Called by external components to close a policy handle
//
__inline
void
PmxClosePolicy(
    HANDLE hPmx
    )
{
    DeviceIoControl(hPmx, 
                    IOCTL_PMX_CLOSEPOLICY, 
                    NULL, 
                    0, 
                    NULL, 
                    0, 
                    NULL, 
                    NULL
                    );

    CloseHandle(hPmx);
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxGetPolicyInfo
//
//  Called by external components to impose a policy
//
__inline
HANDLE
PmxGetPolicyInfo(
    HANDLE hPmx,
    void  *pParam,
    UINT   size
    )
{
    return NULL;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraint
//
//  Called by external components to dynamically load a constraint adapter
//
__inline
HANDLE
PmxLoadConstraint(
    void *context
    )
{
    BOOL rc;
    HANDLE hPmx = NULL;

    // open handle to pmx proxy as that will act as a handle to the
    // constraint adapter
    hPmx = CreateFile(PMXPROXY_DRIVER, 
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE, 
                      NULL, 
                      OPEN_EXISTING, 
                      0, 
                      NULL
                      );
    
    if (hPmx == INVALID_HANDLE_VALUE) return NULL;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_LOADCONSTRAINT, 
                         context, 
                         sizeof(WCHAR)*wcslen((WCHAR*)context), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    if (rc == FALSE)
        {
        CloseHandle(hPmx);
        hPmx = NULL;
        }

    return hPmx;
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraintAdapter
//
//  Called by external components to dynamically load a constraint adapter
//
__inline
HANDLE
PmxLoadPolicy(
    void *context
    )
{
    BOOL rc;
    HANDLE hPmx = NULL;

    // open handle to pmx proxy as that will act as a handle to the
    // constraint adapter
    hPmx = CreateFile(PMXPROXY_DRIVER, 
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE, 
                      NULL, 
                      OPEN_EXISTING, 
                      0, 
                      NULL
                      );
    
    if (hPmx == INVALID_HANDLE_VALUE) return NULL;

    // apply constraint
    rc = DeviceIoControl(hPmx, 
                         IOCTL_PMX_LOADPOLICY, 
                         context, 
                         sizeof(WCHAR)*wcslen((WCHAR*)context), 
                         NULL, 
                         0, 
                         NULL, 
                         NULL
                         );

    if (rc == FALSE)
        {
        CloseHandle(hPmx);
        hPmx = NULL;
        }

    return hPmx;
}

#endif // PMXPROXY_DRIVER

#ifdef __cplusplus
}
#endif

#endif //__PMXPROXY_H__
//-----------------------------------------------------------------------------