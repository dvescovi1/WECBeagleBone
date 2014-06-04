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
//  File: clock.cpp
//
#include "omap.h"
#include "oalex.h"
#include "oal_clock.h"

//-----------------------------------------------------------------------------
static OMAP_DEVCLKMGMT_FNTABLE clockFnTable;
static BOOL	bclockTableInit = FALSE;

//-----------------------------------------------------------------------------
static
BOOL
ClockInitialize()
{
    if (bclockTableInit == FALSE)
    {
        if (KernelIoControl(IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE, NULL, 0,(void*)&clockFnTable,
            sizeof(OMAP_DEVCLKMGMT_FNTABLE),  NULL))
        {
            bclockTableInit = TRUE;
        }
        else
        {
            memset(&clockFnTable,0,sizeof(clockFnTable));
        }
    }
    return bclockTableInit;
}

//-----------------------------------------------------------------------------

BOOL
EnableDeviceClockAutoIdle(
			  OMAP_DEVICE	devID,
			  BOOL fEnable
			 )
{
	if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnEnableDeviceClockAutoIdle)
	    return clockFnTable.pfnEnableDeviceClockAutoIdle( devID, fEnable );
    return FALSE;
}

//-----------------------------------------------------------------------------

BOOL
EnableDeviceClocks(
			  OMAP_DEVICE	devID,
			  BOOL			bEnable
			 )
{
	if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnEnableDeviceClocks)
	    return clockFnTable.pfnEnableDeviceClocks( devID, bEnable );
    return FALSE;
}


//-----------------------------------------------------------------------------

BOOL
EnableDeviceIClock(
			  OMAP_DEVICE	devID,
			  BOOL			bEnable
			 )
{
	if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnEnableDeviceIClock)
	    return clockFnTable.pfnEnableDeviceIClock( devID, bEnable );
    return FALSE;
}

//-----------------------------------------------------------------------------

BOOL
EnableDeviceFClock(
			  OMAP_DEVICE	devID,
			  BOOL			bEnable
			 )
{
	if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnEnableDeviceFClock)
	    return clockFnTable.pfnEnableDeviceFClock( devID, bEnable );
    return FALSE;
}

//-----------------------------------------------------------------------------

BOOL
SelectDeviceSourceClocks(
	 OMAP_DEVICE		devID,
     UINT count,
	 OMAP_CLOCK	clockIDs[]
	 )
{
	if (!bclockTableInit) ClockInitialize();

    if (clockFnTable.pfnSetSourceDeviceClocks)
	    return clockFnTable.pfnSetSourceDeviceClocks( devID, count, clockIDs );
    return FALSE;
}

//-----------------------------------------------------------------------------

DWORD
GetSystemClockFrequency()
{
	if (!bclockTableInit) ClockInitialize();
    
    return (DWORD) clockFnTable.pfnGetSystemClockFrequency();
}

BOOL
PrcmDeviceGetContextState(
    UINT                    devId, 
    BOOL                    bSet
    )
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnGetDeviceContextState)
	    return clockFnTable.pfnGetDeviceContextState( devId, bSet );
    return FALSE;

}

BOOL PrcmDomainResetStatus(UINT powerDomain, UINT *pResetStatus, BOOL bClear)
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnPrcmDomainResetStatus)
	    return clockFnTable.pfnPrcmDomainResetStatus( powerDomain, pResetStatus, bClear );
    return FALSE;
}

BOOL PrcmDomainReset(UINT powerDomain, UINT resetMask)
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnPrcmDomainReset)
	    return clockFnTable.pfnPrcmDomainReset( powerDomain, resetMask );
    return FALSE;
}
BOOL PrcmDomainResetRelease(UINT powerDomain, UINT resetMask)
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnPrcmDomainResetRelease)
	    return clockFnTable.pfnPrcmDomainResetRelease( powerDomain, resetMask );
    return FALSE;
}

UINT32 PrcmClockGetClockRate(OMAP_CLOCKID clock_id)
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnPrcmClockGetClockRate)
	    return clockFnTable.pfnPrcmClockGetClockRate( clock_id);
    return 0;
}

void PrcmCM3ResetAndHandshake()
{
    if (!bclockTableInit) ClockInitialize();
    if (clockFnTable.pfnPrcmCM3ResetAndHandshake)
	    clockFnTable.pfnPrcmCM3ResetAndHandshake();
}

