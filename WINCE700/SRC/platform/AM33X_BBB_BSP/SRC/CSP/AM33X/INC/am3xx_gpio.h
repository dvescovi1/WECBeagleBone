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
//  Header:  am3xx_gpio.h
//
#ifndef __AM3XX_GPIO_H
#define __AM3XX_GPIO_H

//------------------------------------------------------------------------------

typedef volatile struct {
    UINT32 REVISION;                // 0x0000
    UINT32 zzzReserved01[3];
    UINT32 SYSCONFIG;               // 0x0010
    UINT32 zzzReserved02[3];
    UINT32 EOI;						// 0x0020
    UINT32 IRQSTATUS_RAW_0;         // 0x0024
    UINT32 IRQSTATUS_RAW_1;         // 0x0028
    UINT32 IRQSTATUS_0;             // 0x002C
    UINT32 IRQSTATUS_1;             // 0x0030
    UINT32 IRQSTATUS_SET_0;         // 0x0034
    UINT32 IRQSTATUS_SET_1;         // 0x0038
    UINT32 IRQSTATUS_CLR_0;         // 0x003C
    UINT32 IRQSTATUS_CLR_1;         // 0x0040
    UINT32 WAKEN_0;                 // 0x0044
    UINT32 WAKEN_1;                 // 0x0048
    UINT32 zzzReserved03[50];
    UINT32 SYSSTATUS;		// 0x0114
    UINT32 zzzReserved04[6];   //118-12F
    UINT32 CTRL;				// 0x0130
    UINT32 OE;                      	// 0x0134
    UINT32 DATAIN;                	// 0x0138
    UINT32 DATAOUT;                 // 0x013C
    UINT32 LEVELDETECT0;            // 0x0140
    UINT32 LEVELDETECT1;            // 0x0144
    UINT32 RISINGDETECT;            // 0x0148
    UINT32 FALLINGDETECT;           // 0x014C
    UINT32 DEBOUNCENABLE;           // 0x0150
    UINT32 DEBOUNCINGTIME;          // 0x0154
    UINT32 zzzReserved05[2];
    UINT32 CLEARIRQENABLE1;         // 0x0160
    UINT32 SETIRQENABLE1;           // 0x0164
    UINT32 zzzReserved06[2];
    UINT32 CLEARIRQENABLE2;         // 0x0170
    UINT32 SETIRQENABLE2;           // 0x0174
    UINT32 zzzReserved07[2];
    UINT32 CLEARWAKEUPENA;          // 0x0180
    UINT32 SETWAKEUPENA;            // 0x0184
    UINT32 zzzReserved08[2];
    UINT32 CLEARDATAOUT;            // 0x0190
    UINT32 SETDATAOUT;              // 0x0194
    UINT32 zzzReserved09[2];
} AM3XX_GPIO_REGS;


//------------------------------------------------------------------------------

#endif // __AM3XX_GPIO_H

