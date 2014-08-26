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
//  DMA helper routines.
//
#ifndef __EDMA_DEVICE_MAP_H
#define __EDMA_DEVICE_MAP_H

#include "edma3_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// hardware directed mapped events 
#define EDMA_REQ_PRU_HOST7				EDMA3_DRV_HW_CHANNEL_EVENT_0
#define EDMA_REQ_PRU_HOST6				EDMA3_DRV_HW_CHANNEL_EVENT_1
#define EDMA_REQ_MMC2_TX 				EDMA3_DRV_HW_CHANNEL_EVENT_2
#define EDMA_REQ_MMC2_RX 				EDMA3_DRV_HW_CHANNEL_EVENT_3
#define EDMA_REQ_AES_TX 				EDMA3_DRV_HW_CHANNEL_EVENT_2
#define EDMA_REQ_AES_CTX				EDMA3_DRV_HW_CHANNEL_EVENT_4
#define EDMA_REQ_AES_DATA_IN			EDMA3_DRV_HW_CHANNEL_EVENT_5
#define EDMA_REQ_AES_DATA_OUT			EDMA3_DRV_HW_CHANNEL_EVENT_6
#define EDMA_REQ_AES_CONTEXT_OUT		EDMA3_DRV_HW_CHANNEL_EVENT_7
#define EDMA_REQ_MCASP_AXEVT0			EDMA3_DRV_HW_CHANNEL_EVENT_8
#define EDMA_REQ_MCASP_AREVT0			EDMA3_DRV_HW_CHANNEL_EVENT_9
#define EDMA_REQ_MCASP_AXEVT1			EDMA3_DRV_HW_CHANNEL_EVENT_10
#define EDMA_REQ_MCASP_AREVT1			EDMA3_DRV_HW_CHANNEL_EVENT_11

#define EDMA_REQ_PWMEVT0				EDMA3_DRV_HW_CHANNEL_EVENT_14
#define EDMA_REQ_PWMEVT1				EDMA3_DRV_HW_CHANNEL_EVENT_15
#define EDMA_REQ_SPI1_TX0				EDMA3_DRV_HW_CHANNEL_EVENT_16
#define EDMA_REQ_SPI1_RX0				EDMA3_DRV_HW_CHANNEL_EVENT_17
#define EDMA_REQ_SPI1_TX1				EDMA3_DRV_HW_CHANNEL_EVENT_18
#define EDMA_REQ_SPI1_RX1				EDMA3_DRV_HW_CHANNEL_EVENT_19

#define EDMA_REQ_GPIO0       	        EDMA3_DRV_HW_CHANNEL_EVENT_22
#define EDMA_REQ_GPIO1               	EDMA3_DRV_HW_CHANNEL_EVENT_23
#define EDMA_REQ_MMC1_TX 				EDMA3_DRV_HW_CHANNEL_EVENT_24
#define EDMA_REQ_MMC1_RX				EDMA3_DRV_HW_CHANNEL_EVENT_25
#define EDMA_REQ_UART1_TX				EDMA3_DRV_HW_CHANNEL_EVENT_26
#define EDMA_REQ_UART1_RX				EDMA3_DRV_HW_CHANNEL_EVENT_27
#define EDMA_REQ_UART2_TX				EDMA3_DRV_HW_CHANNEL_EVENT_28
#define EDMA_REQ_UART2_RX				EDMA3_DRV_HW_CHANNEL_EVENT_29
#define EDMA_REQ_UART3_TX				EDMA3_DRV_HW_CHANNEL_EVENT_30
#define EDMA_REQ_UART3_RX				EDMA3_DRV_HW_CHANNEL_EVENT_31
#define EDMA_REQ_MMC3_TX 				EDMA3_DRV_HW_CHANNEL_EVENT_32	// crossbar enabled
#define EDMA_REQ_MMC3_RX 				EDMA3_DRV_HW_CHANNEL_EVENT_33  // crossbar enabled

#define EDMA_REQ_ECAP0					EDMA3_DRV_HW_CHANNEL_EVENT_38
#define EDMA_REQ_ECAP1					EDMA3_DRV_HW_CHANNEL_EVENT_39
#define EDMA_REQ_DCANIF1				EDMA3_DRV_HW_CHANNEL_EVENT_40
#define EDMA_REQ_DCANIF2				EDMA3_DRV_HW_CHANNEL_EVENT_41
#define EDMA_REQ_SPI2_TX0				EDMA3_DRV_HW_CHANNEL_EVENT_42
#define EDMA_REQ_SPI2_RX0				EDMA3_DRV_HW_CHANNEL_EVENT_43
#define EDMA_REQ_SPI2_TX1				EDMA3_DRV_HW_CHANNEL_EVENT_44
#define EDMA_REQ_SPI2_RX1				EDMA3_DRV_HW_CHANNEL_EVENT_45
#define EDMA_REQ_EQEPEVT0				EDMA3_DRV_HW_CHANNEL_EVENT_46
#define EDMA_REQ_DCANIF3				EDMA3_DRV_HW_CHANNEL_EVENT_47
#define EDMA_REQ_TIMER4					EDMA3_DRV_HW_CHANNEL_EVENT_48
#define EDMA_REQ_TIMER5					EDMA3_DRV_HW_CHANNEL_EVENT_49
#define EDMA_REQ_TIMER6					EDMA3_DRV_HW_CHANNEL_EVENT_50
#define EDMA_REQ_TIMER7					EDMA3_DRV_HW_CHANNEL_EVENT_51
#define EDMA_REQ_GPMC					EDMA3_DRV_HW_CHANNEL_EVENT_52
#define EDMA_REQ_TSCADC_FIFO0			EDMA3_DRV_HW_CHANNEL_EVENT_53

#define EDMA_REQ_EQEPEVT1				EDMA3_DRV_HW_CHANNEL_EVENT_56
#define EDMA_REQ_TSCADC_FIFO1			EDMA3_DRV_HW_CHANNEL_EVENT_57
#define EDMA_REQ_I2C1_TX				EDMA3_DRV_HW_CHANNEL_EVENT_58
#define EDMA_REQ_I2C1_RX				EDMA3_DRV_HW_CHANNEL_EVENT_59
#define EDMA_REQ_I2C2_TX				EDMA3_DRV_HW_CHANNEL_EVENT_60
#define EDMA_REQ_I2C2_RX				EDMA3_DRV_HW_CHANNEL_EVENT_61
#define EDMA_REQ_ECAP2					EDMA3_DRV_HW_CHANNEL_EVENT_62
#define EDMA_REQ_EHRPWMEVT2				EDMA3_DRV_HW_CHANNEL_EVENT_63



// crossbar directed mapped events 
#define EDMA_XBAR_SDTXEVT2				1
#define EDMA_XBAR_SDRXEVT2				2
#define EDMA_XBAR_I2CTXEVT2				3
#define EDMA_XBAR_I2CRXEVT2				4

#define EDMA_XBAR_UTXEVT3				7
#define EDMA_XBAR_URXEVT3				8
#define EDMA_XBAR_UTXEVT4				9
#define EDMA_XBAR_URXEVT4				10
#define EDMA_XBAR_UTXEVT5				11
#define EDMA_XBAR_URXEVT5				12
#define EDMA_XBAR_CAN_IF1DMA			13
#define EDMA_XBAR_CAN_IF2DMA			14
#define EDMA_XBAR_CAN_IF3DMA			15

#define EDMA_XBAR_TINT0					22

#define EDMA_XBAR_TINT2					24
#define EDMA_XBAR_TINT3					25

#define EDMA_XBAR_pi_x_dma_event_intr0	28
#define EDMA_XBAR_pi_x_dma_event_intr1	29
#define EDMA_XBAR_pi_x_dma_event_intr2	30
#define EDMA_XBAR_eQEPEVT2				31
#define EDMA_XBAR_GPIOEVT2				32


//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //__EDMA_DEVICE_MAP_H

