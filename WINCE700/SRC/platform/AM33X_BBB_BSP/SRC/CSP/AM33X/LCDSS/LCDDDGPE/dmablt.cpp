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

#include "precomp.h"
#include "dispperf.h"

#include "dma_utility.h"

//------------------------------------------------------------------------------
//  Defines

#define MIN_DMA_WIDTH   32
#define MIN_DMA_HEIGHT  32

//------------------------------------------------------------------------------
//  Structures


//------------------------------------------------------------------------------
//  Globals

CRITICAL_SECTION    g_csDmaLock;

HANDLE              g_hDmaChannel1 = NULL;
DmaDataInfo_t       g_tDmaDataInfo1;

HANDLE              g_hDmaChannel2 = NULL;
DmaDataInfo_t       g_tDmaDataInfo2;

extern OMAPDDGPEGlobals g_Globals;

//------------------------------------------------------------------------------
int
OMAPDDGPE::IsBusy()
{
    int result = 0;

    //  Check for a HW BLT in progress
    if( g_hDmaChannel1 )
    {
        if( IsDmaEnable(&g_tDmaDataInfo1) && IsDmaActive(&g_tDmaDataInfo1) )
            result = 1;
    }

    if( g_hDmaChannel2  )
    {
        if( IsDmaEnable(&g_tDmaDataInfo2) && IsDmaActive(&g_tDmaDataInfo2) )
            result = 1;
    }

    //  Return result
    return result;
}

//------------------------------------------------------------------------------
void
OMAPDDGPE::WaitForNotBusy()
{
    //  Wait for any active DMA operation to complete
    while( IsBusy() );
    return;
}

/* 
  DMAFill is only available with Socs that has SDMA, which has native support for color filling. 
  This feature can be disabled from Display driver register settings - "DMAFill". It is enabled by 
  default if this entry is not available display driver register file.
  
  For EDMA, it is hard to implement this feature. The reasons are as follows:
  a.  When implemented in EDMA FIFO mode, if Acnt is set to the size of the color parameter,
  and using AB sync mode, then Bcnt(16 bits number) is not big enough to fill the whole screen.
  There is not evernt available to use multiple AB transfers, so Ccnt can only be set to 1.
  
  b.  To overcome the problem in (a), and use pre-allocated the 32 bytes buffer (for example),  
  it is hard to handle the case to fill color in a window which is not 32 bytes aligned. This is 
  very common on desktop operation. 

 */
//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::DMAFill(
    GPEBltParms* pParms
    )
{
    SCODE   result = S_OK;
    RECTL*  prclDst = pParms->prclDst;
    DWORD   dwWidth  = prclDst->right - prclDst->left;
    DWORD   dwHeight = prclDst->bottom - prclDst->top;
    DWORD   dwStride = pParms->pDst->Stride();
    DWORD   dwOffset = 0;

    OMAPDDGPESurface*   pOmapSurf = (OMAPDDGPESurface*) pParms->pDst;

    DmaConfigInfo_t DmaSettings = {
        0,                          // elemSize
        1,                          // srcElemIndex
        1,                          // srcFrameIndex
        DMA_CCR_SRC_AMODE_CONST,    // srcAddrMode
        1,                          // dstElemIndex
        1,                          // dstFrameIndex
        DMA_CCR_DST_AMODE_DOUBLE,   // dstAddrMode
        FALSE,                      // dmaPrio
        DMA_SYNCH_TRIGGER_NONE,     // synchTrigger
        DMA_SYNCH_NONE,             // synchMode
        0,                          // interrupts
        0                           // syncMap
    };

    //  Check that a DMA Blt is worth doing
    if( dwWidth < MIN_DMA_WIDTH || dwHeight < MIN_DMA_HEIGHT )
    {
        if (g_Globals.m_dwEnableNeonBlts)
        {
            pParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DesignateBlt;
            return OMAPDDGPE::DesignateBlt(pParms);
        }
		else
		{
            pParms->pBlt = &GPE::EmulatedBlt;
            return GPE::EmulatedBlt(pParms);
		}
    }

    //  Allocate DMA channel
    if( g_hDmaChannel1 == NULL )
    {
        g_hDmaChannel1 = DmaAllocateChannel(DMA_TYPE_SYSTEM);
        InitializeCriticalSection( &g_csDmaLock );
    }

    //  Lock access to DMA registers
    EnterCriticalSection( &g_csDmaLock );

    //  Wait for any pending operations to complete
    WaitForNotBusy();

    //  Configure DMA channel for FILL operation
    switch( pParms->pDst->BytesPerPixel() )
    {
        case 1:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S8;
            DmaSettings.dstFrameIndex = 1 + dwStride - dwWidth;
            dwOffset = prclDst->top * dwStride + prclDst->left;
            break;

        case 2:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S16;
            DmaSettings.dstFrameIndex = 1 + dwStride - 2*dwWidth;
            dwOffset = prclDst->top * dwStride + 2 * prclDst->left;
            break;

        case 4:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S32;
            DmaSettings.dstFrameIndex = 1 + dwStride - 4*dwWidth;
            dwOffset = prclDst->top * dwStride + 4 * prclDst->left;
            break;
    }

    //  Clear any clipping rect for the operation
    pOmapSurf->OmapSurface()->SetClipping( NULL );

    //  Enable bursting for improved memory performance
    DmaSettings.elemSize |= DMA_CSDP_DST_BURST_64BYTES_16x32_8x64 | DMA_CSDP_DST_PACKED;


    //  Configure the DMA channel
    DmaConfigure( g_hDmaChannel1, &DmaSettings, 0, &g_tDmaDataInfo1 );

    DmaSetColor( &g_tDmaDataInfo1, DMA_CCR_CONST_FILL_ENABLE, (DWORD)pParms->solidColor );
    DmaSetSrcBuffer( &g_tDmaDataInfo1, NULL, 0 );
    DmaSetDstBuffer( &g_tDmaDataInfo1, NULL, pOmapSurf->OmapSurface()->PhysicalAddr() + dwOffset );
    DmaSetElementAndFrameCount( &g_tDmaDataInfo1, dwWidth, (UINT16) dwHeight );

    //  Start the DMA operation
    DmaStart( &g_tDmaDataInfo1 );

    //  Unlock access to DMA registers
    LeaveCriticalSection( &g_csDmaLock );

    return result;
}


//------------------------------------------------------------------------------
SCODE
OMAPDDGPE::DMASrcCopy(
    GPEBltParms* pParms
    )
{
    SCODE   result = S_OK;
    BOOL    bDualDMA = FALSE;
    RECTL*  prclSrc = pParms->prclSrc;
    RECTL*  prclDst = pParms->prclDst;
    DWORD   dwPixelSize = pParms->pDst->BytesPerPixel();
    DWORD   dwWidth  = prclDst->right - prclDst->left;
    DWORD   dwHeight = prclDst->bottom - prclDst->top;
    DWORD   dwSrcStride = pParms->pSrc->Stride();
    DWORD   dwDstStride = pParms->pDst->Stride();
    DWORD   dwSrcOffset = 0;
    DWORD   dwDstOffset = 0;
    DWORD   dwSrcMidpoint = 0;
    DWORD   dwDstMidpoint = 0;
    DWORD   dwWidth1 = dwWidth,
            dwWidth2 = dwWidth;
    DWORD   dwHeight1 = dwHeight,
            dwHeight2 = dwHeight;

    OMAPDDGPESurface*   pOmapSrcSurf = (OMAPDDGPESurface*) pParms->pSrc;
    OMAPDDGPESurface*   pOmapDstSurf = (OMAPDDGPESurface*) pParms->pDst;

    DmaConfigInfo_t DmaSettings = {
        0,                          // elemSize
        1,                          // srcElemIndex
        1,                          // srcFrameIndex
        DMA_CCR_SRC_AMODE_DOUBLE,   // srcAddrMode
        1,                          // dstElemIndex
        1,                          // dstFrameIndex
        DMA_CCR_DST_AMODE_DOUBLE,   // dstAddrMode
        FALSE,                      // dmaPrio
        DMA_SYNCH_TRIGGER_NONE,     // synchTrigger
        DMA_SYNCH_NONE,             // synchMode
        0,                          // interrupts
        0                           // syncMap
    };

    //  Check that a DMA Blt is worth doing
    if( dwWidth < MIN_DMA_WIDTH || dwHeight < MIN_DMA_HEIGHT || 
	    /* EDMA does not support Transparent copy, using Neon or Emulated BLT */
		((pParms->bltFlags & BLT_TRANSPARENT) && !g_Globals.m_dwDMAFill))
    {
        if (g_Globals.m_dwEnableNeonBlts)
        {
        pParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &OMAPDDGPE::DesignateBlt;
        return OMAPDDGPE::DesignateBlt(pParms);
		}
		else
		{
            pParms->pBlt = &GPE::EmulatedBlt;
            return GPE::EmulatedBlt(pParms);
		}
    }

    //  Allocate DMA channels
    if( g_hDmaChannel1 == NULL )
    {
        g_hDmaChannel1 = DmaAllocateChannel(DMA_TYPE_SYSTEM);
        InitializeCriticalSection( &g_csDmaLock );
    }

    if( g_hDmaChannel2 == NULL )
    {
        g_hDmaChannel2 = DmaAllocateChannel(DMA_TYPE_SYSTEM);
    }

    //  Lock access to DMA registers
    EnterCriticalSection( &g_csDmaLock );

    //  Wait for any pending operations to complete
    WaitForNotBusy();

    //  Configure DMA channel for SRCCPY operation
    switch( pParms->pDst->BytesPerPixel() )
    {
        case 1:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S8;
            break;

        case 2:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S16;
            break;

        case 4:
            DmaSettings.elemSize = DMA_CSDP_DATATYPE_S32;
            break;
    }


    //  Compute element indexing, frame indexing and starting offset for both surfaces
    //  Note that both xPos and yPos will never be both < 0
    //  Also note that xPos !=1 prevents bursting and actually slows DMA down below memcpy speeds
    if( pParms->xPositive )
    {
        //  Index x axis in positive direction (left to right)
        DmaSettings.srcElemIndex = 1;
        DmaSettings.dstElemIndex = 1;

        //  Offset from left side of surface
        dwSrcOffset = dwPixelSize * prclSrc->left;
        dwDstOffset = dwPixelSize * prclDst->left;
    }
    else
    {
        //  Index x axis in negative direction (right to left)
        DmaSettings.srcElemIndex = 1 - 2*dwPixelSize;
        DmaSettings.dstElemIndex = 1 - 2*dwPixelSize;

        //  Offset from right side of surface
        dwSrcOffset = dwPixelSize * (prclSrc->right - 1);
        dwDstOffset = dwPixelSize * (prclDst->right - 1);
    }

    if( pParms->yPositive )
    {
        //  Index y axis in positive direction (top to bottom)
        DmaSettings.srcFrameIndex = DmaSettings.srcElemIndex + dwSrcStride;
        DmaSettings.dstFrameIndex = DmaSettings.dstElemIndex + dwDstStride;

        if( pParms->xPositive )
        {
            DmaSettings.srcFrameIndex -= dwWidth*dwPixelSize;
            DmaSettings.dstFrameIndex -= dwWidth*dwPixelSize;
        }
        else
        {
            DmaSettings.srcFrameIndex += dwWidth*dwPixelSize;
            DmaSettings.dstFrameIndex += dwWidth*dwPixelSize;
        }

        //  Offset from top side of surface
        dwSrcOffset = dwSrcOffset + prclSrc->top * dwSrcStride;
        dwDstOffset = dwDstOffset + prclDst->top * dwDstStride;
    }
    else
    {
        //  Index y axis in negative direction (bottom to top)
        DmaSettings.srcFrameIndex = DmaSettings.srcElemIndex - dwSrcStride;
        DmaSettings.dstFrameIndex = DmaSettings.dstElemIndex - dwDstStride;

        if( pParms->xPositive )
        {
            DmaSettings.srcFrameIndex -= dwWidth*dwPixelSize;
            DmaSettings.dstFrameIndex -= dwWidth*dwPixelSize;
        }
        else
        {
            DmaSettings.srcFrameIndex += dwWidth*dwPixelSize;
            DmaSettings.dstFrameIndex += dwWidth*dwPixelSize;
        }

        //  Offset from bottom side of surface
        dwSrcOffset = dwSrcOffset + (prclSrc->bottom - 1) * dwSrcStride;
        dwDstOffset = dwDstOffset + (prclDst->bottom - 1) * dwDstStride;
    }

    //
    //  Check for fast dual DMA cases
    //

    //  Different src and dst surfaces can use dual DMA
    if( pOmapSrcSurf != pOmapDstSurf )
    {
        //  Split work in half vertically
        dwHeight1 = dwHeight/2;
        dwHeight2 = dwHeight - dwHeight1;

        dwSrcMidpoint = (pParms->yPositive) ? dwSrcOffset + dwHeight1*dwSrcStride : dwSrcOffset - dwHeight1*dwSrcStride;
        dwDstMidpoint = (pParms->yPositive) ? dwDstOffset + dwHeight1*dwDstStride : dwDstOffset - dwHeight1*dwDstStride;

        bDualDMA = TRUE;
    }


    //  Clear any clipping rect for the operation
    pOmapSrcSurf->OmapSurface()->SetClipping( NULL );
    pOmapDstSurf->OmapSurface()->SetClipping( NULL );


    //  Enable bursting for improved memory performance
    DmaSettings.elemSize |= DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64 | DMA_CSDP_SRC_PACKED;
    DmaSettings.elemSize |= DMA_CSDP_DST_BURST_64BYTES_16x32_8x64 | DMA_CSDP_DST_PACKED;


    //  Configure the DMA channel
    DmaConfigure( g_hDmaChannel1, &DmaSettings, 0, &g_tDmaDataInfo1 );

    DmaSetSrcBuffer( &g_tDmaDataInfo1, NULL, pOmapSrcSurf->OmapSurface()->PhysicalAddr() + dwSrcOffset );
    DmaSetDstBuffer( &g_tDmaDataInfo1, NULL, pOmapDstSurf->OmapSurface()->PhysicalAddr() + dwDstOffset );
    DmaSetElementAndFrameCount( &g_tDmaDataInfo1, dwWidth1, (UINT16) dwHeight1);

    if( bDualDMA )
    {
        DmaConfigure( g_hDmaChannel2, &DmaSettings, 0, &g_tDmaDataInfo2 );

        DmaSetSrcBuffer( &g_tDmaDataInfo2, NULL, pOmapSrcSurf->OmapSurface()->PhysicalAddr() + dwSrcMidpoint );
        DmaSetDstBuffer( &g_tDmaDataInfo2, NULL, pOmapDstSurf->OmapSurface()->PhysicalAddr() + dwDstMidpoint );
        DmaSetElementAndFrameCount( &g_tDmaDataInfo2, dwWidth2, (UINT16) dwHeight2 );
    }

    //  Configure for transparent copy if requested
    if( (pParms->bltFlags & BLT_TRANSPARENT) && g_Globals.m_dwDMAFill )
    {   
        DmaSetColor( &g_tDmaDataInfo1, DMA_CCR_TRANSPARENT_COPY_ENABLE, (DWORD)pParms->solidColor );
        if( bDualDMA )
            DmaSetColor( &g_tDmaDataInfo2, DMA_CCR_TRANSPARENT_COPY_ENABLE, (DWORD)pParms->solidColor );
    }
	else if(pParms->bltFlags & BLT_TRANSPARENT)
	{
	    RETAILMSG(1, (L"DMASrcCopy: Transparent copy is not supported.\r\n"));
	}


    //  Start the DMA operation(s)
    DmaStart( &g_tDmaDataInfo1 );

    if( bDualDMA )
        DmaStart( &g_tDmaDataInfo2 );

    //  Unlock access to DMA registers
    LeaveCriticalSection( &g_csDmaLock );

    return result;
}

