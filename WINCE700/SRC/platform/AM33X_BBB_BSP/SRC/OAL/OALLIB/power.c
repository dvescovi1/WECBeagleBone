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

#include "bsp.h"
#include "bsp_cfg.h"
#include "oalex.h"
#include "am33x_clocks.h"
#include "am33x_prcm.h"
#include "am33x_oal_prcm.h"

extern void OEMDeinitDebugSerial();
extern void OEMInitDebugSerial();

#if 0
//-----------------------------------------------------------------------------
//
//  Static :  s_enableSmartReflex
//
//  Saves smartreflex states 
//  
static BOOL s_enableSmartReflex1;
static BOOL s_enableSmartReflex2;
#endif

//-----------------------------------------------------------------------------
// prototypes
//
extern BOOL IsSmartReflexMonitoringEnabled(UINT channel);
extern BOOL SmartReflex_EnableMonitor(UINT channel, BOOL bEnable);
extern VOID OALTimerStop(VOID);
extern void PrcmDebugChkAndDisableDevice(UINT devId, DWORD * preMMode, DWORD * postIdleSt, DWORD * postStdbySt, DWORD *refCount);


#ifdef DEBUG_PRCM_SUSPEND_RESUME

extern PTCHAR DeviceNames[];

AM33X_DEVICE_ID skipCheckDeviceList[] = 
{   
    AM_DEVICE_CONTROL,
    AM_DEVICE_L4WKUP,
    AM_DEVICE_SMARTREFLEX0,
    AM_DEVICE_SMARTREFLEX1,
    AM_DEVICE_UART0,
    AM_DEVICE_WDT1,
    AM_DEVICE_WKUP_M3,
    AM_DEVICE_CLKDIV32K,
    AM_DEVICE_EMIF,
    AM_DEVICE_EMIF_FW,
    AM_DEVICE_L3,
    AM_DEVICE_L3_INSTR,
    AM_DEVICE_L4FW,
    AM_DEVICE_L4LS,
    AM_DEVICE_L4_HS,
    AM_DEVICE_MAILBOX0,
    AM_DEVICE_MSTR_EXPS,
    AM_DEVICE_OCMCRAM,
    AM_DEVICE_OCPWP,
    AM_DEVICE_SLV_EXPS,
    AM_DEVICE_TIMER2,
    AM_DEVICE_TIMER3,
    AM_DEVICE_MPU,
    AM_DEVICE_RTC,
    AM_DEVICE_CEFUSE,
    AM_DEVICE_EFUSE,
    AM_DEVICE_GPIO0,
    AM_DEVICE_GPIO1,
    AM_DEVICE_GPIO2,
    AM_DEVICE_GPIO3,
    AM_DEVICE_CPGMAC0,
    AM_DEVICE_DEBUGSS,
    AM_DEVICE_TIMER1
};

BOOL DebugEnableDeviceFlag[AM_DEVICE_COUNT];

BOOL skipDevice(UINT32 devId)
{
    DWORD i=0;
    for (i=0; i<sizeof(skipCheckDeviceList)/sizeof(AM33X_DEVICE_ID); i++)
    {
        if (skipCheckDeviceList[i]==devId) return TRUE;
    }
    return FALSE;
}

void BSPDebugTurnOFFClocks()
{
    DWORD devId=0;
    DWORD preMMode;
    DWORD postIdleSt;
    DWORD postStdbySt;
    DWORD refCount;

    memset(DebugEnableDeviceFlag,0,sizeof(DebugEnableDeviceFlag));
    
    for (devId=0; devId<AM_DEVICE_COUNT; devId++)
    {  
        if (skipDevice(devId)) continue;
        PrcmDebugChkAndDisableDevice(devId,&preMMode,&postIdleSt,&postStdbySt,&refCount);
        if ((preMMode == ClKCTRL_MODULEMODE_EN) || (refCount >0)) {
            OALMSG(1,(L"BSPDebugTurnOFFClocks: %s devId=%d preMMode=%d postIdleSt=%d postStdbySt=%d refCount=%d\r\n",
                                                DeviceNames[devId], devId,preMMode,postIdleSt,postStdbySt,refCount));
            DebugEnableDeviceFlag[devId]=TRUE;
        }
    }
    
}

void BSPDebugTurnONClocks()
{
    DWORD devId=0;    
    for (devId=0; devId<AM_DEVICE_COUNT; devId++)
    {        
        if (skipDevice(devId)) continue;
        if (DebugEnableDeviceFlag[devId]==TRUE) PrcmDeviceEnableClocks(devId,TRUE);
    }
    
}
#endif
    
//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID
BSPPowerOff(
    )
{
    // enable this code after smart reflex is implemented
#if 0
    // Disable Smartreflex if enabled.
    if (IsSmartReflexMonitoringEnabled(kSmartReflex_Channel1))
        {
        s_enableSmartReflex1 = TRUE;
        SmartReflex_EnableMonitor(kSmartReflex_Channel1, FALSE);
        }
    else
        {
        s_enableSmartReflex1 = FALSE;
        }

    if (IsSmartReflexMonitoringEnabled(kSmartReflex_Channel2))
        {
        s_enableSmartReflex2 = TRUE;
        SmartReflex_EnableMonitor(kSmartReflex_Channel2, FALSE);
        }
    else
        {
        s_enableSmartReflex2 = FALSE;
        }
#endif

    // this is for debugging only - ideally all the drivers should takecare of their modules/clocks when going into suspend
#ifdef DEBUG_PRCM_SUSPEND_RESUME
    BSPDebugTurnOFFClocks();
#endif

    // stop GPTIMER1
    OALTimerStop();

    // for debugging purpose, dont turn off the debug UART - anyways it belongs to WAKEUP domain, so it shouldn't interfere with deepsleep modes.
#ifndef DEBUG_PRCM_SUSPEND_RESUME
    if (g_oalRetailMsgEnable)
	{
        OEMDeinitDebugSerial();
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,FALSE);
    }
#endif    
}

//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID
BSPPowerOn(
    )
{
    // enable this code after smart reflex is implemented
#if 0
    if (s_enableSmartReflex1)
        {
        SmartReflex_EnableMonitor(kSmartReflex_Channel1, TRUE);
        }

    if (s_enableSmartReflex2)
        {
        SmartReflex_EnableMonitor(kSmartReflex_Channel2, TRUE);
        }

#endif

#ifndef DEBUG_PRCM_SUSPEND_RESUME
    if (g_oalRetailMsgEnable)
    {
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE);
	    OEMInitDebugSerial();
	}
#endif    
    g_ResumeRTC = TRUE;

    // this is for debugging only - ideally all the drivers should takecare of their modules/clocks when going into suspend
#ifdef DEBUG_PRCM_SUSPEND_RESUME
    BSPDebugTurnONClocks();
#endif
}

void BSPCPUIdle()
{
    fnOALCPUIdle(g_pCPUInfo);
}

//------------------------------------------------------------------------------
