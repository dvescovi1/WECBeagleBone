//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File: edma_test.c
//
//  Driver to test the EDMA driver.
//

#include <windows.h>
#include <oal_log.h>  //+++WMK
#include <ceddk.h>
#include <am33x.h>
#include <edma.h>

//+++WMK for debug only
#define ZONE_ERROR          1
#define ZONE_WARN           1
#define ZONE_FUNCTION       1
#define ZONE_INIT           1
#define ZONE_INFO           0
#define ZONE_IST            0
#define ZONE_IOCTL          0
#define ZONE_VERBOSE        0
#include <edma3_dbg.h>

//------------------------------------------------------------------------------
//  Local Definitions
//

#define DEFAULT_TRANSFER_SIZE (0x1000)
#define DEFAULT_EVENT_QUEUE   1

// Table of event queues to use for each CC instance
static DWORD g_dwEventQueue[EDMA3_MAX_EDMA3_INSTANCES] = {
    0
}; 

typedef struct
{
    EDMA3_DRV_Handle hEdma[EDMA3_MAX_EDMA3_INSTANCES];
    UINT32 nOpen;
} EDMATEST_CONTEXT;

typedef struct
{
    UINT32 nChId;
    UINT32 nTcc;
    HANDLE hEvent;
    HANDLE hError;
} EDMA_TRANSFER;

typedef struct
{
    UINT32 nChAlloc[EDMA3_MAX_DMA_CH];
    UINT32 nTotalTime;
    UINT32 nTransfers;
} EDMA_STATS;

//------------------------------------------------------------------------------
//  Local Functions
//
static void EdmaTransferDeinit(EDMA3_DRV_Handle hEdma, EDMA_TRANSFER *pTransfer);


//------------------------------------------------------------------------------
//  Debug Zones
//
#ifdef DEBUG

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARNING        DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_TEST           DEBUGZONE(3)

DBGPARAM dpCurSettings = {
    L"EDMATEST", {
        L"Errors",      L"Warnings",    L"Function",    L"Test",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif


//------------------------------------------------------------------------------
//  Function:  EdmaTransferInit
//
static EDMA3_DRV_Result EdmaTransferInit(
    EDMA3_DRV_Handle hEdma,
    EDMA_TRANSFER *pTransfer,
    DWORD paFrom,
    DWORD paTo,
    DWORD dwSize,
    DWORD dwQueue,
    BOOL bIntMode)
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;

    ZeroMemory(pTransfer, sizeof(EDMA_TRANSFER));

    // Allocate a DMA channel
    pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
    pTransfer->nTcc = EDMA3_DRV_TCC_ANY;

    result = EDMA3_DRV_requestChannel(hEdma,
                                      &pTransfer->nChId,
                                      &pTransfer->nTcc,
                                      (EDMA3_RM_EventQueue)dwQueue,
                                      (EDMA3_RM_TccCallback)NULL,
                                      NULL);
    if (result != EDMA3_DRV_SOK)
    {
        pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
        RETAILMSG(1, (_T("EDT: Failed to allocate channel: %d\r\n"), result));
        goto _cleanup;
    }

    //+++WMK
#if 0
    OALMSG(ZONE_INIT,(_T("+++EdmaTransferInit: req'd chan=%d Tcc=%d Queue=%d \r\n"), 
               pTransfer->nChId, pTransfer->nTcc, dwQueue
             ));
#endif

    result = EDMA3_DRV_setSrcParams(hEdma,
                                    pTransfer->nChId,
                                    paFrom,
                                    EDMA3_DRV_ADDR_MODE_INCR,
                                    EDMA3_DRV_W8BIT);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set src params: %d\r\n"), result));
        goto _cleanup;
    }
    
    result = EDMA3_DRV_setDestParams(hEdma,
                                     pTransfer->nChId,
                                     paTo,
                                     EDMA3_DRV_ADDR_MODE_INCR,
                                     EDMA3_DRV_W8BIT);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set dst params: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setSrcIndex(hEdma,
                                   pTransfer->nChId,
                                   1,
                                   0);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set src indexing: %d\r\n"), result));
        goto _cleanup;
    }
    
    result = EDMA3_DRV_setDestIndex(hEdma,
                                    pTransfer->nChId,
                                    1,
                                    1);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set dst indexing: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setTransferParams(hEdma,
                                         pTransfer->nChId,
                                         dwSize,
                                         1,
                                         1,
                                         0,
                                         EDMA3_DRV_SYNC_A);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set transfer params: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setOptField(hEdma,
                                   pTransfer->nChId,
                                   EDMA3_DRV_OPT_FIELD_TCC,
                                   pTransfer->nTcc);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set TCC opt field: %d\r\n"), result));
        goto _cleanup;
    }

    if (bIntMode)
    {
        result = EDMA3_DRV_setOptField(hEdma,
                                       pTransfer->nChId,
                                       EDMA3_DRV_OPT_FIELD_TCINTEN,
                                       1);
        if (result != EDMA3_DRV_SOK)
        {
            RETAILMSG(1, (_T("EDT: Failed to set TCINTEN opt field: %d\r\n"), result));
            goto _cleanup;
        }
    }

    pTransfer->hEvent = EDMA3_DRV_getTransferEvent(hEdma, pTransfer->nChId);
    if (!pTransfer->hEvent)
    {
        RETAILMSG(1, (_T("EDT: Failed to get transfer event for ch %d\r\n"), pTransfer->nChId));
        result = EDMA3_DRV_E_INVALID_STATE;
        goto _cleanup;
    }

    pTransfer->hError = EDMA3_DRV_getErrorEvent(hEdma, pTransfer->nChId);
    if (!pTransfer->hError)
    {
        RETAILMSG(1, (_T("EDT: Failed to get error event for ch %d\r\n"), pTransfer->nChId));
        result = EDMA3_DRV_E_INVALID_STATE;
        goto _cleanup;
    }

_cleanup:

    if (result != EDMA3_DRV_SOK)
    {
        EdmaTransferDeinit(hEdma, pTransfer);
    }

    return result;
}

//------------------------------------------------------------------------------
//  Function:  EdmaQDMAInit
//
static EDMA3_DRV_Result EdmaQDMAInit(
    EDMA3_DRV_Handle hEdma,
    EDMA_TRANSFER *pTransfer,
    DWORD paFrom,
    DWORD paTo,
    DWORD dwSize,
    DWORD dwQueue,
    BOOL bIntMode)
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;

    ZeroMemory(pTransfer, sizeof(EDMA_TRANSFER));

    // Allocate a DMA channel
    pTransfer->nChId = EDMA3_DRV_QDMA_CHANNEL_ANY;
    pTransfer->nTcc = EDMA3_DRV_TCC_ANY;

    result = EDMA3_DRV_requestChannel(hEdma,
                                      &pTransfer->nChId,
                                      &pTransfer->nTcc,
                                      (EDMA3_RM_EventQueue)dwQueue,
                                      (EDMA3_RM_TccCallback)NULL,
                                      NULL);
    if (result != EDMA3_DRV_SOK)
    {
        pTransfer->nChId = EDMA3_DRV_DMA_CHANNEL_ANY;
        RETAILMSG(1, (_T("EDT: Failed to allocate channel: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setQdmaTrigWord(hEdma,
                                       pTransfer->nChId,
                                       EDMA3_RM_QDMA_TRIG_DST);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set QDMA trig word: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setSrcParams(hEdma,
                                    pTransfer->nChId,
                                    paFrom,
                                    EDMA3_DRV_ADDR_MODE_INCR,
                                    EDMA3_DRV_W8BIT);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set src params: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setOptField(hEdma,
                                   pTransfer->nChId,
                                   EDMA3_DRV_OPT_FIELD_DAM,
                                   EDMA3_DRV_ADDR_MODE_INCR);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set DAM opt field: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setOptField(hEdma,
                                   pTransfer->nChId,
                                   EDMA3_DRV_OPT_FIELD_FWID,
                                   EDMA3_DRV_W8BIT);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set FWIW opt field: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setOptField(hEdma,
                                   pTransfer->nChId,
                                   EDMA3_DRV_OPT_FIELD_STATIC,
                                   EDMA3_DRV_STATIC_EN);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set STATIC opt field: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setSrcIndex(hEdma,
                                   pTransfer->nChId,
                                   1,
                                   0);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set src indexing: %d\r\n"), result));
        goto _cleanup;
    }
    
    result = EDMA3_DRV_setDestIndex(hEdma,
                                    pTransfer->nChId,
                                    1,
                                    1);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set dst indexing: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setTransferParams(hEdma,
                                         pTransfer->nChId,
                                         dwSize,
                                         1,
                                         1,
                                         0,
                                         EDMA3_DRV_SYNC_A);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set transfer params: %d\r\n"), result));
        goto _cleanup;
    }

    result = EDMA3_DRV_setOptField(hEdma,
                                   pTransfer->nChId,
                                   EDMA3_DRV_OPT_FIELD_TCC,
                                   pTransfer->nTcc);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to set TCC opt field: %d\r\n"), result));
        goto _cleanup;
    }

    if (bIntMode)
    {
        result = EDMA3_DRV_setOptField(hEdma,
                                       pTransfer->nChId,
                                       EDMA3_DRV_OPT_FIELD_TCINTEN,
                                       1);
        if (result != EDMA3_DRV_SOK)
        {
            RETAILMSG(1, (_T("EDT: Failed to set TCINTEN opt field: %d\r\n"), result));
            goto _cleanup;
        }
    }

    pTransfer->hEvent = EDMA3_DRV_getTransferEvent(hEdma, pTransfer->nChId);
    if (!pTransfer->hEvent)
    {
        RETAILMSG(1, (_T("EDT: Failed to get transfer event for ch %d\r\n"), pTransfer->nChId));
        result = EDMA3_DRV_E_INVALID_STATE;
        goto _cleanup;
    }

    pTransfer->hError = EDMA3_DRV_getErrorEvent(hEdma, pTransfer->nChId);
    if (!pTransfer->hError)
    {
        RETAILMSG(1, (_T("EDT: Failed to get error event for ch %d\r\n"), pTransfer->nChId));
        result = EDMA3_DRV_E_INVALID_STATE;
        goto _cleanup;
    }

_cleanup:

    if (result != EDMA3_DRV_SOK)
    {
        EdmaTransferDeinit(hEdma, pTransfer);
    }

    return result;
}

//------------------------------------------------------------------------------
//  Function:  EdmaTransferDeinit
//
static void EdmaTransferDeinit(
    EDMA3_DRV_Handle hEdma, EDMA_TRANSFER *pTransfer)
{
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
}


//------------------------------------------------------------------------------
//  Function:  EdmaPerformTransfer
//
static EDMA3_DRV_Result EdmaPerformTransfer(EDMA3_DRV_Handle hEdma, BOOL bIntMode, BOOL bLinked,
                                            DWORD paSrcBuff1, DWORD paDstBuff1, DWORD dwSize1,
                                            DWORD paSrcBuff2, DWORD paDstBuff2, DWORD dwSize2,
                                            DWORD dwQueue, EDMA_STATS *stats)
{
    EDMA_TRANSFER transfer1;
    EDMA_TRANSFER transfer2;
    EDMA_TRANS_STATUS channelStatus;
    DWORD nWaitResult;
    HANDLE aHandles[2];
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    LARGE_INTEGER nStartTime, nEndTime;

    ZeroMemory(&transfer1, sizeof(EDMA_TRANSFER));
    ZeroMemory(&transfer2, sizeof(EDMA_TRANSFER));

    result = EdmaTransferInit(hEdma, &transfer1, 
                              paSrcBuff1, paDstBuff1,
                              dwSize1, dwQueue, bIntMode);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to initialise transfer 1\r\n")));
        goto _cleanup;
    }

    stats->nChAlloc[transfer1.nChId]++;

    if (bLinked)
    {
        result = EdmaTransferInit(hEdma, &transfer2,
                                  paSrcBuff2, paDstBuff2,
                                  dwSize2, dwQueue, bIntMode);
        if (result != EDMA3_DRV_SOK)
        {
            RETAILMSG(1, (_T("EDT: Failed to initialise transfer 2\r\n")));
            goto _cleanup;
        }

        stats->nChAlloc[transfer2.nChId]++;

        result = EDMA3_DRV_linkChannel(hEdma, transfer1.nChId, transfer2.nChId);
        if (result != EDMA3_DRV_SOK)
        {
            RETAILMSG(1, (_T("EDT: Failed to link transfers: %d\r\n"), result));
            goto _cleanup;
        }
    }

    QueryPerformanceCounter(&nStartTime);

    result = EDMA3_DRV_enableTransfer(hEdma, transfer1.nChId, EDMA3_DRV_TRIG_MODE_MANUAL);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to enable transfer 1: %d\r\n"), result));
        goto _cleanup;
    }

    if (bIntMode)
    {
        aHandles[0] = transfer1.hEvent;
        aHandles[1] = transfer1.hError;
        nWaitResult = WaitForMultipleObjects(2, aHandles, FALSE, 2000 /*+++WMK orig: 2000*/);
        if (nWaitResult == WAIT_TIMEOUT)
        {
            RETAILMSG(1, (_T("EDT: Timeout waiting for transfer 1 DMA completion\r\n")));

#if 0
            EDMA3_DRV_ShowIRs(hEdma); //+++WMK
            EDMA3_DRV_ShowShadowIRs(hEdma); //+++WMK
#endif

            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
        if (nWaitResult == WAIT_OBJECT_0 + 1)
        {
            RETAILMSG(1, (_T("EDT: EDMA error indicated for transfer 1\r\n")));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }

        QueryPerformanceCounter(&nEndTime);
        stats->nTotalTime += (UINT32)(nEndTime.QuadPart - nStartTime.QuadPart);
        stats->nTransfers++;

        channelStatus = EDMA3_DRV_getTransferStatus(hEdma, transfer1.nChId);
        if ((channelStatus & EDMA_STAT_TRANSFER_COMPLETE) == 0)
        {
            RETAILMSG(1, (_T("EDT: EDMA reports transfer 1 not complete (0x%x)\n"), channelStatus));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
        if (channelStatus & EDMA_STAT_EVENT_MISSED)
        {
            RETAILMSG(1, (_T("EDT: EDMA reports event missed for transfer 1 (0x%x)\n"), channelStatus));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
    }
    else
    {
        // 500ms should be enough to complete transfer
        Sleep(500);
    }

    if(bLinked)
    {
        // Enable again for linked channel (Bcnt * Ccnt = 1)
        result = EDMA3_DRV_enableTransfer(hEdma, transfer1.nChId, EDMA3_DRV_TRIG_MODE_MANUAL);
        if (result != EDMA3_DRV_SOK)
        {
            RETAILMSG(1, (_T("EDT: Failed to enable link transfer: %d\r\n"), result));
            goto _cleanup;
        }

        if (bIntMode)
        {
            aHandles[0] = transfer1.hEvent;
            aHandles[1] = transfer1.hError;
            nWaitResult = WaitForMultipleObjects(2, aHandles, FALSE, 2000);
            if (nWaitResult == WAIT_TIMEOUT)
            {
                RETAILMSG(1, (_T("EDT: Timeout waiting for link transfer DMA completion\r\n")));
                result = EDMA3_DRV_E_INVALID_STATE;
                goto _cleanup;
            }
            if (nWaitResult == WAIT_OBJECT_0 + 1)
            {
                RETAILMSG(1, (_T("EDT: EDMA error indicated for link transfer\r\n")));
                result = EDMA3_DRV_E_INVALID_STATE;
                goto _cleanup;
            }

            channelStatus = EDMA3_DRV_getTransferStatus(hEdma, transfer1.nChId);
            if ((channelStatus & EDMA_STAT_TRANSFER_COMPLETE) == 0)
            {
                RETAILMSG(1, (_T("EDT: EDMA reports link transfer not complete (0x%x)\n"), channelStatus));
                result = EDMA3_DRV_E_INVALID_STATE;
                goto _cleanup;
            }
            if (channelStatus & EDMA_STAT_EVENT_MISSED)
            {
                RETAILMSG(1, (_T("EDT: EDMA reports event missed for link transfer (0x%x)\n"), channelStatus));
                result = EDMA3_DRV_E_INVALID_STATE;
                goto _cleanup;
            }
        }
        else
        {
            // 500ms should be enough to complete transfer
            Sleep(500);
        }
    }

    EDMA3_DRV_disableTransfer(hEdma, transfer1.nChId, EDMA3_DRV_TRIG_MODE_MANUAL);

_cleanup:

    EdmaTransferDeinit(hEdma, &transfer1);
    if (bLinked)
        EdmaTransferDeinit(hEdma, &transfer2);

    return result;
}


//------------------------------------------------------------------------------
//  Function:  EdmaPerformQDMA
//
static EDMA3_DRV_Result EdmaPerformQDMA(EDMA3_DRV_Handle hEdma, BOOL bIntMode, BOOL bLinked,
                                        DWORD paSrcBuff1, DWORD paDstBuff1, DWORD dwSize1,
                                        DWORD paSrcBuff2, DWORD paDstBuff2, DWORD dwSize2,
                                        DWORD dwQueue, EDMA_STATS *stats)
{
    EDMA_TRANSFER transfer1;
    EDMA_TRANSFER transfer2;
    EDMA_TRANS_STATUS channelStatus;
    DWORD nWaitResult;
    HANDLE aHandles[2];
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;

    // NOTE: Linked QDMA not implemented

    ZeroMemory(&transfer1, sizeof(EDMA_TRANSFER));
    ZeroMemory(&transfer2, sizeof(EDMA_TRANSFER));

    result = EdmaQDMAInit(hEdma, &transfer1, 
                          paSrcBuff1, paDstBuff1,
                          dwSize1, dwQueue, bIntMode);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to initialise transfer 1\r\n")));
        goto _cleanup;
    }

    // Trigger QDMA by writing dest address
    result = EDMA3_DRV_setPaRAMEntry(hEdma,
                                     transfer1.nChId,
                                     EDMA3_DRV_PARAM_ENTRY_DST,
                                     paDstBuff1);
    if (result != EDMA3_DRV_SOK)
    {
        RETAILMSG(1, (_T("EDT: Failed to enable QDMA transfer: %d\r\n"), result));
        goto _cleanup;
    }

    if (bIntMode)
    {
        aHandles[0] = transfer1.hEvent;
        aHandles[1] = transfer1.hError;
        nWaitResult = WaitForMultipleObjects(2, aHandles, FALSE, 2000);
        if (nWaitResult == WAIT_TIMEOUT)
        {
            RETAILMSG(1, (_T("EDT: Timeout waiting for transfer 1 DMA completion\r\n")));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
        if (nWaitResult == WAIT_OBJECT_0 + 1)
        {
            RETAILMSG(1, (_T("EDT: EDMA error indicated for transfer 1\r\n")));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }

        channelStatus = EDMA3_DRV_getTransferStatus(hEdma, transfer1.nChId);
        if ((channelStatus & EDMA_STAT_TRANSFER_COMPLETE) == 0)
        {
            RETAILMSG(1, (_T("EDT: EDMA reports transfer 1 not complete (0x%x)\n"), channelStatus));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
        if (channelStatus & EDMA_STAT_EVENT_MISSED)
        {
            RETAILMSG(1, (_T("EDT: EDMA reports event missed for transfer 1 (0x%x)\n"), channelStatus));
            result = EDMA3_DRV_E_INVALID_STATE;
            goto _cleanup;
        }
    }
    else
    {
        // 500ms should be enough to complete transfer
        Sleep(500);
    }

    EDMA3_DRV_disableTransfer(hEdma, transfer1.nChId, EDMA3_DRV_TRIG_MODE_MANUAL);

_cleanup:

    EdmaTransferDeinit(hEdma, &transfer1);

    return result;
}


//------------------------------------------------------------------------------
//  Function:  EdmaMemToMemCopyTest
//
static BOOL EdmaMemToMemCopyTest(EDMA3_DRV_Handle hEdma,
                                 BOOL bQDMA, BOOL bIntMode, BOOL bLinked,
                                 DWORD dwSize, DWORD dwQueue, EDMA_STATS *stats)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    DMA_ADAPTER_OBJECT DmaAdapter;
    UINT32 *srcBuff1,*dstBuff1,*srcBuff2,*dstBuff2;    
    DWORD paSrcBuff1, paDstBuff1, paSrcBuff2, paDstBuff2;
    UINT8 *pDmaBufferBase;
    BOOL fTestPassed = FALSE;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    UINT i;

    DmaAdapter.ObjectSize = sizeof(DmaAdapter);
    DmaAdapter.InterfaceType = Internal;
    DmaAdapter.BusNumber = 0;

    memset(&PhysicalAddress, 0, sizeof(PHYSICAL_ADDRESS));
    pDmaBufferBase = (UINT8*)HalAllocateCommonBuffer(
        &DmaAdapter, dwSize * 4, &PhysicalAddress, FALSE);
    if(!pDmaBufferBase)
    {
        RETAILMSG(1, (_T("EDT: Failed to allocate DMA buffer\r\n")));
        goto _cleanup;
    }

    srcBuff1 = (UINT32*)(pDmaBufferBase);
    dstBuff1 = (UINT32*)(pDmaBufferBase + dwSize);
    srcBuff2 = (UINT32*)(pDmaBufferBase + dwSize * 2);
    dstBuff2 = (UINT32*)(pDmaBufferBase + dwSize * 3);

    paSrcBuff1 = PhysicalAddress.LowPart;
    paDstBuff1 = PhysicalAddress.LowPart + dwSize;
    paSrcBuff2 = PhysicalAddress.LowPart + dwSize * 2;
    paDstBuff2 = PhysicalAddress.LowPart + dwSize * 3;

    for (i = 0; i < dwSize / 4; i++) 
    {
        srcBuff1[i] = i | 0x1234;
        dstBuff1[i] = 0;
        srcBuff2[i] = i | 0xABCD;
        dstBuff2[i] = 0;
    }

//    EDMA3_DRV_ShowCCCFG(hEdma); //+++WMK

    if (bQDMA)
        result = EdmaPerformQDMA(hEdma, bIntMode, bLinked,
                                 paSrcBuff1, paDstBuff1, dwSize,
                                 paSrcBuff2, paDstBuff2, dwSize,
                                 dwQueue, stats);
    else
        result = EdmaPerformTransfer(hEdma, bIntMode, bLinked,
                                     paSrcBuff1, paDstBuff1, dwSize,
                                     paSrcBuff2, paDstBuff2, dwSize,
                                     dwQueue, stats);

    if (result != EDMA3_DRV_SOK)
        goto _cleanup;

    fTestPassed = TRUE;
    for (i = 0; i < dwSize / 4; i++)
    {
        if (srcBuff1[i] != dstBuff1[i])
        { 
            fTestPassed = FALSE;
            RETAILMSG(1, (_T("EDT: Transfer 1 data mismatch at offset %d\r\n"), i));
            break;
        }
        if (bLinked)
        {
            if (srcBuff2[i] != dstBuff2[i])
            {
                fTestPassed = FALSE;
                RETAILMSG(1, (_T("EDT: Transfer 2 data mismatch at offset %d\r\n"), i));
                break;
            }
        }
    }

//    EDMA3_DRV_ShowQSTAT(hEdma); //+++WMK

_cleanup:

    if (pDmaBufferBase)
        HalFreeCommonBuffer(&DmaAdapter, dwSize * 4, PhysicalAddress, pDmaBufferBase, FALSE);

    if (result != EDMA3_DRV_SOK)
        fTestPassed = FALSE;

    return fTestPassed;
}


//------------------------------------------------------------------------------
//  Function:  EdmaAttach
//
static BOOL EdmaAttach(EDMATEST_CONTEXT *pContext)
{
    BOOL rc = FALSE;
    EDMA3_DRV_Result errorCode;
    int i;
    
    for (i = 0; i < EDMA3_MAX_EDMA3_INSTANCES; i++)
    {
        pContext->hEdma[i] = EDMA3_DRV_getInstHandle(i,
                                              EDMA3_ARM_REGION_ID, &errorCode);
        if (pContext->hEdma[i] == NULL || errorCode != EDMA3_DRV_SOK)
        {
            DEBUGMSG(ZONE_ERROR, (L"EdmaAttach: ERROR - EDMA3_DRV_getInstHandle() failed! Error %08X\r\n",
                                  errorCode));
            goto _done;
        }
    
        rc = TRUE;
    }
_done:
    return rc;
}

//------------------------------------------------------------------------------
//  Function:  EdmaDetach
//
static void EdmaDetach(EDMATEST_CONTEXT *pContext)
{
    int i;
    
    for (i = 0; i < EDMA3_MAX_EDMA3_INSTANCES; i++)
    {
        if (pContext->hEdma[i] != NULL)
        {
            EDMA3_DRV_releaseInstHandle(pContext->hEdma[i]);
            pContext->hEdma[i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------
//  Function:  EDT_Init
//
DWORD EDT_Init(LPCTSTR szContext, LPCVOID pBusContext)
{
    EDMATEST_CONTEXT *pContext;

    DEBUGMSG(ZONE_FUNCTION, (L"EDT_Init(0x%08x, 0x%08x)\r\n", szContext, pBusContext));

    pContext = (EDMATEST_CONTEXT *)LocalAlloc(LPTR, sizeof(EDMATEST_CONTEXT));
    if (pContext == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
                 (L"EDT_Init: ERROR - Failed allocate EDT context structure\r\n"));
    }

    return (DWORD)pContext;
}

//------------------------------------------------------------------------------
//  Function:  EDT_Deinit
//
BOOL EDT_Deinit(DWORD context)
{
    EDMATEST_CONTEXT *pContext = (EDMATEST_CONTEXT *)context;

    DEBUGMSG(ZONE_FUNCTION, (L"EDT_Deinit(0x%08x)\r\n", context));

    if (pContext)
    {
        LocalFree(pContext);
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//  Function:  EDT_Open
//
DWORD EDT_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{
    EDMATEST_CONTEXT *pContext = (EDMATEST_CONTEXT *)context;

    DEBUGMSG(ZONE_FUNCTION, (L"EDT_Open(0x%08x, 0x%08x, 0x%08x)\r\n",
                             context, accessCode, shareMode));

    if (pContext->nOpen == 0)
    {
        if (!EdmaAttach(pContext))
        {
            DEBUGMSG(ZONE_ERROR, (L"EDT_Open: ERROR - Failed to attach to EDMA\r\n"));
            pContext = NULL;
        }
    }

    if (pContext)
        pContext->nOpen++;

    return (DWORD)pContext;
}

//------------------------------------------------------------------------------
//  Function:  EDT_Close
//
BOOL EDT_Close(DWORD context)
{
    EDMATEST_CONTEXT *pContext = (EDMATEST_CONTEXT *)context;

    DEBUGMSG(ZONE_FUNCTION, (L"EDT_Close(0x%08x)\r\n", context));

    pContext->nOpen--;

    if (pContext->nOpen == 0)
    {
        EdmaDetach(pContext);
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  EDT_IOControl
//
//  This function sends a command to a device.
//
BOOL EDT_IOControl(
    DWORD context, DWORD dwCode,
    BYTE *pInBuffer, DWORD inSize,
    BYTE *pOutBuffer, DWORD outSize, DWORD *pOutSize)
{
    EDMATEST_CONTEXT *pContext = (EDMATEST_CONTEXT *)context;
    EDMA_STATS stats;
    DWORD *pParams;
    DWORD tick;
    BOOL rc = FALSE;
    int i;
    DWORD dwInstance;

    DEBUGMSG(ZONE_FUNCTION,
             (L"+EDT_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
              context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize));

    ZeroMemory(&stats, sizeof(stats));

    // First parameter is the channel controller instance number and
    // is required for all tests
    if (pInBuffer == NULL || inSize < sizeof(DWORD))
    {
        DEBUGMSG(ZONE_ERROR, (L"+EDT_IOControl: ERROR - invalid parameters\r\n"));
        return FALSE;
    }
    
    pParams = (DWORD *)pInBuffer;
    dwInstance = pParams[0];
    if (dwInstance >= EDMA3_MAX_EDMA3_INSTANCES)
    {
        DEBUGMSG(ZONE_ERROR, (L"+EDT_IOControl: ERROR - invalid CC instance number\r\n"));
        return FALSE;
    }

    switch (dwCode)
    {
    case 0: // parameterised test
        if (pInBuffer == NULL || inSize != sizeof(DWORD) * 7)
        {
            DEBUGMSG(ZONE_ERROR, (L"+EDT_IOControl: ERROR - invalid parameters\r\n"));
            rc = FALSE;
        }
        else
        {
            pParams = (DWORD *)pInBuffer;
            rc = TRUE;
            tick = GetTickCount();
            for (i = 0; i < (int)pParams[6] && rc; ++i)
                rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], pParams[1], pParams[2], pParams[3],
                                          pParams[4], pParams[5], &stats);
            RETAILMSG(1, (L"EDT: Test duration %dms\r\n", GetTickCount() - tick));
            if (stats.nTransfers > 0)
            {
                LARGE_INTEGER nFreq;
                QueryPerformanceFrequency(&nFreq);
                RETAILMSG(1, (L"EDT: Average transfer time %dus\r\n",
                              (stats.nTotalTime / stats.nTransfers) * 1000 / (nFreq.LowPart / 1000)));
            }
            for (i = 0; i < EDMA3_MAX_DMA_CH; ++i)
            {
                if (stats.nChAlloc[i])
                    RETAILMSG(1, (L"EDT: Ch %d allocated %d times\r\n", i, stats.nChAlloc[i]));
            }
        }
        break;

    case 1: // manual transfer, no interrupt
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], FALSE, FALSE, FALSE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    case 2: // manual transfer, with completion interrupt
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], FALSE, TRUE, FALSE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    case 3: // manual transfer, no interrupt, 2 linked channels
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], FALSE, FALSE, TRUE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    case 4: // manual transfer, with completion interrupt, 2 linked channels
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], FALSE, TRUE, TRUE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    case 5: // test 2 repeated 5000 times with small transfer size
        rc = TRUE;
        for (i = 0; i < 5000 && rc; ++i)
            rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], FALSE, TRUE, FALSE,
                                      0x10, g_dwEventQueue[dwInstance], &stats);

        for (i = 0; i < EDMA3_MAX_DMA_CH; ++i)
        {
            if (stats.nChAlloc[i])
                RETAILMSG(1, (L"EDT: Ch %d allocated %d times\r\n", i, stats.nChAlloc[i]));
        }
        break;

    case 6: // QDMA transfer, no interrupt
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], TRUE, FALSE, FALSE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    case 7: // QDMA transfer, with completion interrupt
        rc = EdmaMemToMemCopyTest(pContext->hEdma[dwInstance], TRUE, TRUE, FALSE,
                                  DEFAULT_TRANSFER_SIZE, g_dwEventQueue[dwInstance], &stats);
        break;

    default:
        DEBUGMSG(ZONE_ERROR,
                 (L"+EDT_IOControl: ERROR - unrecognised IOCTL %d\r\n", dwCode));
        rc = FALSE;
        break;
    }

    DEBUGMSG(ZONE_FUNCTION, (L"-EDT_IOControl(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//  Function:  DllMain
//
BOOL __stdcall DllMain(HANDLE hInstance, DWORD dwReason, VOID *pReserved)
{
    switch (dwReason) 
    {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER((HMODULE)hInstance);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
