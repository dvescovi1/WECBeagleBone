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
//  File:  oal_timer.h
//
#ifndef __OAL_TIMER_H_
#define __OAL_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  typedef: OMAP_TIMER_CONTEXT
//
//  Structure maintaining system timer and tick count values
//

typedef struct {

    UINT32 maxPeriodMSec;               // Maximal timer period in MSec
    UINT32 margin;                      // Margin of time need to reprogram timer interrupt

    volatile UINT64 curCounts;          // Counts at last system tick
    volatile UINT32 base;               // Timer value at last interrupt
    volatile UINT32 match;              // Timer match value for next interrupt
    volatile UINT32 Posted;             // Timer posted mode seletion: 0x1: Posted, 0x0:non-posted mode

} OMAP_TIMER_CONTEXT;

#define TIMER_POSTED_TIMEOUT 1000

//------------------------------------------------------------------------------
// Function prototypes
_inline UINT32 OALTimerGetReg(REG32 *addr);
_inline VOID OALTimerSetReg(REG32 *addr, UINT32 val);
extern VOID OALTimerStart(VOID);
extern VOID OALTimerStop(VOID);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
