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
//  File:  bci.c
//
#include "omap.h"
#undef ZONE_ERROR
#undef ZONE_INIT
#include <battimpl.h>
#include "ceddkex.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"
#include "tps659xx_bci.h"
#include "tps659xx_batt.h"

#include <initguid.h>
#include "bci.h"
#include "madc.h"
#include "twl.h"
#include "triton.h"


#define EVENT_POWER         0
#define EVENT_BCI           1
#define EVENT_HOTDIE        2

#define VAC_CHANNEL         MADC_CHANNEL_11
#define VBUS_CHANNEL        MADC_CHANNEL_8

#define ACCHGOVTH_DEFAULT   2
#define VBATOVTH_DEFAULT    0
#define VBUSOVTH_DEFAULT    0

#define IDX_BCIMFSTS2       0
#define IDX_BCIMFSTS3       1
#define IDX_BCIMFSTS4       2

#define ACMINVOLT           (3300)
#define VBUSMINVOLT         (3000)
#define THRESHOLD_DISABLED   (0x80000000)





HANDLE hHotDieTriggerEvent_Test=NULL;
#define HOT_DIE_MASK_INT         (1<<4)
#define HOT_DIE_RISING           (1<<1)
#define HOT_DIE_FALLING          (1<<0)
#define REG_BATTERY_DRV_PATH TEXT("Drivers\\BuiltIn\\Battery")

//------------------------------------------------------------------------------
// look-up tables

static 
DWORD const _rgACCHGOVTH[4] = {5500, 6000, 6500, 6800};

static 
DWORD const _rgVBATOVTH[4] = {4550, 4750, 4950, 5050};

static 
DWORD const _rgVBUSOVTH[4] = {5500, 6000, 6500, 6800};


//------------------------------------------------------------------------------
// opaque structures

typedef struct
{
    CRITICAL_SECTION    cs;
    BatteryChargeMode_e currentMode;
    HANDLE              hTwl;
    HANDLE              hMadc;
    HANDLE              hBattSignal;
    HANDLE              hIntrThread;
    HANDLE              rgIntrEvents[3];
    UINT                preScale;
    UINT16              usbChargeCurrent;
    UINT16              acChargeCurrent;
    DWORD               threadsExit;
    DWORD               priority256;
    DWORD               dwACCHGOVTH;
    DWORD               dwVBATOVTH;
    DWORD               dwVBUSOVTH;
    DWORD               nEndOfChargeTimeout;
    DWORD               nOverVoltageTimeout;
    DWORD               nOOBTemperatureTimeout;
    DWORD               nOverChargeTimeout;
    UINT8               rgBciStatus[3];
    BOOL                bAttached_USBHost;
    
} Device_t;


//------------------------------------------------------------------------------
// prototypes

static DWORD
IntrThread(
    VOID *pContext
    );

//------------------------------------------------------------------------------
// Function: QueryVoltageMonitors
//
// Desc: query the overcharge monitors
//
static DWORD
QueryVoltageMonitors(
    Device_t *pDevice
    )
{
    UINT8 data;
    DWORD dwResult = 0;
    DEBUGMSG(ZONE_FUNCTION, (L"+QueryVoltageMonitors(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // get access to BCIMFEN2
    data = KEY_BCIMFEN2;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // update over-charge monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
    if (data & ACCHGOVEN) dwResult |= BCI_OV_VAC;
    if (data & VBUSOVEN) dwResult |= BCI_OV_VBUS;

    // get access to BCIMFEN4
    data = KEY_BCIMFEN4;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // update over-charge monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN4, &data, 1);
    if (data & VBATOVEN) dwResult |= BCI_OV_VBAT;

    DEBUGMSG(ZONE_FUNCTION, (L"-QueryVoltageMonitors()\r\n"));
    return dwResult;
}


//------------------------------------------------------------------------------
// Function: QueryTemperatureMonitors
//
// Desc: query the temperature monitors
//
static DWORD
QueryTemperatureMonitors(
    Device_t *pDevice
    )
{
    UINT8 data;
    DWORD dwResult = 0;
    DEBUGMSG(ZONE_FUNCTION, (L"+QueryTemperatureMonitors(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // get access to BCIMFEN2
    data = KEY_BCIMFEN2;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // check temperature monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
    if (data & TBATOR1EN) dwResult |= BCI_TEMP_VBAT1;
    if (data & TBATOR2EN) dwResult |= BCI_TEMP_VBAT2;

    DEBUGMSG(ZONE_FUNCTION, (L"-QueryTemperatureMonitors()\r\n"));
    return dwResult;
}


//------------------------------------------------------------------------------
// Function: QueryTemperatureMonitors
//
// Desc: query the temperature monitors
//
static DWORD
QueryCurrentMonitors(
    Device_t *pDevice
    )
{
    UINT8 data;
    DWORD dwResult = 0;
    DEBUGMSG(ZONE_FUNCTION, (L"+QueryChargeMonitors(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // get access to BCIMFEN2
    data = KEY_BCIMFEN3;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // check charging monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN3, &data, 1);
    if (data & ICHGHIGHEN) dwResult |= BCI_OC_VBAT;
    if (data & ICHGEOCEN) dwResult |= BCI_EOC_VBAT;

    // check current gain settings
    TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
    if (data & CGAIN) dwResult |= BCI_CGAIN_VBAT;

    DEBUGMSG(ZONE_FUNCTION, (L"-QueryChargeMonitors()\r\n"));
    return dwResult;
}


//------------------------------------------------------------------------------
// Function: ContinueChargeOnOOBTemperature
//
// Desc: flags to disable default behavior and continue charge on out of bound
//       temperature
//
static BOOL 
ContinueChargeOnOOBTemperature(
    Device_t *pDevice, 
    DWORD ffEnable, 
    DWORD ffMask
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+ContinueChargeOnOOBTemperature("
        L"pDevice=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        pDevice, ffEnable, ffMask
        ));

    // get access to BCIMFEN2
    data = KEY_BCIMFEN2;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // update over-charge monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
    if (ffMask & BCI_TEMP_VBAT1)
        {
        data = (ffEnable & BCI_TEMP_VBAT1) ? (data | TBATOR1CF) :
                                             (data & ~TBATOR1CF);
        }

    if (ffMask & BCI_TEMP_VBAT2)
        {
        data = (ffEnable & BCI_TEMP_VBAT2) ? (data | TBATOR2CF) :
                                             (data & ~TBATOR2CF);
        }
        
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);

    bResult = TRUE;
            
    DEBUGMSG(ZONE_FUNCTION, (L"-ContinueChargeOnOOBTemperature()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: ContinueChargeOnOverVoltage
//
// Desc: flags to disable default behavior and continue charge on overcharge
//       notification
//
static BOOL 
ContinueChargeOnOverVoltage(
    Device_t *pDevice, 
    DWORD ffEnable, 
    DWORD ffMask
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+ContinueChargeOnOverVoltage("
        L"pDevice=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        pDevice, ffEnable, ffMask
        ));

    // over-charge monitoring for usb and ac
    if (ffMask & (BCI_OV_VAC | BCI_OV_VBUS))
        {
        // get access to BCIMFEN2
        data = KEY_BCIMFEN2;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

        // update over-charge monitors
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
        if (ffMask & BCI_OV_VAC)
            {
            data = (ffEnable & BCI_OV_VAC) ? (data | ACCHGOVCF) :
                                             (data & ~ACCHGOVCF);
            }

        if (ffMask & BCI_OV_VBUS)
            {
            data = (ffEnable & BCI_OV_VBUS) ? (data | VBUSOVCF) :
                                             (data & ~VBUSOVCF);
            }

        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
        }

    // over-charge monitoring for battery
    if (ffMask & BCI_OV_VBAT)
        {
        // get access to BCIMFEN4
        data = KEY_BCIMFEN4;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

        // update over-charge monitors
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN4, &data, 1);
        data = (ffEnable & BCI_OV_VBAT) ? (data | VBATOVCF) :
                                          (data & ~VBATOVCF);
                                          
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN4, &data, 1);
        }

    bResult = TRUE;
            
    DEBUGMSG(ZONE_FUNCTION, (L"-ContinueChargeOnOverVoltage()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: GetCurrentThreshold
//
// Desc: gets the charge current thresholds
//
static UINT8 
GetCurrentThreshold(
    Device_t *pDevice, 
    DWORD ffMask
    )
{
    UINT8 data;
    UINT8 threshold = 0;    
    DEBUGMSG(ZONE_FUNCTION, (L"+GetCurrentThreshold("
        L"pDevice=0x%08X, ffMask=%d)\r\n",
        pDevice, ffMask
        ));

    switch (ffMask)
        {
        case BCI_OC_VBAT:
            // get access to BCIMFTH8
            data = KEY_BCIMFTH8;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);
            threshold = (data & 0x0F);
            
            // get access to BCIMFTH9
            data = KEY_BCIMFTH9;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH9, &data, 1);
            threshold |= (data & 0xF0);            
            break;

        case BCI_EOC_VBAT:
            // get access to BCIMFTH6
            data = KEY_BCIMFTH8;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);
            threshold = (data & 0xF0) >> 4;
            break;            
        }   

    DEBUGMSG(ZONE_FUNCTION, (L"-GetCurrentThreshold()\r\n"));
    return threshold;    
}


//------------------------------------------------------------------------------
// Function: GetVoltageThreshold
//
// Desc: gets the overvoltage threshold for a voltage domain
//
static UINT 
GetVoltageThreshold(
    Device_t *pDevice, 
    DWORD ffMask
    )
{
    UINT8 data = 0;
    DEBUGMSG(ZONE_FUNCTION, (L"+GetVoltageThreshold("
        L"pDevice=0x%08X, ffMask=%d)\r\n",
        pDevice, ffMask
        ));

    // get access to BCIMFTH3
    data = KEY_BCIMFTH3;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // get contents
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH3, &data, 1);
    switch (ffMask)
        {
        case BCI_OV_VAC:
            data = (data & ACCHGOVTH) >> 2;
            break;

        case BCI_OV_VBUS:
            data = (data & VBUSOVTH);
            break;
            
        case BCI_OV_VBAT:
            data = (data & VBATOVTH) >> 4;
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-GetVoltageThreshold()\r\n"));
    return data;    
}


//------------------------------------------------------------------------------
// Function: GetOverVoltageThreshold
//
// Desc: gets the overvoltage threshold for a voltage domain
//
static BOOL
GetTemperatureThreshold(
    Device_t *pDevice, 
    DWORD     ffMask,
    UINT8    *pThresholdHigh,
    UINT8    *pThresholdLow
    )
{
    UINT8 data = 0;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+GetTemperatureThreshold("
        L"pDevice=0x%08X, ffMask=%d)\r\n",
        pDevice, ffMask
        ));

    if (pThresholdHigh == NULL || pThresholdLow == NULL) goto cleanUp;

    switch (ffMask)
        {
        case BCI_TEMP_VBAT1:
            // get access to BCIMFTH4
            data = KEY_BCIMFTH4;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH4, 
                pThresholdLow, 1
                );
            
            // get access to BCIMFTH5
            data = KEY_BCIMFTH5;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH5, 
                pThresholdHigh, 1
                );            
            bResult = TRUE;
            break;

        case BCI_TEMP_VBAT2:
            // get access to BCIMFTH6
            data = KEY_BCIMFTH6;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH6, 
                pThresholdLow, 1
                );

            // get access to BCIMFTH7
            data = KEY_BCIMFTH7;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH7, 
                pThresholdHigh, 1
                );  
            bResult = TRUE;
            break;            
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-GetTemperatureThreshold()\r\n"));
    return bResult;    
}


//------------------------------------------------------------------------------
// Function: SetCurrentThreshold
//
// Desc: sets the charge current thresholds
//
static BOOL 
SetCurrentThreshold(
    Device_t *pDevice, 
    DWORD ffMask,
    UINT8 threshold
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetCurrentThreshold("
        L"pDevice=0x%08X, ffMask=%d, threshold=%d,\r\n",
        pDevice, ffMask, threshold
        ));

    switch (ffMask)
        {
        case BCI_OC_VBAT:
            // get access to BCIMFTH8
            data = KEY_BCIMFTH8;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);

            data = (data & 0xF0) | (threshold & 0x0F);
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);
            
            // get access to BCIMFTH9
            data = KEY_BCIMFTH9;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH9, &data, 1);

            data = (data & 0x0F) | (threshold & 0xF0);
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH9, &data, 1);           
            bResult = TRUE;
            break;

        case BCI_EOC_VBAT:
            // get access to BCIMFTH6
            data = KEY_BCIMFTH8;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);
            TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);

            data = (data & 0x0F) | (threshold << 4);
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH8, &data, 1);
            bResult = TRUE;
            break;            
        }

    if (bResult != FALSE)
        {
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH3, &data, 1);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-SetCurrentThreshold()\r\n"));
    return bResult;    
}


//------------------------------------------------------------------------------
// Function: SetTemperatureThreshold
//
// Desc: sets the temperature thresholds
//
static BOOL 
SetTemperatureThreshold(
    Device_t *pDevice, 
    DWORD ffMask,
    UINT8 thresholdHigh,
    UINT8 thresholdLow
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetTemperatureThreshold("
        L"pDevice=0x%08X, ffMask=%d, thresholdHigh=%d, thresholdLow=%d)\r\n",
        pDevice, ffMask, thresholdHigh, thresholdLow
        ));

    switch (ffMask)
        {
        case BCI_TEMP_VBAT1:
            // get access to BCIMFTH4
            data = KEY_BCIMFTH4;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH4, 
                &thresholdLow, 1
                );
            
            // get access to BCIMFTH5
            data = KEY_BCIMFTH5;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH5, 
                &thresholdHigh, 1
                );            
            bResult = TRUE;
            break;

        case BCI_TEMP_VBAT2:
            // get access to BCIMFTH6
            data = KEY_BCIMFTH6;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH6, 
                &thresholdLow, 1
                );

            // get access to BCIMFTH7
            data = KEY_BCIMFTH7;
            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

            TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH7, 
                &thresholdHigh, 1
                );  
            bResult = TRUE;
            break;            
        }

    if (bResult != FALSE)
        {
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH3, &data, 1);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-SetTemperatureThreshold()\r\n"));
    return bResult;    
}


//------------------------------------------------------------------------------
// Function: SetVoltageThreshold
//
// Desc: sets the overvoltage threshold for a voltage domain
//
static BOOL 
SetVoltageThreshold(
    Device_t *pDevice, 
    DWORD ffMask,
    UINT8 threshold
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetVoltageThreshold("
        L"pDevice=0x%08X, ffMask=%d, threshold=%d)\r\n",
        pDevice, ffMask, threshold
        ));

    // make sure the value is valid
    if (threshold > 3) goto cleanUp;

    // get access to BCIMFTH3
    data = KEY_BCIMFTH3;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // get contents
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFTH3, &data, 1);
    switch (ffMask)
        {
        case BCI_OV_VAC:
            pDevice->dwACCHGOVTH = _rgACCHGOVTH[threshold];
            data = (data & ~ACCHGOVTH) | (threshold << 2);
            bResult = TRUE;
            break;

        case BCI_OV_VBUS:
            pDevice->dwVBUSOVTH = _rgVBUSOVTH[threshold];
            data = (data & ~VBUSOVTH) | (threshold);
            bResult = TRUE;
            break;
            
        case BCI_OV_VBAT:
            pDevice->dwVBATOVTH = _rgVBATOVTH[threshold];
            data = (data & ~VBATOVTH) | (threshold << 4);
            bResult = TRUE;
            break;
        }

    if (bResult != FALSE)
        {
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFTH3, &data, 1);
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SetVoltageThreshold()\r\n"));
    return bResult;    
}


//------------------------------------------------------------------------------
// Function: SetCurrentMonitors
//
// Desc: enables/disables over current monitoring
//
static BOOL 
SetCurrentMonitors(
    Device_t *pDevice, 
    DWORD ffEnable, 
    DWORD ffMask
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetCurrentMonitors("
        L"pDevice=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        pDevice, ffEnable, ffMask
        ));

    if (ffMask & (BCI_OC_VBAT | BCI_EOC_VBAT))
        {
        // get access to BCIMFEN3
        data = KEY_BCIMFEN3;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

        // update over-charge monitors
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN3, &data, 1);
        if (ffMask & BCI_OC_VBAT)
            {
            data = (ffEnable & BCI_OC_VBAT) ? (data | ICHGHIGHEN) :
                                              (data & ~ICHGHIGHEN);
            }

        if (ffMask & BCI_EOC_VBAT)
            {
            data = (ffEnable & BCI_EOC_VBAT) ? (data | ICHGEOCEN) :
                                               (data & ~ICHGEOCEN);
            }
            
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN3, &data, 1);
        }

    bResult = TRUE;
            
    DEBUGMSG(ZONE_FUNCTION, (L"-SetCurrentMonitors()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: SetPrescale
//
// Desc: sets the prescale value for the BCI module
//
static void 
SetPrescale(
    Device_t *pDevice, 
    DWORD preScale
    )
{   
    UINT8 data;
    UINT8 bcictl1;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetPrescale("
        L"pDevice=0x%08X, preScale)\r\n",
        pDevice, preScale
        ));

    // update charge current scaler
    TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &bcictl1, 1);
    bcictl1 = (preScale > 1) ? bcictl1 | CGAIN : bcictl1 & ~CGAIN;
    
    // need to pull the BCI module out of autoac mode before 
    // setting prescale value    
    data = KEY_BCIOFF;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMDKEY, &data, 1);
        
    // update scaling mode
    TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &bcictl1, 1);

    // enable auto charge
    data = CONFIG_DONE | BCIAUTOAC;
    TWLWriteRegs(pDevice->hTwl, TWL_BOOT_BCI, &data, 1);

    pDevice->preScale = preScale;

    DEBUGMSG(ZONE_FUNCTION, (L"-SetPrescale()\r\n"));
}


//------------------------------------------------------------------------------
// Function: SetTemperatureMonitors
//
// Desc: enables/disables temperature monitoring
//
static BOOL 
SetTemperatureMonitors(
    Device_t *pDevice, 
    DWORD ffEnable, 
    DWORD ffMask
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetTemperatureMonitors("
        L"pDevice=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        pDevice, ffEnable, ffMask
        ));

    // get access to BCIMFEN2
    data = KEY_BCIMFEN2;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

    // update over-charge monitors
    TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
    if (ffMask & BCI_TEMP_VBAT1)
        {
        data = (ffEnable & BCI_TEMP_VBAT1) ? (data | TBATOR1EN) :
                                             (data & ~TBATOR1EN);
        }

    if (ffMask & BCI_TEMP_VBAT2)
        {
        data = (ffEnable & BCI_TEMP_VBAT2) ? (data | TBATOR2EN) :
                                             (data & ~TBATOR2EN);
        }
        
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);

    bResult = TRUE;
            
    DEBUGMSG(ZONE_FUNCTION, (L"-SetOverVoltageMonitors()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: UpdateChargeCurrent
//
// Desc: updates the charge current.
//
static BOOL
UpdateChargeCurrent(
    Device_t *pDevice,
    UINT16    chargeCurrent
    )
{
    UCHAR bcimfkey = KEY_BCIMFTH9;
    UINT8 regVal=0;
    BOOL bSuccess = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (L"+UpdateChargeCurrent("
        L"pDevice=0x%08X, chargeCurrent=%d)\r\n",
        pDevice, chargeCurrent
        ));

    // update charge current
    //
    if (TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &bcimfkey, 1) == FALSE)
        {
        goto cleanUp;
        }
    
    regVal = (UINT8) (chargeCurrent & 0xFF);
    if (TWLWriteRegs(pDevice->hTwl, TWL_BCIIREF1, &regVal, 1) == FALSE)
        {
        goto cleanUp;
        }


    if (TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &bcimfkey, 1) == FALSE)
        {
        goto cleanUp;
        }

    regVal = (UINT8) (chargeCurrent >> 8);
    if (TWLWriteRegs(pDevice->hTwl, TWL_BCIIREF2, &regVal, 1) == FALSE)
        {
        goto cleanUp;
        }

    bSuccess = TRUE;

cleanUp:
    return bSuccess;
}


//------------------------------------------------------------------------------
// Function: SetVoltageMonitors
//
// Desc: sets the overcharge monitors.  Internally keeps track of which 
//       monitors are enabled by setting MSB if disabled and clearing
//       MSB if enabled.
//
static BOOL 
SetVoltageMonitors(
    Device_t *pDevice, 
    DWORD ffEnable, 
    DWORD ffMask
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetVoltageMonitors("
        L"pDevice=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        pDevice, ffEnable, ffMask
        ));

    // over-charge monitoring for usb and ac
    if (ffMask & (BCI_OV_VAC | BCI_OV_VBUS))
        {
        // get access to BCIMFEN2
        data = KEY_BCIMFEN2;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

        // update over-charge monitors
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
        if (ffMask & BCI_OV_VAC)
            {
            if (ffEnable & BCI_OV_VAC)
                {
                data |= ACCHGOVEN;
                pDevice->dwACCHGOVTH &= ~THRESHOLD_DISABLED;
                }
            else
                {
                data &= ~ACCHGOVEN;
                pDevice->dwACCHGOVTH |= THRESHOLD_DISABLED;
                }
            }

        if (ffMask & BCI_OV_VBUS)
            {
            if (ffEnable & BCI_OV_VBUS)
                {
                data |= VBUSOVEN;
                pDevice->dwVBUSOVTH &= ~THRESHOLD_DISABLED;
                }
            else
                {
                data &= ~VBUSOVEN;
                pDevice->dwVBUSOVTH |= THRESHOLD_DISABLED;
                }
            }

        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN2, &data, 1);
        }

    // over-charge monitoring for battery
    if (ffMask & BCI_OV_VBAT)
        {
        // get access to BCIMFEN4
        data = KEY_BCIMFEN4;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFKEY, &data, 1);

        // update over-charge monitors
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFEN4, &data, 1);
        if (ffEnable & BCI_OV_VBAT)
            {
            data |= VBATOVEN;
            pDevice->dwVBATOVTH &= ~THRESHOLD_DISABLED;
            }
        else
            {
            data &= ~VBATOVEN;
            pDevice->dwVBATOVTH |= THRESHOLD_DISABLED;
            }
                                          
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMFEN4, &data, 1);
        }

    bResult = TRUE;
            
    DEBUGMSG(ZONE_FUNCTION, (L"-SetVoltageMonitors()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: SetChargeMode_ACCharge
//
// Desc: puts the device into AC charging mode
//
static BOOL 
SetChargeMode_ACCharge(
    Device_t *pDevice
    )
{
    UINT8 data;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetChargeMode_ACCharge(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // check if already in auto mode
    TWLReadRegs(pDevice->hTwl, TWL_BOOT_BCI, &data, 1);
    if ((data & BCIAUTOAC) == 0)
        {
        // disable charge mode
        data = KEY_BCIOFF;
        TWLWriteRegs(pDevice->hTwl, TWL_BCIMDKEY, &data, 1);

        // enable auto charge
        data = CONFIG_DONE | BCIAUTOAC;
        TWLWriteRegs(pDevice->hTwl, TWL_BOOT_BCI, &data, 1);
        }

    UpdateChargeCurrent(pDevice, pDevice->acChargeCurrent);
            
    DEBUGMSG(ZONE_FUNCTION, (L"-SetChargeMode_ACCharge()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: SetChargeMode_NoCharge
//
// Desc: puts the device into a non-charging mode
//
static BOOL 
SetChargeMode_NoCharge(
    Device_t 
    *pDevice
    )
{
    UINT8 data;
    BOOL bResult;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetChargeMode_NoCharge(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // rely on HW FSM to manage charge states when nothing is plugged in
    //bResult = SetChargeMode_ACCharge(pDevice);
    // disable charge mode
    data = KEY_BCIOFF;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMDKEY, &data, 1);
    bResult = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (L"-SetChargeMode_NoCharge()\r\n"));
    return bResult;
}


//------------------------------------------------------------------------------
// Function: SetChargeMode_USBHost
//
// Desc: puts the device into usb host charging mode
//
static BOOL 
SetChargeMode_USBHost(
    Device_t *pDevice
    )
{
    UINT8 data;
    DWORD dwOVMon = 0;
    BOOL bResult = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+SetChargeMode_USBHost(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // disable auto mode
    data = CONFIG_DONE;
    TWLWriteRegs(pDevice->hTwl, TWL_BOOT_BCI, &data, 1);

    // disable hw monitoring functions
    dwOVMon = QueryVoltageMonitors(pDevice);
    SetVoltageMonitors(pDevice, 0, BCI_OV_VBAT | BCI_OV_VBUS | BCI_OV_VAC);

    UpdateChargeCurrent(pDevice, pDevice->usbChargeCurrent);    

    // put device in USB Linear charge mode
    data = KEY_BCIOFF;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMDKEY, &data, 1);

    // indicate USB-host charge
    data = USBSLOWMCHG;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMFSTS4, &data, 1);

    data = KEY_BCIUSBLINEAR;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIMDKEY, &data, 1);

    // disable watchdog
    data = KEY_BCIWDKEY5;
    TWLWriteRegs(pDevice->hTwl, TWL_BCIWDKEY, &data, 1);

    // re-enable monitoring functions
    if ((pDevice->dwACCHGOVTH & THRESHOLD_DISABLED) == 0) dwOVMon |= BCI_OV_VAC;
    if ((pDevice->dwVBUSOVTH & THRESHOLD_DISABLED) == 0) dwOVMon |= BCI_OV_VBUS;
    if ((pDevice->dwVBATOVTH & THRESHOLD_DISABLED) == 0) dwOVMon |= BCI_OV_VBAT;
    SetVoltageMonitors(pDevice, dwOVMon, BCI_OV_VBAT | BCI_OV_VBUS | BCI_OV_VAC);

    TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
    data |= MESVBUS;
    TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
        
    DEBUGMSG(ZONE_FUNCTION, (L"-SetChargeMode_USBHost()\r\n"));
    return bResult;
}

//------------------------------------------------------------------------------
// Function: BCI_Initialize
//
// Desc: Initializes the BCI library
//          - set overcharge values
//          - set power levels
//
HANDLE 
BCI_Initialize(
    DWORD   nEOCTimeout,
    DWORD   nOverChargeTimeout,
    DWORD   nOverVoltageTimeout,
    DWORD   nOOBTemperatureTimeout
    )
{
    Device_t *pDevice;
    BOOL bSuccess = FALSE;
    HKEY hKey;
    DWORD m_dwTempSel;
    DWORD size;
    DWORD dwType = REG_DWORD;
    UINT8 regval;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_Initialize()\r\n"));
    UNREFERENCED_PARAMETER(nOverVoltageTimeout);
    
    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed allocate BCI structure\r\n"
            ));
        goto cleanUp;
        }

    // initialize memory
    memset(pDevice, 0, sizeof(Device_t));

    // copy timeout values
    pDevice->nEndOfChargeTimeout = nEOCTimeout;
    pDevice->nOverChargeTimeout = nOverChargeTimeout;
    pDevice->nOOBTemperatureTimeout = nOOBTemperatureTimeout;
    pDevice->nOverVoltageTimeout = nOverChargeTimeout;
    
    // initialize data structure
    InitializeCriticalSection(&pDevice->cs);
    pDevice->currentMode = kBCI_Unknown;

    // set to default charge current levels (mA)
    pDevice->preScale = 1;
    BCI_SetChargeCurrent(pDevice, kBCI_AC, BCI_DEFAULT_ACCHARGECURRENT);
    BCI_SetChargeCurrent(pDevice, kBCI_USBHost, BCI_DEFAULT_USBCHARGECURRENT);

    // open handle to Triton driver
    pDevice->hTwl = TWLOpen();
    if ( pDevice->hTwl == NULL )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed open Triton device driver\r\n"
            ));
        goto cleanUp;
        }
    
    // open handle to Triton driver
    pDevice->hMadc = MADCOpen();
    if ( pDevice->hMadc == NULL )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to open MADC device driver\r\n"
            ));
        goto cleanUp;
        }

    // create a named event to signal battery of state change
    pDevice->hBattSignal = CreateEvent(NULL, FALSE, FALSE, BATTERYSINGAL_NAMED_EVENT);
    DEBUGMSG(pDevice->hBattSignal == NULL, (L"ERROR: BCI_Initialize: "
        L"Failed to open battery named event (%s)", BATTERYSINGAL_NAMED_EVENT)
        );


    // Create interrupt event
    pDevice->rgIntrEvents[EVENT_POWER] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->rgIntrEvents[EVENT_POWER] == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed create interrupt Power event\r\n"
            ));
        goto cleanUp;
        }

    // Create interrupt event
    pDevice->rgIntrEvents[EVENT_BCI] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->rgIntrEvents[EVENT_BCI] == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed create interrupt BCI event\r\n"
            ));
        goto cleanUp;
        }

    // register for AC charger interrupts
    if (!TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_CHG_PRES, 
        pDevice->rgIntrEvents[EVENT_POWER]))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed associate event with TWL POWER interrupt\r\n"
            ));
        goto cleanUp;
        }
  
    // Enable AC charger event
    if (!TWLInterruptMask(pDevice->hTwl, TWL_INTR_CHG_PRES, FALSE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to enable TWL POWER interrupt\r\n"
            ));
        goto cleanUp;
        }

    // register for bci interrupts
    if (!TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_ICHGHIGH, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_ICHGLOW, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_ICHGEOC, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_TBATOR2, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_TBATOR1, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_BATSTS, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_VBATLVL, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_VBATOV, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_VBUSOV, pDevice->rgIntrEvents[EVENT_BCI]) ||
        !TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_ACCHGOV, pDevice->rgIntrEvents[EVENT_BCI])
        )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed associate event with TWL BCI interrupt\r\n"
            ));
        goto cleanUp;
        }
  
    // Enable BCI events
    if (!TWLInterruptMask(pDevice->hTwl, TWL_INTR_ICHGHIGH, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_ICHGLOW, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_ICHGEOC, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_TBATOR2, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_TBATOR1, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_BATSTS, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_VBATLVL, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_VBATOV, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_VBUSOV, FALSE) ||
        !TWLInterruptMask(pDevice->hTwl, TWL_INTR_ACCHGOV, FALSE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to enable TWL BCI interrupt\r\n"
            ));
        goto cleanUp;
        }

    //Open registry to read the configuration of Hot-die interrupt temperature selection
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPWSTR)REG_BATTERY_DRV_PATH, 0, 0, &hKey))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to open Battery registry!\r\n"
            ));
        goto cleanUp;
        }
 
    size=sizeof(m_dwTempSel);
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"HotDieTempSel", 0, &dwType, (BYTE*)&m_dwTempSel, &size))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to read HotDieTempSel configuration in Battery registry!\r\n"
            ));
        RegCloseKey(hKey);
        goto cleanUp;
        }
        RegCloseKey(hKey);
    
    //Configure the hot-die detector for operation in its lowest temperature range
    TWLReadRegs(pDevice->hTwl, TWL_MISC_CFG , &regval, sizeof(regval));
    regval |=(((UINT8)m_dwTempSel)<<6);
    TWLWriteRegs(pDevice->hTwl, TWL_MISC_CFG , &regval, sizeof(regval));

    //Ensure that the hot-die interrupt in _int1  is not masked
    TWLReadRegs(pDevice->hTwl, TWL_PWR_IMR1 , &regval, sizeof(regval)); 
    regval &=~(HOT_DIE_MASK_INT);
    TWLWriteRegs(pDevice->hTwl, TWL_PWR_IMR1 , &regval, sizeof(regval));

    //Ensure that the hot-die interrupt in _int2 is masked
    TWLReadRegs(pDevice->hTwl, TWL_PWR_IMR2 , &regval, sizeof(regval)); 
    regval |=HOT_DIE_MASK_INT;
    TWLWriteRegs(pDevice->hTwl, TWL_PWR_IMR2 , &regval, sizeof(regval));

    //Ensure that the hot-die detector rising and falling threshold is active
    TWLReadRegs(pDevice->hTwl, TWL_PWR_EDR2 , &regval, sizeof(regval));
    regval |=HOT_DIE_RISING | HOT_DIE_FALLING;
    TWLWriteRegs(pDevice->hTwl, TWL_PWR_EDR2 , &regval, sizeof(regval));

    // Create interrupt event
    pDevice->rgIntrEvents[EVENT_HOTDIE]=CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->rgIntrEvents[EVENT_HOTDIE]== NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed create interrupt Power (Hot die) event\r\n"
            ));
        
        goto cleanUp;
        }

    // Create interrupt event
    hHotDieTriggerEvent_Test=CreateEvent(NULL, FALSE, FALSE, HOTDIETESTSIGNAL_NAMED_EVENT);
    if (hHotDieTriggerEvent_Test== NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed create interrupt Power (Hot die) event \r\n"
            ));
       
        goto cleanUp;
        }
 
    // register for hot die interrupts
    if (!TWLInterruptInitialize(pDevice->hTwl, TWL_INTR_HOT_DIE, 
        pDevice->rgIntrEvents[EVENT_HOTDIE]))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed associate event with TWL POWER (Hot die) interrupt\r\n"
            ));
        goto cleanUp;
        }
     
    // Enable RTC event
    if (!TWLInterruptMask(pDevice->hTwl, TWL_INTR_HOT_DIE, FALSE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed to enable TWL POWER (Hot die)interrupt\r\n"
            ));
        goto cleanUp;
        }   


    // Start interrupt service thread
    pDevice->threadsExit = FALSE;
    pDevice->hIntrThread = CreateThread(
        NULL, 0, IntrThread, pDevice, 0,NULL
        );
    if (!pDevice->hIntrThread)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: BCI_Initialize: "
            L"Failed create interrupt thread\r\n"
            ));
        goto cleanUp;
        }

    // Set thread priority
    pDevice->priority256 = 130;
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256); 

    // set threshold values to default
    pDevice->dwACCHGOVTH = _rgACCHGOVTH[ACCHGOVTH_DEFAULT];
    pDevice->dwVBATOVTH = _rgVBATOVTH[VBATOVTH_DEFAULT];
    pDevice->dwVBUSOVTH = _rgVBUSOVTH[VBUSOVTH_DEFAULT];

    // force update of battery state
    PulseEvent(pDevice->rgIntrEvents[EVENT_BCI]); 

    bSuccess = TRUE;

cleanUp:

    if (bSuccess == FALSE) 
        {
        BCI_Uninitialize(pDevice);
        pDevice = NULL;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_Initialize()\r\n"));    
    return (HANDLE)pDevice;
}


//------------------------------------------------------------------------------
// Function: BCI_Uninitialize
//
// Desc: Unitializes the BCI library
//
void 
BCI_Uninitialize(
    HANDLE hBCI
    )
{
    Device_t *pDevice;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_Uninitialize(hBCI=0x%08X)\r\n", hBCI));

    // check for valid parameter
    if (hBCI == NULL) return;
    pDevice = (Device_t*)hBCI;

    // Signal stop to threads
    pDevice->threadsExit = TRUE;

    // Close interrupt thread
    if (pDevice->hIntrThread != NULL)
        {
        // Set event to wake it
        SetEvent(pDevice->rgIntrEvents[EVENT_BCI]);
        // Wait until thread exits
        WaitForSingleObject(pDevice->hIntrThread, INFINITE);
        // Close handle
        CloseHandle(pDevice->hIntrThread);
        }

    if (pDevice->hBattSignal != NULL)
        {
        CloseHandle(pDevice->hBattSignal);
        }

    // release allocated resources
    if (pDevice->rgIntrEvents[EVENT_BCI] != NULL)
        {
        // register for bci interrupts
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_ICHGHIGH);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_ICHGLOW);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_ICHGEOC);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_TBATOR2);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_TBATOR1);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_BATSTS);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_VBATLVL);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_VBATOV);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_VBUSOV);
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_ACCHGOV);
        CloseHandle(pDevice->rgIntrEvents[EVENT_BCI]);
        }

    if (pDevice->rgIntrEvents[EVENT_POWER] != NULL)
        {
        TWLInterruptMask(pDevice->hTwl, TWL_INTR_CHG_PRES, TRUE);        
        CloseHandle(pDevice->rgIntrEvents[EVENT_POWER]);
        }

    if (pDevice->rgIntrEvents[EVENT_HOTDIE] != NULL)
        {
        TWLInterruptMask(pDevice->hTwl, TWL_INTR_HOT_DIE, TRUE);
        // Unregister for Hot-Die detector interrupts
        TWLInterruptDisable(pDevice->hTwl, TWL_INTR_HOT_DIE);
        CloseHandle(pDevice->rgIntrEvents[EVENT_HOTDIE]);
        }
    if (hHotDieTriggerEvent_Test!=NULL)
        {
        CloseHandle(hHotDieTriggerEvent_Test);
        }
    
    // close all handles
    CloseHandle(pDevice->hTwl);

    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);

    LocalFree(pDevice);
    
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_Uninitialize()\r\n"));
}

//------------------------------------------------------------------------------
// Function: BCI_GetBatteryVoltage
//
// Desc: Gets current battery voltage
//
DWORD 
BCI_GetBatteryVoltage(
    HANDLE hBCI
    )
{
    DWORD data = 0;
    DWORD volts;
    Device_t *pDevice = (Device_t*)hBCI;
    
    // return current battery voltage
    MADCReadValue(pDevice->hMadc, MADC_CHANNEL_BCI4, &data, 1);
    MADCConvertToVolts(pDevice->hMadc, MADC_CHANNEL_BCI4, &data, &volts, 1);

    return volts;
}

//------------------------------------------------------------------------------
// Function: BCI_GetBatteryTemperature
//
// Desc: Gets current battery temperature
//
DWORD 
BCI_GetBatteryTemperature(
    HANDLE hBCI
    )
{
    DWORD data = 0;
    DWORD temp;
    Device_t *pDevice = (Device_t*)hBCI;
    
    // return current battery voltage
    MADCReadValue(pDevice->hMadc, MADC_CHANNEL_BCI0, &data, 1);
    MADCConvertToVolts(pDevice->hMadc, MADC_CHANNEL_BCI0, &data, &temp, 1);

    return temp;
}

//------------------------------------------------------------------------------
// Function: BCI_SetTimeouts
//
// Desc: sets the timeouts
//
BOOL 
BCI_SetTimeouts(
    HANDLE  hBCI,
    DWORD   nEOCTimeout,
    DWORD   nOverChargeTimeout,
    DWORD   nOverVoltageTimeout,
    DWORD   nOOBTemperatureTimeout
    )
{
    BOOL bResult = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetTimeouts("
        L"nEOCTimeout=%d, nOverChargeTimeout=%d, nOverVoltageTimeout=%d"
        L" nOOBTemperatureTimeout=%d)\r\n", nEOCTimeout, nOverChargeTimeout,
        nOverVoltageTimeout, nOOBTemperatureTimeout)
        );

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    pDevice->nEndOfChargeTimeout = nEOCTimeout;
    pDevice->nOverChargeTimeout = nOverChargeTimeout;
    pDevice->nOverVoltageTimeout = nOverVoltageTimeout;
    pDevice->nOOBTemperatureTimeout = nOOBTemperatureTimeout;

    bResult = TRUE;
       
cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetTimeouts()\r\n"));
    return bResult;
}

//------------------------------------------------------------------------------
// Function: BCI_SetPrescale
//
// Desc: sets the timeouts
//
BOOL 
BCI_SetPrescale(
    HANDLE  hBCI,
    DWORD   preScale
    )
{
    BOOL bResult = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetPrescale("L"preScale=%d\r\n)", 
        preScale)
        );

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    SetPrescale(pDevice, preScale);

    bResult = TRUE;
       
cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetPrescale()\r\n"));
    return bResult;
}


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
    )
{
    UINT16 bciiref1;
    BOOL bResult = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DWORD maxChargeCurrent = BCI_MAX_CHARGECURRENT;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetChargeCurrent("
        L"mode=%d, chargeCurrent=%d)\r\n", mode, chargeCurrent)
        );

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    // check if prescalar is disabled, adjust max current accordingly
    if (pDevice->preScale < 2)
        {
        maxChargeCurrent >>= 1;
        }
    
    if (chargeCurrent > maxChargeCurrent)
        {
        DEBUGMSG(ZONE_WARN, (L"WARN: invalid charge current settings "
            L"%d mA, must be in range of [%d, %d]mA\r\n", chargeCurrent,
            0, maxChargeCurrent)
            );
        goto cleanUp;
        }

    // calculate new charge current
    bciiref1 = (UINT16)((float)chargeCurrent/BCI_DEFAULT_CHARGECURRENT_STEPS);

    // check for scaled values
    if (pDevice->preScale > 1)
        {
        bciiref1 >>= 1;
        }

    // account for maximas
    if (bciiref1 == BCI_MAX_BCIIREF1)
        {
        bciiref1 -= 1;
        }

    // always set high bit of BCIIREF1
    bciiref1 |= BCIREF1_HBIT;

    // save charge current info
    switch (mode)
        {
        case kBCI_AC:
            pDevice->acChargeCurrent = bciiref1;
            break;

        case kBCI_USBHost:
            pDevice->usbChargeCurrent = bciiref1;
            break;

        default:
            goto cleanUp;
        }

    // check current charge mode and apply new settings if applicable
    if (pDevice->currentMode == mode)
        {
        UpdateChargeCurrent(pDevice, bciiref1);
        }

    bResult = TRUE;
       
cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetChargeCurrent()\r\n"));
    return bResult;
}

//------------------------------------------------------------------------------
// Function: BCI_QueryVoltageMonitors
//
// Desc: enable/disable over charge monitors
//
DWORD 
BCI_QueryVoltageMonitors(
    HANDLE hBCI
    )
{   
    DWORD dwResult = 0;
    Device_t *pDevice = (Device_t*)hBCI;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_QueryVoltageMonitors()\r\n"));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    dwResult = QueryVoltageMonitors(pDevice);    
    LeaveCriticalSection(&pDevice->cs);
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_QueryVoltageMonitors()\r\n"));
    return dwResult;
}


//------------------------------------------------------------------------------
// Function: BCI_QueryTemperatureMonitors
//
// Desc: query the temperature monitors
//
DWORD
BCI_QueryTemperatureMonitors(
    HANDLE hBCI
    )
{
    DWORD dwResult = 0;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_QueryTemperatureMonitors(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    dwResult = QueryTemperatureMonitors(pDevice);    
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_QueryTemperatureMonitors()\r\n"));
    return dwResult;
}


//------------------------------------------------------------------------------
// Function: BCI_QueryCurrentMonitors
//
// Desc: query the current monitors
//
DWORD
BCI_QueryCurrentMonitors(
    HANDLE hBCI
    )
{
    DWORD dwResult = 0;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_QueryCurrentMonitors(pDevice=0x%08X)\r\n",
        pDevice
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    dwResult = QueryCurrentMonitors(pDevice);    
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_QueryCurrentMonitors()\r\n"));
    return dwResult;
}


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
    )
{   
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetVoltageMonitors()\r\n"));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetVoltageMonitors(pDevice, ffEnable, ffMask);    
    LeaveCriticalSection(&pDevice->cs);
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetVoltageMonitors()\r\n"));
    return bSuccess;
}


//------------------------------------------------------------------------------
// Function: BCI_Notify
//
// Desc: Sets the battery charging mode
//
BOOL 
BCI_SetChargeMode(
    HANDLE hBCI, 
    BatteryChargeMode_e mode,
    BOOL bAttached
    )
{   
    UINT8 data;
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetChargeMode()\r\n"));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);
    
    // look at requested mode and call correlating function
    switch (mode)
        {
        case kBCI_USBHost:
            if (bAttached != FALSE)
                {
                pDevice->bAttached_USBHost = TRUE;

                // enable resistive divide for usb
                TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
                data |= MESVBUS;
                TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
                }
            else
                {
                pDevice->bAttached_USBHost = FALSE;
                
                // disable resistive divide for usb
                TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
                data &= ~MESVBUS;
                TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &data, 1);
                }
            SetEvent(pDevice->rgIntrEvents[EVENT_POWER]);
            break;
        
        case kBCI_AC:
        case kBCI_Battery:
            // We update based on AC voltage readings
            SetEvent(pDevice->rgIntrEvents[EVENT_POWER]);
            break;

        default:
            bSuccess = FALSE;
        }

    LeaveCriticalSection(&pDevice->cs);
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetChargeMode()\r\n"));
    return bSuccess;
}


//------------------------------------------------------------------------------
// Function: BCI_GetChargeMode
//
// Desc: Gets current battery charging mode
//
BatteryChargeMode_e 
BCI_GetChargeMode(
    HANDLE hBCI
    )
{
    Device_t *pDevice;
    BatteryChargeMode_e mode = kBCI_Unknown;

    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_GetChargeMode(hBCI=0x%08X)\r\n", hBCI));

    // check for valid parameter
    if (hBCI != NULL)
        {
        pDevice = (Device_t*)hBCI;
        mode = pDevice->currentMode;
        }
    
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_GetChargeMode(mode=%d)\r\n", mode));
    return mode;
}


//------------------------------------------------------------------------------
// Function: BCI_GetVoltageThreshold
//
// Desc: gets over voltage threshold
//
UINT 
BCI_GetVoltageThreshold(
    HANDLE hBCI, 
    DWORD ffMask
    )
{   
    UINT dwResult = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_GetVoltageThreshold()\r\n"));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    dwResult = GetVoltageThreshold(pDevice, ffMask);    
    LeaveCriticalSection(&pDevice->cs);
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_GetVoltageThreshold()\r\n"));
    return dwResult;
}


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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetVoltageThreshold("
        L"hBCI=0x%08X, ffMask=%d, threshold=%d)\r\n",
        pDevice, ffMask, threshold
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetVoltageThreshold(pDevice, ffMask, threshold);    
    LeaveCriticalSection(&pDevice->cs);
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetVoltageThreshold()\r\n"));
    return bSuccess;
}


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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_GetTemperatureThreshold("
        L"pDevice=0x%08X, ffMask=%d)\r\n",
        pDevice, ffMask
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = GetTemperatureThreshold(pDevice, ffMask, 
                   pThresholdHigh, pThresholdLow
                   );
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_GetTemperatureThreshold()\r\n"));
    return bSuccess;    
}



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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetTemperatureThreshold("
        L"hBCI=0x%08X, ffMask=%d, thresholdHigh=%d, thresholdLow=%d)\r\n",
        hBCI, ffMask, thresholdHigh, thresholdLow
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetTemperatureThreshold(pDevice, ffMask, 
                   thresholdHigh, thresholdLow
                   );
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetTemperatureThreshold()\r\n"));
    return bSuccess;  

}


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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetTemperatureMonitors("
        L"hBCI=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        hBCI, ffEnable, ffMask
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetTemperatureMonitors(pDevice, ffEnable, ffMask);
    LeaveCriticalSection(&pDevice->cs);

cleanUp:            
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetTemperatureMonitors()\r\n"));
    return bSuccess;
}


//------------------------------------------------------------------------------
// Function: BCI_GetCurrentThreshold
//
// Desc: gets the current threshold for charging
//
UINT8
BCI_GetCurrentThreshold(
    HANDLE      hBCI, 
    DWORD       ffMasks
    )
{
    UINT8 Result = 0;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_GetCurrentThreshold("
        L"pDevice=0x%08X, ffMask=%d)\r\n",
        pDevice, ffMasks
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    Result = GetCurrentThreshold(pDevice, ffMasks);
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_GetCurrentThreshold()\r\n"));
    return Result;    
}



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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetCurrentThreshold("
        L"hBCI=0x%08X, ffMask=%d, threshold=%d)\r\n",
        hBCI, ffMask, threshold
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetCurrentThreshold(pDevice, ffMask, threshold);
    LeaveCriticalSection(&pDevice->cs);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetCurrentThreshold()\r\n"));
    return bSuccess;  

}


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
    )
{
    BOOL bSuccess = FALSE;
    Device_t *pDevice = (Device_t*)hBCI;
    DEBUGMSG(ZONE_FUNCTION, (L"+BCI_SetCurrentMonitors("
        L"hBCI=0x%08X, ffEnable=%d, ffMask=%d)\r\n",
        hBCI, ffEnable, ffMask
        ));

    // check for valid parameter
    if (pDevice == NULL) goto cleanUp;

    EnterCriticalSection(&pDevice->cs);    
    bSuccess = SetCurrentMonitors(pDevice, ffEnable, ffMask);
    LeaveCriticalSection(&pDevice->cs);

cleanUp:            
    DEBUGMSG(ZONE_FUNCTION, (L"-BCI_SetCurrentMonitors()\r\n"));
    return bSuccess;
}


//------------------------------------------------------------------------------
// Function: IsVACValid
//
// Desc: Checks to see if AC is connected with valid voltage
//
static BOOL 
IsVACValid(
    Device_t   *pDevice
    )
{
    UINT8 fMESOrig;
    UINT8 fMESOut;
    DWORD dwResult;
    DWORD dwVAC_mv = 0;
    DWORD dwVACReading;
    BOOL bValid = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+IsVACValid(pDevice=0x%08X)\r\n", pDevice));

    // first we need to enable the pre-scaler to get any measurements from madc
    TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOrig, 1);
    fMESOut = fMESOrig | MESVAC;
    TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOut, 1);

    // check if AC is connected
    dwResult = MADCReadValue(pDevice->hMadc, VAC_CHANNEL, &dwVACReading, 1);
    if (dwResult != 0)
        {
        // convert results to voltage
        MADCConvertToVolts(pDevice->hMadc, VAC_CHANNEL,
            &dwVACReading, &dwVAC_mv, 1);
        
        // is the result above the ac threshold?
        if (ACMINVOLT <= dwVAC_mv && dwVAC_mv < pDevice->dwACCHGOVTH) 
            {
            bValid = TRUE;
            }
        }

    TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOrig, 1);
           
    DEBUGMSG(ZONE_FUNCTION, (L"-IsVACValid()=%d\r\n", bValid));
    return bValid;
}


//------------------------------------------------------------------------------
// Function: IsVBUSValid
//
// Desc: Checks to see if USB is connected with valid voltage
//
static BOOL 
IsVBUSValid(
    Device_t   *pDevice
    )
{
    UINT8 fMESOrig;
    UINT8 fMESOut;
    DWORD dwResult;
    DWORD dwVBUS_mv = 0;
    DWORD dwVBUSReading;
    BOOL bValid = FALSE;
    DEBUGMSG(ZONE_FUNCTION, (L"+IsVBUSValid(pDevice=0x%08X)\r\n", pDevice));

    // check if notified of a USB connection
    if (pDevice->bAttached_USBHost != FALSE)
        {
        // first we need to enable the pre-scaler to get any measurements from madc
        TWLReadRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOrig, 1);
        fMESOut = fMESOrig | MESVBUS;
        TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOut, 1);
        
        // check if vbus is connected
        dwResult = MADCReadValue(pDevice->hMadc, VBUS_CHANNEL, &dwVBUSReading, 1);
        if (dwResult != 0)
            {
            // convert results to voltage
            MADCConvertToVolts(pDevice->hMadc, VBUS_CHANNEL,
                &dwVBUSReading, &dwVBUS_mv, 1);
            
            // is the result above the ac threshold?
            if (VBUSMINVOLT <= dwVBUS_mv && dwVBUS_mv < pDevice->dwVBUSOVTH) 
                {
                bValid = TRUE;
                }
            }

        TWLWriteRegs(pDevice->hTwl, TWL_BCICTL1, &fMESOrig, 1);
        }
      
    DEBUGMSG(ZONE_FUNCTION, (L"-IsVBUSValid()=%d\r\n", bValid));
    return bValid;
}

//------------------------------------------------------------------------------
//
//  Function:  IntrThread
//
//  This function acts as the IST for BCI interrupts.
//
DWORD
IntrThread(
    VOID *pContext
    )
{
    Device_t   *pDevice = (Device_t*)pContext;
    DWORD       timeout = INFINITE;
    UINT8       status;
    DWORD       dwEventId;
    
    BatteryChargeMode_e prevMode = kBCI_Unknown;


    // Loop until we are not stopped...
    while (!pDevice->threadsExit)
        {
        // Typically event handles associated with interrupts aren't allowed
        // to use WaitForMultipleObjects but since the interrupts are
        // actually signalled by the triton driver this will work.
        
        dwEventId = WaitForMultipleObjects(3, pDevice->rgIntrEvents, 
                        FALSE, timeout
                        );
        if (pDevice->threadsExit) break;

        switch (dwEventId)
            {
            case WAIT_OBJECT_0:
                TWLReadRegs(pDevice->hTwl, TWL_PWR_ISR1, &status, 1);                
                TWLWriteRegs(pDevice->hTwl, TWL_PWR_ISR1, &status, 1);
                if (timeout == INFINITE)
                    {
                    timeout = 100;
                    }
                DEBUGMSG(ZONE_PDD, (L"BCI: interrupt: status=0x%02X\r\n", status));
                continue;

            case WAIT_TIMEOUT:
                DEBUGMSG(ZONE_PDD, (L"BCI: timeout: %d ms\r\n", timeout));
                timeout = INFINITE;
                // fall-through

            case (WAIT_OBJECT_0 + 1):
                break;

            case (WAIT_OBJECT_0 + 2):
                RETAILMSG(1, (L"BCI: Receive Hot-Die warning interrupt !\r\n"));
                SetEvent(hHotDieTriggerEvent_Test);
                
                //Below actions to decrease the temperature of the Power IC should be done by OEM
                
                continue;

            default:
                continue;
            }        

        // always read/update status registers
        TWLReadRegs(pDevice->hTwl, TWL_BCIMFSTS2, pDevice->rgBciStatus, 3);
        DEBUGMSG(ZONE_PDD, (L"BCI: interrupt: "
            L"status[0]=%02X, status[1]=%02X, status[2]=%02X\r\n", 
            pDevice->rgBciStatus[0], pDevice->rgBciStatus[1],
            pDevice->rgBciStatus[2])
            );

        // choose correct charge mode to enter
        EnterCriticalSection(&pDevice->cs);
        // check for over-voltage, over-current, or over-temperature
        if (pDevice->rgBciStatus[IDX_BCIMFSTS2] & (VBUSOV | ACCHGOV))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: over voltage detected\r\n"));
            if (pDevice->currentMode == kBCI_USBHost)
                {
                // let hw FSM handle overcharges
                SetChargeMode_ACCharge(pDevice);
                pDevice->currentMode = kBCI_SurgeProtect;

                // wait a second and check voltages again
                timeout = pDevice->nOverVoltageTimeout;
                }
            }
        else if (pDevice->rgBciStatus[IDX_BCIMFSTS3] & (TBATOR1 | TBATOR2))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: OOB temperature detected\r\n"));
            if (pDevice->currentMode == kBCI_USBHost)
                {
                // let hw FSM handle overcharges
                SetChargeMode_ACCharge(pDevice);
                pDevice->currentMode = kBCI_SurgeProtect;

                // wait a second and check voltages again
                timeout = pDevice->nOOBTemperatureTimeout;
                }
            }
        else if (pDevice->rgBciStatus[IDX_BCIMFSTS3] & (ICHGHIGH))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: over/under charge current detected\r\n"));
            if (pDevice->currentMode == kBCI_USBHost)
                {
                // let hw FSM handle overcharges
                SetChargeMode_ACCharge(pDevice);
                pDevice->currentMode = kBCI_SurgeProtect;

                // wait a second and check voltages again
                timeout = pDevice->nOverChargeTimeout;
                }
            }
        else if (pDevice->rgBciStatus[IDX_BCIMFSTS3] & (ICHGEOC))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: end of charge detected\r\n"));
            if (pDevice->currentMode == kBCI_USBHost)
                {
                // let hw FSM handle overcharges
                SetChargeMode_ACCharge(pDevice);
                pDevice->currentMode = kBCI_EOC;

                // wait a second and check voltages again
                timeout = pDevice->nEndOfChargeTimeout;
                }
            }
        else if (IsVACValid(pDevice))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: Charging via VAC\r\n"));
            SetChargeMode_ACCharge(pDevice);
            pDevice->currentMode = kBCI_AC;
            }
        else if (IsVBUSValid(pDevice))
            {
            DEBUGMSG(ZONE_PDD, (L"BCI: Charging via USBHost\r\n"));
            SetChargeMode_USBHost(pDevice);
            pDevice->currentMode = kBCI_USBHost;
            }
        else
            {            
            DEBUGMSG(ZONE_PDD, (L"BCI: Not Charging\r\n"));
            SetChargeMode_ACCharge(pDevice);
            pDevice->currentMode = kBCI_Battery;
            }
        LeaveCriticalSection(&pDevice->cs);

        // signal battery driver of state change
        if (prevMode != pDevice->currentMode)
            {
            prevMode = pDevice->currentMode;
            if (pDevice->hBattSignal != NULL) SetEvent(pDevice->hBattSignal);
            }
        
        }

    return ERROR_SUCCESS;
}

