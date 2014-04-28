// All rights reserved ADENEO EMBEDDED 2010
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
//  File:  args.h
//
//  This header file defines device structures and constant related to boot
//  configuration. BOOT_CFG structure defines layout of persistent device
//  information. It is used to control boot process.
//
#ifndef __BOOT_ARGS_H
#define __BOOT_ARGS_H

//------------------------------------------------------------------------------

#include <oal_args.h>
#include <oal_kitl.h>
#include <bsp_cfg.h>


//------------------------------------------------------------------------------

#define BOOT_CFG_SIGNATURE      'BCFG'
#define BOOT_CFG_VERSION        3

#define BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE     (1 << 0)
#define BOOT_CFG_OAL_FLAGS_CFG_SAVE             (1 << 1)
#define BOOT_CFG_OAL_FLAGS_CLEAN_REGISTRY       (1 << 3)

#define ENABLE_FLASH_NK          (1<<0)


// BOOT_SDCARD_TYPE must not overlap with OAL_KITL_TYPE enum values
#define BOOT_SDCARD_TYPE        (OAL_KITL_TYPE_FLASH+1)

typedef struct {
    UINT32 signature;                   // Structure signature
    UINT32 version;                     // Structure version

    DEVICE_LOCATION bootDevLoc;         // Boot device
    DEVICE_LOCATION kitlDevLoc;
    
    UINT32 kitlFlags;                   // Debug/KITL mode
    UINT32 oalFlags;
    UINT32 ipAddress;
    UINT32 ipMask;
    UINT32 ipRoute;    

    UINT32 deviceID;                    // Unique ID for development platform
    
    UINT32 osPartitionSize;             // Space to reserve for OS partition in NAND
    
    WCHAR filename[13];                 // Space to reserve for SDCard filename (8.3 format)

	UINT16 mac[3];						// mac address for the ethernet    

	UINT16 mac1[3];						// mac address for the ethernet    

	OMAP_LCD_DVI_RES displayRes;        // display resolution	
    UINT32 flashNKFlags;
    UCHAR ECCtype;
    UINT32 opp_mode;
} BOOT_CFG;

//------------------------------------------------------------------------------

#endif
