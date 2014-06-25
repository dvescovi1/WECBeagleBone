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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include "wavemain.h"

DeviceContext::DeviceContext()
{
    InitializeListHead(&m_StreamList);
    m_dwGain = MAX_GAIN;
    m_dwDefaultStreamGain = MAX_GAIN;
    m_bExclusiveStream = FALSE;
    for (int i=0;i<SECONDARYGAINCLASSMAX;i++)
    {
        m_dwSecondaryGainLimit[i]=MAX_GAIN;
    }
}

DWORD DeviceContext::GetGain()
{
    return m_dwGain;
}

DWORD DeviceContext::SetGain(DWORD dwGain)
{
    m_dwGain = dwGain;
    RecalcAllGains();
    return MMSYSERR_NOERROR;
}

DWORD DeviceContext::GetDefaultStreamGain()
{
    return m_dwDefaultStreamGain;
}

DWORD DeviceContext::SetDefaultStreamGain(DWORD dwGain)
{
    m_dwDefaultStreamGain = dwGain;
    return MMSYSERR_NOERROR;
}

DWORD DeviceContext::GetSecondaryGainLimit(DWORD GainClass)
{
    return m_dwSecondaryGainLimit[GainClass];
}

DWORD DeviceContext::SetSecondaryGainLimit(DWORD GainClass, DWORD Limit)
{
    if (GainClass>=SECONDARYGAINCLASSMAX)
    {
        return MMSYSERR_ERROR;
    }
    m_dwSecondaryGainLimit[GainClass]=Limit;
    RecalcAllGains();
    return MMSYSERR_NOERROR;
}

BOOL DeviceContext::IsSupportedFormat(LPWAVEFORMATEX lpFormat)
{
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM)
        return FALSE;

    if (  (lpFormat->nChannels<1) || (lpFormat->nChannels>2) )
        return FALSE;

    if (  (lpFormat->wBitsPerSample!=8) && (lpFormat->wBitsPerSample!=16) )
        return FALSE;

    if (lpFormat->nSamplesPerSec < 100 || lpFormat->nSamplesPerSec > 192000)
        return FALSE;

    return TRUE;
}

// We also support MIDI on output
BOOL OutputDeviceContext::IsSupportedFormat(LPWAVEFORMATEX lpFormat)
{

#if ENABLE_MIDI
    if (lpFormat->wFormatTag == WAVE_FORMAT_MIDI)
    {
        return TRUE;
    }
#endif

    // Give HW a chance to decide as well
    if (g_pHWContext->IsSupportedOutputFormat(lpFormat))
    {
        return TRUE;
    }

    return DeviceContext::IsSupportedFormat(lpFormat);
}

// Assumes lock is taken
DWORD DeviceContext::NewStream(StreamContext *pStreamContext)
{
    if (pStreamContext->IsExclusive())
    {
        if (m_bExclusiveStream)
        {
            return MMSYSERR_ALLOCATED;
        }

        // Exclusive streams are inserted at the head of the list so they can block other streams
        // further down in the list (e.g. by setting the pTransferStatus->Mute bit)
        m_bExclusiveStream = TRUE;
        InsertHeadList(&m_StreamList,&pStreamContext->m_Link);
    }
    else
    {
        InsertTailList(&m_StreamList,&pStreamContext->m_Link);
    }

    return MMSYSERR_NOERROR;
}

// Assumes lock is taken
void DeviceContext::DeleteStream(StreamContext *pStreamContext)
{
    if (pStreamContext->IsExclusive())
    {
        m_bExclusiveStream = FALSE;
    }

    RemoveEntryList(&pStreamContext->m_Link);
}

// Returns # of samples of output buffer filled
// Assumes that g_pHWContext->Lock already held.
PBYTE DeviceContext::TransferBuffer(PBYTE pBuffer, PBYTE pBufferEnd, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus)
{
    PLIST_ENTRY pListEntry;
    StreamContext *pStreamContext;
    PBYTE pBufferLastThis;
    PBYTE pBufferLast=pBuffer;
    DWORD NumStreams=0;

    pListEntry = m_StreamList.Flink;
    while (pListEntry != &m_StreamList)
    {
        // Get a pointer to the stream context
        pStreamContext = CONTAINING_RECORD(pListEntry,StreamContext,m_Link);

        // Note: The stream context may be closed and removed from the list inside
        // of Render, and the context may be freed as soon as we call Release.
        // Therefore we need to grab the next Flink first in case the
        // entry disappears out from under us.
        pListEntry = pListEntry->Flink;

        // Render buffers
        pStreamContext->AddRef();
        pBufferLastThis = pStreamContext->Render(pBuffer, pBufferEnd, pBufferLast, BytesToSkip, pTransferStatus);
        pStreamContext->Release();
        if (pBufferLastThis>pBuffer)
        {
            NumStreams++;
        }
        if (pBufferLast < pBufferLastThis)
        {
            pBufferLast = pBufferLastThis;
        }
    }

    if (pTransferStatus)
    {
        pTransferStatus->NumStreams=NumStreams;
    }
    return pBufferLast;
}

void DeviceContext::RecalcAllGains()
{
    PLIST_ENTRY pListEntry;
    StreamContext *pStreamContext;

    for (pListEntry = m_StreamList.Flink;
        pListEntry != &m_StreamList;
        pListEntry = pListEntry->Flink)
    {
        pStreamContext = CONTAINING_RECORD(pListEntry,StreamContext,m_Link);
        pStreamContext->GainChange();
    }
    return;
}

void OutputDeviceContext::StreamReadyToRender(StreamContext *pStreamContext)
{
    g_pHWContext->Lock();
    g_pHWContext->StartAudioOutput();
    g_pHWContext->Unlock();
    return;
}

void InputDeviceContext::StreamReadyToRender(StreamContext *pStreamContext)
{
    g_pHWContext->Lock();
    g_pHWContext->StartAudioInput();
    g_pHWContext->Unlock();

    return;
}

DWORD OutputDeviceContext::GetDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEOUTCAPS wc =
    {
        MM_MICROSOFT,
        24,
        0x0001,
        TEXT("Audio Output"),
        WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08 |
        WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 |
        WAVE_FORMAT_1M16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M16 |
        WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16,
        OUTCHANNELS,
        0,
        WAVECAPS_VOLUME | WAVECAPS_PLAYBACKRATE
#if !(MONO_GAIN)
        | WAVECAPS_LRVOLUME
#endif
    };

    if (dwSize>sizeof(wc))
    {
        dwSize=sizeof(wc);
    }

    memcpy( pCaps, &wc, dwSize);
    return MMSYSERR_NOERROR;
}

DWORD InputDeviceContext::GetDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEINCAPS wc =
    {
        MM_MICROSOFT,
        23,
        0x0001,
        TEXT("Audio Input"),
        WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08 |
        WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 |
        WAVE_FORMAT_1M16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M16 |
        WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16,
        INCHANNELS,
        0
    };

    if (dwSize>sizeof(wc))
    {
        dwSize=sizeof(wc);
    }

    memcpy( pCaps, &wc, dwSize);

    return MMSYSERR_NOERROR;
}

DWORD OutputDeviceContext::GetExtDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEOUTEXTCAPS wec =
    {
#if USE_OS_MIXER
        0x1,                                // max number of hw-mixed streams
        0x1,                                // available HW streams
#else
        0xFFFF,                             // max number of hw-mixed streams
        0xFFFF,                             // available HW streams
#endif
        0,                                  // preferred sample rate for software mixer (0 indicates no preference)
        0,                                  // preferred buffer size for software mixer (0 indicates no preference)
        0,                                  // preferred number of buffers for software mixer (0 indicates no preference)
        8000,                               // minimum sample rate for a hw-mixed stream
        48000                               // maximum sample rate for a hw-mixed stream
    };

    if (dwSize>sizeof(wec))
    {
        dwSize=sizeof(wec);
    }

    memcpy( pCaps, &wec, dwSize);
    return MMSYSERR_NOERROR;
}

DWORD InputDeviceContext::GetExtDevCaps(LPVOID pCaps, DWORD dwSize)
{
    return MMSYSERR_NOTSUPPORTED;
}

StreamContext *InputDeviceContext::CreateStream(LPWAVEOPENDESC lpWOD)
{
    return new InputStreamContext;
}

StreamContext *OutputDeviceContext::CreateStream(LPWAVEOPENDESC lpWOD)
{
    LPWAVEFORMATEX lpFormat=lpWOD->lpFormat;

    if (lpFormat->wBitsPerSample==8)
    {
        if (lpFormat->nChannels==1)
        {
            return new OutputStreamContextM8;
        }
        else if (lpFormat->nChannels==2)
        {
            return new OutputStreamContextS8;
        }
    }
    else if (lpFormat->wBitsPerSample==16)
    {
        if (lpFormat->nChannels==1)
        {
            return new OutputStreamContextM16;
        }
        else if (lpFormat->nChannels==2)
        {
            return new OutputStreamContextS16;
        }
    }

    return NULL;
}

DWORD DeviceContext::OpenStream(LPWAVEOPENDESC lpWOD, DWORD dwFlags, StreamContext **ppStreamContext)
{
    DWORD mmRet = MMSYSERR_NOERROR;

    StreamContext *pStreamContext;

    if (lpWOD->lpFormat==NULL)
    {
        mmRet = WAVERR_BADFORMAT;
        goto Exit;
    }

    if (!IsSupportedFormat(lpWOD->lpFormat))
    {
        mmRet = WAVERR_BADFORMAT;
        goto Exit;
    }

    // Query format support only - don't actually open device?
    if (dwFlags & WAVE_FORMAT_QUERY)
    {
        mmRet = MMSYSERR_NOERROR;
        goto Exit;
    }

    pStreamContext = CreateStream(lpWOD);
    if (!pStreamContext)
    {
        mmRet = MMSYSERR_NOMEM;
        goto Exit;
    }

    mmRet = pStreamContext->Open(this,lpWOD,dwFlags);
    if (MMSYSERR_NOERROR != mmRet)
    {
        delete pStreamContext;
        goto Exit;
    }

    *ppStreamContext=pStreamContext;

Exit:
    return mmRet;
}

void DeviceContext::SetBaseSampleRate(DWORD BaseSampleRate)
{
    m_BaseSampleRate = BaseSampleRate;

    // Also calculate inverse sample rate, in .32 fixed format,
    // with 1 added at bottom to ensure round up.
    m_BaseSampleRateInverse = ((UINT32)(((1i64<<32)/BaseSampleRate)));

    ResetBaseInfo();

    return;
}

void DeviceContext::ResetBaseInfo()
{
    // Iterate through any existing streams and let them know that something has changed
    PLIST_ENTRY pListEntry;
    StreamContext *pStreamContext;

    for (pListEntry = m_StreamList.Flink;
        pListEntry != &m_StreamList;
        pListEntry = pListEntry->Flink)
    {
        pStreamContext = CONTAINING_RECORD(pListEntry,StreamContext,m_Link);
        pStreamContext->ResetBaseInfo();
    }

    return;
}

DWORD DeviceContext::GetBaseSampleRate()
{
    return m_BaseSampleRate;
}

DWORD DeviceContext::GetBaseSampleRateInverse()
{
    return m_BaseSampleRateInverse;
}

DWORD OutputDeviceContext::GetChannels()
{
    return OUTCHANNELS;
}

DWORD InputDeviceContext::GetChannels()
{
    return INCHANNELS;
}

DWORD DeviceContext::GetNativeFormat(ULONG Index, LPVOID pData, ULONG cbData, ULONG *pcbReturn)
{
    // This driver only supports one native format
    if (Index!=0)
    {
        return MMSYSERR_ERROR;
    }

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample = BITSPERSAMPLE;
    wfx.nChannels = (WORD)GetChannels();
    wfx.nSamplesPerSec = GetBaseSampleRate();
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nChannels;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;

    wfx.cbSize = 0;

    ULONG BytesToCopy = min(sizeof(wfx),cbData);

    if (!CeSafeCopyMemory(pData, &wfx, BytesToCopy))
    {
        return  MMSYSERR_INVALPARAM;
    }

    *pcbReturn=BytesToCopy;

    return MMSYSERR_NOERROR;
}

// Surround with ifndef in case this is ever added to mmsystem.h in a future OS release
#ifndef MM_PROPSET_GETNATIVEFORMAT
#define MM_PROPSET_GETNATIVEFORMAT { 0x40c42894, 0x7114, 0x489a, { 0xaf, 0xf3, 0xc6, 0x88, 0xba, 0x76, 0x74, 0x21 } }
#endif

DWORD DeviceContext::GetProperty(PWAVEPROPINFO pPropInfo)
{
    DWORD dwRet = MMSYSERR_NOTSUPPORTED;

    static const GUID guid_GetNativeFormat = MM_PROPSET_GETNATIVEFORMAT;
    if (IsEqualGUID(*pPropInfo->pPropSetId, guid_GetNativeFormat))
    {
        dwRet = GetNativeFormat(pPropInfo->ulPropId, pPropInfo->pvPropData, pPropInfo->cbPropData, pPropInfo->pcbReturn);
    }

    return dwRet;
}

DWORD DeviceContext::SetProperty(PWAVEPROPINFO pPropInfo)
{
    DWORD dwRet = MMSYSERR_NOTSUPPORTED;

    return dwRet;
}


