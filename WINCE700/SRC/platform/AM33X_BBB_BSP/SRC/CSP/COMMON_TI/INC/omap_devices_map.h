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
//  File:  omap_devices_map.h
//

#ifndef _OMAP_DEVICES_MAP_H
#define _OMAP_DEVICES_MAP_H

#include "omap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OMAP_DEVICE_NONE        ((OMAP_DEVICE)-1)
#define OMAP_MAX_DEVICE_COUNT   (128)

typedef struct {
    UINT32          deviceAddress;
    OMAP_DEVICE     device;
} DEVICE_ADDRESS_MAP;

typedef struct {
    UINT32          irq;
    OMAP_DEVICE     device;
    const WCHAR*    extra;
} DEVICE_IRQ_MAP;


UINT32  GetAddressByDevice(OMAP_DEVICE dev);
DWORD   GetIrqByDevice(OMAP_DEVICE dev, const WCHAR* extra);
DWORD   GetDeviceByIrq(UINT32 irq);
OMAP_DEVICE GetDeviceByAddress(UINT32 addr);

#ifdef __cplusplus
}
#endif

#endif//_OMAP_DEVICES_MAP_H