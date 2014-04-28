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
#include "am33x.h"
#include "am33x_config.h"
#include "am33x_oal_prcm.h"
#include <oal_clock.h>
#include "wavemain.h"
#include "ceddkex.h"
#include "gpio.h"
//#include "gpiodefs.h"
//#include "psc.h"
#include "sdk_padcfg.h"

#if PROFILE_ENABLE
#define TIME_ARRAY_SIZE  800
DWORD dma_next_time = 0;
LARGE_INTEGER   dmaIntrTimeHighRes[TIME_ARRAY_SIZE];
LARGE_INTEGER   StartAudioOutputTimeHighRes[2];
ULONG           StartAudioOutputBytesTransferred[2];
ULONG           dmaIntrBytesTransferred[TIME_ARRAY_SIZE];
#endif /* DMA_PROFILE_ENABLE */


HardwareContext *g_pHWContext=NULL;

/*
  Init configuration for mcasp device - DMA mode

  See also Tx/Rx serializer no. #defines in audio_mcasp.cpp
 */

static McaspHwSetup g_McaspHwSetup = 
{
    {
        /* .pfunc    = */ 0x00000000,                /* all mcasp pin */
        /* .pdir     = */ 0x00000004,                /* only pin-2 is output */
        /* .gblctl   = */ 0x00000000,                /* ctrl clk, hclk, statem - reset,release - driver ctrl */
        /* .ditctl   = */ 0x00000000,                /* DIT mode seting */
        /* .dlbctl   = */ 0x00000000,                /* loop back mode setting */
        /* .amute    = */ 0x00000000,                /* Amute setting */
        /* .srctl0   = */ 0x00000000,                /* serialiser 0 */
        /* .srctl1   = */ 0x00000000,                /* serialiser 1 */
        /* .srctl2   = */ 0x0000000D,                /* serialiser 2 Tx */
        /* .srctl3   = */ 0x0000000E,                /* serialiser 3 Rx */
        /* .srctl4   = */ 0x00000000,                /* serialiser 4 */
        /* .srctl5   = */ 0x00000000,                /* serialiser 5 */
        /* .srctl6   = */ 0x00000000,                /* serialiser 6 */
        /* .srctl7   = */ 0x00000000,                /* serialiser 7 */
        /* .srctl8   = */ 0x00000000,                /* serialiser 8 */
        /* .srctl9   = */ 0x00000000,                /* serialiser 9 */
        /* .srctl10   = */ 0x00000000,               /* serialiser 10 */
        /* .srctl11   = */ 0x00000000,               /* serialiser 11 */
        /* .srctl12   = */ 0x00000000                /* serialiser 12 */

    },                                          

    {                                              
        /* .rmask    = */ 0xFFFFFFFF,                 
        /* .rfmt     = */ 0x00018074,
        /* .afsrctl  = */ 0x00000112,             
        /* .rtdm     = */ 0x00000003,             
        /* .rintctl  = */ 0x00000000,             
        /* .rstat    = */ 0x000001F7,             	 
        /* .revtctl  = */ 0x00000000,             
        {                                         
            /* .aclkrctl  = */ 0x000000AF,          
            /* .ahclkrctl = */ 0x00000000,          
            /* .rclkchk   = */ 0x00FF0008          
        },                                        
    },                                             
    {                                              
        /* .xmask    = */ 0xFFFFFFFF,             
        /* .xfmt     = */ 0x00018074,
        /* .afsxctl  = */ 0x00000110,             
        /* .xtdm     = */ 0x00000003,             
        /* .xintctl  = */ 0x00000000,             
        /* .xstat    = */ 0x000001F7,               
        /* .xevtctl  = */ 0x00000000,             
        {                                         
            /* .aclkxctl  = */ 0x00000080,          
            /* .ahclkxctl = */ 0x00000000,          
            /* .xclkchk   = */ 0x00FF0008             
        },                                        
    },                                             
    0x00000001              /*emu free*/
};

McaspHwSetupData g_McAspXmtSetup = 
{
    /* .xmask    = */ 0xFFFFFFFF,        // No padding used
    /* .xfmt     = */ 0x00018074,        // MSB first, 16bit, 1bit-delay, writ from DMA, Rot by 16bit pos
    /* .afsxctl  = */ 0x00000110,        // 2TDM, 1bit Rising, EXT FS, bit-width
    /* .xtdm     = */ 0x00000003,        // TDM slots 0 & 1 enabled
    /* .xintctl  = */ 0x0000000F,        // reset any existing status bits
    /* .xstat    = */ 0x000001F7,        // reset any existing status bits
    /* .xevtctl  = */ 0x00000000,        // Generate DMA events
    {                                         
         /* .aclkxctl  = */ 0x00000080,  // Rising, SYNC, EXT, /1 (ASYNC, Rising INTERNAL CLK, div-by-16)
         /* .ahclkxctl = */ 0x00000000,  // EXTERNAL CLK, div-by-1
         /* .xclkchk   = */ 0x00FF0008   // 255-MAX 0-MIN, div-by-256          
    },
};

McaspHwSetupData g_McAspRcvSetup = 
{
    /* .rmask    = */ 0xFFFFFFFF,        // No padding used
    /* .rfmt     = */ 0x00018074 ,       // MSB 16bit, 1-delay, no pad, CFGBus
    /* .afsrctl  = */ 0x00000110,        // 2TDM, 1bit Rising, INTERNAL FS, word

    /* .rtdm     = */ 0x00000003,        // Slots 0,1
    /* .rintctl  = */ 0x00000000,        // reset any existing status bits
    /* .rstat    = */ 0x000001F7,        // reset any existing status bits
    /* .revtctl  = */ 0x00000000,        // Generate DMA events
    {                                         
         /* .aclkrctl  = */ 0x0000008F,  // Rising INTERNAL CLK (from tx side)
         /* .ahclkrctl = */ 0x00000000,  // INT CLK (from tx side)
         /* .rclkchk   = */ 0x00FF0008   // 255-MAX 0-MIN, div-by-256
    },
};

// -----------------------------------------------------------------------------
//
// Function to get the SysIntr and I/O port range from the registry
//
// NOTE: lpRegPath is assumed to be under HKEY_LOCAL_MACHINE
//
// Returns ERROR_SUCCESS on success or a Win32 error code on failure
//
// -----------------------------------------------------------------------------
DWORD
HardwareContext::GetRegistryConfig(
                 LPWSTR lpRegPath,
                 DWORD * lpdwIoBase,
                 DWORD * lpdwIrqTX,                 // OUT - IRQ number, used to hook ISR handler
                 DWORD * lpdwIrqRX,                 // OUT - IRQ number, used to hook ISR handler
                 DWORD * lpdwThreadPriority,
                 DWORD * lpdwEventIdTx,
                 DWORD * lpdwEventIdRx,
                 DWORD * lpdwEdmaEventQueue
                 )
{
    HKEY hConfig = NULL;
    DWORD dwRet = ERROR_SUCCESS;

    // get a pointer to our configuration key in the registry
    hConfig = OpenDeviceKey(lpRegPath);
    if (hConfig == NULL)
    {
        DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: OpenDeviceKey('%s') failed\r\n"), lpRegPath));
        dwRet = ERROR_BADKEY;
        goto EXIT;
    }

    *lpdwIoBase = GetDriverRegValue(hConfig, L"AudioCntrlRegAddr", 0);
    *lpdwIrqTX = GetDriverRegValue(hConfig, L"IrqTx", 0);
    *lpdwIrqRX = GetDriverRegValue(hConfig, L"IrqRx", 0);
    *lpdwThreadPriority = GetDriverRegValue(hConfig, L"Priority256", 0x60);    
    *lpdwEventIdTx = GetDriverRegValue(hConfig, L"EventIdTx", 0x0A);
    *lpdwEventIdRx = GetDriverRegValue(hConfig, L"EventIdRx", 0x0B);
    *lpdwEdmaEventQueue = GetDriverRegValue(hConfig, L"EdmaEventQueue", 0);
    
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: IO Base = 0x%x\r\n"), *lpdwIoBase));
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: TX Irq = 0x%x\r\n"), *lpdwIrqTX));
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: RX Irq = 0x%x\r\n"), *lpdwIrqRX));
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: Thread priority = 0x%x\r\n"), *lpdwThreadPriority));
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: Event ID TX = 0x%x\r\n"), *lpdwEventIdTx));
    DEBUGMSG(ZONE_PDD, (_T("WAVEDEV: Event ID RX = 0x%x\r\n"), *lpdwEventIdRx));
        
EXIT:
    if (hConfig != NULL)
    {
        RegCloseKey(hConfig);
    }
    return dwRet;
}

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

HardwareContext::HardwareContext()
{   
    InitializeCriticalSection(&m_Lock);
        
    m_Initialized=FALSE;
    m_Running = TRUE;
    
    m_pMcASPRegs = NULL;
    m_pCodec = NULL;
    m_pMcASP = NULL;
    m_nVolume = 0xFFFFFFFF;
    m_fMute = FALSE;
    m_DxState = D0;
}

HardwareContext::~HardwareContext()
{
    DeleteCriticalSection(&m_Lock);
}


BOOL HardwareContext::Init(DWORD Index)
{
    UINT DeviceId;
    PHYSICAL_ADDRESS PhysicalAddress;

    if (m_Initialized)
    {
        return FALSE;
    }

    //----- 1. Initialize the state/status variables -----
    m_DriverIndex       = Index;
    m_InPowerHandler    = FALSE;
    m_AudioOutputRunning  = FALSE;
    m_AudioInputRunning  = FALSE;
    m_NumForcedSpeaker  = 0;
    m_NextOutputBuffer = 0;
    m_NextInputBuffer = 0;

    for (DeviceId = 0; DeviceId < AUDIO_NUM_OUTPUT_DEVICES; DeviceId++)
    {
        m_OutputDeviceContext[DeviceId].SetBaseSampleRate(OUTPUT_SAMPLERATE);
        m_dwOutputGain[DeviceId] = 0xFFFFFFFF;
        m_fOutputMute[DeviceId] = FALSE;
    }
    
    m_InputDeviceContext.SetBaseSampleRate(INPUT_SAMPLERATE);
    m_dwInputGain = INPUT_VOLUME_DEFAULT; //should match what is initialized in aic
    m_fInputMute = FALSE;

    DWORD dwRet;

    ULONG ulIoBase = 0;
    ULONG ulEdmaEventQueue = 0;
    
    //FUNC_WPDD("PDD_AudioInitialize");

    if ((dwRet = GetRegistryConfig((LPWSTR)Index, &ulIoBase, &m_McASPIrqTx, &m_McASPIrqRx, &m_dwInterruptThreadPriority, 
                                   &m_ulEventIdTx, &m_ulEventIdRx, &ulEdmaEventQueue))
        != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR,
                 (TEXT("WAVEDEV.DLL : GetRegistryConfig failed %d\r\n"),
                  dwRet));
        return FALSE;
    }

    // Setup McASP IO space
    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart  = ulIoBase;

    m_pMcASPRegs  = (PMCASPREGS) MmMapIoSpace(PhysicalAddress,
                                     sizeof (AM3X_MCASP_REGS), FALSE);

    if (m_pMcASPRegs == NULL)
    {
        ERRORMSG (TRUE,(_T("Unable to Map Audio Serial port Registers.\r\n")));
        return FALSE;
    }
    
    // Allocate DMA buffer
    DMA_ADAPTER_OBJECT AdapterObject;

    AdapterObject.ObjectSize = sizeof(AdapterObject);
    AdapterObject.InterfaceType = Internal;
    AdapterObject.BusNumber = 0;

    m_DMAOutBufVirtAddr = (PBYTE)HalAllocateCommonBuffer(&AdapterObject, AUDIO_DMA_BUFFER_SIZE, &m_DMAOutBufPhysAddr, FALSE);
    if (m_DMAOutBufVirtAddr == NULL)
    {
        ERRMSG("PDD_AudioInitialize: DMA TX Buffer Page Allocation Failed");
        return FALSE;
    }

    //
    // Setup dma_pages
    //
    m_DMAOutPageVirtAddr[0] = m_DMAOutBufVirtAddr;
    m_DMAOutPageVirtAddr[1] = m_DMAOutBufVirtAddr + AUDIO_DMA_PAGE_SIZE;

    // Receive buffer
    AdapterObject.ObjectSize = sizeof(AdapterObject);
    AdapterObject.InterfaceType = Internal;
    AdapterObject.BusNumber = 0;

    m_DMAInBufVirtAddr = (PBYTE)HalAllocateCommonBuffer(&AdapterObject, AUDIO_DMA_BUFFER_SIZE, &m_DMAInBufPhysAddr, FALSE);
    if (m_DMAInBufVirtAddr == NULL)
    {
        ERRMSG("PDD_AudioInitialize: DMA RX Buffer Page Allocation Failed");
        return FALSE;
    }

    //
    // Setup dma_pages
    //
    m_DMAInPageVirtAddr[0] = m_DMAInBufVirtAddr;
    m_DMAInPageVirtAddr[1] = m_DMAInBufVirtAddr + AUDIO_DMA_PAGE_SIZE;

    
    // Initialize the mixer controls in mixerdrv.cpp
    InitMixerControls();

    // Now setup the codec
    m_pCodec = new TLV320AIC3106CodecConfig();
    if (!m_pCodec)
    {
        goto Exit;
    }
    
    if (!m_pCodec->CodecInit(OUTPUT_SAMPLERATE, OUTPUT_VOLUME_MAX))
    {
        goto Exit;
    }
    
    // Setup the TX DMA
    m_pDmaTx = new AudioDma(m_DMAOutBufPhysAddr.LowPart, AM33X_MCASP1_DATA_REGS_PA,
                    AUDIO_DMA_BUFFER_SIZE, m_ulEventIdTx, ulEdmaEventQueue, CHANNEL_XMIT);
    
    if (!m_pDmaTx)
    {
        goto Exit;
    }
    
    if (!m_pDmaTx->Init(&m_hAudioInterrupt))
    {
        goto Exit;
    }
    
    // Setup the RX DMA
    m_pDmaRx = new AudioDma(AM33X_MCASP1_DATA_REGS_PA, m_DMAInBufPhysAddr.LowPart,
                    AUDIO_DMA_BUFFER_SIZE, m_ulEventIdRx, ulEdmaEventQueue, CHANNEL_RCV);
    
    if (!m_pDmaRx)
    {
        goto Exit;
    }
    
    if (!m_pDmaRx->Init(&m_hAudioInterruptRx))
    {
        goto Exit;
    }

    // Init interrupt thread
    if (!InitInterruptThread())
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init() - Failed to initialize interrupt thread.\r\n")));
        goto Exit;
    }

    // Make sure the McASP PSC is on
    PowerUp();

    // Finally setup the McASP
    m_pMcASP = new McASP(m_pCodec, m_pDmaTx, m_pDmaRx, m_pMcASPRegs, &g_McaspHwSetup, &g_McAspXmtSetup, &g_McAspRcvSetup);
    if (!m_pMcASP)
    {
        goto Exit;
    }
    
    if (!m_pMcASP->Init())
    {
        goto Exit;
    }

    // Initialisation was successful
    m_Initialized=TRUE;

Exit:
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
    DMA_ADAPTER_OBJECT AdapterObject;
    HANDLE hThreads[3] = {m_hAudioInterruptThread, m_hAudioInterruptThreadRx, m_hMcASPInterruptThread};
    //
    // The driver is being unloaded. Deinitialize. Free up memory, etc.
    //

    m_Running = FALSE; // This will cause runnng interrupt threads to
                           // exit their loop
    
    // Wake up sleeping threads
    SetEvent(m_hAudioInterrupt);
    SetEvent(m_hAudioInterruptRx);
    SetEvent(m_hMcASPInterruptEvent);
    
    // Wait for up to 1 second for threads to exit
    if (WaitForMultipleObjects(3, hThreads, TRUE, 1000) == WAIT_TIMEOUT)
    {
        DEBUGMSG(ZONE_WARN, (_T("HardwareContext::Deinit: Timed out waiting for threads to exit\r\n")));
    }    
    
    // Clean up audio objects
    if (m_pMcASP)
    {
        delete m_pMcASP;
    }
    
    if (m_pDmaTx)
    {
        delete m_pDmaTx;
    }
    
    if (m_pDmaRx)
    {
        delete m_pDmaRx;
    }
    
    if (m_pCodec)
    {
        delete m_pCodec;
    }
    
    // Free up DMA buffer
    AdapterObject.ObjectSize = sizeof(AdapterObject);
    AdapterObject.InterfaceType = Internal;
    AdapterObject.BusNumber = 0;
    HalFreeCommonBuffer(&AdapterObject, AUDIO_DMA_BUFFER_SIZE, m_DMAOutBufPhysAddr, m_DMAOutBufVirtAddr, FALSE);

    AdapterObject.ObjectSize = sizeof(AdapterObject);
    AdapterObject.InterfaceType = Internal;
    AdapterObject.BusNumber = 0;
    HalFreeCommonBuffer(&AdapterObject, AUDIO_DMA_BUFFER_SIZE, m_DMAInBufPhysAddr, m_DMAInBufVirtAddr, FALSE);

    // Free memory mapped registers
    if (m_pMcASPRegs)
    {
        MmUnmapIoSpace(m_pMcASPRegs, sizeof(AM3X_MCASP_REGS));
    }
    
    m_Initialized = FALSE;
    
    return TRUE;
}

inline void HardwareContext::UpdateOutputGain(UINT DeviceId)
{
    m_pCodec->SetOutputVolume(DeviceId, m_fOutputMute[DeviceId] ? 0 : m_dwOutputGain[DeviceId]);
}

MMRESULT HardwareContext::SetOutputGain (UINT DeviceId, DWORD dwGain)
{
    m_dwOutputGain[DeviceId] = dwGain;
    UpdateOutputGain(DeviceId);

    return MMSYSERR_NOERROR;
}

inline void HardwareContext::UpdateInputGain(void)
{
    m_pCodec->SetInputVolume(m_fInputMute ? 0 : m_dwInputGain);
}

MMRESULT HardwareContext::SetInputGain (DWORD dwGain)
{
    m_dwInputGain = dwGain;
    UpdateInputGain();

    return MMSYSERR_NOERROR;
}

MMRESULT HardwareContext::SetMasterVolume(DWORD dwVolume)
{
    m_nVolume = dwVolume;
 
    m_pCodec->SetMasterVolume((ULONG)dwVolume);
    
    return MMSYSERR_NOERROR;   
}

DWORD HardwareContext::GetMasterVolume()
{
    return m_nVolume;
}

MMRESULT HardwareContext::SetMasterMute(BOOL fMute)
{
    m_fMute = fMute;
    
    m_pCodec->SetMasterVolume(fMute ? 0: (USHORT)m_nVolume);
    
    return MMSYSERR_NOERROR;
}

BOOL HardwareContext::GetMasterMute()
{
    return m_fMute;
}

MMRESULT HardwareContext::SetOutputMute (UINT DeviceId, BOOL fMute)
{
    m_fOutputMute[DeviceId] = fMute;
    UpdateOutputGain(DeviceId);

    return MMSYSERR_NOERROR;
}

DWORD HardwareContext::GetOutputGain (UINT DeviceId)
{
    return m_dwOutputGain[DeviceId];
}

BOOL HardwareContext::GetOutputMute (UINT DeviceId)
{
    return m_fOutputMute[DeviceId];
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

BOOL HardwareContext::GetInputMute (void)
{
    return m_fInputMute;
}
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StartAudioOutput()

Description:    Starts outputting the sound data to the audio codec
                                chip.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/

BOOL HardwareContext::StartAudioOutput()
{
    BOOL bRet = FALSE;
    
    DEBUGMSG(ZONE_FUNCTION, (_T("HardwareContext::StartAudioOutput\r\n")));
    
    if (!m_AudioOutputRunning)
    {   
        //----- 1. Initialize our buffer counters -----
        m_AudioOutputRunning=TRUE;
        m_OutBytes[0]=m_OutBytes[1]=0;

        //----- 2. Prime both output buffers with as much sound data as possible -----
#if PROFILE_ENABLE
        ULONG OutputTransferred = 0;

        QueryPerformanceCounter(&(StartAudioOutputTimeHighRes[0]));
        StartAudioOutputBytesTransferred[0] = TransferOutputBuffer(0);

        QueryPerformanceCounter(&(StartAudioOutputTimeHighRes[1]));
        StartAudioOutputBytesTransferred[1] = TransferOutputBuffer(1);

        OutputTransferred = StartAudioOutputBytesTransferred[0];
        OutputTransferred += StartAudioOutputBytesTransferred[1];
#else
        ULONG OutputTransferred = TransferOutputBuffer(0);
        OutputTransferred += TransferOutputBuffer(1);
#endif

        // Set next output buffer to be filled
        m_NextOutputBuffer = 0;
        
        //----- 3. If we did transfer any data to the DMA buffers, go ahead and enable DMA -----
        if (OutputTransferred)
        {
            //
            // Start the audio
            //
            m_pMcASP->StartAudio(WAV_PLAY_CHAN);
        }
        else    // We didn't transfer any data, so DMA wasn't enabled
        {
            m_AudioOutputRunning=FALSE;
        }
        
        bRet = TRUE;
    }
    
    // Exit:
    return bRet;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StartAudioInput()

Description:    Starts inputting the recorded sound data from the
                                audio codec chip via DMA.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/

BOOL HardwareContext::StartAudioInput()
{
    BOOL bRet=FALSE;

    if (!m_AudioInputRunning)
    {
        m_AudioInputRunning=TRUE;

		m_NextInputBuffer = 0;

       //
	   // Start the audio
	   //
        m_pMcASP->StartAudio(WAV_REC_CHAN);
    }
    else
	{
	}

    bRet=TRUE;

    // Exit:
    return bRet;

}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StopAudioOutput()

Description:    Stops any DMA activity on the output channel.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::StopAudioOutput()
{

   DEBUGMSG(ZONE_FUNCTION, (_T("HardwareContext::StopAudioOutput\r\n")));
   
    if (m_AudioOutputRunning)
    {
        //$$ Set SpeakerAmplifier muting and turn off/on mute bit in control register B.
        m_pMcASP->StopAudio(WAV_PLAY_CHAN);
        m_AudioOutputRunning=FALSE;
    }
    
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               StopAudioInput()

Description:    Stops any DMA activity on the input channel.

Notes:

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::StopAudioInput()
{
    PRINTMSG(ZONE_FUNCTION, (_T("WAV2: StopAudioInput\r\n")));

    if ( m_AudioInputRunning )
    {
        m_pMcASP->StopAudio(WAV_REC_CHAN);
        m_AudioInputRunning=FALSE;
    }
    else
    {
		PRINTMSG(ZONE_FUNCTION, (_T("WAV2: StopAudioInput NOT running !\r\n")));
	}
}

DWORD HardwareContext::GetDriverRegValue(HKEY hKey, LPWSTR ValueName, DWORD DefaultValue)
{
    if (hKey)
    {
        DWORD ValueLength = sizeof(DWORD);
        if (RegQueryValueEx(
                       hKey,
                       ValueName,
                       NULL,
                       NULL,
                       (PUCHAR)&DefaultValue,
                       &ValueLength) != ERROR_SUCCESS)
        {
            DEBUGMSG(ZONE_ERROR, (_T("HardwareContext::GetDriverRegValue: Failed to get value %s\r\n"), ValueName));   
        }
    }

    return DefaultValue;
}

void HardwareContext::SetDriverRegValue(HKEY hKey, LPWSTR ValueName, DWORD Value)
{
    if (hKey)
    {
        RegSetValueEx(
                     hKey,
                     ValueName,
                     NULL,
                     REG_DWORD,
                     (PUCHAR)&Value,
                     sizeof(DWORD)
                     );

        RegCloseKey(hKey);
    }

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               InitInterruptThread()

Description:    Initializes the IST for handling DMA interrupts.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
BOOL HardwareContext::InitInterruptThread()
{
    // TX
    m_hAudioInterruptThread  = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                            0,
                                            (LPTHREAD_START_ROUTINE)CallInterruptThread,
                                            this,
                                            0,
                                            NULL);
    if (!m_hAudioInterruptThread)
    {
        return FALSE;
    }

    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(m_hAudioInterruptThread, GetInterruptThreadPriority());


    // RX
    m_hAudioInterruptThreadRx  = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                            0,
                                            (LPTHREAD_START_ROUTINE)CallInterruptThreadRx,
                                            this,
                                            0,
                                            NULL);
    if (!m_hAudioInterruptThreadRx)
    {
        return FALSE;
    }

    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(m_hAudioInterruptThreadRx, GetInterruptThreadPriority());

    // Set up audio interrupt handler
    m_hMcASPInterruptEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hMcASPInterruptEvent)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to create interrupt event.\r\n")));
        return FALSE;
    }
    
    m_hMcASPInterruptThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CallMcASPInterruptThread, this, 0, NULL);
    if (!m_hMcASPInterruptThread)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to create interrupt thread.\r\n")));
        return FALSE;
    }
    
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &m_McASPIrqTx, \
		(DWORD)sizeof(UINT32), &m_McASPSysIntr, (DWORD)sizeof(UINT32), NULL))
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to allocate audio sysintr.\r\n")));
        m_McASPSysIntr = (DWORD)SYSINTR_UNDEFINED;
        return FALSE;
    }
    
    if (!InterruptInitialize(m_McASPSysIntr, m_hMcASPInterruptEvent, 0, 0))
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): InterruptInitialize failed.\r\n")));
        return FALSE;
    }
    
    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(m_hAudioInterruptThread, GetInterruptThreadPriority());


    // Set up audio interrupt handler
    m_hMcASPInterruptEventRx = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hMcASPInterruptEventRx)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to create interrupt event.\r\n")));
        return FALSE;
    }
    
    m_hMcASPInterruptThreadRx = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CallMcASPInterruptThreadRx, this, 0, NULL);
    if (!m_hMcASPInterruptThreadRx)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to create interrupt thread.\r\n")));
        return FALSE;
    }

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &m_McASPIrqRx, \
		(DWORD)sizeof(UINT32), &m_McASPSysIntrRx, (DWORD)sizeof(UINT32), NULL))
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): Failed to allocate audio sysintr.\r\n")));
        m_McASPSysIntrRx = (DWORD)SYSINTR_UNDEFINED;
        return FALSE;
    }
    
    if (!InterruptInitialize(m_McASPSysIntrRx, m_hMcASPInterruptEventRx, 0, 0))
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("WAVEDEV.DLL:HardwareContext::Init(): InterruptInitialize failed.\r\n")));
        return FALSE;
    }
    CeSetThreadPriority(m_hMcASPInterruptThread, GetInterruptThreadPriority());
    CeSetThreadPriority(m_hMcASPInterruptThreadRx, GetInterruptThreadPriority());
    return TRUE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               PowerUp()

Description:            Powers up the audio codec chip.

Returns:                Boolean indicating success
-------------------------------------------------------------------*/
void HardwareContext::PowerUp()
{
    // Power on the codec
    m_pCodec->PowerOff(FALSE);

    // Power on the McASP
    if (FALSE == EnableDeviceClocks(AM_DEVICE_MCASP1, TRUE))
    {
        // Failed
        DEBUGMSG (ZONE_ERROR, (TEXT("HardwareContext::PowerUp: Failed to power on McASP\r\n")));
    }

    RequestDevicePads(AM_DEVICE_MCASP1);
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
    // Power off the codec
    m_pCodec->PowerOff(TRUE);
    
    // Power off the McASP
    if (FALSE == EnableDeviceClocks(AM_DEVICE_MCASP1, FALSE))
    {
        // Failed
        DEBUGMSG (ZONE_ERROR, (TEXT("HardwareContext::PowerDown: Failed to power off McASP\r\n")));
    }
}

//############################################ Helper Functions #############################################

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               TransferOutputBuffer()

Description:    Retrieves the next "mixed" audio buffer of data to
                                DMA into the output channel.

Returns:                Number of bytes needing to be transferred.
-------------------------------------------------------------------*/
ULONG HardwareContext::TransferOutputBuffer(ULONG NumBuf)
{
    ULONG BytesTransferred = 0;

    PBYTE pBufferStart;
    PBYTE pBufferEnd;
    PBYTE pBufferLast;
    UINT DeviceId;
    
    // The following line of code calculates the number of bytes
    // to skip to get to the next sample data in the DMA buffer.
    // 4 bytes (32-bits) * 2 (left + right channels) * number of output devices. 
    BYTE BytesToSkip = 4 * 2 * AUDIO_NUM_OUTPUT_DEVICES;
    
    TRANSFER_STATUS TransferStatus = {0};

    BytesTransferred = m_OutBytes[NumBuf] = 0;

    // Clear the DMA buffer to get rid of any previous left over data
    memset(m_DMAOutPageVirtAddr[NumBuf], 0, AUDIO_DMA_PAGE_SIZE);

    // Render each audio device in turn 
    for (DeviceId = 0; DeviceId < AUDIO_NUM_OUTPUT_DEVICES; DeviceId++)
    {
        // Calculate where in the buffer to write to
        // Remember 1 serializer per device, 32-bit wide serializer FIFO
        pBufferStart = m_DMAOutPageVirtAddr[NumBuf] + (DeviceId * 4);
        pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE - (BytesToSkip / 2);
        pBufferLast = m_OutputDeviceContext[DeviceId].TransferBuffer(pBufferStart, pBufferEnd, BytesToSkip,&TransferStatus);
        BytesTransferred += (pBufferLast-pBufferStart);
        m_OutBytes[NumBuf] += (pBufferLast-pBufferStart);
    }

    // Swap output buffer ready for the next transfer
    m_NextOutputBuffer = (m_NextOutputBuffer ? 0 : 1);

    return BytesTransferred;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               TransferInputBuffer()

Description:    Retrieves the chunk of recorded sound data and inputs
                                it into an audio buffer for potential "mixing".

Returns:                Number of bytes needing to be transferred.
-------------------------------------------------------------------*/
ULONG HardwareContext::TransferInputBuffer(ULONG NumBuf)
{
    ULONG BytesTransferred = 0;
    PBYTE pBufferStart = m_DMAInPageVirtAddr[NumBuf];
    PBYTE pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE;
    PBYTE pBufferLast;
	TRANSFER_STATUS TransferStatus = {0};

    // The following line of code calculates the number of bytes
    // to skip to get to the next sample data in the DMA buffer.
    // 4 bytes (32-bits) * 2 (left + right channels) *
    // number of output devices. 
    BYTE BytesToSkip = 2 * AUDIO_NUM_INPUT_DEVICES * 4;

	PRINTMSG(0, (_T("WAV2: TransInpBuff\r\n")));

    pBufferLast = m_InputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, BytesToSkip, &TransferStatus);

    BytesTransferred = pBufferLast-pBufferStart;

	// Swap input buffer ready for the next transfer
    m_NextInputBuffer = (m_NextInputBuffer ? 0 : 1);

    return BytesTransferred;
}

void HardwareContext::InterruptThreadRx()
{
    while(m_Running)
    {
        WaitForSingleObject( m_hAudioInterruptRx, INFINITE);

        Lock();

        if(!TransferInputBuffer(m_NextInputBuffer))
            StopAudioInput();

        Unlock();

    }

    CloseHandle(m_hAudioInterruptRx);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               InterruptThread()

Description:    Main audio DMA interrupt handler thread. Waits for
                DMA transfer complete interrupts and processes the next
                audio buffer

Returns:                None
-------------------------------------------------------------------*/
void HardwareContext::InterruptThread()
{
    ULONG bytesTransferred;

#if (_WINCEOSVER < 600)
    // Need to be able to access everyone else's address space during interrupt handler.
    SetProcPermissions((DWORD)-1);
#endif
    

    while (m_Running)
    {
        WaitForSingleObject(m_hAudioInterrupt, INFINITE);

#if PROFILE_ENABLE
        if(dma_next_time >= TIME_ARRAY_SIZE)
            dma_next_time = 0;

        QueryPerformanceCounter(&(dmaIntrTimeHighRes[dma_next_time]));
//        dma_next_time++;
        bytesTransferred = 0;
#endif /* DMA_PROFILE_ENABLE */

        // Grab the lock
        Lock();

        {
            ULONG OutputTransferred;

            bytesTransferred = TransferOutputBuffer(m_NextOutputBuffer);

            // For output DMA, if we stop the DMA as soon as we have no data to transfer,
            // we may cutoff unplayed data in the other buffer. Therefore, we can't stop
            // the output DMA until all output buffers are totally empty.
            OutputTransferred = m_OutBytes[0] + m_OutBytes[1];
            if (OutputTransferred == 0)
            {
                StopAudioOutput();
            }
        }

#if PROFILE_ENABLE
        dmaIntrBytesTransferred[dma_next_time] = bytesTransferred;
        dma_next_time++;
#endif

        Unlock();
    }
 
    // Finished with event   
    CloseHandle(m_hAudioInterrupt);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               McASPInterruptThread()

Description:    McASP interrupt thread. Handles interrupts generated
                by the McASP. Used to log errors generated by the
                McASP.

Returns:                None
-------------------------------------------------------------------*/
void HardwareContext::McASPInterruptThread()
{
#if (_WINCEOSVER < 600)
    // Need to be able to access everyone else's address space during interrupt handler.
    SetProcPermissions((DWORD)-1);
#endif

    DEBUGMSG(ZONE_PDD, (_T("HardwareContext::McASPInterruptThread: Running...\r\n")));
    
    while (m_Running)
    {
        WaitForSingleObject(m_hMcASPInterruptEvent, INFINITE);

        // Grab the lock
        Lock();
        
        m_pMcASP->HandleInterrupt();
        
        Unlock();
      
        InterruptDone(m_McASPSysIntr);          
    } // while(m_Running)
    
    // Finished with event
    CloseHandle(m_hMcASPInterruptEvent);
}
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:               McASPInterruptThread()

Description:    McASP interrupt thread. Handles interrupts generated
                by the McASP. Used to log errors generated by the
                McASP.

Returns:                None
-------------------------------------------------------------------*/
void HardwareContext::McASPInterruptThreadRx()
{
    DEBUGMSG(ZONE_PDD, (_T("HardwareContext::McASPInterruptThread: Running...\r\n")));
    
    while (m_Running)
    {
        WaitForSingleObject(m_hMcASPInterruptEventRx, INFINITE);

        // Grab the lock
        Lock();

        m_pMcASP->HandleInterruptRx();
        
        Unlock();
      
        InterruptDone(m_McASPSysIntrRx);  
        
    } // while(m_Running)
    
    // Finished with event
    CloseHandle(m_hMcASPInterruptEventRx);
}


void CallInterruptThread(HardwareContext *pHWContext)
{
    pHWContext->InterruptThread();
}

void CallInterruptThreadRx(HardwareContext *pHWContext)
{
    pHWContext->InterruptThreadRx();
}

void CallMcASPInterruptThread(HardwareContext *pHWContext)
{
    pHWContext->McASPInterruptThread();
}

void CallMcASPInterruptThreadRx(HardwareContext *pHWContext)
{
    pHWContext->McASPInterruptThreadRx();
}

// Control the hardware speaker enable
void HardwareContext::SetSpeakerEnable(BOOL bEnable)
{
    // Code to turn speaker on/off here
    return;
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
                ppc->DeviceDx = DX_MASK(D0)|DX_MASK(D4);

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

                    switch (*pDxState)
                    {
                    case D0:
                        // Power on.
                        PowerUp();
                        break;
                    case D1:
                    case D2:
                    case D3:
                        // Not supported. Set to next best state
                        *pDxState = D0;
                        break;
                    case D4:
                        // Power off.
                        PowerDown();
                        break;
                    default:
                        DEBUGMSG(ZONE_ERROR, ( TEXT( "WAVE: IOCTL_POWER_SET invalid power state.\r\n" ) ) );
                        break;
                    };                    

                    m_DxState = *pDxState;

                    Unlock();

                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
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
            PCEDEVICE_POWER_STATE pDxState = (PCEDEVICE_POWER_STATE)pBufOut;
            
            if (pDxState && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)) && pdwActualOut)
            {
                *pDxState = m_DxState;
                *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                DEBUGMSG(ZONE_FUNCTION, (TEXT("WAV_IOControl: IOCTL_POWER_GET = %d\r\n"), *pDxState));                
                bRetVal = TRUE;
            }
            else
            {
                DEBUGMSG(ZONE_ERROR, ( TEXT( "WAV_IOControl: IOCTL_POWER_GET invalid parameter.\r\n" ) ) );
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
    BOOL bRet = FALSE;

    // Check if WMAPro-over-S/PDIF is enabled
    if (lpFormat->nSamplesPerSec == 48000) // Ensoniq only supports 48000 data rate over S/PDIF
    {
        bRet = TRUE;
    }

    return bRet;
}


void HardwareContext::GetMcaspReg(DWORD *reg)
{
    UINT32 reg_addr;
    volatile UINT32 val;
    volatile UINT32 *pMcaspReg;

    // add offset to base
    reg_addr = (UINT32)((DWORD)m_pMcASPRegs + (*reg & 0x0000FFFF));

    pMcaspReg = (volatile UINT32 *)reg_addr;

    val = *pMcaspReg;

    // return value
    *reg = (DWORD)val;

    RETAILMSG(1, (TEXT("HardwareContext::GetMcaspReg  reg=0x%08X val=0x%08X\r\n"), pMcaspReg, val));
}


