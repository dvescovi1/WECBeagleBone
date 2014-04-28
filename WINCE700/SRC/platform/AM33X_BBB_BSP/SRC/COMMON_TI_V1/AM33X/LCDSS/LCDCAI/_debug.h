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
#define DUMP_DISPC_LCD          1


//-----------------------------------------------------------
//  OMAP Register dump
//
void
Dump_DISPC_LCD(
    LCDC_REGS* pRegs
    );


#else // DEBUG

//-----------------------------------------------------------
//  Debug stubs
//

#define Dump_DISPC_LCD(x)               ((void)0)

#endif // DEBUG

#endif //___DEBUG_H

