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

#pragma once
#include <BusDriver.hxx>
#include <string.hxx>
#include <list.hxx>

//------------------------------------------------------------------------------

class DefaultBusChildContainer_t;

//------------------------------------------------------------------------------
//
//  Class:  DefaultBus_t
//
class DefaultBus_t : public BusDriver_t
{

protected:
    // This bus interface type
    INTERFACE_TYPE m_busType;
    // This bus number
    DWORD m_busNumber;
    // This bus device handle
    HANDLE m_hDevice;
    // Parent bus handle
    HANDLE m_hParent;
    // This bus registry device key
    ce::wstring m_deviceKey;
    // This bus device name
    ce::wstring m_deviceName;
    // Bus prefix to be used for child device name creation
    ce::wstring m_busPrefix;
    // Open contexts container
    ce::list<DriverContext_t*>* m_pContextContainer;
    // Child devices container
    DefaultBusChildContainer_t* m_pChildContainer;    
    //bootphase info
    DWORD m_bootphase;

protected:

    // Constructor    
    DefaultBus_t(
        );

    // Destructor
    virtual ~DefaultBus_t(
        );

public:

    //----------------------------------------------------------------------
    // Driver_t interface, see Driver.hxx for comments

    virtual 
    BOOL
    PostInit(
        );
    
    virtual
    BOOL
    Init(
        LPCWSTR context, 
        LPCVOID pBusContext
        );

    virtual
    BOOL
    Deinit(
        );

    virtual
    VOID
    PreDeinit(
        );

    virtual
    VOID
    PowerUp(
        );

    virtual
    VOID
    PowerDown(
        );

    virtual
    DriverContext_t*
    Open(
        DWORD accessCode, DWORD shareMode
        );

    virtual
    BOOL
    Close(
        DriverContext_t* pContext
        );

    //----------------------------------------------------------------------
    // BusDriver_t interface, see BusDriver.hxx for comments

    virtual
    BusDriverChild_t* 
    CreateBusDeviceChild(
        LPCWSTR templateKeyName
        );
    
    virtual
    BOOL
    IoCtlPostInit(
        );

    virtual
    DWORD
    IoCtlBusNamePrefix(
        LPWSTR pBuffer,
        DWORD size
        );

    virtual
    BusDriverChild_t*
    FindChildByName(
        LPCWSTR childName
        );

    virtual
    VOID
    GetBusInfo(
        HANDLE& hBus,
        INTERFACE_TYPE& busType,
        DWORD& busNumber
        );

    //----------------------------------------------------------------------
    // CreateDefaultBusChildContainer is manufacturer function which
    // will produce container used to store child devices. We use
    // manufacturer approach to allow container implementation
    // replacement in busses derived from DefaultBus_t.
    
    virtual
    DefaultBusChildContainer_t* 
    CreateDefaultBusChildContainer(
        );

    //----------------------------------------------------------------------
    // Following method are specific for default bus driver. In most
    // cases they correspond to BusDriverChild_t methods. 
    // DefaultBusChild_t redirect calls to those methods as in case
    // of default buffer implementation isn't child specific.
    //

    virtual
    BOOL
    IoCtlTranslateBusAddress(
        INTERFACE_TYPE interfaceType,
        ULONG busNumber,
        PHYSICAL_ADDRESS busAddress,
        ULONG *pAddressSpace,
        PHYSICAL_ADDRESS *pTranslatedAddress
        );

    virtual
    BOOL
    IoCtlTranslateSystemAddress(
        INTERFACE_TYPE  interfaceType,
        ULONG busNumber,
        PHYSICAL_ADDRESS systemAddress,
        PHYSICAL_ADDRESS *pTranslatedAddress
        );

    virtual
    BOOL
    IoCtlGetPowerState(
        LPCWSTR childName,
        CEDEVICE_POWER_STATE *pPowerState
        );

    virtual
    BOOL
    IoCtlSetPowerState(
        LPCWSTR childName,
        CEDEVICE_POWER_STATE *pPowerState
        );
    
    virtual
    BOOL
    IoCtlGetConfigurationData(
        DWORD space,
        DWORD slotNumber,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        );
    
    virtual
    BOOL
    IoCtlSetConfigurationData(
        DWORD space,
        DWORD slotNumber,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        );

    
    DWORD 
    GetBootphase(){return m_bootphase;};

    //----------------------------------------------------------------------

};

//------------------------------------------------------------------------------

