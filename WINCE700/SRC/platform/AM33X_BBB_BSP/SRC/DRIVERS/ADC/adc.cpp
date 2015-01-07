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
//  File: adc.c
//
//  This file implements device driver for am335x ADC.
//  Uses stream driver model
//  ADC is also used by the touch screen and battery implementation.
//	This driver should be loaded before ether of these.
//	This driver is responsable for setting up the ADC clock, the step
//	configs and delays for all channels.
//
#include "bsp.h"
#include "pm.h"
#include "oal_clock.h"
#include "ceddkex.h"
#include "am33x_tsc_adc.h"

#include <initguid.h>


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IST            DEBUGZONE(4)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"adc", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x00ff
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define ADC_DEVICE_COOKIE           'adcD'

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD cookie;
    DWORD priority256;
	HANDLE hWriteTouchSampleBufferQueue;
	HANDLE hTouchEvent;
    CRITICAL_SECTION csDevice;
	TSCADC_REGS *regs;
    DWORD clk_rate;
    DWORD enableWake;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    DWORD irq;
    DWORD dwSysintr;
    BOOL bIntrThreadExit;
    DWORD powerMask;
    CEDEVICE_POWER_STATE powerState;
    volatile DWORD bWakeFromSuspend;
	UINT32 sampleBuffer[8];
} Device_t;


//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE, 
        offset(Device_t, priority256),
        fieldsize(Device_t, priority256), (VOID*)80
    }, {
        L"EnableWake", PARAM_DWORD, FALSE, 
        offset(Device_t, enableWake),
        fieldsize(Device_t, enableWake), (VOID*)1
    }
};


static TouchSample_t touchSample;

//------------------------------------------------------------------------------

BOOL ADC_Deinit(DWORD context);
DWORD ADC_IntrThread(VOID *pContext);

//------------------------------------------------------------------------------
//
//  Function:  ADC_Init
//
//  Called by device manager to initialize device.
//
DWORD
ADC_Init(
    LPCTSTR szContext,
    LPCVOID pBusContext
    )
{
	DWORD rc = (DWORD)NULL;
	Device_t *pDevice = NULL;
    PHYSICAL_ADDRESS pa = { 0, 0 };
    DWORD clk_value;
    MSGQUEUEOPTIONS queueOptions;
	int i;
    
    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"+ADC_Init(%s, 0x%08x)\r\n", szContext, pBusContext));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_Init: Failed allocate ADC driver structure\r\n"));
        goto cleanUp;
    }

    memset(pDevice, 0, sizeof(Device_t));

    // Set Cookie
    pDevice->cookie = ADC_DEVICE_COOKIE;

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_Init: Failed read ADC driver registry parameters\r\n"));
        goto cleanUp;
    }

    InitializeCriticalSection(&(pDevice->csDevice));

	// Map ADC controller
    pa.LowPart = GetAddressByDevice(AM_DEVICE_ADC_TSC);
    pDevice->regs = (TSCADC_REGS *)MmMapIoSpace(pa, sizeof(TSCADC_REGS), FALSE);
    if (!pDevice->regs) 
	{
        DEBUGMSG(ZONE_ERROR,(L"ERROR: ADC_Init: "
            L"Cannot map TSCADC regs\r\n"
            ));
    	goto cleanUp;
    }

	ReleaseDevicePads(AM_DEVICE_ADC_TSC);
	
	// Request Pads for ADC
    if (!RequestDevicePads(AM_DEVICE_ADC_TSC))
    {
        DEBUGMSG(ZONE_ERROR,(L"ERROR: ADC_Init: "
                     L"Failed to request ADC pads\r\n"
                    ));
        goto cleanUp;
    }

    //  Request clock
    EnableDeviceClocks(AM_DEVICE_ADC_TSC, TRUE);

	pDevice->clk_rate = PrcmClockGetClockRate(SYS_CLK) * 1000000;
	DEBUGMSG(ZONE_INIT,   (L"clock rate is %d\r\\n", pDevice->clk_rate));
	
    clk_value = pDevice->clk_rate / ADC_CLK;
    if (clk_value < 7) {
    	DEBUGMSG(ZONE_ERROR,  (L"clock input less than min clock requirement\r\n"));
    	goto cleanUp;
    }

    // TSCADC_CLKDIV needs to be configured to the value minus 1
    pDevice->regs->adc_clkdiv = clk_value -1;

	// must be disabled to configure
	pDevice->regs->adc_ctrl &= ~TSCADC_CNTRLREG_ENABLE;

    if (IsDVIMode())
	{ // use all ADC channels as general purpose ADC
		pDevice->regs->adc_ctrl |= (TSCADC_CNTRLREG_STEPCONFIGWRT | TSCADC_CNTRLREG_STEPID);

		// use step cfg 9
		pDevice->regs->tsc_adc_step_cfg[8].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
			TSCADC_STEPCONFIG_FIFO0 | (0 << 19); // AN0
		pDevice->regs->tsc_adc_step_cfg[8].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

		// use step cfg 10
		pDevice->regs->tsc_adc_step_cfg[9].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
			TSCADC_STEPCONFIG_FIFO0 | (1 << 19); // AN1
		pDevice->regs->tsc_adc_step_cfg[9].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

		// use step cfg 11
		pDevice->regs->tsc_adc_step_cfg[10].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
			TSCADC_STEPCONFIG_FIFO0 | (2 << 19); // AN2
		pDevice->regs->tsc_adc_step_cfg[10].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

		// use step cfg 12
		pDevice->regs->tsc_adc_step_cfg[11].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
			TSCADC_STEPCONFIG_FIFO0 | (3 << 19); // AN3
		pDevice->regs->tsc_adc_step_cfg[11].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

		pDevice->regs->fifo0_threshold = 4;
		pDevice->regs->step_enable |= 0x0001e000;
	}
	else
	{ // use ADC channels AN1-AN3 as touch
		pDevice->regs->adc_ctrl |= (TSCADC_CNTRLREG_STEPCONFIGWRT |
    			TSCADC_CNTRLREG_TSCENB |
    			TSCADC_CNTRLREG_STEPID |
				TSCADC_CNTRLREG_HWPREEMPT |
				TSCADC_CNTRLREG_4WIRE);
		
		pDevice->regs->idle_config = (TSCADC_STEPCONFIG_YNN |
				TSCADC_STEPCONFIG_INM | TSCADC_STEPCONFIG_IDLE_INP |
				TSCADC_STEPCONFIG_YPN);

		// all touch data to fofo1
		for (i = 0; i < XSTEPS; i++) {
			pDevice->regs->tsc_adc_step_cfg[i].step_config = (TSCADC_STEPCONFIG_MODE_HWS_OS |
				TSCADC_STEPCONFIG_2SAMPLES_AVG | TSCADC_STEPCONFIG_FIFO1 |
				TSCADC_STEPCONFIG_XPP |
				TSCADC_STEPCONFIG_INP |
				TSCADC_STEPCONFIG_XNN);
			pDevice->regs->tsc_adc_step_cfg[i].step_delay= (TSCADC_STEPCONFIG_SAMPLEDLY | TSCADC_STEPCONFIG_OPENDLY);
		}

		for (i = XSTEPS; i < (XSTEPS + YSTEPS); i++) {
			pDevice->regs->tsc_adc_step_cfg[i].step_config = TSCADC_STEPCONFIG_MODE_HWS_OS |
				TSCADC_STEPCONFIG_2SAMPLES_AVG | TSCADC_STEPCONFIG_FIFO1 |
				TSCADC_STEPCONFIG_YNN |
				TSCADC_STEPCONFIG_INM |
				TSCADC_STEPCONFIG_YPP;
			pDevice->regs->tsc_adc_step_cfg[i].step_delay= (TSCADC_STEPCONFIG_SAMPLEDLY | TSCADC_STEPCONFIG_OPENDLY);
		}

		pDevice->regs->charge_stepcfg = (TSCADC_STEPCONFIG_XPP |
				TSCADC_STEPCONFIG_YNN |
				TSCADC_STEPCONFIG_RFP |
				TSCADC_STEPCHARGE_RFM |
				TSCADC_STEPCHARGE_INM | 
				TSCADC_STEPCHARGE_INP);

		pDevice->regs->charge_delay= TSCADC_STEPCHARGE_DELAY;

		pDevice->regs->fifo1_threshold = XSTEPS + YSTEPS - 1;
		pDevice->regs->irq_enable_set = TSCADC_IRQ_FIFO1_THRES;
		pDevice->regs->step_enable |= 0x00001FFF;
	}

	// setup AN4-7 as analog inputs
	// use step cfg 13
	pDevice->regs->tsc_adc_step_cfg[12].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
		TSCADC_STEPCONFIG_FIFO0 | (4 << 19); // AN4
	pDevice->regs->tsc_adc_step_cfg[12].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

	// use step cfg 14
	pDevice->regs->tsc_adc_step_cfg[13].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
		TSCADC_STEPCONFIG_FIFO0 | (5 << 19); // AN5
	pDevice->regs->tsc_adc_step_cfg[13].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

	// use step cfg 15
	pDevice->regs->tsc_adc_step_cfg[14].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
		TSCADC_STEPCONFIG_FIFO0 | (6 << 19); // AN6
	pDevice->regs->tsc_adc_step_cfg[14].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

	// use step cfg 16
	pDevice->regs->tsc_adc_step_cfg[15].step_config = TSCADC_STEPCONFIG_MODE_SW_OS | TSCADC_STEPCONFIG_16SAMPLES_AVG |
		TSCADC_STEPCONFIG_FIFO0 | (7 << 19); // AN7
	pDevice->regs->tsc_adc_step_cfg[15].step_delay = (UINT32)(TSCADC_STEPCONFIG_SAMPLEDLY << 24 | TSCADC_STEPCONFIG_OPENDLY);

	pDevice->regs->fifo0_threshold += 4;
	pDevice->regs->irq_enable_set = TSCADC_IRQ_FIFO0_THRES;

	pDevice->regs->adc_ctrl |= TSCADC_CNTRLREG_ENABLE;

    // Create queue for writing touch sample messages
    queueOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    queueOptions.dwFlags = MSGQUEUE_ALLOW_BROKEN;
    queueOptions.dwMaxMessages = 1;
    queueOptions.cbMaxMessage = sizeof(TouchSample_t);
    queueOptions.bReadAccess = FALSE;    // we need write-access to msgqueue

    pDevice->hWriteTouchSampleBufferQueue = CreateMsgQueue(TOUCH_SAMPLE_QUEUE, &queueOptions);
    if (pDevice->hWriteTouchSampleBufferQueue == NULL) {
        DEBUGMSG(ZONE_ERROR,(L"ERROR: ADC_Init: "
                     L"creating touch sample buffer queue\r\n"
                    ));
    	goto cleanUp;
    }

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR,(L"ERROR: ADC_Init: "
                     L"Failed create interrupt event\r\n"
                    ));
        goto cleanUp;
    }

	pDevice->irq = GetIrqByDevice(AM_DEVICE_ADC_TSC, NULL);
	
    if (KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,&pDevice->irq,sizeof(pDevice->irq),&pDevice->dwSysintr,sizeof(pDevice->dwSysintr),NULL)== FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_Init: unable to request a SYSINTR\r\n"));
        goto cleanUp;
    }

    // initialize interrupt
    if (InterruptInitialize(pDevice->dwSysintr,pDevice->hIntrEvent,NULL,0)==FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_Init: unable to initialize the interrupt\r\n"));
        goto cleanUp;
    }

    // Start interrupt service thread
    pDevice->bIntrThreadExit = FALSE;
    pDevice->hIntrThread = CreateThread(NULL, 0, ADC_IntrThread, pDevice, 0, NULL);
    if (!pDevice->hIntrThread)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: ADC_Init: Failed create interrupt thread\r\n"));
        goto cleanUp;
    }
    
    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256);

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0)
    {
        ADC_Deinit((DWORD)pDevice);
    }
    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADD_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
ADC_Deinit(
    DWORD context
    )
{
	BOOL rc = FALSE;
	Device_t *pDevice = (Device_t*)context;

	DEBUGMSG(ZONE_FUNCTION, (L"+ADC_Deinit(0x%08x)\r\n", context));

    // Signal stop to threads
    pDevice->bIntrThreadExit = TRUE;
        
    // Close interrupt thread
    if (pDevice->hIntrThread != NULL)
    {
        // Set event to wake it
        SetEvent(pDevice->hIntrEvent);
        // Wait until thread exits
        WaitForSingleObject(pDevice->hIntrThread, INFINITE);
        // Close handle
        CloseHandle(pDevice->hIntrThread);
    }

    // Close interrupt handle
    if (pDevice->hIntrEvent != NULL) 
        CloseHandle(pDevice->hIntrEvent);

    if (pDevice->regs != NULL){
        MmUnmapIoSpace((VOID*)pDevice->regs, sizeof(TSCADC_REGS));
    }

	DeleteCriticalSection(&pDevice->csDevice);

	if (pDevice->hWriteTouchSampleBufferQueue != NULL)
		CloseMsgQueue(pDevice->hWriteTouchSampleBufferQueue);

	// Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
ADC_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);
    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Close
//
//  This function closes the device context.
//
BOOL
ADC_Close(
    DWORD context
    )
{
    UNREFERENCED_PARAMETER(context);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_PowerUp
//
//  Called on resume of device.
//
void
ADC_PowerUp(
    DWORD context
    )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_PowerDown
//
//  Called on suspend of device.
//
void
ADC_PowerDown(
    DWORD context
    )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_IOControl
//
//  This function sends a command to a device.
//
BOOL
ADC_IOControl(
    DWORD context, 
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;
	DWORD bytesReturned;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+ADC_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));
        
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != ADC_DEVICE_COOKIE))
    {
        RETAILMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: Incorrect context parameter\r\n"));
        goto cleanUp;
    }
    
	switch (code)
	{
		case IOCTL_ADC_GETCHANNEL:
			if (pOutBuffer != NULL && outSize == sizeof(DWORD) && 
				pInBuffer != NULL && inSize == sizeof(DWORD))
			{
				int index = *((int *)pInBuffer);
				if (index < 0 || index > 7)
				{
					rc = ERROR_INVALID_PARAMETER;
					break;
				}
				if (!CeSafeCopyMemory(pOutBuffer, &pDevice->sampleBuffer[index], sizeof(DWORD)))
				{
					rc = ERROR_INVALID_PARAMETER;
					break;
				}
				*pOutSize = sizeof(DWORD);
				rc = TRUE;
			}
		break;
		case IOCTL_ADC_SCANCHANNEL:
			if (pInBuffer != NULL && inSize == sizeof(DWORD))
			{
				int index = *((int *)pInBuffer);
				if (index < 0 || index > 7)
				{
					rc = ERROR_INVALID_PARAMETER;
					break;
				}
				EnterCriticalSection(&pDevice->csDevice);
				pDevice->regs->fifo0_threshold = 1;
				pDevice->regs->step_enable |= (0x200<<index); 
				LeaveCriticalSection(&pDevice->csDevice);
				rc = TRUE;
			}
		break;
		case IOCTL_ADC_AVAILABLECHANNELS:
			if (pOutBuffer != NULL && outSize == sizeof(DWORD))
			{
				if (IsDVIMode())
				{
					*pOutBuffer = 0x000000FF;
				}
				else
				{
					*pOutBuffer = 0x000000F0;
				}
				*pOutSize = sizeof(DWORD);
				rc = TRUE;
			}
		break;
	}

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_IOControl(rc = %d)\r\n", rc));
	return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_InterruptThread
//
//  This function acts as the IST for the ADC.
//
//
DWORD ADC_IntrThread(VOID *pContext)
{
	UINT32 irqStatus,sample,channel; 
	Device_t *pDevice = (Device_t*)pContext;
	INT32 fifo0_count, fifo1_count;
    
    DEBUGMSG(ZONE_IST, (L"+ADC_IntrThread\r\n"));

    // Loop until we are stopped...
    while (!pDevice->bIntrThreadExit)
    {
        // Wait for ADC interrupt
        WaitForSingleObject(pDevice->hIntrEvent, INFINITE);
		if (pDevice->bIntrThreadExit) break;
        
		DEBUGMSG(ZONE_IST, (L"+ADC_IntrThread interrupt!\r\n"));

		EnterCriticalSection(&pDevice->csDevice);
		irqStatus = pDevice->regs->irq_status;
		if (irqStatus & TSCADC_IRQ_FIFO0_THRES)
		{
			fifo0_count = pDevice->regs->fifo0_count;

			for (int z=0;z<fifo0_count;z++)
			{
				sample = pDevice->regs->fifo0_data;
				channel = (sample & 0x000f0000) >> 16;
				pDevice->sampleBuffer[channel-8] = sample & 0x00000fff;
			}
			pDevice->regs->irq_status = TSCADC_IRQ_FIFO0_THRES;
		}

		if (irqStatus & TSCADC_IRQ_FIFO1_THRES)
		{
			fifo1_count = pDevice->regs->fifo1_count;

			if (fifo1_count == XSTEPS+YSTEPS)
			{	// unload all touch data
				for (int x=0;x<XSTEPS;x++)
					touchSample.bufferX[x] = pDevice->regs->fifo1_data;
				for (int y=0;y<YSTEPS;y++)
					touchSample.bufferY[y] = pDevice->regs->fifo1_data;
				WriteMsgQueue(pDevice->hWriteTouchSampleBufferQueue,&touchSample,sizeof(touchSample),0,0);
			}
			pDevice->regs->irq_status = TSCADC_IRQ_FIFO1_THRES;
			pDevice->regs->step_enable |= 0x1fff; 
		}

		pDevice->regs->irq_eoi = 0x0;
		LeaveCriticalSection(&pDevice->csDevice);

		InterruptDone(pDevice->dwSysintr);
    }

    DEBUGMSG(ZONE_IST, (L"-ADC_IntrThread\r\n"));
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//------------------------------------------------------------------------------

