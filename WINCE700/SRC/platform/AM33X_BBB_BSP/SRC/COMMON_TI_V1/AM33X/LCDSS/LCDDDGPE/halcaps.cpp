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

#include "precomp.h"

extern OMAPDDGPEGlobals    g_Globals;

//
//  OMAP supported YUV overlay formats
//
DWORD g_FourCC[] = {
    FOURCC_YUY2,
    FOURCC_YUYV,   // Same pixel format as YUY2
    FOURCC_UYVY
};

#define MAX_FOURCC      sizeof(g_FourCC)/sizeof(DWORD)


//
// callbacks from the DIRECTDRAW object
//
DDHAL_DDCALLBACKS cbDDCallbacks =
{
    sizeof( DDHAL_DDCALLBACKS ),
        DDHAL_CB32_CREATESURFACE        |
        DDHAL_CB32_WAITFORVERTICALBLANK |
        DDHAL_CB32_CANCREATESURFACE     |
//      DDHAL_CB32_CREATEPALETTE        |
        DDHAL_CB32_GETSCANLINE          |
        0,
    HalCreateSurface,
    HalWaitForVerticalBlank,
    HalCanCreateSurface,
    NULL,
    HalGetScanLine
};

//
// callbacks from the DIRECTDRAWSURFACE object
//
DDHAL_DDSURFACECALLBACKS cbDDSurfaceCallbacks =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),
        DDHAL_SURFCB32_DESTROYSURFACE     |
        DDHAL_SURFCB32_FLIP               |
        DDHAL_SURFCB32_LOCK               |
        DDHAL_SURFCB32_UNLOCK             |
        DDHAL_SURFCB32_SETCOLORKEY        |
        DDHAL_SURFCB32_GETBLTSTATUS       |
        DDHAL_SURFCB32_GETFLIPSTATUS      |
        DDHAL_SURFCB32_UPDATEOVERLAY      |
        DDHAL_SURFCB32_SETOVERLAYPOSITION |
//      DDHAL_SURFCB32_SETPALETTE         |
        0,
    HalDestroySurface,
    HalFlip,
    HalLock,
    HalUnlock,
    HalSetColorKey,
    HalGetBltStatus,
    HalGetFlipStatus,
    HalUpdateOverlay,
    HalSetOverlayPosition,
    NULL
};



//-----------------------------------------------------------
DWORD WINAPI
HalGetDriverInfo(
    LPDDHAL_GETDRIVERINFODATA lpInput
    )
{
    OMAPDDGPE * pDDGPE = static_cast<OMAPDDGPE *>(GetDDGPE());

    //  Default return
    lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;


#if (_WINCEOSVER==700)
    //  Check for video memory base request(s)
    if (IsEqualIID(lpInput->guidInfo, GUID_GetDriverInfo_VidMemList) )
    {
        lpInput->dwActualSize = sizeof ( DDHAL_DDVIDMEMLIST );
        if (lpInput->dwExpectedSize < lpInput->dwActualSize)
        {
            lpInput->ddRVal = DDERR_MOREDATA;
        }
        else
        {
            LPDDHAL_DDVIDMEMLIST pVidMemList = reinterpret_cast< LPDDHAL_DDVIDMEMLIST >( lpInput->lpvData );

            //  Get video memory attributes
            pDDGPE->GetVirtualVideoMemoryList( pVidMemList );
            lpInput->ddRVal = DD_OK;
        }
    }
#endif

#if (_WINCEOSVER>=600)
    if (IsEqualIID(lpInput->guidInfo, GUID_GetDriverInfo_VidMemBase) )
    {
        DWORD   dwVideoMemoryStart,
                dwVideoMemoryLength,
                dwVideoMemoryFree;

        //  Get video memory attributes
        pDDGPE->GetVirtualVideoMemory( &dwVideoMemoryStart, &dwVideoMemoryLength, &dwVideoMemoryFree );

        //  Return base address of video memory
        *(DWORD*)(lpInput->lpvData) = dwVideoMemoryStart;
        lpInput->dwActualSize = sizeof(DWORD);
        lpInput->ddRVal = DD_OK;
    }
#endif

    return DDHAL_DRIVER_HANDLED;
}



//-----------------------------------------------------------
EXTERN_C void
buildDDHALInfo(
    LPDDHALINFO lpddhi,
    DWORD modeidx
    )
{
    OMAPDDGPE * pDDGPE = static_cast<OMAPDDGPE *>(GetDDGPE());
    DWORD       dwVideoMemoryStart,
                dwVideoMemoryLength,
                dwVideoMemoryFree;


    //  Get video memory attributes
    pDDGPE->GetVirtualVideoMemory( &dwVideoMemoryStart, &dwVideoMemoryLength, &dwVideoMemoryFree );


    // Clear the DDHALINFO structure
    memset( lpddhi, 0, sizeof(DDHALINFO) );

    lpddhi->dwSize = sizeof(DDHALINFO);

    //  Set callback functions
    lpddhi->lpDDCallbacks = &cbDDCallbacks;

    lpddhi->lpDDSurfaceCallbacks = &cbDDSurfaceCallbacks;
    lpddhi->GetDriverInfo = HalGetDriverInfo;


    // Capability bits.
    lpddhi->ddCaps.dwSize = sizeof(DDCAPS);

    lpddhi->ddCaps.dwVidMemTotal  = dwVideoMemoryLength;
    lpddhi->ddCaps.dwVidMemFree   = dwVideoMemoryFree;
    lpddhi->ddCaps.dwVidMemStride = 0;

    #ifndef DDSCAPS_OWNDC
        #error DDSCAPS_OWNDC not defined, please install all QFEs (DDSCAPS_OWNDC was added in January 2008 QFE).
    #endif

    lpddhi->ddCaps.ddsCaps.dwCaps =
        DDSCAPS_PRIMARYSURFACE |     // Has a primary surface
        DDSCAPS_FRONTBUFFER |        // Can create front buffer surfaces
        DDSCAPS_BACKBUFFER |         // Can create backbuffer surface
        DDSCAPS_FLIP |               // Can flip between surfaces
//        DDSCAPS_OVERLAY |            // Can create overlay surfaces
//        DDSCAPS_PALETTE |            // Can create paletted surfaces
        DDSCAPS_SYSTEMMEMORY |       // Surfaces are in system memory
        DDSCAPS_VIDEOMEMORY |        // Surfaces are in video memory
        DDSCAPS_OWNDC |
        0;

    if(g_Globals.m_dwEnableOverlay)
    {
        lpddhi->ddCaps.ddsCaps.dwCaps |=  DDSCAPS_OVERLAY ;
    }
	
    lpddhi->ddCaps.dwNumFourCCCodes = MAX_FOURCC;
    lpddhi->lpdwFourCC = g_FourCC;


    // Palette caps
    lpddhi->ddCaps.dwPalCaps = 0;

    // Blt caps
    lpddhi->ddCaps.dwBltCaps =
        DDBLTCAPS_READSYSMEM  |
        DDBLTCAPS_WRITESYSMEM |
//        DDBLTCAPS_COPYFOURCC  |
//        DDBLTCAPS_FILLFOURCC  |
        0;

    SETROPBIT(lpddhi->ddCaps.dwRops,SRCCOPY);
    SETROPBIT(lpddhi->ddCaps.dwRops,PATCOPY);
    SETROPBIT(lpddhi->ddCaps.dwRops,BLACKNESS);
    SETROPBIT(lpddhi->ddCaps.dwRops,WHITENESS);


    // Color key caps
    lpddhi->ddCaps.dwCKeyCaps =
//        DDCKEYCAPS_BOTHBLT |
//        DDCKEYCAPS_DESTBLT |
//        DDCKEYCAPS_DESTBLTCLRSPACE |
//        DDCKEYCAPS_DESTBLTCLRSPACEYUV |
        DDCKEYCAPS_SRCBLT |
//        DDCKEYCAPS_SRCBLTCLRSPACE |
//        DDCKEYCAPS_SRCBLTCLRSPACEYUV |
        0;

    // Alpha blending caps
    lpddhi->ddCaps.dwAlphaCaps =
        DDALPHACAPS_ALPHAPIXELS |
//        DDALPHACAPS_ALPHASURFACE |
//        DDALPHACAPS_ALPHAPALETTE |
        DDALPHACAPS_ALPHACONSTANT |
//        DDALPHACAPS_ARGBSCALE |
//        DDALPHACAPS_SATURATE |
//        DDALPHACAPS_PREMULT |
        DDALPHACAPS_NONPREMULT |
        DDALPHACAPS_ALPHAFILL |
//        DDALPHACAPS_ALPHANEG |
        0;
	
    // Overlay caps.
    if(g_Globals.m_dwEnableOverlay){
    
        lpddhi->ddCaps.dwOverlayCaps=               // overlay capabilities
            DDOVERLAYCAPS_OVERLAYSUPPORT |          // Supports overlays
            DDOVERLAYCAPS_FLIP |                    // Overlay may be flipped.
            DDOVERLAYCAPS_FOURCC |                  // YUV overlays supported.
            DDOVERLAYCAPS_CKEYSRC |                 // Supports source color keying for overlays
            DDOVERLAYCAPS_CKEYSRCCLRSPACE |
            DDOVERLAYCAPS_CKEYDEST |                // Supports destination color keying for overlays
            DDOVERLAYCAPS_CKEYDESTCLRSPACE |
            DDOVERLAYCAPS_MIRRORLEFTRIGHT |         // Supports mirror horizontal
    //        DDOVERLAYCAPS_MIRRORUPDOWN |          // Supports mirror vertical
            DDOVERLAYCAPS_ALPHADEST |
            DDOVERLAYCAPS_ALPHASRC |
            DDOVERLAYCAPS_ALPHACONSTANT |
            DDOVERLAYCAPS_ALPHAANDKEYDEST |
            DDOVERLAYCAPS_ZORDER |
            DDOVERLAYCAPS_ALPHAPREMULT |
            DDOVERLAYCAPS_ALPHANONPREMULT |
            0;
    
        lpddhi->ddCaps.dwMaxVisibleOverlays = 2;
        lpddhi->ddCaps.dwCurrVisibleOverlays = pDDGPE->NumVisibleOverlays();
    
        lpddhi->ddCaps.dwAlignBoundarySrc = 0;
        lpddhi->ddCaps.dwAlignSizeSrc = 0;
        lpddhi->ddCaps.dwAlignBoundaryDest = 0;
        lpddhi->ddCaps.dwAlignSizeDest = 0;
        
        if (LcdPdd_Get_PixClkDiv() >= 8)
            lpddhi->ddCaps.dwMinOverlayStretch = 250;   // max shrink 25%
        else if (LcdPdd_Get_PixClkDiv() >= 4)
            lpddhi->ddCaps.dwMinOverlayStretch = 500;   // max shrink 50%
        else
            lpddhi->ddCaps.dwMinOverlayStretch = 1000;   // Shrink not supported

		lpddhi->ddCaps.dwMaxOverlayStretch = 8000;  // Expand to 8x original size
    }
	else
        lpddhi->ddCaps.dwOverlayCaps= 0;                
    // Misc caps
    lpddhi->ddCaps.dwMiscCaps=
        DDMISCCAPS_FLIPINTERVAL |
        DDMISCCAPS_FLIPODDEVEN |
        DDMISCCAPS_FLIPVSYNCWITHVBI | 
//        DDMISCCAPS_READSCANLINE |
        DDMISCCAPS_READVBLANKSTATUS |
        0;

    //  Call virtual method for any customization
    pDDGPE->DDHALInfo( lpddhi, modeidx );
}

