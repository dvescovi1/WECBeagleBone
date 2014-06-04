// -----------------------------------------------------------------------------
//
//  Module Name:    lcdgpp.cpp
//
// -----------------------------------------------------------------------------

#include "precomp.h"
#include <syspal.h>
#include <am33x.h>
#include <oal_clock.h>
#include "bsp.h"

//------------------------------------------------------------------------------
//  Debug zone settings
//INSTANTIATE_GPE_ZONES(0xC807,"OMAPDDGPE Driver","Video Memory","DDraw HAL")
INSTANTIATE_GPE_ZONES(0xFFFFFFFF,"OMAPDDGPE Driver","Video Memory","DDraw HAL")

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//------------------------------------------------------------------------------
//  Globals

DDGPE * g_pGPE = (DDGPE *)NULL;

// Bit masks defining the RED, GREEN and BLUE channels of the 32bit pixels
ULONG gBitMasks[] = { 0x00ff0000,0x0000ff00,0x000000ff };

//------------------------------------------------------------------------------
//  Prototypes

DWORD WINAPI LCDVSyncISTProc (LPVOID pParams);

void lcd_debug(  LCDC_REGS * pLcd );

BOOL
APIENTRY
GPEEnableDriver(
    ULONG           engineVersion,
    ULONG           cj,
    DRVENABLEDATA * data,
    PENGCALLBACKS   engineCallbacks
    );


BOOL
APIENTRY
DrvEnableDriver(
    ULONG           engineVersion,
    ULONG           cj,
    DRVENABLEDATA * data,
    PENGCALLBACKS   engineCallbacks
    )
{
    return GPEEnableDriver(engineVersion, cj, data, engineCallbacks);
}


ULONG *
APIENTRY
DrvGetMasks(
    DHPDEV dhpdev
    )
{
    return gBitMasks;
}


//
// Main entry point for a GPE-compliant driver
//
GPE *
GetGPE()
{
    if (!g_pGPE)
    {
        LCDDDGPE *pGPE = new LCDDDGPE();
        
        if (pGPE && !pGPE->Init())
        {
            delete pGPE;
            pGPE = NULL;
        }

        g_pGPE = pGPE;
    }

    DEBUGCHK(g_pGPE != NULL);

    return g_pGPE;
}


//------------------------------------------------------------------------------
//
//  Method: Constructor
//
//  Clears members  
//
LCDDDGPE::LCDDDGPE()
{
    DEBUGMSG(GPE_ZONE_INIT, ((L"Display driver starting\r\n"))); 

    m_colorDepth = 0;
    m_hDisplay = NULL;
    m_pMode = &m_ModeInfo;
    m_currentDX = D4;
    m_pVideoMemoryHeap = NULL;
    m_iRotate = GetRotateModeFromReg();

    // Advertise that we are a power-managable device
    AdvertisePowerInterface();
}

//------------------------------------------------------------------------------
//
//  Method: Destructor
//
//  Releases all resources allocated by the driver
//
LCDDDGPE::~LCDDDGPE()
{
    // Thread

    // Interrupts
    CloseHandle( m_Lcdc.lcd_int_event );

    // Events
    CloseHandle( m_VSyncEvent );
    CloseHandle( m_VSyncFlipFinished );

    // Mutexes
    CloseHandle( m_VSyncBufferMutex );

    // Delete LCD object
    if (m_hDisplay != NULL) 
    {
        DisplayPddDeinit(m_hDisplay);
    }
    
    if (m_Lcdc.regs != NULL) 
    {
        MmUnmapIoSpace((VOID*)m_Lcdc.regs, sizeof(LCDC_REGS));
    }

    if (m_Lcdc.fb_va != NULL)
    {
        MmUnmapIoSpace(m_Lcdc.fb_va, m_Lcdc.fb_size);        
    }

    if (m_pVideoMemoryHeap != NULL)
    {
        delete m_pVideoMemoryHeap;
    }
}

//------------------------------------------------------------------------------
//
//  Method: SetMode
//
//  Gets display configuration from the lower level driver, and allocate
//  resources for the driver to use.
//
SCODE LCDDDGPE::SetMode(INT modeId, HPALETTE *palette)
{
    PHYSICAL_ADDRESS pa;
    SCODE sc;

    DEBUGMSG(GPE_ZONE_INIT,(TEXT("LCDDDGPE::SetMode\r\n")));

    // Release previous setup
    if (m_pPrimarySurface)
    {
        delete[] m_pPrimarySurface;
        m_pPrimarySurface = NULL;
    }

    if (m_Lcdc.fb_va != NULL)
    {
        MmUnmapIoSpace(m_Lcdc.fb_va, m_Lcdc.fb_size);        
        m_Lcdc.fb_va = NULL;
    }

    
    // Convert to GPEMode
    if (!LcdMode2GpeMode(modeId, m_pMode))
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: LcdMode2GpeMode failed!\n"));
        return E_FAIL;
    }
    
    // Save mode info
    m_nScreenWidth = m_nScreenWidthSave = m_pMode->width;
    m_nScreenHeight = m_nScreenHeightSave = m_pMode->height;
    m_colorDepth = m_pMode->Bpp;
   
    // Set rotate params
    SetRotateParams();
    
    // Allocate frame buffer
    pa.QuadPart = m_Lcdc.fb_pa;
    m_Lcdc.fb_va = MmMapIoSpace(pa, m_Lcdc.fb_size, FALSE);
    if (NULL == m_Lcdc.fb_va)
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: Unable to allocate frame buffer!!!\n"));
        return E_FAIL;
    }

    // Manager for video RAM - called during surface allocations
    if (!m_pVideoMemoryHeap)
        m_pVideoMemoryHeap = new SurfaceHeap(m_Lcdc.fb_size, (UINT32)m_Lcdc.fb_va, NULL, NULL);

    // Create primary surface using the normal allocator
    if (!m_pPrimarySurface)
    {
        if (FAILED(sc = AllocSurface(&m_pPrimarySurface, m_pMode->width, m_pMode->height, m_pMode->format, GPE_REQUIRE_VIDEO_MEMORY)))
        {
            DEBUGMSG(GPE_ZONE_ERROR, (TEXT("Couldn't allocate primary surface\r\n")));
            return sc;
        }
    }

    // Clear all VRAM to black
//dv    memset( (PUCHAR)m_Lcdc.fb_va, 0, m_Lcdc.fb_size );

    m_pPrimarySurface->SetRotation(m_nScreenWidth, m_nScreenHeight, m_iRotate);

    // Setup palette
    if (palette)
    {
        switch (m_colorDepth)
        {
            case 8:
                *palette = EngCreatePalette (PAL_INDEXED,
                                             PALETTE_SIZE,
                                             (ULONG*)_rgbIdentity,
                                             0,
                                             0,
                                             0);
                break;

            case 16:
            case 24:
            case 32:
                *palette = EngCreatePalette (PAL_BITFIELDS,
                                             0,
                                             NULL,
                                             gBitMasks[ 0 ],
                                             gBitMasks[ 1 ],
                                             gBitMasks[ 2 ]);

                break;
        }

        if (*palette == NULL)
        {
            DEBUGMSG(GPE_ZONE_ERROR, (L"Couldn't create palette\n"));

            return E_FAIL;
        }
    }

    // Set power state to D0. 
    SetPower(D0);

    // PDD specific mode setup
    if (!DisplayPddSetMode(m_hDisplay, modeId)) 
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: LCDGPE::SetMode: "
            L"LCD failed set mode %d\r\n", modeId));
        return E_FAIL;
    }

    LcdSetMode();

    // Display the primary surface
    SetVisibleSurface( m_pPrimarySurface, TRUE );

    // PDD specific display enable
    if (!DisplayPddEnableDisplay(m_hDisplay, modeId))
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: LCDGPE::SetMode: "
            L"LCD failed enable display %d\r\n", modeId));
        return E_FAIL;
    }

    // Initialize cursor
    m_CursorVisible = FALSE;
    m_CursorDisabled = TRUE;
    m_CursorForcedOff = FALSE;
    memset(&m_CursorRect, 0x0, sizeof(m_CursorRect));

    return S_OK;
}

//------------------------------------------------------------------------------
//  Method:  GetModeInfo
//
//  Populate the supplied structure with details of the specified mode
//
SCODE LCDDDGPE::GetModeInfo(GPEMode *mode, INT modeNumber)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::GetModeInfo\r\n")));
        
    // Convert to GPEMode
    if (!LcdMode2GpeMode(modeNumber, mode))
    {
        DEBUGMSG(GPE_ZONE_INIT, (L"ERROR: LcdMode2GpeMode failed!\n"));
        return E_FAIL;
    }

    // Swap dimensions if rotated
    if (m_iRotate == DMDO_90 || m_iRotate == DMDO_270)
    {
        int width = mode->width;
        mode->width = mode->height;
        mode->height = width;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
//  Method:  NumModes
//
//  Returns the number of video modes supported by the driver
//
int LCDDDGPE::NumModes()
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::NumModes\r\n")));
    return DisplayPddNumModes(m_hDisplay);
}


//------------------------------------------------------------------------------
//  Method:  GetPhysicalVideoMemory
//
//  Return the physical address and size of the framebuffer
//
void LCDDDGPE::GetPhysicalVideoMemory(unsigned long *physicalMemoryBase, unsigned long *videoMemorySize)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::GetPhysicalVideoMemory\r\n")));

    *physicalMemoryBase = m_Lcdc.fb_pa;
    *videoMemorySize    = m_Lcdc.fb_size;
}


//------------------------------------------------------------------------------
//  Method:  GetVirtualVideoMemory
//
//  Return the virtual address and size of the framebuffer
//
void LCDDDGPE::GetVirtualVideoMemory(unsigned long *virtualMemoryBase, unsigned long *videoMemorySize)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::GetVirtualVideoMemory\r\n")));

    *virtualMemoryBase = (UINT32)m_Lcdc.fb_va;
    *videoMemorySize   = m_Lcdc.fb_size;
}


//------------------------------------------------------------------------------
//  Method:  Init
//
//  Initialise the low-level driver, map registers into memory, and create
//  threads and events.
//
BOOL LCDDDGPE::Init()
{
    BOOL    bResult;
    PHYSICAL_ADDRESS pa;
	DWORD dwPixelSize;

    DEBUGMSG(GPE_ZONE_INIT,(TEXT("LCDDDGPE::LCDDDGPE\r\n")));

    // First initialize LCD display module connected to the controller
    if ((m_hDisplay = DisplayPddInit(L"")) == NULL) {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: LCDGPE::Init: "
            L"Failed initialize LCD display object\r\n"));
        return FALSE;
    }

    pa.QuadPart = GetAddressByDevice(AM_DEVICE_LCDC);
    m_Lcdc.regs = (LCDC_REGS*)MmMapIoSpace(pa, sizeof(LCDC_REGS), FALSE);
    if (m_Lcdc.regs == NULL) {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: LCDGPE::LCDGPE: "
            L"Failed to map LCDC registers (pa = 0x%08x)\r\n", pa.LowPart));
        return FALSE;
    }
    
    // Request Pads for LCD
    if (!RequestDevicePads(AM_DEVICE_LCDC))
    {
        ERRORMSG(TRUE, (L"ERROR: OMAPDisplayController::InitController: "
                     L"Failed to request pads\r\n"
                    ));
        return FALSE;
    }

    EnableDeviceClocks(AM_DEVICE_LCDC, TRUE);

	m_Lcdc.clk = PrcmClockGetClockRate(LCD_PCLK);
	
    //  Initialize the LCD by calling PDD
    bResult = LcdPdd_LCD_Initialize(&m_Lcdc);

    m_Lcdc.fb_cur_win_size = lcdc_PixelFormatToPixelSize(m_Lcdc.panel->pixel_format) * m_Lcdc.panel->x_res * m_Lcdc.panel->y_res;
	m_Lcdc.color_mode = m_Lcdc.panel->pixel_format;

	lcdc_change_mode(&m_Lcdc, m_Lcdc.color_mode);

    m_Queued_FrameBuffer = m_Lcdc.fb_pa;
    m_Queued_FrameBufferSize = m_Lcdc.fb_pa + m_Lcdc.fb_cur_win_size - 4;  // Stop on last DWORD (off by 1)
	
    lcdc_set_update_mode(&m_Lcdc, FB_AUTO_UPDATE);
	
	m_pVideoMemoryHeap = NULL;

    // Initialize the infrastructure for detecting the VSync events
    m_Lcdc.lcdc_irq = IRQ_LCDCINT;

    if ( FALSE == KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &m_Lcdc.lcdc_irq,
                                  sizeof(DWORD),(LPVOID) &m_Lcdc.sys_interrupt,
                                  sizeof(DWORD), NULL))
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to get SYSINTR.\r\n"));
        return FALSE;
    }

    DEBUGMSG(GPE_ZONE_INIT, (TEXT("LCDGPE: VSync Irq 0x%x is sysintr 0x%x\r\n"), m_Lcdc.lcdc_irq, m_Lcdc.sys_interrupt));
    
    // LCD interrupt set on every H/W IRQ.
    m_Lcdc.lcd_int_event = CreateEvent( NULL, FALSE, FALSE, NULL);
    if (NULL == m_Lcdc.lcd_int_event)
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to create event.\r\n"));
        return FALSE;
    }

    // Pulsed on every Vsync.
    m_VSyncEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (NULL == m_VSyncEvent)
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to create m_VSyncEvent event.\r\n"));
        return FALSE;
    }

    // Cleared when waiting for flip, set when finished. (default = finished = SET)
    m_VSyncFlipFinished = CreateEvent (NULL, TRUE, TRUE, NULL);
    if (NULL == m_VSyncFlipFinished)
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to create m_VSyncFlipFinished event.\r\n"));
        return FALSE;
    }

    // Protection mutex around next framebuffer address. (default = UN_OWNED)
    m_VSyncBufferMutex = CreateMutex (NULL, FALSE, NULL);
    if (NULL == m_VSyncBufferMutex)
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to create m_VSyncBufferMutex event.\r\n"));
        return FALSE;
    }

    // Interrupt management Thread
    m_Lcdc.lcd_int_thread = CreateThread (NULL, 0,
                                            LCDVSyncISTProc,
                                            this, 0, NULL);

    if (NULL == m_Lcdc.lcd_int_thread)
    {
        ERRORMSG( TRUE, (L"InitializeHardware: Unable to create IST.\r\n"));
        return FALSE;
    }


    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Method:  LcdMode2GpeMode
//
//  Get config settings (bpp, refresh rate, size etc..) for the specified mode.
//
BOOL LCDDDGPE::LcdMode2GpeMode(INT nModeId, GPEMode *pGpeInfo)
{
    // Validate input args
    if (!pGpeInfo)
    {
        return FALSE;
    }

    // Get the resolution from the LCD configuration
	pGpeInfo->width = m_Lcdc.panel->x_res;
    pGpeInfo->height = m_Lcdc.panel->y_res;
    pGpeInfo->frequency = 60;

	switch(m_Lcdc.panel->pixel_format)
	{
        case DISPC_PIXELFORMAT_RGB16:
        case DISPC_PIXELFORMAT_ARGB16:
            pGpeInfo->format = gpe16Bpp;
			pGpeInfo->Bpp = 16;
            break;

        case DISPC_PIXELFORMAT_YUV2:
        case DISPC_PIXELFORMAT_UYVY:
            pGpeInfo->format = gpe16YCrCb;
			pGpeInfo->Bpp = 16;
            break;
			
        case DISPC_PIXELFORMAT_RGB24:
            pGpeInfo->format = gpe24Bpp;
			pGpeInfo->Bpp = 24;
            break;

        case DISPC_PIXELFORMAT_RGB32:
        case DISPC_PIXELFORMAT_ARGB32:
        case DISPC_PIXELFORMAT_RGBA32:
            pGpeInfo->format = gpe32Bpp;
			pGpeInfo->Bpp = 32;
            break;
    }
	
    pGpeInfo->modeId = nModeId;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method:  SetPower
//
//  Extra things we could do here to save more power :
//           - Power down the DAC's : DACTST.DAPD3:0 = 0
//           - Gate the clock       : PCR.CLK_OFF    = 1
//  For a lower power usage you could always do a more complete power 
//  off of the whole VPSS via the PSC but would need to consider
//  VPFE usage as well.
//
BOOL LCDDDGPE::SetPower(CEDEVICE_POWER_STATE dx)
{
    BOOL    bRetVal = FALSE;

    //DEBUGMSG(1, (L"LCDGPE::SetPower State - %d\r\n",dx));

    if (dx == D1 || dx == D2)   // On is D0, D1 or D2
        dx = D0;
    else if (dx == D3)          // Off is D3 and D4
        dx = D4;
    

    // only do something if the state has changed
    if (m_currentDX != dx)
    {
        if (dx <= D2)
        {     
            DEBUGMSG(1, (L"LCDGPE::SetPower switching on LCD controller\r\n"));

//            PSCSetModuleState( PSC_MODULE_LCDC, PSC_MDCTL_NEXT_ENABLE);

            // Power on LCD device after powering on controller
            DisplayPddSetPower(m_hDisplay, dx);
        }
        else if (dx >= D3)
        {  
            // Power off LCD device before powering off controller
            DisplayPddSetPower(m_hDisplay, dx);

            DEBUGMSG(1, (L"LCDGPE::SetPower switching off LCD controller\r\n"));
            
//            PSCSetModuleState( PSC_MODULE_LCDC, PSC_MDCTL_NEXT_DISABLE);
        }
        
        // Update current power state
        m_currentDX = dx;
        bRetVal = TRUE;
    }

    //DEBUGMSG(1, (L"-LCDGPE::SetPower State - %d\r\n",dx));
    return bRetVal;
}


//------------------------------------------------------------------------------
//
//  Function:  lcd_debug
// 
//  Print contents of LCD controller registers
//
//
void lcd_debug(LCDC_REGS * pLcd)
{
    RETAILMSG(TRUE, ((L"----\r\n")));
    RETAILMSG(TRUE, ((L"pLcd->PID =            		0x%08x  addr=0x%08x\r\n"), pLcd->PID,               &pLcd->PID));
    RETAILMSG(TRUE, ((L"pLcd->CTRL =           		0x%08x  addr=0x%08x\r\n"), pLcd->CTRL,              &pLcd->CTRL));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CTRL =      		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CTRL,         &pLcd->LIDD_CTRL));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS0_CONF =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS0_CONF,     &pLcd->LIDD_CS0_CONF));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS0_ADDR =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS0_ADDR,     &pLcd->LIDD_CS0_ADDR));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS0_DATA =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS0_DATA,     &pLcd->LIDD_CS0_DATA));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS1_CONF =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS1_CONF,     &pLcd->LIDD_CS1_CONF));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS1_ADDR =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS1_ADDR,     &pLcd->LIDD_CS1_ADDR));
    //RETAILMSG(TRUE, ((L"pLcd->LIDD_CS1_DATA =  		0x%08x  addr=0x%08x\r\n"), pLcd->LIDD_CS1_DATA,     &pLcd->LIDD_CS1_DATA));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_CTRL =       	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_CTRL,       &pLcd->RASTER_CTRL));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_TIMING_0 =   	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_TIMING_0,   &pLcd->RASTER_TIMING_0));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_TIMING_1 =   	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_TIMING_1,   &pLcd->RASTER_TIMING_1));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_TIMING_2 =   	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_TIMING_2,   &pLcd->RASTER_TIMING_2));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_SUBPANEL =   	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_SUBPANEL,   &pLcd->RASTER_SUBPANEL));
    RETAILMSG(TRUE, ((L"pLcd->RASTER_SUBPANEL2 =  	0x%08x  addr=0x%08x\r\n"), pLcd->RASTER_SUBPANEL2,  &pLcd->RASTER_SUBPANEL2));
    RETAILMSG(TRUE, ((L"pLcd->LCDDMA_CTRL =       	0x%08x  addr=0x%08x\r\n"), pLcd->LCDDMA_CTRL,       &pLcd->LCDDMA_CTRL));
    RETAILMSG(TRUE, ((L"pLcd->LCDDMA_FB0_BASE =   	0x%08x  addr=0x%08x\r\n"), pLcd->LCDDMA_FB0_BASE,   &pLcd->LCDDMA_FB0_BASE));
    RETAILMSG(TRUE, ((L"pLcd->LCDDMA_FB0_CEILING =	0x%08x  addr=0x%08x\r\n"), pLcd->LCDDMA_FB0_CEILING,&pLcd->LCDDMA_FB0_CEILING));
    RETAILMSG(TRUE, ((L"pLcd->LCDDMA_FB1_BASE =   	0x%08x  addr=0x%08x\r\n"), pLcd->LCDDMA_FB1_BASE,   &pLcd->LCDDMA_FB1_BASE));
    RETAILMSG(TRUE, ((L"pLcd->LCDDMA_FB1_CEILING =	0x%08x  addr=0x%08x\r\n"), pLcd->LCDDMA_FB1_CEILING,&pLcd->LCDDMA_FB1_CEILING));    
    //RETAILMSG(TRUE, ((L"pLcd->SYSCONFIG =         	0x%08x  addr=0x%08x\r\n"), pLcd->SYSCONFIG,         &pLcd->SYSCONFIG));    
    //RETAILMSG(TRUE, ((L"pLcd->IRQSTATUS_RAW =     	0x%08x  addr=0x%08x\r\n"), pLcd->IRQSTATUS_RAW,     &pLcd->IRQSTATUS_RAW));    
    //RETAILMSG(TRUE, ((L"pLcd->IRQSTATUS =        	0x%08x  addr=0x%08x\r\n"), pLcd->IRQSTATUS,         &pLcd->IRQSTATUS));    
    //RETAILMSG(TRUE, ((L"pLcd->IRQENABLE_SET =     	0x%08x  addr=0x%08x\r\n"), pLcd->IRQENABLE_SET,     &pLcd->IRQENABLE_SET));    
    //RETAILMSG(TRUE, ((L"pLcd->IRQENABLE_CLEAR =   	0x%08x  addr=0x%08x\r\n"), pLcd->IRQENABLE_CLEAR,   &pLcd->IRQENABLE_CLEAR));    
    //RETAILMSG(TRUE, ((L"pLcd->CLKC_ENABLE =       	0x%08x  addr=0x%08x\r\n"), pLcd->CLKC_ENABLE,       &pLcd->CLKC_ENABLE));    
    //RETAILMSG(TRUE, ((L"pLcd->CLKC_RESET =        	0x%08x  addr=0x%08x\r\n"), pLcd->CLKC_RESET,        &pLcd->CLKC_RESET));    
}



//------------------------------------------------------------------------------
//
//  Function:  PaletteSetup
//
//  The LCD controller always reads the palette, even in non-palettised modes
//  because the first entry of the palette defines the bit-depth of the
//  framebuffer.  So we must setup the palette BEFORE displaying anthing
//  otherwise the framebuffer will be interpreted as a 1 bit bitmap.
//
//
void    LCDDDGPE::PaletteSetup( void )
{
    //DWORD timeout;
    //DWORD i;
    //UINT16 *pPal;
    //UINT16 *pStart;

    //// Although the palette is not used in 16 bit mode,
    //// The first 32 byte (16 words) are still treated
    //// as a palette.
    //// These must be loaded into the LCD controller as
    //// as seperate step before the framebuffer can be
    //// displayed.

    //// Entry 0 (16 bits) specifies the colour format (0x4000 = 16bit RGB565)
    //pStart = pPal;
    //*pPal = 0x4000;
    //pPal++;

    //// For RGB565 mode, the other 15 palette entries must be zeros
    //for ( i=0; i<15; i++ )
    //{
    //    *pPal = 0x0000;
    //    pPal++;
    //}


    //// Wait a moment for the palette to be loaded.
    //timeout = WAIT_FOR_PAL_LOAD_TIMEOUT;
    //while ( (timeout > 0) && !(m_Lcdc->regs->LCD_STAT & (1 << OMAPL13X_LCDC_LC_STAT_PL_SHIFT)) )
    //{
    //    Sleep(1);
    //    timeout --;
    //}

    //// Disable LCD controller
    //m_Lcdc->regs->raster_control &= ~OMAPL13X_LCDC_RASTER_CTRL_RASTER_EN_MASK;

    //// Set PLM for loading framebuffer data
    //m_Lcdc->regs->raster_control &= ~OMAPL13X_LCDC_RASTER_CTRL_PLM_MASK;
    //m_Lcdc->regs->raster_control |= (2 << OMAPL13X_LCDC_RASTER_CTRL_PLM_SHIFT);

    //// Increase DMA burst size for normal framebuffer data.
    //m_Lcdc->regs->dma_control  = 0;
    //m_Lcdc->regs->dma_control |= (4 << OMAPL13X_LCDC_LCDDMA_CTRL_BURST_SIZE_SHIFT);

    //// Enable double buffering mode so we can flip between two framebuffers
    //m_Lcdc->regs->dma_control |= (1 << OMAPL13X_LCDC_LCDDMA_CTRL_FRAME_MODE_SHIFT);
}


//------------------------------------------------------------------------------
//
//  Function:  LcdSetMode
//
//  Configures for the particular panel, but does not enable the display yet
//  - LCD controller must be disabled before calling, then enabled afterwards//
//
//
void LCDDDGPE::LcdSetMode( void )
{
    //DWORD lines;
    //DWORD a, b, c, d;
    //DWORD acb_i, acb;
    //UINT32 dwSysclockSpeed;    
    //DWORD dwDivisor;
    //DWORD rc;
    //
    //// LCD controller must be disabled before configuration
    //LcdDisable();

    //// Select LCD mode
    //// ---------------------

    //// Work out the required divisor by finding the frequency supplied
    //// to the LCD module and the desired LCD pixel clock
    //
    //// Get bus clock rate (LCD controller is fed from PLL0_SYSCLK2)
    //rc = PLLCGetClockFreq(PLLC_0, PLLC_SYSCLK_2, &dwSysclockSpeed);
    //if (!rc) {
    //    RETAILMSG(1,(L"ERROR: LcdSetMode: Failed to read LCDC bus clock rate\r\n"));
    //    return;    
    //}

    //
    //// enable raster mode
    //m_Lcdc->regs->control &= ~OMAPL13X_LCDC_LCD_CTRL_MODESEL_MASK;
    //m_Lcdc->regs->control |= OMAPL13X_LCDC_LCD_CTRL_MODESEL_RASTER;


    //// LCD panel parameters
    //// ---------------------

    //// Configure raster mode, and also make sure LCD controller is disabled
    //m_Lcdc->regs->raster_control = (16 << OMAPL13X_LCDC_RASTER_CTRL_FIFO_DMA_DELAY_SHIFT) |
    //                          (1 << OMAPL13X_LCDC_RASTER_CTRL_RD_ORDER_SHIFT) |
    //                          (1 << OMAPL13X_LCDC_RASTER_CTRL_TFT_STN_SHIFT);
    //       

    //// Horizontal Timing
    //a = m_LCDParams.hstartvaliddata;
    //b = m_LCDParams.numpxlperline;
    //c = m_LCDParams.hdsyncwidth;
    //m_Lcdc->regs->timing0 = ((a) << OMAPL13X_LCDC_RASTER_TIMING_0_HBP_SHIFT) |         // Back Porch
    //                              ((b-(a+d)) << OMAPL13X_LCDC_RASTER_TIMING_0_HFP_SHIFT) |   // Front Porch
    //                              ((c-1) << OMAPL13X_LCDC_RASTER_TIMING_0_HSW_SHIFT) |       // Sync
    //                              (((d / 16) - 1) << OMAPL13X_LCDC_RASTER_TIMING_0_PPL_SHIFT);        // Active Pixels

    //// Vertical timing
    //a = m_LCDParams.vstartvaliddata;
    //b = m_LCDParams.numlinperfld;
    //c = m_LCDParams.vdsyncwidth;
    //m_Lcdc->regs->timing1 = ((a) << OMAPL13X_LCDC_RASTER_TIMING_1_VBP_SHIFT) |         // Back Porch
    //                              ((b-(a+d)) << OMAPL13X_LCDC_RASTER_TIMING_1_VFP_SHIFT) |   // Front Porch
    //                              ((c-1) << OMAPL13X_LCDC_RASTER_TIMING_1_VSW_SHIFT) |       // Sync
    //                              ((d-1) << OMAPL13X_LCDC_RASTER_TIMING_1_LPP_SHIFT);        // Active Lines

    //// Sync and clock setup
    //m_Lcdc->regs->timing2 = (1 << OMAPL13X_LCDC_RASTER_TIMING_2_SYNC_CTRL_SHIFT);
    //
    //if (m_LCDParams.syncedge == LCD_SYNCEDGE_FALLING)
    //{
    //    m_Lcdc->regs->timing2 |= 1 << OMAPL13X_LCDC_RASTER_TIMING_2_SYNC_EDGE_SHIFT;
    //}

    //if (m_LCDParams.pixclkpol == LCD_PIXELCLKPOL_NEGATIVE)
    //{
    //    m_Lcdc->regs->timing2 |= 1 << OMAPL13X_LCDC_RASTER_TIMING_2_IPC_SHIFT;
    //}


    //// Set Vertical Sync Polarity (default = +ve)
    //if (m_LCDParams.vdsyncpol == LCD_SYNCPOL_NEGATIVE)
    //{
    //    m_Lcdc->regs->timing2 |= (1 << OMAPL13X_LCDC_RASTER_TIMING_2_IVS_SHIFT);
    //}
    //// Set Vertical Sync Polarity (default = +ve)
    //if (m_LCDParams.hdsyncpol == LCD_SYNCPOL_NEGATIVE)
    //{
    //    m_Lcdc->regs->timing2 |= (1 << OMAPL13X_LCDC_RASTER_TIMING_2_IHS_SHIFT);
    //}

    //// Subpanel control (use part of the LCD)
    //m_Lcdc->regs->subpanel1 = 0;  // disabled!


    //// The LCD controller does not have a vsync IRQ.
    //// However, it can generate an IRQ after a defined number of HSYNCs.  This is intended
    //// for generating the AC bias signal for STN panels.
    //// We can use it to generate an IRQ at least twice every frame, then manually interrogate
    //// the 'current framebuffer id' bits in LCD_STAT to work out when the frame has changed.
    //// Then we can choose the correct buffer for the next backbuffer flip.

    //// This algorithm chooses the suitable multiples for ACB and ACB_I registers
    //acb_i = 0;
    //do
    //{
    //    acb_i ++;
    //    acb = lines / acb_i;
    //}
    //while ( acb > ACB_MAX );

    //if ( (acb_i > ACB_I_MAX) || (acb > ACB_MAX) )
    //{
    //    acb_i = ACB_I_MAX;
    //    acb   = ACB_MAX;
    //}
    //DEBUGMSG(GPE_ZONE_INIT, ((L"acb_i=%d  acb=%d\r\n"), acb_i, acb));
    //m_Lcdc->regs->timing2 |= (acb_i << OMAPL13X_LCDC_RASTER_TIMING_2_ACB_I_SHIFT);
    //m_Lcdc->regs->timing2 |= (acb << OMAPL13X_LCDC_RASTER_TIMING_2_ACB_SHIFT);

    //// The bitdepth (bpp) is set using the palette entries
    //// which must be loaded into the LCD controller's palette area.
    //PaletteSetup();

    ////lcd_debug(m_Lcdc->regs);    
}


//------------------------------------------------------------------------------
//
//  Function:  LcdSetFrameBufferOnVsync
//
//  Set framebuffer address, size and enable controller on the next Vsync cycle
//
//
void    LCDDDGPE::LcdSetFrameBufferOnVsync( UINT32 FrameBuffer, UINT32 Size )
{
    if ( WaitForSingleObject(m_VSyncBufferMutex, SET_FRAME_BUFFER_TIMEOUT) == WAIT_OBJECT_0 )
    {
        m_Queued_FrameBuffer = FrameBuffer;
        m_Queued_FrameBufferSize = FrameBuffer + Size - 4;  // Stop on last DWORD (off by 1)
        ResetEvent( m_VSyncFlipFinished );
        ReleaseMutex( m_VSyncBufferMutex );
    }
}


//------------------------------------------------------------------------------
//
//  Function:  LcdSetFrameBufferImmediate
//
//  Set framebuffer address, size and enable controller imemdiately!
//
//  !!WARNING!!
//  This must *ONLY* be called when the LCD controller is diabled otherwise,
//  since both FB0 and FB1 are updated, the DMA engine could end up wandering
//  past the end of the buffers into invalid memory.
//
//  This happens because the DMA count starts at 'BASE' and continues until
//  the counter == 'CEILING'.  If 'CEILING' is set to a lower address while
//  DMA is running, then 'CEILING' will never be reached!
//  
//
void    LCDDDGPE::LcdSetFrameBufferImmediate( UINT32 FrameBuffer, UINT32 Size )
{
    m_Lcdc.regs->LCDDMA_FB0_BASE = FrameBuffer;
    m_Lcdc.regs->LCDDMA_FB0_CEILING = FrameBuffer + Size - 4;  // Stop on last DWORD (off by 1)

    m_Lcdc.regs->LCDDMA_FB1_BASE = FrameBuffer;
    m_Lcdc.regs->LCDDMA_FB1_CEILING = FrameBuffer + Size - 4;  // Stop on last DWORD (off by 1)
}


//------------------------------------------------------------------------------
//
//  Function:  LcdEnable
//
//  Turn on controller
//
// 
void    LCDDDGPE::LcdEnable( void )
{
	m_Lcdc.regs->RASTER_CTRL |= LCDC_RASTER_CTRL_LCD_EN;
}


//------------------------------------------------------------------------------
//
//  Function:  LcdDisable
//
//  Turn off controller
//
// 
void    LCDDDGPE::LcdDisable( void )
{
	m_Lcdc.regs->RASTER_CTRL &= ~LCDC_RASTER_CTRL_LCD_EN;
}


//------------------------------------------------------------------------------
//
//  Function:  LCDVSyncISTProc
//
//  Wrapper passed to CreateThread, which then redirects to the IST function
//  'VSyncISTImpl' of the LCDDDGPE class.
//
DWORD WINAPI LCDVSyncISTProc(LPVOID pParams)
{
    LCDDDGPE * pLCDDDGPEObj = (LCDDDGPE * ) pParams;

    pLCDDDGPEObj->VSyncISTImpl( pParams );
    return 0;
}


//------------------------------------------------------------------------------
//
//  Method:  VSyncISTImpl
//
//  IST wich services any LCD controller interrupt
//  Will pulses 'm_VSyncEvent' whenever a raster frame done (vsync)
//  interrupt occurs.
//
DWORD LCDDDGPE::VSyncISTImpl(LPVOID pParams)
{
    DWORD dwIrqStatusNew;
    DWORD dwIrqStatusOld;
    DWORD dwBufferIdNew;
    DWORD dwBufferIdOld;

    if (FALSE == CeSetThreadPriority( m_Lcdc.lcd_int_thread, 150))
    {
        ERRORMSG( TRUE, (L"VSyncISTImpl: Unable to Set IST priority.\r\n"));
    }

    if ((NULL == m_Lcdc.lcd_int_event) ||
        (NULL == m_Lcdc.sys_interrupt))
    {
        ERRORMSG( TRUE, (L"VSyncISTImpl: Event ID or the SYSINTR numbers are invalid.\r\n"));
        return (-1);
    }

    // Initialize the interupt
    if (FALSE == InterruptInitialize (m_Lcdc.sys_interrupt, m_Lcdc.lcd_int_event, NULL, 0))
    {
        ERRORMSG( TRUE, (L"VSyncISTImpl: Unable to initialize the Interrupt.\r\n"));
        return (-1);
    }

    // Track what has changed.
    dwBufferIdNew = 0;
    dwBufferIdOld = 0;
    dwIrqStatusNew = 0;
    dwIrqStatusOld = 0;

    DEBUGMSG(GPE_ZONE_FLIP, ((L"Start vsync-IRQ thread: IRQSTATUS_RAW=0x%08x\r\n"), m_Lcdc.regs->IRQSTATUS_RAW) );
    while (TRUE)
    {
		if (WAIT_FAILED == WaitForSingleObject( m_Lcdc.lcd_int_event, INFINITE))
		{
			ERRORMSG( TRUE, (L"VSyncISTImpl: WaitForSingleObject failed.\r\n"));
			break;
		}

		dwIrqStatusNew = INREG32(&m_Lcdc.regs->IRQSTATUS);

		// Check for error conditions
		if (dwIrqStatusNew & LCDC_IRQSTATUS_SYNC_LOST)
		{
			DEBUGMSG(GPE_ZONE_ERROR, (L"VSyncISTImpl: Sync error\r\n"));
			SETREG32(&m_Lcdc.regs->IRQSTATUS,LCDC_IRQSTATUS_SYNC_LOST);
		}
		if (dwIrqStatusNew & LCDC_IRQSTATUS_FUF)
		{
			DEBUGMSG(GPE_ZONE_ERROR, (L"VSyncISTImpl: Underflow error\r\n"));
			SETREG32(&m_Lcdc.regs->IRQSTATUS,LCDC_IRQSTATUS_FUF);
		}
		if (dwIrqStatusNew & LCDC_IRQSTATUS_PALETTE_LOADED) 
		{
			LcdPdd_Handle_EndofPalette_int(&m_Lcdc);
		} 
        
        // There must have been a framebuffer change
        if ( dwIrqStatusNew & (LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1) )
        {
			// clear frame interrupt
			SETREG32(&m_Lcdc.regs->IRQSTATUS,(LCDC_IRQSTATUS_FRAME_DONE0 | LCDC_IRQSTATUS_FRAME_DONE1)); 

            // Check for loss of sync (bits for both frames 0 and 1 set at the same time)
            if ( (dwIrqStatusNew & LCDC_IRQSTATUS_FRAME_DONE0) && (dwIrqStatusNew & LCDC_IRQSTATUS_FRAME_DONE1) )
            {
                // Can we infer which buffer based on the previous frame?
                // The old status bits will ALWAYS make sense because we set them ourselves
                // if anything went wrong!  This means that if we lose sync, the system will
                // effectively freewheel from the last accurately known frame ID.
                if ( dwIrqStatusOld & LCDC_IRQSTATUS_FRAME_DONE1)
                {
                    dwIrqStatusNew = LCDC_IRQSTATUS_FRAME_DONE0;
                }
                else
                {
                    dwIrqStatusNew = LCDC_IRQSTATUS_FRAME_DONE1;
                }
            }

            if ( dwIrqStatusNew & LCDC_IRQSTATUS_FRAME_DONE0 )  dwBufferIdNew = 0;
            else                                           dwBufferIdNew = 1;
        }

        // Put pending data into buffers
        if ( dwBufferIdNew != dwBufferIdOld )
        {
            // Only enter if nothing else is trying to queue a new display buffer
            // (which would cause [m_VSyncBufferMutex] to be set)
            if ( WaitForSingleObject(m_VSyncBufferMutex, 0) == WAIT_OBJECT_0 )
            {
                // Set DMA pointer for the next buffer
                if ( dwBufferIdNew == 0 )
                {
                    OUTREG32(&m_Lcdc.regs->LCDDMA_FB0_BASE, m_Queued_FrameBuffer);
                    OUTREG32(&m_Lcdc.regs->LCDDMA_FB0_CEILING, m_Queued_FrameBufferSize);  // Stop on last DWORD (off by 1)
                }
                else
                {
                    OUTREG32(&m_Lcdc.regs->LCDDMA_FB1_BASE, m_Queued_FrameBuffer);
                    OUTREG32(&m_Lcdc.regs->LCDDMA_FB1_CEILING, m_Queued_FrameBufferSize);  // Stop on last DWORD (off by 1)
                }

                // Set event to signal the flip has completed
                SetEvent( m_VSyncFlipFinished );
                ReleaseMutex( m_VSyncBufferMutex );
				//RETAILMSG( TRUE, ((L"%d n=0x%08x o=0x%08x\r\n"), dwBufferIdNew, dwIrqStatusNew, dwIrqStatusOld) );
			}

            dwIrqStatusOld = dwIrqStatusNew;
            dwBufferIdOld = dwBufferIdNew;
        }

		if (dwIrqStatusNew & LCDC_IRQSTATUS_RASTER_DONE)
		{
			SETREG32(&m_Lcdc.regs->IRQSTATUS,LCDC_IRQSTATUS_RASTER_DONE);
            // Signal that vsync IRQ has occured
            // This will unblock any threads waiting for the a vsync event
            PulseEvent( m_VSyncEvent );
		}

		// Prepare for next IRQ
        InterruptDone (m_Lcdc.sys_interrupt);
    }
    DEBUGMSG(GPE_ZONE_FLIP, ((L"Finish vsync-IRQ thread: IRQSTATUS_RAW=0x%08x\r\n"), m_Lcdc.regs->IRQSTATUS_RAW) );

    return 0;
}




// <end of file>

