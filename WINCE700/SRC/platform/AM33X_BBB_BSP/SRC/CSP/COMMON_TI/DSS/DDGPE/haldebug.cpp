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

#ifdef DEBUG

//-----------------------------------------------------------
void
DumpDD_CANCREATESURFACE(
    LPDDHAL_CANCREATESURFACEDATA pd
    )
{
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_CANCREATESURFACEDATA:\r\n"));
    DumpDDSURFACEDESC(pd->lpDDSurfaceDesc);
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->bIsDifferentPixelFormat = %d\r\n", pd->bIsDifferentPixelFormat));
}

//-----------------------------------------------------------
void
DumpDD_CREATESURFACE(
    LPDDHAL_CREATESURFACEDATA pd
    )
{
    DWORD   i;

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_CREATESURFACEDATA:\r\n"));
    DumpDDSURFACEDESC(pd->lpDDSurfaceDesc);
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->dwSCnt = %d\r\n", pd->dwSCnt));
    
    for( i = 0; i < pd->dwSCnt; i++ )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lplpSList[%d] = 0x%x\r\n", i, pd->lplpSList[i]));
    }
}

//-----------------------------------------------------------
void
DumpDD_FLIP(
    LPDDHAL_FLIPDATA pd
    )
{
    DDGPESurf*  surfCurr = DDGPESurf::GetDDGPESurf(pd->lpSurfCurr);
    DDGPESurf*  surfTarg = DDGPESurf::GetDDGPESurf(pd->lpSurfTarg);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_FLIPDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfCurr           = 0x%x\r\n", pd->lpSurfCurr));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfCurr ddsurf    = 0x%x\r\n", surfCurr));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfCurr vidmem    = 0x%x\r\n", surfCurr->Buffer()));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfTarg           = 0x%x\r\n", pd->lpSurfTarg));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfTarg ddsurf    = 0x%x\r\n", surfTarg));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfTarg vidmem    = 0x%x\r\n", surfTarg->Buffer()));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->dwFlags              = 0x%x\r\n", pd->dwFlags ));

    if( pd->dwFlags & DDFLIP_EVEN )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_EVEN; \r\n")));
    if( pd->dwFlags & DDFLIP_ODD )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_ODD; \r\n")));
    if( pd->dwFlags & DDFLIP_INTERVAL1 )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_INTERVAL1; \r\n")));
    if( pd->dwFlags & DDFLIP_INTERVAL2 )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_INTERVAL2; \r\n")));
    if( pd->dwFlags & DDFLIP_INTERVAL4 )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_INTERVAL4; \r\n")));
    if( pd->dwFlags & DDFLIP_WAITNOTBUSY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_WAITNOTBUSY; \r\n")));
    if( pd->dwFlags & DDFLIP_WAITVSYNC )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDFLIP_WAITVSYNC; \r\n")));
    
}

//-----------------------------------------------------------
void
DumpDD_LOCK(
    LPDDHAL_LOCKDATA pd
    )
{
    DDGPESurf*  surfCurr = DDGPESurf::GetDDGPESurf(pd->lpDDSurface);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_LOCKDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface          = 0x%x\r\n", pd->lpDDSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface vidmem   = 0x%x\r\n", surfCurr->Buffer()));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->bHasRect             = %d\r\n", pd->bHasRect));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->rArea                = (%d,%d) (%d,%d)\r\n", pd->rArea.left, pd->rArea.top, pd->rArea.right, pd->rArea.bottom ));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpSurfData           = 0x%x\r\n", pd->lpSurfData));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->dwFlags              = 0x%x\r\n", pd->dwFlags ));

    if( pd->dwFlags & DDLOCK_DISCARD )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDLOCK_DISCARD; \r\n")));
    if( pd->dwFlags & DDLOCK_READONLY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDLOCK_READONLY; \r\n")));
    if( pd->dwFlags & DDLOCK_WRITEONLY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDLOCK_WRITEONLY; \r\n")));
    if( pd->dwFlags & DDLOCK_WAITNOTBUSY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDLOCK_WAITNOTBUSY; \r\n")));
}

//-----------------------------------------------------------
void
DumpDD_UNLOCK(
    LPDDHAL_UNLOCKDATA pd
    )
{
    DDGPESurf*  surfCurr = DDGPESurf::GetDDGPESurf(pd->lpDDSurface);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_UNLOCKDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface          = 0x%x\r\n", pd->lpDDSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface vidmem   = 0x%x\r\n", surfCurr->Buffer()));
}

//-----------------------------------------------------------
void
DumpDD_SETCOLORKEY(
    LPDDHAL_SETCOLORKEYDATA pd
    )
{
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_SETCOLORKEYDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface = 0x%x\r\n", pd->lpDDSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->dwFlags     = 0x%x\r\n", pd->dwFlags));

    if( pd->dwFlags & DDCKEY_COLORSPACE)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCKEY_COLORSPACE; ")));
    if( pd->dwFlags & DDCKEY_DESTBLT)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCKEY_DESTBLT; ")));
    if( pd->dwFlags & DDCKEY_DESTOVERLAY)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCKEY_DESTOVERLAY; ")));
    if( pd->dwFlags & DDCKEY_SRCBLT)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCKEY_SRCBLT; ")));
    if( pd->dwFlags & DDCKEY_SRCOVERLAY)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCKEY_SRCOVERLAY; ")));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->ckNew\r\n"));
    DumpDDCOLORKEY(pd->ckNew);
}

//-----------------------------------------------------------
void
DumpDD_UPDATEOVERLAY(
    LPDDHAL_UPDATEOVERLAYDATA pd
    )
{
    DWORD   i = 0;
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_UPDATEOVERLAYDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDDestSurface = 0x%x\r\n", pd->lpDDDestSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->rDest           = (%d,%d) (%d,%d)\r\n", pd->rDest.left, pd->rDest.top, pd->rDest.right, pd->rDest.bottom ));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSrcSurface  = 0x%x\r\n", pd->lpDDSrcSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->rSrc            = (%d,%d) (%d,%d)\r\n", pd->rSrc.left, pd->rSrc.top, pd->rSrc.right, pd->rSrc.bottom ));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->dwFlags         = 0x%x\r\n", pd->dwFlags));
    if( pd->dwFlags & DDOVER_ALPHACONSTOVERRIDE)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_ALPHACONSTOVERRIDE; ")));
    if( pd->dwFlags & DDOVER_ALPHADEST)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_ALPHADEST; ")));
    if( pd->dwFlags & DDOVER_ALPHADESTNEG)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_ALPHADESTNEG; ")));
    if( pd->dwFlags & DDOVER_ALPHASRC)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_ALPHASRC; ")));
    if( pd->dwFlags & DDOVER_ALPHASRCNEG)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_ALPHASRCNEG; ")));
    if( pd->dwFlags & DDOVER_SHOW)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_SHOW; ")));
    if( pd->dwFlags & DDOVER_HIDE)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_HIDE; ")));
    if( pd->dwFlags & DDOVER_AUTOFLIP)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_AUTOFLIP; ")));
    if( pd->dwFlags & DDOVER_WAITNOTBUSY)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_WAITNOTBUSY; ")));
    if( pd->dwFlags & DDOVER_WAITVSYNC)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_WAITVSYNC; ")));
    if( pd->dwFlags & DDOVER_MIRRORLEFTRIGHT)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_MIRRORLEFTRIGHT; ")));
    if( pd->dwFlags & DDOVER_MIRRORUPDOWN)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_MIRRORUPDOWN; ")));
    if( pd->dwFlags & DDOVER_KEYDEST)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_KEYDEST; ")));
    if( pd->dwFlags & DDOVER_KEYDESTOVERRIDE)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_KEYDESTOVERRIDE; ")));
    if( pd->dwFlags & DDOVER_KEYSRC)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_KEYSRC; ")));
    if( pd->dwFlags & DDOVER_KEYSRCOVERRIDE)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVER_KEYSRCOVERRIDE; ")));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->overlayFX\r\n"));    
    DumpDDOVERLAYFX(pd->overlayFX);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"Overlay Z-order:\r\n"));    
    while( pd->lpDD->pOverlays[i] )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"  %d: 0x%x\r\n", i, pd->lpDD->pOverlays[i]));    
        i++;
    }
}

//-----------------------------------------------------------
void
DumpDD_SETOVERLAYPOSITION(
    LPDDHAL_SETOVERLAYPOSITIONDATA pd
    )
{
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"LPDDHAL_SETOVERLAYPOSITIONDATA:\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDDestSurface = 0x%x\r\n", pd->lpDDDestSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSrcSurface  = 0x%x\r\n", pd->lpDDSrcSurface));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lXPos           = %d\r\n", pd->lXPos));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lYPos           = %d\r\n", pd->lYPos));
}

//-----------------------------------------------------------
void
DumpDDSURFACEDESC(
    LPDDSURFACEDESC lpDDSurfaceDesc
    )
{
    if( lpDDSurfaceDesc->dwFlags  & DDSD_PIXELFORMAT )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDPIXELFORMAT:\r\n")));
        DumpDDPIXELFORMAT(lpDDSurfaceDesc->ddpfPixelFormat);
    }
    
    if( lpDDSurfaceDesc->dwFlags  & DDSD_CAPS )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSCAPS:\r\n")));
        DumpDDSCAPS(lpDDSurfaceDesc->ddsCaps);
    }

    if( lpDDSurfaceDesc->dwFlags  & DDSD_WIDTH )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_WIDTH           = %d\r\n"), lpDDSurfaceDesc->dwWidth));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_WIDTH )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_HEIGHT          = %d\r\n"), lpDDSurfaceDesc->dwHeight));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_PITCH )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_PITCH           = %d\r\n"), lpDDSurfaceDesc->lPitch));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_XPITCH )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_XPITCH          = %d\r\n"), lpDDSurfaceDesc->lXPitch));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_BACKBUFFERCOUNT )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_BACKBUFFERCOUNT = %d\r\n"), lpDDSurfaceDesc->dwBackBufferCount));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_LPSURFACE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_LPSURFACE       = 0x%x\r\n"), lpDDSurfaceDesc->lpSurface));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_REFRESHRATE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_REFRESHRATE     = %d\r\n"), lpDDSurfaceDesc->dwRefreshRate));
    if( lpDDSurfaceDesc->dwFlags  & DDSD_SURFACESIZE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_SURFACESIZE     = %d\r\n"), lpDDSurfaceDesc->dwSurfaceSize));


    if( lpDDSurfaceDesc->dwFlags  & DDSD_CKDESTOVERLAY )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_CKDESTOVERLAY:\r\n")));
        DumpDDCOLORKEY(lpDDSurfaceDesc->ddckCKDestOverlay);
    }

    if( lpDDSurfaceDesc->dwFlags  & DDSD_CKSRCOVERLAY )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_CKSRCOVERLAY:\r\n")));
        DumpDDCOLORKEY(lpDDSurfaceDesc->ddckCKSrcOverlay);
    }
    
    if( lpDDSurfaceDesc->dwFlags  & DDSD_CKDESTBLT )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_CKDESTBLT:\r\n")));
        DumpDDCOLORKEY(lpDDSurfaceDesc->ddckCKDestBlt);
    }
    
    if( lpDDSurfaceDesc->dwFlags  & DDSD_CKSRCBLT )
    {
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("DDSD_CKSRCBLT:\r\n")));
        DumpDDCOLORKEY(lpDDSurfaceDesc->ddckCKSrcBlt);
    }
}

//-----------------------------------------------------------
void
DumpDDPIXELFORMAT(
    DDPIXELFORMAT ddPixelFormat
    )
{
    UCHAR   *p = (UCHAR*) &ddPixelFormat.dwFourCC;
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwFlags  = 0x%x\r\n"), ddPixelFormat.dwFlags));

    if( ddPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_ALPHAPIXELS; ")));
    if( ddPixelFormat.dwFlags & DDPF_ALPHA)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_ALPHA; ")));
    if( ddPixelFormat.dwFlags & DDPF_FOURCC)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_FOURCC; ")));
    if( ddPixelFormat.dwFlags & DDPF_PALETTEINDEXED)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_PALETTEINDEXED; ")));
    if( ddPixelFormat.dwFlags & DDPF_RGB)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_RGB; ")));
    if( ddPixelFormat.dwFlags & DDPF_ALPHAPREMULT)
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("    DDPF_ALPHAPREMULT; ")));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwFourCC = 0x%x %c%c%c%c\r\n"), ddPixelFormat.dwFourCC, p[0], p[1], p[2], p[3]));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwRGBBitCount     = %d\r\n"), ddPixelFormat.dwRGBBitCount));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwRBitMask (Y)    = 0x%08x\r\n"), ddPixelFormat.dwRBitMask));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwGBitMask (U)    = 0x%08x\r\n"), ddPixelFormat.dwGBitMask));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwBBitMask (V)    = 0x%08x\r\n"), ddPixelFormat.dwBBitMask));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDPIXELFORMAT dwRGBAlphaBitMask = 0x%08x\r\n"), ddPixelFormat.dwRGBAlphaBitMask));
}

//-----------------------------------------------------------
void
DumpDDOVERLAYFX(
    DDOVERLAYFX ddOverlayFX
    )
{
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVERLAYFX dwAlphaConstBitDepth  = 0x%x\r\n"), ddOverlayFX.dwAlphaConstBitDepth));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVERLAYFX dwAlphaConst          = 0x%x\r\n"), ddOverlayFX.dwAlphaConst));

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVERLAYFX dckDestColorkey:\r\n")));
    DumpDDCOLORKEY(ddOverlayFX.dckDestColorkey);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDOVERLAYFX dckSrcColorkey:\r\n")));
    DumpDDCOLORKEY(ddOverlayFX.dckSrcColorkey);
}

//-----------------------------------------------------------
void
DumpDDCOLORKEY(
    DDCOLORKEY ddColorKey
    )
{
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCOLORKEY dwLowVal  = 0x%x\r\n"), ddColorKey.dwColorSpaceLowValue));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDCOLORKEY dwHighVal = 0x%x\r\n"), ddColorKey.dwColorSpaceHighValue));
}

//-----------------------------------------------------------
void
DumpDDSCAPS_local(
    DDSCAPS ddsCaps
    )
{
    if( ddsCaps.dwCaps & DDSCAPS_OVERLAY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_OVERLAY; ")));
    if( ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_FRONTBUFFER; ")));
    if( ddsCaps.dwCaps & DDSCAPS_BACKBUFFER )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_BACKBUFFER; ")));
    if( ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_PRIMARYSURFACE; ")));
    if( ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_VIDEOMEMORY; ")));
    if( ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_SYSTEMMEMORY; ")));
    if( ddsCaps.dwCaps & DDSCAPS_ALPHA )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_ALPHA; ")));
    if( ddsCaps.dwCaps & DDSCAPS_FLIP )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_FLIP; ")));
    if( ddsCaps.dwCaps & DDSCAPS_PALETTE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_PALETTE; ")));
    if( ddsCaps.dwCaps & DDSCAPS_WRITEONLY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_WRITEONLY; ")));
    if( ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_VIDEOPORT; ")));
    if( ddsCaps.dwCaps & DDSCAPS_READONLY )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_READONLY; ")));
    if( ddsCaps.dwCaps & DDSCAPS_HARDWAREDEINTERLACE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_HARDWAREDEINTERLACE; ")));
    if( ddsCaps.dwCaps & DDSCAPS_NOTUSERLOCKABLE )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_NOTUSERLOCKABLE; ")));
    if( ddsCaps.dwCaps & DDSCAPS_DYNAMIC )
        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (TEXT("  DDSCAPS_DYNAMIC; ")));
}

#endif //DEBUG