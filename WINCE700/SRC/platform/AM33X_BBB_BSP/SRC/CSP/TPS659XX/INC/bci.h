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
//  File:  bci.h
//
#ifndef __BCI_H
#define __BCI_H

#ifdef __cplusplus
extern "C" {
#endif


// over voltage flags
//
#define BCI_OV_VBAT                             0x00000001
#define BCI_OV_VBUS                             0x00000002
#define BCI_OV_VAC                              0x00000004

// charge current flags
//
#define BCI_OC_VBAT                             0x00000001
#define BCI_EOC_VBAT                            0x00000002
#define BCI_CGAIN_VBAT                          0x00000003

// vbat temp flags
#define BCI_TEMP_VBAT1                          0x00000001
#define BCI_TEMP_VBAT2                          0x00000002

// default charge current values in mA
#define BCI_DEFAULT_USBCHARGECURRENT            (500)
#define BCI_DEFAULT_ACCHARGECURRENT             (600)
#define BCI_MAX_CHARGECURRENT                   (1704)
#define BCI_MAX_BCIIREF1                        (0x200)
#define BCI_DEFAULT_CHARGECURRENT_STEPS         (1.6640325f)


//------------------------------------------------------------------------------
// battery charging mode

typedef enum
{
    kBCI_Battery,
    kBCI_AC,
    kBCI_USBHost,

//  Currently not supported
//
//    kBCI_USBMCPC,
//    kBCI_USBCharger,
//    kBCI_Carkit,

    kBCI_EOC,
    kBCI_SurgeProtect,
    kBCI_Unknown
} BatteryChargeMode_e;

//------------------------------------------------------------------------------
// Function: BCI_Initialize
//
// Desc: Initializes the BCI library
//
HANDLE 
BCI_Initialize(
    DWORD   nEOCTimeout,
    DWORD   nOverChargeTimeout,
    DWORD   nOverVoltageTimeout,
    DWORD   nOOBTemperatureTimeout
    );

//------------------------------------------------------------------------------
// Function: BCI_Uninitialize
//
// Desc: Unitializes the BCI library
//
void 
BCI_Uninitialize(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_GetBatteryVoltage
//
// Desc: Gets current battery voltage
//
DWORD 
BCI_GetBatteryVoltage(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_GetBatteryTemperature
//
// Desc: Gets current battery temperature
//
DWORD 
BCI_GetBatteryTemperature(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_SetTimeouts
//
// Desc: sets timeouts for overvoltage, overcharge, OOB temperature and eoc
//
BOOL 
BCI_SetTimeouts(
    HANDLE  hBCI,
    DWORD   nEOCTimeout,
    DWORD   nOverChargeTimeout,
    DWORD   nOverVoltageTimeout,
    DWORD   nOOBTemperatureTimeout
    );

//------------------------------------------------------------------------------
// Function: BCI_SetChargeMode
//
// Desc: Sets the battery charging mode
//
BOOL 
BCI_SetChargeMode(
    HANDLE hBCI, 
    BatteryChargeMode_e mode,
    BOOL bAttached
    );

//------------------------------------------------------------------------------
// Function: BCI_GetChargeMode
//
// Desc: Gets current battery charging mode
//
BatteryChargeMode_e 
BCI_GetChargeMode(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_GetVoltageThreshold
//
// Desc: gets over voltage threshold
//
UINT 
BCI_GetVoltageThreshold(
    HANDLE hBCI, 
    DWORD ffMask
    );

//------------------------------------------------------------------------------
// Function: BCI_SetVoltageThreshold
//
// Desc: sets over voltage threshold
//
BOOL 
BCI_SetVoltageThreshold(
    HANDLE hBCI, 
    DWORD ffMask,
    UINT8 threshold
    );

//------------------------------------------------------------------------------
// Function: BCI_GetTemperatureThreshold
//
// Desc: gets the overvoltage threshold for a voltage domain
//
BOOL
BCI_GetTemperatureThreshold(
    HANDLE      hBCI, 
    DWORD       ffMask,
    UINT8      *pThresholdHigh,
    UINT8      *pThresholdLow
    );

//------------------------------------------------------------------------------
// Function: BCI_SetTemperatureThreshold
//
// Desc: sets the temperature thresholds
//
BOOL 
BCI_SetTemperatureThreshold(
    HANDLE      hBCI,  
    DWORD       ffMask,
    UINT8       thresholdHigh,
    UINT8       thresholdLow
    );

//------------------------------------------------------------------------------
// Function: BCI_GetCurrentThreshold
//
// Desc: gets the current threshold for charging
//
UINT8
BCI_GetCurrentThreshold(
    HANDLE      hBCI, 
    DWORD       ffMasks
    );

//------------------------------------------------------------------------------
// Function: BCI_SetCurrentThreshold
//
// Desc: sets a charge current threshold
//
BOOL 
BCI_SetCurrentThreshold(
    HANDLE      hBCI,  
    DWORD       ffMask,
    UINT8       threshold
    );

//------------------------------------------------------------------------------
// Function: BCI_SetCurrentMonitors
//
// Desc: enables/disables charge current monitoring
//
BOOL 
BCI_SetCurrentMonitors(
    HANDLE      hBCI, 
    DWORD       ffEnable, 
    DWORD       ffMask
    );

//------------------------------------------------------------------------------
// Function: BCI_SetVoltageMonitors
//
// Desc: enable/disable over charge monitors
//
BOOL 
BCI_SetVoltageMonitors(
    HANDLE hBCI, 
    DWORD ffEnable,
    DWORD ffMask
    );

//------------------------------------------------------------------------------
// Function: BCI_SetTemperatureMonitors
//
// Desc: enables/disables temperature monitoring
//
BOOL 
BCI_SetTemperatureMonitors(
    HANDLE      hBCI, 
    DWORD       ffEnable, 
    DWORD       ffMask
    );

//------------------------------------------------------------------------------
// Function: BCI_QueryVoltageMonitors
//
// Desc: enable/disable over charge monitors
//
DWORD 
BCI_QueryVoltageMonitors(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_QueryTemperatureMonitors
//
// Desc: query the temperature monitors
//
DWORD
BCI_QueryTemperatureMonitors(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_QueryCurrentMonitors
//
// Desc: query the current monitors
//
DWORD
BCI_QueryCurrentMonitors(
    HANDLE hBCI
    );

//------------------------------------------------------------------------------
// Function: BCI_SetChargeCurrent
//
// Desc: sets the charge current
//
BOOL 
BCI_SetChargeCurrent(
    HANDLE              hBCI,
    BatteryChargeMode_e mode,
    DWORD               chargeCurrent
    );

//------------------------------------------------------------------------------
// Function: BCI_SetPrescale
//
// Desc: sets the timeouts
//
BOOL 
BCI_SetPrescale(
    HANDLE  hBCI,
    DWORD   preScale
    );

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

