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
//  Module Name:    surf.cpp
//
// -----------------------------------------------------------------------------

/* surf.cpp
 *
 * Allocation of Directdraw surfaces
 *
 * Copyright (C) MPC Data Limited 2007. Some rights reserved.
 *
 */

#include "precomp.h"

// An extra flag so we can identify when a GPE surface is required instead of
// a full DirectDraw one.
#define GPE_SURF            0x80000000

//------------------------------------------------------------------------------
//
//  Member: SetVisibleSurface
//
//  Set the surface to be displayed on the direct draw device.
//  This is *NOT* called for overlays
//
void LCDDDGPE::SetVisibleSurface(GPESurf *pSurf, BOOL  bWaitForVBlank)
{
    DEBUGMSG( GPE_ZONE_CREATE, (TEXT("SetVisibleSurface: blank=%d : offset=%p\n"), bWaitForVBlank, pSurf->OffsetInVideoMemory()) );

    // The new framebuffer address is not latched until the next vsync
    // so we must wait until that happens.  Otherwise the previous buffer
    // will still be shown and the redraw process will be visible

    // We *MUST* make sure the LCD controller is diabled before updating
    // the DMA registers.  Otherwise we the DMA engine could miss the
    // end of framebuffer 'CEILING' and continue into invlaid addresses.
    LcdDisable();
    Sleep(10);      // Give it a moment to stop.

    // We must write the framebuffer addresses to the DMA engine registers
    // *BEFORE* enabling the LCD controller so the DMA engine is always
    // transferring vlaid data, and therefore starts in sync.
    // We must also specify the same buffer to be set on each vsync IRQ.
    LcdSetFrameBufferImmediate( m_Lcdc.fb_pa + pSurf->OffsetInVideoMemory(), m_Lcdc.fb_cur_win_size );
    LcdSetFrameBufferOnVsync( m_Lcdc.fb_pa + pSurf->OffsetInVideoMemory(), m_Lcdc.fb_cur_win_size );

    // Ok, the buffers are setup, so we are ready to go!
    LcdEnable();

    DEBUGMSG(GPE_ZONE_CREATE, ((L"SetVisibleSurface completed!\r\n")));
    
    return;
}



//------------------------------------------------------------------------------
//
//  Member: AllocSurface (GPESurf)
//
//  Called when a GPE surface is required.  We just forward the request to
//  the DirectDraw allocator, but add the GPE_SURF flag. 
//
SCODE LCDDDGPE::AllocSurface(GPESurf **ppSurf, int width, int height, EGPEFormat format, int surfaceFlags)
{ 
    return AllocSurface((DDGPESurf**)ppSurf, 
                        width, 
                        height,
                        format, 
                        EGPEFormatToEDDGPEPixelFormat[format],
                        surfaceFlags | GPE_SURF);
}


//------------------------------------------------------------------------------
//
//  Member: AllocSurface (DDGPESurf)
//
//  Allocate a DirectDraw surface (or GPE if the GPE_SURF flag is set)
//
SCODE LCDDDGPE::AllocSurface(DDGPESurf **ppSurf, int width, int height,
    EGPEFormat format, EDDGPEPixelFormat pixelFormat, int surfaceFlags)
{
    DWORD bpp;
    LONG align;
    DWORD alignedWidth;
    DWORD nSurfaceSize;
    DWORD stride;

    bpp = EGPEFormatToBpp[format];

    // LCD unit requires every display line to be 32byte
    // aligned.  So we calculate the alignment here.
    align = VRAM_GRAIN;
    alignedWidth = ((width + align - 1) & (-align));
    nSurfaceSize = (bpp * (alignedWidth * height)) / 8;
    stride = ( ((alignedWidth * bpp) / 8) + (align-1) ) & (-align);

    DEBUGMSG( GPE_ZONE_CREATE,(L"bpp=%d alignW=%d sufsize=%d stride=%d flags=0x%08x\n",
            bpp, alignedWidth, nSurfaceSize, stride, surfaceFlags));

    if ((surfaceFlags & GPE_BACK_BUFFER) ||
        (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY) )
    {
        DEBUGMSG( GPE_ZONE_CREATE,(L"backbuffer or VRAM required\n"));

        // Attempt to allocate from video memory
        SurfaceHeap *pStack = m_pVideoMemoryHeap->Alloc(nSurfaceSize);
        if (pStack)
        {
            *ppSurf = new LCDDDGPESurf( width, alignedWidth, height,
                                    (DWORD)pStack->Address() - (DWORD)m_Lcdc.fb_va,
                                    (void*)pStack->Address(),
                                    stride,
                                    format,
                                    pixelFormat,
                                    pStack,
                                    this);

            if (!(*ppSurf))
            {
                DEBUGMSG( GPE_ZONE_CREATE,(L"out of VRAM!\n"));
                pStack->Free();
                return DDERR_OUTOFVIDEOMEMORY;
            }
            return S_OK;
        }
        else
        {
            DEBUGMSG( GPE_ZONE_CREATE,(L"out of VRAM!\n"));
            *ppSurf = NULL;
            return DDERR_OUTOFVIDEOMEMORY;
        }
    }
    DEBUGMSG( GPE_ZONE_CREATE,(L"allocte from sysram\n"));

    // A DirectDraw surface
    if ((surfaceFlags & GPE_SURF) == 0)
    {
        // Create DDGPE surface in memory
        *ppSurf = new DDGPESurf(width, height, stride, format, pixelFormat);
        DEBUGMSG( GPE_ZONE_CREATE,(TEXT("AllocSurface: create DDGPESurf in SYSRAM %p"), *ppSurf) );
    }
    // A regular GPE surface
    else 
    {
        // Create GPE surface in memory
        *((GPESurf**)ppSurf) = new GPESurf(width, height, format);
        DEBUGMSG( GPE_ZONE_CREATE,(TEXT("AllocSurface: create GPESurf in SYSRAM %p"), *ppSurf) );
    }

    if (*ppSurf)
    {
        // check we allocated bits succesfully
        if (!((*ppSurf)->Buffer()))
        {
            DEBUGMSG( GPE_ZONE_CREATE,(L"out of sysram!\n"));
            delete *ppSurf;
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}


//------------------------------------------------------------------------------
//
//  Member: Constructor
//
//  This is a wrapper which inherits from [DDGPESurf] that actually creates
//  the surface.  It also saves some details about the created surface.
//
LCDDDGPESurf::LCDDDGPESurf(int width, int alignedWidth, int height, ULONG offset, PVOID pBits, 
                 int stride, EGPEFormat format, EDDGPEPixelFormat pixelFormat,
                 SurfaceHeap *pHeap, LCDDDGPE *pLCDDDGPE)
                : DDGPESurf(width, height, pBits, stride, format, pixelFormat)
{
    m_nHandle = NULL;
    m_pHeap = pHeap;
    m_nAlignedWidth = alignedWidth;
    m_fInVideoMemory = TRUE;
    m_nOffsetInVideoMemory = offset;
    m_pLCDDDGPE = pLCDDDGPE;
}


//------------------------------------------------------------------------------
//
//  Member: Destructor
//
//  Automatically releases the VRAM allocated to the surface.
//
LCDDDGPESurf::~LCDDDGPESurf(void)
{
    m_pHeap->Free();
}



