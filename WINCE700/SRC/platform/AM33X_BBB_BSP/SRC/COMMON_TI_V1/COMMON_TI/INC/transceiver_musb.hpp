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
//
//------------------------------------------------------------------------------
//
//  File: transceiver_musb.hpp
//

#include "omap_musbcore.h"

class HSUSBOTGTransceiver
{
public:
    HSUSBOTGTransceiver() {};
    ~HSUSBOTGTransceiver() {};
    virtual void EnableVBusDischarge(BOOL fDischarge) = 0;
    virtual void SetVBusSource(BOOL fBat) = 0;
    virtual void EnableWakeupInterrupt(BOOL fEnable) = 0;
    virtual void AconnNotifHandle(HANDLE hAconnEvent) = 0;
    virtual BOOL UpdateUSBVBusSleepOffState(BOOL fActive) = 0;
    virtual BOOL SupportsTransceiverWakeWithoutClock() = 0;
    virtual BOOL SetLowPowerMode() = 0;
    virtual BOOL IsSE0() = 0;
    virtual void DumpULPIRegs() = 0;
    virtual void Reset() = 0;
    virtual void Resume() = 0;
    virtual BOOL ResetPHY() = 0;
    virtual BOOL IsADeviceConnected() = 0;
    virtual BOOL IsBDeviceConnected() = 0;
    virtual void SetOPmode(UCHAR mode)=0;
	
};

EXTERN_C HSUSBOTGTransceiver * CreateHSUSBOTGTransceiver(PHSMUSB_T pOTG);

