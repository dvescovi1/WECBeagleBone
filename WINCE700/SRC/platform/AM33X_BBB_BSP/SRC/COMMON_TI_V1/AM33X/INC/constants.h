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
#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define KHZ(x)              (x * 1000)
#define MHZ(x)              (x * 1000000)
//-----------------------------------------------------------------------------

// Bits
#define BIT0    0x00000001
#define BIT1    0x00000002
#define BIT2    0x00000004
#define BIT3    0x00000008
#define BIT4    0x00000010
#define BIT5    0x00000020
#define BIT6    0x00000040
#define BIT7    0x00000080
#define BIT8    0x00000100
#define BIT9    0x00000200
#define BIT10   0x00000400
#define BIT11   0x00000800
#define BIT12   0x00001000
#define BIT13   0x00002000
#define BIT14   0x00004000
#define BIT15   0x00008000
#define BIT16   0x00010000
#define BIT17   0x00020000
#define BIT18   0x00040000
#define BIT19   0x00080000
#define BIT20   0x00100000
#define BIT21   0x00200000
#define BIT22   0x00400000
#define BIT23   0x00800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000

// Blocks.
#define N1KB    BIT10           //  1KB block
#define N2KB    BIT11           //  2KB block
#define N4KB    BIT12           //  4KB block
#define N8KB    BIT13           //  8KB block
#define N16KB   BIT14           // 16KB block
#define N32KB   BIT15           // 32KB block
#define N64KB   BIT16           // 64KB block
#define N128KB  BIT17           // 128KB block
#define N192KB  (N128KB+N64KB)  // 192KB block
#define N200KB  (N192KB+N8KB)   // 200KB block
#define N256KB  BIT18           // 256KB block
#define N384KB  (N256KB+N128KB) // 384KB block
#define N512KB  BIT19           // 512KB block
#define N596KB  (596 * 1024)    // 596KB block
#define N1MB    BIT20           //  1MB block
#define N2MB    BIT21           //  2MB block
#define N4MB    BIT22           //  4MB block
#define N8MB    BIT23           //  8MB block
#define N9MB    (N8MB+N1MB)     //  9MB block
#define N10MB   (N8MB+N2MB)     // 10MB block
#define N16MB   BIT24           // 16MB block
#define N32MB   BIT25           // 32MB block
#define N64MB   BIT26           // 64MB block

// Block masks.
#define MASK_32B    0x0000001f
#define MASK_4KB    0x00000fff
#define MASK_8KB    0x00001fff
#define MASK_16KB   0x00003fff
#define MASK_32KB   0x00007fff
#define MASK_64KB   0x0000ffff
#define MASK_128KB  0x0001ffff
#define MASK_1MB    0x000fffff
#define MASK_4MB    0x003FFFFF
#define MASK_8MB    0x007FFFFF
#define MASK_16MB   0x00ffffff
#define MASK_32MB   0x01ffffff
#define MASK_64MB   0x03ffffff
#define MASK_512MB  0x1fffffff
#define MASK_1GB    0x3fffffff
#define MASK_2GB    0x7fffffff

//-----------------------------------------------------------------------------

#endif
