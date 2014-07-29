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
//
//=============================================================================
//            Texas Instruments OMAP(TM) Platform Software
// (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
//
//  Use of this software is controlled by the terms and conditions found
// in the license agreement under which this software has been supplied.
//
//=============================================================================
//

//------------------------------------------------------------------------------
//
//  File:  args.h
//
//  This header file defines device structures and constant related to boot
//  configuration. BOOT_CFG structure defines layout of persistent device
//  information. It is used to control boot process. BSP_ARGS structure defines
//  information passed from boot loader to kernel HAL/OAL. Each structure has
//  version field which should be updated each time when structure layout
//  change.
//
#ifndef __ARGS_H
#define __ARGS_H

//------------------------------------------------------------------------------

#include <oal_args.h>
#include <oal_kitl.h>
#include <bsp_cfg.h>
#include "bsp_def.h"

//------------------------------------------------------------------------------

#define BSP_ARGS_VERSION    3

typedef struct {
    OAL_ARGS_HEADER header;
    BOOL updateMode;                    // Should IPL run in update mode?
    BOOL coldBoot;                      // Cold boot
    UINT32 deviceID;                    // Unique ID for development platform
    UINT32 imageLaunch;                 // Image launch address
    OAL_KITL_ARGS kitl;                 // KITL parameters
    UINT32 oalFlags;                    // OAL flags
    UCHAR DevicePrefix[24];
    UINT16 mac[3];						// mac address for the ethernet
    UINT16 mac1[3];						// mac address 1 for the ethernet
    OMAP_LCD_DVI_RES  dispRes;          // display resolution
	UINT32 bootInfo;					// boot information boot cause and boot slot
	UINT32 spare1;
	UINT32 spare2;
	UINT32 spare3;
    UINT32 ECCtype;    
    UINT32 opp_mode;
    UINT32 cfgSize;                     // size of eboot cfg
    UINT8  ebootCfg[256];               // copy of eboot cfg to save to SD      
} BSP_ARGS;

//------------------------------------------------------------------------------

#endif
