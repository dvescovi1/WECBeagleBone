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

Notes: 
--*/
#ifndef __WRAP2BPP_H__
#define __WRAP2BPP_H__

#include <dispdrvr.h>	// Contains interface definition for "old-style" 2bpp drivers

class Wrap2bpp  : public GPE
{
private:

	GPEMode			m_ModeInfo;
	void			*m_pVirtualFrameBuffer;

	SCODE			WrappedEmulatedLine
					(
						GPELineParms *pParms
					);
	SCODE			WrappedEmulatedBlt
					(
						GPEBltParms *pParms
					);
	
public:
					Wrap2bpp();
	virtual int		NumModes();
	virtual SCODE	SetMode
					(
						int modeId,
						HPALETTE *pPalette
					);
	virtual int		InVBlank();
	virtual SCODE	SetPalette
					(
						const PALETTEENTRY *src,
						unsigned short firstEntry,
						unsigned short numEntries
					);
	virtual SCODE	GetModeInfo
					(
						GPEMode *pMode,
						int modeNo
					);
	virtual SCODE	SetPointerShape
					(
						GPESurf *pMask,
						GPESurf *pColorSurf,
						int xHot,
						int yHot,
						int cx,
						int cy
					);
	virtual SCODE	MovePointer
					(
						int x,
						int y
					);
	virtual void	WaitForNotBusy();
	virtual int		IsBusy();
	virtual void	GetPhysicalVideoMemory
					(
						unsigned long *pPhysicalMemoryBase,
						unsigned long *pVideoMemorySize
					);
	virtual SCODE	AllocSurface
					(
						GPESurf **ppSurf,
						int width,
						int height,
						EGPEFormat format,
						int surfaceFlags
					);
	virtual SCODE	Line
					(
						GPELineParms *pLineParms,
						EGPEPhase phase
					);
	virtual SCODE	BltPrepare
					(
						GPEBltParms *pBltParms
					);
	virtual SCODE	BltComplete
					(
						GPEBltParms *pBltParms
					);
};


#endif __WRAP2BPP_H__

