// All rights reserved ADENEO EMBEDDED 2010
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
#pragma warning (push)
#pragma warning (disable:4201)
#include <windows.h>
#include <types.h>
#include <winddi.h>
#include <emul.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <ddrawi.h>
#include <ddgpe.h>
#include <ddhfuncs.h>
#pragma warning (pop)

#include "omap.h"
#include "omap_dss_regs.h"
#include "dssai.h"
#include "lcd.h"

#include "display_drvesc.h"

#include "omap_ddgpe.h"
#include "_debug.h"

#if (_WINCEOSVER<600)

#include <winuserm.h>
#include <gxinfo.h>

#ifndef GETRAWFRAMEBUFFER
    #define GETRAWFRAMEBUFFER   0x00020001
    typedef struct _RawFrameBufferInfo
    {
        WORD wFormat;
        WORD wBPP;
        VOID *pFramePointer;
        int cxStride;
        int cyStride;
        int cxPixels;
        int cyPixels;
    } RawFrameBufferInfo;

    #define FORMAT_565 1
    #define FORMAT_555 2
    #define FORMAT_OTHER 3
#endif

#define FOURCC_YUY2                     FOURCC_YUY2422
#define FOURCC_YUYV                     FOURCC_YUYV422
#define FOURCC_UYVY                     FOURCC_UYVY422
#define FOURCC_YVYU                     0

#define ddgpePixelFormat_YUY2           ddgpePixelFormat_YUY2422
#define ddgpePixelFormat_UYVY           ddgpePixelFormat_UYVY422

#ifndef DDSCAPS_OWNDC
#define DDSCAPS_OWNDC                   0
#endif

#define DDOVER_AUTOFLIP                 0
#define DDSCAPS_VIDEOPORT               0
#define DDSCAPS_HARDWAREDEINTERLACE     0
#define DDOVER_AUTOFLIP                 0
#define DDOVER_AUTOFLIP                 0

#endif
