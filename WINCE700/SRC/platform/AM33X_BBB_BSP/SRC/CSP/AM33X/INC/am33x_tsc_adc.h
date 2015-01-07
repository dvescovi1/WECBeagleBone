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
//  Header:  am33x_tsc_adc.h
//
#ifndef __AM3XX_TSC_ADC_H
#define __AM3XX_TSC_ADC_H

#pragma warning(push)
#pragma warning(disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>
#pragma warning(pop)

//------------------------------------------------------------------------------

typedef volatile struct {
	UINT32	step_config;		//0x4c
	UINT32	step_delay;			//0x50
}TSC_ADC_STEP_CFG;

typedef volatile struct {
	UINT32	revision;			//0x0
	UINT32 resv1[3];
	UINT32	sysconfig;			//0x10
	UINT32 resv2[3];
	UINT32	irq_eoi;			//0x20
	UINT32	irq_status_raw;		//0x24
	UINT32	irq_status;			//0x28
	UINT32	irq_enable_set;		//0x2c
	UINT32	irq_enable_clr;		//0x30
	UINT32	irq_wakeup;			//0x34
	UINT32	dma_enable_set;		//0x38
	UINT32	dma_enable_clr;		//0x3c
	UINT32	adc_ctrl;			//0x40
	UINT32	adc_stat;			//0x44
	UINT32	adc_range;			//0x48
	UINT32	adc_clkdiv;			//0x4c
	UINT32	adc_misc;			//0x50
	UINT32	step_enable;		//0x54
	UINT32	idle_config;		//0x58
	UINT32	charge_stepcfg;		//0x5c
	UINT32	charge_delay;		//0x60
    TSC_ADC_STEP_CFG	tsc_adc_step_cfg[16];
	UINT32	fifo0_count;		//0xe4	
	UINT32	fifo0_threshold;	//0xe8
	UINT32	dma0_req;			//0xec
	UINT32	fifo1_count;		//0xf0	
	UINT32	fifo1_threshold;	//0xf4
	UINT32	dma1_req;			//0xf8
	UINT32 resv3;				//0xfc
	UINT32	fifo0_data;			//0x100
	UINT32 resv4[63];
	UINT32	fifo1_data;			//0x200
} TSCADC_REGS;


/* IRQxxx */
#define TSCADC_IRQ_PEN_EVENT_ASYNC		(1 << 0)
#define TSCADC_IRQ_EOS					(1 << 1)
#define TSCADC_IRQ_FIFO0_THRES			(1 << 2)
#define TSCADC_IRQ_FIFO0_OVERFLOW		(1 << 3)
#define TSCADC_IRQ_FIFO0_UNDERFLOW		(1 << 4)
#define TSCADC_IRQ_FIFO1_THRES			(1 << 5)
#define TSCADC_IRQ_FIFO1_OVERFLOW		(1 << 6)
#define TSCADC_IRQ_FIFO1_UNDERFLOW		(1 << 7)
#define TSCADC_IRQ_OUT_OF_RANGE			(1 << 8)
#define TSCADC_IRQ_PENUP				(1 << 9)
#define TSCADC_IRQ_PENIRQ_SYNC			(1 << 10)

/* IRQWAKEUP */
#define TSCADC_IRQWKUP_ENB				(1 << 0)

/* step config */
#define TSCADC_STEPCONFIG_MODE_SW_OS	(0 << 0)
#define TSCADC_STEPCONFIG_MODE_SW_CN	(1 << 0)
#define TSCADC_STEPCONFIG_MODE_HWS_OS	(2 << 0)
#define TSCADC_STEPCONFIG_MODE_HWS_CN	(3 << 0)
#define TSCADC_STEPCONFIG_0SAMPLES_AVG	(0 << 2)
#define TSCADC_STEPCONFIG_2SAMPLES_AVG	(1 << 2)
#define TSCADC_STEPCONFIG_4SAMPLES_AVG	(2 << 2)
#define TSCADC_STEPCONFIG_8SAMPLES_AVG	(3 << 2)
#define TSCADC_STEPCONFIG_16SAMPLES_AVG	(4 << 2)
#define TSCADC_STEPCONFIG_XPP			(1 << 5)
#define TSCADC_STEPCONFIG_XNN			(1 << 6)
#define TSCADC_STEPCONFIG_YPP			(1 << 7)
#define TSCADC_STEPCONFIG_YNN			(1 << 8)
#define TSCADC_STEPCONFIG_XNP			(1 << 9)
#define TSCADC_STEPCONFIG_YPN			(1 << 10)
#define TSCADC_STEPCONFIG_WPN			(1 << 11)
#define TSCADC_STEPCONFIG_RFP			(1 << 12)

#define TSCADC_STEPCONFIG_INM			(1 << 18)
#define TSCADC_STEPCONFIG_IDLE_INP		(1 << 22)
#define TSCADC_STEPCONFIG_INP_4			(1 << 19)
#define TSCADC_STEPCONFIG_INP			(1 << 20)
#define TSCADC_STEPCONFIG_INP_5			(1 << 21)
#define TSCADC_STEPCONFIG_INP_8_X		(3 << 20)
#define TSCADC_STEPCONFIG_INP_8_Y		(1 << 21)

#define TSCADC_STEPCONFIG_RFM_Y			(1 << 24)
#define TSCADC_STEPCONFIG_RFM_5_X		(1 << 24)
#define TSCADC_STEPCONFIG_RFM_4_X		(1 << 23)
#define TSCADC_STEPCONFIG_RFM_8_X		(1 << 23)

#define TSCADC_STEPCONFIG_DIFF_CNTRL	(0 << 25)
#define TSCADC_STEPCONFIG_FIFO0			(0 << 26)
#define TSCADC_STEPCONFIG_FIFO1			(1 << 26)
#define TSCADC_STEPCONFIG_RANGE_CHK		(1 << 27)

/* step charge */
#define TSCADC_STEPCHARGE_INM			(1 << 15)
#define TSCADC_STEPCHARGE_INM_SWAP		(1 << 16)
#define TSCADC_STEPCHARGE_INP			(1 << 19)
#define TSCADC_STEPCHARGE_INP_SWAP		(1 << 20)
#define TSCADC_STEPCHARGE_RFM			(1 << 23)
#define TSCADC_STEPCHARGE_DELAY			0x1

/* control*/
#define TSCADC_CNTRLREG_ENABLE			(1 << 0)
#define TSCADC_CNTRLREG_STEPID			(1 << 1)
#define TSCADC_CNTRLREG_STEPCONFIGWRT	(1 << 2)
#define TSCADC_CNTRLREG_TSCENB			(1 << 7)
#define TSCADC_CNTRLREG_HWEVENTMAPPING	(1 << 8)
#define TSCADC_CNTRLREG_HWPREEMPT		(1 << 9)
#define TSCADC_CNTRLREG_4WIRE			(0x1 << 5)
#define TSCADC_CNTRLREG_5WIRE			(0x1 << 6)
#define TSCADC_CNTRLREG_8WIRE			(0x3 << 5)

/* ADCSTAT */
#define TSCADC_ADCFSM_STEPID_IDLE		0x10
#define TSCADC_ADCFSM_FSM_BUSY			(1 << 5)
#define TSCADC_ADCFSM_FSM_IRQ1			(1 << 7)


#define TSCADC_STEPCONFIG_OPENDLY		(0x18)
#define TSCADC_STEPCONFIG_SAMPLEDLY		(0x88)//(0x88<<24)

// step enable
#define TSCADC_STPENB_TS_CHARGE			(1 << 0)
#define TSCADC_STPENB_STEP1				(1 << 1)
#define TSCADC_STPENB_STEP2				(1 << 2)
#define TSCADC_STPENB_STEP3				(1 << 3)
#define TSCADC_STPENB_STEP4				(1 << 4)
#define TSCADC_STPENB_STEP5				(1 << 5)
#define TSCADC_STPENB_STEP6				(1 << 6)
#define TSCADC_STPENB_STEP7				(1 << 7)
#define TSCADC_STPENB_STEP8				(1 << 8)
#define TSCADC_STPENB_STEP9				(1 << 9)
#define TSCADC_STPENB_STEP10			(1 << 10)
#define TSCADC_STPENB_STEP11			(1 << 11)
#define TSCADC_STPENB_STEP12			(1 << 12)
#define TSCADC_STPENB_STEP13			(1 << 13)
#define TSCADC_STPENB_STEP14			(1 << 14
#define TSCADC_STPENB_STEP15			(1 << 15
#define TSCADC_STPENB_STEP16			(1 << 16)

#define TSCADC_STPENB_ALL				(0x1ffff)

// ADC clock rate we will run at
#define ADC_CLK				3000000

#endif // __AM3XX_TSC_ADC_H
