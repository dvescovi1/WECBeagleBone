// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File: usbhpdd.c
//
//  Platform specific funtionality for the USB Host driver.
//

#pragma warning(push)
#pragma warning(disable: 4115)
#include <windows.h>
#include <dbgapi.h>
#include <ohcdddsi.h>
#pragma warning(pop)

BOOL USBHPDD_PowerVBUS(BOOL bPowerOn);

//------------------------------------------------------------------------------
//
//  Function:  USBHPDD_Init
//
//  Initialize the USB host PDD.
//
BOOL USBHPDD_Init(void)
{
    BOOL rc = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (L"+USBHPDD_Init: Entry\r\n"));

    // Start with VBUS power off
    rc = USBHPDD_PowerVBUS(FALSE);

    DEBUGMSG(ZONE_FUNCTION, (L"-USBHPDD_Init: rc %d\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  USBHPDD_PowerVBUS
//
//  Power the USB VBUS on or off
//
BOOL USBHPDD_PowerVBUS(BOOL bPowerOn)
{
	UNREFERENCED_PARAMETER(bPowerOn);

    DEBUGMSG(ZONE_FUNCTION, (L"USBHPDD_PowerVBUS(): bPowerOn %d\r\n", bPowerOn));

    return TRUE;
}

//------------------------------------------------------------------------------
