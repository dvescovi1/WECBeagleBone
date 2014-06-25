// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  ti_types.h
//
#ifndef __TI_TYPES_H
#define __TI_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile UINT8  REG8;
typedef volatile UINT16 REG16;
typedef volatile UINT32 REG32;

typedef UINT32  OMAP_DEVICE;
typedef UINT32	OMAP_CLOCK;

typedef enum OMAP_ClockId
{
    SYS_CLK=0,
    MPU_CLK,
    IVA_CLK,
    CORE_CLK,
    SGX_CLK,
    LCD_PCLK
}OMAP_CLOCKID;

#ifdef __cplusplus
}
#endif

#endif

