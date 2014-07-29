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
//  File:  dma.h
//
//  This file contains DDK local definitions for platform specific
//  dma operations.
//  
#ifndef __EDMA_DDK_H
#define __EDMA_DDK_H

//random numbers to indentify handle types
#define DMA_HANDLE_CHANNEL_COOKIE               0x43A608F0

typedef struct
{
    UINT32 nChId;
    UINT32 nTcc;
    HANDLE hEvent;
    HANDLE hError;
	BOOL   transfer_busy;
    DMA_ADAPTER_OBJECT 	dmaAdapter;
	UINT32				*pColorBuffer;
	PHYSICAL_ADDRESS	ColorBufferPA;
} EDMA_TRANSFER;

//------------------------------------------------------------------------------
//
//  constitutes a handles used by the ceddk dma library
//
typedef struct {
    DWORD                       cookie;
    EDMA_TRANSFER               pDmaTransfer;
    DmaChannelContext_t         context;
} CeddkDmaContext_t;

//------------------------------------------------------------------------------
// Prototypes
//
BOOL LoadDmaDriver();
void* DmaGetLogicalChannel(HANDLE hDmaChannel);



#define ValidateDmaDriver()         (hEdma != NULL ? TRUE : LoadDmaDriver())

#endif __EDMA_DDK_H
