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
//  This file contains DDK library local definitions for platform specific
//  dma operations .
//
#pragma warning(push)
#pragma warning(disable: 4115)
#include <windows.h>
#include <types.h>
#pragma warning(pop)

#include "omap.h"
#include "omap_sdma_regs.h"
#include "omap_sdma_utility.h"
#include "soc_cfg.h"
#include "ceddkex.h"
#include "dma.h"

//------------------------------------------------------------------------------
//
//  Global:  g_hDmaDrv
//
//  This global variable holding a reference to the dma driver
//
extern HANDLE g_hDmaDrv;

//------------------------------------------------------------------------------
// Prototypes
//
static VOID DmaDeallocateContext(HANDLE hDmaInterrupt);


//------------------------------------------------------------------------------
//
//  Function:  DmaDiAllocateInterrupt
//
//  Allocates a dedicated DMA interrupt for a specified number of DMA channels.  
//  If successful a non-null handle is returned.
//
//  NumberOfChannels - number of DMA channels to allocate.
//  hInterruptEvent - handle to an event to be registered with the DMA hardware interrupt.
//  phChannelHandles - phChannelHandles array of handles to each DMA channel.
//                     handles should be used with functions in dma_utility.
//
HANDLE 
DmaDiAllocateInterrupt(
    DWORD   NumberOfChannels, 
    HANDLE  hInterruptEvent,
    HANDLE  *phChannelHandles
    )
{
    BOOL rc = FALSE;
    static int dwDmaStartIrq = -1;
    CeddkDmaDIContext_t *pContext = NULL;
    DWORD i;
    PHYSICAL_ADDRESS pa;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: ValidateDmaDriver Failure\r\n"
                       ));
        SetLastError(ERROR_DLL_INIT_FAILED);
        goto cleanUp;
        }

    // ensure valid request
    if(NumberOfChannels > OMAP_DMA_LOGICAL_CHANNEL_COUNT)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: NumberOfChannels too large\r\n"
                       ));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
        }

    // allocate array of dma dedicated interrupt context
    pContext = (CeddkDmaDIContext_t*)LocalAlloc(LPTR, sizeof(CeddkDmaDIContext_t));
    if (pContext == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: memory allocation failure\r\n"
                       ));
        //last error already set
        goto cleanUp;
        }
    pContext->cookie = DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE;
    pContext->NumberOfChannels = NumberOfChannels;

    //request the interrupt and channels
    {
    IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT bufferOut;

    if (!DeviceIoControl(
            g_hDmaDrv, IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT, 
            (VOID*)&NumberOfChannels, sizeof(IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_IN), 
            (VOID*)&bufferOut, sizeof(IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT), 
            NULL, NULL))
        {
        // ERROR_BUSY indcates that max channels or max interrupts requested, don't print error
        if(GetLastError() != ERROR_BUSY)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT failure\r\n"
                       ));
            }

        //last error already set
        goto cleanUp;
        }

    pContext->IrqNum = bufferOut.IrqNum;
    pContext->DmaControllerPhysAddr = bufferOut.DmaControllerPhysAddr;
    pContext->ffDmaChannels = bufferOut.ffDmaChannels;
    }

    // allocate array of channel register pointers
    pContext->ChannelContextArray = (CeddkDmaDIChannelContext_t *)LocalAlloc(LPTR, (sizeof(CeddkDmaDIChannelContext_t) * NumberOfChannels));
    if (pContext->ChannelContextArray == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: memory allocation failure\r\n"
                       ));
        //last error already set
        goto cleanUp;
        }
    
    // map memory to registers
    pa.HighPart = 0;
    pa.LowPart = pContext->DmaControllerPhysAddr;
    pContext->pDmaRegs = (OMAP_SDMA_REGS*)MmMapIoSpace(pa, 
                                                sizeof(OMAP_SDMA_REGS), 
                                                FALSE
                                                );
    if (pContext->pDmaRegs == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiAllocateInterrupt: MmMapIoSpace failure\r\n"
                       ));
        //last error already set
        goto cleanUp;
        }

    // Get the Mask and Status registers for the coresponding SDMA IRQ
    if (dwDmaStartIrq == -1)
    {
        dwDmaStartIrq = GetIrqByDevice(SOCGetDMADevice(0),L"IRQ0");
    }

    pContext->pMaskRegister = (volatile DWORD*)(&pContext->pDmaRegs->DMA4_IRQENABLE_L0) + (pContext->IrqNum - dwDmaStartIrq);
    pContext->pStatusRegister = (volatile DWORD*)(&pContext->pDmaRegs->DMA4_IRQSTATUS_L0) + (pContext->IrqNum - dwDmaStartIrq);

    // get system interrupt for irq
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &pContext->IrqNum, 
            sizeof(pContext->IrqNum), &pContext->SysInterruptNum, sizeof(pContext->SysInterruptNum), 
            NULL)) 
        {
        ERRORMSG(TRUE, (L"DmaDiAllocateInterrupt "
            L"Failed map DMA interrupt(irq=%d)\r\n", pContext->IrqNum
            ));
        //last error already set
        goto cleanUp;
        }

    // register event handle with system interrupt
    if (!InterruptInitialize(pContext->SysInterruptNum, hInterruptEvent, NULL, 0)) 
        {
        ERRORMSG(TRUE, (L"DmaDiAllocateInterrupt "
            L"InterruptInitialize failed\r\n"
            ));
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanUp;
        }
    else
        {
        pContext->hInterruptEvent = hInterruptEvent;
        }

    // configure channel info
    {
    DWORD HandleIndex =0;
    CeddkDmaDIChannelContext_t *pChannelContext = pContext->ChannelContextArray;
    DWORD ff = 1;

    for(i =0; i < OMAP_DMA_LOGICAL_CHANNEL_COUNT; i++)
        {
        if(ff & pContext->ffDmaChannels)
            {
            //initalize the cookie
            pChannelContext->cookie = DMA_DEDICATED_CHANNEL_HANDLE_COOKIE;

            //Save index of the channel
            pChannelContext->index = i;

            //Save the bit mask
            pChannelContext->ChannelIntMask = ff;

            // get pointer to the channel registers
            pChannelContext->pDmaChannel = &pContext->pDmaRegs->CHNL_CTRL[i];

            //return a handle to the channel registers such that dma_utility functions can be used.
            phChannelHandles[HandleIndex] = (void*) pChannelContext;

            //Point to next entry
            pChannelContext++;
            HandleIndex++;
            }
        ff <<= 1;
        }
        
    }

    // reset dma interrupts and status
    WRITE_REGISTER_ULONG(pContext->pMaskRegister, pContext->ffDmaChannels);
    WRITE_REGISTER_ULONG(pContext->pStatusRegister, 0xFFFFFFFF);

    // clear interrupt status 
    for(i =0; i < NumberOfChannels; i++)
        {
        WRITE_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CSR, 0x00000FFE);
        WRITE_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CLNK_CTRL, 0x00000000);
        }

    rc = TRUE;
    SetLastError(ERROR_SUCCESS);

cleanUp:
    // request dma channel
    if (rc == FALSE && pContext != NULL)
        {
        DmaDeallocateContext(pContext);
        pContext = NULL;
        }
    
    return pContext;    
}

static 
VOID
DmaDeallocateContext(
    CeddkDmaDIContext_t *pContext
    )
{
    if(pContext)
        {
        //free DMA Driver resources for DMA interrupt
         if(pContext->SysInterruptNum)
            {
            InterruptDisable(pContext->SysInterruptNum);
            }

        if(pContext->IrqNum)
            {
            if (!DeviceIoControl(
                                g_hDmaDrv, IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT, 
                                (VOID*)&pContext->IrqNum, sizeof(IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT_IN), 
                                NULL, 0, 
                                NULL, NULL))
                {
                ERRORMSG(TRUE, (
                                L"DmaDeallocateContext: IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT failure\r\n"
                               ));
                //last error already set
                }

            if (!KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, 
                                &pContext->SysInterruptNum, sizeof(pContext->SysInterruptNum), 
                                NULL, 0, 
                                NULL)) 
                {
                ERRORMSG(TRUE, (
                            L"DmaDeallocateContext "
                            L"Failed IOCTL_HAL_RELEASE_SYSINTR (SysInterruptNum=%d)\r\n", pContext->SysInterruptNum
                            ));
                //last error already set
                }

            }

        //unmap registers
        if(pContext->pDmaRegs)
            {
            MmUnmapIoSpace((PVOID)pContext->pDmaRegs, sizeof(OMAP_SDMA_REGS));
            }

        //free memory
        if(pContext->ChannelContextArray)
            {
            LocalFree((PVOID)pContext->ChannelContextArray);
            }

        if(pContext->hInterruptEvent)
            {
            InterruptDisable(pContext->SysInterruptNum);
            }
              
        LocalFree(pContext);   
        }
}


//------------------------------------------------------------------------------
//
//  Function:  DmaDiDmaFreeInterrupt
//
//  Frees a dedicated DMA interrupt and all associated DMA channels.
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
BOOL 
DmaDiDmaFreeInterrupt(
    HANDLE hDmaInterrupt
    )
{
    BOOL rc = FALSE;
    CeddkDmaDIContext_t *pContext = (CeddkDmaDIContext_t*)hDmaInterrupt;
    DWORD i;

    volatile ULONG ulCCR;
    BOOL breakLoop = FALSE;
    int  DelayCnt = 0x7FFF;

    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaInterrupt == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiDmaFreeInterrupt: ValidateDmaDriver Failure\r\n"
                       ));
        SetLastError(ERROR_DLL_INIT_FAILED);
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiDmaFreeInterrupt: Invalid Handle Type\r\n"
                       ));
            SetLastError(ERROR_INVALID_HANDLE);
            goto cleanUp;
            }
        
        for(i =0; i < pContext->NumberOfChannels; i++)
            {
            DmaDisableStandby((HANDLE)&pContext->ChannelContextArray[i], TRUE);
            // stop dma, break all links, and reset status
            WRITE_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CCR, 0x00000000);
            // ensure DMA transfer is completed by polling the active bits
            //
            while ((breakLoop == FALSE) && (DelayCnt != 0))
            {
            ulCCR = READ_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CCR);
            if ((ulCCR & DMA_CCR_WR_ACTIVE) || (ulCCR & DMA_CCR_RD_ACTIVE))
               {
               DelayCnt--;
               //RETAILMSG (1,(TEXT("-->DmaDiDmaFreeInterrupt - DrainDMA (DelayCnt=%d)\r\n"), DelayCnt));
               }
            else
               {
               breakLoop = TRUE;
               }
            }

            // re-enable standby in dma controller
            //
            DmaDisableStandby((HANDLE)&pContext->ChannelContextArray[i], FALSE);
            WRITE_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CSR, 0x00000FFE);
            WRITE_REGISTER_ULONG(&pContext->ChannelContextArray[i].pDmaChannel->CLNK_CTRL, 0x00000000);
            }

        // reset dma interrupts and status
        WRITE_REGISTER_ULONG(pContext->pMaskRegister, 0x00000000);
        WRITE_REGISTER_ULONG(pContext->pStatusRegister, 0xFFFFFFFF);

        //free all the resources
        DmaDeallocateContext(pContext);

        SetLastError(ERROR_SUCCESS);
        rc = TRUE;
        }
    __except (TRUE)
        {
        ERRORMSG(TRUE, (
                        L"ERROR! CEDDK: "
                        L"exception free'ing dma handle\r\n"
                       ));
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        }

cleanUp:
    return rc; 
}

//------------------------------------------------------------------------------
//
//  Function:  DmaDiFindInterruptChannelByIndex
//
//  Return Channel that is causing an interrupt
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
DWORD 
DmaDiFindInterruptChannelByIndex(
    HANDLE hDmaInterrupt
    )
{
    DWORD RetVal = (DWORD)-1;
    CeddkDmaDIContext_t *pContext = (CeddkDmaDIContext_t*)hDmaInterrupt;
    DWORD i;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaInterrupt == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiFindInterruptChannelByIndex: ValidateDmaDriver Failure\r\n"
                       ));
        SetLastError(ERROR_DLL_INIT_FAILED);
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiFindInterruptChannelByIndex: Invalid Handle Type\r\n"
                       ));
            SetLastError(ERROR_INVALID_HANDLE);
            goto cleanUp;
            }

        for(i =0; i < pContext->NumberOfChannels; i++)
            {
            if(pContext->ChannelContextArray[i].ChannelIntMask & *pContext->pStatusRegister)
                {
                RetVal = i;
                }
            }
        SetLastError(ERROR_SUCCESS);
        }
    __except (TRUE)
        {
        RETAILMSG(TRUE, (L"DmaDiFindInterruptChannelByIndex: "
            L"exception getting dma channel interrupt\r\n"
            ));
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        }

cleanUp:

    return RetVal;
}


//------------------------------------------------------------------------------
//
//  Function:  DmaDiDmaDiClearInterruptChannelByIndex
//
//  Clear the DMA Channel interrupt bit flag 
//
//  hDmaInterrupt  - Handle generated by "DmaDiAllocateInterrupt"
//  ChannelIndex   - Channel Index returned from call to DmaDiFindInterruptChannelByIndex
BOOL
DmaDiClearInterruptChannelByIndex(
    HANDLE hDmaInterrupt,
    DWORD ChannelIndex
    )
{
    BOOL rc = FALSE;
    CeddkDmaDIContext_t *pContext = (CeddkDmaDIContext_t*)hDmaInterrupt;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaInterrupt == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiClearInterruptChannelByIndex: ValidateDmaDriver Failure\r\n"
                       ));
        SetLastError(ERROR_DLL_INIT_FAILED);
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiClearInterruptChannelByIndex: Invalid Handle Type\r\n"
                       ));
            SetLastError(ERROR_INVALID_HANDLE);
            goto cleanUp;
            }

        if(ChannelIndex > pContext->NumberOfChannels)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiClearInterruptChannelByIndex: Invalid Channel Number %d\r\n",
                       ChannelIndex
                       ));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto cleanUp;
            }

        *pContext->pStatusRegister = pContext->ChannelContextArray[ChannelIndex].ChannelIntMask;

        if(*pContext->pStatusRegister & pContext->ChannelContextArray[ChannelIndex].ChannelIntMask)
            {
            //most likely DMA4_CSRi was not cleared
            DEBUGMSG(1, (
                L"DmaDiClearInterruptChannelByIndex: Channel %d was not cleared.\r\n",
               ChannelIndex
               ));
            }

        rc = TRUE;
        SetLastError(ERROR_SUCCESS);
        }
    __except (TRUE)
        {
        ERRORMSG(TRUE, (
            L"DmaDiClearInterruptChannelByIndex: "
            L"exception clearing channel interrupt\r\n"
            ));
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        }

cleanUp:

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DmaDiInterruptDone
//
//  Call interrupt done dedicated DMA interrupt
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
BOOL
DmaDiInterruptDone(
    HANDLE hDmaInterrupt
    )
{
    BOOL rc = FALSE;
    CeddkDmaDIContext_t *pContext = (CeddkDmaDIContext_t*)hDmaInterrupt;
    
    // check if dma handle is valid
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaInterrupt == NULL)
        {
        ERRORMSG(TRUE, (
                        L"DmaDiInterruptDone: ValidateDmaDriver Failure\r\n"
                       ));
        SetLastError(ERROR_DLL_INIT_FAILED);
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_DEDICATED_INTERRUPT_HANDLE_COOKIE)
            {
            ERRORMSG(TRUE, (
                        L"DmaDiInterruptDone: Invalid Handle Type\r\n"
                       ));
            SetLastError(ERROR_INVALID_HANDLE);
            goto cleanUp;
            }

            InterruptDone(pContext->SysInterruptNum); 
            rc = TRUE;
            SetLastError(ERROR_SUCCESS);

        }
    __except (TRUE)
        {
        ERRORMSG(TRUE, (
            L"DmaDiInterruptDone: "
            L"exception clearing channel interrupt\r\n"
            ));
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        }

cleanUp:

    return rc;
}


//------------------------------------------------------------------------------
