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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

Abstract:  
  
Functions:


Notes: 


--*/

#include "precomp.h"

#define PALETTE_SIZE              	4
RGBQUAD _rgb2bpp[PALETTE_SIZE] =
{
    { 0x00, 0x00, 0x00, 0 },    /*   0 */	/* Black        */   	   \
    { 0x80, 0x80, 0x80, 0 },    /* 248 */	/* Dark Grey         	*/ \
    { 0xc0, 0xc0, 0xc0, 0 },    /*   7 */	/* Light Grey   */   	   \
    { 0xff, 0xff, 0xff, 0 }     /* 255 */	/* White         		*/ \
};


INSTANTIATE_GPE_ZONES(0x3,"MGDI Driver","unused1","unused2")	 /* Start with Errors, warnings, and temporary messages */

BOOL APIENTRY GPEEnableDriver(          // This gets around problems exporting from .lib
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded,
	PENGCALLBACKS  pEngCallbacks);
BOOL APIENTRY DrvEnableDriver(
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded,
	PENGCALLBACKS  pEngCallbacks)
{
	return GPEEnableDriver( iEngineVersion, cj, pded, pEngCallbacks );
}


static GPE *pGPE = (GPE *)NULL;
// Main entry point for a GPE-compliant driver
GPE *GetGPE()
{
	if( !pGPE )
		pGPE = new Wrap2bpp();
	return pGPE;
}


Wrap2bpp::Wrap2bpp()
{
	DEBUGMSG( GPE_ZONE_INIT,(TEXT("Wrap2bpp::Wrap2bpp\r\n")));

	DispDrvrInitialize();	// call entry point to 2bpp GDI driver
	
	// When this DispDrvrInitialize returns, DispDrvrPhysicalFrameBuffer contains
	// the physical address of the screen if it is in 2bpp DIB format. - Alternatively it
	// is NULL - in which case this is a "dirty-rect" driver

	m_ModeInfo.modeId = 0;
	m_ModeInfo.width = m_nScreenWidth = DispDrvr_cxScreen;
	m_ModeInfo.height = m_nScreenHeight = DispDrvr_cyScreen;
	m_ModeInfo.Bpp = 16;
	m_ModeInfo.frequency = 60;		// not too important
	m_ModeInfo.format = gpe16Bpp;

	m_pMode = &m_ModeInfo;
	
	if( DispDrvrPhysicalFrameBuffer == NULL )
	{
		// it is a "dirty-rect" driver - we create a system memory bitmap to represent
		// the screen and refresh rectangles of this to the screen as they are altered
		m_pPrimarySurface = new GPESurf( m_nScreenWidth, m_nScreenHeight, gpe16Bpp );
		if (!m_pPrimarySurface)
		{
		    RETAILMSG(1, (TEXT("Wrap2bpp::Wrap2bpp: Error allocating GPESurf.\r\n")));
		    return;

		}

		DispDrvrSetDibBuffer( m_pPrimarySurface->Buffer() );
	}
	else
	{
	   // RETAILMSG(1, ("HARDWARE DIb!!!!\r\n"));
		// The hardware is arranged as a DIB
		// Map it into virtual addr space
		unsigned long fbSize = DispDrvr_cxScreen * DispDrvr_cdwStride / 4;
		m_pVirtualFrameBuffer = VirtualAlloc( 0, fbSize, MEM_RESERVE, PAGE_NOACCESS );
		VirtualCopy( m_pVirtualFrameBuffer, DispDrvrPhysicalFrameBuffer, fbSize,
			PAGE_READWRITE | PAGE_NOCACHE );
			
		m_pPrimarySurface = new GPESurf( m_nScreenWidth, m_nScreenHeight, m_pVirtualFrameBuffer,
			 DispDrvr_cdwStride / 4, gpe2Bpp );
	}
}

SCODE Wrap2bpp::SetMode( int modeId, HPALETTE *pPalette )
{
	ULONG	uColors[4];

	if( modeId != 0 )
		return E_INVALIDARG;

	// Here, we use EngCreatePalette to create a palette that that MGDI will use as a
	// stock palette

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("SetMode - creating stock palette\r\n")));

	if( pPalette )
	{
#if 0	
		*pPalette = EngCreatePalette
							(
								PAL_INDEXED,
								PALETTE_SIZE, 	// i.e. 2
								(ULONG *)_rgb2bpp,
								0,
								0,
								0
							);
#else

	//	Colors are always in RGBA order
	uColors[0] = 0xf800;
	uColors[1] = 0x07e0;
	uColors[2] = 0x001f;
	uColors[3] = 0;

	//	Create palette info for selected mode
	*pPalette = EngCreatePalette (
					PAL_BITFIELDS,
					3,
					uColors,
					0,
					0,
					0);

#endif
//		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Created 2 Bpp palette, handle = 0x%08x\r\n"),*pPalette));
	}

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("SetMode done\r\n")));

	return S_OK;				// Mode is inherently set
}

SCODE Wrap2bpp::GetModeInfo
(
	GPEMode *pMode,
	int modeNo
)
{
	if( modeNo != 0 )
		return E_INVALIDARG;

	*pMode = m_ModeInfo;

	return S_OK;
}

int Wrap2bpp::NumModes()
{
	return 1;
}

SCODE Wrap2bpp::SetPointerShape(
	GPESurf *pMask,
	GPESurf *pColorSurf,
	int xHot,
	int yHot,
	int cx,
	int cy )
{
	return S_OK;
}

SCODE Wrap2bpp::MovePointer(
	int x,
	int y )
{
	DEBUGMSG(GPE_ZONE_HW,(TEXT("Moving cursor to %d,%d\r\n"), x, y ));
	if( x == -1 )
	{
		DispDrvrMoveCursor( DispDrvr_cxScreen, DispDrvr_cyScreen );		// disable cursor
	}
	{
		DispDrvrMoveCursor( x, y );		// disable cursor
	}
	return S_OK;
}

void Wrap2bpp::WaitForNotBusy()
{
	return;
}

int Wrap2bpp::IsBusy()
{
	return 0;	// Never busy as there is no acceleration
}

void Wrap2bpp::GetPhysicalVideoMemory
(
	unsigned long *pPhysicalMemoryBase,
	unsigned long *pVideoMemorySize
)
{
	*pPhysicalMemoryBase = (unsigned long)DispDrvrPhysicalFrameBuffer;
	*pVideoMemorySize = DispDrvr_cdwStride * DispDrvr_cyScreen / 4;
}


SCODE Wrap2bpp::AllocSurface(
	GPESurf **ppSurf,
	int width,
	int height,
	EGPEFormat format,
	int surfaceFlags )
{
	if( surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY )
		return E_OUTOFMEMORY;	// Can't allocate video-memory surfaces in the Wrap2bpp environment
	// Allocate from system memory
	DEBUGMSG(GPE_ZONE_CREATE,(TEXT("Creating a GPESurf in system memory. EGPEFormat = %d\r\n"), (int)format ));
	*ppSurf = new GPESurf( width, height, format );
	if( *ppSurf )
	{
		// check we allocated bits succesfully
		if( !((*ppSurf)->Buffer()) )
			delete *ppSurf;	// and then return E_OUTOFMEMORY
		else
			return S_OK;
	}
	return E_OUTOFMEMORY;
}

SCODE Wrap2bpp::WrappedEmulatedLine( GPELineParms *pParms )
{
	SCODE sc = EmulatedLine( pParms );	// Draw to the backup framebuffer

	if( FAILED(sc) )
		return sc;

	// Now, calculate the dirty-rect to refresh to the actual hardware
	RECT bounds;

	int N_plus_1;			// Minor length of bounding rect + 1

	if( pParms->dN )	// The line has a diagonal component (we'll refresh the bounding rect)
		N_plus_1 = 1 + ( ( pParms->cPels * pParms->dN ) / pParms->dM );
	else
		N_plus_1 = 1;

	switch( pParms->iDir )
	{
		case 0:
			bounds.left = pParms->xStart;
			bounds.top = pParms->yStart;
			bounds.right = pParms->xStart + pParms->cPels + 1;
			bounds.bottom = bounds.top + N_plus_1;
			break;
		case 1:
			bounds.left = pParms->xStart;
			bounds.top = pParms->yStart;
			bounds.bottom = pParms->yStart + pParms->cPels + 1;
			bounds.right = bounds.left + N_plus_1;
			break;
		case 2:
			bounds.right = pParms->xStart + 1;
			bounds.top = pParms->yStart;
			bounds.bottom = pParms->yStart + pParms->cPels + 1;
			bounds.left = bounds.right - N_plus_1;
			break;
		case 3:
			bounds.right = pParms->xStart + 1;
			bounds.top = pParms->yStart;
			bounds.left = pParms->xStart - pParms->cPels;
			bounds.bottom = bounds.top + N_plus_1;
			break;
		case 4:
			bounds.right = pParms->xStart + 1;
			bounds.bottom = pParms->yStart + 1;
			bounds.left = pParms->xStart - pParms->cPels;
			bounds.top = bounds.bottom - N_plus_1;
			break;
		case 5:
			bounds.right = pParms->xStart + 1;
			bounds.bottom = pParms->yStart + 1;
			bounds.top = pParms->yStart - pParms->cPels;
			bounds.left = bounds.right - N_plus_1;
			break;
		case 6:
			bounds.left = pParms->xStart;
			bounds.bottom = pParms->yStart + 1;
			bounds.top = pParms->yStart - pParms->cPels;
			bounds.right = bounds.left + N_plus_1;
			break;
		case 7:
			bounds.left = pParms->xStart;
			bounds.bottom = pParms->yStart + 1;
			bounds.right = pParms->xStart + pParms->cPels + 1;
			bounds.top = bounds.bottom - N_plus_1;
			break;
		default:
			DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid direction: %d\r\n"),pParms->iDir));
			return E_INVALIDARG;
	}

	DispDrvrDirtyRectDump( (LPRECT)&bounds );

	return sc;
}
   
			  
SCODE Wrap2bpp::Line(
	GPELineParms *pLineParms,
	EGPEPhase phase )
{
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Wrap2bpp::Line\r\n")));

	if( phase == gpeSingle || phase == gpePrepare )
	{
		if( ( pLineParms->pDst != m_pPrimarySurface ) || ( DispDrvrPhysicalFrameBuffer != NULL ) )
			pLineParms->pLine = &GPE::EmulatedLine;
		else
			pLineParms->pLine = (SCODE (GPE::*)(struct GPELineParms *))&Wrap2bpp::WrappedEmulatedLine;
	}
	return S_OK;
}

#undef SWAP
#define SWAP(type,a,b) { type tmp; tmp=a; a=b; b=tmp; }

SCODE Wrap2bpp::WrappedEmulatedBlt( GPEBltParms *pParms )
{
	SCODE sc = EmulatedBlt( pParms );	// Draw to the backup framebuffer

	if( FAILED(sc) )
		return sc;

	// Now, calculate the dirty-rect to refresh to the actual hardware
	RECT bounds;

	bounds.left = pParms->prclDst->left;
	bounds.top = pParms->prclDst->top;
	bounds.right = pParms->prclDst->right;
	bounds.bottom = pParms->prclDst->bottom;

	if( bounds.left > bounds.right )
	{
		SWAP( int, bounds.left, bounds.right )
	}
	if( bounds.top > bounds.bottom )
	{
		SWAP( int, bounds.top, bounds.bottom )
	}

	DispDrvrDirtyRectDump( (LPRECT)&bounds );

	return sc;
}


SCODE Wrap2bpp::BltPrepare(
	GPEBltParms *pBltParms )
{
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Wrap2bpp::BltPrepare\r\n")));

	if( ( pBltParms->pDst != m_pPrimarySurface ) || ( DispDrvrPhysicalFrameBuffer != NULL ) )
		pBltParms->pBlt = &GPE::EmulatedBlt;
	else
		pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *))&Wrap2bpp::WrappedEmulatedBlt;

	return S_OK;
}

// This function would be used to undo the setting of clip registers etc
SCODE Wrap2bpp::BltComplete( GPEBltParms *pBltParms )
{
	return S_OK;
}

int Wrap2bpp::InVBlank()
{
	return 0;
}

SCODE Wrap2bpp::SetPalette
(
	const PALETTEENTRY *src,
	unsigned short firstEntry,
	unsigned short numEntries
)
{
	return S_OK;
}

void RegisterDDHALAPI()
{
	;	// No DDHAL support in 2bpp wrapper
}

ulong BitMasks[] = { 0x0001,0x0002,0x0000 };

ULONG *APIENTRY DrvGetMasks(
    DHPDEV     dhpdev)
{
	return BitMasks;
}
