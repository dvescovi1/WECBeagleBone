// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File: dataport.cpp
//

#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <Pkfuncs.h>
#include <pm.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <mcbsp.h>

#include <omap.h>
#include "debug.h"
#include "dataport.h"

//------------------------------------------------------------------------------
//  Function:  DataPort_t()
//
//  Initialize member variables
//
DataPort_t::DataPort_t(
    McBSPDevice_t *pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    memset(&m_DmaInfo, 0, sizeof(DmaDataInfo_t));

    m_pDevice           = pDevice;
    m_DmaLoopCounter    = 0;
    m_PortState         = kMCBSP_Port_Uninitialized;
    m_DmaPhysAddr       = 0;
    m_pActiveDmaBuffer  = NULL;
    m_pDmaBufferStart   = NULL;
    m_pDmaBufferMiddle  = NULL;
    m_SamplesPerPage    = 0;
    m_pActiveInstance   = NULL;
    m_hEvent            = NULL;
    m_hDmaChannel       = NULL;

    InitializeCriticalSection(&m_cs);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//  Function:  ~DataPort_t()
//
//  Release all allocated resources
//
DataPort_t::~DataPort_t()
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    // free allocated resources
    //
    DeleteCriticalSection(&m_cs);

    if (m_pDmaBufferStart)
        {
        FreePhysMem(m_pDmaBufferStart);
        }

     if (m_hEvent)
        {
        CloseHandle(m_hEvent);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//  Function:  GetDataBuffer()
//
//  Retrieves the data buffer
//
BYTE*
DataPort_t::GetDataBuffer(
    BufferRequest_e type
    )
{
    BYTE* pBuffer;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(type=%d)\r\n", __FUNCTION__, type));

    switch (type)
        {
        case kBufferStart:
            pBuffer = m_pDmaBufferStart;
            break;

        case kBufferMiddle:
            pBuffer = m_pDmaBufferMiddle;
            break;

        case kBufferActive:
            pBuffer = (m_pActiveDmaBuffer == m_pDmaBufferStart) ?
                        m_pDmaBufferStart : m_pDmaBufferMiddle;
            break;

        case kBufferInactive:
            pBuffer = (m_pActiveDmaBuffer == m_pDmaBufferStart) ?
                        m_pDmaBufferMiddle : m_pDmaBufferStart;
            break;

        default:
            ASSERT(0);
            return NULL;
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(pBuffer=%d)\r\n", __FUNCTION__,
        pBuffer)
        );
    return pBuffer;
}


//------------------------------------------------------------------------------
// Function:  Initialize()
//
//  Intialize class with dma information
//
BOOL
DataPort_t::Initialize(
    DmaConfigInfo_t *pDmaConfigInfo,
    DWORD nBufferSize,
    UINT16 dmaSyncMap,
    LPTHREAD_START_ROUTINE pIstDma)
{
    BOOL bResult = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    // verify mcbsp dma mapping
    //
    if (dmaSyncMap == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"mcbsp dma mapping not specified\r\n")
            );
        goto cleanUp;
        }

    // TDM mode is sets DMA element size of 16bits.
    // I2S mode supports  DMA element size from 8 bits to 32 bits
    //
    if ((m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave) ||
        (m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Master))
        {
        // Determine DMA datatype
        //
        if (m_pDevice->wordLength > 16)
            {
            //  32 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S32;
            }
        else if (m_pDevice->wordLength > 8)
            {
            //  16 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S16;
            }
        else
            {
            //  8 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S8;
            }
        }
    else if (m_pDevice->mcbspProfile == kMcBSPProfile_TDM)
        {
        //  16 bit data type
        pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S16;
        }

    // allocate contiguous physical memory to be used with DMA
    //
    PHYSICAL_ADDRESS pa;
    pa.LowPart = 0;
    m_pDmaBufferStart = (BYTE*)AllocPhysMem(nBufferSize,
        PAGE_READWRITE | PAGE_NOCACHE, 0, 0, &pa.LowPart
        );
    if (m_pDmaBufferStart == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"Failed allocate dma buffer (size %u)\r\n", nBufferSize)
            );
        goto cleanUp;
        }

    m_DmaPhysAddr = pa.LowPart;
    m_pDmaBufferMiddle = (BYTE*)((DWORD)m_pDmaBufferStart + (nBufferSize / 2));
    m_sizeDmaBuffer = nBufferSize;

    UpdateSamplesPerPage(nBufferSize, pDmaConfigInfo->elemSize);

    // Packet burst mode for I2S mode only
    //
    if ((m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave) ||
        (m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Master))
        {
        // TX and RX CSDP settings for MCBSP FIFO packet burst
        //
        if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
            {
            // To make sure the TX CSDP configurations
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_DST_PACKED |
                DMA_CSDP_DST_BURST_64BYTES_16x32_8x64;
            }
        else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
            {
            // To make sure the RX CSDP configurations
            //
            pDmaConfigInfo->elemSize |=  DMA_CSDP_SRC_PACKED |
                DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64;
            }
        }
    else if (m_pDevice->mcbspProfile == kMcBSPProfile_TDM)
        {
        // TX and RX CSDP settings for MCBSP FIFO packet burst
        //
        if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
            {
            // To make sure the TX CSDP configurations
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_DST_PACKED |
                DMA_CSDP_DST_BURST_64BYTES_16x32_8x64;
            }
        else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
            {
            // To make sure the RX CSDP configurations
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_SRC_PACKED |
                DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64;
            }
        }

    if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
        {
        // DMA destination frame index must be equal to the TX threshold
        // value of the McBSP
        pDmaConfigInfo->dstFrameIndex = m_pDevice->fifoThresholdTx + 1;
        }
    else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
        {
        // DMA source frame index must be equal to the RX threshold
        // value of the McBSP
        pDmaConfigInfo->srcFrameIndex = m_pDevice->fifoThresholdRx + 1;
        }

    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEvent == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"Event creation failed\r\n")
            );
        goto cleanUp;
        }

    m_hDmaChannel = DmaAllocateChannel(DMA_TYPE_SYSTEM);
    if (m_hDmaChannel == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"Failed DmaAllocateChannel\r\n")
            );
        goto cleanUp;
        }

    // register dma for interrupts
    if (DmaEnableInterrupts(m_hDmaChannel, m_hEvent) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"Failed to register for interrupts\r\n")
            );
        goto cleanUp;
        }

    // configure dma
    //
    DmaConfigure(m_hDmaChannel, pDmaConfigInfo, dmaSyncMap, &m_DmaInfo);
    DmaSetElementAndFrameCount(&m_DmaInfo, (WORD)GetSamplesPerPage(), 2);
    DmaSetRepeatMode(&m_DmaInfo, TRUE);

    // spawn thread
    //
    if (NULL == CreateThread(NULL, 0, pIstDma, m_pDevice, 0, NULL))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: DataPort_t::Initialize: "
            L"Failed to create Ist Thread\r\n")
            );
        goto cleanUp;
        }

    m_PortState = kMcBSP_Port_Idle;
    bResult = TRUE;

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
    return bResult;
}

//------------------------------------------------------------------------------
// Function:  RestoreDMAcontext()
//
//  Reintialize the dma information
//
BOOL
DataPort_t::RestoreDMAcontext(
    DmaConfigInfo_t *pDmaConfigInfo,
    DWORD nBufferSize,
    UINT16 dmaSyncMap
    )
{
    BOOL bResult = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    // TDM mode is sets DMA element size of 16bits.
    // I2S mode supports  DMA element size from 8 bits to 32 bits
    //
    if ((m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave) ||
        (m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Master))
        {
        // Determine DMA datatype
        //
        if (m_pDevice->wordLength > 16)
            {
            //  32 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S32;
            }
        else if (m_pDevice->wordLength > 8)
            {
            //  16 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S16;
            }
        else
            {
            //  8 bit data type
            pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S8;
            }
        }
    else if (m_pDevice->mcbspProfile == kMcBSPProfile_TDM)
        {
        //  16 bit data type
        pDmaConfigInfo->elemSize = DMA_CSDP_DATATYPE_S16;
        }

    UpdateSamplesPerPage(nBufferSize, pDmaConfigInfo->elemSize);

    // Packet burst mode for I2S mode only
    //
    if ((m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave) ||
        (m_pDevice->mcbspProfile == kMcBSPProfile_I2S_Master))
        {
        // TX and RX CSDP settings for MCBSP FIFO packet burst
        //
        if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
            {
            // To make sure the TX CSDP configurations are not over written
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_DST_PACKED |
                DMA_CSDP_DST_BURST_64BYTES_16x32_8x64;
            }
        else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
            {
            // To make sure the RX CSDP configurations are not over written
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_SRC_PACKED |
                DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64;
            }
        }
    else if (m_pDevice->mcbspProfile == kMcBSPProfile_TDM)
        {
        // TX and RX CSDP settings for MCBSP FIFO packet burst
        //
        if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
            {
            // To make sure the TX CSDP configurations are not over written
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_DST_PACKED |
                DMA_CSDP_DST_BURST_64BYTES_16x32_8x64;
            }
        else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
            {
            // To make sure the RX CSDP configurations are not over written
            //
            pDmaConfigInfo->elemSize |= DMA_CSDP_SRC_PACKED |
                DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64;
            }
        }

    if (dmaSyncMap == m_pDevice->dmaTxSyncMap)
        {
        // DMA destination frame index must be equal to the TX threshold
        // value of the McBSP
        pDmaConfigInfo->dstFrameIndex = m_pDevice->fifoThresholdTx + 1;
        }
    else if (dmaSyncMap == m_pDevice->dmaRxSyncMap)
        {
        // DMA source frame index must be equal to the RX threshold
        // value of the McBSP
        pDmaConfigInfo->srcFrameIndex = m_pDevice->fifoThresholdRx + 1;
        }
    // configure dma
    //
    DmaUpdate(pDmaConfigInfo, dmaSyncMap, &m_DmaInfo);
    DmaSetElementAndFrameCount(&m_DmaInfo, (WORD)GetSamplesPerPage(), 2);
    DmaSetRepeatMode(&m_DmaInfo, TRUE);

    m_PortState = kMcBSP_Port_Idle;
    bResult = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return bResult;
}


//------------------------------------------------------------------------------
//  Function:  StartDma()
//
//  Sets up the DMA and begins transfering
//
BOOL
DataPort_t::StartDma(
    BOOL bTransmitMode
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(bTransmitMode=%d)\r\n", __FUNCTION__,
        bTransmitMode)
        );

    m_PortState = kMcBSP_Port_Active;
    DmaSetRepeatMode(&m_DmaInfo, TRUE);

    if (bTransmitMode)
        {
        // the buffer is the source
        //
        DmaSetSrcBuffer(&m_DmaInfo, m_pDmaBufferStart, m_DmaPhysAddr);
        }
    else
        {
        DmaSetDstBuffer(&m_DmaInfo, m_pDmaBufferStart, m_DmaPhysAddr);
        }

    DmaStart(&m_DmaInfo);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return TRUE;
}


//------------------------------------------------------------------------------
//  Function:  StopDma()
//
//  Stops the DMA
//
BOOL
DataPort_t::StopDma()
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    m_DmaLoopCounter = 0;
    m_PortState = kMcBSP_Port_Idle;
    DmaStop(&m_DmaInfo);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return TRUE;
}


//------------------------------------------------------------------------------
//  Function:  SetDstPhysAddr()
//
//  Set the physical address of the destination to write to
//
void
DataPort_t::SetDstPhysAddr(
    DWORD PhysAddr
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(PhysAddr=0x%08X)\r\n", __FUNCTION__,
        PhysAddr)
        );

    DmaSetDstBuffer(&m_DmaInfo, NULL, PhysAddr);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//  Function:  SetSrcPhysAddr()
//
//  Set the physical address of the destination to write to
//
void
DataPort_t::SetSrcPhysAddr(
    DWORD PhysAddr
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(PhysAddr=0x%08X)\r\n", __FUNCTION__,
        PhysAddr)
        );

    DmaSetSrcBuffer(&m_DmaInfo, NULL, PhysAddr);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//  Function:  SwapBuffer()
//
//  Initialize member variables
//
void
DataPort_t::SwapBuffer(
    BOOL bTransmitMode
    )
{
    UINT8 const * pPos;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    // it's insufficient to just check the active buffer since the
    // dma interrupt may not get serviced in time and the next page is
    // already being rendered.  Check for this and select the appropriate
    // page to swap to.
    //

    if (bTransmitMode)
        {
        pPos = DmaGetLastReadPos(&m_DmaInfo);
        }
    else
        {
        pPos = DmaGetLastWritePos(&m_DmaInfo);
        }

    // assume circular buffer.  therefore we only need to keep active
    // buffer pointer in sync with DMA
    //
    if (pPos ==
        ((m_pDmaBufferMiddle - m_pDmaBufferStart) + (m_pDmaBufferMiddle)))
        {
        m_pActiveDmaBuffer = m_pDmaBufferStart;
        }
    else if (pPos < m_pDmaBufferMiddle)
        {
        m_pActiveDmaBuffer = m_pDmaBufferStart;
        }
    else
        {
        m_pActiveDmaBuffer = m_pDmaBufferMiddle;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}



