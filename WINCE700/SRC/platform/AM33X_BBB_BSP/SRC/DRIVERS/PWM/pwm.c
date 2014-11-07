/*
* (c) David Vescovi 2014. All Rights Reserved.
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
//  File: pwm.c
//
/* EHPWM device driver
 *
 * This device driver supports only the basic features of the PWM
 * controller, it produces a square wave of configurable frequency and
 * duty-cycle.
 *
 * Frequency is configurable from 50Hz to 10MHz. At higher frequencies,
 * the frequency and duty cycle accuracy is not as good.
 * 
 * It uses count-up mode, with TBPRD defining the period and CMPA
 * specifying the duty-cycle.
 *
 * The driver expects to read the following configuration from the
 * registry:
 *   RegisterBaseAddress - A DWORD value specifying the base address of
 *                         the PWM peripheral to use
 *   EPWMXA_Active       - 0 to disable the EPWMxA signal output, 1 to
 *                         enable it
 *   EPWMXB_Active       - 0 to disable the EPWMxB signal output, 1 to
 *                         enable it
 *
 * The following features of the PWM controller are not currently
 * supported:
 *  Linking PWM units together
 *  Dead band
 *  PWM chopper
 *  Trip zone
 *
 * Copyright MPC Data Limited 2010
 *
 */

#include "bsp.h"
#include <initguid.h>
#include "ceddkex.h"
#include "am33x_pwmss.h"
#include "sdk_padcfg.h"
#include "bsp_padcfg.h"

//------------------------------------------------------------------------------
//  Local Definitions

#define PWM_DEVICE_COOKIE       'pwmD'

#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO
#undef ZONE_IOCTL

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IOCTL          DEBUGZONE(3)

DBGPARAM dpCurSettings = {
    L"PWM", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IOCTL",       L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x001f
};
#endif

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD					cookie;
    CRITICAL_SECTION        csDevice;       // serialize access to this device's state
    CEDEVICE_POWER_STATE    CurrentDx;      // current power level

	BOOL					fRunning;
	AM33X_DEVICE_CONF_REGS	*pDevConfRegs;
	AM33X_PWMSS_REGS	    *pPWMSSRegs;        // Pointer to PWM subsystem registers
	AM33X_EPWM_REGS		    *pPWMRegs;          // Pointer to PWM registers
	OMAP_DEVICE				deviceID;

    DWORD                   dwBaseAddress;  // Base address of PWM peripheral registers
    DWORD                   dwFrequency;    // Desired PWM output frequency
    DWORD                   dwDutyCycle;    // Current duty cycle as a percentage
    BOOL                    fEPWMXA_Active; // Are we using PWMxA output?
    BOOL                    fEPWMXB_Active; // Are we using PWMxB output?
    DWORD                   dwInitRunning;  // TRUE if initialize running
} Device_t;

//------------------------------------------------------------------------------
//
// PWM IOCTL definitions
//

#define PWM_IOCTL_GET_FREQUENCY    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PWM_IOCTL_GET_DUTYCYCLE    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0201, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PWM_IOCTL_GET_START_STOP    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0202, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PWM_IOCTL_SET_FREQUENCY    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PWM_IOCTL_SET_DUTYCYCLE    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0204, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PWM_IOCTL_SET_START_STOP    \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0205, METHOD_BUFFERED, FILE_ANY_ACCESS)


// Values for the default frequency and duty-cycle
#define PWM_DEFAULT_FREQUENCY 1000
#define PWM_DEFAULT_DUTY_CYCLE 50

#define CLOCK_TIME_BASE			100000000

//------------------------------------------------------------------------------
// Local functions

static void PWMStart(Device_t *pDevice);
static void PWMStop(Device_t *pDevice);
static UINT32 FindClockSettings(UINT32 DesiredFrequency, UINT32 DutyCycle, Device_t *pDevice);
static void PWMRegDump(Device_t *pDevice);

typedef struct FrequencyDividerValues {
    UINT32 clkdiv_value;
    UINT32 hspclkdiv_value;
    UINT32 resulting_divider;
} FREQUENCY_DIVIDER_VALUES;

// This is a table of all the combinations of CLKDIV and HSPCLKDIV that
// are available, and the resulting divider.
FREQUENCY_DIVIDER_VALUES FreqDividerValues[] = {
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_1,     1},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_2,     2},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_4,     4},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_6,     6},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_8,     8},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_10,    10},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_12,    12},
        {EPWM_TBCNT_CLKDIV_DIV_1, EPWM_TBCNT_HSPCLKDIV_DIV_14,    14},
        {EPWM_TBCNT_CLKDIV_DIV_2, EPWM_TBCNT_HSPCLKDIV_DIV_8,     16},
        {EPWM_TBCNT_CLKDIV_DIV_2, EPWM_TBCNT_HSPCLKDIV_DIV_10,    20},
        {EPWM_TBCNT_CLKDIV_DIV_2, EPWM_TBCNT_HSPCLKDIV_DIV_12,    24},
        {EPWM_TBCNT_CLKDIV_DIV_2, EPWM_TBCNT_HSPCLKDIV_DIV_14,    28},
        {EPWM_TBCNT_CLKDIV_DIV_4, EPWM_TBCNT_HSPCLKDIV_DIV_8,     32},
        {EPWM_TBCNT_CLKDIV_DIV_4, EPWM_TBCNT_HSPCLKDIV_DIV_10,    40},
        {EPWM_TBCNT_CLKDIV_DIV_4, EPWM_TBCNT_HSPCLKDIV_DIV_12,    48},
        {EPWM_TBCNT_CLKDIV_DIV_4, EPWM_TBCNT_HSPCLKDIV_DIV_14,    56},
        {EPWM_TBCNT_CLKDIV_DIV_8, EPWM_TBCNT_HSPCLKDIV_DIV_8,     64},
        {EPWM_TBCNT_CLKDIV_DIV_8, EPWM_TBCNT_HSPCLKDIV_DIV_10,    80},
        {EPWM_TBCNT_CLKDIV_DIV_8, EPWM_TBCNT_HSPCLKDIV_DIV_12,    96},
        {EPWM_TBCNT_CLKDIV_DIV_8, EPWM_TBCNT_HSPCLKDIV_DIV_14,    112},
        {EPWM_TBCNT_CLKDIV_DIV_16, EPWM_TBCNT_HSPCLKDIV_DIV_8,    128},
        {EPWM_TBCNT_CLKDIV_DIV_16, EPWM_TBCNT_HSPCLKDIV_DIV_10,   160},
        {EPWM_TBCNT_CLKDIV_DIV_16, EPWM_TBCNT_HSPCLKDIV_DIV_12,   192},
        {EPWM_TBCNT_CLKDIV_DIV_16, EPWM_TBCNT_HSPCLKDIV_DIV_14,   224},
        {EPWM_TBCNT_CLKDIV_DIV_32, EPWM_TBCNT_HSPCLKDIV_DIV_8,    256},
        {EPWM_TBCNT_CLKDIV_DIV_32, EPWM_TBCNT_HSPCLKDIV_DIV_10,   320},
        {EPWM_TBCNT_CLKDIV_DIV_32, EPWM_TBCNT_HSPCLKDIV_DIV_12,   384},
        {EPWM_TBCNT_CLKDIV_DIV_32, EPWM_TBCNT_HSPCLKDIV_DIV_14,   448},
        {EPWM_TBCNT_CLKDIV_DIV_64, EPWM_TBCNT_HSPCLKDIV_DIV_8,    512},
        {EPWM_TBCNT_CLKDIV_DIV_64, EPWM_TBCNT_HSPCLKDIV_DIV_10,   640},
        {EPWM_TBCNT_CLKDIV_DIV_64, EPWM_TBCNT_HSPCLKDIV_DIV_12,   768},
        {EPWM_TBCNT_CLKDIV_DIV_64, EPWM_TBCNT_HSPCLKDIV_DIV_14,   896},
        {EPWM_TBCNT_CLKDIV_DIV_128, EPWM_TBCNT_HSPCLKDIV_DIV_8,   1024},
        {EPWM_TBCNT_CLKDIV_DIV_128, EPWM_TBCNT_HSPCLKDIV_DIV_10,  1280},
        {EPWM_TBCNT_CLKDIV_DIV_128, EPWM_TBCNT_HSPCLKDIV_DIV_12,  1536},
        {EPWM_TBCNT_CLKDIV_DIV_128, EPWM_TBCNT_HSPCLKDIV_DIV_14,  1792}
};

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"RegisterBaseAddress", PARAM_DWORD, TRUE, 
        offset(Device_t, dwBaseAddress),
        fieldsize(Device_t, dwBaseAddress), (VOID*)0
    }, {
        L"EPWMXA_Active", PARAM_DWORD, TRUE, 
        offset(Device_t, fEPWMXA_Active),
        fieldsize(Device_t, fEPWMXA_Active), (VOID*)1
    }, {
        L"EPWMXB_Active", PARAM_DWORD, TRUE, 
        offset(Device_t, fEPWMXB_Active),
        fieldsize(Device_t, fEPWMXB_Active), (VOID*)1
    }, {
        L"Frequency", PARAM_DWORD, FALSE, 
        offset(Device_t, dwFrequency),
        fieldsize(Device_t, dwFrequency), (VOID*)PWM_DEFAULT_FREQUENCY
    }, {
        L"DutyCycle", PARAM_DWORD, FALSE, 
        offset(Device_t, dwDutyCycle),
        fieldsize(Device_t, dwDutyCycle), (VOID*)PWM_DEFAULT_DUTY_CYCLE
    }, {
        L"InitRunning", PARAM_DWORD, FALSE, 
        offset(Device_t, dwInitRunning),
        fieldsize(Device_t, dwInitRunning), (VOID*)1
    }
};

//------------------------------------------------------------------------------
//
//  Function:  PWM_Init
//
//  Called by the device manager when loading the driver
//
DWORD PWM_Init(
    LPCTSTR szContext, DWORD dwBusContext)
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = NULL;
    PHYSICAL_ADDRESS pa;
    
    UNREFERENCED_PARAMETER(szContext);
    UNREFERENCED_PARAMETER(dwBusContext);

    DEBUGMSG( ZONE_FUNCTION,
              (L"PWM_Init: %s 0x%x\r\n", szContext, dwBusContext));
    
    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed allocate PWM driver structure\r\n"));
        goto cleanUp;
    }

    memset(pDevice, 0, sizeof(Device_t));


	// Set cookie
    pDevice->cookie = PWM_DEVICE_COOKIE;

	// Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed read PWM driver registry parameters\r\n"));
        goto cleanUp;
    }

    InitializeCriticalSection(&(pDevice->csDevice));
    
    DEBUGMSG(ZONE_INIT, (L"PWM_INIT: Using base address 0x%x, EPWMXA active:%d, EPWMXB active: %d\r\n",
             pDevice->dwBaseAddress, pDevice->fEPWMXA_Active, pDevice->fEPWMXB_Active));
    
    pa.QuadPart = AM33X_DEVICE_CONF_REGS_PA;
    pDevice->pDevConfRegs = (AM33X_DEVICE_CONF_REGS *)MmMapIoSpace (pa, sizeof(AM33X_DEVICE_CONF_REGS), FALSE);
    if (pDevice->pDevConfRegs == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed map device config registers\r\n"));
        goto cleanUp;
    }

	switch (pDevice->dwBaseAddress - 0x200) {
//		case AM33X_ECAP_EQEP_EPWM0_REGS_PA:
//			pDevice->deviceID = AM_DEVICE_EPWM0;
//			break;
		case AM33X_ECAP_EQEP_EPWM1_REGS_PA:
			pDevice->deviceID = AM_DEVICE_EPWM1;
			pDevice->pDevConfRegs->PWMSS_CTRL |= 0x2;	// enable timebase clock
			if (pDevice->fEPWMXA_Active)
			{
				ReleasePad(PAD_ID(GPMC_A2));
				ConfigurePad(PAD_ID(GPMC_A2),(MODE(6)));
			}
			if (pDevice->fEPWMXB_Active)
			{
				ReleasePad(PAD_ID(GPMC_A3));
				ConfigurePad(PAD_ID(GPMC_A3),(MODE(6)));
			}
			break;
		case AM33X_ECAP_EQEP_EPWM2_REGS_PA:
			pDevice->deviceID = AM_DEVICE_EPWM2;
			pDevice->pDevConfRegs->PWMSS_CTRL |= 0x4;	// enable timebase clock
			if (pDevice->fEPWMXA_Active)
			{
				ReleasePad(PAD_ID(GPMC_AD8));
				ConfigurePad(PAD_ID(GPMC_AD8),MODE(4));
			}
			if (pDevice->fEPWMXB_Active)
			{
				ReleasePad(PAD_ID(GPMC_AD9));
				ConfigurePad(PAD_ID(GPMC_AD9),MODE(4));
			}
			break;
		default:
			pDevice->deviceID = OMAP_DEVICE_NONE;
			break;
	}
	
	if (pDevice->deviceID == OMAP_DEVICE_NONE){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed to find device ID for this PWM controller\r\n"));
        goto cleanUp;
	}
	
    // Map PWM subsystem
	pa.QuadPart = GetAddressByDevice(pDevice->deviceID);
    pDevice->pPWMSSRegs = MmMapIoSpace(pa, sizeof(AM33X_PWMSS_REGS), FALSE);
    if (pDevice->pPWMSSRegs == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed map PWM subsystem registers\r\n"));
        goto cleanUp;
    }
	
	// Map PWM controller
	pa.QuadPart = pDevice->dwBaseAddress;
    pDevice->pPWMRegs = MmMapIoSpace(pa, sizeof(AM33X_EPWM_REGS), FALSE);
    if (pDevice->pPWMRegs == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PWM_Init: Failed map PWM controller registers\r\n"));
        goto cleanUp;
    }

	EnableDeviceClocks(pDevice->deviceID, TRUE);

    // Setup Time Base Control Register
    pDevice->pPWMRegs->TBCTL = EPWM_TBCNT_SYNCOSEL_DISABLE |
                  EPWM_TBCNT_PHSEN_LOAD |
                  EPWM_TBCNT_CTRLMODE_FREEZE;                  

    // Set the PWM output high when TBCNT == 0, then set low when TBCNT == CMPA     
    if (pDevice->fEPWMXA_Active) {
        pDevice->pPWMRegs->AQCTLA = (EPWM_AQCTLA_ZRO_SET | EPWM_AQCTLA_CAU_CLEAR);
    }
    if (pDevice->fEPWMXB_Active) {
        pDevice->pPWMRegs->AQCTLB = (EPWM_ACQTLB_ZRO_SET | EPWM_ACQTLB_CAU_CLEAR);
    }

    FindClockSettings(pDevice->dwFrequency, pDevice->dwDutyCycle, pDevice);

	if (pDevice->dwInitRunning) 
	{
		DEBUGMSG( ZONE_INIT, (L"PWM_Init: Setting PWM running\r\n"));        
		PWMStart(pDevice);
	}
	else
	{
		DEBUGMSG( ZONE_INIT, (L"PWMInit: Setting PWM halted\r\n")); 
		PWMStop(pDevice);
	}

    rc = (DWORD)pDevice;
    
cleanUp:

    DEBUGMSG( ZONE_FUNCTION, (L"-PWM_Init: rc=%x\r\n", rc));

    return rc;

}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Deinit
//
BOOL PWM_Deinit(DWORD context)
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;
    
    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != PWM_DEVICE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: PWM_Deinit: Incorrect context paramer\r\n"));
        goto cleanUp;
    }

	EnableDeviceClocks(pDevice->deviceID, FALSE);

	DeleteCriticalSection(&pDevice->csDevice);

	if (pDevice->pPWMRegs != NULL){
        MmUnmapIoSpace((VOID*)pDevice->pPWMRegs, sizeof(AM33X_EPWM_REGS));
    }

	if (pDevice->pPWMSSRegs != NULL){
        MmUnmapIoSpace((VOID*)pDevice->pPWMSSRegs, sizeof(AM33X_PWMSS_REGS));
    }

	if (pDevice->pDevConfRegs != NULL){
        MmUnmapIoSpace((VOID*)pDevice->pDevConfRegs, sizeof(AM33X_DEVICE_CONF_REGS));
    }

	LocalFree(pDevice);

	rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Open
//
//  Called by the device manager on a CreateFile
//
//
DWORD PWM_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{

	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    DEBUGMSG(ZONE_FUNCTION, (L"+PWM_Open(0x%08x, 0x%08x, 0x%08x\r\n", context, accessCode, shareMode));

    DEBUGMSG(ZONE_FUNCTION, (L"-PWM_Open(rc = 0x%08x)\r\n", context));
    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Close
//
//  Called by the device manager on a CloseHandle
//
//
BOOL PWM_Close(DWORD context)
{
	BOOL rc = TRUE;
	DEBUGMSG(ZONE_FUNCTION, (L"+PWM_Close(0x%08x)\r\n", context));

    DEBUGMSG(ZONE_FUNCTION, (L"-PWM_Close(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Read
//
DWORD PWM_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
    UNREFERENCED_PARAMETER(hOpenContext);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(Count);

    return 0;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Write
//
DWORD PWM_Write(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
    UNREFERENCED_PARAMETER(hOpenContext);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(Count);

    return 0;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_Seek
//
DWORD PWM_Seek(DWORD hOpenContext, long Amount, WORD Type)
{
    UNREFERENCED_PARAMETER(hOpenContext);
    UNREFERENCED_PARAMETER(Amount);
    UNREFERENCED_PARAMETER(Type);

    return (DWORD)-1;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_IOControl
//
//  Called to configure the PWM peripheral
//
BOOL PWM_IOControl(
    DWORD context, DWORD dwCode, BYTE *pInBuffer, DWORD inSize, BYTE *pOutBuffer,
    DWORD outSize, DWORD *pOutSize)
{
    BOOL rc;
    Device_t *pDevice = (Device_t *)context;
	DWORD temp;
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+PWM_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    ));

	switch(dwCode) {

        // Get PWM frequency
        case PWM_IOCTL_GET_FREQUENCY:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_GET_FREQUENCY\r\n"));
			if ((pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
			if (pOutSize != 0) *pOutSize = sizeof(DWORD);
			if (!CeSafeCopyMemory(pOutBuffer, &pDevice->dwFrequency, sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
            break;

        // Get PWM duty cycle
        case PWM_IOCTL_GET_DUTYCYCLE:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_GET_DUTYCYCLE\r\n"));
			if ((pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
			if (pOutSize != 0) *pOutSize = sizeof(DWORD);
			if (!CeSafeCopyMemory(pOutBuffer, &pDevice->dwDutyCycle, sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
            break;

        // Get PWM start stop
        case PWM_IOCTL_GET_START_STOP:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_GET_START_STOP\r\n"));
			if ((pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
			if (pOutSize != 0) *pOutSize = sizeof(DWORD);
			temp = (DWORD)pDevice->fRunning;
			if (!CeSafeCopyMemory(pOutBuffer, &temp, sizeof(DWORD)))
			{
				dwErr = ERROR_INVALID_PARAMETER;
				break;
			}
            break;

       // Set PWM frequency
        case PWM_IOCTL_SET_FREQUENCY:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_SET_FREQUENCY\r\n"));
			if (pInBuffer && (inSize == sizeof(DWORD)))
			{
				pDevice->dwFrequency = *(DWORD *)pInBuffer;
				EnterCriticalSection(&pDevice->csDevice);
				FindClockSettings(pDevice->dwFrequency, pDevice->dwDutyCycle, pDevice);
				LeaveCriticalSection(&pDevice->csDevice);
				dwErr = ERROR_SUCCESS;
			}
            break;

        // Set PWM duty cycle
        case PWM_IOCTL_SET_DUTYCYCLE:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_SET_DUTYCYCLE\r\n"));
			if (pInBuffer && (inSize == sizeof(DWORD)))
			{
				pDevice->dwDutyCycle = *(DWORD *)pInBuffer;
				EnterCriticalSection(&pDevice->csDevice);
				FindClockSettings(pDevice->dwFrequency, pDevice->dwDutyCycle, pDevice);
				LeaveCriticalSection(&pDevice->csDevice);
				dwErr = ERROR_SUCCESS;
			}
            break;

		// Start/Stop PWM output
        case PWM_IOCTL_SET_START_STOP:
            DEBUGMSG( ZONE_IOCTL,
                (L"PWM_IOControl: PWM_IOCTL_SET_START_STOP\r\n"));
			if (pInBuffer && (inSize == sizeof(DWORD)))
			{
				temp = *(DWORD *)pInBuffer;
				EnterCriticalSection(&pDevice->csDevice);
				if (temp)
					PWMStart(pDevice);
				else
					PWMStop(pDevice);
				LeaveCriticalSection(&pDevice->csDevice);
				dwErr = ERROR_SUCCESS;
			}
            break;


        // --------------------- POWER MANAGEMENT IOCTLs --------------------
        
        case IOCTL_POWER_CAPABILITIES:
            // Tell the power manager about ourselves.
            DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: IOCTL_POWER_CAPABILITIES\r\n")));
            if (pOutBuffer != NULL 
                  && outSize >= sizeof(POWER_CAPABILITIES) 
                  && pOutSize != NULL) {
                __try {
                    PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pOutBuffer;
                    memset(ppc, 0, sizeof(*ppc));
                    ppc->DeviceDx = (DX_MASK(D0) | DX_MASK(D4));    // Support D0 and D4 only
                    *pOutSize = sizeof(*ppc);
                    dwErr = ERROR_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: exception in ioctl\r\n")));
                }
            }
            break;

        case IOCTL_POWER_QUERY: 
            if(pOutBuffer != NULL 
               && outSize == sizeof(CEDEVICE_POWER_STATE) 
               && pOutSize != NULL) {
                // Even though we don't really support D1, D2 or D3 we will not return an error on
                // the query.  Instead, we will go to D4 when asked to
                // go to D3, and D0 when asked to go to D1 or D2.
                __try {
                    CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pOutBuffer;
                    if(VALID_DX(NewDx)) {
                        // this is a valid Dx state so return a good status
                        *pOutSize = sizeof(CEDEVICE_POWER_STATE);
                        dwErr = ERROR_SUCCESS;
                    }
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: IOCTL_POWER_QUERY %u %s\r\n"),
                                          NewDx, dwErr == ERROR_SUCCESS ? _T("succeeded") : _T("failed")));
                } 
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: exception in ioctl\r\n")));
                }
            }
            break;

        case IOCTL_POWER_SET: 
            if(pOutBuffer != NULL 
               && outSize == sizeof(CEDEVICE_POWER_STATE) 
               && pOutSize != NULL) {
                // Allow a set to any state, but if requested to go to
                // D3, go to D4 instead. If D1 or D2 is requested, go
                // to D0
                __try {
                    CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pOutBuffer;
                    if(VALID_DX(NewDx)) {
                        // Map states to those that we support
                        if(NewDx == D3) {
                            NewDx = D4;
                        }
                        else if (NewDx == D1 || NewDx == D2) {
                            NewDx = D0;
                        }
                        *(PCEDEVICE_POWER_STATE) pOutBuffer = NewDx;
                        *pOutSize = sizeof(CEDEVICE_POWER_STATE);
//                        PWMSetPowerState(NewDx);
                        dwErr = ERROR_SUCCESS;
                    }
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: IOCTL_POWER_SET %u %s; passing back %u\r\n"), 
                                          NewDx, dwErr == ERROR_SUCCESS ? _T("succeeded") : _T("failed"), pDevice->CurrentDx));
                } 
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("%PWM_IOControl: exception in ioctl\r\n")));
                }
            }
            break;

        case IOCTL_POWER_GET: 
            if(pOutBuffer != NULL 
               && outSize == sizeof(CEDEVICE_POWER_STATE) 
               && pOutSize != NULL) {
                // Just return our CurrentDx value
                __try {
                    *(PCEDEVICE_POWER_STATE) pOutBuffer = pDevice->CurrentDx;
                    *pOutSize = sizeof(CEDEVICE_POWER_STATE);
                    dwErr = ERROR_SUCCESS;
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: IOCTL_POWER_GET %s; passing back %u\r\n"), 
                                          dwErr == ERROR_SUCCESS ? _T("succeeded") : _T("failed"), pDevice->CurrentDx));
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PWM_IOControl: exception in ioctl\r\n")));
                }
            }
            break;
        
        default:
            dwErr = ERROR_NOT_SUPPORTED;
            break;
    };
    
    DEBUGMSG( ZONE_FUNCTION,
        (L"-PWM_IOControl\r\n"));

    // pass back appropriate response codes
    SetLastError(dwErr);
    if(dwErr != ERROR_SUCCESS) {
        rc = FALSE;
    } else {
        rc = TRUE;
    }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_PowerDown
//
// Legacy power management function - not used
//
void PWM_PowerDown(DWORD context)
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  PWM_PowerUp
//
// Legacy power management function - not used
//
void PWM_PowerUp(DWORD context)
{
    UNREFERENCED_PARAMETER(context);
}


/* Find the closest frequency to DesiredFrequency, given that we have
 * only a fixed set of divisors which we can use with the module clock
 *
 * Parameters:
 *  DesiredFrequency    - (IN) The frequency value in Hz we want to achieve 
 *  ModuleClock         - (IN) The module clock frequency in Hz (SYSCLK2 value)
 *  clkdiv              - (OUT) Value to use for clkdiv register
 *  hspclkdiv           - (OUT) Value to use for hspclkdiv register
 * 
 * Returns:
 *  The closest achievable frequency in Hz
 */
static UINT32 FindClosestFrequency(double DesiredFrequency,
                                   double ModuleClock,
                                   UINT16 *clkdiv,
                                   UINT16 *hspclkdiv)
{
    UINT32 DesiredDivisor;
    UINT32 NearestDivisorSoFar;
    UINT32 SmallestDifferenceSoFar;
    UINT32 Difference;
    int i;

    // Work out what the ideal divisor would be, to convert the module
    // clock to the desired frequency
    DesiredDivisor = (UINT32)(ModuleClock / DesiredFrequency);
    NearestDivisorSoFar = 0;
    SmallestDifferenceSoFar = (UINT32)ModuleClock;

    // Go through the table of possible frequency divider values and
    // find the closest resultant divisor
    for(i=0; i<(sizeof(FreqDividerValues)/sizeof(FreqDividerValues[0])); ++i) {
        if (FreqDividerValues[i].resulting_divider <= DesiredDivisor)
            Difference = DesiredDivisor - FreqDividerValues[i].resulting_divider;
        else
            Difference = FreqDividerValues[i].resulting_divider - DesiredDivisor;

        // This is the best match so far, record the details including
        // the values to be written to clkdiv and hspclkdiv
        if (Difference < SmallestDifferenceSoFar) {
            *clkdiv = (UINT16)FreqDividerValues[i].clkdiv_value;
            *hspclkdiv = (UINT16)FreqDividerValues[i].hspclkdiv_value;
            SmallestDifferenceSoFar = Difference;
            NearestDivisorSoFar = FreqDividerValues[i].resulting_divider;
        }
    }
    
    return (UINT32)(ModuleClock / NearestDivisorSoFar);
}

// Work out the closest we can get to a specific frequency and duty
// cycle, and configure the PWM registers appropriately.
//
// Parameters:
//   DesiredFreq - (IN) Desired frequency in Hz
//   DutyCycle   - (IN) Desired positive duty cycle as a percentage
//
// Returns:
//   0 if not achievable
//   or the frequency we have selected (as close to the DesiredFreq as
//   we can achieve)
static UINT32 FindClockSettings(UINT32 DesiredFreq, UINT32 DutyCycle, Device_t *pDevice)
{
    double DesiredFrequency;
    double DesiredClockFrequency;   
    double ActualClockFrequency;
    double ActualFrequency;
    double LowestFrequencyDifferenceSoFar;
    UINT32 Multiple;
    UINT32 MultipleMin, MultipleMax;
    UINT32 BestMultipleSoFar;
    UINT32 PeriodCount;
    UINT32 DutyCycleCount;
    UINT16 clkdiv, hspclkdiv;
    DWORD ModuleClock;


    DesiredFrequency = (double)DesiredFreq;
    
    /* Check boundaries - currently only produce frequencies in the
     * range 50Hz -> 10MHz
     */
    if (DesiredFrequency < 50.0 ||
          DesiredFrequency > (10ul * 1000ul * 1000ul)) {
        DEBUGMSG( ZONE_ERROR, (L"PWM,FindClockSettings: Invalid frequency (%d)\r\n", DesiredFrequency));
        return 0;
    }   

    DEBUGMSG(ZONE_IOCTL,(L"Changing to frequency %dHz, duty cycle %d percent\r\n", DesiredFreq, DutyCycle));
    

    ModuleClock = CLOCK_TIME_BASE;

	/* We need to produce a clock signal to drive the timebase
     * counter that is a multiple of the desired frequency. What
     * multiple we use depends on the desired frequency
     *
     * The lower the multiple, the less accurate the duty cycle
     * and period we'll be able to specify but for higher frequencies we have
     * no choice.
     */

    /* This is a speed optimisation - we could just go through all
     * multipliers whatever the frequency but this takes significantly
     * longer */
    if (DesiredFrequency < (double)1000.0) {
        // At these low frequencies we can multiply it up quite
        // a bit to achieve greater accuracy
        MultipleMin = 32768;
        MultipleMax = 32768;        
    }   
    else if (DesiredFrequency < (double)500000.0) {
        MultipleMin = 256;
        MultipleMax = 2048;
    }
    else {
        // At higher frequencies we have to use a smaller multiple
        // which means the frequency and duty cycle will be less
        // precise.
        MultipleMin = 4;
        MultipleMax = 64;
    }

    /* Now go through the possible multipliers specified above and
     * find the one that produces a frequency we can get closest to
     */
    LowestFrequencyDifferenceSoFar = 0xffffffff;
    for(Multiple=MultipleMin; Multiple<=MultipleMax; ++Multiple) {
        /* Now find the closest clock frequency that we can derive
         * from the module clock */
        DesiredClockFrequency = DesiredFrequency * Multiple;
        ActualClockFrequency = FindClosestFrequency(DesiredClockFrequency, ModuleClock, &clkdiv, &hspclkdiv);

        /* Work out what count values we need to achieve the desired
         * frequency and duty cycle */
        PeriodCount = (UINT32)(ActualClockFrequency / DesiredFrequency);
        ActualFrequency = ActualClockFrequency / (double)PeriodCount;

        if ( (PeriodCount <= 0xffff) && // This is the maximum value it can have
             (LowestFrequencyDifferenceSoFar >= fabs(DesiredFrequency - ActualFrequency)) ) {
            /* This is the best yet, remember it */
            BestMultipleSoFar = Multiple;
            LowestFrequencyDifferenceSoFar = fabs(DesiredFrequency - ActualFrequency);
        }
    }

    /* Use the best frequency we found */
    DesiredClockFrequency = DesiredFrequency * BestMultipleSoFar;
    ActualClockFrequency = FindClosestFrequency(DesiredClockFrequency, ModuleClock, &clkdiv, &hspclkdiv);

    /* Work out what count values we need to achieve the desired
     * frequency and duty cycle */
    PeriodCount = (UINT32)(ActualClockFrequency / DesiredFrequency);

    DutyCycleCount = PeriodCount * DutyCycle / 100;

    ActualFrequency = ActualClockFrequency / (double)PeriodCount;

    // Set the time base clock
    pDevice->pPWMRegs->TBCTL &= ~(EPWM_TBCNT_CLKDIV_MASK | EPWM_TBCNT_HSPCLKDIV_MASK);
    pDevice->pPWMRegs->TBCTL |= (clkdiv | hspclkdiv);
    
    // Configure the period
    pDevice->pPWMRegs->TBPRD = PeriodCount;

    // Set the initial CMPA to determine the duty cycle
    pDevice->pPWMRegs->CMPA = DutyCycleCount;

    return (UINT32)ActualFrequency;
}

// Start the PWM signal generation
static void PWMStart(Device_t *pDevice)
{
    // Select Up-count mode. In this mode, the time-base counter starts
    // from zero and increments until it reaches the value in the period
    // register (TBPRD). When the period value is reached, the time-base
    // counter resets to zero and begins to increment once again.
    pDevice->pPWMRegs->TBCTL &= ~(EPWM_TBCNT_CTRLMODE_MASK);
    pDevice->pPWMRegs->TBCTL |= EPWM_TBCNT_CTRLMODE_COUNT_UP;

    pDevice->fRunning = TRUE;  
}

// Stop the PWM signal generation
static void PWMStop(Device_t *pDevice)
{
    // Select freeze count mode.
    pDevice->pPWMRegs->TBCTL &= ~(EPWM_TBCNT_CTRLMODE_MASK);
    pDevice->pPWMRegs->TBCTL |= EPWM_TBCNT_CTRLMODE_FREEZE;

    pDevice->fRunning = FALSE; 
}


static void PWMRegDump(Device_t *pDevice)
{
	RETAILMSG(TRUE, (L"\r\n"));
	RETAILMSG(TRUE, (L"IDVER:     0x%08x (offset:0x%x)\r\n", pDevice->pPWMSSRegs->IDVER, &(pDevice->pPWMSSRegs->IDVER)));
	RETAILMSG(TRUE, (L"SYSCONFIG: 0x%08x (offset:0x%x)\r\n", pDevice->pPWMSSRegs->SYSCONFIG, &(pDevice->pPWMSSRegs->SYSCONFIG)));
	RETAILMSG(TRUE, (L"CLKCONFIG: 0x%08x (offset:0x%x)\r\n", pDevice->pPWMSSRegs->CLKCONFIG, &(pDevice->pPWMSSRegs->CLKCONFIG)));
	RETAILMSG(TRUE, (L"CLKSTATUS: 0x%08x (offset:0x%x)\r\n", pDevice->pPWMSSRegs->CLKSTATUS, &(pDevice->pPWMSSRegs->CLKSTATUS)));
	RETAILMSG(TRUE, (L"\r\n"));
	RETAILMSG(TRUE, (L"TBCTL:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBCTL, &(pDevice->pPWMRegs->TBCTL)));
	RETAILMSG(TRUE, (L"TBSTS:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBSTS, &(pDevice->pPWMRegs->TBSTS)));
	RETAILMSG(TRUE, (L"TBPHSHR: 0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBPHSHR, &(pDevice->pPWMRegs->TBPHSHR)));
	RETAILMSG(TRUE, (L"TBPHS:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBPHS, &(pDevice->pPWMRegs->TBPHS)));
	RETAILMSG(TRUE, (L"TBCNT:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBCNT, &(pDevice->pPWMRegs->TBCNT)));
	RETAILMSG(TRUE, (L"TBPRD:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->TBPRD, &(pDevice->pPWMRegs->TBPRD)));
	RETAILMSG(TRUE, (L"CMPCTL:  0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->CMPCTL, &(pDevice->pPWMRegs->CMPCTL)));
	RETAILMSG(TRUE, (L"CMPAHR:  0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->CMPAHR, &(pDevice->pPWMRegs->CMPAHR)));
	RETAILMSG(TRUE, (L"CMPA:    0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->CMPA, &(pDevice->pPWMRegs->CMPA)));
	RETAILMSG(TRUE, (L"CMPB:    0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->CMPB, &(pDevice->pPWMRegs->CMPB)));
	RETAILMSG(TRUE, (L"AQCTLA:  0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->AQCTLA, &(pDevice->pPWMRegs->AQCTLA)));
	RETAILMSG(TRUE, (L"AQCTLB:  0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->AQCTLB, &(pDevice->pPWMRegs->AQCTLB)));
	RETAILMSG(TRUE, (L"AQSFRC:  0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->AQSFRC, &(pDevice->pPWMRegs->AQSFRC)));
	RETAILMSG(TRUE, (L"AQCSFRC: 0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->AQCSFRC, &(pDevice->pPWMRegs->AQCSFRC)));
	RETAILMSG(TRUE, (L"ETSEL:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->ETSEL, &(pDevice->pPWMRegs->ETSEL)));
	RETAILMSG(TRUE, (L"ETPS:    0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->ETPS, &(pDevice->pPWMRegs->ETPS)));
	RETAILMSG(TRUE, (L"ETFLG:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->ETFLG, &(pDevice->pPWMRegs->ETFLG)));
	RETAILMSG(TRUE, (L"ETCLR:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->ETCLR, &(pDevice->pPWMRegs->ETCLR)));
	RETAILMSG(TRUE, (L"ETFRC:   0x%04x (offset:0x%x)\r\n", pDevice->pPWMRegs->ETFRC, &(pDevice->pPWMRegs->ETFRC)));
}

