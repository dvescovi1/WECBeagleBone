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
#ifndef __BUS_H
#define __BUS_H

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4100)
#include <defbus.h>
#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------

typedef enum OMAP_DISPLAY_CLOCK_SOURCE {
    OMAP_DISP_FCLOCK_1,
    OMAP_DISP_FCLOCK_2,
    OMAP_DISP_FCLOCK_ALL
} OMAP_DISPLAY_CLOCK_SOURCE;

//------------------------------------------------------------------------------

typedef enum OMAP_POWER {
    OMAP_EXTPOWER_VRMMC1,
    OMAP_EXTPOWER_VRMMC2,
    OMAP_EXTPOWER_VRUSB,
    OMAP_EXTPOWER_VRVUSB,
    OMAP_EXTPOWER_VRI2S,
    OMAP_EXTPOWER_LCD,
    OMAP_EXTPOWER_VRCAM,
    OMAP_EXTPOWER_VDAC,  
    OMAP_POWER_COUNT,
    OMAP_POWER_NONE = -1
} OMAP_POWER;

//------------------------------------------------------------------------------

typedef struct OMAP_BIND_CHILD_INFO{
    DWORD   devId;
    WCHAR   szData[MAX_PATH];
} OMAP_BIND_CHILD_INFO;


/*#if (_WINCEOSVER>600)
//------------------------------------------------------------------------------

typedef struct _CE_BUS_POWER_STATE {
    LPTSTR lpDeviceBusName;
    PCEDEVICE_POWER_STATE lpceDevicePowerState;
    PVOID  lpReserved;
} CE_BUS_POWER_STATE, *PCE_BUS_POWER_STATE;

//------------------------------------------------------------------------------

typedef struct _CE_BUS_TRANSLATE_BUS_ADDR {
    LPTSTR lpDeviceBusName;
    INTERFACE_TYPE  InterfaceType;
    ULONG BusNumber;
    PHYSICAL_ADDRESS BusAddress;
    PULONG AddressSpace;
    PPHYSICAL_ADDRESS TranslatedAddress;
} CE_BUS_TRANSLATE_BUS_ADDR,*PCE_BUS_TRANSLATE_BUS_ADDR;

//------------------------------------------------------------------------------

typedef struct _CE_BUS_TRANSLATE_SYSTEM_ADDR {
    LPTSTR lpDeviceBusName;
    INTERFACE_TYPE  InterfaceType;
    ULONG BusNumber;
    PHYSICAL_ADDRESS SystemAddress;
    PPHYSICAL_ADDRESS TranslatedAddress;
} CE_BUS_TRANSLATE_SYSTEM_ADDR,*PCE_BUS_TRANSLATE_SYSTEM_ADDR;

//------------------------------------------------------------------------------

typedef struct _CE_BUS_DEVICE_CONFIGURATION_DATA {
    LPTSTR lpDeviceBusName;
    DWORD dwSpace;
    DWORD dwOffset;
    DWORD dwLength;
    PVOID pBuffer;
} CE_BUS_DEVICE_CONFIGURATION_DATA,*PCE_BUS_DEVICE_CONFIGURATION_DATA ;

#endif*/

//------------------------------------------------------------------------------

typedef struct _CE_BUS_DEVICE_SOURCE_CLOCKS {
    DWORD   devId;
    UINT    count;
    UINT    rgSourceClocks[MAX_PATH];
} CE_BUS_DEVICE_SOURCE_CLOCKS;

//------------------------------------------------------------------------------

typedef struct _CE_BUS_DEVICESTATE_CALLBACKS {
    DWORD   size;
    void   *PreDeviceStateChange;
    void   *PostDeviceStateChange;
} CE_BUS_DEVICESTATE_CALLBACKS;

//------------------------------------------------------------------------------

#define IOCTL_BUS_REQUEST_CLOCK         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_BUS_RELEASE_CLOCK         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_BUS_USBFN_ATTACH          \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0302, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
#define IOCTL_BUS_CHILD_BIND  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0303, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_BUS_CHILD_UNBIND  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
#define IOCTL_BUS_SOURCE_CLOCKS  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0306, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_BUS_DEVICESTATE_CALLBACKS  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0307, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma warning (push)
// BusIoControl definition is not correct, unused parameters may be NULL/0
#pragma warning (disable: 6309)
#pragma warning (disable: 6387)

//------------------------------------------------------------------------------
//
//  Function:  BusClockRequest
//
__inline
BOOL
BusClockRequest(
    HANDLE hBus, 
    UINT id
    )
{
    // Call bus driver to enable clock
    PREFAST_SUPPRESS(6309 6387, "BusIoControl signature is invalid")
    return BusIoControl(
        hBus, IOCTL_BUS_REQUEST_CLOCK, &id, sizeof(id),
        NULL, 0, NULL, NULL
        );
}

//------------------------------------------------------------------------------
//
//  Function:  BusClkRelease
//
__inline
BOOL
BusClockRelease(
    HANDLE hBus, 
    UINT id
    )
{
    // Call bus driver to disable clock
    PREFAST_SUPPRESS(6309 6387, "BusIoControl signature is invalid")
    return BusIoControl(
        hBus, IOCTL_BUS_RELEASE_CLOCK, &id, sizeof(id),
        NULL, 0, NULL, NULL
        );
}

//------------------------------------------------------------------------------
//
//  Function:  BusSourceClocks
//
__inline
BOOL
BusSourceClocks(
    HANDLE hBus, 
    UINT id,
    UINT count,
    UINT rgSourceClocks[]
    )
{
    // Call bus driver to disable clock
    UINT i;
    CE_BUS_DEVICE_SOURCE_CLOCKS clockInfo;

    count = max(count, MAX_PATH);
    clockInfo.devId = id;
    clockInfo.count = count;
    for (i = 0; i < count; ++i)
        {
        clockInfo.rgSourceClocks[i] = rgSourceClocks[i];
        }
    
    PREFAST_SUPPRESS(6309 6387, "BusSourceClocks signature is invalid")
    return BusIoControl(
        hBus, IOCTL_BUS_SOURCE_CLOCKS, &clockInfo, sizeof(clockInfo),
        NULL, 0, NULL, NULL
        );
}

#pragma warning (pop)

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif


#endif // __BUS_H
