//
// Copyright (c) MPC-Data Limited 2011.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  pru_drvr_api.h
//
//  External API definition for the PRU SUART driver. Other drivers should include
//  this header in order to the use the PRU SUART driver.
//
//  Usage of the PRU SUART driver:
//
//    #include "pru_drvr_api.h"
//    PRU_DRV_Handle hPru;
//    PRU_DRV_Result result;
//    UINT32 pruSuartId = 0;
//    hPru = PRU_DRV_getInstHandle( dwPruSuartId );
//    if ( hPru == NULL )
//    {
//        // Handle failure
//    }
//    else
//    {
//        // Call PRU APIs as defined in pru_drv_api.h (see use 
//        // cases at: -# PRU driver APIs).
//
//        PRU_DRV_releaseInstHandle( hPru );
//    }
//
#ifndef __PRU_DRVR_API_H
#define __PRU_DRVR_API_H


#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
// Common definitions

#define PRU_DRVR_IRQ_FLAG_TX		1
#define PRU_DRVR_IRQ_FLAG_RX		2
#define PRU_DRVR_IRQ_FLAG_ERR		4

// These status codes match those defined in PRU land
#define PRU_DRVR_STATUS_TIMEOUT		(1 << 6)
#define PRU_DRVR_STATUS_BI			(1 << 5)
#define PRU_DRVR_STATUS_FE			(1 << 4)
#define PRU_DRVR_STATUS_OVRNERR		(1 << 3)
#define PRU_DRVR_STATUS_ERR			(1 << 2)
#define PRU_DRVR_STATUS_CMPLT		(1 << 1)
#define PRU_DRVR_STATUS_RDY			(1 << 0)


//------------------------------------------------------------------------------
// Definitions for use with PRU_DRV_getInstHandle()

/*
 * PRU Driver Handle.
 * It will be returned from PRU_DRV_open() and will be used to call
 * other PRU Driver APIs.
 */
typedef void    *PRU_DRV_Handle;


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __PRU_DRVR_API_H
