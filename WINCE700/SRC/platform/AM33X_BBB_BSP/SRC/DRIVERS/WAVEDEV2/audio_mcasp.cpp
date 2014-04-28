//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  audio_mcasp.cpp
//
//  Implementation of the McASP audio interface
//

#include <wtypes.h>
#include <windows.h>
#include <windev.h>
#include <wavedbg.h>
#include "bsp_cfg.h"
#include <am33x.h>
#include "audiodma.h"
#include "tlv320aic3106.h"
#include "wavemain.h"
#include "audio_mcasp.h"
#include "oemsettings.h"
#include "am33x_config.h"

#if PROFILE_ENABLE
#define MCASP_TIME_ARRAY_SIZE  800
DWORD mcasp_next_time = 0;
LARGE_INTEGER   mcaspIntrTimeHighRes[MCASP_TIME_ARRAY_SIZE];
#endif


#define MAX_NUMBER_SERIALIZER     6 

// See also corresponding tx/rx serializer config in hwctxt.cpp/g_McaspHwSetup
#define SERIALIZER_XMTER          SERIALIZER_2
#define SERIALIZER_RCVER          SERIALIZER_3

//==============================================================================
//!  \fn      McASP
//!  \brief  Constructor
//!  \param  void
//!  \return    
//========================================================================
McASP::McASP(TLV320AIC3106CodecConfig *pCodec, AudioDma *pDmaTx,AudioDma *pDmaRx, PMCASPREGS pMcASPRegs,
                     McaspHwSetup *pHwSetup, McaspHwSetupData* pXmtSetup,McaspHwSetupData* pRcvSetup)
                                                                                : m_pCodec(pCodec),
                                                                                 m_pDmaTx(pDmaTx),
                                                                                 m_pDmaRx(pDmaRx),
                                                                                 m_pMcASPRegs(pMcASPRegs),
                                                                                 m_RefCount(0),
                                                                                 m_pHwSetup(pHwSetup),
                                                                                 m_pXmtSetup(pXmtSetup),
                                                                                 m_pRcvSetup(pRcvSetup),
                                                                                 m_RecRefCount(0),
                                                                                 m_PlayRefCount(0)

{

}

//==============================================================================
//!  \fn      McASP
//!  \brief  Destructor
//!  \param  void
//!  \return    
//========================================================================
McASP::~McASP()
{

}

//==============================================================================
//!  \fn      Init
//!  \brief  Initialises the McASP h/w
//!  \param  None
//!  \return  TRUE if successful, FALSE otherwise  
//========================================================================
BOOL McASP::Init()
{
    DEBUGMSG(ZONE_INIT, (_T("McASP::Init: Initialising McASP\r\n")));
    
    // Initialise the McASP hardware
    mcaspHwSetup(m_pMcASPRegs, m_pHwSetup, MAX_NUMBER_SERIALIZER); 
       
    // Configure the transmit channels
    mcaspResetXmt(m_pMcASPRegs);
    
    mcaspConfigXmtSection(m_pMcASPRegs, m_pXmtSetup);

    // Enable the XMT serialisers
    mcaspSetSerXmt(m_pMcASPRegs, (McaspSerializerNum)(SERIALIZER_XMTER));
    
    // Receive channel
    // Configure the transmit channels
    mcaspResetRcv(m_pMcASPRegs);
    
    mcaspConfigRcvSection(m_pMcASPRegs, m_pRcvSetup);

    // Enable the RCV serialisers
    mcaspSetSerRcv(m_pMcASPRegs, (McaspSerializerNum)(SERIALIZER_RCVER));

    return TRUE;
}

//==============================================================================
//!  \fn      StartAudio
//!  \brief  Starts audio playback
//!  \param  void
//!  \return    
//========================================================================
void McASP::StartAudio(WAV_AUDIO_CHANNEL channel)
{
    if(channel == WAV_PLAY_CHAN)
    {
        if (m_PlayRefCount == 0)
        {
            UINT32 aCnt, bCnt;
            // Audio not already started, so start
            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StartAudioTX: Starting McASP\r\n")));   

            // re-align the buffer
            m_pDmaTx->GetActiveCount(&aCnt, &bCnt);
            if (bCnt % 2)
                m_pDmaTx->SetEvent();

            // Start DMA
            m_pDmaTx->Start();

            // Clear status
            m_pMcASPRegs->XSTAT = 0x1FF;

            if (m_RecRefCount == 0)
                mcaspActivateXmtClkSer(m_pMcASPRegs);

            mcaspResetSmFsXmt(m_pMcASPRegs);

            // Enable the XMT serialisers
            mcaspSetSerXmt(m_pMcASPRegs, (McaspSerializerNum)(SERIALIZER_XMTER));

            mcaspSerXmtEn(m_pMcASPRegs);

            // Start transmit state machine & frame sync
            mcaspActivateSmFsXmt(m_pMcASPRegs);

            // Enable McASP interrupts
            m_pMcASPRegs->XINTCTL = 0x00000007;

            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StartAudioTX: McASP started\r\n")));        

        }
        m_PlayRefCount ++;
    }
    else if(channel == WAV_REC_CHAN)
    {
        if (m_RecRefCount == 0)
        {
            // Audio not already started, so start
            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StartAudioRX: Starting McASP\r\n")));   
            // Start DMA
            m_pDmaRx->Start();

            // Clear status
            m_pMcASPRegs->RSTAT = 0x1FF;

            if(m_PlayRefCount == 0)
                mcaspActivateXmtClkSer(m_pMcASPRegs);

            // Enable the RCV serialisers
            mcaspSetSerRcv(m_pMcASPRegs, (McaspSerializerNum)(SERIALIZER_RCVER));

            mcaspSerRcvEn(m_pMcASPRegs);

            // Start receive state machine
            mcaspActivateSmFsRcv(m_pMcASPRegs);

            if(m_PlayRefCount == 0)
                mcaspActivateSmFsXmt(m_pMcASPRegs);

            // Enable McASP interrupts
            m_pMcASPRegs->RINTCTL = 0x00000007;

            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StartAudioRX: McASP started\r\n")));        

        }
        m_RecRefCount ++;
    }
}

//==============================================================================
//!  \fn      StopAudio
//!  \brief  Stops audio playback
//!  \param  void
//!  \return    
//========================================================================
void McASP::StopAudio(WAV_AUDIO_CHANNEL channel)
{
    if(channel == WAV_PLAY_CHAN)
    {
        m_PlayRefCount --;

        if (m_PlayRefCount == 0)
        {
            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StopAudio: Stopping McASP\r\n")));
            // No streams left playing audio, so stop the McASP
            m_pMcASPRegs->XINTCTL = 0x00000000;

            if (m_RecRefCount == 0) {

                mcaspResetSmFsXmt(m_pMcASPRegs);
                mcaspResetXmt(m_pMcASPRegs);

            }

            // Stop DMA
            m_pDmaTx->Stop();    
        }
    }
    else if(channel == WAV_REC_CHAN)
    {
        m_RecRefCount --;

        if (m_RecRefCount == 0)
        {
            DEBUGMSG(ZONE_FUNCTION, (_T("McASP::StopAudio: Stopping McASP\r\n")));
            // No streams left playing audio, so stop the McASP
            m_pMcASPRegs->RINTCTL = 0x00000000;
            mcaspResetSmFsRcv(m_pMcASPRegs);
            mcaspResetRcv(m_pMcASPRegs);

#if !CODEC_IS_MASTER
            if(m_PlayRefCount == 0)
            {
                mcaspResetSmFsXmt(m_pMcASPRegs);
                mcaspResetXmt(m_pMcASPRegs);
            }
#endif
            // Stop DMA
            m_pDmaRx->Stop();    
        }
    }
}

//==============================================================================
//!  \fn      HandleInterrupt
//!  \brief  Handles interrupts generated by the McASP
//!  \param  void
//!  \return    
//========================================================================
void McASP::HandleInterrupt()
{
    UINT32 status;
    BOOL bReset = FALSE;
    
    // Read status from McASP
    status = m_pMcASPRegs->XSTAT;
    if (status & 0x100)
    {
        // Error bit set. Query error
        if (status & 0x1)
        {
            // Buffer under-run
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPTX::HandleInterrupt: Buffer underun.\r\n")));
            bReset = TRUE;   
#if PROFILE_ENABLE
        if(mcasp_next_time >= MCASP_TIME_ARRAY_SIZE)
            mcasp_next_time = 0;

        QueryPerformanceCounter(&(mcaspIntrTimeHighRes[mcasp_next_time]));
        mcasp_next_time++;
#endif
        }
        if (status & 0x2)
        {
            // Frame sync error
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPTX::HandleInterrupt: Un-expected frame sync.\r\n")));   
        }
        if (status & 0x4)
        {
            // Clock failure
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPTX::HandleInterrupt: Clock failure.\r\n")));   
        }
        if (status & 0x80)
        {
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPTX::HandleInterrupt: DMA error.\r\n")));
        }
            
        // Clear status bits
        m_pMcASPRegs->XSTAT = 0x1FF;
        
        if (bReset)
        { 
            UINT32 aCnt, bCnt;
            
            m_pDmaTx->GetActiveCount(&aCnt, &bCnt);
            
            // If the buffer underrun occurs on an on transfer then
            // we need to throw away one channels worth of data in
            // order to re-align the buffer before resetting the McASP
            if (bCnt % 2)
            {
                 
                m_pDmaTx->SetEvent();
                m_pDmaTx->GetActiveCount(&aCnt, &bCnt);
            }
            
            // Reset serialiser and state machine
            m_pMcASPRegs->XGBLCTL = 0x0;
            
            mcaspActivateXmtClkSer(m_pMcASPRegs);
                    
            // Enable the XMT serialiser
            mcaspSetSerXmt(m_pMcASPRegs, (McaspSerializerNum)SERIALIZER_XMTER);
            mcaspSerXmtEn(m_pMcASPRegs);
            // Start transmit state machine
            mcaspActivateSmFsXmt(m_pMcASPRegs);
        }
    }
}

//==============================================================================
//!  \fn      HandleInterruptRx
//!  \brief  Handles interrupts generated by the McASP
//!  \param  void
//!  \return    
//========================================================================
void McASP::HandleInterruptRx()
{
    UINT32 status;   
 
    status = m_pMcASPRegs->RSTAT;
    if (status & 0x100)
    {
        // Error bit set. Query error
        if (status & 0x1)
        {
            // Buffer under-run
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPRX::HandleInterrupt: Buffer overun.\r\n")));
        }
        if (status & 0x2)
        {
            // Frame sync error
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPRX::HandleInterrupt: Un-expected frame sync.\r\n")));   
        }
        if (status & 0x4)
        {
            // Clock failure
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPRX::HandleInterrupt: Clock failure.\r\n")));   
        }
        if (status & 0x80)
        {
            DEBUGMSG (ZONE_WARN, (TEXT("WARNING: McASPRX::HandleInterrupt: DMA error.\r\n")));
        }
            
        // Clear status bits
        m_pMcASPRegs->RSTAT = 0x1FF;
    }
}
