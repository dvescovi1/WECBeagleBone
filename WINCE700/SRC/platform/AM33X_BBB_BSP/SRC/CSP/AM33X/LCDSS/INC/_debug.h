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
//
//  File: _debug.h
//

#ifndef ___DEBUG_H
#define ___DEBUG_H


#ifdef DEBUG

extern  DBGPARAM    dpCurSettings;

//-----------------------------------------------------------
//  GPE Custom debug zones
//
#define GPE_ZONE_VIDEOMEMORY    DEBUGZONE(14)
#define GPE_ZONE_DDRAW_HAL      DEBUGZONE(15)

#define GPE_ZONE_BLT    		DEBUGZONE(3)



//-----------------------------------------------------------
//  DDraw HAL debug dump of structures
//
void
DumpDD_CANCREATESURFACE(
    LPDDHAL_CANCREATESURFACEDATA pd
    );

void
DumpDD_CREATESURFACE(
    LPDDHAL_CREATESURFACEDATA pd
    );

void
DumpDD_FLIP(
    LPDDHAL_FLIPDATA pd
    );

void
DumpDD_LOCK(
    LPDDHAL_LOCKDATA pd
    );

void
DumpDD_UNLOCK(
    LPDDHAL_UNLOCKDATA pd
    );

void
DumpDD_SETCOLORKEY(
    LPDDHAL_SETCOLORKEYDATA pd
    );

void
DumpDD_UPDATEOVERLAY(
    LPDDHAL_UPDATEOVERLAYDATA pd
    );

void
DumpDD_SETOVERLAYPOSITION(
    LPDDHAL_SETOVERLAYPOSITIONDATA pd
    );

void
DumpDDSURFACEDESC(
    LPDDSURFACEDESC lpDDSurfaceDesc
    );

void
DumpDDSCAPS(
    DDSCAPS ddsCaps
    );

void
DumpDDPIXELFORMAT(
    DDPIXELFORMAT ddPixelFormat
    );

void
DumpDDOVERLAYFX(
    DDOVERLAYFX ddOverlayFX
    );

void
DumpDDCOLORKEY(
    DDCOLORKEY ddColorKey
    );


#else // DEBUG

//-----------------------------------------------------------
//  DDraw HAL debug stubs
//

#define DumpDD_CANCREATESURFACE(x)      ((void)0)
#define DumpDD_CREATESURFACE(x)         ((void)0)
#define DumpDD_FLIP(x)                  ((void)0)
#define DumpDD_LOCK(x)                  ((void)0)
#define DumpDD_UNLOCK(x)                ((void)0)
#define DumpDD_SETCOLORKEY(x)           ((void)0)
#define DumpDD_UPDATEOVERLAY(x)         ((void)0)
#define DumpDD_SETOVERLAYPOSITION(x)    ((void)0)
#define DumpDDSURFACEDESC(x)            ((void)0)
#define DumpDDSCAPS(x)                  ((void)0)
#define DumpDDPIXELFORMAT(x)            ((void)0)
#define DumpDDOVERLAYFX(x)              ((void)0)
#define DumpDDCOLORKEY(x)               ((void)0)

#endif // DEBUG
#endif //___DEBUG_H

