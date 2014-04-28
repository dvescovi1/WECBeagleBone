// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File: usbfnpdd.c
//
//  Platform specific funtionality for the USB Function driver.
//

#pragma warning(push)
#pragma warning(disable:4115 4214)
#include <windows.h>
#include <usbfn.h>

#include "bsp.h"
#include "am33x_usb.h"
#include "am33x_usbcdma.h"
#pragma warning(pop)

//------------------------------------------------------------------------------
//
//  Function:  USBFNPDD_PowerVBUS
//
//  Power the USB VBUS on or off
//
BOOL USBFNPDD_PowerVBUS(BOOL bPowerOn)
{
	UNREFERENCED_PARAMETER(bPowerOn);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  USBFNPDD_Init
//
//  Initialize the USB function PDD.
//
BOOL USBFNPDD_Init(void)
{
    BOOL rc = TRUE;

    // Start with VBUS power off
    USBFNPDD_PowerVBUS(FALSE);

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  USBFNPDD_WaitForAPIsReady
//
//  This function should block execution (normally using
//  WaitForAPIReady() API function) until the API sets
//  the function driver depends on are ready.
//
VOID USBFNPDD_WaitForAPIsReady()
{
    // Add/remove as appropriate
    // WaitForAPIReady(SH_COMM, INFINITE);
    // WaitForAPIReady(SH_TAPI, INFINITE);
    // WaitForAPIReady(SH_WMGR, INFINITE);    
}

//------------------------------------------------------------------------------


