//
// Copyright (c) MPC-Data Limited 2009.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  edma.h
//
//  External API definition for the EDMA driver. Other drivers should include
//  this header in order to the use the EDMA driver.
//
//  Usage of the EDMA3 driver:
//
//    #include "edma.h"
//    EDMA3_DRV_Handle hEdma;
//    EDMA3_DRV_Result result;
//    hEdma = EDMA3_DRV_getInstHandle(EDMA3_DEFAULT_PHY_CTRL_INSTANCE,
//                                    EDMA3_ARM_REGION_ID, &result);
//    if (hEdma == NULL || result != EDMA3_DRV_SOK)
//    {
//        // Handle failure
//    }
//    else
//    {
//        // Call EDMA3 APIs as defined in edma3_drv.h (see use 
//        // cases at: -# EDMA3 driver APIs).
//
//        EDMA3_DRV_releaseInstHandle(hEdma);
//    }
//
#ifndef __EDMA_H
#define __EDMA_H

#include "edma3_drv.h"   // Most of the API is defined in here

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Definitions for use with EDMA3_DRV_getInstHandle()

#define EDMA3_DEFAULT_PHY_CTRL_INSTANCE  0
#define EDMA3_ARM_REGION_ID              ((EDMA3_RM_RegionId)0)


//------------------------------------------------------------------------------
// Transfer completion status.  Note that these are bit numbers, a transfer may 
// have completed and have a missed event.

typedef enum
{
    EDMA_STAT_INVALID_CHANNEL_NUM   = 0,
    EDMA_STAT_TRANSFER_COMPLETE     = 1,
    EDMA_STAT_EVENT_MISSED          = 2
} EDMA_TRANS_STATUS;


//------------------------------------------------------------------------------
//  Prototypes

// Read and reset the transfer status for an EDMA channel.
EDMA_TRANS_STATUS EDMA3_DRV_getTransferStatus(EDMA3_DRV_Handle, UINT32 edmaChannel);

//  Resets the CC error events.
void EDMA3_DRV_resetCCErrors(unsigned int instanceId);

// Returns the transfer complete event for an EDMA channel.
// The event handle should be closed when finised with.
HANDLE EDMA3_DRV_getTransferEvent(EDMA3_DRV_Handle hEdma,
                                  unsigned int edmaChannel);

// Returns the error event for an EDMA channel.
// The event handle should be closed when finised with.
HANDLE EDMA3_DRV_getErrorEvent(EDMA3_DRV_Handle hEdma,
                               unsigned int edmaChannel);

// Returns the CC error event.
// The event handle should be closed when finised with.
HANDLE EDMA3_DRV_getCCErrorEvent(EDMA3_DRV_Handle hEdma,
                                 unsigned int instanceId);

// Releases an EDMA3 handle previous obtained via EDMA3_DRV_getInstHandle().
void EDMA3_DRV_releaseInstHandle(EDMA3_DRV_Handle hEdma);


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __EDMA_H
