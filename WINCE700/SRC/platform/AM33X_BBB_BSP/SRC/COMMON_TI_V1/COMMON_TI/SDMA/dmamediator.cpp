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
//  File: dmamediator.cpp
//

#include "omap.h"
//#include <pm.h>
#include <ceddk.h>
#include "ceddkex.h"
#include "_debug.h"
#include "omap_sdma_regs.h"
#include "dmacontroller.h"
#include "dmamediator.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//------------------------------------------------------------------------------
// DmaMediator
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
DWORD 
DmaMediator::InterruptHandler(
    void* pData
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::InterruptHandler: "
        L"pData=0x%08X\r\n",pData
        ));
    
    DmaMediator* pMediator = (DmaMediator*)pData;
    DWORD sysIntr = pMediator->m_sysIntr;

    for(;;)
        {
        __try
            {
            // wait for interrupt
            WaitForSingleObject(pMediator->m_hEvent, INFINITE);            
            if (pMediator->m_bExit == TRUE) break;

            DEBUGMSG(ZONE_IST, (L"DmaMediator::InterruptHandler: "
                L"received dma interrupt\r\n"
                ));

            // process interrupt
            pMediator->ProcessInterrupt();
            }
        __except (EXCEPTION_EXECUTE_HANDLER)
            {
            RETAILMSG(ZONE_ERROR, (L"DmaMediator::InterruptHandler: "
                L"exception in interrupt handler\r\n"
                ));
            }
        // reset for next interrupt
        ::InterruptDone(sysIntr);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::InterruptHandler\r\n"));
    return 1;
}


//------------------------------------------------------------------------------
void 
DmaMediator::SetDmaController(
    HANDLE context, 
    DmaControllerBase *pDmaController
    )    
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::SetDmaController"
        L"context=0x%08X, pDmaController=0x%08X\r\n",
        context, pDmaController
        ));

    m_context = context;
    m_pDmaController = pDmaController;
    

    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::SetDmaController\r\n"));
}


//------------------------------------------------------------------------------
BOOL 
DmaMediator::StartInterruptHandler(
    DWORD irq, 
    DWORD priority
    )    
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::StartInterruptHandler: "
        L"irq=%d, priority=%d\r\n",
        irq, priority
        ));

    BOOL rc = FALSE;

    // make sure we have a way of getting cause of interrupt
    if (m_pDmaController == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: DmaMediator::StartInterruptHandler: "
            L"DmaController not set\r\n"
            ));
        goto cleanUp;
        }
    
    // get system interrupt for irq
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &irq, 
            sizeof(irq), &m_sysIntr, sizeof(m_sysIntr), 
            NULL)) 
        {
        DEBUGMSG(ZONE_WARN, (L"ERROR: DmaMediator::StartInterruptHandler: "
            L"Failed map DMA interrupt(irq=%d)\r\n", irq
            ));
        goto cleanUp;
        }

    // create thread event handle
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEvent == NULL) 
        {
        DEBUGMSG(ZONE_WARN, (L"ERROR: DmaMediator::StartInterruptHandler: "
            L"Failed create event object for interrupt\r\n"
            ));
        goto cleanUp;
        }

    // register event handle with system interrupt
    if (!InterruptInitialize(m_sysIntr, m_hEvent, NULL, 0)) 
        {
        DEBUGMSG(ZONE_WARN, (L"ERROR: DmaMediator::StartInterruptHandler: "
            L"InterruptInitialize failed\r\n"
            ));
        goto cleanUp;
        }

    // spawn thread
    m_bExit = FALSE;
    m_hIST = CreateThread(NULL, 0, InterruptHandler, this, 0, NULL);
    if (!m_hIST)
        {
        DEBUGMSG(ZONE_WARN, (L"ERROR: DmaMediator::StartInterruptHandler: "
            L"Failed create interrupt thread\r\n"
            ));
        goto cleanUp;
        }

    // set thread priority
    CeSetThreadPriority(m_hIST, priority);

    rc = TRUE;
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::StartInterruptHandler: "
        L"rc=%d\r\n", rc
        ));
    if (rc == FALSE) StopInterruptHandler();
    
    return rc;
}


//------------------------------------------------------------------------------
void 
DmaMediator::StopInterruptHandler()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::StopInterruptHandler\r\n"));

    // stop thread
    if (m_hEvent != NULL)
        {
        if (m_hIST != NULL)
            {
            // Signal stop to thread
            m_bExit = TRUE;
            
            // Set event to wake it
            SetEvent(m_hEvent);
            
            // Wait until thread exits
            WaitForSingleObject(m_hIST, INFINITE);
            
            // Close handle
            CloseHandle(m_hIST);

            // reinit
            m_hIST = NULL;
            }

        // close event handle
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
        }

    // unregister system interrupt
    if (m_sysIntr != 0)
        {
        InterruptDisable(m_sysIntr);
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR, &m_sysIntr,
            sizeof(m_sysIntr), NULL, 0, NULL
            );

        // reinit
        m_sysIntr = 0;
        }    

    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::StopInterruptHandler\r\n"));
}


//------------------------------------------------------------------------------
BOOL 
DmaMediator::ProcessInterrupt()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::ProcessInterrupt\r\n"));

    // get the dma's which caused the interrupt
    BOOL rc = FALSE;
    DWORD dwInterruptMask;
    DWORD dwInterruptOrigMask;
    if (m_pDmaController->GetInterruptMask(m_context, &dwInterruptOrigMask) == FALSE)
        {
        RETAILMSG(ZONE_WARN, (L"WARN! DmaMediator::ProcessInterrupt: ",
            L"failed to get dma interrupt mask\r\n"
            ));
        goto cleanUp;
        }

    // filter mask to only include valid logical channels
    dwInterruptMask = dwInterruptOrigMask & (0xFFFFFFFF >> (32 - m_dmaChannelCount));

    // loop through and service all interrupts for logical channels
    int i = 0;    
    while (dwInterruptMask)
        {
        if (dwInterruptMask & 1)
            {
            // notify secondary interrupts
            if ((1 << i) & m_ffSIHList)
                {
                SetEvent(m_pDmaChannelInfo[i].secondaryEventHandle);
                }

            DEBUGMSG(ZONE_INFO, (L"ERROR! DmaMediator::ProcessInterrupt: "
                L"dwInterruptMask(0x%08X)\r\n",
                dwInterruptMask
                ));
            }
        ++i;
        dwInterruptMask >>= 1;
        }

    if (m_pDmaController->ClearInterruptMask(m_context, dwInterruptOrigMask) == FALSE)
        {
        RETAILMSG(ZONE_WARN, (L"WARN! DmaMediator::ProcessInterrupt: ",
            L"failed to clear dma interrupt mask\r\n"
            ));
        goto cleanUp;
        }
    
    rc = TRUE;
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::ProcessInterrupt: "
        L"(rc=%d)\r\n", rc
        ));
    
    return rc;
}


//------------------------------------------------------------------------------
BOOL DmaMediator::ResetChannel(int index)
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::ResetChannel: " 
        L"index=%d\r\n", index
        ));

    // disable interrupts
    EnableInterrupt(index, FALSE);

    // create mask and use it to see if anything was set for the dma channel
    DWORD dwMask = (1 << index);
    if (m_ffSIHList & dwMask)
        {
        SetEventHandler(index, NULL, 0);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::ResetChannel: " 
        L"index=%d\r\n", index
        ));

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL 
DmaMediator::SetEventHandler(
    int index, 
    HANDLE hEvent,
    DWORD processId
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::SetEventHandler: " 
        L"index=%d, hEvent=0x%08X, processId=0x%08X\r\n", 
        index, hEvent, processId
        ));
    
    BOOL rc = FALSE;
    HANDLE hLocalHandle = NULL;
    
    DEBUGMSG(ZONE_INFO, (L"DmaMediator::SetEventHandler: "
        L"Setting event handler(0x%08X) for index(%d)", hEvent, index
        ));

    // Disable/Enable interrupts for logical channel
    EnableInterrupt(index, hEvent != NULL);

    ASSERT(m_pDmaChannelInfo);
    ASSERT(index < m_dmaChannelCount);
    if (hEvent != NULL)
        {        
        HANDLE hProc;
        BOOL bResult;
        
        // make sure there isn't someone else already registered
        if (m_pDmaChannelInfo[index].secondaryEventHandle != NULL)
            {
            RETAILMSG(ZONE_WARN, (L"WARN! DmaMediator::SetEventHandler: "
                L"SIH already exists(%d)\r\n", index
                ));
            goto cleanUp;
            }

        // get process handle which owns the event handle
        hProc = OpenProcess(0, FALSE, processId);
        if (hProc == NULL)
            {
            RETAILMSG(ZONE_ERROR, (L"ERROR! DmaMediator::SetEventHandler: "
                L"Unable to get process handle (err=0x%08X)\r\n",
                GetLastError()
                ));
            EnableInterrupt(index, FALSE);
            goto cleanUp;
            }
        
        // create duplicate handle and close process handle
        bResult = DuplicateHandle(hProc, hEvent, GetCurrentProcess(),
                        &hLocalHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

        CloseHandle(hProc);

        // check if we have a valid local copy of event handle
        if (bResult == FALSE)
            {
            RETAILMSG(ZONE_ERROR, (L"ERROR! DmaMediator::SetEventHandler: "
                L"unable to duplicate event handle err(0x%08X)\r\n",
                GetLastError()
                ));
            EnableInterrupt(index, FALSE);
            goto cleanUp;
            }

        // update SIH list
        m_ffSIHList |= (1 << index);
        
        }
    else if (m_pDmaChannelInfo[index].secondaryEventHandle != NULL)
        {
        // delete old handle if there was one
        CloseHandle(m_pDmaChannelInfo[index].secondaryEventHandle);

        // update SIH list
        m_ffSIHList &= ~(1 << index);
        }

    m_pDmaChannelInfo[index].secondaryEventHandle = hLocalHandle;

    rc = TRUE;
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::SetEventHandler(rc=%d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
BOOL 
DmaMediator::Initialize(
    int count, 
    OMAP_DMA_LC_REGS rgDmaChannels[]
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::Initialize: " 
        L"count=%d\r\n", count
        ));

    BOOL rc = FALSE;

    // make sure memory isn't already allocated
    ASSERT(m_pDmaChannelInfo == NULL);
    if (m_pDmaChannelInfo != NULL)
        {
        RETAILMSG(ZONE_WARN, (L"DmaMediator::Initialize: "
            L"memory already allocated for array\r\n"
            ));
        goto cleanUp;
        }

    // allocate memory and initialize
    m_pDmaChannelInfo = (DmaChannelInfo_t*)LocalAlloc(LPTR, 
                                        sizeof(DmaChannelInfo_t) * count);
    if (m_pDmaChannelInfo == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR! DmaMediator::Initialize: "
            L"memory allocation failed for array\r\n"
            ));
        goto cleanUp;
        }

    memset(m_pDmaChannelInfo, 0, sizeof(DmaChannelInfo_t) * count);

    // copy virtual address of dma logical channels
    m_dmaChannelCount = count;
    for (int i = 0; i < count; ++i)
        {
        m_pDmaChannelInfo[i].pDmaRegs = &rgDmaChannels[i];
        }

    m_ffSIHList = 0;
    
    rc = TRUE;
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::Initialize(rc=%d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
void 
DmaMediator::Uninitialize()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+DmaMediator::Uninitialize\r\n"));

    // stop interrupt thread
    StopInterruptHandler();
    
    if (m_pDmaChannelInfo != NULL)
        {
        // free local copy of handles
        for (int i = 0; i < m_dmaChannelCount; ++i)
            {
            if (m_pDmaChannelInfo[i].secondaryEventHandle != NULL)
                {
                CloseHandle(m_pDmaChannelInfo[i].secondaryEventHandle);
                }
            }

        // free memory
        LocalFree(m_pDmaChannelInfo);
        m_pDmaChannelInfo = NULL;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-DmaMediator::Uninitialize\r\n"));
}




