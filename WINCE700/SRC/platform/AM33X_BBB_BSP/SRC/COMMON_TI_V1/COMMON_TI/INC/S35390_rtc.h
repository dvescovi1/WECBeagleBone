// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  s35390_rtc.h
//
#ifndef __S35390_RTC_H
#define __S35390_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------
// Defines
#define RTC_RESOLUTION_MS 60000

//------------------------------------------------------------------------------
//  RTC hardware initialization
//------------------------------------------------------------------------------
BOOL OALS35390RTCInit(OMAP_DEVICE i2cdev,DWORD i2cBaudIndex,UINT16 slaveAddress, BOOL fEnableAsWakeUpSource);

//------------------------------------------------------------------------------
//  RTC interrupt
//------------------------------------------------------------------------------
DWORD BSPGetRTCGpioIrq();

//------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __S35390_RTC_H

