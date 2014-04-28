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
//  File:  devloadex.h
//
//  This file contains OMAP specific ceddk extensions.
//
#ifndef _DEVLOADEX_H_
#define _DEVLOADEX_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// indicates device driver is a omap specific device driver
//
#define DEVICEID_VALNAME                    L"DriverId"     // driver id (optional)


//
// These are the optional values under a device key.
//
#define DEVLOADEX_POWERFLAGS_VALNAME        L"PowerFlags"   // power mask

//
// Flag values.
//
#define POWERFLAGS_NONE                     0x00000000      // No flags defined
#define POWERFLAGS_PRESTATECHANGENOTIFY     0x00000001      // send notification on pre-device state change
#define POWERFLAGS_POSTSTATECHANGENOTIFY    0x00000002      // send notification on post-device state change
#define POWERFLAGS_CONTEXTRESTORE           0x00000100      // device recieves context restore notifiation


//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
#endif //_DEVLOADEX_H_
