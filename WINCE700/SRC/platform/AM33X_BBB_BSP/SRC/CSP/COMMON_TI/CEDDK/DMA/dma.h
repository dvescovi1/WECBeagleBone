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
#ifndef __DMA_H
#define __DMA_H

//random numbers to indentify handle types
#define DMA_HANDLE_CHANNEL_COOKIE               0x43A608F0
#define DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE   0x43A608F1
#define DMA_DEDICATED_CHANNEL_HANDLE_COOKIE     0x43A608F2

//------------------------------------------------------------------------------
//
//  constitutes a handles used by the ceddk dma library
//
typedef struct {
    DWORD                       cookie;
    OMAP_DMA_LC_REGS           *pDmaChannel;
    DmaChannelContext_t         context;
} CeddkDmaContext_t;

typedef struct {
    DWORD                       cookie;
    OMAP_DMA_LC_REGS           *pDmaChannel;
    int                         index;
    DWORD                       ChannelIntMask;
} CeddkDmaDIChannelContext_t;

typedef struct
{
    DWORD                       cookie;
    DWORD                       DmaControllerPhysAddr;
    DWORD                       IrqNum;
    DWORD                       SysInterruptNum;
    DWORD                       NumberOfChannels;
    DWORD                       ffDmaChannels;
    OMAP_SDMA_REGS              *pDmaRegs; 
    volatile DWORD              *pMaskRegister;
    volatile DWORD              *pStatusRegister;
    HANDLE                      hInterruptEvent;
    CeddkDmaDIChannelContext_t  *ChannelContextArray;
} CeddkDmaDIContext_t;


//------------------------------------------------------------------------------
// Prototypes
//
BOOL LoadDmaDriver();
void* DmaGetLogicalChannel(HANDLE hDmaChannel);



#define ValidateDmaDriver()         (g_hDmaDrv != NULL ? TRUE : LoadDmaDriver())

#endif __DMA_H
