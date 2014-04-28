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
//  File: prcm_device.c
//
#include "am33x.h"
#include "am33x_clocks.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"
#include "prcm_device.h"
#include <oal.h>


extern ClockDomainLookupEntry s_rgClkDomainLookupTable[];
extern BOOL g_bSingleThreaded;

BOOL _PrcmHwEnableModuleClocks(AM33X_DEVICE_ID devId, BOOL bEnable)
{
	BOOL bUpdateModuleMode = FALSE;
	
	if (devId >= AM_DEVICE_COUNT) return FALSE;   

    OALMSG(OAL_FUNC,(L"_PrcmHwEnableModuleClocks devId:%d bEnable:%d refcount:%d\r\n",devId, bEnable, s_rgDeviceLookupTable[devId].refCount));
    
    if (bEnable != FALSE)
    {
        if (InterlockedIncrement(&s_rgDeviceLookupTable[devId].refCount) == 1 )
        {
            bUpdateModuleMode = TRUE;
        }
    }
    else if (s_rgDeviceLookupTable[devId].refCount > 0)
    {
        if (InterlockedDecrement(&s_rgDeviceLookupTable[devId].refCount) == 0)
        {
            bUpdateModuleMode = TRUE;
        }
    }    
    if (bUpdateModuleMode && (s_rgDeviceLookupTable[devId].CLKCTRL_REG != 0))
    {
        UINT32   timeout = 2000000;
	    volatile unsigned int *pmodulemode;
        UINT32 val = 0;
        
    	pmodulemode = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_rgDeviceLookupTable[devId].CLKCTRL_REG);
        val = ((INREG32(pmodulemode) & ~ClKCTRL_MODULEMODE_MASK) | 
               ((bEnable)? ClKCTRL_MODULEMODE_EN : ClKCTRL_MODULEMODE_DIS));
    	OUTREG32(pmodulemode, val);
    	if (bEnable){
    		while((timeout) && (INREG32(pmodulemode) & ClKCTRL_IDLEST_DIS))
    			timeout--;
    		if (timeout == 0)
    			OALMSG(1,(L"_PrcmHwEnableModuleClocks: CANNOT enable clock for devId=%d\r\n", devId));
    	}
    }	
	return TRUE;	
}

UINT PrcmDeviceGetClockDevice(UINT devId)
{
    if ((devId >= AM_DEVICE_END) || (devId == AM_DEVICE_COUNT)) return AM_DEVICE_END;
    if (devId > AM_DEVICE_COUNT)
        return (s_rgDeviceRemapTable[(devId-1)-AM_DEVICE_COUNT]);  
    
    return devId;
    
}
//-----------------------------------------------------------------------------
//
//  Function:  PrcmDeviceGetEnabledState
//
//  returns current activity state of the device
//
BOOL
PrcmDeviceGetEnabledState(
    UINT devId,
    BOOL *pbEnable
    )
{
    BOOL rc = FALSE;
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"+PrcmDeviceGetEnabledState(devId=%d)\r\n", devId));

    devId = PrcmDeviceGetClockDevice(devId);
    if (devId >= AM_DEVICE_END) goto cleanUp;
        
    *pbEnable = s_rgDeviceLookupTable[devId].refCount != 0;
    rc = TRUE;

cleanUp:
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"-PrcmDeviceGetEnabledState()=%d\r\n", rc));
    return rc;
}
//-----------------------------------------------------------------------------
// Sets the clock source for a device
// Not implemented as devID is not enough to identify which clock for the device we want to set the Source for.
// Dont see the need for this function yet. Will implement as needed.
BOOL PrcmDeviceSetSourceClocks( UINT devId, UINT count, UINT rgClocks[])
{
    BOOL rc = FALSE;
    if (!g_bSingleThreaded) 
		OALMSG(OAL_FUNC, (L"+PrcmDeviceSetSourceClocks(devId=%d), %d\r\n",
				devId, count ));
    
    devId = PrcmDeviceGetClockDevice(devId);
    if (devId >= AM_DEVICE_END) goto cleanUp;
    
    Lock(Mutex_DeviceClock);
    
    OALMSG(1, (L"+PrcmDeviceSetSourceClocks(devId=%d) IF YOU NEED ME, IMPLEMENT ME \r\n",
				devId ));
        
    Unlock(Mutex_DeviceClock);    
    
cleanUp:    
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"-PrcmDeviceSetSourceClocks()=%d\r\n", rc));
    return rc;
}   

//------------------------------------------------------------------------------
//
//  Function:  PrcmDeviceEnableIClock
//
//  Enables the appropriate interface clock
//
BOOL PrcmDeviceEnableIClock( UINT devId, BOOL bEnable){    
    BOOL rc = TRUE;
    UINT32 iclk = 0;
    OALMSG(OAL_FUNC, (L"+PrcmDeviceEnableIClock(devId=%d, %d)\r\n", devId, bEnable));    
    
    devId = PrcmDeviceGetClockDevice(devId);
    if (devId >= AM_DEVICE_END) goto cleanUp;
    
    if (s_rgDeviceLookupTable[devId].iclk.size == 0) goto cleanUp;

    Lock(Mutex_DeviceClock);
    
    if (!bEnable) _PrcmHwEnableModuleClocks(devId,bEnable);

    for (iclk=0;iclk<s_rgDeviceLookupTable[devId].iclk.size;iclk++)
    {
        int clkId = s_rgDeviceLookupTable[devId].iclk.rgSourceClocks[iclk];
        if (bEnable)
            ClockUpdateParentClock(clkId,bEnable);

        PrcmClockEnableClockDomain(clkId,bEnable);
        
        if (!bEnable)
            ClockUpdateParentClock(clkId,bEnable);                
    }
    
    if (bEnable) _PrcmHwEnableModuleClocks(devId,bEnable);
    
    PrcmDomainUpdateRefCount(s_rgDeviceLookupTable[devId].powerDomain, bEnable);   

    Unlock(Mutex_DeviceClock);    
cleanUp:    
    OALMSG(OAL_FUNC, (L"-PrcmDeviceEnableIClock()=%d\r\n", rc));    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PrcmDeviceEnableFClock
//
//  Enables the appropriate functional clock
//
BOOL PrcmDeviceEnableFClock(UINT devId, BOOL bEnable){
    
    BOOL rc = TRUE;
    UINT fclk = 0;
    
    OALMSG(OAL_FUNC, (L"+PrcmDeviceEnableFClock(devId=%d, %d)\r\n", devId, bEnable));
    
    devId = PrcmDeviceGetClockDevice(devId);
    if (devId >= AM_DEVICE_END) goto cleanUp;
    
    if (s_rgDeviceLookupTable[devId].fclk.size==0) goto cleanUp;

    Lock(Mutex_DeviceClock);
    
    if (!bEnable) _PrcmHwEnableModuleClocks(devId,bEnable);

    for (fclk=0;fclk<s_rgDeviceLookupTable[devId].fclk.size;fclk++)
    {
        int clkId = s_rgDeviceLookupTable[devId].fclk.rgSourceClocks[fclk];        
        if (bEnable)
            ClockUpdateParentClock(clkId,bEnable);

        PrcmClockEnableClockDomain(clkId,bEnable);
        
        if (!bEnable)
            ClockUpdateParentClock(clkId,bEnable);                
    }

    if (bEnable) _PrcmHwEnableModuleClocks(devId,bEnable);
    
    PrcmDomainUpdateRefCount(s_rgDeviceLookupTable[devId].powerDomain, bEnable);   

    Unlock(Mutex_DeviceClock);    

cleanUp:    
    OALMSG(OAL_FUNC, (L"-PrcmDeviceEnableFClock()=%d\r\n", rc));

    return rc;
   
    
}

BOOL EnableDeviceClocks(AM33X_DEVICE_ID devID, BOOL bEnable)
{    
    return PrcmDeviceEnableClocks(devID,bEnable);
}

//------------------------------------------------------------------------------
//
//  Function:  PrcmDeviceEnableClocks
//
//  Enables the appropriate functional and interface clocks
//
BOOL PrcmDeviceEnableClocks( UINT devId, BOOL bEnable)
//  Enables the appropriate functional and interface clocks
{
//	UINT devIdSave = devId;
    UINT oldState = bEnable ? D4 : D0;
    UINT newState = bEnable ? D0 : D4;

	if (devId >= AM_DEVICE_END) return FALSE;

    OALMSG(0, (L"PrcmDeviceEnableClocks %d %d \r\n",devId, bEnable));
  
    OALMux_UpdateOnDeviceStateChange(devId, oldState, newState, TRUE);

    PrcmDeviceEnableIClock(devId, bEnable);
    PrcmDeviceEnableFClock(devId, bEnable);
    
    OALMux_UpdateOnDeviceStateChange(devId, oldState, newState, FALSE);   

    return TRUE;
}


//------------------------------------------------------------------------------
//  retrieves the context state of a device.
//
//  TRUE - context for device is retained
//  FALSE - context for device is reset.
BOOL PrcmDeviceGetContextState(UINT devId, BOOL bSet)
{
    UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(bSet);
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function:  PrcmDeviceGetSourceClockInfo
//
//  returns clock information for a given device
//
BOOL
PrcmDeviceGetSourceClockInfo(
    UINT devId,
    IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pInfo
    )
{
    UINT i, j=0;
    BOOL rc = FALSE;
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"+PrcmDeviceGetSourceClockInfo(devId=%d)\r\n", devId));

    devId = PrcmDeviceGetClockDevice(devId);
    if (devId >= AM_DEVICE_END) goto cleanUp;
    
    for (i = 0; i < s_rgDeviceLookupTable[devId].fclk.size; ++i)
    {
        pInfo->rgSourceClocks[j].nLevel = 1;
        pInfo->rgSourceClocks[j].clockId = s_rgDeviceLookupTable[devId].fclk.rgSourceClocks[i];
        PrcmClockGetParentClockRefcount(pInfo->rgSourceClocks[j].clockId, 
            pInfo->rgSourceClocks[j].nLevel, 
            &pInfo->rgSourceClocks[j].refCount
            );
        ++j;
    }    
    for (i = 0; i < s_rgDeviceLookupTable[devId].iclk.size; ++i)
    {
        pInfo->rgSourceClocks[j].nLevel = 1;
        pInfo->rgSourceClocks[j].clockId = s_rgDeviceLookupTable[devId].iclk.rgSourceClocks[i];
        PrcmClockGetParentClockRefcount(pInfo->rgSourceClocks[j].clockId, 
            pInfo->rgSourceClocks[j].nLevel, 
            &pInfo->rgSourceClocks[j].refCount
            );
        ++j;
    }
    pInfo->count = s_rgDeviceLookupTable[devId].iclk.size+s_rgDeviceLookupTable[devId].fclk.size; //same as j       
    rc = TRUE;

cleanUp:
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"-PrcmDeviceGetSourceClockInfo()=%d\r\n", rc));
    return rc;
}


//-----------------------------------------------------------------------------
BOOL DeviceInitialize(void)
{
	int j;
	volatile unsigned int *pmodulemode;
   

	OALMSG(OAL_FUNC, (L"DeviceInitialize\r\n"));
	// update reference counters for devices enabled by bootloader or earlier
	for (j=0; j<AM_DEVICE_COUNT; j++){
		if (s_rgDeviceLookupTable[j].CLKCTRL_REG == 0)
			continue;
		pmodulemode = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs + s_rgDeviceLookupTable[j].CLKCTRL_REG);
		if ((INREG32(pmodulemode) & ClKCTRL_MODULEMODE_MASK) == ClKCTRL_MODULEMODE_DIS)
			continue;
		OALMSG(OAL_FUNC, (L"DeviceInitialize: %d is enabled\r\n", j));
		if (s_rgDeviceLookupTable[j].fclk.size>0){            
			PrcmDeviceEnableFClock(j,TRUE);
		}
		if (s_rgDeviceLookupTable[j].iclk.size > 0){
            PrcmDeviceEnableIClock(j,TRUE);
		}		
	}
	return TRUE;
}


PTCHAR DeviceNames[] = {    
    L"AM_DEVICE_ADC_TSC",
    L"AM_DEVICE_CONTROL",
    L"AM_DEVICE_DEBUGSS",
    L"AM_DEVICE_GPIO0",
    L"AM_DEVICE_I2C0",
    L"AM_DEVICE_L4WKUP",
    L"AM_DEVICE_SMARTREFLEX0",
    L"AM_DEVICE_SMARTREFLEX1",
    L"AM_DEVICE_TIMER0",
    L"AM_DEVICE_TIMER1",
    L"AM_DEVICE_UART0",               /* 10 */
    L"AM_DEVICE_WDT0",
    L"AM_DEVICE_WDT1",
    L"AM_DEVICE_WKUP_M3",
    L"AM_DEVICE_AES0",         /* PER */
    L"AM_DEVICE_AES1",
    L"AM_DEVICE_CLKDIV32K",
    L"AM_DEVICE_CPGMAC0",
    L"AM_DEVICE_DCAN0",
    L"AM_DEVICE_DCAN1",
    L"AM_DEVICE_ELM",               /* 20 */
    L"AM_DEVICE_EMIF",
    L"AM_DEVICE_EMIF_FW",
    L"AM_DEVICE_EPWM0",
    L"AM_DEVICE_EPWM1",
    L"AM_DEVICE_EPWM2",
    L"AM_DEVICE_GPIO1",
    L"AM_DEVICE_GPIO2",
    L"AM_DEVICE_GPIO3",
    L"AM_DEVICE_GPMC",
    L"AM_DEVICE_I2C1",               /* 30 */
    L"AM_DEVICE_I2C2",
    L"AM_DEVICE_ICSS",
    L"AM_DEVICE_IEEE5000",
    L"AM_DEVICE_L3",
    L"AM_DEVICE_L3_INSTR",
    L"AM_DEVICE_L4FW",
    L"AM_DEVICE_L4LS",
    L"AM_DEVICE_L4_HS",
    L"AM_DEVICE_LCDC",
    L"AM_DEVICE_MAILBOX0",             /* 40 */
    L"AM_DEVICE_MCASP0",
    L"AM_DEVICE_MCASP1",
    L"AM_DEVICE_MMCHS0",
    L"AM_DEVICE_MMCHS1",
    L"AM_DEVICE_MMCHS2",
    L"AM_DEVICE_MSTR_EXPS",
    L"AM_DEVICE_OCMCRAM",
    L"AM_DEVICE_OCPWP",
    L"AM_DEVICE_PCIE",
    L"AM_DEVICE_PKA",                  /* 50 */
    L"AM_DEVICE_RNG",
    L"AM_DEVICE_SHA0",
    L"AM_DEVICE_SLV_EXPS",
    L"AM_DEVICE_SPARE0",
    L"AM_DEVICE_SPARE1",
    L"AM_DEVICE_MCSPI0",
    L"AM_DEVICE_MCSPI1",
    L"AM_DEVICE_SPINLOCK",
    L"AM_DEVICE_TIMER2",
    L"AM_DEVICE_TIMER3",                  /* 60 */
    L"AM_DEVICE_TIMER4",
    L"AM_DEVICE_TIMER5",
    L"AM_DEVICE_TIMER6",
    L"AM_DEVICE_TIMER7",
    L"AM_DEVICE_TPCC",
    L"AM_DEVICE_TPTC0",
    L"AM_DEVICE_TPTC1",
    L"AM_DEVICE_TPTC2",
    L"AM_DEVICE_UART1",
    L"AM_DEVICE_UART2",               /* 70 */
    L"AM_DEVICE_UART3",
    L"AM_DEVICE_UART4",
    L"AM_DEVICE_UART5",
    L"AM_DEVICE_USB0",
    L"AM_DEVICE_BITBLT",           /* GFX */
    L"AM_DEVICE_GFX",
    L"AM_DEVICE_MMU",
    L"AM_DEVICE_MMUCFG",
    L"AM_DEVICE_MPU",              /* MPU */
    L"AM_DEVICE_RTC",                /* 80 */
    L"AM_DEVICE_CEFUSE",
    L"AM_DEVICE_EFUSE",   
};

PTCHAR idleStStr[] = {
    L"Functional",
    L"Idle Trans",
    L"Idle OCP  ",
    L"Disabled  ",
};
    
PTCHAR stdbyStStr[] = {
    L"Functional",
    L"Standby   ",
    L"N/A       ",
};

extern void DumpClockStatus(UINT clock_id);

void PrcmDebugChkAndDisableDevice(UINT devId, DWORD * preMMode, DWORD * postIdleSt, DWORD * postStdbySt, DWORD *refCount)
{
    UINT32 val = 0;

    if (devId >= AM_DEVICE_COUNT) return;
    
	val = INREG32((volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          s_rgDeviceLookupTable[devId].CLKCTRL_REG));
    *preMMode = (val & ClKCTRL_MODULEMODE_MASK)>>ClKCTRL_MODULEMODE_SHIFT;
    *refCount = s_rgDeviceLookupTable[devId].refCount;
    if (*preMMode == ClKCTRL_MODULEMODE_EN) {        
        while (s_rgDeviceLookupTable[devId].refCount>0)
        {
            PrcmDeviceEnableClocks(devId,FALSE);
        }
    }    
    *postIdleSt = (val & s_rgDeviceLookupTable[devId].idlestMask)>>ClKCTRL_IDLEST_SHIFT;
    if (s_rgDeviceLookupTable[devId].stbyMask)
        *postStdbySt = (val & s_rgDeviceLookupTable[devId].stbyMask)>>ClKCTRL_STBYST_SHIFT; 
    else
        *postStdbySt = 2;
}

void DumpDeviceStatus(UINT devId)
{ 
    DWORD mmode, idleSt, stdbySt;
    UINT i;
    UINT32 val = 0;

    if (devId >= AM_DEVICE_COUNT) return;
    
	val = INREG32((volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          s_rgDeviceLookupTable[devId].CLKCTRL_REG));
    mmode = (val & ClKCTRL_MODULEMODE_MASK)>>ClKCTRL_MODULEMODE_SHIFT;
    idleSt = (val & s_rgDeviceLookupTable[devId].idlestMask)>>ClKCTRL_IDLEST_SHIFT;
    if (s_rgDeviceLookupTable[devId].stbyMask)
        stdbySt = (val & s_rgDeviceLookupTable[devId].stbyMask)>>ClKCTRL_STBYST_SHIFT; 
    else
        stdbySt = 2;
        
    RETAILMSG(1,(L"DEVICE:\t%s(%d)\tState:%s \t%s \t%s \tRefcount:%d\r\n",
                    DeviceNames[devId],devId,(mmode==0x2)? L"ENABLED":L"DISABLED",
                    idleStStr[idleSt],
                    stdbyStStr[stdbySt],
                    s_rgDeviceLookupTable[devId].refCount));
    for (i = 0; i < s_rgDeviceLookupTable[devId].fclk.size; ++i)
    {
        DumpClockStatus(s_rgDeviceLookupTable[devId].fclk.rgSourceClocks[i]);
    }
    for (i = 0; i < s_rgDeviceLookupTable[devId].iclk.size; ++i)
    {
        DumpClockStatus(s_rgDeviceLookupTable[devId].iclk.rgSourceClocks[i]);
    }    
    
}

void DumpAllDeviceStatus()
{
    UINT i;
    for (i=0; i<AM_DEVICE_COUNT;i++) {
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));    
        DumpDeviceStatus(i);
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));
    }
}

