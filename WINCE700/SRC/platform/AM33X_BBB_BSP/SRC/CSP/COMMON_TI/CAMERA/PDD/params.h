// All rights reserved ADENEO EMBEDDED 2010
#ifndef __PARMS_H
#define __PARMS_H

#include "tvp5146.h"

#define TV_D1_PAL_HORZ_RES          (704)
#define TV_D1_PAL_VERT_RES          (576)
#define DVD_PAL_HORZ_RES            (720)
#define DVD_PAL_VERT_RES            (576)
#define CIF_PAL_HORZ_RES            (352)
#define CIF_PAL_VERT_RES            (288)
#define QCIF_PAL_HORZ_RES           (176)
#define QCIF_PAL_VERT_RES           (144)


#define TV_D1_NTSC_HORZ_RES         (704)
#define TV_D1_NTSC_VERT_RES         (480)
#define DVD_NTSC_HORZ_RES           (720)
#define DVD_NTSC_VERT_RES           (480)
#define CIF_NTSC_HORZ_RES           (352)
#define CIF_NTSC_VERT_RES           (240)
#define QCIF_NTSC_HORZ_RES          (176)
#define QCIF_NTSC_VERT_RES          (120)

#define VGA_HORZ_RES                (640)
#define VGA_VERT_RES                (480)
#define QVGA_HORZ_RES               (320)
#define QVGA_VERT_RES               (240)


#define MAX_X_RES               (DVD_PAL_HORZ_RES)
#define MAX_Y_RES               (DVD_PAL_VERT_RES)

#define HORZ_INFO_SPH_VAL       (202)
#define VERT_START_VAL          (24)

#define ENABLE_BT656            // using BT656 output from video decoder
// for AM3517: Do not Define ENABLE_PACk8 if using older App boards - 1013690, 1014473; Define if using newer app board 1015189
#define ENABLE_PACK8            // use 8-bit data width
//#define ENABLE_YUY2           // uncomment this to enable YUY2 output
#define ENABLE_PAL            // uncomment to enable PAL resolutions and frame rates

//#define ENABLE_DEINTERLACED_OUTPUT
#define ENABLE_PROGRESSIVE_INPUT


#ifdef ENABLE_BT656
    #undef HORZ_INFO_SPH_VAL
    //#define HORZ_INFO_SPH_VAL       (60)
    #define HORZ_INFO_SPH_VAL       (8)
#endif //ENABLE_BT656

#ifdef INSTANT_TVP_SETTINGS
    TVP_SETTINGS tvpSettings[]={\
        {REG_INPUT_SEL, 0x0C},//Cpmposite path
        {REG_AFE_GAIN_CTRL, 0x0F},//default
        {REG_VIDEO_STD, 0x00},//default: auto mode
        {REG_OPERATION_MODE, 0x00},//normal mode
        {REG_AUTOSWITCH_MASK, 0x3F},//autoswitch mask: enable all
        {REG_COLOR_KILLER, 0x10},//default
        {REG_LUMA_CONTROL1, 0x00},//default
        {REG_LUMA_CONTROL2, 0x00},//default
        {REG_LUMA_CONTROL3, 0x02},//default
        {REG_BRIGHTNESS, 0x80},//default
        {REG_CONTRAST, 0x80},//default
        {REG_SATURATION, 0x80},//default
        {REG_HUE, 0x00},//default
        {REG_CHROMA_CONTROL1, 0x00},//default
        {REG_CHROMA_CONTROL2, 0x0E},//default
        {REG_COMP_PR_SATURATION, 0x80},//default
        {REG_COMP_Y_CONTRAST, 0x80},//default
        {REG_COMP_PB_SATURATION, 0x80},//default
        {REG_COMP_Y_BRIGHTNESS, 0x80},//default             
        {REG_SYNC_CONTROL, 0x0C},//FID 1st field "1", 2nd field  "0"; HS, VS active high        
        #ifndef ENABLE_BT656
            {REG_OUTPUT_FORMATTER1, 0x03},//ITU601 coding range, 10bit 4:2:2 separate sync
        #else
            {REG_OUTPUT_FORMATTER1, 0x00},//ITU601 coding range, 10bit 4:2:2 embedded sync
        #endif 
        {REG_OUTPUT_FORMATTER2, 0x13},//Enable clk & y[9:0],c[9:0] active, DATACLK OUT "rising edge "   
        {REG_OUTPUT_FORMATTER3, 0x02},//Set FID as FID output
        {REG_OUTPUT_FORMATTER4, 0xAF},//default: HS, VS is output, C1~C0 is input 
        {REG_OUTPUT_FORMATTER5, 0xFF},//default: C5~C2 is input 
        {REG_OUTPUT_FORMATTER6, 0xFF},//default: C9~C6 is input 
        {REG_CLEAR_LOST_LOCK, 0x01}//Clear status               
};

	UINT num_tvp_settings = (sizeof(tvpSettings)/sizeof(TVP_SETTINGS));

#else
    extern TVP_SETTINGS tvpSettings[];
	extern UINT num_tvp_settings;
#endif //INSTANT_TVP_SETTINGS

#endif
