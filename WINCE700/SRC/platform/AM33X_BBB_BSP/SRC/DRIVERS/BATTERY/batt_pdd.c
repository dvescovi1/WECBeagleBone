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
//  File: batt_pdd.c
//
//  Basic battery driver PDD.
//
#include "bsp.h"
#undef ZONE_ERROR
#undef ZONE_INIT
#include <battimpl.h>
#include <pm.h>
#include "oal_clock.h"
#include "ceddkex.h"
#include "tps65217.h"

#include <initguid.h>
#include "twl.h"
#include "triton.h"

// Extended Battery IOCTL codes
#define IOCTL_BATTERY_GETVBAT            \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x300, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETVSYS            \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x301, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETVTS             \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x302, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETICHG            \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x303, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETCHGSTATUS       \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x304, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETSTATUS			 \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x305, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BATTERY_GETCHGCFG			 \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x306, METHOD_BUFFERED, FILE_ANY_ACCESS)


DWORD BatteryPddIOControl(
    DWORD  dwContext,
    DWORD  Ioctl,
    PUCHAR pInBuf,
    DWORD  InBufLen, 
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    );

//------------------------------------------------------------------------------
//
//  Type:  BATTERY_PDD
//
typedef struct {
    HANDLE	hTWL;
	HANDLE	hADC;
	DWORD	batADCChannel;
	DWORD	batHigh;
	DWORD	batLow;
	DWORD	vbat;
	DWORD	vsys;
	DWORD	vts;
	DWORD	ichg;
	DWORD	chgst;
	DWORD	status;
	DWORD	chgcfg3;
} Device_t;

static Device_t s_Device;

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    {
        L"BatADCchannel", PARAM_DWORD, FALSE, offset(Device_t, batADCChannel),
        fieldsize(Device_t, batADCChannel), (VOID*)6
    }, {
        L"BatHigh", PARAM_DWORD, FALSE, offset(Device_t, batHigh),
        fieldsize(Device_t, batHigh), (VOID*)3600
    }, {
        L"BatLow", PARAM_DWORD, FALSE, offset(Device_t, batLow),
        fieldsize(Device_t, batLow), (VOID*)3400
    }
};


void DumpPMIC()
{
	byte data;
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHIPID, &data);
	RETAILMSG(1,(L"PMIC_REG_CHIPID	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_POWER_PATH, &data);
	RETAILMSG(1,(L"PMIC_REG_POWER_PATH	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_INTERRUPT, &data);
	RETAILMSG(1,(L"PMIC_REG_INTERRUPT	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG0, &data);
	RETAILMSG(1,(L"PMIC_REG_CHGCONFIG0	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG1, &data);
	RETAILMSG(1,(L"PMIC_REG_CHGCONFIG1	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG2, &data);
	RETAILMSG(1,(L"PMIC_REG_CHGCONFIG2	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG3, &data);
	RETAILMSG(1,(L"PMIC_REG_CHGCONFIG3	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_MUXCTRL, &data);
	RETAILMSG(1,(L"PMIC_REG_MUXCTRL	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_STATUS, &data);
	RETAILMSG(1,(L"PMIC_REG_STATUS	%02x\r\n",data ));
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_ENABLE, &data);
	RETAILMSG(1,(L"PMIC_REG_ENABLE	%02x\r\n",data ));
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDInitialize
//
BOOL WINAPI 
BatteryPDDInitialize(
    LPCTSTR szContext
    )
{
    BOOL rc = FALSE;
    PHYSICAL_ADDRESS pa = { 0, 0 };
	BYTE data;
    DWORD	clk_value;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+BatteryPDDInitialize()\r\n"));

    // Clear structure
    memset(&s_Device, 0, sizeof(Device_t));

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, &s_Device, 
        dimof(g_deviceRegParams), g_deviceRegParams) != ERROR_SUCCESS) 
        {
        DEBUGMSG(ZONE_ERROR,(L"ERROR: BatteryPDDInitialize: "
            L"Failed read driver registry parameters\r\n"
            ));
        goto cleanUp;
        }
    
    // Open T2 driver
    if ((s_Device.hTWL = TWLOpen()) == NULL) 
    {
		DEBUGMSG(ZONE_ERROR,(L"ERROR: BatteryPDDInitialize: "
			L"Failed open TWL bus driver\r\n"
			));
		goto cleanUp;
    }

	s_Device.hADC = CreateFile(L"ADC1:", 0, 0, NULL, 0, 0, NULL);
    if (s_Device.hADC == INVALID_HANDLE_VALUE)
	{
		DEBUGMSG(ZONE_ERROR,(L"ERROR: BatteryPDDInitialize: "
			L"Failed open ADC driver\r\n"
			));
		goto cleanUp;
	}

    // Reference the PDD_IOCTL function to MDD.
	gpfnBatteryPddIOControl = BatteryPddIOControl;
	
	// Set charger parameters for specific battery 
	// timer = 4hr
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG1, &data);
	data = data & ~PMIC_CHGCONFIG1_TIMER_MASK;
	data |= PMIC_CHGCONFIG1_TIMER_4HR;
	TWLWriteByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG1, data);
	// charge at 4.25v
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG2, &data);
	data = data & ~PMIC_CHGCONFIG2_VOREG_MASK;
	data |= PMIC_CHGCONFIG2_VOREG_4_25V;
	TWLWriteByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG2, data);

	TWLReadByteReg(s_Device.hTWL, PMIC_REG_STATUS, &data);
	if (data & PMIC_STATUS_ACPWR)
	{
		// charge current 700ma (on AC)
		TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG3, &data);
		data = data & ~PMIC_CHGCONFIG3_ICHRG_MASK;
		data |= PMIC_CHGCONFIG3_ICHRG_700MA;
	}
	else
	{
		// charge current 500ma (on USB etc.)
		TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG3, &data);
		data = data & ~PMIC_CHGCONFIG3_ICHRG_MASK;
		data |= PMIC_CHGCONFIG3_ICHRG_500MA;
	}
	TWLWriteByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG3, data);
	s_Device.chgcfg3 = data;

	rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BatteryPDDInitialize\r\n"));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDDeinitialize
//
void WINAPI 
BatteryPDDDeinitialize()
{
	DEBUGMSG(ZONE_FUNCTION, (L"+BatteryPDDDeinitialize\r\n"));

	if (s_Device.hADC != NULL) CloseHandle(s_Device.hADC);

	if (s_Device.hTWL != NULL) TWLClose(s_Device.hTWL);

	DEBUGMSG(ZONE_FUNCTION, (L"-BatteryPDDDeinitialize\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDResume
//
void WINAPI 
BatteryPDDResume()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+BatteryPDDResume\r\n"));
    DEBUGMSG(ZONE_FUNCTION, (L"-BatteryPDDResume\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDPowerHandler
//
void WINAPI 
BatteryPDDPowerHandler(
    BOOL off
    )
{
    UNREFERENCED_PARAMETER(off);
    DEBUGMSG(ZONE_FUNCTION, (L"+BatteryPDDPowerHandler(%d)\r\n",  off));
    DEBUGMSG(ZONE_FUNCTION, (L"-BatteryPDDPowerHandler\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPDDGetLevels
//
//  Indicates how many battery levels will be reported by BatteryPDDGetStatus()
//  in the BatteryFlag and BackupBatteryFlag fields of PSYSTEM_POWER_STATUS_EX2.
//
//  Returns the main battery level in the low word, and the backup battery
//  level in the high word.
//
LONG 
BatteryPDDGetLevels()
{
    LONG lLevels = MAKELONG(
        3,      // Main battery levels
        0       // Backup battery levels
    );
    return lLevels;
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPDDSupportsChangeNotification
//
//  Returns FALSE since this platform does not support battery change
//  notification.
//
BOOL 
BatteryPDDSupportsChangeNotification()
{
    return FALSE;
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPddIOControl
//
//  Battery driver needs to handle D0-D4 power notifications
//
//  Returns ERROR code.
//
DWORD
BatteryPddIOControl(
    DWORD  dwContext,
    DWORD  Ioctl,
    PUCHAR pInBuf,
    DWORD  InBufLen, 
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    DWORD dwRet;

    UNREFERENCED_PARAMETER(dwContext);
    UNREFERENCED_PARAMETER(Ioctl);
    UNREFERENCED_PARAMETER(pInBuf);
    UNREFERENCED_PARAMETER(InBufLen);
    
    switch (Ioctl)
        {
			case IOCTL_BATTERY_GETVBAT:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.vbat;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETVBAT\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;

			case IOCTL_BATTERY_GETVSYS:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.vsys;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETVSYS\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
			case IOCTL_BATTERY_GETVTS:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.vts;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETVTS\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
			case IOCTL_BATTERY_GETICHG:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.ichg;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETICHG\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
			case IOCTL_BATTERY_GETCHGSTATUS:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.chgst;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETCHGSTATUS\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
			case IOCTL_BATTERY_GETSTATUS:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.status;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETSTATUS\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
			case IOCTL_BATTERY_GETCHGCFG:
				// sanity check parameters
				if(pOutBuf != NULL && OutBufLen == sizeof(DWORD) && pdwBytesTransferred != NULL) {
				    
					// pass back return values
					__try {
						*((PDWORD) pOutBuf) = s_Device.chgcfg3;
						*pdwBytesTransferred = sizeof(DWORD);
						dwRet = ERROR_SUCCESS;
					}
					__except(EXCEPTION_EXECUTE_HANDLER) {
						DEBUGMSG(ZONE_WARN, 
							(_T("exception in IOCTL_BATTERY_GETCHGCFG\r\n")));
						dwRet = ERROR_INVALID_PARAMETER;
					}
				}
			break;
	
        default:
            dwRet = ERROR_NOT_SUPPORTED;
			break;
        }
    return dwRet;
}


// return channel voltage
static DWORD GetADCVoltage()
{
	DWORD temp, bytesRet;
	BYTE mux = 0;
	BOOL exit = FALSE;
	DWORD mvolts = 0;

	// trigger scan
	DeviceIoControl(s_Device.hADC,IOCTL_ADC_SCANCHANNEL,&s_Device.batADCChannel,sizeof(DWORD),NULL,0,NULL,NULL);

	while(!exit)
	{
		temp = 0;
		DeviceIoControl(s_Device.hADC,IOCTL_ADC_GETCHANNEL,&s_Device.batADCChannel,sizeof(DWORD),
			&temp,sizeof(DWORD),&bytesRet,NULL);
//		RETAILMSG(1,(L"data  %08x\r\n", temp));
		switch (mux)
		{
			case PMIC_MUXCTRL_MUX_VBAT:
				temp *= 85393;
				temp >>= 16;
				s_Device.vbat = temp;
				DEBUGMSG(ZONE_INIT,(L"BAT %d mv\r\n",temp));
				mvolts = temp;
				mux = PMIC_MUXCTRL_MUX_VSYS;
				TWLWriteRegs(s_Device.hTWL, PMIC_REG_MUXCTRL, &mux, 1);
				break;
			case PMIC_MUXCTRL_MUX_VSYS:
				temp *= 85393;
				temp >>= 16;
				s_Device.vsys = temp;
				DEBUGMSG(ZONE_INIT,(L"SYS %d mv\r\n",temp ));
				mux = PMIC_MUXCTRL_MUX_VTS;
				TWLWriteRegs(s_Device.hTWL, PMIC_REG_MUXCTRL, &mux, 1);
				break;
			case PMIC_MUXCTRL_MUX_VTS:
				temp *= 28753;
				temp >>= 16;
				s_Device.vts = temp;
				DEBUGMSG(ZONE_INIT,(L"VTS %d mv\r\n",temp ));
				mux = PMIC_MUXCTRL_MUX_VICHARGE;
				TWLWriteRegs(s_Device.hTWL, PMIC_REG_MUXCTRL, &mux, 1);
				break;
			case PMIC_MUXCTRL_MUX_VICHARGE:
				switch (s_Device.chgcfg3 & PMIC_CHGCONFIG3_ICHRG_MASK) {
					case PMIC_CHGCONFIG3_ICHRG_700MA:
						temp *= 11203;	// only valid for 700ma charge setting
						break;
					case PMIC_CHGCONFIG3_ICHRG_500MA:
						temp *= 8005;	// only valid for 500ma charge setting
						break;
					case PMIC_CHGCONFIG3_ICHRG_400MA:
						temp *= 6401;	// only valid for 400ma charge setting
					break;
					case PMIC_CHGCONFIG3_ICHRG_300MA:
						temp *= 4801;	// only valid for 300ma charge setting
					break;
				}
				temp >>= 16;
				s_Device.ichg = temp;
				DEBUGMSG(ZONE_INIT,(L"CHG %d mA\r\n",temp ));
				mux = PMIC_MUXCTRL_MUX_VBAT;
				TWLWriteRegs(s_Device.hTWL, PMIC_REG_MUXCTRL, &mux, 1);
				exit = TRUE;
				break;
			default:
				mux = PMIC_MUXCTRL_MUX_VBAT;
				TWLWriteRegs(s_Device.hTWL, PMIC_REG_MUXCTRL, &mux, 1);
				break;
		}
		Sleep(100);
		// trigger scan
		DeviceIoControl(s_Device.hADC,IOCTL_ADC_SCANCHANNEL,&s_Device.batADCChannel,sizeof(DWORD),NULL,0,NULL,NULL);
	}
	DEBUGMSG(ZONE_BATTERY, (L"Bat voltage %dmv\r\n", mvolts));
	return mvolts;
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDGetStatus()
//
//  Obtains the battery and power status.
//
BOOL WINAPI 
BatteryPDDGetStatus(
    SYSTEM_POWER_STATUS_EX2 *pStatus, 
    BOOL *pBatteriesChangedSinceLastCall
    ) 
{
    BOOL rc = FALSE;
    BYTE data = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"+BatteryPDDGetStatus\r\n"));

    pStatus->BatteryLifePercent         = BATTERY_PERCENTAGE_UNKNOWN;
    pStatus->BatteryLifeTime            = BATTERY_LIFE_UNKNOWN;
    pStatus->BatteryFullLifeTime        = BATTERY_LIFE_UNKNOWN;

    pStatus->Reserved1                  = 0;
    pStatus->Reserved2                  = 0;
    pStatus->Reserved3                  = 0;    

    pStatus->BatteryChemistry           = BATTERY_CHEMISTRY_LION;
    pStatus->BatteryVoltage             = GetADCVoltage();
    pStatus->BatteryCurrent             = 0;
    pStatus->BatteryAverageCurrent      = 0;
    pStatus->BatteryAverageInterval     = 0;
    pStatus->BatterymAHourConsumed      = 0;
    pStatus->BatteryTemperature         = 250;  // unit is 0.1 deg C

    pStatus->BackupBatteryFlag          = BATTERY_FLAG_UNKNOWN;
	pStatus->BackupBatteryVoltage       = 0;
    pStatus->BackupBatteryLifeTime      = BATTERY_LIFE_UNKNOWN;
    pStatus->BackupBatteryFullLifeTime  = BATTERY_LIFE_UNKNOWN;
    
    pStatus->BackupBatteryLifePercent   = 100;

    *pBatteriesChangedSinceLastCall = FALSE;

	
    // Update status with new data
	data = 0;
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_STATUS, &data);
	s_Device.status = (DWORD)data;
	data = 0;
	TWLReadByteReg(s_Device.hTWL, PMIC_REG_CHGCONFIG0, &data);
	s_Device.chgst = (DWORD)data;

    pStatus->BatteryFlag = 0;
	if (s_Device.status & (PMIC_STATUS_ACPWR | PMIC_STATUS_USBPWR))
	{
		pStatus->ACLineStatus = AC_LINE_ONLINE;
		if (s_Device.chgst & PMIC_CHGCONFIG0_ACTIVE)
			pStatus->BatteryFlag |= BATTERY_FLAG_CHARGING;
	}
	else
	{
		pStatus->ACLineStatus = AC_LINE_OFFLINE;
	}
	if (pStatus->BatteryVoltage >= s_Device.batHigh)
	{        
		pStatus->BatteryFlag |= BATTERY_FLAG_HIGH;
	}
	else if (pStatus->BatteryVoltage >= s_Device.batLow)
	{
		pStatus->BatteryFlag |= BATTERY_FLAG_LOW;
	}
	else if (pStatus->BatteryVoltage < s_Device.batLow)
	{
		pStatus->BatteryFlag |= BATTERY_FLAG_CRITICAL;
	}
	else
	{
		pStatus->BatteryFlag = BATTERY_FLAG_NO_BATTERY;
	}

	rc = TRUE;

    DEBUGMSG(ZONE_PDD, (L"-BatteryPDDGetStatus\r\n"));
    return rc;
}

//------------------------------------------------------------------------------
