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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// -----------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Module Name:    halcaps.cpp
//
// -----------------------------------------------------------------------------


/* halcaps.cpp
 *
 * Functions for reporting HAL capabilities to DirectDraw
 *
 * Copyright (C) MPC Data Limited 2007. Some rights reserved.
 *
 */


#include "precomp.h"



// callbacks from the DIRECTDRAW object
DDHAL_DDCALLBACKS cbDDCallbacks =
{
    sizeof( DDHAL_DDCALLBACKS ),
        DDHAL_CB32_CREATESURFACE        |
        DDHAL_CB32_WAITFORVERTICALBLANK |
        DDHAL_CB32_CANCREATESURFACE     |
//      DDHAL_CB32_CREATEPALETTE        |
//      DDHAL_CB32_GETSCANLINE          |
        0,
    HalCreateSurface,
    DDGPEWaitForVerticalBlank,
    HalCanCreateSurface,
    NULL,
    NULL
};


// callbacks from the DIRECTDRAWSURFACE object
DDHAL_DDSURFACECALLBACKS cbDDSurfaceCallbacks =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),
        DDHAL_SURFCB32_DESTROYSURFACE     |
        DDHAL_SURFCB32_FLIP               |
        DDHAL_SURFCB32_LOCK               |
        DDHAL_SURFCB32_UNLOCK             |
//        DDHAL_SURFCB32_SETCOLORKEY        |
        DDHAL_SURFCB32_GETBLTSTATUS       |
        DDHAL_SURFCB32_GETFLIPSTATUS      |
//      DDHAL_SURFCB32_SETPALETTE         |
        0,
    DDGPEDestroySurface,
    HalFlip,
    DDGPELock,
    DDGPEUnlock,
    NULL,
    HalGetBltStatus,
    HalGetFlipStatus,
    NULL,
    NULL,
    NULL 
};

// InitDDHALInfo must set up this information
unsigned long           g_nVideoMemorySize      = 0L;
unsigned char *         g_pVideoMemory          = NULL; // virtual address of video memory from client's side


//------------------------------------------------------------------------------
//
//  Function: HalGetDriverInfo
//
//  Stub function.  It should return extended DirectDraw info
//
DWORD WINAPI 
HalGetDriverInfo(
    LPDDHAL_GETDRIVERINFODATA lpInput
    )
{
    DEBUGENTER(HalGetDriverInfo);

    // The flat driver does not implement any of the extended DirectDraw
    // caps or callbacks.
    lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;

    DEBUGLEAVE(HalGetDriverInfo);
    return DDHAL_DRIVER_HANDLED;
}


//------------------------------------------------------------------------------
//
//  Function: HalGetDriverInfo
//
//  Stub function.  Should report status of the current blit operation
//
DWORD WINAPI 
HalGetBltStatus(
    LPDDHAL_GETBLTSTATUSDATA pd 
    )
{
    DEBUGENTER(HalGetBltStatus);
    
    // The flat driver always completes blts immidiately.
    
    pd->ddRVal = DD_OK;

    DEBUGLEAVE(HalGetBltStatus);
    return DDHAL_DRIVER_HANDLED;
}


//------------------------------------------------------------------------------
//
//  Function: buildDDHALInfo
//
//  Returns the capabilities of this display device
//
EXTERN_C void 
buildDDHALInfo( 
    LPDDHALINFO lpddhi,
    DWORD modeidx 
    )
{
    LCDDDGPE* pGpe = static_cast<LCDDDGPE *>(GetDDGPE());

    if( !g_pVideoMemory )   // in case this is called more than once...
    {
        unsigned long VideoMemoryStart;

        pGpe->GetVirtualVideoMemory( &VideoMemoryStart, &g_nVideoMemorySize );
        DEBUGMSG( GPE_ZONE_INIT,(TEXT("GetVirtualVideoMemory returned addr=0x%08x size=%d\r\n"), VideoMemoryStart, g_nVideoMemorySize));

        g_pVideoMemory = (BYTE*)VideoMemoryStart;
        DEBUGMSG( GPE_ZONE_INIT,(TEXT("gpVidMem=%08x\r\n"), g_pVideoMemory ));
    }

    memset( lpddhi, 0, sizeof(DDHALINFO) );         // Clear the DDHALINFO structure

    lpddhi->dwSize = sizeof(DDHALINFO);

    lpddhi->lpDDCallbacks = &cbDDCallbacks;
    lpddhi->lpDDSurfaceCallbacks = &cbDDSurfaceCallbacks;
    lpddhi->GetDriverInfo = HalGetDriverInfo;

    // Additional video modes
    lpddhi->lpdwFourCC = 0;

    // Capability bits.
    lpddhi->ddCaps.dwSize = sizeof(DDCAPS);

    lpddhi->ddCaps.dwVidMemTotal = g_nVideoMemorySize;
    lpddhi->ddCaps.dwVidMemFree = g_nVideoMemorySize;
    lpddhi->ddCaps.dwVidMemStride = VRAM_GRAIN;

    lpddhi->ddCaps.ddsCaps.dwCaps=               // DDSCAPS structure has all the general capabilities
            DDSCAPS_BACKBUFFER |                 // Can create backbuffer surfaces
            DDSCAPS_FLIP |                       // Can flip between surfaces
            DDSCAPS_FRONTBUFFER |                // Can create front-buffer surfaces
            DDSCAPS_PRIMARYSURFACE |             // Has a primary surface
            DDSCAPS_SYSTEMMEMORY |               // Surfaces are in system memory
            DDSCAPS_VIDEOMEMORY |                // Surfaces are in video memory
            DDSCAPS_OWNDC |                      // Surface will have a device context (DC) association for a long period. 
            0;

    lpddhi->ddCaps.dwPalCaps = 0;
    lpddhi->ddCaps.dwMiscCaps =
        DDMISCCAPS_FLIPVSYNCWITHVBI |            // Supports V-Sync-coordinated flipping
        0;

    // Number of additional video modes
    lpddhi->ddCaps.dwNumFourCCCodes = 0;

    // DirectDraw Blttting caps refer to hardware blitting support only.
    lpddhi->ddCaps.dwBltCaps = 0;
    lpddhi->ddCaps.dwCKeyCaps = 0;
    lpddhi->ddCaps.dwAlphaCaps = 0;

    // No overlays supported
    lpddhi->ddCaps.dwOverlayCaps = 0;
    lpddhi->ddCaps.dwMaxVisibleOverlays = 0;
    lpddhi->ddCaps.dwCurrVisibleOverlays = 0;

    lpddhi->ddCaps.dwAlignBoundarySrc = 0;
    lpddhi->ddCaps.dwAlignSizeSrc = 0;
    lpddhi->ddCaps.dwAlignBoundaryDest = 0;
    lpddhi->ddCaps.dwAlignSizeDest = 0;

    // no overlay scaling supported
    lpddhi->ddCaps.dwMinOverlayStretch = 1;
    lpddhi->ddCaps.dwMaxOverlayStretch = 1;
}


//------------------------------------------------------------------------------
//
//  Function: HalGetFlipStatus
//
//  Stub function.  Should return whether a vsync flip has just completed
//
DWORD WINAPI HalGetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA pd )
{
    // Implementation
    pd->ddRVal = DDERR_UNSUPPORTED;

    return DDHAL_DRIVER_HANDLED;
}



//------------------------------------------------------------------------------
//
//  Function: HalFlip
//
//  Public API function to display the next backbuffer.
//
DWORD WINAPI HalFlip( LPDDHAL_FLIPDATA pd )
{
    return ((LCDDDGPE*)GetDDGPE())->Flip(pd);
}



//------------------------------------------------------------------------------
//
//  Member: Flip
//
// Queue the next buffer in the swap-chain, then wait for it to be displayed
//
DWORD WINAPI LCDDDGPE::Flip( LPDDHAL_FLIPDATA pd )
{
    DDGPESurf *surfTarg;

    surfTarg = DDGPESurf::GetDDGPESurf(pd->lpSurfTarg);

    // This queues the flip request until the IRQ is ready for it.
    LcdSetFrameBufferOnVsync( m_Lcdc.fb_pa + surfTarg->OffsetInVideoMemory(), m_Lcdc.fb_cur_win_size );

    // Always wait for the hardware to start displaying the next frame otherwise
    // the wrong buffer could be displayed.  This will look *VERY* jerky.
    WaitForVBlank();
    WaitForVBlank();

    pd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}




