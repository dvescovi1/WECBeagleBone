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

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// hardware directed mapped events 
#define SDMA_REQ_PRU_HOST7				0
#define SDMA_REQ_PRU_HOST6				1
#define SDMA_REQ_MMC2_TX 				2
#define SDMA_REQ_MMC2_RX 				3
#define SDMA_REQ_AES_TX 				2
#define SDMA_REQ_MMC2_RX 				3
#define SDMA_REQ_AES_CTX				4
#define SDMA_REQ_AES_DATA_IN			5
#define SDMA_REQ_AES_DATA_OUT			6
#define SDMA_REQ_AES_CONTEXT_OUT		7
#define SDMA_REQ_MCASP_AXEVT0			8
#define SDMA_REQ_MCASP_AREVT0			9
#define SDMA_REQ_MCASP_AXEVT1			10
#define SDMA_REQ_MCASP_AREVT1			11

#define SDMA_REQ_PWMEVT0				14
#define SDMA_REQ_PWMEVT1				15
#define SDMA_REQ_SPI1_TX0               16
#define SDMA_REQ_SPI1_RX0               17
#define SDMA_REQ_SPI1_TX1               18
#define SDMA_REQ_SPI1_RX1               19

#define SDMA_REQ_GPIO0       	        22
#define SDMA_REQ_GPIO1               	23
#define SDMA_REQ_MMC1_TX 				24
#define SDMA_REQ_MMC1_RX				25
#define SDMA_REQ_UART1_TX				26
#define SDMA_REQ_UART1_RX				27
#define SDMA_REQ_UART2_TX				28
#define SDMA_REQ_UART2_RX				29
#define SDMA_REQ_UART3_TX				30
#define SDMA_REQ_UART3_RX				31

#define SDMA_REQ_SHA2_CTXIN				35
#define SDMA_REQ_SHA2_DIN				36
#define SDMA_REQ_SHA2_CTXOUT			37
#define SDMA_REQ_ECAP0					38
#define SDMA_REQ_ECAP1					39
#define SDMA_REQ_DCANIF1				40
#define SDMA_REQ_DCANIF2				41
#define SDMA_REQ_SPI2_TX0				42
#define SDMA_REQ_SPI2_RX0				43
#define SDMA_REQ_SPI2_TX1				44
#define SDMA_REQ_SPI2_RX1				45
#define SDMA_REQ_EQEPEVT0				46
#define SDMA_REQ_DCANIF3				47
#define SDMA_REQ_TIMER4					48
#define SDMA_REQ_TIMER5					49
#define SDMA_REQ_TIMER6					50
#define SDMA_REQ_TIMER7					51
#define SDMA_REQ_GPMC					52
#define SDMA_REQ_TSCADC_FIFO0			53

#define SDMA_REQ_EQEPEVT1				56
#define SDMA_REQ_TSCADC_FIFO1			57
#define SDMA_REQ_I2C1_TX				58
#define SDMA_REQ_I2C1_RX				59
#define SDMA_REQ_I2C2_TX				60
#define SDMA_REQ_I2C2_RX				61
#define SDMA_REQ_ECAP2					62
#define SDMA_REQ_EHRPWMEVT2				63


//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //__EDMA_DEVICE_MAP_H

