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
//  File:  tps659xx_gpio.h
//
#ifndef __TPS659XX_GPIO_H
#define __TPS659XX_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Type:  GPIO_SUPP_REGS
//
typedef struct {
    UINT    GPIOPUPDCTR;
    UINT    GPIO_EDR;
} TPS659XX_SUBGRP;

//------------------------------------------------------------------------------
//
//  Type:  TPS659XX_GPIO_DATA_REGS
//
typedef struct {
    UINT            GPIODATAIN;
    UINT            GPIODATADIR;
    UINT            GPIODATAOUT;
    UINT            CLEARGPIODATAOUT;
    UINT            SETGPIODATAOUT;
    UINT            GPIO_DEBEN;
    TPS659XX_SUBGRP  rgSubGroup[2];        
} TPS659XX_GPIO_DATA_REGS;





#ifdef __cplusplus
}
#endif

#endif //__TPS659XX_GPIO_H

