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
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  kitlreg.c
//

//-------------------------------------------------------------------------------

#include <bsp.h>
#include <devload.h>

//------------------------------------------------------------------------------
//  Local definition

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE          ((HKEY)(ULONG_PTR)0x80000002)
#endif

static BOOL SetDeviceGroup(LPCWSTR szKeyPath, LPCWSTR groupName )
{
    LONG code;
    HKEY hKey;
    DWORD value;

    code = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value );          
    if (code != ERROR_SUCCESS) goto cleanUp;

    code = NKRegSetValueEx(hKey, L"Group", 0, REG_SZ, (UCHAR*)groupName,
                            (wcslen(groupName) + 1) * sizeof(WCHAR));

    NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}


//------------------------------------------------------------------------------
static BOOL SetDeviceDriverFlags( LPCWSTR szKeyPath, DWORD flags )
{
    LONG code;
    HKEY hKey;
    UINT32 value;

    // Open/create key
    code = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value);          
    if (code != ERROR_SUCCESS) goto cleanUp;

    code = NKRegSetValueEx(hKey, L"Flags", 0, REG_DWORD, (UCHAR*)&flags, sizeof(DWORD));
	NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}

//------------------------------------------------------------------------------

void OEMEthernetDriverEnable(BOOL bEnable)
{
    // Note that this requires a custom check in the ethernet
    // driver since it is loaded by NDIS and not devmgr.
    // We are using the same registry key implementation since 
    // it does not otherwise conflict
//    if (bEnable == TRUE) SetDeviceDriverFlags(L"Comm\\EMACMiniport", DEVFLAGS_NONE);
//    else                 SetDeviceDriverFlags(L"Comm\\EMACMiniport", DEVFLAGS_NOLOAD);

    if (bEnable == TRUE)
	{
		SetDeviceGroup(L"Comm\\CPSW3G", L"NDIS");
		SetDeviceGroup(L"Comm\\CPSW3G1", L"NDIS");
	}
    else
	{
    	SetDeviceGroup(L"Comm\\CPSW3G", L"");
    	SetDeviceGroup(L"Comm\\CPSW3G1", L"");
	}

}

//------------------------------------------------------------------------------

void OEMUsbDriverEnable(BOOL bEnable)
{
	
    //if (bEnable == TRUE)
    //{
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg", DEVFLAGS_NONE
    //        );
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg\\Hcd", DEVFLAGS_NONE
    //        );
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg\\UsbFn", DEVFLAGS_NONE
    //        );
    //}
    //else
    //{
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg", DEVFLAGS_NOLOAD
    //        );
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg\\Hcd", DEVFLAGS_NOLOAD
    //        );
    //    SetDeviceDriverFlags(
    //        L"Drivers\\BuiltIn\\MUsbOtg\\UsbFn", DEVFLAGS_NOLOAD
    //        );
    //}
}
