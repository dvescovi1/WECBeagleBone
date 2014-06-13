// All rights reserved Texas Instruments, Inc.  2011
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  cohcd.hpp
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
//     COhcd.hpp
//
// Abstract:  COhcd implements the HCDI interface. It mostly
//            just passes requests on to other objects, which
//            do the real work.
//
// Notes:
//

#ifndef __COHCD_HPP__
#define __COHCD_HPP__

#include <new>
#include <globals.hpp>
#include "am33x_usb.h"

class COhcd;
#include "Chw.hpp"
#include "CDevice.hpp"
#include "CPipe.hpp"


// this class gets passed into the COhcd Initialize routine
class CPhysMem;
// this class is our access point to all USB devices
//class CRootHub;

class COhcd : public CHW, public CHCCArea
{
public:
    // ****************************************************
    // Public Functions for COhcd
    // ****************************************************
    COhcd(IN LPVOID pvOhcdPddObject,
          IN CPhysMem * pCPhysMem,
          IN LPCWSTR szDriverRegistryKey,
          IN CSL_UsbRegs * portBase,
          IN CSL_CppiRegs * cppiBase,
          IN DWORD dwSysIntr,
          IN DWORD dwDescriptorCount);

    ~COhcd();

    // These functions are called by the HCDI interface
    virtual BOOL DeviceInitialize(void);
    virtual void DeviceDeInitialize( void );
// "CE6.0" virtual BOOL AddedTt( UCHAR uHubAddress,UCHAR uPort) { uHubAddress = uHubAddress;uPort = uPort;return TRUE; };
    virtual PVOID AddedTt( UCHAR uHubAddress,UCHAR uPort) { uHubAddress = uHubAddress;uPort = uPort;return NULL; };
    virtual BOOL DeleteTt( UCHAR uHubAddress,UCHAR uPort, PVOID pContext)
		   { uHubAddress = uHubAddress;uPort = uPort; pContext=pContext; return TRUE; };

    // ****************************************************
    // Public Variables for COhcd
    // ****************************************************

    // no public variables

private:
    // ****************************************************
    // Private Functions for COhcd
    // ****************************************************

    // ****************************************************
    // Private Variables for COhcd
    // ****************************************************
                                            // the built-in hardware USB ports
    REGISTER        m_portBase;
    REGISTER        m_cppiBase;
    DWORD           m_dwSysIntr;

	COhcd operator=(COhcd&);
};

#endif // __COHCD_HPP__

