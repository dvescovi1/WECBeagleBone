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
//  File:  omap35xx_led.h
//
#ifndef __OMAP35XX_LED_H
#define __OMAP35XX_LED_H

//------------------------------------------------------------------------------
//
//  Define:  LED_IDX_xxx
//
//  Following constants are used to specify OALLED indexes for different
//  information displayed on debug LED drivers.
//
#ifndef SHIP_BUILD

#define LED_IDX_GP0             0x00      // used for suspend resume diagnostics (CM_IDLEST_CKGEN)
#define LED_IDX_GP1             0x01      // used for suspend resume diagnostics (CM_FCLKEN1_CORE | CM_FCLKEN2_CORE)
#define LED_IDX_GP2             0x02
#define LED_IDX_GP3             0x03
#define LED_IDX_GP4             0x04
#define LED_IDX_GP5             0x05
#define LED_IDX_GP6             0x06
#define LED_IDX_GP7             0x07
#define LED_IDX_GP8             0x08
#define LED_IDX_GP9             0x09
#define LED_IDX_GP10            0x0A
#define LED_IDX_GP11            0x0B
#define LED_IDX_GP12            0x0C
#define LED_IDX_GP13            0x0D
#define LED_IDX_GP14            0x0E
#define LED_IDX_GP15            0x0F
#define LED_IDX_GP16            0x10      
#define LED_IDX_GP17            0x11      
#define LED_IDX_GP18            0x12
#define LED_IDX_GP19            0x13
#define LED_IDX_GP20            0x14
#define LED_IDX_GP21            0x15
#define LED_IDX_GP22            0x16
#define LED_IDX_GP23            0x17
#define LED_IDX_GP24            0x18
#define LED_IDX_GP25            0x19
#define LED_IDX_GP26            0x1A
#define LED_IDX_GP27            0x1B
#define LED_IDX_GP28            0x1C
#define LED_IDX_GP29            0x1D
#define LED_IDX_GP30            0x1E
#define LED_IDX_GP31            0x1F


#define LED_IDX_PLLS            0x00
#define LED_IDX_FCLKS           0x01

#define LED_IDX_CORE_CAM_USB_SGX_PREV_STATE     0x02
#define LED_IDX_MPU_PER_NEON_DSS_PREV_STATE     0x03

#define LED_IDX_DOMAIN_MASK     0x04

#define LED_IDX_CORE_VDD        0x0E
#define LED_IDX_MPU_VDD         0x0F

#define LED_IDX_PRE_CHIPSTATE   0x16
#define LED_IDX_CHIPSTATE       0x17

#define LED_IDX_MPU_PREV_STATE  0x18 
#define LED_IDX_PER_PREV_STATE  0x19
#define LED_IDX_CORE_PREV_STATE 0x1A
#define LED_IDX_CHIP_PREV_STATE 0x1B

#define LED_IDX_PROFILER_TICK   0x1C
#define LED_IDX_PROFILER_INC    0x1D

#define LED_IDX_IDLE            0x1E
#define LED_IDX_TIMER           0x1F



#endif // SHIP_BUILD

//------------------------------------------------------------------------------

#endif // __OMAP35XX_LED_H
