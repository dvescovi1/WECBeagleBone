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
//  File:  oal_prcm.h
//
#ifndef _OAL_PRCM_H
#define _OAL_PRCM_H

#include "omap_gpmc_regs.h"
#include "omap_sdrc_regs.h"
#include "omap_gptimer_regs.h"
#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMStartOffset
//
//  offset to start of SRAM where SRAM routines will be copied to. 
//  Reinitialized in config.bib (FIXUPVAR)
//
extern DWORD                    dwOEMSRAMStartOffset;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMPUContextRestore
//
//  location to store context restore information from off mode (FIXUPVAR)
//
extern const volatile DWORD     dwOEMMPUContextRestore;
/*
//-----------------------------------------------------------------------------
//
//  Global:  g_pSysCtrlGenReg
//
//  reference to system control general register set
//  Initialized in OALPowerInit().
//
extern OMAP_SYSC_GENERAL_REGS  *g_pSysCtrlGenReg;
*/
//-----------------------------------------------------------------------------
//
//  Global:  g_pGPMCRegs
//
//  References the gpmc registers.  Initialized in OALPowerInit().
//
extern OMAP_GPMC_REGS          *g_pGPMCRegs;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSDRCRegs
//
//  References the SDRC registers.  Initialized in OALPowerInit().
//
extern OMAP_SDRC_REGS          *g_pSDRCRegs;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSMSRegs
//
//  References the SMS registers.  Initialized in OALPowerInit().
//
extern OMAP_SMS_REGS           *g_pSMSRegs;

//------------------------------------------------------------------------------
//
//  Global: g_pTimerRegs
//
//  Reference to GPTIMER1 registers.  Initialized in OALTimerInit().
//
extern OMAP_GPTIMER_REGS       *g_pTimerRegs;

//-----------------------------------------------------------------------------
//
//  Global:  g_pContextRestore
//
//  Reference to context restore registers. Initialized in OALPowerInit()
//
extern OMAP_CONTEXT_RESTORE_REGS  *g_pContextRestore;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSdrcRestore
//
//  Reference to Sdrc restore registers. Initialized in OALPowerInit()
//
extern OMAP_SDRC_RESTORE_REGS  *g_pSdrcRestore;

//-----------------------------------------------------------------------------
//
//  Global:  g_pPrcmRestore
//
//  Reference to Prcm restore registers. Initialized in OALPowerInit()
//
extern OMAP_PRCM_RESTORE_REGS  *g_pPrcmRestore;

//-----------------------------------------------------------------------------
typedef enum {
    kVoltageProcessor1 = 0,
    kVoltageProcessor2,
    kVoltageProcessorCount
}
VoltageProcessor_e;

//-----------------------------------------------------------------------------
typedef enum {
    kI2C1 = 0,
    kI2C2,
    kI2C3,
    kI2C_Count
} i2c_enum;

//-----------------------------------------------------------------------------
typedef struct {
    UINT                    SDRC_REGS;
    UINT                    MPU_CM_REGS;
    UINT                    CORE_CM_REGS;
    UINT                    CLOCK_CTRL_CM_REGS;
    UINT                    GPTIMER_REGS;
    UINT                    MPU_PRM_REGS;
    UINT                    CORE_PRM_REGS;
	UINT					GLOBAL_PRM_REGS;
    UINT                    MPU_CONTEXT_PA;
    UINT                    MPU_CONTEXT_VA;
    UINT                    SDRC_HIGH_RFR_FREQ;
    UINT                    SDRC_LOW_RFR_FREQ;
    UINT                    TLB_INV_FUNC_ADDR;
    UINT                    OMAP_DEVICE_TYPE;
    UINT                    DPLL_ARGS[5];
} 
CPU_INFO;

//-----------------------------------------------------------------------------
typedef struct {
    int                     dpllId;
    float                   frequency;
    UINT                    m;
    UINT                    n;
    UINT                    freqSel;
    UINT                    outputDivisor;
} DpllFrequencySetting_t;

//-----------------------------------------------------------------------------
typedef struct {
    VoltageProcessor_e      vp;
    UINT                    initVolt;
    UINT                    lpVolt;
} VoltageProcessorSetting_t;

//-----------------------------------------------------------------------------
typedef struct {
    OMAP_PRCM_PRM              *pPrcmPrm;
    OMAP_PRCM_CM               *pPrcmCm;
} PrcmInitInfo;

//-----------------------------------------------------------------------------
typedef struct {
    UINT                    clockId;
    LONG                    refCount;
    UINT                    nLevel;
} SourceClockInfo_t;

//-----------------------------------------------------------------------------
//
//  Global:  g_pCPUInfo
//
//  contains references to relevant chip/power information used in SRAM
//  functions
//
extern CPU_INFO            *g_pCPUInfo;

//-----------------------------------------------------------------------------
//  SRAM functions for power management
//

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
//  Global:  fnOALCPUWarmReset
//
//  OAL specific OALCPUWarmReset routine
//
typedef 
int 
(*pCPUWarmReset)(
    CPU_INFO               *pInfo
    );

extern pCPUWarmReset       fnOALCPUWarmReset;

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
//  Global:  fnOALUpdateCoreFreq
//
//  SRAM function to update core frequency as well as dividers
//
typedef 
int
(*pOALUpdateCoreFreq)(
    CPU_INFO               *pInfo,
    DWORD                   cm_clken_pll,
    DWORD                   cm_clksel1_pll
    );

extern pOALUpdateCoreFreq   fnOALUpdateCoreFreq;

//-----------------------------------------------------------------------------
//
//  Function: PRCM SRAM functions
//
int
OALUpdateCoreFreq(
    CPU_INFO               *pInfo,
    DWORD                   cm_clken_pll,
    DWORD                   cm_clksel1_pll
    );

int
OALCPUWarmReset(
    CPU_INFO               *pInfo
    );

//-----------------------------------------------------------------------------
//
//  Function: PRCM power functions
//
void
PrcmInit(
    PrcmInitInfo           *pInfo
    );

void
PrcmPostInit();

void
PrcmSuspend();

BOOL
PrcmRestoreDomain(
    UINT powerDomain
    );

void
PrcmContextRestore();

void
PrcmContextRestoreInit();

void
PrcmCapturePrevPowerState();

void 
PrcmProfilePrevPowerState(
    DWORD timer_val,
    DWORD wakeup_delay
    );    

void
PrcmInitializePrevPowerState();

UINT
PrcmInterruptClearStatus(
    UINT                    mask
    );

UINT
PrcmInterruptProcess(
    UINT                    mask
    );


UINT
PrcmInterruptEnable(
    UINT                    mask,
    BOOL                    bEnable
    );

BOOL
PrcmClockSetParent(
    UINT                    clockId,
    UINT                    newParentClockId
    );

BOOL
PrcmClockSetDivisor(
    UINT                    clockId,
    UINT                    parentClockId,
    UINT                    divisor
    );

BOOL
PrcmClockSetSystemClockSetupTime(
    USHORT                  setupTime
    );

BOOL
PrcmClockSetDpllFrequency(
    UINT                    dpllId,
    UINT                    m,
    UINT                    n,
    UINT                    freqSel,
    UINT                    outputDivisor
    );

BOOL
PrcmDeviceEnableClocks(
    UINT                    clkId, 
    BOOL                    bEnable
    );

BOOL
PrcmDeviceEnableClocksKernel(
    UINT                    clkId, 
    BOOL                    bEnable
    );

BOOL
PrcmDeviceEnableIClock(
    UINT                    clkId, 
    BOOL                    bEnable
    );

BOOL
PrcmDeviceEnableFClock(
    UINT                    clkId, 
    BOOL                    bEnable
    );

BOOL
PrcmDeviceGetEnabledState(
    UINT                    clkId, 
    BOOL                   *pbEnable
    );
   
BOOL
PrcmDeviceEnableAutoIdle(
    UINT                    clkId, 
    BOOL                    bEnable
    );

BOOL
PrcmDeviceGetAutoIdleState(
    UINT                    clkId, 
    BOOL                   *pbEnable
    );

BOOL
PrcmDeviceGetSourceClockInfo(
    UINT                    devId,
    IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pInfo
    );

BOOL
PrcmDeviceSetSourceClocks(
    UINT                    devId,
    UINT                    count,
    UINT                    rgClocks[]
    );

BOOL
PrcmDeviceGetContextState(
    UINT                    devId, 
    BOOL                    bSet
    );

UINT 
PrcmClockGetDpllState(
    UINT                    dpllId
    );

BOOL
PrcmClockGetParentClockInfo(
    UINT                    clockId,
    UINT                    nLevel,
    SourceClockInfo_t      *pInfo
    );

BOOL
PrcmClockGetParentClockRefcount(
    UINT                    clockId,
    UINT                    nLevel,
    LONG                   *pRefCount
    );

float 
PrcmClockGetSystemClockFrequency();

BOOL
PrcmClockSetExternalClockRequestMode(
    UINT                    extClkReqMode
    );

BOOL
PrcmClockSetDpllState(
    IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN *pInfo
    );

BOOL
PrcmClockSetDpllAutoIdleState(
    UINT                    dpllId,
    UINT                    dpllAutoidleState
    );

VOID
PrcmClockRestoreDpllState(
    UINT dpll
    );

UINT32 
PrcmClockGetClockRate(
    OMAP_CLOCKID clock_id
    );

BOOL
PrcmDomainSetClockState(
    UINT                    powerDomain,
    UINT                    clockDomain,
    UINT                    clockState
    );

BOOL
PrcmDomainSetClockStateKernel(
    UINT                    powerDomain,
    UINT                    clockDomain,
    UINT                    clockState
    );

BOOL
PrcmDomainSetPowerState(
    UINT                    powerDomain,
    UINT                    powerState,
    UINT                    logicState
    );

BOOL
PrcmDomainSetSleepDependency(
    UINT                    powerDomain,
    UINT                    ffDependency,
    BOOL                    bEnable
    );

BOOL
PrcmDomainSetWakeupDependency(
    UINT                    powerDomain,
    UINT                    ffDependency,
    BOOL                    bEnable
    );

BOOL
PrcmDomainSetMemoryState(
    UINT                    powerDomain,
    UINT                    memoryState,
    UINT                    memoryStateMask
    );

BOOL
PrcmDomainResetStatus(
    UINT                    powerDomain,
    UINT                   *pResetStatus,
    BOOL                    bClear
    );

BOOL
PrcmDomainClearWakeupStatus(
    UINT                    powerDomain
    );

BOOL
PrcmDomainReset(
    UINT                    powerDomain,
    UINT                    resetMask
    );

BOOL
PrcmDomainResetRelease(
    UINT                    powerDomain,
    UINT                    resetMask
    );

void
PrcmGlobalReset(
    );

void
PrcmDomainUpdateRefCount(
    UINT                    powerDomain,
    UINT                    bEnable
    );

BOOL
PrcmDomainSetPowerStateInternal(
    UINT        powerDomain,
    UINT        powerState,
    UINT        logicState,
    BOOL        bNotify
    );

void
PrcmVoltSetControlMode(
    UINT                    voltCtrlMode,
    UINT                    voltCtrlMask
    );

void
PrcmVoltSetControlPolarity(
    UINT                    polMode,
    UINT                    polModeMask
    );

void
PrcmVoltSetAutoControl(
    UINT                    autoCtrlMode,
    UINT                    autoCtrlMask
    );

void
PrcmVoltI2cInitialize(
    VoltageProcessor_e      vp,
    UINT8                   slaveAddr,
    UINT8                   cmdAddr,
    UINT8                   voltAddr,
    BOOL                    bUseCmdAddr
    );

void
PrcmVoltI2cSetHighSpeedMode(
    BOOL                    bHSMode,
    BOOL                    bRepeatStartMode,
    UINT8                   mcode
    );

void
PrcmVoltInitializeVoltageLevels(
    VoltageProcessor_e      vp,
    UINT                    vddOn,
    UINT                    vddOnLP,
    UINT                    vddRetention,
    UINT                    vddOff
    );

void
PrcmVoltSetErrorConfiguration(
    VoltageProcessor_e      vp,
    UINT                    errorOffset,
    UINT                    errorGain
    );

void
PrcmVoltSetSlewRange(
    VoltageProcessor_e      vp,
    UINT                    minVStep,
    UINT                    minWaitTime,
    UINT                    maxVStep,
    UINT                    maxWaitTime
    );

void
PrcmVoltSetLimits(
    VoltageProcessor_e      vp,
    UINT                    minVolt,
    UINT                    maxVolt,
    UINT                    timeOut
    );

void
PrcmVoltSetVoltageLevel(
    VoltageProcessor_e      vp,
    UINT                    vdd,
    UINT                    mask
    );

void
PrcmVoltSetInitVddLevel(
    VoltageProcessor_e      vp,
    UINT                    initVolt
    );

void
PrcmVoltEnableTimeout(
    VoltageProcessor_e      vp,
    BOOL                    bEnable
    );

void
PrcmVoltEnableVp(
    VoltageProcessor_e      vp,
    BOOL                    bEnable
    );

void
PrcmVoltFlushVoltageLevels(
    VoltageProcessor_e      vp
    );

BOOL
PrcmVoltIdleCheck(
    VoltageProcessor_e      vp
    );

UINT
PrcmVoltGetVoltageRampDelay(
    VoltageProcessor_e      vp
    );

void
PrcmProcessPostMpuWakeup();

void
PrcmDomainClearReset();

int 
PrcmVoltScaleVoltageABB(UINT32 target_opp_no);

    
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
