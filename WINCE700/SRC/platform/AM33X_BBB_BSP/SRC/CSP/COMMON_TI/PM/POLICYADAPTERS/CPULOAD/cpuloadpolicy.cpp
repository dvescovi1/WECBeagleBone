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
//  File:  cpuloadpolicy.cpp
//


//******************************************************************************
//  #INCLUDES
//******************************************************************************
#include "omap.h"
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include "omap_dvfs.h"
#include <ceddkex.h>

#pragma warning(disable: 4200)

//-----------------------------------------------------------------------------
#define FRACTION_PART               (20)
#define QUOTIENT_PART               (32 - FRACTION_PART)
#define POLICY_CONTEXT_COOKIE       'cpul'
#define CONSTRAINT_ID_DVFS          L"DVFS"

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_OPM            DEBUGZONE(4)

DBGPARAM dpCurSettings = {
    L"TI_CPULOADPOLICY", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"OPM",         L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif

//-----------------------------------------------------------------------------
//  local structures
//
typedef struct {
    DWORD                           units;
    DWORD                           mhzSum;
    DWORD                           currentBucket;
    DWORD                           rgBuckets[];
} BucketInfo_t;

typedef struct {
    DWORD                           floor;
    DWORD                           ceiling;
} OpmThresholdRange_t;


typedef struct
{
    HANDLE                          hDvfsConstraint;
    HANDLE                          hDvfsCallback;
    HANDLE                          hCpuLoadThread;
    HANDLE                          hCpuLoadEvent;
    HANDLE                          hBootEvent;
    DWORD                           bucketCount;
    DWORD                           sysIntr;
    BOOL                            bExit;
    DWORD                           dwMonitorPeriod;
    DWORD                           dwMonitorWindow;
    DWORD                           dwCeilingOpm;
    DWORD                           dwFloorOpm;
    DWORD                           dwNominalOpm;
    DWORD                           dwInterruptThreshold;
    DWORD                           priority256;
    DWORD                           dwBootOpm;
    DWORD                           dwBootTimeout;
    WCHAR                           szBootEventName[MAX_PATH];
    DWORD                           irq;

    OpmThresholdRange_t             rgOpmRange[kOpmCount];
    DWORD                           rgOpmFrequency[kOpmCount];
    DWORD                           rgOpmThreshold[kOpmCount];
    BucketInfo_t                   *pBucketInfo;

} CpuPolicyInfo_t;

//------------------------------------------------------------------------------
//  policy registry parameters
//
static const DEVICE_REGISTRY_PARAM g_policyRegParams[] = {
   {
        L"MonitorPeriod", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwMonitorPeriod),
        fieldsize(CpuPolicyInfo_t, dwMonitorPeriod), NULL
    }, {
        L"MonitorWindow", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwMonitorWindow),
        fieldsize(CpuPolicyInfo_t, dwMonitorWindow), NULL
    }, {
        L"InterruptThreshold", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwInterruptThreshold),
        fieldsize(CpuPolicyInfo_t, dwInterruptThreshold), NULL
    }, {
        L"priority256", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, priority256),
        fieldsize(CpuPolicyInfo_t, priority256), NULL
    },  {
        L"BootEventName", PARAM_STRING, TRUE, offset(CpuPolicyInfo_t, szBootEventName),
        fieldsize(CpuPolicyInfo_t, szBootEventName), NULL
    }, {
        L"BootTimeout", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwBootTimeout),
        fieldsize(CpuPolicyInfo_t, dwBootTimeout), NULL
    }, {
        L"irq", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, irq),
        fieldsize(CpuPolicyInfo_t, irq), NULL
    } 
};
//------------------------------------------------------------------------------
//  CPU dependent policy registry parameters 
//
static const DEVICE_REGISTRY_PARAM g_CpuFamilypolicyRegParams[] = {
    {
        L"CeilingOpm", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwCeilingOpm),
        fieldsize(CpuPolicyInfo_t, dwCeilingOpm), NULL
    }, {
        L"FloorOpm", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwFloorOpm),
        fieldsize(CpuPolicyInfo_t, dwFloorOpm), NULL
    }, {
        L"NominalOpm", PARAM_DWORD, FALSE, offset(CpuPolicyInfo_t, dwNominalOpm),
        fieldsize(CpuPolicyInfo_t, dwNominalOpm), (void*)kOpm3
    }, {
        L"BootOpm", PARAM_DWORD, TRUE, offset(CpuPolicyInfo_t, dwBootOpm),
        fieldsize(CpuPolicyInfo_t, dwBootOpm), NULL
    }, {
        L"OpmFrequency", PARAM_MULTIDWORD, FALSE, offset(CpuPolicyInfo_t, rgOpmFrequency),
        fieldsize(CpuPolicyInfo_t, rgOpmFrequency), NULL
    }, {
        L"OpmThreshold", PARAM_MULTIDWORD, FALSE, offset(CpuPolicyInfo_t, rgOpmThreshold),
        fieldsize(CpuPolicyInfo_t, rgOpmThreshold), NULL
    }    
};    
//-----------------------------------------------------------------------------
//  local variables
//
static DWORD                    s_currentOpm;
static DWORD                    s_requestedOpm;
static CpuPolicyInfo_t          s_CpuPolicyInfo;
static BOOL                     s_bSleeping = FALSE;

//-----------------------------------------------------------------------------
//
//  Function:  DvfsConstraintCallback
//
//  Called whenever there is a OPM change
//
BOOL DvfsConstraintCallback(
    HANDLE hRefContext,
    DWORD msg,
    void *pParam,
    UINT size
    )
{
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(hRefContext);
    if (msg == CONSTRAINT_MSG_DVFS_NEWOPM)
        {
        s_currentOpm = (DWORD)pParam;
        if (s_bSleeping == TRUE && s_currentOpm > kOpm0)
            {
            // wake load monitor if sleeping
            SetEvent(s_CpuPolicyInfo.hCpuLoadEvent);
            }
        }
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function:  UpdateTimeBuckets
//
//  Updates bucket info based on given tickTime and idleTime
//
void
UpdateTimeBuckets(
    BucketInfo_t   *pBucketInfo,
    DWORD           mhz                     //uQUOTIENT_PART.FRACTION_PART
    )
{
    int idx = pBucketInfo->currentBucket;
    DWORD mhzSum = pBucketInfo->mhzSum;

    // subtract data point from running total
    mhzSum -= pBucketInfo->rgBuckets[idx];

    // update with new data point
    pBucketInfo->rgBuckets[idx] = mhz;

    // update running total
    mhzSum += mhz;
    pBucketInfo->mhzSum = mhzSum;

    // move to next bucket
    pBucketInfo->units = min(pBucketInfo->units + 1, s_CpuPolicyInfo.bucketCount);
    pBucketInfo->currentBucket = (pBucketInfo->currentBucket + 1) % s_CpuPolicyInfo.bucketCount;

    DEBUGMSG(ZONE_INFO, (L"mhzSum=%d, avgmhz=%d, currentBucket=%d\r\n",
        pBucketInfo->mhzSum >> FRACTION_PART,
        (pBucketInfo->mhzSum / pBucketInfo->units) >> FRACTION_PART,
        pBucketInfo->currentBucket)
        );
}

//-----------------------------------------------------------------------------
//
//  Function:  CpuLoadThreadFn
//
//  Contains the Cpu load calculation and Opm change desition functionality
//
DWORD WINAPI CpuLoadThreadFn(LPVOID pvParam)
{
    DWORD opm=0;
    DWORD mhz;
    DWORD cpuLoad;
    DWORD tickTime;
    DWORD idleTime;
    DWORD lastTick = 0;
    DWORD lastIdle = 0;
    DWORD currentTick;
    DWORD currentIdle;
    DWORD code = WAIT_TIMEOUT;
    DWORD ceilingOpm = s_CpuPolicyInfo.dwCeilingOpm;
    DWORD floorOpm = s_CpuPolicyInfo.dwFloorOpm;
    DWORD timeOut = s_CpuPolicyInfo.dwBootTimeout;
    BucketInfo_t *pBucketInfo = s_CpuPolicyInfo.pBucketInfo;

    UNREFERENCED_PARAMETER(pvParam);

    // wait for boot event or timeout
    WaitForSingleObject(s_CpuPolicyInfo.hBootEvent, timeOut);

    // start cpu monitor loop
    s_bSleeping = FALSE;
    while (s_CpuPolicyInfo.bExit == FALSE)
        {
        // get tick and idle times
        currentTick = GetTickCount();
        currentIdle = GetIdleTime();

        // calculate tick and idle time from previous
        tickTime = currentTick - lastTick;
        idleTime = currentIdle - lastIdle;

        // check if there's tracked data
        if (lastTick != 0)
            {
            // initially set to 100% cpu load
            cpuLoad = 1 << FRACTION_PART;
            if (tickTime != 0)
                {
                cpuLoad = cpuLoad - ((idleTime << FRACTION_PART) / tickTime);
                }

            mhz = cpuLoad * s_CpuPolicyInfo.rgOpmFrequency[s_currentOpm];
            UpdateTimeBuckets(s_CpuPolicyInfo.pBucketInfo, mhz);

            mhz = (s_CpuPolicyInfo.pBucketInfo->mhzSum / s_CpuPolicyInfo.pBucketInfo->units);

            // update opm if necessary
            if (mhz > s_CpuPolicyInfo.rgOpmRange[s_requestedOpm].ceiling) opm = ceilingOpm;
            if (mhz <= s_CpuPolicyInfo.rgOpmRange[s_requestedOpm].floor) opm = max(s_requestedOpm - 1, floorOpm);
            if ((opm > s_CpuPolicyInfo.dwNominalOpm) && (s_requestedOpm < s_CpuPolicyInfo.dwNominalOpm)) opm = s_CpuPolicyInfo.dwNominalOpm;

            RETAILMSG(ZONE_OPM,
                (L"opmCurrent=OPM%d, opmRequest=OPM%d, avg mhz=%d\r\n",
                s_currentOpm, opm, mhz >> FRACTION_PART)
                );
            }
        else
            {
            opm = s_CpuPolicyInfo.dwNominalOpm;
            }

        // update operating point
        if (s_requestedOpm != opm)
            {
            // update operating mode
            PmxUpdateConstraint(s_CpuPolicyInfo.hDvfsConstraint,
                        CONSTRAINT_MSG_DVFS_REQUEST,
                        (void*)&opm,
                        sizeof(DWORD)
                        );

            s_requestedOpm = opm;
            }

        if (s_currentOpm == kOpm0)
            {
            // clear bucket info
            //
            lastTick = lastIdle = 0;
            memset(pBucketInfo, 0,
                sizeof(BucketInfo_t) +
                (sizeof(DWORD) * s_CpuPolicyInfo.bucketCount)
                );

            // rely on cpu load monitor to check for
            // cpu load threshold checks
            timeOut = INFINITE;
            s_bSleeping = TRUE;
            ResetEvent(s_CpuPolicyInfo.hCpuLoadEvent);
            InterruptMask(s_CpuPolicyInfo.sysIntr, FALSE);
            }
        else
            {
            // Disable cpu monitor interrupt as we want
            // to wake periodically to check cpu load
            //
            timeOut = s_CpuPolicyInfo.dwMonitorPeriod;

            // update tick info
            lastTick = currentTick;
            lastIdle = currentIdle;
            }
        code = WaitForSingleObject(s_CpuPolicyInfo.hCpuLoadEvent, timeOut);

        // If interrupt was generated disable cpu idle interrupt
        if (code == WAIT_OBJECT_0)
            {
            s_bSleeping = FALSE;
            InterruptMask(s_CpuPolicyInfo.sysIntr, TRUE);
            }
        }

    return 0;
}

//-----------------------------------------------------------------------------
//
//  Function:  CPULD_InitPolicy
//
//  Policy  initialization
//
HANDLE
CPULD_InitPolicy(
    _TCHAR const *szContext
    )
{
    int i, j;
    int opmCount;
    DWORD opmFreq;
    HANDLE ret = NULL;
    _TCHAR szRegKey[MAX_PATH];
    DWORD cpuFamily = CPU_FAMILY_OMAP35XX;

    // initializt global structure
    memset(&s_CpuPolicyInfo, 0, sizeof(CpuPolicyInfo_t));
	
    KernelIoControl(
        IOCTL_HAL_GET_CPUFAMILY,
        &cpuFamily,
        sizeof(DWORD),
        &cpuFamily,
        sizeof(DWORD),
        NULL
        );

    _tcscpy_s(szRegKey, _countof(szRegKey), szContext);
	
    if( cpuFamily == CPU_FAMILY_DM37XX)
    {
        _tcscat(szRegKey, _T("\\37xx"));
    }
    else if( cpuFamily == CPU_FAMILY_OMAP35XX)
    {
        _tcscat(szRegKey, _T("\\35xx"));
    }
    else if( cpuFamily == CPU_FAMILY_AM35XX)
    {
        _tcscat(szRegKey, _T("\\3517"));
    }
    else
    {
        RETAILMSG(ZONE_ERROR,(L"CPULD_InitPolicy: Unsupported CPU family=(%x)", cpuFamily));
        goto cleanUp;
    }

    // Read policy registry params
    if (GetDeviceRegistryParams(
        szContext, &s_CpuPolicyInfo, dimof(g_policyRegParams),
        g_policyRegParams) != ERROR_SUCCESS)
        {
        RETAILMSG(ZONE_ERROR, (L"CPULD_InitPolicy: Invalid/Missing "
            L"registry parameters.  Unloading policy\r\n")
            );
        goto cleanUp;
        }
    // Read policy registry params
    if (GetDeviceRegistryParams(
        szRegKey, &s_CpuPolicyInfo, dimof(g_CpuFamilypolicyRegParams),
        g_CpuFamilypolicyRegParams) != ERROR_SUCCESS)
        {
        RETAILMSG(ZONE_ERROR, (L"CPULD_InitPolicy: Invalid/Missing "
            L"registry parameters.  Unloading policy\r\n")
            );
        goto cleanUp;
        }

    // floor frequency
    s_CpuPolicyInfo.rgOpmRange[0].floor = 0;
    s_CpuPolicyInfo.rgOpmRange[0].ceiling = s_CpuPolicyInfo.rgOpmThreshold[0] << FRACTION_PART;

    // set upper bound to max frequency
    opmFreq = s_CpuPolicyInfo.rgOpmFrequency[s_CpuPolicyInfo.dwCeilingOpm];
    s_CpuPolicyInfo.rgOpmThreshold[s_CpuPolicyInfo.dwCeilingOpm] = opmFreq;

    // everything else in the middle
    i = 0;
    j = 1;
    opmCount = s_CpuPolicyInfo.dwCeilingOpm;
    while (opmCount--)
        {
        // set floor and ceiling
        s_CpuPolicyInfo.rgOpmRange[j].floor = s_CpuPolicyInfo.rgOpmThreshold[i] << FRACTION_PART;
        s_CpuPolicyInfo.rgOpmRange[j].ceiling = s_CpuPolicyInfo.rgOpmThreshold[j] << FRACTION_PART;

        // next count
        j++;
        i++;
        }

    for (i = 0; i <= (signed)s_CpuPolicyInfo.dwCeilingOpm; ++i)
        {
        RETAILMSG(1, (L"frequency=%d, floor=%d, ceiling=%d\r\n",
            s_CpuPolicyInfo.rgOpmFrequency[i],
            s_CpuPolicyInfo.rgOpmRange[i].floor >> FRACTION_PART,
            s_CpuPolicyInfo.rgOpmRange[i].ceiling >> FRACTION_PART
            ));
        }

    // Open boot named event
    s_CpuPolicyInfo.hBootEvent = CreateEvent(NULL,
                                        TRUE,
                                        FALSE,
                                        s_CpuPolicyInfo.szBootEventName
                                        );
    if (s_CpuPolicyInfo.hBootEvent == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"CPULD_InitPolicy: Invalid/Missing "
            L"BootEventName(%s).  Unloading policy\r\n",
            s_CpuPolicyInfo.szBootEventName)
            );
        goto cleanUp;
        }

    // Map SW CPUMONITOR interrupt
    if (!KernelIoControl(
            IOCTL_HAL_REQUEST_SYSINTR, &s_CpuPolicyInfo.irq,
            sizeof(s_CpuPolicyInfo.irq),
            &s_CpuPolicyInfo.sysIntr, sizeof(s_CpuPolicyInfo.sysIntr),
            NULL
            ))
        {
        goto cleanUp;
        }

    // Create interrupt event
    s_CpuPolicyInfo.hCpuLoadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (s_CpuPolicyInfo.hCpuLoadEvent == NULL)
        {
        goto cleanUp;
        }

    // bind system interrupt with event
    InterruptInitialize(s_CpuPolicyInfo.sysIntr,
                s_CpuPolicyInfo.hCpuLoadEvent,
                &s_CpuPolicyInfo.dwInterruptThreshold,
                sizeof(s_CpuPolicyInfo.dwInterruptThreshold)
                );

    // don't enable interrupt until monitor thread is active
    InterruptMask(s_CpuPolicyInfo.sysIntr, TRUE);

    // Set boot opm value
    s_currentOpm = s_CpuPolicyInfo.dwBootOpm;
    s_requestedOpm = s_CpuPolicyInfo.dwBootOpm;

    // Obtain DVFS constraint handler
    s_CpuPolicyInfo.hDvfsConstraint = PmxSetConstraintById(
                                    CONSTRAINT_ID_DVFS,
                                    CONSTRAINT_MSG_DVFS_REQUEST,
                                    (void*)&s_currentOpm,
                                    sizeof(DWORD)
                                    );

    // register for DVFS change notifications
    s_CpuPolicyInfo.hDvfsCallback = PmxRegisterConstraintCallback(
                                        s_CpuPolicyInfo.hDvfsConstraint,
                                        DvfsConstraintCallback,
                                        NULL,
                                        0,
                                        (HANDLE)&s_CpuPolicyInfo
                                        );

    // allocate buffer to hold history info
    if (s_CpuPolicyInfo.dwMonitorWindow < s_CpuPolicyInfo.dwMonitorPeriod)
        {
        s_CpuPolicyInfo.dwMonitorWindow = s_CpuPolicyInfo.dwMonitorPeriod;
        }

    s_CpuPolicyInfo.bucketCount = s_CpuPolicyInfo.dwMonitorWindow / s_CpuPolicyInfo.dwMonitorPeriod;
    s_CpuPolicyInfo.pBucketInfo = (BucketInfo_t*)LocalAlloc(LPTR,
                                    sizeof(BucketInfo_t) +
                                    (sizeof(DWORD) * s_CpuPolicyInfo.bucketCount)
                                    );

    memset(s_CpuPolicyInfo.pBucketInfo, 0,
                sizeof(BucketInfo_t) +
                (sizeof(DWORD) * s_CpuPolicyInfo.bucketCount)
                );

    // Start running the thread that will check for the cpu load
    s_CpuPolicyInfo.hCpuLoadThread = CreateThread(NULL, 0,
                                            CpuLoadThreadFn, NULL, 0, NULL
                                            );

    CeSetThreadPriority(s_CpuPolicyInfo.hCpuLoadThread,
                      s_CpuPolicyInfo.priority256
                      );

    ret = (HANDLE)&s_CpuPolicyInfo;

cleanUp:
    return ret;
}

//-----------------------------------------------------------------------------
//
//  Function:  CPULD_DeinitPolicy
//
//  Policy uninitialization
//
BOOL
CPULD_DeinitPolicy(
    HANDLE hPolicyAdapter
    )
{
    BOOL rc = FALSE;

    // validate parameters
    if (hPolicyAdapter != (HANDLE)&s_CpuPolicyInfo) goto cleanUp;

    if (s_CpuPolicyInfo.hCpuLoadThread != NULL)
        {
        s_CpuPolicyInfo.bExit = TRUE;
        SetEvent(s_CpuPolicyInfo.hCpuLoadEvent);
        WaitForSingleObject(s_CpuPolicyInfo.hCpuLoadThread, INFINITE);
        CloseHandle(s_CpuPolicyInfo.hCpuLoadThread);
        }

    // release interrupt resources
    if (s_CpuPolicyInfo.sysIntr != 0)
        {
        InterruptDisable(s_CpuPolicyInfo.sysIntr);
        }

    // release OS resources
    if (s_CpuPolicyInfo.hBootEvent != NULL)
        {
        CloseHandle(s_CpuPolicyInfo.hBootEvent);
        }

    if (s_CpuPolicyInfo.hCpuLoadEvent != NULL)
        {
        CloseHandle(s_CpuPolicyInfo.hCpuLoadEvent);
        }

    // release all PM resoureces
    if (s_CpuPolicyInfo.hDvfsConstraint != NULL)
        {
        if (s_CpuPolicyInfo.hDvfsCallback != NULL)
            {
            PmxUnregisterConstraintCallback(s_CpuPolicyInfo.hDvfsConstraint,
                s_CpuPolicyInfo.hDvfsCallback
                );
            }
        PmxReleaseConstraint(s_CpuPolicyInfo.hDvfsConstraint);
        }

    if (s_CpuPolicyInfo.pBucketInfo != NULL)
        {
        LocalFree(s_CpuPolicyInfo.pBucketInfo);
        }

    // clear structures
    memset(&s_CpuPolicyInfo, 0, sizeof(CpuPolicyInfo_t));

    rc = TRUE;

cleanUp:
    return rc;
}


//-----------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);

    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//-----------------------------------------------------------------------------

