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
/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    HWCTXT.CPP

Abstract:               Platform dependent code for the mixing audio driver.

Notes:                  The following file contains all the hardware specific code
for the mixing audio driver.  This code's primary responsibilities
are:

* Initialize audio hardware (including codec chip)
* Schedule DMA operations (move data from/to buffers)
* Handle audio interrupts

All other tasks (mixing, volume control, etc.) are handled by the "upper"
layers of this driver.
*/

#include <windows.h>
#include <types.h>
#include <memory.h>
#include <excpt.h>
#include <wavepdd.h>
#include <waveddsi.h>
#include <wavedbg.h>
#include <nkintr.h>
#include <ceddk.h>
#include <memtxapi.h>
#include <omap.h>
#include <mcbsptypes.h>

#include "aic23.h"
#include "wavemain.h"

//--------------------------------------------------------------------------------

EXTERN_C int PopulateTxBuffer(void* pStart, void* pData, unsigned int dwLength);
EXTERN_C int PopulateRxBuffer(void* pStart, void* pData, unsigned int dwLength);
EXTERN_C int TxCommand(ExternalDrvrCommand cmd, void* pData, PortConfigInfo_t* pPortConfigInfo);
EXTERN_C int RxCommand(ExternalDrvrCommand cmd, void* pData, PortConfigInfo_t* pPortConfigInfo);
EXTERN_C int Mutex(BOOL bLock, DWORD dwTime, void* pData);

HardwareContext *g_pHWContext=NULL;

#define MAX_STRING_SIZE				16
#define REGISTRY_I2CBR_INDEX_STR	(L"I2CBaudrateIndex")
#define REGISTRY_I2CBUS_STR			(L"I2CBus")
#define REGISTRY_I2CADDR_STR		(L"I2CAddress")
#define REGISTRY_PORTDRIVER_STR		(L"PortDriver")

//--------------------------------------------------------------------------------

BOOL HardwareContext::CreateHWContext(DWORD Index)
{
    if (g_pHWContext)
    {
        return TRUE;
    }

    g_pHWContext = new HardwareContext;
    if (!g_pHWContext)
    {
        return FALSE;
    }

    return g_pHWContext->Init(Index);
}

HardwareContext::HardwareContext():
m_CAIC23(NULL),
m_hMCBSPDevice(NULL),
m_pCallbackData(NULL),
m_fnRxCommand(NULL),
m_fnTxCommand(NULL)
{
    InitializeCriticalSection(&m_Lock);
    m_Initialized=FALSE;
}

HardwareContext::~HardwareContext()
{
    DeleteCriticalSection(&m_Lock);
}

BOOL HardwareContext::Init(DWORD Index)
{
	DWORD dwI2Cbus, dwI2Caddr, dwI2CBrIndex;
	wchar_t deviceStr[MAX_STRING_SIZE] = L"";

    if (m_Initialized)
    {
        return FALSE;
    }

    //----- 1. Initialize the state/status variables -----
    m_DriverIndex       = Index;
    m_InPowerHandler    = FALSE;
    m_InputDMARunning   = FALSE;
    m_OutputDMARunning  = FALSE;
    m_NumForcedSpeaker  = 0;

    m_dwOutputGain = 0xFFFFFFFF;
    m_dwInputGain  = 0xFFFFFFFF;
    m_fInputMute  = FALSE;
    m_fOutputMute = FALSE;
	m_fMicBoost = FALSE;

	// Get I2C Baudrate index 
	dwI2CBrIndex = GetDriverRegValue(REGISTRY_I2CBR_INDEX_STR ,0);

	// Get I2C bus parameter
	dwI2Cbus = GetDriverRegValue(REGISTRY_I2CBUS_STR ,1);

	// Get I2C address parameter
	dwI2Caddr = GetDriverRegValue(REGISTRY_I2CADDR_STR ,0);

	// Get MCSBSP port driver paramete
	if (!GetDriverRegString(REGISTRY_PORTDRIVER_STR, (LPCWSTR&)deviceStr))
	{
		ERRMSG("PDD_AudioInitialize: Failed to get MCBSP port from the registry");
		goto cleanup;
	}

	// Open the MCBSP driver
	m_hMCBSPDevice = CreateFile(deviceStr,
								0,0,NULL,
								OPEN_EXISTING,
								0,NULL);

	if ((m_hMCBSPDevice == NULL) || (m_hMCBSPDevice == INVALID_HANDLE_VALUE))
	{
        ERRMSG((L"CAIC23::InitHardware: Failed to open %s driver", deviceStr));
		goto cleanup;
	}

	// Instantiate the codec
	m_CAIC23 = new CAIC23();

	// Start hardware initialization of the driver
	if (!m_CAIC23->InitHardware(dwI2CBrIndex, dwI2Cbus, dwI2Caddr))
	{
		ERRMSG("PDD_AudioInitialize: Failed to init codec hardware");
		goto cleanup;
	}

	// Initialize default sample rate
	m_InputDeviceContext.SetBaseSampleRate(INPUT_SAMPLERATE);
	m_OutputDeviceContext.SetBaseSampleRate(OUTPUT_SAMPLERATE);

	// Initialize config info for the MCBSP driver
	m_PlayPortConfigInfo.numOfChannels = 2;
	m_PlayPortConfigInfo.portProfile = kMcBSPProfile_I2S_Master;
	m_PlayPortConfigInfo.requestedChannels[0] = 1;
	m_PlayPortConfigInfo.requestedChannels[1] = 0;
	m_PlayPortConfigInfo.requestedChannels[2] = 1;
	m_PlayPortConfigInfo.requestedChannels[3] = 0;
	m_RecPortConfigInfo.numOfChannels = 2;
	m_RecPortConfigInfo.portProfile = kMcBSPProfile_I2S_Master;
	m_RecPortConfigInfo.requestedChannels[0] = 0;
	m_RecPortConfigInfo.requestedChannels[1] = 1;
	m_RecPortConfigInfo.requestedChannels[2] = 0;
	m_RecPortConfigInfo.requestedChannels[3] = 1;

	// Register MCBSP callbacks
	if (!RegisterDirectMemoryTransferCallbacks())
	{
        ERRMSG(L"CAIC23::InitHardware: Failed to register callbacks to the MCBSP driver");
		goto exit;
	}

	// Initialize the mixer controls in mixerdrv.cpp
    InitMixerControls();

    m_Initialized=TRUE;

	goto exit;

cleanup:

	Deinit();

exit:

    return m_Initialized;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               Deinit()

Description:    Deinitializest the hardware: disables DMA channel(s),
                                clears any pending interrupts, powers down the audio
                                codec chip, etc.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
BOOL HardwareContext::Deinit()
{
	UnregisterDirectMemoryTransferCallbacks();

	if (m_hMCBSPDevice != NULL)
	{
		CloseHandle(m_hMCBSPDevice);
		m_hMCBSPDevice = NULL;
	}

	if (m_CAIC23 != NULL)
	{
		delete m_CAIC23;
		m_CAIC23 = NULL;
	}

    return TRUE;
}

inline void HardwareContext::UpdateOutputGain()
{
	m_CAIC23->SetOutputVolume(TRUE,  m_fOutputMute ? 0 : (UINT16) (m_dwOutputGain        & 0xFFFF));
	m_CAIC23->SetOutputVolume(FALSE, m_fOutputMute ? 0 : (UINT16)((m_dwOutputGain >> 16) & 0xFFFF));
    m_OutputDeviceContext.SetGain(m_fOutputMute ? 0 : m_dwOutputGain);
}

inline void HardwareContext::UpdateInputGain()
{
	m_CAIC23->SetInputVolume(TRUE,  m_fOutputMute ? 0 : (UINT16) (m_dwInputGain        & 0xFFFF));
	m_CAIC23->SetInputVolume(FALSE, m_fOutputMute ? 0 : (UINT16)((m_dwInputGain >> 16) & 0xFFFF));
    m_InputDeviceContext.SetGain(m_fInputMute ? 0 : m_dwInputGain);
}

inline void HardwareContext::UpdateMicBoost()
{
	m_CAIC23->SetMicBoost(m_fMicBoost);
}

MMRESULT HardwareContext::SetOutputGain (DWORD dwGain)
{
    m_dwOutputGain = dwGain;
	UpdateOutputGain();

    return MMSYSERR_NOERROR;
}

MMRESULT HardwareContext::SetOutputMute (BOOL fMute)
{
    m_fOutputMute = fMute;
	UpdateOutputGain();

    return MMSYSERR_NOERROR;
}

DWORD HardwareContext::GetOutputGain (void)
{
    return m_dwOutputGain;
}

BOOL HardwareContext::GetOutputMute (void)
{
    return m_fOutputMute;
}

BOOL HardwareContext::GetInputMute (void)
{
    return m_fInputMute;
}

BOOL HardwareContext::GetMicBoost (void)
{
    return m_fMicBoost;
}

MMRESULT HardwareContext::SetInputMute (BOOL fMute)
{
    m_fInputMute = fMute;
	UpdateInputGain();

    return MMSYSERR_NOERROR;
}

DWORD HardwareContext::GetInputGain (void)
{
    return m_dwInputGain;
}

MMRESULT HardwareContext::SetInputGain (DWORD dwGain)
{
    m_dwInputGain = dwGain;
	UpdateInputGain();

    return MMSYSERR_NOERROR;
}

MMRESULT HardwareContext::SetMicBoost (BOOL fMicBoost)
{
	m_fMicBoost = fMicBoost;
	UpdateMicBoost();

    return MMSYSERR_NOERROR;
}

MMRESULT HardwareContext::SetInputMux (BOOL bMic)
{
	return ((m_CAIC23->SetLineInMux(bMic) == TRUE) ? MMSYSERR_NOERROR : MMSYSERR_ERROR);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StartOutputDMA()

Description:    Starts outputting the sound data to the audio codec
                                chip via DMA.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/

BOOL HardwareContext::StartOutputDMA()
{
    if (!m_OutputDMARunning)
    {
        m_OutputDMARunning = TRUE;

		// Enable DAC
		m_CAIC23->OutputEnable(TRUE);

        // Start the DMA channel
       	m_fnTxCommand(kExternalDrvrDx_Start, m_pCallbackData, (PortConfigInfo_t*)&m_PlayPortConfigInfo);
    }

    return TRUE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StopOutputDMA()

Description:    Stops any DMA activity on the output channel.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::StopOutputDMA()
{
    if (m_OutputDMARunning)
    {
        m_OutputDMARunning = FALSE;

		// Disable DAC
		m_CAIC23->OutputEnable(FALSE);

		// Stop the DMA channel
        m_fnTxCommand(kExternalDrvrDx_Stop, m_pCallbackData, NULL);
    }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StartInputDMA()

Description:    Starts inputting the recorded sound data from the
                                audio codec chip via DMA.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/

BOOL HardwareContext::StartInputDMA()
{
    if (!m_InputDMARunning)
    {
        m_InputDMARunning = TRUE;

		// Enable ADC
		m_CAIC23->InputEnable(TRUE);

		//
        // Start the DMA channel
        //
       	m_fnRxCommand(kExternalDrvrDx_Start, m_pCallbackData, (PortConfigInfo_t*)&m_PlayPortConfigInfo);
    }

    return TRUE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StopInputDMA()

Description:    Stops any DMA activity on the input channel.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::StopInputDMA()
{
    if ( m_InputDMARunning )
    {
        m_fnRxCommand(kExternalDrvrDx_Stop, m_pCallbackData, NULL);

		// Disable ADC
		m_CAIC23->InputEnable(FALSE);

		m_InputDMARunning = FALSE;
    }
}

DWORD HardwareContext::GetDriverRegValue(LPWSTR ValueName, DWORD DefaultValue)
{
    HKEY DevKey = OpenDeviceKey((LPWSTR)m_DriverIndex);

    if (DevKey)
    {
        DWORD ValueLength = sizeof(DWORD);
        RegQueryValueEx(
                       DevKey,
                       ValueName,
                       NULL,
                       NULL,
                       (PUCHAR)&DefaultValue,
                       &ValueLength);

        RegCloseKey(DevKey);
    }

    return DefaultValue;
}

BOOL HardwareContext::GetDriverRegString(LPWSTR ValueName, LPCWSTR& outputStr)
{
	HRESULT result = ERROR_BADKEY;
    HKEY DevKey = OpenDeviceKey((LPWSTR)m_DriverIndex);

    if (DevKey)
    {
        DWORD ValueLength = MAX_STRING_SIZE;
        result = RegQueryValueEx(
							   DevKey,
							   ValueName,
							   NULL,
							   NULL,
							   (PUCHAR)&outputStr,
							   &ValueLength);

        RegCloseKey(DevKey);
    }

	return ((result == ERROR_SUCCESS) ? TRUE : FALSE);
}

void HardwareContext::SetDriverRegValue(LPWSTR ValueName, DWORD Value)
{
    HKEY DevKey = OpenDeviceKey((LPWSTR)m_DriverIndex);

    if (DevKey)
    {
        RegSetValueEx(
                     DevKey,
                     ValueName,
                     NULL,
                     REG_DWORD,
                     (PUCHAR)&Value,
                     sizeof(DWORD)
                     );

        RegCloseKey(DevKey);
    }

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               PowerUp()

Description:            Powers up the audio codec chip.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::PowerUp()
{
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               PowerDown()

Description:    Powers down the audio codec chip.

Notes:                  Even if the input/output channels are muted, this
                                function powers down the audio codec chip in order
                                to conserve battery power.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::PowerDown()
{
}

//############################################ Helper Functions #############################################

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               TransferOutputBuffer()

Description:    Retrieves the next "mixed" audio buffer of data to
                                DMA into the output channel.

Returns:                Number of bytes needing to be transferred.
-------------------------------------------------------------------*/
ULONG HardwareContext::TransferOutputBuffer(PBYTE pStart, DWORD dwLength, DWORD* dwNumStreams)
{
    ULONG BytesTransferred = 0;
    PBYTE pBufferStart = pStart;
    PBYTE pBufferEnd = pBufferStart + dwLength;
    PBYTE pBufferLast;

	TRANSFER_STATUS TransferStatus = {0};

    pBufferLast = m_OutputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, &TransferStatus);
    BytesTransferred = pBufferLast-pBufferStart;
	*dwNumStreams = TransferStatus.NumStreams;

#if 1  // Enable if you need to clear the rest of the DMA buffer
    StreamContext::ClearBuffer(pBufferLast,pBufferEnd);
#endif

    return BytesTransferred;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               TransferInputBuffer()

Description:    Retrieves the chunk of recorded sound data and inputs
                                it into an audio buffer for potential "mixing".

Returns:                Number of bytes needing to be transferred.
-------------------------------------------------------------------*/
ULONG HardwareContext::TransferInputBuffer(PBYTE pStart, DWORD dwLength, DWORD* dwNumStreams)
{
    ULONG BytesTransferred = 0;
    PBYTE pBufferStart = pStart;
    PBYTE pBufferEnd = pBufferStart + dwLength;
    PBYTE pBufferLast;

	TRANSFER_STATUS TransferStatus = {0};

    pBufferLast = m_InputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, &TransferStatus);
    BytesTransferred = pBufferLast-pBufferStart;
	*dwNumStreams = TransferStatus.NumStreams;

    return BytesTransferred;
}

// Control the hardware speaker enable
void HardwareContext::SetSpeakerEnable(BOOL bEnable)
{
	m_CAIC23->OutputEnable(bEnable);
}

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// RecalcSpeakerEnable decides whether to enable the speaker or not.
// For now, it only looks at the m_bForceSpeaker variable, but it could
// also look at whether the headset is plugged in
// and/or whether we're in a voice call. Some mechanism would
// need to be implemented to inform the wave driver of changes in the state of
// these variables however.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

void HardwareContext::RecalcSpeakerEnable()
{
    SetSpeakerEnable(m_NumForcedSpeaker);
}

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// SetForceSpeaker is called from the device context to update the state of the
// m_bForceSpeaker variable.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

DWORD HardwareContext::ForceSpeaker( BOOL bForceSpeaker )
{
    // If m_NumForcedSpeaker is non-zero, audio should be routed to an
    // external speaker (if hw permits).
    if (bForceSpeaker)
    {
        m_NumForcedSpeaker++;
        if (m_NumForcedSpeaker==1)
        {
            RecalcSpeakerEnable();
        }
    }
    else
    {
        m_NumForcedSpeaker--;
        if (m_NumForcedSpeaker==0)
        {
            RecalcSpeakerEnable();
        }
    }

    return MMSYSERR_NOERROR;
}

//------------------------------------------------------------------------------
//
//  Function: PmControlMessage
//
//  Power management routine.
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

BOOL
HardwareContext::PmControlMessage (
                                  DWORD  dwCode,
                                  PBYTE  pBufIn,
                                  DWORD  dwLenIn,
                                  PBYTE  pBufOut,
                                  DWORD  dwLenOut,
                                  PDWORD pdwActualOut)
{
	UNREFERENCED_PARAMETER(pBufIn);
	UNREFERENCED_PARAMETER(dwLenIn);

	BOOL bRetVal = FALSE;

    switch (dwCode)
    {
    // Return device specific power capabilities.
    case IOCTL_POWER_CAPABILITIES:
        {
            PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pBufOut;

            // Check arguments.
            if ( ppc && (dwLenOut >= sizeof(POWER_CAPABILITIES)) && pdwActualOut )
            {
                // Clear capabilities structure.
                memset( ppc, 0, sizeof(POWER_CAPABILITIES) );

                // Set power capabilities. Supports D0 and D4.
                ppc->DeviceDx = DX_MASK(D0)|DX_MASK(D1)|DX_MASK(D2)|DX_MASK(D3)|DX_MASK(D4);

                DEBUGMSG(ZONE_FUNCTION, (TEXT("WAVE: IOCTL_POWER_CAPABILITIES = 0x%x\r\n"), ppc->DeviceDx));

                // Update returned data size.
                *pdwActualOut = sizeof(*ppc);
                bRetVal = TRUE;
            }
            else
            {
                DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: Invalid parameter.\r\n" ) ) );
            }
            break;
        }
#if (_WINCEOSVER<700)
        // Indicate if the device is ready to enter a new device power state.
    case IOCTL_POWER_QUERY:
        {
            PCEDEVICE_POWER_STATE pDxState = (PCEDEVICE_POWER_STATE)pBufOut;

            // Check arguments.
            if (pDxState && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)) && pdwActualOut)
            {
                DEBUGMSG(ZONE_FUNCTION, (TEXT("WAVE: IOCTL_POWER_QUERY = %d\r\n"), *pDxState));

                // Check for any valid power state.
                if (VALID_DX (*pDxState))
                {
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                    bRetVal = TRUE;
                }
                else
                {
                    DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: IOCTL_POWER_QUERY invalid power state.\r\n" ) ) );
                }
            }
            else
            {
                DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: IOCTL_POWER_QUERY invalid parameter.\r\n" ) ) );
            }
            break;
        }
#endif
        // Request a change from one device power state to another.
    case IOCTL_POWER_SET:
        {
            PCEDEVICE_POWER_STATE pDxState = (PCEDEVICE_POWER_STATE)pBufOut;

            // Check arguments.
            if (pDxState && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)))
            {
                DEBUGMSG(ZONE_FUNCTION, ( TEXT( "WAVE: IOCTL_POWER_SET = %d.\r\n"), *pDxState));

                // Check for any valid power state.
                if (VALID_DX(*pDxState))
                {
                    Lock();

                    // Power off.
                    if ( *pDxState == D4 )
                    {
                        PowerDown();
                    }
                    // Power on.
                    else
                    {
                        PowerUp();
                    }

                    m_DxState = *pDxState;

                    Unlock();

                    bRetVal = TRUE;
                }
                else
                {
                    DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: IOCTL_POWER_SET invalid power state.\r\n" ) ) );
                }
            }
            else
            {
                DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: IOCTL_POWER_SET invalid parameter.\r\n" ) ) );
            }

            break;
        }

        // Return the current device power state.
    case IOCTL_POWER_GET:
        {
            if (pBufOut != NULL && dwLenOut >= sizeof(CEDEVICE_POWER_STATE)) 
            {
                *(PCEDEVICE_POWER_STATE)pBufOut = m_DxState;

                bRetVal = TRUE;

                DEBUGMSG(ZONE_FUNCTION, (L"WAVE: "
                        L"IOCTL_POWER_GET to D%u \r\n",
                        m_DxState
                        ));
            }
            break;
        }

    default:
        DEBUGMSG(ZONE_WARN, (TEXT("WAVE: Unknown IOCTL_xxx(0x%0.8X) \r\n"), dwCode));
        break;
    }

    return bRetVal;
}

BOOL HardwareContext::IsSupportedOutputFormat(LPWAVEFORMATEX lpFormat)
{
	UNREFERENCED_PARAMETER(lpFormat);

    return FALSE;
}

BOOL HardwareContext::RegisterDirectMemoryTransferCallbacks()
{
	BOOL bRet = TRUE;
    EXTERNAL_DRVR_DATA_TRANSFER_IN wavDx;
    EXTERNAL_DRVR_DATA_TRANSFER_OUT wavDxOut;

    wavDx.pInData = this;
    wavDx.pfnInTxCommand = TxCommand;
    wavDx.pfnInRxCommand = RxCommand;
    wavDx.pfnInTxPopulateBuffer = PopulateTxBuffer;
    wavDx.pfnInRxPopulateBuffer = PopulateRxBuffer;
    wavDx.pfnMutexLock = Mutex;

    bRet = DeviceIoControl(m_hMCBSPDevice,
							IOCTL_EXTERNAL_DRVR_REGISTER_TRANSFERCALLBACKS,
							(UCHAR*)&wavDx,
							sizeof(EXTERNAL_DRVR_DATA_TRANSFER_IN),
							(UCHAR*)&wavDxOut,
							sizeof(EXTERNAL_DRVR_DATA_TRANSFER_OUT),
							NULL,
							NULL
							);

    if (bRet == TRUE)
    {
		m_pCallbackData = wavDxOut.pOutData;
		m_fnRxCommand = wavDxOut.pfnOutRxCommand;
		m_fnTxCommand = wavDxOut.pfnOutTxCommand;
    }

	return bRet;
}

BOOL HardwareContext::UnregisterDirectMemoryTransferCallbacks()
{
	BOOL bRet = TRUE;

    if (m_pCallbackData)
	{
        bRet = DeviceIoControl(m_hMCBSPDevice,
								IOCTL_EXTERNAL_DRVR_UNREGISTER_TRANSFERCALLBACKS,
								NULL,
								0,
								NULL,
								0,
								NULL,
								NULL
								);

        CloseHandle(m_hMCBSPDevice);

        m_pCallbackData = NULL;
        m_fnRxCommand = NULL;
        m_fnTxCommand = NULL;
        m_hMCBSPDevice = NULL;
	}

	return bRet;
}

//--------------------------------------------------------------------------------

int
TxCommand(
    ExternalDrvrCommand cmd,
    void* pData,
    PortConfigInfo_t* pPortConfigInfo
    )
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pPortConfigInfo);

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+TxCommand(cmd=%d)", cmd)
        );

    switch (cmd)
        {
        case kExternalDrvrDx_Stop:
            break;

        case kExternalDrvrDx_Start:
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-TxCommand(cmd=%d)", cmd)
        );

    return 0;
}

int
RxCommand(
    ExternalDrvrCommand cmd,
    void* pData,
    PortConfigInfo_t* pPortConfigInfo
    )
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pPortConfigInfo);

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+RxCommand(cmd=%d)", cmd)
        );

    switch (cmd)
        {
        case kExternalDrvrDx_Stop:
            break;

        case kExternalDrvrDx_Start:
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-RxCommand(cmd=%d)", cmd)
        );

    return 0;
}

int
PopulateTxBuffer(
    void* pStart,
    void* pData,
    unsigned int dwLength
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::+PopulateTxBuffer(pStart=0x%08X\r\n)",
        pStart));

    ASSERT(pData);
	HardwareContext* pHwCtxt = (HardwareContext*)pData;
	DWORD dwNumStreams = 0;
    
    if ( pHwCtxt->TransferOutputBuffer((PBYTE)pStart, dwLength, &dwNumStreams) == 0 )
    {
		pHwCtxt->StopOutputDMA();
    }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::-PopulateTxBuffer()\r\n"));

    return (int)dwNumStreams;
}

int
PopulateRxBuffer(
    void* pStart,
    void* pData,
    unsigned int dwLength
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::+PopulateRxBuffer(pStart=0x%08X\r\n)",
        pStart));

    ASSERT(pData);
	HardwareContext* pHwCtxt = (HardwareContext*)pData;
	DWORD dwNumStreams = 0;

	pHwCtxt->TransferInputBuffer((PBYTE)pStart, dwLength, &dwNumStreams);

    if ( dwNumStreams == 0 )
    {
		pHwCtxt->StopInputDMA();
    }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::-PopulateRxBuffer()\r\n"));

    return 1;
}

int
Mutex(
    BOOL bLock,
    DWORD dwTime,
    void* pData
    )
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(pData);

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+Mutex(bLock=%d)", bLock)
        );

    bLock ? EnterMutex() : ExitMutex();

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-Mutex(bLock=%d)", bLock)
        );

    return TRUE;
}
