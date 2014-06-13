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
//-----------------------------------------------------------------------------
//
//  File: proxyapi.h
//
#ifndef __PROXYAPI_H__
#define __PROXYAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
//  Definitions

#define IOCTL_VIRTUAL_COPY_EX    \
    CTL_CODE(FILE_DEVICE_STREAMS, 101, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD       idDestProc;
    void*       pvDestMem;
    UINT        physAddr;
    UINT        size;
} VIRTUAL_COPY_EX_DATA;


//-----------------------------------------------------------------------------
//  Definitions

#define IOCTL_DVFS_REQUEST    \
    CTL_CODE(FILE_DEVICE_STREAMS, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DVFS_FORCE    \
    CTL_CODE(FILE_DEVICE_STREAMS, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PROFILE_DVFS_CORE    \
    CTL_CODE(FILE_DEVICE_STREAMS, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_POWERDOMAIN_REQUEST    \
    CTL_CODE(FILE_DEVICE_STREAMS, 105, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PROFILE_INTERRUPTLATENCY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PROFILE_DVFS    \
    CTL_CODE(FILE_DEVICE_STREAMS, 107, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PROFILE_WAKEUPACCURACY    \
    CTL_CODE(FILE_DEVICE_STREAMS, 108, METHOD_BUFFERED, FILE_ANY_ACCESS)


//-----------------------------------------------------------------------------
//  function prototypes
void
RunOppLatencyProfiler(
    int nSamples
    );

//-----------------------------------------------------------------------------
BOOL
ProfileInterruptLatency(
    HANDLE  hDomainConstraint,
    int     nTargetSamples
    );

//-----------------------------------------------------------------------------
typedef struct{
    UINT _period;
    UINT _duration;
    BOOL _random;
    DWORD _hiopm;   
    DWORD _lowopm;
    BOOL _currentOpm;
} DVFS_STRESS_TEST_PARAMETERS;

BOOL
ProfileDVFSLatency(
    HANDLE                          hDvfsConstraint,
    DVFS_STRESS_TEST_PARAMETERS    *pTestParam
    );

//-----------------------------------------------------------------------------
typedef struct {
    UINT _numberOfSamples;
    UINT _sleepPeriod;
} WAKEUPACCURACY_TEST_PARAMETERS;

BOOL
ProfileWakupAccuracy(
    HANDLE hDomainConstraint,
    WAKEUPACCURACY_TEST_PARAMETERS *pTestParam
    );

#ifdef __cplusplus
}
#endif

#endif
