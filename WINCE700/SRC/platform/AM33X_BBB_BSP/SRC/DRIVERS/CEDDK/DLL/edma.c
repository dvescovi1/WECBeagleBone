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
//  File:  dma.c
//
//  This file contains DDK library implementation for platform specific
//  dma operations.
//  
#include "omap.h"
#include "ceddkex.h"
#include "edma_ddk.h"
#include "edma.h"
#include "edma_utility.h"
#include "edma3_common.h"
// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)
#pragma warning (disable: 6322)

// Table of event queues to use for each CC instance
static DWORD g_dwEventQueue[EDMA3_MAX_EDMA3_INSTANCES] = {
    0
}; 
#define EDMA3_DRV_ADDR_MODE_COLOR   0xcc
#define EDMA3_COLOR_BUFFER_SIZE     32   /*bytes*/

//------------------------------------------------------------------------------
//
//  Global:  hEdma
//
//  This global variable holding a reference to the dma driver
//
EDMA3_DRV_Handle hEdma = NULL;

//------------------------------------------------------------------------------
//
//  Global:  LoadDmaDriver
//
//  loads the dma driver and retrieves a handle to it
//
BOOL LoadDmaDriver()
{
    BOOL rc = FALSE;
    EDMA3_DRV_Result errorCode;

    hEdma = EDMA3_DRV_getInstHandle(EDMA3_DEFAULT_PHY_CTRL_INSTANCE, EDMA3_ARM_REGION_ID, &errorCode);
    if (hEdma == NULL || errorCode != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (L"LoadDmaDriver: ERROR - EDMA3_DRV_getInstHandle() failed! Error %08X\r\n",
                              errorCode));
        goto _done;
    }

    rc = TRUE;
_done:
    return rc;
	
}

//------------------------------------------------------------------------------
//
//  Global:  DmaAllocateChannel
//
//  allocates a dma channel of the requested type.  If successful a non-null
//  handle is returned
//
HANDLE DmaAllocateChannel(DWORD dmaType)
{
    BOOL rc = FALSE;
    CeddkDmaContext_t *pContext = NULL;
    EDMA_TRANSFER *pTransfer;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE)
        {
        goto cleanUp;
        }

    // allocate handle for dma
    pContext = (CeddkDmaContext_t*)LocalAlloc(LPTR, sizeof(CeddkDmaContext_t));
    if (pContext == NULL)
        {
        goto cleanUp;
        }

    memset(pContext, 0, sizeof(CeddkDmaContext_t));
    pContext->cookie = DMA_HANDLE_CHANNEL_COOKIE;

    // init a DMA channel
	pTransfer = &pContext->pDmaTransfer;
    
    pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
    pTransfer->nTcc = EDMA3_DRV_TCC_ANY;
	pTransfer->hEvent = NULL;

    // allocate the color DMA buffer
    pTransfer->dmaAdapter.ObjectSize = sizeof(pTransfer->dmaAdapter);
    pTransfer->dmaAdapter.InterfaceType = Internal;
    pTransfer->dmaAdapter.BusNumber = 0;

    pTransfer->pColorBuffer = (UINT32 *)HalAllocateCommonBuffer( &pTransfer->dmaAdapter, EDMA3_COLOR_BUFFER_SIZE, &pTransfer->ColorBufferPA, FALSE );
	
    if (pTransfer->pColorBuffer == NULL)
    {
        RETAILMSG(1 /*ZONE_ERROR*/, (L"ERROR: DmaSetColor: "
             L"Failed Allocate buffer r\n"
            ));
		goto cleanUp;
	}
	
    rc = TRUE;

cleanUp:
    // request dma channel
    if (rc == FALSE && pContext != NULL)
        {
        LocalFree(pContext);
        pContext = NULL;
        }
    
    return pContext;    
}


//------------------------------------------------------------------------------
//
//  Global:  DmaFreeChannel
//
//  free's a dma channel
//
BOOL DmaFreeChannel(HANDLE hDmaChannel)
{
    BOOL rc = FALSE;
    CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;
    EDMA_TRANSFER *pTransfer;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_HANDLE_CHANNEL_COOKIE)
            {
            goto cleanUp;
            }
        
		pTransfer = &pContext->pDmaTransfer;
        // unmap dma registers
        if (pTransfer->nChId != (UINT32)NULL)
        {
            // stop dma, break all links, and reset status
            // unregister for all interrupts
            DmaEnableInterrupts(hDmaChannel, NULL);

            // release reserved dma channel
			if (pTransfer->hEvent)
			{
				CloseHandle(pTransfer->hEvent);
				pTransfer->hEvent = NULL;
			}
			
			if (pTransfer->hError)
			{
				CloseHandle(pTransfer->hError);
				pTransfer->hError = NULL;
			}
			
			if (pTransfer->nChId != EDMA3_DRV_DMA_CHANNEL_ANY)
			{
				EDMA3_DRV_freeChannel(hEdma, pTransfer->nChId);
				pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
			}
            if(pTransfer->pColorBuffer)
        	{
        	    HalFreeCommonBuffer(&pTransfer->dmaAdapter, EDMA3_COLOR_BUFFER_SIZE, 
					pTransfer->ColorBufferPA, pTransfer->pColorBuffer, FALSE );
        	}
        }

        LocalFree(pContext);
        rc = TRUE;
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception free'ing dma handle\r\n"
            ));
        }

cleanUp:
    return rc; 
}


//------------------------------------------------------------------------------
//
//  Global:  DmaEnableInterrupts
//
//  enables interrupts for a dma channel.  The client will be notified of
//  the interrupt by signaling the handle given.  Pass in NULL to disable
//  interrupts
//
BOOL DmaEnableInterrupts(HANDLE hDmaChannel, HANDLE hEvent)
{
    BOOL rc = FALSE;
    //IOCTL_DMA_REGISTER_EVENTHANDLE_IN RegisterEvent;
    CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }
    __try
        {
        if (pContext->cookie != DMA_HANDLE_CHANNEL_COOKIE)
            {
            goto cleanUp;
            }
		/* Disable interrupt */
        if (hEvent == NULL)
		{
		    rc = TRUE;
    		goto cleanUp;
    	}
		//To be implemented for interrupt handling
		
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception trying to enable dma interrupts\r\n"
            ));
        }
cleanUp:
    return rc; 
}


//------------------------------------------------------------------------------
//
//  Global:  DmaGetLogicalChannelId
//
//  returns the logical id
//
DWORD DmaGetLogicalChannelId(HANDLE hDmaChannel)
{
	//TAO: no one calls this function

    DWORD dwId = (DWORD) -1;

    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }

    __try
        {
            CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;

            if (pContext->cookie == DMA_HANDLE_CHANNEL_COOKIE)
            {
               dwId = pContext->pDmaTransfer.nChId;
            }
            if(dwId == -1)
            {
                goto cleanUp;
            }
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception trying to enable dma repeat mode\r\n"
            ));
        }
    
cleanUp:
    return dwId; 
}


//------------------------------------------------------------------------------
//
//  Global:  DmaInterruptDone
//
//  re-enable dma interrupt
//
BOOL DmaInterruptDone(HANDLE hDmaChannel)
{
	//EDMA does not have interrupt support
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Global:  DmaGetLogicalChannel
//
//  returns a pointer to the logical dma channels registers.
//
void* DmaGetLogicalChannel(HANDLE hDmaChannel)
{
     /* Not Available to EDMA, this function is not called from DMA client */
     return (void*)NULL;
}


//------------------------------------------------------------------------------
//
//  Global:  DmaDisableStandby
//
//  disable standby in dma controller
//
BOOL DmaDisableStandby(HANDLE hDmaChannel, BOOL noStandby)
{
	//N.A.

    return TRUE;
}
//------------------------------------------------------------------------------
//  Configures the DMA port
BOOL
DmaConfigure (
    HANDLE            hDmaChannel,
    DmaConfigInfo_t  *pConfigInfo,
    DWORD             syncMap,
    DmaDataInfo_t    *pDataInfo
    )
{
    BOOL rc = FALSE;
    CeddkDmaContext_t *pContext = (CeddkDmaContext_t *)hDmaChannel;	
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA_TRANSFER *pTransfer = &pContext->pDmaTransfer;
	

    // initialize dma DataInfo if necessary, pDataInfo is provided by DMA consumer
    if (pDataInfo != NULL)
	{
    	memset(pDataInfo, 0, sizeof(DmaDataInfo_t));
	}
    else 
		goto cleanUp;

	/* checking parameters */
	if(hDmaChannel == NULL) 
	{
        RETAILMSG(1, (_T("DmaConfigure: hDmaChannel is null: %x\r\n"), hDmaChannel));
 	    goto cleanUp;
	}
	
	pDataInfo->pTransfer = (UINT32 *)&pContext->pDmaTransfer;
	if(pDataInfo->pTransfer == NULL)
	    goto cleanUp;

	/* Save the configuration for later use */
    if(pConfigInfo != NULL)
        memcpy((void *)&pDataInfo->DmaCfgInfo, (void *)pConfigInfo, sizeof(DmaConfigInfo_t));

    /* save event triggered channel id */
    pDataInfo->DmaCfgInfo.syncMap = syncMap;

	/* Allocate channel */
    if(pTransfer->nChId == EDMA3_DRV_DMA_CHANNEL_ANY)
	{
	    /* event triggered channel, use channel id to to request channel */
    	if(syncMap != 0)
    		pTransfer->nChId = syncMap;
    
        result = EDMA3_DRV_requestChannel(hEdma,
                                          &pTransfer->nChId,
                                          &pTransfer->nTcc,
                                          (EDMA3_RM_EventQueue)g_dwEventQueue[0],
                                          (EDMA3_RM_TccCallback)NULL,
                                          NULL);
        if (result != EDMA3_DRV_SOK)
        {
            pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
            RETAILMSG(1, (_T("DmaConfigure: Failed to allocate channel: %d\r\n"), result));
            goto cleanUp;
        }
	}
	
	pDataInfo->hDmaChannel = pTransfer->nChId;

    /* Set TCC */
    result = EDMA3_DRV_setOptField(hEdma,
                                   pContext->pDmaTransfer.nChId,
                                   EDMA3_DRV_OPT_FIELD_TCC,
                                   pContext->pDmaTransfer.nTcc);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("DmaConfigure: Failed to set TCC opt field: %d\r\n"), result));
        goto cleanUp;
    }

	pTransfer->transfer_busy = FALSE;

	if(pTransfer->hEvent == NULL)
	{
    	pTransfer->hEvent = EDMA3_DRV_getTransferEvent(hEdma, (unsigned int)pDataInfo->hDmaChannel);
    	if (!pTransfer->hEvent)
    	{
    		RETAILMSG(1, (_T("DmaSetElementAndFrameCount: Failed to get transfer event for ch %d\r\n"), pTransfer->nChId));
    		goto cleanUp;
    	}
    
    	pTransfer->hError = EDMA3_DRV_getErrorEvent(hEdma, pTransfer->nChId);
    	if (!pTransfer->hError)
    	{
    		RETAILMSG(1, (_T("DmaSetElementAndFrameCount: Failed to get error event for ch %d\r\n"), pTransfer->nChId));
    		goto cleanUp;
    	}
	}
    rc = TRUE;
    
cleanUp:
    return rc;

}


//------------------------------------------------------------------------------
//  Updates dma registers with cached information from DmaConfigInfo.
BOOL
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
void
DmaSetDstBuffer (
    DmaDataInfo_t    *pDataInfo,
    UINT8            *pBuffer,
    DWORD             PhysAddr
    )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;

    // save values
    //
    pDataInfo->pDstBuffer = pBuffer;
    pDataInfo->PhysAddrDstBuffer = PhysAddr;


    result = EDMA3_DRV_setDestParams(hEdma,
    								 (unsigned int)pDataInfo->hDmaChannel,
    								 PhysAddr,
    								 DMA_CCR_DST_AMODE_POST_INC, // pDataInfo->DmaCfgInfo.dstAddrMode,
    								 pDataInfo->DmaCfgInfo.elemSize & 0x3);
    
	
    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (_T("DmaSetDstBuffer: Failed to set dst params: %d\r\n"), result));
		
    }
}


//------------------------------------------------------------------------------
//  sets the src address and buffer
 void
DmaSetSrcBuffer (
        DmaDataInfo_t    *pDataInfo,
        UINT8            *pBuffer,
        DWORD             PhysAddr
        )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;

    if(PhysAddr)
	{
	
        // save values
        //
        pDataInfo->pSrcBuffer = pBuffer;
        pDataInfo->PhysAddrSrcBuffer = PhysAddr;
    
    
        result = EDMA3_DRV_setSrcParams(hEdma,
        								(unsigned int)pDataInfo->hDmaChannel,
        								PhysAddr,
        								pDataInfo->DmaCfgInfo.srcAddrMode,
        								pDataInfo->DmaCfgInfo.elemSize & 0x3);
        if (result != EDMA3_DRV_SOK)
        {
        	RETAILMSG(1, (_T("DmaSetSrcBuffer: Failed to set src params: %d\r\n"), result));
        }
	}
	
}


//------------------------------------------------------------------------------
//  sets the element and frame count
void
DmaSetElementAndFrameCount (
    DmaDataInfo_t    *pDataInfo,
    UINT32            countElements,
    UINT16            countFrames
    )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
	UINT32 Acnt, Bcnt, Ccnt, srcBidx, srcCidx, dstBidx, dstCidx, elem_bytes,temp;

	elem_bytes = 0x1 << (pDataInfo->DmaCfgInfo.elemSize & 0x3);

    /* Settings for FIFO mode*/
	if ((pDataInfo->DmaCfgInfo.srcAddrMode==EDMA3_DRV_ADDR_MODE_FIFO) ||
		(pDataInfo->DmaCfgInfo.dstAddrMode==EDMA3_DRV_ADDR_MODE_FIFO))
	{
    	Acnt = elem_bytes; 
    	Bcnt = countElements;
    	Ccnt = countFrames;
    	
    	srcBidx = (pDataInfo->DmaCfgInfo.srcAddrMode==EDMA3_DRV_ADDR_MODE_FIFO)? 0 : Acnt;
    	srcCidx = (pDataInfo->DmaCfgInfo.srcAddrMode==EDMA3_DRV_ADDR_MODE_FIFO)? 0 : Acnt * Bcnt;
    	dstBidx = (pDataInfo->DmaCfgInfo.dstAddrMode==EDMA3_DRV_ADDR_MODE_FIFO)? 0 : Acnt;
    	dstCidx = (pDataInfo->DmaCfgInfo.dstAddrMode==EDMA3_DRV_ADDR_MODE_FIFO)? 0 : Acnt * Bcnt;
	}
	/* Settings for COlor mode - display driver */
    else if(pDataInfo->DmaCfgInfo.srcAddrMode == EDMA3_DRV_ADDR_MODE_COLOR)
	{
	    if(countElements * countFrames <= EDMA3_COLOR_BUFFER_SIZE)
    	{
    		Acnt = elem_bytes; 
    		Bcnt = countElements * countFrames;
    		Ccnt = 1;
    
    		srcBidx = 0;
    		srcCidx = 0;
    		dstBidx = Acnt;
    		dstCidx = Acnt * Bcnt;
    	}
	    else
		{
		    temp = EDMA3_COLOR_BUFFER_SIZE/elem_bytes;
			
			Acnt = temp; 
			Bcnt = countElements * countFrames / temp ;
			Ccnt = 1;
		
			srcBidx = Acnt;
			srcCidx = 0;
			dstBidx = Acnt;
			dstCidx = 0;
		}
	}
	/* Memory transfer */
	else
	{
    	Acnt = countElements * elem_bytes;
    	Bcnt = countFrames;
    	Ccnt = 1;
		//srcBidx = pDataInfo->DmaCfgInfo.srcFrameIndex/elem_bytes * elem_bytes + countElements * elem_bytes;
        srcBidx = pDataInfo->DmaCfgInfo.srcFrameIndex - 1 + countElements * elem_bytes;		
		srcCidx = 0;
		//dstBidx = pDataInfo->DmaCfgInfo.dstFrameIndex/elem_bytes * elem_bytes + countElements * elem_bytes;
		dstBidx = pDataInfo->DmaCfgInfo.dstFrameIndex -1 + countElements * elem_bytes;
		dstCidx = 0;
	}
	
    result = EDMA3_DRV_setSrcIndex(hEdma,
    							   (unsigned int)pDataInfo->hDmaChannel,
									srcBidx,
									srcCidx);

    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (L"DmaSetElementAndFrameCount: Failed to set src indexing: %d\r\n", result));
		goto cleanUp;
		
    }
    
    result = EDMA3_DRV_setDestIndex(hEdma,
                                    (unsigned int)pDataInfo->hDmaChannel,
									dstBidx,
									dstCidx
                                    );

    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (L"DmaSetElementAndFrameCount: Failed to set dst indexing: %d\r\n", result));
		goto cleanUp;
    }

    result = EDMA3_DRV_setTransferParams(hEdma,
    									 (unsigned int)pDataInfo->hDmaChannel,
    									 Acnt,
    									 Bcnt, 
    									 Ccnt, 
    									 1,
    									 pDataInfo->DmaCfgInfo.synchMode);
    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (_T("DmaSetElementAndFrameCount: Failed to set transfer params: %d\r\n"), result));
		goto cleanUp;
    }

cleanUp:
	return;
}


//------------------------------------------------------------------------------
//  Check dma Enable bit
BOOL
IsDmaEnable (
    DmaDataInfo_t    *pDataInfo
    )
{
    EDMA_TRANSFER *pTransfer;
    if((pDataInfo == NULL) || (pDataInfo->pTransfer == NULL))
	    return FALSE;
	
	pTransfer = (EDMA_TRANSFER *)pDataInfo->pTransfer;
    
    return (pTransfer->transfer_busy);
}

//------------------------------------------------------------------------------
//  Check dma status for read write
BOOL
IsDmaActive (
    DmaDataInfo_t    *pDataInfo
    )
{
    EDMA_TRANS_STATUS channelStatus;
    EDMA_TRANSFER *pTransfer = (EDMA_TRANSFER *)pDataInfo->pTransfer;

	if(!pTransfer->transfer_busy)
		return FALSE;

    channelStatus = EDMA3_DRV_getTransferStatus(hEdma, (unsigned int)pDataInfo->hDmaChannel);
	
    if (channelStatus & EDMA_STAT_EVENT_MISSED)
    {
        RETAILMSG(1, (_T("IsDmaActive: EDMA reports event missed for transfer 1 (0x%x)\r\n"), channelStatus));
    }
    if (channelStatus & EDMA_STAT_TRANSFER_COMPLETE)
    {
        RETAILMSG(0, (_T("IsDmaActive: EDMA reports transfer complete (0x%x)\r\n"), channelStatus));
		pTransfer->transfer_busy = FALSE;
		return FALSE;
    }
	else
	{
		return TRUE;
	}
	
}

//------------------------------------------------------------------------------
//  Stops dma from running
void
DmaStop (
    DmaDataInfo_t    *pDataInfo
    )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA_TRANSFER *pTransfer = (EDMA_TRANSFER *)pDataInfo->pTransfer;

    result = EDMA3_DRV_disableTransfer(hEdma, (unsigned int)pDataInfo->hDmaChannel, pDataInfo->DmaCfgInfo.synchTrigger);
	
    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (_T("DmaStop: Failed to stop: %d\r\n"), result));
		goto cleanUp;
    }
	
	pTransfer->transfer_busy = FALSE;
cleanUp:
	return;
	
}


//------------------------------------------------------------------------------
//  Starts dma
void
DmaStart (
    DmaDataInfo_t    *pDataInfo
    )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA_TRANSFER *pTransfer = (EDMA_TRANSFER *)pDataInfo->pTransfer;

	
	result = EDMA3_DRV_setOptField(hEdma,
								   (unsigned int)pDataInfo->hDmaChannel,
								   EDMA3_DRV_OPT_FIELD_TCINTEN,
								   1);
	if (result != EDMA3_DRV_SOK)
	{
		RETAILMSG(1, (_T("EDT: Failed to set TCINTEN opt field: %d\r\n"), result));
		goto cleanUp;
	}
	
    result = EDMA3_DRV_enableTransfer(hEdma, (unsigned int)pDataInfo->hDmaChannel, pDataInfo->DmaCfgInfo.synchTrigger);
    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (_T("DmaStart: Failed to start EDMA: %d\r\n"), result));
		goto cleanUp;
    }
	
	
    pTransfer->transfer_busy = TRUE;
	
cleanUp:
	return;
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
//  Gets dma ststus
__inline UINT32
DmaGetStatus (
    DmaDataInfo_t    *pDataInfo
    )
{
    EDMA_TRANS_STATUS channelStatus;

    channelStatus = EDMA3_DRV_getTransferStatus(hEdma, (unsigned int)pDataInfo->hDmaChannel);
	return channelStatus;
}


//------------------------------------------------------------------------------
//
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
void
DmaSetColor (
    DmaDataInfo_t    *pDataInfo,
    DWORD             dwFlag,
    DWORD             dwColor
    )
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA_TRANSFER *pTransfer = (EDMA_TRANSFER *)pDataInfo->pTransfer;

	UNREFERENCED_PARAMETER(dwFlag);

	RETAILMSG(1, (_T("DmaSetColor: enter:dwColor %x\r\n"), dwColor));

    pDataInfo->pSrcBuffer = (UINT8 *)pTransfer->pColorBuffer;
	memset(pTransfer->pColorBuffer, dwColor, EDMA3_COLOR_BUFFER_SIZE);
    pDataInfo->PhysAddrSrcBuffer = (ULONG)pTransfer->ColorBufferPA.QuadPart;
    pDataInfo->DmaCfgInfo.srcAddrMode = EDMA3_DRV_ADDR_MODE_COLOR;   
    pDataInfo->DmaCfgInfo.synchMode = EDMA3_DRV_SYNC_AB;
		
    result = EDMA3_DRV_setSrcParams(hEdma,
    								(unsigned int)pDataInfo->hDmaChannel,
    								pDataInfo->PhysAddrSrcBuffer,
    								EDMA3_DRV_ADDR_MODE_INCR,
    								pDataInfo->DmaCfgInfo.elemSize & 0x3);
    if (result != EDMA3_DRV_SOK)
    {
    	RETAILMSG(1, (_T("DmaSetColor: Failed to set src params: %d\r\n"), result));
    }

	return;
}


//------------------------------------------------------------------------------
