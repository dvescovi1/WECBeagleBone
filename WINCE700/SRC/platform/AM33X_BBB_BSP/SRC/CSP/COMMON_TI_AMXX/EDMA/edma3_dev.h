//
//   Copyright (c) MPC Data Limited 2009. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File:  edma3_dev.h
//
//  Internal EDMA driver definitions. Other drivers should include edma.h.
//
#ifndef __EDMA3_DEV_H
#define __EDMA3_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "edma.h"
#include "edma3.h"
#include "edma3_drv.h"
#include "edma3resmgr.h"

// Prevent this header being included outside of the EDMA driver build.
#ifndef EDMA_INTERNAL_BUILD
#error "Internal EDMA header included, use edma.h instead"
#endif


//------------------------------------------------------------------------------
//  Misc definitions

// Number of transfer controllers
#define EDMA3_NUM_TC 4

// Number of CC error events
#define EDMA3_NUM_CCERRORS (EDMA3_NUM_TC+1)

// Completion / error event names
#define EDMA_EVENT_NAME         TEXT("EDMA_EVENT")
#define EDMA_ERROR_NAME         TEXT("EDMA_ERROR")
#define EDMA_CC_ERROR_NAME      TEXT("EDMA_CC_ERROR")
#define EDMA_MAX_EVENT_NAME_LEN 30


//------------------------------------------------------------------------------
//  Debug print

#ifndef PRINTMSG
#ifdef DEBUG
#define PRINTMSG    DEBUGMSG
#else
//#define PRINTMSG    // RETAILMSG
#define PRINTMSG     OALMSG
#endif
#endif


//------------------------------------------------------------------------------
//  Event table definitions

typedef struct
{
    HANDLE hEvent;                 /* Event indicated when a transfer completes        */
    HANDLE hError;                 /* Event indicated when an error occurs             */
    EDMA_TRANS_STATUS transStatus; /* Indicates status of last transfer on the channel */
} EDMA_DMA_EVENT;

typedef struct
{
    HANDLE hError;                 /* Event indicated when an error occurs             */
} EDMA_CC_ERROR_EVENT;

typedef struct
{
    unsigned char instanceId;
    unsigned char tcNum;
} EDMA_TC_ERROR_EVENT;

//------------------------------------------------------------------------------
//  EDMA context

typedef struct {
    // Handle to EDMA3 driver object
    EDMA3_DRV_Handle hEDMA;

    // Pointers to global configuration data
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    EDMA3_DRV_InstanceInitConfig *pInstanceConfig;

    // EDMA event tables
    EDMA_DMA_EVENT DMAEventTable[EDMA3_MAX_DMA_CH];
    EDMA_DMA_EVENT QDMAEventTable[EDMA3_MAX_QDMA_CH];
    EDMA_CC_ERROR_EVENT CCEventTable[EDMA3_NUM_CCERRORS];

    // EDMA IRQs and Sys Intrs
    DWORD dwEDMACCSysIntr;
    DWORD dwEDMACCErrSysIntr;
    DWORD dwEDMATCErrSysIntr[EDMA3_MAX_TCC];

    // Default IST thread priority
    DWORD dwEDMAIstThreadPriority;

    // Flag to exit ISTs
    BOOL intrThreadExit;

    // Interrupt events and thread handles
    HANDLE hEDMAIntrEventTransferComplete;
    HANDLE hEDMAIntrEventCCError;
    HANDLE hEDMAIntrEventTCError[EDMA3_MAX_TCC];
    HANDLE hEDMAIntrThreadTransferComplete;
    HANDLE hEDMAIntrThreadCCError;
    HANDLE hEDMAIntrThreadTCError[EDMA3_MAX_TCC];

    // OS thread protection
    CRITICAL_SECTION csEdma;

} EDMA_CONTEXT;


#ifdef __cplusplus
}
#endif

#endif // __EDMA3_DEV_H
