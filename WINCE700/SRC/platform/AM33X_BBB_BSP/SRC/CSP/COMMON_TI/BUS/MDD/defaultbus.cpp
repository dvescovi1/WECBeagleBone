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
#include <cebuscfg.h>
#include <registry.hxx>
#include <DefaultBus.h>
#include <DefaultBusContext.h>
#include <DefaultBusChild.h>
#include <DefaultBusChildContainer.h>

#define    PCCARD_PCI_CONFIGURATION_SPACE    0  // for cardbus cards

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::CreateBusDeviceChild
//
BusDriverChild_t*
DefaultBus_t::
CreateBusDeviceChild(
    LPCWSTR templateKeyName
    )
{
    UNREFERENCED_PARAMETER(templateKeyName);
    return new DefaultBusChild_t(this);
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::CreateBusDeviceChildContainer
//
DefaultBusChildContainer_t*
DefaultBus_t::
CreateDefaultBusChildContainer(
    )
{
    return new DefaultBusChildContainer_t(this);
}

//------------------------------------------------------------------------------
//
// Constructor    
//
DefaultBus_t::
DefaultBus_t(
    ) :
    BusDriver_t(),
    m_busType(InterfaceTypeUndefined),
    m_busNumber(0),
    m_hDevice(NULL),
    m_hParent(NULL),
    m_pChildContainer(NULL)
{
}

//------------------------------------------------------------------------------
//
// Destructor
//
DefaultBus_t::
~DefaultBus_t(
    )
{
}

//------------------------------------------------------------------------------
//
// Post initialization
//
BOOL
DefaultBus_t::
PostInit(
        )
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Init
//
//  Called from XXX_Init
//
BOOL
DefaultBus_t::
Init(
    LPCWSTR context,
    LPCVOID pBusContext
    )
{
    BOOL rc = FALSE;
    DEVMGR_DEVICE_INFORMATION info;
    HKEY hKey = NULL;

    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"DefaultBus_t::"
        L"+Init('%s', 0x%08x)\r\n", context, pBusContext
        ));
    
    // Get this bus handle 
    m_hDevice = GetDeviceHandleFromContext(context);
    if (m_hDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"Failed get device handle from context\r\n"
            ));
        goto cleanUp;
        }

    // Get information about myself
    info.dwSize = sizeof(info);
    if (!GetDeviceInformationByDeviceHandle(m_hDevice, &info))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"Failed get device information by handle\r\n"
            ));
        goto cleanUp;
        }

    // Print little info
    DEBUGMSG(ZONE_INFO, (L"DefaultBus_t::Init: "
        L"Parent bus handle: 0x%08x\r\n", info.hParentDevice
        ));
    DEBUGMSG(ZONE_INFO, (L"DefaultBus_t::Init: "
        L"Legacy name: '%s'\r\n", info.szLegacyName
        ));
    DEBUGMSG(ZONE_INFO, (L"DefaultBus_t::Init: "
        L"Device registry key: '%s'r\n", info.szDeviceKey
        ));
    DEBUGMSG(ZONE_INFO, (L"DefaultBus_t::Init: "
        L"Device name: '%s'r\n", info.szDeviceName
        ));
    DEBUGMSG(ZONE_INFO, (L"DefaultBus_t::Init: "
        L"Bus name: '%s'r\n", info.szBusName
        ));

    // Store info for later use
    m_deviceKey = info.szDeviceKey;
    m_deviceName = info.szBusName;

    // Could read and store DEVLOAD_REPARMS_VALNAME...
    // To implement this we should add function to obtain information from BusChildDriver_t.

    // Open device registry key
    hKey = OpenDeviceKey(context);
    if (hKey == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"Failed open device key from context\r\n"
            ));
        goto cleanUp;
        }

    // Read "InterfaceType" from registry
    ce::RegQueryValue(
        hKey, DEVLOAD_INTERFACETYPE_VALNAME, 
        reinterpret_cast<DWORD*>(&m_busType), (DWORD)InterfaceTypeUndefined
        );

    // Read "BusNumber" from registry
    ce::RegQueryValue(hKey, DEVLOAD_BUSNUMBER_VALNAME, m_busNumber, 0);

    // We don't need open registry key anymore
    RegCloseKey(hKey);
    hKey = NULL;

    switch (m_busType)
        {
        case Internal:
            if (m_deviceName == L"$bus\\BuiltInPhase1")
                {
                m_busPrefix = L"InternalPhase1_";
                }
            else if (m_deviceName == L"$bus\\BuiltIn")
                {
                m_busPrefix = L"InternalPhase2_";
                }
            else
                {
                m_busPrefix = L"Internal_";
                }
            break;
        case PCIBus:
            m_busPrefix = L"Pci_";
            break;
        default:
            m_busPrefix = L"Other_";
            break;
        }

    {
        m_bootphase = 1;
        HANDLE hevBootPhase2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("SYSTEM/BootPhase2"));
        if (hevBootPhase2) 
        {
            if( WaitForSingleObject(hevBootPhase2, 0) == WAIT_OBJECT_0) 
            { // If Phase2 Signaled.
                m_bootphase = 2;
            }            
        }
        CloseHandle(hevBootPhase2);
    }

    if (PostInit() == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"post init failed\r\n"
            ));
        goto cleanUp;
        }

    // Create context container
    m_pContextContainer = new ce::list<DriverContext_t*>;

    // Create child container
    m_pChildContainer = CreateDefaultBusChildContainer();
    if (m_pChildContainer == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"Failed create child container object\r\n"
            ));
        goto cleanUp;
        }

    // Let container initialize itself (find all childs)
    rc = m_pChildContainer->Init(m_deviceKey);
    if (!rc)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERRROR: DefaultBus_t::Init: "
            L"Failed initialize child container object\r\n"
            ));
        }
    
cleanUp:
    if (hKey != NULL) RegCloseKey(hKey);
    DEBUGMSG(ZONE_FUNCTION, (L"DefaultBus_t::"
        L"-Init(%d)\r\n", rc
        ));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::PreDeinit
//
VOID
DefaultBus_t::
PreDeinit(
    )
{
    // Deactivate all children in reverse order
    while (!m_pChildContainer->empty())
        {
        // Get child on last position
        BusDriverChild_t* pChild = m_pChildContainer->back();
        // Deactivate it
        pChild->IoCtlDeactivateChild();
        // Remove it from container
        m_pChildContainer->pop_back();
        // And finally decrement reference count
        pChild->Release();
        }    
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Deinit
//
BOOL
DefaultBus_t::
Deinit(
    )
{
    // Could close and deallocate all open contexts...
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::PowerUp
//
VOID
DefaultBus_t::
PowerUp(
    )
{
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::PowerDown
//
VOID
DefaultBus_t::
PowerDown(
    )
{
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Open
//
DriverContext_t*
DefaultBus_t::
Open(
    DWORD accessCode, DWORD shareMode
    )
{
    DriverContext_t *pContext = NULL;

    UNREFERENCED_PARAMETER(shareMode);
    
    // Check access code
    if ((accessCode & DEVACCESS_BUSNAMESPACE) == 0)
        {
        DEBUGMSG(ZONE_WARN, (L"WARN: DefaultBus_t::Open: "
            L"Unsufficient access code to open bus driver\r\n"
            ));
        goto cleanUp;
        }

    // Create new context object
    pContext = new DefaultBusContext_t(this);

    // Add open context to container
    m_pContextContainer->push_back(pContext);

    // There is reference in container
    pContext->AddRef();

cleanUp:
    return pContext;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Close
//
BOOL
DefaultBus_t::
Close(
    DriverContext_t* pContext
    )
{
    // Remove context from list and release reference
    m_pContextContainer->remove(pContext);
    pContext->Release();

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlPostInit
//
BOOL
DefaultBus_t::
IoCtlPostInit(
    )
{
    int size;
    DefaultBusChildContainer_t::iterator ppChild, ppChildEnd;

    // Iterate over vector (ordered by priority) and
    // activate bus child devices
    size = m_pChildContainer->size();
    ppChild = m_pChildContainer->begin();
    ppChildEnd = m_pChildContainer->end();
    while (ppChild != ppChildEnd && size--)
        {
        BusDriverChild_t* pChild = *ppChild;
        pChild->IoCtlActivateChild();
        ppChild++;
        }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Deinit
//
//  Called from XXX_IOControl/IOCTL_BUS_NAME_PREFIX
//
DWORD
DefaultBus_t::
IoCtlBusNamePrefix(
    LPWSTR pBuffer,
    DWORD size
    )
{
    DWORD count = size / sizeof(WCHAR);

    // Account for final L'\0'
    if (count > 0) count--;
    // Get number of character     
    if (m_busPrefix.length() < count) count = m_busPrefix.length();
    // Copy & add final L'\0'
    memcpy(pBuffer, m_busPrefix.get_buffer(), count * sizeof(WCHAR));
    pBuffer[count] = L'\0';
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::FindChildByName
//
BusDriverChild_t*
DefaultBus_t::
FindChildByName(
    LPCWSTR childName
    )
{
    // Forward call to container
    return m_pChildContainer->FindChildByName(childName);
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::GetBusInfo
//
VOID
DefaultBus_t::
GetBusInfo(
    HANDLE& hBus,
    INTERFACE_TYPE& busType,
    DWORD& busNumber
    )
{
    // Simply copy internal values
    hBus = m_hDevice;
    busType = m_busType;
    busNumber = m_busNumber;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlTranslateBusAddress
//
BOOL
DefaultBus_t::
IoCtlTranslateBusAddress(
    INTERFACE_TYPE interfaceType,
    ULONG busNumber,
    PHYSICAL_ADDRESS busAddress,
    ULONG *pAddressSpace,
    PHYSICAL_ADDRESS *pTranslatedAddress
    )
{
    BOOL rc = FALSE;

    if (m_hParent != NULL)
        {
        // There is parent bus ask him to do work
        rc = ::TranslateBusAddr(
            m_hParent, interfaceType, busNumber, busAddress,
            pAddressSpace, pTranslatedAddress
            );
        }
    else
        {
        // Call CEDDK when we are orphan
        rc = ::HalTranslateBusAddress(
            interfaceType, busNumber, busAddress, pAddressSpace,
            pTranslatedAddress
            );
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlTranslateSystemAddress
//
BOOL
DefaultBus_t::
IoCtlTranslateSystemAddress(
    INTERFACE_TYPE  interfaceType,
    ULONG busNumber,
    PHYSICAL_ADDRESS systemAddress,
    PHYSICAL_ADDRESS *pTranslatedAddress
    )
{
    BOOL rc = FALSE;

    if (m_hParent != NULL)
        {
        // There is parent bus ask him to do work
        rc = ::TranslateSystemAddr(
            m_hParent, interfaceType, busNumber, systemAddress,
            pTranslatedAddress
            );
        }
    else
        {
        // Call CEDDK when we are orphan
        rc = ::HalTranslateSystemAddress(
            interfaceType, busNumber, systemAddress, pTranslatedAddress
            );
        }
    return rc;
}


//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlGetPowerState
//
BOOL
DefaultBus_t::
IoCtlGetPowerState(
    LPCWSTR childName,
    CEDEVICE_POWER_STATE *pPowerState
    )
{
    BOOL rc = FALSE;

    if (m_hParent != NULL)
        {
        // There is parent bus ask him to do work
        // There is problem with API, code uses reserved parameter to pass child name....
        rc = ::GetDevicePowerState(
            m_hParent, pPowerState, const_cast<LPWSTR>(childName)
            );
        }
    return rc;
}


//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlSetPowerState
//
BOOL
DefaultBus_t::
IoCtlSetPowerState(
    LPCWSTR childName,
    CEDEVICE_POWER_STATE *pPowerState
    )
{
    BOOL rc = FALSE;

    if (m_hParent != NULL)
        {
        // There is parent bus ask him to do work
        // There is problem with API, code uses reserved parameter to pass child name....
        rc = ::SetDevicePowerState(
            m_hParent, *pPowerState, const_cast<LPWSTR>(childName)
            );
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlGetConfigurationData
//
BOOL
DefaultBus_t::
IoCtlGetConfigurationData(
    DWORD space,
    DWORD slotNumber,
    DWORD offset,
    DWORD length,
    VOID  *pBuffer
    )
{
    BOOL rc = FALSE;

    if (m_hParent != 0)
        {
        // There is parent bus, ask it to do the work.
        // Note: Clear it, we are loosing information there...
        rc = ::GetDeviceConfigurationData(
            m_hParent, space, m_busNumber, slotNumber, offset, length,
            pBuffer
            );
        }
    else if ((space == PCI_WHICHSPACE_CONFIG) ||
             (space == PCCARD_PCI_CONFIGURATION_SPACE))
        {
        // Call CEDDK when we are orphan
        rc = ::HalSetBusDataByOffset(
            PCIConfiguration, m_busNumber, slotNumber, pBuffer, offset, length
            );
        }

    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::IoCtlSetConfigurationData
//
BOOL
DefaultBus_t::
IoCtlSetConfigurationData(
    DWORD space,
    DWORD slotNumber,
    DWORD offset,
    DWORD length,
    VOID  *pBuffer
    )
{
    BOOL rc = FALSE;

    if (m_hParent != 0)
        {
        // There is parent bus, ask it to do the work
        // Note: Clear it, we are loosing information there...
        rc = ::GetDeviceConfigurationData(
            m_hParent, space, m_busNumber, slotNumber, offset, length,
            pBuffer
            );
        }
    else if ((space == PCI_WHICHSPACE_CONFIG) ||
             (space == PCCARD_PCI_CONFIGURATION_SPACE))
        {
        // Call CEDDK when we are orphan
        rc = ::HalGetBusDataByOffset(
            PCIConfiguration, m_busNumber, slotNumber, pBuffer, offset, length
            );
        }

    return rc;
}

//------------------------------------------------------------------------------

