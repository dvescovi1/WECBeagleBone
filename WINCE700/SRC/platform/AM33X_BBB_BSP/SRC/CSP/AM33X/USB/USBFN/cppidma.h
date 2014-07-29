// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file cppidma.h
//! \brief USB CPPI Dma Controller Header File. This header file
//!        contains the Software abstraction [typedef declarations] for
//!        the CPPI Descriptor and Controller Settings.
//!
//! \version  1.00 Oct 16 2006 File Created

#ifndef __CPPIDMA_H_INCLUDED__
#define __CPPIDMA_H_INCLUDED__

/* WINDOWS CE Public Includes -------------------------------------- */
#include <windows.h>
#include <linklist.h>

/* Platform Includes ---------------------------------------------- */
#include "am33x_usb.h"
#include "am33x_usbcdma.h"

/**
 * DMA channel status ... updated by the dma controller driver whenever that
 * status changes, and protected by the overall controller spinlock.
 */
enum dma_channel_status {
    /** A channel's status is unknown */
    MGC_DMA_STATUS_UNKNOWN,
    /** A channel is available (not busy and no errors) */
    MGC_DMA_STATUS_FREE,
    /** A channel is busy (not finished attempting its transactions) */
    MGC_DMA_STATUS_BUSY,
    /** A channel aborted its transactions due to a local bus error */
    MGC_DMA_STATUS_BUS_ABORT,
    /** A channel aborted its transactions due to a core error or USB fault */
    MGC_DMA_STATUS_CORE_ABORT
};

typedef enum dma_channel_status MGC_DmaChannelStatus;

/***************************** TYPES ******************************/

/**
 * struct dma_channel - A DMA channel.
 * @field pPrivateData channel-private data; not to be interpreted by the ICD
 * @field dwMaxLength the maximum number of bytes the channel can move
 * in one transaction (typically representing many USB maximum-sized packets)
 * @field dwActualLength how many bytes have been transferred
 * @field bStatus current channel status (updated e.g. on interrupt)
 * @field bDesiredMode TRUE if mode 1 is desired; FALSE if mode 0 is desired
 */
struct dma_channel {
    void                *pPrivateData;
    MGC_DmaChannelStatus bStatus;
};
typedef struct dma_channel MGC_DmaChannel;

/* Program a DMA channel to move data at the core's request.
 * The local core endpoint and direction should already be known,
 * since they are specified in the channel_alloc call.
 *
 * @channel: pointer to a channel obtained by channel_alloc
 * @maxpacket: the maximum packet size
 * @bMode: TRUE if mode 1; FALSE if mode 0
 * @dma_addr: base address of data (in DMA space)
 * @length: the number of bytes to transfer; no larger than the channel's
 * reported dwMaxLength
 *
 * Returns TRUE on success, else FALSE
 */
typedef int (*MGC_pfDmaProgramChannel) (struct dma_channel *channel,
    UINT16 maxpacket, UINT32 dmaAddr, UINT32 length);

/* DMA Completion Handler Routine. This routine will be invoked
 * by the CPPI Module upon completion of the DMA Transfer.
 *
 * @pDriverContext: pointer to the PDD Context Struct
 * @chanNumber: The Channel on which transmission is complete
 * @epNum: EndPoint for which the Transfer was requested.
 *
 * Returns TRUE on success, else FALSE
 */
typedef int (*MGC_pfDmaCompletion) (void * pDriverContext,
                            int chanNumber, int epNum);

/**
 * struct dma_controller - A DMA Controller.
 * @pPrivateData: controller-private data;
 * @start: call this to start a DMA controller;
 * return 0 on success, else negative errno
 * @stop: call this to stop a DMA controller
 * return 0 on success, else negative errno
 * @channel_alloc: call this to allocate a DMA channel
 * @channel_release: call this to release a DMA channel
 * @channel_abort: call this to abort a pending DMA transaction,
 * returning it to FREE (but allocated) state
 *
 * Controllers manage dma channels.
 */
struct dma_controller
{
    int (*pfnStart)(struct dma_controller *);
    int (*pfnStop)(struct dma_controller *);
    struct dma_channel *(*pfnChannelAlloc)(struct dma_controller *,
        UsbFnEp *, UINT8 is_tx, MGC_pfDmaCompletion);
    void (*pfnChannelRelease)(struct dma_channel *);
    MGC_pfDmaProgramChannel channelProgram;
    int (*pfnChannelAbort)(struct dma_channel *);
};

/**
 *  Channel Control Structure
 *
 * CPPI  Channel Control structure. Using he same for Tx/Rx. If need be
 * derive out of this later.
 */

//******************************************************************
//! \typedef cppi_channel
//! \brief CPPI Channel Control structure. Using the same for Tx/Rx. 
//!        If need be derive out of this later.
//!
//*******************************************************************
typedef struct cppi_channel {
    struct dma_channel  Channel;           /**< First field must be dma_channel for easy type casting */
    struct cppi        *pController;       /**< CPPI controller */
    CSL_ChannelRegs    *pRegs;             /**< CPPI DMA channel registers */

    UsbFnEp            *pEndPt;            /**< BackPointer to EndPoint Struct */
    BOOL                transmit;          /**< Transmit or Receive */
    UINT8               channelNo;         /**< Channel Number */
    UINT8               queueNo;           /**< CPPI Queue Number */

    UINT8               lastModeRndis;     /**< DMA modes: RNDIS or "transparent" */

    BOOL volatile       isTeardownPending; /**< Flag to indicate channel tear-down is pending */

    UINT32              startAddr;         /**< Physical Start Address of the transfer buffer */
    UINT32              transferSize;      /**< Transfer Size requested on this Channel */
    UINT32              pktSize;           /**< EP Fifo Max Size */
    UINT32              currOffset;        /**< requested segments */
    UINT32              actualLen;         /**< completed (Channel.actual) */

    UINT32              nISOHDPerTransfer; /* Number of ISO HDs in each transfer */
    UINT32              nISOHDForCallback; /* Index of ISO HD at which to make callback (overlapped IO) */
    UINT32              nISOHDQueued;      /* Total number of ISO HDs currently queued */
    UINT32              nISOHDLastIndex;   /* Index of last ISO HD that completed */

    MGC_pfDmaCompletion pfnDmaCompleted;   /**<  DMA Completion Handler */
};

//******************************************************************
//! \typedef cppi
//! \brief CPPI DMA Controller Object. Encapsulates all bookeeping
//!         and Data structures pertaining to the CPPI DMA Controller.
//*******************************************************************
struct cppi {
    struct dma_controller   controller; /**< CPPI DMA Controller */

    USBFNPDDCONTEXT        *pPdd;       /**< Pointer to USB PDD Struct */
    CSL_CppiRegs           *pRegs;      /**< Pointer to USB CPPI Registers Base */
    HANDLE                  hUsbCdma;   /**< HANDLE returned from registering with USBCDMA */
    UINT8                   chanOffset; /**< CPPI channel offset (0 or 15) */
    UINT8                   rxqOffset;  /**< CPPI RX queue offset (0 or 15) */
    UINT8                   txqOffset;  /**< CPPI TX queue offset (32 or 62) */

    struct cppi_channel     txCppi[USB_CPPI_MAX_TX_CHANNELS];
    struct cppi_channel     rxCppi[USB_CPPI_MAX_RX_CHANNELS];

    void                   *pool;           /**< Virtual Base of Descriptor Pool */
    PHYSICAL_ADDRESS        paPool;         /**< Physical Base of Descriptor Pool */
    UINT32                  poolMaxSize;    /**< Size of Descriptor Pool (Host + Teardown) */
    UINT32                  poolUsed;       /**< Maintains Used Count in bytes (Host) */
    UINT32                  poolFree;       /**< Maintains Free Count in bytes */

    HOST_DESCRIPTOR         *poolHead;       /**< Free Host-Descriptor head pointer */
    CRITICAL_SECTION        poolLock;       /**< Critical Section to protect access */
};

#define LOCK_HD_POOL(c)     EnterCriticalSection(&c->poolLock)
#define UNLOCK_HD_POOL(c)   LeaveCriticalSection(&c->poolLock)

#endif  /* ifndef __CPPIDMA_H_INCLUDED__ */
