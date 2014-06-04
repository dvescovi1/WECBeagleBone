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
//
//  File: omap_pmext.h
//
#ifndef __OMAP_PMEXT_H__
#define __OMAP_PMEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
#ifndef DEVACCESS_PMEXT_MODE
#define DEVACCESS_PMEXT_MODE      0x80000000
#endif

//------------------------------------------------------------------------------
//
//  GUID:  power management GUID's
//
DEFINE_GUID(
    PMCLASS_PMEXT_GUID, 0x0ae2066f, 0x89a2, 0x4d70, 
    0x8f, 0xc2, 0x29, 0xae, 0xfa, 0x68, 0x41, 0x3c
    );

DEFINE_GUID(
    PMCLASS_BUSNAMESPACE_GUID, 0x0ab2066f, 0x89a2, 0x4d70, 
    0x8f, 0xc2, 0x29, 0xae, 0x1a, 0xab, 0x34, 0x00
    );

DEFINE_GUID(
    PMCLASS_DISPLAY_GUID, 0xEB91C7C9, 0x8BF6, 0x4a2d, 
    0x9A, 0xB8, 0x69, 0x72, 0x4E, 0xED, 0x97, 0xD1
    );

//-----------------------------------------------------------------------------
//  typedefs and enums
//
typedef 
DWORD 
(*fnPmxSendDeviceNotification)(
    UINT listId, 
    UINT dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    LPDWORD pdwBytesRet
    );

typedef
HANDLE
(*fnPmxSetConstraintByClass)(
    DWORD classId,
    DWORD msg,
    void *pParam,
    UINT  size
    );

typedef
HANDLE
(*fnPmxSetConstraintById)(
    LPCWSTR szId,
    DWORD msg,
    void *pParam,
    UINT  size
    );

typedef
BOOL
(*fnPmxUpdateConstraint)(
    HANDLE hConstraintContext,
    DWORD msg,
    void *pParam,
    UINT  size
    );

typedef
void
(*fnPmxReleaseConstraint)(
    HANDLE hConstraintContext
    );

typedef
HANDLE
(*fnPmxOpenPolicy)(
    LPCWSTR szId
    );

typedef
BOOL
(*fnPmxNotifyPolicy)(
    HANDLE hPolicyContext,
    DWORD msg,
    void *pParam,
    UINT  size
    );

typedef
void
(*fnPmxClosePolicy)(
    HANDLE hPolicyContext
    );

typedef
HANDLE
(*fnPmxGetPolicyInfo)(
    HANDLE hPolicyContext,
    void  *pParam,
    UINT   size
    );

typedef
HANDLE
(*fnPmxRegisterConstraintCallback)(
    HANDLE hConstraintContext,
    void *fnCallback, 
    void *pParam, 
    UINT  size, 
    HANDLE hRefContext
    );

typedef
BOOL
(*fnPmxUnregisterConstraintCallback)(
    HANDLE hConstraintContext,
    HANDLE hConstraintCallback
    );

typedef
HANDLE
(*fnPmxLoadConstraint)(
    void *context
    );

typedef
HANDLE
(*fnPmxLoadPolicy)(
    void *context
    );



//------------------------------------------------------------------------------
// Define API's exposed by OMAP power extensions
//
typedef struct {
    BOOL                            bInitialized; 
    fnPmxSendDeviceNotification     PmxSendDeviceNotification;
    fnPmxSetConstraintByClass       PmxSetConstraintByClass;
    fnPmxSetConstraintById          PmxSetConstraintById;
    fnPmxUpdateConstraint           PmxUpdateConstraint;
    fnPmxReleaseConstraint          PmxReleaseConstraint;
    fnPmxRegisterConstraintCallback PmxRegisterConstraintCallback;
    fnPmxUnregisterConstraintCallback PmxUnregisterConstraintCallback;
    fnPmxOpenPolicy                 PmxOpenPolicy;
    fnPmxNotifyPolicy               PmxNotifyPolicy;
    fnPmxClosePolicy                PmxClosePolicy;
    fnPmxLoadConstraint             PmxLoadConstraint;
    fnPmxLoadPolicy                 PmxLoadPolicy;
} OMAP_PMEXT_IFC;


//------------------------------------------------------------------------------
typedef 
BOOL 
(*fnOmapPmExtensions)(
    OMAP_PMEXT_IFC *pIfc,
    DWORD           size
    );

//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif

