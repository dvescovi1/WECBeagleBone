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
/*
================================================================================
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

//------------------------------------------------------------------------------
//
//  File:  am33x_ic.h
//
#ifndef __AM33X_IC_H
#define __AM37X_IC_H

#include <windows.h>


//------------------------------------------------------------------------------
//
// MPU Interrupt Controller
//
typedef /*volatile */struct {

   UINT32 INTC_REVISION;    //offset 0x0, IP revision code
   
   UINT32 zzzReserved01[3];

   UINT32 INTC_SYSCONFIG;   //offset 0x10,OCP i/f params ctrl
   UINT32 INTC_SYSSTATUS;   //offset 0x14,status info of mod

   UINT32 zzzReserved02[10];

   UINT32 INTC_SIR_IRQ;     //offset 0x40,supplies active IRQ intr number
   UINT32 INTC_SIR_FIQ;     //offset 0x44,supplies active FIQ intr number
   UINT32 INTC_CONTROL;     //offset 0x48,global mask and new intr agreement bits
   UINT32 INTC_PROTECTION;  //offset 0x4C,ctrls protection of other registers
   UINT32 INTC_IDLE;        //offset 0x50,func clock auto-idle disable register
   UINT32 zzzReserved03[3];
   
   UINT32 INTC_IRQ_PRIORITY; //offset 0x60, currently active IRQ priority level.
   UINT32 INTC_FIQ_PRIORITY; //offset 0x64, currently active FIQ priority level.
   UINT32 INTC_THRESHOLD;    //offset 0x68, sets the priority threshold.
   UINT32 zzzReserved04[5];

   UINT32 INTC_ITR0;        //offset 0x80,raw intr i/p status before masking
   UINT32 INTC_MIR0;        //offset 0x84,interrupt mask register
   UINT32 INTC_MIR_CLEAR0;  //offset 0x88,clear intr mask bits
   UINT32 INTC_MIR_SET0;    //offset 0x8C,set intr mask bits
   UINT32 INTC_ISR_SET0;    //offset 0x90,set SW intr bits
   UINT32 INTC_ISR_CLEAR0;  //offset 0x94,clear SW intr bits
   UINT32 INTC_PENDING_IRQ0;//offset 0x98,IRQ stat aft masking
   UINT32 INTC_PENDING_FIQ0;//offset 0x9C,FIQ stat aft masking
   UINT32 INTC_ITR1;        //offset 0xA0,raw intr i/p status before masking
   UINT32 INTC_MIR1;        //offset 0xA4,intr mask reg
   UINT32 INTC_MIR_CLEAR1;  //offset 0xA8,clr intr mask bits
   UINT32 INTC_MIR_SET1;    //offset 0xAC,set intr mask bits
   UINT32 INTC_ISR_SET1;    //offset 0xB0,set SW intr bits
   UINT32 INTC_ISR_CLEAR1;  //offset 0xB4,clear SW intr bits
   UINT32 INTC_PENDING_IRQ1;//offset 0xB8,IRQ stat aft masking
   UINT32 INTC_PENDING_FIQ1;//offset 0xBC,FIQ stat aft masking
   UINT32 INTC_ITR2;        //offset 0xC0,raw intr i/p status before masking
   UINT32 INTC_MIR2;        //offset 0xC4,intr mask register
   UINT32 INTC_MIR_CLEAR2;  //offset 0xC8,clr intr mask bits
   UINT32 INTC_MIR_SET2;    //offset 0xCC,set intr mask bits
   UINT32 INTC_ISR_SET2;    //offset 0xD0,set SW intr bits
   UINT32 INTC_ISR_CLEAR2;  //offset 0xD4,clr SW intr bits
   UINT32 INTC_PENDING_IRQ2;//offset 0xD8,IRQ status after masking
   UINT32 INTC_PENDING_FIQ2;//offset 0xDC,FIQ status after masking
   UINT32 INTC_ITR3;        //offset 0xE0,raw intr i/p status before masking
   UINT32 INTC_MIR3;        //offset 0xE4,intr mask register
   UINT32 INTC_MIR_CLEAR3;  //offset 0xE8,clr intr mask bits
   UINT32 INTC_MIR_SET3;    //offset 0xEC,set intr mask bits
   UINT32 INTC_ISR_SET3;    //offset 0xF0,set SW intr bits
   UINT32 INTC_ISR_CLEAR3;  //offset 0xF4,clr SW intr bits
   UINT32 INTC_PENDING_IRQ3;//offset 0xF8,IRQ status after masking
   UINT32 INTC_PENDING_FIQ3;//offset 0xFC,FIQ status after masking
   
   UINT32 INTC_ILR[128];     //offset 0x100, IRQ/FIQ priority 0-96
} AM33X_INTC_MPU_REGS;

//------------------------------------------------------------------------------

#define IC_ILR_IRQ                  (0 << 0)
#define IC_ILR_FIQ                  (1 << 0)

#define IC_ILR_EDGE                 (0 << 1)
#define IC_ILR_LEVEL                (1 << 1)

#define IC_ILR_PRI0                 (0 << 2)
#define IC_ILR_PRI1                 (1 << 2)
#define IC_ILR_PRI2                 (2 << 2)
#define IC_ILR_PRI3                 (3 << 2)
#define IC_ILR_PRI4                 (4 << 2)
#define IC_ILR_PRI5                 (5 << 2)
#define IC_ILR_PRI6                 (6 << 2)
#define IC_ILR_PRI7                 (7 << 2)
#define IC_ILR_PRI8                 (8 << 2)
#define IC_ILR_PRI9                 (9 << 2)
#define IC_ILR_PRI10                (10 << 2)
#define IC_ILR_PRI11                (11 << 2)
#define IC_ILR_PRI12                (12 << 2)
#define IC_ILR_PRI13                (13 << 2)
#define IC_ILR_PRI14                (14 << 2)
#define IC_ILR_PRI15                (15 << 2)
#define IC_ILR_PRI16                (16 << 2)
#define IC_ILR_PRI17                (17 << 2)
#define IC_ILR_PRI18                (18 << 2)
#define IC_ILR_PRI19                (19 << 2)
#define IC_ILR_PRI20                (20 << 2)
#define IC_ILR_PRI21                (21 << 2)
#define IC_ILR_PRI22                (22 << 2)
#define IC_ILR_PRI23                (23 << 2)
#define IC_ILR_PRI24                (24 << 2)
#define IC_ILR_PRI25                (25 << 2)
#define IC_ILR_PRI26                (26 << 2)
#define IC_ILR_PRI27                (27 << 2)
#define IC_ILR_PRI28                (28 << 2)
#define IC_ILR_PRI29                (29 << 2)
#define IC_ILR_PRI30                (30 << 2)
#define IC_ILR_PRI31                (31 << 2)

//------------------------------------------------------------------------------

#define OMAP_MPUIC_MASKALL          (0xFFFFFFFF)

//------------------------------------------------------------------------------

#define IC_CNTL_NEW_IRQ             (1 << 0)
#define IC_CNTL_NEW_FIQ             (1 << 1)

//------------------------------------------------------------------------------

#endif // __AM33X_IC_H

