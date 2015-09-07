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
#include "soc_cfg.h"
/* Have to include omap header files to use PowerVR */
#include "omap_dss_regs.h"
#include "omap_prcm_regs.h"
#include "dssai.h"
#include "lcdc.h"
#include "ceddkex.h"
#include "_debug.h"
#include <am33x.h>
#include <oal_clock.h>
#include <sdk_padcfg.h>

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//
//  Defines
//

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
};

/* The following definition is not in OMAPDisplayController structure, this is due to PowerVR's dependency on dssai.h */
DWORD				m_last_vsync_dma_channel;
DWORD				m_enableSGXevent;	
struct lcdc			m_lcdc;
BOOL                m_flip_inprogress;

//------------------------------------------------------------------------------
OMAPDisplayController::OMAPDisplayController()
{
    
	// enabled in bootloader so assume its already on
    m_dwPowerLevel = D0;
    
    m_bGammaEnable = FALSE;
    m_dwEnableWaitForVerticalBlank = FALSE;
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
    
    m_pColorSpaceCoeffs = NULL;
    
    m_bLPREnable = FALSE;

    m_enableSGXevent = FALSE;
	m_flip_inprogress = FALSE;
    m_hSmartReflexPolicyAdapter = NULL;

    m_last_vsync_dma_channel = 0;
    //m_dssinfo local varible, no need to set it
    SOCGetDSSInfo(&m_dssinfo);
}

//------------------------------------------------------------------------------
OMAPDisplayController::~OMAPDisplayController()
{
    UninitInterrupts();

    //  Release all clocks
    EnableDeviceClocks( m_dssinfo.DSSDevice, FALSE );

    //  Delete power lock critical section
    DeleteCriticalSection( &m_csPowerLock );

    // Close SmartReflex policy adapter
    //if (m_hSmartReflexPolicyAdapter != NULL)
        //PmxClosePolicy(m_hSmartReflexPolicyAdapter);

    //  Unmap registers
    if (m_lcdc.regs != NULL) 
        MmUnmapIoSpace((VOID*)m_lcdc.regs, sizeof(LCDC_REGS));
	
    //  Free allocated memory
    if( m_pGammaBufVirt != NULL )
        FreePhysMem( m_pGammaBufVirt );
        
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitController(BOOL bEnableGammaCorr, BOOL bEnableWaitForVerticalBlank, BOOL bEnableISPResizer)
{
    BOOL    bResult = FALSE;
    PHYSICAL_ADDRESS pa;
    DWORD size;

    //
    //  Map LCDC registers
    //
    pa.QuadPart = GetAddressByDevice(m_dssinfo.DSSDevice);
    size = sizeof(LCDC_REGS);
    m_lcdc.regs= (LCDC_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_lcdc.regs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
             L"Failed map LCDC control registers\r\n"
            ));
        goto cleanUp;
        }

	pa.QuadPart = AM33X_PRCM_REGS_PA;
    size = sizeof(AM33X_PRCM_REGS);
    m_lcdc.prcmregs = (AM33X_PRCM_REGS*)MmMapIoSpace(pa, size, FALSE);
    if (m_lcdc.prcmregs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::InitController: "
             L"Failed map PRCM registers\r\n"
            ));
        goto cleanUp;
        }

    // Disable gamma correction based on registry
    if(!bEnableGammaCorr)
        m_bGammaEnable = FALSE;

    // Enable VSYNC code based on registry
    if (bEnableWaitForVerticalBlank)
    	m_dwEnableWaitForVerticalBlank = TRUE;

    //  Initialize power lock critical section
    InitializeCriticalSection( &m_csPowerLock );

    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );

    // Request Pads for LCD
    if (!RequestDevicePads(m_dssinfo.DSSDevice))
    {
        ERRORMSG(TRUE, (L"ERROR: OMAPDisplayController::InitController: "
                     L"Failed to request pads\r\n"
                    ));
        goto cleanUp;
    }

    //  Enable controller power
    SetPowerLevel( D0 );

    //  Unlock access to power level
    LeaveCriticalSection( &m_csPowerLock );
        
    // Open a handle to SmartReflex policy adapter
    //m_hSmartReflexPolicyAdapter = PmxOpenPolicy(SMARTREFLEX_POLICY_NAME);
        
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
    DWORD bpp;

    //  Lock access to power level
    EnterCriticalSection( &m_csPowerLock );

    //  Enable controller power
    SetPowerLevel( D0 );

    //  Get LCD parameters
    LcdPdd_LCD_GetMode(
                    (DWORD*)&m_eLcdPixelFormat,
                    &m_dwLcdWidth,
                    &m_dwLcdHeight,
                    &m_dwPixelClock
                    );

    bpp = PixelFormatToPixelSize(m_eLcdPixelFormat);

    m_lcdc.fb_cur_win_size = bpp * m_dwLcdWidth * m_dwLcdHeight;

    //  Initialize the LCD by calling PDD
    bResult = LcdPdd_LCD_Initialize(&m_lcdc);
    
    //  Set up gamma correction to support contrast control
    SetContrastLevel( m_dwContrastLevel );
    
    m_lcdc.color_mode = m_eLcdPixelFormat;
    lcdc_change_mode(&m_lcdc, m_lcdc.color_mode);
    lcdc_set_update_mode(&m_lcdc, FB_AUTO_UPDATE);
	
    // Could calculate actual period...
    m_dwVsyncPeriod = 1000/60 + 2;//Add delta 2 ms since frameRate is not exactly 60fps

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

    m_lcdc.lcdc_irq =  GetIrqByDevice( m_dssinfo.DSSDevice, NULL); 
	
    // get system interrupt for irq
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &m_lcdc.lcdc_irq,
        sizeof(m_lcdc.lcdc_irq), &m_dwDssSysIntr, sizeof(m_dwDssSysIntr),
        NULL))
    {
        DEBUGMSG(ZONE_ERROR,
            (TEXT("%S: ERROR: Failed map LCDC interrupt(irq=%d)\r\n"), __FUNCTION__,m_lcdc.lcdc_irq));
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
OMAPDisplayController::DssProcessInterrupt(void)
{
    struct lcdc *lcdc = &m_lcdc;
    DWORD irqStatus = 0;


    // Access the regs
    if( AccessRegs() == FALSE )
    {
        // failure will cause lockup because the interrupt will still be pending...
        DEBUGMSG(ZONE_ERROR,(L"AccessRegs failed in DssProcessInterrupt\r\n"));
        ASSERT(0);
        goto cleanUp;
    }
    irqStatus = INREG32(&lcdc->regs->IRQSTATUS);

    if (irqStatus & LCDC_IRQSTATUS_SYNC_LOST) 
    {
		DEBUGMSG(ZONE_ERROR,(L"DssProcessInterrupt: Sync lost!\r\n"));
		SETREG32(&lcdc->regs->IRQSTATUS,LCDC_IRQSTATUS_SYNC_LOST);
	}
    if (irqStatus & LCDC_IRQSTATUS_FUF) 
    {
		DEBUGMSG(ZONE_ERROR,(L"DssProcessInterrupt: Underflow!\r\n"));
		SETREG32(&lcdc->regs->IRQSTATUS,LCDC_IRQSTATUS_FUF);
	}
    if (irqStatus & LCDC_IRQSTATUS_PALETTE_LOADED) 
    {
        LcdPdd_Handle_EndofPalette_int(lcdc);
    } 
    if (irqStatus & (LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1)) 
    {
		// clear frame interrupt
		SETREG32(&lcdc->regs->IRQSTATUS,(LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1)); 
        
        if (irqStatus & LCDC_IRQSTATUS_FRAME_DONE0) 
        {
            OUTREG32(&lcdc->regs->LCDDMA_FB1_BASE,lcdc->fb_pa);
            OUTREG32(&lcdc->regs->LCDDMA_FB1_CEILING,lcdc->fb_pa + lcdc->fb_cur_win_size -1);
            m_last_vsync_dma_channel = 0;
			// update flip status
			m_flip_inprogress = (lcdc->regs->LCDDMA_FB0_BASE == lcdc->fb_pa)? FALSE : TRUE;
        }
        if (irqStatus & LCDC_IRQSTATUS_FRAME_DONE1) 
        {
            OUTREG32(&lcdc->regs->LCDDMA_FB0_BASE,lcdc->fb_pa);
            OUTREG32(&lcdc->regs->LCDDMA_FB0_CEILING,lcdc->fb_pa + lcdc->fb_cur_win_size -1);
            m_last_vsync_dma_channel = 1;	
			// update flip status
			m_flip_inprogress = (INREG32(&lcdc->regs->LCDDMA_FB1_BASE) == lcdc->fb_pa)? FALSE : TRUE;
        }
    }
    if (irqStatus & LCDC_IRQSTATUS_RASTER_DONE) 
    {
		SETREG32(&lcdc->regs->IRQSTATUS,LCDC_IRQSTATUS_RASTER_DONE);
		// generate Vsync Event
        if (m_dwEnableWaitForVerticalBlank)
        {
            SetEvent(m_hVsyncEvent);
        	if(m_enableSGXevent)
            {
                SetEvent(m_hVsyncEventSGX);
                SetEvent(m_hVsyncEventWSEGL);
            }
        }
	}

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
    UINT32 dwLength;	

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;


    //  Get rotation and mirror settings for pipeline output
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror   = g_rgPipelineMapping[ePipeline].bMirror;
    
    //  GFX pipeline
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {

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

    dwLength = pSurface->Height( eRotation) * pSurface->Stride(eRotation);
	
    m_lcdc.regs->LCDDMA_FB0_BASE = pSurface->PhysicalAddr(eRotation, bMirror);
    m_lcdc.regs->LCDDMA_FB0_CEILING = m_lcdc.regs->LCDDMA_FB0_BASE + dwLength -1; 

    m_lcdc.regs->LCDDMA_FB1_BASE = pSurface->PhysicalAddr(eRotation, bMirror);
    m_lcdc.regs->LCDDMA_FB1_CEILING = m_lcdc.regs->LCDDMA_FB1_BASE + dwLength -1; 
	
    //  Result
    bResult = TRUE;         

cleanUp:    
    //  Release regs
    ReleaseRegs();
    
    //  Return result
    return bResult;
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
    /* scaling is not supported by LCDC */
    UNREFERENCED_PARAMETER(ePipeline);
    UNREFERENCED_PARAMETER(pSrcRect);
    UNREFERENCED_PARAMETER(pDestRect);
    return TRUE;	
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::EnablePipeline(
    OMAP_DSS_PIPELINE       ePipeline
    )
{
    /* There is no Pipeline concept in LCDC, return TRUE for primary surface only */
    if(ePipeline == OMAP_DSS_PIPELINE_GFX)
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::DisablePipeline(
    OMAP_DSS_PIPELINE       ePipeline
    )
{
    /* There is no Pipeline concept in LCDC, return TRUE for primary surface only */
    if(ePipeline == OMAP_DSS_PIPELINE_GFX)
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::FlipPipeline(
    OMAP_DSS_PIPELINE       ePipeline,
    OMAPSurface*            pSurface
    )
{
    BOOL    bResult = FALSE;
    OMAP_DSS_ROTATION   eRotation;
    BOOL                bMirror;
    DWORD               dwInterlace;

    if( AccessRegs() == FALSE )
        goto cleanUp;
    //  Get rotation and mirror settings for pipeline output
    eRotation = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror   = g_rgPipelineMapping[ePipeline].bMirror;
    dwInterlace = g_rgPipelineScaling[ePipeline].dwInterlace;

    //  Update GFX pipeline display base address
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        m_lcdc.fb_pa = pSurface->PhysicalAddr(eRotation, bMirror);
        //DEBUGMSG(ZONE_ERROR, (L"OMAPDisplayController::FlipPipeline , new pa is set to %x . \r\n", m_lcdc.fb_pa));
        m_flip_inprogress = TRUE;
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

    //  Access the regs
    if( AccessRegs() == FALSE )
        goto cleanUp;

    //  Get rotation and mirror settings for pipeline output
    eRotation   = g_rgPipelineMapping[ePipeline].eRotation;
    bMirror     = g_rgPipelineMapping[ePipeline].bMirror;
   
    //See if we are flipping to the surface given
    if( ePipeline == OMAP_DSS_PIPELINE_GFX )
    {
        if(m_last_vsync_dma_channel == 0)
            bResult = m_flip_inprogress;
		    if(matchExactSurface)
                bResult &= (m_lcdc.regs->LCDDMA_FB1_BASE == pSurface->PhysicalAddr(eRotation, bMirror));     
        else
            bResult = m_flip_inprogress ;
		
    		if(matchExactSurface)
    			bResult &= (m_lcdc.regs->LCDDMA_FB0_BASE == pSurface->PhysicalAddr(eRotation, bMirror));	   
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
    UNREFERENCED_PARAMETER(ePipeline);
    UNREFERENCED_PARAMETER(lXPos);
    UNREFERENCED_PARAMETER(lYPos);

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
    UNREFERENCED_PARAMETER(ePipeline);
    UNREFERENCED_PARAMETER(eRotation);
	
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
    UNREFERENCED_PARAMETER(ePipeline);
    UNREFERENCED_PARAMETER(bMirror);
	
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

    UNREFERENCED_PARAMETER(ePipeline);
    //  Check if pipeline is enabled; ignore operation if not
    if( ePipeline != OMAP_DSS_PIPELINE_GFX )
        return FALSE;

    //  Update the scaling attributes
    bResult = SetScalingAttribs( ePipeline, pSrcRect, pDestRect );
    UNREFERENCED_PARAMETER(pSrcRect);
    UNREFERENCED_PARAMETER(pDestRect);

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
	
    UNREFERENCED_PARAMETER(eColorKey);
    UNREFERENCED_PARAMETER(eDestination);
    UNREFERENCED_PARAMETER(dwColor);

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

    UNREFERENCED_PARAMETER(eColorKey);
    UNREFERENCED_PARAMETER(eDestination);
    
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
   // memcpy(m_pGammaBufVirt, &(g_dwGammaTable[(NUM_CONTRAST_LEVELS - 1) - m_dwContrastLevel][0]), NUM_GAMMA_VALS*sizeof(DWORD));
    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::SaveRegisters(
    OMAP_DSS_DESTINATION eDestination
    )
{
    BOOL    bResult = FALSE;
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
            
				// device pads
				RequestDevicePads(AM_DEVICE_LCDC);

                //  Enable device clocks
                RequestClock( m_dssinfo.DSSDevice );         

                //  Call PDD layer
                LcdPdd_SetPowerLevel( &m_lcdc, dwPowerLevel );
            }

        break;
        
        case D3:
        case D4:
            //  Check against current level
            if( m_dwPowerLevel == D0 || m_dwPowerLevel == D1 || m_dwPowerLevel == D2)
            {
                //  Call PDD layer (again in case LCD was not enabled)
                LcdPdd_SetPowerLevel(&m_lcdc,  dwPowerLevel );
				
				WaitForFrameDone(DISPLAY_TIMEOUT);

        		// device pads for low power
				ReleaseDevicePads(AM_DEVICE_LCDC);

        		//  Disable device clocks 
                ReleaseClock( m_dssinfo.DSSDevice );         
        	}	
            
            //  Set the new power level
            m_dwPowerLevel = dwPowerLevel;

            break;            
    }

cleanUp:    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetDSS()
{
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetDISPC()
{
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ResetVENC()
{
    return TRUE;
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
    UNREFERENCED_PARAMETER(dwDestGo);
    //  Return result
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
OMAPDisplayController::WaitForFlushDone(
    DWORD   dwDestGo
    )
{
    UNREFERENCED_PARAMETER(dwDestGo);
    //  Return result
    return TRUE;
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
    DWORD irqStatus = 0;
    irqStatus = m_lcdc.regs->IRQSTATUS;

    //  Wait for VYSNC status
    while (((INREG32(&m_lcdc.regs->IRQSTATUS) & (LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1)) == 0) 
		   && (dwTimeout-- > 0))
    {
        // delay
        Sleep(1);
    }

    if ( dwTimeout == 0 )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: OMAPDisplayController::WaitForFrameDone: timeout\r\n"));
        return FALSE;
    }

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

        case OMAP_DSS_PIXELFORMAT_RGB24:
            //  3 bytes per pixel
            dwResult = 3;
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
    DEBUGMSG(ZONE_ERROR, (L"EnableVSyncInterruptEx: \r\n"));
    
    if (!m_dwEnableWaitForVerticalBlank)
        return FALSE;

    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }
	
	bInterruptAlreadyEnabled = m_enableSGXevent;

cleanup:

    ReleaseRegs();
    return bInterruptAlreadyEnabled;
}

//------------------------------------------------------------------------------
void OMAPDisplayController::EnableVSyncInterrupt()
{
    DEBUGMSG(ZONE_ERROR, (L"EnableVSyncInterrupt: \r\n"));
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }
    m_enableSGXevent = TRUE;
cleanup:

    ReleaseRegs();
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::DisableVSyncInterrupt()
{
    DEBUGMSG(ZONE_ERROR, (L"DisableVSyncInterrupt: \r\n"));
    m_lastVsyncIRQStatus = 0;
    if(AccessRegs() == FALSE)
    {
        goto cleanup;
    }
	m_enableSGXevent = FALSE;

cleanup:

    ReleaseRegs();
}

//------------------------------------------------------------------------------
BOOL OMAPDisplayController::InVSync(BOOL bClearStatus)
{
    BOOL bInVSync = TRUE;
    DWORD irqStatus = 0;
	
    UNREFERENCED_PARAMETER(bClearStatus);
    if (!m_dwEnableWaitForVerticalBlank)
        return TRUE;

    if(AccessRegs() == FALSE)
    {
        bInVSync = TRUE;
        goto cleanup;
    }
    irqStatus = m_lcdc.regs->IRQSTATUS & (LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1);
    if(!irqStatus) bInVSync = FALSE;
cleanup:

    ReleaseRegs();
    DEBUGMSG(ZONE_ERROR, (L"InVSync: bInVSync=%d\r\n", bInVSync));

    return bInVSync;
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::WaitForVsync()
{
	BOOL bVsyncPreviouslyEnabled = FALSE;

    DEBUGMSG(ZONE_ERROR,(L"WaitForVsync: m_dwEnableWaitForVerticalBlank=%d\r\n", m_dwEnableWaitForVerticalBlank));

    if (!m_dwEnableWaitForVerticalBlank)
        return;

    if(!InVSync(TRUE))
    {
		bVsyncPreviouslyEnabled = EnableVSyncInterruptEx();
        WaitForSingleObject(m_hVsyncEvent, m_dwVsyncPeriod);
		DEBUGMSG(ZONE_ERROR,(L"WaitForVsync: m_hVsyncEvent is set\r\n"));
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
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::DisableScanLineInterrupt()
{
}

//------------------------------------------------------------------------------
DWORD OMAPDisplayController::GetScanLine()
{
    DWORD scanLine = 0;
    RETAILMSG (1, (L"GetScanLine!!!!!!!!!!\r\n"));
    return scanLine;
}

//------------------------------------------------------------------------------
VOID OMAPDisplayController::WaitForScanLine(DWORD dwLineNumber)
{
    // Not supported
        return;
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
    return NULL;
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


//-------------------------------------------------------------------------------
VOID
OMAPDisplayController::EnableLPR(BOOL bEnable, BOOL bHdmiEnable)
{
    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(bHdmiEnable);

}

//------------------------------------------------------------------------------
VOID   
OMAPDisplayController::EnableOverlayOptimization(BOOL bEnable)
{
    UNREFERENCED_PARAMETER(bEnable);
}

//----------------------------------------------------------------------------
BOOL 
OMAPDisplayController::SetDssFclk(
    OMAP_DSS_FCLK      eDssFclkSource,
    OMAP_DSS_FCLKVALUE eDssFclkValue
    )
{
    
    UNREFERENCED_PARAMETER(eDssFclkValue);
    UNREFERENCED_PARAMETER(eDssFclkSource);

    return TRUE;
}

//----------------------------------------------------------------------------
BOOL
OMAPDisplayController::InitDsiPll()
{
    return TRUE;
}

//-------------------------------------------------------------------------------
BOOL
OMAPDisplayController::DeInitDsiPll()
{
    return TRUE;
}

//-------------------------------------------------------------------------------
BOOL
OMAPDisplayController::ConfigureDsiPll(ULONG clockFreq)
{

    return TRUE;
}


// ---------------------------------------------------------------------------
BOOL
OMAPDisplayController::SwitchDssFclk(
    OMAP_DSS_FCLK eFclkSrc, 
    OMAP_DSS_FCLKVALUE eFclkValue
    )
{
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
