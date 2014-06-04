// All rights reserved Texas Instruments, Inc 2011

//  This file contains offset addresses for WatchDog registers.

#ifndef __AM33X_WDOG_REGS_H
#define __AM33X_WDOG_REGS_H

//------------------------------------------------------------------------------
//
// Watchdog Timer
//
typedef struct {
	REG32	WIDR;		// 0x00
	UINT32  res_04_0c[3];
	REG32   WDSC;		// 0x10
	REG32	WDST;		// 0x14
	REG32	WISR;		// 0x18
	REG32	WIER;		// 0x1c
	UINT32	res_20;
	REG32	WCLR;		// 0x24
	REG32	WCRR;		// 0x28
	REG32	WLDR;		// 0x2C
	REG32	WTGR;		// 0x30
	REG32	WWPS;		// 0x34
	UINT32	res_38_40[3];
	REG32	WDLY;		// 0x44
	REG32	WSPR;		// 0x48
	UINT32	res_4c_50[2];
	REG32	WIRQSTATRAW;// 0x54
	REG32	WIRQSTAT;	// 0x58
	REG32	WIRQENSET;	// 0x5C
	REG32	WIRQENCLR;	// 0x60
} AM33X_WDOG_REGS;

//------------------------------------------------------------------------------

#define WDOG_DISABLE_SEQ1       0x0000AAAA
#define WDOG_DISABLE_SEQ2       0x00005555

#define WDOG_ENABLE_SEQ1        0x0000BBBB
#define WDOG_ENABLE_SEQ2        0x00004444

#define WDOG_WCLR_PRES_ENABLE   (1<<5)
#define WDOG_WCLR_PRESCALE(x)   ((x)<<2)

//------------------------------------------------------------------------------

#endif //__AM33X_WDOG_REGS_H
