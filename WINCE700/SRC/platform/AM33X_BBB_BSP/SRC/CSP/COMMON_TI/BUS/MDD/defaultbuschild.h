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
#include <DefaultBus.h>
#include <string.hxx>

//------------------------------------------------------------------------------

class DefaultBusChild_t : public BusDriverChild_t
{
    friend class DefaultBus_t;
    
protected:
    DefaultBus_t* m_pBus;
    ce::wstring m_deviceName;
    ce::wstring m_deviceRegistryKey;
    DWORD m_flags;
    ce::wstring m_entryPoint;
    ce::wstring m_dllName;
    HANDLE m_hDevice;
    DWORD  m_driverId;
    DWORD  m_ffPower;
    
protected:

    // Constructor
    DefaultBusChild_t(
        DefaultBus_t* pBus
        );

    // Destructor
    virtual
    ~DefaultBusChild_t(
        );
    
public:
    
    //----------------------------------------------------------------------
    // BusDriverChild_t interface, see BusDriver.h for comments

    virtual
    BOOL
    Init(
        DWORD index, 
        LPCWSTR deviceRegistryKey 
        );

    virtual
    DWORD        
    DriverId(
        );

    virtual
    LPCWSTR        
    DeviceName(
        );

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
        CEDEVICE_POWER_STATE *pPowerState
        );

    virtual
    BOOL
    IoCtlSetPowerState(
        CEDEVICE_POWER_STATE *pPowerState
        );
    
    virtual
    BOOL
    IoCtlGetConfigurationData(
        DWORD space,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        );
    
    virtual
    BOOL
    IoCtlSetConfigurationData(
        DWORD space,
        DWORD offset,
        DWORD length,
        VOID  *pBuffer
        );

    virtual
    BOOL
    IoCtlActivateChild(
        );

    virtual
    BOOL
    IoCtlDeactivateChild(
        );

    virtual
    BOOL
    IoCtlIsChildRemoved(
        );

    virtual
    BOOL
    IoControl(
        DWORD code, 
        UCHAR *pInBuffer, 
        DWORD inSize, 
        UCHAR *pOutBuffer,
        DWORD outSize, 
        DWORD *pOutSize
        );
    
};

//------------------------------------------------------------------------------

