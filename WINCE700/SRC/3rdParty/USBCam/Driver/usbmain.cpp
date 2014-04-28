//-------------------------------------------------------------------------
// <copyright file="main.cpp" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#include <windows.h>
#include <pkfuncs.h>
#include <pm.h>
#include <devload.h>
#include <nkintr.h>
#include <usbdi.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include "CameraDriver.h"

#define DRIVER_NAME   TEXT("USBCAM.DLL")
#define CLASS_NAME_SZ    TEXT("Video_Class")
#define CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\Video_Class")
#define LOAD_REGKEY_SZ TEXT("Drivers\\USB\\LoadClients\\Default\\Default\\14\\Video_Class")
//#define LOAD_REGKEY_SZ TEXT("Drivers\\USB\\LoadClients\\Default\\Default\\Default\\Video_Class")

#define TEST_TRAP() { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: %s, Line: %d\n"), DRIVER_NAME, TEXT(__FILE__), __LINE__); \
   DebugBreak();  \
}

#define USB_DEVICE_CLASS_VIDEO                0x0E
//#define USB_DEVICE_SUBCLASS_VIDEO             0x03

//
// USB_DRIVER_SETTINGS
//
#define DRIVER_SETTINGS \
            sizeof(USB_DRIVER_SETTINGS),  \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_DEVICE_CLASS_VIDEO,      \
            USB_NO_INFO,   \
            USB_NO_INFO

BOOL USBInstallDriver(LPCWSTR szDriverLibFile)
{
    HKEY  hKey = NULL;
    BOOL  bRc;

    const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;
    WCHAR wsSubClassRegKey[sizeof(CLIENT_REGKEY_SZ)+16] = CLIENT_REGKEY_SZ;

    USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

    DEBUGMSG(ZONE_USB_INIT, (TEXT(">USBInstallDriver(%s)\n"), szDriverLibFile));
    TEST_TRAP();
    
    //
    // register with USBD
    //   
    bRc = RegisterClientDriverID(wsUsbDeviceID);
    if (!bRc) 
    {
      DEBUGMSG( ZONE_ERROR, (TEXT("RegisterClientDriverID error:%d\n"), GetLastError()));
      return FALSE;
    }
        
    bRc = RegisterClientSettings(szDriverLibFile,
                                wsUsbDeviceID, 
                                NULL, 
                                &usbDriverSettings);
    if (!bRc) 
    {
      DEBUGMSG( ZONE_ERROR, (TEXT("RegisterClientSettings error:%d\n"), GetLastError()));
      return FALSE;
    }
 
    DEBUGMSG( ZONE_USB_INIT, (TEXT("<USBInstallDriver:%d\n"), bRc ));

    return bRc;
}

BOOL 
USBUnInstallDriver(
   VOID
   )
{
   BOOL bRc;
   const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;
   USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">USBUnInstallDriver\n")));

   bRc = UnRegisterClientSettings(wsUsbDeviceID,
                                  NULL,
                                  &usbDriverSettings);

   bRc = bRc & UnRegisterClientDriverID(wsUsbDeviceID);

   DEBUGMSG(ZONE_USB_INIT, (TEXT("<USBUnInstallDriver:%d\n"), bRc));

   return bRc;
}
