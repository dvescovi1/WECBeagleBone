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
#include "omap.h"
#include "omap_dss_regs.h"
#include "omap_prcm_regs.h"
#include "dssai.h"
#include "lcd.h"
#include "ceddkex.h"
#include "_debug.h"
#include <oal_clock.h>
#include <sdk_padcfg.h>

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//
//  Defines
//

#define DSS_REGS_SIZE           1024
#define GAMMA_BUFFER_SIZE       1024


#define FIFO_HIGHTHRESHOLD_NORMAL   (1024-1)
#define FIFO_HIGHTHRESHOLD_MERGED   (3072-1)

#define FIFO_LOWTHRESHOLD_NORMAL(x) (FIFO_HIGHTHRESHOLD_NORMAL - x * FIFO_NUM_BURSTS)
#define FIFO_LOWTHRESHOLD_MERGED(x) (FIFO_HIGHTHRESHOLD_MERGED - 3 * x * FIFO_NUM_BURSTS)

#define FIFO_NUM_BURSTS             (8)

#define FIFO_BURSTSIZE_4x32         (16)
#define FIFO_BURSTSIZE_8x32         (32)
#define FIFO_BURSTSIZE_16x32        (64)

#define HDMI_CLK                    148500000
#define DSI_PLL_FREQINT               2000000   // Fint = 2MHz
#define DSI_PLL_FREQSELVAL            0x07      // Fint select = 1.75MHz to 2.1MHz


#define CEIL_MULT(x, mult)   (x) = ( ( (x)/mult )+1) * mult;
#define FLOOR_MULT(x, mult)  (x) = ((x)/mult) * mult;

//
//  Structs
//

typedef struct OMAPPipelineConfig
{
    BOOL                    bEnabled;           // Enabled flag
    OMAP_DSS_DESTINATION    eDestination;       // Pipeline destination
    OMAPSurface*            pSurface;           // Surface to output
    OMAP_DSS_ROTATION       eRotation;          // Rotation angle of pipeline output
    BOOL                    bMirror;            // Mirror pipeline (left/right)
    DWORD                   dwDestWidth;        // Width of output
    DWORD                   dwDestHeight;       // Height of output
    OMAPSurface*            pOldSurface;        // Actual Surface to output in case of flipping
}
OMAPPipelineConfig;

typedef struct OMAPPipelineScaling
{
    DWORD                   dwHorzScaling;      // Horizontal surface decimation factor for pipeline
    DWORD                   dwVertScaling;      // Vertical surface decimation factor for pipeline
    DWORD                   dwInterlace;        // Interlace offset
}
OMAPPipelineScaling;


//
//  Globals
//

OMAPPipelineConfig  g_rgPipelineMapping[NUM_DSS_PIPELINES] =
{
    { FALSE, OMAP_DSS_DESTINATION_LCD, NULL, OMAP_DSS_ROTATION_0, FALSE, 0, 0, NULL },    // GFX
    { FALSE, OMAP_DSS_DESTINATION_LCD, NULL, OMAP_DSS_ROTATION_0, FALSE, 0, 0, NULL },    // VIDEO1
    { FALSE, OMAP_DSS_DESTINATION_LCD, NULL, OMAP_DSS_ROTATION_0, FALSE, 0, 0, NULL },    // VIDEO2
};

OMAPPipelineScaling g_rgPipelineScaling[NUM_DSS_PIPELINES] =
{
    { 1, 1, 0 },    // GFX
    { 1, 1, 0 },    // VIDEO1
    { 1, 1, 0 },    // VIDEO2
};


DWORD   g_dwDestinationRefCnt[NUM_DSS_DESTINATION] =
{
    0,      // LCD
    0       // TV Out
};





//------------------------------------------------------------------------------
OMAPDisplayController::OMAPDisplayController()
{
    m_pDSSRegs = NULL;
    m_pDispRegs = NULL;
    m_pVencRegs = NULL;
    
    m_dwPowerLevel = D4;
    
    m_bTVEnable = FALSE;
    m_bHDMIEnable = FALSE;

    if (LcdPdd_DVI_Enabled())
        m_bDVIEnable = TRUE;
    else
        m_bDVIEnable = FALSE;

    m_bGammaEnable = TRUE;
   	m_dwEnableWaitForVerticalBlank = FALSE;
    m_bDssIspRszEnabled = FALSE;
    m_lastVsyncIRQStatus = 0;
    
    m_dwContrastLevel = DEFAULT_CONTRAST_LEVEL;
    m_pGammaBufVirt = NULL;
    
    m_bDssIntThreadExit = FALSE;
    m_hDssIntEvent = NULL;
    m_hDssIntThread = NULL;
    m_dwDssSysIntr = 0;

    m_dwVsyncPeriod = 0;
    m_hVsyncEvent = NULL;
    m_hVsyncEventSGX = NULL;
	m_hVsyncEventWSEGL = NULL;

    m_hScanLineEvent = NULL;
    
    m_pColorSpaceCoeffs = g_dwColorSpaceCoeff_BT601_Limited;
    
    m_bLPREnable = FALSE;

    m_hSmartReflexPolicyAdapter = NULL;

    SOCGetDSSInfo(&m_dssinfo);
}

//------------------------------------------------------------------------------
OMAPDisplayController::~OMAPDisplayController()
{
    UninitInterrupts();

    //  Release all clocks
    EnableDeviceClocks( m_dssinfo.DSSDevice, FALSE );
    EnableDeviceClocks( m_dssinfo.TVEncoderDevice, FALSE );         

    //  Delete power lock critical section
    DeleteCriticalSection( &m_csPowerLock );

    // Close SmartReflex policy adapter
    if (m_hSmartReflexPolicyAdapter != NULL)
        PmxClosePolicy(m_hSmartReflexPolicyAdapter);
    
    //  Free allocated memory
    if( m_pGammaBufVirt != NULL )
        FreePhysMem( m_pGammaBufVirt );
        
    //  Unmap registers
    if (m_pDSSRegs != NULL) 
        MmUnmapIoSpace((VOID*)m_pDSSRegs, DSS_REGS_SIZE);

    if (m_pDispRegs != NULL) 
        MmUnmapIoSpace((VOID*)m_pDispRegs, DSS_REGS_SIZE);

    if (m_pVencRegs != NULL) 
        MmUnmapIoSpace((VOID*)m_pVencRegs, DSS_REGS_SIZE);
        
    if (m_pDSIRegs != NULL)
        MmUnmapIoSpace((VOID*)m_pDSIRegs,  sizeof(OMAP_DSI_REGS));

    if (m_pDSIPLLRegs != NULL)
        MmUnmapIoSpace((VOID*)m_pDSIPLLRegs, sizeof(OMAP_DSI_PLL_REGS));

}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitController(BOOL bEnableGammaCorr, BOOL bEnableWaitForVerticalBlank, BOOL bEnableISPResizer)
{
    BOOL    bResult = FALSE;
    PHYSICAL_ADDRESS pa;
    DWORD size;

    //
    //  Map display controller registers
    //
    pa.QuadPart = m_dssinfo.DSS1_REGS_PA;
    size = DSS_REGS_SIZE;
    m_pDSSRegs = (OMAP_DSS_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_pDSSRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
             L"Failed map DSS control registers\r\n"
            ));
        goto cleanUp;
        }

    pa.QuadPart = m_dssinfo.DISC1_REGS_PA;
    size = DSS_REGS_SIZE;
    m_pDispRegs = (OMAP_DISPC_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_pDispRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
             L"Failed map DISPC control registers\r\n"
            ));
        goto cleanUp;
        }

    pa.QuadPart = m_dssinfo.VENC1_REGS_PA;
    size = DSS_REGS_SIZE;
    m_pVencRegs = (OMAP_VENC_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_pVencRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
             L"Failed map VENC control registers\r\n"
            ));
        goto cleanUp;
        }

    // Disable gamma correction based on registry
    if(!bEnableGammaCorr)
        m_bGammaEnable = FALSE;

    // Enable VSYNC code based on registry
    if (bEnableWaitForVerticalBlank)
    	m_dwEnableWaitForVerticalBlank = TRUE;

    //enable ISP resizer based on registry
    if (bEnableISPResizer)
        m_bDssIspRszEnabled = TRUE;
    

    //  Allocate physical memory for gamma table buffer
    m_pGammaBufVirt = (DWORD*)AllocPhysMem(NUM_GAMMA_VALS*sizeof(DWORD), PAGE_READWRITE | PAGE_NOCACHE, 0, 0,&m_dwGammaBufPhys);
    if( m_pGammaBufVirt == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: COMAPDisplayController::InitController: "
            L"Failed allocate Gamma phys buffer\r\n"
            ));
        goto cleanUp;
        }

    // map DSI regs
    pa.QuadPart = m_dssinfo.DSI_REGS_PA;
    size = sizeof(OMAP_DSI_REGS);
    m_pDSIRegs = (OMAP_DSI_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_pDSIRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
                 L"Failed map DSI control registers\r\n"
                ));
        goto cleanUp;
        }

    // map DSI Pll regs
    pa.QuadPart = m_dssinfo.DSI_PLL_REGS_PA;
    size = sizeof(OMAP_DSI_PLL_REGS);
    m_pDSIPLLRegs = (OMAP_DSI_PLL_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_pDSIPLLRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
                     L"Failed map DSIPLL control registers\r\n"
                    ));
        goto cleanUp;
        }

    //  Initialize power lock critical section
    InitializeCriticalSection( &m_csPowerLock );

    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );

    // Configure the DssFclk source and value
    m_eDssFclkSource = OMAP_DSS_FCLK_DSS2ALWON;
    m_eDssFclkValue  = OMAP_DSS_FCLKVALUE_NORMAL;

	// Request Pads for LCD
	if (!RequestDevicePads(m_dssinfo.DSSDevice))
	{
        ERRORMSG(TRUE, (L"ERROR: OMAPDisplayController::InitController: "
                     L"Failed to request pads\r\n"
                    ));
        goto cleanUp;
	}

    //  Reset the DSS controller
    ResetDSS();

    //  Enable controller power
    SetPowerLevel( D0 );

    //  Configure the clock source
    //
    //  DSS1_ALWON = 172MHz
    //  DSI1_PLL   = 148.5MHz
    //
    OUTREG32( &m_pDSSRegs->DSS_CONTROL, 
                DSS_CONTROL_DISPC_CLK_SWITCH_DSS1_ALWON |
                DSS_CONTROL_DSI_CLK_SWITCH_DSS1_ALWON
                );

                
    //  Configure interconnect parameters
    OUTREG32( &m_pDSSRegs->DSS_SYSCONFIG, DISPC_SYSCONFIG_AUTOIDLE );
    OUTREG32( &m_pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_AUTOIDLE|SYSCONFIG_NOIDLE|SYSCONFIG_NOSTANDBY );


    //  Enable DSS fault notification interrupts
    g_rgDisplaySaveRestore.DISPC_IRQENABLE = DISPC_IRQENABLE_OCPERROR|
                                             DISPC_IRQENABLE_SYNCLOST;
    OUTREG32( &m_pDispRegs->DISPC_IRQENABLE , g_rgDisplaySaveRestore.DISPC_IRQENABLE);

    //  Unlock access to power level
    LeaveCriticalSection( &m_csPowerLock );
        
    // Open a handle to SmartReflex policy adapter
    m_hSmartReflexPolicyAdapter = PmxOpenPolicy(SMARTREFLEX_POLICY_NAME);
        
    //  Success
    bResult = TRUE;

cleanUp:    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitLCD()
{
    BOOL    bResult;

    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );

    //  Enable controller power
    SetPowerLevel( D0 );

    //  Initialize the LCD by calling PDD
    bResult = LcdPdd_LCD_Initialize(
                    m_pDSSRegs,
                    m_pDispRegs,
                    NULL,
                    m_pVencRegs);
    
    //  Get LCD parameters
    LcdPdd_LCD_GetMode(
                    (DWORD*) &m_eLcdPixelFormat,
                    &m_dwLcdWidth,
                    &m_dwLcdHeight,
                    &m_dwPixelClock
                    );

    //  Set up gamma correction to support contrast control
    SetContrastLevel( m_dwContrastLevel );
    
    // Enable/Disable Gamma correction
    if(m_bGammaEnable)
    {
        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_PALETTEGAMMATABLE );
        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_LOADMODE(0) );
    }
    else
    {
        CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_PALETTEGAMMATABLE );
        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_LOADMODE(2) );
    }

    // Load Gamma Table
    OUTREG32( &m_pDispRegs->DISPC_GFX_TABLE_BA, m_dwGammaBufPhys);
    

    //  Set default global alpha values to be opaque
    OUTREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA, DISPC_GLOBAL_ALPHA_GFX(0xFF)|DISPC_GLOBAL_ALPHA_VID2(0xFF) );
    
    // Could calculate actual period...
    m_dwVsyncPeriod = 1000/60 + 2;//Add delta 2 ms since frameRate is not exactly 60fps

    // Initialize the DSI PLL
    InitDsiPll();
    // Configure the DSI PLL with the FCLK value reqd
    ConfigureDsiPll( OMAP_DSS_FCLKVALUE_NORMAL );
    //  Unlock access to power level
    LeaveCriticalSection( &m_csPowerLock );
    //  Return result
    return bResult;
}
//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitInterrupts(DWORD irq, DWORD istPriority)
{
    BOOL rc = FALSE;

    // get system interrupt for irq
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &irq,
        sizeof(irq), &m_dwDssSysIntr, sizeof(m_dwDssSysIntr),
        NULL))
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed map DSS interrupt(irq=%d)\r\n"), __FUNCTION__,irq));
        goto cleanUp;
    }

    // create thread event handle
    m_hDssIntEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hDssIntEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to create Interrupt event object!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    // register event handle with system interrupt
    if (!InterruptInitialize(m_dwDssSysIntr, m_hDssIntEvent, NULL, 0))
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to initialize system interrupt!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    //Create specific interrupt events
    m_hVsyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hVsyncEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to create Vsync interrupt event object!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    //Create specific interrupt events (Multiple Threads)
    m_hVsyncEventSGX = CreateEvent(NULL, FALSE, FALSE, VSYNC_EVENT_NAME);
    if (m_hVsyncEventSGX == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to create Vsync interrupt event object for SGX!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

	m_hVsyncEventWSEGL = CreateEvent(NULL, FALSE, FALSE, VSYNC_EVENT_WSEGL);
    if (m_hVsyncEventSGX == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to create Vsync interrupt event object for SGX!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    //Create specific interrupt events
    m_hScanLineEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hScanLineEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed to create ScanLine interrupt event object!\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    // spawn thread
    m_hDssIntThread = CreateThread(NULL, 0, DssInterruptHandler, this, 0, NULL);
    if (!m_hDssIntThread)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: Failed to create interrupt thread\r\n"), __FUNCTION__));
        goto cleanUp;
    }

    // set thread priority
    CeSetThreadPriority(m_hDssIntThread, istPriority);

    rc = TRUE;

cleanUp:
    if (rc == FALSE) UninitInterrupts();
    return rc;
}

//------------------------------------------------------------------------------
void
OMAPDisplayController::UninitInterrupts()
{
    // unregister system interrupt
    if (m_dwDssSysIntr != 0)
    {
        InterruptDisable(m_dwDssSysIntr);
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR, &m_dwDssSysIntr,
            sizeof(m_dwDssSysIntr), NULL, 0, NULL
            );

        // reinit
        m_dwDssSysIntr = 0;
    }

    // stop thread
    if (m_hDssIntEvent != NULL)
    {
        if (m_hDssIntThread != NULL)
        {
            // Signal stop to thread
            m_bDssIntThreadExit = TRUE;

            // Set event to wake it
            SetEvent(m_hDssIntEvent);

            // Wait until thread exits
            WaitForSingleObject(m_hDssIntThread, INFINITE);

            // Close handle
            CloseHandle(m_hDssIntThread);

            // reinit
            m_hDssIntThread = NULL;
        }

        // close event handle
        CloseHandle(m_hDssIntEvent);
        m_hDssIntEvent = NULL;
    }

    if(m_hVsyncEvent != NULL)
    {
        CloseHandle(m_hVsyncEvent);
        m_hVsyncEvent = NULL;
    }

    if(m_hScanLineEvent != NULL)
    {
        CloseHandle(m_hScanLineEvent);
        m_hScanLineEvent = NULL;
    }

}

//------------------------------------------------------------------------------
DWORD
OMAPDisplayController::DssInterruptHandler( void* pData )
{
    OMAPDisplayController *pController = (OMAPDisplayController *)pData;
    DWORD sysIntr = pController->m_dwDssSysIntr;
    for(;;)
    {
        __try
        {
            // wait for interrupt
            WaitForSingleObject(pController->m_hDssIntEvent, INFINITE);
            if (pController->m_bDssIntThreadExit == TRUE) break;

            // process interrupt
            pController->DssProcessInterrupt();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_ERROR,
                (TEXT("%S: exception in interrupt handler!\r\n"), __FUNCTION__));
        }
        // reset for next interrupt
        ::InterruptDone(sysIntr);
    }
    return 1;
}
//------------------------------------------------------------------------------
VOID
OMAPDisplayController::DssProcessInterrupt()
{
    DWORD irqStatus = 0;
    DWORD dwTimeout = DISPLAY_TIMEOUT;
    OMAP_DSS_PIPELINE pipelineID = OMAP_DSS_PIPELINE_GFX;
    BOOL  lcdVsync = FALSE;
    //Alwasy set to true, in case tv-out is disabled.
    BOOL  tvVsync = TRUE;
    DWORD irqEnableMask = 0;

    // Access the regs
    if( AccessRegs() == FALSE )
    {
        // failure will cause lockup because the interrupt will still be pending...
        DEBUGMSG(ZONE_ERROR,(L"AccessRegs failed in DssProcessInterrupt\r\n"));
        ASSERT(0);
        goto cleanUp;
    }
    irqEnableMask = INREG32(&m_pDispRegs->DISPC_IRQENABLE);
    irqStatus = (INREG32(&m_pDispRegs->DISPC_IRQSTATUS) & irqEnableMask);
    /* Check if we should worry about Vsync or not */
    m_lastVsyncIRQStatus &= irqEnableMask;
    
    //  Enabling Interrupt for SYNCLOSTDIGITAL causes tvout to not recover 
    //  from a suspend/resume cycle. Hence disabled. SYNCLOSTDIGITAL is not
    //  causing any issue with functionality of tvout.
    
/*    if (irqStatus & (DISPC_IRQSTATUS_SYNCLOST|
                     DISPC_IRQSTATUS_SYNCLOSTDIGITAL))*/
    if (irqStatus & (DISPC_IRQSTATUS_SYNCLOST))                         
    {
        DEBUGMSG(ZONE_ERROR,(L"SYNCLOST DSSIRQ:%x\r\n",irqStatus));

        // SYNCLOST recovery process

        // clear existing Frame done interrupt
        OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS,DISPC_IRQSTATUS_FRAMEDONE );

        // disable scanning
        CLRREG32( &m_pDispRegs->DISPC_CONTROL,
                  DISPC_CONTROL_DIGITALENABLE |
                  DISPC_CONTROL_LCDENABLE
                  );

        // Wait for the frame done
        while (((INREG32(&m_pDispRegs->DISPC_IRQSTATUS) &
                 DISPC_IRQSTATUS_FRAMEDONE) == 0) &&
                 (dwTimeout-- > 0)
                 )
        {
            Sleep(1);
        }

        //  Clear all DSS interrupts
        OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, 0xFFFFFFFF );

        // Set the power level to D4(OFF)
        SetPowerLevel(D4);

        // Issue a reset cmd to DSS to recover from failure
        ResetDSS();

        //  Enable controller power
        SetPowerLevel( D0 );

        // Initialize the LCD panel related parameters
        InitLCD();

        // Clear all the interrupts status
        OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, 0xFFFFFFFF );

        // Could change to reset the pipeline status to inactive state...
        if (g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_LCD] > 0)
        {
            RestoreRegisters(OMAP_DSS_DESTINATION_LCD);
        }

        if (m_bTVEnable)
        {
            RestoreRegisters(OMAP_DSS_DESTINATION_TVOUT);
        }

        //  Clear refcounts on pipelines
        g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_LCD] = 0;
        g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_TVOUT] = 0;
        
        //EnablePipeline function to turn the active pipelines
        if ((g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled)||
            (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled) ||
            (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled)
            )
        {

            for ( DWORD i = 0;
                  i < OMAP_DSS_PIPELINE_MAX;
                  i++)
            {
                pipelineID = (OMAP_DSS_PIPELINE)i;
                if (g_rgPipelineMapping[pipelineID].bEnabled == TRUE)
                    {
                    g_rgPipelineMapping[pipelineID].bEnabled = FALSE;
                    EnablePipeline(pipelineID);
                    }
            }

            // Enable the LCD control bit
            SETREG32( &m_pDispRegs->DISPC_CONTROL,
                       DISPC_CONTROL_LCDENABLE );

            // Enable the TVOut control bit
            if (m_bTVEnable)
            {
                SETREG32( &m_pDispRegs->DISPC_CONTROL,
                           DISPC_CONTROL_DIGITALENABLE);
            }

        }

        goto cleanUp;
    }

    if (irqStatus & (DISPC_IRQSTATUS_GFXFIFOUNDERFLOW  |
                     DISPC_IRQSTATUS_VID1FIFOUNDERFLOW |
                     DISPC_IRQSTATUS_VID2FIFOUNDERFLOW
                     ))
    {
         DEBUGMSG(ZONE_ERROR,(L"DSS pipeline underflow error.Intrstatus:%x\r\n",irqStatus));
    }

    //Vsync event has to be asserted when both LCD and TV have actually vsync'ed (if tv-out is enabled).
    //There's 3 scenarios:
    //1. LCD v-sync occurs before TV v-sync.
    //2. TV-out v-sync occurs before LCD v-sync. 
    //3. Both occurred
    //For case 3 then the check below will signal the v-sync event
    //For case 1, the status bit's are saved. The next interrupt 
    //occuring due to tv-out (or lcd for case 2) will happen and the check below will signal the vsync event.
    //
    //If tv-out is disabled, since tvVysnc is always true, then only the state of lcdVsync matters.

    
    lcdVsync = ((irqStatus & DISPC_IRQSTATUS_VSYNC) == DISPC_IRQSTATUS_VSYNC) ||
                   ((m_lastVsyncIRQStatus & DISPC_IRQSTATUS_VSYNC) == DISPC_IRQSTATUS_VSYNC);
       
    if(m_bTVEnable == TRUE)
    {
       tvVsync = ((irqStatus & DISPC_IRQSTATUS_EVSYNC_EVEN) == DISPC_IRQSTATUS_EVSYNC_EVEN) ||
                 ((irqStatus & DISPC_IRQSTATUS_EVSYNC_ODD) == DISPC_IRQSTATUS_EVSYNC_ODD) ||
                 ((m_lastVsyncIRQStatus & DISPC_IRQSTATUS_EVSYNC_EVEN) == DISPC_IRQSTATUS_EVSYNC_EVEN) ||
                 ((m_lastVsyncIRQStatus & DISPC_IRQSTATUS_EVSYNC_ODD) == DISPC_IRQSTATUS_EVSYNC_ODD);
         
    }

    if(lcdVsync && tvVsync) 
    {        
        SetEvent(m_hVsyncEvent);
        SetEvent(m_hVsyncEventSGX);    
		 SetEvent(m_hVsyncEventWSEGL);
        m_lastVsyncIRQStatus = 0;
    }
    else
    {
       //Save the status of the Vsync IRQ's if we didn't signal the Vsync event.         
       // Note that if the interrupt status is not cleared we will be right back...        
       m_lastVsyncIRQStatus = irqStatus & (DISPC_IRQENABLE_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD|DISPC_IRQSTATUS_VSYNC);
    }
    

    if(irqStatus & DISPC_IRQSTATUS_PROGRAMMEDLINENUMBER)
    {
        SetEvent(m_hScanLineEvent);
    }

    if (irqStatus & DISPC_IRQSTATUS_OCPERROR)
    {
        DEBUGMSG(ZONE_ERROR,(L"OCP_ERROR FATAL!!\r\n"));
    }

    // Clear all interrupts
    // Note that SETREG32 does read, OR of argument and write, the IRQSTATUS is write '1' to clear,
    // this will clear all interrupt bits, not just the ones in the argument.
    OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, irqStatus);

cleanUp:

    ReleaseRegs();
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetSurfaceMgr(
    OMAPSurfaceManager *pSurfMgr
    )
{
    //  Reference the given surface mamager
    m_pSurfaceMgr = pSurfMgr;
    
    //  Return result
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetPipelineAttribs(
    OMAP_DSS_PIPELINE       ePipeline,
    OMAP_DSS_DESTINATION    eDestination,
    OMAPSurface*            pSurface,
    DWORD                   dwPosX,
    DWORD                   dwPosY
    )
{
    BOOL    bResult = FALSE;
    OMAP_DSS_ROTATION   eRotation;
    BOOL                bMirror;
    DWORD               dwVidRotation = 0;
    DWORD               dwX, dwY;


    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Get rotation and mirror settings for pipeline output
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror   = g_rgPipelineMapping[ePipeline].bMirror;


    //  Set rotation attributes for video pipelines if pixel format is YUV
    if( pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_YUV2 ||    
        pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_UYVY )  
    {
        //  Depending on rotation and mirror settings, change the VID rotation attributes
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Settings for rotation angle 0
                dwVidRotation = (bMirror) ? DISPC_VID_ATTR_VIDROTATION_180 : DISPC_VID_ATTR_VIDROTATION_0;
                break;

            case OMAP_DSS_ROTATION_90:
                //  Settings for rotation angle 90 (270 for DSS setting)
                dwVidRotation = DISPC_VID_ATTR_VIDROTATION_270 | DISPC_VID_ATTR_VIDROWREPEATENABLE;
                //dwVidRotation |= DISPC_VID_ATTR_VIDDMAOPTIMIZATION;
                break;

            case OMAP_DSS_ROTATION_180:
                //  Settings for rotation angle 180
                dwVidRotation = (bMirror) ? DISPC_VID_ATTR_VIDROTATION_0 : DISPC_VID_ATTR_VIDROTATION_180;
                break;

            case OMAP_DSS_ROTATION_270:
                //  Settings for rotation angle 270 (90 for DSS setting)
                dwVidRotation = DISPC_VID_ATTR_VIDROTATION_90 | DISPC_VID_ATTR_VIDROWREPEATENABLE;
                //dwVidRotation |= DISPC_VID_ATTR_VIDDMAOPTIMIZATION;
                break;
        }
    }


    //  Compute new origin and swap width/height based on GFX pipeline rotation angle
    switch( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].eRotation )
    {
        case OMAP_DSS_ROTATION_0:
            dwX = dwPosX;
            dwY = dwPosY;
            break;
            
        case OMAP_DSS_ROTATION_90:
            dwX = dwPosY;
            dwY = GetLCDHeight() - pSurface->Width() - dwPosX;
            break;
            
        case OMAP_DSS_ROTATION_180:
            dwX = GetLCDWidth() - pSurface->Width() - dwPosX;
            dwY = GetLCDHeight() - pSurface->Height() - dwPosY;
            break;
            
        case OMAP_DSS_ROTATION_270:
            dwX = GetLCDWidth() - pSurface->Height() - dwPosY;
            dwY = dwPosX;
            break;
            
        default:
            ASSERT(0);
            goto cleanUp;
    }


    //  Configure the attributes of the selected pipeline

    
    //  GFX pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        //  Set attributes of pipeline
        OUTREG32( &m_pDispRegs->DISPC_GFX_ATTRIBUTES,
                    ((eDestination == OMAP_DSS_DESTINATION_TVOUT) ? DISPC_GFX_ATTR_GFXCHANNELOUT : 0) |
                    DISPC_GFX_ATTR_GFXBURSTSIZE_16x32 |
                    DISPC_GFX_ATTR_GFXREPLICATIONENABLE |
                    DISPC_GFX_ATTR_GFXFORMAT(pSurface->PixelFormat())
                    );

        //  Size of window
        OUTREG32( &m_pDispRegs->DISPC_GFX_SIZE,
                    DISPC_GFX_SIZE_GFXSIZEX(pSurface->Width(eRotation)) |
                    DISPC_GFX_SIZE_GFXSIZEY(pSurface->Height(eRotation))
                    );

        //  Position of window
        OUTREG32( &m_pDispRegs->DISPC_GFX_POSITION,
                    DISPC_GFX_POS_GFXPOSX(dwX) |
                    DISPC_GFX_POS_GFXPOSY(dwY)
                    );
                    
        //  Pipeline FIFO and DMA settings
        OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD,
                    DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                    DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                    );

        OUTREG32( &m_pDispRegs->DISPC_GFX_PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_ROW_INC, pSurface->RowIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_WINDOW_SKIP, 0 );

        OUTREG32( &m_pDispRegs->DISPC_GFX_BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_BA1, pSurface->PhysicalAddr(eRotation, bMirror) );

    }    


    //  VIDEO1 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        //  Set attributes of pipeline
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES,
                    ((eDestination == OMAP_DSS_DESTINATION_TVOUT) ? DISPC_VID_ATTR_VIDCHANNELOUT : 0) |
                    dwVidRotation |
                    DISPC_VID_ATTR_VIDBURSTSIZE_16x32 |
                    DISPC_VID_ATTR_VIDCOLORCONVENABLE |
                    DISPC_VID_ATTR_VIDRESIZE_NONE |
                    DISPC_VID_ATTR_VIDFORMAT(pSurface->PixelFormat())
                    );

        //  Size of window; picture size is the same for no scaling
        OUTREG32( &m_pDispRegs->tDISPC_VID1.SIZE,
                    DISPC_VID_SIZE_VIDSIZEX(pSurface->Width(eRotation)) |
                    DISPC_VID_SIZE_VIDSIZEY(pSurface->Height(eRotation))
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.PICTURE_SIZE,
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEX(pSurface->Width(eRotation)) |
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEY(pSurface->Height(eRotation))
                    );

        //  Position of window
        OUTREG32( &m_pDispRegs->tDISPC_VID1.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );
                    
        //  Pipeline FIFO and DMA settings
        OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD,
                    DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                    DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );
        
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );

        
        //  Color conversion coefficients
        OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF0, m_pColorSpaceCoeffs[0] );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF1, m_pColorSpaceCoeffs[1] );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF2, m_pColorSpaceCoeffs[2] );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF3, m_pColorSpaceCoeffs[3] );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF4, m_pColorSpaceCoeffs[4] );
    }    


    //  VIDEO2 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        //  Set attributes of pipeline
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES,
                    ((eDestination == OMAP_DSS_DESTINATION_TVOUT) ? DISPC_VID_ATTR_VIDCHANNELOUT : 0) |
                    dwVidRotation |
                    DISPC_VID_ATTR_VIDBURSTSIZE_16x32 |
                    DISPC_VID_ATTR_VIDCOLORCONVENABLE |
                    DISPC_VID_ATTR_VIDRESIZE_NONE |
                    DISPC_VID_ATTR_VIDFORMAT(pSurface->PixelFormat())
                    );

        //  Size of window; picture size is the same for no scaling
        OUTREG32( &m_pDispRegs->tDISPC_VID2.SIZE,
                    DISPC_VID_SIZE_VIDSIZEX(pSurface->Width(eRotation)) |
                    DISPC_VID_SIZE_VIDSIZEY(pSurface->Height(eRotation))
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.PICTURE_SIZE,
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEX(pSurface->Width(eRotation)) |
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEY(pSurface->Height(eRotation))
                    );

        //  Position of window
        OUTREG32( &m_pDispRegs->tDISPC_VID2.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );
                    
        //  Pipeline FIFO and DMA settings
        OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD,
                    DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                    DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );



        //  Color conversion coefficients
        OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF0, m_pColorSpaceCoeffs[0] );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF1, m_pColorSpaceCoeffs[1] );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF2, m_pColorSpaceCoeffs[2] );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF3, m_pColorSpaceCoeffs[3] );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF4, m_pColorSpaceCoeffs[4] );
    }    



    //  Set mapping of pipeline to destination and surface
    g_rgPipelineMapping[ePipeline].eDestination = eDestination;
    g_rgPipelineMapping[ePipeline].pSurface     = pSurface;
    g_rgPipelineMapping[ePipeline].dwDestWidth  = pSurface->Width(eRotation);
    g_rgPipelineMapping[ePipeline].dwDestHeight = pSurface->Height(eRotation);
    g_rgPipelineMapping[ePipeline].pOldSurface  = pSurface;

    //  Reset the scaling factors to 100% and no interlacing
    g_rgPipelineScaling[ePipeline].dwHorzScaling = 1;
    g_rgPipelineScaling[ePipeline].dwVertScaling = 1;
    g_rgPipelineScaling[ePipeline].dwInterlace   = 0;
    
    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
void GetFIRCoef(void *pCoeffs, DWORD dwTaps, DWORD dwPhaseInc, DWORD dwFlickerFilter)
{
    if (dwTaps == 3)
    {
        if (dwPhaseInc > (128*14))  {memcpy(pCoeffs, g_coef3_M16, sizeof(g_coef3_M16));}
        else if (dwFlickerFilter)   {memcpy(pCoeffs, g_coef3_M16, sizeof(g_coef3_M16));}
        else                        {memcpy(pCoeffs, g_coef3_M8, sizeof(g_coef3_M8));  }
    }
    else //dwTaps == 5
    {
        if(dwPhaseInc > (128*26))                               {memcpy(pCoeffs, g_coef_M32, sizeof(g_coef_M32));}
        else if (dwPhaseInc > (128*22))                         {memcpy(pCoeffs, g_coef_M26, sizeof(g_coef_M26));}
        else if (dwPhaseInc > (128*19))                         {memcpy(pCoeffs, g_coef_M22, sizeof(g_coef_M22));}
        else if (dwPhaseInc > (128*16))                         {memcpy(pCoeffs, g_coef_M19, sizeof(g_coef_M19));}
        else if (dwPhaseInc > (128*14))                         {memcpy(pCoeffs, g_coef_M16, sizeof(g_coef_M16));}
        else if ((dwPhaseInc > (128*13)) && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M14, sizeof(g_coef_M14));}
        else if ((dwPhaseInc > (128*12)) && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M13, sizeof(g_coef_M13));}
        else if ((dwPhaseInc > (128*11)) && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M12, sizeof(g_coef_M12));}
        else if ((dwPhaseInc > (128*10)) && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M11, sizeof(g_coef_M11));}
        else if ((dwPhaseInc > (128*9))  && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M10, sizeof(g_coef_M10));}
        else if ((dwPhaseInc > (128*8))  && (!dwFlickerFilter)) {memcpy(pCoeffs, g_coef_M9,  sizeof(g_coef_M9)); }
        else if (dwFlickerFilter)                               {memcpy(pCoeffs, g_coef_M16, sizeof(g_coef_M16));}
        else                                                    {memcpy(pCoeffs, g_coef_M8,  sizeof(g_coef_M8)); }
  }
}

//------------------------------------------------------------------------------
BOOL NeedISPResizer(DWORD dwHorzScale,DWORD dwVertScale,DWORD PCD)
{
    DWORD totalScaleRequired;

    /* Not using ISP resizer for upsampling */    
    if ((dwHorzScale<1024) || (dwVertScale<1024))
        return FALSE;
    /* no scalar needed */
    if ((dwHorzScale==1024) && (dwVertScale==1024))
        return FALSE;

    /* all other cases involve downsampling */
    dwHorzScale=(DWORD)ceil((double)dwHorzScale/1024);
    dwVertScale=(DWORD)ceil((double)dwVertScale/1024);

    totalScaleRequired=((dwHorzScale==1)?0:dwHorzScale)+
                        ((dwVertScale==1)?0:dwVertScale);
                        
    if ((totalScaleRequired>PCD) && (dwHorzScale<=4) && (dwVertScale<=4))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
        
}


BOOL
OMAPDisplayController::SetScalingAttribs(
    OMAP_DSS_PIPELINE       ePipeline,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    DWORD                   dwPosX,
    DWORD                   dwPosY
    )
{
    RECT    srcRect,
            destRect;

    //  Check for surface
    if( g_rgPipelineMapping[ePipeline].pSurface == NULL )
        return FALSE;
        
    //  Create src and dest RECTs for this scaling setup
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = g_rgPipelineMapping[ePipeline].pSurface->Width();
    srcRect.bottom = g_rgPipelineMapping[ePipeline].pSurface->Height();

    destRect.left = dwPosX;
    destRect.top = dwPosY;
    destRect.right = dwPosX + dwWidth;
    destRect.bottom = dwPosY + dwHeight;

    //  Call the RECT based scaling method
    return SetScalingAttribs( ePipeline, &srcRect, &destRect );
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetScalingAttribs(
    OMAP_DSS_PIPELINE       ePipeline,
    RECT*                   pSrcRect,
    RECT*                   pDestRect
    )
{
    BOOL                bResult = FALSE;
    OMAPSurface*        pSurface;
    OMAP_DSS_ROTATION   eRotation;
    BOOL                bMirror;
    DWORD   dwSrcWidth,
            dwSrcHeight,
            dwDestWidth,
            dwDestHeight;
    DWORD   dwHorzScale,
            dwVertScale;
    DWORD   dwX, dwY;
    DWORD   dwCurrAttribs;
    DWORD   dwScaleEnable = 0;
    DWORD               dwAccum0 = 0;
    DWORD               dwAccum1 = 0;
    DWORD               dwHorzDecimation = 1;
    DWORD               dwVertDecimation = 1;
    DWORD               dwInterlace = 0;
    DWORD*  pHorizCoeffs = NULL;
    DWORD*  pVertCoeffs = NULL;
    DWORD               dwPixelsPerLine;
    DWORD   i;
    BOOL                bYUVRotated = FALSE;
    DWORD               dwInterlaceCorrection;
    DWORD               dwData1, dwData2;
    DWORD               dwVTaps;
    DWORD               dwFlickerFilter = 0;
    BYTE                Coefficients[5][8];

    // Temp array to hold calculated Scaling Coeffs
    DWORD               dwHorizCoeffs[NUM_SCALING_COEFFS];
    DWORD               dwVertCoeffs[NUM_SCALING_COEFFS];
    RECT                privSrcRect = *pSrcRect;
    BOOL                bUseResizer = FALSE;
    
    
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Get rotation and mirror settings for pipeline output
    pSurface  = g_rgPipelineMapping[ePipeline].pSurface;
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror   = g_rgPipelineMapping[ePipeline].bMirror;


    //  Compute horizontal and vertical scaling factors
    dwSrcWidth  = pSrcRect->right - pSrcRect->left;
    dwSrcHeight = pSrcRect->bottom - pSrcRect->top;

    dwDestWidth  = pDestRect->right - pDestRect->left;
    dwDestHeight = pDestRect->bottom - pDestRect->top;

    
    //  Check for odd destination RECT values only for YUV pixel formats
    if( (pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_YUV2) ||
        (pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_UYVY) )
    {
        //  Check for odd destination RECT values
        if( (dwDestWidth % 2) == 1 )
        {
            pDestRect->right -= 1;
            dwDestWidth -= 1;
        }

        if( (dwDestHeight % 2) == 1 )
        {
            pDestRect->bottom -= 1;
            dwDestHeight -= 1;
        }
        if( (dwSrcWidth % 2) == 1 )
        {
            pSrcRect->right -= 1;
            dwSrcWidth -= 1;
        }

        if( (dwSrcHeight % 2) == 1 )
        {
            pSrcRect->bottom -= 1;
            dwSrcHeight -= 1;
        }
    }

    //  Compute scaling factors
    dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
    dwVertScale = 1024 * dwSrcHeight / dwDestHeight;

    privSrcRect = *pSrcRect;

	if (( g_rgPipelineMapping[ePipeline].eDestination == OMAP_DSS_DESTINATION_LCD ) &&
        (ePipeline != OMAP_DSS_PIPELINE_GFX ))
    {
		if ( m_bDssIspRszEnabled && // is globally enabled
             ((NeedISPResizer(dwHorzScale,dwVertScale,LcdPdd_Get_PixClkDiv()))==TRUE) && // do we need it
             (pSurface->GetRSZHandle(TRUE) != NULL) &&  // can we use it
             (pSurface->UseResizer(TRUE) == TRUE)) // do we have the buffers
		{		    
		    //all check passed
            bUseResizer = TRUE;
            /* Since ISP Resizer would be providing the src image to 
            DSS, make the SRC RECT for DSS same as DEST RECT */
            privSrcRect.left=0;
            privSrcRect.right = dwDestWidth;
            privSrcRect.top=0;
			privSrcRect.bottom = dwDestHeight;			
			
			dwSrcWidth  = privSrcRect.right - privSrcRect.left;
            dwSrcHeight = privSrcRect.bottom - privSrcRect.top;
            dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
			dwVertScale = 1024 * dwSrcHeight / dwDestHeight;            
        } 
        else
        {
            /* some check did not pass - disable ISP resizer */
            pSurface->SetRSZHandle(NULL,TRUE);
            pSurface->UseResizer(FALSE);
            memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));
        }        
    }
    else /* for cases where surfaces are re-assigned to different pipelines */
    {
        /* some check did not pass - disable ISP resizer */
        pSurface->SetRSZHandle(NULL,TRUE);
        pSurface->UseResizer(FALSE);
        memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));
    }    
    
    if (bUseResizer)
    {
        /* lets allocate assoc surface if not already allocated */
        if (pSurface->OmapAssocSurface() == NULL)
        {
            //  Allocate OMAP surface
            OMAPSurface*    pAssocOmapSurface;
            bResult = m_pSurfaceMgr->Allocate(
                            pSurface->PixelFormat(),
                            pSurface->Width(),
                            pSurface->Height(),
                            &pAssocOmapSurface,
                            pSurface );
            if (bResult == FALSE)
            {
                RETAILMSG(1, (L"SetScalingAttribs: surfaceMgr Allocate - Out of Video Memory\n"));
                bUseResizer = FALSE;
                pSurface->SetRSZHandle(NULL,TRUE);
                pSurface->UseResizer(FALSE);
                memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));
            }
        }
    }
    
    
    if ((eRotation == OMAP_DSS_ROTATION_0)||(eRotation == OMAP_DSS_ROTATION_180))
        dwPixelsPerLine = GetLCDWidth();
    else
        dwPixelsPerLine = GetLCDHeight();

    // Limit the scaling to 1/3rd of the original size
    if ((dwHorzScale > 3072) && (dwSrcWidth > dwPixelsPerLine))
    {
        // Max scale factor feasible is 1/3rd
        // Based on 1/3rd scale factor, increase the dest rect size
        DWORD dstWidthOffset = 0;
        DWORD newDstWidth    = 0;
        newDstWidth    = dwSrcWidth/3;

        // Make sure to set the dst width >= 1/3 x srcwidth
        if (dwSrcWidth%3 != 0)
            newDstWidth += 1;
            
        // Check the size of the new dst width calculated.
        newDstWidth    = (newDstWidth > GetLCDWidth())? GetLCDWidth() : newDstWidth ;

        // Divide it equally to the left and right side of the dst window
        dstWidthOffset = (newDstWidth-dwDestWidth)/2;
          
        // adjust the Dest rect based on the new scale factor
        if ((DWORD)pDestRect->left > dstWidthOffset)
            pDestRect->left  -= dstWidthOffset;

        pDestRect->right += dstWidthOffset;
    }
    else
    {
        //  If playback is rotated and scaled and color converted, adjust clipping to avoid sync lost
        if( (eRotation == OMAP_DSS_ROTATION_90)||(eRotation == OMAP_DSS_ROTATION_270) )
        {
            if( (pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_YUV2) ||
                (pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_UYVY) )
            {
                //  Flag special case of YUV rotated
                bYUVRotated = TRUE;
                
                //  Rotated scaling with color conversion in these bounds has issues
                if( dwHorzScale > 1536 && dwHorzScale < 2048 )            
                {
                    DWORD   dwOldSrcWidth = dwSrcWidth;
                    
                    //  Clip src width  
                    dwSrcWidth = dwSrcWidth * 1536 / dwHorzScale;
                    dwSrcWidth = ((dwSrcWidth % 2) == 0) ? dwSrcWidth : dwSrcWidth - 1;
                    privSrcRect.left  = privSrcRect.left + (dwOldSrcWidth - dwSrcWidth)/2;
                    privSrcRect.right = privSrcRect.left + dwSrcWidth;
                }
            
                //  Rotated scaling with color conversion in these bounds has issues
                if( dwVertScale > 1536 && dwVertScale < 2048 )            
                {
                    DWORD   dwOldSrcHeight = dwSrcHeight;
                    
                    //  Clip src height  
                    dwSrcHeight = dwSrcHeight * 1536 / dwVertScale;
                    dwSrcHeight = ((dwSrcHeight % 2) == 0) ? dwSrcHeight : dwSrcHeight - 1;
                    privSrcRect.top    = privSrcRect.top + (dwOldSrcHeight - dwSrcHeight)/2;
                    privSrcRect.bottom = privSrcRect.top + dwSrcHeight;
                }
            }
        }
    }

    if (!bUseResizer)
        //  Set the clipping region for the surface
        g_rgPipelineMapping[ePipeline].pSurface->SetClipping( &privSrcRect);
    else
    {
        RECT rszRect;
        rszRect.top = 0;
        rszRect.left = 0;
        rszRect.bottom = rszRect.top + dwDestHeight;
        rszRect.right = rszRect.left + dwDestWidth;        
        g_rgPipelineMapping[ePipeline].pSurface->SetClipping( pSrcRect);
        g_rgPipelineMapping[ePipeline].pSurface->OmapAssocSurface()->SetClipping( &rszRect );
    }

    //  Compute src and dest width/height
    dwSrcWidth  = privSrcRect.right - privSrcRect.left;
    dwSrcHeight = privSrcRect.bottom - privSrcRect.top;

    dwDestWidth  = pDestRect->right - pDestRect->left;
    dwDestHeight = pDestRect->bottom - pDestRect->top;

    //  Swap src width/height based on pipeline rotation angle
    switch( g_rgPipelineMapping[ePipeline].eRotation )
    {
        case OMAP_DSS_ROTATION_90:
            //  Settings for rotation angle 90
            i = dwSrcWidth;
            dwSrcWidth = dwSrcHeight;
            dwSrcHeight = i;
            break;

        case OMAP_DSS_ROTATION_270:
            //  Settings for rotation angle 270
            i = dwSrcWidth;
            dwSrcWidth = dwSrcHeight;
            dwSrcHeight = i;
            break;
    }

    //  Default origin
    dwX = pDestRect->left;
    dwY = pDestRect->top;


    //------------------------------------------------------------------------------
    //  Configure the scaling of the pipeline for LCD display
    //
    if( g_rgPipelineMapping[ePipeline].eDestination == OMAP_DSS_DESTINATION_LCD )
    {
        //  Compute new origin and swap destination width/height based on GFX pipeline rotation angle
        switch( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Settings for rotation angle 0
                dwX = pDestRect->left;
                dwY = pDestRect->top;
                break;

            case OMAP_DSS_ROTATION_90:
                //  Settings for rotation angle 90
                dwX = pDestRect->top;
                dwY = GetLCDHeight() - dwDestWidth - pDestRect->left;

                i = dwDestWidth;
                dwDestWidth = dwDestHeight;
                dwDestHeight = i;
                break;

            case OMAP_DSS_ROTATION_180:
                //  Settings for rotation angle 180
                dwX = GetLCDWidth() - dwDestWidth - pDestRect->left;
                dwY = GetLCDHeight() - dwDestHeight - pDestRect->top;
                break;

            case OMAP_DSS_ROTATION_270:
                //  Settings for rotation angle 270
                dwX = GetLCDWidth() - dwDestHeight - pDestRect->top;
                dwY = pDestRect->left;

                i = dwDestWidth;
                dwDestWidth = dwDestHeight;
                dwDestHeight = i;
                break;
        }
    

        //  Compute horizontal and vertical scaling factors
        dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
        dwVertScale = 1024 * dwSrcHeight / dwDestHeight;

        //  Determine if scaling is within HW scaling capabilities
        //  If not, use surface scaling factor to decimate the source surface
        //  by 2, 4 or 8
        if( bYUVRotated )
        {
            if( dwHorzScale >= 2048 )
                dwHorzDecimation = 2;

            if( dwHorzScale >= 4096 )
                dwHorzDecimation = 4;

            if( dwHorzScale >= 8192 )
                dwHorzDecimation = 8;


            if( dwVertScale >= 2048 )
                dwVertDecimation = 2;

            if( dwVertScale >= 4096 )
                dwVertDecimation = 4;

            if( dwVertScale >= 8192 )
                dwVertDecimation = 8;
        }
        else
        {
            if( dwHorzScale > 4096 )
                dwHorzDecimation = 2;

            if( dwHorzScale > 8192 )
                dwHorzDecimation = 4;


            if( dwVertScale > 2048 )
                dwVertDecimation = 2;

            if( dwVertScale > 4096 )
                dwVertDecimation = 4;

            if( dwVertScale > 8192 )
                dwVertDecimation = 8;
        }

            
        //  Adjust the HW scaling factors by the decimation factor      
        dwHorzScale = 1024 * (dwSrcWidth/dwHorzDecimation) / dwDestWidth;    
        dwVertScale = 1024 * (dwSrcHeight/dwVertDecimation) / dwDestHeight;    
        
        
        //  Set the decimation factors for the surface 
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
            case OMAP_DSS_ROTATION_180:            
                //  Standard orientation
                pSurface->SetHorizontalScaling( dwHorzDecimation );
                pSurface->SetVerticalScaling( dwVertDecimation );
                break;

            case OMAP_DSS_ROTATION_90:
            case OMAP_DSS_ROTATION_270:            
                //  Rotated orientation
                pSurface->SetHorizontalScaling( dwVertDecimation );
                pSurface->SetVerticalScaling( dwHorzDecimation );
                break;
        }
        
        //  Adjust the source width and height by the decimation factor
        dwSrcWidth  = dwSrcWidth / dwHorzDecimation;
        dwSrcHeight = dwSrcHeight / dwVertDecimation;


        //  If YUV rotated, check for any odd width or height values due to decimation
        if( bYUVRotated )
        {
            if( (dwSrcWidth % 2) == 1 )
            {
                dwSrcWidth -= 1;
                
                //  Adjust clipping; note rotation means adjust clipping height here
                privSrcRect.bottom -= dwHorzDecimation;
            }

            if( (dwSrcHeight % 2) == 1 )
            {
                dwSrcHeight -= 1;

                //  Adjust clipping; note rotation means adjust clipping width here
                privSrcRect.right -= dwVertDecimation;
            }

            //  Set the clipping region for the surface
            g_rgPipelineMapping[ePipeline].pSurface->SetClipping( &privSrcRect );
            if (g_rgPipelineMapping[ePipeline].pSurface->OmapAssocSurface())
            {
                RECT rszRect;
                rszRect.top = 0;
                rszRect.left = 0;
                rszRect.bottom = rszRect.top + (pDestRect->bottom - pDestRect->top);
                rszRect.right = rszRect.left + (pDestRect->right - pDestRect->left);                            
                g_rgPipelineMapping[ePipeline].pSurface->OmapAssocSurface()->SetClipping( &rszRect );
            }                

            //  Recalculate the scale factors to account for adjustments made
            dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
            dwVertScale = 1024 * dwSrcHeight / dwDestHeight;
        } 


        //  Based on scaling factor, determine which coeffs to use and if to enable scaling
        dwScaleEnable |= (dwHorzScale == 1024) ? 0 : DISPC_VID_ATTR_VIDRESIZE_HORIZONTAL;
        dwScaleEnable |= (dwVertScale == 1024) ? 0 : DISPC_VID_ATTR_VIDRESIZE_VERTICAL;


        //  Horizontal scaling
        if( dwHorzScale > 1024 )
        {
            //  Use down-sampling horizontal coeffs
            dwScaleEnable |= DISPC_VID_ATTR_VIDHRESIZE_CONF_DOWN;
            pHorizCoeffs = g_dwScalingCoeff_Horiz_Down;
        }
        else
        {
            //  Use up-sampling horizontal coeffs
            dwScaleEnable |= DISPC_VID_ATTR_VIDHRESIZE_CONF_UP;
            pHorizCoeffs = g_dwScalingCoeff_Horiz_Up;
        }


        //  Vertical scaling
        if( dwVertScale > 1024 )
        {
            //  Use 5 tap down-sampling vertical coeffs for scaling between 50% and 25%
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_5;
            dwScaleEnable |= DISPC_VID_ATTR_VIDLINEBUFFERSPLIT;
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_5_Taps;
        } 
        //else if( dwVertScale > 1024 )
        //{
        //    //  Use 3 tap down-sampling vertical coeffs for scaling between 100% and 50%
        //    dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
        //    dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
        //    pVertCoeffs = g_dwScalingCoeff_Vert_Down_3_Taps;
        //}
        else
        {
            //  Use up-sampling vertical coeffs
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_UP;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            pVertCoeffs = g_dwScalingCoeff_Vert_Up_3_Taps;
        }

        // For Portrait mode, the Vertical down scale coeff has to be 5 tap to
        // prevent SYNCLOST. so ignore the scale factor and force the 5-tap.
        if ((( eRotation == OMAP_DSS_ROTATION_90 )|| 
            ( eRotation == OMAP_DSS_ROTATION_270 )) && 
            ( pVertCoeffs == g_dwScalingCoeff_Vert_Down_3_Taps ))
        {
            //clear existing scale option
            dwScaleEnable &= ~DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_5;
            dwScaleEnable |= DISPC_VID_ATTR_VIDLINEBUFFERSPLIT;
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_5_Taps;
        }
    }


    //------------------------------------------------------------------------------
    //  Configure the scaling of the pipeline for TV display
    //
    else if( g_rgPipelineMapping[ePipeline].eDestination == OMAP_DSS_DESTINATION_TVOUT )
    {
        dwInterlaceCorrection = 2;

        //  Half the dest height to enable filtering for TV interlace
        dwDestHeight /= dwInterlaceCorrection;

#if 0
        //  Compute horizontal and vertical scaling factors
        dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
        dwVertScale = 1024 * dwSrcHeight / dwDestHeight;
        
        //  Determine if scaling is within HW scaling capabilities
        //  If not, use surface scaling factor to decimate the source surface
        //  by 2, 4 or 8
        if( dwHorzScale > 4096 )
            dwHorzDecimation = 2;

        if( dwHorzScale > 8192 )
            dwHorzDecimation = 4;


        if( dwVertScale > 2048 )
            dwVertDecimation = 2;

        if( dwVertScale > 4096 )
            dwVertDecimation = 4;

        if( dwVertScale > 8192 )
            dwVertDecimation = 8;

            
        //  Adjust the HW scaling factors by the decimation factor      
        dwHorzScale = 1024 * (dwSrcWidth/dwHorzDecimation) / dwDestWidth;    
        dwVertScale = 1024 * (dwSrcHeight/dwVertDecimation) / dwDestHeight;    
        
        
        //  Set the decimation factors for the surface (always standard orientation for TV out)
        pSurface->SetHorizontalScaling( dwHorzDecimation );
        pSurface->SetVerticalScaling( dwVertDecimation );
        
        //  Adjust the source width and height by the decimation factor
        dwSrcWidth  = dwSrcWidth / dwHorzDecimation;
        dwSrcHeight = dwSrcHeight / dwVertDecimation;


        //  Based on scaling factor, determine which coeffs to use and if to enable scaling
        dwScaleEnable |= (dwHorzScale == 1024) ? 0 : DISPC_VID_ATTR_VIDRESIZE_HORIZONTAL;
        dwScaleEnable |= (dwVertScale == 1024) ? 0 : DISPC_VID_ATTR_VIDRESIZE_VERTICAL;


        //  Accum0/1 controls the scaler phase for the even/odd fields
        //  This is affected by the field polarity - bit 17 of VENC_GEN_CTRL_VAL register -
        //  If active low, the ratio is VIDFIRVINC/2, otherwise it's 1024 * dwDestHeight / dwSrcHeight
        dwAccum1 = (1024 * dwDestHeight / dwSrcHeight) << 16; 
    

        //  Horizontal scaling
        if( dwHorzScale > 1024 )
        {
            //  Use down-sampling horizontal coeffs
            dwScaleEnable |= DISPC_VID_ATTR_VIDHRESIZE_CONF_DOWN;
            pHorizCoeffs = g_dwScalingCoeff_Horiz_Down;
        }
        else
        {
            //  Use up-sampling horizontal coeffs
            dwScaleEnable |= DISPC_VID_ATTR_VIDHRESIZE_CONF_UP;
            pHorizCoeffs = g_dwScalingCoeff_Horiz_Up;
        }


        //  Vertical scaling (based on flicker filter setting instead of scaling value)
        if( m_dwTVFilterLevel >= OMAP_TV_FILTER_LEVEL_HIGH )
        {
            //  Use 5 tap down-sampling vertical coeffs for HIGH flicker filtering
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_5;
            dwScaleEnable |= DISPC_VID_ATTR_VIDLINEBUFFERSPLIT;
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_5_Taps;
        }
        else if( m_dwTVFilterLevel == OMAP_TV_FILTER_LEVEL_MEDIUM )
        {
            //  Use 3 tap down-sampling vertical coeffs for MEDIUM flicker filtering
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_3_Taps;
        }
        else if( m_dwTVFilterLevel == OMAP_TV_FILTER_LEVEL_LOW )
        {
            //  Use 3 tap up-sampling vertical coeffs for LOW flicker filtering
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_UP;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            pVertCoeffs = g_dwScalingCoeff_Vert_Up_3_Taps;
        }
        else if( m_dwTVFilterLevel == OMAP_TV_FILTER_LEVEL_OFF )
        {
            //  No flicker filtering but only if the destination height can be decimated to fit the TV height
            //  (account for the /2 of destHeight above)
            if( (dwSrcHeight == dwDestHeight) && (dwDestHeight <= GetTVHeight()/2) )
            {
                DEBUGMSG(ZONE_WARNING, (L"----- Using No interlace mode for TV out --------\r\n"));

                //  Can fit the surface on TV without interlacing
                dwInterlace = 0;
                
                //  No scaling
                dwScaleEnable &= ~DISPC_VID_ATTR_VIDRESIZE_VERTICAL;
                dwVertScale = 1024;
                
                //  No FIR accumulator values
                dwAccum0 = dwAccum1 = 0;
            }
            else if( (dwSrcHeight == 2*dwDestHeight) && (dwDestHeight <= GetTVHeight()) )
            {
                DEBUGMSG(ZONE_WARNING, (L"----- Using Interlace mode for TV out --------\r\n"));
            
                //  Need to decimate the height by 2 to fit on the TV via interlacing
                dwVertDecimation = 2;
                pSurface->SetVerticalScaling( dwVertDecimation );
        
                dwSrcHeight = dwSrcHeight / dwVertDecimation;
            
                //  No scaling
                dwScaleEnable &= ~DISPC_VID_ATTR_VIDRESIZE_VERTICAL;
                dwVertScale = 1024;

                //  Need to use the surface stride to output via interlacing
                dwInterlace = pSurface->Stride();

                //  No FIR accumulator values
                dwAccum0 = dwAccum1 = 0;
            }
            else
            {
                DEBUGMSG(ZONE_WARNING, (L"----- Using Scaling mode for TV out --------\r\n"));

                //  Have to scale the output to TV because it just won't fit otherwise
                //  Use 3 tap up-sampling vertical coeffs for LOW flicker filtering
                dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_UP;
                dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            }
                        
            //  Always need to point to some set of vertical coeffs
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_3_Taps;
        }
        else
        {
            //  Use 3 tap down-sampling vertical coeffs for MEDIUM flicker filtering as the default setting
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_3;
            pVertCoeffs = g_dwScalingCoeff_Vert_Down_3_Taps;
        }
#else
        //  Compute horizontal and vertical scaling factors
        dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
        dwVertScale = 1024 * dwSrcHeight / dwDestHeight;

        //  Determine if scaling is within HW scaling capabilities
        //  If not, use surface scaling factor to decimate the source surface
        //  by 2, 4 or 8
//        if( dwHorzScale > 4096 )
//            dwHorzDecimation = 2;

//        if( dwHorzScale > 8192 )
//            dwHorzDecimation = 4;


//        if( dwVertScale > 2048 )
//            dwVertDecimation = 2;

//        if( dwVertScale > 4096 )
//            dwVertDecimation = 4;

//        if( dwVertScale > 8192 )
//            dwVertDecimation = 8;

        //  Adjust the source width and height by the decimation factor
//        dwSrcWidth  = dwSrcWidth / dwHorzDecimation;
//        dwSrcHeight = dwSrcHeight / dwVertDecimation;

        //  Adjust the HW scaling factors by the decimation factor      
//        dwHorzScale = 1024 * dwSrcWidth / dwDestWidth;
//        dwVertScale = 1024 * dwSrcHeight / dwDestHeight;

        if (m_dwTVFilterLevel)
        {
            // Flicker filter not supported currently (If flickerFilter is used, scaling cannot be done)
            // dwFlickerFilter = 1;
        }

        //  Set the decimation factors for the surface (always standard orientation for TV out)
        pSurface->SetHorizontalScaling( dwHorzDecimation );
        pSurface->SetVerticalScaling( dwVertDecimation );

         //Get vertical scaler coefficients
        if (dwSrcWidth > 1280)
        {
            dwVTaps = 3;
        }
        else
        {
            dwVTaps = 5;
            dwScaleEnable |= DISPC_VID_ATTR_VIDLINEBUFFERSPLIT;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVERTICALTAPS_5;
        }

        GetFIRCoef((void*)Coefficients, dwVTaps, dwVertScale, dwFlickerFilter);
        for (i = 0; i < 8; i++)
        {
            dwData1 = (Coefficients[0][i]) | ((Coefficients[4][i]) << 8);
            dwData2 = ((Coefficients[1][i]) << 8) |((Coefficients[2][i]) << 16) | ((Coefficients[3][i]) << 24);

            /* Make up coefficients in the format needed further down */
            dwVertCoeffs[i*3]   = 0;
            dwVertCoeffs[i*3+1] = dwData2;
            dwVertCoeffs[i*3+2] = dwData1;
        }
        pVertCoeffs = dwVertCoeffs;

        //Now get horizontal scaler coefficients
        GetFIRCoef((void*)Coefficients, 5, dwHorzScale, 0);
        for (i = 0; i < 8; i++)
        {
            dwData1 = (Coefficients[0][i] | (Coefficients[1][i]) << 8) |((Coefficients[2][i]) << 16) | ((Coefficients[3][i]) << 24);
            dwData2 = (Coefficients[4][i]);

            /* Make up coefficients in the format needed further down */
            dwHorizCoeffs[i*3]   = dwData1;
            dwHorizCoeffs[i*3+1] = dwData2;
            dwHorizCoeffs[i*3+2] = 0;
        }
        pHorizCoeffs = dwHorizCoeffs;

        if (dwSrcHeight > (dwDestHeight * dwInterlaceCorrection))
        {
            //Down scale
            dwScaleEnable |= DISPC_VID_ATTR_VIDRESIZE_VERTICAL;
            dwScaleEnable |= DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN;
        }
        else if (dwSrcHeight < (dwDestHeight * dwInterlaceCorrection))
        {
            //Up scale
            dwScaleEnable |= DISPC_VID_ATTR_VIDRESIZE_VERTICAL;
        }
        else
        {
            //No scale
        }

        if (dwSrcWidth > dwDestWidth)
        {
            //Down scale
            dwScaleEnable |= DISPC_VID_ATTR_VIDRESIZE_HORIZONTAL;
            dwScaleEnable |= DISPC_VID_ATTR_VIDHRESIZE_CONF_DOWN;
        }
        else if (dwSrcWidth < dwDestWidth)
        {
            //Up scale
            dwScaleEnable |= DISPC_VID_ATTR_VIDRESIZE_HORIZONTAL;
        }
        else
        {
            //No scale
        }

        //  Accum0/1 controls the scaler phase for the even/odd fields
        if (m_dwTVFilterLevel == OMAP_TV_FILTER_LEVEL_OFF)
        {
            dwAccum0 = (((dwVertScale/dwInterlaceCorrection) % 1024) << 16);
            dwAccum1 = 0;
        }
        else
        {
            /* This case is not supported/tested */
            dwHorzScale = 1024;
            dwVertScale = 1024;
            dwAccum0 = 0;
            dwAccum1 = 0;
        }

        //  Need to use the surface stride to output via interlacing
        dwInterlace = pSurface->Stride();

        // Account for interlacing for Y offset (this is the line offset in each field).
        dwY /= dwInterlaceCorrection; 
#endif
    }
    else
    {
        ASSERT(0);
        goto cleanUp;
    }

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPDisplayController::SetScalingAttribs: "));
    DEBUGMSG(ZONE_WARNING, (L"  Src  RECT (%d,%d) (%d,%d)\r\n", privSrcRect.left, privSrcRect.top, privSrcRect.right, privSrcRect.bottom));
    DEBUGMSG(ZONE_WARNING, (L"  Dest RECT (%d,%d) (%d,%d)\r\n", pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom));
    DEBUGMSG(ZONE_WARNING, (L"  Computed Origin (%d,%d) for rotation angle %d\r\n", dwX, dwY, g_rgPipelineMapping[ePipeline].eRotation));
    DEBUGMSG(ZONE_WARNING, (L"  dwScaleEnable = 0x%08X  dwHorzScale = %d  dwVertScale = %d\r\n", dwScaleEnable, dwHorzScale, dwVertScale));
    DEBUGMSG(ZONE_WARNING, (L"  dwSrcWidth  = %d  dwSrcHeight  = %d\r\n", dwSrcWidth, dwSrcHeight));
    DEBUGMSG(ZONE_WARNING, (L"  dwDestWidth = %d  dwDestHeight = %d\r\n", dwDestWidth, dwDestHeight));
    DEBUGMSG(ZONE_WARNING, (L"  dwHorzDecimation = %d  dwVertDecimation = %d\r\n", dwHorzDecimation, dwVertDecimation));
    DEBUGMSG(ZONE_WARNING, (L"  ePipeline = %d\r\n", ePipeline));
    DEBUGMSG(ZONE_WARNING, (L"  pSurface = 0x%x Stride = %d Row_Inc = %d\r\n", pSurface,pSurface->Stride(eRotation),pSurface->RowIncr(eRotation, bMirror)));
   

    //  GFX pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        //  Scaling is not supported on the GFX plane
    }


    //  VIDEO1 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {   
        DEBUGMSG(ZONE_WARNING, (L"\t Resizer Enabled %d \r\n",bUseResizer));

        if (pSurface->isResizerEnabled())
        {   
            DWORD rszStatus = FALSE;
            /* set Resizer */
            rszStatus = pSurface->ConfigResizerParams(pSrcRect, pDestRect,eRotation);
            /* Start resizer */
            if (rszStatus)
                rszStatus = pSurface->StartResizer(pSurface->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror,OMAP_ASSOC_SURF_FORCE_OFF), //input
                                                   pSurface->OmapAssocSurface()->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror,OMAP_ASSOC_SURF_FORCE_OFF));               
            if (!(rszStatus))
            {
                RETAILMSG(TRUE,(L"SetScalingAttribs: Cannot configure/start ISP resizer; Cropping the image\r\n"));
                bUseResizer = FALSE;
                pSurface->UseResizer(FALSE);
                pSurface->SetRSZHandle(NULL, TRUE);
                memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));  
                pSurface->SetClipping( &privSrcRect);
            }            
        }
        
        //  Get the current attribute settings
        dwCurrAttribs = INREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES );

        //  Mask off the scaling bits
        dwCurrAttribs &= ~(DISPC_VID_ATTR_VIDRESIZE_MASK|DISPC_VID_ATTR_VIDLINEBUFFERSPLIT|DISPC_VID_ATTR_VIDVERTICALTAPS_5);
        
        //  Enable video resizing by or'ing with scale enable attribs
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES,
                    dwCurrAttribs | dwScaleEnable
                    );

        
        //  Size of resized output window and original picture size
        OUTREG32( &m_pDispRegs->tDISPC_VID1.SIZE,
                    DISPC_VID_SIZE_VIDSIZEX(dwDestWidth) |
                    DISPC_VID_SIZE_VIDSIZEY(dwDestHeight)
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.PICTURE_SIZE,
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEX(dwSrcWidth) |
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEY(dwSrcHeight)
                    );
        
        //  Position of window
        OUTREG32( &m_pDispRegs->tDISPC_VID1.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );

        //  DMA properties
        OUTREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC, (pSurface->RowIncr(eRotation, bMirror)) );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );

        //  Initialize FIR accumulators
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ACCU0,
                    dwAccum0
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.ACCU1,
                    dwAccum1
                    );

        //  Set FIR increment value and coeffs
        OUTREG32( &m_pDispRegs->tDISPC_VID1.FIR,
                    DISPC_VID_FIR_VIDFIRHINC(dwHorzScale) |
                    DISPC_VID_FIR_VIDFIRVINC(dwVertScale)
                    );

        for( i = 0; i < NUM_SCALING_PHASES; i++ )
        {
            //  OR the horiz and vert coeff values b/c some registers span both H and V coeffs
            OUTREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulH,
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );

            OUTREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulHV,
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );

            OUTREG32( &m_pDispRegs->DISPC_VID1_FIR_COEF_V[i],
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );
        }

        Dump_DISPC_VID( &m_pDispRegs->tDISPC_VID1, (UINT32*) &m_pDispRegs->DISPC_VID1_FIR_COEF_V[0], 1 );
    }


    //  VIDEO2 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        DEBUGMSG(ZONE_WARNING, (L"\t Resizer Enabled %d \r\n",bUseResizer));
        /* If using resizer */        
        if (pSurface->isResizerEnabled())
        {   
            DWORD rszStatus = FALSE;
            /* set Resizer */
            rszStatus = pSurface->ConfigResizerParams(pSrcRect, pDestRect,eRotation);
            /* Start resizer */
            if (rszStatus)
                rszStatus = pSurface->StartResizer(pSurface->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror,OMAP_ASSOC_SURF_FORCE_OFF), //input
                                                   pSurface->OmapAssocSurface()->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror, OMAP_ASSOC_SURF_FORCE_OFF));               
            if (!(rszStatus))
            {
                RETAILMSG(TRUE,(L"SetScalingAttribs: Cannot configure/start ISP resizer; Cropping the image\r\n"));
                bUseResizer = FALSE;
                pSurface->UseResizer(FALSE);
                pSurface->SetRSZHandle(NULL, TRUE);
                memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));                
            }            
        }
        
        
        //  Get the current attribute settings
        dwCurrAttribs = INREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES );

        //  Mask off the scaling bits
        dwCurrAttribs &= ~(DISPC_VID_ATTR_VIDRESIZE_MASK|DISPC_VID_ATTR_VIDLINEBUFFERSPLIT|DISPC_VID_ATTR_VIDVERTICALTAPS_5);
        
        //  Enable video resizing by or'ing with scale enable attribs
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES,
                    dwCurrAttribs | dwScaleEnable
                    );

        //  Size of resized output window; picture size was set by in SetPipelineAttribs()
        OUTREG32( &m_pDispRegs->tDISPC_VID2.SIZE,
                    DISPC_VID_SIZE_VIDSIZEX(dwDestWidth) |
                    DISPC_VID_SIZE_VIDSIZEY(dwDestHeight)
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.PICTURE_SIZE,
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEX(dwSrcWidth) |
                    DISPC_VID_PICTURE_SIZE_VIDORGSIZEY(dwSrcHeight)
                    );

        //  Position of window
        OUTREG32( &m_pDispRegs->tDISPC_VID2.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );

        //  DMA properties
        OUTREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );

        //  Initialize FIR accumulators
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ACCU0,
                    dwAccum0
                    );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.ACCU1,
                    dwAccum1
                    );

        //  Set FIR increment value and coeffs
        OUTREG32( &m_pDispRegs->tDISPC_VID2.FIR,
                    DISPC_VID_FIR_VIDFIRHINC(dwHorzScale) |
                    DISPC_VID_FIR_VIDFIRVINC(dwVertScale)
                    );

        for( i = 0; i < NUM_SCALING_PHASES; i++ )
        {
            //  OR the horiz and vert coeff values b/c some registers span both H and V coeffs
            OUTREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulH,
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );

            OUTREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulHV,
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );

            OUTREG32( &m_pDispRegs->DISPC_VID2_FIR_COEF_V[i],
                        *pHorizCoeffs++ | *pVertCoeffs++
                        );
        }

        Dump_DISPC_VID( &m_pDispRegs->tDISPC_VID2, (UINT32*) &m_pDispRegs->DISPC_VID2_FIR_COEF_V[0], 2 );
    }


    //  Update output width and height
    g_rgPipelineMapping[ePipeline].dwDestWidth  = dwDestWidth;
    g_rgPipelineMapping[ePipeline].dwDestHeight = dwDestHeight;

    //  Cache the decimation factors applied to the source surface
    g_rgPipelineScaling[ePipeline].dwHorzScaling = dwHorzDecimation;
    g_rgPipelineScaling[ePipeline].dwVertScaling = dwVertDecimation;
    g_rgPipelineScaling[ePipeline].dwInterlace   = dwInterlace;

    //  Set the decimation factors for the surface back to normal
    pSurface->SetHorizontalScaling( 1 );
    pSurface->SetVerticalScaling( 1 );


    //  Result
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnablePipeline(
    OMAP_DSS_PIPELINE       ePipeline
    )
{
    BOOL                    bResult = FALSE;
    OMAP_DSS_DESTINATION    eDest;
    DWORD                   dwNumPipelinesOn = 0;
    DWORD                   dwDestEnable,
                            dwDestGo;

    //  Check if pipeline is already enabled
    if( g_rgPipelineMapping[ePipeline].bEnabled == TRUE )
        return TRUE;
        
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Enable GFX pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        //  Enable the interrupt for reporting the GFX under flow error
        SETREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_GFXFIFOUNDERFLOW);

        //  Enable the pipeline
        SETREG32( &m_pDispRegs->DISPC_GFX_ATTRIBUTES, DISPC_GFX_ATTR_GFXENABLE );        
        g_rgPipelineMapping[ePipeline].bEnabled = TRUE;
    }    

    //  Enable VID1 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        // Enable the interrupt for reporting the VID1 under flow error
        SETREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VID1FIFOUNDERFLOW);

        //  Enable the pipeline
        SETREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES, DISPC_VID_ATTR_VIDENABLE );
        g_rgPipelineMapping[ePipeline].bEnabled = TRUE;
    }    

    //  Enable VID2 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        // Enable the interrupt for reporting the VID2 under flow error
        SETREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VID2FIFOUNDERFLOW);

        //  Enable the pipeline
        SETREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES, DISPC_VID_ATTR_VIDENABLE );
        g_rgPipelineMapping[ePipeline].bEnabled = TRUE;
    }    


    //  Count the number of pipelines that will be on
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled) ? 1 : 0;
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled) ? 1 : 0;
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled) ? 1 : 0;


    //  If there is only one pipeline enabled, use FIFO merge to make 1 large FIFO
    //  for better power management
    if( dwNumPipelinesOn == 1 )
    {
        //  Enable FIFO merge
        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_FIFOMERGE );

        //  Adjust the FIFO high and low thresholds for all the enabled pipelines
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD,
                        DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
        }
    }
    else
    {
        //  Disable FIFO merge
        CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_FIFOMERGE );

        //  Adjust the FIFO high and low thresholds for all the enabled pipelines
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD,
                        DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }
    }


    //  Get the destination for the pipeline
    eDest = g_rgPipelineMapping[ePipeline].eDestination;
    switch( eDest )
    {
        case OMAP_DSS_DESTINATION_LCD:
            //  Set enable and go bits for LCD
            dwDestEnable = DISPC_CONTROL_LCDENABLE;
            dwDestGo     = DISPC_CONTROL_GOLCD;
            break;

        case OMAP_DSS_DESTINATION_TVOUT:
            //  Set enable and go bits for TV Out
            dwDestEnable = (m_bTVEnable) ? DISPC_CONTROL_DIGITALENABLE : 0;
            dwDestGo     = DISPC_CONTROL_GODIGITAL;
            break;

        default:
            ASSERT(0);
            goto cleanUp;
    }
        
    //  Try enabling overlay optimization
    EnableOverlayOptimization( TRUE );

    //  Flush the shadow registers
    FlushRegs( dwDestGo );


    //  If the destination for pipeline is not enabled, enable it
    if( g_dwDestinationRefCnt[eDest]++ == 0 )
    {
        if (eDest == OMAP_DSS_DESTINATION_LCD)
        {
            SETREG32( &m_pDispRegs->DISPC_CONTROL, dwDestEnable );
        }
        else
        {
            // For TVOUT enable, the SYNCLOST_DIGITAL interrupt
            // has to be cleared at the 1st EVSYNC after DIGITALENABLE

            DWORD irqStatus, irqEnable;

            irqEnable = INREG32( &m_pDispRegs->DISPC_IRQENABLE );
            // Disable all the DSS interrupts
            OUTREG32( &m_pDispRegs->DISPC_IRQENABLE , 0 );
            // Clear the Existing IRQ status
            OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, 0xFFFFFFFF );

            // Enable the DIGITAL Path
            SETREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );

            // Wait for E-VSYNC
            WaitForIRQ( DISPC_IRQSTATUS_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD );
            
            // Clear the pending interrupt status
            irqStatus = INREG32( &m_pDispRegs->DISPC_IRQSTATUS );
            OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS,  irqStatus );

            // Re-enable the DSS interrupts
            OUTREG32( &m_pDispRegs->DISPC_IRQENABLE , irqEnable );
        }
    }

    // Configure the LPR mode based on active Pipeline(s)
    BOOL bEnable = ((dwNumPipelinesOn == 1) && 
                   (g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled)) ?
                   TRUE : FALSE; 
    EnableLPR( bEnable );

    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::DisablePipeline(
    OMAP_DSS_PIPELINE       ePipeline
    )
{
    BOOL                    bResult = FALSE;    
    OMAP_DSS_DESTINATION    eDest;
    DWORD                   dwNumPipelinesOn = 0;
    DWORD                   dwIntrStatus;
    DWORD                   dwDestEnable,
                            dwDestGo;
    BOOL                    bLPRState = FALSE;    

    //  Check if pipeline is already disabled
    if( g_rgPipelineMapping[ePipeline].bEnabled == FALSE )
        return TRUE;
        
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

     // Clear GO_XXX bit if it current enabled. The attributes register for 
    // the pipeline is currently modified and so it is required to turn off
    // the GOLCD/GODIGITAL bit during the configuration.
    if ((INREG32( &m_pDispRegs->DISPC_CONTROL) & DISPC_CONTROL_GOLCD ) != 0)
        CLRREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_GOLCD );

    if ((INREG32( &m_pDispRegs->DISPC_CONTROL) & DISPC_CONTROL_GODIGITAL ) != 0)
        CLRREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_GODIGITAL );

    //  Disable GFX pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        //  Disable the pipeline
        CLRREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_GFXFIFOUNDERFLOW);
        CLRREG32( &m_pDispRegs->DISPC_GFX_ATTRIBUTES, DISPC_GFX_ATTR_GFXENABLE );        
        g_rgPipelineMapping[ePipeline].bEnabled = FALSE;
        g_rgPipelineMapping[ePipeline].bMirror = FALSE;
    }    

    //  Disable VID1 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        //  Disable the pipeline
        CLRREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VID1FIFOUNDERFLOW);
        CLRREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES, DISPC_VID_ATTR_VIDENABLE );
        g_rgPipelineMapping[ePipeline].bEnabled = FALSE;
        g_rgPipelineMapping[ePipeline].bMirror = FALSE;
    }    

    //  Disable VID2 pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        //  Disable the pipeline
        CLRREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VID2FIFOUNDERFLOW);
        CLRREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES, DISPC_VID_ATTR_VIDENABLE );
        g_rgPipelineMapping[ePipeline].bEnabled = FALSE;
        g_rgPipelineMapping[ePipeline].bMirror = FALSE;
    }    

    //  Count the number of pipelines that will be on
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled) ? 1 : 0;
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled) ? 1 : 0;
    dwNumPipelinesOn += (g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled) ? 1 : 0;


    //  If there is only one pipeline enabled, use FIFO merge to make 1 large FIFO
    //  for better power management
    if( dwNumPipelinesOn == 1 )
    {
        //  Enable FIFO merge
        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_FIFOMERGE );

        //  Adjust the FIFO high and low thresholds for all the enabled pipelines
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD,
                        DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
            bLPRState = TRUE;
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED)
                        );
        }


    }
    else
    {

        //  Disable FIFO merge
        CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_FIFOMERGE );

        //  Adjust the FIFO high and low thresholds for all the enabled pipelines
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD,
                        DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }

        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD,
                        DISPC_VID_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_NORMAL(FIFO_BURSTSIZE_16x32)) |
                        DISPC_VID_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_NORMAL)
                        );
        }

    }

    //  Get the destination for the pipeline
    eDest = g_rgPipelineMapping[ePipeline].eDestination;
    switch( eDest )
    {
        case OMAP_DSS_DESTINATION_LCD:
            //  Set enable and go bits for LCD
            dwDestEnable = DISPC_CONTROL_LCDENABLE;
            dwDestGo     = DISPC_CONTROL_GOLCD;
            break;

        case OMAP_DSS_DESTINATION_TVOUT:
            //  Set enable and go bits for TV Out
            dwDestEnable = DISPC_CONTROL_DIGITALENABLE;
            dwDestGo     = DISPC_CONTROL_GODIGITAL;
            break;

        default:
            ASSERT(0);
            goto cleanUp;
    }
    
    //  Try enabling overlay optimization
    EnableOverlayOptimization( TRUE );

    // First turn on the GO bit corresponding to the pipeline
    // that has to be disabled and wait for GO bit to clear.
    FlushRegs( dwDestGo );     
    WaitForFlushDone( dwDestGo );

    // Additional flush required, when pipeline that is disabled in this
    // function was connected to DIGITALPATH. There could be another pipeline 
    // driving the LCD path and so the LCD path has to be flushed as well
    if ( (dwDestGo != DISPC_CONTROL_GOLCD) && (dwNumPipelinesOn > 0) )
        FlushRegs( DISPC_CONTROL_GOLCD );
    
    //  Update ref count on destination
    //  If ref count on destination is 0, disable output
    if( --g_dwDestinationRefCnt[eDest] == 0 )
        CLRREG32( &m_pDispRegs->DISPC_CONTROL, dwDestEnable );

    // clear any pending interrupts
    dwIntrStatus = INREG32 ( &m_pDispRegs->DISPC_IRQSTATUS );
    SETREG32( &m_pDispRegs->DISPC_IRQSTATUS, dwIntrStatus );

    // Configure the LPR mode based on active Pipeline(s)
    // If the pipeline count is 0, then LPR should be ON
    bLPRState = (dwNumPipelinesOn == 0) ? TRUE : bLPRState;
    EnableLPR( bLPRState );
    
    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::FlipPipeline(
    OMAP_DSS_PIPELINE       ePipeline,
    OMAPSurface*            pSurface
    )
{
    BOOL    bResult = FALSE;
    DWORD   dwDestGo;
    OMAP_DSS_ROTATION   eRotation;
    BOOL                bMirror;
    DWORD               dwInterlace;


    //  Check if pipeline is already enabled; if not, no reason to flip
    if( g_rgPipelineMapping[ePipeline].bEnabled == FALSE )
        return FALSE;

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

        
    //  Get rotation and mirror settings for pipeline output
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror   = g_rgPipelineMapping[ePipeline].bMirror;
    dwInterlace = g_rgPipelineScaling[ePipeline].dwInterlace;

    //Update clipping rectangle
#pragma warning(push)
#pragma warning(disable:4238)
    pSurface->SetClipping(&(g_rgPipelineMapping[ePipeline].pSurface->GetClipping()));    

    /* check for resizer */
    if (pSurface->isResizerEnabled())
    {
        if (pSurface->OmapAssocSurface() == NULL)
        {
            //  Allocate OMAP surface
            OMAPSurface*    pAssocOmapSurface;
            bResult = m_pSurfaceMgr->Allocate(
                            pSurface->PixelFormat(),
                            pSurface->Width(),
                            pSurface->Height(),
                            &pAssocOmapSurface,
                            pSurface );
            if (bResult == FALSE)
            {
                RETAILMSG(1, (L"FlipPipeline: surfaceMgr Allocate - Out of Video Memory\n"));                
                pSurface->SetRSZHandle(NULL,TRUE);
                pSurface->UseResizer(FALSE);
                memset(pSurface->ResizeParams(),0,sizeof(RSZParams_t));
            }
        }
        pSurface->OmapAssocSurface()->SetClipping(
            &(g_rgPipelineMapping[ePipeline].pSurface->OmapAssocSurface()->GetClipping()));
        pSurface->StartResizer(pSurface->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror,OMAP_ASSOC_SURF_FORCE_OFF), //input
                               pSurface->OmapAssocSurface()->PhysicalAddr(OMAP_DSS_ROTATION_0, bMirror,OMAP_ASSOC_SURF_FORCE_OFF));            
    }
#pragma warning(pop)    

    //  Update GFX pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        OUTREG32( &m_pDispRegs->DISPC_GFX_BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_BA1, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
    }    

    //  Update VID1 pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pSurface->PhysicalAddr(eRotation, bMirror)+ dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );        
    }    

    //  Update VID2 pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );
    }    

        
    //  Get the destination for the pipeline
    switch( g_rgPipelineMapping[ePipeline].eDestination )
    {
        case OMAP_DSS_DESTINATION_LCD:
            //  Set go bit for LCD
            dwDestGo  = DISPC_CONTROL_GOLCD;
            break;

        case OMAP_DSS_DESTINATION_TVOUT:
            //  Set go bit for TV Out
            dwDestGo  = DISPC_CONTROL_GODIGITAL;
            break;

        default:
            ASSERT(0);
            goto cleanUp;
    }

    //  Flush the shadow registers
    FlushRegs( dwDestGo );

    
    //Clear Vysnc since we are about to flip (to avoid false signaling Vsync event)
    SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_VSYNC);
    ResetEvent(m_hVsyncEvent);
    
    if(m_bTVEnable == TRUE)
    {
        SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_EVEN);
        SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_ODD);        
    }
    
    //  Update mapping of pipeline surface    
    g_rgPipelineMapping[ePipeline].pOldSurface = g_rgPipelineMapping[ePipeline].pSurface;
    g_rgPipelineMapping[ePipeline].pSurface  = pSurface;
    
    //  Set the decimation factors for the surface back to normal
    //  Leave the clipping setting as is
    pSurface->SetHorizontalScaling( 1 );
    pSurface->SetVerticalScaling( 1 );


    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::IsPipelineFlipping(
    OMAP_DSS_PIPELINE       ePipeline,
    OMAPSurface*            pSurface,
    BOOL                    matchExactSurface
    )
{
    BOOL                bResult = FALSE;
    OMAP_DSS_ROTATION   eRotation;
    BOOL                bMirror;
    DWORD               dwDestGo = DISPC_CONTROL_GOLCD;
    
    BOOL                bDestGoStatus = FALSE;

     //  Check if pipeline is already enabled; if not, no reason to query flip status
    if( g_rgPipelineMapping[ePipeline].bEnabled == FALSE )
        return FALSE;

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    //  Get rotation and mirror settings for pipeline output
    eRotation   = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror     = g_rgPipelineMapping[ePipeline].bMirror;

    //  Get the destination for the pipeline
    switch( g_rgPipelineMapping[ePipeline].eDestination )
    {
        case OMAP_DSS_DESTINATION_LCD:
            //  Set go bit for LCD
            dwDestGo  = DISPC_CONTROL_GOLCD;
            break;

        case OMAP_DSS_DESTINATION_TVOUT:
            //  Set go bit for TV Out
            dwDestGo  = DISPC_CONTROL_GODIGITAL;
            break;
    }

    //Test if we have already flipped (destGo has been cleared)
    bDestGoStatus = ((INREG32(&m_pDispRegs->DISPC_CONTROL) & dwDestGo) == dwDestGo);
    
    if (!matchExactSurface)
    {
        /* return value based on Go bit only */
        bResult=bDestGoStatus;
        goto cleanUp;
    }

    //See if we are flipping to the surface given
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        bResult = (INREG32(&m_pDispRegs->DISPC_GFX_BA0) == pSurface->PhysicalAddr(eRotation, bMirror));            
    }

    //  Update VID1 pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        bResult = INREG32(&m_pDispRegs->tDISPC_VID1.BA1) == pSurface->PhysicalAddr(eRotation, bMirror);
    }

    //  Update VID2 pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        bResult = INREG32(&m_pDispRegs->tDISPC_VID2.BA1) == pSurface->PhysicalAddr(eRotation, bMirror);
    }
    
    //Test if we have are flipping (destGo has not been cleared)
    if(bDestGoStatus)
    {       
        /* Since destGo is pending the shadow register is different than actual BA0 being DMA'ed out.
           So compare the surfaces. The g_rgPipelineMapping.pOldSurface stores the actual surface being DMA'ed out if the GO Bit is not cleared*/        
        bResult |= (g_rgPipelineMapping[ePipeline].pOldSurface == pSurface);
        /* when vsync interrupt is forced enable by SGX, then there may be old m_hVsyncEvent set for previous vsync interrupt before the
                  Go bit was set. In short, if we dont reset event here, then there is chance that in WaitForVsync code we may return immediately
                  even though GO bit is still set (i.e. Flipping is still not complete) and application can get false indication resulting in screen tearing */
        ResetEvent(m_hVsyncEvent);
    }       

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;

}
//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::MovePipeline(
    OMAP_DSS_PIPELINE       ePipeline,
    LONG                    lXPos,
    LONG                    lYPos
    )
{
    BOOL    bResult = FALSE;
    DWORD   dwDestGo = DISPC_CONTROL_GOLCD;
    DWORD   dwX, dwY;
    

    //  Check if pipeline is enabled; ignore operation if not
    if( g_rgPipelineMapping[ePipeline].bEnabled == FALSE )
        return FALSE;

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    //  Compute new origin based on pipeline rotation angle
    switch( g_rgPipelineMapping[ePipeline].eRotation )
    {
        case OMAP_DSS_ROTATION_0:
            dwX = lXPos;
            dwY = lYPos;
            break;
            
        case OMAP_DSS_ROTATION_90:
            dwX = lYPos;
            dwY = GetLCDHeight() - g_rgPipelineMapping[ePipeline].dwDestHeight - lXPos;
            break;
            
        case OMAP_DSS_ROTATION_180:
            dwX = GetLCDWidth() - g_rgPipelineMapping[ePipeline].dwDestWidth - lXPos;
            dwY = GetLCDHeight() - g_rgPipelineMapping[ePipeline].dwDestHeight - lYPos;
            break;
            
        case OMAP_DSS_ROTATION_270:
            dwX = GetLCDWidth() - g_rgPipelineMapping[ePipeline].dwDestWidth - lYPos;
            dwY = lXPos;
            break;

        default:
            ASSERT(0);
            goto cleanUp;
    }


    //  Update GFX pipeline display position
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        OUTREG32( &m_pDispRegs->DISPC_GFX_POSITION,
                    DISPC_GFX_POS_GFXPOSX(dwX) |
                    DISPC_GFX_POS_GFXPOSY(dwY)
                    );
    }

    //  Update VID1 pipeline display position
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID1.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );
    }

    //  Update VID2 pipeline display position
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID2.POSITION,
                    DISPC_VID_POS_VIDPOSX(dwX) |
                    DISPC_VID_POS_VIDPOSY(dwY)
                    );
    }


    //  Get the destination for the pipeline
    switch( g_rgPipelineMapping[ePipeline].eDestination )
    {
        case OMAP_DSS_DESTINATION_LCD:
            //  Set go bit for LCD
            dwDestGo  = DISPC_CONTROL_GOLCD;
            break;

        case OMAP_DSS_DESTINATION_TVOUT:
            //  Set go bit for TV Out
            dwDestGo  = DISPC_CONTROL_GODIGITAL;
            break;
    }


    //  Enable/update overlay optimization
    EnableOverlayOptimization( TRUE );

    //  Flush the shadow registers
    FlushRegs( dwDestGo );


    //  Result
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::RotatePipeline(
    OMAP_DSS_PIPELINE       ePipeline,
    OMAP_DSS_ROTATION       eRotation
    )
{
    BOOL    bResult = FALSE;
    OMAPSurface*        pSurface;
    BOOL                bMirror = FALSE;
    DWORD               dwVidRotation = 0;
    DWORD               dwHorzDecimation = 1;
    DWORD               dwVertDecimation = 1;
    DWORD               dwInterlace = 0;
    
    
    //  If no change in the rotation, do nothing
    if( g_rgPipelineMapping[ePipeline].eRotation == eRotation )
        return TRUE;

    //  If no associated pipeline, just set the default rotation of the pipeline
    if( g_rgPipelineMapping[ePipeline].pSurface == NULL )
    {
        g_rgPipelineMapping[ePipeline].eRotation = eRotation;
        return TRUE;
    }


    //  Get the surface being output
    pSurface = g_rgPipelineMapping[ePipeline].pSurface;
    bMirror = g_rgPipelineMapping[ePipeline].bMirror;

    //  Get the decimation settings for the surface
    dwHorzDecimation = g_rgPipelineScaling[ePipeline].dwHorzScaling;
    dwVertDecimation = g_rgPipelineScaling[ePipeline].dwVertScaling;
    dwInterlace      = g_rgPipelineScaling[ePipeline].dwInterlace;


    //  Set rotation attributes for video pipelines if pixel format is YUV
    if( pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_YUV2 ||    
        pSurface->PixelFormat() == OMAP_DSS_PIXELFORMAT_UYVY )  
    {
        //  Depending on rotation and mirror settings, change the VID rotation attributes
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Settings for rotation angle 0
                dwVidRotation = (bMirror) ? DISPC_VID_ATTR_VIDROTATION_180 : DISPC_VID_ATTR_VIDROTATION_0;

                //  Set the decimation for the surface
                pSurface->SetHorizontalScaling( dwHorzDecimation );
                pSurface->SetVerticalScaling( dwVertDecimation );
                break;

            case OMAP_DSS_ROTATION_90:
                //  Settings for rotation angle 90 (270 for DSS setting)
                dwVidRotation = DISPC_VID_ATTR_VIDROTATION_270 | DISPC_VID_ATTR_VIDROWREPEATENABLE;

                //  Set the decimation for the surface
                pSurface->SetHorizontalScaling( dwVertDecimation );
                pSurface->SetVerticalScaling( dwHorzDecimation );
                break;

            case OMAP_DSS_ROTATION_180:
                //  Settings for rotation angle 180
                dwVidRotation = (bMirror) ? DISPC_VID_ATTR_VIDROTATION_0 : DISPC_VID_ATTR_VIDROTATION_180;

                //  Set the decimation for the surface
                pSurface->SetHorizontalScaling( dwHorzDecimation );
                pSurface->SetVerticalScaling( dwVertDecimation );
                break;

            case OMAP_DSS_ROTATION_270:
                //  Settings for rotation angle 270 (90 for DSS setting)
                dwVidRotation = DISPC_VID_ATTR_VIDROTATION_90 | DISPC_VID_ATTR_VIDROWREPEATENABLE;

                //  Set the decimation for the surface
                pSurface->SetHorizontalScaling( dwVertDecimation );
                pSurface->SetVerticalScaling( dwHorzDecimation );
                break;

            default:
                ASSERT(0);
                return FALSE;
        }
    }


    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Update GFX pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        OUTREG32( &m_pDispRegs->DISPC_GFX_PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->DISPC_GFX_BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_BA1, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
    }

    //  Update VID1 pipeline display base address and attributes for rotation
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        CLRREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES, DISPC_VID_ATTR_VIDROTATION_MASK|DISPC_VID_ATTR_VIDROWREPEATENABLE );
        SETREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES, dwVidRotation );
        
        OUTREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );
       
    }

    //  Update VID2 pipeline display base address and attributes for rotation
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        CLRREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES, DISPC_VID_ATTR_VIDROTATION_MASK|DISPC_VID_ATTR_VIDROWREPEATENABLE );
        SETREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES, dwVidRotation );
        
        OUTREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );
    }


    //  Update pipeline output rotation
    g_rgPipelineMapping[ePipeline].eRotation = eRotation;

    //  Set the decimation factors for the surface back to normal
    pSurface->SetHorizontalScaling( 1 );
    pSurface->SetVerticalScaling( 1 );


    //  Result
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::MirrorPipeline(
    OMAP_DSS_PIPELINE       ePipeline,
    BOOL                    bMirror
    )
{
    BOOL    bResult = FALSE;
    OMAPSurface*        pSurface;
    OMAP_DSS_ROTATION   eRotation;
    DWORD               dwHorzDecimation = 1;
    DWORD               dwVertDecimation = 1;
    DWORD               dwInterlace = 0;
    
    
    //  If no change in the mirror setting, do nothing
    if( g_rgPipelineMapping[ePipeline].bMirror == bMirror )
        return TRUE;

    //  If no associated pipeline, just set the default mirror setting of the pipeline
    if( g_rgPipelineMapping[ePipeline].pSurface == NULL )
    {
        g_rgPipelineMapping[ePipeline].bMirror = bMirror;
        return TRUE;
    }

    //  Get the surface being output
    pSurface = g_rgPipelineMapping[ePipeline].pSurface;
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;

    //  Get the decimation settings for the surface
    dwHorzDecimation = g_rgPipelineScaling[ePipeline].dwHorzScaling;
    dwVertDecimation = g_rgPipelineScaling[ePipeline].dwVertScaling;
    dwInterlace      = g_rgPipelineScaling[ePipeline].dwInterlace;


    //  Depending on rotation settings, change the surface scaling attributes
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  Set the decimation for the surface
            pSurface->SetHorizontalScaling( dwHorzDecimation );
            pSurface->SetVerticalScaling( dwVertDecimation );
            break;

        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  Set the decimation for the surface
            pSurface->SetHorizontalScaling( dwVertDecimation );
            pSurface->SetVerticalScaling( dwHorzDecimation );
            break;

        default:
            ASSERT(0);
            return FALSE;
    }

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Update GFX pipeline for mirror setting
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        OUTREG32( &m_pDispRegs->DISPC_GFX_PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->DISPC_GFX_BA0, pSurface->PhysicalAddr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->DISPC_GFX_BA1, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
    }

    //  Update VID1 pipeline for mirror setting
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO1 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );

    }

    //  Update VID2 pipeline for mirror setting
    if( ePipeline == OMAP_DSS_PIPELINE_VIDEO2 )
    {
        OUTREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC, pSurface->PixelIncr(eRotation, bMirror) );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC, pSurface->RowIncr(eRotation, bMirror) );

        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pSurface->PhysicalAddr(eRotation, bMirror) + dwInterlace );
        OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pSurface->PhysicalAddr(eRotation, bMirror) );
    }


    //  Update pipeline output mirror setting
    g_rgPipelineMapping[ePipeline].bMirror = bMirror;

    //  Set the decimation factors for the surface back to normal
    pSurface->SetHorizontalScaling( 1 );
    pSurface->SetVerticalScaling( 1 );


    //  Result
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::UpdateScalingAttribs(
    OMAP_DSS_PIPELINE       ePipeline,
    RECT*                   pSrcRect,
    RECT*                   pDestRect
    )
{
    BOOL    bResult = FALSE;


    //  Check if pipeline is enabled; ignore operation if not
    if( g_rgPipelineMapping[ePipeline].bEnabled == FALSE )
        return FALSE;

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    
    //  Update the scaling attributes
    bResult = SetScalingAttribs( ePipeline, pSrcRect, pDestRect );
    if( bResult )
    {
        DWORD dwDestGo = 0;
    
        //  Get the destination for the pipeline
        switch( g_rgPipelineMapping[ePipeline].eDestination )
        {
            case OMAP_DSS_DESTINATION_LCD:
                //  Set go bit for LCD
                dwDestGo  = DISPC_CONTROL_GOLCD;
                break;

            case OMAP_DSS_DESTINATION_TVOUT:
                //  Set go bit for TV Out
                dwDestGo  = DISPC_CONTROL_GODIGITAL;
                break;

            default:
                ASSERT(0);
                goto cleanUp;
        }

        //  Enable/update overlay optimization
        EnableOverlayOptimization( TRUE );

        //  Flush the shadow registers
        FlushRegs( dwDestGo );
    }    


cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnableColorKey(
    OMAP_DSS_COLORKEY       eColorKey,
    OMAP_DSS_DESTINATION    eDestination,
    DWORD                   dwColor
    )
{
    BOOL    bResult = FALSE;
    DWORD   dwCurrentColor = 0;
    DWORD   dwDestGo = 0;


    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    
    //  Enable color key for the LCD
    if( eDestination == OMAP_DSS_DESTINATION_LCD )
    {
        //  Set color key for LCD
        switch( eColorKey )
        {
            case OMAP_DSS_COLORKEY_TRANS_SOURCE:
            case OMAP_DSS_COLORKEY_TRANS_DEST:
                //  Set transparent color for LCD
                OUTREG32( &m_pDispRegs->DISPC_TRANS_COLOR0, dwColor );
                
                //  Enable LCD transparent color blender
                SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKLCDENABLE );

                //  Select source or destination transparency
                if( eColorKey == OMAP_DSS_COLORKEY_TRANS_SOURCE )
                    SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKLCDSELECTION );
                else
                    CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKLCDSELECTION );
                
                break;

                
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX:
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2:
                //  Get current color (both LCD and VID2 are in this single register)
                dwCurrentColor = INREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA );
                
                //  Set global alpha color for LCD
                if( eColorKey == OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX )
                    dwColor = (dwCurrentColor & 0xFFFF0000) | DISPC_GLOBAL_ALPHA_GFX(dwColor);
                else
                    dwColor = (dwCurrentColor & 0x0000FFFF) | DISPC_GLOBAL_ALPHA_VID2(dwColor);

                OUTREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA, dwColor );
                
                //  Enable LCD alpha blender
                SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_LCDALPHABLENDERENABLE );
                break;

            default:
                ASSERT(0);
                goto cleanUp;
        }

        //  Display overlay optimization
        EnableOverlayOptimization( FALSE );

        //  Set dest GO
        dwDestGo = DISPC_CONTROL_GOLCD;
    }


    //  Enable color key for TV out
    if( eDestination == OMAP_DSS_DESTINATION_TVOUT )
    {
        //  Set color key for TV out
        switch( eColorKey )
        {
            case OMAP_DSS_COLORKEY_TRANS_SOURCE:
            case OMAP_DSS_COLORKEY_TRANS_DEST:
                //  Set transparent color for DIG
                OUTREG32( &m_pDispRegs->DISPC_TRANS_COLOR1, dwColor );
                
                //  Enable DIG transparent color blender
                SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKDIGENABLE );

                //  Select source or destination transparency
                if( eColorKey == OMAP_DSS_COLORKEY_TRANS_SOURCE )
                    SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKDIGSELECTION );
                else
                    CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKDIGSELECTION );
                
                break;

                
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX:
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2:
                //  Get current color (both LCD and VID2 are in this single register)
                dwCurrentColor = INREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA );
                
                //  Set global alpha color
                if( eColorKey == OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX )
                    dwColor = (dwCurrentColor & 0xFFFF0000) | DISPC_GLOBAL_ALPHA_GFX(dwColor);
                else
                    dwColor = (dwCurrentColor & 0x0000FFFF) | DISPC_GLOBAL_ALPHA_VID2(dwColor);

                OUTREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA, dwColor );
                
                //  Enable DIG alpha blender
                SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_DIGALPHABLENDERENABLE );
                break;

            default:
                ASSERT(0);
                goto cleanUp;
        }

        //  Set dest GO
        dwDestGo = DISPC_CONTROL_GODIGITAL;
    }

    
    //  Flush the shadow registers
    FlushRegs( dwDestGo );

    
    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::DisableColorKey(
    OMAP_DSS_COLORKEY       eColorKey,
    OMAP_DSS_DESTINATION    eDestination
    )
{
    BOOL    bResult = FALSE;
    DWORD   dwDestGo = 0;


    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    
    //  Enable color key for the LCD
    if( eDestination == OMAP_DSS_DESTINATION_LCD )
    {
        //  Set color key for LCD
        switch( eColorKey )
        {
            case OMAP_DSS_COLORKEY_TRANS_SOURCE:
            case OMAP_DSS_COLORKEY_TRANS_DEST:
                //  Disable LCD transparent color blender
                CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKLCDENABLE );
                break;

                
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX:
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2:
                //  Disable LCD alpha blender
                CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_LCDALPHABLENDERENABLE );
                break;

            default:
                ASSERT(0);
                goto cleanUp;
        }
        
        //  Set dest GO
        dwDestGo = DISPC_CONTROL_GOLCD;
    }


    //  Enable color key for TV out
    if( eDestination == OMAP_DSS_DESTINATION_TVOUT )
    {
        //  Set color key for TV out
        switch( eColorKey )
        {
            case OMAP_DSS_COLORKEY_TRANS_SOURCE:
            case OMAP_DSS_COLORKEY_TRANS_DEST:
                //  Disable DIG transparent color blender
                CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_TCKDIGENABLE );
                break;

                
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX:
            case OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2:
                //  Disable DIG alpha blender
                CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_DIGALPHABLENDERENABLE );
                break;

            default:
                ASSERT(0);
                goto cleanUp;
        }

        //  Set dest GO
        dwDestGo = DISPC_CONTROL_GODIGITAL;
    }

    
    //  Flush the shadow registers
    FlushRegs( dwDestGo );

    
    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetContrastLevel(
    DWORD dwContrastLevel
    )
{
    //  Set contrast level by copying in new gamma correction curve
    m_dwContrastLevel = (dwContrastLevel < NUM_CONTRAST_LEVELS) ? dwContrastLevel : NUM_CONTRAST_LEVELS - 1;

    //  Copy the selected table to the gamma physical memory location
    memcpy(m_pGammaBufVirt, &(g_dwGammaTable[(NUM_CONTRAST_LEVELS - 1) - m_dwContrastLevel][0]), NUM_GAMMA_VALS*sizeof(DWORD));
    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SaveRegisters(
    OMAP_DSS_DESTINATION eDestination
    )
{
    BOOL    bResult = FALSE;
    DWORD   i;
    OMAP_DISPC_REGS   *pDisplaySaveRestore = 0;
    
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    // Choose the last active LCD context ( internal LCD/external LCD)
    if (!m_bHDMIEnable)
        pDisplaySaveRestore = &g_rgDisplaySaveRestore;
    else
        pDisplaySaveRestore = &g_rgDisplaySaveRestore_eLcd;

    if (pDisplaySaveRestore == NULL)
        goto cleanUp;

    // Save the DISPC common register contents 
    pDisplaySaveRestore->DISPC_CONFIG = INREG32 ( &m_pDispRegs->DISPC_CONFIG );
    pDisplaySaveRestore->DISPC_GLOBAL_ALPHA = INREG32 (&m_pDispRegs->DISPC_GLOBAL_ALPHA );
    pDisplaySaveRestore->DISPC_IRQENABLE = INREG32( &m_pDispRegs->DISPC_IRQENABLE );
    pDisplaySaveRestore->DISPC_TRANS_COLOR0 = INREG32( &m_pDispRegs->DISPC_TRANS_COLOR0 );
    pDisplaySaveRestore->DISPC_TRANS_COLOR1 = INREG32( &m_pDispRegs->DISPC_TRANS_COLOR1 );

    //  Save the DSS and LCD registers
    if( eDestination == OMAP_DSS_DESTINATION_LCD )
    {
        //  Save off GFX plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            pDisplaySaveRestore->DISPC_GFX_BA0 = INREG32( &m_pDispRegs->DISPC_GFX_BA0 );
            pDisplaySaveRestore->DISPC_GFX_BA1 = INREG32( &m_pDispRegs->DISPC_GFX_BA1 );
            pDisplaySaveRestore->DISPC_GFX_POSITION = INREG32( &m_pDispRegs->DISPC_GFX_POSITION );
            pDisplaySaveRestore->DISPC_GFX_SIZE = INREG32( &m_pDispRegs->DISPC_GFX_SIZE );
            pDisplaySaveRestore->DISPC_GFX_ATTRIBUTES = INREG32( &m_pDispRegs->DISPC_GFX_ATTRIBUTES );
            pDisplaySaveRestore->DISPC_GFX_FIFO_THRESHOLD = INREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD );
            pDisplaySaveRestore->DISPC_GFX_ROW_INC = INREG32( &m_pDispRegs->DISPC_GFX_ROW_INC );
            pDisplaySaveRestore->DISPC_GFX_PIXEL_INC = INREG32( &m_pDispRegs->DISPC_GFX_PIXEL_INC );
            pDisplaySaveRestore->DISPC_GFX_WINDOW_SKIP = INREG32( &m_pDispRegs->DISPC_GFX_WINDOW_SKIP );
        }

        //  Save off VID1 plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            pDisplaySaveRestore->tDISPC_VID1.BA0 = INREG32( &m_pDispRegs->tDISPC_VID1.BA0 );
            pDisplaySaveRestore->tDISPC_VID1.BA1 = INREG32( &m_pDispRegs->tDISPC_VID1.BA1 );
            pDisplaySaveRestore->tDISPC_VID1.POSITION = INREG32( &m_pDispRegs->tDISPC_VID1.POSITION );
            pDisplaySaveRestore->tDISPC_VID1.SIZE = INREG32( &m_pDispRegs->tDISPC_VID1.SIZE );
            pDisplaySaveRestore->tDISPC_VID1.ATTRIBUTES = INREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES );
            pDisplaySaveRestore->tDISPC_VID1.FIFO_THRESHOLD = INREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD );
            pDisplaySaveRestore->tDISPC_VID1.ROW_INC = INREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC );
            pDisplaySaveRestore->tDISPC_VID1.PIXEL_INC = INREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC );
            pDisplaySaveRestore->tDISPC_VID1.FIR = INREG32( &m_pDispRegs->tDISPC_VID1.FIR );
            pDisplaySaveRestore->tDISPC_VID1.PICTURE_SIZE = INREG32( &m_pDispRegs->tDISPC_VID1.PICTURE_SIZE );
            pDisplaySaveRestore->tDISPC_VID1.ACCU0 = INREG32( &m_pDispRegs->tDISPC_VID1.ACCU0 );
            pDisplaySaveRestore->tDISPC_VID1.ACCU1 = INREG32( &m_pDispRegs->tDISPC_VID1.ACCU1 );

            //  Scaling coefficients
            for( i = 0; i < NUM_SCALING_PHASES; i++ )
            {
                pDisplaySaveRestore->tDISPC_VID1.aFIR_COEF[i].ulH = INREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulH );
                pDisplaySaveRestore->tDISPC_VID1.aFIR_COEF[i].ulHV = INREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulHV );
                pDisplaySaveRestore->DISPC_VID1_FIR_COEF_V[i] = INREG32( &m_pDispRegs->DISPC_VID1_FIR_COEF_V[i] );
            }
        }
        
        //  Save off VID2 plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            pDisplaySaveRestore->tDISPC_VID2.BA0 = INREG32( &m_pDispRegs->tDISPC_VID2.BA0 );
            pDisplaySaveRestore->tDISPC_VID2.BA1 = INREG32( &m_pDispRegs->tDISPC_VID2.BA1 );
            pDisplaySaveRestore->tDISPC_VID2.POSITION = INREG32( &m_pDispRegs->tDISPC_VID2.POSITION );
            pDisplaySaveRestore->tDISPC_VID2.SIZE = INREG32( &m_pDispRegs->tDISPC_VID2.SIZE );
            pDisplaySaveRestore->tDISPC_VID2.ATTRIBUTES = INREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES );
            pDisplaySaveRestore->tDISPC_VID2.FIFO_THRESHOLD = INREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD );
            pDisplaySaveRestore->tDISPC_VID2.ROW_INC = INREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC );
            pDisplaySaveRestore->tDISPC_VID2.PIXEL_INC = INREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC );
            pDisplaySaveRestore->tDISPC_VID2.FIR = INREG32( &m_pDispRegs->tDISPC_VID2.FIR );
            pDisplaySaveRestore->tDISPC_VID2.PICTURE_SIZE = INREG32( &m_pDispRegs->tDISPC_VID2.PICTURE_SIZE );
            pDisplaySaveRestore->tDISPC_VID2.ACCU0 = INREG32( &m_pDispRegs->tDISPC_VID2.ACCU0 );
            pDisplaySaveRestore->tDISPC_VID2.ACCU1 = INREG32( &m_pDispRegs->tDISPC_VID2.ACCU1 );

            //  Scaling coefficients
            for( i = 0; i < NUM_SCALING_PHASES; i++ )
            {
                pDisplaySaveRestore->tDISPC_VID2.aFIR_COEF[i].ulH = INREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulH );
                pDisplaySaveRestore->tDISPC_VID2.aFIR_COEF[i].ulHV = INREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulHV );
                pDisplaySaveRestore->DISPC_VID2_FIR_COEF_V[i] = INREG32( &m_pDispRegs->DISPC_VID2_FIR_COEF_V[i] );
            }
        }
    }
    
    
    //  Success
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::RestoreRegisters(
    OMAP_DSS_DESTINATION eDestination
    )
{
    BOOL    bResult = FALSE;
    DWORD   i;
    OMAP_DISPC_REGS   *pDisplaySaveRestore = 0;
    
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    // Choose the last active LCD context ( internal LCD/external LCD)
    if (!m_bHDMIEnable)
        pDisplaySaveRestore = &g_rgDisplaySaveRestore;
    else
        pDisplaySaveRestore = &g_rgDisplaySaveRestore_eLcd;

    if (pDisplaySaveRestore == NULL)
        goto cleanUp;

        
    //  Restore the DSS and LCD registers
    if( eDestination == OMAP_DSS_DESTINATION_LCD )
    {
        //  Configure interconnect parameters
        OUTREG32( &m_pDSSRegs->DSS_SYSCONFIG, DISPC_SYSCONFIG_AUTOIDLE );
        OUTREG32( &m_pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_AUTOIDLE|SYSCONFIG_NOIDLE|SYSCONFIG_NOSTANDBY );

        //  Not enabling any interrupts
        OUTREG32( &m_pDispRegs->DISPC_IRQENABLE , pDisplaySaveRestore->DISPC_IRQENABLE);

        //  Initialize the LCD by calling PDD
        LcdPdd_LCD_Initialize(
            m_pDSSRegs,
            m_pDispRegs,
            NULL,
            m_pVencRegs);

        OUTREG32( &m_pDispRegs->DISPC_CONFIG, pDisplaySaveRestore->DISPC_CONFIG );

        //Enable/Disable Gamma correction
        if(m_bGammaEnable)
            SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_PALETTEGAMMATABLE );
        else
            CLRREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_PALETTEGAMMATABLE );

        // Load Gamma Table
        OUTREG32( &m_pDispRegs->DISPC_GFX_TABLE_BA, m_dwGammaBufPhys);

        //  Restore global alpha value
        OUTREG32( &m_pDispRegs->DISPC_GLOBAL_ALPHA, pDisplaySaveRestore->DISPC_GLOBAL_ALPHA );

        // Restore transparency value
        OUTREG32( &m_pDispRegs->DISPC_TRANS_COLOR0, pDisplaySaveRestore->DISPC_TRANS_COLOR0 );
        OUTREG32( &m_pDispRegs->DISPC_TRANS_COLOR1, pDisplaySaveRestore->DISPC_TRANS_COLOR1 );

        //  Restore GFX plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled )
        {
            OUTREG32( &m_pDispRegs->DISPC_GFX_BA0, pDisplaySaveRestore->DISPC_GFX_BA0 );
            OUTREG32( &m_pDispRegs->DISPC_GFX_BA1, pDisplaySaveRestore->DISPC_GFX_BA1 );
            OUTREG32( &m_pDispRegs->DISPC_GFX_POSITION, pDisplaySaveRestore->DISPC_GFX_POSITION );
            OUTREG32( &m_pDispRegs->DISPC_GFX_SIZE, pDisplaySaveRestore->DISPC_GFX_SIZE );
            OUTREG32( &m_pDispRegs->DISPC_GFX_ATTRIBUTES, pDisplaySaveRestore->DISPC_GFX_ATTRIBUTES );
            OUTREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD, pDisplaySaveRestore->DISPC_GFX_FIFO_THRESHOLD );
            OUTREG32( &m_pDispRegs->DISPC_GFX_ROW_INC, pDisplaySaveRestore->DISPC_GFX_ROW_INC );
            OUTREG32( &m_pDispRegs->DISPC_GFX_PIXEL_INC, pDisplaySaveRestore->DISPC_GFX_PIXEL_INC );
            OUTREG32( &m_pDispRegs->DISPC_GFX_WINDOW_SKIP, pDisplaySaveRestore->DISPC_GFX_WINDOW_SKIP );
        }


        //  Restore VID1 plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID1.BA0, pDisplaySaveRestore->tDISPC_VID1.BA0 );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.BA1, pDisplaySaveRestore->tDISPC_VID1.BA1 );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.POSITION, pDisplaySaveRestore->tDISPC_VID1.POSITION );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.SIZE, pDisplaySaveRestore->tDISPC_VID1.SIZE );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.ATTRIBUTES, pDisplaySaveRestore->tDISPC_VID1.ATTRIBUTES );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIFO_THRESHOLD, pDisplaySaveRestore->tDISPC_VID1.FIFO_THRESHOLD );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.ROW_INC, pDisplaySaveRestore->tDISPC_VID1.ROW_INC );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.PIXEL_INC, pDisplaySaveRestore->tDISPC_VID1.PIXEL_INC );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.FIR, pDisplaySaveRestore->tDISPC_VID1.FIR );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.PICTURE_SIZE, pDisplaySaveRestore->tDISPC_VID1.PICTURE_SIZE );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.ACCU0, pDisplaySaveRestore->tDISPC_VID1.ACCU0 );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.ACCU1, pDisplaySaveRestore->tDISPC_VID1.ACCU1 );

            //  Scaling coefficients
            for( i = 0; i < NUM_SCALING_PHASES; i++ )
            {
                OUTREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulH, pDisplaySaveRestore->tDISPC_VID1.aFIR_COEF[i].ulH );
                OUTREG32( &m_pDispRegs->tDISPC_VID1.aFIR_COEF[i].ulHV, pDisplaySaveRestore->tDISPC_VID1.aFIR_COEF[i].ulHV );
                OUTREG32( &m_pDispRegs->DISPC_VID1_FIR_COEF_V[i], pDisplaySaveRestore->DISPC_VID1_FIR_COEF_V[i] );
            }

            //  Color conversion coefficients
            OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF0, m_pColorSpaceCoeffs[0] );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF1, m_pColorSpaceCoeffs[1] );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF2, m_pColorSpaceCoeffs[2] );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF3, m_pColorSpaceCoeffs[3] );
            OUTREG32( &m_pDispRegs->tDISPC_VID1.CONV_COEF4, m_pColorSpaceCoeffs[4] );
        }

        //  Restore VID2 plane registers if enabled
        if( g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled )
        {
            OUTREG32( &m_pDispRegs->tDISPC_VID2.BA0, pDisplaySaveRestore->tDISPC_VID2.BA0 );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.BA1, pDisplaySaveRestore->tDISPC_VID2.BA1 );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.POSITION, pDisplaySaveRestore->tDISPC_VID2.POSITION );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.SIZE, pDisplaySaveRestore->tDISPC_VID2.SIZE );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.ATTRIBUTES, pDisplaySaveRestore->tDISPC_VID2.ATTRIBUTES );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIFO_THRESHOLD, pDisplaySaveRestore->tDISPC_VID2.FIFO_THRESHOLD );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.ROW_INC, pDisplaySaveRestore->tDISPC_VID2.ROW_INC );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.PIXEL_INC, pDisplaySaveRestore->tDISPC_VID2.PIXEL_INC );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.FIR, pDisplaySaveRestore->tDISPC_VID2.FIR );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.PICTURE_SIZE, pDisplaySaveRestore->tDISPC_VID2.PICTURE_SIZE );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.ACCU0, pDisplaySaveRestore->tDISPC_VID2.ACCU0 );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.ACCU1, pDisplaySaveRestore->tDISPC_VID2.ACCU1 );

            //  Scaling coefficients
            for( i = 0; i < NUM_SCALING_PHASES; i++ )
            {
                OUTREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulH, pDisplaySaveRestore->tDISPC_VID2.aFIR_COEF[i].ulH );
                OUTREG32( &m_pDispRegs->tDISPC_VID2.aFIR_COEF[i].ulHV, pDisplaySaveRestore->tDISPC_VID2.aFIR_COEF[i].ulHV );
                OUTREG32( &m_pDispRegs->DISPC_VID2_FIR_COEF_V[i], pDisplaySaveRestore->DISPC_VID2_FIR_COEF_V[i] );
            }

            //  Color conversion coefficients
            OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF0, m_pColorSpaceCoeffs[0] );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF1, m_pColorSpaceCoeffs[1] );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF2, m_pColorSpaceCoeffs[2] );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF3, m_pColorSpaceCoeffs[3] );
            OUTREG32( &m_pDispRegs->tDISPC_VID2.CONV_COEF4, m_pColorSpaceCoeffs[4] );
        }

    }

    //  Restore the TV out registers
    //  TV regs are not saved off b/c most are set to defaults
    if( eDestination == OMAP_DSS_DESTINATION_TVOUT )
    {
        DWORD*  pVencPtr = NULL;

        //  Initialize the TV by calling PDD
        bResult = LcdPdd_TV_Initialize(
                        m_pDSSRegs,
                        m_pDispRegs,
                        NULL,
                        m_pVencRegs );

        //  Get TV parameters
        LcdPdd_TV_GetMode(
                        &m_dwTVWidth,
                        &m_dwTVHeight,
                        &m_dwTVMode );


        //  Initialize Video Encoder registers for NTSC or PAL based on size of mode
        //  Default to NTSC
        if( m_dwTVWidth == PAL_WIDTH && m_dwTVHeight == PAL_HEIGHT )
        {
            //  Set for PAL
            pVencPtr = g_dwVencValues_PAL;
            m_dwTVMode = (m_dwTVMode == 0) ?  g_dwVencValues_PAL[VENC_OUTPUT_CONTROL] : m_dwTVMode;
        }
        else
        {
            //  Set for NTSC
            pVencPtr = g_dwVencValues_NTSC;
            m_dwTVMode = (m_dwTVMode == 0) ?  g_dwVencValues_NTSC[VENC_OUTPUT_CONTROL] : m_dwTVMode;
        }

        OUTREG32( &m_pVencRegs->VENC_F_CONTROL, pVencPtr[VENC_F_CONTROL] );
        OUTREG32( &m_pVencRegs->VENC_SYNC_CTRL, (pVencPtr[VENC_SYNC_CTRL] | 0x00000040) );     
        //  Initialize encoder
        OUTREG32( &m_pVencRegs->VENC_VIDOUT_CTRL, pVencPtr[VENC_VIDOUT_CTRL] );
        OUTREG32( &m_pVencRegs->VENC_LLEN, pVencPtr[VENC_LLEN] );
        OUTREG32( &m_pVencRegs->VENC_FLENS, pVencPtr[VENC_FLENS] );
        OUTREG32( &m_pVencRegs->VENC_HFLTR_CTRL, pVencPtr[VENC_HFLTR_CTRL] );
        OUTREG32( &m_pVencRegs->VENC_CC_CARR_WSS_CARR, pVencPtr[VENC_CC_CARR_WSS_CARR] );
        OUTREG32( &m_pVencRegs->VENC_C_PHASE, pVencPtr[VENC_C_PHASE] );
        OUTREG32( &m_pVencRegs->VENC_GAIN_U, pVencPtr[VENC_GAIN_U] );
        OUTREG32( &m_pVencRegs->VENC_GAIN_V, pVencPtr[VENC_GAIN_V] );
        OUTREG32( &m_pVencRegs->VENC_GAIN_Y, pVencPtr[VENC_GAIN_Y] );
        OUTREG32( &m_pVencRegs->VENC_BLACK_LEVEL, pVencPtr[VENC_BLACK_LEVEL] );
        OUTREG32( &m_pVencRegs->VENC_BLANK_LEVEL, pVencPtr[VENC_BLANK_LEVEL] );
        OUTREG32( &m_pVencRegs->VENC_X_COLOR, pVencPtr[VENC_X_COLOR] );
        OUTREG32( &m_pVencRegs->VENC_M_CONTROL, pVencPtr[VENC_M_CONTROL] );
        OUTREG32( &m_pVencRegs->VENC_BSTAMP_WSS_DATA, pVencPtr[VENC_BSTAMP_WSS_DATA] );
        OUTREG32( &m_pVencRegs->VENC_S_CARR, pVencPtr[VENC_S_CARR] );
        OUTREG32( &m_pVencRegs->VENC_LINE21, pVencPtr[VENC_LINE21] );
        OUTREG32( &m_pVencRegs->VENC_LN_SEL, pVencPtr[VENC_LN_SEL] );
        OUTREG32( &m_pVencRegs->VENC_L21__WC_CTL, pVencPtr[VENC_L21__WC_CTL] );
        OUTREG32( &m_pVencRegs->VENC_HTRIGGER_VTRIGGER, pVencPtr[VENC_HTRIGGER_VTRIGGER] );
        OUTREG32( &m_pVencRegs->VENC_SAVID_EAVID, pVencPtr[VENC_SAVID_EAVID] );
        OUTREG32( &m_pVencRegs->VENC_FLEN_FAL, pVencPtr[VENC_FLEN_FAL] );
        OUTREG32( &m_pVencRegs->VENC_LAL_PHASE_RESET, pVencPtr[VENC_LAL_PHASE_RESET] );
        OUTREG32( &m_pVencRegs->VENC_HS_INT_START_STOP_X, pVencPtr[VENC_HS_INT_START_STOP_X] );
        OUTREG32( &m_pVencRegs->VENC_HS_EXT_START_STOP_X, pVencPtr[VENC_HS_EXT_START_STOP_X] );
        OUTREG32( &m_pVencRegs->VENC_VS_INT_START_X, pVencPtr[VENC_VS_INT_START_X] );
        OUTREG32( &m_pVencRegs->VENC_VS_INT_STOP_X__VS_INT_START_Y, pVencPtr[VENC_VS_INT_STOP_X__VS_INT_START_Y] );
        OUTREG32( &m_pVencRegs->VENC_VS_INT_STOP_Y__VS_EXT_START_X, pVencPtr[VENC_VS_INT_STOP_Y__VS_EXT_START_X] );
        OUTREG32( &m_pVencRegs->VENC_VS_EXT_STOP_X__VS_EXT_START_Y, pVencPtr[VENC_VS_EXT_STOP_X__VS_EXT_START_Y] );
        OUTREG32( &m_pVencRegs->VENC_VS_EXT_STOP_Y, pVencPtr[VENC_VS_EXT_STOP_Y] );
        OUTREG32( &m_pVencRegs->VENC_AVID_START_STOP_X, pVencPtr[VENC_AVID_START_STOP_X] );
        OUTREG32( &m_pVencRegs->VENC_AVID_START_STOP_Y, pVencPtr[VENC_AVID_START_STOP_Y] );
        OUTREG32( &m_pVencRegs->VENC_FID_INT_START_X__FID_INT_START_Y, pVencPtr[VENC_FID_INT_START_X__FID_INT_START_Y] );
        OUTREG32( &m_pVencRegs->VENC_FID_INT_OFFSET_Y__FID_EXT_START_X, pVencPtr[VENC_FID_INT_OFFSET_Y__FID_EXT_START_X] );
        OUTREG32( &m_pVencRegs->VENC_FID_EXT_START_Y__FID_EXT_OFFSET_Y, pVencPtr[VENC_FID_EXT_START_Y__FID_EXT_OFFSET_Y] );
        OUTREG32( &m_pVencRegs->VENC_TVDETGP_INT_START_STOP_X, pVencPtr[VENC_TVDETGP_INT_START_STOP_X] );
        OUTREG32( &m_pVencRegs->VENC_TVDETGP_INT_START_STOP_Y, pVencPtr[VENC_TVDETGP_INT_START_STOP_Y] );
        OUTREG32( &m_pVencRegs->VENC_GEN_CTRL, pVencPtr[VENC_GEN_CTRL] );
        OUTREG32( &m_pVencRegs->VENC_OUTPUT_CONTROL, m_dwTVMode );
        OUTREG32( &m_pVencRegs->VENC_OUTPUT_TEST, pVencPtr[VENC_OUTPUT_TEST] );

        OUTREG32( &m_pVencRegs->VENC_F_CONTROL, pVencPtr[VENC_F_CONTROL] );     // TRM mentions that these regs need to be
        OUTREG32( &m_pVencRegs->VENC_SYNC_CTRL, pVencPtr[VENC_SYNC_CTRL] );     // programmed last

        //  Flush shadow registers
        FlushRegs( DISPC_CONTROL_GODIGITAL );
    }

    //  Success
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetPowerLevel(
    DWORD dwPowerLevel
    )
{
    BOOL            bResult = TRUE;
    DWORD   dwTimeout;
    
    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );
    
    //  Check if there is a change in the power level
    if( m_dwPowerLevel == dwPowerLevel )
        goto cleanUp;

    //  Enable/disable devices based on power level
    switch( dwPowerLevel )
    {
        case D0:
        case D1: 
        case D2:
            //  Check against current level
            if( m_dwPowerLevel == D3 || m_dwPowerLevel == D4)
            {
                //  Set the new power level
                m_dwPowerLevel = dwPowerLevel;
            
                //  Enable device clocks
                RequestClock( m_dssinfo.DSSDevice );         

                //  Call PDD layer
                LcdPdd_SetPowerLevel( dwPowerLevel );

                //  Re-enable LCD outputs
                if( g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_LCD] > 0 )
                {
                    // The HDMI uses DSI clock. At init time, DSS is turned
                    // ON and so HDMI cannot be configured at init time.
                    // The HDMI config seq is given below
                    // init -> InternalLCD(DSS) -> HDMI(DSI)
                    
                    // Store the current HDMI state
                    BOOL bHdmiEnable = m_bHDMIEnable;
                    // Force HDMI to inactive state during init
                    m_bHDMIEnable = FALSE;

#if 0
					// Turn on the DSS2_ALWON_FCLK if the FCLK source is DSI clock
                    if ( m_eDssFclkSource == OMAP_DSS_FCLK_DSS2ALWON )
                    {
                        ULONG count = 2;
                        ULONG clockSrc[2] = {kDSS1_ALWON_FCLK, kDSS2_ALWON_FCLK};
                        SelectDSSSourceClocks( count, clockSrc);
                        InitDsiPll();
                    }
#endif
                    
                    //  Restore LCD settings
                    RestoreRegisters(OMAP_DSS_DESTINATION_LCD);

                    // enable interrupt for reporting SYNCLOST errors
                    SETREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_SYNCLOST);

                    LcdPdd_SetPowerLevel( dwPowerLevel );

                    // Check the FIFO threshold level and decide if LPR is required
                    DWORD dwFIFOThreshold = 
                        (DISPC_GFX_FIFO_THRESHOLD_LOW(FIFO_LOWTHRESHOLD_MERGED(FIFO_BURSTSIZE_16x32))|
                         DISPC_GFX_FIFO_THRESHOLD_HIGH(FIFO_HIGHTHRESHOLD_MERGED));
                    
                    BOOL bLPREnable = FALSE;
                        
                    // Enable the LPR if the FIFO's are merged.
                    if ( INREG32( &m_pDispRegs->DISPC_GFX_FIFO_THRESHOLD) == dwFIFOThreshold )
                    {
                        SETREG32( &m_pDispRegs->DISPC_CONFIG, DISPC_CONFIG_FIFOMERGE);
                        
                        // Make sure the LPR is disabled when HDMI is enabled
                        if (bHdmiEnable == FALSE)
                        {
                            // LPR should be turned ON
                            bLPREnable = TRUE;
                        }
                    }

                    // Toggle the LPR state
                    m_bLPREnable = ( m_bLPREnable == TRUE ) ? FALSE : TRUE;
                    // Turn on LPR and also switch to DSI clock (if clksource == DSI)
                    EnableLPR( bLPREnable );
                    
                    //  Flush shadow registers
                    FlushRegs( DISPC_CONTROL_GOLCD );
                    
                    if (bHdmiEnable)
                    {
                        WaitForFlushDone( DISPC_CONTROL_GOLCD );
                        // Restore the HDMI specific context
                        m_bHDMIEnable = TRUE;
                        RestoreRegisters( OMAP_DSS_DESTINATION_LCD );
                        // Configure and enable the Hdmi Panel 
                        EnableHdmi( TRUE );
                    }
                }
                
                //  Re-enable TV out if it was enabled prior to display power change
                if( m_bTVEnable )
                {
                    //  Enable the video encoder clock
                    RequestClock( m_dssinfo.TVEncoderDevice );

                    //  Restore the TV out registers
                    RestoreRegisters( OMAP_DSS_DESTINATION_TVOUT );

                    //  Enable TV out if there is something to show
                if( g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_TVOUT] > 0 )
                        {
                      // enable interrupt for reporting SYNCLOST errors
                        //SETREG32( &m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_SYNCLOSTDIGITAL);
                        // enable the tvout path
                        SETREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );
                }
                    }
                
                //  Wait for VSYNC
                dwTimeout = DISPLAY_TIMEOUT;
                OUTREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_VSYNC);
                while (((INREG32(&m_pDispRegs->DISPC_IRQSTATUS) & DISPC_IRQSTATUS_VSYNC) == 0) && (dwTimeout-- > 0))
                {
                    Sleep(1);
                }

                //  Clear all DSS interrupts
                OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, 0xFFFFFFFF );
            }
			else    
            {         
                LcdPdd_SetPowerLevel(dwPowerLevel);
                if (dwPowerLevel == D2)
                    CLRREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );
                else
                    SETREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );
			    m_dwPowerLevel = dwPowerLevel;
            }

        break;
        
        case D3:
        case D4:
            //  Check against current level
            if( m_dwPowerLevel == D0 || m_dwPowerLevel == D1 || m_dwPowerLevel == D2)
            {

                //  Disable TV out
                if( g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_TVOUT] > 0 )
                {
                    //  Disable TV out control
                    CLRREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );        

                    //  Wait for EVSYNC
                    WaitForIRQ(DISPC_IRQSTATUS_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD);

                    // clear all the pending interrupts
                    SETREG32( &m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD);

                    //  Save TV out settings
                    SaveRegisters(OMAP_DSS_DESTINATION_TVOUT);

                    //  Reset the video encoder
                    ResetVENC();                           

                    //  Release the clock
                    ReleaseClock( m_dssinfo.TVEncoderDevice );
                }

                //  Disable LCD
                if( g_dwDestinationRefCnt[OMAP_DSS_DESTINATION_LCD] > 0 )
                {
                    //  Save LCD settings
                    SaveRegisters(OMAP_DSS_DESTINATION_LCD);

                    //  Disable LCD
                    LcdPdd_SetPowerLevel( dwPowerLevel );
                    
                    //  Wait for the frame to complete
                    WaitForFrameDone();
                }
		else
	            //  Call PDD layer (in case LCD out was not enabled in DSS)
        	    LcdPdd_SetPowerLevel( dwPowerLevel );

                //  Clear all DSS interrupts
                OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, 0xFFFFFFFF );
                
                //  Change interconnect parameters to disable controller
                OUTREG32( &m_pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_AUTOIDLE|SYSCONFIG_FORCEIDLE|SYSCONFIG_FORCESTANDBY );

#if 0
				if ( m_eDssFclkSource == OMAP_DSS_FCLK_DSS2ALWON )
                {
                    // De-init the DSI Pll and Power Down the DSI PLL
                    DeInitDsiPll();
                    // Set clock to DSS1
                    ULONG count = 1;
                    ULONG clockSrc = kDSS1_ALWON_FCLK;
                    SelectDSSSourceClocks( count, &clockSrc);
                }
#endif

				//  Disable device clocks 
                ReleaseClock( m_dssinfo.DSSDevice );         
			}	
            
            //  Set the new power level
            m_dwPowerLevel = dwPowerLevel;

            break;            
    }

cleanUp:    
    //  Unlock access to power level
    LeaveCriticalSection( &m_csPowerLock );
                
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnableTvOut(
    BOOL bEnable
    )
{
    BOOL    bResult = FALSE;

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    if (bEnable == m_bTVEnable)
        goto cleanUp;

    //  Enable/disable TV out
    if( bEnable )
    {
        //  Enable TV out clock
        RequestClock( m_dssinfo.TVEncoderDevice );

        //  Restore the TV out registers
        RestoreRegisters( OMAP_DSS_DESTINATION_TVOUT );
            
        m_bTVEnable = TRUE;         
    }
    else
    {
        //  Disable TV out
        CLRREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_DIGITALENABLE );
        
        // Stop the VENC
        ResetVENC();
 
        // Wait for the EVSYNC
        WaitForIRQ(DISPC_IRQSTATUS_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD);

        // clear all the pending interrupts
        SETREG32( &m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_EVEN|DISPC_IRQSTATUS_EVSYNC_ODD);
        
        //  Disable TV out clock
        ReleaseClock( m_dssinfo.TVEncoderDevice );

        m_bTVEnable = FALSE;       
    }

    //  Success
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SetTvOutFilterLevel(
    DWORD dwTVFilterLevel
    )
{
    //  Set flicker filter level
    m_dwTVFilterLevel = dwTVFilterLevel;
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnableHdmi(
    BOOL bEnable
    )
{
    BOOL    bResult = FALSE;

    BOOL    bLPRState   = TRUE;
    
    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    //  Enable or disable HDMI output
    if( bEnable )
    {
        DWORD   dwX = (1280 - m_dwLcdWidth)/2,
                dwY = (720 - m_dwLcdHeight)/2;

        m_dwPixelClock = OMAP_DSS_FCLKVALUE_HDMI/2;
        
        //Save the configuration of internal LCD
        SaveRegisters( OMAP_DSS_DESTINATION_LCD );

        // Issue Power Down command to Internal LCD Panel
        LcdPdd_SetPowerLevel(D4);
        
        // Disable LPR and configure DSI for HDMI clk
        bLPRState = FALSE;
        EnableLPR( bLPRState, TRUE );

		//
        //  Configure the HDMI timing parameters
        //

        // Timing logic for HSYNC signal
        OUTREG32( &m_pDispRegs->DISPC_TIMING_H,
                    DISPC_TIMING_H_HSW(40) |
                    DISPC_TIMING_H_HFP(110) |
                    DISPC_TIMING_H_HBP(220)
                    );

        // Timing logic for VSYNC signal
        OUTREG32( &m_pDispRegs->DISPC_TIMING_V,
                    DISPC_TIMING_V_VSW(5) |
                    DISPC_TIMING_V_VFP(5) |
                    DISPC_TIMING_V_VBP(20)
                    );

        // Signal configuration
        OUTREG32( &m_pDispRegs->DISPC_POL_FREQ,
                    0
                    );

        // Configures the divisor
        OUTREG32( &m_pDispRegs->DISPC_DIVISOR,
                    DISPC_DIVISOR_PCD(2) |
                    DISPC_DIVISOR_LCD(1)
                    );


        // Configures the panel size
        OUTREG32( &m_pDispRegs->DISPC_SIZE_LCD,
                    DISPC_SIZE_LCD_LPP(720) |
                    DISPC_SIZE_LCD_PPL(1280)
                    );


        //  Center the output
        OUTREG32( &m_pDispRegs->DISPC_GFX_POSITION,
                    DISPC_GFX_POS_GFXPOSX(dwX) |
                    DISPC_GFX_POS_GFXPOSY(dwY)
                    );

        m_bHDMIEnable = TRUE;
    }
    else
    {
        // Get the int LCD params
        LcdPdd_LCD_GetMode( (DWORD*) &m_eLcdPixelFormat,
                              &m_dwLcdWidth,
                              &m_dwLcdHeight,
                              &m_dwPixelClock
                              );
        
        // Clear the HDMI at the beginning.
        // Required for restoring IntLCD context
        m_bHDMIEnable = FALSE;

        // Change the power state to LCD panel
        LcdPdd_SetPowerLevel(D0);

        // Restore internal LCD configurations
        RestoreRegisters(OMAP_DSS_DESTINATION_LCD);

        // Enable the LPR if the FIFO's are merged.
        if ( (INREG32( &m_pDispRegs->DISPC_CONFIG) & DISPC_CONFIG_FIFOMERGE ) )
            bLPRState = TRUE;
        else
            bLPRState = FALSE;

        // Restore LPR if enabled and configure DSI to IntLCD FCLK
        EnableLPR( bLPRState, FALSE );
    }
    
    //  Flush shadow registers
    FlushRegs( DISPC_CONTROL_GOLCD );

    //  Enable the LCD
    SETREG32( &m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_LCDENABLE );

    //  Success
    bResult = TRUE;

cleanUp:
    //  Release regs
    ReleaseRegs();

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL OMAPDisplayController::DVISelect(
    BOOL bSelectDVI
    )
{
    LcdPdd_DVI_Select(bSelectDVI);
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnableDVI(
    BOOL bEnable
    )
{
    BOOL    bResult = FALSE;

    //  Enable/disable DVI
    if ( bEnable )
    {
        //  Enable DVI        
        LcdPdd_SetPowerLevel(D4);
        LcdPdd_DVI_Select(TRUE);
        LcdPdd_SetPowerLevel(D0);
        m_bDVIEnable = TRUE;
    }
    else
    {
        //  Disable DVI        
        LcdPdd_SetPowerLevel(D4);
        LcdPdd_DVI_Select(FALSE);
        LcdPdd_SetPowerLevel(D0);
        m_bDVIEnable = FALSE;
    }

    //  Success
    bResult = TRUE;

    //  Return result
    return bResult;
}


//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetDSS()
{
    DWORD   dwTimeout;
    DWORD   dwVal;

    //  Need to enable DSS1, DSS2 and TVOUT to reset controller
    RequestClock( m_dssinfo.DSSDevice );         
    RequestClock( m_dssinfo.TVEncoderDevice );

    // check if digital output or the lcd output are enabled
    dwVal = INREG32(&m_pDispRegs->DISPC_CONTROL);

    if(dwVal & (DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE))
    {
        // disable the lcd output and digital output
        dwVal &= ~(DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE);
        OUTREG32(&m_pDispRegs->DISPC_CONTROL, dwVal);

        // wait until frame is done
        WaitForFrameDone(DISPLAY_TIMEOUT);
    }


    //  Reset the whole display subsystem    
    SETREG32( &m_pDSSRegs->DSS_SYSCONFIG, DSS_SYSCONFIG_SOFTRESET );
    
    //  Wait until reset completes OR timeout occurs
    dwTimeout=DISPLAY_TIMEOUT;
    while(((INREG32(&m_pDSSRegs->DSS_SYSSTATUS) & DSS_SYSSTATUS_RESETDONE) == 0) && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::ResetDSS: "
             L"Failed reset DSS\r\n"
            ));
    }

    //  Release clocks
    ReleaseClock( m_dssinfo.TVEncoderDevice );
    ReleaseClock( m_dssinfo.DSSDevice );         
        
    //  Return result
    return (dwTimeout > 0);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetDISPC()
{
    DWORD   dwVal;
    DWORD   dwTimeout;


    //  Need to enable DSS1, DSS2 and TVOUT to reset controller
    RequestClock( m_dssinfo.DSSDevice );         

    // check if digital output or the lcd output are enabled
    dwVal = INREG32(&m_pDispRegs->DISPC_CONTROL);

    if(dwVal & (DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE))
    {
        // disable the lcd output and digital output
        dwVal &= ~(DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE);
        OUTREG32(&m_pDispRegs->DISPC_CONTROL, dwVal);

        // wait until frame is done
        WaitForFrameDone(DISPLAY_TIMEOUT);
    }


    //  Reset the controller    
    SETREG32( &m_pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_SOFTRESET );
    
    //  Wait until reset completes OR timeout occurs
    dwTimeout=DISPLAY_TIMEOUT;
    while(((INREG32(&m_pDispRegs->DISPC_SYSSTATUS) & DISPC_SYSSTATUS_RESETDONE) == 0) && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::ResetDISPC: "
             L"Failed reset DISPC\r\n"
            ));
    }

    //  Release clocks
    ReleaseClock( m_dssinfo.DSSDevice );         
        
    //  Return result
    return (dwTimeout > 0);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetVENC()
{
    DWORD   dwTimeout;


    //  Need to enable DSS1, DSS2 and TVOUT to reset video encoder
    RequestClock( m_dssinfo.TVEncoderDevice );         

    
    //  Reset the video encoder   
    SETREG32( &m_pVencRegs->VENC_F_CONTROL, VENC_F_CONTROL_RESET );
    
    //  Wait until reset completes OR timeout occurs
    dwTimeout=DISPLAY_TIMEOUT;
    while(((INREG32(&m_pVencRegs->VENC_F_CONTROL) & VENC_F_CONTROL_RESET) == 0) && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::ResetVENC: "
             L"Failed reset DSS\r\n"
            ));
    }


    //  Clear video encoder F-control and SYNC Control regsiters
    OUTREG32( &m_pVencRegs->VENC_F_CONTROL, 0 );
    OUTREG32( &m_pVencRegs->VENC_SYNC_CTRL, 0 );


    //  Release clocks
    ReleaseClock( m_dssinfo.TVEncoderDevice );         
        
    //  Return result
    return (dwTimeout > 0);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::AccessRegs()
{
    BOOL    bResult = FALSE;

    //  Ensures that DSS regs can be accessed at current power level
    //  Locks power level at current level until ReleaseRegs called
    //  Returns FALSE if power level is too low to access regs

    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );
    
    //  Check power level
    switch( m_dwPowerLevel )
    {
        case D0:
        case D1:
        case D2:
            //  Clocks are on at this level
            bResult = TRUE;
            break;

        case D3:
        case D4:
            //  Clocks are off at this level
            bResult = FALSE;
            break;
    }
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ReleaseRegs()
{
    //  Releases power lock
    LeaveCriticalSection( &m_csPowerLock );
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::FlushRegs(
    DWORD   dwDestGo
    )
{
    DWORD   dwTimeout = DISPLAY_TIMEOUT;

    //  Ensure that registers can be flushed
    while(((INREG32(&m_pDispRegs->DISPC_CONTROL) & dwDestGo) == dwDestGo) &&  (dwTimeout-- > 0))
    {
        // delay
        StallExecution(10);
    }

    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::FlushRegs: "
             L"Failed to flush regs\r\n"
            ));
    }

    //  Flush the shadow registers
    SETREG32( &m_pDispRegs->DISPC_CONTROL, dwDestGo );


    //  Return result
    return (dwTimeout > 0);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::WaitForFlushDone(
    DWORD   dwDestGo
    )
{
    DWORD dwTimeout = DISPLAY_TIMEOUT;
    
    //  Ensure that registers can be flushed
    while(((INREG32(&m_pDispRegs->DISPC_CONTROL) & dwDestGo) == dwDestGo) &&  (dwTimeout-- > 0))
    {
        // delay = 1ms
        Sleep(1);
    }
    
    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::FlushRegs: "
             L"Failed to flush regs\r\n"
            ));
    }
    
    return (dwTimeout > 0);
    
}    
//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::RequestClock(
    DWORD   dwClock
    )
{
    return EnableDeviceClocks(dwClock, TRUE);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ReleaseClock(
    DWORD   dwClock
    )
{
    return EnableDeviceClocks(dwClock, FALSE);
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::WaitForFrameDone(
    DWORD dwTimeout
    )
{
    //  Wait for VYSNC status
    OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_FRAMEDONE );
    while (((INREG32(&m_pDispRegs->DISPC_IRQSTATUS) & DISPC_IRQSTATUS_FRAMEDONE) == 0) && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if ( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::WaitForFrameDone: timeout\r\n"));
        return FALSE;
    }

    //  Clear the status
    OUTREG32( &m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_FRAMEDONE );

    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::WaitForIRQ(
    DWORD dwIRQ,
    DWORD dwTimeout
    )
{
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }


    //  Wait for VYSNC status
    SETREG32( &m_pDispRegs->DISPC_IRQSTATUS, dwIRQ );
    while(((INREG32(&m_pDispRegs->DISPC_IRQSTATUS) & dwIRQ) == 0) && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::WaitForIRQ: "
             L"IRQ = 0x%X failed to happen before timeout\r\n", dwIRQ
            ));
    }

    //  Clear the status
    SETREG32( &m_pDispRegs->DISPC_IRQSTATUS, dwIRQ );
    
cleanup:

    ReleaseRegs();

    //  Status
    return TRUE;
}

//------------------------------------------------------------------------------
DWORD
OMAPDisplayController::PixelFormatToPixelSize(
    OMAP_DSS_PIXELFORMAT    ePixelFormat
)
{
    DWORD   dwResult = 1;
    
    //  Convert pixel format into bytes per pixel
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
        case OMAP_DSS_PIXELFORMAT_ARGB16:
        case OMAP_DSS_PIXELFORMAT_YUV2:
        case OMAP_DSS_PIXELFORMAT_UYVY:
            //  2 bytes per pixel
            dwResult = 2;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
        case OMAP_DSS_PIXELFORMAT_ARGB32:
        case OMAP_DSS_PIXELFORMAT_RGBA32:
            //  4 bytes per pixel
            dwResult = 4;
            break;
    }

    //  Return result
    return dwResult;
}

//------------------------------------------------------------------------------
BOOL OMAPDisplayController::EnableVSyncInterruptEx()
{
    BOOL bInterruptAlreadyEnabled = FALSE;
    DWORD irqEnableStatus;
    
    if (!m_dwEnableWaitForVerticalBlank)
        return FALSE;

    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }

    irqEnableStatus = INREG32(&m_pDispRegs->DISPC_IRQENABLE);
    bInterruptAlreadyEnabled = ((irqEnableStatus & DISPC_IRQENABLE_VSYNC) == DISPC_IRQENABLE_VSYNC) ||
                                ((irqEnableStatus & DISPC_IRQSTATUS_EVSYNC_EVEN) == DISPC_IRQSTATUS_EVSYNC_EVEN) ||
                                ((irqEnableStatus & DISPC_IRQSTATUS_EVSYNC_ODD) == DISPC_IRQSTATUS_EVSYNC_ODD);
                        
                                
    
    SETREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VSYNC);
    if(m_bTVEnable == TRUE)
    {
        SETREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_EVSYNC_EVEN | DISPC_IRQENABLE_EVSYNC_ODD);
    }


cleanup:

    ReleaseRegs();
    return bInterruptAlreadyEnabled;
}

//------------------------------------------------------------------------------
void OMAPDisplayController::EnableVSyncInterrupt()
{
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }

    SETREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VSYNC);
    if(m_bTVEnable == TRUE)
    {
        SETREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_EVSYNC_EVEN | DISPC_IRQENABLE_EVSYNC_ODD);
    }

cleanup:

    ReleaseRegs();
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::DisableVSyncInterrupt()
{
    
    m_lastVsyncIRQStatus = 0;
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }
        
    CLRREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_VSYNC);
    if(m_bTVEnable == TRUE)
    {
        CLRREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_EVSYNC_EVEN | DISPC_IRQENABLE_EVSYNC_ODD);
    }

cleanup:

    ReleaseRegs();
}

//------------------------------------------------------------------------------
BOOL OMAPDisplayController::InVSync(BOOL bClearStatus)
{
    BOOL bInVSync = FALSE;
    DWORD irqStatus = 0;
    BOOL  lcdVsync = FALSE;
    //Alwasy set to true, in case tv-out is disabled.
    BOOL  tvVsync = TRUE;

    if (!m_dwEnableWaitForVerticalBlank)
        return TRUE;

    if(AccessRegs() == FALSE)
    {
        bInVSync = TRUE;
        goto cleanup;
    }

    irqStatus = INREG32(&m_pDispRegs->DISPC_IRQSTATUS);
    lcdVsync = (irqStatus & DISPC_IRQSTATUS_VSYNC) == DISPC_IRQSTATUS_VSYNC;
    if(m_bTVEnable == TRUE)
    {
        tvVsync = ((irqStatus & DISPC_IRQSTATUS_EVSYNC_EVEN) == DISPC_IRQSTATUS_EVSYNC_EVEN) ||
                  ((irqStatus & DISPC_IRQSTATUS_EVSYNC_ODD) == DISPC_IRQSTATUS_EVSYNC_ODD);
          
    }
    //If tv-out is enabled we also need to check of it's VSYNC signal. Once both have been asserted then 
    //we can say that Vsync has occurred. 
    if( lcdVsync && tvVsync)
    {
        bInVSync = TRUE;

        if(bClearStatus)
        {
            SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_VSYNC);
            if(m_bTVEnable == TRUE)
            {
                SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_EVEN); 
                SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_EVSYNC_ODD); 
            }
        }
    }

cleanup:

    ReleaseRegs();

    return bInVSync;
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::WaitForVsync()
{
    BOOL bVsyncPreviouslyEnabled = FALSE;

    if (!m_dwEnableWaitForVerticalBlank)
        return;

    if(!InVSync(TRUE))
    {             
        bVsyncPreviouslyEnabled = EnableVSyncInterruptEx();
        WaitForSingleObject(m_hVsyncEvent, m_dwVsyncPeriod);
        //SGX may have turned on the vsync interrupt, keep it on if that's the case.
        if(!bVsyncPreviouslyEnabled)
        {
            DisableVSyncInterrupt();
        }
    }
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::EnableScanLineInterrupt(DWORD dwLineNumber)
{
    if (!m_dwEnableWaitForVerticalBlank)
        return;

    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }

    //  Program line number to interrupt on
    if(INREG32(&m_pDispRegs->DISPC_LINE_NUMBER) != dwLineNumber)
    {
        OUTREG32(&m_pDispRegs->DISPC_LINE_NUMBER, dwLineNumber);
        FlushRegs(DISPC_CONTROL_GOLCD);
    }

    //  Enable                                
    SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_PROGRAMMEDLINENUMBER);
    SETREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_PROGRAMMEDLINENUMBER);

cleanup:
    ReleaseRegs();
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::DisableScanLineInterrupt()
{
    if (!m_dwEnableWaitForVerticalBlank)
        return;

    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }

    //  Disable interrupt
    SETREG32(&m_pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_PROGRAMMEDLINENUMBER);
    CLRREG32(&m_pDispRegs->DISPC_IRQENABLE, DISPC_IRQENABLE_PROGRAMMEDLINENUMBER);

cleanup:
    ReleaseRegs();
}

//------------------------------------------------------------------------------
DWORD OMAPDisplayController::GetScanLine()
{
    DWORD scanLine = 0;
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }

    //  Get current scanline value
    scanLine = INREG32(&m_pDispRegs->DISPC_LINE_STATUS);

cleanup:
    ReleaseRegs();
    return scanLine;
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::WaitForScanLine(DWORD dwLineNumber)
{
    if (!m_dwEnableWaitForVerticalBlank)
        return;

    //  Enable the scanline interrupt for the given line number and wait
    EnableScanLineInterrupt(dwLineNumber);
    WaitForSingleObject(m_hScanLineEvent, m_dwVsyncPeriod);
    DisableScanLineInterrupt();
}

//------------------------------------------------------------------------------
BOOL OMAPSurface::SetClipping(RECT* pClipRect )
{
    BOOL    bResult;
    RECT    rcSurf;
    
    //  Set the rect of the entire surface
    rcSurf.left = 0;
    rcSurf.top = 0;
    rcSurf.right = m_dwWidth;
    rcSurf.bottom = m_dwHeight;
    
    //  Set the clipping region of the surface
    if( pClipRect == NULL )
    {
        //  No clipping; use entire surface size
        m_rcClip = rcSurf;
        bResult = TRUE;
    }
    else
    {
        //  Find intersection of surface rect and clipping rect
        bResult = IntersectRect( &m_rcClip, &rcSurf, pClipRect );
    }
    
    UpdateClipping(pClipRect);

    //  Update the given clipping rect
    if( pClipRect )
        *pClipRect = m_rcClip;

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL OMAPSurface::UpdateClipping( RECT* pClipRect )
{
    BOOL bResult = FALSE;

    // Could change to to ensure rectangle alignment with different
    // scale and decimation factors...
    
    //Force the clipping rectangle to fall in a pack pixel boundary
    if (pClipRect != NULL)
    {
        AdjustClippingRect(&m_rcClip, 2, 2);
    }
    
    bResult = TRUE;
    return bResult;
}

//------------------------------------------------------------------------------
RECT OMAPSurface::GetClipping()
{
    return m_rcClip;
}

//------------------------------------------------------------------------------
BOOL OMAPSurface::AdjustClippingRect(RECT *srcRect, UINT8 horzValue, UINT8 vertValue)
{
    BOOL bResult = FALSE;

    if(srcRect == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Null rectangle passed!\r\n"), __FUNCTION__));
        return bResult;
    }
    if(vertValue > 1)
    {
        if( ((srcRect->top)%vertValue)!= 0)
            CEIL_MULT(srcRect->top, vertValue);
        if( ((srcRect->bottom)%vertValue)!= 0)
            FLOOR_MULT(srcRect->bottom, vertValue);
    }
    if(horzValue > 1)
    {
        if( ((srcRect->left)%horzValue)!= 0)
            CEIL_MULT(srcRect->left, horzValue);
        if( ((srcRect->right)%horzValue)!= 0)
            FLOOR_MULT(srcRect->right, horzValue);
    }

    bResult = TRUE;
    return bResult;

}

//------------------------------------------------------------------------------
BOOL OMAPSurface::SetHorizontalScaling(DWORD dwScaleFactor)
{
    BOOL    bResult;

    //  Validate scaling factor
    switch( dwScaleFactor )
    {
        case 1:
        case 2:
        case 4:
        case 8:
            //  Valid scaling factors
            m_dwHorizScale = dwScaleFactor;
            bResult = TRUE;
            break;
                    
        default:
            //  Invalid
            bResult = FALSE;
            break;
    }

    if ((m_pAssocSurface) && (m_eSurfaceType==OMAP_SURFACE_NORMAL))
        m_pAssocSurface->SetHorizontalScaling(dwScaleFactor);
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL OMAPSurface::SetVerticalScaling(DWORD dwScaleFactor)
{
    BOOL    bResult;

    //  Validate scaling factor
    switch( dwScaleFactor )
    {
        case 1:
        case 2:
        case 4:
        case 8:
            //  Valid scaling factors
            m_dwVertScale = dwScaleFactor;
            bResult = TRUE;
            break;
                    
        default:
            //  Invalid
            bResult = FALSE;
            break;
    }

    if ((m_pAssocSurface) && (m_eSurfaceType==OMAP_SURFACE_NORMAL))
        m_pAssocSurface->SetVerticalScaling(dwScaleFactor);
    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------

BOOL    
OMAPSurface::SetSurfaceType(
    OMAP_SURFACE_TYPE eSurfaceType
    )
{
    m_eSurfaceType = eSurfaceType;
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL    
OMAPSurface::SetAssocSurface(
    OMAPSurface* pAssocSurface
    )
{
    m_pAssocSurface = pAssocSurface;
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL   
OMAPSurface::UseResizer(
    BOOL bUseResizer
)
{
    if (m_pAssocSurface)
        m_bUseResizer = bUseResizer;
    else /* force to false if Assoc Surface is not allocated */
        m_bUseResizer = FALSE;
    return m_bUseResizer;
}

//------------------------------------------------------------------------------
BOOL                    
OMAPSurface::isResizerEnabled()
{
    return m_bUseResizer;
}


HANDLE OMAPSurface::GetRSZHandle(BOOL alloc)
{
    if ((m_hRSZHandle == NULL) && alloc)
    {
        m_hRSZHandle = CreateFile( _T("RSZ1:"), 
                                        GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                        OPEN_EXISTING, 0, 0 );
        if (m_hRSZHandle == INVALID_HANDLE_VALUE)
            m_hRSZHandle = NULL;

        DEBUGMSG(ZONE_WARNING, (L"GetRSZHandle: Open handle 0x%x \r\n",m_hRSZHandle));
        
    }
    return m_hRSZHandle;
}

VOID OMAPSurface::SetRSZHandle(HANDLE rszHandle, BOOL freeHandle)
{
    if ((m_hRSZHandle != NULL) && freeHandle)
    {        
        CloseHandle(m_hRSZHandle);
        
        m_hRSZHandle = NULL;
    }

    m_hRSZHandle = rszHandle;
}


BOOL OMAPSurface::ConfigResizerParams(RECT* pSrcRect, RECT* pDestRect,OMAP_DSS_ROTATION eRotation)
{	
    BOOL retCode = FALSE;
     
    //m_sRSZParams.ulReadAddr;
    m_sRSZParams.ulReadOffset = Stride(eRotation); /* input width * 2 */
    m_sRSZParams.ulReadAddrOffset = 0;
    m_sRSZParams.ulOutOffset = Stride(eRotation);     
    //m_sRSZParams.ulWriteAddr;

    m_sRSZParams.ulInputImageWidth = pSrcRect->right - pSrcRect->left;
    m_sRSZParams.ulInputImageHeight = pSrcRect->bottom - pSrcRect->top;

    m_sRSZParams.ulOutputImageWidth = pDestRect->right - pDestRect->left;
    m_sRSZParams.ulOutputImageHeight = pDestRect->bottom - pDestRect->top;
    m_sRSZParams.h_startphase = RSZ_DEFAULTSTPHASE;
    m_sRSZParams.v_startphase = RSZ_DEFAULTSTPHASE;
    //m_sRSZParams.h_resz;
    //m_sRSZParams.v_resz;
    //m_sRSZParams.algo;
    m_sRSZParams.width = m_sRSZParams.ulInputImageWidth;
    m_sRSZParams.height = m_sRSZParams.ulInputImageHeight;
    m_sRSZParams.cropTop = 0;
    m_sRSZParams.cropLeft = 0;     
    m_sRSZParams.cropWidth = m_sRSZParams.ulOutputImageWidth;
    m_sRSZParams.cropHeight = m_sRSZParams.ulOutputImageHeight;
    m_sRSZParams.bReadFromMemory = TRUE;
    m_sRSZParams.enableZoom = FALSE;
    m_sRSZParams.ulZoomFactor = 0;
    if (m_ePixelFormat == OMAP_DSS_PIXELFORMAT_UYVY)
    {
        m_sRSZParams.ulInpType = RSZ_INTYPE_YCBCR422_16BIT; 
        m_sRSZParams.ulPixFmt = RSZ_PIX_FMT_UYVY; 
    }
    else if (m_ePixelFormat == OMAP_DSS_PIXELFORMAT_YUV2)        
    {
        m_sRSZParams.ulInpType = RSZ_INTYPE_YCBCR422_16BIT; 
        m_sRSZParams.ulPixFmt = RSZ_PIX_FMT_YUYV; 
    }
    else
    {
        DEBUGMSG(ZONE_WARNING, (L"ConfigResizerParams: Unsupported pixel type %d \r\n",m_ePixelFormat));
        return FALSE;
    }
    
    retCode = DeviceIoControl (  m_hRSZHandle,
                                 RSZ_IOCTL_SET_PARAMS,
                                 (LPVOID)&m_sRSZParams, 
                                 sizeof (RSZParams_t), 
                                 (LPVOID)&m_sRSZParams, 
                                 sizeof (RSZParams_t), NULL, NULL);

    if (!retCode)     
        DEBUGMSG(ZONE_WARNING, (L"ConfigResizerParams: handle 0x%x returned retCode %d \r\n",m_hRSZHandle,retCode));


     return retCode;
     
}

BOOL OMAPSurface::StartResizer(DWORD dwInAddr, DWORD dwOutAddr)
{
    BOOL retCode = FALSE;   
    DWORD dwInAddrOffset = 0;
    if (dwInAddr%32!=0)
    {
        dwInAddrOffset=dwInAddr&0x1F;
        dwInAddr=(dwInAddr>>5)<<5;
        if (m_sRSZParams.ulInpType == RSZ_INTYPE_YCBCR422_16BIT)
            dwInAddrOffset/=2;
    }
    m_sRSZParams.ulReadAddr = dwInAddr;
    m_sRSZParams.ulWriteAddr = dwOutAddr;
    m_sRSZParams.cropLeft = dwInAddrOffset;
    retCode = DeviceIoControl ( m_hRSZHandle,
                                 RSZ_IOCTL_START_RESIZER,
                                 (LPVOID)&m_sRSZParams, 
                                 sizeof (RSZParams_t), 
                                 NULL, 0, NULL, NULL);
    if (!retCode)     
        DEBUGMSG(ZONE_WARNING, (L"StartResizer: handle 0x%x inAddr 0x%x outAddr 0x%x cropLeft %d returned retCode %d",
                        m_hRSZHandle,dwInAddr,dwOutAddr,dwInAddrOffset,retCode));

    return retCode;
}


//-------------------------------------------------------------------------------
VOID
OMAPDisplayController::EnableLPR(BOOL bEnable, BOOL bHdmiEnable)
{
    OMAP_DSS_FCLKVALUE eFclkValue = m_eDssFclkValue;

    if (m_hSmartReflexPolicyAdapter == NULL)
	    return;

    if (m_bDVIEnable)
        return;

    if ((bEnable == m_bLPREnable) && (bHdmiEnable == FALSE))
    {
        return;
    }
    
    // For HDMI Panel, the FCLK specific to HDMI panel should be used
    eFclkValue = ( bHdmiEnable ) ? OMAP_DSS_FCLKVALUE_HDMI : eFclkValue ;
    // LPR is disabled when HDMI panel is active
    bEnable    = ( bHdmiEnable ) ? FALSE : bEnable;
 
    if (bEnable)
        {

        //Send LPR status to SmartReflex policy adapter
        PmxNotifyPolicy(m_hSmartReflexPolicyAdapter,SMARTREFLEX_LPR_MODE,&bEnable,sizeof(BOOL));
        
        // Set the FCLK corresponding to LPR mode
        SetDssFclk ( m_eDssFclkSource, OMAP_DSS_FCLKVALUE_LPR );
     
        // Enable LPR
        OUTREG32( &m_pDispRegs->DISPC_SYSCONFIG,
                    DISPC_SYSCONFIG_AUTOIDLE|
                    SYSCONFIG_SMARTIDLE|
                    SYSCONFIG_ENAWAKEUP|
                    SYSCONFIG_CLOCKACTIVITY_I_ON|
                    SYSCONFIG_SMARTSTANDBY
                    );
                  
        }
    else
        {
        // Disable LPR
        OUTREG32( &m_pDispRegs->DISPC_SYSCONFIG,
                    DISPC_SYSCONFIG_AUTOIDLE|
                    SYSCONFIG_NOIDLE|
                    SYSCONFIG_NOSTANDBY
                    );
                  
        // Set the FCLK corresponding to LPR mode
        SetDssFclk ( m_eDssFclkSource, eFclkValue );  

        //Send LPR status to SmartReflex policy adapter
        PmxNotifyPolicy(m_hSmartReflexPolicyAdapter,SMARTREFLEX_LPR_MODE,&bEnable,sizeof(BOOL));
        }
    m_bLPREnable = bEnable; 
}

//------------------------------------------------------------------------------
VOID   
OMAPDisplayController::EnableOverlayOptimization(BOOL bEnable)
{
    DWORD dwWindowSkip = 0;  

    //Enable Overlay optimization if no colorkeying, no alpha blending and only VID1 layer is visible
    //going to LCD destination
    if( bEnable &&
        g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].pSurface != NULL &&           
        g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].bEnabled == TRUE &&
        g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].eDestination == OMAP_DSS_DESTINATION_LCD &&
        g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].bEnabled == TRUE &&
        g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO2].bEnabled == FALSE && 
        (INREG32(&m_pDispRegs->DISPC_CONFIG) & DISPC_CONFIG_TCKLCDENABLE) == 0 &&
        (INREG32( &m_pDispRegs->DISPC_CONFIG) & DISPC_CONFIG_LCDALPHABLENDERENABLE) == 0)
    {
         //Assume overlay destination is always contained within the GFX window inclusive  
        OMAPSurface *pGFXSurface = g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].pSurface;             
        DWORD dwGFXWidth = g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].dwDestWidth;
        DWORD dwGFXHeight = g_rgPipelineMapping[OMAP_DSS_PIPELINE_GFX].dwDestHeight;
        DWORD dwDestWidth = g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].dwDestWidth;
        DWORD dwDestHeight = g_rgPipelineMapping[OMAP_DSS_PIPELINE_VIDEO1].dwDestHeight;
        DWORD dwPosX =  INREG32(&m_pDispRegs->tDISPC_VID1.POSITION) & 0xFFFF;
        DWORD dwGFXPixInc = INREG32(&m_pDispRegs->DISPC_GFX_PIXEL_INC);
        DWORD dwGFXRowInc = INREG32(&m_pDispRegs->DISPC_GFX_ROW_INC);
               
        //Simulate how the DMA controller would skip bytes
        //1.after every pixel read DMA controller index = index + BPP-1
        //2.After every pixel read, index = index+PIXEL_INC, unless it's the last pixel in the row
        //3.At the end of each row, index = index+ROW_INC       
        if( (dwDestWidth == dwGFXWidth) && (dwDestHeight == dwGFXHeight) )
        {
            //  Whole video layer covers GFX
            dwWindowSkip = 0;
        }
        else if(dwDestWidth == dwGFXWidth)
        {               
            DWORD dwPixIncPerRow = dwGFXPixInc*(dwDestWidth-1);
            DWORD dwByteReadsPerRow = dwDestWidth*(pGFXSurface->PixelSize()-1);
            dwWindowSkip = dwDestHeight*(dwPixIncPerRow+dwByteReadsPerRow+dwGFXRowInc);         
        }        
        else  
        {              
            dwWindowSkip = dwDestWidth*(dwGFXPixInc+(pGFXSurface->PixelSize()-1));  
            if(dwPosX != 0 && (dwPosX+dwDestWidth) != dwGFXWidth)
            {
                //additional pixel increment needed
                dwWindowSkip +=dwGFXPixInc;  
            }
        }                        
    }


    //  Set window skip value
    OUTREG32(&m_pDispRegs->DISPC_GFX_WINDOW_SKIP, dwWindowSkip);

    //  Enable/disable window skip
    if( dwWindowSkip != 0 )
        SETREG32(&m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_OVERLAY_OPTIMIZATION);        
    else
        CLRREG32(&m_pDispRegs->DISPC_CONTROL, DISPC_CONTROL_OVERLAY_OPTIMIZATION);        
}

//----------------------------------------------------------------------------
BOOL 
OMAPDisplayController::SetDssFclk(
    OMAP_DSS_FCLK      eDssFclkSource,
    OMAP_DSS_FCLKVALUE eDssFclkValue
    )
{
    BOOL bRet = TRUE;
    
    UNREFERENCED_PARAMETER(eDssFclkValue);
    UNREFERENCED_PARAMETER(eDssFclkSource);

    return bRet;
}

//----------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitDsiPll()
{
    BOOL  bRet = TRUE;
    ULONG count = 100;                  //count for timed status check
    ULONG value;
    
    // PCLKFREE should be set for DSI
    value  = INREG32( &m_pDispRegs->DISPC_CONTROL);
    value |= DISPC_CONTROL_PCKFREEENABLE_ENABLED;
    OUTREG32( &m_pDispRegs->DISPC_CONTROL, value);

    // Reset the DSI protocol engine
    SETREG32( &m_pDSIRegs->DSI_SYSCONFIG, SYSCONFIG_SOFTRESET);

    // Wait for reset to complete
    while (((INREG32(&m_pDSIRegs->DSI_SYSSTATUS)) != SYSSTATUS_RESETDONE) && (--count))
    {
        StallExecution(1000);
    }

    if (count == 0)
    {
        RETAILMSG(1,(L"ERROR: InitDSIPLL::"
                     L"Failed to reset DSI Module\r\n"));
        bRet = FALSE;
        goto Clean;
    }

    //  Configure for idle during inactivity
    OUTREG32( &m_pDSIRegs->DSI_SYSCONFIG,  
                SYSCONFIG_AUTOIDLE  |
                SYSCONFIG_ENAWAKEUP |
                SYSCONFIG_SMARTIDLE
                );

    // Clear the DSI IRQ status
    value = INREG32( &m_pDSIRegs->DSI_IRQSTATUS);
    OUTREG32( &m_pDSIRegs->DSI_IRQSTATUS, value);

    // Enable the Pwr to the DSI sub modules
    value  = INREG32( &m_pDSIRegs->DSI_CLK_CTRL);
    // clear the current pwr cmd value
    value  = (value & ~(DSI_CLK_CTRL_PLL_PWR_CMD_MASK));
    // set the pwr cmd for enabling pwr to PLL and HS DIVIDER
    value |= (DSI_CLK_CTRL_PLL_PWR_CMD_ON_PLLANDHS)|(1 << 13);
    OUTREG32( &m_pDSIRegs->DSI_CLK_CTRL, value);

    // Check whether the power status is changed
    count = 1000;
    do
    {
        value = INREG32( &m_pDSIRegs->DSI_CLK_CTRL);
        value = (value & DSI_CLK_CTRL_PLL_PWR_STATUS_ON_PLLANDHS);
        StallExecution(1000);
    }
    while ((value == 0) && (--count));

    if (count == 0)
    {
        bRet = FALSE;
        RETAILMSG(1,(L"Unable to turn on DSI PLL %x\r\n",
            INREG32(&m_pDSIRegs->DSI_CLK_CTRL)\
            ));
        goto Clean;
    }

    // Check for the PLL reset complete status
    count = 100;
    do
    {
        value = INREG32( &m_pDSIPLLRegs->DSI_PLL_STATUS);
        value = (value & DSI_PLLCTRL_RESET_DONE_STATUS);
        StallExecution(1000);
    }
    while ((value == 0) && (--count));

    if (count == 0)
    {
        bRet = FALSE;
        RETAILMSG(1,(L"DSI PLL not reset\r\n"));
    }

Clean:
    return bRet;
}

//-------------------------------------------------------------------------------
BOOL
OMAPDisplayController::DeInitDsiPll()
{
    BOOL bRet = TRUE;
    DWORD dwRegValue = 0;
    
    // clear the DSI IRQ status
    dwRegValue = INREG32( &m_pDSIRegs->DSI_IRQSTATUS);
    OUTREG32( &m_pDSIRegs->DSI_IRQSTATUS, dwRegValue);
    
    // configure the DSI PLL for bypass mode before updating PLL
    SETREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, DSI_PLL_IDLE);

    // select the manual mode of PLL update
    dwRegValue = INREG32( &m_pDSIPLLRegs->DSI_PLL_CONTROL);
    dwRegValue = dwRegValue & ~(DSI_PLL_AUTOMODE);
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONTROL, dwRegValue);

    // disable DSIPHY clock and set HSDIV in bypass mode
    dwRegValue = INREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2);
    dwRegValue = dwRegValue & ~(DSIPHY_CLKINEN);
    dwRegValue = dwRegValue | DSI_HSDIVBYPASS;
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, dwRegValue);
    
    // PCLKFREE should be set for DSI
    dwRegValue = INREG32( &m_pDispRegs->DISPC_CONTROL);
    dwRegValue = dwRegValue | DISPC_CONTROL_PCKFREEENABLE_ENABLED;
    OUTREG32( &m_pDispRegs->DISPC_CONTROL, dwRegValue);
    
    // issue Power off cmd to DSI
    dwRegValue = INREG32( &m_pDSIRegs->DSI_CLK_CTRL);
    dwRegValue = (dwRegValue & ~(DSI_CLK_CTRL_PLL_PWR_CMD_MASK));
    dwRegValue = dwRegValue | DSI_CLK_CTRL_PLL_PWR_CMD_OFF;
    OUTREG32( &m_pDSIRegs->DSI_CLK_CTRL, dwRegValue);

    return bRet;
}

//-------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ConfigureDsiPll(ULONG clockFreq)
{

    BOOL  bRet = TRUE;
    ULONG count = 100;                  //count for timed status check
    ULONG m,n,m3,m4;                    //variables for PLL freq configuration
    ULONG fint    = DSI_PLL_FREQINT;    //internal ref frequency for PLL
    ULONG sysClk  = GetSystemClockFrequency();
    ULONG value;
    ULONG dsiPhyClock;
    ULONG highFreqDiv = 0;
    ULONG pllConfig1,pllConfig2; // variables for PLLConfig1 & 2


    //  Check clock bounds
    if (clockFreq < fint ) // || clockFreq > DSS_FCLK_MAX)
    {
        bRet = FALSE;
        RETAILMSG(1,(L"ConfigureDsiPll:"
                     L"Cannot configure DSI for given freq: %d\r\n",
                     clockFreq));
        goto Clean;
    }

    // Calculate the dsiPhyClock
    // Number of Lanes for DSI = 2
    // DSI PHY clock = 2 * data rate * NumberOfLanes
    dsiPhyClock = clockFreq * 2 * 2;

    // The steps for m and n calculation are to ensure the DSI PLL generates
    // DSI1_PLL_FCLK.

    // Use the formula for deriving m, m3 and m4 values
    //                             2xRegM       SYSCLK
    //             dsiPhyClock =  -------- x -------------
    //                            REGN + 1   HIGHFREQDIV+1

    // Set the highfreq divider if the input clock is > 32Mhz
    if (sysClk > DSI_HIGHFREQ_MAX)
        highFreqDiv = 1;

    n  = (sysClk/fint/(highFreqDiv+1)) - 1;

    m = (dsiPhyClock*(highFreqDiv+1))/(2*fint);

    // calculate m3 value DSI1_PLL_FCLK (m3)
    m3 = (dsiPhyClock/clockFreq) - 1;

    // m4 value is actually used by DSI module and we can set it
    // as the same value of m3.
    m4 = m3;

    DEBUGMSG(1,(L"N:%d M:%d M3:%d M4:%d\r\n",
                   n,m,m3,m4
                   ));

    // Enable the Pwr to the DSI sub modules
    value  = INREG32( &m_pDSIRegs->DSI_CLK_CTRL);
    if ((value & DSI_CLK_CTRL_PLL_PWR_STATUS_ON_PLLANDHS) != 0)
    {
        InitDsiPll();
    }
    
    // Configure the DSI PLL for bypass mode before updating PLL
    SETREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, DSI_PLL_IDLE);

    // Select the manual mode of PLL update
    value = INREG32( &m_pDSIPLLRegs->DSI_PLL_CONTROL);
    value = value & ~(DSI_PLL_AUTOMODE);
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONTROL, value);

    // DSIPHY clock is disabled and HSDIV in bypass mode
    pllConfig2  = INREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2);
    pllConfig2  = pllConfig2 & ~(DSIPHY_CLKINEN);
    pllConfig2  = pllConfig2 | DSI_HSDIVBYPASS;
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, pllConfig2);

    // Input clock to PLL is SYSCLK
    pllConfig2  = pllConfig2 & ~(DSI_PLL_CLKSEL_PCLKFREE);

    // Program high freq divider
    if (highFreqDiv != 0)
    {
        pllConfig2 |= DSI_PLL_HIGHFREQ_PIXELCLKBY2;
    }
    else
    {
        pllConfig2 &= ~(DSI_PLL_HIGHFREQ_PIXELCLKBY2);
    }

    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, pllConfig2);

    // Configure the divisor values
    pllConfig1 =  DSI_PLL_REGN(n)
                 |DSI_PLL_REGM(m)
                 |DSS_CLOCK_DIV(m3)
                 |DSIPROTO_CLOCK_DIV(m4)
                 |DSI_PLL_STOPMODE  
                 ;

    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION1, pllConfig1);

    // Enable the DSS clock divider from HSDIV
    pllConfig2 |=  DSS_CLOCK_EN
                  |DSI_PROTO_CLOCK_EN
                  |DSI_PLL_FREQSEL(DSI_PLL_FREQSELVAL)
                  |DSI_PLL_REFEN
                  ;
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, pllConfig2);

    // Set HSDIV and CLK from DSI PLL
    pllConfig2 &= ~(DSI_HSDIVBYPASS);
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, pllConfig2);

    // Let the Pll go out of idle
    CLRREG32( &m_pDSIPLLRegs->DSI_PLL_CONFIGURATION2, DSI_PLL_IDLE);
    
    // Start the PLL locking by setting PLL GO
    OUTREG32( &m_pDSIPLLRegs->DSI_PLL_GO, DSI_PLL_GO_CMD);

    count = 100;
    // Waiting for the lock request to be issued to PLL
    while ((INREG32( &m_pDSIPLLRegs->DSI_PLL_GO) != 0) && (--count))
    {
        StallExecution(1000);
    }

    if (count == 0)
    {
        /* lock request timed out */
        bRet = FALSE;
        RETAILMSG(1,(L"DSI PLL Go not set\r\n"));
        goto Clean;
    }

    // Wait for the PLL to be locked
    count = 1000;
    while (((INREG32( &m_pDSIPLLRegs->DSI_PLL_STATUS) & DSI_PLL_LOCK_STATUS) != 
              DSI_PLL_LOCK_STATUS) && (--count))
    {
        StallExecution(1000);
    }

    // check the PLL lock status for timeout
    if (count == 0)
    {
        bRet = FALSE;
        RETAILMSG(1,(L"DSI PLL Lock timed out\r\n"));
        goto Clean;
    }
    
    
    
Clean:

  return bRet;
}


// ---------------------------------------------------------------------------
BOOL
OMAPDisplayController::SwitchDssFclk(
    OMAP_DSS_FCLK eFclkSrc, 
    OMAP_DSS_FCLKVALUE eFclkValue
    )
{
    DWORD dssStatusBit = 1;
    DWORD count     = 100;
    DWORD bitMask   = 1;
    DWORD sdiStatus = 0;
    DWORD reg,
          lcd = 1,
          pcd = 1;
          
    if ( eFclkSrc == OMAP_DSS_FCLK_DSS1ALWON )
    {
        // Change the source clock to DSS
        CLRREG32( &m_pDSSRegs->DSS_CONTROL, 
                    DSS_CONTROL_DISPC_CLK_SWITCH_DSI1_PLL
                    );
        dssStatusBit = 1;
    }
    else if ( eFclkSrc == OMAP_DSS_FCLK_DSS2ALWON )
    {
        // Change the source clock DSI1_PLL
        SETREG32( &m_pDSSRegs->DSS_CONTROL, 
                    DSS_CONTROL_DISPC_CLK_SWITCH_DSI1_PLL
                    );
        dssStatusBit = 0;
    }

    pcd = eFclkValue / m_dwPixelClock ;
    reg = DISPC_DIVISOR_LCD(lcd) | DISPC_DIVISOR_PCD(pcd);
    OUTREG32( &m_pDispRegs->DISPC_DIVISOR, reg );    
    // Update the shadow register contents into main
    FlushRegs( DISPC_CONTROL_GOLCD );

    // check for the clock switch by reading SDI_STATUS register
    sdiStatus = INREG32(&m_pDSSRegs->DSS_SDI_STATUS);
    while (((sdiStatus & bitMask) != dssStatusBit) && (--count))
    {
        StallExecution(1000);
        sdiStatus = INREG32( &m_pDSSRegs->DSS_SDI_STATUS);
    }

    return TRUE;
}


extern "C" void LcdStall(DWORD dwMicroseconds)
{
    StallExecution(dwMicroseconds);
}

extern "C" void LcdSleep(DWORD dwMilliseconds)
{
    Sleep(dwMilliseconds);
}
