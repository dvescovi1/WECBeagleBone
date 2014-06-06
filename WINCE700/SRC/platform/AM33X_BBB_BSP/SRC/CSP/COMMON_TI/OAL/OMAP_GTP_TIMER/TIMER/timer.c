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
//  File:  timer.c
//
#include "omap.h"
#include "oal_gptimer.h"
#include "omap_gptimer_regs.h"
#include "omap_prcm_regs.h"
#include "bsp_cfg.h"
#include "soc_cfg.h"
#include "oalex.h"
#include "oal_prcm.h"
#include "omap_led.h"
#include "omap_prof.h"
#include <oal_clock.h>
#include <nkintr.h>

//-----------------------------------------------------------------------------
#define DELTA_TIME              20         // In TICK
#ifndef MAX_INT
#define MAX_INT                 0x7FFFFFFF
#endif

UINT32 timer_posted_pending_bit[23] = 
{
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    TCLR_PENDING_BIT,
    TCRR_PENDING_BIT,
    TLDR_PENDING_BIT,
    TTGR_PENDING_BIT,
    0,
    TMAR_PENDING_BIT,
    0,
    0,
    0,
    TPIR_PENDING_BIT,
    TNIR_PENDING_BIT,
    TCVR_PENDING_BIT,
    TOCR_PENDING_BIT,
    TOWR_PENDING_BIT,
};

//------------------------------------------------------------------------------
//
//  External: g_oalTimerIrq
//
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish system timer interrupt.
//
extern UINT32           g_oalTimerIrq;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMaxIdlePeriod
//
//  maximum idle period during OS Idle in milliseconds
//
extern DWORD dwOEMMaxIdlePeriod;


//------------------------------------------------------------------------------
//
//  Global: g_TimerDevice
//
//  This is the ID of the Timer 
//
OMAP_DEVICE g_TimerDevice;

//------------------------------------------------------------------------------
//
//  Global: g_pTimerRegs
//
//  This is global instance of timer registers
//
OMAP_GPTIMER_REGS*      g_pTimerRegs;

//------------------------------------------------------------------------------
//
//  Global: g_oalTimer
//
//  This is global instance of timer control block.
//
OMAP_TIMER_CONTEXT      g_oalTimerContext;

//-----------------------------------------------------------------------------
//
//  Global:  g_pPrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_PRM   *g_pPrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_pPrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_CM    *g_pPrcmCm;


//-----------------------------------------------------------------------------
//
//  Global:  g_wakeupLatencyConstraintTickCount
//
//  latency time, in 32khz ticks, associated with current latency state
//
INT g_wakeupLatencyConstraintTickCount = MAX_INT;


//------------------------------------------------------------------------------
//
//  Define: MSEC / TICK conversions

//  Conversions for 32kHz clock
#define MSEC_TO_TICK(msec)  (((msec) << 12)/125 + 1)      // msec * 32.768
#define TICK_TO_MSEC(tick)  (((tick) * 1000) >> 15)    // tick / 32.768

//-----------------------------------------------------------------------------
// prototypes
//
extern BOOL IsSmartReflexMonitoringEnabled(UINT channel);
extern BOOL SmartReflex_EnableMonitor(UINT channel, BOOL bEnable);


//------------------------------------------------------------------------------
UINT32
OEMGetTickCount(
    );

VOID
UpdatePeriod(
    UINT32 periodMSec
    );

//------------------------------------------------------------------------------
//
//  Function: OALTimerGetReg
//
//  General function to read Timer registers
//

_inline UINT32 OALTimerGetReg(REG32 *addr)
{
    UINT32 i=0;

    if(g_oalTimerContext.Posted)
        while(INREG32(&g_pTimerRegs->TWPS) & timer_posted_pending_bit[((UINT32)addr&0xff)>>2]) 
		if(i++>TIMER_POSTED_TIMEOUT)
		{
		    RETAILMSG(1, (L"\r\n OALTimerGetReg: wait timeout"));
		    break;
	       }
    
    return INREG32(addr);
    
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerSetReg
//
//  General function to Write Timer registers
//   addr: virtural addr of Timer register

_inline VOID OALTimerSetReg(REG32 *addr, UINT32 val)
{
    UINT32 i=0;

    if(g_oalTimerContext.Posted)
        while(INREG32(&g_pTimerRegs->TWPS) & timer_posted_pending_bit[((UINT32)addr&0xff)>>2]) 
		if(i++>TIMER_POSTED_TIMEOUT) 
		{
		    RETAILMSG(1, (L"\r\n OALTimerSetReg: wait timeout"));
		    break;
	       }
    OUTREG32(addr, val);
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerStart
//
//  General function to start timer 1

VOID OALTimerStart(VOID)
{
    UINT32 i;
    UINT tcrrExit = 0;

    // Enable match interrupt
    OALTimerSetReg(&g_pTimerRegs->TIER, GPTIMER_TIER_MATCH);
	
    OALTimerSetReg(&g_pTimerRegs->TCLR, OALTimerGetReg(&g_pTimerRegs->TCLR) | GPTIMER_TCLR_ST);
    for (i = 0; i < 0x100; i++)
    {
    }
    // get current TCRR value: workaround for errata 1.35
    OALTimerGetReg(&g_pTimerRegs->TCRR);
    tcrrExit = OALTimerGetReg(&g_pTimerRegs->TCRR);
    // ERRATA 1.31 workaround (ES 1.0 only)
    // wait for updated TCRR value
    while (tcrrExit == (OALTimerGetReg(&g_pTimerRegs->TCRR)));	
}
//------------------------------------------------------------------------------
//
//  Function: OALTimerStop
//
//  General function to stop timer 1

VOID OALTimerStop(VOID)
{
    // stop GPTIMER1
    OALTimerSetReg(&g_pTimerRegs->TCLR, OALTimerGetReg(&g_pTimerRegs->TCLR) & ~(GPTIMER_TCLR_ST));
}

//------------------------------------------------------------------------------
//
//  Function:  OALTimerSetCompare
//
__inline VOID OALTimerSetCompare(UINT32 compare)
{

    OALTimerSetReg(&g_pTimerRegs->TMAR, compare);

	// We commented out the following line because it causes issues to the overall performance
	// of the system. As the tick timer is clocked at 32 kHz, the TMAR register takes some 
	// time to update and we loose this time waiting for it. The consequence is that we spend
	// more than 5% of the time in this loop where the CPU should actually be idle. Not waiting
	// should not have any consequence as we never actually read its value.
	//
    //while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TMAR) != 0);

    // make sure we don't set next timer interrupt to the past
    //
    if (compare < OALTimerGetReg(&g_pTimerRegs->TCRR)) UpdatePeriod(1);
}


//------------------------------------------------------------------------------
//
//  Function:  UpdatePeriod
//
VOID
UpdatePeriod(
    UINT32 periodMSec
    )
{
    UINT32 period, match;
    INT32 delta;
    UINT64 offsetMSec = periodMSec;
    UINT64 tickCount = OALGetTickCount();
    //INT nDelay;

    // Calculate count difference
    period = (UINT32)MSEC_TO_TICK(offsetMSec);

    /* commenting out the code using "nDelay" since it is limiting the period to 20 TICKS */
    //nDelay = min(period, DELTA_TIME);
	
    // This is compare value
    //match = ((UINT32)MSEC_TO_TICK(tickCount)) + nDelay;
    match = ((UINT32)MSEC_TO_TICK(tickCount)) + period;

    delta = (INT32)(OALTimerGetCount()+ g_oalTimerContext.margin - match);

    // If we are behind, issue interrupt as soon as possible
    if (delta > 0)
    {
        match += MSEC_TO_TICK(1);
    }

    // Save off match value
    g_oalTimerContext.match = match;

    // Set timer match value
    OALTimerSetCompare(match);
}


//------------------------------------------------------------------------------
//
//  Function:     OEMIdle
//
//  This function is called by the kernel when there are no threads ready to
//  run. The CPU should be put into a reduced power mode if possible and halted.
//  It is important to be able to resume execution quickly upon receiving an
//  interrupt.
//
//  Interrupts are disabled when OEMIdle is called and when it returns.
//
//  This implementation doesn't change system tick. It is intend to be used
//  with variable tick implementation. However it should work with fixed
//  variable tick implementation also (with lower efficiency because maximal
//  idle time is 1 ms).
//
//  WARNING: This function is called from deep within the kernel, it cannot make
//  any system calls, use any critical sections, or debug messages.
//
VOID
OEMIdle(
    DWORD idleParam
    )
{
    static UINT _max = 0;
    static UINT _count = 0;
    static DWORD lastTCRRExit = 0;

    INT delta;
    UINT tcrrTemp;
    UINT tcrrEnter, tcrrExit;
    UINT idleDelta, newIdleLow;
    INT wakeupDelay;
    INT maxDelay;
    DWORD latencyState;    
    DWORD curridleAdjust=0;

    UNREFERENCED_PARAMETER(idleParam);
    tcrrEnter = OALTimerGetReg(&g_pTimerRegs->TCRR);

    PrcmInitializePrevPowerState();

    // How far are we from next timer interrupt
    // If we are really near to timer interrupt do nothing...
    latencyState = OALWakeupLatency_GetCurrentState();
    delta = g_oalTimerContext.match - tcrrEnter;
    if (delta < (INT32)g_oalTimerContext.margin) 
    {
        curridleAdjust = delta>0?delta:0;
        goto cleanUp;
    }

    // get latency time...
    //
    // check if current latency is greater than current requirements
    maxDelay = min(delta, g_wakeupLatencyConstraintTickCount);
    wakeupDelay = OALWakeupLatency_GetDelayInTicks(latencyState);
    if (maxDelay < wakeupDelay)
    {
        // check if current state meets timing constraint
        latencyState = OALWakeupLatency_FindStateByMaxDelayInTicks(maxDelay);
        wakeupDelay = OALWakeupLatency_GetDelayInTicks(latencyState);
    }

    // check one last time to make sure we aren't going to sleep longer than
    if (maxDelay >= wakeupDelay)
    {
        //  Indicate in idle
        OALLED(LED_IDX_IDLE, 1);
        
        if (OALWakeupLatency_IsChipOff(latencyState))
        {
            if (!OALContextSave())
            {
                // Context Save Failed
                goto cleanUp;
            }
        }
        // account for wakeup latency
        OALWakeupLatency_PushState(latencyState);
        g_oalTimerContext.match -= wakeupDelay;
        OALTimerSetCompare(g_oalTimerContext.match);
    }
    else
    {
        curridleAdjust = maxDelay>0?maxDelay:0;
        goto cleanUp;
    }

    // Move SoC/CPU to idle mode    
    fnOALCPUIdle(g_pCPUInfo);
    
    // restore latency state
    OALWakeupLatency_PopState();
    
    PrcmCapturePrevPowerState();

    // get current TCRR value: workaround for errata 1.35
    OALTimerGetReg(&g_pTimerRegs->TCRR);
    tcrrExit = OALTimerGetReg(&g_pTimerRegs->TCRR);

    PrcmProcessPostMpuWakeup();

    // ERRATA 1.31 workaround
    do
    {
        tcrrTemp = OALTimerGetReg(&g_pTimerRegs->TCRR);
    }
    while (tcrrTemp == tcrrExit);

    PrcmProfilePrevPowerState(tcrrTemp, wakeupDelay);

cleanUp:    
    // Update idle counter
    tcrrExit=OALTimerGetReg(&g_pTimerRegs->TCRR) ;

    // already accounted for in last OEMIdle
    if (tcrrExit <= lastTCRRExit)
    {
        idleDelta = 0;        
    }
    // partially accounted for in last OEMIdle
    else if ((tcrrEnter <= lastTCRRExit) && (tcrrExit > lastTCRRExit))
    {    
        idleDelta = curridleAdjust ? (curridleAdjust - (lastTCRRExit - tcrrEnter) ) : (tcrrExit - lastTCRRExit);
        lastTCRRExit = curridleAdjust ? (curridleAdjust + tcrrEnter) : tcrrExit;
    }
    // un-accounted for in last OEMIdle
    else
    {
        idleDelta = curridleAdjust ? curridleAdjust : (tcrrExit - tcrrEnter);
        lastTCRRExit = curridleAdjust ? (curridleAdjust + tcrrEnter) : tcrrExit;        
    }
    //if (curridleAdjust > 0) OALMSG(1,(L"idleDelta=%d curridleAdjust=%d tcrrExit=%d tcrrEnter=%d lastTCRRExit=%d\r\n",idleDelta,curridleAdjust,tcrrExit,tcrrEnter,lastTCRRExit));
    
    newIdleLow = curridlelow + idleDelta;
    if (newIdleLow < curridlelow) 
	    curridlehigh++;
    curridlelow = newIdleLow;
    
    
    OALLED(LED_IDX_IDLE, 0);
    return;    
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerInit
//
//  General purpose timer 1 is used for system tick. It supports
//  count/compare mode on 32kHz clock
//
BOOL
OALTimerInit(
    UINT32 sysTickMSec,
    UINT32 countsPerMSec,
    UINT32 countsMargin
    )
{
    BOOL rc = FALSE;
    UINT srcClock;
    UINT32 sysIntr;

	UNREFERENCED_PARAMETER(sysTickMSec);
	UNREFERENCED_PARAMETER(countsPerMSec);
	UNREFERENCED_PARAMETER(countsMargin);

    OALMSG(OAL_TIMER&&OAL_FUNC, (
        L"+OALTimerInit(%d, %d, %d)\r\n", sysTickMSec, countsPerMSec,
        countsMargin
        ));

    //  Initialize timer state information
    g_oalTimerContext.maxPeriodMSec = dwOEMMaxIdlePeriod;   // Maximum period the timer will interrupt on, in mSec
    g_oalTimerContext.margin =        DELTA_TIME;           // Time needed to reprogram the timer interrupt
    g_oalTimerContext.curCounts =     0;
    g_oalTimerContext.base =          0;
    g_oalTimerContext.match =         0xFFFFFFFF;
    g_oalTimerContext.Posted =       0;

    // Set idle conversion constant and counters
    idleconv     = MSEC_TO_TICK(1);
    curridlehigh = 0;
    curridlelow  = 0;

    // Use variable system tick
    pOEMUpdateRescheduleTime = OALTimerUpdateRescheduleTime;

// Get virtual addresses for hardware
    g_TimerDevice = BSPGetSysTimerDevice();
    g_pTimerRegs = OALPAtoUA(GetAddressByDevice(g_TimerDevice));
	
	// Select 32K frequency source clock
    srcClock = BSPGetSysTimer32KClock();
	//PrcmDeviceSetSourceClocks(g_TimerDevice,1,&srcClock);

    // enable gptimer
    EnableDeviceClocks(g_TimerDevice, TRUE);


    // stop timer
    OALTimerSetReg(&g_pTimerRegs->TCLR, 0);

    // Soft reset GPTIMER
    OALTimerSetReg(&g_pTimerRegs->TIOCP, SYSCONFIG_SOFTRESET);

    // While until done
    while ((OALTimerGetReg(&g_pTimerRegs->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);

    // Set smart idle
    OALTimerSetReg(
        &g_pTimerRegs->TIOCP,
            SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP|
            SYSCONFIG_AUTOIDLE
        );

    // Enable posted mode
    OALTimerSetReg(&g_pTimerRegs->TSICR, GPTIMER_TSICR_POSTED);
    g_oalTimerContext.Posted =       1;

    // Set match register to avoid unwanted interrupt
    OALTimerSetReg(&g_pTimerRegs->TMAR, 0xFFFFFFFF);

    // Enable match interrupt
    OALTimerSetReg(&g_pTimerRegs->TIER, GPTIMER_TIER_MATCH);

    // Enable match wakeup
    OALTimerSetReg(&g_pTimerRegs->TWER, GPTIMER_TWER_MATCH);

    // Enable timer in auto-reload and compare mode
    OALTimerSetReg(&g_pTimerRegs->TCLR, GPTIMER_TCLR_CE|GPTIMER_TCLR_AR|GPTIMER_TCLR_ST);

    // Wait until write is done
    //while ((INREG32(&g_pTimerRegs->TWPS) & GPTIMER_TWPS_TCLR) != 0);

    // Set global variable to tell interrupt module about timer used
    g_oalTimerIrq = GetIrqByDevice(g_TimerDevice,NULL);

    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalTimerIrq, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALTimerInit: Interrupt enable for system timer failed"
            ));
        goto cleanUp;
        }

    // Initialize timer to maximum period
    UpdatePeriod(g_oalTimerContext.maxPeriodMSec);

    // Done
    rc = TRUE;

cleanUp:
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OALTimerInit(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OALTimerUpdateRescheduleTime
//
//  This function is called by kernel to set next reschedule time.
//
VOID
OALTimerUpdateRescheduleTime(
    DWORD timeMSec
    )
{
    UINT32 baseMSec, periodMSec;
    INT32 delta;

    // Get current system timer counter
    baseMSec = CurMSec;

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
    if ((INT32)periodMSec < 0)
        {
        periodMSec = 0;
        }
    else if (periodMSec > g_oalTimerContext.maxPeriodMSec)
        {
        periodMSec = g_oalTimerContext.maxPeriodMSec;
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
    UINT32 count;
    INT32 period, delta;
    UINT32 sysIntr = SYSINTR_NOP;



    // allow bsp to process timer interrupt first
    sysIntr = OALTickTimerIntr();
    if (sysIntr != SYSINTR_NOP) return sysIntr;

    // Clear interrupt
    OALTimerSetReg(&g_pTimerRegs->TISR, GPTIMER_TIER_MATCH);

    // How far from interrupt we are?
    count = OALTimerGetCount();
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
    g_oalTimerContext.curCounts += period;    
    g_oalTimerContext.base += period;

    // Calculate actual CurMSec
    CurMSec = (UINT32) TICK_TO_MSEC(g_oalTimerContext.curCounts);

    OALLED(LED_IDX_TIMER, CurMSec >> 10);

    // Reschedule?
    delta = dwReschedTime - CurMSec;
    if (delta <= 0)
        {
        sysIntr = SYSINTR_RESCHED;
        delta = g_oalTimerContext.maxPeriodMSec;
        }

cleanUp:
    // Set new period
    UpdatePeriod(delta);

#ifdef OAL_ILTIMING
    if (g_oalILT.active) {
        if (--g_oalILT.counter == 0) {
            sysIntr = SYSINTR_TIMING;
            g_oalILT.counter = g_oalILT.counterSet;
            g_oalILT.isrTime2 = 0;
        }
    }
#endif

    return sysIntr;
}


//------------------------------------------------------------------------------
//
//  Function:  OALTimerGetCount
//
UINT32 OALTimerGetCount()
{
    //  Return the timer value
    return OALTimerGetReg(&g_pTimerRegs->TCRR);
}

//------------------------------------------------------------------------------
//
//  Function:  OALTimerCountsSinceSysTick
//
INT32 OALTimerCountsSinceSysTick()
{
    // Return timer ticks since last interrupt
    return (INT32)(OALTimerGetReg(&g_pTimerRegs->TCRR) - g_oalTimerContext.base);
}

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount
//
//  This function returns number of 1 ms ticks which elapsed since system boot
//  or reset (absolute value isn't important). The counter can overflow but
//  overflow period should not be shorter then approx 30 seconds. Function
//  is used in  system boot so it must work before interrupt subsystem
//  is active.
//
UINT32
OALGetTickCount(
    )
{
    UINT64 tickCount = OALTimerGetReg(&g_pTimerRegs->TCRR);
    //  Returns number of 1 msec ticks
    return (UINT32) TICK_TO_MSEC(tickCount);
}

//------------------------------------------------------------------------------
//
//  Function: OEMGetTickCount
//
//  This returns the number of milliseconds that have elapsed since Windows
//  CE was started. If the system timer period is 1ms the function simply
//  returns the value of CurMSec. If the system timer period is greater then
//  1 ms, the HiRes offset is added to the value of CurMSec.
//
UINT32
OEMGetTickCount(
    )
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
        baseCounts = g_oalTimerContext.curCounts;
        offset = OALTimerGetCount() - g_oalTimerContext.base;
        }
    while (baseCounts != g_oalTimerContext.curCounts);


   //  Update CurMSec (kernel uses both CurMSec and GetTickCount() at different places) and return msec tick count
    CurMSec = (UINT32)TICK_TO_MSEC(baseCounts + offset);

    return CurMSec;

}



//------------------------------------------------------------------------------

