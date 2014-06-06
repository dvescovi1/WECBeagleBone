// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  profiler_mhz.c
//
#include "omap.h"
#include "omap_gptimer_regs.h"
#include "omap_prcm_regs.h"
#include "bsp_cfg.h"
#include "soc_cfg.h"
#include "oalex.h"
#include "oal_clock.h"
#include "oal_prcm.h"

//------------------------------------------------------------------------------
// globals
//
BOOL g_oalProfilerEnabled = FALSE;
UINT32 g_oalPerfTimerIrq = (UINT32)-1;
DWORD g_oalProfilerIncrement;
OMAP_GPTIMER_REGS   *g_pPerfTimer = NULL;

//------------------------------------------------------------------------------
// staticx
//
static UINT32                s_Frequency;

// UNDONE:
//  Need to initialize all gpt to select clk src to sys_clk

//------------------------------------------------------------------------------
//
//  Function: OALPerformanceTimerInit
//
//  This function initializes the high resolution counter
//
void OALPerformanceTimerInit()
{
    UINT srcClock;
    DWORD tclr = 0;	

    OMAP_DEVICE gptPerfDevice = BSPGetGPTPerfDevice();
    if (gptPerfDevice == OMAP_DEVICE_NONE)
    {
        return;
    }

    // map HighResTimer
    g_pPerfTimer = OALPAtoUA(GetAddressByDevice(gptPerfDevice));
    g_oalPerfTimerIrq = GetIrqByDevice(gptPerfDevice,NULL);

	// Select high frequency source clock and frequency
    srcClock = BSPGetGPTPerfHighFreqClock(&s_Frequency);
	//PrcmDeviceSetSourceClocks(gptPerfDevice,1,&srcClock);
    // Enable GPTimer for high perf/monte carlo profiling
    EnableDeviceClocks(gptPerfDevice, TRUE);


    // configure performance timer
    //---------------------------------------------------
    // Soft reset GPTIMER and wait until finished
    SETREG32(&g_pPerfTimer->TIOCP, SYSCONFIG_SOFTRESET);
    while ((INREG32(&g_pPerfTimer->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);
 
    // Enable smart idle and autoidle
    // Set clock activity - FCLK can be  switched off, 
    // L4 interface clock is maintained during wkup.
    OUTREG32(&g_pPerfTimer->TIOCP, 
        0x200 | SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP|
            SYSCONFIG_AUTOIDLE); 

    // Select posted mode
    SETREG32(&g_pPerfTimer->TSICR, GPTIMER_TSICR_POSTED);

    // clear match register
    OUTREG32(&g_pPerfTimer->TMAR, 0xFFFFFFFF);
    
    // clear interrupts
    OUTREG32(&g_pPerfTimer->TISR, 0x00000000);
    
    // enable match interrupt
    OUTREG32(&g_pPerfTimer->TIER, GPTIMER_TIER_MATCH);
    
    // enable wakeups
    OUTREG32(&g_pPerfTimer->TWER, GPTIMER_TWER_MATCH);

    // Set the load register value.
    OUTREG32(&g_pPerfTimer->TLDR, 0x00000000);
 
    // Trigger a counter reload by writing    
    OUTREG32(&g_pPerfTimer->TTGR, 0xFFFFFFFF);



    OALMSG(1, (L"****Profiler Build****\r\n"));
    OALMSG(1, (L"---High Performance Frequency is %d hz---\r\n", s_Frequency));
    
    // build tclr mask
    tclr |= GPTIMER_TCLR_AR;
    OUTREG32(&g_pPerfTimer->TCLR,  tclr);
   
    // Initialize high resolution timer function pointers
    pQueryPerformanceFrequency = OALTimerQueryPerformanceFrequency;
    pQueryPerformanceCounter = OALTimerQueryPerformanceCounter;    
    g_pOemGlobal->pfnProfileTimerEnable = OEMProfileTimerEnable;
    g_pOemGlobal->pfnProfileTimerDisable = OEMProfileTimerDisable;
    

    //  Start the timer.  Also set for auto reload
    SETREG32(&g_pPerfTimer->TCLR, GPTIMER_TCLR_ST);
    while ((INREG32(&g_pPerfTimer->TWPS) & GPTIMER_TWPS_TCLR) != 0);
    
}


//------------------------------------------------------------------------------
//
//  Function: OALTimerQueryPerformanceFrequency
//
//  This function returns the frequency of the high-resolution 
//  performance counter.
// 
BOOL OALTimerQueryPerformanceFrequency(LARGE_INTEGER *pFrequency)
{    
    pFrequency->QuadPart = s_Frequency;    
    return TRUE;
}

 
//------------------------------------------------------------------------------
//
//  Function: OALQueryPerformanceCounter
//
//  This function returns the current value of the high-resolution 
//  performance counter.
//  
BOOL OALTimerQueryPerformanceCounter(LARGE_INTEGER *pCounter)
{
    static LARGE_INTEGER PreviousValue = {0, 0};
    DWORD CurrentValue;
    BOOL bInterruptsEnabled;
    
    bInterruptsEnabled = INTERRUPTS_ENABLE(FALSE);

    CurrentValue = INREG32(&g_pPerfTimer->TCRR);
    if (CurrentValue < PreviousValue.LowPart)
    {
        // rollover, increment upper 32 bits
        PreviousValue.HighPart++;
    }
    PreviousValue.LowPart = CurrentValue;
    pCounter->QuadPart = PreviousValue.QuadPart;            

    INTERRUPTS_ENABLE(bInterruptsEnabled);

    return TRUE;
}


//------------------------------------------------------------------------------

void OEMProfileTimerEnable(DWORD dwUSecInterval)
{
    DWORD tcrr;
    UINT64 val;
    BOOL enabled;
    
    
    // calculate interrupt intervals   
    val = (UINT64) dwUSecInterval * (UINT64)s_Frequency;
    val = val / 1000000;
    g_oalProfilerIncrement = (UINT32) val;

    // Following code should not be interrupted
    enabled = INTERRUPTS_ENABLE(FALSE);
    
    // set interrupts at requested intervals
    tcrr = INREG32(&g_pPerfTimer->TCRR) + g_oalProfilerIncrement;
    OUTREG32(&g_pPerfTimer->TMAR, tcrr);
    while ((INREG32(&g_pPerfTimer->TWPS) & GPTIMER_TWPS_TMAR) != 0);
    
    // enable high perf interrupt
    g_oalProfilerEnabled = TRUE;
    SETREG32(&g_pPerfTimer->TCLR, GPTIMER_TCLR_CE);

    // Enable profiling interrupt
    OALIntrEnableIrqs(1, &g_oalPerfTimerIrq);

    // Enable interrupts
    INTERRUPTS_ENABLE(enabled);
    
}

//------------------------------------------------------------------------------

void OEMProfileTimerDisable(void)
{
    BOOL enabled;
    
    if (g_oalProfilerEnabled == FALSE) return;

    // Following code should not be interrupted
    enabled = INTERRUPTS_ENABLE(FALSE);

    // Disable the profile timer interrupt
    OALIntrDisableIrqs(1, &g_oalPerfTimerIrq);    

    // Reset flag
    g_oalProfilerEnabled = FALSE;
    CLRREG32(&g_pPerfTimer->TCLR, GPTIMER_TCLR_CE);

    // Enable interrupts
    INTERRUPTS_ENABLE(enabled);
}

//------------------------------------------------------------------------------

void 
OALProfileTimerHit(
    UINT32 ra
    )
{
    UINT32  tcrr;

    //  Call ProfileHit
    ProfilerHit(ra);

    // get current time
    tcrr = INREG32(&g_pPerfTimer->TCRR);
    
    // clear interrupt
    OUTREG32(&g_pPerfTimer->TISR, 
        GPTIMER_TISR_MAT | GPTIMER_TISR_OVF | GPTIMER_TISR_TCAR
        );

    // setup for next interrupt        
    OUTREG32(&g_pPerfTimer->TMAR, tcrr + g_oalProfilerIncrement);

    // clear interrupt status
    OALIntrDoneIrqs(1, &g_oalPerfTimerIrq);
}
