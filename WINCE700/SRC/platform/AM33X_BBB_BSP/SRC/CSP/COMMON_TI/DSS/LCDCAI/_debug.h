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

#ifndef ZONE_ERROR
#define ZONE_ERROR 1
#endif


#ifdef DEBUG

//-----------------------------------------------------------
//  Invididual control of DSS register dumps
//
#define DUMP_DSS                1
#define DUMP_DISPC              1
#define DUMP_DISPC_LCD          1
#define DUMP_DISPC_GFX          1
#define DUMP_DISPC_VID          1


//-----------------------------------------------------------
//  OMAP Register dump
//
void
Dump_DSS(
    OMAP_DSS_REGS* pRegs
    );

void
Dump_DISPC(
    OMAP_DISPC_REGS* pRegs
    );

void
Dump_DISPC_Control(
    OMAP_DISPC_REGS* pRegs
    );

void
Dump_DISPC_LCD(
    OMAP_DISPC_REGS* pRegs
    );

void
Dump_DISPC_GFX(
    OMAP_DISPC_REGS* pRegs
    );
    
void
Dump_DISPC_VID(
    OMAP_VID_REGS* pRegs,
    UINT32* pVidCoeffVRegs,
    DWORD x
    );

#else // DEBUG

//-----------------------------------------------------------
//  Debug stubs
//

#define Dump_DSS(x)                     ((void)0)
#define Dump_DISPC(x)                   ((void)0)
#define Dump_DISPC_Control(x)           ((void)0)
#define Dump_DISPC_LCD(x)               ((void)0)
#define Dump_DISPC_GFX(x)               ((void)0)
#define Dump_DISPC_VID(x, y, z)         ((void)0)

#endif // DEBUG

#if 0
//-----------------------------------------------------------
//  OMAP Register dump
//
void
Retail_Dump_DSS(
    OMAP_DSS_REGS* pRegs
    );

void
Retail_Dump_DISPC(
    OMAP_DISPC_REGS* pRegs
    );
#endif

#endif //___DEBUG_H

