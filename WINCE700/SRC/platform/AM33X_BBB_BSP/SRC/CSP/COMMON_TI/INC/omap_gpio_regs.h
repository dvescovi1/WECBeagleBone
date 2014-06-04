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
//  Header:  omap_gpio_regs.h
//
#ifndef __OMAP_GPIO_REGS_H
#define __OMAP_GPIO_REGS_H

//------------------------------------------------------------------------------

typedef struct {
    REG32 REVISION;                // 0x0000
    REG32 zzzReserved01[3];
    REG32 SYSCONFIG;               // 0x0010
    REG32 SYSSTATUS;               // 0x0014
    REG32 IRQSTATUS1;              // 0x0018
    REG32 IRQENABLE1;              // 0x001C
    REG32 WAKEUPENABLE;            // 0x0020
    REG32 RESERVED_0024;
    REG32 IRQSTATUS2;              // 0x0028
    REG32 IRQENABLE2;              // 0x002C
    REG32 CTRL;                    // 0x0030
    REG32 OE;                      // 0x0034
    REG32 DATAIN;                  // 0x0038
    REG32 DATAOUT;                 // 0x003C
    REG32 LEVELDETECT0;            // 0x0040
    REG32 LEVELDETECT1;            // 0x0044
    REG32 RISINGDETECT;            // 0x0048
    REG32 FALLINGDETECT;           // 0x004C
    REG32 DEBOUNCENABLE;           // 0x0050
    REG32 DEBOUNCINGTIME;          // 0x0054
    REG32 zzzReserved02[2];
    REG32 CLEARIRQENABLE1;         // 0x0060
    REG32 SETIRQENABLE1;           // 0x0064
    REG32 zzzReserved03[2];
    REG32 CLEARIRQENABLE2;         // 0x0070
    REG32 SETIRQENABLE2;           // 0x0074
    REG32 zzzReserved04[2];
    REG32 CLEARWAKEUPENA;          // 0x0080
    REG32 SETWAKEUPENA;            // 0x0084
    REG32 zzzReserved05[2];
    REG32 CLEARDATAOUT;            // 0x0090
    REG32 SETDATAOUT;              // 0x0094
    REG32 zzzReserved06[2];
} OMAP_GPIO_REGS;


//------------------------------------------------------------------------------

#endif // __OMAP_GPIO_REGS_H

