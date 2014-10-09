// Copyright (c) 2011, David Vescovi.  All Rights Reserved.
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
//  File:  oem_reg.c
//

//-------------------------------------------------------------------------------

#include <bsp.h>
#include <devload.h>
#include "args.h"
#include "bsp_oal.h"
#include "bsp_cfg.h"

//------------------------------------------------------------------------------
//  Local definition

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE          ((HKEY)(ULONG_PTR)0x80000002)
#endif

//------------------------------------------------------------------------------
static BOOL
SetDeviceDriverFlags(
    LPCWSTR szKeyPath,
    DWORD flags
    )
{
    LONG code;
    HKEY hKey;
    DWORD value;

    // Open/create key
    code = NKRegCreateKeyEx(
            HKEY_LOCAL_MACHINE, szKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value
            );          

    if (code != ERROR_SUCCESS) goto cleanUp;

    // Set value
    code = NKRegSetValueEx(
        hKey, L"Flags", 0, REG_DWORD, (UCHAR*)&flags, sizeof(DWORD)
        );

    // Close key
    NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}

static BOOL
SetDisplayDriver(
    DWORD angle
    )
{
    LONG code;
    HKEY hKey;
    DWORD value;
    LPCWSTR pszKeyPath = L"\\System\\GDI\\ROTATION";

    // Open/create key
    code = NKRegCreateKeyEx(
            HKEY_LOCAL_MACHINE, pszKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value
            );          

    if (code != ERROR_SUCCESS) goto cleanUp;

    // Set value  ... force portrait mode
    code = NKRegSetValueEx(
        hKey, L"Angle", 0, REG_DWORD, (UCHAR*)&angle, sizeof(DWORD)
        );

	// Close key
    NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}


//------------------------------------------------------------------------------

VOID OEMRegInit()
{
	UINT8 disp;
	
	disp = *(UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_DISP_RES);
	if (OMAP_LCD_480W_272H == disp || OMAP_LCD_800W_480H4D == disp)
	{
		SetDeviceDriverFlags(
            L"\\Drivers\\BuiltIn\\Keypad", DEVFLAGS_NONE
            );
		// can not use these drivers as pads used for buttons on display board
		SetDeviceDriverFlags(
            L"\\Drivers\\BuiltIn\\UART2", DEVFLAGS_NOLOAD	//COM1
            );
		SetDeviceDriverFlags(
            L"\\Drivers\\BuiltIn\\UART3", DEVFLAGS_NOLOAD	//COM2
            );
	}
	else
	{
		SetDeviceDriverFlags(
            L"\\Drivers\\BuiltIn\\Keypad", DEVFLAGS_NOLOAD
            );
	}

	//if (OMAP_LCD_480W_272H == *(UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_DISP_RES))
	//{
	//	SetDisplayDriver(0x5a);	// 5a = 90 degrees (portrait)
	//}
 //   else
 //   {
	//	SetDisplayDriver(0);	// 0 = 0 degrees (landscape)
 //   }
}
