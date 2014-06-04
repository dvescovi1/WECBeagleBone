// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
#ifndef __POWERBUS_H
#define __POWERBUS_H


//-----------------------------------------------------------------------------
//
//  Class:  omapPowerBus_t
//
class omapPowerBus_t
{
public:
    static 
    omapPowerBus_t* 
    CreatePowerBus(
        );
    
    static 
    void
    DestroyPowerBus(
        omapPowerBus_t* pPowerBus
        );

public:

    virtual 
    DWORD 
    PreDevicePowerStateChange(
        DWORD devId, 
        CEDEVICE_POWER_STATE oldPowerState,
        CEDEVICE_POWER_STATE newPowerState,
        DWORD clockCnt
        ) = 0;
    
    virtual 
    DWORD 
    PostDevicePowerStateChange(
        DWORD devId, 
        CEDEVICE_POWER_STATE oldPowerState,
        CEDEVICE_POWER_STATE newPowerState,
        DWORD clockCnt
        ) = 0;
};

//-----------------------------------------------------------------------------

#endif //__POWERBUS_H
