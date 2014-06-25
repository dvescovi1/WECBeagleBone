
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
//  File:  am387x_dmm.h
//
//

#ifndef __AM387X_DMM_H
#define __AM387X_DMM_H

// AM387X_DMM_REGS
//
typedef volatile struct {
    REG32	DMM_REVISION;			// 0x0000 
    UINT32	zzzReserved01[3];  		// 0x0004-0x000C
    REG32	DMM_SYSCONFIG;			// 0x0010 
    UINT32	zzzReserved02[2];		// 0x0014-0x0018
	REG32	DMM_LISA_LOCK;			// 0x001C
    UINT32	zzzReserved03[8];		// 0x0020-0x003C
	REG32	DMM_LISA_MAP__0;		// 0x0040
	REG32	DMM_LISA_MAP__1;		// 0x0044
	REG32	DMM_LISA_MAP__2;		// 0x0048
	REG32	DMM_LISA_MAP__3;		// 0x004C
	// +++TODO: add more regs, when done, adjust zzzReserved04[] size 
    UINT32	zzzReserved04[260];		// 0x0050-0x045C
    REG32	DMM_PAT_VIEW_MAP_BASE;	// 0x0460
	// +++TODO: add more regs; add padding afterwards 
    // UINT32	zzzReservedX[];		// 0x0464-0x0640

} AM387X_DMM_REGS;



// AM387X_DDR_EMIF_REGS
//
typedef volatile struct {
	UINT32	RES_0000;				// 0x0000
	REG32	SDRAM_STATUS;			// 0x0004 
    REG32	SDRAM_CONFIG;			// 0x0008
	UINT32	RES_000C;				// 0x000C
	REG32	SDRAM_REF_CTRL;			// 0x0010
	REG32	SDRAM_REF_CTRL_SHADOW;	// 0x0014
	REG32	SDRAM_TIM_1;			// 0x0018
	REG32	SDRAM_TIM_1_SHADOW;		// 0x001C
	REG32	SDRAM_TIM_2;			// 0x0020
	REG32	SDRAM_TIM_2_SHADOW;		// 0x0024
	REG32	SDRAM_TIM_3;			// 0x0028
	REG32	SDRAM_TIM_3_SHADOW;		// 0x002C

	//++TODO: add more regs, when done, update RES_0030_00E0 accordingly
	UINT32	RES_0030_00E0[45];		// 

	REG32	DDR_PHY_CTRL_1;			// 0x00E4
	REG32	DDR_PHY_CTRL_1_SHADOW;	// 0x00E8

} AM387X_DDR_EMIF_REGS;


//---------------------------------------------------------------------------------
//
// +++TODO: move DDR defs to a more appropriate header file.

/* Only one the following two options (DDR3/DDR2) should be enabled */
//#define CONFIG_TI816X_EVM_DDR3			/* Configure DDR3  */
#define CONFIG_TI816X_EVM_DDR2				/* Configure DDR2  */
#define CONFIG_TI816X_TWO_EMIF		1


#ifdef CONFIG_TI816X_EVM_DDR2

#define INVERT_CLK_OUT		0x0
#define CMD_SLAVE_RATIO		0x80

/*
 * DDR2 ratio values.  These are board dependent
 * obtained from sweep experiments
 */

/* EVM 400 MHz clock Settings */

#define WR_DQS_RATIO_BYTE_LANE3		((0x4a << 10) | 0x4a)
#define WR_DQS_RATIO_BYTE_LANE2		((0x4a << 10) | 0x4a)
#define WR_DQS_RATIO_BYTE_LANE1		((0x4a << 10) | 0x4a)
#define WR_DQS_RATIO_BYTE_LANE0		((0x4a << 10) | 0x4a)

#define WR_DATA_RATIO_BYTE_LANE3	(((0x4a + 0x40) << 10) | (0x4a + 0x40))
#define WR_DATA_RATIO_BYTE_LANE2	(((0x4a + 0x40) << 10) | (0x4a + 0x40))
#define WR_DATA_RATIO_BYTE_LANE1	(((0x4a + 0x40) << 10) | (0x4a + 0x40))
#define WR_DATA_RATIO_BYTE_LANE0	(((0x4a + 0x40) << 10) | (0x4a + 0x40))

#define RD_DQS_RATIO				((0x40 << 10) | 0x40)

#define DQS_GATE_BYTE_LANE0			((0x13a << 10) | 0x13a)
#define DQS_GATE_BYTE_LANE1			((0x13a << 10) | 0x13a)
#define DQS_GATE_BYTE_LANE2			((0x13a << 10) | 0x13a)
#define DQS_GATE_BYTE_LANE3			((0x13a << 10) | 0x13a)

/*
 * EMIF Paramters
 */
#if 0
#define EMIF_TIM1    0xAAB15E2
#define EMIF_TIM2    0x423631D2
#define EMIF_TIM3    0x80032F
#define EMIF_SDREF   0x10000C30
#define EMIF_SDCFG   0x43801A3A  /* 32 bit ddr2, CL=6, CWL=5, 13 rows, 8 banks, 10 bit column, 2 CS */
#define EMIF_PHYCFG  0x0000030B  /* local odt = 3, read latency = 11 (max = 12, min=6) */
#endif

#endif  /* CONFIG_TI816X_EVM_DDR2 */

//---------------------------------------------------------------------------------

#endif

