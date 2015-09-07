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

#include "am33x_dmtimer.h"
#include "bsp_cfg.h"
#include "soc_cfg.h"
#include "oal_clock.h"

//------------------------------------------------------------------------------
// Definitions
//
#define DELTA_TIME              2000          	// In TICK
#ifndef MAX_INT
#define MAX_INT                 0x7FFFFFFF
#endif

#define MSEC_TO_TICK(msec)  ((msec) * g_oalSysFreqKHz)
#define TICK_TO_MSEC(tick)  ((tick) / g_oalSysFreqKHz)

//------------------------------------------------------------------------------
//  Structure maintaining system timer and tick count values
typedef struct AM_TIMER_CONTEXT {
    UINT32 maxPeriodMSec;               // Maximal timer period in MSec
    UINT32 margin;                      // Margin of time need to reprogram timer interrupt

    volatile UINT64 curCounts;          // Counts at last system tick
    volatile UINT32 base;               // Timer value at last interrupt
    volatile UINT32 match;              // Timer match value for next interrupt

} AM_TIMER_CONTEXT;

//------------------------------------------------------------------------------
// Global variables
//
OAL_TIMER_STATE		  g_oalTimer = {0};
AM33X_DMTIMER_REGS*   g_pTimerRegs = NULL;
AM_TIMER_CONTEXT      g_oalTimerContext = {0};
UINT32				  g_oalSysFreqKHz = 0;

#ifdef OAL_TIMER_RTC
extern UINT64 *g_pOALRTCTicks;
extern UINT64 g_oalRTCTicks;
extern UINT64 g_oalRTCAlarm;
#endif

extern void BSPCPUIdle();

//------------------------------------------------------------------------------
// External variables
extern UINT32 g_oalTimerIrq;
extern DWORD dwOEMMaxIdlePeriod;

//------------------------------------------------------------------------------
// Internal prototypes
UINT32 OEMGetTickCount();

//------------------------------------------------------------------------------

static
VOID
OALTimerWritePosted(volatile UINT32* pRegister, UINT32 value, UINT32 pendingBit)
{
	volatile DWORD timeout = 1000;

	if (!g_pTimerRegs)
	{
		return;
	}

	OUTREG32(pRegister, value);
    while (((INREG32(&g_pTimerRegs->TWPS) & pendingBit) != 0) && (timeout-- > 0)); // Wait until write is done

	if (timeout == 0)
	{
		OALMSG(1, ( L"OALTimerWritePosted: posted write timed out\r\n"));
	}
}

static
VOID
UpdatePeriod(UINT32 periodMSec)
{
    UINT32 period, match;
    INT32 delta;
    UINT64 offsetMSec = periodMSec;
    UINT64 tickCount = OALGetTickCount();

    // Calculate count difference
    period = (UINT32)MSEC_TO_TICK(offsetMSec);

    // This is compare value
    match = ((UINT32)MSEC_TO_TICK(tickCount)) + period;

    delta = (INT32)(INREG32(&g_pTimerRegs->TCRR) + g_oalTimerContext.margin - match);

    // If we are behind, issue interrupt as soon as possible
    if (delta > 0)
    {
        match += MSEC_TO_TICK(1);
    }

    // Save off match value
    g_oalTimerContext.match = match;

    // Set timer match value
	OALTimerWritePosted(&g_pTimerRegs->TMAR, match, DMTIMER_TWPS_TMAR);

    // make sure we don't set next timer interrupt to the past
    if (match < INREG32(&g_pTimerRegs->TCRR)) UpdatePeriod(1);
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerInit
//
//  Init system tick timer
//
BOOL OALTimerInit( UINT32 sysTickMSec, UINT32 countsPerMSec, UINT32 countsMargin )
{
    BOOL rc = FALSE;
    UINT32 sysIntr;

    OALMSG(1/*OAL_TIMER&&OAL_FUNC*/, ( L"+OALTimerInit(%d, %d, %d)\r\n", sysTickMSec, countsPerMSec,
        countsMargin ));

	g_oalSysFreqKHz = BSPGetSysTimerFreqKHz();

    g_oalTimer.countsPerMSec          = countsPerMSec;
    g_oalTimer.msecPerSysTick         = sysTickMSec;
    g_oalTimer.actualMSecPerSysTick   = sysTickMSec;
    g_oalTimer.countsMargin           = countsMargin;

    g_oalTimer.countsPerSysTick       = (countsPerMSec * sysTickMSec);
    g_oalTimer.actualCountsPerSysTick = (countsPerMSec * sysTickMSec);
    g_oalTimer.curCounts              = 0;
    g_oalTimer.maxPeriodMSec          = dwOEMMaxIdlePeriod;

	g_oalTimerContext.match			  = 0xFFFFFFFF;
	g_oalTimerContext.margin		  = DELTA_TIME;
	g_oalTimerContext.base			  = 0;

    // Set idle conversion constant and counters
    idleconv     = MSEC_TO_TICK(1);
    curridlehigh = 0;
    curridlelow  = 0;
	CurMSec		 = 0;

	// Use variable system tick
    pOEMUpdateRescheduleTime = OALTimerUpdateRescheduleTime;

    g_pTimerRegs = OALPAtoUA(GetAddressByDevice(BSPGetSysTimerDevice()));

#ifdef OAL_TIMER_RTC
		g_pOALRTCTicks = &g_oalRTCTicks;
		g_oalRTCAlarm = 0;
#endif

    // Enable System timer clock
	EnableDeviceClocks(BSPGetSysTimerDevice(), TRUE);
    
	// stop timer
    OUTREG32(&g_pTimerRegs->TCLR, 0);

	// soft reset timer
    OUTREG32(&g_pTimerRegs->TIOCP, DMTIMER_TIOCP_SOFTRESET);

	// while until done
    while ((INREG32(&g_pTimerRegs->TIOCP) & DMTIMER_TIOCP_SOFTRESET) != 0);

	// Set smart idle
    OUTREG32(&g_pTimerRegs->TIOCP, DMTIMER_TIOCP_SMART_IDLE);

	// Enable posted mode
	OUTREG32(&g_pTimerRegs->TSICR, DMTIMER_TSICR_POSTED);

    // Set match register to avoid unwanted interrupt
	OALTimerWritePosted(&g_pTimerRegs->TMAR, 0xFFFFFFFF, DMTIMER_TWPS_TMAR);

	// Enable match interrupt
    OUTREG32(&g_pTimerRegs->IRQENABLE_SET, DMTIMER_TWER_MATCH); 

	// Enable match wakeup
    OUTREG32(&g_pTimerRegs->IRQWAKEEN, DMTIMER_TWER_MATCH); 

    // Enable timer in auto-reload --- and compare mode VA: not for fixed interval ---
	OALTimerWritePosted(&g_pTimerRegs->TCLR, DMTIMER_TCLR_CE | DMTIMER_TCLR_AR | DMTIMER_TCLR_ST, DMTIMER_TWPS_TCLR);

    // Set global variable to tell interrupt module about timer used
    g_oalTimerIrq = GetIrqByDevice(BSPGetSysTimerDevice(), NULL);

    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalTimerIrq, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
	{
        OALMSG(1 /*OAL_ERROR*/, (L"ERROR: OALTimerInit: Interrupt enable for system timer failed"));
        goto cleanUp;
    }

	// Initialize timer to maximum period
	UpdatePeriod(g_oalTimer.maxPeriodMSec);

	rc = TRUE;

cleanUp:
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OALTimerInit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerStart
//
//  Start system tick timer
//
VOID OALTimerStart(VOID)
{
	OALTimerWritePosted(&g_pTimerRegs->TCLR, (INREG32(&g_pTimerRegs->TCLR) | DMTIMER_TCLR_ST), DMTIMER_TWPS_TCLR);
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerStop
//
//  Stop system tick timer
//
VOID OALTimerStop(VOID)
{
	OALTimerWritePosted(&g_pTimerRegs->TCLR, (INREG32(&g_pTimerRegs->TCLR) & ~(DMTIMER_TCLR_ST)), DMTIMER_TWPS_TCLR);
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerUpdateRescheduleTime
//
//  This function is called by kernel to set next reschedule time.
//
VOID OALTimerUpdateRescheduleTime( DWORD timeMSec )
{
    UINT32 baseMSec, periodMSec;
    INT32 delta;

    baseMSec = CurMSec;   // Get current system timer counter

    // How far we are from next tick
    delta = (INT32)(g_oalTimerContext.match - INREG32(&g_pTimerRegs->TCRR));

    if( delta < 0 )
    {
        UpdatePeriod(0);
        goto cleanUp;
    }

    // If timer interrupts occurs, or we are within 1 ms of the scheduled
    // interrupt, just return - timer ISR will take care of it.
    if ((baseMSec != CurMSec) || (delta < (INT32)MSEC_TO_TICK(1))) goto cleanUp;

    // Calculate the distance between the new time and the last timer interrupt
    periodMSec = timeMSec - OEMGetTickCount();

    // Trying to set reschedule time prior or equal to CurMSec - this could
    // happen if a thread is on its way to sleep while preempted before
    // getting into the Sleep Queue
    if ((INT32)periodMSec < 0)
	{
        periodMSec = 0;
    }
	else if (periodMSec > g_oalTimer.maxPeriodMSec)
	{
        periodMSec = g_oalTimer.maxPeriodMSec;
    }

    // Now we find new period, so update timer
    UpdatePeriod(periodMSec);

cleanUp:
    return;
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerIntrHandler
//
//  This function implement timer interrupt handler. It is called from common
//  ARM interrupt handler.
//
UINT32 OALTimerIntrHandler()
{
	UINT32 sysIntr = SYSINTR_NOP;
	INT32 period = 0, delta = 0;
	UINT32 count = 0;

	// Clear interrupt
	OUTREG32(&g_pTimerRegs->IRQSTATUS, DMTIMER_TIER_MATCH);
	OUTREG32(&g_pTimerRegs->IRQ_EOI, 0x00);

	// How far from interrupt we are?
    count = INREG32(&g_pTimerRegs->TCRR);
    delta = count - g_oalTimerContext.match;

    // If delta is negative, timer fired for some reason
    // To be safe, reprogram the timer for minimum delta
    if (delta < 0)
    {
        delta = 0;
        goto cleanUp;
    }

#ifdef OAL_ILTIMING
    if (g_oalILT.active)
    {
        g_oalILT.isrTime1 = delta;
    }        
#endif

    // Find how long period was
    period = count - g_oalTimerContext.base;
    g_oalTimer.curCounts += period;    
    g_oalTimerContext.base += period;

    // Calculate actual CurMSec
    CurMSec = (UINT32) TICK_TO_MSEC(g_oalTimer.curCounts);

    // Reschedule?
    delta = dwReschedTime - CurMSec;
    if (delta <= 0)
    {
        sysIntr = SYSINTR_RESCHED;
        delta = g_oalTimer.maxPeriodMSec;
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
	
cleanUp:
    // Set new period
    UpdatePeriod(delta);

#ifdef OAL_ILTIMING
	if (TRUE == g_oalILT.active){
		if ( --g_oalILT.counter == 0 ){
			sysIntr = SYSINTR_TIMING;
			g_oalILT.counter = g_oalILT.counterSet;
			g_oalILT.isrTime2 = OALTimerCountsSinceSysTick();
			g_oalILT.interrupts = 0;
		}
	}
#endif

	return sysIntr;
}

//------------------------------------------------------------------------------
//
UINT32 OALTimerGetCount()
{
    //  Return the timer value
    return INREG32(&g_pTimerRegs->TCRR);
}

//------------------------------------------------------------------------------
//
INT32 OALTimerCountsSinceSysTick()
{
    // Return timer ticks since last interrupt
    return (INT32)(INREG32(&g_pTimerRegs->TCRR) - g_oalTimerContext.base);
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
    UINT64 tickCount = INREG32(&g_pTimerRegs->TCRR);
    //  Returns number of 1 msec ticks
    return (UINT32) TICK_TO_MSEC(tickCount);
}

//------------------------------------------------------------------------------
// supports the 1msec system tick only
//
UINT32 OEMGetTickCount( )
{
    UINT64 baseCounts;
    UINT32 offset;

    // This code adjusts the accuracy of the returned value to the nearest
    // MSec when the system tick exceeds 1 ms. The following code checks if
    // a system timer interrupt occurred between reading the CurMSec value
    // and the call to fetch the HiResTicksSinceSysTick. If so, the value of
    // CurMSec and Offset is re-read, with the certainty that a system timer
    // interrupt will not occur again.
    do
        {
        baseCounts = g_oalTimer.curCounts;
		offset = INREG32(&g_pTimerRegs->TCRR) - g_oalTimerContext.base;
        }
    while (baseCounts != g_oalTimer.curCounts);


   //  Update CurMSec (kernel uses both CurMSec and GetTickCount() at different places) and return msec tick count
    CurMSec = (UINT32)TICK_TO_MSEC(baseCounts + offset);

    return CurMSec;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMIdle
//
VOID OEMIdle(DWORD idleParam)
{    
    UINT idleDelta, newIdleLow;
    UINT tcrrEnter, tcrrExit;
    
    UNREFERENCED_PARAMETER(idleParam);
    
    tcrrEnter = OALTimerGetCount();
    BSPCPUIdle();
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
//
//  Function:  OEMIdleEx
//
VOID OEMIdleEx(LARGE_INTEGER *pliIdleTime)
{
	UINT32 timer_value;
	UINT32 new_timer_value;

	timer_value = OALTimerGetCount();
	BSPCPUIdle();
	new_timer_value = OALTimerGetCount();

	pliIdleTime->QuadPart += (new_timer_value - timer_value);
}

//------------------------------------------------------------------------------

