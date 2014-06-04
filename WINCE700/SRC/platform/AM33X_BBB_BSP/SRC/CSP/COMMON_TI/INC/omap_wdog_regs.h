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

//------------------------------------------------------------------------------
//
//  File:  omap_wdog_regs.h
//
//  This file contains offset addresses for WatchDog registers.

#ifndef __OMAP_WDOG_REGS_H
#define __OMAP_WDOG_REGS_H

//------------------------------------------------------------------------------
//
// Watchdog Timer
//
typedef struct {

    REG32 WIDR;              // offset 0x0000, WIDR
    REG32 zzzReserved01[3];
    
    REG32 WD_SYSCONFIG;      // offset 0x0010, WD_SYSCONFIG
    REG32 WD_SYSSTATUS;      // offset 0x0014, WD_SYSSTATUS
    REG32 WISR;              // offset 0x0018, WISR
    REG32 WIER;              // offset 0x001C, WIER
    REG32 zzzReserved02[1];
    
    REG32 WCLR;              // offset 0x0024, WCLR
    REG32 WCRR;              // offset 0x0028, WCRR
    REG32 WLDR;              // offset 0x002C, WLDR
    REG32 WTGR;              // offset 0x0030, WTGR
    REG32 WWPS;              // offset 0x0034, WWPS   
    REG32 zzzReserved03[4];
    
    REG32 WSPR;              // offset 0x0048, WSPR   
    
} OMAP_WDOG_REGS;

//------------------------------------------------------------------------------

#define WDOG_DISABLE_SEQ1       0x0000AAAA
#define WDOG_DISABLE_SEQ2       0x00005555

#define WDOG_ENABLE_SEQ1        0x0000BBBB
#define WDOG_ENABLE_SEQ2        0x00004444

#define WDOG_WCLR_PRES_ENABLE   (1<<5)
#define WDOG_WCLR_PRESCALE(x)   ((x)<<2)

//------------------------------------------------------------------------------

#endif //__OMAP_WDOG_REGS_H
