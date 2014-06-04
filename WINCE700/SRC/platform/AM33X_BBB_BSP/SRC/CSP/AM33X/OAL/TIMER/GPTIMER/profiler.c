//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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
//  File:  profiler_khz.c
//
#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <oal.h>

//#include <oalex.h>
//#include <am33x.h>
//#include "am33x_oal_prcm.h"

#include <am3xx_gptimer.h>
#include "bsp_cfg.h"
#include "soc_cfg.h"

BOOL EnableDeviceClocks( DWORD devID, BOOL bEnable );

//------------------------------------------------------------------------------
BOOL g_oalProfilerEnabled = FALSE;
UINT32 g_oalPerfTimerIrq = (UINT32)-1;
DWORD g_oalProfilerIncrement;

extern UINT32          g_oalTimerIrq;
AM3XX_GPTIMER_REGS*   g_pHPTimerRegs;
//------------------------------------------------------------------------------
// static
//
static UINT32                s_Frequency;

//------------------------------------------------------------------------------
void OALPerformanceTimerInit(DWORD clock, DWORD flag)
//	Initialize the high performance clock for profiling
{
    OALMSG(TRUE, (L"--- High Performance Frequecy is 24 MHz---\r\n"));

    g_pHPTimerRegs = OALPAtoUA(GetAddressByDevice(BSPGetGPTPerfDevice()));
    s_Frequency = SOCGetSysFreqKHz()* 1000;
    g_oalPerfTimerIrq = GetIrqByDevice(BSPGetGPTPerfDevice(),NULL);    

    EnableDeviceClocks(BSPGetGPTPerfDevice(), TRUE);
//    PrcmClockSetParent(kTIMER2_GCLK, kSYS_CLKIN_CK);

    OUTREG32(&g_pHPTimerRegs->TCLR, 0);						      // stop timer
    OUTREG32(&g_pHPTimerRegs->TIOCP, GPTIMER_TIOCP_SOFTRESET);	  // Soft reset GPTIMER
    while ((INREG32(&g_pHPTimerRegs->TIOCP) & GPTIMER_TIOCP_SOFTRESET) != 0);	// While until done

	//TODO: 0x4 for test only REMOVE later  
    OUTREG32( &g_pHPTimerRegs->TIOCP, 0x4 /*SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP| SYSCONFIG_AUTOIDLE*/);
    OUTREG32(&g_pHPTimerRegs->TSICR, GPTIMER_TSICR_POSTED);	// Enable posted mode

    OUTREG32(&g_pHPTimerRegs->TMAR, 0xFFFFFFFF);            // Set match register to avoid unwanted interrupt
    while ((INREG32(&g_pHPTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0); // Wait until write is done

    OUTREG32(&g_pHPTimerRegs->TLDR,  0x00000000); 
    while ((INREG32(&g_pHPTimerRegs->TWPS) & GPTIMER_TWPS_TLDR) != 0); // Wait until write is done

    // enable match interrupt
    OUTREG32(&g_pHPTimerRegs->IRQENABLE_SET, GPTIMER_TIER_MATCH);
    
    // enable wakeups
    OUTREG32(&g_pHPTimerRegs->IRQWAKEEN, GPTIMER_TWER_MATCH);

    // Enable timer in auto-reload --- and compare mode VA: not for fixed interval ---
    OUTREG32(&g_pHPTimerRegs->TCLR, GPTIMER_TCLR_AR | GPTIMER_TCLR_ST );
    while ((INREG32(&g_pHPTimerRegs->TWPS) & GPTIMER_TWPS_TCLR) != 0); // Wait until write is done

    // Initialize high resolution timer function pointers
    pQueryPerformanceFrequency = OALTimerQueryPerformanceFrequency;
    pQueryPerformanceCounter = OALTimerQueryPerformanceCounter;

    // Profiler timer is system tick timer
   // g_oalPerfTimerIrq = g_oalTimerIrq;
}

//------------------------------------------------------------------------------
BOOL OALTimerQueryPerformanceFrequency(LARGE_INTEGER *pFrequency)
//	This function returns the frequency of the high-resolution 
//	performance counter.
{
    pFrequency->QuadPart = s_Frequency;    
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL OALTimerQueryPerformanceCounter(LARGE_INTEGER *pCounter)
//	This function returns the current value of the high-resolution 
//	performance counter.
{
    static LARGE_INTEGER PreviousValue = {0, 0};
    DWORD CurrentValue;
    BOOL bInterruptsEnabled;

    bInterruptsEnabled = INTERRUPTS_ENABLE(FALSE);

    CurrentValue = INREG32(&g_pHPTimerRegs->TCRR);
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
VOID OEMProfileTimerEnable( DWORD interval )
{
    DWORD tcrr;
    UINT64 val;
    BOOL enabled;

    OALMSG(1, (L"OEMProfileTimerEnable:interval =%d\r\n", interval));
    // calculate interrupt intervals   
    val = (UINT64) interval * (UINT64)s_Frequency;
    val = val / 1000000;
    g_oalProfilerIncrement = (UINT32) val;

    // Following code should not be interrupted
    enabled = INTERRUPTS_ENABLE(FALSE);
    
    // set interrupts at requested intervals
    tcrr = INREG32(&g_pHPTimerRegs->TCRR) + g_oalProfilerIncrement;
    OUTREG32(&g_pHPTimerRegs->TMAR, tcrr);
    while ((INREG32(&g_pHPTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0);
    
    // enable high perf interrupt
    g_oalProfilerEnabled = TRUE;
    SETREG32(&g_pHPTimerRegs->TCLR, GPTIMER_TCLR_CE);

    // Enable profiling interrupt
    OALIntrEnableIrqs(1, &g_oalPerfTimerIrq);

    // Enable interrupts
    INTERRUPTS_ENABLE(enabled);
	
}

//------------------------------------------------------------------------------
VOID OEMProfileTimerDisable( ) 
{
    BOOL enabled;

    OALMSG(1, (L"OEMProfileTimerDisable\r\n"));
    
    if (g_oalProfilerEnabled == FALSE) return;

    // Following code should not be interrupted
    enabled = INTERRUPTS_ENABLE(FALSE);

    // Disable the profile timer interrupt
    OALIntrDisableIrqs(1, &g_oalPerfTimerIrq);    

    // Reset flag
    g_oalProfilerEnabled = FALSE;
    CLRREG32(&g_pHPTimerRegs->TCLR, GPTIMER_TCLR_CE);

    // Enable interrupts
    INTERRUPTS_ENABLE(enabled);
	
}

//------------------------------------------------------------------------------
void  OALProfileTimerHit( UINT32 ra )
{
    UINT32  tcrr;

    OALMSG(1, (L"OALProfileTimerHit, ra=%d\r\n", ra));
    //  Call ProfileHit
    ProfilerHit(ra);

    // get current time
    tcrr = INREG32(&g_pHPTimerRegs->TCRR);
    
    // clear interrupt
    OUTREG32(&g_pHPTimerRegs->IRQENABLE_CLR, 
        GPTIMER_TISR_MAT | GPTIMER_TISR_OVF | GPTIMER_TISR_TCAR
        );

    // setup for next interrupt        
    OUTREG32(&g_pHPTimerRegs->TMAR, tcrr + g_oalProfilerIncrement);
    while ((INREG32(&g_pHPTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0);

    // clear interrupt status
    OALIntrDoneIrqs(1, &g_oalPerfTimerIrq);
	
}

