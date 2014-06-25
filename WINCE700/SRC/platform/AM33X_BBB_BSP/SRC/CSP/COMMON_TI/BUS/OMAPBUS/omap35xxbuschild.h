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
#include <DefaultBusChild.h>

//------------------------------------------------------------------------------

class omap35xxBus_t;

//------------------------------------------------------------------------------

class omap35xxBusChild_t : public DefaultBusChild_t
{
    friend class omap35xxBus_t;

protected:
    CEDEVICE_POWER_STATE m_powerState;
    DWORD m_devId;

    BOOL m_bSendPrePinMuxChange;
    BOOL m_bSendPostPinMuxChange;
    BOOL m_bSendPreDevicePowerStateChange;
    BOOL m_bSendPostDevicePowerStateChange;
    BOOL m_bSendPreNotifyDevicePowerStateChange;
    BOOL m_bSendPostNotifyDevicePowerStateChange;
    BOOL m_bRequestContextRestoreNotice;
    
protected:
    omap35xxBusChild_t(
        omap35xxBus_t* pBus,
        DWORD clock   
        );

public:
    virtual
    BOOL
    Init(
        DWORD index, 
        LPCWSTR deviceRegistryKey 
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

