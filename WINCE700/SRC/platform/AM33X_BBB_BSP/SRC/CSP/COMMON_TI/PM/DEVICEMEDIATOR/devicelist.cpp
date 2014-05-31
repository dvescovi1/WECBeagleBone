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
//  File:  devicelist.cpp
//
#include "omap.h"
#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <devloadex.h>
#include <cregedit.h>
#include "_constants.h"
#include "devicelist.h"
#include "string.hxx"
//#include "omap35xx_clocks.h"
//#include "omap35xx_base_regs.h"
#include "oalex.h"
#include "devicebase.h"



//------------------------------------------------------------------------------
//
//  Static :  s_DeviceAddressMap[]    
//
//  Array of device membase to device id mapping.

extern "C" const DEVICE_ADDRESS_MAP s_DeviceAddressMap[];

//------------------------------------------------------------------------------
//
//  Static :  s_rgDeviceList[] 
//
//  Stores the device base pointer in corresponding device id location
//  makes it easy to send ioctl without searching the list.
static DeviceBase *s_rgDeviceList[OMAP_MAX_DEVICE_COUNT];

//-----------------------------------------------------------------------------
// Macro : REGISTRY_MEMBASE_STRING_SIZE
//  
//  Defines the size to read MULTI_SZ membase registry entry.
// 
#define REGISTRY_MEMBASE_STRING_SIZE    256 

//-----------------------------------------------------------------------------
BOOL
DeviceList::Initialize()
{
    ::InitializeCriticalSection(&m_cs);
    for(UINT32 count=0; count<OMAP_MAX_DEVICE_COUNT; count++)
        {
        s_rgDeviceList[count] = NULL;
        }
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
DeviceList::Uninitialize()
{
    return TRUE;
}


//-----------------------------------------------------------------------------
BOOL 
DeviceList::InsertDevice(
    _TCHAR const* szDeviceName,
    DeviceBase *pDevice,
    HKEY hKey
    )
{
    BOOL rc = FALSE;
    
    // Get Notification flags
    DWORD dwType = REG_DWORD;
    DWORD dwSize;
    _TCHAR szMembase[MAX_PATH];

    // Get the device MemBase
    DWORD dwMemBase = 0;

    UNREFERENCED_PARAMETER(szDeviceName);

    if (::RegQueryValueEx(hKey, DEVLOAD_MEMBASE_VALNAME, NULL, &dwType, 
            NULL, NULL) != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    switch(dwType)
        {
        case REG_DWORD:
            dwSize = sizeof(dwMemBase);
            if (::RegQueryValueEx(hKey, DEVLOAD_MEMBASE_VALNAME, NULL, &dwType, 
                (BYTE*)&dwMemBase, &dwSize) != ERROR_SUCCESS)
            {

            goto cleanUp;
            }
            break;
            
        case REG_SZ:
        case REG_MULTI_SZ:
            // Read string or multi string
            dwSize = MAX_PATH;
            if (::RegQueryValueEx(hKey, DEVLOAD_MEMBASE_VALNAME, NULL, &dwType, 
                    (BYTE*)szMembase, &dwSize) != ERROR_SUCCESS)
                {
                goto cleanUp;
                }
           
            // Convert first string as hexa number
            dwMemBase = wcstoul(szMembase, NULL, 16);
            break;
        }

     // Update the device list array 

     // Find the device id from the address to device id mapping table
     // and updated the device base pointer in the device id location of 
     // s_DeviceList
     for (int i = 0; s_DeviceAddressMap[i].deviceAddress != 0; ++i)
        {
        if (s_DeviceAddressMap[i].deviceAddress == dwMemBase)
            {
            s_rgDeviceList[(s_DeviceAddressMap[i].device)] = pDevice;
            break;
            }
        }

    rc = TRUE;

cleanUp:
    return rc;    
}

//-----------------------------------------------------------------------------
BOOL 
DeviceList::RemoveDevice(
    _TCHAR const* szDeviceName,
    DeviceBase *pDevice
    )
{   
    BOOL rc = TRUE;

    UNREFERENCED_PARAMETER(szDeviceName);

    Lock();
    
    // Remove the device entry from the list
    for(int i = 0; i<OMAP_MAX_DEVICE_COUNT; i++)
        {
        if(s_rgDeviceList[i] == pDevice)
            {
            s_rgDeviceList[i] = NULL;
            break;
            }
        }

    Unlock();
    return rc;
}

//-----------------------------------------------------------------------------
BOOL 
DeviceList::SendIoControl(
    DWORD dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    DWORD *lpBytesReturned
    )
{ 
    BOOL rc = TRUE;

    UNREFERENCED_PARAMETER(lpBytesReturned);
    UNREFERENCED_PARAMETER(nOutBufSize);
    UNREFERENCED_PARAMETER(lpOutBuf);
    UNREFERENCED_PARAMETER(nInBufSize);
    UNREFERENCED_PARAMETER(lpInBuf);

    if( dwParam >= OMAP_MAX_DEVICE_COUNT) return rc;

    if( s_rgDeviceList[dwParam] == NULL) return rc;
 
    Lock();

    // Send IOCTL to device
    rc = (s_rgDeviceList[dwParam])->SendIoControl(dwIoControlCode, &dwParam, 
                sizeof(dwParam), NULL, NULL, NULL
                );
        
    Unlock();

    return rc;
}

//-----------------------------------------------------------------------------
