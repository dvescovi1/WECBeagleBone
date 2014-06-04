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
//
//  file: interrupt_struct.h
//
#ifndef __INTERRUPT_STRUCT_H
#define __INTERRUPT_STRUCT_H

#include <am33x.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define: AM33X_INTR_CONTEXT
//
typedef struct {
    AM33X_INTC_MPU_REGS *pICLRegs;
    AM3XX_GPIO_REGS  *pGPIORegs[AM33X_GPIO_BANK_COUNT];   
} AM33X_INTR_CONTEXT;

#ifdef __cplusplus
}
#endif

#endif


