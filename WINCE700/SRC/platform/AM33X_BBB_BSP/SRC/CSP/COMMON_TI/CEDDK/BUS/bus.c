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
#include "omap.h"
#include "omap_bus.h"
#include <devload.h>

//------------------------------------------------------------------------------
//  Local Definitions

#define BUS_ACCESS_COOKIE       'Asub'
#define PCCARD_PCI_CONFIGURATION_SPACE    0  // for cardbus cards

//------------------------------------------------------------------------------

typedef struct {
    DWORD cookie;
    HANDLE hRefBus;             // Parent bus device manager handler
    HANDLE hBus;                // Parent bus I/O handler
    LPWSTR pszDeviceName;       // Device bus name
} PARENT_BUS_ACCESS;

//------------------------------------------------------------------------------

HANDLE CreateBusAccessHandle(LPCTSTR szActivePath)
{
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = NULL;
    HANDLE hDevice;
    DEVMGR_DEVICE_INFORMATION di;
    DWORD length;

    // Get device handle
    hDevice = GetDeviceHandleFromContext(szActivePath);
    if (hDevice == NULL) goto cleanUp;

    // Get device name and parent bus device manager handler
    memset(&di, 0, sizeof(di));
    di.dwSize = sizeof(di);
    if (!GetDeviceInformationByDeviceHandle(hDevice, &di)) goto cleanUp;

    // Allocate internal structure
    pBusAccess = LocalAlloc(LPTR, sizeof(PARENT_BUS_ACCESS));
    if (pBusAccess == NULL) goto cleanUp;

    // Set cookie & hBus
    pBusAccess->cookie = BUS_ACCESS_COOKIE;
    pBusAccess->hBus = INVALID_HANDLE_VALUE;

    // Save parent bus handler
    pBusAccess->hRefBus = di.hParentDevice;

    // Save device bus name
    length = (wcslen(di.szBusName) + 1) * sizeof(WCHAR);
    pBusAccess->pszDeviceName = LocalAlloc(LPTR, length);
    if (pBusAccess->pszDeviceName == NULL) goto cleanUp;
    memcpy(pBusAccess->pszDeviceName, di.szBusName, length);

    // Get parent bus info when there is one
    if (pBusAccess->hRefBus != NULL)
        {
        // Get parent bus device information
        memset(&di, 0, sizeof(di));
        di.dwSize = sizeof(di);
        if (!GetDeviceInformationByDeviceHandle(pBusAccess->hRefBus, &di))
            {
            goto cleanUp;
            }
        // Open bus for later calls
        pBusAccess->hBus = CreateFile(
            di.szBusName, GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
            );
        }        

    // Done
    rc = TRUE;

cleanUp:
    if (!rc && pBusAccess != NULL)
        {
        if (pBusAccess->hBus != INVALID_HANDLE_VALUE)
            {
            CloseHandle(pBusAccess->hBus);
            }            
        if (pBusAccess->pszDeviceName != NULL)
            {
            LocalFree(pBusAccess->pszDeviceName);
            }
        LocalFree(pBusAccess);
        pBusAccess = NULL;
        }
    return pBusAccess;
}

//------------------------------------------------------------------------------

VOID CloseBusAccessHandle(HANDLE hBusAccess)
{
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    if ((pBusAccess == NULL) || (pBusAccess->cookie != BUS_ACCESS_COOKIE))
        {
        SetLastError(ERROR_INVALID_HANDLE);
        goto cleanUp;
        }

    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) CloseHandle(pBusAccess->hBus);
    if (pBusAccess->pszDeviceName != NULL) LocalFree(pBusAccess->pszDeviceName);
    LocalFree(pBusAccess);

cleanUp:
    return;
}

//------------------------------------------------------------------------------

BOOL SetDevicePowerState(
    HANDLE hBusAccess, CEDEVICE_POWER_STATE powerState, VOID *pReserved
) {
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_POWER_STATE ps, psout;
    
    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE)
        {
        SetLastError(ERROR_INVALID_HANDLE);
        goto cleanUp;
        }

    // Call parent device
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE)
        {
        ps.lpDeviceBusName = pBusAccess->pszDeviceName;
        ps.lpceDevicePowerState = &powerState;
        ps.lpReserved = pReserved;

        // Due to problem with public\common\oak\busenum\defbus\defbus.cpp
        // IOCTL_BUS_SET_POWER_STATE should always be called with lpOutBuffer
        // pointing to a CE_BUS_POWER_STATE structure.
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_SET_POWER_STATE, &ps, sizeof(ps),
            &psout, sizeof(psout), NULL, 0
            );
        }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL GetDevicePowerState(
    HANDLE hBusAccess, CEDEVICE_POWER_STATE *pPowerState, VOID *pReserved
) {
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_POWER_STATE ps;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE)
        {
        SetLastError(ERROR_INVALID_HANDLE);
        goto cleanUp;
        }

    // Call parent device
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE)
        {
        ps.lpDeviceBusName = pBusAccess->pszDeviceName;
        ps.lpceDevicePowerState = pPowerState;
        ps.lpReserved = pReserved;
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_GET_POWER_STATE, &ps, sizeof(ps),
            NULL, 0, NULL, 0
            );
        }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL TranslateBusAddr(
    HANDLE hBusAccess, INTERFACE_TYPE interfaceType, ULONG busNumber,
    PHYSICAL_ADDRESS busAddress, ULONG *pAddressSpace,
    PHYSICAL_ADDRESS *pTranslatedAddress
) {
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_TRANSLATE_BUS_ADDR busTranslate;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        // Call parent device bus
        busTranslate.lpDeviceBusName = pBusAccess->pszDeviceName;
        busTranslate.InterfaceType = interfaceType;
        busTranslate.BusNumber = busNumber;
        busTranslate.BusAddress = busAddress;
        busTranslate.AddressSpace = pAddressSpace;
        busTranslate.TranslatedAddress = pTranslatedAddress;
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_TRANSLATE_BUS_ADDRESS, &busTranslate,
            sizeof(busTranslate), NULL, 0, NULL, 0
        );
    } else {
        // Call HAL
        rc = HalTranslateBusAddress(
            interfaceType, busNumber, busAddress, pAddressSpace, 
            pTranslatedAddress
        );
    }
    
clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL TranslateSystemAddr(
    HANDLE hBusAccess, INTERFACE_TYPE interfaceType, ULONG busNumber,
    PHYSICAL_ADDRESS systemAddress, PHYSICAL_ADDRESS *pTranslatedAddress
) {
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_TRANSLATE_SYSTEM_ADDR systemTranslate;


    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        // Call parent device bus
        systemTranslate.lpDeviceBusName = pBusAccess->pszDeviceName;
        systemTranslate.InterfaceType =  interfaceType;
        systemTranslate.BusNumber = busNumber;
        systemTranslate.SystemAddress = systemAddress;
        systemTranslate.TranslatedAddress = pTranslatedAddress;
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_TRANSLATE_SYSTEM_ADDRESS,
            &systemTranslate, sizeof(systemTranslate), NULL, 0, NULL, 0
        );
    }  else {
        // Call HAL
        rc = HalTranslateSystemAddress(
            interfaceType, busNumber, systemAddress, pTranslatedAddress
        );
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

ULONG SetDeviceConfigurationData(
    HANDLE hBusAccess, DWORD space, DWORD busNumber, DWORD slotNumber,
    DWORD offset, DWORD length, VOID *pBuffer
) {
    ULONG rc = 0;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_DEVICE_CONFIGURATION_DATA deviceConfigData;


    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        // Call parent device bus
        deviceConfigData.lpDeviceBusName = pBusAccess->pszDeviceName;
        deviceConfigData.dwSpace = space;
        deviceConfigData.dwOffset = offset;
        deviceConfigData.dwLength = length;
        deviceConfigData.pBuffer = pBuffer;
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_SET_CONFIGURE_DATA, &deviceConfigData,
            sizeof(deviceConfigData), NULL, 0, NULL, 0
        );
    } else if (
        space == PCI_WHICHSPACE_CONFIG || 
        space == PCCARD_PCI_CONFIGURATION_SPACE
    ) {
        // Call HAL for config space
        rc = HalSetBusDataByOffset(
            PCIConfiguration, busNumber, slotNumber, pBuffer, offset, length
        );
    } else {
        SetLastError(ERROR_INVALID_HANDLE);
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

ULONG GetDeviceConfigurationData(
    HANDLE hBusAccess, DWORD space, DWORD busNumber, DWORD slotNumber,
    DWORD offset, DWORD length, VOID *pBuffer
) {
    ULONG rc = 0;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;
    CE_BUS_DEVICE_CONFIGURATION_DATA deviceConfigData;


    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        // Call parent device bus
        deviceConfigData.lpDeviceBusName = pBusAccess->pszDeviceName;
        deviceConfigData.dwSpace = space;
        deviceConfigData.dwOffset = offset;
        deviceConfigData.dwLength = length;
        deviceConfigData.pBuffer = pBuffer;
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_GET_CONFIGURE_DATA, &deviceConfigData,
            sizeof(deviceConfigData), NULL, 0, NULL, 0
        );
    } else if (
        space == PCI_WHICHSPACE_CONFIG || 
        space == PCCARD_PCI_CONFIGURATION_SPACE
    ) {
        // Call HAL for config space
        rc = HalGetBusDataByOffset(
            PCIConfiguration, busNumber, slotNumber, pBuffer, offset, length
        );
    } else {
        SetLastError(ERROR_INVALID_HANDLE);
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL GetParentDeviceInfo(HANDLE hBusAccess, DEVMGR_DEVICE_INFORMATION *pInfo)
{
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    rc = GetDeviceInformationByDeviceHandle(pBusAccess->hRefBus, pInfo);

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL GetChildDeviceRemoveState(HANDLE hBusAccess, DWORD *pChildDeviceState)
{
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    // Call parent device if exists
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_GET_CONFIGURE_DATA, 
            pBusAccess->pszDeviceName, 
            (_tcslen(pBusAccess->pszDeviceName) + 1) * sizeof(TCHAR),
            pChildDeviceState, sizeof(DWORD), NULL, 0
        );
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL GetBusNamePrefix(HANDLE hBusAccess, LPTSTR pOutString, DWORD outSize)
{
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    // Call parent device if exists
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {
        rc = DeviceIoControl(
            pBusAccess->hBus, IOCTL_BUS_NAME_PREFIX, pBusAccess->pszDeviceName, 
            (_tcslen(pBusAccess->pszDeviceName) + 1) * sizeof(TCHAR),
            pOutString, outSize * sizeof(TCHAR), NULL, 0
        );
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL BusTransBusAddrToVirtual(
    HANDLE hBusAccess, INTERFACE_TYPE interfaceType, ULONG busNumber,
    PHYSICAL_ADDRESS busAddress, ULONG length, ULONG *pAddressSpace,
    VOID **ppMappedAddress
) {
    BOOL rc = FALSE;
    PHYSICAL_ADDRESS translatedAddress;

    if (!TranslateBusAddr(
        hBusAccess, interfaceType, busNumber, busAddress, pAddressSpace, 
        &translatedAddress
    )) goto clean;

    if (*pAddressSpace == 0) {
        // Memory-mapped I/O, get virtual address
        *ppMappedAddress = MmMapIoSpace(translatedAddress, length, FALSE);
        rc = (*ppMappedAddress != NULL);
    } else {
        // I/O port
        *ppMappedAddress = (VOID*)translatedAddress.LowPart;
        rc = TRUE;
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL BusTransBusAddrToStatic(
    HANDLE hBusAccess, INTERFACE_TYPE interfaceType, ULONG busNumber,
    PHYSICAL_ADDRESS busAddress, ULONG length, ULONG *pAddressSpace,
    VOID **ppMappedAddress
) {
    BOOL rc = FALSE;
    PHYSICAL_ADDRESS translatedAddress;
    UCHAR *pStaticAddress;
    DWORD alignedAddress, alignedSize;

    if (!TranslateBusAddr(
        hBusAccess, interfaceType, busNumber, busAddress, pAddressSpace, 
        &translatedAddress
    )) goto clean;

    if (*pAddressSpace == 0) {
        alignedAddress = translatedAddress.LowPart & ~(PAGE_SIZE - 1);
        alignedSize = length + (translatedAddress.LowPart & (PAGE_SIZE - 1));
        pStaticAddress = CreateStaticMapping(alignedAddress >> 8, alignedSize);
        if (pStaticAddress == NULL) goto clean;
        pStaticAddress += translatedAddress.LowPart & (PAGE_SIZE - 1);
        *ppMappedAddress = pStaticAddress;
        rc = TRUE;
    } else {
        // I/O port
        *ppMappedAddress = (VOID*)translatedAddress.LowPart;
        rc = TRUE;
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL BusIoControl(
    HANDLE hBusAccess, DWORD code, VOID *pInBuffer, DWORD inSize, 
    VOID *pOutBuffer, DWORD outSize, DWORD *pOutSize, OVERLAPPED *pOverlapped
) {
    BOOL rc = FALSE;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    // Call parent device if exists
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {

        // If there isn't any input buffer, pass device name instead
        if (pInBuffer == NULL && inSize == 0) {
            pInBuffer = pBusAccess->pszDeviceName;
            inSize = (_tcslen(pBusAccess->pszDeviceName) + 1)*sizeof(TCHAR);
        }        

        // Call bus driver
        rc = DeviceIoControl(
            pBusAccess->hBus, code, pInBuffer, inSize, pOutBuffer, outSize, 
            pOutSize, pOverlapped
        );        
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------

BOOL BusChildIoControl(
    HANDLE hBusAccess, DWORD code, LPVOID pBuffer, DWORD size
) {
    BOOL rc = FALSE;
    LPVOID pInBuffer;
    DWORD inSize;
    PARENT_BUS_ACCESS *pBusAccess = (PARENT_BUS_ACCESS *)hBusAccess;

    // Check passed handler
    if (pBusAccess == NULL || pBusAccess->cookie != BUS_ACCESS_COOKIE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto clean;
    }

    // Call parent device if exists
    if (pBusAccess->hBus != INVALID_HANDLE_VALUE) {

        pInBuffer = pBusAccess->pszDeviceName;
        inSize = (_tcslen(pBusAccess->pszDeviceName) + 1)*sizeof(TCHAR);

        // Call bus driver
        rc = DeviceIoControl(
            pBusAccess->hBus, code, pInBuffer, inSize, pBuffer, size, NULL, NULL
        );        
    }

clean:
    return rc;
}

//------------------------------------------------------------------------------
