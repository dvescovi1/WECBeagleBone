//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#pragma once
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.


Module Name:    HWCTXT.H

Abstract:               Platform dependent code for the mixing audio driver.

-*/


#include <ceddk.h>
#include <ddkreg.h>
#include <am33x.h>
#include "am33x_config.h"
#include "audiodma.h"
#include "tlv320aic3106.h"
#include "audio_mcasp.h"

class HardwareContext
{
public:
    static BOOL CreateHWContext(DWORD Index);
    HardwareContext();
    ~HardwareContext();

    void Lock()   {EnterCriticalSection(&m_Lock);}
    void Unlock() {LeaveCriticalSection(&m_Lock);}

    DWORD GetNumInputDevices()  {return AUDIO_NUM_INPUT_DEVICES;}
    DWORD GetNumOutputDevices() {return AUDIO_NUM_OUTPUT_DEVICES;}
    DWORD GetNumMixerDevices()  {return AUDIO_NUM_MIXER_DEVICES;}

    DeviceContext *GetInputDeviceContext(UINT DeviceId)
    {
        return &m_InputDeviceContext;
    }

    DeviceContext *GetOutputDeviceContext(UINT DeviceId)
    {
        return &m_OutputDeviceContext[DeviceId];
    }

    BOOL Init(DWORD Index);
    BOOL Deinit();

    void PowerUp();
    void PowerDown();

    BOOL StartAudioOutput();
    void StopAudioOutput();

    BOOL StartAudioInput();
    void StopAudioInput();

    void InterruptThread();
    void InterruptThreadRx();

    void McASPInterruptThread();
    
    void McASPInterruptThreadRx();
    
    DWORD ForceSpeaker (BOOL bSpeaker);

    BOOL PmControlMessage (
                      DWORD  dwCode,
                      PBYTE  pBufIn,
                      DWORD  dwLenIn,
                      PBYTE  pBufOut,
                      DWORD  dwLenOut,
                      PDWORD pdwActualOut);

    BOOL IsReady() {return m_DxState == D0;}

    TLV320AIC3106CodecConfig* GetCodec() {return m_pCodec;}

    void GetMcaspReg(DWORD *reg);

protected:

    BOOL InitInterruptThread();

    ULONG TransferOutputBuffer(ULONG NumBuf);
    ULONG TransferInputBuffer(ULONG NumBuf);

    DWORD GetInterruptThreadPriority()
    {
        return m_dwInterruptThreadPriority;
    }

    DWORD m_DriverIndex;
    CRITICAL_SECTION m_Lock;

    BOOL m_Initialized;
    BOOL m_Running;
    BOOL m_InPowerHandler;

    OutputDeviceContext m_OutputDeviceContext[AUDIO_NUM_OUTPUT_DEVICES];
    InputDeviceContext  m_InputDeviceContext;

    // Track number of valid samples in each DMA output buffer
    ULONG m_OutBytes[AUDIO_DMA_NUMBER_PAGES];

    // Which DMA channels are running
    BOOL m_AudioOutputRunning;
    BOOL m_AudioInputRunning;
    
    BYTE m_NextOutputBuffer;
	BYTE m_NextInputBuffer;

    LONG m_NumForcedSpeaker;
    void SetSpeakerEnable(BOOL bEnable);
    void RecalcSpeakerEnable();

    CEDEVICE_POWER_STATE m_DxState;

public:
    DWORD GetDriverRegValue(HKEY hKey, LPWSTR ValueName, DWORD DefaultValue);
    void SetDriverRegValue(HKEY hKey, LPWSTR ValueName, DWORD Value);
    DWORD GetRegistryConfig(LPWSTR lpRegPath, DWORD * lpdwIoBase, DWORD * IrqTX, DWORD * IrqRX,
                            DWORD * lpdwThreadPriority, DWORD * lpdwEventIdTx, DWORD * lpdwEventIdRx,
                            DWORD * lpdwEdmaEventQueue);
    
// Gain-related APIs
public:
    DWORD       GetOutputGain (UINT DeviceId);
    MMRESULT    SetOutputGain (UINT DeviceId, DWORD dwVolume);
    DWORD       GetMasterVolume();
    MMRESULT    SetMasterVolume(DWORD dwVolume);
    MMRESULT    SetMasterMute(BOOL fMute);
    BOOL        GetMasterMute();
    BOOL        GetOutputMute (UINT DeviceId);
    MMRESULT    SetOutputMute (UINT DeviceId, BOOL fMute);

    DWORD       GetInputGain (void);
    MMRESULT    SetInputGain (DWORD dwVolume);
    BOOL        GetInputMute (void);
    MMRESULT    SetInputMute (BOOL fMute);

protected:
    void UpdateOutputGain(UINT DeviceId);
    DWORD m_dwOutputGain[AUDIO_NUM_OUTPUT_DEVICES];
    BOOL  m_fOutputMute[AUDIO_NUM_OUTPUT_DEVICES];

    void UpdateInputGain();
    DWORD m_dwInputGain;
    BOOL  m_fInputMute;

public:
    BOOL IsSupportedOutputFormat(LPWAVEFORMATEX lpFormat);

protected:
    DWORD m_McASPSysIntr;
    DWORD m_McASPSysIntrRx;

    HANDLE m_hAudioInterrupt;          // Handle to Audio Interrupt event.
    HANDLE m_hAudioInterruptRx;          // Handle to Audio RX Interrupt event.
    HANDLE m_hAudioInterruptThread;    // Handle to thread which waits on an audio interrupt event.
    HANDLE m_hAudioInterruptThreadRx;
    HANDLE m_hMcASPInterruptEvent;          
    HANDLE m_hMcASPInterruptThread;    
    HANDLE m_hMcASPInterruptEventRx;          
    HANDLE m_hMcASPInterruptThreadRx;    
    ULONG m_nVolume;                   // Current HW Playback Volume
    BOOL  m_fMute;
    ULONG m_ulEventIdTx;                 // DMA TX event ID
    ULONG m_ulEventIdRx;                 // DMA RX event ID

    // Physical starting address of each DMA buffer (what the HW cares about)
    PHYSICAL_ADDRESS m_DMAOutBufPhysAddr;
    PHYSICAL_ADDRESS m_DMAInBufPhysAddr;
    
    // Virtual starting address of each DMA buffer (what the SW cares about)
    PBYTE m_DMAOutBufVirtAddr;
    PBYTE m_DMAInBufVirtAddr;

    // Virtual addresses of each DMA page
    PBYTE m_DMAOutPageVirtAddr[AUDIO_DMA_NUMBER_PAGES];
    PBYTE m_DMAInPageVirtAddr[AUDIO_DMA_NUMBER_PAGES];
    
    PMCASPREGS m_pMcASPRegs;
    DWORD m_dwInterruptThreadPriority;
    
    TLV320AIC3106CodecConfig *m_pCodec;
    AudioDma *m_pDmaTx;
    AudioDma *m_pDmaRx;
    McASP *m_pMcASP;   
    ULONG m_McASPIrqTx;     
    ULONG m_McASPIrqRx; 
};

void CallInterruptThread(HardwareContext *pHWContext);
void CallInterruptThreadRx(HardwareContext *pHWContext);

void CallMcASPInterruptThread(HardwareContext *pHWContext);
void CallMcASPInterruptThreadRx(HardwareContext *pHWContext);

extern HardwareContext *g_pHWContext;


