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
#include <windows.h>
#include <devload.h>
#include <devloadex.h>
#include <registry.hxx>
#include <DefaultBusChild.h>

//------------------------------------------------------------------------------
//
//  Constructor:  DefaultBusChild_t
//
DefaultBusChild_t::
DefaultBusChild_t(
    DefaultBus_t *pBus
    ) : 
    m_pBus(pBus), 
    m_hDevice(NULL),
    m_flags(0),
    m_driverId((DWORD)-1),
    m_ffPower(POWERFLAGS_CONTEXTRESTORE) // UNDONE : Context Restore notification enabled by 
                                         // default to support display driver Context Restore
{
    // We made copy of pointer
    m_pBus->AddRef();
}

//------------------------------------------------------------------------------
//
//  Constructor:  DefaultBusChild_t
//
DefaultBusChild_t::
~DefaultBusChild_t(
    )
{
    // We need to release reference
    m_pBus->Release();
}

//----------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::Init
//
BOOL
DefaultBusChild_t::
Init(
    DWORD index, 
    LPCWSTR deviceRegistryKey
    )
{
    DWORD rc = FALSE;
    LONG code;
    HKEY hKey = NULL;
    HANDLE hBus;
    INTERFACE_TYPE busType;
    DWORD busNumber;
    ce::wstring name;


    // Save device registry key for later use
    m_deviceRegistryKey = deviceRegistryKey;

    // Open device registry key
    code = ::RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, deviceRegistryKey, 0, 0, &hKey
        );
    if (code != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    // Read load flags
    ce::RegQueryValue(hKey, DEVLOAD_FLAGS_VALNAME, m_flags, DEVFLAGS_NONE);

    // Read power mask
    ce::RegQueryValue(hKey, DEVLOADEX_POWERFLAGS_VALNAME, m_ffPower, POWERFLAGS_NONE);

    // Read device id
    ce::RegQueryValue(hKey, DEVICEID_VALNAME, m_driverId, (DWORD)-1);

    // Try read obsolete "Entry" value
    code = ce::RegQueryValue(hKey, DEVLOAD_ENTRYPOINT_VALNAME, m_entryPoint);
    if (code == ERROR_SUCCESS)
        {
        // We find "Entry" so we must get DLL name also
        code = ce::RegQueryValue(hKey, DEVLOAD_DLLNAME_VALNAME, m_dllName);
        if (code != ERROR_SUCCESS)
            {
            goto cleanUp;
            }
        }
    
    // Get bus prefix
    name.reserve(MAX_PATH);
    if (m_pBus->IoCtlBusNamePrefix(name.get_buffer(), name.capacity()) == 0)
        {
        goto cleanUp;
        }
    // Start device name with bus prefix
    m_deviceName = name;

    // Now create second part of device name
    m_pBus->GetBusInfo(hBus, busType, busNumber);
    StringCchPrintf(
        name.get_buffer(), name.capacity()/sizeof(WCHAR), L"%d_%d_0",
        busNumber, index
        );

    // Attach it to device name
    m_deviceName += name;

    // Done
    rc = TRUE;
    
cleanUp:    
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//------------------------------------------------------------------------------

LPCWSTR        
DefaultBusChild_t::
DeviceName(
    )
{
    return m_deviceName;
}


//------------------------------------------------------------------------------
//
//  Method:  DeviceId
//
DWORD 
DefaultBusChild_t::
DriverId(
    )
{
    return m_driverId;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlTranslateBusAddress
//
BOOL
DefaultBusChild_t::
IoCtlTranslateBusAddress(
    INTERFACE_TYPE interfaceType,
    ULONG busNumber,
    PHYSICAL_ADDRESS busAddress,
    ULONG *pAddressSpace,
    PHYSICAL_ADDRESS *pTranslatedAddress
    )
{
    // Forward request on bus level for default driver
    return m_pBus->IoCtlTranslateBusAddress(
        interfaceType, busNumber, busAddress, pAddressSpace, pTranslatedAddress
        );
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlTranslateSystemAddress
//
BOOL
DefaultBusChild_t::
IoCtlTranslateSystemAddress(
    INTERFACE_TYPE  interfaceType,
    ULONG busNumber,
    PHYSICAL_ADDRESS systemAddress,
    PHYSICAL_ADDRESS *pTranslatedAddress
    )
{
    // Forward request on bus level for default driver
    return m_pBus->IoCtlTranslateSystemAddress(
        interfaceType, busNumber, systemAddress, pTranslatedAddress
        );
}
    
//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlGetPowerState
//
BOOL
DefaultBusChild_t::
IoCtlGetPowerState(
    CEDEVICE_POWER_STATE *pPowerState
    )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(pPowerState);
    return rc;
}
    
//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlSetPowerState
//
BOOL
DefaultBusChild_t::
IoCtlSetPowerState(
    CEDEVICE_POWER_STATE *pPowerState
    )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(pPowerState);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlGetConfigurationData
//
BOOL
DefaultBusChild_t::
IoCtlGetConfigurationData(
    DWORD space,
    DWORD offset,
    DWORD length,
    VOID  *pBuffer
    )
{
    // Forward request on bus level for default driver
    // Should we do it? We don't pass info about child...
    return m_pBus->IoCtlGetConfigurationData(
        space, 0, offset, length, pBuffer
        );
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlSetConfigurationData
//
BOOL
DefaultBusChild_t::
IoCtlSetConfigurationData(
    DWORD space,
    DWORD offset,
    DWORD length,
    VOID  *pBuffer
    )
{
    // Forward request on bus level for default driver
    // Should we do it? We don't pass info about child...
    return m_pBus->IoCtlSetConfigurationData(
        space, 0, offset, length, pBuffer
        );
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlActivateChild
//
BOOL
DefaultBusChild_t::
IoCtlActivateChild(
    )
{
    BOOL rc = FALSE;
    HANDLE hBus;
    INTERFACE_TYPE busType;
    DWORD busNumber;
    REGINI regini[3];
    CEDEVICE_POWER_STATE powerState;


    // If flag is set to avoid load, simply exit
    if ((m_flags & DEVFLAGS_NOLOAD) != 0)
        {
        return TRUE;
        }

    
    if (((m_flags & DEVFLAGS_BOOTPHASE_1)!=0) && (m_pBus->GetBootphase()!=1))
        return TRUE;

    // Get bus information
    m_pBus->GetBusInfo(hBus, busType, busNumber);
    
    // Prepare registry entries needed by new bus model
    regini[0].lpszVal = DEVLOAD_BUSPARENT_VALNAME;
    regini[0].dwType  = DEVLOAD_BUSPARENT_VALTYPE;
    regini[0].pData   = reinterpret_cast<UCHAR*>(&hBus);
    regini[0].dwLen   = sizeof(hBus);
    
    regini[1].lpszVal = DEVLOAD_INTERFACETYPE_VALNAME;
    regini[1].dwType  = DEVLOAD_INTERFACETYPE_VALTYPE;
    regini[1].pData   = reinterpret_cast<UCHAR*>(&busType);
    regini[1].dwLen   = sizeof(busType);

    regini[2].lpszVal = DEVLOAD_BUSNAME_VALNAME;
    regini[2].dwType  = DEVLOAD_BUSNAME_VALTYPE;
    regini[2].pData   = (UCHAR*)((LPCWSTR)m_deviceName);
    regini[2].dwLen   = (m_deviceName.length() + 1) * sizeof(WCHAR);

    // Hardware should be set to D0 before driver is loaded
    powerState = D0;
    IoCtlSetPowerState(&powerState);
    
    // Based on "Entry" registry value existence use new or old loading scheme
    if (m_entryPoint.empty())
        {
        // Use new device activation schema
        m_hDevice = ActivateDeviceEx(m_deviceRegistryKey, regini, 3, NULL);
        rc = (m_hDevice != NULL);
        }
    else
        {
        HMODULE hDevice;
        
        if ((m_flags & DEVFLAGS_LOADLIBRARY) != 0)
            {
            hDevice = ::LoadLibrary(m_dllName);
            }
        else
            {
            hDevice = ::LoadDriver(m_dllName);
            }
        if (hDevice == NULL)
            {
            goto cleanUp;
            }
        // Get entry function
        PFN_DEV_ENTRY pfnEntry = reinterpret_cast<PFN_DEV_ENTRY>(
            ::GetProcAddress(hDevice, m_entryPoint)
            );
        if (pfnEntry == NULL)
            {
            ::FreeLibrary(reinterpret_cast<HMODULE>(hDevice));
            goto cleanUp;
            }
        // Finally call it
        rc = pfnEntry(const_cast<LPWSTR>((LPCWSTR)m_deviceRegistryKey));
        // Save handle for later use
        m_hDevice = hDevice;
        }

cleanUp:    
    if (m_hDevice == NULL)
        {
        // If activation failed, set hardware back to D4
        powerState = D4;
        IoCtlSetPowerState(&powerState);
        }
    return (m_hDevice != NULL);
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlDeactivateChild
//
BOOL
DefaultBusChild_t::
IoCtlDeactivateChild(
    )
{
    BOOL rc = FALSE;
    CEDEVICE_POWER_STATE powerState;

    if (m_hDevice == NULL)
        {
        goto cleanUp;
        }

    // Depending on load scheme used
    if (m_entryPoint.empty())
        {
        // Ask device manager to unload child driver
        rc = ::DeactivateDevice(m_hDevice);
        }
    else
        {
        // In theory we can do this...
        rc = ::FreeLibrary(reinterpret_cast<HMODULE>(m_hDevice));
        }

    // Make sure that hardware is switched off
    powerState = D4;
    IoCtlSetPowerState(&powerState);
    
    // Even if unload fail, there isn't better way how to manage situation...
    m_hDevice = NULL;

cleanUp:    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlIsChildRemoved
//
BOOL
DefaultBusChild_t::
IoCtlIsChildRemoved(
    )
{
    // Child device can't be removed...
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::IoCtlIsChildRemoved
//
BOOL
DefaultBusChild_t::
IoControl(
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);

    return FALSE;
}

//------------------------------------------------------------------------------

