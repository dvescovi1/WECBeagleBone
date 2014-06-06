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

//------------------------------------------------------------------------------
//
//  File:  omap_vrfb_regs.h
//
//  This header file is comprised of display module register details defined as
// structures and macros for configuring and controlling VRFB (Virtually Rotated
// Frame Buffer)

#ifndef __OMAP_VRFB_REGS_H
#define __OMAP_VRFB_REGS_H

#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------

//
// VRFB (Virtual Remote Frame Buffer)
//

#define VRFB_ROTATION_CONTEXTS          12
#define VRFB_ROTATION_ANGLES            4
#define VRFB_IMAGE_WIDTH_MIN            64
#define VRFB_IMAGE_WIDTH_MAX            2048
#define VRFB_IMAGE_HEIGHT_MIN           64
#define VRFB_IMAGE_HEIGHT_MAX           2048
#define VRFB_VIEW_SIZE                  (sizeof(DWORD)*VRFB_IMAGE_WIDTH_MAX*VRFB_IMAGE_HEIGHT_MAX)
#define VRFB_CONTEXT_SIZE               (VRFB_ROTATION_ANGLES*VRFB_VIEW_SIZE)

//
// Sub-structure of OMAP_VRFB_REGS
//

typedef volatile struct 
{
    UINT32 VRFB_SMS_ROT_CONTROL;            //offset 0x00
    UINT32 VRFB_SMS_ROT_SIZE;               //offset 0x04
    UINT32 VRFB_SMS_ROT_PHYSICAL_BA;        //offset 0x08
    UINT32 zzzRESERVED;
}
OMAP_VRFB_ELEMENT;

//
// VRFB Control Registers
//

// Base Address: OMAP_VRFB_REGS_PA

typedef volatile struct
{
    UINT32 VRFB_SMS_CLASS_ROTATION0;        //offset 0x0164
    UINT32 VRFB_SMS_CLASS_ROTATION1;        //offset 0x0168
    UINT32 VRFB_SMS_CLASS_ROTATION2;        //offset 0x016C

    UINT32 VRFB_SMS_ERR_ADDR;               //offset 0x0170
    UINT32 VRFB_SMS_ERR_TYPE;               //offset 0x0174
    UINT32 VRFB_SMS_POW_CTRL;               //offset 0x0178
    UINT32 zzzRESERVED;

    OMAP_VRFB_ELEMENT aVRFB_SMS_ROT_CTRL[VRFB_ROTATION_CONTEXTS];
}
OMAP_VRFB_REGS;


// VRFB Rotation Control

#define SMS_ROT_CONTROL_PIXELSIZE_1B            (0 << 0)
#define SMS_ROT_CONTROL_PIXELSIZE_2B            (1 << 0)
#define SMS_ROT_CONTROL_PIXELSIZE_4B            (2 << 0)
#define SMS_ROT_CONTROL_PAGEWIDTH(pw)           ((pw) << 4)
#define SMS_ROT_CONTROL_PAGEHEIGHT(ph)          ((ph) << 8)

#define SMS_ROT_SIZE_WIDTH(iw)                  ((iw & 0x07FF) << 0)
#define SMS_ROT_SIZE_HEIGHT(ih)                 ((ih & 0x07FF) << 16)


#ifdef __cplusplus
}
#endif

#endif //__OMAP_VRFB_REGS_H
