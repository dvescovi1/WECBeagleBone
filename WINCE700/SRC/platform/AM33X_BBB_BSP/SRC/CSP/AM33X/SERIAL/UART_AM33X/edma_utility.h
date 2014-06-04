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
} DmaDataInfo_t;

typedef struct {
	UINT32	field1;
} DmaConfigInfo_t;


//------------------------------------------------------------------------------
//  Configures the DMA port
__inline BOOL
DmaConfigure (
    HANDLE            hDmaChannel,
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    )
{
    BOOL rc = FALSE;
	UNREFERENCED_PARAMETER(hDmaChannel);
	UNREFERENCED_PARAMETER(pConfigInfo);
	UNREFERENCED_PARAMETER(syncMap);
	UNREFERENCED_PARAMETER(pDataInfo);

    return rc;
}


//------------------------------------------------------------------------------
//  Updates dma registers with cached information from DmaConfigInfo.
__inline BOOL
DmaUpdate (
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    )
{
    BOOL rc = FALSE;
	UNREFERENCED_PARAMETER(pConfigInfo);
	UNREFERENCED_PARAMETER(syncMap);
	UNREFERENCED_PARAMETER(pDataInfo);
    return rc;
}


//------------------------------------------------------------------------------
//  sets the destination address and buffer
__inline void
DmaSetDstBuffer (
    DmaDataInfo_t    *pDataInfo,
    UINT8            *pBuffer,
    DWORD             PhysAddr
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(PhysAddr);
}


//------------------------------------------------------------------------------
//  sets the src address and buffer
__inline void
DmaSetSrcBuffer (
        DmaDataInfo_t    *pDataInfo,
        UINT8            *pBuffer,
        DWORD             PhysAddr
        )
{
	UNREFERENCED_PARAMETER(pDataInfo);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(PhysAddr);
}


//------------------------------------------------------------------------------
//  sets the element and frame count
__inline void
DmaSetElementAndFrameCount (
    DmaDataInfo_t    *pDataInfo,
    UINT32            countElements,
    UINT16            countFrames
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
	UNREFERENCED_PARAMETER(countElements);
	UNREFERENCED_PARAMETER(countFrames);
}


//------------------------------------------------------------------------------
//  Check dma Enable bit
__inline BOOL
IsDmaEnable (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
    return FALSE;
}

//------------------------------------------------------------------------------
//  Check dma status for read write
__inline BOOL
IsDmaActive (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
    return FALSE;
}

//------------------------------------------------------------------------------
//  Stops dma from running
__inline void
DmaStop (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
}


//------------------------------------------------------------------------------
//  Starts dma
__inline void
DmaStart (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
}


//------------------------------------------------------------------------------
//  Starts dma
__inline UINT8*
DmaGetLastWritePos (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
    return ((UINT8*)0xbad1bad1);
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
	UNREFERENCED_PARAMETER(pDataInfo);
    return ((UINT8*)0xbad0bad0);
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
	UNREFERENCED_PARAMETER(pDataInfo);
	UNREFERENCED_PARAMETER(bEnable);
    return TRUE;
}



//------------------------------------------------------------------------------
//  Starts dma
__inline UINT32
DmaGetStatus (
    DmaDataInfo_t    *pDataInfo
    )
{
	UNREFERENCED_PARAMETER(pDataInfo);
	return 0;
}


//------------------------------------------------------------------------------
//  Starts dma
__inline void
DmaClearStatus (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwStatus
    )
{
	UNREFERENCED_PARAMETER(dwStatus);
	UNREFERENCED_PARAMETER(pDataInfo);
}


//------------------------------------------------------------------------------
//  sets the source endian
__inline void
DmaSetSrcEndian (
    DmaDataInfo_t    *pDataInfo,
    BOOL              srcEndian,
    BOOL              srcEndianLock
    )
{
	UNREFERENCED_PARAMETER(srcEndianLock);
	UNREFERENCED_PARAMETER(srcEndian);
	UNREFERENCED_PARAMETER(pDataInfo);
}

//------------------------------------------------------------------------------
//  sets the destination endian
__inline void
DmaSetDstEndian (
    DmaDataInfo_t    *pDataInfo,
    BOOL              dstEndian,
    BOOL              dstEndianLock
    )
{
	UNREFERENCED_PARAMETER(dstEndianLock);
	UNREFERENCED_PARAMETER(dstEndian);
	UNREFERENCED_PARAMETER(pDataInfo);
}

//------------------------------------------------------------------------------
//  Sets DMA Color value
__inline void
DmaSetColor (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwFlag,
    DWORD             dwColor
    )
{
	UNREFERENCED_PARAMETER(dwColor);
	UNREFERENCED_PARAMETER(dwFlag);
	UNREFERENCED_PARAMETER(pDataInfo);
}


#define DUMP_DMA_REGS(a,b)
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //__EDMA_UTILITY_H

