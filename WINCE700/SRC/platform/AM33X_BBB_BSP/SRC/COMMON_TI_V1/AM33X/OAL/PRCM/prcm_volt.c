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
//  File: prcm_volt.c
//

#include "omap.h"
#include "omap_prof.h"
#include "am33x.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"

//------------------------------------------------------------------------------
void
PrcmVoltSetControlMode(
    UINT                    voltCtrlMode,
    UINT                    voltCtrlMask
    )
{
	UNREFERENCED_PARAMETER(voltCtrlMode);
	UNREFERENCED_PARAMETER(voltCtrlMask);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetControlPolarity(
    UINT                    polMode,
    UINT                    polModeMask
    )
{    
	UNREFERENCED_PARAMETER(polMode);
	UNREFERENCED_PARAMETER(polModeMask);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetAutoControl(
    UINT                    autoCtrlMode,
    UINT                    autoCtrlMask
    )
{    
	UNREFERENCED_PARAMETER(autoCtrlMode);
	UNREFERENCED_PARAMETER(autoCtrlMask);
}

//-----------------------------------------------------------------------------
void
PrcmVoltI2cInitialize(
    VoltageProcessor_e      vp,
    UINT8                   slaveAddr,
    UINT8                   cmdAddr,
    UINT8                   voltAddr,
    BOOL                    bUseCmdAddr
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(slaveAddr);
	UNREFERENCED_PARAMETER(cmdAddr);
	UNREFERENCED_PARAMETER(voltAddr);
	UNREFERENCED_PARAMETER(bUseCmdAddr);
}

//------------------------------------------------------------------------------
void
PrcmVoltI2cSetHighSpeedMode(
    BOOL                    bHSMode,
    BOOL                    bRepeatStartMode,
    UINT8                   mcode
    )
{
	UNREFERENCED_PARAMETER(bHSMode);
	UNREFERENCED_PARAMETER(bRepeatStartMode);
	UNREFERENCED_PARAMETER(mcode);
}

//------------------------------------------------------------------------------
void
PrcmVoltInitializeVoltageLevels(
    VoltageProcessor_e      vp,
    UINT                    vddOn,
    UINT                    vddOnLP,
    UINT                    vddRetention,
    UINT                    vddOff
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(vddOn);
	UNREFERENCED_PARAMETER(vddOnLP);
	UNREFERENCED_PARAMETER(vddRetention);
	UNREFERENCED_PARAMETER(vddOff);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetErrorConfiguration(
    VoltageProcessor_e      vp,
    UINT                    errorOffset,
    UINT                    errorGain
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(errorOffset);
	UNREFERENCED_PARAMETER(errorGain);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetSlewRange(
    VoltageProcessor_e      vp,
    UINT                    minVStep,
    UINT                    minWaitTime,
    UINT                    maxVStep,
    UINT                    maxWaitTime
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(minVStep);
	UNREFERENCED_PARAMETER(minWaitTime);
	UNREFERENCED_PARAMETER(maxVStep);
	UNREFERENCED_PARAMETER(maxWaitTime);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetLimits(
    VoltageProcessor_e      vp,
    UINT                    minVolt,
    UINT                    maxVolt,
    UINT                    timeOut
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(minVolt);
	UNREFERENCED_PARAMETER(maxVolt);
	UNREFERENCED_PARAMETER(timeOut);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetVoltageLevel(
    VoltageProcessor_e      vp,
    UINT                    vdd,
    UINT                    mask
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(vdd);
	UNREFERENCED_PARAMETER(mask);
}

//------------------------------------------------------------------------------
void
PrcmVoltSetInitVddLevel(
    VoltageProcessor_e      vp,
    UINT                    initVolt
    )
{
    UNREFERENCED_PARAMETER(vp);
    UNREFERENCED_PARAMETER(initVolt);
}

//------------------------------------------------------------------------------
void
PrcmVoltEnableTimeout(
    VoltageProcessor_e      vp,
    BOOL                    bEnable
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(bEnable);
}

//------------------------------------------------------------------------------
void
PrcmVoltEnableVp(
    VoltageProcessor_e      vp,
    BOOL                    bEnable
    )
{
	UNREFERENCED_PARAMETER(vp);
	UNREFERENCED_PARAMETER(bEnable);
}

//------------------------------------------------------------------------------
void
PrcmVoltFlushVoltageLevels(
    VoltageProcessor_e      vp
    )
{
	UNREFERENCED_PARAMETER(vp);
}

//-----------------------------------------------------------------------------
BOOL
PrcmVoltIdleCheck(
    VoltageProcessor_e      vp
    )
{
    BOOL rc = FALSE;

	UNREFERENCED_PARAMETER(vp);

	return rc;
}

//-----------------------------------------------------------------------------
UINT
PrcmVoltGetVoltageRampDelay(
    VoltageProcessor_e      vp
    )
{
    UINT delay = 0;

	UNREFERENCED_PARAMETER(vp);

	return delay;
}
