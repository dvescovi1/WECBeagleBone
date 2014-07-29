// All rights reserved ADENEO EMBEDDED 2010
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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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


#define IsRGB16(ppixelFormat) \
    ( \
     ((ppixelFormat)->dwRGBAlphaBitMask == 0x0000) && \
     ((ppixelFormat)->dwRBitMask == 0xf800) && \
     ((ppixelFormat)->dwGBitMask == 0x07e0) && \
     ((ppixelFormat)->dwBBitMask == 0x001f))

#define IsARGB16(ppixelFormat) \
    ( \
     ((ppixelFormat)->dwRGBAlphaBitMask == 0xf000) && \
     ((ppixelFormat)->dwRBitMask == 0x0f00) && \
     ((ppixelFormat)->dwGBitMask == 0x00f0) && \
     ((ppixelFormat)->dwBBitMask == 0x000f))


#define IsRGB32(ppixelFormat) \
    ( \
     ((ppixelFormat)->dwRGBAlphaBitMask == 0x00000000) && \
     ((ppixelFormat)->dwRBitMask == 0x00ff0000) && \
     ((ppixelFormat)->dwGBitMask == 0x0000ff00) && \
     ((ppixelFormat)->dwBBitMask == 0x000000ff))

#define IsARGB32(ppixelFormat) \
    ( \
     ((ppixelFormat)->dwRGBAlphaBitMask == 0xff000000) && \
     ((ppixelFormat)->dwRBitMask == 0x00ff0000) && \
     ((ppixelFormat)->dwGBitMask == 0x0000ff00) && \
     ((ppixelFormat)->dwBBitMask == 0x000000ff))

#define IsRGBA32(ppixelFormat) \
    ( \
     ((ppixelFormat)->dwRGBAlphaBitMask == 0x000000ff) && \
     ((ppixelFormat)->dwRBitMask == 0xff000000) && \
     ((ppixelFormat)->dwGBitMask == 0x00ff0000) && \
     ((ppixelFormat)->dwBBitMask == 0x0000ff00))


DWORD   g_dwSurfaceCount = 1;


//-----------------------------------------------------------
DWORD WINAPI 
HalGetBltStatus(
    LPDDHAL_GETBLTSTATUSDATA pd 
    )
{
    pd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalCanCreateSurface( 
    LPDDHAL_CANCREATESURFACEDATA pd 
    )
{
    DWORD   result;
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"+HalCanCreateSurface() -------------------------------\r\n"));
    
    //  Check for surface to create
    if( pd->bIsDifferentPixelFormat )
    {
        LPDDSCAPS           pDDSCaps = &(pd->lpDDSurfaceDesc->ddsCaps);
        LPDDPIXELFORMAT     pDDPixelFormat = &(pd->lpDDSurfaceDesc->ddpfPixelFormat);
        
        
        //  Set default result
        pd->ddRVal = DDERR_UNSUPPORTEDFORMAT;
        result = DDHAL_DRIVER_HANDLED;


        //  Surface is different from primary
        //  Check where surface needs to be allocated on
        if( (pDDSCaps->dwCaps & (DDSCAPS_VIDEOMEMORY|DDSCAPS_SYSTEMMEMORY)) == 0 )
        {
            //  No preference indicated; just use DDGPE function
            result = DDGPECanCreateSurface(pd);
        }
        else if( pDDSCaps->dwCaps & DDSCAPS_SYSTEMMEMORY )
        {
            //  System memory preference indicated; just use DDGPE function
            result = DDGPECanCreateSurface(pd);
        }
        else if( pDDSCaps->dwCaps & DDSCAPS_PRIMARYSURFACE )
        {
            OMAPDDGPE*  pDDGPE = (OMAPDDGPE*) GetGPE();

            //  Check for a match of the pixel type with the primary surface pixel type
            if( pDDPixelFormat->dwFlags & DDPF_RGB )
            {
                //  Check pixel sizes
                if( (pDDPixelFormat->dwRGBBitCount == 16) ||
                    (pDDPixelFormat->dwRGBBitCount == 32) )
                {
                    switch( pDDGPE->GetPrimaryPixelFormat() )
                    {
                        case OMAP_DSS_PIXELFORMAT_RGB16:
                            pd->ddRVal = (IsRGB16(pDDPixelFormat)) ? DD_OK : DDERR_UNSUPPORTEDFORMAT;
                            break;

                        case OMAP_DSS_PIXELFORMAT_RGB32:
                            pd->ddRVal = (IsRGB32(pDDPixelFormat)) ? DD_OK : DDERR_UNSUPPORTEDFORMAT;
                            break;

                        case OMAP_DSS_PIXELFORMAT_ARGB16:
                            pd->ddRVal = (IsARGB16(pDDPixelFormat)) ? DD_OK : DDERR_UNSUPPORTEDFORMAT;
                            break;

                        case OMAP_DSS_PIXELFORMAT_ARGB32:
                            pd->ddRVal = (IsARGB32(pDDPixelFormat)) ? DD_OK : DDERR_UNSUPPORTEDFORMAT;
                            break;

                        case OMAP_DSS_PIXELFORMAT_RGBA32:
                            pd->ddRVal = (IsRGBA32(pDDPixelFormat)) ? DD_OK : DDERR_UNSUPPORTEDFORMAT;
                            break;                            
                    }
                }
            }
        }
        else
        {
            //  Video memory preference; check pixel type
            if( pDDSCaps->dwCaps & DDSCAPS_OVERLAY )
            {
                //  Overlays can support YUV pixel formats
                if( pDDPixelFormat->dwFlags & DDPF_FOURCC )
                {
                    switch( pDDPixelFormat->dwFourCC )
                    {
                        case FOURCC_YUY2:
                        case FOURCC_YUYV:
                        case FOURCC_YVYU:
                        case FOURCC_UYVY:
                            //  Supported formats
                            pd->ddRVal = DD_OK;
                            break;
                    }
                }

                //  Check for RGB formats on overlay
                //  Even though VID2 can support alpha pixels, we are limiting the
                //  set of support pixel types to allow swapping of VID1 and VID2
                //  surfaces to support Z-ordering of overlays
                if( pDDPixelFormat->dwFlags & DDPF_RGB )
                {
                    //  Check pixel sizes
                    if( (pDDPixelFormat->dwRGBBitCount == 16) ||
                        (pDDPixelFormat->dwRGBBitCount == 32) )
                    {
                        //  Check pixel formats
                        if( IsRGB16(pDDPixelFormat) ||
                            IsRGB32(pDDPixelFormat) )
                        {
                            //  Supported formats
                            pd->ddRVal = DD_OK;
                        }
                     }
                 }

                //  Special case to support ARGB32 virtual overlay surfaces 
                //  that are composited onto primary surface via BLT operations
                //  in the display dirver
                if( pDDPixelFormat->dwFlags & DDPF_RGB )
                {
                    //  Check pixel sizes
                    if( (pDDPixelFormat->dwRGBBitCount == 16) ||
                        (pDDPixelFormat->dwRGBBitCount == 32) )
                    {
                        //  Check pixel formats and only single surfaces
                        if( IsARGB32(pDDPixelFormat) && (pd->lpDDSurfaceDesc->dwBackBufferCount == 0) )
                        {
                            //  Supported formats
                            pd->ddRVal = DD_OK;
                        }
                     }
                 }
            }
            
            //  Check for RGB formats on primary display
            else if( pDDPixelFormat->dwFlags & DDPF_RGB )
            {
                //  Check pixel sizes
                if( (pDDPixelFormat->dwRGBBitCount == 16) ||
                    (pDDPixelFormat->dwRGBBitCount == 32) )
                {
                    //  Check pixel formats
                    if( IsRGB16(pDDPixelFormat) ||
                        IsARGB16(pDDPixelFormat) ||
                        IsRGB32(pDDPixelFormat) ||
                        IsARGB32(pDDPixelFormat) ||
                        IsRGBA32(pDDPixelFormat) )
                    {
                        //  Supported formats
                        pd->ddRVal = DD_OK;
                    }
                }
            }
        }
    }
    else
    {
        //  Can create surfaces that match primary surface
        pd->ddRVal = DD_OK;
        result = DDHAL_DRIVER_HANDLED;
    }
    
 //   DumpDD_CANCREATESURFACE(pd);
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"-HalCanCreateSurface() result = 0x%x  pd->ddRVal = 0x%x\r\n\r\n", result, pd->ddRVal));
    
    return result;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalCreateSurface( 
    LPDDHAL_CREATESURFACEDATA pd 
    )
{
    DWORD                       result = DDHAL_DRIVER_HANDLED;
    OMAPDDGPE*                  pDDGPE = (OMAPDDGPE*) GetGPE();
    LPDDSCAPS                   pDDSCaps = &(pd->lpDDSurfaceDesc->ddsCaps);
    LPDDPIXELFORMAT             pDDPixelFormat = &(pd->lpDDSurfaceDesc->ddpfPixelFormat);
    unsigned int                iSurf = 0;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurf;
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"+HalCreateSurface() --------------------------------\r\n"));


    //  Check where surface needs to be allocated on
    if( pDDSCaps->dwCaps & DDSCAPS_PRIMARYSURFACE )
    {
        OMAPDDGPESurface*   pPrimarySurface = (OMAPDDGPESurface*) pDDGPE->DDGPEPrimarySurface();

        //  Primary surface and back buffers
        for( iSurf = 0; iSurf < pd->dwSCnt; iSurf++ )
        {
            //  Initialize surface object
            pSurf = pd->lplpSList[iSurf];
            pSurf->dwReserved1 = 0;

            //  For first surface in chain, just point to primary surface
            if( iSurf == 0 )
            {
                pSurf->dwReserved1 = (DWORD) pPrimarySurface; 
                pSurf->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE|DDSCAPS_VIDEOMEMORY;
                
                //  Update surface description attributes to reflect the primary surface
                pd->lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE|DDSCAPS_VIDEOMEMORY;
                pd->lpDDSurfaceDesc->dwFlags        |= DDSD_PITCH|DDSD_XPITCH|DDSD_SURFACESIZE;
                pd->lpDDSurfaceDesc->lPitch         = pPrimarySurface->OmapSurface()->Stride();
                pd->lpDDSurfaceDesc->lXPitch        = pPrimarySurface->BytesPerPixel();
                pd->lpDDSurfaceDesc->dwSurfaceSize  = pPrimarySurface->SurfaceSize();            
            }
            else
            {
                SCODE               scResult;
                OMAPDDGPESurface*   pBackSurface;
                
                //  Allocate a back buffer to the primary surface
                scResult = pDDGPE->AllocSurface(
                                        &pBackSurface,
                                        pPrimarySurface->OmapSurface()->PixelFormat(),
                                        pPrimarySurface->OmapSurface()->Width(),
                                        pPrimarySurface->OmapSurface()->Height() );
                if (scResult != S_OK)
                {
                    DEBUGMSG (GPE_ZONE_ERROR, (L"ERROR: Unable to allocate primary back buffer surface\r\n"));
                    pd->ddRVal = DDERR_OUTOFVIDEOMEMORY;
                    goto cleanUp;
                }
                
                //  Configure created surface
                pSurf->dwReserved1 = (DWORD) pBackSurface;
                pSurf->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
                pBackSurface->lpDDSurface = pSurf;
                
                //  Parent surface for back buffer chain is the primary surface
                pBackSurface->SetParent( pPrimarySurface );
                
                g_dwSurfaceCount++;
            }
        }
        
        //  Surface creation complete
        pd->ddRVal = DD_OK;
    }
    else if( (pDDSCaps->dwCaps & (DDSCAPS_VIDEOMEMORY|DDSCAPS_SYSTEMMEMORY)) == 0 )
    {
        //  No preference indicated; just use DDGPE function
        result = DDGPECreateSurface(pd);
        g_dwSurfaceCount++;
    }
    else if( pDDSCaps->dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
        //  System memory preference indicated; just use DDGPE function
        result = DDGPECreateSurface(pd);
        g_dwSurfaceCount++;
    }
    else
    {
        OMAP_DSS_PIXELFORMAT    dssPixelFormat = OMAP_DSS_PIXELFORMAT_UNKNOWN;
        
        //  Video memory preference; check pixel type
        if( pDDSCaps->dwCaps & DDSCAPS_OVERLAY )
        {
            //  Overlays can support YUV pixel formats
            if( pDDPixelFormat->dwFlags & DDPF_FOURCC )
            {
                switch( pDDPixelFormat->dwFourCC )
                {
                    case FOURCC_YUY2:
                    case FOURCC_YUYV:
                        //  Supported format
                        dssPixelFormat = OMAP_DSS_PIXELFORMAT_YUV2;
                        break;
                    
                    case FOURCC_YVYU:
                    case FOURCC_UYVY:
                        //  Supported format
                        dssPixelFormat = OMAP_DSS_PIXELFORMAT_UYVY;
                        break;
                }
            }
        }
        
        //  Check for RGB formats (applicable to both primary and overlay surfaces)
        if( pDDPixelFormat->dwFlags & DDPF_RGB )
        {
            //  Check pixel sizes
            if( (pDDPixelFormat->dwRGBBitCount == 16) ||
                (pDDPixelFormat->dwRGBBitCount == 32) )
            {
                //  Check pixel formats
                if( IsRGB16(pDDPixelFormat) )
                {
                    dssPixelFormat = OMAP_DSS_PIXELFORMAT_RGB16;
                }
                else if( IsARGB16(pDDPixelFormat) )
                {
                    dssPixelFormat = OMAP_DSS_PIXELFORMAT_ARGB16;
                }
                else if( IsRGB32(pDDPixelFormat) )
                {
                    dssPixelFormat = OMAP_DSS_PIXELFORMAT_RGB32;
                }
                else if( IsARGB32(pDDPixelFormat) )
                {
                    dssPixelFormat = OMAP_DSS_PIXELFORMAT_ARGB32;
                }
                else if( IsRGBA32(pDDPixelFormat) )
                {
                    dssPixelFormat = OMAP_DSS_PIXELFORMAT_RGBA32;
                }
            }
        }

        //  If the pixel format is supported, create some surfaces
        if( dssPixelFormat != OMAP_DSS_PIXELFORMAT_UNKNOWN )
        {
            OMAPDDGPESurface*   pParent = NULL;

            for( iSurf = 0; iSurf < pd->dwSCnt; iSurf++ )
            {
                SCODE               scResult;
                OMAPDDGPESurface*   pSurface;

                //  Initialize surface object
                pSurf = pd->lplpSList[iSurf];
                pSurf->dwReserved1 = 0;

                //  Allocate a back buffer to the primary surface
                scResult = pDDGPE->AllocSurface(
                                        &pSurface,
                                        dssPixelFormat,
                                        pd->lpDDSurfaceDesc->dwWidth,
                                        pd->lpDDSurfaceDesc->dwHeight
                                        );
                if (scResult != S_OK)
                {
                    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"ERROR: Unable to allocate surface\r\n"));
                    pd->ddRVal = DDERR_OUTOFVIDEOMEMORY;
                    goto cleanUp;
                }
				else
				{
					DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"allocate surface 0x%x\r\n",pSurface->OmapSurface()->PhysicalAddr()));
                }
                
                //  Configure created surface
                pSurf->dwReserved1 = (DWORD) pSurface;
                pSurf->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
                pSurface->lpDDSurface = pSurf;
                
                if( pDDSCaps->dwCaps & DDSCAPS_OVERLAY )
                {                    
                    pSurface->SetOverlay();
#if 0                    
                    /* allocate assoc bufs - to be used by backend processes such as ISP resizer */
                    scResult = pDDGPE->AllocSurface(
                                        pSurface,
                                        dssPixelFormat,
                                        pd->lpDDSurfaceDesc->dwWidth,
                                        pd->lpDDSurfaceDesc->dwHeight
                                        );
                    if (scResult == DDERR_OUTOFVIDEOMEMORY)
                    {
                        DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"ERROR: Unable to allocate RSZ surface\r\n"));
                        pd->ddRVal = DDERR_OUTOFVIDEOMEMORY;
                        goto cleanUp;
                    }
					else if (scResult == S_OK)
					{
						DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"allocate RSZ surface 0x%x\r\n",pSurface->OmapAssocSurface()->PhysicalAddr(OMAP_DSS_ROTATION_0,0,OMAP_ASSOC_SURF_FORCE_OFF)));
					}
                    else
                    {
                        /* other reasons - no need to log */
                    }
#endif                    
                }
                
                //  Update surface description attributes to reflect the created surface
                //  All back buffer surfaces will match first surface in chain
                if( iSurf == 0 )
                {
                    pd->lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
                    pd->lpDDSurfaceDesc->dwFlags        |= DDSD_PITCH|DDSD_XPITCH|DDSD_SURFACESIZE;
                    pd->lpDDSurfaceDesc->lPitch         = pSurface->OmapSurface()->Stride();
                    pd->lpDDSurfaceDesc->lXPitch        = pSurface->BytesPerPixel();
                    pd->lpDDSurfaceDesc->dwSurfaceSize  = pSurface->SurfaceSize();            

                    pParent = pSurface;
                }
                else
                {
                    //  Parent surface for back buffer chain is the first surface created
                    pSurface->SetParent( pParent );
                }

                g_dwSurfaceCount++;
            }
            
            //  Surface creation complete
            pd->ddRVal = DD_OK;
        }
        else
        {
            pd->ddRVal = DDERR_UNSUPPORTEDFORMAT;
        }
    }

  
cleanUp:
    //  Clean up any allocations that were successful on a failure condition
    if( pd->ddRVal != DD_OK )
    {
        unsigned int    i;
        for( i = 0; i < iSurf; i++ )
        {
            pSurf = pd->lplpSList[i];

            //  Delete allocated surfaces (except for primary)
            if( (pSurf->dwReserved1 != 0) && (pSurf->dwReserved1 != (DWORD) pDDGPE->DDGPEPrimarySurface()) )
            {
                DDGPESurf::DeleteSurface(pSurf);
                pSurf->dwReserved1 = 0;
            }
        }
    }
    
        
    DumpDD_CREATESURFACE(pd);
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"-HalCreateSurface() result = 0x%x  pd->ddRVal = 0x%x  surface count = %d\r\n\r\n", result, pd->ddRVal, g_dwSurfaceCount));
    
    return result;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalDestroySurface( 
    LPDDHAL_DESTROYSURFACEDATA pd 
    )
{
    DWORD               result;
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface*   pSurf = (OMAPDDGPESurface*) DDGPESurf::GetDDGPESurf(pd->lpDDSurface);
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"+HalDestroySurface() ----------------------------\r\n"));
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"pd->lpDDSurface = 0x%x  pSurf = 0x%x\r\n", pd->lpDDSurface, pSurf));

    //  If the primary surface is being destroyed, disable all color keys
    if( pSurf == GetDDGPE()->PrimarySurface() )
    {
        pDDGPE->DisableColorKey();
        pDDGPE->DisableAlphaConst();
    }
    else
    {
        //  If the surface is an overlay and the first in a chain of flipping surfaces, hide it
        //  For some reason, WM7 doesn't hide overlay surfaces prior to deleting them
        if( pSurf && pSurf->IsOverlay() && (pSurf == pSurf->Parent()) )
        {
            pDDGPE->HideOverlay( pSurf );
        }

        g_dwSurfaceCount--;
    }

    //  Call DDGPE function
    result = DDGPEDestroySurface(pd);

    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"-HalDestroySurface() result = 0x%x  pd->ddRVal = 0x%x surface count = %d\r\n\r\n", result, pd->ddRVal, g_dwSurfaceCount));
    
    return result;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalFlip(
    LPDDHAL_FLIPDATA pd
    )
{
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface*   pSurfCurr = (OMAPDDGPESurface*) DDGPESurf::GetDDGPESurf(pd->lpSurfCurr);
    OMAPDDGPESurface*   pSurfTarg = (OMAPDDGPESurface*) DDGPESurf::GetDDGPESurf(pd->lpSurfTarg);

    if(pDDGPE->FlipInProgress() && 
        (pSurfCurr != NULL && pDDGPE->SurfaceFlipping(pSurfCurr,FALSE)))
    {   
        if (pd->dwFlags & (DDFLIP_WAITNOTBUSY|DDFLIP_WAITVSYNC)) 
        {            
            pDDGPE->WaitForVBlank();            
        }
        else
        {
           //  Display controller still performing a flip
            pd->ddRVal = DDERR_WASSTILLDRAWING; 
            return DDHAL_DRIVER_HANDLED;
        }
    }  

    //  Lock the display driver
    pDDGPE->Lock();
    //  Flip to the target surface
    pd->ddRVal = pDDGPE->FlipSurface(pSurfTarg);    
    //  Unlock the display driver
    pDDGPE->Unlock();   
    
    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalGetFlipStatus(
    LPDDHAL_GETFLIPSTATUSDATA pd 
    )
{
    OMAPDDGPE*  pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface * pSurf = (OMAPDDGPESurface*)DDGPESurf::GetDDGPESurf(pd->lpDDSurface);

	pd->ddRVal = DD_OK;

    if ( (pd->dwFlags & DDGFS_ISFLIPDONE) &&
            pDDGPE->FlipInProgress())
    {               
        if((pSurf != NULL && pDDGPE->SurfaceFlipping(pSurf,FALSE)))
        {
            pd->ddRVal = DDERR_WASSTILLDRAWING;        
        }
		else
			pd->ddRVal = DD_OK;
    }   
    else if ( (pd->dwFlags & DDGFS_CANFLIP) &&
                pDDGPE->SurfaceFlipping(pSurf,FALSE))
    {        
        pd->ddRVal = DDERR_WASSTILLDRAWING;             
    }
    else 
    {
        pd->ddRVal = DD_OK;
    }
    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI
HalGetScanLine(
    LPDDHAL_GETSCANLINEDATA lpgsld
    )
{
    OMAPDDGPE*  pDDGPE = (OMAPDDGPE*) GetGPE();

    lpgsld->dwScanLine = pDDGPE->GetScanLine();
    lpgsld->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;    
}
//-----------------------------------------------------------
DWORD WINAPI 
HalLock(
    LPDDHAL_LOCKDATA pd
    )
{
    OMAPDDGPE*  pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface * pSurf = (OMAPDDGPESurface*)DDGPESurf::GetDDGPESurf(pd->lpDDSurface);

    //Check to see if the surface to be locked is still being shown
    //check is only done on primary or overlay flip chains, other surfaces will always return false
    if (pDDGPE->SurfaceFlipping(pSurf,TRUE))
    {
        if (pd->dwFlags & DDLOCK_WAITNOTBUSY)
        {            
            pDDGPE->WaitForVBlank();
            // sometimes due to weird timing issue, Vysnc and Go bit are set at almost the same time
            // this causes the GO bit to not reset but the IRQStatus register shows Vsync bit set after 
            // HalFlip is called. Hence we check twice the IRQStatus register twice here.            
            if (pDDGPE->SurfaceFlipping(pSurf,TRUE)) {                
                pDDGPE->WaitForVBlank();
            }
        }
        else
        {
            pd->ddRVal = DDERR_WASSTILLDRAWING;
            return DDHAL_DRIVER_HANDLED;
        }
    }

    
    //  Get pointer to surface memory
    DWORD ulAddress;
    int x = 0;
    int y = 0;
    if (pd->bHasRect) 
    {
        x = pd->rArea.left;
        y = pd->rArea.top;
    }

    ulAddress = (ULONG) pSurf->GetPtr(x,y);
    pd->lpSurfData = reinterpret_cast<LPVOID>(ulAddress);
    pd->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalUnlock(
    LPDDHAL_UNLOCKDATA pd
    )
{
    DWORD       result;
    
    //  Call DDGPE function
    result = DDGPEUnlock(pd);

    return result;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalSetColorKey(
    LPDDHAL_SETCOLORKEYDATA pd
    )
{
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();
    
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"+HalSetColorKey() -------------------------------\r\n"));

    DumpDD_SETCOLORKEY(pd);
    

    //  Default result
    pd->ddRVal = DDERR_COLORKEYNOTSET;

    //  Set the desired color key for overlays
    if( pd->dwFlags & DDCKEY_SRCOVERLAY )
    {
        //  Check for special values for disable
        if( pd->ckNew.dwColorSpaceLowValue == -1 )
        {
            pd->ddRVal = pDDGPE->DisableColorKey();
        }
        else
        {        
            pd->ddRVal = pDDGPE->SetSrcColorKey( pd->ckNew.dwColorSpaceLowValue );
        }
    }

    if( pd->dwFlags & DDCKEY_DESTOVERLAY )
    {
        //  Check for special values for disable
        if( pd->ckNew.dwColorSpaceLowValue == -1 )
        {
            pd->ddRVal = pDDGPE->DisableColorKey();
        }
        else
        {        
            pd->ddRVal = pDDGPE->SetDestColorKey( pd->ckNew.dwColorSpaceLowValue );
        }
    }

    //  SW BLT supports this color key type
    if( pd->dwFlags & DDCKEY_SRCBLT )
    {
        pd->ddRVal = DDGPESetColorKey( pd );
    }

          
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"-HalSetColorKey() pd->ddRVal = 0x%x\r\n\r\n", pd->ddRVal));
    
    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalUpdateOverlay(
    LPDDHAL_UPDATEOVERLAYDATA pd
    )
{
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface*   pOverlaySurf = (OMAPDDGPESurface*) DDGPESurf::GetDDGPESurf(pd->lpDDSrcSurface);
    BOOL                bMirror = (pd->dwFlags & DDOVER_MIRRORLEFTRIGHT) ? TRUE : FALSE;
    
    DWORD               result = DDHAL_DRIVER_NOTHANDLED;
   
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"+HalUpdateOverlay() -------------------------------\r\n"));
    
    DumpDD_UPDATEOVERLAY(pd);
    

    //  Set the desired color key for over-rides
    if( pd->dwFlags & DDOVER_KEYSRCOVERRIDE )
    {
        pd->ddRVal = pDDGPE->SetSrcColorKey( pd->overlayFX.dckSrcColorkey.dwColorSpaceLowValue );
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }

    if( pd->dwFlags & DDOVER_KEYDESTOVERRIDE )
    {
        pd->ddRVal = pDDGPE->SetDestColorKey( pd->overlayFX.dckDestColorkey.dwColorSpaceLowValue );
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }

    if( pd->dwFlags & DDOVER_ALPHACONSTOVERRIDE )
    {
        //  If alpha const bit depth is set at 1 bits, disable constant alpha
        if( pd->overlayFX.dwAlphaConstBitDepth == 1 )
        {
            pDDGPE->DisableAlphaConst();
        }
        
        //  If alpha const bit depth is set at 8 bits, set GFX alpha to given value and VID2 to 100%
        if( pd->overlayFX.dwAlphaConstBitDepth == 8 )
        {
            pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX, pd->overlayFX.dwAlphaConst );
            pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2, 0xff );
        }
        
        //  If alpha const bit depth is set at 16 bits, set GFX and VID2 alphas to given values
        if( pd->overlayFX.dwAlphaConstBitDepth == 16 )
        {
            pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX, (pd->overlayFX.dwAlphaConst & 0x00ff) );
            pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2, ((pd->overlayFX.dwAlphaConst >> 8) & 0x00ff) );
        }
        
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }

    if ( pd->dwFlags & DDOVER_ALPHADEST )
    {
        pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_GFX, 0xff );
        pd->ddRVal = pDDGPE->SetAlphaConst( OMAP_DSS_COLORKEY_GLOBAL_ALPHA_VIDEO2, 0xff );
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }

    //  Show overlay pipeline
    if( pd->dwFlags & DDOVER_SHOW )
    {
        if (pOverlaySurf != NULL)
        {
        pd->ddRVal = pDDGPE->ShowOverlay( pOverlaySurf, &pd->rSrc, &pd->rDest, bMirror);
        }
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }  

    //  Hide overlay pipeline
    if( pd->dwFlags & DDOVER_HIDE )
    {
        if (pOverlaySurf != NULL)
        {
        pd->ddRVal = pDDGPE->HideOverlay( pOverlaySurf );
        }
        result = DDHAL_DRIVER_HANDLED;
        if (DD_OK != pd->ddRVal)
        {
            goto Cleanup;
        }
    }

    if( pd->dwFlags == 0 )
    {
        pd->ddRVal = 0;
        result = DDHAL_DRIVER_HANDLED;
    }


Cleanup:
    DEBUGMSG(GPE_ZONE_DDRAW_HAL, (L"-HalUpdateOverlay() result = 0x%x  pd->ddRVal = 0x%x\r\n\r\n", result, pd->ddRVal));
    
    return result;
}

//-----------------------------------------------------------
DWORD WINAPI 
HalSetOverlayPosition(
    LPDDHAL_SETOVERLAYPOSITIONDATA  pd
    )
{
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();
    OMAPDDGPESurface*   pOverlaySurf = (OMAPDDGPESurface*) DDGPESurf::GetDDGPESurf(pd->lpDDSrcSurface);
    
    //  Update the overlay position
    pd->ddRVal = pDDGPE->MoveOverlay( pOverlaySurf, pd->lXPos, pd->lYPos );

    return DDHAL_DRIVER_HANDLED;
}

//-----------------------------------------------------------
DWORD WINAPI
HalWaitForVerticalBlank(
    LPDDHAL_WAITFORVERTICALBLANKDATA lpwfvbd
    )
{
    OMAPDDGPE*          pDDGPE = (OMAPDDGPE*) GetGPE();

    if(lpwfvbd->dwFlags & DDWAITVB_I_TESTVB)
    {
        lpwfvbd->bIsInVB = pDDGPE->InVBlank();
    }

    //DSS only interrupts at the beginning of vsync.
    //OMAP3xxx does not have a way to query if we are out of vertical blank interval
    //the only information is the current scanline. 
    if((lpwfvbd->dwFlags & DDWAITVB_BLOCKEND) ||
        (lpwfvbd->dwFlags & DDWAITVB_BLOCKBEGIN))
    {
        pDDGPE->WaitForVBlank();
    }

    lpwfvbd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;


}
