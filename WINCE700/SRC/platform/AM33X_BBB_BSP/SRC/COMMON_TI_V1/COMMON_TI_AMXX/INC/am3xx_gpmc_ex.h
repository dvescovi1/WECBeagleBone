/*
===============================================================================
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  am3xx_gpmc_ex.h

#ifndef __AM3XX_GPMC_EX_H
#define __AM3XX_GPMC_EX_H

#ifdef __cplusplus
extern "C" {
#endif

// this is extention to the omap_gpmc_regs 
typedef struct {
	struct {
		REG32	GPMC_BCH_RESULT0;	// RW	0x0240
		REG32	GPMC_BCH_RESULT1;	// RW 	0x0244
		REG32	GPMC_BCH_RESULT2;	// RW 	0x0248
		REG32	GPMC_BCH_RESULT3;	// RW 	0x024C
	} BCH1[8];						// 0x240-0x2BC  ONLY 6 CSs are valid
	UINT32	rez_2C0_2CC[2];
	REG32	GPMC_BCH_SWDATA; 		// RW 0x02D0;  
	struct {
		REG32	GPMC_BCH_RESULT4;	// RW 	0x0300
		REG32	GPMC_BCH_RESULT5;	// RW 	0x0304
		REG32	GPMC_BCH_RESULT6;	// RW 	0x0308
		UINT32	res;
	} BCH2[8];						// 0x300-0x37C  ONLY 6 CSs are valid
} AM3XX_GPMC_EX_REGS;

#ifdef __cplusplus
}
#endif

#endif
