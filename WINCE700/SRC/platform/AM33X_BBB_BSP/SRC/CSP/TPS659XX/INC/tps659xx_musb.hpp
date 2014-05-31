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
//  File: tps659xx_musb.hpp
//

#include "transceiver_musb.hpp"

class HSUSBOTGTransceiverTps659xx : public HSUSBOTGTransceiver
{
public:
    HSUSBOTGTransceiverTps659xx(PHSMUSB_T pOTG);
    ~HSUSBOTGTransceiverTps659xx();
    void EnableVBusDischarge(BOOL fDischarge);
    void SetVBusSource(BOOL fBat);
    void EnableWakeupInterrupt(BOOL fEnable);
    void AconnNotifHandle(HANDLE hAconnEvent);
    BOOL UpdateUSBVBusSleepOffState(BOOL fActive);
    BOOL SupportsTransceiverWakeWithoutClock() {return TRUE;};
    BOOL SetLowPowerMode();
    BOOL IsSE0();
    void DumpULPIRegs();
    void Reset() {};
    void Resume() {};
    BOOL ResetPHY();
    BOOL IsADeviceConnected();
    BOOL IsBDeviceConnected();
    void SetOPmode(UCHAR mode) {UNREFERENCED_PARAMETER(mode);};
  
private:
    void Configure();
    BOOL ReadOmapTriton2(DWORD reg, BYTE *pData);
    BOOL WriteOmapTriton2(DWORD reg, BYTE Data);
    BOOL SetT2USBXvcrActiveT2(BOOL fActive);
    BOOL WakeT2USBPhyDPLLT2();
    BOOL SendSingularPBMessageT2(UCHAR power_res_id,UCHAR res_state);
    BOOL SetT2USBXvcrPowerOff(BOOL fActive);
    BOOL EnableFClock(BOOL enable);
    HANDLE  m_hTriton2Handle;
    HANDLE  m_hUsbPresenceEvent;
    HANDLE m_hStateChangeThread;
    HANDLE m_hAconnEvent;
    static DWORD StateChangeThread(void* pvData);
    PHSMUSB_T m_pOTG;
};
