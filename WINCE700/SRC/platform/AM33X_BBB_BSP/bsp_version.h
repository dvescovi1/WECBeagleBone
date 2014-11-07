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
// Copyright (c) 2008 BSQUARE Corporation. All rights reserved.

//
//  File:  bsp_version.h
//
#ifndef __BSP_VERSION_H
#define __BSP_VERSION_H

// This BSP uses the following versioning scheme
// x.yy.zz
// x  -> major release number, typically CE version number
// yy -> minor release number, typically incremented on every BSP release
// zz -> additional incremental release number that can be used to version 
// patches to an incremental release

// NOTE: The BSP version is displayed in the IDE using the BSP Properties 
//  window.  The information in this window comes from the FileVersion field
//  in the pbcxml file.  The FileVersion field in the PBCXML file must be
//  synchronized with the contents of this file.

// Note: do not use a leading zero, compiler interprets leading 0 as octal number
#define BSP_VERSION_MAJOR       1
#define BSP_VERSION_MINOR       13
#define BSP_VERSION_QFES        0
#define BSP_VERSION_INCREMENTAL 0

#define BSP_VERSION_STRING      L"BSP_WINWEC7_ARM_A8 01.13.00.00"


#endif
