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
//  Header:  eboot.h
//
//  This header file is comprised of component header files that defines
//  the standard include hierarchy for the bootloader. It also defines few
//  trivial constant.
//
#ifndef __EBOOT_H
#define __EBOOT_H

//------------------------------------------------------------------------------
#include "bsp.h"

#pragma warning(push)
#pragma warning(disable: 4201 4115)

#include <blcommon.h>
#include <nkintr.h>
#include <halether.h>
#include <fmd.h>
#include <bootpart.h>
#include <oal.h>
#include <oal_blmenu.h>

#pragma warning(pop)

#include "boot_args.h"
#include "args.h"


//------------------------------------------------------------------------------

#define EBOOT_VERSION_MAJOR         0
#define EBOOT_VERSION_MINOR         0
#define EBOOT_VERSION_BUILD         1

//------------------------------------------------------------------------------

typedef struct {
    UINT32 start;
    UINT32 length;
    UINT32 base;
} REGION_INFO_EX;

typedef struct OMAP_LCD_DVI_RES_MENU {
    DWORD       lcdDviRes;
    LPCWSTR     resName;
} OMAP_LCD_DVI_RES_MENU;

//------------------------------------------------------------------------------

#define DOWNLOAD_TYPE_UNKNOWN       0
#define DOWNLOAD_TYPE_RAM           1
#define DOWNLOAD_TYPE_BINDIO        2
#define DOWNLOAD_TYPE_XLDR          3
#define DOWNLOAD_TYPE_EBOOT         4
#define DOWNLOAD_TYPE_IPL           5
#define DOWNLOAD_TYPE_FLASHNAND     6
#define DOWNLOAD_TYPE_FLASHNOR      7
#define DOWNLOAD_TYPE_LOGO			8

//------------------------------------------------------------------------------

#define LOGO_NB0_FILE				"logo.nb0"
#define LOGO_NB0_FILE_LEN           8

//------------------------------------------------------------------------------

typedef struct {
    OAL_KITL_TYPE bootDeviceType;
    UINT32 type;
    UINT32 numRegions;
    UINT32 launchAddress;
    REGION_INFO_EX region[BL_MAX_BIN_REGIONS];

    UINT32 recordOffset;
    UINT8  *pReadBuffer;
    UINT32 readSize;
} EBOOT_CONTEXT;

//------------------------------------------------------------------------------

extern BOOT_CFG g_bootCfg;
extern EBOOT_CONTEXT g_eboot;
extern OAL_KITL_DEVICE g_bootDevices[];
extern OAL_KITL_DEVICE g_kitlDevices[];

extern UINT32   g_ulFlashBase;

//------------------------------------------------------------------------------

VOID OEMMultiBinNotify(MultiBINInfo *pInfo);
BOOL OEMVerifyMemory(DWORD dwStartAddr, DWORD dwLength);
VOID DrawProgressBar(UINT32 total, UINT32 pos, BOOL invert);

//------------------------------------------------------------------------------

VOID   BLMenu(BOOL bForced);    
BOOL   BLReadBootCfg(BOOT_CFG *pBootCfg);
BOOL   BLWriteBootCfg(BOOT_CFG *pBootCfg);
BOOL   BLReserveBootBlocks();
BOOL   BLConfigureFlashPartitions(BOOL bForceEnable);
BOOL   BLShowLogo(BOOL invert);
UINT32 BLEthDownload(BOOT_CFG *pBootCfg, OAL_KITL_DEVICE *pBootDevices);
BOOL   BLEthReadData(ULONG size, UCHAR *pData);
VOID   BLEthConfig(BSP_ARGS *pArgs);
UINT32 BLSDCardDownload(WCHAR *filename);
BOOL   BLSDCardReadData(ULONG size, UCHAR *pData);
UINT32 BLFlashDownload(BOOT_CFG *pConfig, OAL_KITL_DEVICE *pBootDevices);
BOOL   BLSDCardReadLogo(WCHAR *filename, UCHAR *pData, DWORD *size);
//UINT32 BLVAtoPA(UINT32 address);

UINT32 OALStringToUINT32(LPCWSTR psz);
UINT32 OALHexStringToUINT32(LPCWSTR psz);

WCHAR *BLSDCardCfgLineGetField(WCHAR *Line);
WCHAR *BLSDCardCfgLineGetValue(WCHAR *Line);
BOOL BLSDCardCfgGetLine(WCHAR *CfgBuf, DWORD *pos, WCHAR *Line, DWORD size);
BOOL BLSDCardReadCfg(WCHAR *filename, UCHAR *pData, DWORD size);

//------------------------------------------------------------------------------

#endif
