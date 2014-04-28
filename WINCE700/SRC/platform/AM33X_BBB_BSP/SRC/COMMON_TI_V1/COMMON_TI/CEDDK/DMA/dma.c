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
//#include <ceddk.h>
#include "ceddkex.h"
#include "omap_sdma_regs.h"
#include "omap_sdma_utility.h"
#include "dma.h"
// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)
#pragma warning (disable: 6322)


//------------------------------------------------------------------------------
//
//  Global:  g_hDmaDrv
//
//  This global variable holding a reference to the dma driver
//
HANDLE g_hDmaDrv = NULL;


//------------------------------------------------------------------------------
//
//  Global:  LoadDmaDriver
//
//  loads the dma driver and retrieves a handle to it
//
BOOL LoadDmaDriver()
{
    BOOL rc = FALSE;
    g_hDmaDrv = CreateFile(L"DMA1:", 0, 0, NULL, 0, 0, NULL);
    if (g_hDmaDrv == INVALID_HANDLE_VALUE)
        {
        g_hDmaDrv = NULL;
        goto cleanUp;
        }

    rc = TRUE;
cleanUp:
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
    PHYSICAL_ADDRESS pa;
    CeddkDmaContext_t *pContext = NULL;
    
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
    if (!DeviceIoControl(
            g_hDmaDrv, IOCTL_DMA_RESERVE_CHANNEL, (VOID*)&dmaType,
            sizeof(IOCTL_DMA_RESERVE_IN), &pContext->context, 
            sizeof(IOCTL_DMA_RESERVE_OUT), NULL, NULL))
        {
        goto cleanUp;
        }

    // map memory to register
    pa.HighPart = 0;
    pa.LowPart = pContext->context.channelAddress;
    pContext->pDmaChannel = (OMAP_DMA_LC_REGS*)MmMapIoSpace(pa, 
                                                pContext->context.registerSize, 
                                                FALSE
                                                );

    if (pContext->pDmaChannel == NULL)
        {
        DmaFreeChannel(pContext);
        goto cleanUp;
        }

    // clear status mask
    WRITE_REGISTER_ULONG(&pContext->pDmaChannel->CSR, 0x00000FFE);
    WRITE_REGISTER_ULONG(&pContext->pDmaChannel->CLNK_CTRL, 0x00000000);

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
        
        // unmap dma registers
        if (pContext->pDmaChannel != NULL)
            {
            // stop dma, break all links, and reset status
            WRITE_REGISTER_ULONG(&pContext->pDmaChannel->CCR, 0x00000000);
            WRITE_REGISTER_ULONG(&pContext->pDmaChannel->CSR, 0x00000FFE);
            WRITE_REGISTER_ULONG(&pContext->pDmaChannel->CLNK_CTRL, 0x00000000);

            // unregister for all interrupts
            DmaEnableInterrupts(hDmaChannel, NULL);

            // release reserved dma channel
            if (pContext->context.channelAddress != 0)
                {
                DeviceIoControl(g_hDmaDrv, IOCTL_DMA_RELEASE_CHANNEL, 
                    (VOID*)&pContext->context, sizeof(DmaChannelContext_t), NULL, 
                    0, NULL, NULL);
                }

            MmUnmapIoSpace((void*)pContext->pDmaChannel, 
                pContext->context.registerSize
                );
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
    IOCTL_DMA_REGISTER_EVENTHANDLE_IN RegisterEvent;
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

        RegisterEvent.hEvent = hEvent;
        RegisterEvent.processId = GetCurrentProcessId();
        RegisterEvent.pContext = &pContext->context;
        if (!DeviceIoControl(g_hDmaDrv, IOCTL_DMA_REGISTER_EVENTHANDLE, 
            (VOID*)&RegisterEvent, sizeof(IOCTL_DMA_REGISTER_EVENTHANDLE_IN), 
            NULL, 0, NULL, NULL))
            {
            goto cleanUp;
            }
        rc = TRUE;

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
    DWORD dwId = (DWORD) -1;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }

    __try
        {
            {
            CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;

            if (pContext->cookie == DMA_HANDLE_CHANNEL_COOKIE)
                {
        dwId = pContext->context.index;
        }
            }
            if(dwId == -1)
                {
                CeddkDmaDIChannelContext_t *pContext = (CeddkDmaDIChannelContext_t*)hDmaChannel;
                
                if(pContext->cookie == DMA_DEDICATED_CHANNEL_HANDLE_COOKIE)
                {
                dwId = pContext->index;
                }
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
    BOOL rc = FALSE;
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

        if (!DeviceIoControl(g_hDmaDrv, IOCTL_DMA_INTERRUPTDONE, 
            (VOID*)&pContext->context, sizeof(IOCTL_DMA_INTERRUPTDONE_IN), 
            NULL, 0, NULL, NULL))
            {
            goto cleanUp;
            }
        
        rc = TRUE;
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception trying to get last interrupt status\r\n"
            ));
        }
    
cleanUp:
    return rc; 
}


//------------------------------------------------------------------------------
//
//  Global:  DmaGetLogicalChannel
//
//  returns a pointer to the logical dma channels registers.
//
void* DmaGetLogicalChannel(HANDLE hDmaChannel)
{
    OMAP_DMA_LC_REGS* pChannel = NULL;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }

    __try
        {
            {
            CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;

            if (pContext->cookie == DMA_HANDLE_CHANNEL_COOKIE)
                {
                pChannel = pContext->pDmaChannel;
                }
            }
            if(pChannel == NULL)
                {
                CeddkDmaDIChannelContext_t *pContext = (CeddkDmaDIChannelContext_t*)hDmaChannel;

                if(pContext->cookie == DMA_DEDICATED_CHANNEL_HANDLE_COOKIE)
                {
                pChannel = pContext->pDmaChannel;
        }
            }
            if(pChannel == NULL)
            {
            goto cleanUp;
            }
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception trying to get dma registers\r\n"
            ));
        }
    
cleanUp:
    return (void*)pChannel;
}


//------------------------------------------------------------------------------
//
//  Global:  DmaDisableStandby
//
//  disable standby in dma controller
//
BOOL DmaDisableStandby(HANDLE hDmaChannel, BOOL noStandby)
{
    BOOL bDelicatedChannel = FALSE;
    BOOL rc = FALSE;
    IOCTL_DMA_DISABLESTANDBY_IN IoctlInput;
    CeddkDmaContext_t *pContext = (CeddkDmaContext_t*)hDmaChannel;
    CeddkDmaDIChannelContext_t  *pContext1= NULL;
    
    // check if dma handle is valid
    if (ValidateDmaDriver() == FALSE || hDmaChannel == NULL)
        {
        goto cleanUp;
        }

    __try
        {
        if (pContext->cookie != DMA_HANDLE_CHANNEL_COOKIE)
            {
            pContext1 = (CeddkDmaDIChannelContext_t  *) hDmaChannel;
            if (pContext1->cookie == DMA_DEDICATED_CHANNEL_HANDLE_COOKIE)
                {
                bDelicatedChannel = TRUE;
                }
                else
                {
                goto cleanUp;
                }
            }
            else
            {
                bDelicatedChannel = FALSE;
            }

        IoctlInput.bNoStandby = noStandby;
        IoctlInput.bDelicatedChannel = bDelicatedChannel;

        if (bDelicatedChannel)
            {
            IoctlInput.pContext = (void *) pContext1;
            }
            else
            {
            IoctlInput.pContext = (void *) &pContext->context;
            }
        if (!DeviceIoControl(g_hDmaDrv, IOCTL_DMA_DISABLESTANDBY, 
            (VOID*)&IoctlInput, sizeof(IOCTL_DMA_DISABLESTANDBY_IN),
            NULL, 0, NULL, NULL))
            {
            goto cleanUp;
            }

        rc = TRUE;
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        RETAILMSG(TRUE, (L"ERROR! CEDDK: "
            L"exception DmaDisableStandby\r\n"
            ));
        }

cleanUp:
    return rc; 
}

//------------------------------------------------------------------------------
