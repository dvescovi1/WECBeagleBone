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
//  DMA Channels assigned to different Hardware Events.

typedef enum
{
    // Channel Controller 0 events
    EDMA3_DRV_HW_CHANNEL_McASP0RXEVT_CH0 = 0,
    EDMA3_DRV_HW_CHANNEL_McASP0TXEVT_CH1 = 1u,
    EDMA3_DRV_HW_CHANNEL_McBSP0RXEVT_CH2 = 2u, // L138 only
    EDMA3_DRV_HW_CHANNEL_McASP1RXEVT_CH2 = 2u, // L137 only
    EDMA3_DRV_HW_CHANNEL_McBSP0TXEVT_CH3 = 3u, // L138 only
    EDMA3_DRV_HW_CHANNEL_McASP1TXEVT_CH3 = 3u, // L137 only
    EDMA3_DRV_HW_CHANNEL_McBSP1RXEVT_CH4 = 4u, // L138 only
    EDMA3_DRV_HW_CHANNEL_McASP2RXEVT_CH4 = 4u, // L137 only
    EDMA3_DRV_HW_CHANNEL_McBSP1TXEVT_CH5 = 5u, // L138 only
    EDMA3_DRV_HW_CHANNEL_McASP2TXEVT_CH5 = 5u, // L137 only
    EDMA3_DRV_HW_CHANNEL_GPBNKINT0EVT_CH6 = 6u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT1EVT_CH7 = 7u,
    EDMA3_DRV_HW_CHANNEL_UART0RXEVT_CH8 = 8u,
    EDMA3_DRV_HW_CHANNEL_UART0TXEVT_CH9 = 9u,
    EDMA3_DRV_HW_CHANNEL_TIM64P0OUT12EVT_CH10 = 10u,
    EDMA3_DRV_HW_CHANNEL_TIM64P0OUT34EVT_CH11 = 11u,
    EDMA3_DRV_HW_CHANNEL_UART1RXEVT_CH12 = 12u,
    EDMA3_DRV_HW_CHANNEL_UART1TXEVT_CH13 = 13u,
    EDMA3_DRV_HW_CHANNEL_SPI0RXEVT_CH14 = 14u,
    EDMA3_DRV_HW_CHANNEL_SPI0TXEVT_CH15 = 15u,
    EDMA3_DRV_HW_CHANNEL_MMCSD0RXEVT_CH16 = 16u,
    EDMA3_DRV_HW_CHANNEL_MMCSD0TXEVT_CH17 = 17u,
    EDMA3_DRV_HW_CHANNEL_SPI1RXEVT_CH18 = 18u,
    EDMA3_DRV_HW_CHANNEL_SPI1TXEVT_CH19 = 19u,
    EDMA3_DRV_HW_CHANNEL_PRUOUT6EVT_CH20 = 20u, // L138 only
    EDMA3_DRV_HW_CHANNEL_PRUOUT7EVT_CH21 = 21u, // L138 only
    EDMA3_DRV_HW_CHANNEL_GPBNKINT2EVT_CH22 = 22u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT3EVT_CH23 = 23u,
    EDMA3_DRV_HW_CHANNEL_I2C0RXEVT_CH24 = 24u,
    EDMA3_DRV_HW_CHANNEL_I2C0TXEVT_CH25 = 25u,
    EDMA3_DRV_HW_CHANNEL_I2C1RXEVT_CH26 = 26u,
    EDMA3_DRV_HW_CHANNEL_I2C1TXEVT_CH27 = 27u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT4EVT_CH28 = 28u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT5EVT_CH29 = 29u,
    EDMA3_DRV_HW_CHANNEL_UART2RXEVT_CH30 = 30u,
    EDMA3_DRV_HW_CHANNEL_UART2TXEVT_CH31 = 31u,
    
    // Channel Controller 1 events (L138 only)
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT0_CH0 = 0,    
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT1_CH1 = 1u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT2_CH2 = 2u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT3_CH3 = 3u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT4_CH4 = 4u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT5_CH5 = 5u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT6_CH6 = 6u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2CMPEVT7_CH7 = 7u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT0_CH8 = 8u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT1_CH9 = 9u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT2_CH10 = 10u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT3_CH11 = 11u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT4_CH12 = 12u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT5_CH13 = 13u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT6_CH14 = 14u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3CMPEVT7_CH15 = 15u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT6EVT_CH16 = 16u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT7EVT_CH17 = 17u,
    EDMA3_DRV_HW_CHANNEL_GPBNKINT8EVT_CH18 = 18u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2OUT12EVT_CH24 = 24u,
    EDMA3_DRV_HW_CHANNEL_TIM64P2OUT34EVT_CH25 = 25u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3OUT12EVT_CH26 = 26u,
    EDMA3_DRV_HW_CHANNEL_TIM64P3OUT34EVT_CH27 = 27u,
    EDMA3_DRV_HW_CHANNEL_MMCSD1RXEVT_CH28 = 28u,
    EDMA3_DRV_HW_CHANNEL_MMCSD1TXEVT_CH29 = 29u
} EDMA3_DRV_HW_CHANNEL_EVENT_DEFS;


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
