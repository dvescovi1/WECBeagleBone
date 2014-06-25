//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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

#ifndef __TOUCHPDD_H
#define __TOUCHPDD_H

// undef here since these are already defined.
#undef ZONE_INIT
#undef ZONE_ERROR

#include <tchddsi.h>

//------------------------------------------------------------------------------
// Sample rate during polling.
#define DEFAULT_SAMPLE_RATE                 200               // Hz.
#define TOUCHPANEL_SAMPLE_RATE_LOW          DEFAULT_SAMPLE_RATE
#define TOUCHPANEL_SAMPLE_RATE_HIGH         DEFAULT_SAMPLE_RATE
#define DEFAULT_THREAD_PRIORITY             109

// Number of samples to discard when pen is initially down
#define DEFAULT_INITIAL_SAMPLES_DROPPED    1

#define RK_HARDWARE_DEVICEMAP_TOUCH     (TEXT("HARDWARE\\DEVICEMAP\\TOUCH"))
#define RV_CALIBRATION_DATA             (TEXT("CalibrationData"))

#define DIFF_X_MAX            10
#define DIFF_Y_MAX            10

#define FILTEREDSAMPLESIZE              3
#define SAMPLESIZE                      1

#define DEFAULT_DEVICE_ADDRESS          0 //CS0 of SPI1

#define COMMAND_EN_TOUCH                  0x800000
#define COMMAND_XPOS                      0xD00000
#define COMMAND_YPOS                      0x900000
#define POWER_MASK                        0x010000

#define MAX_PTS                     3
#define DELTA                       30

#define CAL_DELTA_RESET             20
#define CAL_HOLD_STEADY_TIME        1500
#define RANGE_MIN                   0
#define RANGE_MAX                   4096

//------------------------------------------------------------------------------
// local data structures
//

typedef struct {
	volatile UINT32	step_config;		//0x4c
	volatile UINT32	step_delay;			//0x50
}TSC_ADC_STEP_CFG;

typedef struct {
	volatile UINT32	revision;			//0x0
	volatile UINT32 resv1[3];
	volatile UINT32	sysconfig;			//0x10
	volatile UINT32 resv2[3];
	volatile UINT32	irq_eoi;			//0x20
	volatile UINT32 irq_status_raw;		//0x24
	volatile UINT32	irq_status;			//0x28
	volatile UINT32	irq_enable_set;		//0x2c
	volatile UINT32	irq_enable_clr;		//0x30
	volatile UINT32	irq_wakeup;			//0x34
	volatile UINT32	dma_enable_set;		//0x38
	volatile UINT32	dma_enable_clr;		//0x3c
	volatile UINT32	adc_ctrl;			//0x40
	volatile UINT32	adc_stat;			//0x44
	volatile UINT32	adc_range;			//0x48
	volatile UINT32	adc_clkdiv;			//0x4c
	volatile UINT32	adc_misc;			//0x50
	volatile UINT32	step_enable;		//0x54
	volatile UINT32	idle_config;		//0x58
	volatile UINT32	charge_stepcfg;		//0x5c
	volatile UINT32	charge_delay;		//0x60
    TSC_ADC_STEP_CFG	tsc_adc_step_cfg[16];
	volatile UINT32	fifo0_count;		//0xe4	
	volatile UINT32	fifo0_threshold;	//0xe8
	volatile UINT32	dma0_req;			//0xec
	volatile UINT32	fifo1_count;		//0xf0	
	volatile UINT32	fifo1_threshold;	//0xf4
	volatile UINT32	dma1_req;			//0xf8
	volatile UINT32 resv3;				//0xfc
	volatile UINT32	fifo0_data;			//0x100
	volatile UINT32 resv4[63];
	volatile UINT32	fifo1_data;			//0x200
} TSCADC_REGS;

/*	Register Bitfields	*/
#define TSCADC_IRQWKUP_ENB				(1 << 0)
#define TSCADC_STPENB_STEPENB			(0x1FFF)
#define TSCADC_IRQENB_FIFO0THRES		(1 << 2)
#define TSCADC_IRQENB_FIFO1THRES		(1 << 5)
#define TSCADC_IRQENB_PENUP				(1 << 9)
#define TSCADC_IRQENB_IRQHWPEN			(1 << 10)
#define TSCADC_IRQENB_IRQEOS			(1 << 1)
#define TSCADC_IRQENB_FIFO_OVERFLOW		(1 << 3)
#define TSCADC_IRQENB_PEN_EVENT_SYNC	(1 << 0)

/* step config */
#define TSCADC_STEPCONFIG_MODE_HWSYNC	0x2
#define TSCADC_STEPCONFIG_2SAMPLES_AVG	(1 << 4)
#define TSCADC_STEPCONFIG_XPP			(1 << 5)
#define TSCADC_STEPCONFIG_XNN			(1 << 6)
#define TSCADC_STEPCONFIG_YPP			(1 << 7)
#define TSCADC_STEPCONFIG_YNN			(1 << 8)
#define TSCADC_STEPCONFIG_XNP			(1 << 9)
#define TSCADC_STEPCONFIG_YPN			(1 << 10)
#define TSCADC_STEPCONFIG_RFP			(1 << 12)
#define TSCADC_STEPCONFIG_INM			(1 << 18)
#define TSCADC_STEPCONFIG_INP_4			(1 << 19)
#define TSCADC_STEPCONFIG_INP			(1 << 20)
#define TSCADC_STEPCONFIG_INP_5			(1 << 21)
#define TSCADC_STEPCONFIG_INP_8_X		(3 << 20)
#define TSCADC_STEPCONFIG_INP_8_Y		(1 << 21)
#define TSCADC_STEPCONFIG_RFM_4_X		(1 << 23)
#define TSCADC_STEPCONFIG_RFM_5_X		(1 << 24)
#define TSCADC_STEPCONFIG_RFM_8_X		(1 << 23)
#define TSCADC_STEPCONFIG_RFM_Y			(1 << 24)
#define TSCADC_STEPCONFIG_OPENDLY		(0x18)
#define TSCADC_STEPCONFIG_SAMPLEDLY		(0x88)//(0x88<<24)
#define TSCADC_STEPCONFIG_FIFO1			(1 << 26)
#define TSCADC_STEPCONFIG_IDLE_INP		(1 << 22)
/* step charge */
#define TSCADC_STEPCHARGE_INM			(1 << 15)
#define TSCADC_STEPCHARGE_INM_SWAP		(1 << 16)
#define TSCADC_STEPCHARGE_INP			(1 << 19)
#define TSCADC_STEPCHARGE_INP_SWAP		(1 << 20)
#define TSCADC_STEPCHARGE_RFM			(1 << 23)
#define TSCADC_STEPCHARGE_DELAY			0x1
/* contrl*/
#define TSCADC_CNTRLREG_TSCSSENB		(1 << 0)
#define TSCADC_CNTRLREG_STEPID			(1 << 1)
#define TSCADC_CNTRLREG_STEPCONFIGWRT	(1 << 2)
#define TSCADC_CNTRLREG_TSCENB			(1 << 7)
#define TSCADC_CNTRLREG_4WIRE			(0x1 << 5)
#define TSCADC_CNTRLREG_5WIRE			(0x1 << 6)
#define TSCADC_CNTRLREG_8WIRE			(0x3 << 5)

/* ADCSTAT */
#define TSCADC_ADCFSM_STEPID_IDLE		0x10
#define TSCADC_ADCFSM_FSM_BUSY			(1 << 5)
#define TSCADC_ADCFSM_FSM_IRQ1			(1 << 7)

#define ADC_CLK				3000000
#define MAX_12BIT                       ((1 << 12) - 1)
#define XSTEPS    6
#define YSTEPS    6


//------------------------------------------------------------------------------
// local data structures
//

typedef struct
{
    BOOL        bInitialized;

    TSCADC_REGS *regs;
    DWORD       nSampleRate;
    DWORD       nInitialSamplesDropped;
    LONG        nPenGPIO;
    DWORD       PenUpDebounceMS;
    LONG        nSPIAddr;
    LONG        nSPIBaudrate;
    LONG        nSPIWordlength;

    DWORD       dwSysIntr;
    DWORD       dwSamplingTimeOut;
    BOOL        bTerminateIST;
    HANDLE      hTouchPanelEvent;
    DWORD       dwPowerState;
    HANDLE      hIST;
    HANDLE      hGPIO;
    HANDLE      hSPI;
    LONG        nPenIRQ;
    DWORD       dwISTPriority;
    DWORD       clk_rate;
    DWORD       dwWires;
	DWORD 		analog_input;
}TOUCH_DEVICE;


//------------------------------------------------------------------------------
//  Device registry parameters
static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"SampleRate", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, nSampleRate),
        fieldsize(TOUCH_DEVICE, nSampleRate), (VOID*)DEFAULT_SAMPLE_RATE
    },
    {
        L"Priority256", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, dwISTPriority),
        fieldsize(TOUCH_DEVICE, dwISTPriority), (VOID*)DEFAULT_THREAD_PRIORITY
    },
    {
        L"SysIntr", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, dwSysIntr),
        fieldsize(TOUCH_DEVICE, dwSysIntr), (VOID*)SYSINTR_NOP
    },
    {
        L"Wires", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, dwWires),
        fieldsize(TOUCH_DEVICE, dwWires), (VOID*)4
    },
	{
		L"AnalogInput", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, analog_input),
		fieldsize(TOUCH_DEVICE, analog_input), (VOID*)1
	},
};

//------------------------------------------------------------------------------
// global variables
//
static TOUCH_DEVICE s_TouchDevice =  {
    FALSE,                                          //bInitialized
    NULL,                                           //regs
    DEFAULT_SAMPLE_RATE,                            //nSampleRate
    0,                                              //nInitialSamplesDropped
    0,                                              //nPenGPIO
    0,                                              //PenUpDebounceMS
    0,                                              //nSPIAddr
    0,                                              //nSPIBaudrate
    0,                                              //nSPIWordlength
    SYSINTR_NOP,                                    //dwSysIntr
    0,                                              //dwSamplingTimeOut
    FALSE,                                          //bTerminateIST
    0,                                              //hTouchPanelEvent
    D0,                                             //dwPowerState
    0,                                              //hIST
    0,                                              //hGPIO
    0,                                              //hSPI
    0,                                              //nPenIRQ
    DEFAULT_THREAD_PRIORITY                         //dwISTPriority
};

// Internal functions.
static HRESULT PDDCalibrationThread();
void PDDStartCalibrationThread();

BOOL PDDGetTouchIntPinState( VOID );
BOOL PDDGetTouchData(UINT32 * pxPos, UINT32 * pyPos);
BOOL PDDGetRegistrySettings( PDWORD );
BOOL PDDInitializeHardware(LPCTSTR pszActiveKey );
VOID PDDDeinitializeHardware( VOID );
VOID  PDDTouchPanelDisable();
BOOL  PDDTouchPanelEnable();
ULONG PDDTouchIST(PVOID   reserved);
void PDDTouchPanelPowerHandler(BOOL boff);

//TCH PDD DDSI functions
extern "C" DWORD WINAPI TchPdd_Init(
    LPCTSTR pszActiveKey,
    TCH_MDD_INTERFACE_INFO* pMddIfc,
    TCH_PDD_INTERFACE_INFO* pPddIfc,
    DWORD hMddContext
    );

void WINAPI TchPdd_Deinit(DWORD hPddContext);
void WINAPI TchPdd_PowerUp(DWORD hPddContext);
void WINAPI TchPdd_PowerDown(DWORD hPddContext);
BOOL WINAPI TchPdd_Ioctl(
    DWORD hPddContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

#endif
