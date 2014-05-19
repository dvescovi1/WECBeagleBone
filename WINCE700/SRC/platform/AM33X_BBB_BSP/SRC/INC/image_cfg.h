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
//  File:  image_cfg.h
//
//  This file contains image layout definition. They should be consistent
//  with *.bib files and addrtab_cfg.inc mapping table. Generally saying
//  *.bib files constants should be determined from constants in this file
//  and mapping file (*.bib file values are virtual cached addresses for
//  Windows CE OS, but physical address for IPL/EBOOT if they don't use
//  MMU).
//
//#ifndef __IMAGE_CFG_H
//#define __IMAGE_CFG_H

//------------------------------------------------------------------------------
//
//  Define: DEVICE_RAM/FLASH_xxx
//
//  First SDRAM bank located physically at 0x80000000
//
#define DEVICE_RAM_PA                   0x80000000
#define DEVICE_RAM_CA                   0x80000000

#define DEVICE_RAM_512_PA				0x90000000		// upper 256mb of 512mb RAM boundry

//------------------------------------------------------------------------------
//
//  Define: IMAGE_SHARE_ARGS_xxx
//
//  Following constants define location and maximal size of arguments shared
//  between loader and kernel. For actual structure see args.h file.
//
#define IMAGE_SHARE_ARGS_PA             0x80000000
#define IMAGE_SHARE_ARGS_CA             0x80000000
#define IMAGE_SHARE_ARGS_SIZE           0x00001000
//------------------------------------------------------------------------------
//
//  Define: CPU_INFO_ADDR
//
//  Following constants define location and maximal size of arguments shared
//  between loader and kernel. For actual structure see args.h file.
//  
#define CPU_INFO_ADDR_PA                0x80001000 
#define CPU_INFO_ADDR_CA                0x80001000
#define CPU_INFO_ADDR_SIZE              0x00001000
//------------------------------------------------------------------------------
//
//  Define: IMAGE_WINCE_CODE
//
//  Following constants define Windows CE OS image layout.
//
#define IMAGE_WINCE_CODE_PA             0x80002000
#define IMAGE_WINCE_CODE_CA             0x80002000
#define IMAGE_WINCE_CODE_SIZE           0x031FE000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_WINCE_RAM
//
//  Following constants define Windows CE OS image layout.
//
#define IMAGE_WINCE_RAM_PA             0x83200000
#define IMAGE_WINCE_RAM_CA             0x83200000
#define IMAGE_WINCE_RAM_SIZE           0x07600000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_WINCE_RAMDISK
//
//  Following constants define Windows CE OS image layout.
//NOT USED at present
#define HEX_VALUE(a)      0x##a
#define IMAGE_WINCE_RAM_DISK_CA_HEX     8A800000  // virtual address for ramdisk
#define IMAGE_WINCE_RAM_DISK_SIZE_HEX   02800000  // RAMDISK 40 MB
#define IMAGE_WINCE_RAM_DISK_CA         HEX_VALUE(IMAGE_WINCE_RAM_DISK_CA_HEX)
#define IMAGE_WINCE_RAM_DISK_SIZE       HEX_VALUE(IMAGE_WINCE_RAM_DISK_SIZE_HEX)

//------------------------------------------------------------------------------
//
//  Define: IMAGE_WINCE_DISPLAY
//
//  Following constants define Windows display driver frame buffer.
//
#define IMAGE_WINCE_DISPLAY_PA				0x8D000000
#define IMAGE_WINCE_DISPLAY_CA				0x8D000000
#define IMAGE_WINCE_DISPLAY_SIZE        	0x01F00000
#define IMAGE_WINCE_DISPLAY_PALETTE_PA		0x8EF00000
#define IMAGE_WINCE_DISPLAY_PALETTE_SIZE	0x00100000
#define IMAGE_WINCE_DISPLAY_TOTAL_SIZE		0x02000000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_WINCE_DRIVER
//
#define IMAGE_WINCE_DRIVER_CA			  0x8F000000
#define IMAGE_WINCE_DRIVER_TOTAL_SIZE	  0x01000000	// 16 MB

//------------------------------------------------------------------------------
// TODO !!!!!!!!!!!!!!!!! set the real values for NETRA
#define NAND_ROMOFFSET                 0x40000000
#define NOR_ROMOFFSET                  0x60000000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_XLDR_xxx
//
//  Following constants define image layout for X-Loader. 
//  XLDR executes from SRAM
//
#define IMAGE_XLDR_CODE_PA              0x402F1000
#define IMAGE_XLDR_CODE_SIZE            0x00015000

#define IMAGE_XLDR_DATA_PA              0x40306000
#define IMAGE_XLDR_DATA_SIZE            0x00004000

#define IMAGE_XLDR_STACK_PA             0x4030A000
#define IMAGE_XLDR_STACK_SIZE           0x00001000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_EBOOT_xxx
//
//  Following constants define EBOOT image layout. 
//
#define IMAGE_EBOOT_CODE_PA             0x8FE00000
#define IMAGE_EBOOT_CODE_CA             0x8FE00000
#define IMAGE_EBOOT_CODE_SIZE           0x00040000

#define IMAGE_EBOOT_DATA_PA             0x8FE80000
#define IMAGE_EBOOT_DATA_SIZE           0x00050000

//------------------------------------------------------------------------------
//
//  Define: IMAGE_STARTUP_xxx
//
//  Jump address XLDR uses to bring-up the device.
//
#define IMAGE_STARTUP_IMAGE_PA         (IMAGE_EBOOT_CODE_PA)
#define IMAGE_STARTUP_IMAGE_SIZE       (IMAGE_EBOOT_CODE_SIZE)


//------------------------------------------------------------------------------
//
//  Define: IMAGE_BOOTLOADER_xxx
//
//  Following constants define bootloader information
//
#define IMAGE_XLDR_BOOTSEC_NAND_SIZE        (4 * 128 * 1024)        // Needs to be equal to four NAND flash blocks due to boot ROM requirements
#define IMAGE_EBOOT_BOOTSEC_NAND_SIZE       IMAGE_EBOOT_CODE_SIZE   // Needs to be a multiple of flash block size

//#define IMAGE_BOOTLOADER_BITMAP_SIZE        0x00040000                  // Needs to be a multiple of 128k, and minimum of 240x320x3 (QVGA)  
#define IMAGE_BOOTLOADER_BITMAP_SIZE        0x00180000                  // Needs to be a multiple of 128k, and minimum 480x640x3 (VGA)  

#define IMAGE_BOOTLOADER_NAND_SIZE      (IMAGE_XLDR_BOOTSEC_NAND_SIZE + \
                                         IMAGE_EBOOT_BOOTSEC_NAND_SIZE + \
                                         IMAGE_BOOTLOADER_BITMAP_SIZE)

//------------------------------------------------------------------------------

//#endif
