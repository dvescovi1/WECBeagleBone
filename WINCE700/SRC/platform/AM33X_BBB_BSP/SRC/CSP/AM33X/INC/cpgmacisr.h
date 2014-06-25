//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Copyright (c) Texas Instruments Incorporated 2008-2009
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// File: CPGMACISR.H
//
// Purpose: Header file shared between Cpsw3gMiniport.dll and cpgmacisr.dll
//
//

#ifndef _CPGMACISR_H_
#define _CPGMACISR_H_

#include <pkfuncs.h>

#define USER_IOCTL(X)           (IOCTL_KLIB_USER + (X))

#define IOCTL_CPGMACISR_INFO    USER_IOCTL(0)

typedef struct __CPGMAC_ISR_INFO__
{
    DWORD   SysIntr;            //  SYSINTR for ISR handler to return (if associated device is asserting IRQ)
    UINT32   RxIntPortAddr;           //  Port Virt Address
    UINT32   TxIntPortAddr;           //  Port Virt Address
    UINT32   DMAIntAddr;           //  Port Virt Address
    UINT32   Cpdma_Vector;

    UINT32   TxIntMask;  //  Tx channel interrupt mask 
    UINT32   RxIntMask;  // Rx channel interrupt mask
    UINT32   DMAIntMask;
    
} CPGMAC_ISR_INFO, *PCPGMAC_ISR_INFO;
    
#endif // _NE2000ISR_H_
