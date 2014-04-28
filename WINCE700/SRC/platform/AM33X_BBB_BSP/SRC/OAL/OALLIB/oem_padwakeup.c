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
//  File: oem_padwakeup.c
//
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
//#include <gpio.h>
#include <bsp.h>

//-----------------------------------------------------------------------------
//  determines if the context for a particular register set need to be saved
extern UINT32                      g_ffContextSaveMask;

void OEMEnableIOPadWakeup( DWORD   gpio, BOOL    bEnable )
//  Enable/Disable IO PAD wakeup capability for a particular GPIO
{
}

//------------------------------------------------------------------------------
DWORD OEMGetIOPadWakeupStatus()
//  Returns number of the GPIO which received IO PAD wakeup
{
    return 0/*(DWORD)GPIO_0*/;
}
