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
//  File:  am33x_oal_prcm.h
//
#ifndef _AM33X_OAL_PRCM_H
#define _AM33X_OAL_PRCM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "am33x_prcm.h"
#include <oalex.h>
#include "tps6591x.h"
#include "am3xx_gptimer.h"

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMStartOffset
//
//  offset to start of SRAM where SRAM routines will be copied to. 
//
extern DWORD                    dwOEMSRAMStartOffset;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMPUContextRestore
//
//  location to store context restore information from off mode
//
extern const volatile DWORD     dwOEMMPUContextRestore;

//------------------------------------------------------------------------------
//
//  Global: g_pTimerRegs
//
//  Reference to GPTIMER1 registers.  Initialized in OALTimerInit().
//
extern AM3XX_GPTIMER_REGS       *g_pTimerRegs;

//-----------------------------------------------------------------------------
typedef struct {
    UINT                    clockId;
    LONG                    refCount;
    UINT                    nLevel;
} SourceClockInfo_t;

//-----------------------------------------------------------------------------
typedef enum {
    kVoltageProcessor1 = 0,
    kVoltageProcessor2,
    kVoltageProcessorCount
}
VoltageProcessor_e;


//-----------------------------------------------------------------------------
typedef struct {
    VoltageRail_e           vp;
    UINT                    voltInMV;
    UINT                    voltSelBitsVal;                    
} VoltageProcessorSetting_t;

//-----------------------------------------------------------------------------
typedef struct {
    int                     dpllId;
    UINT                    frequency;
    UINT                    m;
    UINT                    n;    
} DpllFrequencySetting_t;


typedef struct {
    int                     dpllClkOutId;
    UINT                    divisor;    
} DpllClkOutFreqSetting_t;

typedef struct {
    int                     clockId;
    UINT                    divisor;    
} clkFreqSetting_t;


//-----------------------------------------------------------------------------
//  SRAM functions for power management
//
//-----------------------------------------------------------------------------
typedef struct {
    UINT                    MPU_CONTEXT_PA;
    UINT                    MPU_CONTEXT_VA;
    UINT                    TLB_INV_FUNC_ADDR;
    UINT                    EMIF_REGS;
    UINT                    CM_PER_REGS;
    UINT                    CM_WKUP_REGS;
    UINT                    DDR_PHY_REGS;
    UINT                    SYS_MISC2_REGS;    
    UINT                    suspendState;
} 
CPU_INFO;

//-----------------------------------------------------------------------------
//
//  Global:  g_pCPUInfo
//
//  contains references to relevant chip/power information used in SRAM
//  functions
//
extern CPU_INFO            *g_pCPUInfo;


//-----------------------------------------------------------------------------
//
//  Global:  fnOALCPUStart
//
//  OAL specific CPUStart routine
//
typedef 
int 
(*pCPUStart)(
    );

//-----------------------------------------------------------------------------
//
//  Global:  fnOALCPUIdle
//
//  OAL specific CPUIdle routine
//
typedef 
int 
(*pCPUIdle)(
    CPU_INFO               *pInfo
    );

extern pCPUIdle             fnOALCPUIdle;

//-----------------------------------------------------------------------------
//
//  Global:  fnRomRestoreLoc
//
//  OAL specific ROM restore routine
//
typedef 
int 
(*pRomRestore)(    
    );

extern pRomRestore             fnRomRestoreLoc;

//-----------------------------------------------------------------------------
//
//  Global:  fnCfgEMIFOPP100Loc
//
//  Reference to EMIF OPP100 config in SRAM
//
typedef 
int 
(*pCfgEMIFOPP100)(    
    CPU_INFO               *pInfo
    );

extern pCfgEMIFOPP100             fnCfgEMIFOPP100Loc;

//-----------------------------------------------------------------------------
//
//  Global:  fnCfgEMIFOPP50Loc
//
//  Reference to EMIF OPP50 config in SRAM
//
typedef 
int 
(*pCfgEMIFOPP50)(    
    CPU_INFO               *pInfo
    );

extern pCfgEMIFOPP50             fnCfgEMIFOPP50Loc;

//-----------------------------------------------------------------------------
//
//
//  OAL TLB InValidation function pointer typedef
//

typedef 
void 
(*pInvalidateTlb)( );

//-----------------------------------------------------------------------------
//
//  Function: PRCM power functions
//
void PrcmInit();
void PrcmPostInit();
void PrcmSuspend();
BOOL PrcmRestoreDomain( UINT powerDomain );
void PrcmContextRestore();
void PrcmContextRestoreInit();
void PrcmCapturePrevPowerState();
void PrcmProfilePrevPowerState(DWORD timer_val, DWORD wakeup_delay );    
void PrcmInitializePrevPowerState();
UINT PrcmInterruptClearStatus(UINT mask);
UINT PrcmInterruptProcess(UINT mask);
UINT PrcmInterruptEnable(UINT mask, BOOL bEnable);
BOOL PrcmClockSetParent(UINT clockId, UINT newParentClockId);
BOOL PrcmClockSetSystemClockSetupTime(USHORT setupTime);
BOOL PrcmClockSetDpllFrequency( UINT dpllId, UINT m, UINT n, UINT freqSel, UINT outputDivisor);
BOOL PrcmClockSetDpllClkOutDivisor(UINT dpllClkOutId, UINT div);
BOOL PrcmDeviceEnableClocks( UINT clkId, BOOL bEnable);
BOOL PrcmDeviceEnableClocksKernel(UINT clkId, BOOL bEnable);
BOOL PrcmDeviceEnableIClock(UINT clkId, BOOL bEnable);
BOOL PrcmDeviceEnableFClock(UINT clkId, BOOL bEnable);
BOOL PrcmDeviceGetEnabledState(UINT clkId, BOOL *pbEnable);
BOOL PrcmDeviceGetSourceClockInfo(UINT devId,
		IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pInfo);

BOOL PrcmDeviceSetSourceClocks(UINT devId, UINT count, UINT rgClocks[]);
BOOL PrcmDeviceGetContextState(UINT devId, BOOL bSet);
UINT PrcmClockGetDpllState(UINT dpllId);
BOOL PrcmClockGetParentClockInfo(UINT clockId, UINT nLevel, SourceClockInfo_t *pInfo);
BOOL PrcmClockGetParentClockRefcount(UINT clockId, UINT nLevel, LONG *pRefCount);
float PrcmClockGetSystemClockFrequency();
BOOL PrcmClockSetDpllState(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN *pInfo);

BOOL PrcmClockSetDpllAutoIdleState(UINT dpllId, UINT dpllAutoidleState);
VOID PrcmClockRestoreDpllState(UINT dpll);
UINT32 PrcmClockGetClockRate(OMAP_CLOCKID clock_id);
BOOL PrcmDomainSetPowerState(UINT powerDomain, UINT powerState, UINT logicState);
BOOL PrcmDomainSetMemoryState(UINT powerDomain, UINT memoryState, UINT memoryStateMask);
BOOL PrcmDomainResetStatus(UINT powerDomain, UINT *pResetStatus, BOOL bClear);
BOOL PrcmDomainReset(UINT powerDomain, UINT resetMask);
BOOL PrcmDomainResetRelease(UINT powerDomain, UINT resetMask);
void PrcmDomainUpdateRefCount(UINT powerDomain, UINT bEnable);
BOOL PrcmDomainSetPowerStateInternal(UINT powerDomain, UINT powerState, UINT logicState, BOOL bNotify);
#if 0
void PrcmVoltSetControlMode(UINT voltCtrlMode, UINT voltCtrlMask);
void PrcmVoltSetControlPolarity(UINT polMode, UINT polModeMask);
void PrcmVoltSetAutoControl(UINT autoCtrlMode, UINT autoCtrlMask);
void PrcmVoltI2cInitialize(VoltageProcessor_e vp, UINT8 slaveAddr,
                      UINT8 cmdAddr, UINT8 voltAddr, BOOL bUseCmdAddr);
void PrcmVoltI2cSetHighSpeedMode(BOOL bHSMode, BOOL bRepeatStartMode, UINT8 mcode);
void PrcmVoltInitializeVoltageLevels(VoltageProcessor_e vp, UINT vddOn, UINT vddOnLP,
                                     UINT vddRetention, UINT vddOff );
void PrcmVoltSetErrorConfiguration(VoltageProcessor_e vp, UINT errorOffset, UINT errorGain);
void PrcmVoltSetSlewRange(VoltageProcessor_e vp, UINT minVStep,
                          UINT minWaitTime, UINT maxVStep, UINT maxWaitTime);
void PrcmVoltSetLimits(VoltageProcessor_e vp, UINT minVolt, UINT maxVolt, UINT timeOut);
void PrcmVoltSetVoltageLevel(VoltageProcessor_e vp, UINT vdd, UINT mask );
void PrcmVoltSetInitVddLevel(VoltageProcessor_e vp, UINT initVolt);
void PrcmVoltEnableTimeout(VoltageProcessor_e vp, BOOL bEnable );
void PrcmVoltEnableVp(VoltageProcessor_e vp, BOOL bEnable );
void PrcmVoltFlushVoltageLevels(VoltageProcessor_e vp);
BOOL PrcmVoltIdleCheck(VoltageProcessor_e vp);
UINT PrcmVoltGetVoltageRampDelay(VoltageProcessor_e vp);
#endif
void PrcmProcessPostMpuWakeup();
void PrcmDomainClearReset();
void PrcmCM3ResetAndHandshake();


#ifdef __cplusplus
}
#endif

#endif
