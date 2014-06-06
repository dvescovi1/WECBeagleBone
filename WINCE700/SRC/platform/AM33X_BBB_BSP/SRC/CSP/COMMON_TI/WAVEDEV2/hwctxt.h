// All rights reserved ADENEO EMBEDDED 2010
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

#define OUTPUT_SAMPLERATE  (44100)
#define INPUT_SAMPLERATE   OUTPUT_SAMPLERATE

#define AUDIO_DMA_BUFFER_SIZE   0x1000
#define AUDIO_DMA_NUMBER_PAGES  2

#define AUDIO_DMA_PAGE_SIZE (AUDIO_DMA_BUFFER_SIZE/AUDIO_DMA_NUMBER_PAGES)

class HardwareContext
{
    void*                   m_pCallbackData;
    DataTransfer_Command    m_fnRxCommand;
    DataTransfer_Command    m_fnTxCommand;
	PortConfigInfo_t		m_PlayPortConfigInfo;
	PortConfigInfo_t		m_RecPortConfigInfo;

public:
    static BOOL CreateHWContext(DWORD Index);
    HardwareContext();
    ~HardwareContext();

    void Lock()   {EnterCriticalSection(&m_Lock);}
    void Unlock() {LeaveCriticalSection(&m_Lock);}

    DWORD GetNumInputDevices()  {return 1;}
    DWORD GetNumOutputDevices() {return 1;}
    DWORD GetNumMixerDevices()  {return 1;}

    DeviceContext *GetInputDeviceContext(UINT DeviceId)
    {
		UNREFERENCED_PARAMETER(DeviceId);

		return &m_InputDeviceContext;
    }

    DeviceContext *GetOutputDeviceContext(UINT DeviceId)
    {
		UNREFERENCED_PARAMETER(DeviceId);

		return &m_OutputDeviceContext;
    }

    BOOL Init(DWORD Index);
    BOOL Deinit();

    void PowerUp();
    void PowerDown();

    BOOL StartInputDMA();
    BOOL StartOutputDMA();

    void StopInputDMA();
    void StopOutputDMA();

    DWORD ForceSpeaker (BOOL bSpeaker);

    BOOL PmControlMessage (
                      DWORD  dwCode,
                      PBYTE  pBufIn,
                      DWORD  dwLenIn,
                      PBYTE  pBufOut,
                      DWORD  dwLenOut,
                      PDWORD pdwActualOut);

    ULONG TransferInputBuffer(PBYTE pStart, DWORD dwLength, DWORD* dwNumStreams);
    ULONG TransferOutputBuffer(PBYTE pStart, DWORD dwLength, DWORD* dwNumStreams);

protected:

    DWORD m_DriverIndex;
    CRITICAL_SECTION m_Lock;

    BOOL m_Initialized;
    BOOL m_InPowerHandler;

    InputDeviceContext  m_InputDeviceContext;
    OutputDeviceContext m_OutputDeviceContext;

    // Which DMA channels are running
    BOOL m_InputDMARunning;
    BOOL m_OutputDMARunning;

    LONG m_NumForcedSpeaker;
    void SetSpeakerEnable(BOOL bEnable);
    void RecalcSpeakerEnable();

    CEDEVICE_POWER_STATE m_DxState;

public:
    DWORD GetDriverRegValue(LPWSTR ValueName, DWORD DefaultValue);
    void SetDriverRegValue(LPWSTR ValueName, DWORD Value);
	BOOL GetDriverRegString(LPWSTR ValueName, LPCWSTR& outputStr);

// Gain-related APIs
public:
    DWORD       GetOutputGain (void);
    MMRESULT    SetOutputGain (DWORD dwVolume);
    DWORD       GetInputGain (void);
    MMRESULT    SetInputGain (DWORD dwVolume);

    BOOL        GetOutputMute (void);
    MMRESULT    SetOutputMute (BOOL fMute);
    BOOL        GetInputMute (void);
    MMRESULT    SetInputMute (BOOL fMute);

	MMRESULT	SetInputMux(BOOL bMic);

	BOOL		GetMicBoost (void);
	MMRESULT	SetMicBoost (BOOL fMicBoost);

protected:
    void UpdateOutputGain();
    void UpdateInputGain();
	void UpdateMicBoost();
    DWORD m_dwOutputGain;
    DWORD m_dwInputGain;
    BOOL  m_fInputMute;
    BOOL  m_fOutputMute;
	BOOL  m_fMicBoost;

public:
    BOOL IsSupportedOutputFormat(LPWAVEFORMATEX lpFormat);

protected:

    DWORD    m_dwBusNumber;
    INTERFACE_TYPE m_IfcType;

    ULONG m_nVolume;        // Current HW Playback Volume

    CAIC23 *m_CAIC23;		// Pointer to the AIC23 codec
	HANDLE	m_hMCBSPDevice; // Handle to the MCBSP driver

	BOOL	RegisterDirectMemoryTransferCallbacks();
	BOOL	UnregisterDirectMemoryTransferCallbacks();
};

void CallInterruptThread(HardwareContext *pHWContext);

extern HardwareContext *g_pHWContext;


