// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file CppiDma.c
//! \brief AM33x USB CPPI DMA Controller Source File. This Source File
//!        contains the routines which directly interact with the
//!        CPPI N-Channel DMA Controller.

#pragma warning (disable: 4206)

#ifdef CPPI_DMA_SUPPORT

#pragma warning(push)
#pragma warning(disable: 4115 4127 4214)
/* WINDOWS CE Public Includes ---------------------------------------------- */
#include <windows.h>
#include <nkintr.h>
#include <types.h>
#include <ddkreg.h>
#include <ceddk.h>
#include <usbfn.h>

/* PLATFORM Specific Includes ---------------------------------------------- */
#include "am33x.h"
#include "am33x_usb.h"
#include "am33x_usbcdma.h"
#include "UsbFnPdd.h"
#include "CppiDma.h"
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4053)

/* Global CPPI Abstraction Object */
static struct cppi *f_CppiObj = NULL;

/* Local routines */
static int  cppiChannelAbort ( struct dma_channel *);
static void cppiNextSegment ( USBFNPDDCONTEXT *pPdd, struct cppi_channel *tx );
void cppiCompletionCallback ( USBFNPDDCONTEXT *pPdd );
static void cppiProcessCompletionEvent ( USBFNPDDCONTEXT *pPdd, void *ptr);
static void cppiProcessCompletedPacket ( USBFNPDDCONTEXT *pPdd, 
					HOST_DESCRIPTOR *hdPtr, struct cppi_channel *chanPtr );
static void cppiProcessCompletedTeardown ( USBFNPDDCONTEXT *pPdd,
					TEARDOWN_DESCRIPTOR *tdPtr, struct cppi_channel *chanPtr);
static void cppiQueuePush ( CSL_CppiRegs *pCppiRegs, BYTE queueNo, void *pDescriptor);
static void cppiQueuePush ( CSL_CppiRegs *pCppiRegs, BYTE queueNo, void *pDescriptor);
static BOOL cppiPoolInit ( struct cppi *cppi );
static void cppiPoolDeinit ( struct cppi *cppi );
static void cppiHdPoolInit ( struct cppi *cppi );
static HOST_DESCRIPTOR * cppiHdAlloc( struct cppi *cppi );
static void cppiHdFree ( struct cppi *cppi, HOST_DESCRIPTOR *hd );
static UINT32 cppiDescriptorVAtoPA ( void *va );
static void * cppiDescriptorPAtoVA ( UINT32 pa );
static void cppiDumpRx ( struct cppi_channel *c );
static void cppiDumpTx ( struct cppi_channel *c );

//========================================================================
BOOL cppiPoolInit ( struct cppi *cppi )
// Initializes the Descriptor Pool for the given CPPI Object.
// We carve out the Host Descriptors and Teardown Descriptors
// from the allocated Memory Pool.
{
    unsigned descriptorCount;

    DEBUGCHK(cppi != NULL);
    if (cppi == NULL)        return FALSE;
    if (cppi->pool != NULL)  return TRUE; /* Only initialise once */

    descriptorCount = cppi->pPdd->descriptorCount;
    DEBUGMSG(ZONE_INIT, (L"+cppiPoolInit: %u HDs\r\n", descriptorCount));

    InitializeCriticalSection(&cppi->poolLock);

	LOCK_HD_POOL(cppi);

    cppi->hUsbCdma = USBCDMA_RegisterUsbModule( (UINT16)descriptorCount,
        CPPI_HD_SIZE, &cppi->paPool, &cppi->pool, cppiCompletionCallback, cppi->pPdd, cppi->pPdd->dwIndex);



    cppi->poolMaxSize = descriptorCount * CPPI_HD_SIZE;
    cppi->poolUsed = 0;
    cppi->poolFree = cppi->poolMaxSize;
    cppi->poolHead = NULL;

    UNLOCK_HD_POOL(cppi);

    ERRORMSG(cppi->pool == NULL, (L"ERROR: Failed to allocate %u bytes\r\n", cppi->poolMaxSize));
    DEBUGMSG(ZONE_INIT, (L"-cppiPoolInit: %s - Allocated space for %u HDs\r\n",
				(cppi->pool != NULL) ? L"SUCCEEDED" : L"FALIED", descriptorCount));

    cppiHdPoolInit(cppi);
    return (cppi->pool != NULL);
}

//========================================================================
void cppiPoolDeinit ( struct cppi *cppi )
// brief Frees all the descriptors for the controller
{
    DEBUGCHK(cppi != NULL);
    if (cppi == NULL) return;
    if (cppi->pool == NULL)  return; // Only free once 

    DEBUGMSG(ZONE_INIT, (L"+cppiPoolDeinit\r\n"));

    LOCK_HD_POOL(cppi);

    USBCDMA_DeregisterUsbModule(cppi->hUsbCdma);

    cppi->poolHead = NULL;
    cppi->poolFree = 0;
    cppi->poolUsed = 0;
    cppi->poolMaxSize = 0;
    cppi->paPool.QuadPart = 0;
    cppi->pool = NULL;

    UNLOCK_HD_POOL(cppi);

    DeleteCriticalSection(&cppi->poolLock);

    DEBUGMSG(ZONE_INIT, (L"-cppiPoolDeinit\r\n"));
}

void cppiHdPoolInit ( struct cppi *cppi )
{
    unsigned descriptorCount = 0;
    unsigned n				 = 0;

    DEBUGCHK(cppi != NULL);
    if (cppi == NULL) return;
    if (cppi->poolUsed) goto done; // Only initialise once

    descriptorCount = cppi->pPdd->descriptorCount;
    DEBUGMSG(ZONE_INIT, (L"+cppiPoolInit: %u HDs\r\n", descriptorCount));

    if (cppi->pool == NULL) {
        ERRORMSG(1, (L"ERROR: cppiHdPoolInit before cppiPoolInit!\r\n"));
        goto done;
    }

    LOCK_HD_POOL(cppi);

    /* Build the Host Descriptor Free list */
    for (n = 0; n < descriptorCount; n++) {
        HOST_DESCRIPTOR* hd = NULL;

        /* Allocate Memory for this Host Descriptor from our Pool. Have to return Physical Address
           back so that it helps in easier setup of CPPI Transfers */
        if (cppi->poolFree < CPPI_HD_SIZE) {
            ERRORMSG(TRUE, (L"+cppiPoolInit Unable to Allocate Descriptors !!!\r\n"));
            break;
        } else {
            hd = (HOST_DESCRIPTOR *)((UINT32)cppi->pool + cppi->poolUsed);
            /* Set descriptor type to Host */
            hd->DescInfo = (UINT32)(USB_CPPI41_DESC_TYPE_HOST << USB_CPPI41_DESC_TYPE_SHIFT);
            /* For every descriptor, we also maintain Physical Address */
            hd->addr = (cppi->paPool.LowPart + cppi->poolUsed);

            cppi->poolUsed += CPPI_HD_SIZE;
            cppi->poolFree -= CPPI_HD_SIZE;
            cppiHdFree(cppi, hd);
            PRINTMSG(ZONE_INIT, (L"HD %04u: PAddr 0x%08x VAddr 0x%08x\r\n", n, hd->addr, hd));
        }
    }

    UNLOCK_HD_POOL(cppi);
done:
    PRINTMSG(ZONE_INIT, (L"-cppiPoolInit: Allocated %u HDs GlobalPool Free %u bytes\r\n", n, cppi->poolFree));

    return;
}

//========================================================================
HOST_DESCRIPTOR * cppiHdAlloc( struct cppi *cppi )
//  Allocates a Host Descriptor from the Desc Free Pool
{
    HOST_DESCRIPTOR *hd;

    DEBUGCHK(cppi != NULL);
    if (cppi == NULL) return NULL;

    LOCK_HD_POOL(cppi);
    hd = cppi->poolHead;
    if (hd != NULL)
        cppi->poolHead = hd->next;

    UNLOCK_HD_POOL(cppi);

    if (hd == NULL) {
        ERRORMSG(1, (L"cppiHdAlloc: FAILED - Out of descriptors (increase DescriptorCount registry setting)!\r\n"));
    } else {
        ERRORMSG(((UINT32)hd & (CPPI_HD_ALIGN - 1)), (L"cppiHdAlloc: Misaligned descriptor - 0x%08x!\r\n", hd));
    }

    return hd;
}

//========================================================================
void cppiHdFree ( struct cppi *cppi, HOST_DESCRIPTOR *hd )
// Frees the given Descriptor and returns it back to the Desc Free Pool.
{
    UINT32 type;

    DEBUGCHK(cppi != NULL);
    if (cppi == NULL) return;

    DEBUGCHK(hd != NULL);
    if (hd == NULL) return;

    type = (hd->DescInfo & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;
    if (type != USB_CPPI41_DESC_TYPE_HOST) {
        ERRORMSG(TRUE, (L"cppiHdFree: Not a Host descriptor type %u\r\n", type));
        return;
    }

    LOCK_HD_POOL(cppi);
    hd->next = cppi->poolHead;
    cppi->poolHead = hd;
    UNLOCK_HD_POOL(cppi);
}

UINT32 cppiDescriptorVAtoPA (void *va)
{
    UINT32 pa = 0;

    if (va != NULL) {
        UINT32 vaPoolBase  = (UINT32)f_CppiObj->pool;
        UINT32 vaPoolLimit = (UINT32)f_CppiObj->pool + f_CppiObj->poolMaxSize;

        if (((UINT32)va < vaPoolBase) || ((UINT32)va > vaPoolLimit))
            pa = USBCDMA_DescriptorVAtoPA(f_CppiObj->hUsbCdma, va);
        else
            pa = f_CppiObj->paPool.LowPart + ((UINT32)va - vaPoolBase);
    }

    return pa;
}

void * cppiDescriptorPAtoVA ( UINT32 pa )
{
    void *va = NULL;

    if (pa != 0) {
        UINT32 paPoolBase  = f_CppiObj->paPool.LowPart;
        UINT32 paPoolLimit = f_CppiObj->paPool.LowPart + f_CppiObj->poolMaxSize;

        if ((pa < paPoolBase) || (pa > paPoolLimit)){
            va = USBCDMA_DescriptorPAtoVA(f_CppiObj->hUsbCdma, pa);
        } else
            va = (void *)(((UINT32)f_CppiObj->pool) + (pa - paPoolBase));
    }

    return va;
}

void cppiQueuePush(CSL_CppiRegs *pCppiRegs, BYTE queueNo, void *pDescriptor)
{
    UINT32 value = 0;

    DEBUGCHK(pCppiRegs != NULL);
    if (pCppiRegs == NULL) return;

    if (pDescriptor != NULL) {
        UINT32 addr  = cppiDescriptorVAtoPA(pDescriptor);
        UINT32 size  = 0;
        UINT32 type = (*(UINT32*)pDescriptor & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;

		switch (type) {
			case USB_CPPI41_DESC_TYPE_HOST:     size = (CPPI_HD_SIZE - 24) / 4; break;
	        case USB_CPPI41_DESC_TYPE_TEARDOWN: size = (USB_CPPI_TD_SIZE - 24) / 4; break;
			default: ERRORMSG(TRUE, (L"Invalid descriptor type %u\r\n", type));
        }

        value = ((addr & QMGR_QUEUE_N_REG_D_DESC_ADDR_MASK) | (size & QMGR_QUEUE_N_REG_D_DESCSZ_MASK   ));
    }

    pCppiRegs->QMQUEUEMGMT[queueNo].QCTRLD = value;

    PRINTMSG(/*ZONE_PDD_DMA*/0, (L"cppiQueuePush: queue %u, value 0x%08x\r\n", queueNo, value));
}

static void * cppiQueuePop(CSL_CppiRegs *pCppiRegs, BYTE queueNo)
{
    UINT32 value;

    DEBUGCHK(pCppiRegs != NULL);
    if (pCppiRegs == NULL) return NULL;

    value = pCppiRegs->QMQUEUEMGMT[queueNo].QCTRLD & QMGR_QUEUE_N_REG_D_DESC_ADDR_MASK;
    PRINTMSG(ZONE_PDD_DMA, (L"cppiQueuePop: queue %u, value 0x%08x\r\n",
		queueNo, value));

    return cppiDescriptorPAtoVA(value);
}

//========================================================================
void cppiCompletionCallback ( USBFNPDDCONTEXT *pPdd )
// Processes all the CPPI Tx and Rx Completion Events.
// Takes appropriate actions based on the various events.
{
    CSL_CppiRegs *pCppiRegs = NULL;
    UINT32 pending2			= 0;
    UINT32 pending3			= 0;
	UINT32 tx_cmpl;
	UINT32 rx_cmpl;
	BYTE   qnum;
	void *ptr = NULL;

    DEBUGCHK(pPdd != NULL);
    if (pPdd == NULL) return;

    pCppiRegs = pPdd->pCppiRegs;

	pending2 = pCppiRegs->PEND2;
	pending3 = pCppiRegs->PEND3;

	tx_cmpl = (pending2 >> 29) | ((pending3 << 3) & 0xFFF);
	rx_cmpl = (pending3 & 0x0FFFE000) >> 13;	
	
    PRINTMSG(ZONE_PDD_DMA, (L"+cppiCompletionCallback p2-%08x p3-%08x tx%08x rx%08x\r\n",
		 pending2, pending3, tx_cmpl, rx_cmpl));

	if (rx_cmpl){
		for (qnum = 109; rx_cmpl != 0; rx_cmpl >>= 1, qnum++){
			while((ptr = cppiQueuePop(pCppiRegs, qnum)) != NULL){
				cppiProcessCompletionEvent(pPdd, ptr);
			}
		}
	}
	if (tx_cmpl){
		for (qnum = 93; tx_cmpl != 0; tx_cmpl >>= 1, qnum++){
			while((ptr = cppiQueuePop(pCppiRegs, qnum)) != NULL){
				cppiProcessCompletionEvent(pPdd, ptr);
			}
		}
	}

    PRINTMSG(ZONE_PDD_DMA, (L"-cppiCompletionCallback\r\n"));
}


static void cppiProcessCompletionEvent ( USBFNPDDCONTEXT *pPdd, void *ptr)
{
    UINT32 type;

    DEBUGCHK(pPdd != NULL);
    DEBUGCHK(ptr  != NULL);
    if (pPdd == NULL || ptr == NULL) return;

    type = (*(UINT32*)ptr & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;
  
	switch (type){
		case USB_CPPI41_DESC_TYPE_HOST:
        {
            HOST_DESCRIPTOR *hdPtr = (HOST_DESCRIPTOR *)ptr;
            struct cppi *cppi = f_CppiObj;
            struct cppi_channel *chanPtr = NULL;
            BYTE chanNum  = (BYTE)((hdPtr->TagInfo2 & 0x1f0) >> 4);
            BOOL transmit = (hdPtr->TagInfo2 & 0x200) ? TRUE : FALSE;

			chanPtr = (transmit) ? cppi->txCppi + chanNum : cppi->rxCppi + chanNum;
            cppiProcessCompletedPacket(pPdd, hdPtr, chanPtr);
            break;
        }

		case USB_CPPI41_DESC_TYPE_TEARDOWN: 
        {
            TEARDOWN_DESCRIPTOR *tdPtr = (TEARDOWN_DESCRIPTOR *)ptr;
            struct cppi *cppi = f_CppiObj;
            struct cppi_channel *chanPtr = NULL;
            BYTE chanNum  = (BYTE)((tdPtr->DescInfo & 0x0001f) >> 0);
            BOOL transmit = (tdPtr->DescInfo & 0x10000) ? FALSE : TRUE;
            
			chanPtr = (transmit) ? cppi->txCppi + (chanNum - cppi->chanOffset) :
								   cppi->rxCppi + (chanNum - cppi->chanOffset);

            cppiProcessCompletedTeardown(pPdd, tdPtr, chanPtr);
            break;
        }

    default:
        ERRORMSG(TRUE, (L"Unknown descriptor type %u\r\n", type));
    }
}

static void cppiProcessCompletedPacket ( USBFNPDDCONTEXT     *pPdd,
    HOST_DESCRIPTOR      *hdPtr, struct cppi_channel *chanPtr )
{
    BYTE   chanNum = chanPtr->channelNo;
    BYTE   epNum   = chanNum + 1;
    MGC_pfDmaCompletion pfnDmaCompleted = NULL;
    BOOL transferComplete = TRUE;
    UINT32 buffLen = 0;
	UINT32 timeout = 50;

#ifndef SHIP_BUILD
    WCHAR *chanDir = chanPtr->transmit ? L"TX" : L"RX";
#endif

    PRINTMSG(ZONE_PDD_DMA, (L"+cppiProcessCompletedPacket: %s Ch %u (EP %u)\r\n", chanDir, chanNum, epNum));
    if (hdPtr == NULL)  {
        PRINTMSG(ZONE_WARNING, (L"-cppiProcessCompletedPacket: %s Ch %u (EP %u), NULL HD\r\n", chanDir, chanNum, epNum));
        return;
    }

    ERRORMSG(hdPtr->NextPtr != 0, (L"cppiProcessCompletedPacket: %s Ch %u (EP %u), Linked HD\r\n",
        chanDir, chanNum, epNum));
    
    LOCK_ENDPOINT(pPdd); /* For overlapped IO need to protect channel vars */

	if (!(chanPtr->transmit)){
		while(--timeout){
			buffLen = (hdPtr->BuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK);
			if (buffLen != 0)
				break;
			StallExecution(1);
		}
		if (buffLen == 0){
			UNLOCK_ENDPOINT(pPdd);
			ERRORMSG(1,(L"Race condition!!! rxBD read before updated by DMA\r\n"));
			return;
		}
	} else {
		buffLen = (hdPtr->BuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK);
	}


    /* Check for zero length packet */
    if (hdPtr->PacketInfo & USB_CPPI41_PKT_FLAGS_ZLP)
		buffLen = 0;

    chanPtr->actualLen += buffLen;

    if (chanPtr->pEndPt == NULL)  {
        RETAILMSG(ZONE_WARNING, (L"cppiProcessCompletedPacket: %s Ch %u (EP %u) [Q%u, %s], NULL pEndPt\r\n",
            chanDir, chanNum, epNum, chanPtr->queueNo, chanPtr->transmit ? L"TX" : L"RX"));
        goto done;
    }

    if (chanPtr->isTeardownPending) {
		DEBUGMSG(ZONE_WARNING, (L"cppiProcessCompletedPacket: %s Ch %u (EP %u) - Dropped (pending teardown)!\r\n",
            chanDir, chanNum, epNum));
        goto done;
    }

    if (chanPtr->pEndPt->endpointType != USB_ENDPOINT_TYPE_ISOCHRONOUS){
        if ((chanPtr->transmit) && (chanPtr->actualLen < chanPtr->transferSize)){
            cppiNextSegment(pPdd, chanPtr);
            transferComplete = FALSE;
        } else if ((!chanPtr->transmit) &&
                 (chanPtr->actualLen < chanPtr->transferSize) &&
                 (buffLen != 0) &&
                 (buffLen == (hdPtr->OrigBuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK)))
        {
            cppiNextSegment(pPdd, chanPtr);
            transferComplete = FALSE;
        }
    }

    if (transferComplete) {
//        if(!chanPtr->transmit) // Disable Rx for this channel until a new BD has been supplied
//            USBCDMA_ConfigureScheduleRx(chanPtr->channelNo, FALSE);

        if (chanPtr->pEndPt->endpointType != USB_ENDPOINT_TYPE_ISOCHRONOUS) {
            chanPtr->Channel.bStatus = MGC_DMA_STATUS_FREE;
            pfnDmaCompleted = chanPtr->pfnDmaCompleted;
        } else {
            chanPtr->nISOHDQueued--;
            chanPtr->nISOHDLastIndex = hdPtr->Index;
            if (chanPtr->nISOHDLastIndex == chanPtr->nISOHDPerTransfer)
                chanPtr->nISOHDLastIndex = 0;

            // Make callback when we see the required HD
            if (hdPtr->Index == chanPtr->nISOHDForCallback) {
                PRINTMSG(ZONE_PDD_ISO,
                  (L"cppiProcessCompletedPacket: %s Ch %u (EP %u), ISO HD index %d, total %d, actualLen %d, callback\r\n",
                   chanDir, chanNum, epNum, hdPtr->Index, chanPtr->nISOHDPerTransfer, chanPtr->actualLen));

                chanPtr->Channel.bStatus = MGC_DMA_STATUS_FREE;
                pfnDmaCompleted = chanPtr->pfnDmaCompleted;
            }
        }
    }

done:
    cppiHdFree(f_CppiObj, hdPtr);
    UNLOCK_ENDPOINT(pPdd);

    if (pfnDmaCompleted != NULL){ // Invoke DMA Completion Handler
    	if (chanPtr->transmit)
			StallExecution(20);
        pfnDmaCompleted(pPdd, chanNum, epNum);
    }

    PRINTMSG(/*ZONE_PDD_DMA*/0, (L"-cppiProcessCompletedPacket\r\n"));
}


static void cppiProcessCompletedTeardown ( USBFNPDDCONTEXT *pPdd,
			TEARDOWN_DESCRIPTOR *tdPtr, struct cppi_channel *chanPtr )
{
#ifndef SHIP_BUILD
    BYTE   chanNum = chanPtr->channelNo;
    WCHAR *chanDir = chanPtr->transmit ? L"TX" : L"RX";
#endif

    DEBUGMSG(ZONE_PDD_DMA, (L"+cppiProcessCompletedTeardown: %s Ch %u\r\n",chanDir, chanNum));

	if (tdPtr == NULL){ 
        ERRORMSG(TRUE,(L"cppiProcessCompletedTeardown: %s Ch %u, NULL TD\r\n",chanDir,chanNum));
	} else {
        ERRORMSG(chanPtr->isTeardownPending == FALSE,
            (L"cppiProcessCompletedTeardown: %s Ch %u - Not requested for this channel\r\n",
            chanDir, chanNum));

        cppiQueuePush(pPdd->pCppiRegs, USB_CPPI_TDFREE_QNUM, tdPtr);
        chanPtr->isTeardownPending = FALSE;
    }

    DEBUGMSG(ZONE_PDD_DMA, (L"-cppiProcessCompletedTeardown: %s Ch %u\r\n",chanDir, chanNum));
}

//========================================================================
static int cppiControllerStart ( struct dma_controller *dmac )
// Enables the main CPPI DMA Controller Abstraction Structure
// by enabling the individual Channel Members. Initializes the
// CPPI related Registers in the USB Controller.
{
    struct cppi *pController = (struct cppi *)dmac;
    CSL_CppiRegs *pCppiRegs	 = pController->pRegs;
    int n					 = 0;

    PRINTMSG(ZONE_PDD_INIT, (L"+cppiControllerStart: Resetting Channel Info\r\n"));

    /* Initialise the CPPI tx channels */
    for (n = 0; n < _countof(pController->txCppi); n++) {
        struct cppi_channel *txChannel = pController->txCppi + n;

        /* initialize channel fields */
        txChannel->Channel.pPrivateData = txChannel;
        txChannel->Channel.bStatus = MGC_DMA_STATUS_UNKNOWN;
        txChannel->pController = pController;
        txChannel->transmit  = TRUE;
        txChannel->channelNo = (UINT8)n;
        txChannel->queueNo   = pController->txqOffset + (UINT8)(2 * n);
        txChannel->lastModeRndis = 0;
        txChannel->isTeardownPending = FALSE;
        txChannel->pRegs = &(pCppiRegs->CDMACHANNEL[n + pController->chanOffset]);

        PRINTMSG(/*ZONE_PDD_INIT*/0, (L"TX Ch %02u: Queue %u, CDMA regs 0x%08x\r\n",
            txChannel->channelNo, txChannel->queueNo, txChannel->pRegs));
    }

    /* Initialise the CPPI rx channels */
    for (n = 0; n < _countof(pController->rxCppi); n++) {
        struct cppi_channel *rxChannel = pController->rxCppi + n;

        /* initialize channel fields */
        rxChannel->Channel.pPrivateData = rxChannel;
        rxChannel->Channel.bStatus = MGC_DMA_STATUS_UNKNOWN;
        rxChannel->pController = pController;
        rxChannel->transmit  = FALSE;
        rxChannel->channelNo = (UINT8)n;
        rxChannel->queueNo   = pController->rxqOffset + (UINT8)n;
        rxChannel->lastModeRndis = 0;
        rxChannel->isTeardownPending = FALSE;
        rxChannel->pRegs = &(pCppiRegs->CDMACHANNEL[n + pController->chanOffset]);

        PRINTMSG(/*ZONE_PDD_INIT*/0, (L"RX Ch %02u: Queue %u, CDMA regs 0x%08x\r\n",
            rxChannel->channelNo, rxChannel->queueNo, rxChannel->pRegs));
    }

    PRINTMSG(ZONE_PDD_INIT, (L"-cppiControllerStart\r\n"));

    return 0;
}

//========================================================================
//!  \fn cppiControllerStop ()
//!  \brief Stops the working of the CPPI DMA Controller. Performs this
//!         function by writing into the TxCPPi and RxCPPI registers of
//!         of the USB Controller.
//!  \param struct dma_controller *dmac - CPPI DMA Abstraction Pointer
//!  \return int.
//========================================================================
static int cppiControllerStop ( struct dma_controller *dmac )
// Stops the working of the CPPI DMA Controller. Performs this
// function by writing into the TxCPPi and RxCPPI registers of
// of the USB Controller.
{
    struct cppi *cppi = (struct cppi *)dmac;
    unsigned n;

    PRINTMSG(ZONE_INIT,(L"cppiControllerStop: Tearing down RX and TX Channels\r\n"));

    /* Traverse through each Tx/RX channel and abort any transfers */
    for (n = 0; n < _countof(cppi->txCppi); n++)
        cppiChannelAbort((struct dma_channel *)(cppi->txCppi + n));

    for (n = 0; n < _countof(cppi->rxCppi); n++)
        cppiChannelAbort((struct dma_channel *)(cppi->rxCppi + n));

    return 0;
}

//========================================================================
//!  \brief Allocate a CPPI Channel for DMA. With CPPI, channels are bound to
//!         each transfer direction of a non-control endpoint
//!  \param struct dma_controller *dmac - CPPI DMA Abstraction Pointer
//!          UsbFnEp *ep - EndPoint Descriptor Structure
//!          UINT8 bTransmit - Flag to denote Tx EndPoint
//!          MGC_pfDmaCompletion pFnDma - DMA Completion Handler specified
//!             by the Calling Routine.
//!  \return struct dma_channel * - Pointer to a valid DMA Channel Struct
//========================================================================
struct dma_channel * cppiChannelAllocate ( struct dma_controller *dmac,
    UsbFnEp *ep, UINT8 bTransmit, MGC_pfDmaCompletion pFnDma )
{
    struct cppi *pController;
    struct cppi_channel *otgCh;
    CSL_UsbRegs *pUsbRegs = (CSL_UsbRegs *)NULL;
    UINT8 chanNum;

    pController = (struct cppi *)dmac;
    pUsbRegs = pController->pPdd->pUsbdRegs;

    PRINTMSG(ZONE_PDD_INIT, (L"+cppiChannelAllocate: EP %u Tx%x pfmDma 0x%08x\r\n",
        ep->epNumber, bTransmit, pFnDma));

    /* Note: The CPPI Channel numbering scheme starts from 0.
     * whereas the Non-Control EndPoints starts with 1.
     * Hence in order to derive Channel Number associated
     * for a given EP, we will subtract 1
     */
    chanNum = (UINT8)(ep->epNumber - 1);

    /* return the corresponding CPPI Channel Handle, and
     * probably disable the non-CPPI irq until we need it.
     */
    if (bTransmit == TRUE) {
        if (ep->epNumber > _countof(pController->txCppi)) {
            ERRORMSG (TRUE, (L"cppiChannelAllocate: no Tx DMA channel for EP %u\r\n", ep->epNumber));
            return NULL;
        }

        otgCh = pController->txCppi + chanNum;
        otgCh->pRegs->TXGCR = BIT31 | USB_CPPI_TX0CMPL_QNUM + chanNum;
    } else {
        if (ep->epNumber > _countof(pController->rxCppi)) {
            ERRORMSG (TRUE, (L"cppiChannelAllocate: no RX DMA channel for EP %u\r\n", ep->epNumber));
            return NULL;
        }

        otgCh = pController->rxCppi + chanNum;

        otgCh->pRegs->HPCRA = ((chanNum + pController->chanOffset) | ((chanNum + pController->chanOffset) << 16));
        otgCh->pRegs->HPCRB = ((chanNum + pController->chanOffset) | ((chanNum + pController->chanOffset) << 16));
        otgCh->pRegs->RXGCR =
            BIT31 | /* Enable */
            BIT24 | /* Retry on starvation */
            BIT14 | /* Host descriptor type (default) */
            USB_CPPI_RX0CMPL_QNUM + chanNum;
    }

    if (otgCh->pEndPt != NULL) {
        PRINTMSG (ZONE_PDD_INIT, (L"cppiChannelAllocate: re-allocating DMA %u channel from EP %u\r\n",
                   otgCh->channelNo, otgCh->pEndPt->epNumber));
    }

    /* Update the Channel structure with the information given */
    otgCh->pEndPt = ep;
    otgCh->Channel.bStatus = MGC_DMA_STATUS_FREE;
    otgCh->Channel.pPrivateData = otgCh;

    otgCh->pfnDmaCompleted = pFnDma;
    otgCh->transmit        = bTransmit;
    otgCh->nISOHDQueued      = 0;
    otgCh->nISOHDPerTransfer = 0;
    otgCh->nISOHDForCallback = 0;
    otgCh->nISOHDLastIndex   = 0;

    DEBUGMSG(ZONE_PDD_INIT, (L"-cppiChannelAllocate: EP %u DMA Channel 0x%08x\r\n", ep->epNumber, &otgCh->Channel));
    return &(otgCh->Channel);
}

//========================================================================
//!  \fn cppiChannelRelease ()
//!  \brief Releases an allocated CPPI Channel.
//!  \param struct dma_channel *channel
//!  \return None.
//========================================================================
static void cppiChannelRelease ( struct dma_channel *channel )
{
    struct cppi_channel *c = (struct cppi_channel *)channel;

    DEBUGCHK(channel != NULL);
    PRINTMSG(ZONE_PDD_INIT, (L"cppiChannelRelease: Ch %u\r\n", c->channelNo));

    /* Sanity check on the Arguments. Verify if the State is not UNKNOWN
     * before trying to release the Channel. */
    if (channel == NULL){
        ERRORMSG (TRUE, (L"cppiChannelRelease: Invalid Arguments..\r\n"));
    } else if (channel->bStatus != MGC_DMA_STATUS_UNKNOWN) {
        channel->bStatus = MGC_DMA_STATUS_UNKNOWN;

        if (c->pEndPt == NULL)
            PRINTMSG(ZONE_PDD_INIT, (L"cppiChannelRelease: Releasing idle DMA channel\r\n"));
        else
            c->pEndPt = NULL;

        ERRORMSG(c->isTeardownPending, (L"cppiChannelRelease: ERROR - Teardown pending\r\n"));

        /* Disable the CDMA channel */
        if (c->transmit) {
            c->pRegs->TXGCR = USB_CPPI_TX0CMPL_QNUM + c->channelNo;
        } else {
            c->pRegs->RXGCR =
            BIT24 |    /* Retry on starvation */
            BIT14 |    /* Host descriptor type (default) */
			USB_CPPI_RX0CMPL_QNUM + c->channelNo;
        }
    }
}

//========================================================================
//!  \fn cppiChannelProgram ()
//!  \brief Programs the CPPI Channel for Data transfer. Note that
//!         this routine tries to program the CPPI DMA Channel based
//!         on the available Buffer Descriptors. If the total Buffer
//!         Descriptors cannot service the entire chunk of transfer in
//!         one iteration, this routine tries to break up the transfer
//!         into Multiple Chunks. It internally invokes cppiNextSegment().
//!  \param struct dma_channel *pChannel - CPPI channel Pointer
//!         UINT16  packetSize - Maximum Size of EP FIFO on which the
//!             DMA Operation is requested.
//!         UINT32  dmaAddr - DMA Address of the Buffer
//!         UINT32  dwLength - length of Total Transfer
//!  \return int.
//========================================================================
static int cppiChannelProgram ( struct dma_channel *pChannel, UINT16  packetSize,
    UINT32  dmaAddr, UINT32  dwLength )
{
    struct cppi_channel *otgChannel = pChannel->pPrivateData;
    struct cppi *pController = otgChannel->pController;
    USBFNPDDCONTEXT *pPdd = pController->pPdd;

    BYTE chanNum = otgChannel->channelNo;

    PRINTMSG(/*ZONE_PDD_DMA*/0, (L"cppiChannelProgram: Ch %u, %s, len %d, pktsize %d, addr 0x%08x\r\n",
              chanNum, otgChannel->transmit ? L"TX" : L"RX", dwLength, packetSize, dmaAddr));

    LOCK_ENDPOINT(pPdd);

    switch (pChannel->bStatus)    {
		case MGC_DMA_STATUS_BUS_ABORT:
		case MGC_DMA_STATUS_CORE_ABORT:
        /* fault irq handler should have handled cleanup */
			PRINTMSG (ZONE_WARNING, (L"%sX DMA %u not cleaned up after abort!\r\n",
                   otgChannel->transmit ? L"T" : L"R", chanNum));
        break;
		case MGC_DMA_STATUS_BUSY:
			PRINTMSG (ZONE_WARNING, (L"program active channel? %sX DMA %u\r\n",
                   otgChannel->transmit ? L"T" : L"R", chanNum)) ;
        break;
		case MGC_DMA_STATUS_UNKNOWN:
			PRINTMSG (ZONE_ERROR, (L"%sX DMA %u not allocated!\r\n",
                   otgChannel->transmit ? L"T" : L"R", chanNum)) ;

		case MGC_DMA_STATUS_FREE:
			break;
    }
    // A channel that has been torn down must be re-enabled before re-use
    if (otgChannel->transmit) {
        if ((otgChannel->pRegs->TXGCR & BIT31) == 0) {
            otgChannel->pRegs->TXGCR = BIT31 | /* Enable */
			USB_CPPI_TX0CMPL_QNUM + chanNum;
		}
    } else {
        if ((otgChannel->pRegs->RXGCR & BIT31) == 0) {
            otgChannel->pRegs->RXGCR =
                BIT31 | /* Enable */
                BIT24 | /* Retry on starvation */
                BIT14 | /* Host descriptor type (default) */
			    USB_CPPI_RX0CMPL_QNUM + chanNum;

        }
    }

    pChannel->bStatus = MGC_DMA_STATUS_BUSY;

    /* set transfer parameters, then queue up its first segment */
    otgChannel->startAddr = dmaAddr;
    otgChannel->currOffset = 0;
    otgChannel->pktSize = packetSize;
    otgChannel->actualLen = 0;
    otgChannel->transferSize = dwLength;

    cppiNextSegment(pPdd, otgChannel);

    UNLOCK_ENDPOINT(pPdd);
    return TRUE;
}

//========================================================================
static void cppiDumpRx ( struct cppi_channel *c )
{
	UNREFERENCED_PARAMETER(c);
}

//========================================================================
static void cppiDumpTx ( struct cppi_channel *c )
{
	UNREFERENCED_PARAMETER(c);
}

//========================================================================
//!  \fn cppiRndisUpdate ()
//!  \brief Updates the RNDIS Mode for the given Channel. Note that
//!         the CPPI DMA Controller can operate in both Transparent
//!         and RNDIS Mode. This routine is invoked from the
//!         cppiNextSegment() routine to configure
//!         RNDIS Mode operation on need basis. The CPPI Channel Structure
//!         maintains a member to denote the current state with respect
//!         to RNDIS Mode.This routine first checks this member to
//!         ascertain whether a Mode Change is really required or not.
//!
//!  \param struct cppi_channel *c - CPPI Channel Structure
//!         CSL_UsbRegs  *pUsbdRegs - USB Controller Reg Base
//!         int isRndis - Flag to denote the RNDIS Mode
//!  \return None.
//========================================================================
static void cppiRndisUpdate ( struct cppi_channel *channel,
    CSL_UsbRegs  *pRegBase, int isRndis )
{
    volatile UINT32* pRndisReg	 = NULL;
	UINT32  rndisRegVal			 = 0;
    UINT32 mode					 = 0;

    /* we may need to change the rndis flag for this cppi channel */
    if (channel->lastModeRndis != isRndis) {
        /* Mode
        00: Transparent
        01: RNDIS
        10: CDC
        11: Generic RNDIS
        */
		pRndisReg = (channel->transmit) ? &pRegBase->TXMODE : &pRegBase->RXMODE;

		rndisRegVal = *pRndisReg;
        mode = 0x3 << (channel->channelNo * 2);

        if (isRndis > 0) rndisRegVal |= mode;
        else             rndisRegVal &= ~mode;

        if (*pRndisReg != rndisRegVal){
            PRINTMSG(ZONE_PDD_DMA,(L"cppiRndisUpdate: writing mode register %x\r\n", rndisRegVal));
            *pRndisReg = rndisRegVal;
        }
        channel->lastModeRndis = (UINT8)isRndis;
    }

    // The generic RNDIS size register is only used for receive channels
    if (isRndis && !channel->transmit){
        // Round transfer size up to multiple of EP size
        UINT32 rndisSize = ((channel->transferSize + channel->pktSize - 1) / channel->pktSize) * channel->pktSize;
        if (rndisSize == 0)
            rndisSize = channel->pktSize;

        /* Set the Generic RNDIS EP reg */
        switch (channel->channelNo){
            case 0  : pRegBase->GENRNDISSZ1  = rndisSize; break;
            case 1  : pRegBase->GENRNDISSZ2  = rndisSize; break;
            case 2  : pRegBase->GENRNDISSZ3  = rndisSize; break;
            case 3  : pRegBase->GENRNDISSZ4  = rndisSize; break;
            case 4  : pRegBase->GENRNDISSZ5  = rndisSize; break;
            case 5  : pRegBase->GENRNDISSZ6  = rndisSize; break;
            case 6  : pRegBase->GENRNDISSZ7  = rndisSize; break;
            case 7  : pRegBase->GENRNDISSZ8  = rndisSize; break;
            case 8  : pRegBase->GENRNDISSZ9  = rndisSize; break;
            case 9  : pRegBase->GENRNDISSZ10 = rndisSize; break;
            case 10 : pRegBase->GENRNDISSZ11 = rndisSize; break;
            case 11 : pRegBase->GENRNDISSZ12 = rndisSize; break;
            case 12 : pRegBase->GENRNDISSZ13 = rndisSize; break;
            case 13 : pRegBase->GENRNDISSZ14 = rndisSize; break;
            case 14 : pRegBase->GENRNDISSZ15 = rndisSize; break;
            default:
				ERRORMSG(TRUE,
                     (L"cppiRndisUpdate: Unexpected channel number %d!\r\n", channel->channelNo));
				break;
        }
    }

    PRINTMSG (FALSE, (L"cppiRndisUpdate: Ch %u MODE RegVal 0x%08x\n",
               channel->channelNo, *pRndisReg));
    return;
}

//========================================================================
//!  \fn cppiNextSegment ()
//!  \brief Checks for the Next Segment to be setup and initiated
//!         for Transfer. This routine is invoked from the
//!         cppiChannelProgram() routine for DMA Operations.
//!         Note that this routine assumes that the Channel structure
//!         is already updated with the Transfer Parameters.
//!
//!  \param USBFNPDDCONTEXT *pPdd - USB PDD Context Structure
//!         struct cppi_channel *chanPtr - Pointer to the CPPI Channel.
//!  \return None.
//========================================================================
static void cppiNextSegment ( USBFNPDDCONTEXT *pPdd, struct cppi_channel *chanPtr )
{
    unsigned maxpacket =  chanPtr->pktSize;
    UINT32 remaining = chanPtr->transferSize - chanPtr->currOffset;
    struct cppi *cppi = chanPtr->pController;
    HOST_DESCRIPTOR *hd = NULL;
    BOOL   isRndis = FALSE;
    UINT32 buffSz = 0;
    UINT32 buffPtr = 0;
    UINT32 nHD = 0;
    UINT32 i;
    UINT cppiCmplQNum;

    // For overlapped IO need to protect channel vars
    LOCK_ENDPOINT(pPdd);

    isRndis = (chanPtr->pEndPt->endpointType != USB_ENDPOINT_TYPE_ISOCHRONOUS) ?
			(((maxpacket & 0x3f) == 0) && (remaining <= 0x00010000)) :
			FALSE; 	                                                     // Transparent mode for ISO

    // The registry setting 'DisableRxGenRNDIS' should force transparent mode for RX transfers
    if ((!chanPtr->transmit) && (pPdd->disableRxGenRNDIS))
        isRndis = FALSE;

	buffSz = (isRndis) ? remaining : min(remaining, maxpacket);
    buffPtr = chanPtr->startAddr + chanPtr->currOffset;
    cppiRndisUpdate(chanPtr, pPdd->pUsbdRegs, isRndis);

    PRINTMSG(ZONE_PDD_DMA && (chanPtr->transmit ? ZONE_PDD_TX : ZONE_PDD_RX),
             (L"cppiNextSegment: %s Ch %u, segLen %u, addr 0x%08x, lenDone %u, totalLen %u\r\n",
              chanPtr->transmit ? L"TX" : L"RX", chanPtr->channelNo, buffSz, buffPtr, chanPtr->actualLen, chanPtr->transferSize));

	// Determine completion queue based on direction
    if (chanPtr->transmit)
        cppiCmplQNum = USB_CPPI_TX0CMPL_QNUM + chanPtr->channelNo;
    else
        cppiCmplQNum = USB_CPPI_RX0CMPL_QNUM + chanPtr->channelNo;

    if (chanPtr->pEndPt->endpointType != USB_ENDPOINT_TYPE_ISOCHRONOUS){
        hd = cppiHdAlloc(cppi);
        DEBUGCHK(hd != NULL);
        if (hd == NULL)
            return;

        // Initialise the descriptor
        hd->DescInfo    = (USB_CPPI41_DESC_TYPE_HOST   << USB_CPPI41_DESC_TYPE_SHIFT) |
                          (((sizeof(*hd) - 40) / 4)    << USB_CPPI41_DESC_WORDS_SHIFT) |
                          buffSz;
        hd->TagInfo     = 0;
        hd->PacketInfo  = (USB_CPPI41_PKT_TYPE_USB     << USB_CPPI41_PKT_TYPE_SHIFT) |
                          (USB_CPPI41_PKT_RETPLCY_FULL << USB_CPPI41_PKT_RETPLCY_SHIFT) |
                          (USB_CPPI41_DESC_LOC_OFFCHIP << USB_CPPI41_DESC_LOC_SHIFT) |
                          (USB_CPPI_XXCMPL_QMGR        << USB_CPPI41_PKT_RETQMGR_SHIFT) |
                          (cppiCmplQNum                << USB_CPPI41_PKT_RETQ_SHIFT);
        hd->BuffLen     = buffSz;
        hd->BuffPtr     = buffPtr;
        hd->NextPtr     = 0;
        hd->OrigBuffLen = hd->BuffLen | 0x80000000;
        hd->OrigBuffPtr = hd->BuffPtr;
        hd->SWData[0]   = 0;
        hd->SWData[1]   = 0;
        hd->TagInfo2    = (/* isTx */ (chanPtr->transmit ? 1 : 0) << 9) |
                          (/* Ch */ chanPtr->channelNo << 4) |
                          (/* EP */ chanPtr->channelNo + 1);
        hd->Index       = 0;

         // Special handling for ZLP TX - set length to 1 and set ZLP bit
        if (chanPtr->transmit && buffSz == 0)
        {
            hd->PacketInfo |= USB_CPPI41_PKT_FLAGS_ZLP;
            hd->DescInfo |= 1;
            hd->BuffLen = 1;
            hd->OrigBuffLen = 1 | 0x80000000;
        }

        // Write the pointer to the packet descriptor to the appropriate Queue
        //   in the Queue Manager to initiate the transfer
        cppiQueuePush(cppi->pRegs, chanPtr->queueNo, hd);

        chanPtr->currOffset += buffSz;
    } else {
        nHD = (remaining + chanPtr->pktSize - 1) / chanPtr->pktSize;
        chanPtr->nISOHDPerTransfer = nHD;
        chanPtr->nISOHDForCallback = nHD;

        if (chanPtr->pEndPt->pOverlappedInfo){
            nHD = (chanPtr->pEndPt->pOverlappedInfo->dwBytesToIssueCallback + chanPtr->pktSize - 1) /
                   chanPtr->pktSize;

            chanPtr->nISOHDForCallback = nHD;
        }

        chanPtr->nISOHDQueued += chanPtr->nISOHDPerTransfer;

        PRINTMSG(ZONE_PDD_ISO,
                 (L"cppiNextSegment: %s Ch %u, ISO, transferSize %d, pktSize %d, %d HDs, "
                  L"callback on %d, queued %d\r\n",
                  chanPtr->transmit ? L"TX" : L"RX",
				  chanPtr->channelNo, chanPtr->transferSize, chanPtr->pktSize,
                  chanPtr->nISOHDPerTransfer, chanPtr->nISOHDForCallback,
                  chanPtr->nISOHDQueued));

        for (i = 1; i <= chanPtr->nISOHDPerTransfer; ++i){
            hd = cppiHdAlloc(cppi);
            DEBUGCHK(hd != NULL);
            if (hd == NULL)
                return;

            // Initialise the descriptor
            hd->DescInfo    = (USB_CPPI41_DESC_TYPE_HOST   << USB_CPPI41_DESC_TYPE_SHIFT) |
                              (((sizeof(*hd) - 40) / 4)    << USB_CPPI41_DESC_WORDS_SHIFT) |
                              buffSz;
            hd->TagInfo     = 0;
            hd->PacketInfo  = (USB_CPPI41_PKT_TYPE_USB     << USB_CPPI41_PKT_TYPE_SHIFT) |
                              (USB_CPPI41_PKT_RETPLCY_FULL << USB_CPPI41_PKT_RETPLCY_SHIFT) |
                              (USB_CPPI41_DESC_LOC_OFFCHIP << USB_CPPI41_DESC_LOC_SHIFT) |
                              (USB_CPPI_XXCMPL_QMGR        << USB_CPPI41_PKT_RETQMGR_SHIFT) |
                              (cppiCmplQNum                << USB_CPPI41_PKT_RETQ_SHIFT);
            hd->BuffLen     = buffSz;
            hd->BuffPtr     = buffPtr;
            hd->NextPtr     = 0;
            hd->OrigBuffLen = hd->BuffLen | 0x80000000;
            hd->OrigBuffPtr = hd->BuffPtr;
            hd->SWData[0]   = 0;
            hd->SWData[1]   = 0;
            hd->TagInfo2    = (/* isTx */ (chanPtr->transmit ? 1 : 0) << 9) |
                              (/* Ch */ chanPtr->channelNo << 4) |
                              (/* EP */ chanPtr->channelNo + 1);
            hd->Index       = i; // 1 based index

            // Write the pointer to the packet descriptor to the appropriate Queue
            //   in the Queue Manager to initiate the transfer
            cppiQueuePush(cppi->pRegs, chanPtr->queueNo, hd);

            chanPtr->currOffset += buffSz;
            buffPtr = chanPtr->startAddr + chanPtr->currOffset;
            remaining -= buffSz;
            buffSz  = min(remaining, maxpacket);
        }
    }

    /* If this is a receive, we need to enable Rx for this channel 
       in the DMA scheduler */
//    if(!chanPtr->transmit)
//        USBCDMA_ConfigureScheduleRx(chanPtr->channelNo, TRUE);

    UNLOCK_ENDPOINT(pPdd);
}

static void cppiChannelTeardown ( struct cppi_channel *channel )
{
    struct cppi *cppi = channel->pController;
    CSL_UsbRegs *pUsbRegs = cppi->pPdd->pUsbdRegs;

    BYTE   chanNum = channel->channelNo;
    BYTE   epNum   = chanNum + 1;
    DWORD  tc0     = 0;
    UINT16 csr     = 0;

    DEBUGMSG(ZONE_INIT || ZONE_PDD_DMA, (L"+cppiChannelTeardown: %s Ch %u (EP %u)\r\n",
        channel->transmit ? L"TX" : L"RX", chanNum, epNum));

    // We need to cancel interrupt pacing to improve our chances of oustanding transfers being
    // detected on their completion queue
    channel->isTeardownPending = TRUE;

    if (channel->transmit){
        /* 1. Initiate channel tear-down */
        channel->pRegs->TXGCR =
            BIT31 | /* Enable */
            BIT30 | /* Tear-down */
            USB_CPPI_TX0CMPL_QNUM + chanNum;

        /* 2. Set USB controller TD bit */
        pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_TXTD_SHIFT);

        /* 3. Wait for CDMA completion and retry (due to TX disconnect issue) */
        if (channel->isTeardownPending)
            Sleep(10);

        tc0 = GetTickCount();
        while ((channel->isTeardownPending) && ((GetTickCount() - tc0) < 2000)) {
            pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_TXTD_SHIFT);
            USBCDMA_KickCompletionCallback(cppi->hUsbCdma);
            Sleep(10);
        }

        /* 4. Set USB controller TD bit */
        pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_TXTD_SHIFT);

        /* 5. Flush the EP's FIFO */
        csr = pUsbRegs->EPCSR[chanNum + 1].TXCSR;
        csr &= ~MGC_M_TXCSR_DMAENAB;
        csr |=  MGC_M_TXCSR_FLUSHFIFO;
        pUsbRegs->EPCSR[chanNum + 1].TXCSR = csr;
    } else {
        /* 0. Enable Scheduler during teardown */
//        USBCDMA_ConfigureScheduleRx(chanNum, TRUE);

        /* 1. Flush the EP's FIFO */
        csr = pUsbRegs->EPCSR[chanNum + 1].RXCSR;
        csr |= (MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_P_WZC_BITS);
        pUsbRegs->EPCSR[chanNum + 1].PERI_RXCSR = csr;

        /* 2. Set USB controller TD bit */
        pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_RXTD_SHIFT);

        /* 3. Initiate channel tear-down */
        channel->pRegs->RXGCR =
            BIT31 | /* Enable */
            BIT30 | /* Tear-down */
            BIT24 | /* Retry on starvation */
            BIT14 | /* Host descriptor type (default) */
            USB_CPPI_RX0CMPL_QNUM + chanNum;

        /* 4. Wait for CDMA completion and retry (due to TX disconnect issue) */
        if (channel->isTeardownPending)
            Sleep(10);

        tc0 = GetTickCount();
        while ((channel->isTeardownPending) && ((GetTickCount() - tc0) < 2000)) {
            pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_RXTD_SHIFT);
            USBCDMA_KickCompletionCallback(cppi->hUsbCdma);
            Sleep(10);
        }

        /* 5. Set USB controller TD bit */
        pUsbRegs->TEARDOWN |= ((1 << epNum) << USB_OTG_RXTD_SHIFT);

        /* 6. Flush the EP's FIFO */
        csr = pUsbRegs->EPCSR[chanNum + 1].RXCSR;
        csr &= ~MGC_M_RXCSR_DMAENAB;
        csr |= (MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_P_WZC_BITS);
        pUsbRegs->EPCSR[chanNum + 1].RXCSR = csr;

        /* quiesce: wait for current dma to finish (if not cleanup)
         * we can't use bit zero of stateram->sopDescPtr since that
         * refers to an entire "DMA packet" not just emptying the
         * current fifo; most segments need multiple usb packets.
         */
        if (channel->Channel.bStatus == MGC_DMA_STATUS_BUSY)
            Sleep(50);

        /* 7. Disable Scheduler during teardown */
//        USBCDMA_ConfigureScheduleRx(chanNum, FALSE);
    }

    if (channel->isTeardownPending) {
        RETAILMSG(ZONE_WARNING,
            (L"cppiChannelTeardown: %s Ch %u (EP %u) - Timed-out (lost teardown descriptor)!\r\n",
            channel->transmit ? L"TX" : L"RX", chanNum, epNum));
    }

    /* Pop all aborted BDs from the channel's queue and free them */
    for(;;) {
        HOST_DESCRIPTOR *hd = cppiQueuePop(cppi->pRegs, channel->queueNo);
        if (hd == NULL) break;
        cppiHdFree(cppi, hd);
        DEBUGMSG(/*ZONE_WARNING*/0, (L"cppiChannelTeardown: Recovered %s BD on queue %u after tear-down\r\n",
                channel->transmit ? L"TX" : L"RX", channel->queueNo));
    }

    DEBUGMSG(ZONE_INIT || ZONE_PDD_DMA, (L"-cppiChannelTeardown: %s Ch %u (EP %u) - %s\r\n",
        channel->transmit ? L"TX" : L"RX", chanNum, epNum,
        channel->isTeardownPending ? L"FAILED!" : L"SUCCEEDED"));
}

//========================================================================
//!  \fn cppiChannelAbort ()
//!  \brief This function is used to abort a currently configured
//!         CPPI DMA Channel. This routine is used during the Driver
//!         shutdown sequence. Depending on whether the specified
//!         Channel is setup in Tx or Rx Mode, this routine takes up
//!         Appropriate Actions. For a Tx CPPI Channel, this routine
//!         Masks the Tx DMA Interrupt, clears the EndPoint Registers.
//!         For a Rx CPPI Channel, this routine first invokes the
//!         cppiRxScan() to check if there are any active/completed
//!         DMA Transfers. It later, clears the DMA Support for this
//!         Channel.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
static int cppiChannelAbort ( struct dma_channel *channel )
{
    int rc = 0;
    struct cppi_channel *otgCh = (struct cppi_channel *)channel;

    DEBUGMSG(ZONE_INIT || ZONE_PDD_DMA, (L"+cppiChannelAbort: %s Ch %u (EP %u)\r\n",
        otgCh->transmit ? L"TX" : L"RX", otgCh->channelNo, otgCh->channelNo + 1));

    switch (channel->bStatus)
    {
    case MGC_DMA_STATUS_BUS_ABORT:
    case MGC_DMA_STATUS_CORE_ABORT:
        // from RX or TX fault irq handler
    case MGC_DMA_STATUS_BUSY:
        // The hardware needs shutting down
        cppiChannelTeardown(otgCh);
        break;

    case MGC_DMA_STATUS_UNKNOWN:
        DEBUGMSG(ZONE_INIT || ZONE_PDD_DMA,
            (L"cppiChannelAbort: %s Ch %u (EP %u) not allocated\r\n",
            otgCh->transmit ? L"TX" : L"RX", otgCh->channelNo, otgCh->channelNo + 1));

    case MGC_DMA_STATUS_FREE:
        break;

    default:
        ERRORMSG(TRUE, (L"cppiChannelAbort: Invalid DMAC state\r\n"));
        rc = -1;
        break;
    }

    channel->bStatus = MGC_DMA_STATUS_FREE;
    otgCh->startAddr = 0;
    otgCh->currOffset = 0;
    otgCh->transferSize = 0;
    otgCh->pktSize = 0;
    otgCh->nISOHDQueued      = 0;
    otgCh->nISOHDPerTransfer = 0;
    otgCh->nISOHDForCallback = 0;
    otgCh->nISOHDLastIndex   = 0;

    DEBUGMSG(ZONE_INIT || ZONE_PDD_DMA, (L"-cppiChannelAbort: %s Ch %u (EP %u)\r\n",
        otgCh->transmit ? L"TX" : L"RX", otgCh->channelNo, otgCh->channelNo + 1));

    return rc;
}

//========================================================================
struct dma_controller * cppiControllerInit ( USBFNPDDCONTEXT *pPdd )
// This function is used to instantiate a CPPI Controller abstraction in Software.
{
    struct cppi *cppi;

    if (f_CppiObj != NULL)
        return (&f_CppiObj->controller);

    if (NULL == (f_CppiObj = (struct cppi *) LocalAlloc(LPTR, sizeof(struct cppi)))){
        ERRORMSG(TRUE, (TEXT("ERROR: Unable to Alloc CPPI DMA Object!!\r\n")));
        return NULL;
    }

    cppi = f_CppiObj;

    // Initialize the Cppi DmaController structure
    cppi->pPdd = pPdd;
    cppi->pRegs = pPdd->pCppiRegs;
    cppi->hUsbCdma = NULL;

    cppi->chanOffset = 0;
    cppi->rxqOffset = USB_CPPI_RX_QNUM;
    cppi->txqOffset = USB_CPPI_TX_QNUM;

    cppi->controller.pfnStart = cppiControllerStart;
    cppi->controller.pfnStop  = cppiControllerStop;
    cppi->controller.pfnChannelAlloc = cppiChannelAllocate;
    cppi->controller.pfnChannelRelease = cppiChannelRelease;
    cppi->controller.channelProgram = cppiChannelProgram;
    cppi->controller.pfnChannelAbort = cppiChannelAbort;

    cppiPoolInit(cppi);

    pPdd->pUsbdRegs->AUTOREQR = 0;
	pPdd->pUsbdRegs->TXMODE  = 0;
	pPdd->pUsbdRegs->RXMODE  = 0;

    return &cppi->controller;
}

void cppiControllerDeinit( USBFNPDDCONTEXT *pPdd )
{
    struct cppi *cppi = f_CppiObj;
	UNREFERENCED_PARAMETER(pPdd);
    cppiPoolDeinit(cppi);
}

#pragma warning(pop)

#endif /* #ifdef CPPI_DMA_SUPPORT */
