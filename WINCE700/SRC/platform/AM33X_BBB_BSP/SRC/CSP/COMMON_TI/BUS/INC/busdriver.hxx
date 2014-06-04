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
#include <driver.hxx>
#include <ceddk.h>

//------------------------------------------------------------------------------
// Debug macros

#ifndef SHIP_BUILD
#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#endif

//------------------------------------------------------------------------------
// Forward declaration

class BusDriver_t;
class BusDriverContext_t;
class BusDriverChild_t;

//------------------------------------------------------------------------------
//
//  Class:  BusDriver_t
//

class BusDriver_t : public Driver_t
{
public:

    //----------------------------------------------------------------------
    // Following CreateBusDeviceChild function is manufacturer
    // method for BusDriverChild_t objects. The deviceRegistryKey
    // parameter allows to create object based on different classes
    // based on registry settings. Note there is similar manufacturer
    // class CreateDeviceContext inherited from Driver_t.

    virtual
    BusDriverChild_t* 
    CreateBusDeviceChild(
        LPCWSTR deviceRegistryKey
        ) = 0;
    
    //----------------------------------------------------------------------
    // Handlers for IOCTL_BUS_xxx codes which should be solved
    // on bus driver level.

    // Called from XXX_IOControl/IOCTL_BUS_POSTINIT
    virtual
    BOOL
    IoCtlPostInit(
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_NAME_PREFIX
    virtual
    DWORD
    IoCtlBusNamePrefix(
        LPWSTR pBuffer,
        DWORD size
        ) = 0;

    //----------------------------------------------------------------------
    // FindChildByName function finds child object based on child name.
    // In most cases this call will be redirected to 
    // BusDriverChildContainer_t::FindChildByName.
    
    virtual
    BusDriverChild_t*
    FindChildByName(
        LPCWSTR childName
        ) = 0;

    //----------------------------------------------------------------------
    // GetBusInfo function returns information about bus driver. Function
    // returns only information required by device manager when child
    // device is activated for new driver model. Note that busType and
    // busNumber unambiguously identify bus in system.
    
    virtual
    VOID
    GetBusInfo(
        HANDLE& hBus,
        INTERFACE_TYPE& busType,
        DWORD& busNumber
        ) = 0;
    
};

//------------------------------------------------------------------------------
//
//  Class:  BusDriverContext_t
//
//  There isn't BusDriverContext_t extension to common DriverContext_t class,
//  but using BusDriverConext_t will make situation little more consistent.
//
class BusDriverContext_t : public DriverContext_t
{
};

//------------------------------------------------------------------------------
//
//  Class:  BusDriverChild_t
//
class BusDriverChild_t : public RefCount_t
{
    //----------------------------------------------------------------------
    // We want to left BusDriverChild_t constructor protected, so
    // manufacturer function must be our friend

    friend BusDriverChild_t* BusDriver_t::CreateBusDeviceChild(
        LPCWSTR deviceRegistryKey
        );

public:

    //----------------------------------------------------------------------
    // Child object initialization. It is called to initialize child
    // object based on template passed as parameter. Paramer index
    // specify index of child driver on bus. It can be used to create
    // device bus name.

    virtual
    BOOL
    Init(
        DWORD index, 
        LPCWSTR deviceRegistryKey 
        ) = 0;

    //----------------------------------------------------------------------
    // This function simply returns driverId.  The id is a custom value
    // which may be used to uniquely associate context information
    // with the driver
    
    virtual
    DWORD        
    DriverId(
        ) = 0;

    //----------------------------------------------------------------------
    // This function simply returns device name. As we return pointer
    // to internal structure in object, function also increase
    // reference count. Caller must decrement reference count, when
    // he don't need pointer returned by this function anymore.
    
    virtual
    LPCWSTR        
    DeviceName(
        ) = 0;

    //----------------------------------------------------------------------
    // Handlers for IOCTL_BUS_xxx codes which should be solved
    // on bus child object level.

    // Called from XXX_IOControl/IOCTL_BUS_TRANSLATE_BUS_ADDRESS
    virtual
    BOOL
    IoCtlTranslateBusAddress(
        INTERFACE_TYPE interfaceType,
        ULONG busNumber,
        PHYSICAL_ADDRESS busAddress,
        ULONG *pAddressSpace,
        PHYSICAL_ADDRESS *pTranslatedAddress
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_TRANSLATE_BUS_ADDRESS
    virtual
    BOOL
    IoCtlTranslateSystemAddress(
        INTERFACE_TYPE  interfaceType,
        ULONG busNumber,
        PHYSICAL_ADDRESS systemAddress,
        PHYSICAL_ADDRESS *pTranslatedAddress
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_GET_POWER_STATE
    virtual
    BOOL
    IoCtlGetPowerState(
        CEDEVICE_POWER_STATE *pPowerState
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_SET_POWER_STATE
    virtual
    BOOL
    IoCtlSetPowerState(
        CEDEVICE_POWER_STATE *pPowerState
        ) = 0;
    
    // Called from XXX_IOControl/IOCTL_BUS_GET_CONFIGURE_DATA
    virtual
    BOOL
    IoCtlGetConfigurationData(
        DWORD space,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        ) = 0;
    
    // Called from XXX_IOControl/IOCTL_BUS_SET_CONFIGURE_DATA
    virtual
    BOOL
    IoCtlSetConfigurationData(
        DWORD space,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_ACTIVATE_CHILD
    virtual
    BOOL
    IoCtlActivateChild(
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_ACTIVATE_CHILD
    virtual
    BOOL
    IoCtlDeactivateChild(
        ) = 0;

    // Called from XXX_IOControl/IOCTL_BUS_IS_CHILD_REMOVED    
    virtual
    BOOL
    IoCtlIsChildRemoved(
        ) = 0;

    // Called from XXX_IOControl/IOCTL_xxx
    virtual
    BOOL
    IoControl(
        DWORD code, 
        UCHAR *pInBuffer, 
        DWORD inSize, 
        UCHAR *pOutBuffer,
        DWORD outSize, 
        DWORD *pOutSize
        ) = 0;

};

//------------------------------------------------------------------------------

