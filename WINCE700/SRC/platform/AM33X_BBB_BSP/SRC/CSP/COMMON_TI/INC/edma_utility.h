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
//  DMA helper routines.
//
#ifndef __EDMA_UTILITY_H
#define __EDMA_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "omap.h"
#include "ceddkex.h"
#include "edma_device_map.h"

#define DMA_CSDP_DATATYPE_S8                0 //EDMA3_DRV_W8BIT
#define DMA_CSDP_DATATYPE_S16               1 //EDMA3_DRV_W16BIT
#define DMA_CSDP_DATATYPE_S32               2 //EDMA3_DRV_W32BIT


#define DMA_CCR_SRC_AMODE_DOUBLE            0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_SRC_AMODE_SINGLE            0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_SRC_AMODE_POST_INC          0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_SRC_AMODE_CONST             1 //EDMA3_DRV_ADDR_MODE_FIFO
#define DMA_CCR_DST_AMODE_DOUBLE            0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_DST_AMODE_SINGLE            0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_DST_AMODE_POST_INC          0 //EDMA3_DRV_ADDR_MODE_INCR
#define DMA_CCR_DST_AMODE_CONST             1 //EDMA3_DRV_ADDR_MODE_FIFO
#define DMA_TYPE_SYSTEM                         0
/* values are taken from SDMA, however they are not used right now */
#define DMA_CSDP_DST_BURST_MASK                 0x0000C000
#define DMA_CSDP_DST_BURST_NONE                 0x00000000
#define DMA_CSDP_DST_BURST_16BYTES_4x32_2x64    0x00004000
#define DMA_CSDP_DST_BURST_32BYTES_8x32_4x64    0x00008000
#define DMA_CSDP_DST_BURST_64BYTES_16x32_8x64   0x0000C000
#define DMA_CSDP_DST_PACKED                     0x00002000
#define DMA_CSDP_SRC_BURST_MASK                 0x00000180
#define DMA_CSDP_SRC_BURST_NONE                 0x00000000
#define DMA_CSDP_SRC_BURST_16BYTES_4x32_2x64    0x00000080
#define DMA_CSDP_SRC_BURST_32BYTES_8x32_4x64    0x00000100
#define DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64   0x00000180
#define DMA_CSDP_SRC_PACKED                     0x00000040
#define DMA_CCR_CONST_FILL_ENABLE           (1 << 16)
#define DMA_CCR_TRANSPARENT_COPY_ENABLE     (1 << 17)

#define DMA_PRIORITY                        0
//------------------------------------------------------------------------------
//
//  defines the dma configuration information
//  **Must select from stated valid values
//
#define DMA_SYNCH_BLOCK             0 //(EDMA3_DRV_SYNC_A)
#define DMA_SYNCH_FRAME             1 //(EDMA3_DRV_SYNC_AB)
#define DMA_SYNCH_PACKET            1 //(EDMA3_DRV_SYNC_AB)
#define DMA_SYNCH_NONE              1 //(EDMA3_DRV_SYNC_A)

#define DMA_SYNCH_TRIGGER_NONE      0 //(EDMA3_DRV_TRIG_MODE_MANUAL)
#define DMA_SYNCH_TRIGGER_DST       2 //(EDMA3_DRV_TRIG_MODE_EVENT)
#define DMA_SYNCH_TRIGGER_SRC       2 //(EDMA3_DRV_TRIG_MODE_EVENT)


typedef struct {

    // element width
    // valid values are:
    //  DMA_CSDP_DATATYPE_S8
    //  DMA_CSDP_DATATYPE_S16
    //  DMA_CSDP_DATATYPE_S32
    //
    UINT32  elemSize;

    // source element index
    // valid values are:
    //  [-32768, 32768]
    //
    UINT32 srcElemIndex;

    // source frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    UINT32 srcFrameIndex;

    // source addressing mode
    // valid values are:
    //   DMA_CCR_SRC_AMODE_DOUBLE
    //   DMA_CCR_SRC_AMODE_SINGLE
    //   DMA_CCR_SRC_AMODE_POST_INC
    //   DMA_CCR_SRC_AMODE_CONST
    //
    UINT32 srcAddrMode;

    // destination element index
    // valid values are:
    //  [-32768, 32767]
    //
    UINT32 dstElemIndex;

    // destination frame index
    // valid values are:
    //  [-2147483648, 2147483647] : non-packet mode
    //  [-32768, 32768] : packet mode
    //
    UINT32 dstFrameIndex;

    // destination addressing mode
    // valid values are:
    //   DMA_CCR_DST_AMODE_DOUBLE
    //   DMA_CCR_DST_AMODE_SINGLE
    //   DMA_CCR_DST_AMODE_POST_INC
    //   DMA_CCR_DST_AMODE_CONST
    //
    UINT32 dstAddrMode;

    // dma priority level
    // valid values are
    //   DMA_PRIORITY           : high priority
    //   FALSE                  : low priority
    //
    UINT32 dmaPrio;

    // synch mode
    // valid values are
    //   DMA_SYNCH_TRIGGER_NONE : dma is asynchronous
    //   DMA_SYNCH_TRIGGER_DST  : dma to synchronize on destination
    //   DMA_SYNCH_TRIGGER_SRC  : dma to synchronize on source
    //
    UINT32 synchTrigger;

    // synch mode
    // valid values are
    //   DMA_SYNCH_NONE         : no synch mode
    //   DMA_SYNCH_FRAME        : write/read entire frames
    //   DMA_SYNCH_BLOCK        : write/read entire blocks
    //   DMA_SYNCH_PACKET       : write/read entire packets
    //
    UINT32 synchMode;

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
    UINT32 interrupts;

    // sync map
    // dma synchronization signal
    //
    UINT32 syncMap;

} DmaConfigInfo_t;

//------------------------------------------------------------------------------
//
//  defines the data structure for a DMA
//
typedef struct {
    UINT8              *pSrcBuffer;
    UINT8              *pDstBuffer;
    ULONG               PhysAddrSrcBuffer;
    ULONG               PhysAddrDstBuffer;
    UINT32              hDmaChannel;
	DmaConfigInfo_t     DmaCfgInfo;
    UINT32 	    		*pTransfer;
} DmaDataInfo_t;

HANDLE DmaAllocateChannel(DWORD dmaType);
BOOL DmaFreeChannel(HANDLE hDmaChannel);
BOOL DmaEnableInterrupts(HANDLE hDmaChannel, HANDLE hEvent);
DWORD DmaGetLogicalChannelId(HANDLE hDmaChannel);
BOOL DmaInterruptDone(HANDLE hDmaChannel);
void* DmaGetLogicalChannel(HANDLE hDmaChannel);
BOOL DmaDisableStandby(HANDLE hDmaChannel, BOOL noStandby);
BOOL DmaConfigure (
    HANDLE            hDmaChannel,
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    );

BOOL DmaUpdate (
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    );
void
DmaSetDstBuffer (
    DmaDataInfo_t    *pDataInfo,
    UINT8            *pBuffer,
    DWORD             PhysAddr
    );
void
DmaSetSrcBuffer (
        DmaDataInfo_t    *pDataInfo,
        UINT8            *pBuffer,
        DWORD             PhysAddr
        );

void DmaSetElementAndFrameCount (
    DmaDataInfo_t    *pDataInfo,
    UINT32            countElements,
    UINT16            countFrames
    );

BOOL IsDmaEnable (
    DmaDataInfo_t    *pDataInfo
    );

BOOL IsDmaActive (
    DmaDataInfo_t    *pDataInfo
    );

void DmaStop (
    DmaDataInfo_t    *pDataInfo
    );

void DmaStart (
    DmaDataInfo_t    *pDataInfo
    );

void DmaSetColor (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwFlag,
    DWORD             dwColor
    );

//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //__EDMA_UTILITY_H

