//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  Header:  am3xx_gptimer.h
//
#ifndef __AM3XX_GPTIMER_H
#define __AM3XX_GPTIMER_H


//------------------------------------------------------------------------------

typedef volatile struct {
    UINT32 TIDR;                // 0000
    UINT32 THWINFO;             // 0004
    UINT32 zzzReserved01[2];
    UINT32 TIOCP;               // 0010
    UINT32 zzzReserved02[3];
	UINT32 IRQ_EOI;				// 0020
	UINT32 IRQSTATUS_RAW;		// 0024
	UINT32 IRQSTATUS;			// 0028
	UINT32 IRQENABLE_SET;		// 002C
	UINT32 IRQENABLE_CLR;		// 0030
	UINT32 IRQWAKEEN;			// 0034
    UINT32 TCLR;                // 0038
    UINT32 TCRR;                // 003C
    UINT32 TLDR;                // 0040
    UINT32 TTGR;                // 0044
    UINT32 TWPS;                // 0048
    UINT32 TMAR;                // 004C
    UINT32 TCAR1;               // 0050
    UINT32 TSICR;               // 0054
    UINT32 TCAR2;               // 0058
} AM3XX_GPTIMER_REGS;


//------------------------------------------------------------------------------

#define GPTIMER_TIOCP_SOFTRESET		(1 << 0)
#define GPTIMER_TIOCP_EMUFREE       (1 << 1)
#define GPTIMER_TIOCP_FORCE_IDLE    (0 << 2)
#define GPTIMER_TIOCP_NO_IDLE       (1 << 2)
#define GPTIMER_TIOCP_SMART_IDLE    (2 << 2)

//------------------------------------------------------------------------------

#define GPTIMER_TIER_CAPTURE        (1 << 2)
#define GPTIMER_TIER_OVERFLOW       (1 << 1)
#define GPTIMER_TIER_MATCH          (1 << 0)

//------------------------------------------------------------------------------

#define GPTIMER_TWER_CAPTURE        (1 << 2)
#define GPTIMER_TWER_OVERFLOW       (1 << 1)
#define GPTIMER_TWER_MATCH          (1 << 0)

//------------------------------------------------------------------------------

#define GPTIMER_TCLR_PT                 (1 << 12)
#define GPTIMER_TCLR_TRG_OVERFLOWMATCH  (2 << 10)
#define GPTIMER_TCLR_TRG_OVERFLOW       (1 << 10)
#define GPTIMER_TCLR_CE                 (1 << 6)
#define GPTIMER_TCLR_PRE                (1 << 5)
#define GPTIMER_TCLR_PTV_DIV_2          (0 << 2)
#define GPTIMER_TCLR_PTV_DIV_4          (1 << 2)
#define GPTIMER_TCLR_PTV_DIV_8          (2 << 2)
#define GPTIMER_TCLR_PTV_DIV_16         (3 << 2)
#define GPTIMER_TCLR_PTV_DIV_32         (4 << 2)
#define GPTIMER_TCLR_PTV_DIV_64         (5 << 2)
#define GPTIMER_TCLR_PTV_DIV_128        (6 << 2)
#define GPTIMER_TCLR_PTV_DIV_256        (7 << 2)
#define GPTIMER_TCLR_AR                 (1 << 1)
#define GPTIMER_TCLR_ST                 (1 << 0)

//------------------------------------------------------------------------------

#define GPTIMER_TWPS_TMAR           (1 << 4)
#define GPTIMER_TWPS_TTGR           (1 << 3)
#define GPTIMER_TWPS_TLDR           (1 << 2)
#define GPTIMER_TWPS_TCRR           (1 << 1)
#define GPTIMER_TWPS_TCLR           (1 << 0)

//------------------------------------------------------------------------------

#define GPTIMER_TSICR_POSTED        (1 << 2)
#define GPTIMER_TSICR_SFT           (1 << 1)

//------------------------------------------------------------------------------

#define GPTIMER_TISR_TCAR           (1 << 2)
#define GPTIMER_TISR_OVF            (1 << 1)
#define GPTIMER_TISR_MAT            (1 << 0)

//------------------------------------------------------------------------------

#define GPTIMER_TLCR_GPO_CFG        (1 << 14)
#define GPTIMER_TLCR_CAPT_MODE      (1 << 13)
#define GPTIMER_TLCR_PT             (1 << 12)
#define GPTIMER_TLCR_SCPWM          (1 << 7)
#define GPTIMER_TLCR_CE             (1 << 6)
#define GPTIMER_TLCR_PRE            (1 << 5)
#define GPTIMER_TLCR_AR             (1 << 1)
#define GPTIMER_TLCR_ST             (1 << 0)

#define GPTIMER_TLCR_TRG_SHIFT      (10)
#define GPTIMER_TLCR_TCM_SHIFT      (8)
#define GPTIMER_TLCR_PTV_SHIFT      (2)

//------------------------------------------------------------------------------

#endif // __AM3XX_GPTIMER_H

