// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

#include "omap.h"
#include "omap_wdog_regs.h"
#include "oal_clock.h"
#include "bsp_cfg.h"



//------------------------------------------------------------------------------
//
//  Global: g_pWatchogTimerRegs
//
//  This is global instance of watchdog timer registers
//
static OMAP_WDOG_REGS*      g_pWatchogTimerRegs;
static OMAP_DEVICE          g_WatchdogDevice = OMAP_DEVICE_NONE;
static DWORD                g_dwWatchdogPeriod;


static void WatchdogRefresh(void)
{
    static BOOL fWatchdogInit = FALSE;
    if (fWatchdogInit == FALSE)
    {
        // Initialize watchdog hardware
        g_WatchdogDevice = BPSGetWatchdogDevice();
        g_pWatchogTimerRegs = OALPAtoUA(GetAddressByDevice(g_WatchdogDevice));
        
        // Make sure interface/functional clocks are running
        EnableDeviceClocks(g_WatchdogDevice, TRUE);

#if 0
        SetDeviceIdleMode(g_WatchdogDevice,OMAP_SMART_IDLE_FLAG);
#endif

        // Ensure the timer is stopped
        // Note - writes are posted; must ensure they have completed before 
        // writing to the same register again.
        OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_DISABLE_SEQ1);
        while( INREG32(&g_pWatchogTimerRegs->WWPS) );
        OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_DISABLE_SEQ2);
        while( INREG32(&g_pWatchogTimerRegs->WWPS) );
        

        // Set prescaler, so that the watcdog counter is incremented around every 1 ms (32768 Hz / 32 => 1024 Hz)
        OUTREG32(&g_pWatchogTimerRegs->WCLR, WDOG_WCLR_PRESCALE(5) | WDOG_WCLR_PRES_ENABLE);    
        
        // Set reload value in both the reload register and base counter register
        OUTREG32(&g_pWatchogTimerRegs->WLDR, (DWORD) (0-g_dwWatchdogPeriod));
        OUTREG32(&g_pWatchogTimerRegs->WCRR, (DWORD) (0-g_dwWatchdogPeriod));
        while( INREG32(&g_pWatchogTimerRegs->WWPS) );
        
        // Refresh the watchdog timer before starting it
        while( INREG32(&g_pWatchogTimerRegs->WWPS) );
        OUTREG32(&g_pWatchogTimerRegs->WTGR, INREG32(&g_pWatchogTimerRegs->WTGR) + 1);

        // Start the watchdog timer
        OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_ENABLE_SEQ1);
        while( INREG32(&g_pWatchogTimerRegs->WWPS) );
        OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_ENABLE_SEQ2);
        
        fWatchdogInit = TRUE;
    }
    
    // Refresh the watchdog timer
    while( INREG32(&g_pWatchogTimerRegs->WWPS) );
    OUTREG32(&g_pWatchogTimerRegs->WTGR, INREG32(&g_pWatchogTimerRegs->WTGR) + 1);    
}

void OALWatchdogInit(DWORD dwPeriod, DWORD dwThreadPriority)
{
    g_dwWatchdogPeriod = dwPeriod;
    g_pOemGlobal->pfnRefreshWatchDog = WatchdogRefresh;
    g_pOemGlobal->dwWatchDogPeriod = g_dwWatchdogPeriod / 2; //set the refresh period to half the watchdog period
    g_pOemGlobal->dwWatchDogThreadPriority = dwThreadPriority;
}

// Called from OEMPowerOff - no system calls, critical sections, OALMSG, etc., are allowed
//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
void OALWatchdogEnable(BOOL bEnable)
{
    if (g_WatchdogDevice != OMAP_DEVICE_NONE)
    {
        if (bEnable == TRUE)
        {
            // Enable clock
            EnableDeviceClocks(g_WatchdogDevice, TRUE);

            // Refresh the watchdog timer
            while( INREG32(&g_pWatchogTimerRegs->WWPS) );
            OUTREG32(&g_pWatchogTimerRegs->WTGR, INREG32(&g_pWatchogTimerRegs->WTGR) + 1);

            // Start Watchdog
            OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_ENABLE_SEQ1);
            while( INREG32(&g_pWatchogTimerRegs->WWPS) );
            OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_ENABLE_SEQ2);
        }
        else
        {
            // Ensure the timer is stopped
            OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_DISABLE_SEQ1);
            while( INREG32(&g_pWatchogTimerRegs->WWPS) );
            OUTREG32(&g_pWatchogTimerRegs->WSPR, WDOG_DISABLE_SEQ2);
            while( INREG32(&g_pWatchogTimerRegs->WWPS) );

            // Disable clock
            EnableDeviceClocks(g_WatchdogDevice, FALSE);
        }
    }
}

