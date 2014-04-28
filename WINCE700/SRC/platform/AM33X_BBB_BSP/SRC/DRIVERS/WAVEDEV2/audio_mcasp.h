//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  audio_mcasp.h
//
//  Defines the class for the McASP audio interface  
//

#ifndef __AUDIO_MCASP_H_
#define __AUDIO_MCASP_H_

#include "mcasp.h"

class AudioDma;
class TLV320AIC3106CodecConfig;

class McASP
{
public:
    McASP(TLV320AIC3106CodecConfig *pCodec, AudioDma *pDmaTx, AudioDma *pDmaRx, PMCASPREGS pMcASPRegs, 
                McaspHwSetup *pHwSetup, McaspHwSetupData* pXmtSetup, McaspHwSetupData* pRcvSetup);
    ~McASP();
    BOOL Init();
    void StartAudio(WAV_AUDIO_CHANNEL channel);
    void StopAudio(WAV_AUDIO_CHANNEL channel);
    void HandleInterrupt();
    void HandleInterruptRx();
private:
    AudioDma *m_pDmaTx;
    AudioDma *m_pDmaRx;
    TLV320AIC3106CodecConfig *m_pCodec;
    PMCASPREGS m_pMcASPRegs;
    UINT m_RefCount;
    UINT m_RecRefCount;
    UINT m_PlayRefCount;
    McaspHwSetup *m_pHwSetup;
    McaspHwSetupData *m_pXmtSetup;
    McaspHwSetupData *m_pRcvSetup;
};


#endif // __AUDIO_MCASP_H_
