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
//  File: prcm.c
//
#include "windows.h"
#include "omap.h"
#include "am33x.h"
#include "am33x_oal_prcm.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"
#include "am33x_config.h"
#include "oal_clock.h"

#include "cm3_ipc.h"
#include "mailbox.h"
#include "am33x_interrupt_struct.h"

//-----------------------------------------------------------------------------
//
//  Used for debugging suspend/resume
//
#if 1

extern BOOL g_PrcmDebugSuspendResume;

//-----------------------------------------------------------------------------
//  Global:  g_pIntr
//  Reference to all interrupt related registers. Initialized in OALIntrInit
extern AM33X_INTR_CONTEXT const *g_pIntr;

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish system timer interrupt.
extern UINT32           g_oalTimerIrq;

//  Used for save/restore of GPIO clock state
//DWORD prevGpioIClkState;
//DWORD prevGpioFClkState;

//-----------------------------------------------------------------------------
//  function which returns the start of the MMU L1 table
extern DWORD OALGetTTBR();

//-----------------------------------------------------------------------------
//  function which Invalidates the TLB
extern VOID OALInvalidateTlb();

//-----------------------------------------------------------------------------
//  Global:  dwOEMSRAMSaveAddr
//  Secure SRAM Save location in SDRAM
extern DWORD dwOEMSRAMSaveAddr;

//-----------------------------------------------------------------------------
//
//  Global:  g_pCPUInfo
//
//  contains references to relevant chip/power information used in SRAM
//  functions
//
CPU_INFO                   *g_pCPUInfo = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  fnCpuStart
//
//  Beginning of all SRAM routines
//
pCPUStart                   fnCpuStart = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  fnOALCPUIdle
//
//  Reference to cpu idle code in SRAM
//
pCPUIdle                    fnOALCPUIdle = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  fnRomRestoreLoc
//
//  Reference to cpu restore code in SRAM
//
pRomRestore                    fnRomRestoreLoc = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  fnCfgEMIFOPP50Loc
//
//  Reference to EMIF OPP50 config in SRAM
//
pCfgEMIFOPP50                    fnCfgEMIFOPP50Loc = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  fnCfgEMIFOPP50Loc
//
//  Reference to EMIF OPP50 config in SRAM
//
pCfgEMIFOPP100                    fnCfgEMIFOPP100Loc = NULL;


//-----------------------------------------------------------------------------
//  Reference to all PRCM-registers. Initialized in PrcmInit
AM33X_PRCM_REGS        *g_pPrcmRegs;
AM33X_SECnFUSE_KEY_REGS *g_pSecnFuseRegs;
AM33X_SYSC_MISC2_REGS   *g_pSyscMisc2Regs;
AM33X_SUPPL_DEVICE_CTRL_REGS *g_pSupplDeviceCtrlRegs;


//------------------------------------------------------------------------------
//
//  Global:  g_pSysCtrlGenReg
//
//  reference to system control general register set
//
//OMAP_SYSC_GENERAL_REGS     *g_pSysCtrlGenReg = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPostInit
//
//  Indicates if prcm library has been fully initialized.
//  Initialized in PrcmPostInit
//
BOOL                        g_PrcmPostInit = FALSE;

//------------------------------------------------------------------------------
//
//  Global:  g_bSingleThreaded
//
//  Indicated that the OAL PRCM functions are being called while the kernel is 
//  single threaded (OEMIdle, OEMPoweroff)
//
BOOL                        g_bSingleThreaded = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_rgPrcmMutex
//
//  Contains a list of CRITICAL_SECTIONS used for synchronized access to
//  PRCM registers
//
CRITICAL_SECTION            g_rgPrcmMutex[Mutex_Count];

//-----------------------------------------------------------------------------
void PrcmCapturePrevPowerState()
{
    // Stub, nothing to do on AM3517
}

//-----------------------------------------------------------------------------
void PrcmProfilePrevPowerState(DWORD timer_val, DWORD wakeup_delay )
{
    UNREFERENCED_PARAMETER(timer_val);
    UNREFERENCED_PARAMETER(wakeup_delay);
    // Stub, nothing to do on AM3517
}

//-----------------------------------------------------------------------------
void
PrcmInitializePrevPowerState()
{
    // Stub, nothing to do on AM3517
}

//-----------------------------------------------------------------------------
UINT PrcmInterruptEnable(UINT mask, BOOL bEnable )
{
    UINT val = 0;
    Lock(Mutex_Intr);

OALMSG(1, (L"+PrcmInterruptEnable() mask %08x, bEnable %d\r\n", mask, bEnable));
/*
    // enable/disable prcm interrupts
    val = INREG32(&g_pPrcmPrm->pOMAP_OCP_SYSTEM_PRM->PRM_IRQENABLE_MPU);
    val = (bEnable != FALSE) ? (val | mask) : (val & ~mask);
    OUTREG32(&g_pPrcmPrm->pOMAP_OCP_SYSTEM_PRM->PRM_IRQENABLE_MPU, val);
*/
    Unlock(Mutex_Intr);
    return val;
}

//-----------------------------------------------------------------------------
UINT PrcmInterruptClearStatus(UINT mask)
{
    UINT val = 0;

    // This routine should only be called during system boot-up or
    // from OEMIdle.  Hence, serialization within this routine
    // should not be performed.

    // clear prcm interrupt status
OALMSG(1, (L"+PrcmInterruptClearStatus() mask %08x\r\n", mask));
    // return the status prior to clearing the status
    return val;
}

static void ClearXNBit( void *pvAddr )
{
    const UINT ARM_L1_NO_EXECUTE = 0x00000010;

    DWORD   idxL1MMU = ((DWORD)pvAddr) >> 20;
    DWORD  *pL1MMUTbl = (DWORD*)OALPAtoVA(OALGetTTBR(), TRUE); // TODO: get UNCACHED ADDR
    
    pL1MMUTbl[idxL1MMU] &= ~ARM_L1_NO_EXECUTE;
}


//-----------------------------------------------------------------------------
//
//  Function:  OALSRAMFnInit
//
//  copies power critical code into SRAM to be executed in the sanctuary of
//  SRAM
//
BOOL OALSRAMFnInit()
{
    pInvalidateTlb  fnInvalidateTlb;
    UINT32          fnCPUStartPA = AM33X_OCMC0_PA + dwOEMSRAMStartOffset;
    UINT32          * romRestoreStorePtr = (UINT32 *) OALPAtoVA(0x4030FFFC, FALSE);  

    OALMSG(OAL_FUNC, (L"+OALSRAMFnInit()\r\n"));

#pragma warning (push)
#pragma warning (disable:4152) //disable warning that prevents using function pointers as data pointers
    // get reference to SRAM
    fnCpuStart = OALPAtoVA(fnCPUStartPA, FALSE);

    // initialize cpu idle data structure
    g_pCPUInfo = (CPU_INFO*)OALPAtoVA(dwOEMMPUContextRestore, FALSE);    
    g_pCPUInfo->MPU_CONTEXT_VA = (UINT)g_pCPUInfo + sizeof(CPU_INFO);
    g_pCPUInfo->MPU_CONTEXT_PA = (UINT)dwOEMMPUContextRestore + sizeof(CPU_INFO);
    g_pCPUInfo->EMIF_REGS = (UINT)OALPAtoVA(AM33X_EMIF4_0_CFG_REGS_PA, FALSE);
    g_pCPUInfo->CM_PER_REGS = (UINT)OALPAtoVA(AM33X_PRCM_REGS_PA, FALSE);
    g_pCPUInfo->CM_WKUP_REGS = (UINT)OALPAtoVA(AM33X_PRCM_REGS_PA+0x400, FALSE);
    g_pCPUInfo->DDR_PHY_REGS = (UINT)OALPAtoVA(AM33X_DDR_REGS_PA, FALSE);
    g_pCPUInfo->SYS_MISC2_REGS = (UINT)OALPAtoVA(AM33X_SYSC_MISC2_REGS_PA, FALSE);
    g_pCPUInfo->suspendState = PM_CMD_NO_PM;  


    OALMSG(OAL_INFO,(L"g_pCPUInfo=0x%x, MPU_CONTEXT_PA=0x%x, MPU_CONTEXT_VA=0x%x, TLB_INV_FUNC_ADDR=0x%x, "
              L"EMIF_REGS=0x%x, CM_PER_REGS=0x%x, CM_WKUP_REGS=0x%x, DDR_PHY_REGS=0x%x, SYS_MISC2_REGS=0x%x, suspendState=%d\r\n",
        g_pCPUInfo,
        g_pCPUInfo->MPU_CONTEXT_PA,
        g_pCPUInfo->MPU_CONTEXT_VA,
        g_pCPUInfo->TLB_INV_FUNC_ADDR,
        g_pCPUInfo->EMIF_REGS,
        g_pCPUInfo->CM_PER_REGS,
        g_pCPUInfo->CM_WKUP_REGS,
        g_pCPUInfo->DDR_PHY_REGS,
        g_pCPUInfo->SYS_MISC2_REGS,
        g_pCPUInfo->suspendState));
    
    // Populate fnOALCPUIdle function pointer with SRAM address of
    // OALCPUIdle function.
#if (_WINCEOSVER <= 700)
    fnOALCPUIdle = (pCPUIdle)((UINT)fnCpuStart + 
                                ((UINT)OALCPUIdle - (UINT)OALCPUStart)); 

    // Populate fnTlbValidate function pointer with SRAM address of
    // OALInvalidateTlb function which will be called by cpu.s 
    // after mpu restore.
    fnInvalidateTlb = (pInvalidateTlb)((UINT)fnCpuStart + 
                                ((UINT)OALInvalidateTlb - (UINT)OALCPUStart)); 

    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnRomRestoreLoc = (pRomRestore)((UINT)fnCPUStartPA + 
                                ((UINT)OALCPURestoreContext - (UINT)OALCPUStart)); 

    if (romRestoreStorePtr)
        OUTREG32(romRestoreStorePtr,fnRomRestoreLoc);
    else
        OALMSG(1,(L"*****romRestoreStorePtr is NULL\r\n"));
    

    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnCfgEMIFOPP100Loc = (pCfgEMIFOPP100)((UINT)fnCpuStart + 
                                ((UINT)OALConfigEMIFOPP100 - (UINT)OALCPUStart)); 
    
    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnCfgEMIFOPP50Loc = (pCfgEMIFOPP50)((UINT)fnCpuStart + 
                                ((UINT)OALConfigEMIFOPP50 - (UINT)OALCPUStart)); 
#else
	    fnOALCPUIdle = (pCPUIdle)((UINT)fnCpuStart + 
                                ((UINT)OALCPUIdle - (UINT)OALCPUStart) + 1); 
    
    // Populate fnTlbValidate function pointer with SRAM address of
    // OALInvalidateTlb function which will be called by cpu.s 
    // after mpu restore.
    fnInvalidateTlb = (pInvalidateTlb)((UINT)fnCpuStart + 
                                ((UINT)OALInvalidateTlb - (UINT)OALCPUStart) + 1); 

    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnRomRestoreLoc = (pRomRestore)((UINT)fnCPUStartPA + 
                                ((UINT)OALCPURestoreContext - (UINT)OALCPUStart) + 1); 

    if (romRestoreStorePtr)
        OUTREG32(romRestoreStorePtr,fnRomRestoreLoc);
    else
        OALMSG(1,(L"*****romRestoreStorePtr is NULL\r\n"));
    

    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnCfgEMIFOPP100Loc = (pCfgEMIFOPP100)((UINT)fnCpuStart + 
                                ((UINT)OALConfigEMIFOPP100 - (UINT)OALCPUStart) + 1); 
    
    // Populate fnRomRestoreLoc function pointer with SRAM address of
    // OALCPURestoreContext function.
    fnCfgEMIFOPP50Loc = (pCfgEMIFOPP50)((UINT)fnCpuStart + 
                                ((UINT)OALConfigEMIFOPP50 - (UINT)OALCPUStart) + 1);
#endif

    // update the TLB Inv addr in cpuInfo
    g_pCPUInfo->TLB_INV_FUNC_ADDR = (UINT)fnInvalidateTlb;

    // Kernel marks all uncached adress as "no execute", we need to clear
    // it here since we're going to run some routines from non-cached memory
    ClearXNBit(fnCpuStart);
    ClearXNBit(fnOALCPUIdle);
    ClearXNBit(fnInvalidateTlb);
	ClearXNBit(fnCfgEMIFOPP50Loc);
    ClearXNBit(fnCfgEMIFOPP100Loc);

    // We assume OALCPUStart code is *always* the first function in the
    // group of routines which get copied to SRAM and OALCPUEnd is last
#if (_WINCEOSVER <= 700)
	memcpy(fnCpuStart, (LPVOID)(((UINT)OALCPUStart)) , (UINT)OALCPUEnd - (UINT)OALCPUStart);
#else
    memcpy(fnCpuStart, (LPVOID)(((UINT)OALCPUStart)- 1) , (UINT)OALCPUEnd - (UINT)OALCPUStart);
#endif

	//bmartin : OALCPUStart was already incremented by the linker to be executed in Thumb mode,
	//so we decrement it to make the copy correctly

    //  Flush the cache to ensure idle code is in SRAM
    OEMCacheRangeFlush( fnCpuStart, (UINT)OALCPUEnd - (UINT)OALCPUStart, CACHE_SYNC_ALL );

    OALMSG(OAL_INFO,(L"fnCpuStart=0x%x fnOALCPUIdle=0x%x fnInvalidateTlb=0x%x fnRomRestoreLoc=0x%x fnCfgEMIFOPP100Loc=0x%x, fnCfgEMIFOPP50Loc=0x%x\r\n",
                fnCpuStart,fnOALCPUIdle,fnInvalidateTlb, fnRomRestoreLoc,fnCfgEMIFOPP100Loc,fnCfgEMIFOPP50Loc ));

#pragma warning (pop)

    OALMSG(OAL_FUNC, (L"-OALSRAMFnInit()\r\n"));

    return TRUE;
}

//-----------------------------------------------------------------------------
void OALSDRCRefreshCounter(UINT highFreq, UINT lowFreq  )
//	Sets the SDRC Refresh counter
{
OALMSG(1, (L"OALSDRCRefreshCounter(): %u %u\r\n", highFreq, lowFreq));

//    g_pCPUInfo->SDRC_HIGH_RFR_FREQ = highFreq;
//    g_pCPUInfo->SDRC_LOW_RFR_FREQ = lowFreq;
}

void OALSavePrcmContext(){}
void OALSaveSdrcContext(){}

//-----------------------------------------------------------------------------
void PrcmInit()
//	Initializes the prcm module out of reset
{
    OALMSG(OAL_FUNC, (L"+PrcmInit()\r\n"));

	g_pPrcmRegs  = OALPAtoUA(AM33X_PRCM_REGS_PA);
    g_pSecnFuseRegs = OALPAtoUA(AM33X_SECnFUSE_KEY_REGS_PA);
    g_pSyscMisc2Regs = OALPAtoUA(AM33X_SYSC_MISC2_REGS_PA);
    g_pSupplDeviceCtrlRegs = OALPAtoUA(AM33X_SUPPL_DEVICE_CTRL_REGS_PA);

    // initialize all internal data structures
    ResetInitialize();
    DomainInitialize();
    ClockInitialize();
    DeviceInitialize();

    OALMSG(OAL_FUNC, (L"-PrcmInit()\r\n"));
}

void initializeTimer1(void)
{
    OALMSG(1,(L"initializeTimer1: Enter\r\n"));    
    
    OUTREG32(OALPAtoVA(0x44e3e06c, FALSE),0x83e70b13);
    OUTREG32(OALPAtoVA(0x44e3e070, FALSE),0x95a4f1e0);
    OUTREG32(OALPAtoVA(0x44e3e054, FALSE),0x48);
    
	//enableModuleClock(CLK_TIMER1);
	PrcmClockSetParent(kTIMER1_GCLK,kCLK_32768_CK);
    EnableDeviceClocks(AM_DEVICE_TIMER1,TRUE);    
	
	/*	wake up configs	*/
	//HWREG(0x44e31010) = 0x214;
	OUTREG32(OALPAtoVA(0x44e31010, FALSE),0x214);
	
	/*	enable overflow int	*/
	//HWREG(0x44e3101c) = 0x2;
	OUTREG32(OALPAtoVA(0x44e3101c, FALSE),0x2);
	
	/*	enable overflow wakeup	*/
	//HWREG(0x44e31020) = 0x2;
	OUTREG32(OALPAtoVA(0x44e31020, FALSE),0x2);
    
    OALMSG(1,(L"initializeTimer1: Exit\r\n"));    
	
}

void setTimerCount(unsigned int count)
{   
    
	/*	Set timer counter	*/
	//HWREG(0x44e31028) = count;
    OUTREG32(OALPAtoVA(0x44e31028, FALSE),count);

	/*	Start timer	*/
	//HWREG(0x44e31024) =  0x23;	
	OUTREG32(OALPAtoVA(0x44e31024, FALSE),0x23);


}


void clearTimerInt(void)
{
	//HWREG(0x44e31018) = 0x2;
	OUTREG32(OALPAtoVA(0x44e31018, FALSE),0x2);


}

void disableTimer1()
{
    EnableDeviceClocks(AM_DEVICE_TIMER1,FALSE);    
}

//-----------------------------------------------------------------------------
void PrcmPostInit()
//	Initializes the prcm module out of reset
{
    int i;
    OALMSG(OAL_FUNC, (L"+PrcmPostInit()\r\n"));

    for (i = 0; i < Mutex_Count; ++i) // initialize synchronization objects
        InitializeCriticalSection(&g_rgPrcmMutex[i]);

    // Take the graphics SGX core out of reset. 
    // ToDo: Do it based on the appropriate chip variant.
    PrcmDomainResetRelease(POWERDOMAIN_GFX, GFX_RST);

    g_PrcmPostInit = TRUE; // update flag indicating PRCM library is fully initialized
    
    OALMSG(OAL_FUNC, (L"-PrcmPostInit()\r\n"));
};

//-----------------------------------------------------------------------------
//	Initializes the context restore registers
void PrcmContextRestoreInit(){}
//-----------------------------------------------------------------------------
//	Performs the necessary steps to restore from CORE OFF
void PrcmContextRestore(){}

void PrcmClearContextRegisters() {}

DWORD _suspendState = PM_CMD_DS1_MODE;
deepSleepData _suspend_DSData;


//enable any one of these
#define USE_TIMER1_AS_WAKEUP_SOURCE
//#define USE_RTC_AS_WAKEUP_SOURCE

// Modules that dont have a driver should be disabled in this function
void PrcmPreSuspendDisableClock(DWORD suspendState)
{
//    EnableDeviceClocks(AM_DEVICE_CLKDIV32K,FALSE);
    EnableDeviceClocks(AM_DEVICE_MAILBOX0,FALSE);
    EnableDeviceClocks(AM_DEVICE_MSTR_EXPS,FALSE);
    EnableDeviceClocks(AM_DEVICE_OCPWP,FALSE);
    EnableDeviceClocks(AM_DEVICE_SLV_EXPS,FALSE);
    EnableDeviceClocks(AM_DEVICE_CEFUSE,FALSE);
    
}

void PrcmPostResumeEnableClock(DWORD suspendState)
{
//    EnableDeviceClocks(AM_DEVICE_CLKDIV32K,TRUE);
    EnableDeviceClocks(AM_DEVICE_MAILBOX0,TRUE);
//    EnableDeviceClocks(AM_DEVICE_MSTR_EXPS,TRUE);
//    EnableDeviceClocks(AM_DEVICE_OCPWP,TRUE);
//    EnableDeviceClocks(AM_DEVICE_SLV_EXPS,TRUE);
//    EnableDeviceClocks(AM_DEVICE_CEFUSE,TRUE);
}

extern void DumpAllDeviceStatus();

typedef VOID (*PFN_FmtPuts)(WCHAR *s, ...);


#define DISPLAY_REGISTER_VALUE_USING_STRUCT(x, y)  \
                                      OALMSG(1,(L"Reg 0X%X=0X%08X\r\n",&x->y,INREG32(&x->y))); 

#define MESSAGE_BUFFER_SIZE     280
 

void
Dump_PRCM()
{
    AM33X_PRCM_REGS *pPrcmGlobalPrm;
//    WCHAR szBuffer[MESSAGE_BUFFER_SIZE];

    pPrcmGlobalPrm = (AM33X_PRCM_REGS*)OALPAtoVA(AM33X_PRCM_REGS_PA, FALSE);

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nPER CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nPER Clock Domains:\r\n")));    
    DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4LS_CLKSTCTRL);		// 0x0000
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3S_CLKSTCTRL);		// 0x0004
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4FW_CLKSTCTRL);		// 0x0008
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_CLKSTCTRL);		// 0x000C	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4HS_CLKSTCTRL);		// 0x011C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCPWP_L3_CLKSTCTRL);	// 0x012C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ICSS_CLKSTCTRL);		// 0x0140
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CPSW_CLKSTCTRL);		// 0x0144
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_LCDC_CLKSTCTRL);		// 0x0148
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CLK_24MHZ_CLKSTCTRL);	// 0x0150
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nPER Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_PCIE_CLKCTRL);		// 0x0010
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CPGMAC0_CLKCTRL);		// 0x0014
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_LCDC_CLKCTRL);		// 0x0018
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_USB0_CLKCTRL);		// 0x001C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MLB_CLKCTRL);			// 0x0020
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC0_CLKCTRL);		// 0x0024
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EMIF_CLKCTRL);		// 0x0028
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCMCRAM_CLKCTRL);		// 0x002C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPMC_CLKCTRL);		// 0x0030
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP0_CLKCTRL);		// 0x0034
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART5_CLKCTRL);		// 0x0038
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC0_CLKCTRL);		// 0x003C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ELM_CLKCTRL);			// 0x0040
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_I2C2_CLKCTRL);		// 0x0044
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_I2C1_CLKCTRL);		// 0x0048
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI0_CLKCTRL);		// 0x004C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI1_CLKCTRL);		// 0x0050
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI2_CLKCTRL);		// 0x0054
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPI3_CLKCTRL);		// 0x0058
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4LS_CLKCTRL);		// 0x0060
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4FW_CLKCTRL);		// 0x0064
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP1_CLKCTRL);		// 0x0068
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART1_CLKCTRL);		// 0x006C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART2_CLKCTRL);		// 0x0070
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART3_CLKCTRL);		// 0x0074
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_UART4_CLKCTRL);		// 0x0078
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER7_CLKCTRL);		// 0x007C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER2_CLKCTRL);		// 0x0080
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER3_CLKCTRL);		// 0x0084
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER4_CLKCTRL);		// 0x0088
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MCASP2_CLKCTRL);		// 0x008C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_RNG_CLKCTRL);			// 0x0090
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_AES0_CLKCTRL);		// 0x0094
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_AES1_CLKCTRL);		// 0x0098
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DES_CLKCTRL);			// 0x009C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SHA0_CLKCTRL);		// 0x00A0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_PKA_CLKCTRL);			// 0x00A4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO6_CLKCTRL);		// 0x00A8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO1_CLKCTRL);		// 0x00AC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO2_CLKCTRL);		// 0x00B0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO3_CLKCTRL);		// 0x00B4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO4_CLKCTRL);		// 0x00B8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPCC_CLKCTRL);		// 0x00BC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DCAN0_CLKCTRL);		// 0x00C0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_DCAN1_CLKCTRL);		// 0x00C4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE_CLKCTRL);		// 0x00C8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS1_CLKCTRL);		// 0x00CC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EMIF_FW_CLKCTRL);		// 0x00D0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS0_CLKCTRL);		// 0x00D4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_EPWMSS2_CLKCTRL);		// 0x00D8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_INSTR_CLKCTRL);	// 0x00DC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L3_CLKCTRL);			// 0x00E0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_IEEE5000_CLKCTRL);	// 0x00E4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_ICSS_CLKCTRL);		// 0x00E8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER5_CLKCTRL);		// 0x00EC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TIMER6_CLKCTRL);		// 0x00F0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC1_CLKCTRL);		// 0x00F4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MMC2_CLKCTRL);		// 0x00F8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC1_CLKCTRL);		// 0x00FC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_TPTC2_CLKCTRL);		// 0x0100
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_GPIO5_CLKCTRL);		// 0x0104
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPINLOCK_CLKCTRL);	// 0x010C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MAILBOX0_CLKCTRL);	// 0x0110
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_L4HS_CLKCTRL);		// 0x0120
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MSTR_EXPS_CLKCTRL);	// 0x0124
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SLV_EXPS_CLKCTRL);	// 0x0128
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_OCPWP_CLKCTRL);		// 0x0130
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_MAILBOX1_CLKCTRL);	// 0x0134
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE0_CLKCTRL);		// 0x0138
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_SPARE1_CLKCTRL);		// 0x013C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_PER_CLKDIV32K_CLKCTRL);	// 0x014C
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nWKUP CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nWKUP Clock Domains:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_CLKSTCTRL);			// 0x0400
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_L3_AON_CLKSTCTRL);		// 0x0418
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_L4_WKUP_AON_CLKSTCTRL);	// 0x04CC	
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nWKUP Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_CONTROL_CLKCTRL);	// 0x0404
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_GPIO0_CLKCTRL);		// 0x0408
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_L4WKUP_CLKCTRL);		// 0x040C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_TIMER0_CLKCTRL);		// 0x0410
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_DEBUGSS_CLKCTRL);	// 0x0414
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WKUP_M3_CLKCTRL);	// 0x04B0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_UART0_CLKCTRL);		// 0x04B4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_I2C0_CLKCTRL);		// 0x04B8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_ADC_TSC_CLKCTRL);	// 0x04BC
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_SMARTREFLEX0_CLKCTRL);// 0x04C0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_TIMER1_CLKCTRL);		// 0x04C4
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_SMARTREFLEX1_CLKCTRL);// 0x04C8
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WDT0_CLKCTRL);		// 0x04D0
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_WKUP_WDT1_CLKCTRL);		// 0x04D4
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nMPU CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nMPU Clock Domains:\r\n")));   
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MPU_CLKSTCTRL);			// 0x0600
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nMPU Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MPU_MPU_CLKCTRL);			// 0x0604
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nRTC CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nRTC Clock Domains:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_RTC_CLKSTCTRL);			// 0x0804
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nRTC Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_RTC_RTC_CLKCTRL);			// 0x0800
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nGFX CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nGFX Clock Domains:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_L3_CLKSTCTRL);		// 0x0900
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_L4LS_GFX_CLKSTCTRL);// 0x090C
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nGFX Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_GFX_CLKCTRL);			// 0x0904
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_BITBLT_CLKCTRL);		// 0x0908
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_MMUCFG_CLKCTRL);		// 0x0910
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_GFX_MMUDATA_CLKCTRL);		// 0x0914
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nCEFUSE CM:\r\n")));
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nCEFUSE Clock Domains:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CEFUSE_CLKSTCTRL);			// 0x0A00
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nCEFUSE Modules:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CEFUSE_CEFUSE_CLKCTRL);	// 0x0A20
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));

    
    
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nPRM:\r\n")));    
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nOCP PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, REVISION_PRM);				// 0x0B00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQSTATUS_MPU);			// 0x0B04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQENABLE_MPU);			// 0x0B08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQSTATUS_M3);			// 0x0B0C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_IRQENABLE_M3);			// 0x0B10
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nPER PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_PER_RSTCTRL);				// 0x0C00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_PER_RSTST);				// 0x0C04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_PER_PWRSTST);				// 0x0C08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_PER_PWRSTCTRL);			// 0x0C0C
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nWKUP PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_WKUP_RSTCTRL);			// 0x0D00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_WKUP_PWRSTCTRL);			// 0x0D04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_WKUP_PWRSTST);			// 0x0D08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_WKUP_RSTST);				// 0x0D0C
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nMPU PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_MPU_PWRSTCTRL);			// 0x0E00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_MPU_PWRSTST);				// 0x0E04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_MPU_RSTST);				// 0x0E08
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDEVICE PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTCTRL);				// 0x0F00
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTTIME);				// 0x0F04
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_RSTST);					// 0x0F08
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_SRAM_COUNT);				// 0x0F0C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_CORE_SETUP);	// 0x0F10
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_CORE_CTRL);		// 0x0F14
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_MPU_SETUP);		// 0x0F18
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PRM_LDO_SRAM_MPU_CTRL);		// 0x0F1C
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nRTC PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_RTC_PWRSTCTRL);			// 0x1000
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_RTC_PWRSTST);				// 0x1004
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nGFX PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_GFX_PWRSTCTRL);			// 0x1100
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_GFX_RSTCTRL);				// 0x1104
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_GFX_PWRSTST);				// 0x1110
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, RM_GFX_RSTST);				// 0x1114
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nCEFUSE PRM:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_CEFUSE_PWRSTCTRL);		// 0x1200
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, PM_CEFUSE_PWRSTST);			// 0x1204	
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nCLKOUT:\r\n")));
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKOUT_CTRL);				// 0x0700
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    

    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nDPLLs:\r\n")));        
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDPLL MPU:\r\n")));    
    DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_MPU);		// 0x041C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_MPU);			// 0x0420
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_MPU);	// 0x0424
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_MPU);	// 0x0428
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_MPU);			// 0x042C	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_MPU);		// 0x0488
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_MPU);			// 0x04A8	
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDPLL DDR:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_DDR);		// 0x0430
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_DDR);			// 0x0434
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_DDR);	// 0x0438
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_DDR);	// 0x043C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_DDR);			// 0x0440
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_DDR);		// 0x0494
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_DDR);			// 0x04A0	
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDPLL DISP:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_DISP);		// 0x0444
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_DISP);		// 0x0448
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_DISP);// 0x044C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_DISP);// 0x0450
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_DISP);		// 0x0454
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_DISP);		// 0x0498
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_DISP);		// 0x04A4	
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDPLL CORE:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_CORE);		// 0x0458
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_CORE);		// 0x045C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_CORE);// 0x0460
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_CORE);// 0x0464
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_CORE);		// 0x0468	
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_CORE);		// 0x0490
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M4_DPLL_CORE);		// 0x0480
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M5_DPLL_CORE);		// 0x0484
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M6_DPLL_CORE);		// 0x04D8	
    OALMSG(1,(TEXT("\r\n---------------------------------------------------\r\n")));
    OALMSG(1,(TEXT("\r\nDPLL PER:\r\n")));    
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_AUTOIDLE_DPLL_PER);		// 0x046C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_IDLEST_DPLL_PER);			// 0x0470
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_DELTAMSTEP_DPLL_PER);	// 0x0474
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_SSC_MODFREQDIV_DPLL_PER);	// 0x0478
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKSEL_DPLL_PERIPH);		// 0x049C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKMODE_DPLL_PER);		// 0x048C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CLKDCOLDO_DPLL_PER);		// 0x047C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_DIV_M2_DPLL_PER);			// 0x04AC	
	OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    
    
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));
    OALMSG(1,(TEXT("\r\nCLKSEL:\r\n")));        
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER7_CLK);			// 0x0504
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER2_CLK);			// 0x0508
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER3_CLK);			// 0x050C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER4_CLK);			// 0x0510
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_MAC_CLKSEL);				// 0x0514
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER5_CLK);			// 0x0518
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER6_CLK);			// 0x051C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CM_CPTS_RFT_CLKSEL);			// 0x0520
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_TIMER1MS_CLK);		// 0x0528
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_GFX_FCLK);			// 0x052C
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_ICSS_OCP_CLK);		// 0x0530
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_LCDC_PIXEL_CLK);		// 0x0534
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_WDT1_CLK);			// 0x0538
	DISPLAY_REGISTER_VALUE_USING_STRUCT(pPrcmGlobalPrm, CLKSEL_GPIO0_DBCLK);			// 0x053C
    OALMSG(1,(TEXT("\r\n===================================================\r\n")));

}


//------------------------------------------------------------------------------
//
//  Function:  PrcmSuspend()
//
//  Performs the necessary steps to enter full retention and then wake-up
//  from full retention.  This routine only sets up the necessary clocks
//  to try to enter full retention.  The external preconditions for full
//  retentions must be met for full retention to be entered.
//
void PrcmSuspend()
{
    UINT32 irq = (UINT32) OAL_INTR_IRQ_UNDEFINED;

#ifdef USE_RTC_AS_WAKEUP_SOURCE    
    SYSTEMTIME startSysTime, alarmSysTime;
    LONGLONG   startFileTime, alarmFileTime;
#endif    
    
    //--------------------------------------------------------------------------
    // perform power down sequence
    //--------------------------------------------------------------------------

    // Disable match interrupt
    OALMSG(OAL_INFO,(L"PrcmSuspend: Enter\r\n"));    
    
    irq = INREG32(&g_pIntr->pICLRegs->INTC_SIR_IRQ);
    if (irq == g_oalTimerIrq)
    {
        OUTREG32(&g_pTimerRegs->IRQSTATUS, DMTIMER_TIER_OVERFLOW);
        OUTREG32(&g_pTimerRegs->IRQ_EOI, 0x00);
        OUTREG32(&g_pIntr->pICLRegs->INTC_CONTROL, IC_CNTL_NEW_IRQ);    
    }    
    PrcmDeviceEnableClocks(AM_DEVICE_TIMER3, FALSE);

    OALMSG(OAL_INFO,(L"PrcmSuspend: Disabled Timer Clock\r\n"));
    
    g_pCPUInfo->suspendState = _suspendState;
    PrcmCM3ConfigDeepSleep(_suspendState, &_suspend_DSData);

    /*	Configure EMIF for OPP50 if PER domain is left ON and VDD_CORE is reduced to 0.95V	*/   
    /* this function crashes and needs to be debugged; currently commented out
/*    if (_suspend_DSData.dsDataBits.pdPerState == PM_PER_POWERSTATE_ON)
        fnCfgEMIFOPP50Loc(g_pCPUInfo);    
    OALMSG(OAL_INFO,(L"PrcmSuspend: Done config EMIF at OPP 50\r\n"));   */
        
    // disable misc clocks - that are not handled by any driver
    PrcmPreSuspendDisableClock(_suspendState);

#ifdef USE_RTC_AS_WAKEUP_SOURCE
    // You will need to change the OAL RTC driver before you can use it as a wakeup source
    // Changes needed are:
    // 1. Set SYSCONFIG register to Smart Idle w/ wakeup
    // 2. Use 32768 Crystal as the source and not the PER PLLL 32K output; set the OSC register accordingly
    // 3. Enable alarm wakeup interrupt.
    OEMGetRealTime(&startSysTime);
    if (NKSystemTimeToFileTime(&startSysTime,(FILETIME *)&startFileTime)==0)
        OALMSG(1,(L"SystemTimeToFileTime failed\r\n"));
    alarmFileTime = startFileTime + 90 * 1000 * 10000; //30 secs
    if (NKFileTimeToSystemTime((FILETIME *)&alarmFileTime,&alarmSysTime)==0)
        OALMSG(1,(L"FileTimeToSystemTime failed\r\n"));
    if (OEMSetAlarmTime(&alarmSysTime)==0)
        OALMSG(1,(L"OEMSetAlarmTime failed\r\n"));
#endif    

#ifdef USE_TIMER1_AS_WAKEUP_SOURCE
    initializeTimer1();
    clearTimerInt();
#endif       
    
    if (g_PrcmDebugSuspendResume)
    {
        OALMSG(1,(L"PrcmSuspend: Done disabling MISC clocks %d \r\n",INTERRUPTS_STATUS()));   

        OALMSG(1,(L"OEMPowerOff: ITR before suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR3)));
        OALMSG(1,(L"OEMPowerOff: ISR_SET before suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET3)));    
        OALMSG(1,(L"OEMPowerOff: ISR_CLEAR before suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR3)));
        OALMSG(1,(L"OEMPowerOff: PENDING_IRQ before suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ0),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ1),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ2),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ3)));

        DumpAllDeviceStatus();

        OALMSG(1,(L"PrcmSuspend: PRCM DUMP REGS Before suspend\r\n"));
        Dump_PRCM();
        OALMSG(1,(L"PrcmSuspend: IPC DUMP REGS Before suspend\r\n"));
        PrcmCM3DumpIPCRegs();
    }

    
 
#ifdef USE_TIMER1_AS_WAKEUP_SOURCE 
    setTimerCount(0xffe1ffff); //60 secs
#endif

    //--------------------------------------------------------------------------
    // Suspend
    //--------------------------------------------------------------------------
    // Move SoC/CPU to idle mode (suspend)
    fnOALCPUIdle(g_pCPUInfo);

    OALMSG(1,(L"PrcmSuspend: Out of Suspend\r\n"));

    if (g_PrcmDebugSuspendResume) 
    {
        PrcmCM3DumpIPCRegs();

        OALMSG(1,(L"OEMPowerOff: after suspend %d %d \r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_SIR_IRQ),
                            INREG32(&g_pIntr->pICLRegs->INTC_SIR_FIQ)));

        OALMSG(1,(L"OEMPowerOff: ITR after suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ITR3)));
        OALMSG(1,(L"OEMPowerOff: ISR_SET after suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_SET3)));    
        OALMSG(1,(L"OEMPowerOff: ISR_CLEAR after suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR0),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR1),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR2),
                            INREG32(&g_pIntr->pICLRegs->INTC_ISR_CLEAR3)));
        OALMSG(1,(L"OEMPowerOff: PENDING_IRQ after suspend 0x%x 0x%x 0x%x 0x%x\r\n",
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ0),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ1),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ2),
                            INREG32(&g_pIntr->pICLRegs->INTC_PENDING_IRQ3)));
    }  

#ifdef USE_TIMER1_AS_WAKEUP_SOURCE 
    /*	Clear timer interrupt	*/
    clearTimerInt();
    disableTimer1();
#endif
    
    //--------------------------------------------------------------------------
    // perform power up sequence
    //--------------------------------------------------------------------------
    
    /*  Configure EMIF back for OPP100  */
/* this function crashes and needs to be debugged; currently commented out */
//    if (_suspend_DSData.dsDataBits.pdPerState == PM_PER_POWERSTATE_ON)
//        fnCfgEMIFOPP100Loc(g_pCPUInfo);

    OALMSG(OAL_INFO,(L"PrcmSuspend: Done config EMIF back to OPP100\r\n"));
    

    //enable misc clocks - that are not handled by any driver
    PrcmPostResumeEnableClock(_suspendState);

    OALMSG(OAL_INFO,(L"PrcmSuspend: Re-enable MISC clocks\r\n"));
    
    
    PrcmProcessPostMpuWakeup();

    // restart Timer
    PrcmDeviceEnableClocks(AM_DEVICE_TIMER3, TRUE);    
    OALTimerStart();

    OALMSG(OAL_INFO,(L"PrcmSuspend: Re-enable TIMER\r\n"));
    

    // turn off PM
    g_pCPUInfo->suspendState = PM_CMD_NO_PM;    
    
}

//------------------------------------------------------------------------------
BOOL OALIoCtlPrcmDeviceGetDeviceManagementTable(
    UINT32 code, VOID  *pInBuffer,     UINT32 inSize, 
    VOID  *pOutBuffer, UINT32 outSize, UINT32 *pOutSize
    )
//	returns a table referencing the device clock management functions
{
    BOOL rc = FALSE;
    OMAP_DEVCLKMGMT_FNTABLE *pfnTbl;

    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetDeviceManagementTable\r\n"));
    if (pOutBuffer == NULL || outSize < sizeof(OMAP_DEVCLKMGMT_FNTABLE))
        {
        goto cleanUp;
        }

    // update return information
    //
    if (pOutSize != NULL) *pOutSize = sizeof(OMAP_DEVCLKMGMT_FNTABLE);

    // update function pointers
    //
    pfnTbl = (OMAP_DEVCLKMGMT_FNTABLE*)pOutBuffer;    
    pfnTbl->pfnEnableDeviceClocks= PrcmDeviceEnableClocks;
    pfnTbl->pfnEnableDeviceIClock= PrcmDeviceEnableIClock;
    pfnTbl->pfnEnableDeviceFClock= PrcmDeviceEnableFClock;
    pfnTbl->pfnSetSourceDeviceClocks = PrcmDeviceSetSourceClocks;
    pfnTbl->pfnEnableDeviceClockAutoIdle = NULL;
    pfnTbl->pfnGetDeviceContextState = PrcmDeviceGetContextState;
    pfnTbl->pfnUpdateOnDeviceStateChange = OALMux_UpdateOnDeviceStateChange;
    pfnTbl->pfnGetSystemClockFrequency = PrcmClockGetSystemClockFrequency;
	pfnTbl->pfnPrcmDomainResetRelease  = PrcmDomainResetRelease;
	pfnTbl->pfnPrcmDomainReset         = PrcmDomainReset;
	pfnTbl->pfnPrcmDomainResetStatus   = PrcmDomainResetStatus; 
    pfnTbl->pfnPrcmClockGetClockRate   = PrcmClockGetClockRate; 
    pfnTbl->pfnPrcmCM3ResetAndHandshake = PrcmCM3ResetAndHandshake;
    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetDeviceManagementTable(rc = %d)\r\n", rc));
    return rc;
}

#endif
//-----------------------------------------------------------------------------
