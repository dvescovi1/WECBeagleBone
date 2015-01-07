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

#pragma warning(push)
#pragma warning(disable: 4127 4201 28251 6054 6388)
//------------------------------------------------------------------------------
// Public
//
#include <windows.h>
#include <nkintr.h>
#include <creg.hxx>
#include <tchstream.h>
#include <tchstreamddsi.h>
#include <pm.h>

//------------------------------------------------------------------------------
// Platform
//
#include "omap.h"
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <initguid.h>
#include "touchscreenpdd.h"
#include "am33x_clocks.h"
#include "bsp_def.h"

//------------------------------------------------------------------------------
// Defines
//


//------------------------------------------------------------------------------
// Debug zones
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO
#undef ZONE_TIPSTATE

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_TIPSTATE       DEBUGZONE(4)

#endif


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
};

//------------------------------------------------------------------------------
// global variables
//
static TOUCH_DEVICE s_TouchDevice =  {
    FALSE,                                          //bInitialized
	NULL,
    NULL,                                           //regs
    DEFAULT_SAMPLE_RATE,                            //nSampleRate
    0,                                              //nInitialSamplesDropped
    0,                                              //PenUpDebounceMS
    SYSINTR_NOP,                                    //dwSysIntr
    0,                                              //dwSamplingTimeOut
    FALSE,                                          //bTerminateIST
    0,                                              //hTouchPanelEvent
    D0,                                             //dwPowerState
    0,                                              //hIST
    0,                                              //nPenIRQ
    DEFAULT_THREAD_PRIORITY                         //dwISTPriority
};

static TouchSample_t touchSample;
static DWORD s_mddContext;
static PFN_TCH_MDD_REPORTSAMPLESET    s_pfnMddReportSampleSet;

//==============================================================================
// Function Name: TchPdd_Init
//
// Description: PDD should always implement this function. MDD calls it during
//              initialization to fill up the function table with rest of the
//              Helper functions.
//
// Arguments:
//              [IN] pszActiveKey - current active touch driver key
//              [IN] pMddIfc - MDD interface info
//              [OUT]pPddIfc - PDD interface (the function table to be filled)
//              [IN] hMddContext - mdd context (send to MDD in callback fn)
//
// Ret Value:   pddContext.
//==============================================================================
extern "C" DWORD WINAPI TchPdd_Init(LPCTSTR pszActiveKey,
    TCH_MDD_INTERFACE_INFO* pMddIfc,
    TCH_PDD_INTERFACE_INFO* pPddIfc,
    DWORD hMddContext
    )
{
    DWORD pddContext = 0;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_Init+\r\n")));

    // Initialize once only
    if (s_TouchDevice.bInitialized)
    {
        pddContext = (DWORD) &s_TouchDevice;
        goto cleanUp;
    }

    // Remember the callback function pointer
    s_pfnMddReportSampleSet = pMddIfc->pfnMddReportSampleSet;

    // Remember the mdd context
    s_mddContext = hMddContext;

    s_TouchDevice.nSampleRate = DEFAULT_SAMPLE_RATE;
    s_TouchDevice.dwPowerState = D0;
    s_TouchDevice.dwSamplingTimeOut = INFINITE;
    s_TouchDevice.bTerminateIST = FALSE;
    s_TouchDevice.hTouchPanelEvent = NULL;

    // Initialize HW
    if (!PDDInitializeHardware(pszActiveKey))
    {
        DEBUGMSG(ZONE_ERROR,  (TEXT("ERROR: TOUCH: Failed to initialize touch PDD.\r\n")));
        goto cleanUp;
    }

    //Calibrate the screen, if the calibration data is not already preset in the registry
    PDDStartCalibrationThread();

    //Initialization of the h/w is done
    s_TouchDevice.bInitialized = TRUE;

    pddContext = (DWORD) &s_TouchDevice;

    // fill up pddifc table
    pPddIfc->version        = 1;
    pPddIfc->pfnDeinit      = TchPdd_Deinit;
    pPddIfc->pfnIoctl       = TchPdd_Ioctl;
    pPddIfc->pfnPowerDown   = TchPdd_PowerDown;
    pPddIfc->pfnPowerUp     = TchPdd_PowerUp;


cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_Init-\r\n")));
    return pddContext;
}


//==============================================================================
// Function Name: TchPdd_DeInit
//
// Description: MDD calls it during deinitialization. PDD should deinit hardware
//              and deallocate memory etc.
//
// Arguments:   [IN] hPddContext. pddcontext returned in TchPdd_Init
//
// Ret Value:   None
//
//==============================================================================
void WINAPI TchPdd_Deinit(DWORD hPddContext)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_Deinit+\r\n")));

    // Remove-W4: Warning C4100 workaround
    UNREFERENCED_PARAMETER(hPddContext);

    // Close the IST and release the resources before
    // de-initializing.
    PDDTouchPanelDisable();

    //  Shutdown HW
    PDDDeinitializeHardware();

    // Release interrupt
    if (s_TouchDevice.dwSysIntr != 0)
        {
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR,
            &s_TouchDevice.dwSysIntr,
            sizeof(s_TouchDevice.dwSysIntr),
            NULL,
            0,
            NULL
            );
        }

    s_TouchDevice.bInitialized = FALSE;


    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_Deinit-\r\n")));
}


//==============================================================================
// Function Name: TchPdd_Ioctl
//
// Description: The MDD controls the touch PDD through these IOCTLs.
//
// Arguments:   [IN] hPddContext. pddcontext returned in TchPdd_Init
//              DOWRD dwCode. IOCTL code
//              PBYTE pBufIn. Input Buffer pointer
//              DWORD dwLenIn. Input buffer length
//              PBYTE pBufOut. Output buffer pointer
//              DWORD dwLenOut. Output buffer length
//              PWORD pdwAcutalOut. Actual output buffer length.
//
// Ret Value:   TRUE if success else FALSE. SetLastError() if FALSE.
//==============================================================================
BOOL WINAPI TchPdd_Ioctl(DWORD hPddContext,
      DWORD dwCode,
      PBYTE pBufIn,
      DWORD dwLenIn,
      PBYTE pBufOut,
      DWORD dwLenOut,
      PDWORD pdwActualOut
)
{
    DWORD dwResult = ERROR_INVALID_PARAMETER;

    // Remove-W4: Warning C4100 workaround
    UNREFERENCED_PARAMETER(hPddContext);

    switch (dwCode)
    {
        //  Enable touch panel
        case IOCTL_TOUCH_ENABLE_TOUCHPANEL:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_TOUCH_ENABLE_TOUCHPANEL\r\n"));
            PDDTouchPanelEnable();
            dwResult = ERROR_SUCCESS;
            break;

        //  Disable touch panel
        case IOCTL_TOUCH_DISABLE_TOUCHPANEL:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_TOUCH_DISABLE_TOUCHPANEL\r\n"));
            PDDTouchPanelDisable();
            dwResult = ERROR_SUCCESS;
            break;


        //  Get current sample rate
        case IOCTL_TOUCH_GET_SAMPLE_RATE:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_TOUCH_GET_SAMPLE_RATE\r\n"));

            //  Check parameter validity
            if ((pBufOut != NULL) && (dwLenOut == sizeof(DWORD)))
            {
                if (pdwActualOut)
                    *pdwActualOut = sizeof(DWORD);

                //  Get the sample rate
                *((DWORD*)pBufOut) = s_TouchDevice.nSampleRate;
                dwResult = ERROR_SUCCESS;
            }
            break;

        //  Set the current sample rate
        case IOCTL_TOUCH_SET_SAMPLE_RATE:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_TOUCH_SET_SAMPLE_RATE\r\n"));

            //  Check parameter validity
            if ((pBufIn != NULL) && (dwLenIn == sizeof(DWORD)))
            {
                //  Set the sample rate
                s_TouchDevice.nSampleRate = *((DWORD*)pBufIn);
                dwResult = ERROR_SUCCESS;

            }
            break;

        //  Get touch properties
        case IOCTL_TOUCH_GET_TOUCH_PROPS:
            //  Check parameter validity
            if ((pBufOut != NULL) && (dwLenOut == sizeof(TCH_PROPS)))
            {
                if (pdwActualOut)
                    *pdwActualOut = sizeof(TCH_PROPS);\

                //  Fill out the touch driver properties
                ((TCH_PROPS*)pBufOut)->minSampleRate            = TOUCHPANEL_SAMPLE_RATE_LOW;
                ((TCH_PROPS*)pBufOut)->maxSampleRate            = TOUCHPANEL_SAMPLE_RATE_HIGH;
                ((TCH_PROPS*)pBufOut)->minCalCount              = 5;
                ((TCH_PROPS*)pBufOut)->maxSimultaneousSamples   = 1;
                ((TCH_PROPS*)pBufOut)->touchType                = TOUCHTYPE_SINGLETOUCH;
                ((TCH_PROPS*)pBufOut)->calHoldSteadyTime        = CAL_HOLD_STEADY_TIME;
                ((TCH_PROPS*)pBufOut)->calDeltaReset            = CAL_DELTA_RESET;
                ((TCH_PROPS*)pBufOut)->xRangeMin                = RANGE_MIN;
                ((TCH_PROPS*)pBufOut)->xRangeMax                = RANGE_MAX;
                ((TCH_PROPS*)pBufOut)->yRangeMin                = RANGE_MIN;
                ((TCH_PROPS*)pBufOut)->yRangeMax                = RANGE_MAX;

                dwResult = ERROR_SUCCESS;
            }
            break;

        //  Power management IOCTLs
        case IOCTL_POWER_CAPABILITIES:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_POWER_CAPABILITIES\r\n"));
            if (pBufOut != NULL && dwLenOut == sizeof(POWER_CAPABILITIES))
            {
                PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pBufOut;
                memset(ppc, 0, sizeof(*ppc));
                ppc->DeviceDx = DX_MASK(D0) | DX_MASK(D1) | DX_MASK(D2) | DX_MASK(D3) | DX_MASK(D4);

                if (pdwActualOut)
                    *pdwActualOut = sizeof(POWER_CAPABILITIES);

                dwResult = ERROR_SUCCESS;
            }
            break;

        case IOCTL_POWER_GET:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_POWER_GET\r\n"));
            if(pBufOut != NULL && dwLenOut == sizeof(CEDEVICE_POWER_STATE))
            {
                *(PCEDEVICE_POWER_STATE) pBufOut = (CEDEVICE_POWER_STATE) s_TouchDevice.dwPowerState;

                if (pdwActualOut)
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);

                dwResult = ERROR_SUCCESS;
            }
            break;

        case IOCTL_POWER_SET:
            DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_POWER_SET\r\n"));
            if(pBufOut != NULL && dwLenOut == sizeof(CEDEVICE_POWER_STATE))
            {
                CEDEVICE_POWER_STATE dx = *(CEDEVICE_POWER_STATE*)pBufOut;
                if( VALID_DX(dx) )
                {
                    DEBUGMSG(ZONE_FUNCTION, (L"TchPdd_Ioctl: IOCTL_POWER_SET = to D%u\r\n", dx));

                    if (pdwActualOut)
                        *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);

                    //  Enable touchscreen for D0-D2; otherwise disable
                    switch( dx )
                    {
                        case D0:
                        case D1:
                        case D2:
                            //  Enable touchscreen
                            if(s_TouchDevice.dwPowerState==D3 ||
								s_TouchDevice.dwPowerState==D4 )
                                PDDTouchPanelPowerHandler(FALSE);    //Enable Touch ADC
                            break;

                        case D3:
                        case D4:
                            //  Disable touchscreen
                            if(s_TouchDevice.dwPowerState==D0 ||
            				   s_TouchDevice.dwPowerState==D1 ||
            				   s_TouchDevice.dwPowerState==D2 )
                                PDDTouchPanelPowerHandler(TRUE); //Disable Touch ADC
                            break;

                        default:
                            //  Ignore
                            break;
                    }
                    s_TouchDevice.dwPowerState = dx;

                    dwResult = ERROR_SUCCESS;
                }
            }
            break;

        default:
            dwResult = ERROR_NOT_SUPPORTED;
            break;
    }

    if (dwResult != ERROR_SUCCESS)
    {
        SetLastError(dwResult);
        return FALSE;
    }
    return TRUE;
}


//==============================================================================
// Function Name: TchPdd_PowerUp
//
// Description: MDD passes xxx_PowerUp stream interface call to PDD.
//
// Arguments:   [IN] hPddContext. pddcontext returned in TchPdd_Init
//
// Ret Value:   None
//==============================================================================
void WINAPI TchPdd_PowerUp(
    DWORD hPddContext
    )
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_PowerUp+\r\n")));
    // Remove-W4: Warning C4100 workaround
    UNREFERENCED_PARAMETER(hPddContext);
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_PowerUp-\r\n")));
}

//==============================================================================
// Function Name: TchPdd_PowerDown
//
// Description: MDD passes xxx_PowerDown stream interface call to PDD.
//
// Arguments:   [IN] hPddContext. pddcontext returned in TchPdd_Init
//
// Ret Value:   None
//==============================================================================
void WINAPI TchPdd_PowerDown(
    DWORD hPddContext
    )
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_PowerDown+\r\n")));
    // Remove-W4: Warning C4100 workaround
    UNREFERENCED_PARAMETER(hPddContext);
    DEBUGMSG(ZONE_FUNCTION, (TEXT("TchPdd_PowerDown-\r\n")));
}

//==============================================================================
//Internal Functions
//==============================================================================
static void tscadc_getdatapoint(
    TOUCH_DEVICE *pDevice, 
    TOUCH_PANEL_SAMPLE_FLAGS *pTipStateFlags,
    INT *pUncalX,
    INT *pUncalY
)
{
	INT32 i, stepID;
	DWORD bytesRead, flags;
	UINT32 sampleX = 0, sampleY = 0;
	UINT32 prevX = 0xffffffff, prevY = 0xffffffff;
	UINT32 prevDiffX = 0xffffffff, prevDiffY = 0xffffffff;
	UINT32 diffX = 0, diffY = 0;
	UINT32 curDiffX = 0, curDiffY = 0;
	static UINT32 usLastFilteredX = 0, usLastFilteredY = 0;
    static UINT32 usSavedFilteredX = 0, usSavedFilteredY = 0;      // This holds the last reported X,Y sample
    static BOOL bPrevReportedPenDown = FALSE;
    static BOOL bActualPenDown       = FALSE; // This indicates if the pen is actually down, whether we report or not

	if (ReadMsgQueue(pDevice->hReadTouchSampleBufferQueue,&touchSample,sizeof(touchSample),
		&bytesRead,INFINITE,&flags))
	{
		for (i=0;i<XSTEPS;i++)
		{
			stepID = (touchSample.bufferX[i] & 0xf0000) >> 16;
			sampleX = touchSample.bufferX[i] & 0xfff;
			DEBUGMSG(ZONE_INFO, (L"x fifo raw data=%d\t =%04X\t id=%d\r\n", sampleX, sampleX, stepID));
			
			if (sampleX > prevX)
				curDiffX = sampleX - prevX;
			else
				curDiffX = prevX - sampleX;

			if (curDiffX < prevDiffX)
				prevDiffX = curDiffX;
			prevX = sampleX;

			stepID = (touchSample.bufferY[i] & 0xf0000) >> 16;
			sampleY = touchSample.bufferY[i] & 0xfff; 
			DEBUGMSG(ZONE_INFO, (L"y fifo raw data=%d\t =%04X\t id=%d\r\n", sampleY, sampleY, stepID));
				
			if (sampleY > prevY)
				curDiffY = sampleY - prevY;
			else
				curDiffY = prevY - sampleY;

			if (curDiffY < prevDiffY)
				prevDiffY = curDiffY;
			prevY = sampleY;
		}

		if (sampleX > usLastFilteredX)
		{
			diffX = sampleX - usLastFilteredX;
			diffY = sampleY - usLastFilteredY;
		}
		else
		{
			diffX = usLastFilteredX - sampleX;
			diffY = usLastFilteredY - sampleY;
		}
		usLastFilteredX = sampleX;
		usLastFilteredY = sampleY;
	}


    if(bActualPenDown)
	{
		if ((diffX < 15) && (diffY < 15)) 
		{
			*pUncalX = usLastFilteredX;
			*pUncalY = usLastFilteredY;
			usSavedFilteredX = usLastFilteredX;
			usSavedFilteredY = usLastFilteredY;
			
			*pTipStateFlags = TouchSampleDownFlag | TouchSampleValidFlag;
			DEBUGMSG(ZONE_INFO, ( TEXT( "PEN DOWN,X,Y=(%d, %d)\r\n" ), usLastFilteredX, usLastFilteredY) );
		}
    }
	
    /* if PEN event triggered, and FSM is in idle state, then report PEN up event */
	if ((pDevice->regs->irq_status_raw & TSCADC_IRQ_PENUP))  
	{
		if((pDevice->regs->adc_stat == TSCADC_ADCFSM_STEPID_IDLE))
    	{
            bActualPenDown = FALSE;
            usLastFilteredX = 0;
            usLastFilteredY = 0;
            
            // Return the valid pen data.  
            *pUncalX = usSavedFilteredX;
            *pUncalY = usSavedFilteredY;
            
            DEBUGMSG(ZONE_INFO, ( TEXT( "PEN UP,X,Y=(%d, %d)\r\n" ), usSavedFilteredX, usSavedFilteredY) );
            // Store reported pen state.
            *pTipStateFlags = TouchSampleValidFlag;
    	}
		else
		    bActualPenDown = TRUE;
	}
	
    if (!(pDevice->regs->irq_status_raw & TSCADC_IRQ_PENUP))  
    {
		if (pDevice->regs->adc_stat == TSCADC_ADCFSM_FSM_IRQ1)
		{
			bActualPenDown = TRUE;
		}
    }

	return ;
}

//==============================================================================
// Function Name: PDDCalibrationThread
//
// Description: This function is called from the PDD Init to calibrate the screen.
//              If the calibration data is already present in the registry,
//              this step is skipped, else a call is made into the GWES for calibration.
//
// Arguments:   None.
//
// Ret Value:   Success(1), faliure(0)
//==============================================================================
static HRESULT PDDCalibrationThread()
{
    HKEY hKey;
    DWORD dwType;
    LONG lResult;
    HANDLE hAPIs;
	BOOL ret;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationThread+\r\n")));

    // try to open [HKLM\hardware\devicemap\touch] key
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HARDWARE_DEVICEMAP_TOUCH, 0, KEY_ALL_ACCESS, &hKey))
    {
        DEBUGMSG(ZONE_CALIBRATE, (TEXT("CalibrationThread: calibration: Can't find [HKLM/%s]\r\n"), RK_HARDWARE_DEVICEMAP_TOUCH));
        return E_FAIL;
    }

    // check for calibration data (query the type of data only)
    lResult = RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, NULL, NULL);
    RegCloseKey(hKey);
    if (lResult == ERROR_SUCCESS)
    {
        // registry contains calibration data, return
        return S_OK;
    }

    hAPIs = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("SYSTEM/GweApiSetReady"));
    if (hAPIs)
    {
        WaitForSingleObject(hAPIs, INFINITE);
        CloseHandle(hAPIs);
    }

    // Perform calibration
    ret = TouchCalibrate();

    // try to open [HKLM\hardware\devicemap\touch] key
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HARDWARE_DEVICEMAP_TOUCH, 0, KEY_ALL_ACCESS, &hKey))
    {
        DEBUGMSG(ZONE_CALIBRATE, (TEXT("CalibrationThread: calibration: Can't find [HKLM/%s]\r\n"), RK_HARDWARE_DEVICEMAP_TOUCH));
        return E_FAIL;
    }

    // display new calibration data
    lResult = RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, NULL, NULL);
    if (lResult == ERROR_SUCCESS)
    {
        TCHAR szCalibrationData[100];
        DWORD Size = sizeof(szCalibrationData);

        RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, (BYTE *) szCalibrationData, (DWORD *) &Size);
        DEBUGMSG(ZONE_CALIBRATE, (TEXT("touchp: calibration: new calibration data is \"%s\"\r\n"), szCalibrationData));
    }
    RegCloseKey(hKey);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationThread-\r\n")));

    return S_OK;
}


//==============================================================================
// Function Name: PDDStartCalibrationThread
//
// Description: This function is creates the calibration thread with
//              PDDCalibrationThread as entry.
//
// Arguments:   None.
//
// Ret Value:   None
//==============================================================================
void PDDStartCalibrationThread()
{
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PDDCalibrationThread, NULL, 0, NULL);
    // We don't need the handle, close it here
    CloseHandle(hThread);
}

//==============================================================================
// Function: PDDTouchPanelPowerHandler
//
// This function indicates to the driver that the system is entering
// or leaving the suspend state.
//
// Parameters:
//      bOff
//          [in] TRUE indicates that the system is turning off. FALSE
//          indicates that the system is turning on.
//
// Returns:
//      None.
//==============================================================================
void PDDTouchPanelPowerHandler(BOOL boff)
{
    DEBUGMSG(ZONE_FUNCTION, (_T("PDDTouchPanelPowerHandler+\r\n")));

    if(boff)
	{
        if (s_TouchDevice.dwSysIntr != SYSINTR_NOP)
            InterruptMask(s_TouchDevice.dwSysIntr, boff);
	}
    else
	{
        if (s_TouchDevice.dwSysIntr != SYSINTR_NOP)
            InterruptMask(s_TouchDevice.dwSysIntr, boff);
	}
    DEBUGMSG(ZONE_FUNCTION, (_T("PDDTouchPanelPowerHandler-\r\n")));

    return;
}

//==============================================================================
// Function Name: PDDTouchPanelGetPoint
//
// Description: This function is used to read the touch coordinates from the h/w.
//
// Arguments:
//                  TOUCH_PANEL_SAMPLE_FLAGS * - Pointer to the sample flags. This flag is filled with the
//                                                                      values TouchSampleDownFlag or TouchSampleValidFlag;
//                  INT * - Pointer to the x coordinate of the sample.
//                  INT * - Pointer to the y coordinate of the sample.
//
// Ret Value:   None
//==============================================================================
void PDDTouchPanelGetPoint(
    TOUCH_PANEL_SAMPLE_FLAGS *pTipStateFlags,
    INT *pUncalX,
    INT *pUncalY
    )
{
    // By default, any sample returned will be ignored.
    *pTipStateFlags = TouchSampleIgnore;

    tscadc_getdatapoint(&s_TouchDevice, pTipStateFlags, pUncalX, pUncalY);
    // Set the proper state for the next interrupt.
    InterruptDone( s_TouchDevice.dwSysIntr );

    return;
}

//==============================================================================
// Function Name: PDDTouchIST
//
// Description: This is the IST which waits on the touch event or Time out value.
//              Normally the IST waits on the touch event infinitely, but once the
//              pen down condition is recognized the time out interval is changed
//              to dwSamplingTimeOut.
//
// Arguments:
//                  PVOID - Reseved not currently used.
//
// Ret Value:   None
//==============================================================================
ULONG PDDTouchIST(PVOID reserved)
{
    TOUCH_PANEL_SAMPLE_FLAGS    SampleFlags = 0;
    CETOUCHINPUT input;
    INT32                       RawX, RawY;

    // Remove-W4: Warning C4100 workaround
    UNREFERENCED_PARAMETER(reserved);

    DEBUGMSG(ZONE_TOUCH_IST, (L"PDDTouchIST: IST thread started\r\n"));

    //  Loop until told to stop
    while(!s_TouchDevice.bTerminateIST)
    {
        //  Wait for touch event, timeout or terminate flag
       WaitForSingleObject(s_TouchDevice.hReadTouchSampleBufferQueue, s_TouchDevice.dwSamplingTimeOut);

        //  Check for terminate flag
        if (s_TouchDevice.bTerminateIST)
            break;

        PDDTouchPanelGetPoint( &SampleFlags, &RawX, &RawY);

        if ( SampleFlags & TouchSampleIgnore )
        {
            // do nothing, not a valid sample
            continue;
        }

         //convert x-y coordination to one sample set before sending it to touch MDD.
         if(ConvertTouchPanelToTouchInput(SampleFlags, RawX, RawY, &input))
         {
               //send this 1 sample to mdd
               s_pfnMddReportSampleSet(s_mddContext, 1, &input);
         }

        DEBUGMSG(ZONE_TOUCH_SAMPLES, ( TEXT( "Sample:   (%d, %d)\r\n" ), RawX, RawY ) );
    }

    // IST thread terminating
    InterruptDone(s_TouchDevice.dwSysIntr);
    InterruptDisable(s_TouchDevice.dwSysIntr);

    CloseHandle(s_TouchDevice.hTouchPanelEvent);
    s_TouchDevice.hTouchPanelEvent = NULL;

    DEBUGMSG(ZONE_TOUCH_IST, (L"PDDTouchIST: IST thread ending\r\n"));

    return ERROR_SUCCESS;
}

//==============================================================================
// Function Name: PDDInitializeHardware
//
// Description: This routine configures the touch ADC channels.
//
// Arguments:  None
//
// Ret Value:   TRUE - Success
//                   FAIL - Failure
//==============================================================================
BOOL PDDInitializeHardware(LPCTSTR pszActiveKey)
{
    BOOL   rc = FALSE;
    PHYSICAL_ADDRESS pa = { 0, 0 };
    MSGQUEUEOPTIONS queueOptions;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDInitializeHardware+\r\n")));

    if (IsDVIMode())
        goto cleanUp;

    // Read parameters from registry
    if (GetDeviceRegistryParams(
            pszActiveKey,
            &s_TouchDevice,
            dimof(s_deviceRegParams),
            s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: PDDInitializeHardware: Error reading from Registry.\r\n")));
        goto cleanUp;
    }

    //map regs memory
    pa.LowPart = GetAddressByDevice(AM_DEVICE_ADC_TSC);
    s_TouchDevice.regs = (TSCADC_REGS*)MmMapIoSpace(pa, sizeof(TSCADC_REGS), FALSE);
    if (!s_TouchDevice.regs) {
    	RETAILMSG(1,  (L"PDDInitializeHardware: Cannot map TSCADC regs\r\n" ));
    	goto cleanUp;
    }

    s_TouchDevice.nPenIRQ = GetIrqByDevice(AM_DEVICE_ADC_TSC, NULL);

    // configure interrupt
    if (!KernelIoControl(
            IOCTL_HAL_REQUEST_SYSINTR,
            &s_TouchDevice.nPenIRQ,
            sizeof(s_TouchDevice.nPenIRQ),
            &s_TouchDevice.dwSysIntr,
            sizeof(s_TouchDevice.dwSysIntr),
            NULL
            ) )
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: TOUCH: Failed to request the touch sysintr.\r\n")));
        s_TouchDevice.dwSysIntr = (DWORD)SYSINTR_UNDEFINED;
        goto cleanUp;
    }
    DEBUGMSG(ZONE_ERROR, (TEXT(" TOUCH: touch sysintr:%d.\r\n"), s_TouchDevice.dwSysIntr));

	//  Add interrupts to wakeup sources:
	if(!KernelIoControl(
		IOCTL_HAL_ENABLE_WAKE,
		&s_TouchDevice.dwSysIntr,
		sizeof(s_TouchDevice.dwSysIntr),
		NULL,
		0,
		NULL ))
	{
		DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: TOUCH: Failed to register sysintr as wake-up source!\r\n")));
		goto cleanUp;
	}

    // Create queue for writing touch sample messages
    queueOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    queueOptions.dwFlags = MSGQUEUE_ALLOW_BROKEN;
    queueOptions.dwMaxMessages = 1;
    queueOptions.cbMaxMessage = sizeof(TouchSample_t);
    queueOptions.bReadAccess = TRUE;    // we need read-access to msgqueue

    s_TouchDevice.hReadTouchSampleBufferQueue = CreateMsgQueue(TOUCH_SAMPLE_QUEUE, &queueOptions);
    if (s_TouchDevice.hReadTouchSampleBufferQueue == NULL) {
        DEBUGMSG(ZONE_ERROR,(L"ERROR: TOUCH: "
                     L"creating touch sample buffer queue\r\n"
                    ));
    	goto cleanUp;
    }

    // Done
    rc = TRUE;

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDInitializeHardware-\r\n")));
    if( rc == FALSE )
    {
        PDDDeinitializeHardware();
    }

    return rc;
}


//==============================================================================
// Function Name: PDDDeinitializeHardware
//
// Description: This routine Deinitializes the h/w by closing the touch channels
//
// Arguments:  None
//
// Ret Value:   None
//==============================================================================
VOID PDDDeinitializeHardware()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDDeinitializeHardware+\r\n")));

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDDeinitializeHardware-\r\n")));
}


//==============================================================================
// Function Name: PDDTouchPanelEnable
//
// Description: This routine creates the touch thread(if it is not already created)
//              initializes the interrupt and unmasks it.
//
// Arguments:  None
//
// Ret Value:   TRUE - Success
//==============================================================================
BOOL  PDDTouchPanelEnable()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDTouchPanelEnable+\r\n")));

    //  Check if already running
    if( s_TouchDevice.hTouchPanelEvent == NULL )
    {
        //  Clear terminate flag
        s_TouchDevice.bTerminateIST = FALSE;

        //  Create touch event
        s_TouchDevice.hTouchPanelEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( !s_TouchDevice.hTouchPanelEvent )
        {
            DEBUGMSG(ZONE_ERROR,(TEXT("PDDTouchPanelEnable: Failed to initialize touch event.\r\n")));
            return FALSE;
        }

        //  Map SYSINTR to event
        if (!InterruptInitialize(s_TouchDevice.dwSysIntr, s_TouchDevice.hTouchPanelEvent, NULL, 0))
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("PDDTouchPanelEnable: Failed to initialize interrupt.\r\n")));
            return FALSE;
        }

        //  Create IST thread
       s_TouchDevice.hIST = CreateThread( NULL, 0, PDDTouchIST, 0, 0, NULL );
        if( s_TouchDevice.hIST == NULL )
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("PDDTouchPanelEnable: Failed to create IST thread\r\n")));
            return FALSE;
        }

        // set IST thread priority
        CeSetThreadPriority (s_TouchDevice.hIST, s_TouchDevice.dwISTPriority);

        if (s_TouchDevice.dwSysIntr != SYSINTR_NOP)
            InterruptMask(s_TouchDevice.dwSysIntr, FALSE);
    }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDTouchPanelEnable-\r\n")));
    return TRUE;
}

//==============================================================================
// Function Name: PDDTouchPanelDisable
//
// Description: This routine closes the IST and releases other resources.
//
// Arguments:  None
//
// Ret Value:   TRUE - Success
//==============================================================================
VOID  PDDTouchPanelDisable()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDTouchPanelDisable+\r\n")));

    //  Disable touch interrupt service thread
    if( s_TouchDevice.hTouchPanelEvent )
    {
        if (s_TouchDevice.dwSysIntr != SYSINTR_NOP)
            InterruptMask(s_TouchDevice.dwSysIntr, TRUE);

        s_TouchDevice.bTerminateIST = TRUE;
        SetEvent( s_TouchDevice.hTouchPanelEvent );
        s_TouchDevice.hTouchPanelEvent = 0;
    }

    //Closing IST handle
    if(s_TouchDevice.hIST)
    {
        CloseHandle(s_TouchDevice.hIST);
        s_TouchDevice.hIST=NULL;
     }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PDDTouchPanelDisable-\r\n")));
}
