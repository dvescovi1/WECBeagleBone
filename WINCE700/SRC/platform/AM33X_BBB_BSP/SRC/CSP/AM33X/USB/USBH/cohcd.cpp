// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  cohd.cpp
//
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name:
//     cohcd.cpp
// Abstract:
//     This file contains the COhcd object, which is the main entry
//     point for all HCDI calls by USBD
//
// Notes:
//

#pragma warning(push)
#pragma warning(disable: 4201 4510 4512 4610)

#include "COhcd.hpp"
#include "am33x.h"
#include "am33x_usb.h"
#include "drvcommon.h"
#pragma warning(pop)

// ****************************************************************
// PUBLIC FUNCTIONS
// ****************************************************************

// ******************************************************************
COhcd::COhcd( IN LPVOID pvOhcdPddObject,
                        IN CPhysMem * pCPhysMem,
                        IN LPCWSTR, // szDriverRegistryKey ignored for now
                        IN CSL_UsbRegs * portBase,
                        IN CSL_CppiRegs *cppiBase,
                        IN DWORD dwSysIntr,
                        IN DWORD dwDescriptorCount)
:CHW(portBase, cppiBase, dwSysIntr, dwDescriptorCount, pCPhysMem, pvOhcdPddObject)
,CHCCArea(pCPhysMem )
// Purpose: Initialize variables associated with this class
//
// Parameters: None
//
// Returns: Nothing (Constructor can NOT fail!)
//
// Notes: *All* initialization which could possibly fail should be done
//        via the Initialize() routine, which is called right after
//        the constructor
// ******************************************************************
{
    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("+COhcd::COhcd\n")));
    m_dwSysIntr = dwSysIntr;
    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("-COhcd::COhcd\n")));
}

// ******************************************************************
COhcd::~COhcd()
//
// Purpose: Destroy all memory and objects associated with this class
//
// Parameters: None
//
// Returns: Nothing
//
// Notes:
// ******************************************************************
{
    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("+COhcd::~COhcd\n")));

    // make the API set inaccessible
    CRootHub *pRoot = SetRootHub(NULL);
    // signal root hub to close
    if ( pRoot ) {
        pRoot->HandleDetach();
        delete pRoot;
    }
    CHW::StopHostController();

    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("-COhcd::~COhcd\n")));
}
void COhcd::DeviceDeInitialize( void )
{
    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("+COhcd::DeInitialize\n")));

    // Wait for all the pipes to finish first...
    CHCCArea::WaitForPipes(3000);

    CHW::StopHostController();

    // make the API set inaccessible
    CRootHub *pRoot = SetRootHub(NULL);
    // signal root hub to close
    if ( pRoot ) {
        pRoot->HandleDetach();
        delete pRoot;
    }
    // this is safe because by now all clients have been unloaded
    //DeleteCriticalSection ( &m_csHCLock );

    CHW::DeInitialize();
    CHCCArea::DeInitialize();
    CDeviceGlobal::DeInitialize();

    DEBUGMSG(ZONE_HCD && ZONE_VERBOSE, (TEXT("-COhcd::DeInitialize\n")));
}

// ******************************************************************
BOOL COhcd::DeviceInitialize()
//
// Purpose: Set up the Host Controller hardware, associated data structures,
//          and threads so that schedule processing can begin.
//
// Parameters: pvOhcdPddObject - pointer to the PDD object for this driver
//
//             pCPhysMem - pointer to class for managing physical memory
//
//             szDriverRegistryKey - unused ?
//
//             portBase - base address for USB registers
//
//             dwSysIntr - interrupt identifier for USB interrupts
//
// Returns: TRUE - if initializes successfully and is ready to process
//                 the schedule
//          FALSE - if setup fails
//
// Notes: This function is called by right after the constructor.
//        It is the starting point for all initialization.
//
//        This needs to be implemented for HCDI
// ******************************************************************
{
    DEBUGMSG(ZONE_INIT,(TEXT("+COhcd::DeviceInitialize. Entry\r\n")));

    // All Initialize routines must be called, so we can't write
    // if ( !CDevice::Initialize() || !CPipe::Initialize() etc )
    // due to short circuit eval.
    {
        BOOL fCDeviceInitOK = CDeviceGlobal::Initialize(this);
        BOOL fCHWInitOK = CHW::Initialize( );
        BOOL fCPipeInitOK = CHCCArea::Initialize(this);

        if ( !fCDeviceInitOK || !fCPipeInitOK || !fCHWInitOK ) {
            DEBUGMSG(ZONE_ERROR, (TEXT("-COhcd::DeviceInitialize. Error - could not initialize device/pipe/hw classes\n")));
            return FALSE;
        }
    }

    // set up the root hub object
    {
        USB_DEVICE_INFO deviceInfo;
        USB_HUB_DESCRIPTOR usbHubDescriptor;

        deviceInfo.dwCount = sizeof( USB_DEVICE_INFO );
        deviceInfo.lpConfigs = NULL;
        deviceInfo.lpActiveConfig = NULL;
        deviceInfo.Descriptor.bLength = sizeof( USB_DEVICE_DESCRIPTOR );
        deviceInfo.Descriptor.bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
        deviceInfo.Descriptor.bcdUSB = 0x110; // USB spec 1.10
        deviceInfo.Descriptor.bDeviceClass = USB_DEVICE_CLASS_HUB;
        deviceInfo.Descriptor.bDeviceSubClass = 0xff;
        deviceInfo.Descriptor.bDeviceProtocol = 0xff;
        deviceInfo.Descriptor.bMaxPacketSize0 = 0;
        deviceInfo.Descriptor.bNumConfigurations = 0;

        CHW::GetRootHubDescriptor(usbHubDescriptor);

        // FALSE indicates root hub is not low speed
        // (though, this is ignored for hubs anyway)
        // during root hub object creation, we are configuring as
        // though the root hub is operation in High speed. But actually,
        // the root hub should(will?) be configured to match the speed of the device
        // directly connected to root hub

        /*2 & 3 arguments may be confusing. these two arguments initialize
        m_fIsLowSpeed & m_fIsHighSpeed but it is updated in CRootHub::GetStatus
        */
        SetRootHub( new CRootHub( deviceInfo, FALSE,TRUE, usbHubDescriptor,this ));
    }
    if ( !GetRootHub() ) {
        DEBUGMSG( ZONE_ERROR, (TEXT("-COhcd::DeviceInitialize - unable to create root hub object\n")) );
        return FALSE;
    }

    // Signal root hub to start processing port changes
    // The root hub doesn't have any pipes, so we pass NULL as the
    // endpoint0 pipe
    if ( FXN_DEV_STATE_OPERATIONAL != GetRootHub()->EnterOperationalState( NULL ) ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("-COhcd::DeviceInitialize. Error initializing root hub\n")));
        return FALSE;
    }

    // Start processing frames
    CHW::EnterOperationalState();

    DEBUGMSG(ZONE_INIT,(TEXT("-COhcd::DeviceInitialize. Success!!\r\n")));
    return TRUE;
};

CHcd * CreateHCDObject(IN LPVOID pvOhcdPddObject,
                       IN CPhysMem * pCPhysMem,
                       IN LPCWSTR szDriverRegistryKey,
                       IN REGISTER portBase,
                       IN DWORD dwSysIntr)
{
    SOhcdPdd *pPddObject = (SOhcdPdd *)pvOhcdPddObject;

    DEBUGCHK(pPddObject != NULL);
    if (pPddObject == NULL)
        return NULL;

    CSL_CppiRegs * cppiBase = pPddObject->ioCppiBase;
    DWORD dwDescriptorCount = pPddObject->dwDescriptorCount;

    return new COhcd(pvOhcdPddObject,
        pCPhysMem,
        szDriverRegistryKey,
        (CSL_UsbRegs *)portBase,
        cppiBase,
        dwSysIntr,
        dwDescriptorCount);
}

#ifdef USB_STAT
extern "C" void ChwDmaGetStat(void *pCohcd, UINT32 *retBuf)
{
	COhcd *cohcd = (COhcd *)pCohcd;

	cohcd->m_dmaCrtl.GetStat(retBuf);
}

extern "C" void ChwGetInfo(void *pCohcd, UINT32 *retBuf)
{
	COhcd *cohcd = (COhcd *)pCohcd;

	cohcd->GetStatInfo(retBuf);
}

#endif
