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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

// Define how much to shift the output after applying volume
#define VOLSHIFT (32-BITSPERSAMPLE)

class StreamContext
{
public:
    LIST_ENTRY  m_Link;               // Link into list of streams

    StreamContext();
    virtual ~StreamContext();

    LONG AddRef();
    LONG Release();

    virtual DWORD Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);
    virtual DWORD Close();
    virtual DWORD GetPos(PMMTIME pmmt);

    virtual DWORD Run();
    virtual DWORD Stop();
    virtual DWORD Reset();
    virtual PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus)=0;

    // Command to reset info derived from devicecontext/hardware
    virtual void ResetBaseInfo() = 0;

    BOOL StillPlaying();
    DWORD GetByteCount();
    WAVEFORMATEX *GetWaveFormat();
    DeviceContext *GetDeviceContext();

    void DoDriverCallback(UINT msg, DWORD dwParam1, DWORD dwParam2);
    virtual void DoCallbackReturnBuffer(LPWAVEHDR lpHdr);
    virtual void DoCallbackStreamOpened();
    virtual void DoCallbackStreamClosed();

    virtual DWORD QueueBuffer(LPWAVEHDR lpWaveHdr);
    PBYTE GetNextBuffer();

    // Default implementation
    void ReturnBuffer(LPWAVEHDR lpHdr);
    DWORD GetGain();
    DWORD SetGain(DWORD dwGain);
    DWORD SetSecondaryGainClass(DWORD GainClass);

    DWORD MapGain(DWORD Gain, DWORD Channel);

    virtual void GainChange();

    static void ClearBuffer(PBYTE pStart, PBYTE pEnd);

    DWORD BreakLoop();

    DWORD ForceSpeaker (BOOL bForceSpeaker);

    virtual BOOL IsExclusive();

    DWORD GetProperty(PWAVEPROPINFO pPropInfo);
    DWORD SetProperty(PWAVEPROPINFO pPropInfo);

protected:
    LONG            m_RefCount;
    BOOL            m_bRunning;         // Is stream running or stopped

    DWORD           m_dwFlags;          // allocation flags
    HWAVE           m_hWave;            // handle for stream
    DRVCALLBACK*    m_pfnCallback;      // client's callback
    DWORD           m_dwInstance;       // client's instance data

    WAVEFORMATEX    m_WaveFormat;       // Format of wave data

    LPWAVEHDR   m_lpWaveHdrHead;
    LPWAVEHDR   m_lpWaveHdrCurrent;
    LPWAVEHDR   m_lpWaveHdrTail;
    PBYTE       m_lpCurrData;            // position in current buffer
    PBYTE       m_lpCurrDataEnd;         // end of current buffer

    DWORD       m_dwByteCount;          // byte count since last reset

    DeviceContext *m_pDeviceContext;   // Device which this stream renders to

    // Loopcount shouldn't really be here, since it's really for wave output only, but it makes things easier
    DWORD       m_dwLoopCount;          // Number of times left through loop

    DWORD       m_dwGain;
    DWORD       m_SecondaryGainClass;
    DWORD       m_fxpGain[2];

    BOOL        m_bForceSpeaker;
};

class WaveStreamContext : public StreamContext
{
public:
    DWORD Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);

    DWORD GetRate(DWORD *pdwMultiplier);
    virtual DWORD SetRate(DWORD dwMultiplier);
    PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
    virtual PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus)=0;

protected:
    PCM_TYPE m_SampleType;       // Enum of sample type, e.g. M8, M16, S8, S16
    ULONG    m_SampleSize;       // # of bytes per sample in client buffer
    DWORD    m_dwMultiplier;
    LONG     m_PrevSamp[MAXCHANNELS];
    LONG     m_CurrSamp[MAXCHANNELS];

    LONG     m_CurrPos;
    DWORD    m_ClientRate;
    DWORD    m_ClientRateInv;
};

class InputStreamContext : public WaveStreamContext
{
public:
    DWORD Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);
    DWORD Stop();   // On input, stop has special handling
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
    void DoCallbackReturnBuffer(LPWAVEHDR lpHdr);
    void DoCallbackStreamOpened();
    void DoCallbackStreamClosed();
    void ResetBaseInfo();
};

class OutputStreamContext : public WaveStreamContext
{
public:
    DWORD Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);
    DWORD Reset();
    void ResetBaseInfo();
};

class OutputStreamContextM8 : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
};
class OutputStreamContextM16 : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
};
class OutputStreamContextS8 : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
};
class OutputStreamContextS16 : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
};
class OutputStreamContextMC : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
};

class OutputStreamContextEncodedSPDIF : public OutputStreamContext
{
public:
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast, BYTE BytesToSkip, TRANSFER_STATUS *pTransferStatus);
    DWORD SetRate(DWORD dwMultiplier);
    BOOL IsExclusive();
};


