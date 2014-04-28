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

#include "am33x_base_regs.h"
#include "am33x_sdhc_regs.h"

/* MMCHS_CMD */
/*Response lengths*/
#define MMCHS_RSP_MASK            (0x00030000)

#define MMCHS_RSP_LEN48           (0x00020000)
#define MMCHS_RSP_LEN48B          (0x00030000)
#define MMCHS_RSP_LEN136          (0x00010000)
#define MMCHS_RSP_NONE            (0x00000000)

/*command type*/
#define MMCHS_CMD_NORMAL          (0x00000000)
#define MMCHS_CMD_SUSP            (0x1 << 22)
#define MMCHS_FUNC_SEL            (0x2 << 22)
#define MMCHS_CMD_ABORT           (0x3 << 22)

#define MMCHS_CMD_READ            (0x00000010) /*read, card to host*/
#define MMCHS_CMD_WRITE           (0x00000000) /*write, host to card*/

#define STD_HC_MAX_CLOCK_FREQUENCY     48000000    // 48 MHz

#define MMCSD_CLOCK_INPUT           (96 * 1000 * 1000)
#define MMCSD_CLOCK_INIT            (400 * 1000)

#define SDMMC_DEFAULT_BLOCK_LEN         512
#define SDMMC_DEFAULT_NUM_BLOCKS        1

#define STD_HC_MIN_BLOCK_LENGTH         1
#define STD_HC_MAX_BLOCK_LENGTH         2048

#define SOFT_RESET_ALL                  MMCHS_SYSCTL_SRA

// DMA input channel.
#define SDIO_INPUT_DMA_SOURCE1          (AM33X_MMCHS0_REGS_PA + 0x220)    // Data register
#define SDIO_INPUT_DMA_SOURCE2          (AM33X_MMCHS1_REGS_PA + 0x220)
#define SDIO_INPUT_DMA_SOURCE3          (AM33X_MMCHS2_REGS_PA + 0x220)

// DMA output channel.
#define SDIO_OUTPUT_DMA_DEST1           (AM33X_MMCHS0_REGS_PA + 0x220)    // Data register
#define SDIO_OUTPUT_DMA_DEST2           (AM33X_MMCHS1_REGS_PA + 0x220)
#define SDIO_OUTPUT_DMA_DEST3           (AM33X_MMCHS2_REGS_PA + 0x220)


#endif // _SDHCREGS_DEFINED


