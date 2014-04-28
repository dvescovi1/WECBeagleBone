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
//  File:  am33x_gpio.h
//
//  This header defines interface for GPIO device driver. This driver control
//  GPIO pins on hardware. It allows abstract GPIO interface and break up
//  physicall and logical pins. To avoid overhead involved the driver exposes
//  interface which allows obtain funtion pointers to base set/clr/get etc.
//  functions.
//
#ifndef __AM33X_GPIO_H
#define __AM33X_GPIO_H

//------------------------------------------------------------------------------
// Enum : Gpio Numbers
//
typedef enum {
    //GPIOBank0
    GPIO0_0 = 0,
    GPIO0_1,
    GPIO0_2,
    GPIO0_3,
    GPIO0_4,
    GPIO0_5,
    GPIO0_6,
    GPIO0_7,
    GPIO0_8,
    GPIO0_9,
    GPIO0_10,
    GPIO0_11,
    GPIO0_12,
    GPIO0_13,
    GPIO0_14,
    GPIO0_15,
    GPIO0_16,
    GPIO0_17,
    GPIO0_18,
    GPIO0_19,
    GPIO0_20,
    GPIO0_21,
    GPIO0_22,
    GPIO0_23,
    GPIO0_24,
    GPIO0_25,
    GPIO0_26,
    GPIO0_27,
    GPIO0_28,
    GPIO0_29,
    GPIO0_30,
    GPIO0_31,

    //GPIOBank1
    GPIO1_0,
    GPIO1_1,
    GPIO1_2,
    GPIO1_3,
    GPIO1_4,
    GPIO1_5,
    GPIO1_6,
    GPIO1_7,
    GPIO1_8,
    GPIO1_9,
    GPIO1_10,
    GPIO1_11,
    GPIO1_12,
    GPIO1_13,
    GPIO1_14,
    GPIO1_15,
    GPIO1_16,
    GPIO1_17,
    GPIO1_18,
    GPIO1_19,
    GPIO1_20,
    GPIO1_21,
    GPIO1_22,
    GPIO1_23,
    GPIO1_24,
    GPIO1_25,
    GPIO1_26,
    GPIO1_27,
    GPIO1_28,
    GPIO1_29,
    GPIO1_30,
    GPIO1_31,

    //GPIOBank2
    GPIO2_0,
    GPIO2_1,
    GPIO2_2,
    GPIO2_3,
    GPIO2_4,
    GPIO2_5,
    GPIO2_6,
    GPIO2_7,
    GPIO2_8,
    GPIO2_9,
    GPIO2_10,
    GPIO2_11,
    GPIO2_12,
    GPIO2_13,
    GPIO2_14,
    GPIO2_15,
    GPIO2_16,
    GPIO2_17,
    GPIO2_18,
    GPIO2_19,
    GPIO2_20,
    GPIO2_21,
    GPIO2_22,
    GPIO2_23,
    GPIO2_24,
    GPIO2_25,
    GPIO2_26,
    GPIO2_27,
    GPIO2_28,
    GPIO2_29,
    GPIO2_30,
    GPIO2_31,
	
    //GPIOBank3
    GPIO3_0,
    GPIO3_1,
    GPIO3_2,
    GPIO3_3,
    GPIO3_4,
    GPIO3_5,
    GPIO3_6,
    GPIO3_7,
    GPIO3_8,
    GPIO3_9,
    GPIO3_10,
    GPIO3_11,
    GPIO3_12,
    GPIO3_13,
    GPIO3_14,
    GPIO3_15,
    GPIO3_16,
    GPIO3_17,
    GPIO3_18,
    GPIO3_19,
    GPIO3_20,
    GPIO3_21,
    GPIO3_22,
    GPIO3_23,
    GPIO3_24,
    GPIO3_25,
    GPIO3_26,
    GPIO3_27,
    GPIO3_28,
    GPIO3_29,
    GPIO3_30,
    GPIO3_31
} am33x_GpioNum_e;


#endif
