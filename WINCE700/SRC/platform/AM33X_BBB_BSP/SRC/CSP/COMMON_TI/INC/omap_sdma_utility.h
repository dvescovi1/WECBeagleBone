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
#ifndef __OMAP_DMA_UTILITY_H
#define __OMAP_DMA_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "omap.h"
#include "ceddkex.h"
#include "omap_sdma_regs.h"
//------------------------------------------------------------------------------
//
//  defines the data structure for a DMA
//
typedef struct {
    UINT8              *pSrcBuffer;
    UINT8              *pDstBuffer;
    ULONG               PhysAddrSrcBuffer;
    ULONG               PhysAddrDstBuffer;
    HANDLE              hDmaChannel;
    OMAP_DMA_LC_REGS   *pDmaLcReg;
} DmaDataInfo_t;

//------------------------------------------------------------------------------
//
//  defines the dma configuration information
//  **Must select from stated valid values
//
#define DMA_SYNCH_BLOCK             (DMA_CCR_BS)
#define DMA_SYNCH_FRAME             (DMA_CCR_FS)
#define DMA_SYNCH_PACKET            (DMA_CCR_FS | DMA_CCR_BS)
#define DMA_SYNCH_NONE              (0)
#define DMA_PRIORITY                (DMA_CCR_WRITE_PRIO | DMA_CCR_READ_PRIO)

#define DMA_SYNCH_TRIGGER_NONE      (0)
#define DMA_SYNCH_TRIGGER_DST       (0)
#define DMA_SYNCH_TRIGGER_SRC       (DMA_CCR_SEL_SRC_DST_SYNCH)

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
//  Function:  DmaConfigure
//
//  Configures the DMA port
//
__inline BOOL
DmaConfigure (
    HANDLE            hDmaChannel,
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    )
{
    BOOL rc = FALSE;
    OMAP_DMA_LC_REGS *pDmaLcReg = (OMAP_DMA_LC_REGS*)DmaGetLogicalChannel(hDmaChannel);
    if (pDmaLcReg == NULL || pConfigInfo == NULL)
        {
        goto cleanUp;
        }

    // initialize dma DataInfo if necessary
    if (pDataInfo != NULL)
        {
        memset(pDataInfo, 0, sizeof(DmaDataInfo_t));
        pDataInfo->hDmaChannel = hDmaChannel;
        pDataInfo->pDmaLcReg = pDmaLcReg;
        }

    // Disable the DMA in case it is running
    CLRREG32(&pDmaLcReg->CCR, DMA_CCR_ENABLE);

    // update syncmap
    pConfigInfo->syncMap = syncMap;

    // Initialize logical channel registers
    //
    OUTREG32(&pDmaLcReg->CCR, 0);
    OUTREG32(&pDmaLcReg->CLNK_CTRL, 0);
    OUTREG32(&pDmaLcReg->COLOR, 0);

    // update CSDP
    //  DATA_TYPE
    //  DST
    //  SRC
    OUTREG32(&pDmaLcReg->CSDP, pConfigInfo->elemSize);

    // update CCR
    //  DST_MODE
    //  SRC_MODE
    //  PRIO
    //  SYNC
    //
    OUTREG32(&pDmaLcReg->CCR, pConfigInfo->srcAddrMode |
        pConfigInfo->dstAddrMode | pConfigInfo->dmaPrio |
        DMA_CCR_SYNC(syncMap) | pConfigInfo->synchTrigger |
        pConfigInfo->synchMode
        );

    // update CSEI
    //
    OUTREG32(&pDmaLcReg->CSEI, pConfigInfo->srcElemIndex);

    // update CDEI
    //
    OUTREG32(&pDmaLcReg->CDEI, pConfigInfo->dstElemIndex);

    // update CSFI
    //
    OUTREG32(&pDmaLcReg->CSFI, pConfigInfo->srcFrameIndex);

    // update CDFI
    //
    OUTREG32(&pDmaLcReg->CDFI, pConfigInfo->dstFrameIndex);

    // update CICR
    //
    OUTREG32(&pDmaLcReg->CICR , pConfigInfo->interrupts);
    
    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DmaUpdate
//
//  Updates dma registers with cached information from DmaConfigInfo.
//
__inline BOOL
DmaUpdate (
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    )
{
    BOOL rc = FALSE;
    OMAP_DMA_LC_REGS *pDmaLcReg;

    if (pDataInfo == NULL || pDataInfo->pDmaLcReg == NULL)
        {
        goto cleanUp;
        }
    pDmaLcReg = pDataInfo->pDmaLcReg;

    // Disable the DMA in case it is running
    CLRREG32(&pDmaLcReg->CCR, DMA_CCR_ENABLE);

    // update syncmap
    pConfigInfo->syncMap = syncMap;

    // Initialize logical channel registers
    //
    OUTREG32(&pDmaLcReg->CCR, 0);
    OUTREG32(&pDmaLcReg->CLNK_CTRL, 0);
    OUTREG32(&pDmaLcReg->COLOR, 0);

    // update CSDP
    //  DATA_TYPE
    //  DST
    //  SRC
    OUTREG32(&pDmaLcReg->CSDP, pConfigInfo->elemSize);

    // update CCR
    //  DST_MODE
    //  SRC_MODE
    //  PRIO
    //  SYNC
    //
    OUTREG32(&pDmaLcReg->CCR, pConfigInfo->srcAddrMode |
        pConfigInfo->dstAddrMode | pConfigInfo->dmaPrio |
        DMA_CCR_SYNC(syncMap) | pConfigInfo->synchTrigger |
        pConfigInfo->synchMode
        );

    // update CSEI
    //
    OUTREG32(&pDmaLcReg->CSEI, pConfigInfo->srcElemIndex);

    // update CDEI
    //
    OUTREG32(&pDmaLcReg->CDEI, pConfigInfo->dstElemIndex);

    // update CSFI
    //
    OUTREG32(&pDmaLcReg->CSFI, pConfigInfo->srcFrameIndex);

    // update CDFI
    //
    OUTREG32(&pDmaLcReg->CDFI, pConfigInfo->dstFrameIndex);

    // update CICR
    //
    OUTREG32(&pDmaLcReg->CICR , pConfigInfo->interrupts);

    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DmaSetDstBuffer
//
//  sets the destination address and buffer
//
__inline void
DmaSetDstBuffer (
    DmaDataInfo_t    *pDataInfo,
    UINT8            *pBuffer,
    DWORD             PhysAddr
    )
{
    // save values
    //
    pDataInfo->pDstBuffer = pBuffer;
    pDataInfo->PhysAddrDstBuffer = PhysAddr;

    // set destination address
    //
    if (pDataInfo->pDmaLcReg != NULL)
        {
        OUTREG32(&pDataInfo->pDmaLcReg->CDSA, PhysAddr);
        OUTREG32(&pDataInfo->pDmaLcReg->CDAC, PhysAddr);
        }
}


//------------------------------------------------------------------------------
//
//  Function:  DmaSetSrcBuffer
//
//  sets the src address and buffer
//
__inline void
DmaSetSrcBuffer (
        DmaDataInfo_t    *pDataInfo,
        UINT8            *pBuffer,
        DWORD             PhysAddr
        )
{
    // save values
    //
    pDataInfo->pSrcBuffer = pBuffer;
    pDataInfo->PhysAddrSrcBuffer = PhysAddr;

    // set source address
    //
    if (pDataInfo->pDmaLcReg != NULL)
        {
        OUTREG32(&pDataInfo->pDmaLcReg->CSSA, PhysAddr);
        OUTREG32(&pDataInfo->pDmaLcReg->CSAC, PhysAddr);
        }
}


//------------------------------------------------------------------------------
//
//  Function:  DmaSetElementAndFrameCount
//
//  sets the element and frame count
//
__inline void
DmaSetElementAndFrameCount (
    DmaDataInfo_t    *pDataInfo,
    UINT32            countElements,
    UINT16            countFrames
    )
{
    // setup frame and element count for destination side
    //
    OUTREG32(&pDataInfo->pDmaLcReg->CEN , countElements);
    OUTREG32(&pDataInfo->pDmaLcReg->CFN , countFrames);
}


//------------------------------------------------------------------------------
//
//  Function:  IsDmaEnable
//
//  Check dma Enable bit
//
__inline BOOL
IsDmaEnable (
    DmaDataInfo_t    *pDataInfo
    )
{
    volatile ULONG ulCCR;

    if ((NULL == pDataInfo) || (pDataInfo->pDmaLcReg == NULL))
       return FALSE;

    ulCCR = INREG32(&pDataInfo->pDmaLcReg->CCR);

    if (ulCCR & DMA_CCR_ENABLE)
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  IsDmaActive
//
//  Check dma status for read write
//
__inline BOOL
IsDmaActive (
    DmaDataInfo_t    *pDataInfo
    )
{
    volatile ULONG ulCCR;

    if ((NULL == pDataInfo) || (pDataInfo->pDmaLcReg == NULL))
       return FALSE;

    ulCCR = INREG32(&pDataInfo->pDmaLcReg->CCR);

    if ((ulCCR & DMA_CCR_WR_ACTIVE) || (ulCCR & DMA_CCR_RD_ACTIVE))
        return TRUE;
    else
        return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  DmaStop
//
//  Stops dma from running
//
__inline void
DmaStop (
    DmaDataInfo_t    *pDataInfo
    )
{
    volatile ULONG ulCCR;
    BOOL breakLoop = FALSE;
    int  DelayCnt = 0x7FFF;

    if (pDataInfo == NULL || pDataInfo->pDmaLcReg == NULL)
    {
        goto cleanUp;
    }

    // disable standby in dma controller
    //
    DmaDisableStandby(pDataInfo->hDmaChannel, TRUE);
	
    CLRREG32(&pDataInfo->pDmaLcReg->CCR , DMA_CCR_ENABLE);

    // ensure DMA transfer is completed by polling the active bits
    //
    while ((breakLoop == FALSE) && (DelayCnt != 0))
    {
        ulCCR = INREG32(&pDataInfo->pDmaLcReg->CCR);
        if ((ulCCR & DMA_CCR_WR_ACTIVE) || (ulCCR & DMA_CCR_RD_ACTIVE))
        {
            // fix this infinite loop
            //
            DelayCnt--;
        }
        else
        {
            breakLoop = TRUE;
        }
        DelayCnt--;
    }
    DmaDisableStandby(pDataInfo->hDmaChannel, FALSE);
	
cleanUp:
    return;
}


//------------------------------------------------------------------------------
//
//  Function:  DmaStart
//
//  Starts dma
//
__inline void
DmaStart (
    DmaDataInfo_t    *pDataInfo
    )
{
    // enable the dma channel
    //
    SETREG32(&pDataInfo->pDmaLcReg->CCR , DMA_CCR_ENABLE);
}


//------------------------------------------------------------------------------
//
//  Function:  DmaGetLastWritePos
//
//  Starts dma
//
__inline UINT8*
DmaGetLastWritePos (
    DmaDataInfo_t    *pDataInfo
    )
{
    UINT32  offset;
    offset = INREG32(&pDataInfo->pDmaLcReg->CDAC) - pDataInfo->PhysAddrDstBuffer;
    return ((UINT8*)pDataInfo->pDstBuffer + offset);
}


//------------------------------------------------------------------------------
//
//  Function:  DmaGetLastReadPos
//
//  Starts dma
//
__inline UINT8*
DmaGetLastReadPos (
    DmaDataInfo_t    *pDataInfo
    )
{
    UINT32  offset;
    offset = INREG32(&pDataInfo->pDmaLcReg->CSAC) - pDataInfo->PhysAddrSrcBuffer;
    return ((UINT8*)pDataInfo->pSrcBuffer + offset);
}

//------------------------------------------------------------------------------
//
//  Function:  DmaSetRepeatMode
//
//  puts the dma in repeat mode
//
__inline BOOL
DmaSetRepeatMode (
    DmaDataInfo_t    *pDataInfo,
    BOOL              bEnable
    )
{
    DWORD dwMode = DmaGetLogicalChannelId(pDataInfo->hDmaChannel);
    if (dwMode == -1) return FALSE;

    dwMode |= (bEnable == TRUE) ? DMA_CLNK_CTRL_ENABLE_LINK : 0;
    OUTREG32(&pDataInfo->pDmaLcReg->CLNK_CTRL, dwMode);
    return TRUE;
}



//------------------------------------------------------------------------------
//
//  Function:  DmaGetStatus
//
//  Starts dma
//
__inline UINT32
DmaGetStatus (
    DmaDataInfo_t    *pDataInfo
    )
{
    return INREG32(&pDataInfo->pDmaLcReg->CSR);
}


//------------------------------------------------------------------------------
//
//  Function:  DmaClearStatus
//
//  Starts dma
//
__inline void
DmaClearStatus (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwStatus
    )
{
    OUTREG32(&pDataInfo->pDmaLcReg->CSR, dwStatus);
}


//------------------------------------------------------------------------------
//
//  Function:  DmaSetSrcEndian
//
//  sets the source endian
//
__inline void
DmaSetSrcEndian (
    DmaDataInfo_t    *pDataInfo,
    BOOL              srcEndian,
    BOOL              srcEndianLock
    )
{
    // setup endian for source side
    //
    if (srcEndian)
        {
        SETREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_SRC_ENDIAN_BIG);
        }
    else
        {
        CLRREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_SRC_ENDIAN_BIG);
        }

    // setup endian lock for source side
    //
    if (srcEndianLock)
        {
        SETREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_SRC_ENDIAN_LOCK);
        }
    else
        {
        CLRREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_SRC_ENDIAN_LOCK);
        }
}


//------------------------------------------------------------------------------
//
//  Function:  DmaSetDstEndian
//
//  sets the destination endian
//
__inline void
DmaSetDstEndian (
    DmaDataInfo_t    *pDataInfo,
    BOOL              dstEndian,
    BOOL              dstEndianLock
    )
{
    // setup endian for destination side
    //
    if (dstEndian)
        {
        SETREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_DST_ENDIAN_BIG);
        }
    else
        {
        CLRREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_DST_ENDIAN_BIG);
        }

    // setup endian lock for source side
    //
    if (dstEndianLock)
        {
        SETREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_DST_ENDIAN_LOCK);
        }
    else
        {
        CLRREG32(&pDataInfo->pDmaLcReg->CSDP, DMA_CSDP_DST_ENDIAN_LOCK);
        }
}

//------------------------------------------------------------------------------
//
//  Function:  DmaSetColor
//
//  Sets DMA Color value
//
__inline void
DmaSetColor (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwFlag,
    DWORD             dwColor
    )
{
    SETREG32(&pDataInfo->pDmaLcReg->CCR, dwFlag);
    OUTREG32(&pDataInfo->pDmaLcReg->COLOR, dwColor);
}



#ifndef SHIP_BUILD
__inline void
DUMP_DMA_REGS(
    HANDLE hDmaChannel,
    LPCTSTR szMsg
    )
{
    OMAP_DMA_LC_REGS *pDmaLcReg = (OMAP_DMA_LC_REGS*)DmaGetLogicalChannel(hDmaChannel);
    if (pDmaLcReg == NULL)
        {
        return;
        }

    RETAILMSG(1,(szMsg));

    RETAILMSG(1,(TEXT("CCR      : 0x%08X\r\n"), INREG32(&pDmaLcReg->CCR   )));
    RETAILMSG(1,(TEXT("CLNK_CTRL: 0x%08X\r\n"), INREG32(&pDmaLcReg->CLNK_CTRL)));
    RETAILMSG(1,(TEXT("CICR     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CICR  )));
    RETAILMSG(1,(TEXT("CSR      : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSR   )));
    RETAILMSG(1,(TEXT("CSDP     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSDP  )));
    RETAILMSG(1,(TEXT("CEN      : 0x%08X\r\n"), INREG32(&pDmaLcReg->CEN   )));
    RETAILMSG(1,(TEXT("CFN      : 0x%08X\r\n"), INREG32(&pDmaLcReg->CFN   )));
    RETAILMSG(1,(TEXT("CSSA     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSSA  )));
    RETAILMSG(1,(TEXT("CDSA     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CDSA  )));
    RETAILMSG(1,(TEXT("CSEI     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSEI  )));
    RETAILMSG(1,(TEXT("CSFI     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSFI  )));
    RETAILMSG(1,(TEXT("CDEI     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CDEI  )));
    RETAILMSG(1,(TEXT("CDFI     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CDFI  )));
    RETAILMSG(1,(TEXT("CSAC     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CSAC  )));
    RETAILMSG(1,(TEXT("CDAC     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CDAC  )));
    RETAILMSG(1,(TEXT("CCEN     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CCEN  )));
    RETAILMSG(1,(TEXT("CCFN     : 0x%08X\r\n"), INREG32(&pDmaLcReg->CCFN  )));
    RETAILMSG(1,(TEXT("COLOR    : 0x%08X\r\n"), INREG32(&pDmaLcReg->COLOR )));
}
#else
#define DUMP_DMA_REGS(a,b)

#endif //SHIP_BUILD

//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //__OMAP_DMA_UTILITY_H

