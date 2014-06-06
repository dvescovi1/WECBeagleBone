// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: dma_init.c
//
#include "omap.h"
#include <omap_sdma_regs.h>
#include <omap_mcspi_regs.h>
#include <omap_sdma_utility.h>
#include <spi_priv.h>

//------------------------------------------------------------------------------

DmaConfigInfo_t TxDmaSettings = {

    // element width
    // valid values are:
    //  DMA_CSDP_DATATYPE_S8
    //  DMA_CSDP_DATATYPE_S16
    //  DMA_CSDP_DATATYPE_S32
    //
    DMA_CSDP_DATATYPE_S8,

    // source element index
    // valid values are:
    //  [-32768, 32768]
    //
    0,

    // source frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    0,

    // source addressing mode
    // valid values are:
    //   DMA_CCR_SRC_AMODE_DOUBLE
    //   DMA_CCR_SRC_AMODE_SINGLE
    //   DMA_CCR_SRC_AMODE_POST_INC
    //   DMA_CCR_SRC_AMODE_CONST
    //
    DMA_CCR_SRC_AMODE_POST_INC,

    // destination element index
    // valid values are:
    //  [-32768, 32767]
    //
    0,

    // destination frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    0,

    // destination addressing mode
    // valid values are:
    //   DMA_CCR_DST_AMODE_DOUBLE
    //   DMA_CCR_DST_AMODE_SINGLE
    //   DMA_CCR_DST_AMODE_POST_INC
    //   DMA_CCR_DST_AMODE_CONST
    //
    DMA_CCR_DST_AMODE_CONST,

    // dma priority level
    // valid values are
    //   DMA_PRIORITY           : high priority
    //   FALSE                  : low priority
    //
    DMA_PRIORITY,

    // synch mode
    // valid values are
    //   DMA_SYNCH_TRIGGER_NONE : dma is asynchronous
    //   DMA_SYNCH_TRIGGER_DST  : dma to synchronize on destination
    //   DMA_SYNCH_TRIGGER_SRC  : dma to synchronize on source
    //
    DMA_SYNCH_TRIGGER_DST,

    // synch mode
    // valid values are
    //   DMA_SYNCH_NONE         : no synch mode
    //   DMA_SYNCH_FRAME        : write/read entire frames
    //   DMA_SYNCH_BLOCK        : write/read entire blocks
    //   DMA_SYNCH_PACKET       : write/read entire packets
    //
    DMA_SYNCH_NONE,

    // dma interrupt mask
    // may be any combination of:
    //   DMA_CICR_PKT_IE
    //   DMA_CICR_BLOCK_IE
    //   DMA_CICR_LAST_IE
    //   DMA_CICR_FRAME_IE
    //   DMA_CICR_HALF_IE
    //   DMA_CICR_DROP_IE
    //   DMA_CICR_MISALIGNED_ERR_IE
    //   DMA_CICR_SUPERVISOR_ERR_IE
    //   DMA_CICR_SECURE_ERR_IE
    //   DMA_CICR_TRANS_ERR_IE
    //
    DMA_CICR_BLOCK_IE

};

DmaConfigInfo_t RxDmaSettings = {

    // element width
    // valid values are:
    //  DMA_CSDP_DATATYPE_S8
    //  DMA_CSDP_DATATYPE_S16
    //  DMA_CSDP_DATATYPE_S32
    //
    DMA_CSDP_DATATYPE_S8,

    // source element index
    // valid values are:
    //  [-32768, 32768]
    //
    0,

    // source frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    0,

    // source addressing mode
    // valid values are:
    //   DMA_CCR_SRC_AMODE_DOUBLE
    //   DMA_CCR_SRC_AMODE_SINGLE
    //   DMA_CCR_SRC_AMODE_POST_INC
    //   DMA_CCR_SRC_AMODE_CONST
    //
    DMA_CCR_SRC_AMODE_CONST,

    // destination element index
    // valid values are:
    //  [-32768, 32767]
    //
    0,

    // destination frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    0,

    // destination addressing mode
    // valid values are:
    //   DMA_CCR_DST_AMODE_DOUBLE
    //   DMA_CCR_DST_AMODE_SINGLE
    //   DMA_CCR_DST_AMODE_POST_INC
    //   DMA_CCR_DST_AMODE_CONST
    //
    DMA_CCR_DST_AMODE_POST_INC,

    // dma priority level
    // valid values are
    //   DMA_PRIORITY           : high priority
    //   FALSE                  : low priority
    //
    DMA_PRIORITY,

    // synch mode
    // valid values are
    //   DMA_SYNCH_TRIGGER_NONE : dma is asynchronous
    //   DMA_SYNCH_TRIGGER_DST  : dma to synchronize on destination
    //   DMA_SYNCH_TRIGGER_SRC  : dma to synchronize on source
    //
    DMA_SYNCH_TRIGGER_SRC,

    // synch mode
    // valid values are
    //   DMA_SYNCH_NONE         : no synch mode
    //   DMA_SYNCH_FRAME        : write/read entire frames
    //   DMA_SYNCH_BLOCK        : write/read entire blocks
    //   DMA_SYNCH_PACKET       : write/read entire packets
    //
    DMA_SYNCH_NONE,

    // dma interrupt mask
    // may be any combination of:
    //   DMA_CICR_PKT_IE
    //   DMA_CICR_BLOCK_IE
    //   DMA_CICR_LAST_IE
    //   DMA_CICR_FRAME_IE
    //   DMA_CICR_HALF_IE
    //   DMA_CICR_DROP_IE
    //   DMA_CICR_MISALIGNED_ERR_IE
    //   DMA_CICR_SUPERVISOR_ERR_IE
    //   DMA_CICR_SECURE_ERR_IE
    //   DMA_CICR_TRANS_ERR_IE
    //
    DMA_CICR_BLOCK_IE

};

DWORD   dwSyncMasksTX[MCSPI_MAX_PORTS][MCSPI_MAX_CHANNELS]=
{
    SDMA_REQ_SPI1_TX0, SDMA_REQ_SPI1_TX1, SDMA_REQ_SPI1_TX2, SDMA_REQ_SPI1_TX3,
    SDMA_REQ_SPI2_TX0, SDMA_REQ_SPI2_TX1, 0,                 0,
    SDMA_REQ_SPI3_TX0, SDMA_REQ_SPI3_TX1, 0,                 0,
    SDMA_REQ_SPI4_TX0, 0,                 0,                 0,
};


DWORD   dwSyncMasksRX[MCSPI_MAX_PORTS][MCSPI_MAX_CHANNELS]=
{
    SDMA_REQ_SPI1_RX0, SDMA_REQ_SPI1_RX1, SDMA_REQ_SPI1_RX2, SDMA_REQ_SPI1_RX3,
    SDMA_REQ_SPI2_RX0, SDMA_REQ_SPI2_RX1, 0,                 0,
    SDMA_REQ_SPI3_RX0, SDMA_REQ_SPI3_RX1, 0,                 0,
    SDMA_REQ_SPI4_RX0, 0,                 0,                 0,
};

//------------------------------------------------------------------------------
//
//  Function:  SpiDmaRestore
//
//
BOOL SpiDmaRestore(SPI_INSTANCE *pInstance)
{
    DWORD   dwSyncMask;

    dwSyncMask = dwSyncMasksTX[pInstance->pDevice->dwPort][pInstance->address];

    // Reconfigure the DMA channel
    return DmaUpdate(&pInstance->txDmaConfig, dwSyncMask, &pInstance->txDmaInfo);
}

//------------------------------------------------------------------------------
//
//  Function:  SpiDmaInit
//
//  Called initialize DMA channels for SPI driver instance
//
BOOL SpiDmaInit(SPI_INSTANCE *pInstance)
{
    BOOL    bResult = FALSE;
    DWORD   dwSyncMask;
    DWORD   dwWordLen;
    UINT8*  pRegAddr;
    DWORD   paRegAddr;


    DEBUGMSG(ZONE_DMA, (
        L"+SpiDmaInit(0x%08x)\r\n", pInstance
    ));

    //  Setup TX DMA channel if needed
    if( pInstance->config & MCSPI_CHCONF_DMAW_ENABLE )
    {
        //  Allocate DMA channel
        pInstance->hTxDmaChannel = DmaAllocateChannel(DMA_TYPE_SYSTEM);
        if (pInstance->hTxDmaChannel == NULL )
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"DmaAllocateChannel Tx failed\r\n"
            ));
            goto cleanUp;
            }

        // Allocate DMA buffer
        pInstance->pTxDmaBuffer = AllocPhysMem(
            pInstance->pDevice->dwTxBufferSize, PAGE_READWRITE | PAGE_NOCACHE, 0, 0,
            &pInstance->paTxDmaBuffer
            );
        if (pInstance->pTxDmaBuffer == NULL)
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed allocate DMA Tx buffer (size %d)\r\n",
                pInstance->pDevice->dwTxBufferSize
            ));
            goto cleanUp;
            }


        // Determine the DMA settings for the SPI channel
        switch( pInstance->address )
        {
            case 0:
                //  Channel 0 configuration
                dwSyncMask = dwSyncMasksTX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_TX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_TX0);
                break;

            case 1:
                //  Channel 1 configuration
                dwSyncMask = dwSyncMasksTX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_TX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_TX1);
                break;

            case 2:
                //  Channel 2 configuration
                dwSyncMask = dwSyncMasksTX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_TX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_TX2);
                break;

            case 3:
                //  Channel 3 configuration
                dwSyncMask = dwSyncMasksTX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_TX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_TX3);
                break;

            default:
				goto cleanUp;
                break;
        }

        // Copy the default DMA config settings
        pInstance->txDmaConfig = TxDmaSettings;


        // Determine DMA datatype
        dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);
        if( dwWordLen > 16 )
        {
            //  32 bit data type
            pInstance->txDmaConfig.elemSize = DMA_CSDP_DATATYPE_S32;
        }
        else if( dwWordLen > 8 )
        {
            //  16 bit data type
            pInstance->txDmaConfig.elemSize = DMA_CSDP_DATATYPE_S16;
        }
        else
        {
            //  8 bit data type
            pInstance->txDmaConfig.elemSize = DMA_CSDP_DATATYPE_S8;
        }

        // Configure the DMA channel
        DmaConfigure(pInstance->hTxDmaChannel, &pInstance->txDmaConfig, dwSyncMask, &pInstance->txDmaInfo);
        DmaSetSrcBuffer(&pInstance->txDmaInfo, pInstance->pTxDmaBuffer, pInstance->paTxDmaBuffer);
        DmaSetDstBuffer(&pInstance->txDmaInfo, pRegAddr, paRegAddr);
        DmaSetRepeatMode(&pInstance->txDmaInfo, FALSE);


        // Create DMA interrupt event
        pInstance->hTxDmaIntEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pInstance->hTxDmaIntEvent == NULL) {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed create TX DMA interrupt event\r\n"
            ));
            goto cleanUp;
        }

        // Initialize interrupt
        if (!DmaEnableInterrupts(pInstance->hTxDmaChannel, pInstance->hTxDmaIntEvent)) {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed to enable TX DMA interrupt\r\n"
            ));
            goto cleanUp;
        }
    }



    //  Setup RX DMA channel if needed
    if( pInstance->config & MCSPI_CHCONF_DMAR_ENABLE )
    {
        //  Allocate DMA channel
        pInstance->hRxDmaChannel = DmaAllocateChannel(DMA_TYPE_SYSTEM);
        if (pInstance->hRxDmaChannel == NULL )
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"DmaAllocateChannel Rx failed\r\n"
            ));
            goto cleanUp;
            }

        // Allocate DMA buffer
        pInstance->pRxDmaBuffer = AllocPhysMem(
            pInstance->pDevice->dwRxBufferSize, PAGE_READWRITE | PAGE_NOCACHE, 0, 0,
            &pInstance->paRxDmaBuffer
            );
        if (pInstance->pRxDmaBuffer == NULL)
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed allocate DMA Rx buffer (size %d)\r\n",
                pInstance->pDevice->dwRxBufferSize
            ));
            goto cleanUp;
            }


        // Determine the DMA settings for the SPI channel
        switch( pInstance->address )
        {
            case 0:
                //  Channel 0 configuration
                dwSyncMask = dwSyncMasksRX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_RX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_RX0);
                break;

            case 1:
                //  Channel 1 configuration
                dwSyncMask = dwSyncMasksRX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_RX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_RX1);
                break;

            case 2:
                //  Channel 2 configuration
                dwSyncMask = dwSyncMasksRX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_RX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_RX2);
                break;

            case 3:
                //  Channel 3 configuration
                dwSyncMask = dwSyncMasksRX[pInstance->pDevice->dwPort][pInstance->address];
                pRegAddr = (UINT8*) &pInstance->pSPIChannelRegs->MCSPI_RX;
                paRegAddr = pInstance->pDevice->memBase[0] + offset(OMAP_MCSPI_REGS, MCSPI_RX3);
                break;

            default:
				goto cleanUp;
                break;
        }

        // Copy the default DMA config settings
        pInstance->rxDmaConfig = RxDmaSettings;


        // Determine DMA datatype
        dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);
        if( dwWordLen > 16 )
        {
            //  32 bit data type
            pInstance->rxDmaConfig.elemSize = DMA_CSDP_DATATYPE_S32;
        }
        else if( dwWordLen > 8 )
        {
            //  16 bit data type
            pInstance->rxDmaConfig.elemSize = DMA_CSDP_DATATYPE_S16;
        }
        else
        {
            //  8 bit data type
            pInstance->rxDmaConfig.elemSize = DMA_CSDP_DATATYPE_S8;
        }

        // Configure the DMA channel
        DmaConfigure(pInstance->hRxDmaChannel, &pInstance->rxDmaConfig, dwSyncMask, &pInstance->rxDmaInfo);
        DmaSetSrcBuffer(&pInstance->rxDmaInfo, pRegAddr, paRegAddr);
        DmaSetDstBuffer(&pInstance->rxDmaInfo, pInstance->pRxDmaBuffer, pInstance->paRxDmaBuffer);
        DmaSetRepeatMode(&pInstance->rxDmaInfo, FALSE);


        // Create DMA interrupt event
        pInstance->hRxDmaIntEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pInstance->hRxDmaIntEvent == NULL) {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed create RX DMA interrupt event\r\n"
            ));
            goto cleanUp;
        }

        // Initialize interrupt
        if (!DmaEnableInterrupts(pInstance->hRxDmaChannel, pInstance->hRxDmaIntEvent)) {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SpiDmaInit: "
                L"Failed to enable RX DMA interrupt\r\n"
            ));
            goto cleanUp;
        }
    }

    //  Success
    bResult = TRUE;

cleanUp:
    DEBUGMSG(ZONE_DMA, (
        L"-SpiDmaInit() rc = \r\n", bResult
    ));

    //  Cleanup DMA on failures
    if( bResult == FALSE )
    {
        SpiDmaDeinit(pInstance);
    }

    //  Return result
    return bResult;
}


//------------------------------------------------------------------------------
//
//  Function:  SpiDmaDeinit
//
//  Called free DMA channels for SPI driver instance
//
BOOL SpiDmaDeinit(SPI_INSTANCE *pInstance)
{
    BOOL    bResult = FALSE;

    DEBUGMSG(ZONE_DMA, (
        L"+SpiDmaDeinit(0x%08x)\r\n", pInstance
    ));


    //  Disable DMA interrupts
    if( pInstance->hTxDmaIntEvent )
    {
        DmaEnableInterrupts(pInstance->hTxDmaChannel, NULL);
        CloseHandle(pInstance->hTxDmaIntEvent);
        pInstance->hTxDmaIntEvent = NULL;
    }

    if( pInstance->hRxDmaIntEvent )
    {
        DmaEnableInterrupts(pInstance->hRxDmaChannel, NULL);
        CloseHandle(pInstance->hRxDmaIntEvent);
        pInstance->hRxDmaIntEvent = NULL;
    }


    //  Close DMA channel
    if( pInstance->hTxDmaChannel )
    {
        DmaFreeChannel(pInstance->hTxDmaChannel);
        pInstance->hTxDmaChannel = NULL;
    }

    if( pInstance->hRxDmaChannel )
    {
        DmaFreeChannel(pInstance->hRxDmaChannel);
        pInstance->hRxDmaChannel = NULL;
    }

    //  Free DMA memory
    if( pInstance->pTxDmaBuffer )
    {
        FreePhysMem( pInstance->pTxDmaBuffer );
        pInstance->pTxDmaBuffer = NULL;
    }

    if( pInstance->pRxDmaBuffer )
    {
        FreePhysMem( pInstance->pRxDmaBuffer );
        pInstance->pRxDmaBuffer = NULL;
    }


    DEBUGMSG(ZONE_DMA, (
        L"-SpiDmaDeinit() rc = \r\n", bResult
    ));

    //  Return result
    return bResult;
}




//------------------------------------------------------------------------------

