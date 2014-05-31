// Copyright (c) Texas Instruments, Inc. 2011.  All rights reserved.
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2010.  All rights reserved.
//
// Module Name: 
//        usbotgpdd.h
//
// Abstract:
//        USB OTG Driver Header.
//
#pragma once

#include "am33x_config.h"
#include <usbotg.hpp>
#include <usbotgxr.hpp>

class CAM3xxOTG :public USBOTG{
public:
    CAM3xxOTG (LPCTSTR lpActivePath) ;
    ~CAM3xxOTG () ;
    BOOL Init() ;
    BOOL PostInit() ;

    // Overwrite 
    virtual BOOL IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut);
    virtual BOOL PowerUp();
    virtual BOOL PowerDown();
    virtual BOOL UpdatePowerState();

    // OTG PDD Function.
    BOOL    SessionRequest(BOOL fPulseLocConn, BOOL fPulseChrgVBus);
    BOOL    DischargVBus()  { return TRUE; };
    BOOL    NewStateAction(USBOTG_STATES usbOtgState , USBOTG_OUTPUT usbOtgOutput) ;
    BOOL    IsSE0();
    BOOL    UpdateInput() ;
    //BOOL    StateChangeNotification (USBOTG_TRANSCEIVER_STATUS_CHANGE , USBOTG_TRANSCEIVER_STATUS);
    USBOTG_MODE UsbOtgConfigure(USBOTG_MODE usbOtgMode ) { return usbOtgMode; };
	void	StartUSBModule();

protected:
    HANDLE  m_hParent;
    LPTSTR  m_ActiveKeyPath;
    HANDLE  m_hPollThread;
	AM33X_DEVICE_CONF_REGS *m_pSys;
    CSL_UsbRegs  *m_pUsbRegs;


    static DWORD CALLBACK PollThread(void *data);
    VOID HostMode(BOOL start);
    VOID FunctionMode(BOOL start);
    HANDLE m_hPollThreadExitEvent;
    DWORD  m_PollTimeout;
    BOOL   m_bUSB11Enabled;
    BOOL   m_bEnablePolling;
	BOOL   m_bFunctionMode;
    BOOL   m_InFunctionModeFn;
	BOOL   m_bHostMode;
	UINT32	m_Index;
    CEDEVICE_POWER_STATE  m_dwOldPowerState;

    void ChipCfgLock(BOOL lock);

    friend DWORD UnloadDrivers(CAM3xxOTG* pOTG);
private:
};
