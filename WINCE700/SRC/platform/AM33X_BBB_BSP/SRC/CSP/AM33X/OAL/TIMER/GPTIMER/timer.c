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
//  File:  timer.c
//
#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <oal.h>

//#include <oalex.h>
//#include <am33x.h>
//#include <am33x_oal_prcm.h>

#include <am3xx_gptimer.h>
#include "bsp_cfg.h"
#include "soc_cfg.h"

BOOL EnableDeviceClocks( DWORD devID, BOOL bEnable );

// TODO: VA where is good place for the definition ?   
//#define OAL_TIMER_RTC

OAL_TIMER_STATE g_oalTimer;

#ifdef OAL_TIMER_RTC
extern UINT64 *g_pOALRTCTicks;
extern UINT64 g_oalRTCTicks;
extern UINT64 g_oalRTCAlarm;
#endif

//-----------------------------------------------------------------------------
#define DELTA_TIME              2000          	// In TICK
#ifndef MAX_INT
#define MAX_INT                 0x7FFFFFFF
#endif

//------------------------------------------------------------------------------
//  Structure maintaining system timer and tick count values
typedef struct AM3XX_TIMER_CONTEXT {
    UINT32 maxPeriodMSec;               // Maximal timer period in MSec
    UINT32 margin;                      // Margin of time need to reprogram timer interrupt

    volatile UINT64 curCounts;          // Counts at last system tick
    volatile UINT32 base;               // Timer value at last interrupt
    volatile UINT32 match;              // Timer match value for next interrupt

} AM3XX_TIMER_CONTEXT;

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish system timer interrupt.
extern UINT32           g_oalTimerIrq;

//------------------------------------------------------------------------------
//  maximum idle period during OS Idle in milliseconds
extern DWORD dwOEMMaxIdlePeriod;

//------------------------------------------------------------------------------
//  This is global instance of timer registers
AM3XX_GPTIMER_REGS*      g_pTimerRegs;

AM3XX_TIMER_CONTEXT      g_oalTimerContext;

//------------------------------------------------------------------------------
//  system frequency in kHz
UINT32 g_oalSysFreqKHz;
//#define SYSFREQ_KHZ 24000

//------------------------------------------------------------------------------
//  Define: MSEC / TICK conversions
#define MSEC_TO_TICK(msec)  ((msec) * g_oalSysFreqKHz )      // msec * 2m000
#define TICK_TO_MSEC(tick)  ((tick) / g_oalSysFreqKHz)       // msec / 2m000

//------------------------------------------------------------------------------
VOID OEMIdle( DWORD idleParam )
{    
    UINT idleDelta, newIdleLow;
    UINT tcrrEnter, tcrrExit;
    
    UNREFERENCED_PARAMETER(idleParam);
    
    tcrrEnter = OALTimerGetCount();
    BSPCPUIdle();
//    fnOALCPUIdle(g_pCPUInfo);
    tcrrExit = OALTimerGetCount();
    
    if (tcrrExit < tcrrEnter)
        idleDelta = (tcrrExit + g_oalSysFreqKHz)- tcrrEnter;
    else
        idleDelta = tcrrExit- tcrrEnter;
    
    newIdleLow = curridlelow + idleDelta;
    if (newIdleLow < curridlelow) 
	    curridlehigh++;
    curridlelow = newIdleLow;
    
    return;
}

//------------------------------------------------------------------------------
//  General purpose timer 1 (2-nd timer) is used for system tick
BOOL OALTimerInit( UINT32 sysTickMSec, UINT32 countsPerMSec, UINT32 countsMargin )
{
    BOOL rc = FALSE;
    UINT32 sysIntr;

    OALMSG(1/*OAL_TIMER&&OAL_FUNC*/, ( L"+OALTimerInit(%d, %d, %d)\r\n", sysTickMSec, countsPerMSec,
        countsMargin ));

	g_oalSysFreqKHz = SOCGetSysFreqKHz();

    g_oalTimer.countsPerMSec          = countsPerMSec;
    g_oalTimer.msecPerSysTick         = sysTickMSec;
    g_oalTimer.actualMSecPerSysTick   = sysTickMSec;
    g_oalTimer.countsMargin           = countsMargin;

    g_oalTimer.countsPerSysTick       = (countsPerMSec * sysTickMSec);
    g_oalTimer.actualCountsPerSysTick = (countsPerMSec * sysTickMSec);
    g_oalTimer.curCounts              = 0;
    g_oalTimer.maxPeriodMSec          = 0xFFFFFFFF/g_oalTimer.countsPerMSec;

    // Set idle conversion constant and counters
    idleconv     = MSEC_TO_TICK(1);
    curridlehigh = 0;
    curridlelow  = 0;
	CurMSec		 = 0;

    g_pTimerRegs = OALPAtoUA(GetAddressByDevice(BSPGetSysTimerDeviceId()));

//OALMSG(1, (L"OALTimerInit: g_pTimerRegs - 0x%08X\r\n", g_pTimerRegs));

#ifdef OAL_TIMER_RTC
		g_pOALRTCTicks = &g_oalRTCTicks;
		g_oalRTCAlarm = 0;
#endif

    // enable gptimer3
//TODO VA 	PrcmClockSetParent(kTIMER3_GCLK, kSYS_CLKIN_CK);
	EnableDeviceClocks(BSPGetSysTimerDeviceId(), TRUE);
    
    OUTREG32(&g_pTimerRegs->TCLR, 0);						// stop timer
    OUTREG32(&g_pTimerRegs->TIOCP, GPTIMER_TIOCP_SOFTRESET);	    // Soft reset GPTIMER
    while ((INREG32(&g_pTimerRegs->TIOCP) & GPTIMER_TIOCP_SOFTRESET) != 0);	// While until done
    // Set smart idle
    // VA TODO: use the correct definitions for timet settings
    // Change the definitions in the corresponding *.h file
    // Find out what acctual idle settings hve to be set for that timer

	//TODO: 0x4 for test only REMOVE later  
    OUTREG32( &g_pTimerRegs->TIOCP, 0x4 /*SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP| SYSCONFIG_AUTOIDLE*/);
    OUTREG32(&g_pTimerRegs->TSICR, GPTIMER_TSICR_POSTED);	// Enable posted mode

    OUTREG32(&g_pTimerRegs->TLDR, 0xFFFFFFFF - countsPerMSec + 1); 
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TLDR) != 0); // Wait until write is done

    OUTREG32(&g_pTimerRegs->TMAR, 0xFFFFFFFF); // Set match register to avoid unwanted interrupt
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0); // Wait until write is done
    OUTREG32(&g_pTimerRegs->IRQENABLE_SET, GPTIMER_TWER_OVERFLOW); // Enable match interrupt
    OUTREG32(&g_pTimerRegs->IRQWAKEEN, GPTIMER_TWER_OVERFLOW); // Enable match wakeup
    // Enable timer in auto-reload --- and compare mode VA: not for fixed interval ---
    OUTREG32(&g_pTimerRegs->TCLR,/* GPTIMER_TCLR_CE |*/ GPTIMER_TCLR_AR | GPTIMER_TCLR_ST );
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TCLR) != 0); // Wait until write is done

    // Set global variable to tell interrupt module about timer used
    g_oalTimerIrq = GetIrqByDevice(BSPGetSysTimerDeviceId(),NULL);;

    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalTimerIrq, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0)){
        OALMSG(OAL_ERROR, (L"ERROR: OALTimerInit: Interrupt enable for system timer failed"));
        goto cleanUp;
    }

	OUTREG32(&g_pTimerRegs->TCRR, 0xFFFFFFFF - countsPerMSec + 1); 
	while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TCRR) != 0); // Wait until write is done

    rc = TRUE;

cleanUp:
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OALTimerInit(rc = %d)\r\n", rc));
    return rc;
}

VOID OALTimerStart(VOID)
{
// enable the "if 0" code when testing DS0 mode - since the timer context may be lost when A8 went to DS0
#if 0
    OUTREG32(&g_pTimerRegs->TCLR, 0);						// stop timer
    OUTREG32(&g_pTimerRegs->TIOCP, GPTIMER_TIOCP_SOFTRESET);	    // Soft reset GPTIMER
    while ((INREG32(&g_pTimerRegs->TIOCP) & GPTIMER_TIOCP_SOFTRESET) != 0);	// While until done

	//TODO: 0x4 for test only REMOVE later  
    OUTREG32( &g_pTimerRegs->TIOCP, 0x4 /*SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP| SYSCONFIG_AUTOIDLE*/);
    OUTREG32(&g_pTimerRegs->TSICR, GPTIMER_TSICR_POSTED);	// Enable posted mode

    OUTREG32(&g_pTimerRegs->TLDR, 0xFFFFFFFF - g_oalTimer.countsPerMSec + 1); 
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TLDR) != 0); // Wait until write is done

    OUTREG32(&g_pTimerRegs->TMAR, 0xFFFFFFFF); // Set match register to avoid unwanted interrupt
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0); // Wait until write is done
    OUTREG32(&g_pTimerRegs->IRQENABLE_SET, GPTIMER_TWER_OVERFLOW); // Enable match interrupt
    OUTREG32(&g_pTimerRegs->IRQWAKEEN, GPTIMER_TWER_OVERFLOW); // Enable match wakeup
#endif

    OUTREG32(&g_pTimerRegs->TCLR, GPTIMER_TCLR_AR | GPTIMER_TCLR_ST );
    while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TCLR) != 0); // Wait until write is done

	
}



//------------------------------------------------------------------------------
VOID OALTimerStop(VOID)
{
	OUTREG32(&g_pTimerRegs->TCLR, (INREG32(&g_pTimerRegs->TCLR) & ~(GPTIMER_TCLR_ST)));
	while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TCLR) != 0); // Wait until write is done
}

#if 0
//------------------------------------------------------------------------------
//  This function is called by kernel to set next reschedule time.
//
VOID OALTimerUpdateRescheduleTime( DWORD timeMSec )
{
    UINT32 baseMSec, periodMSec;
    INT32 delta;

    baseMSec = CurMSec;   // Get current system timer counter

    // How far we are from next tick
    delta = (INT32)(g_oalTimerContext.match - OALTimerGetCount());

    if( delta < 0 )
    {
        UpdatePeriod(0);
        goto cleanUp;
    }

    // If timer interrupts occurs, or we are within 1 ms of the scheduled
    // interrupt, just return - timer ISR will take care of it.
    if ((baseMSec != CurMSec) || (delta < MSEC_TO_TICK(1))) goto cleanUp;

    // Calculate the distance between the new time and the last timer interrupt
      periodMSec = timeMSec - OEMGetTickCount();

    // Trying to set reschedule time prior or equal to CurMSec - this could
    // happen if a thread is on its way to sleep while preempted before
    // getting into the Sleep Queue
    if ((INT32)periodMSec < 0){
        periodMSec = 0;
    } else if (periodMSec > g_oalTimerContext.maxPeriodMSec) {
        periodMSec = g_oalTimerContext.maxPeriodMSec;
    }

    // Now we find new period, so update timer
    UpdatePeriod(periodMSec);

cleanUp:
    return;
}

#endif

//------------------------------------------------------------------------------
//
//  Function: OALTimerIntrHandler
//
//  This function implement timer interrupt handler. It is called from common
//  ARM interrupt handler.
//
UINT32 OALTimerIntrHandler()
{
	UINT32 SysIntrVal = SYSINTR_NOP;

	// Update high resolution counter
	g_oalTimer.curCounts += (UINT64)g_oalTimer.countsPerSysTick;

	// Update the millisecond counter
	CurMSec += g_oalTimer.msecPerSysTick;

	// Reschedule?
	if ( (int)(CurMSec - dwReschedTime) >= 0 ){
		SysIntrVal = SYSINTR_RESCHED;
	}

#ifdef OAL_TIMER_RTC
	// Update RTC global variable
	*g_pOALRTCTicks += g_oalTimer.actualMSecPerSysTick;
	// When RTC alarm is active check if it has to be fired
	if (g_oalRTCAlarm > 0) {
		if (g_oalRTCAlarm > g_oalTimer.actualMSecPerSysTick) {
			g_oalRTCAlarm -= g_oalTimer.actualMSecPerSysTick;
		} else {
			OALMSG(1, (L"SYSINTR_RTC_ALARM \r\n" ) );
			g_oalRTCAlarm = 0;
			SysIntrVal = SYSINTR_RTC_ALARM;
		}			 
	}
#endif
	
#ifdef OAL_ILTIMING
	if (TRUE == g_oalILT.active){
		if ( --g_oalILT.counter == 0 ){
			SysIntrVal = SYSINTR_TIMING;
			g_oalILT.counter = g_oalILT.counterSet;
			g_oalILT.isrTime2 = OALTimerCountsSinceSysTick();
			g_oalILT.interrupts = 0;
		}
	}
#endif

	// Clear interrupt
	OUTREG32(&g_pTimerRegs->IRQSTATUS, GPTIMER_TIER_OVERFLOW);
	OUTREG32(&g_pTimerRegs->IRQ_EOI, 0x00);

	return (SysIntrVal);


}

//------------------------------------------------------------------------------
//
//  Function:  OALTimerGetCount
//
UINT32 OALTimerGetCount()
{
    //  Return the timer value
    return INREG32(&g_pTimerRegs->TCRR)-(0xFFFFFFFF-g_oalSysFreqKHz);
}

//------------------------------------------------------------------------------
INT32 OALTimerCountsSinceSysTick()
{
    // Return timer ticks since last interrupt
    return (INT32)OALTimerGetCount();
}

//------------------------------------------------------------------------------
//  This function returns number of 1 ms ticks which elapsed since system boot
//  or reset (absolute value isn't important). The counter can overflow but
//  overflow period should not be shorter then approx 30 seconds. Function
//  is used in  system boot so it must work before interrupt subsystem
//  is active.
//
UINT32 OALGetTickCount( )
{
    return CurMSec;;
}

//------------------------------------------------------------------------------
// supports the 1msec system tick only
UINT32 OEMGetTickCount( )
{
    return CurMSec;
}

//------------------------------------------------------------------------------

