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
#include <windows.h>
#include <devload.h>
#include <devloadex.h>
#include <cebuscfg.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <initguid.h>
#include <omap_pmext.h>
#include <registry.hxx>
#include <omap35xxBus.h>
#include <omap35xxBusChild.h>
#include <omap35xxBusContext.h>
#include <DefaultBusChildContainer.h>

#include <omap_guid.h>

extern const GUID                   PMCLASS_BUSNAMESPACE_GUID;

// depending on if we are running on WinMobile or WinCE we need to
// load the dvfs policy manager at the correct boot phase
//
#define ROOTBUS_APMLOAD_KEY        (L"$bus\\BuiltIn")

//------------------------------------------------------------------------------

DEVICE_DEVICE_REFCOUNT      omap35xxBus_t::s_refCountTbl;
OMAP_DEVCLKMGMT_FNTABLE    *omap35xxBus_t::s_pDevClkMgmtTbl = NULL;
omapPowerBus_t             *omap35xxBus_t::s_pPowerBus = NULL;

omap35xxBus_t::fnDeviceStateChange omap35xxBus_t::s_fnPreDeviceStateChange = NULL;
omap35xxBus_t::fnDeviceStateChange omap35xxBus_t::s_fnPostDeviceStateChange = NULL;

//------------------------------------------------------------------------------

#define REGEDIT_PMEXT_PMEXTPATH         (L"PmExtPath")
#define REGEDIT_PMEXT_DLL               (L"Dll")
#define PME_INIT_NAME                   (L"PMExt_Init")

typedef DWORD (*fnInit)(HKEY hKey, LPCTSTR lpRegistryPath);
static fnInit pme_init;

//------------------------------------------------------------------------------

extern "C" const DEVICE_ADDRESS_MAP s_DeviceAddressMap[];

//------------------------------------------------------------------------------
//
//  Constructor:  omap35xxBus_t::omap35xxBus_t
//
omap35xxBus_t::
omap35xxBus_t(
    ) : DefaultBus_t()
{
}

//------------------------------------------------------------------------------
//
//  Destructor:  omap35xxBus_t::~omap35xxBus_t
//
omap35xxBus_t::
~omap35xxBus_t(
    )
{
}

//------------------------------------------------------------------------------
//
//  Method:  InistializePMExtension
//
static VOID
InitializePMExtension(
    LPCWSTR deviceKey
    )
{
    _TCHAR s_szPmExtPath[MAX_PATH];
    _TCHAR s_szDll[MAX_PATH];
    HMODULE s_hModule;

    LONG code;
    DWORD size;
    HKEY hKey = NULL;

    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, deviceKey, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    size = sizeof(s_szPmExtPath);
    code = RegQueryValueEx(hKey, REGEDIT_PMEXT_PMEXTPATH, 0, 0, (BYTE*)&s_szPmExtPath, &size);
    if (code != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"InitializePMExtension: Can't find 'PmExtPath' key.\r\n"));
        goto cleanUp;
        }
    RegCloseKey(hKey);

    hKey = NULL;
    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, s_szPmExtPath, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read dll name
    size = sizeof(s_szDll);
    code = RegQueryValueEx(hKey, REGEDIT_PMEXT_DLL, 0, 0, (BYTE*)&s_szDll, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    s_hModule = ::LoadLibrary(s_szDll);
    if (s_hModule == NULL) goto cleanUp;

    //
    // load functions
    
    pme_init = reinterpret_cast<fnInit>(::GetProcAddress(s_hModule, PME_INIT_NAME));

    DEBUGMSG(ZONE_INFO, (L"InitializePMExtension: loading pmext.dll(%s)\r\n", s_szPmExtPath));
    (*pme_init)(HKEY_LOCAL_MACHINE, s_szPmExtPath);
cleanUp:
    if (hKey != NULL)
        RegCloseKey(hKey);
    return;
}

//------------------------------------------------------------------------------
//
//  Method:  omap35xxBus_t::PostInit
//
BOOL
omap35xxBus_t::
PostInit(
    )
{
    BOOL bRet = FALSE;
    
    if (s_pDevClkMgmtTbl == NULL)
        {
        s_pDevClkMgmtTbl = (OMAP_DEVCLKMGMT_FNTABLE*)LocalAlloc(LPTR, sizeof(OMAP_DEVCLKMGMT_FNTABLE));
        if (s_pDevClkMgmtTbl == NULL) goto cleanUp;

        // get clock ref counter table from kernel
        if (!KernelIoControl(IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE, (void*)&KERNEL_DEVCLKMGMT_GUID,
                sizeof(KERNEL_DEVCLKMGMT_GUID), s_pDevClkMgmtTbl,
                sizeof(OMAP_DEVCLKMGMT_FNTABLE),
                NULL))
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: omap35xxBus_t::Init: "
                L"Failed get clock management table from kernel\r\n"
                ));
            goto cleanUp;
            }

        // pmext - initialize pme module
        InitializePMExtension((LPCWSTR)m_deviceKey);
        
        memset(&s_refCountTbl, 0, sizeof(s_refCountTbl));

        // get powerbus interface
        s_pPowerBus = omapPowerBus_t::CreatePowerBus();
        }
    else
        {
        bRet = TRUE;
        }

cleanUp:
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method:  omap35xxBus_t::IoCtlPostInit
//
BOOL
omap35xxBus_t:: 
IoCtlPostInit(
    )
{
    // send notification about the bus driver being loaded
    AdvertiseInterface(&PMCLASS_BUSNAMESPACE_GUID, m_deviceName, TRUE);

    // call default ioctl handler
    return DefaultBus_t::IoCtlPostInit();
}

//------------------------------------------------------------------------------
//
//  Method:  omap35xxBus_t::SetDevicePowerStateChangeCallbacks
//
BOOL
omap35xxBus_t:: 
SetDevicePowerStateChangeCallbacks(
    void *pPrePowerStateChange,
    void *pPostPowerStateChange
    )
{
    if (s_fnPreDeviceStateChange == NULL)
        {
        s_fnPreDeviceStateChange = (fnDeviceStateChange)pPrePowerStateChange;
        }

    if (s_fnPostDeviceStateChange == NULL)
        {
        s_fnPostDeviceStateChange = (fnDeviceStateChange)pPostPowerStateChange;
        }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::Init
//
BOOL
omap35xxBus_t::
Init(
    LPCWSTR context,
    LPCVOID pBusContext
    )
{
    BOOL rc = FALSE;

    // At end call default bus implementation
    rc = DefaultBus_t::Init(context, pBusContext);
    return rc;
}


//------------------------------------------------------------------------------
//
//  Method: DefaultBus_t::Open
//
DriverContext_t*
omap35xxBus_t::
Open(
    DWORD accessCode, DWORD shareMode
    )
{
    DriverContext_t *pContext = NULL;

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    // Create new context object
    pContext = new omap35xxBusContext_t(this);
    if (pContext == NULL) goto cleanUp;

    // Add open context to container
    m_pContextContainer->push_back(pContext);

    // There is reference in container
    pContext->AddRef();

cleanUp:
    return pContext;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::MapDeviceIdToMembase
//
BOOL
omap35xxBus_t::
MapDeviceIdToMembase(
    OMAP_DEVICE devId,
    DWORD *pMembase
    )
{
    BOOL rc = FALSE;
    for (int i = 0; s_DeviceAddressMap[i].deviceAddress != 0; ++i)
        {
        if (s_DeviceAddressMap[i].device == devId)
            {
            *pMembase = s_DeviceAddressMap[i].deviceAddress;
            rc = TRUE;
            break;
            }
        }

    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::CreateBusDeviceChildByMembase
//
BusDriverChild_t*
omap35xxBus_t::
CreateBusDeviceChildByMembase(
    UINT memBase
    )
{
    BusDriverChild_t* pChild = NULL;

    for (int i = 0; s_DeviceAddressMap[i].deviceAddress != 0; ++i)
        {
        if (s_DeviceAddressMap[i].deviceAddress == memBase)
            {
            pChild = new omap35xxBusChild_t(this, s_DeviceAddressMap[i].device);
            break;
            }
        }

    return pChild;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::CreateBusDeviceChild
//
BusDriverChild_t*
omap35xxBus_t::
CreateBusDeviceChild(
    LPCWSTR deviceKeyName
    )
{
    BusDriverChild_t* pChild;

    // Set interface and function code based on device physical address
    DWORD memBase = GetMemBase(deviceKeyName);

    pChild = CreateBusDeviceChildByMembase(memBase);
    if (pChild == NULL)
        {
            // check if there is a device id associated with the device
            // driver to handle DVFS notifications
            //
            if (CheckDriverId(deviceKeyName) == TRUE)
                {
                pChild = new omap35xxBusChild_t(this, OMAP_DEVICE_NONE);
                }
            else
                {
                pChild = DefaultBus_t::CreateBusDeviceChild(deviceKeyName);
                }
        }

    return pChild;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::GetMemBase
//
DWORD
omap35xxBus_t::
GetMemBase(
    LPCWSTR deviceKeyName
    )
{
    DWORD memBase = 0;
    HKEY hKey = NULL;
    LONG code;
    DWORD type;


    // We need to get memBase to find which device driver we will create
    code = RegOpenKeyEx(HKEY_LOCAL_MACHINE, deviceKeyName, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    code = ce::RegQueryValueType(hKey, DEVLOAD_MEMBASE_VALNAME, type);
    if (code != ERROR_SUCCESS) goto cleanUp;

    switch (type)
        {
        case REG_DWORD:
            ce::RegQueryValue(hKey, DEVLOAD_MEMBASE_VALNAME, memBase);
            break;
        case REG_SZ:
        case REG_MULTI_SZ:
            ce::wstring value;
            // Read string or multi string
            ce::RegQueryValue(hKey, DEVLOAD_MEMBASE_VALNAME, value);
            // Convert first string as hexa number
            memBase = wcstoul(value, NULL, 16);
            break;
        }

cleanUp:
    if (hKey != NULL) RegCloseKey(hKey);
    return memBase;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::CheckDriverId
//
BOOL
omap35xxBus_t::
CheckDriverId(
        LPCWSTR deviceKeyName
        )
{
    BOOL rc = FALSE;
    HKEY hKey = NULL;
    LONG code;
    DWORD type;


    // We need to get memBase to find which device driver we will create
    code = RegOpenKeyEx(HKEY_LOCAL_MACHINE, deviceKeyName, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    code = ce::RegQueryValueType(hKey, DEVICEID_VALNAME, type);
    if (code != ERROR_SUCCESS) goto cleanUp;

    rc = TRUE;
cleanUp:
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::RequestIClock
//
BOOL
omap35xxBus_t::
RequestIClock(
    DWORD devId
    )
{
    BOOL rc = TRUE;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        if (InterlockedIncrement(&s_refCountTbl.refCount_IClk[devId]) == 1)
            {
            rc = s_pDevClkMgmtTbl->pfnEnableDeviceIClock(devId, TRUE);
            }
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::ReleaseIClock
//
BOOL
omap35xxBus_t::
ReleaseIClock(
    DWORD devId
    )
{
    BOOL rc = TRUE;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        if (InterlockedDecrement(&s_refCountTbl.refCount_IClk[devId]) == 0)
            {
            rc = s_pDevClkMgmtTbl->pfnEnableDeviceIClock(devId, FALSE);
            }
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::RequestFClock
//
BOOL
omap35xxBus_t::
RequestFClock(
    DWORD devId
    )
{
    BOOL rc = TRUE;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        if (InterlockedIncrement(&s_refCountTbl.refCount_FClk[devId]) == 1)
            {
            rc = s_pDevClkMgmtTbl->pfnEnableDeviceFClock(devId, TRUE);
            }
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::ReleaseFClock
//
BOOL
omap35xxBus_t::
ReleaseFClock(
    DWORD devId
    )
{
    BOOL rc = TRUE;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        if (InterlockedDecrement(&s_refCountTbl.refCount_FClk[devId]) == 0)
            {
            rc = s_pDevClkMgmtTbl->pfnEnableDeviceFClock(devId, FALSE);
            }
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::GetDeviceContextState
//
BOOL
omap35xxBus_t::
GetDeviceContextState(
    DWORD devId
    )
{
    BOOL rc = TRUE;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        rc = s_pDevClkMgmtTbl->pfnGetDeviceContextState(devId, TRUE);
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::PreDevicePowerStateChange
//
//  send device power state change notification to power ic
//
DWORD
omap35xxBus_t::
PreDevicePowerStateChange(
    DWORD devId,
    CEDEVICE_POWER_STATE oldState,
    CEDEVICE_POWER_STATE newState
    )
{
    DWORD rc = (DWORD) -1;
    if (devId < OMAP_MAX_DEVICE_COUNT)
    {
        rc = s_pPowerBus->PreDevicePowerStateChange(devId, oldState, newState, s_refCountTbl.refCount_FClk[devId]);
    }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::PostDevicePowerStateChange
//
//  send device power state change notification to power ic
//
DWORD
omap35xxBus_t::
PostDevicePowerStateChange(
    DWORD devId,
    CEDEVICE_POWER_STATE oldState,
    CEDEVICE_POWER_STATE newState
    )
{
    DWORD rc = (DWORD) -1;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        rc = s_pPowerBus->PostDevicePowerStateChange(devId, oldState, newState, s_refCountTbl.refCount_FClk[devId]);
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::PreNotifyDevicePowerStateChange
//
//  send power state change notification to power policy observers
//
DWORD
omap35xxBus_t::
PreNotifyDevicePowerStateChange(
    DWORD devId,
    CEDEVICE_POWER_STATE oldState,
    CEDEVICE_POWER_STATE newState
    )
{
    DWORD rc = (DWORD) -1;
    if (devId < OMAP_MAX_DEVICE_COUNT && s_fnPreDeviceStateChange != NULL)
        {
        rc = s_fnPreDeviceStateChange(devId, oldState, newState);
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::PostNotifyDevicePowerStateChange
//
//  send power state change notification to power policy observers
//
DWORD
omap35xxBus_t::
PostNotifyDevicePowerStateChange(
    DWORD devId,
    CEDEVICE_POWER_STATE oldState,
    CEDEVICE_POWER_STATE newState
    )
{
    DWORD rc = (DWORD) -1;
    if (devId < OMAP_MAX_DEVICE_COUNT && s_fnPostDeviceStateChange != NULL)
        {
        rc = s_fnPostDeviceStateChange(devId, oldState, newState);
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::PrePinmuxDevicePowerStateChange
//
//  allow kernel to pin mux before device state change
//
DWORD
omap35xxBus_t::
UpdateOnDeviceStateChange(
    DWORD devId,
    CEDEVICE_POWER_STATE oldState,
    CEDEVICE_POWER_STATE newState,
    BOOL bPreStateChange
    )
{
    DWORD rc = (DWORD) -1;
    if (devId < OMAP_MAX_DEVICE_COUNT)
        {
        rc = s_pDevClkMgmtTbl->pfnUpdateOnDeviceStateChange(devId, 
                oldState, newState, bPreStateChange
                );
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::SetPowerState
//
BOOL
omap35xxBus_t::
SetPowerState(
    DWORD driverId,
    CEDEVICE_POWER_STATE state
    )
{
    BOOL rc = FALSE;
    BusDriverChild_t* pChild;
    pChild = m_pChildContainer->FindChildByDriverId(driverId);

    // if a child wasn't found then it could be a childbus
    // with no device driver associated with it.  In this case create
    // the child and insert into collection
    //
    if (pChild == NULL)
        {
        DWORD memBase;
        if (MapDeviceIdToMembase((OMAP_DEVICE)driverId, &memBase) == TRUE)
            {
            // initialize child and insert into child collection
            //
            pChild = CreateBusDeviceChildByMembase(memBase);
            if (pChild != NULL)
                {
                pChild->Init(0, NULL);
                m_pChildContainer->push_back(pChild);

                // since the container class will not perform
                // addref on inserted objects we need to do it
                //
                pChild->AddRef();
                }
            }
        }

    if (pChild != NULL)
        {
        pChild->IoCtlSetPowerState(&state);

        // doing any FindChildBy... performs an addref on the child
        // so we need to perform a release since we don't need the pointer
        //
        pChild->Release();
        rc = TRUE;
        }

    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::OpenChildBusToBind
//
BusDriverChild_t*
omap35xxBus_t::
OpenChildBusToBind(
    OMAP_BIND_CHILD_INFO *pInfo
    )
{
    // check if a child is already binded.
    BusDriverChild_t* pChild = NULL;

    // the only child bus which isn't considered a singelton is the
    // generic child bus which is used for notifications to the
    // dvfs policy manager
    if (pInfo->devId != OMAP_MAX_DEVICE_COUNT)
        {
        pChild = m_pChildContainer->FindChildByDriverId(pInfo->devId);
        }

    // if a child wasn't found then it could be a childbus
    // with no device driver associated with it.  In this case create
    // the child and insert into collection
    //
    if (pChild == NULL)
        {
        DWORD memBase;
        if (MapDeviceIdToMembase((OMAP_DEVICE)pInfo->devId, &memBase) == TRUE)
            {
            // initialize child and insert into child collection
            //
            pChild = CreateBusDeviceChildByMembase(memBase);
            if (pChild != NULL)
                {
                pChild->Init(0, pInfo->szData);
                m_pChildContainer->push_back(pChild);

                // since the container class will not perform
                // addref on inserted objects we need to do it
                //
                pChild->AddRef();
                }
            }
        }

    return pChild;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::CloseChildBusFromBind
//
BOOL
omap35xxBus_t::
CloseChildBusFromBind(
    BusDriverChild_t* pChild
    )
{
    // check if the binded childbus is a generic child.  If so remove it
    // from the collection
    //
    if (pChild->DriverId() == OMAP_MAX_DEVICE_COUNT)
        {
        m_pChildContainer->RemoveChild(pChild);
        }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBus_t::SetSourceDeviceClocks
//
BOOL
omap35xxBus_t::
SetSourceDeviceClocks(
        DWORD devId,
        UINT count,
        UINT rgSourceClocks[]
        )
{
    BOOL rc = FALSE;

    if (s_pDevClkMgmtTbl == NULL) goto cleanUp;
    if (s_pDevClkMgmtTbl->pfnSetSourceDeviceClocks == NULL) goto cleanUp;

    rc = s_pDevClkMgmtTbl->pfnSetSourceDeviceClocks(devId, count, rgSourceClocks);

cleanUp:
    return rc;
}
//------------------------------------------------------------------------------


