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
//  File:  devicemap.c
//
// helper fonction to access the device list
//
#include "omap.h"
#include "omap_devices_map.h"

extern const DEVICE_ADDRESS_MAP s_DeviceAddressMap[];
extern const DEVICE_IRQ_MAP s_DeviceIrqMap[];

UINT32 GetAddressByDevice(OMAP_DEVICE dev)
{
    UINT32 Addr = 0;
    DWORD i=0;
    while (s_DeviceAddressMap[i].device != OMAP_DEVICE_NONE)
    {   
        if (s_DeviceAddressMap[i].device == dev)
        {
            Addr = s_DeviceAddressMap[i].deviceAddress;
            break;
        }
        i++;
    }
    return Addr;
}

OMAP_DEVICE GetDeviceByAddress(UINT32 addr)
{
    OMAP_DEVICE dev = 0;
    DWORD i=0;
    while (s_DeviceAddressMap[i].device != OMAP_DEVICE_NONE)
    {   
        if (s_DeviceAddressMap[i].deviceAddress == addr)
        {
            dev = s_DeviceAddressMap[i].device;
            break;
        }
        i++;
    }
    return dev;
}


DWORD   GetIrqByDevice(OMAP_DEVICE dev, const WCHAR* extra)
{
    UINT32 irq = (UINT32) -1;
    DWORD i=0;
    while (s_DeviceIrqMap[i].device != OMAP_DEVICE_NONE)
    {   
        if (s_DeviceIrqMap[i].device == dev)
        {
            BOOL bFound = TRUE;

            //If we extra information, then we will check that the extra information match
            if (s_DeviceIrqMap[i].extra)                 
            {
                if ((extra==NULL) || wcscmp(extra,s_DeviceIrqMap[i].extra))
                {
                    bFound = FALSE;
                }
            }

            if (bFound)
            {
                irq = s_DeviceIrqMap[i].irq;
                break;
            }
            
        }
        i++;
    }
    return irq;
}

DWORD   GetDeviceByIrq(UINT32 irq)
{
	DWORD devID = OMAP_DEVICE_NONE;
    DWORD i=0;

	while (s_DeviceIrqMap[i].irq != 0)
    {   
        if (s_DeviceIrqMap[i].irq == irq)
        {
			devID = s_DeviceIrqMap[i].device;
        }
        i++;
    }

    return devID;
}