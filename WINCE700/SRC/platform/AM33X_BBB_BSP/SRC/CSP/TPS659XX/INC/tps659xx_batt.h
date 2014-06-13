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
//  File: tps659xx_batt.h
//

#ifndef __TPS659XX_BATT_H
#define __TPS659XX_BATT_H

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERYSINGAL_NAMED_EVENT       (L"SSUpdatePower")
#define HOTDIETESTSIGNAL_NAMED_EVENT    (L"HotDieDetect")
//------------------------------------------------------------------------------
// battery charge constants
enum {
    BATTERY_USBHOST_DISCONNECT = 0,
    BATTERY_USBHOST_CONNECT,
    
};

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif


