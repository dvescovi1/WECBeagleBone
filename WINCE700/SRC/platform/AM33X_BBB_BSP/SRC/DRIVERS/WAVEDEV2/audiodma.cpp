//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  audiodma.cpp
//
//  Implementation of Audio DMA to the McASP
//

#include <wtypes.h>
#include <windows.h>
#include <windev.h>
#include <wavedbg.h>
#include "bsp_cfg.h"
#include <am33x.h>
#include "edma.h"
#include "oemsettings.h"
#include "audiodma.h"

//==============================================================================
//!  \fn      AudioDma
//!  \brief  Constructor
//!  \param  void
//!  \return    
//========================================================================

AudioDma::AudioDma(UINT32 PhysSrcAddr, UINT32 PhysDestAddr, UINT32 DmaBufSize,
                   UINT32 nEventId, UINT32 nEventQueue, BOOL bDirection) :
m_PhysSrcAddr(PhysSrcAddr),
m_PhysDestAddr(PhysDestAddr),
m_DmaBufSize(DmaBufSize),
m_hEdma(0),
m_nEventId(nEventId),
m_nEventQueue(nEventQueue),
m_Direction(bDirection)
{

}

//==============================================================================
//!  \fn      ~AudioDma
//!                     
//!                     
//!  \brief  Destructor
//!  \param  void
//!         
//!  \return    
//========================================================================
AudioDma::~AudioDma()
{
    // Close any allocated DMA channels
    EDMA3_DRV_freeChannel(m_hEdma, m_ChannelId);
    EDMA3_DRV_freeChannel(m_hEdma, m_Ch2Id);
    EDMA3_DRV_freeChannel(m_hEdma, m_Ch3Id);

    // Release EDMA3 handle
    if (m_hEdma != NULL)
    {
        EDMA3_DRV_releaseInstHandle(m_hEdma);
        m_hEdma = NULL;
    }
}

//==============================================================================
//!  \fn     Init 
//!  \brief  This function intialiazes the audio dma.
//!  \param  phDmaEvent - DMA completion event handle (out)
//!          
//!  \return    Bool
//========================================================================

BOOL AudioDma::Init(HANDLE *phDmaEvent)
{
    UINT32      TccNum =  EDMA3_DRV_TCC_ANY;
//    UINT32      TccNum1 = EDMA3_DRV_TCC_ANY;
    UINT32      HalfBlockLen=((m_DmaBufSize/2) + (m_DmaBufSize % 2)); 
    EDMA3_DRV_Result edmaResult;

    // The following line calculates how much data to write in
    // order to service all active TX serializers. 4 bytes (32-bit FIFO width) *
    // the numbder of active TX FIFOS.
    //
    // 1 McASP serialiser is enabled per
    // output device. All serializer FIFOS must be written to in response
    // to a McASP DMA event. e.g. 3 output devices (6 channel audio) will
    // require 12 bytes to be written in a single burst
    //
    // <-- A-count = 4 * AUDIO_NUM_OUTPUT_DEVICES -->
    //    | XBUF[0] | XBUF[1] | ....... | XBUF[n] |
    //     ----------------------------------------
    // B0 | 4 bytes | 4 bytes | 4 bytes | 4 bytes |
    // .. | 4 bytes | 4 bytes | 4 bytes | 4 bytes |
    // Bn | 4 bytes | 4 bytes | 4 bytes | 4 bytes |
    //
    // B-count = HalfBlockLen / A-count
    // B-index = A-count
    UINT        DataSize = 4 * AUDIO_NUM_OUTPUT_DEVICES;

    // Open EDMA3
    m_hEdma = EDMA3_DRV_getInstHandle(EDMA3_DEFAULT_PHY_CTRL_INSTANCE,
        EDMA3_ARM_REGION_ID, &edmaResult);
    if (m_hEdma == NULL || edmaResult != EDMA3_DRV_SOK)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: AudioDma::Init: EDMA3_DRV_getInstHandle() failed! Error %08X\r\n",
            edmaResult));
        return (FALSE);
    }

    m_ChannelId = m_nEventId;

    // Request DMA channel
    if (EDMA3_DRV_SOK != EDMA3_DRV_requestChannel(m_hEdma,
        &m_ChannelId,
        &TccNum,
        (EDMA3_RM_EventQueue)m_nEventQueue,
        NULL,
        NULL))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_requestChannel failed.\r\n")));
        goto cleanUp;
    }

    if(m_Direction == CHANNEL_XMIT)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams(m_hEdma,
            m_ChannelId,
            m_PhysSrcAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams (m_hEdma,
            m_ChannelId,
            m_PhysDestAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_ChannelId,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,
            m_ChannelId,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }
    else if(m_Direction == CHANNEL_RCV)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams(m_hEdma,
            m_ChannelId,
            m_PhysSrcAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams (m_hEdma,
            m_ChannelId,
            m_PhysDestAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_ChannelId,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,
            m_ChannelId,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setTransferParams (m_hEdma,
        m_ChannelId,
        DataSize,
        (HalfBlockLen / DataSize),
        1,
        0,
        EDMA3_DRV_SYNC_A))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setTransferParams failed.\r\n")));
        goto cleanUp;
    }

    // Request for a link channel
    m_Ch2Id = EDMA3_DRV_LINK_CHANNEL;

    if (EDMA3_DRV_SOK != EDMA3_DRV_requestChannel (m_hEdma,
        &m_Ch2Id,
        &TccNum,
        (EDMA3_RM_EventQueue)m_nEventQueue,
        NULL,
        NULL))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_requestChannel failed.\r\n")));
        goto cleanUp;
    }

    if(m_Direction == CHANNEL_XMIT)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams (m_hEdma,
            m_Ch2Id,
            m_PhysSrcAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams(m_hEdma,
            m_Ch2Id,
            m_PhysDestAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_Ch2Id,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,    
            m_Ch2Id,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }
    else if(m_Direction == CHANNEL_RCV)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams (m_hEdma,
            m_Ch2Id,
            m_PhysSrcAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams(m_hEdma,
            m_Ch2Id,
            m_PhysDestAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_Ch2Id,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,    
            m_Ch2Id,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setTransferParams (m_hEdma,
        m_Ch2Id,
        DataSize,
        (HalfBlockLen / DataSize),
        1,
        0,
        EDMA3_DRV_SYNC_A))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setTransferParams failed.\r\n")));
        goto cleanUp;
    }

    // Request for another link channel
    m_Ch3Id = EDMA3_DRV_LINK_CHANNEL;
    if (EDMA3_DRV_SOK != EDMA3_DRV_requestChannel (m_hEdma,
        &m_Ch3Id,
        &TccNum,
        (EDMA3_RM_EventQueue)m_nEventQueue,
        NULL,
        NULL))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_requestChannel failed.\r\n")));
        goto cleanUp;
    }

    if(m_Direction == CHANNEL_XMIT)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams (m_hEdma,
            m_Ch3Id,
            m_PhysSrcAddr+HalfBlockLen,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams (m_hEdma,
            m_Ch3Id,
            m_PhysDestAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_Ch3Id,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,
            m_Ch3Id,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }
    else if(m_Direction == CHANNEL_RCV)
    {
        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcParams (m_hEdma,
            m_Ch3Id,
            m_PhysSrcAddr,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestParams (m_hEdma,
            m_Ch3Id,
            m_PhysDestAddr+HalfBlockLen,
            EDMA3_DRV_ADDR_MODE_INCR,
            EDMA3_DRV_W32BIT))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestParams failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setSrcIndex (m_hEdma,
            m_Ch3Id,
            0,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setSrcIndex failed.\r\n")));
            goto cleanUp;
        }

        if (EDMA3_DRV_SOK != EDMA3_DRV_setDestIndex (m_hEdma,
            m_Ch3Id,
            DataSize,
            0))
        {
            DEBUGMSG (ZONE_ERROR,
                (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setDestIndex failed.\r\n")));
            goto cleanUp;
        }
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setTransferParams(m_hEdma,
        m_Ch3Id,
        DataSize,
        (HalfBlockLen / DataSize),
        1,
        0,
        EDMA3_DRV_SYNC_A))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setTransferParams failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_ChannelId,
        EDMA3_DRV_OPT_FIELD_TCC,
        TccNum))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch2Id,
        EDMA3_DRV_OPT_FIELD_TCC,
        TccNum))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch3Id,
        EDMA3_DRV_OPT_FIELD_TCC,
        TccNum))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_ChannelId,
        EDMA3_DRV_OPT_FIELD_TCCMODE,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch2Id,
        EDMA3_DRV_OPT_FIELD_TCCMODE,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch3Id,
        EDMA3_DRV_OPT_FIELD_TCCMODE,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }


    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_ChannelId,
        EDMA3_DRV_OPT_FIELD_TCINTEN,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch2Id,
        EDMA3_DRV_OPT_FIELD_TCINTEN,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    if (EDMA3_DRV_SOK != EDMA3_DRV_setOptField (m_hEdma,
        m_Ch3Id,
        EDMA3_DRV_OPT_FIELD_TCINTEN,
        1))
    {
        DEBUGMSG (ZONE_ERROR,
            (TEXT("ERROR: AudioDma::Init: EDMA3_DRV_setOptField failed.\r\n")));
        goto cleanUp;
    }

    *phDmaEvent = EDMA3_DRV_getTransferEvent(m_hEdma, m_ChannelId);

    return TRUE;

cleanUp:
    EDMA3_DRV_freeChannel(m_hEdma, m_ChannelId);
    EDMA3_DRV_freeChannel(m_hEdma, m_Ch2Id);
    EDMA3_DRV_freeChannel(m_hEdma, m_Ch3Id);

    if (m_hEdma != NULL)
    {
        EDMA3_DRV_releaseInstHandle(m_hEdma);
        m_hEdma = NULL;
    }

    return FALSE;
}

//==============================================================================
//!  \fn     Start 
//!  \brief  This function starts the audio dma.
//!  \param  void
//!          
//!  \return    Bool
//========================================================================
void AudioDma::Start()
{
    EDMA3_DRV_linkChannel (m_hEdma, m_ChannelId, m_Ch3Id);
    EDMA3_DRV_linkChannel (m_hEdma, m_Ch2Id, m_Ch3Id);
    EDMA3_DRV_linkChannel (m_hEdma, m_Ch3Id, m_Ch2Id);

    if (EDMA3_DRV_enableTransfer (m_hEdma, m_ChannelId, EDMA3_DRV_TRIG_MODE_EVENT) != EDMA3_DRV_SOK)
    {
        DEBUGMSG(ZONE_ERROR, ( TEXT("ERROR: AudioDma::Start: EDMA3_DRV_enableTransfer failed.\r\n" )));
    }
}

//==============================================================================
//!  \fn     Start 
//!  \brief  This function stops the audio dma.
//!  \param  void
//!          
//!  \return    Bool
//========================================================================
void AudioDma::Stop()
{
    EDMA3_DRV_disableTransfer(m_hEdma, m_ChannelId, EDMA3_DRV_TRIG_MODE_EVENT);
    EDMA3_DRV_unlinkChannel(m_hEdma, m_ChannelId);
}

//==============================================================================
//!  \fn     GetActiveCount 
//!  \brief  This function gets the source active A & B counts for the 
//!          current transfer
//!  \param  aCnt, bCnt
//!          
//!  \return 
//========================================================================
void AudioDma::GetActiveCount(UINT32 *aCnt, UINT32 *bCnt)
{
    unsigned int param;
    
    if (EDMA3_DRV_getPaRAMEntry(m_hEdma, m_ChannelId, EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT, &param) == EDMA3_DRV_SOK)
    {
        *aCnt = param & 0xFFFF;
        *bCnt = param >> 16;        
    }
}

//==============================================================================
//!  \fn     SetEvent 
//!  \brief  This function manually triggers an EDMA transfer 
//!          
//!  \param  void
//!          
//!  \return    
//========================================================================
void AudioDma::SetEvent()
{
    if (EDMA3_DRV_enableTransfer(m_hEdma, m_ChannelId, EDMA3_DRV_TRIG_MODE_MANUAL) != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (TEXT("AudioDma::SetEvent: Failed to set event\r\n")));
    }
}
