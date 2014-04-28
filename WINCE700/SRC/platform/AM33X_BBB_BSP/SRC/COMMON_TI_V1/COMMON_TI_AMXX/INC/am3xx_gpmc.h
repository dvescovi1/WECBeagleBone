/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  am3xx_gpmc.h

#ifndef __AM3XX_GPMC_H
#define __AM3XX_GPMC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
		REG32		GPMC_CONFIG1;	// RW	0x0060
		REG32		GPMC_CONFIG2;	// RW	0x0064
		REG32		GPMC_CONFIG3;	// RW	0x0068
		REG32		GPMC_CONFIG4;	// RW	0x006C
		REG32		GPMC_CONFIG5;	// RW	0x0070
		REG32		GPMC_CONFIG6;	// RW	0x0074
		REG32		GPMC_CONFIG7;	// RW	0x0078
		REG32		GPMC_NAND_COMMAND;// W	0x007C
		REG32		GPMC_NAND_ADDRESS;// W	0x0080
		REG32		GPMC_NAND_DATA;	// RW	0x0084
		UINT32		rez_088_8C[2];
}  AM3XX_GPMC_CS;

// 0x50000000 base address
typedef struct {
	REG32		GPMC_REVISION;	 	// R	0x0000
	UINT32		res_004_00F[3];
	REG32		GPMC_SYSCONFIG;		// RW	0x0010
	REG32		GPMC_SYSSTATUS;		// R	0x0014
	REG32		GPMC_IRQSTATUS;		// RW	0x0018 
	REG32		GPMC_IRQENABLE;		// RW 	0x001C
	UINT32		res_020_03C[8];
	REG32		GPMC_TIMEOUT_CONTROL;//RW 	0x0040
	REG32		GPMC_ERR_ADDRESS;	// RW 	0x0044
	REG32		GPMC_ERR_TYPE;		// RW 	0x0048
	UINT32		res_04C;
	REG32		GPMC_CONFIG;		// RW 	0x0050
	REG32		GPMC_STATUS;		// RW 	0x0054
	UINT32		res_058_05C[2];

    AM3XX_GPMC_CS CS[8];			// 0x060-1DC 0x30*8 ONLY 6 CSs are valid 

	REG32	GPMC_PREFETCH_CONFIG1;	// RW	0x01E0
	REG32	GPMC_PREFETCH_CONFIG2; 	// RW	0x01E4
	UINT32	res_1E8;
	REG32	GPMC_PREFETCH_CONTROL; 	// RW	0x01EC
	REG32	GPMC_PREFETCH_STATUS; 	// RW	0x01F0
	REG32	GPMC_ECC_CONFIG; 		// RW	0x01F4
	REG32	GPMC_ECC_CONTROL; 		// RW	0x01F8
	REG32	GPMC_ECC_SIZE_CONFIG; 	// RW	0x01FC

	REG32	GPMC_ECC_RESULT[9];		// RW 	0x0200-0x220 
	UINT32	res_224_23C[7];
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
} AM3XX_GPMC_REGS;

#ifdef __cplusplus
}
#endif

#endif
