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
//  File:  gpio_ioctls.h
//
//  This header defines interface for GPIO device driver. This driver control
//  GPIO pins on hardware. It allows abstract GPIO interface and break up
//  physicall and logical pins. To avoid overhead involved the driver exposes
//  interface which allows obtain funtion pointers to base set/clr/get etc.
//  functions.
//
#ifndef __GPIO_IOCTLS_H
#define __GPIO_IOCTLS_H

#include "omap.h"
#include "ceddkex.h"

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_GPIO_GUID
//
DEFINE_GUID(
    DEVICE_IFC_GPIO_GUID, 0xa0272611, 0xdea0, 0x4678,
    0xae, 0x62, 0x65, 0x61, 0x5b, 0x7d, 0x53, 0xaa
);

//------------------------------------------------------------------------------
#define GPIO_DEVICE_COOKIE       0x11223344

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

#define IOCTL_GPIO_SETBIT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_CLRBIT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_UPDATEBIT    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0302, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_GETBIT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0303, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_SETMODE      \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_GETMODE      \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0305, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_GETIRQ       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0306, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_SET_POWER_STATE  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0307, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_GET_POWER_STATE  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0308, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_SET_DEBOUNCE_TIME  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0309, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_GET_DEBOUNCE_TIME  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0310, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_INIT_INTERRUPT  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0311, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_ACK_INTERRUPT  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0312, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GPIO_DISABLE_INTERRUPT  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0313, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------

typedef struct {
    UINT                    gpioId;
    CEDEVICE_POWER_STATE    state;
} IOCTL_GPIO_POWER_STATE_IN;

typedef struct {
    CEDEVICE_POWER_STATE    gpioState;
    CEDEVICE_POWER_STATE    bankState;
} IOCTL_GPIO_GET_POWER_STATE_OUT;

typedef struct {
    UINT                    gpioId;
    UINT                    debounceTime;
} IOCTL_GPIO_SET_DEBOUNCE_TIME_IN;

typedef struct {
    UINT                uGpioID;
    DWORD               dwSysIntrID;
} IOCTL_GPIO_INTERRUPT_INFO,  *PIOCTL_GPIO_INTERRUPT_INFO;

typedef struct {
    UINT                uGpioID;
    DWORD               dwSysIntrID;
    HANDLE              hEvent;
} IOCTL_GPIO_INIT_INTERRUPT_INFO,  *PIOCTL_GPIO_INIT_INTERRUPT_INFO;


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
