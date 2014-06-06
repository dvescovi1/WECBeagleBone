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

DWORD OutputStreamContext::Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags)
{
    DWORD mmRet;

    mmRet = WaveStreamContext::Open(pDeviceContext, lpWOD, dwFlags);

    // Init m_CurrPos to force us to read the first sample
    m_CurrPos = -(LONG)m_pDeviceContext->GetBaseSampleRate();

    if (mmRet==MMSYSERR_NOERROR)
    {
        // Note: Output streams should be initialized in the run state.
        Run();
    }

    return mmRet;
}

DWORD OutputStreamContext::Reset()
{
    HRESULT Result;

    Result = WaveStreamContext::Reset();

    if (Result==MMSYSERR_NOERROR)
    {
        // Note: Output streams should be reset to the run state.
        Run();
    }

    return Result;
};

void OutputStreamContext::ResetBaseInfo()
{
    return;
}


PBYTE OutputStreamContextM8::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain[2];

    if (pTransferStatus->Mute)
    {
        fxpGain[0] = 0;
        fxpGain[1] = 0;
    }
    else
    {
        fxpGain[0] = m_fxpGain[0];
        fxpGain[1] = m_fxpGain[1];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 = (LONG)pSampleSrc->m8.sample;
            CurrSamp0 = (CurrSamp0 - 128) << 8;
            pCurrData+=1;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;

#if (OUTCHANNELS==2)
        LONG OutSamp1;
        OutSamp1 = OutSamp0;
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        OutSamp1 = (OutSamp1 * fxpGain[1]) >> VOLSHIFT;
        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
            OutSamp1 += ((HWSAMPLE *)pBuffer)[1];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
            if (OutSamp1>AUDIO_SAMPLE_MAX)
            {
                OutSamp1=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp1<AUDIO_SAMPLE_MIN)
            {
                OutSamp1=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        ((HWSAMPLE *)pBuffer)[1] = (HWSAMPLE)OutSamp1;
        pBuffer += 2*sizeof(HWSAMPLE);
#else
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        pBuffer += sizeof(HWSAMPLE);
#endif
    }

    Exit:

    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}

PBYTE OutputStreamContextM16::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain[2];

    if (pTransferStatus->Mute)
    {
        fxpGain[0] = 0;
        fxpGain[1] = 0;
    }
    else
    {
        fxpGain[0] = m_fxpGain[0];
        fxpGain[1] = m_fxpGain[1];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 = (LONG)pSampleSrc->m16.sample;
            pCurrData+=2;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;

#if (OUTCHANNELS==2)
        LONG OutSamp1;
        OutSamp1=OutSamp0;
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        OutSamp1 = (OutSamp1 * fxpGain[1]) >> VOLSHIFT;
        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
            OutSamp1 += ((HWSAMPLE *)pBuffer)[1];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
            if (OutSamp1>AUDIO_SAMPLE_MAX)
            {
                OutSamp1=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp1<AUDIO_SAMPLE_MIN)
            {
                OutSamp1=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        ((HWSAMPLE *)pBuffer)[1] = (HWSAMPLE)OutSamp1;
        pBuffer += 2*sizeof(HWSAMPLE);
#else
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        pBuffer += sizeof(HWSAMPLE);
#endif
    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}

#if (OUTCHANNELS==2)
PBYTE OutputStreamContextS8::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG CurrSamp1 = m_CurrSamp[1];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG PrevSamp1 = m_PrevSamp[1];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain[2];

    if (pTransferStatus->Mute)
    {
        fxpGain[0] = 0;
        fxpGain[1] = 0;
    }
    else
    {
        fxpGain[0] = m_fxpGain[0];
        fxpGain[1] = m_fxpGain[1];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;
            PrevSamp1 = CurrSamp1;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 =  (LONG)pSampleSrc->s8.sample_left;
            CurrSamp0 = (CurrSamp0 - 128) << 8;
            CurrSamp1 = (LONG)pSampleSrc->s8.sample_right;
            CurrSamp1 = (CurrSamp1 - 128) << 8;
            pCurrData+=2;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;
        LONG OutSamp1;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;
        OutSamp1 = PrevSamp1 - CurrSamp1;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;
        OutSamp1 = (OutSamp1 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;
        OutSamp1 += CurrSamp1;

        // Gain
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        OutSamp1 = (OutSamp1 * fxpGain[1]) >> VOLSHIFT;

        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
            OutSamp1 += ((HWSAMPLE *)pBuffer)[1];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
            if (OutSamp1>AUDIO_SAMPLE_MAX)
            {
                OutSamp1=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp1<AUDIO_SAMPLE_MIN)
            {
                OutSamp1=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        ((HWSAMPLE *)pBuffer)[1] = (HWSAMPLE)OutSamp1;

        pBuffer += 2*sizeof(HWSAMPLE);

    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_PrevSamp[1] = PrevSamp1;
    m_CurrSamp[0] = CurrSamp0;
    m_CurrSamp[1] = CurrSamp1;
    return pBuffer;
}

PBYTE OutputStreamContextS16::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG CurrSamp1 = m_CurrSamp[1];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG PrevSamp1 = m_PrevSamp[1];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain[2];

    if (pTransferStatus->Mute)
    {
        fxpGain[0] = 0;
        fxpGain[1] = 0;
    }
    else
    {
        fxpGain[0] = m_fxpGain[0];
        fxpGain[1] = m_fxpGain[1];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;
            PrevSamp1 = CurrSamp1;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 = (LONG)pSampleSrc->s16.sample_left;
            CurrSamp1 = (LONG)pSampleSrc->s16.sample_right;
            pCurrData+=4;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;
        LONG OutSamp1;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;
        OutSamp1 = PrevSamp1 - CurrSamp1;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;
        OutSamp1 = (OutSamp1 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;
        OutSamp1 += CurrSamp1;

        // Gain
        OutSamp0 = (OutSamp0 * fxpGain[0]) >> VOLSHIFT;
        OutSamp1 = (OutSamp1 * fxpGain[1]) >> VOLSHIFT;

        if (pBuffer < pBufferLast)
        {
            OutSamp0 += ((HWSAMPLE *)pBuffer)[0];
            OutSamp1 += ((HWSAMPLE *)pBuffer)[1];
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
            if (OutSamp1>AUDIO_SAMPLE_MAX)
            {
                OutSamp1=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp1<AUDIO_SAMPLE_MIN)
            {
                OutSamp1=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        ((HWSAMPLE *)pBuffer)[0] = (HWSAMPLE)OutSamp0;
        ((HWSAMPLE *)pBuffer)[1] = (HWSAMPLE)OutSamp1;

        pBuffer += 2*sizeof(HWSAMPLE);
    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_PrevSamp[1] = PrevSamp1;
    m_CurrSamp[0] = CurrSamp0;
    m_CurrSamp[1] = CurrSamp1;
    return pBuffer;
}

#else

PBYTE OutputStreamContextS8::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain;
    if (pTransferStatus->Mute)
    {
        fxpGain = 0;
    }
    else
    {
        fxpGain = m_fxpGain[0];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 =  (LONG)pSampleSrc->s8.sample_left;
            CurrSamp0 += (LONG)pSampleSrc->s8.sample_right;
            CurrSamp0 = (CurrSamp0 - 256) << 7;
            pCurrData+=2;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;

        // Gain
        OutSamp0 = (OutSamp0 * fxpGain) >> VOLSHIFT;

        if (pBuffer < pBufferLast)
        {
            OutSamp0 += *(HWSAMPLE *)pBuffer;
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        *(HWSAMPLE *)pBuffer = (HWSAMPLE)OutSamp0;

        pBuffer += sizeof(HWSAMPLE);
    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}

PBYTE OutputStreamContextS16::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain;

    if (pTransferStatus->Mute)
    {
        fxpGain = 0;
    }
    else
    {
        fxpGain = m_fxpGain[0];
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            PrevSamp0 = CurrSamp0;

            PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
            CurrSamp0 =  (LONG)pSampleSrc->s16.sample_left;
            CurrSamp0 += (LONG)pSampleSrc->s16.sample_right;
            CurrSamp0 = CurrSamp0>>1;
            pCurrData+=4;
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        CurrPos -= ClientRate;

        LONG OutSamp0;

        // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
        OutSamp0 = PrevSamp0 - CurrSamp0;

        // Now interpolate
        OutSamp0 = (OutSamp0 * Ratio) >> 15;

        // Add to previous number
        OutSamp0 += CurrSamp0;

        // Gain
        OutSamp0 = (OutSamp0 * fxpGain) >> VOLSHIFT;

        if (pBuffer < pBufferLast)
        {
            OutSamp0 += *(HWSAMPLE *)pBuffer;
#if USE_MIX_SATURATE
            // Handle saturation
            if (OutSamp0>AUDIO_SAMPLE_MAX)
            {
                OutSamp0=AUDIO_SAMPLE_MAX;
            }
            else if (OutSamp0<AUDIO_SAMPLE_MIN)
            {
                OutSamp0=AUDIO_SAMPLE_MIN;
            }
#endif
        }
        *(HWSAMPLE *)pBuffer = (HWSAMPLE)OutSamp0;

        pBuffer += sizeof(HWSAMPLE);

    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}
#endif

PBYTE OutputStreamContextMC::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    LONG CurrPos = m_CurrPos;
    DWORD ClientRate = m_ClientRate;
    DWORD BaseRate = m_pDeviceContext->GetBaseSampleRate();
    DWORD BaseRateInv = m_pDeviceContext->GetBaseSampleRateInverse();

    LONG CurrSamp[2];
    LONG PrevSamp[2];
    LONG OutSamp[2];
    LONG fxpGain[2];

    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG nChannels = m_WaveFormat.nChannels;

    int i;

    for (i=0;i<2;i++)
    {
        CurrSamp[i] = m_CurrSamp[i];
        PrevSamp[i] = m_PrevSamp[i];

        if (pTransferStatus->Mute)
        {
            fxpGain[i] = 0;
        }
        else
        {
            fxpGain[i] = m_fxpGain[i];
        }
    }

    while (pBuffer < pBufferEnd)
    {
        while (CurrPos < 0)
        {
            if (pCurrData>=pCurrDataEnd)
            {
                goto Exit;
            }

            CurrPos += BaseRate;

            for (i=0;i<2;i++)
            {
                PrevSamp[i]=CurrSamp[i];
                CurrSamp[i]=0;
            }


            for (i=0;i<nChannels;i++)
            {
                PPCM_SAMPLE pSampleSrc = (PPCM_SAMPLE)pCurrData;
                CurrSamp[i&1] += (LONG)pSampleSrc->m16.sample;
                pCurrData+=2;
            }
        }

        // Calculate ratio between samples as a 17.15 fraction
        // (Only use 15 bits to avoid overflow on next multiply)
        LONG Ratio;
        Ratio = (CurrPos * BaseRateInv)>>17;

        for (i=0;i<2;i++)
        {
            // Calc difference between samples. Note OutSamp0 is a 17-bit signed number now.
            OutSamp[i] = PrevSamp[i] - CurrSamp[i];

            // Now interpolate
            OutSamp[i] = (OutSamp[i] * Ratio) >> 15;

            // Add to previous number
            OutSamp[i] += CurrSamp[i];

            // Gain
            OutSamp[i] = (OutSamp[i] * fxpGain[i]) >> VOLSHIFT;
            // DEBUGMSG(1, (TEXT("PrevSamp0=0x%x, CurrSamp0=0x%x, CurrT=0x%x, OutSamp0=0x%x\r\n"), PrevSamp0,CurrSamp0,CurrT,OutSamp0));

            if (pBuffer < pBufferLast)
            {
                OutSamp[i] += ((HWSAMPLE *)pBuffer)[i];
    #if USE_MIX_SATURATE
                // Handle saturation
                if (OutSamp[i]>AUDIO_SAMPLE_MAX)
                {
                    OutSamp[i]=AUDIO_SAMPLE_MAX;
                }
                else if (OutSamp[i]<AUDIO_SAMPLE_MIN)
                {
                    OutSamp[i]=AUDIO_SAMPLE_MIN;
                }
    #endif
            }
            ((HWSAMPLE *)pBuffer)[i] = (HWSAMPLE)OutSamp[i];
        }

        pBuffer += 2*sizeof(HWSAMPLE);

        CurrPos -= ClientRate;
    }

    Exit:
    m_dwByteCount += (pCurrData - m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrPos = CurrPos;
    for (i=0;i<2;i++)
    {
        m_PrevSamp[i] = PrevSamp[i];
        m_CurrSamp[i] = CurrSamp[i];
    }

    return pBuffer;
}

PBYTE OutputStreamContextEncodedSPDIF::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, TRANSFER_STATUS *pTransferStatus)
{
    DWORD dwSrcBytes = m_lpCurrDataEnd - m_lpCurrData;
    DWORD dwDstBytes = pBufferEnd - pBuffer;
    DWORD dwBytesToCopy = min(dwSrcBytes,dwDstBytes);

    memcpy(pBuffer,m_lpCurrData,dwBytesToCopy);

    m_dwByteCount += dwBytesToCopy;
    m_lpCurrData += dwBytesToCopy;

    pBufferLast = pBuffer+dwBytesToCopy;

    // Send a note to hardware that the current buffer has SPDIF compressed data
    pTransferStatus->Encoded = TRUE;

    // Force other streams to mute their outputs for this DMA buffer
    pTransferStatus->Mute = TRUE;

    return pBufferLast;
}

BOOL OutputStreamContextEncodedSPDIF::IsExclusive()
{
    return TRUE;
}

// We don't support setting the rate on SPDIF encoded outputs, do we?
DWORD OutputStreamContextEncodedSPDIF::SetRate(DWORD dwMultiplier)
{
    return (dwMultiplier == 0x10000) ? MMSYSERR_NOERROR : MMSYSERR_NOTSUPPORTED ;
}

