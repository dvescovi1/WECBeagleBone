//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  audiodma.h
//
//  Defines the class for the AM33X audio dma 
//

#ifndef __AUDIODMA_H_
#define __AUDIODMA_H_

#include "edma.h"

#ifdef __cplusplus
extern "C" {
#endif

    enum {
        CHANNEL_XMIT = 1,
        CHANNEL_RCV
    };

class AudioDma
{
public:
    AudioDma(UINT32 PhysSrcAddr, UINT32 PhysDestAddr, UINT32 DmaBufSize, UINT32 nEventId, UINT32 nEventQueue, BOOL bDirection);
    ~AudioDma();
    BOOL Init(HANDLE *phDmaEvent);
    void Start();
    void Stop();
    void AudioDma::GetActiveCount(UINT32 *aCnt, UINT32 *bCnt);
    void AudioDma::SetEvent();
private:
    UINT32 m_PhysSrcAddr;
    UINT32 m_DmaBufSize;
    UINT32 m_PhysDestAddr;
    UINT32 m_nEventId;
    UINT32 m_nEventQueue;
    UINT m_ChannelId;
    UINT m_Ch2Id;
    UINT m_Ch3Id;
    EDMA3_DRV_Handle m_hEdma;
    BOOL m_Direction;
};

#ifdef __cplusplus
}
#endif


#endif // __AUDIODMA_H_
