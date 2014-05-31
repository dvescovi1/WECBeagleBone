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
#include <omap_bus.h>
#include <powerbus.h>

//------------------------------------------------------------------------------

typedef struct {
    LONG        refCount_IClk[OMAP_MAX_DEVICE_COUNT];
    LONG        refCount_FClk[OMAP_MAX_DEVICE_COUNT];
} DEVICE_DEVICE_REFCOUNT;

//------------------------------------------------------------------------------

class omap35xxBusChild_t;

//------------------------------------------------------------------------------
//
//  Class:  omap35xxBus_t
//
class omap35xxBus_t : public DefaultBus_t
{
    friend Driver_t* Driver_t::CreateDevice();

public:
    typedef BOOL (*fnDeviceStateChange)(UINT dev, UINT oldState, UINT newState);
 
public:
    static DEVICE_DEVICE_REFCOUNT       s_refCountTbl;
    static OMAP_DEVCLKMGMT_FNTABLE     *s_pDevClkMgmtTbl;
    static omapPowerBus_t              *s_pPowerBus;             // Handle to TWL driver
    static fnDeviceStateChange          s_fnPreDeviceStateChange;
    static fnDeviceStateChange          s_fnPostDeviceStateChange;

private:

    // Helper function, which reads memory base    
    DWORD
    GetMemBase(
        LPCWSTR deviceKeyName
        );

    // Helper function, checks for DeviceId registry entry 
    BOOL
    CheckDriverId(
        LPCWSTR deviceKeyName
        );

    BOOL
    MapDeviceIdToMembase(
        OMAP_DEVICE devId,
        DWORD *pMembase
        );

    BusDriverChild_t*
    CreateBusDeviceChildByMembase(
        UINT memBase
        );

protected:

    omap35xxBus_t(
        );
    
    virtual
    ~omap35xxBus_t(
        );
    
public:

    virtual
    BOOL
    PostInit(
        );

    virtual
    BOOL 
    IoCtlPostInit(
        );
    
    virtual
    BOOL
    Init(
        LPCWSTR context, 
        LPCVOID pBusContext
        );

    virtual
    DriverContext_t*
    Open(
        DWORD accessCode,
        DWORD shareMode
        );

    virtual
    BusDriverChild_t* 
    CreateBusDeviceChild(
        LPCWSTR templateKeyName
        );

    BusDriverChild_t*
    OpenChildBusToBind(
        OMAP_BIND_CHILD_INFO *pInfo
        );

    BOOL
    CloseChildBusFromBind(
        BusDriverChild_t* pChild
        );

    BOOL
    RequestIClock(
        DWORD devId
        );

    BOOL
    SetPowerState(
        DWORD driverId,
        CEDEVICE_POWER_STATE state
        );

    BOOL
    SetSourceDeviceClocks(
        DWORD devId,
        UINT count,
        UINT rgSourceClocks[]
        );

    BOOL
    SetDevicePowerStateChangeCallbacks(
        void *pPrePowerStateChange,
        void *pPostPowerStateChange
        );

    BOOL
    ReleaseIClock(
        DWORD devId
        );

    BOOL
    RequestFClock(
        DWORD devId
        );

    BOOL
    ReleaseFClock(
        DWORD devId
        );

    BOOL
    GetDeviceContextState(
        DWORD devId
        );

    DWORD
    PreDevicePowerStateChange(
        DWORD devId,
        CEDEVICE_POWER_STATE oldState,
        CEDEVICE_POWER_STATE newState
        );

    DWORD
    PostDevicePowerStateChange(
        DWORD devId,
        CEDEVICE_POWER_STATE oldState,
        CEDEVICE_POWER_STATE newState
        );
    
    DWORD
    PreNotifyDevicePowerStateChange(
        DWORD devId,
        CEDEVICE_POWER_STATE oldState,
        CEDEVICE_POWER_STATE newState
        );

    DWORD
    PostNotifyDevicePowerStateChange(
        DWORD devId,
        CEDEVICE_POWER_STATE oldState,
        CEDEVICE_POWER_STATE newState
        );

    DWORD
    UpdateOnDeviceStateChange(
        DWORD devId,
        CEDEVICE_POWER_STATE oldState,
        CEDEVICE_POWER_STATE newState,
        BOOL bPreStateChange
        ); 
};
//------------------------------------------------------------------------------

