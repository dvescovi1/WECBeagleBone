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
//  Module Name:    halsurf.cpp
//
// -----------------------------------------------------------------------------

#include "precomp.h"

//------------------------------------------------------------------------------
//
//  Function: DetectPixelFormat
//
//  Retrieve pFormat and pPixelFormat based on pDDPF
//
SCODE DetectPixelFormat(DDPIXELFORMAT *pDDPF,
                        EGPEFormat *pFormat,
                        EDDGPEPixelFormat *pPixelFormat)
{
    if( pDDPF->dwFlags & DDPF_PALETTEINDEXED )
    {
        if( pDDPF->dwRGBBitCount == 8 )
        {
            *pPixelFormat = ddgpePixelFormat_8bpp;
        }
        else
        {
            return DDERR_UNSUPPORTEDFORMAT;
        }
    }
    else if( pDDPF->dwFlags & DDPF_RGB )
    {
        if( pDDPF->dwRGBBitCount == 16 )
        {
            if( pDDPF->dwRGBAlphaBitMask == 0x0000 &&
                pDDPF->dwRBitMask == 0xF800 )
            {
                DEBUGMSG(1, (TEXT("565 surface detected\r\n") ));
                *pPixelFormat = ddgpePixelFormat_565;
            }
//*    Apparently enabling the following formats will fix a no texture issue on 'text3dce.exe'
            else if( pDDPF->dwRGBAlphaBitMask == 0x8000 &&
                     pDDPF->dwRBitMask == 0x7C00 )
            {
                DEBUGMSG(1, (TEXT("1555 surface detected\r\n") ));
                *pPixelFormat = ddgpePixelFormat_5551;
            }
            else if( pDDPF->dwRGBAlphaBitMask == 0xF000 &&
                pDDPF->dwRBitMask == 0x0F00 )
            {
                DEBUGMSG(1, (TEXT("4444 surface detected\r\n") ));
                *pPixelFormat = ddgpePixelFormat_4444;
            }
            else if( pDDPF->dwRGBAlphaBitMask == 0x0000 &&
                     pDDPF->dwRBitMask == 0x7C00 )
            {
                DEBUGMSG(1, (TEXT("0555 surface detected\r\n") ));
                *pPixelFormat = ddgpePixelFormat_5550;
            }
//*/
            else {
                DEBUGMSG(1, (TEXT("DetectFormat: Unsupported 16-bit format\r\n") ));
                return DDERR_UNSUPPORTEDFORMAT;
            }
        }
        else
        {
            DEBUGMSG(1, (TEXT("DetectFormat: Unsupported bit-depth (%d)\r\n"), pDDPF->dwRGBBitCount));
            return DDERR_UNSUPPORTEDFORMAT;
        }
    }
    else if( pDDPF->dwFlags & DDPF_FOURCC )
    {
        DEBUGMSG(1, (TEXT("DetectFormat: Unsupported FOURCC %08x\r\n"), pDDPF->dwFourCC ));
        return DDERR_UNSUPPORTEDFORMAT;
    }
    else
    {
        DEBUGMSG(1, (TEXT("DetectFormat: Unsupported Non-RGB, Non-FOURCC surface.\r\n")));
        return DDERR_UNSUPPORTEDFORMAT;
    }
    *pFormat = EDDGPEPixelFormatToEGPEFormat[*pPixelFormat];

    return DD_OK;
}


//------------------------------------------------------------------------------
//
//  Function: HalCreateSurface
//
//  Verify the surface request is valid.  If so, pass the request to the lower
//  layers.
//
DWORD WINAPI HalCreateSurface( LPDDHAL_CREATESURFACEDATA pd )
{
    DEBUGENTER( HalCreateSurface );

    // Implementation
    DWORD result;
    EGPEFormat format;
    EDDGPEPixelFormat pixelFormat;
    EDDGPEPixelFormat primaryPixelFormat;
    DWORD dwFlags = pd->lpDDSurfaceDesc->dwFlags;
    DWORD dwCaps = pd->lpDDSurfaceDesc->ddsCaps.dwCaps;

    // get the pixel format for the primary surface
    GPEMode modeInfo;
    SCODE sc = ((LCDDDGPE *)GetDDGPE())->GetModeInfo(&modeInfo, 0);
    if (FAILED(sc))
    {
        RETAILMSG(1, (TEXT("ERROR: Display: can't get mode info\r\n")));
        pd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    GPEModeEx modeInfoEx;
    sc = ((LCDDDGPE *)GetDDGPE())->GetModeInfoEx(&modeInfoEx, 0);
    if (FAILED(sc))
    {
        primaryPixelFormat = EGPEFormatToEDDGPEPixelFormat[modeInfo.format];
    }
    else
    {
        primaryPixelFormat = modeInfoEx.ePixelFormat;
    }

    // get the pixel format for the surface to be created
    if (dwFlags & DDSD_PIXELFORMAT)
    {
        pd->ddRVal = DetectPixelFormat(
                         &pd->lpDDSurfaceDesc->ddpfPixelFormat,
                         &format,
                         &pixelFormat);
        if (FAILED(pd->ddRVal))
        {
            RETAILMSG( 1, (TEXT("HalCreateSurface: pixel error\n")) );
            goto TryDDGPE;
        }
    }
    else
    {
       pixelFormat = primaryPixelFormat;
    }

    // All VRAM surfaces must have the same format as the primary surface
    if ((dwCaps & DDSCAPS_VIDEOMEMORY) &&
        (pixelFormat != primaryPixelFormat))
    {
        RETAILMSG(1, (TEXT("HalCreateSurface: non-overlay video memory surface must have same format as primary surface\r\n")));
        pd->ddRVal = DDERR_UNSUPPORTEDFORMAT;
        return DDHAL_DRIVER_HANDLED;
    }

TryDDGPE:
    // let the lower layer decide if there is enough VRAM / sysram for the request
    result = DDGPECreateSurface(pd);
    return result;
}


//------------------------------------------------------------------------------
//
//  Function: HalCanCreateSurface
//
//  Check whether is it is possible to create this sort of surface
//
DWORD WINAPI HalCanCreateSurface( LPDDHAL_CANCREATESURFACEDATA pd )
{
    DEBUGENTER( HalCanCreateSurface );

    // Implementation
    DDPIXELFORMAT    *pddpf  = &pd->lpDDSurfaceDesc->ddpfPixelFormat;
    DWORD            caps    = pd->lpDDSurfaceDesc->ddsCaps.dwCaps;

    // We do not allow a system memory primary.
    if ((caps & DDSCAPS_PRIMARYSURFACE) &&
        (caps & DDSCAPS_SYSTEMMEMORY))
    {
        goto CannotCreate;
    }

    // All VRAM surfaces must have the same format as the primary surface
    if ((caps & DDSCAPS_VIDEOMEMORY) &&
        (pd->bIsDifferentPixelFormat))
    {
        goto CannotCreate;
    }

    pd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

CannotCreate:
    RETAILMSG(1, (TEXT("HalCanCreateSurface: Unsupported\r\n")));
    pd->ddRVal = DDERR_UNSUPPORTEDFORMAT;
    return DDHAL_DRIVER_HANDLED;
}


//-----------------------------------------------------------
//
//  Function: HalWaitForVerticalBlank
//
//  Waits for vertical blank
//
DWORD WINAPI
HalWaitForVerticalBlank(
    LPDDHAL_WAITFORVERTICALBLANKDATA lpwfvbd
    )
{
    LCDDDGPE *pDDGPE = (LCDDDGPE*) GetGPE();

    switch(lpwfvbd->dwFlags)
    {
        case DDWAITVB_I_TESTVB:
            // Request for the current vertical blank status
	        lpwfvbd->bIsInVB = pDDGPE->InVBlank();

            lpwfvbd->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKBEGIN:
            // Returns when the vertical blank interval begins.
			pDDGPE->WaitForVBlank();

            lpwfvbd->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKEND:

			// Returns when the vertical blank interval ends and display begins.
            lpwfvbd->ddRVal = DDERR_NOVSYNCHW; // TODO:
            return DDHAL_DRIVER_HANDLED;
    }

    return DDHAL_DRIVER_NOTHANDLED;
}

// <end of file>

