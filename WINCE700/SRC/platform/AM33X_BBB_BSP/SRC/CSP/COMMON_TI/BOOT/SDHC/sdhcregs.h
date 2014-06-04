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
//  File: sdhcregs.h
//
//
//

#ifndef _SDHCREGS_DEFINED
#define _SDHCREGS_DEFINED


/* MMCHS_CMD */
/*Response lengths*/
#define MMCHS_RSP_MASK              (0x00030000)

#define MMCHS_RSP_LEN48             (0x00020000)
#define MMCHS_RSP_LEN48B            (0x00030000)
#define MMCHS_RSP_LEN136            (0x00010000)
#define MMCHS_RSP_NONE              (0x00000000)

/*command type*/
#define MMCHS_CMD_NORMAL            (0x00000000)
#define MMCHS_CMD_SUSP              (0x1 << 22)
#define MMCHS_FUNC_SEL              (0x2 << 22)
#define MMCHS_CMD_ABORT             (0x3 << 22)

#define MMCHS_CMD_READ              (0x00000010) /*read, card to host*/
#define MMCHS_CMD_WRITE             (0x00000000) /*write, host to card*/

#define MMC_CTO_CONTROL_MAX         0x00FD
#define MMC_CTO_CONTROL_DEFAULT     0x0005      // 5 * 1024 = 5K cycles
#define MMC_DTO_CONTROL_MAX         0xFFFD
#define MMC_DTO_CONTROL_DEFAULT     0x0200      // 512 * 1024 = 512K cycles

#define STD_HC_MAX_CLOCK_FREQUENCY  48000000    // 48 MHz

#define MMCSD_CLOCK_INPUT           (96 * 1000 * 1000)
#define MMCSD_CLOCK_INIT            (400 * 1000)

#define SDMMC_DEFAULT_BLOCK_LEN     512
#define SDMMC_DEFAULT_NUM_BLOCKS    1

#define STD_HC_MAX_SLOTS            2

#define STD_HC_MIN_BLOCK_LENGTH     1
#define STD_HC_MAX_BLOCK_LENGTH     2048

/////
#define SOFT_RESET_ALL              MMCHS_SYSCTL_SRA

// DMA input channel.
#define SDIO_INPUT_DMA_SOURCE1      (OMAP_MMCHS1_REGS_PA + 0x120)
#define SDIO_INPUT_DMA_SOURCE2      (OMAP_MMCHS2_REGS_PA + 0x120)

// DMA output channel.
#define SDIO_OUTPUT_DMA_DEST1       (OMAP_MMCHS1_REGS_PA + 0x120)
#define SDIO_OUTPUT_DMA_DEST2       (OMAP_MMCHS2_REGS_PA + 0x120)

#endif // _SDHCREGS_DEFINED


