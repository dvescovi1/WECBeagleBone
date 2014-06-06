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
//  File: isp1504_musb.hpp
//

#include "transceiver_musb.hpp"

class HSUSBOTGTransceiverIsp1504 : public HSUSBOTGTransceiver
{
public:
    HSUSBOTGTransceiverIsp1504(PHSMUSB_T pOTG);
    ~HSUSBOTGTransceiverIsp1504();
    void EnableVBusDischarge(BOOL fDischarge) {UNREFERENCED_PARAMETER(fDischarge);};
    void SetVBusSource(BOOL fBat) {UNREFERENCED_PARAMETER(fBat);};
    void EnableWakeupInterrupt(BOOL fEnable) {UNREFERENCED_PARAMETER(fEnable);};
    void AconnNotifHandle(HANDLE hAconnEvent);
    BOOL UpdateUSBVBusSleepOffState(BOOL fActive) {UNREFERENCED_PARAMETER(fActive);return TRUE;};
    BOOL SupportsTransceiverWakeWithoutClock() {return FALSE;};
    BOOL SetLowPowerMode();
    BOOL IsSE0();
    void DumpULPIRegs();
    void Reset();
    void Resume();
    BOOL ResetPHY() {return TRUE;};
    BOOL IsADeviceConnected() {return FALSE;};
    BOOL IsBDeviceConnected() {return FALSE;};
    void SetOPmode(UCHAR mode);
  
private:
    HANDLE m_hAconnEvent;
    PHSMUSB_T m_pOTG;
};
