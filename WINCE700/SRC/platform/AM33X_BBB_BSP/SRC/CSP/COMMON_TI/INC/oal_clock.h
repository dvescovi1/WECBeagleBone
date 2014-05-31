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
//  File:  oal_clock.h
//
#ifndef _OAL_CLOCK_H
#define _OAL_CLOCK_H

#include <sal.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

BOOL
EnableDeviceClockAutoIdle(
			  OMAP_DEVICE	devID,
			  BOOL fEnable
			 );

BOOL
EnableDeviceClocks(
    OMAP_DEVICE		devID,
    BOOL			bEnable
    );

BOOL
PrcmDeviceGetContextState(
    UINT                    devId, 
    BOOL                    bSet
    );

BOOL
SelectDeviceSourceClocks(
	 OMAP_DEVICE		devID,
     UINT count,
	 OMAP_CLOCK	clockIDs[]
	 );

BOOL
GetSourceClocks(
	 OMAP_DEVICE		devID,
     UINT count,
	 __out_ecount(count) OMAP_CLOCK	clockID[],
     UINT* pOutCount
	 );

DWORD
GetSystemClockFrequency();

UINT32 PrcmClockGetClockRate(OMAP_CLOCKID clock_id);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
