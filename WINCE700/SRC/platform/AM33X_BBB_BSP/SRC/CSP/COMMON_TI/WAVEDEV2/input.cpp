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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include "wavemain.h"

DWORD InputStreamContext::Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags)
{
    DWORD mmRet;

    mmRet = WaveStreamContext::Open(pDeviceContext, lpWOD, dwFlags);

    // Init m_CurrPos to force us to read the first sample
    m_CurrPos = -(LONG)m_ClientRate;

    return mmRet;
}

void InputStreamContext::DoCallbackReturnBuffer(LPWAVEHDR lpHdr)
{
    DoDriverCallback(WIM_DATA,(DWORD)lpHdr,0);
}

void InputStreamContext::DoCallbackStreamOpened()
{
    DoDriverCallback(WIM_OPEN,0,0);
}

void InputStreamContext::DoCallbackStreamClosed()
{
    DoDriverCallback(WIM_CLOSE,0,0);
}

DWORD InputStreamContext::Stop()
{
    // Stop the stream
    WaveStreamContext::Stop();

    // Return any partially filled buffers to the client
    if ((m_lpWaveHdrCurrent) && (m_lpWaveHdrCurrent->dwBytesRecorded>0))
    {
        GetNextBuffer();
    }

    return MMSYSERR_NOERROR;
}

#if (INCHANNELS==2)
PBYTE InputStreamContext::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTranferStatus)
{
	UNREFERENCED_PARAMETER(pBufferLast);
	UNREFERENCED_PARAMETER(pTranferStatus);

    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD ClientRateInv = m_ClientRateInv;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();

    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;

    LONG fxpGain[2];
    fxpGain[0] = m_fxpGain[0];
    fxpGain[1] = m_fxpGain[1];

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG CurrSamp1 = m_CurrSamp[1];
    LONG PrevSamp1 = m_PrevSamp[1];

    for (;;)
    {
        // Make sure we have a place to put the data
        if (pCurrData>=pCurrDataEnd)
        {
            goto Exit;
        }

        // Get the next sample
        while (CurrPos < 0)
        {
            if (pBuffer>=pBufferEnd)
            {
                goto Exit;
            }

            CurrPos += ClientRate;

            PrevSamp0 = CurrSamp0;
            PrevSamp1 = CurrSamp1;

            CurrSamp0 = ((HWSAMPLE *)pBuffer)[0];
            CurrSamp1 = ((HWSAMPLE *)pBuffer)[1];
            pBuffer += 2*sizeof(HWSAMPLE);
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * ClientRateInv)>>17;

        CurrPos -= BaseRate;

        LONG InSamp0;
        LONG InSamp1;

        // Calc difference between samples. Note InSamp0 is a 17-bit signed number now.
        InSamp0 = PrevSamp0 - CurrSamp0;
        InSamp1 = PrevSamp1- CurrSamp1;

        // Now interpolate
        InSamp0 = (InSamp0 * Ratio) >> 15;
        InSamp1 = (InSamp1 * Ratio) >> 15;

        // Add to previous sample
        InSamp0 += CurrSamp0;
        InSamp1 += CurrSamp1;

        // Apply input gain
        InSamp0 = (InSamp0 * fxpGain[0]) >> 16;
        InSamp1 = (InSamp1 * fxpGain[1]) >> 16;

        PPCM_SAMPLE pSampleDest = (PPCM_SAMPLE)pCurrData;
        switch (m_SampleType)
        {
        case PCM_TYPE_M8:
        default:
            pSampleDest->m8.sample = (UINT8)( ((InSamp0+InSamp1) >> 9) + 128);
            pCurrData  += 1;
            break;

        case PCM_TYPE_S8:
            pSampleDest->s8.sample_left  = (UINT8)((InSamp0 >> 8) + 128);
            pSampleDest->s8.sample_right = (UINT8)((InSamp1 >> 8) + 128);
            pCurrData  += 2;
            break;

        case PCM_TYPE_M16:
            pSampleDest->m16.sample = (INT16)((InSamp0+InSamp1)>>1);
            pCurrData  += 2;
            break;

        case PCM_TYPE_S16:
            pSampleDest->s16.sample_left  = (INT16)InSamp0;
            pSampleDest->s16.sample_right = (INT16)InSamp1;
            pCurrData  += 4;
            break;
        }
    }

Exit:
    m_lpWaveHdrCurrent->dwBytesRecorded += (pCurrData-m_lpCurrData);
    m_dwByteCount += (pCurrData-m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    m_PrevSamp[1] = PrevSamp1;
    m_CurrSamp[1] = CurrSamp1;
    return pBuffer;
}
#else
PBYTE InputStreamContext::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD ClientRateInv = m_ClientRateInv;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();

    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;

    LONG fxpGain = m_fxpGain[0];

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG InSamp0;
    LONG InSamp1;

    PCM_TYPE SampleType = m_SampleType;

    for (;;)
    {
        // Make sure we have a place to put the data
        if (pCurrData>=pCurrDataEnd)
        {
            goto Exit;
        }
        // Get the next sample
        while (CurrPos < 0)
        {
            if (pBuffer>=pBufferEnd)
            {
                goto Exit;
            }

            CurrPos += ClientRate;

            PrevSamp0 = CurrSamp0;
            CurrSamp0 = *(HWSAMPLE *)pBuffer;
            pBuffer += sizeof(HWSAMPLE);
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * ClientRateInv)>>17;

        CurrPos -= BaseRate;

        // Calc difference between samples. Note InSamp0 is a 17-bit signed number now.
        InSamp0 += PrevSamp0 - CurrSamp0;

        // Now interpolate
        InSamp0 = (InSamp0 * Ratio) >> 15;

        // Add to previous sample
        InSamp0 += CurrSamp0;

        // Apply input gain?
        InSamp0 = (InSamp0 * fxpGain) >> 16;

        InSamp1 = InSamp0;

        PPCM_SAMPLE pSampleDest = (PPCM_SAMPLE)pCurrData;
        switch (m_SampleType)
        {
        case PCM_TYPE_M8:
        default:
            pSampleDest->m8.sample = (UINT8)((InSamp0 >> 8) + 128);
            pCurrData  += 1;
            break;

        case PCM_TYPE_S8:
            pSampleDest->s8.sample_left  = (UINT8)((InSamp0 >> 8) + 128);
            pSampleDest->s8.sample_right = (UINT8)((InSamp1 >> 8) + 128);
            pCurrData  += 2;
            break;

        case PCM_TYPE_M16:
            pSampleDest->m16.sample = (INT16)InSamp0;
            pCurrData  += 2;
            break;

        case PCM_TYPE_S16:
            pSampleDest->s16.sample_left  = (INT16)InSamp0;
            pSampleDest->s16.sample_right = (INT16)InSamp1;
            pCurrData  += 4;
            break;
        }
    }

Exit:
    m_lpWaveHdrCurrent->dwBytesRecorded += (pCurrData-m_lpCurrData);
    m_dwByteCount += (pCurrData-m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}
#endif

void InputStreamContext::ResetBaseInfo()
{
    return;
}


