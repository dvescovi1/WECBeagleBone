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

//#define DO_DISPPERF
//#define DISPPERF_DECLARE

#include "precomp.h"
#include "dispperf.h"


//------------------------------------------------------------------------------
//  Debug zone settings
//INSTANTIATE_GPE_ZONES(0xC807,"OMAPDDGPE Driver","Video Memory","DDraw HAL")
INSTANTIATE_GPE_ZONES(0xFFFF,"OMAPDDGPE Driver","Video Memory","DDraw HAL")

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//------------------------------------------------------------------------------
//  Defines

#define ESC_SUCCESS                 1
#define ESC_FAILED                  -1
#define ESC_NOT_SUPPORTED           0

//------------------------------------------------------------------------------
//  Globals

DDGPE * g_pGPE = (DDGPE *)NULL;
OMAPDDGPEGlobals    g_Globals;

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    {
        L"OffscreenMemory", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwOffscreenMemory),
        fieldsize(OMAPDDGPEGlobals, m_dwOffscreenMemory), (VOID*)(4*1024*1024)
    }, {
        L"Angle", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwRotationAngle),
        fieldsize(OMAPDDGPEGlobals, m_dwRotationAngle), (VOID*)0
    }, {
        L"Cursor", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwCursorEnable),
        fieldsize(OMAPDDGPEGlobals, m_dwCursorEnable), (VOID*)0
    }, {
        L"IRQ", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwDSSIRQ),
        fieldsize(OMAPDDGPEGlobals, m_dwDSSIRQ), NULL
    }, {
        L"ISTPriority", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwISTPriority),
        fieldsize(OMAPDDGPEGlobals, m_dwISTPriority), (VOID*)100
    }, {
        L"EnableGammaCorr", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwEnableGammaCorr),
        fieldsize(OMAPDDGPEGlobals, m_dwEnableGammaCorr), (VOID*)1
    }, {
        L"EnableWaitForVerticalBlank", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwEnableWaitForVerticalBlank),
        fieldsize(OMAPDDGPEGlobals, m_dwEnableWaitForVerticalBlank), (VOID*)0
    }, {
        L"EnableNeonBlts", PARAM_DWORD, TRUE, offset(OMAPDDGPEGlobals, m_dwEnableNeonBlts),
        fieldsize(OMAPDDGPEGlobals, m_dwEnableNeonBlts), (VOID*)0
    },{
        L"EnableDMABlts", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwEnableDMABlts),
        fieldsize(OMAPDDGPEGlobals, m_dwEnableDMABlts), (VOID*)1
    },{
        L"EnableOverlay", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwEnableOverlay),
        fieldsize(OMAPDDGPEGlobals, m_dwEnableOverlay), (VOID*)1
    },{
        L"HardwareRotation", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwHardwareRotation),
        fieldsize(OMAPDDGPEGlobals, m_dwHardwareRotation), (VOID*)1
    },{
        L"DMAFill", PARAM_DWORD, FALSE, offset(OMAPDDGPEGlobals, m_dwDMAFill),
        fieldsize(OMAPDDGPEGlobals, m_dwDMAFill), (VOID*)1
    }
    
};

//------------------------------------------------------------------------------
//  Prototypes

BOOL
APIENTRY
GPEEnableDriver(
    ULONG           engineVersion,
    ULONG           cj,
    DRVENABLEDATA * data,
    PENGCALLBACKS   engineCallbacks
    );

static
BOOL
ConvertStringToGuid(
    LPCWSTR szGuid,
    GUID *pGuid
    );


//------------------------------------------------------------------------------
//  Display Driver entry points

BOOL
APIENTRY
DisplayInit(
    LPCTSTR pszInstance,
    DWORD dwNumMonitors
    )
{
    UNREFERENCED_PARAMETER(dwNumMonitors);
    DEBUGMSG(GPE_ZONE_INIT, (L"Display DisplayInit\r\n"));

    // called with pszInstance set to NULL for non-PCI display drivers...
    if (pszInstance == NULL)
    {
        pszInstance = L"System\\GDI\\Drivers";
        DEBUGMSG(GPE_ZONE_INIT, (L"Display DisplayInit, NULL pszInstance, using %s\r\n", pszInstance));
    }

    //  Read display driver initialization parameters
    if( GetDeviceRegistryParams(pszInstance, &g_Globals, dimof(g_deviceRegParams),
        g_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: DisplayInit failed to read registry\r\n"));
        return FALSE;
    }
    if (pszInstance != NULL) _tcscpy_s(g_Globals.m_szContext,_countof(g_Globals.m_szContext), pszInstance);

    return TRUE;
}

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
    ULONG*  pMasks = NULL;

    UNREFERENCED_PARAMETER(dhpdev);
    
    //  Return masks for selected mode
    if( g_pGPE )
    {
        pMasks = ((OMAPDDGPE*)g_pGPE)->GetMasks();
    }

    //  Return
    return pMasks;
}

/* Translate WinCE orientation macros to angles in degrees */
DWORD getAngle (DWORD orientation)
{
    DWORD angle = 0;
    switch(orientation)
    {
        case DMDO_0:
            angle=0;
            break;
        case DMDO_90:
            angle=90;
            break;
        case DMDO_180:
            angle=180;
            break;
        case DMDO_270:
            angle=270;
            break;
        default:
            angle=0;
            break;
    }
    return angle;
}

/* Translate angles in degrees to WinCE orientation macros*/
DWORD getOrientation (DWORD angle)
{
    DWORD orientation = 0;
    switch(angle)
    {
        case 0:
            orientation=DMDO_0;
            break;
        case 90:
            orientation=DMDO_90;
            break;
        case 180:
            orientation=DMDO_180;
            break;
        case 270:
            orientation=DMDO_270;
            break;
        default:
            orientation=DMDO_0;
            break;
    }
    return orientation;
}

//------------------------------------------------------------------------------
//
//  OMAPDDGPE methods
//

OMAPDDGPE::OMAPDDGPE()
{
    GUID    guid;
    DWORD   dwWidth,
            dwHeight;
    TCHAR   szTemp[MAX_PATH];

    DEBUGMSG(GPE_ZONE_INIT,(TEXT("OMAPDDGPE::OMAPDDGPE\r\n")));


    //  No overlay surfaces enabled
    m_pOverlay1Surf = NULL;
    m_pOverlay2Surf = NULL;

    // Convert display power class to GUID
    if (!ConvertStringToGuid(PMCLASS_DISPLAY, &guid))
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: Failed convert display power class '%s' to GUID\r\n"));
        return;
    }

	// Get module's file name
	if (!GetModuleFileName(g_hmodDisplayDll, szTemp, sizeof(szTemp) / sizeof(szTemp[0])))
	{
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: Failed to retrieve module file name\r\n"));
        return;
	}

    // Advertise power class interface
    if (!AdvertiseInterface(&guid, szTemp, TRUE))
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"ERROR: Failed advertise display power class '%s'\r\n"));
        return;
    }


    //  Allocate display controller
    m_pDisplayContr = new OMAPDisplayController();
    if (m_pDisplayContr == NULL)
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to allocate display controller\r\n"));
        return;
    }

    //  Setting for Gamma Correction
    m_bEnableGammaCorr = (g_Globals.m_dwEnableGammaCorr) ? TRUE : FALSE;

    //  Initialize the controller
    if( m_pDisplayContr->InitController(m_bEnableGammaCorr, g_Globals.m_dwEnableWaitForVerticalBlank,FALSE) == FALSE )
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to initialize display controller\r\n"));
        delete m_pDisplayContr;
        m_pDisplayContr = NULL;
        return;
    }

    //  Initialize the LCD
    if( m_pDisplayContr->InitLCD() == FALSE )
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to initialize LCD\r\n"));
        delete m_pDisplayContr;
        m_pDisplayContr = NULL;
        return;
    }

    //  Initialize the DSS interrupt handler
    if( m_pDisplayContr->InitInterrupts(g_Globals.m_dwDSSIRQ, g_Globals.m_dwISTPriority) == FALSE )
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to initialize DSS interrupts!\r\n"));
        delete m_pDisplayContr;
        m_pDisplayContr = NULL;
        return;
    }

    //  Allocate and intialize surface memory manager
	m_pSurfaceMgr = new OMAPFlatSurfaceManager();

    if (m_pSurfaceMgr == NULL)
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to allocate surface manager\r\n"));
        delete m_pDisplayContr;
        m_pDisplayContr = NULL;
        return;
    }

    if( m_pSurfaceMgr->Initialize(g_Globals.m_dwOffscreenMemory) == FALSE )
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to initialize surface manager\r\n"));
        delete m_pDisplayContr;
        delete m_pSurfaceMgr;
        m_pDisplayContr = NULL;
        m_pSurfaceMgr = NULL;
        return;
    }


    //  Associate the surface manager with the display controller
    m_pDisplayContr->SetSurfaceMgr( m_pSurfaceMgr );


    //  Check for initial screen orientation
    if( m_pSurfaceMgr->SupportsRotation() == TRUE )
    {
        m_iGraphicsRotate = g_Globals.m_dwRotationAngle;
		dwWidth  = m_pDisplayContr->GetLCDWidth();
        dwHeight = m_pDisplayContr->GetLCDHeight();

		
		
		
		////  Set initial rotation angle
  //      switch( g_Globals.m_dwRotationAngle )
  //      {
  //          case DMDO_0:
  //          case DMDO_180:
  //              //  Valid rotation values
  //              m_iGraphicsRotate = g_Globals.m_dwRotationAngle;
  //              dwWidth  = m_pDisplayContr->GetLCDWidth();
  //              dwHeight = m_pDisplayContr->GetLCDHeight();
  //              break;

  //          case DMDO_90:
  //          case DMDO_270:
  //              //  Valid rotation values
  //              m_iGraphicsRotate = g_Globals.m_dwRotationAngle;
  //              dwWidth  = m_pDisplayContr->GetLCDHeight();
  //              dwHeight = m_pDisplayContr->GetLCDWidth();
  //              break;

  //          default:
  //              //  Default to angle 0
  //              m_iGraphicsRotate = DMDO_0;
  //              g_Globals.m_dwRotationAngle = DMDO_0;
  //              dwWidth  = m_pDisplayContr->GetLCDWidth();
  //              dwHeight = m_pDisplayContr->GetLCDHeight();
  //              break;
  //      }
    }
    else
    {
        //  Default values for non-rotatable surface managers
        m_iGraphicsRotate = DMDO_0;
        g_Globals.m_dwRotationAngle = DMDO_0;
		dwWidth  = m_pDisplayContr->GetLCDWidth();
        dwHeight = m_pDisplayContr->GetLCDHeight();
    }


    //  Update GPE attributes to match LCD defaults
    m_nScreenWidth  = dwWidth;
    m_nScreenHeight = dwHeight;

    m_pModeEx = &m_gpeDefaultMode;
    m_pMode = &m_gpeDefaultMode.modeInfo;
    memset(m_pModeEx, 0, sizeof(GPEModeEx));
    m_pModeEx->dwSize = sizeof(GPEModeEx);
    m_pModeEx->dwVersion = GPEMODEEX_CURRENTVERSION;

    m_pMode->modeId = 0;
    m_pMode->width = dwWidth;
    m_pMode->height = dwHeight;
    m_pMode->frequency = 60;
    m_pMode->Bpp = OMAPDDGPE::PixelFormatToBpp(m_pDisplayContr->GetLCDPixelFormat());
    m_pMode->format = OMAPDDGPE::PixelFormatToGPEFormat(m_pDisplayContr->GetLCDPixelFormat());
    m_pModeEx->ePixelFormat = OMAPDDGPE::PixelFormatToDDGPEFormat(m_pDisplayContr->GetLCDPixelFormat());


    //  Initialize a critical section for display driver operations that need serialization
    InitializeCriticalSection( &m_csOperationLock );

    m_cursorVisible = FALSE;
    m_cursorDisabled = TRUE;
    m_cursorForcedOff = FALSE;
    memset(&m_cursorRect, 0x0, sizeof(m_cursorRect));
    m_cursorStore = NULL;
    m_cursorXor = NULL;
    m_cursorAnd = NULL;
	
#if DEBUG_NEON_MEMORY_LEAK
    m_bNeonBlt = FALSE;
	m_dwNeonBltCount = 0;
    m_dwNeonBltCountLastMessage = 0;
	m_dwAvailPhysDelta = 0;
	m_dwAvailPageFileDelta = 0;
#endif
}

//------------------------------------------------------------------------------
OMAPDDGPE::~OMAPDDGPE()
{
    //  Display driver never unloads
#if 0
    delete [] m_cursorStore;
    m_cursorStore = NULL;
    delete [] m_cursorXor;
    m_cursorXor = NULL;
    delete [] m_cursorAnd;
    m_cursorAnd = NULL;
#endif
}

//------------------------------------------------------------------------------
int
OMAPDDGPE::NumModes()
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("OMAPDDGPE::NumModes\r\n")));

    //  Return number of modes (the display driver allows only one active mode at a time)
    return 1;
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::GetModeInfo(
    GPEMode *pMode,
    int modeNumber
    )
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("OMAPDDGPE::GetModeInfo\r\n")));

    //  Check display controller enabled
    if( m_pDisplayContr == NULL )
        return E_FAIL;

    //  Return info about selected mode
    if( modeNumber == 0 )
    {
        *pMode = *m_pMode;
		return S_OK;
    }

    return E_INVALIDARG;
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::GetModeInfoEx(
    GPEModeEx *pModeEx,
    int modeNumber
    )
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("OMAPDDGPE::GetModeInfoEx\r\n")));

    //  Check display controller enabled
    if( m_pDisplayContr == NULL )
        return E_FAIL;

    //  Return info about selected mode
    if( modeNumber == 0 )
    {
        *pModeEx = *m_pModeEx;
        return S_OK;
    }

    return E_INVALIDARG;
}

//------------------------------------------------------------------------------
OMAP_DSS_PIXELFORMAT
OMAPDDGPE::GetPrimaryPixelFormat()
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("OMAPDDGPE::GetPrimaryPixelFormat\r\n")));

    //  Return pixel format of primary LCD
    return m_pDisplayContr->GetLCDPixelFormat();
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::SetMode(
    INT modeId,
    HPALETTE *palette
    )
{
    SCODE   scResult = E_FAIL;
    BOOL    bResult;
    int iswap;

    DEBUGMSG(GPE_ZONE_INIT,(TEXT("OMAPDDGPE::SetMode\r\n")));


    //  Check display controller enabled
    if( m_pDisplayContr == NULL )
        return E_FAIL;

    //  Only a single, default mode is supported
    if( modeId == 0 )
    {
		// Save mode info
		m_nScreenWidth = m_nScreenWidthSave = m_pMode->width;
		m_nScreenHeight = m_nScreenHeightSave = m_pMode->height;

		m_iRotate = m_iGraphicsRotate;
//set rotate param
		switch(m_iRotate)
		{
		case DMDO_0:
			m_nScreenHeightSave = m_nScreenHeight;
			m_nScreenWidthSave  = m_nScreenWidth;
			break;

		case DMDO_180:
			m_nScreenHeightSave = m_nScreenHeight;
			m_nScreenWidthSave  = m_nScreenWidth;
			break;

		case DMDO_90:
		case DMDO_270:
			iswap               = m_nScreenHeight;
			m_nScreenHeight     = m_nScreenWidth;
			m_nScreenWidth      = iswap;
			m_nScreenHeightSave = m_nScreenWidth;
			m_nScreenWidthSave  = m_nScreenHeight;
			break;

		default:
			m_nScreenHeightSave = m_nScreenHeight;
			m_nScreenWidthSave  = m_nScreenWidth;
			break;
		}

		
		
		
		//  Allocate display primary surface for LCD
        //scResult = AllocSurface(
        //                &m_pPrimarySurf,
        //                m_pDisplayContr->GetLCDPixelFormat(),
        //                m_pDisplayContr->GetLCDWidth(),
        //                m_pDisplayContr->GetLCDHeight() );
        scResult = AllocSurface(
                        &m_pPrimarySurf,
                        m_pDisplayContr->GetLCDPixelFormat(),
                        m_pMode->width,
                        m_pMode->height );
                        //m_nScreenWidth,
                        //m_nScreenHeight );
        if (scResult != S_OK)
        {
            DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to allocate primary surface\r\n"));
            goto cleanUp;
        }

        //  Set GPE primary surface pointer
        m_pPrimarySurface = m_pPrimarySurf;

    m_pPrimarySurface->SetRotation(m_nScreenWidth, m_nScreenHeight, m_iRotate);
//    m_pPrimarySurface->SetRotation(m_nScreenHeight, m_nScreenWidth, m_iRotate);

		//  Set the desired initial rotation angle if rotation is supported by the surface manager
        if( m_pSurfaceMgr->SupportsRotation() == TRUE )
        {
            if(g_Globals.m_dwHardwareRotation)
            {
                //  Change width and height of primary surface, but not the surface angle of the GPE surface
                //  Have the display controller rotate the output of the primary surface
                switch( m_iGraphicsRotate )
                {
                    case DMDO_0:
                        //  Set the rotation and orientation of the primary surface
                        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_STANDARD );
    
                        //  Set the output rotation angle for the pipeline
                        m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_0 );
                        break;
    
                    case DMDO_90:
                        //  Set the rotation and orientation of the primary surface
                        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_ROTATED );
    
                        //  Set the output rotation angle for the pipeline
                        m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_90 );
                        break;
    
                    case DMDO_180:
                        //  Set the rotation and orientation of the primary surface
                        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_STANDARD );
    
                        //  Set the output rotation angle for the pipeline
                        m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_180 );
                        break;
    
                    case DMDO_270:
                        //  Set the rotation and orientation of the primary surface
                        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_ROTATED );
    
                        //  Set the output rotation angle for the pipeline
                        m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_270 );
                        break;
                }
            }
            else
            {
                //m_nScreenHeightSave = m_pMode->height;
                //m_nScreenWidthSave = m_pMode->width;
//                DynRotate(m_iGraphicsRotate);
//                DynRotate(m_iRotate);
            }
        }

        //  Setup the attributes of the primary surface pipeline to match LCD attributes
        bResult = m_pDisplayContr->SetPipelineAttribs(
                                    OMAP_DSS_PIPELINE_GFX,
                                    OMAP_DSS_DESTINATION_LCD,
                                    m_pPrimarySurf->OmapSurface() );
        if( bResult == FALSE )
        {
            DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to set graphics pipeline attributes\r\n"));
            goto cleanUp;
        }

        //  Enable the pipeline
        bResult = m_pDisplayContr->EnablePipeline(
                                    OMAP_DSS_PIPELINE_GFX );
        if( bResult == FALSE )
        {
            DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to enable graphics pipeline\r\n"));
            goto cleanUp;
        }




        //  Update extended mode information
        //m_pMode->width = m_pPrimarySurf->OmapSurface()->Width();
        //m_pMode->height = m_pPrimarySurf->OmapSurface()->Height();
        //m_pModeEx->lPitch = m_pPrimarySurf->OmapSurface()->Stride();

        OMAPDDGPE::PixelFormatToBitMask(m_pPrimarySurf->OmapSurface()->PixelFormat(),
                                        &m_pModeEx->dwAlphaBitMask,
                                        &m_pModeEx->dwRBitMask,
                                        &m_pModeEx->dwGBitMask,
                                        &m_pModeEx->dwBBitMask);


        //  Allocate palette for selected mode
        if (palette)
        {
            ULONG   uNumColors = (m_pModeEx->dwAlphaBitMask == 0) ? 3 : 4;
            ULONG   uColors[4];

            //  Colors are always in RGBA order
            uColors[0] = m_pModeEx->dwRBitMask;
            uColors[1] = m_pModeEx->dwGBitMask;
            uColors[2] = m_pModeEx->dwBBitMask;
            uColors[3] = m_pModeEx->dwAlphaBitMask;

            //  Create palette info for selected mode
            *palette = EngCreatePalette (
                            PAL_BITFIELDS,
                            uNumColors,
                            uColors,
                            0,
                            0,
                            0);
        }

        //  Success
        scResult = S_OK;
    }
    else
    {
        DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Invalid modeId value %d\r\n", modeId));
        scResult = E_INVALIDARG;
    }

cleanUp:
    //  Retrun result
    return scResult;
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::SetPalette(
    const PALETTEENTRY *source,
    USHORT firstEntry,
    USHORT numEntries
    )
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(firstEntry);
    UNREFERENCED_PARAMETER(numEntries);
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("OMAPDDGPE::SetPalette\r\n")));
    return    S_OK;
}

//------------------------------------------------------------------------------
void
OMAPDDGPE::GetVirtualVideoMemory(
    unsigned long *virtualMemoryBase,
    unsigned long * videoMemorySize,
    unsigned long * videoMemoryFree
    )
{
    DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("OMAPDDGPE::GetVirtualVideoMemory\r\n")));

    *virtualMemoryBase = (ULONG) m_pSurfaceMgr->VirtualBaseAddr();
    *videoMemorySize   = m_pSurfaceMgr->TotalMemorySize();
    *videoMemoryFree   = m_pSurfaceMgr->FreeMemorySize();

    DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    virtualMemoryBase = 0x%08x\r\n"), *virtualMemoryBase));
    DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    videoMemorySize   = 0x%08x\r\n"), *videoMemorySize));
    DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    videoMemoryFree   = 0x%08x\r\n"), *videoMemoryFree));
}

//------------------------------------------------------------------------------
#if (_WINCEOSVER==700)
void
OMAPDDGPE::GetVirtualVideoMemoryList(
    LPDDHAL_DDVIDMEMLIST    pVidMemList
    )
{
    DWORD   i;
    DWORD   dwAddr = (DWORD) m_pSurfaceMgr->VirtualBaseAddr();


    DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("OMAPDDGPE::GetVirtualVideoMemoryList\r\n")));

    //  Report all the video memory segments
    //  VM is contiguous, but physical memory is not
    pVidMemList->cVidMemBlocks = m_pSurfaceMgr->NumPhysicalAddr();

    for( i = 0; i < m_pSurfaceMgr->NumPhysicalAddr(); i++ )
    {
        DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    VirtualMem[%d]  = 0x%08x\r\n"), i, dwAddr));
        DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    PhysicalMem[%d] = 0x%08x\r\n"), i, m_pSurfaceMgr->PhysicalAddr(i)));
        DEBUGMSG (GPE_ZONE_VIDEOMEMORY, (TEXT("    PhysicalLen[%d] = 0x%08x\r\n"), i, m_pSurfaceMgr->PhysicalLen(i)));

        pVidMemList->rgVidMem[i].pVidMemBase  = (void*) dwAddr;
        pVidMemList->rgVidMem[i].dwVidMemSize = m_pSurfaceMgr->PhysicalLen(i);

        dwAddr += m_pSurfaceMgr->PhysicalLen(i);
}
}
#endif

//------------------------------------------------------------------------------
void
OMAPDDGPE::DDHALInfo(
    LPDDHALINFO lpddhi,
    DWORD modeidx
    )
{
    UNREFERENCED_PARAMETER(lpddhi);
    UNREFERENCED_PARAMETER(modeidx);
}

//------------------------------------------------------------------------------
ULONG *
OMAPDDGPE::GetMasks()
{
    DEBUGMSG (GPE_ZONE_HW, (TEXT("OMAPDDGPE::GetMasks\r\n")));

    //  Return RGB masks for selected mode
    return &(m_pModeEx->dwRBitMask);
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::SetPointerShape(
    GPESurf *pMask,
    GPESurf *pColorSurf,
    int xHotspot,
    int yHotspot,
    int xSize,
    int ySize
    )
{
    SCODE sc = S_OK;
    UCHAR *pAndPtr, *pXorPtr, *pAndLine, *pXorLine;
    UCHAR andPtr, xorPtr, mask;
    ULONG size;
    int bytesPerPixel;
    int row, col, i;

    UNREFERENCED_PARAMETER(pColorSurf);

    if (!g_Globals.m_dwCursorEnable)
        return S_OK;

    DEBUGMSG(GPE_ZONE_CURSOR, (
        L"+OMAPDDGPE::SetPointerShape(0x%08x, 0x%08x, %d, %d, %d, %d)\r\n",
        pMask, pColorSurf, xHotspot, yHotspot, xSize, ySize
    ));

    Lock();

    // Turn current cursor off
    CursorOff();

    // Release memory associated with old cursor
    delete [] m_cursorStore;
    m_cursorStore = NULL;
    delete [] m_cursorXor;
    m_cursorXor = NULL;
    delete [] m_cursorAnd;
    m_cursorAnd = NULL;

    // Is there a new mask?
    if (pMask == NULL)
    {
        // No, so tag as disabled
        m_cursorDisabled = TRUE;
    }
    else
    {
        // Yes, so tag as not disabled
        m_cursorDisabled = FALSE;

        // Check if cursor size is correct
        if (xSize > m_nScreenWidth || ySize > m_nScreenHeight)
        {
            DEBUGMSG(GPE_ZONE_ERROR, (L"OMAPDDGPE::SetPointerShape: "
                L"Invalid cursor size %d, %d\r\n", xSize, ySize
            ));
            sc = E_FAIL;
            goto cleanUp;
        }

        // How many bytes we need per pixel on screen
        bytesPerPixel = (m_pMode->Bpp + 7) >> 3;

        // Cursor mask & store size
        size = xSize * ySize * bytesPerPixel;

        // Allocate memory based on new cursor size
        m_cursorStore = new UCHAR[size];
        m_cursorXor   = new UCHAR[size];
        m_cursorAnd   = new UCHAR[size];

        if (m_cursorStore == NULL || m_cursorXor == NULL || m_cursorAnd == NULL)
        {
            DEBUGMSG(GPE_ZONE_ERROR, (L"OMAPDDGPE::SetPointerShape: "
                L"Memory allocation for cursor buffers failed\r\n"
            ));
            sc = E_OUTOFMEMORY;
            goto cleanUp;
        }

        // Store size and hotspot for new cursor
        m_cursorSize.x = xSize;
        m_cursorSize.y = ySize;
        m_cursorHotspot.x = xHotspot;
        m_cursorHotspot.y = yHotspot;

        // Pointers to AND and XOR masks
        pAndPtr = (UCHAR*)pMask->Buffer();
        pXorPtr = (UCHAR*)pMask->Buffer() + (ySize * pMask->Stride());

        // store OR and AND mask for new cursor
        for (row = 0; row < ySize; row++)
        {
            pAndLine = &m_cursorAnd[row * xSize * bytesPerPixel];
            pXorLine = &m_cursorXor[row * xSize * bytesPerPixel];
            for (col = 0; col < xSize; col++)
            {
                andPtr = pAndPtr[row * pMask->Stride() + (col >> 3)];
                xorPtr = pXorPtr[row * pMask->Stride() + (col >> 3)];
                mask = (UCHAR) (0x80 >> (col & 0x7));
                for (i = 0; i < bytesPerPixel; i++)
                {
                    pAndLine[col * bytesPerPixel + i] = andPtr&mask ? 0xFF : 0x00;
                    pXorLine[col * bytesPerPixel + i] = xorPtr&mask ? 0xFF : 0x00;
                }
            }
        }
    }

cleanUp:
    Unlock();
    DEBUGMSG(GPE_ZONE_CURSOR, (L"-OMAPDDGPE::SetPointerShape(sc = 0x%08x)\r\n", sc));
    return sc;
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::MovePointer(
    int xPosition,
    int yPosition
    )
{
    if (!g_Globals.m_dwCursorEnable)
        return S_OK;

    DEBUGMSG(GPE_ZONE_CURSOR, (L"+OMAPDDGPE::MovePointer(%d, %d)\r\n", xPosition, yPosition));

    Lock();

    CursorOff();

    if (xPosition != -1 || yPosition != -1)
    {
        // Compute new cursor rect
        m_cursorRect.left = xPosition - m_cursorHotspot.x;
        m_cursorRect.right = m_cursorRect.left + m_cursorSize.x;
        m_cursorRect.top = yPosition - m_cursorHotspot.y;
        m_cursorRect.bottom = m_cursorRect.top + m_cursorSize.y;
        CursorOn();
    }

    Unlock();

    DEBUGMSG(GPE_ZONE_CURSOR, (L"-OMAPDDGPE::MovePointer(sc = 0x%08x)\r\n", S_OK));
    return  S_OK;
}

//------------------------------------------------------------------------------
//
//  Method: WrapperEmulatedLine
//
//  This function is wrapped around emulated line implementation. It must
//  be used only if software pointer is used. It switches cursor off/on
//  if the line crosses it.
//
SCODE OMAPDDGPE::WrappedEmulatedLine(GPELineParms *lineParameters)
{
    SCODE sc;
    RECT bounds;
    int N_plus_1;

    Lock();

    // If cursor is on check for line overlap
    if (m_cursorVisible && !m_cursorDisabled)
    {
        // Calculate the bounding-rect to determine overlap with cursor
        if (lineParameters->dN)
        {
            // The line has a diagonal component
            N_plus_1 = 2 + (
                (lineParameters->cPels * lineParameters->dN)/lineParameters->dM
            );
        }
        else
        {
            N_plus_1 = 1;
        }

        switch (lineParameters->iDir)
        {
            case 0:
                bounds.left = lineParameters->xStart;
                bounds.top = lineParameters->yStart;
                bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
                bounds.bottom = bounds.top + N_plus_1;
                break;

            case 1:
                bounds.left = lineParameters->xStart;
                bounds.top = lineParameters->yStart;
                bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
                bounds.right = bounds.left + N_plus_1;
                break;

            case 2:
                bounds.right = lineParameters->xStart + 1;
                bounds.top = lineParameters->yStart;
                bounds.bottom = lineParameters->yStart + lineParameters->cPels + 1;
                bounds.left = bounds.right - N_plus_1;
                break;

            case 3:
                bounds.right = lineParameters->xStart + 1;
                bounds.top = lineParameters->yStart;
                bounds.left = lineParameters->xStart - lineParameters->cPels;
                bounds.bottom = bounds.top + N_plus_1;
                break;

            case 4:
                bounds.right = lineParameters->xStart + 1;
                bounds.bottom = lineParameters->yStart + 1;
                bounds.left = lineParameters->xStart - lineParameters->cPels;
                bounds.top = bounds.bottom - N_plus_1;
                break;

            case 5:
                bounds.right = lineParameters->xStart + 1;
                bounds.bottom = lineParameters->yStart + 1;
                bounds.top = lineParameters->yStart - lineParameters->cPels;
                bounds.left = bounds.right - N_plus_1;
                break;

            case 6:
                bounds.left = lineParameters->xStart;
                bounds.bottom = lineParameters->yStart + 1;
                bounds.top = lineParameters->yStart - lineParameters->cPels;
                bounds.right = bounds.left + N_plus_1;
                break;

            case 7:
                bounds.left = lineParameters->xStart;
                bounds.bottom = lineParameters->yStart + 1;
                bounds.right = lineParameters->xStart + lineParameters->cPels + 1;
                bounds.top = bounds.bottom - N_plus_1;
                break;

            default:
                DEBUGMSG(GPE_ZONE_ERROR,(L"OMAPDDGPE::WrappedEmulatedLine: "
                    L"Invalid direction: %d\r\n", lineParameters->iDir
                ));
                sc = E_INVALIDARG;
                goto cleanUp;
        }

        // If line overlap cursor, turn if off
        RECTL cursorRect = m_cursorRect;
        RotateRectl(&cursorRect);

        if (cursorRect.top < bounds.bottom &&
            cursorRect.bottom > bounds.top &&
            cursorRect.left < bounds.right &&
            cursorRect.right > bounds.left
        )
        {
            CursorOff();
            m_cursorForcedOff = TRUE;
        }
    }

    Unlock();

    // Do emulated line
    sc = EmulatedLine(lineParameters);

    // If cursor was forced off turn it back on
    if (m_cursorForcedOff)
    {
        m_cursorForcedOff = FALSE;
        Lock();
        CursorOn();
        Unlock();
    }

cleanUp:
    return sc;
}


//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::Line(
    GPELineParms *lineParameters,
    EGPEPhase phase
    )
{
    DEBUGMSG (GPE_ZONE_LINE, (TEXT("OMAPDDGPE::Line\r\n")));

    if (phase == gpeSingle || phase == gpePrepare)
    {
        DispPerfStart (ROP_LINE);

        if (lineParameters->pDst != m_pPrimarySurface)
        {
            lineParameters->pLine = &GPE::EmulatedLine;
        }
        else
        {
            lineParameters->pLine = (SCODE (GPE::*)(struct GPELineParms *))&OMAPDDGPE::WrappedEmulatedLine;
        }
    }
    else if (phase == gpeComplete)
    {
        DispPerfEnd (0);
    }

    return S_OK;
}

void
OMAPDDGPE::UpdateCursor(GPEBltParms * blitParameters)
{
	RECTL rect;
    LONG swapTmp;

	if (g_Globals.m_dwCursorEnable)
    {
        Lock();

        // Check if destination overlap with cursor
        if (blitParameters->pDst == m_pPrimarySurface && m_cursorVisible && !m_cursorDisabled)
        {
            if (blitParameters->prclDst != NULL)
            {
                rect = *blitParameters->prclDst;     // if so, use it

                // There is no guarantee of a well
                // ordered rect in blitParamters
                // due to flipping and mirroring.
                if (rect.top > rect.bottom)
                {
                    swapTmp = rect.top;
                    rect.top = rect.bottom;
                    rect.bottom = swapTmp;
                }
                if (rect.left > rect.right)
                {
                    swapTmp    = rect.left;
                    rect.left  = rect.right;
                    rect.right = swapTmp;
                }
            }
            else
            {
                rect = m_cursorRect;
            }

            // Turn off cursor if it overlap
            if (
                m_cursorRect.top <= rect.bottom &&
                m_cursorRect.bottom >= rect.top &&
                m_cursorRect.left <= rect.right &&
                m_cursorRect.right >= rect.left
            )
            {
                CursorOff();
                m_cursorForcedOff = TRUE;
            }
        }

        // Check if source overlap with cursor
        if (blitParameters->pSrc == m_pPrimarySurface && m_cursorVisible && !m_cursorDisabled && !m_cursorForcedOff)
        {
            if (blitParameters->prclSrc != NULL) {
                rect = *blitParameters->prclSrc;
            }
            else
            {
                rect = m_cursorRect;
            }
            if (
                m_cursorRect.top < rect.bottom && m_cursorRect.bottom > rect.top &&
                m_cursorRect.left < rect.right && m_cursorRect.right > rect.left
            )
            {
                CursorOff();
                m_cursorForcedOff = TRUE;
            }
        }

        Unlock();
    }
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::BltPrepare(
    GPEBltParms *blitParameters
    )
{
#ifndef SYSGEN_POWERVR
	UpdateCursor(blitParameters);
#endif

    DEBUGMSG (GPE_ZONE_BLT, (TEXT("OMAPDDGPE::BltPrepare\r\n")));

    //  Display perf start
    DispPerfStart( blitParameters->rop4 );

    // Default to base emulated routine
    blitParameters->pBlt = &GPE::EmulatedBlt;

    //  Check for any ROPs that can be accelerated by HW
    if( g_Globals.m_dwEnableDMABlts && blitParameters->pDst->InVideoMemory() )
    {
        switch( blitParameters->rop4 )
        {
            case 0x0000:  // BLACKNESS
                if(g_Globals.m_dwDMAFill)
            	{
                    blitParameters->solidColor = 0;
                    blitParameters->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DMAFill;
					DEBUGMSG(GPE_ZONE_BLT, (TEXT("DMA BLT , rop4=%x\r\n"),  blitParameters->rop4));
            	}
                break;

            case 0xFFFF:  // WHITENESS
                if(g_Globals.m_dwDMAFill)
            	{
                    blitParameters->solidColor = 0x00FFFFFF;
                    blitParameters->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DMAFill;
					DEBUGMSG(GPE_ZONE_BLT, (TEXT("DMA BLT , rop4=%x\r\n"),  blitParameters->rop4));
            	}
                break;

            case 0xF0F0:  // PATCOPY
                // Solid color only
                // Disabled 32 bit sDMA PATCOPY, it causes CETK DirectDraw test failure
                if( g_Globals.m_dwDMAFill && blitParameters->solidColor != -1 && blitParameters->pDst->Format() != gpe32Bpp)
                {
                    blitParameters->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DMAFill;
					DEBUGMSG(GPE_ZONE_BLT, (TEXT("DMA BLT , rop4=%x\r\n"),  blitParameters->rop4));
                }
                break;

            case 0xCCCC:  // SRCCOPY
                if( blitParameters->pSrc->InVideoMemory() &&
                    (blitParameters->pSrc->Format() == blitParameters->pDst->Format()) &&
                    blitParameters->pLookup == NULL &&
                    blitParameters->pConvert == NULL &&
                    blitParameters->xPositive > 0 )
                {
                    //  Check the BLT flags
                    if( (blitParameters->bltFlags & ~(BLT_WAITVSYNC|BLT_WAITNOTBUSY)) == 0 )
                    {
                        blitParameters->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DMASrcCopy;
						DEBUGMSG(GPE_ZONE_BLT, (TEXT("DMA BLT , rop4=%x\r\n"),  blitParameters->rop4));
                    }
                }
                break;
        }
    }

    //  Check for any ROPs that can be accelerated by optimized SW
    if( g_Globals.m_dwEnableNeonBlts && blitParameters->pBlt == &GPE::EmulatedBlt)
    {
        blitParameters->pBlt = (SCODE (GPE::*)(GPEBltParms*))(&OMAPDDGPE::DesignateBlt);
		DEBUGMSG(GPE_ZONE_BLT, (TEXT("****** trying NEON BLT ******, rop4=%x\r\n"),  blitParameters->rop4));
    }
	
    if (!g_Globals.m_dwHardwareRotation && m_iRotate && ((blitParameters->pSrc == m_pPrimarySurface) || 
//    if (m_iRotate && ((blitParameters->pSrc == m_pPrimarySurface) || 
		(blitParameters->pDst == m_pPrimarySurface)))
	{
        blitParameters->pBlt = (SCODE (GPE::*)(GPEBltParms *))&GPE::EmulatedBltRotate;
		DEBUGMSG(GPE_ZONE_BLT, (TEXT("EmulatedBltRotate BLT\r\n")));
	}

    //  Display perf type
    if( blitParameters->pBlt != &GPE::EmulatedBlt )
        DispPerfType(DISPPERF_ACCEL_HARDWARE);
    else
        DispPerfType(DISPPERF_ACCEL_EMUL);

    if( blitParameters->bltFlags & BLT_WAITVSYNC)
    {
        WaitForVBlank();
    }

    return S_OK;
}

//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::BltComplete(
    GPEBltParms *blitParameters
    )
{
    DEBUGMSG (GPE_ZONE_BLT_LO, (TEXT("OMAPDDGPE::BltComplete\r\n")));
    
    UNREFERENCED_PARAMETER(blitParameters);
    
    // need to wait for DMA based operations to complete, else CETK and cursor issues
    WaitForNotBusy();

    if (g_Globals.m_dwCursorEnable)
    {
        // If cursor was forced off turn it back on
        if (m_cursorForcedOff)
        {
            m_cursorForcedOff = FALSE;
            Lock();
            CursorOn();
            Unlock();
        }
    }

    //  Display perf end
    DispPerfEnd(0);
    return S_OK;
}

//------------------------------------------------------------------------------
ULONG
OMAPDDGPE::DrvEscape(
    SURFOBJ * pso,
    ULONG     iEsc,
    ULONG     cjIn,
    void    * pvIn,
    ULONG     cjOut,
    void    * pvOut
    )
{
    int    rc = ESC_NOT_SUPPORTED;    // Default not supported

    UNREFERENCED_PARAMETER(pso);

    DEBUGMSG (GPE_ZONE_ENTER, (TEXT("OMAPDDGPE::DrvEscape\r\n")));

    switch (iEsc)
    {
        case QUERYESCSUPPORT:
            if (cjIn == sizeof(DWORD))
            {
                DWORD   val = *(DWORD *)pvIn;

                //  Check for display perf IOCTLs
#pragma warning (push)
#pragma warning (disable:4127)
                if( DispPerfQueryEsc(val) )
                    rc = ESC_SUCCESS;
#pragma warning (pop)

                switch ( val )
                {
                    case DRVESC_GETSCREENROTATION:
                    case DRVESC_SETSCREENROTATION:
                        //  Return OK only if surface manager supports rotation
                        rc = ((m_pSurfaceMgr->SupportsRotation() == TRUE) && g_Globals.m_dwHardwareRotation) ? ESC_SUCCESS : ESC_FAILED;
                        break;

                    case GETPOWERMANAGEMENT:
                    case SETPOWERMANAGEMENT:
                    case IOCTL_POWER_CAPABILITIES:
#if (_WINCEOSVER<700)
                    case IOCTL_POWER_QUERY:
#endif 
                    case IOCTL_POWER_GET:
                    case IOCTL_POWER_SET:
                    case CONTRASTCOMMAND:
                    case IOCTL_CONTEXT_RESTORE:
                        rc = ESC_SUCCESS;
                        break;
                }
            }
            else
            {
                SetLastError (ERROR_INVALID_PARAMETER);
                rc = ESC_FAILED;
                break;
            }
            break;

        case DRVESC_GETSCREENROTATION:
            //  Only if the surface manager supports rotation
            if( m_pSurfaceMgr->SupportsRotation() == TRUE && g_Globals.m_dwHardwareRotation)
            {
                /* Since WinCE always considers the boot-up width and height to be 0 degree orientation,
                   we need to manipulate the m_iGraphicsRotate rotation angle when the device has booted up in 90,180 or 270
                   orientation (specified through registry setting and stored in g_Globals.m_dwRotationAngle) */

            //  Get state of graphics plane rotation
                DWORD graphicsRotate = getOrientation((getAngle(m_iGraphicsRotate)-getAngle(g_Globals.m_dwRotationAngle)+360)%360);
                *(int *)pvOut = ((DMDO_0 | DMDO_90 | DMDO_180 | DMDO_270) << 8) | ((BYTE)graphicsRotate);
                rc = DISP_CHANGE_SUCCESSFUL;
            }
            break;

        case DRVESC_BEGINSCREENROTATION:
            DEBUGMSG(GPE_ZONE_VIDEOMEMORY, (TEXT("DRVESC_BEGINSCREENROTATION: cjIn = %d\r\n"), cjIn));

            //  Start of screen rotation
            if( m_pSurfaceMgr->SupportsRotation() == TRUE )
            {
                /* Do Nothing */
            }
            break;
            
        case DRVESC_ENDSCREENROTATION:
            DEBUGMSG(GPE_ZONE_VIDEOMEMORY, (TEXT("DRVESC_ENDSCREENROTATION: cjIn = %d\r\n"), cjIn));
            /* Wait for DRVESC_ENDSCREENROTATION to enable the pipelines to reduce the amount of 
               flickr seen during rotation */
            //  End of screen rotation
            if( m_pSurfaceMgr->SupportsRotation() == TRUE && g_Globals.m_dwHardwareRotation)
            {
                Lock();
                //  Re-enable the primary pipeline to have the rotation take effect
                m_pDisplayContr->EnablePipeline( OMAP_DSS_PIPELINE_GFX );

                //  Get the best surface to display on the TV
                Unlock();
            }
            break;

        case DRVESC_SETSCREENROTATION:
            //  Only if the surface manager supports rotation
            if( m_pSurfaceMgr->SupportsRotation() == TRUE && g_Globals.m_dwHardwareRotation)
			{
            //  Set state of graphics plane rotation
            if( (cjIn == DMDO_0) || (cjIn == DMDO_90) || (cjIn == DMDO_180) || (cjIn == DMDO_270) )
            {
                    //  Rotation changes surface parameters, so lock out other DDraw operations until done
                    Lock();

                    DEBUGMSG(GPE_ZONE_VIDEOMEMORY, (TEXT("DRVESC_SETSCREENROTATION: cjIn = %d\r\n"), cjIn));

                    //  Disable all pipelines prior to rotation
                    m_pDisplayContr->DisablePipeline( OMAP_DSS_PIPELINE_GFX );
                    m_pDisplayContr->DisablePipeline( OMAP_DSS_PIPELINE_VIDEO1 );
                    m_pDisplayContr->DisablePipeline( OMAP_DSS_PIPELINE_VIDEO2 );

                    //  Remove overlay references
                    m_pOverlay1Surf = NULL;
                    m_pOverlay2Surf = NULL;

                    /* Since WinCE always considers the boot-up width and height to be 0 degree orientation,
                       we need to manipulate the cjIn rotation angle when the device has booted up in 90,180 or 270
                       orientation (specified through registry setting and stored in g_Globals.m_dwRotationAngle) */
                    //  Set graphics rotate angle
                    m_iGraphicsRotate = getOrientation((getAngle(cjIn)+getAngle(g_Globals.m_dwRotationAngle))%360);

                    //  Change width and height of primary surface, but not the surface angle of the GPE surface
                    //  Have the display controller rotate the output of the primary surface
                    switch( m_iGraphicsRotate )
                    {
                        case DMDO_0:
                            //  Set the rotation and orientation of the primary surface
                            m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_STANDARD );

                            //  Set the output rotation angle for the pipeline
                            m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_0 );
                            break;

                        case DMDO_90:
                            //  Set the rotation and orientation of the primary surface
                            m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_ROTATED );

                            //  Set the output rotation angle for the pipeline
                            m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_90 );
                            break;

                        case DMDO_180:
                            //  Set the rotation and orientation of the primary surface
                            m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_STANDARD );

                            //  Set the output rotation angle for the pipeline
                            m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_180 );
                            break;

                        case DMDO_270:
                            //  Set the rotation and orientation of the primary surface
                            m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_ROTATED );

                            //  Set the output rotation angle for the pipeline
                            m_pDisplayContr->RotatePipeline( OMAP_DSS_PIPELINE_GFX, OMAP_DSS_ROTATION_270 );
                            break;
                    }

                    //  Update extended mode information
                    m_pMode->width = m_pPrimarySurf->OmapSurface()->Width();
                    m_pMode->height = m_pPrimarySurf->OmapSurface()->Height();
                    m_pModeEx->lPitch = m_pPrimarySurf->OmapSurface()->Stride();

                    /*  Dont enable the pipelines yet -Wait for DRVESC_ENDSCREENROTATION 
                        to enable the pipelines to reduce the amount of flicker seen during rotation */

                    //  Rotation done, unlock other operations
                    Unlock();

                    rc = DISP_CHANGE_SUCCESSFUL;
                }
                else
                {
                    rc = DISP_CHANGE_BADMODE;
                }
            }
            else
	        {
		        RETAILMSG(1, (L"Hardware does not support rotation\r\n"));
				if ((cjIn == DMDO_0)   ||
					(cjIn == DMDO_90)  ||
					(cjIn == DMDO_180) ||
					(cjIn == DMDO_270) )
                {
                    //  Rotation changes surface parameters, so lock out other DDraw operations until done
					Lock();
					DynRotate(cjIn);
					Unlock();
					return rc;
                }
                else
                {
                    rc = DISP_CHANGE_BADMODE;
                }
            }
            break;

        case CONTRASTCOMMAND:
            //  Handle the contrast command
            if (cjIn == sizeof(ContrastCmdInputParm))
            {
                ContrastCmdInputParm*   pContrastParm = (ContrastCmdInputParm *)pvIn;
                DWORD                   dwContrastLevel = DEFAULT_CONTRAST_LEVEL;

                //  Default return value
                rc = ESC_SUCCESS;
                switch(pContrastParm->command)
                {
                    case CONTRAST_CMD_GET:
                        //  Get current contrast level
                        dwContrastLevel = m_pDisplayContr->GetContrastLevel();
                        break;

                    case CONTRAST_CMD_SET:
                        //  Set current contrast level
                        m_pDisplayContr->SetContrastLevel(pContrastParm->parm);
                        dwContrastLevel = m_pDisplayContr->GetContrastLevel();
                        break;

                    case CONTRAST_CMD_INCREASE:
                        //  Increase contrast by delta
                        dwContrastLevel = m_pDisplayContr->GetContrastLevel() + pContrastParm->parm;

                        // If caller wants to set the contrast above the upper limit,
                        // we need to return error in order to pass the DDI contrast
                        // Interface LTK test.
                        if (dwContrastLevel >= NUM_CONTRAST_LEVELS)
                        {
                            rc = ESC_FAILED;
                            SetLastError (ERROR_INVALID_PARAMETER);
                        }
                        else
                        {
                        m_pDisplayContr->SetContrastLevel(dwContrastLevel);
                        }
                        break;

                    case CONTRAST_CMD_DECREASE:
                        //  Decrease contrast by delta
                        dwContrastLevel = m_pDisplayContr->GetContrastLevel();

                        // If caller wants to set the contrast below the lower limit,
                        // we need to return error in order to pass the DDI contrast
                        // Interface LTK test.
                        if (dwContrastLevel < (DWORD)pContrastParm->parm)
                        {
                            rc = ESC_FAILED;
                            SetLastError (ERROR_INVALID_PARAMETER);
                        }
                        else
                        {
                            dwContrastLevel -= pContrastParm->parm;
                        m_pDisplayContr->SetContrastLevel(dwContrastLevel);
                        }
                        break;

                    case CONTRAST_CMD_DEFAULT:
                        //  Set default contrast level
                        m_pDisplayContr->SetContrastLevel(DEFAULT_CONTRAST_LEVEL);
                        dwContrastLevel = DEFAULT_CONTRAST_LEVEL;
                        break;

                    case CONTRAST_CMD_MAX:
                        //  Get max contrast level
                        dwContrastLevel = NUM_CONTRAST_LEVELS-1;
                        break;

                    default:
                        rc = ESC_FAILED;
                        SetLastError (ERROR_INVALID_PARAMETER);
                        break;
                }

                //  Return updated contrast setting
                if((rc == ESC_SUCCESS) && (cjOut == sizeof(DWORD)) && (pvOut != NULL))
                {
                    *(DWORD*)pvOut = dwContrastLevel;
                }
            }
            else
            {
                SetLastError (ERROR_INVALID_PARAMETER);
                rc = ESC_FAILED;
                break;
            }
            break;

        case GETPOWERMANAGEMENT:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: GETPOWERMANAGEMENT\r\n"));
            if( (cjOut >= sizeof(VIDEO_POWER_MANAGEMENT)) && (pvOut != NULL) )
            {
                PVIDEO_POWER_MANAGEMENT pvpm = (PVIDEO_POWER_MANAGEMENT)pvOut;
                pvpm->Length = sizeof(VIDEO_POWER_MANAGEMENT);
                pvpm->DPMSVersion = 0;

                //  Convert from Dx state to Video PM state
                switch( m_pDisplayContr->GetPowerLevel() )
                {
                    case D0:
                    case D1:
                        pvpm->PowerState = VideoPowerOn;
                        break;
                    case D2:
                        pvpm->PowerState = VideoPowerStandBy;
                        break;
                    case D3:
                        pvpm->PowerState = VideoPowerSuspend;
                        break;
                    case D4:
                        pvpm->PowerState = VideoPowerOff;
                        break;
                }

                rc = ESC_SUCCESS;
            }
            else
            {
                SetLastError (ERROR_INVALID_PARAMETER);
                rc = ESC_FAILED;
            }
            break;

        case SETPOWERMANAGEMENT:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: SETPOWERMANAGEMENT\r\n"));
            if( (cjIn >= sizeof(VIDEO_POWER_MANAGEMENT)) && (pvIn != NULL) )
            {
                PVIDEO_POWER_MANAGEMENT pvpm = (PVIDEO_POWER_MANAGEMENT)pvIn;
                if (pvpm->Length >= sizeof(VIDEO_POWER_MANAGEMENT) )
                {
                    //  Convert from Video PM state to Dx state
                    switch (pvpm->PowerState)
                    {
                        case VideoPowerOn:
                            m_pDisplayContr->SetPowerLevel(D0);
                            break;
                        case VideoPowerStandBy:
                            m_pDisplayContr->SetPowerLevel(D2);
                            break;
                        case VideoPowerSuspend:
                            m_pDisplayContr->SetPowerLevel(D3);
                            break;
                        case VideoPowerOff:
                            m_pDisplayContr->SetPowerLevel(D4);
                            break;
                    }

                    rc = ESC_SUCCESS;
                }
            }
            else
            {
                SetLastError (ERROR_INVALID_PARAMETER);
                rc = ESC_FAILED;
            }
            break;

        case IOCTL_POWER_CAPABILITIES:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_POWER_CAPABILITIES\r\n"));
            if (pvOut != NULL && cjOut == sizeof(POWER_CAPABILITIES))
            {
                __try
                {
                    PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pvOut;
                    memset(ppc, 0, sizeof(*ppc));
                    ppc->DeviceDx = DX_MASK(D0) | DX_MASK(D1) | DX_MASK(D2) | DX_MASK(D3) | DX_MASK(D4);
                    rc = ESC_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;
#if (_WINCEOSVER<700)
        case IOCTL_POWER_QUERY:
           DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_POWER_QUERY\r\n"));
           if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
            {
                __try
                {
                    CEDEVICE_POWER_STATE dx = *(CEDEVICE_POWER_STATE*)pvOut;
                    rc = VALID_DX(dx) ? ESC_SUCCESS : ESC_FAILED;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;
#endif
        case IOCTL_POWER_GET:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_POWER_GET\r\n"));
            if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
            {
                __try
                {
                    *(PCEDEVICE_POWER_STATE) pvOut = (CEDEVICE_POWER_STATE) m_pDisplayContr->GetPowerLevel();
                    rc = ESC_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        case IOCTL_POWER_SET:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_POWER_SET\r\n"));
            if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
            {
                __try
                {
                    CEDEVICE_POWER_STATE dx = *(CEDEVICE_POWER_STATE*)pvOut;
                    if( VALID_DX(dx) )
                    {
                        DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_POWER_SET = to D%u\r\n", dx));
                        m_pDisplayContr->SetPowerLevel(dx);
                        rc = ESC_SUCCESS;
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        case IOCTL_TIPMX_CONTEXTPATH:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_TIPMX_CONTEXTPATH\r\n"));
            if(pvOut != NULL && cjOut >= sizeof(IOCTL_TIPMX_CONTEXTPATH_OUT))
            {
                IOCTL_TIPMX_CONTEXTPATH_OUT *pCtx = (IOCTL_TIPMX_CONTEXTPATH_OUT*)pvOut;
                __try
                {
                    _tcscpy_s(pCtx->szContext,_countof(pCtx->szContext), g_Globals.m_szContext);
                    rc = ESC_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        case IOCTL_CONTEXT_RESTORE:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: IOCTL_CONTEXT_RESTORE\r\n"));
            if (pvIn != NULL)
            {
                OMAP_DEVICE  deviceId = (OMAP_DEVICE)*((UINT32*)pvIn);

                if (deviceId == m_pDisplayContr->GetDssInfo()->DSSDevice)
                {
                        m_pDisplayContr->RestoreRegisters(OMAP_DSS_DESTINATION_LCD);
                        rc = ESC_SUCCESS;
                }
                else 
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        default:
#pragma warning (push)
#pragma warning (disable:4127)
            //  Check for display perf IOCTLs
            if( DispPerfQueryEsc(iEsc) )
            {
                rc = DispPerfDrvEscape(iEsc, cjIn, pvIn, cjOut, pvOut);
            }
#pragma warning (pop)
            break;


#if (_WINCEOSVER<600)
        case GETGXINFO:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: GETGXINFO\r\n"));
            if(pvOut != NULL)
            {
                __try
                {
                    rc = GetGameXInfo( cjOut, pvOut );
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        case GETRAWFRAMEBUFFER:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: GETRAWFRAMEBUFFER\r\n"));
            if(pvOut != NULL)
            {
                __try
                {
                    rc = GetGameFrameBuffer( cjOut, pvOut );
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    rc = ESC_FAILED;
                }
            }
            break;

        case DRVESC_GAPI_ENABLE:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: DRVESC_GAPI_ENABLE\r\n"));
            m_bGameEnable = TRUE;
            rc = GameEnable(TRUE);
            break;

        case DRVESC_GAPI_DISABLE:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: DRVESC_GAPI_DISABLE\r\n"));
            m_bGameEnable = FALSE;
            rc = GameEnable(FALSE);
            break;

        case DRVESC_GAPI_DRAMTOVRAM:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: DRVESC_GAPI_DRAMTOVRAM\r\n"));
            rc = GameDRAMtoVRAM();
            break;

        case DRVESC_GAPI_VRAMTODRAM:
            DEBUGMSG(GPE_ZONE_HW, (L"OMAPDDGPE::DrvEscape: DRVESC_GAPI_VRAMTODRAM\r\n"));
            rc = GameVRAMtoDRAM();
            break;
#endif
    }

    return rc;
}


//------------------------------------------------------------------------------
static BOOL ConvertStringToGuid(LPCWSTR szGuid, GUID *pGuid)
{
    // ConvertStringToGuid
    // this routine converts a string into a GUID and returns TRUE if the
    // conversion was successful.

    BOOL rc = FALSE;
    int idx, data4[8];
    const LPWSTR fmt = L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";

    if (swscanf_s(
        szGuid, fmt, &pGuid->Data1, &pGuid->Data2, &pGuid->Data3,
        &data4[0], &data4[1], &data4[2], &data4[3],
        &data4[4], &data4[5], &data4[6], &data4[7]
    ) != 11) goto cleanUp;

    for (idx = 0; idx < sizeof(data4)/sizeof (int); idx++)
    {
        pGuid->Data4[idx] = (UCHAR)data4[idx];
    }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
DWORD
OMAPDDGPE::PixelFormatToBpp(
    OMAP_DSS_PIXELFORMAT    ePixelFormat
)
{
    DWORD   dwResult = 0;

    //  Convert OMAP pixel format into bits per pixel
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
        case OMAP_DSS_PIXELFORMAT_ARGB16:
        case OMAP_DSS_PIXELFORMAT_YUV2:
        case OMAP_DSS_PIXELFORMAT_UYVY:
            //  2 bytes per pixel
            dwResult = 16;
            break;
			
        case OMAP_DSS_PIXELFORMAT_RGB24:
            //  3 bytes per pixel
            dwResult = 24;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
        case OMAP_DSS_PIXELFORMAT_ARGB32:
        case OMAP_DSS_PIXELFORMAT_RGBA32:
            //  4 bytes per pixel
            dwResult = 32;
            break;
    }

    //  Return result
    return dwResult;
}

//------------------------------------------------------------------------------
EGPEFormat
OMAPDDGPE::PixelFormatToGPEFormat(
    OMAP_DSS_PIXELFORMAT    ePixelFormat
)
{
    EGPEFormat  eResult = gpeUndefined;

    //  Convert OMAP pixel format into GPE pixel format enum
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
        case OMAP_DSS_PIXELFORMAT_ARGB16:
            eResult = gpe16Bpp;
            break;

        case OMAP_DSS_PIXELFORMAT_YUV2:
        case OMAP_DSS_PIXELFORMAT_UYVY:
            eResult = gpe16YCrCb;
            break;
			
        case OMAP_DSS_PIXELFORMAT_RGB24:
            eResult = gpe24Bpp;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
        case OMAP_DSS_PIXELFORMAT_ARGB32:
        case OMAP_DSS_PIXELFORMAT_RGBA32:
            eResult = gpe32Bpp;
            break;
    }

    //  Return result
    return eResult;
}

//------------------------------------------------------------------------------
EDDGPEPixelFormat
OMAPDDGPE::PixelFormatToDDGPEFormat(
    OMAP_DSS_PIXELFORMAT    ePixelFormat
)
{
    EDDGPEPixelFormat  eResult = ddgpePixelFormat_UnknownFormat;

    //  Convert OMAP pixel format into DDGPE pixel format enum
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
            eResult = ddgpePixelFormat_565;
            break;

        case OMAP_DSS_PIXELFORMAT_ARGB16:
            eResult = ddgpePixelFormat_4444;
            break;

        case OMAP_DSS_PIXELFORMAT_YUV2:
            eResult = ddgpePixelFormat_YUY2;
            break;

        case OMAP_DSS_PIXELFORMAT_UYVY:
            eResult = ddgpePixelFormat_UYVY;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB24:
            eResult = ddgpePixelFormat_8880;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
        case OMAP_DSS_PIXELFORMAT_ARGB32:
        case OMAP_DSS_PIXELFORMAT_RGBA32:
            eResult = ddgpePixelFormat_8888;
            break;
    }

    //  Return result
    return eResult;
}

//------------------------------------------------------------------------------
BOOL
OMAPDDGPE::PixelFormatToBitMask(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   *pAlphaBitMask,
    DWORD                   *pRBitMask,
    DWORD                   *pGBitMask,
    DWORD                   *pBBitMask
)
{
    BOOL    bResult = FALSE;

    //  Check pointers
    if( pAlphaBitMask == NULL ||
        pRBitMask == NULL ||
        pGBitMask == NULL ||
        pBBitMask == NULL )
        return bResult;


    //  Convert OMAP pixel format into bitmask fields
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
            *pAlphaBitMask = 0x0000;
            *pRBitMask     = 0xf800;
            *pGBitMask     = 0x07e0;
            *pBBitMask     = 0x001f;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_ARGB16:
            *pAlphaBitMask = 0xf000;
            *pRBitMask     = 0x0f00;
            *pGBitMask     = 0x00f0;
            *pBBitMask     = 0x000f;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB24:
            *pAlphaBitMask = 0x00000000;
            *pRBitMask     = 0x00ff0000;
            *pGBitMask     = 0x0000ff00;
            *pBBitMask     = 0x000000ff;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
            *pAlphaBitMask = 0x00000000;
            *pRBitMask     = 0x00ff0000;
            *pGBitMask     = 0x0000ff00;
            *pBBitMask     = 0x000000ff;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_ARGB32:
            *pAlphaBitMask = 0xff000000;
            *pRBitMask     = 0x00ff0000;
            *pGBitMask     = 0x0000ff00;
            *pBBitMask     = 0x000000ff;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_RGBA32:
            *pAlphaBitMask = 0x000000ff;
            *pRBitMask     = 0xff000000;
            *pGBitMask     = 0x00ff0000;
            *pBBitMask     = 0x0000ff00;
            bResult        = TRUE;
            break;

        case OMAP_DSS_PIXELFORMAT_YUV2:
        case OMAP_DSS_PIXELFORMAT_UYVY:
            bResult = FALSE;
            break;
    }

    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
//
//  Method:  CursorOn
//
VOID OMAPDDGPE::CursorOn()
{
    UCHAR *pFrame;
    UCHAR *pFrameLine, *pStoreLine, *pXorLine, *pAndLine, data;
    int bytesPerPixel, bytesPerLine;
    int xf, yf, xc, yc, i;
    int   iRotate;

    DEBUGMSG(GPE_ZONE_CURSOR, (L"+OMAPDDGPE::CursonOn\r\n"));

    // If cursor should not be visible or already is then exit
    if (m_cursorForcedOff || m_cursorDisabled || m_cursorVisible)
        goto cleanUp;

    if (m_cursorStore == NULL)
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"OMAPDDGPE::CursorOn: "
            L"No cursor store available\r\n"
        ));
        goto cleanUp;
    }
	
    if( !g_Globals.m_dwHardwareRotation)
        RotateRectl(&m_cursorRect);

    // We support only 1,2,3 and 4 bytes per pixel
    bytesPerPixel = (m_pMode->Bpp + 7) >> 3;
    if (bytesPerPixel <= 0 || bytesPerPixel > 4) goto cleanUp;

    // Get some base metrics
    pFrame = (UCHAR*)m_pPrimarySurface->Buffer();
    bytesPerLine = m_pPrimarySurface->Stride();

    for (yf = m_cursorRect.top, yc = 0; yf < m_cursorRect.bottom; yf++, yc++)
    {
        // Check if we are done
        if (yf < 0) continue;
        if (yf >= m_pMode->height) break;
        pFrameLine = &pFrame[yf * bytesPerLine];
        pStoreLine = &m_cursorStore[yc * m_cursorSize.x * bytesPerPixel];
        pAndLine = &m_cursorAnd[yc * m_cursorSize.x * bytesPerPixel];
        pXorLine = &m_cursorXor[yc * m_cursorSize.x * bytesPerPixel];
		
        if(!g_Globals.m_dwHardwareRotation && (m_iRotate == DMDO_180)){
            pAndLine = &m_cursorAnd[(m_cursorSize.y  -1 - yc) * m_cursorSize.x * bytesPerPixel];
            pXorLine = &m_cursorXor[(m_cursorSize.y -1 - yc) * m_cursorSize.x * bytesPerPixel];
        }
	 
        for (xf = m_cursorRect.left, xc = 0; xf < m_cursorRect.right; xf++, xc++)
        {
            // Check if we are done
            if (xf < 0) continue;
            if (xf >= m_pMode->width) break;

           iRotate = xc;
            if( !g_Globals.m_dwHardwareRotation  && (m_iRotate == DMDO_180))
            {
                iRotate =  m_cursorSize.x -1 - xc ;
            }
			
            // Depending on bytes per pixel
            switch (bytesPerPixel)
            {
                case 1:
                    pStoreLine[xc] = pFrameLine[xf];
                    pFrameLine[xf] &= pAndLine[iRotate];
                    pFrameLine[xf] ^= pXorLine[iRotate];
                    break;

                case 2:
                    ((USHORT*)pStoreLine)[xc]  = ((USHORT*)pFrameLine)[xf];
                    ((USHORT*)pFrameLine)[xf] &= ((USHORT*)pAndLine)[iRotate];
                    ((USHORT*)pFrameLine)[xf] ^= ((USHORT*)pXorLine)[iRotate];
                    break;

                case 3:
                    for (i = 0; i < bytesPerPixel; i++) {
                        data = pFrameLine[xf * bytesPerPixel + i];
                        pStoreLine[xc * bytesPerPixel + i] = data;
                        data &= pAndLine[iRotate * bytesPerPixel + i];
                        data ^= pXorLine[iRotate * bytesPerPixel + i];
                        pFrameLine[xf * bytesPerPixel + i] = data;
                    }
                    break;

                case 4:
                    ((ULONG*)pStoreLine)[xc]  = ((ULONG*)pFrameLine)[xf];
                    ((ULONG*)pFrameLine)[xf] &= ((ULONG*)pAndLine)[iRotate];
                    ((ULONG*)pFrameLine)[xf] ^= ((ULONG*)pXorLine)[iRotate];
                    break;
            }
        }
    }

    // Cursor is visible now
    m_cursorVisible = TRUE;

cleanUp:
    DEBUGMSG(GPE_ZONE_CURSOR, (L"-OMAPDDGPE::CursonOn\r\n"));
    return;
}

//------------------------------------------------------------------------------

VOID OMAPDDGPE::CursorOff()
{
    UCHAR *pFrame, *pFrameLine, *pStoreLine, data;
    int bytesPerPixel, bytesPerLine;
    int xf, yf, xc, yc, i;

    DEBUGMSG(GPE_ZONE_CURSOR, (L"+OMAPDDGPE::CursonOff\r\n"));

    if (m_cursorForcedOff || m_cursorDisabled || !m_cursorVisible)
        goto cleanUp;

    if (m_cursorStore == NULL)
    {
        DEBUGMSG(GPE_ZONE_ERROR, (L"OMAPDDGPE::CursorOff: "
            L"No cursor store available\r\n"
        ));
        goto cleanUp;
    }

    // We support only 1,2,3 and 4 bytes per pixel
    bytesPerPixel = (m_pMode->Bpp + 7) >> 3;
    if (bytesPerPixel <= 0 || bytesPerPixel > 4) goto cleanUp;

    // Get some base metrics
    pFrame = (UCHAR*)m_pPrimarySurface->Buffer();
    bytesPerLine = m_pPrimarySurface->Stride();

    for (yf = m_cursorRect.top, yc = 0; yf < m_cursorRect.bottom; yf++, yc++)
    {
        // Check if we are done
        if (yf < 0) continue;
        if (yf >= m_pMode->height) break;

        pFrameLine = &pFrame[yf * bytesPerLine];
        pStoreLine = &m_cursorStore[yc * m_cursorSize.x * bytesPerPixel];

        for (xf = m_cursorRect.left, xc = 0; xf < m_cursorRect.right; xf++, xc++)
        {
            // Check if we are done
            if (xf < 0) continue;
            if (xf >= m_pMode->width) break;

            // Depending on bytes per pixel
            switch (bytesPerPixel)
            {
                case 1:
                    pFrameLine[xf] = pStoreLine[xc];
                    break;

                case 2:
                    ((USHORT*)pFrameLine)[xf] = ((USHORT*)pStoreLine)[xc];
                    break;

                case 3:
                    for (i = 0; i < bytesPerPixel; i++) {
                        data = pStoreLine[xc * bytesPerPixel + i];
                        pFrameLine[xf * bytesPerPixel + i] = data;
                    }
                    break;

                case 4:
                    ((ULONG*)pFrameLine)[xf] = ((ULONG*)pStoreLine)[xc];
                    break;
            }
        }
    }

    // Cursor isn't visible now
    m_cursorVisible = FALSE;

cleanUp:
    DEBUGMSG(GPE_ZONE_CURSOR, (L"-OMAPDDGPE::CursonOff\r\n"));
    return;
}

LONG OMAPDDGPE::DynRotate( int angle )
{
//    int iswap;

	// Return if no change is needed
    if (angle == m_iRotate)
    {
        return DISP_CHANGE_SUCCESSFUL;
    }

    // Update the rotation
    m_iRotate = angle;

    CursorOff();

    //switch(m_iRotate)
    //{
    //    case DMDO_90:
    //    case DMDO_270:
    //        m_pMode->height = m_nScreenWidthSave;
    //        m_pMode->width = m_nScreenHeightSave;
    //        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_ROTATED );
    //        break;
    //
    //    case DMDO_0:
    //    case DMDO_180:
    //    default:
    //        m_pMode->width = m_nScreenWidthSave;
    //        m_pMode->height = m_nScreenHeightSave;
    //        m_pPrimarySurf->SetOrientation( OMAP_SURF_ORIENTATION_STANDARD );
    //        break;
    //}

    //m_nScreenWidth = m_pMode->width;
    //m_nScreenHeight = m_pMode->height;
    //m_pModeEx->lPitch = m_pPrimarySurf->OmapSurface()->Stride();

	
	
	m_pPrimarySurface->SetRotation(m_nScreenWidth, m_nScreenHeight, angle);

    CursorOn();

    return DISP_CHANGE_SUCCESSFUL;
}


