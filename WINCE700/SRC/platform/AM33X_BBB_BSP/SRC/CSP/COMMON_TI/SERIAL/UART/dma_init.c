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

//------------------------------------------------------------------------------
//
//  Contains DMA configuration information
//
#include "omap.h"
#include "omap_sdma_utility.h"


//------------------------------------------------------------------------------

DmaConfigInfo_t TxDmaSettings = {

    // element width
    // valid values are:
    //  DMA_CSDP_DATA_TYPE_S8
    //  DMA_CSDP_DATA_TYPE_S16
    //  DMA_CSDP_DATA_TYPE_S32
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
    DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE
};


DmaConfigInfo_t RxDmaSettings = {

    // element width
    // valid values are:
    //  DMA_CSDP_DATA_TYPE_S8
    //  DMA_CSDP_DATA_TYPE_S16
    //  DMA_CSDP_DATA_TYPE_S32
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
    //   DMA_CCR_BUFFERING_DISABLE: Write/read Fifo drainning
    //
    DMA_SYNCH_FRAME,

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
    DMA_CICR_FRAME_IE | DMA_CICR_DROP_IE | DMA_CICR_MISALIGNED_ERR_IE |
    DMA_CICR_SUPERVISOR_ERR_IE | DMA_CICR_SECURE_ERR_IE | DMA_CICR_TRANS_ERR_IE |
    DMA_CICR_BLOCK_IE

};

//------------------------------------------------------------------------------

