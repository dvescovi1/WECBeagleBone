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
//  File:  gpio.h
//
//  This header defines interface for GPIO device driver. This driver control
//  GPIO pins on hardware. It allows abstract GPIO interface and break up
//  physicall and logical pins. To avoid overhead involved the driver exposes
//  interface which allows obtain funtion pointers to base set/clr/get etc.
//  functions.
//
#ifndef __GPIO_H
#define __GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define:  GPIO_DEVICE_NAME
//
#define GPIO_DEVICE_NAME        L"GIO1:"

//------------------------------------------------------------------------------
//
//  Define:  GPIO_DIR_xxx/GPIO_INT_xxx
//
#define GPIO_DIR_OUTPUT         (0 << 0)
#define GPIO_DIR_INPUT          (1 << 0)
#define GPIO_INT_LOW_HIGH       (1 << 1)
#define GPIO_INT_HIGH_LOW       (1 << 2)
#define GPIO_INT_LOW            (1 << 3)
#define GPIO_INT_HIGH           (1 << 4)
#define GPIO_DEBOUNCE_ENABLE    (1 << 5)

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_GPIO_GUID
//
DEFINE_GUID(
    DEVICE_IFC_GPIO_GUID, 0xa0272611, 0xdea0, 0x4678,
    0xae, 0x62, 0x65, 0x61, 0x5b, 0x7d, 0x53, 0xaa
);

//------------------------------------------------------------------------------
// Enum : Gpio Numbers
//
typedef enum {
    //GPIOBank0
    GPIO_0 = 0,
    GPIO_1,
    GPIO_2,
    GPIO_3,
    GPIO_4,
    GPIO_5,
    GPIO_6,
    GPIO_7,
    GPIO_8,
    GPIO_9,
    GPIO_10,
    GPIO_11,
    GPIO_12,
    GPIO_13,
    GPIO_14,
    GPIO_15,
    GPIO_16,
    GPIO_17,
    GPIO_18,
    GPIO_19,
    GPIO_20,
    GPIO_21,
    GPIO_22,
    GPIO_23,
    GPIO_24,
    GPIO_25,
    GPIO_26,
    GPIO_27,
    GPIO_28,
    GPIO_29,
    GPIO_30,
    GPIO_31,

    //GPIOBank1
    GPIO_32,
    GPIO_33,
    GPIO_34,
    GPIO_35,
    GPIO_36,
    GPIO_37,
    GPIO_38,
    GPIO_39,
    GPIO_40,
    GPIO_41,
    GPIO_42,
    GPIO_43,
    GPIO_44,
    GPIO_45,
    GPIO_46,
    GPIO_47,
    GPIO_48,
    GPIO_49,
    GPIO_50,
    GPIO_51,
    GPIO_52,
    GPIO_53,
    GPIO_54,
    GPIO_55,
    GPIO_56,
    GPIO_57,
    GPIO_58,
    GPIO_59,
    GPIO_60,
    GPIO_61,
    GPIO_62,
    GPIO_63,

    //GPIOBank2
    GPIO_64,
    GPIO_65,
    GPIO_66,
    GPIO_67,
    GPIO_68,
    GPIO_69,
    GPIO_70,
    GPIO_71,
    GPIO_72,
    GPIO_73,
    GPIO_74,
    GPIO_75,
    GPIO_76,
    GPIO_77,
    GPIO_78,
    GPIO_79,
    GPIO_80,
    GPIO_81,
    GPIO_82,
    GPIO_83,
    GPIO_84,
    GPIO_85,
    GPIO_86,
    GPIO_87,
    GPIO_88,
    GPIO_89,
    GPIO_90,
    GPIO_91,
    GPIO_92,
    GPIO_93,
    GPIO_94,
    GPIO_95,
	
    //GPIOBank3
    GPIO_96,
    GPIO_97,
    GPIO_98,
    GPIO_99,
    GPIO_100,
    GPIO_101,
    GPIO_102,
    GPIO_103,
    GPIO_104,
    GPIO_105,
    GPIO_106,
    GPIO_107,
    GPIO_108,
    GPIO_109,
    GPIO_110,
    GPIO_111,
    GPIO_112,
    GPIO_113,
    GPIO_114,
    GPIO_115,
    GPIO_116,
    GPIO_117,
    GPIO_118,
    GPIO_119,
    GPIO_120,
    GPIO_121,
    GPIO_122,
    GPIO_123,
    GPIO_124,
    GPIO_125,
    GPIO_126,
    GPIO_127,

	GPIO_MAX_NUM
} GpioNum_e;

#include "sdk_gpio.h"
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
