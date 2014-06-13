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
//  File: SDHC.H
//
//  SDHC driver definitions
//

#ifndef _SDHC_DEFINED
#define _SDHC_DEFINED

#include "omap.h"
#pragma warning(push)
#pragma warning(disable: 4127 4214 4201)
#include <SDCardDDK.h>
#pragma warning(pop)

#define MIN_MMC_BLOCK_SIZE 4
#define SD_IO_BUS_CONTROL_BUS_WIDTH_MASK 0x03

#define SD_REMOVE_TIMEOUT 200

#define SD_TIMEOUT 0
#define SD_INSERT 1
#define SD_REMOVE 2

#define SHC_CARD_CONTROLLER_PRIORITY    0x97
#define SHC_CARD_DETECT_PRIORITY        0x98

#define WAKEUP_SDIO                     1
#define WAKEUP_CARD_INSERT_REMOVAL      2

#define MMCSLOT_1                       1
#define MMCSLOT_2                       2

#define TIMERTHREAD_TIMEOUT_NONSDIO     1
#define TIMERTHREAD_TIMEOUT             2000
#define TIMERTHREAD_PRIORITY            252

#endif // _SDHC_DEFINED


