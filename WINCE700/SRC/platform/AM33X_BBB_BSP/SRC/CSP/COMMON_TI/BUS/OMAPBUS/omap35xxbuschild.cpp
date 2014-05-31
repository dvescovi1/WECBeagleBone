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
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <devloadex.h>
#include <omap35xxBusChild.h>
#include <omap35xxBus.h>
#include <omap_dvfs.h>


extern "C" const DEVICE_ADDRESS_MAP s_DeviceAddressMap[];

EXTERN_C const GUID KERNEL_SYSPOWERUPDATE_GUID;

//------------------------------------------------------------------------------
//
//  Constructor:  omap35xxBusChild_t
//
omap35xxBusChild_t::
omap35xxBusChild_t(
    omap35xxBus_t *pBus,
    DWORD devId
    ) : m_devId(devId),
        DefaultBusChild_t(pBus)
{
    // Let assume we start with power off device...
    m_powerState = D4;
}

//----------------------------------------------------------------------
//
//  Method: DefaultBusChild_t::Init
//
BOOL
omap35xxBusChild_t::
Init(
    DWORD index, 
    LPCWSTR deviceRegistryKey
    )
{
    // if m_devId == OMAP_DEVICE_GENERIC, then the child is a generic 
    // child used to communicate with the dvfs policy.  Don't try to 
    // pull data from the registery for the child
    
    BOOL rc = FALSE;
    if (m_devId != OMAP_MAX_DEVICE_COUNT)
        {
        // assume context restore, pre/post device notifications are
        // required
        m_ffPower = POWERFLAGS_PRESTATECHANGENOTIFY | 
                    POWERFLAGS_POSTSTATECHANGENOTIFY |
                    POWERFLAGS_CONTEXTRESTORE;

        if (deviceRegistryKey != NULL)
            {
            if (DefaultBusChild_t::Init(index, deviceRegistryKey) == FALSE)
                {
                goto cleanUp;
                }
            }
        else
            {
            m_flags |= DEVFLAGS_NOLOAD;
            }            
        }

    // if default id is not set then use m_devId
    //
    if (m_driverId == -1)
        m_driverId = m_devId;

    // check if pre/post device state change notifications should
    // be sent for this driver
    m_bSendPrePinMuxChange = TRUE;
    m_bSendPostPinMuxChange = TRUE;
    m_bSendPreDevicePowerStateChange = TRUE;
    m_bSendPostDevicePowerStateChange = TRUE;
    m_bSendPreNotifyDevicePowerStateChange = (m_ffPower & POWERFLAGS_PRESTATECHANGENOTIFY) != 0;
    m_bSendPostNotifyDevicePowerStateChange = (m_ffPower & POWERFLAGS_POSTSTATECHANGENOTIFY) != 0;
    m_bRequestContextRestoreNotice = (m_ffPower & POWERFLAGS_CONTEXTRESTORE) != 0;

    rc = TRUE;
cleanUp:    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBusChild_t::IoCtlGetPowerState
//
BOOL
omap35xxBusChild_t::
IoCtlGetPowerState(
    CEDEVICE_POWER_STATE *pPowerState
    )
{
    *pPowerState = m_powerState;
    return TRUE;
}
    
//------------------------------------------------------------------------------
//
//  Method: omap35xxBusChild_t::IoCtlSetPowerState
//
BOOL
omap35xxBusChild_t::
IoCtlSetPowerState(
    CEDEVICE_POWER_STATE *pPowerState
    )
{    
    CEDEVICE_POWER_STATE oldState = m_powerState;
    CEDEVICE_POWER_STATE newState = *pPowerState;
    omap35xxBus_t* pBus = reinterpret_cast<omap35xxBus_t*>(m_pBus);

    // notify power bus of device power state change
    if (oldState != newState)
        {
        if (m_bSendPrePinMuxChange == TRUE &&
            pBus->UpdateOnDeviceStateChange(m_devId, oldState, newState, TRUE) == -1)
            {
            m_bSendPrePinMuxChange = FALSE;
            }
        
        if (m_bSendPreNotifyDevicePowerStateChange == TRUE &&
            pBus->PreNotifyDevicePowerStateChange(m_devId, oldState, newState) == -1)
            {
            m_bSendPreNotifyDevicePowerStateChange = FALSE;
            }

        if (m_bSendPreDevicePowerStateChange == TRUE &&
            pBus->PreDevicePowerStateChange(m_devId, oldState, newState) == -1)
            {
            m_bSendPreDevicePowerStateChange = FALSE;
            }
        }
    
    switch (newState)
        {
        case D0:
        case D1:
            // enable interface and function clocks
            if (m_powerState > D1)
                {
                if (m_powerState > D2)
                    {                    
                    pBus->RequestFClock(m_devId);
                    } 
                pBus->RequestIClock(m_devId);                               
                }
            break;
            
        case D2:
            // enable only functional clocks
            if (m_powerState < D2)
                {
                pBus->ReleaseIClock(m_devId);
                }
            else if (m_powerState > D2)
                {                
                pBus->RequestFClock(m_devId);
                }
            break;
            
        case D3:
        case D4:
            if (m_powerState < D3)
                {                
                pBus->ReleaseFClock(m_devId);
                pBus->ReleaseIClock(m_devId);
                }
            break;
        }

    // Disable power if new state is D4
    if (oldState != newState)
        {
        // check if context restore needs to be sent
        if (((m_ffPower & POWERFLAGS_CONTEXTRESTORE) != 0) &&
            (oldState >= D3 && newState <= D2))
            {
            // Check if the device context state is retained and device isn't being
            // initialized
            if(pBus->GetDeviceContextState(m_devId) == FALSE && m_hDevice != NULL)
                {
                PmxSendDeviceNotification(DEVICEMEDIATOR_DEVICE_LIST,
                        m_devId, IOCTL_CONTEXT_RESTORE,
                        NULL, 0, NULL, 0, NULL);
                }
            }

        if (m_bSendPostDevicePowerStateChange == TRUE &&
            pBus->PostDevicePowerStateChange(m_devId, oldState, newState) == -1)
            {
            m_bSendPostDevicePowerStateChange = FALSE;
            }

        if (m_bSendPostNotifyDevicePowerStateChange == TRUE &&
            pBus->PostNotifyDevicePowerStateChange(m_devId, oldState, newState) == -1)
            {
            m_bSendPostNotifyDevicePowerStateChange = FALSE;
            }

        if (m_bSendPostPinMuxChange == TRUE &&
            pBus->UpdateOnDeviceStateChange(m_devId, oldState, newState, FALSE) == -1)
            {
            m_bSendPostPinMuxChange = FALSE;
            }
        }
    
    // Save new power state
    m_powerState = newState;
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Method: omap35xxBusChild_t::IoControl
//
BOOL
omap35xxBusChild_t::
IoControl(
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);    
    return rc;
}

//------------------------------------------------------------------------------

