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
//  file: interrupt_struct.h
//
#ifndef __INTERRUPT_STRUCT_H
#define __INTERRUPT_STRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DWORD           irq_start;
    DWORD           irq_end;
    UINT32          bank_irq;
    OMAP_GPIO_REGS* pRegs;
    OMAP_DEVICE     device;
	struct {
		DWORD IRQENABLE1;
		DWORD WAKEUPENABLE;
	} restoreCtxt;
    UINT32          padWakeupEvent;
} INTR_GPIO_CTXT;
//------------------------------------------------------------------------------
//
//  Define: OMAP_INTR_CONTEXT
//
typedef struct {
    OMAP_INTC_MPU_REGS  *pICLRegs;
    DWORD               nbGpioBank;
    INTR_GPIO_CTXT      *pGpioCtxt;
} OMAP_INTR_CONTEXT;

#ifdef __cplusplus
}
#endif

#endif


