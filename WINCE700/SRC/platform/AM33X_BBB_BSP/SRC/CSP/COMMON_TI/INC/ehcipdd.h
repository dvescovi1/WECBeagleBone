// All rights reserved ADENEO EMBEDDED 2010
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
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------

#ifndef __EHCIPDD_H
#define __EHCIPDD_H

//------------------------------------------------------------------------------

typedef struct _SEHCDPdd
{
    DWORD memBase;
    DWORD memLen;
    DWORD BusSuspendResume;
    LPVOID lpvMemoryObject;
    LPVOID lpvEHCDMddObject;
    PVOID pvVirtualAddress;                 // DMA buffers as seen by the CPU
    DWORD dwPhysicalMemSize;
    PHYSICAL_ADDRESS LogicalAddress;        // DMA buffers as seen by the DMA controller and bus interfaces
    DMA_ADAPTER_OBJECT AdapterObject;
    TCHAR szDriverRegKey[MAX_PATH];
    PUCHAR ioPortBase;
    DWORD dwSysIntr;
    CRITICAL_SECTION csPdd;                 // serializes access to the PDD object
    HANDLE          IsrHandle;
    DWORD dwActualPowerState;
    DWORD port1Mode;
    DWORD port2Mode;
    DWORD port3Mode;
    DWORD Port1PwrGpio;
    DWORD Port2PwrGpio;
    DWORD Port3PwrGpio;
	DWORD Port1RstGpio;
	DWORD Port2RstGpio;
	DWORD Port3RstGpio;
    DWORD Port1PwrLevel;
    DWORD Port2PwrLevel;
    DWORD Port3PwrLevel;
    HANDLE hGpio;
	USBH_INFO USBHInfo;
    HANDLE hParentBusHandle;
    HANDLE hRootBus;

    UINT nDVFSOrder;
} SEHCDPdd, *P_SEHCDPdd;

//------------------------------------------------------------------------------

void SOCEhciConfigure(SEHCDPdd * pPddObject);

//------------------------------------------------------------------------------

#endif
