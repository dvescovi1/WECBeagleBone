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
//  File:  omap_prof.h
//
//  This file contains OMAP specific profile extensions.
//
#ifndef __OMAP_PROF_H
#define __OMAP_PROF_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning (push)
#pragma warning (disable:4245 4201)

//-----------------------------------------------------------------------------
// profile markers
#define PROFILE_CORE1_DVFS_BEGIN                0
#define PROFILE_CORE1_DVFS_END                  1
#define PROFILE_WAKEUP_TIMER_MATCH              2
#define PROFILE_WAKEUP_TIMER_MATCH_ORIGINAL     3

#define PROFILE_WAKEUP_LATENCY_CHIP_OFF         4
#define PROFILE_WAKEUP_LATENCY_CHIP_OSWR        5
#define PROFILE_WAKEUP_LATENCY_CHIP_CSWR        6
#define PROFILE_WAKEUP_LATENCY_CORE_CSWR        7
#define PROFILE_WAKEUP_LATENCY_CORE_INACTIVE    8
#define PROFILE_WAKEUP_LATENCY_MPU_INACTIVE     9

#define PROFILE_WAKEUP_TIMER_CHIP_OFF           10
#define PROFILE_WAKEUP_TIMER_CHIP_OSWR          11
#define PROFILE_WAKEUP_TIMER_CHIP_CSWR          12
#define PROFILE_WAKEUP_TIMER_CORE_CSWR          13
#define PROFILE_WAKEUP_TIMER_CORE_INACTIVE      14
#define PROFILE_WAKEUP_TIMER_MPU_INACTIVE       15

#define PROFILE_COUNT                           16

//-----------------------------------------------------------------------------
#ifndef SHIP_BUILD
#define OMAP_PROFILE_START()                OmapProfilerStart()
#define OMAP_PROFILE_STOP()                 OmapProfilerStop()
#define OMAP_PROFILE_MARK(x, y)             OmapProfilerMark(x, (void*)y)
#else
#define OMAP_PROFILE_START()
#define OMAP_PROFILE_STOP()
#define OMAP_PROFILE_MARK(x, y)       
#endif

//-----------------------------------------------------------------------------
typedef struct ProfilerControlEx {
    DWORD dwVersion;                // Version of this struct, set to 1
    DWORD dwOptions;                // PROFILE_* flags
    DWORD dwReserved;
    union {
        // Used to control the kernel profiler, if PROFILE_OEMDEFINED is not set
        struct {
            DWORD dwUSecInterval;   // Sampling interval
        } Kernel;
        
        // Used to control an OEM-defined profiler, if PROFILE_OEMDEFINED is set
        struct {
            DWORD dwProcessorType;  // Type of processor expected
            DWORD dwControlSize;    // Size of the OEM-defined data
            //BYTE  bHardwareSpecificSettings[0]; // Followed by OEM-defined data
            DWORD dwCount;
            DWORD rgTargets[PROFILE_COUNT];
            DWORD rgValues[PROFILE_COUNT];
        } OEM;
    };
} ProfilerControlEx;


//-----------------------------------------------------------------------------
void
OmapProfilerMark(
    DWORD id,
    void *pv
    );

//-----------------------------------------------------------------------------
void
OmapProfilerStart(
    );

//-----------------------------------------------------------------------------
void
OmapProfilerStop(
    );

//------------------------------------------------------------------------------
void
OmapProfilerQuery(
    DWORD count,
    DWORD target[],
    DWORD value[]
    );

#pragma warning (pop)

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif // __OMAP_PROF_H

