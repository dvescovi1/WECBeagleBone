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
//  File:  omap_dss_regs.h
//
//  This header file is comprised of display module register details defined as
// structures and macros for configuring and controlling display module

#ifndef __OMAP_DSS_REGS_H
#define __OMAP_DSS_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

//
// Display Subsystem Specific (DSS)
//

// Base Address: OMAP_DSS1_REGS_PA

typedef struct
{
    REG32 DSS_REVISIONNUMBER;      //offset 0x00, Revision ID
    REG32 zzzRESERVED_1[3];
    REG32 DSS_SYSCONFIG;           //offset 0x10, OCP i/f params
    REG32 DSS_SYSSTATUS;           //offset 0x14, module status info
    REG32 zzzRESERVED_2[10];
    REG32 DSS_CONTROL;             //offset 0x40, DSS control bits
    REG32 DSS_SDI_CONTROL;         //offset 0x44, DSS control bits
    REG32 DSS_PLL_CONTROL;         //offset 0x48, DSS control bits
    REG32 zzzRESERVED_3[4];
    REG32 DSS_SDI_STATUS;          //offset 0x5C, DSS status
} OMAP_DSS_REGS;

//
// Sub-structure of OMAP_VID_REGS
//

typedef struct
{
    REG32 ulH;
    REG32 ulHV;
} OMAP_FIRCOEF_REGS;


//
// Sub-structure of OMAP_DISPC_REGS
//

typedef struct
{
    REG32 BA0;                     //offset 0x00, base addr config of vid buf0 for video win #n
    REG32 BA1;                     //offset 0x04, base addr config of vid buf1 for video win #n
    REG32 POSITION;                //offset 0x08, pos of vid win #n
    REG32 SIZE;                    //offset 0x0C, size of vid win #n
    REG32 ATTRIBUTES;              //offset 0x10, config of vid win #n
    REG32 FIFO_THRESHOLD;          //offset 0x14, video FIFO associated with video pipeline #n
    REG32 FIFO_SIZE_STATUS;        //offset 0x18, GFX FIFO size status
    REG32 ROW_INC;                 //offset 0x1C, no of bytes to inc at end of row
    REG32 PIXEL_INC;               //offset 0x20, no of bytes to incr bet 2 pixels
    REG32 FIR;                     //offset 0x24, resize factors for horz & vert up/down sampling of video win #n
    REG32 PICTURE_SIZE;            //offset 0x28, video pict size conf
    REG32 ACCU0;                   //offset 0x2C, configures resize accumulator init values for horz and vert up/dn-sampling
    REG32 ACCU1;                   //offset 0x30, configures resize accumulator init values for horz and vert up/dn-sampling
    OMAP_FIRCOEF_REGS aFIR_COEF[8]; //offset 0x34, configures up/dn-scaling coeff for vert & horz resize
                                    // of video pict asso with video win #n for the phase i
    REG32 CONV_COEF0;              //offset 0x74, color space conversion matrix coeff
    REG32 CONV_COEF1;              //offset 0x78, color space conversion matrix coeff
    REG32 CONV_COEF2;              //offset 0x7C, color space conversion matrix coeff
    REG32 CONV_COEF3;              //offset 0x80, color space conversion matrix coeff
    REG32 CONV_COEF4;              //offset 0x84, color space conversion matrix coeff
} OMAP_VID_REGS;


//
// Display Controller registers
//

// Base Address: OMAP_DISC1_REGS_PA

typedef struct
{
    REG32 DISPC_REVISION;              //offset 0x0, revision code
    REG32 zzzRESERVED_1[3];
    REG32 DISPC_SYSCONFIG;             //offset 0x10, OCP i/f params
    REG32 DISPC_SYSSTATUS;             //offset 0x14, module status info
    REG32 DISPC_IRQSTATUS;             //offset 0x18, module internal events status
    REG32 DISPC_IRQENABLE;             //offset 0x1C, module intr mask/unmask
    REG32 zzzRESERVED_2[8];
    REG32 DISPC_CONTROL;               //offset 0x40, module configuration
    REG32 DISPC_CONFIG;                //offset 0x44, shadow reg updated on VFP start period
    REG32 DISPC_CAPABLE;               //offset 0x48, shadow reg updated on VFP start period
    REG32 DISPC_DEFAULT_COLOR0;        //offset 0x4C, def solid bkgrnd color config
    REG32 DISPC_DEFAULT_COLOR1;        //offset 0x50, def solid bkgrnd color config
    REG32 DISPC_TRANS_COLOR0;          //offset 0x54, def trans colorvalue for video/graphics
    REG32 DISPC_TRANS_COLOR1;          //offset 0x58, def trans colorvalue for video/graphics
    REG32 DISPC_LINE_STATUS;           //offset 0x5C, current LCD panel display line number
    REG32 DISPC_LINE_NUMBER;           //offset 0x60, LCD panel display line number for  intr&DMA req
    REG32 DISPC_TIMING_H;              //offset 0x64, timing logic for HSYNC signal
    REG32 DISPC_TIMING_V;              //offset 0x68, timing logic for VSYNC signal
    REG32 DISPC_POL_FREQ;              //offset 0x6C, signal config
    REG32 DISPC_DIVISOR;               //offset 0x70, divisor config
    REG32 DISPC_GLOBAL_ALPHA;          //offset 0x74, global alpha color
    REG32 DISPC_SIZE_DIG;              //offset 0x78, configures frame, size of digital o/p field
    REG32 DISPC_SIZE_LCD;              //offset 0x7C, configure panel size
    REG32 DISPC_GFX_BA0;               //offset 0x80, base addr of GFX buf
    REG32 DISPC_GFX_BA1;               //offset 0x84, base addr of GFX buf
    REG32 DISPC_GFX_POSITION;          //offset 0x88, config posof GFX win
    REG32 DISPC_GFX_SIZE;              //offset 0x8C, size of GFX window
    REG32 zzzRESERVED_4[4];
    REG32 DISPC_GFX_ATTRIBUTES;        //offset 0xA0, config attr of GFX
    REG32 DISPC_GFX_FIFO_THRESHOLD;    //offset 0xA4, GFX FIFO config
    REG32 DISPC_GFX_FIFO_SIZE_STATUS;  //offset 0xA8, GFX FIFO size status
    REG32 DISPC_GFX_ROW_INC;           //offset 0xAC, no of bytes to inc at end of row.
    REG32 DISPC_GFX_PIXEL_INC;         //offset 0xB0, no of bytes to incr bet 2 pixels
    REG32 DISPC_GFX_WINDOW_SKIP;       //offset 0xB4, no of bytes to skip during video win#n disp
    REG32 DISPC_GFX_TABLE_BA;          //offset 0xB8, config base addr of palette buffer or the gamma tbl buf
    OMAP_VID_REGS tDISPC_VID1;          //offset 0xBC, video1 pipeline registers
    REG32 zzzRESERVED_5[2];
    OMAP_VID_REGS tDISPC_VID2;          //offset 0x14C, video2 pipeline registers
    REG32 DISPC_DATA_CYCLE1;           //offset 0x1D4, color space conversion matrix coeff
    REG32 DISPC_DATA_CYCLE2;           //offset 0x1D8, color space conversion matrix coeff
    REG32 DISPC_DATA_CYCLE3;           //offset 0x1DC, color space conversion matrix coeff
    REG32 DISPC_VID1_FIR_COEF_V[8];    //offset 0x1E0, additional vertical scaling coeffs for VID1, phases 0-7
    REG32 DISPC_VID2_FIR_COEF_V[8];    //offset 0x200, additional vertical scaling coeffs for VID2, phases 0-7
    REG32 DISPC_CPR_COEF_R;            //offset 0x220, color phase rotation coefficient - red component
    REG32 DISPC_CPR_COEF_G;            //offset 0x224, color phase rotation coefficient - green component
    REG32 DISPC_CPR_COEF_B;            //offset 0x228, color phase rotation coefficient - blue component
    REG32 DISPC_GFX_PRELOAD;           //offset 0x22C, GFX FIFO preload value
    REG32 DISPC_VID1_PRELOAD;          //offset 0x230, VID1 FIFO preload value
    REG32 DISPC_VID2_PRELOAD;          //offset 0x234, VID2 FIFO preload value
} OMAP_DISPC_REGS;


//
// Remote Frame Buffer
//

// Base Address: OMAP_RFBI1_REGS_PA

typedef struct
{
    REG32 RFBI_REVISION;               //offset 0x0, revision ID
    REG32 zzzRESERVED_1[3];
    REG32 RFBI_SYSCONFIG;              //offset 0x10, OCP i/f control
    REG32 RFBI_SYSSTATUS;              //offset 0x14, status information
    REG32 zzzRESERVED_2[10];
    REG32 RFBI_CONTROL;                //offset 0x40, module config
    REG32 RFBI_PIXEL_CNT;              //offset 0x44, pixel cnt value
    REG32 RFBI_LINE_NUMBER;            //offset 0x48, no of lines to sync the beginning of transfer
    REG32 RFBI_CMD;                    //offset 0x4C, cmd config
    REG32 RFBI_PARAM;                  //offset 0x50, parms config
    REG32 RFBI_DATA;                   //offset 0x54, data config
    REG32 RFBI_READ;                   //offset 0x58, read config
    REG32 RFBI_STATUS;                 //offset 0x5C, status config
    REG32 RFBI_CONFIG0;                //offset 0x60, config0 module
    REG32 RFBI_ONOFF_TIME0;            //offset 0x64, timing config
    REG32 RFBI_CYCLE_TIME0;            //offset 0x68, timing config
    REG32 RFBI_DATA_CYCLE1_0;          //offset 0x6C, data format for 1st cycle
    REG32 RFBI_DATA_CYCLE2_0;          //offset 0x70, data format for 2nd cycle
    REG32 RFBI_DATA_CYCLE3_0;          //offset 0x74, data format for 3rd cycle
    REG32 RFBI_CONFIG1;                //offset 0x78, config1 module
    REG32 RFBI_ONOFF_TIME1;            //offset 0x7C, timing config
    REG32 RFBI_CYCLE_TIME1;            //offset 0x80, timing config
    REG32 RFBI_DATA_CYCLE1_1;          //offset 0x84, data format for 1st cycle
    REG32 RFBI_DATA_CYCLE2_1;          //offset 0x88, data format for 2nd cycle
    REG32 RFBI_DATA_CYCLE3_1;          //offset 0x8C, data format for 3rd cycle
    REG32 RFBI_VSYNC_WIDTH;            //offset 0x90, VSYNC min pulse
    REG32 RFBI_HSYNC_WIDTH;            //offset 0x94, HSYNC max pulse
} OMAP_RFBI_REGS;


//
// Video Encoder registers
//

// Base Address: OMAP_VENC1_REGS_PA

typedef struct
{
    REG32 VENC_REV_ID;                         //offset 0x00
    REG32 VENC_STATUS;                         //offset 0x04
    REG32 VENC_F_CONTROL;                      //offset 0x08
    REG32 zzzRESERVED_1;
    REG32 VENC_VIDOUT_CTRL;                    //offset 0x10
    REG32 VENC_SYNC_CTRL;                      //offset 0x14
    REG32 zzzRESERVED_2;
    REG32 VENC_LLEN;                           //offset 0x1C
    REG32 VENC_FLENS;                          //offset 0x20
    REG32 VENC_HFLTR_CTRL;                     //offset 0x24
    REG32 VENC_CC_CARR_WSS_CARR;               //offset 0x28
    REG32 VENC_C_PHASE;                        //offset 0x2C
    REG32 VENC_GAIN_U;                         //offset 0x30
    REG32 VENC_GAIN_V;                         //offset 0x34
    REG32 VENC_GAIN_Y;                         //offset 0x38
    REG32 VENC_BLACK_LEVEL;                    //offset 0x3C
    REG32 VENC_BLANK_LEVEL;                    //offset 0x40
    REG32 VENC_X_COLOR;                        //offset 0x44
    REG32 VENC_M_CONTROL;                      //offset 0x48
    REG32 VENC_BSTAMP_WSS_DATA;                //offset 0x4C
    REG32 VENC_S_CARR;                         //offset 0x50
    REG32 VENC_LINE21;                         //offset 0x54
    REG32 VENC_LN_SEL;                         //offset 0x58
    REG32 VENC_L21__WC_CTL;                    //offset 0x5C
    REG32 VENC_HTRIGGER_VTRIGGER;              //offset 0x60
    REG32 VENC_SAVID_EAVID;                    //offset 0x64
    REG32 VENC_FLEN_FAL;                       //offset 0x68
    REG32 VENC_LAL_PHASE_RESET;                //offset 0x6C
    REG32 VENC_HS_INT_START_STOP_X;            //offset 0x70
    REG32 VENC_HS_EXT_START_STOP_X;            //offset 0x74
    REG32 VENC_VS_INT_START_X;                 //offset 0x78
    REG32 VENC_VS_INT_STOP_X__VS_INT_START_Y;  //offset 0x7C
    REG32 VENC_VS_INT_STOP_Y__VS_EXT_START_X;  //offset 0x80
    REG32 VENC_VS_EXT_STOP_X__VS_EXT_START_Y;  //offset 0x84
    REG32 VENC_VS_EXT_STOP_Y;                  //offset 0x88
    REG32 zzzRESERVED_3;
    REG32 VENC_AVID_START_STOP_X;              //offset 0x90
    REG32 VENC_AVID_START_STOP_Y;              //offset 0x94
    REG32 zzzRESERVED_4[2];
    REG32 VENC_FID_INT_START_X__FID_INT_START_Y;   //offset 0xA0
    REG32 VENC_FID_INT_OFFSET_Y__FID_EXT_START_X;  //offset 0xA4
    REG32 VENC_FID_EXT_START_Y__FID_EXT_OFFSET_Y;  //offset 0xA8
    REG32 zzzRESERVED_5;
    REG32 VENC_TVDETGP_INT_START_STOP_X;           //offset 0xB0
    REG32 VENC_TVDETGP_INT_START_STOP_Y;           //offset 0xB4
    REG32 VENC_GEN_CTRL;                           //offset 0xB8
    REG32 zzzRESERVED_6;
    REG32 zzzRESERVED_7;
    REG32 VENC_OUTPUT_CONTROL;                     //offset 0xC4
    REG32 VENC_OUTPUT_TEST;                        //offset 0xC8
} OMAP_VENC_REGS;

//
// DSI PLL
//

// Base Address OMAP_DSI_REGS_PA
typedef struct
{
    REG32 DSI_REVISION;                 //0x00
    REG32 zzzRESERVED_1[3];
    REG32 DSI_SYSCONFIG;                //0x10
    REG32 DSI_SYSSTATUS;                //0x14
    REG32 DSI_IRQSTATUS;                //0x18
    REG32 DSI_IRQENABLE;                //0x1C
    REG32 zzzRESERVED_2[8];
    REG32 DSI_CTRL;                     //0x40
    REG32 zzzRESERVED_3;
    REG32 DSI_COMPLEXIO_CFG;            //0x48
    REG32 DSI_COMPLEXIO_STATUS;         //0x4C
    REG32 DSI_COMPLEXIO_ENABLE;         //0x50
    REG32 DSI_CLK_CTRL;                 //0x54
    REG32 DSI_TIMING1;                  //0x58
    REG32 DSI_TIMING2;                  //0x5C
    REG32 DSI_VM_TIMING1;               //0x60
    REG32 DSI_VM_TIMING2;               //0x64
    REG32 DSI_VM_TIMING3;               //0x68
    REG32 DSI_CLK_TIMING;               //0x6C
    REG32 DSI_TX_FIFO_VC_SIZE;          //0x70
    REG32 DSI_RX_FIFO_VC_SIZE;          //0x74
    REG32 DSI_COMPLEXIO_CFG2;           //0x78
    REG32 DSI_RX_FIFO_VC_FULLNESS;      //0x7C
    REG32 DSI_VM_TIMING4;               //0x80
    REG32 DSI_TX_FIFO_VC_EMPTINESS;     //0x84
    REG32 DSI_VM_TIMING5;               //0x88
    REG32 DSI_VM_TIMING6;               //0x8C
    REG32 DSI_VM_TIMING7;               //0x90
    REG32 zzzRESERVED_4[3];
    REG32 DSI_VC_CTRL_BASE;             //0x100 to (0x100 + n * 0x20) n = 0 to 3
    REG32 DSI_VC_TE_BASE;               //0x104
    REG32 DSI_VC_LONG_PKTHEADER_BASE;   //0x108
    REG32 DSI_VC_LONG_PKTPAYLOAD_BASE;  //0x10C
    REG32 DSI_VC_SHORT_PKTHEADER_BASE;  //0x110
    REG32 zzzRESERVED_5[1];
    REG32 DSI_VC_IRQSTATUS_BASE;        //0x118
    REG32 DSI_VC_IRQENABLE_BASE;        //0x11C

} OMAP_DSI_REGS;

//
// DSI_PHYCFGx REGS
//
typedef struct
{
    REG32 DSI_PHYCFG0;                 //0x00
    REG32 DSI_PHYCFG1;                 //0x04
    REG32 DSI_PHYCFG2;                 //0x08
    REG32 zzzRESERVED_1[3];
    REG32 DSI_PHYCFG5;                 //0x14
} OMAP_DSIPHY_REGS;

//
// DSI_PLL REGS
//
typedef struct
{
    REG32 DSI_PLL_CONTROL;             //0x00
    REG32 DSI_PLL_STATUS;              //0x04
    REG32 DSI_PLL_GO;                  //0x08
    REG32 DSI_PLL_CONFIGURATION1;      //0x0C
    REG32 DSI_PLL_CONFIGURATION2;      //0x10
} OMAP_DSI_PLL_REGS;


// DSS_SYSCONFIG register fields

#define DSS_SYSCONFIG_AUTOIDLE                  (1 << 0)
#define DSS_SYSCONFIG_SOFTRESET                 (1 << 1)

// DSS_SYSSTATUS register fields

#define DSS_SYSSTATUS_RESETDONE                 (1 << 0)

// DSS_CONTROL register fields

#define DSS_CONTROL_DISPC_CLK_SWITCH_DSS1_ALWON (0 << 0)
#define DSS_CONTROL_DISPC_CLK_SWITCH_DSI1_PLL   (1 << 0)
#define DSS_CONTROL_DSI_CLK_SWITCH_DSS1_ALWON   (0 << 1)
#define DSS_CONTROL_DSI_CLK_SWITCH_DSI2_PLL     (1 << 1)
#define DSS_CONTROL_VENC_CLOCK_MODE_0           (0 << 2)
#define DSS_CONTROL_VENC_CLOCK_MODE_1           (1 << 2)
#define DSS_CONTROL_VENC_CLOCK_4X_ENABLE        (1 << 3)
#define DSS_CONTROL_DAC_DEMEN                   (1 << 4)
#define DSS_CONTROL_DAC_POWERDN_BGZ             (1 << 5)
#define DSS_CONTROL_DAC_VENC_OUT_SEL            (1 << 6)

// DSS_PSA_LCD_REG_2 register fields

#define DSS_PSA_LCD_2_SIG_MSB(sig)              ((sig) << 0)
#define DSS_PSA_LCD_2_DATA_AVAIL                (1 << 31)

// DSS_PSA_VIDEO_REG register fields

#define DSS_PSA_VIDEO_SIG(sig)                  ((sig) << 0)
#define DSS_PSA_VIDEO_DATA_AVAIL                (1 << 31)

// DSS_STATUS register fields

#define DSS_STATUS_DPLL_ENABLE                  (1 << 0)
#define DSS_STATUS_APLL_ENABLE                  (1 << 1)

// DISPC_SYSCONFIG register fields

#define DISPC_SYSCONFIG_AUTOIDLE                (1 << 0)
#define DISPC_SYSCONFIG_SOFTRESET               (1 << 1)
#define DISPC_SYSCONFIG_SIDLEMODE(mode)         ((mode) << 3)
#define DISPC_SYSCONFIG_MIDLEMODE(mode)         ((mode) << 12)

// DISPC_SYSSTATUS register fields

#define DISPC_SYSSTATUS_RESETDONE               (1 << 0)

// DISPC_CONTROL register fields

#define DISPC_CONTROL_LCDENABLE                 (1 << 0)
#define DISPC_CONTROL_DIGITALENABLE             (1 << 1)
#define DISPC_CONTROL_MONCOLOR                  (1 << 2)
#define DISPC_CONTROL_STNTFT                    (1 << 3)
#define DISPC_CONTROL_M8B                       (1 << 4)
#define DISPC_CONTROL_GOLCD                     (1 << 5)
#define DISPC_CONTROL_GODIGITAL                 (1 << 6)
#define DISPC_CONTROL_TFTDITHER_ENABLE          (1 << 7)
#define DISPC_CONTROL_TFTDATALINES_12           (0 << 8)
#define DISPC_CONTROL_TFTDATALINES_16           (1 << 8)
#define DISPC_CONTROL_TFTDATALINES_18           (2 << 8)
#define DISPC_CONTROL_TFTDATALINES_24           (3 << 8)
#define DISPC_CONTROL_SECURE                    (1 << 10)
#define DISPC_CONTROL_RFBIMODE                  (1 << 11)
#define DISPC_CONTROL_OVERLAY_OPTIMIZATION      (1 << 12)
#define DISPC_CONTROL_GPOUT0                    (1 << 15)
#define DISPC_CONTROL_GPOUT1                    (1 << 16)
#define DISPC_CONTROL_HT(ht)                    ((ht) << 17)
#define DISPC_CONTROL_TDMENABLE                 (1 << 20)
#define DISPC_CONTROL_TDMPARALLEL_MODE_8        (0 << 21)
#define DISPC_CONTROL_TDMPARALLEL_MODE_9        (1 << 21)
#define DISPC_CONTROL_TDMPARALLEL_MODE_12       (2 << 21)
#define DISPC_CONTROL_TDMPARALLEL_MODE_16       (3 << 21)
#define DISPC_CONTROL_TDMCYCLE_FORMAT_11        (0 << 23)
#define DISPC_CONTROL_TDMCYCLE_FORMAT_21        (1 << 23)
#define DISPC_CONTROL_TDMCYCLE_FORMAT_31        (2 << 23)
#define DISPC_CONTROL_TDMCYCLE_FORMAT_32        (3 << 23)
#define DISPC_CONTROL_TDMUNUSED_BITS_LO         (0 << 25)
#define DISPC_CONTROL_TDMUNUSED_BITS_HI         (1 << 25)
#define DISPC_CONTROL_TDMUNUSED_BITS_SAME       (2 << 25)
#define DISPC_CONTROL_PCKFREEENABLE_DISABLED    (0 << 27)
#define DISPC_CONTROL_PCKFREEENABLE_ENABLED     (1 << 27)
#define DISPC_CONTROL_LCDENABLESIGNAL_DISABLED  (0 << 28)
#define DISPC_CONTROL_LCDENABLESIGNAL_ENABLED   (1 << 28)
#define DISPC_CONTROL_LCDENABLEPOL_ACTIVELOW    (0 << 29)
#define DISPC_CONTROL_LCDENABLEPOL_ACTIVEHIGH   (1 << 29)
#define DISPC_CONTROL_TFTDITHERING_SPATIAL      (0 << 30)
#define DISPC_CONTROL_TFTDITHERING_TEMPORAL_2   (1 << 30)
#define DISPC_CONTROL_TFTDITHERING_TEMPORAL_4   (2 << 30)

// DISPC_CONFIG register fields

#define DISPC_CONFIG_PIXELGATED                 (1 << 0)
#define DISPC_CONFIG_LOADMODE(mode)             ((mode) << 1)
#define DISPC_CONFIG_PALETTEGAMMATABLE          (1 << 3)
#define DISPC_CONFIG_PIXELDATAGATED             (1 << 4)
#define DISPC_CONFIG_PIXELCLOCKGATED            (1 << 5)
#define DISPC_CONFIG_HSYNCGATED                 (1 << 6)
#define DISPC_CONFIG_VSYNCGATED                 (1 << 7)
#define DISPC_CONFIG_ACBIASGATED                (1 << 8)
#define DISPC_CONFIG_FUNCGATED                  (1 << 9)
#define DISPC_CONFIG_TCKLCDENABLE               (1 << 10)
#define DISPC_CONFIG_TCKLCDSELECTION            (1 << 11)
#define DISPC_CONFIG_TCKDIGENABLE               (1 << 12)
#define DISPC_CONFIG_TCKDIGSELECTION            (1 << 13)
#define DISPC_CONFIG_FIFOMERGE                  (1 << 14)
#define DISPC_CONFIG_CPR                        (1 << 15)
#define DISPC_CONFIG_FIFOHANDCHECK              (1 << 16)
#define DISPC_CONFIG_FIFOFILLING                (1 << 17)
#define DISPC_CONFIG_LCDALPHABLENDERENABLE      (1 << 18)
#define DISPC_CONFIG_DIGALPHABLENDERENABLE      (1 << 19)


// DISPC_TIMING_H register fields

#define DISPC_TIMING_H_HSW(hsw)                 ((hsw) << 0)
#define DISPC_TIMING_H_HFP(hfp)                 ((hfp) << 8)
#define DISPC_TIMING_H_HBP(hbp)                 ((hbp) << 20)

// DISPC_TIMING_V register fields

#define DISPC_TIMING_V_VSW(vsw)                 ((vsw) << 0)
#define DISPC_TIMING_V_VFP(vfp)                 ((vfp) << 8)
#define DISPC_TIMING_V_VBP(vbp)                 ((vbp) << 20)

// DISPC_POL_FREQ register fields

#define DISPC_POL_FREQ_ACB(acb)                 ((acb) << 0)
#define DISPC_POL_FREQ_ACBI(acbi)               ((acbi) << 8)
#define DISPC_POL_FREQ_IVS                      (1 << 12)
#define DISPC_POL_FREQ_IHS                      (1 << 13)
#define DISPC_POL_FREQ_IPC                      (1 << 14)
#define DISPC_POL_FREQ_IEO                      (1 << 15)
#define DISPC_POL_FREQ_RF                       (1 << 16)
#define DISPC_POL_FREQ_ONOFF                    (1 << 17)

// DISPC_DIVISOR register fields

#define DISPC_DIVISOR_PCD(pcd)                  ((pcd) << 0)
#define DISPC_DIVISOR_LCD(lcd)                  ((lcd) << 16)

// DISPC_GLOBAL_ALPHA register fields

#define DISPC_GLOBAL_ALPHA_GFX(color)           ((color & 0x000000ff) << 0)
#define DISPC_GLOBAL_ALPHA_VID2(color)          ((color & 0x000000ff) << 16)

// DISPC_SIZE_DIG register fields

#define DISPC_SIZE_DIG_PPL(ppl)                 (((ppl) - 1) << 0)
#define DISPC_SIZE_DIG_LPP(lpp)                 (((lpp) - 1) << 16)

// DISPC_SIZE_LCD register fields

#define DISPC_SIZE_LCD_PPL(ppl)                 (((ppl) - 1) << 0)
#define DISPC_SIZE_LCD_LPP(lpp)                 (((lpp) - 1) << 16)

// DISPC_GFX_POSITION register fields

#define DISPC_GFX_POS_GFXPOSX(x)                ((x) << 0)
#define DISPC_GFX_POS_GFXPOSY(y)                ((y) << 16)

// DISPC_GFX_SIZE register fields

#define DISPC_GFX_SIZE_GFXSIZEX(x)              (((x) - 1) << 0)
#define DISPC_GFX_SIZE_GFXSIZEY(y)              (((y) - 1) << 16)

// DISPC_GFX_ATTRIBUTES register fields

#define DISPC_GFX_ATTR_GFXENABLE                (1 << 0)
#define DISPC_GFX_ATTR_GFXFORMAT(fmt)           ((fmt) << 1)
#define DISPC_GFX_ATTR_GFXREPLICATIONENABLE     (1 << 5)
#define DISPC_GFX_ATTR_GFXBURSTSIZE_4x32        (0 << 6)
#define DISPC_GFX_ATTR_GFXBURSTSIZE_8x32        (1 << 6)
#define DISPC_GFX_ATTR_GFXBURSTSIZE_16x32       (2 << 6)
#define DISPC_GFX_ATTR_GFXCHANNELOUT            (1 << 8)
#define DISPC_GFX_ATTR_GFXNIBBLEMODE            (1 << 9)
#define DISPC_GFX_ATTR_FIFOPRELOAD              (1 << 11)
#define DISPC_GFX_ATTR_VIDROTATION_0            (0 << 12)
#define DISPC_GFX_ATTR_VIDROTATION_90           (1 << 12)
#define DISPC_GFX_ATTR_VIDROTATION_180          (2 << 12)
#define DISPC_GFX_ATTR_VIDROTATION_270          (3 << 12)
#define DISPC_GFX_ATTR_ARBITRATION              (1 << 14)
#define DISPC_GFX_ATTR_SELFREFRESH              (1 << 15)
#define DISPC_GFX_ATTR_PREMULTIPLYALPHA         (1 << 28)

// DISPC_GFX_FIFO_THRESHOLD register fields

#define DISPC_GFX_FIFO_THRESHOLD_LOW(low)       ((low) << 0)
#define DISPC_GFX_FIFO_THRESHOLD_HIGH(high)     ((high) << 16)

// DISPC_VID1_POSITION and DISPC_VID2_POSITION register fields

#define DISPC_VID_POS_VIDPOSX(x)                ((x) << 0)
#define DISPC_VID_POS_VIDPOSY(y)                ((y) << 16)

// DISPC_VID1_SIZE and DISPC_VID2_SIZE register fields

#define DISPC_VID_SIZE_VIDSIZEX(x)              (((x) - 1) << 0)
#define DISPC_VID_SIZE_VIDSIZEY(y)              (((y) - 1) << 16)

// DISPC_VID1_ATTRIBUTES and DISPC_VID2_ATTRIBUTES register fields

#define DISPC_VID_ATTR_VIDENABLE                (1 << 0)
#define DISPC_VID_ATTR_VIDFORMAT(fmt)           ((fmt) << 1)
#define DISPC_VID_ATTR_VIDRESIZE_NONE           (0 << 5)
#define DISPC_VID_ATTR_VIDRESIZE_HORIZONTAL     (1 << 5)
#define DISPC_VID_ATTR_VIDRESIZE_VERTICAL       (2 << 5)
#define DISPC_VID_ATTR_VIDRESIZE_BOTH           (3 << 5)
#define DISPC_VID_ATTR_VIDHRESIZE_CONF_UP       (0 << 7)
#define DISPC_VID_ATTR_VIDHRESIZE_CONF_DOWN     (1 << 7)
#define DISPC_VID_ATTR_VIDVRESIZE_CONF_UP       (0 << 8)
#define DISPC_VID_ATTR_VIDVRESIZE_CONF_DOWN     (1 << 8)
#define DISPC_VID_ATTR_VIDRESIZE_MASK           (0xF << 5)
#define DISPC_VID_ATTR_VIDCOLORCONVENABLE       (1 << 9)
#define DISPC_VID_ATTR_VIDREPLICATIONENABLE     (1 << 10)
#define DISPC_VID_ATTR_VIDFULLRANGE             (1 << 11)
#define DISPC_VID_ATTR_VIDROTATION_0            (0 << 12)
#define DISPC_VID_ATTR_VIDROTATION_90           (1 << 12)
#define DISPC_VID_ATTR_VIDROTATION_180          (2 << 12)
#define DISPC_VID_ATTR_VIDROTATION_270          (3 << 12)
#define DISPC_VID_ATTR_VIDROTATION_MASK         (3 << 12)
#define DISPC_VID_ATTR_VIDBURSTSIZE_4x32        (0 << 14)
#define DISPC_VID_ATTR_VIDBURSTSIZE_8x32        (1 << 14)
#define DISPC_VID_ATTR_VIDBURSTSIZE_16x32       (2 << 14)
#define DISPC_VID_ATTR_VIDCHANNELOUT            (1 << 16)
#define DISPC_VID_ATTR_VIDENDIANNESS            (1 << 17)
#define DISPC_VID_ATTR_VIDROWREPEATENABLE       (1 << 18)
#define DISPC_VID_ATTR_VIDFIFOPRELOAD           (1 << 19)
#define DISPC_VID_ATTR_VIDDMAOPTIMIZATION       (1 << 20)
#define DISPC_VID_ATTR_VIDVERTICALTAPS_3        (0 << 21)
#define DISPC_VID_ATTR_VIDVERTICALTAPS_5        (1 << 21)
#define DISPC_VID_ATTR_VIDLINEBUFFERSPLIT       (1 << 22)
#define DISPC_VID_ATTR_VIDARBITRATION           (1 << 23)
#define DISPC_VID_ATTR_VIDSELFREFRESH           (1 << 24)
#define DISPC_VID_ATTR_PREMULTIPLYALPHA         (1 << 28)

// DISPC_VID1_FIFO_THRESHOLD and DISPC_VID2_FIFO_THRESHOLD register fields

#define DISPC_VID_FIFO_THRESHOLD_LOW(low)       ((low) << 0)
#define DISPC_VID_FIFO_THRESHOLD_HIGH(high)     ((high) << 16)

// DISPC_VID1_PICTURE_SIZE and DISPC_VID2_PICTURE_SIZE register fields

#define DISPC_VID_PICTURE_SIZE_VIDORGSIZEX(x)   (((x) - 1) << 0)
#define DISPC_VID_PICTURE_SIZE_VIDORGSIZEY(y)   (((y) - 1) << 16)

// DISPC_VID1_FIR and DISPC_VID2_FIR register fields

#define DISPC_VID_FIR_VIDFIRHINC(inc)           ((inc) << 0)
#define DISPC_VID_FIR_VIDFIRVINC(inc)           ((inc) << 16)

// DISPC_VID1_CONV_COEF0 and DISPC_VID2_CONV_COEF0 register fields

#define DISPC_VID_CONV_COEF0_RY(ry)             ((ry&0x7FF) << 0)
#define DISPC_VID_CONV_COEF0_RCR(rcr)           ((rcr&0x7FF) << 16)

// DISPC_VID1_CONV_COEF1 and DISPC_VID2_CONV_COEF1 register fields

#define DISPC_VID_CONV_COEF1_RCB(rcb)           ((rcb&0x7FF) << 0)
#define DISPC_VID_CONV_COEF1_GY(gy)             ((gy&0x7FF) << 16)

// DISPC_VID1_CONV_COEF2 and DISPC_VID2_CONV_COEF2 register fields

#define DISPC_VID_CONV_COEF2_GCR(gcr)           ((gcr&0x7FF) << 0)
#define DISPC_VID_CONV_COEF2_GCB(gcb)           ((gcb&0x7FF) << 16)

// DISPC_VID1_CONV_COEF3 and DISPC_VID2_CONV_COEF3 register fields

#define DISPC_VID_CONV_COEF3_BY(by)             ((by&0x7FF) << 0)
#define DISPC_VID_CONV_COEF3_BCR(bcr)           ((bcr&0x7FF) << 16)

// DISPC_VID1_CONV_COEF4 and DISPC_VID2_CONV_COEF4 register fields

#define DISPC_VID_CONV_COEF4_BCB(bcb)           ((bcb&0x7FF) << 0)


// VENC_F_CONTROL register fields

#define VENC_F_CONTROL_FMT(fmt)                 ((fmt) << 0)
#define VENC_F_CONTROL_BACKGND(clr)             ((clr) << 2)
#define VENC_F_CONTROL_RGBF_RANGE_FULL          (0 << 5)
#define VENC_F_CONTROL_RGBF_RANGE_ITU601        (1 << 5)
#define VENC_F_CONTROL_SVDS_EXT                 (0 << 6)
#define VENC_F_CONTROL_SVDS_COLOR_BAR           (1 << 6)
#define VENC_F_CONTROL_SVDS_BACKGND_COLOR       (2 << 6)
#define VENC_F_CONTROL_RESET                    (1 << 8)



//DISPC_IRQSTATUS

#define DISPC_IRQSTATUS_FRAMEDONE               (1 << 0)
#define DISPC_IRQSTATUS_VSYNC                   (1 << 1)
#define DISPC_IRQSTATUS_EVSYNC_EVEN             (1 << 2)
#define DISPC_IRQSTATUS_EVSYNC_ODD              (1 << 3)
#define DISPC_IRQSTATUS_ACBIASCOUNTSTATUS       (1 << 4)
#define DISPC_IRQSTATUS_PROGRAMMEDLINENUMBER    (1 << 5)
#define DISPC_IRQSTATUS_GFXFIFOUNDERFLOW        (1 << 6)
#define DISPC_IRQSTATUS_GFXENDWINDOW            (1 << 7)
#define DISPC_IRQSTATUS_PALLETEGAMMALOADING     (1 << 8)
#define DISPC_IRQSTATUS_OCPERROR                (1 << 9)
#define DISPC_IRQSTATUS_VID1FIFOUNDERFLOW       (1 << 10)
#define DISPC_IRQSTATUS_VID1ENDWINDOW           (1 << 11)
#define DISPC_IRQSTATUS_VID2FIFOUNDERFLOW       (1 << 12)
#define DISPC_IRQSTATUS_VID2ENDWINDOW           (1 << 13)
#define DISPC_IRQSTATUS_SYNCLOST                (1 << 14)
#define DISPC_IRQSTATUS_SYNCLOSTDIGITAL         (1 << 15)
#define DISPC_IRQSTATUS_WAKEUP                  (1 << 16)

//DISPC_IRQENABLE

#define DISPC_IRQENABLE_FRAMEDONE               (1 << 0)
#define DISPC_IRQENABLE_VSYNC                   (1 << 1)
#define DISPC_IRQENABLE_EVSYNC_EVEN             (1 << 2)
#define DISPC_IRQENABLE_EVSYNC_ODD              (1 << 3)
#define DISPC_IRQENABLE_ACBIASCOUNTSTATUS       (1 << 4)
#define DISPC_IRQENABLE_PROGRAMMEDLINENUMBER    (1 << 5)
#define DISPC_IRQENABLE_GFXFIFOUNDERFLOW        (1 << 6)
#define DISPC_IRQENABLE_GFXENDWINDOW            (1 << 7)
#define DISPC_IRQENABLE_PALLETEGAMMALOADING     (1 << 8)
#define DISPC_IRQENABLE_OCPERROR                (1 << 9)
#define DISPC_IRQENABLE_VID1FIFOUNDERFLOW       (1 << 10)
#define DISPC_IRQENABLE_VID1ENDWINDOW           (1 << 11)
#define DISPC_IRQENABLE_VID2FIFOUNDERFLOW       (1 << 12)
#define DISPC_IRQENABLE_VID2ENDWINDOW           (1 << 13)
#define DISPC_IRQENABLE_SYNCLOST                (1 << 14)
#define DISPC_IRQENABLE_SYNCLOSTDIGITAL         (1 << 15)
#define DISPC_IRQENABLE_WAKEUP                  (1 << 16)


//DISPC_PIXELFORMAT (pixel formats for GFX, VID1 and VID2)

#define DISPC_PIXELFORMAT_BITMAP1               (0x0)
#define DISPC_PIXELFORMAT_BITMAP2               (0x1)
#define DISPC_PIXELFORMAT_BITMAP4               (0x2)
#define DISPC_PIXELFORMAT_BITMAP8               (0x3)
#define DISPC_PIXELFORMAT_RGB12                 (0x4)
#define DISPC_PIXELFORMAT_ARGB16                (0x5)
#define DISPC_PIXELFORMAT_RGB16                 (0x6)
#define DISPC_PIXELFORMAT_RGB32                 (0x8)
#define DISPC_PIXELFORMAT_RGB24                 (0x9)
#define DISPC_PIXELFORMAT_YUV2                  (0xA)
#define DISPC_PIXELFORMAT_UYVY                  (0xB)
#define DISPC_PIXELFORMAT_ARGB32                (0xC)
#define DISPC_PIXELFORMAT_RGBA32                (0xD)
#define DISPC_PIXELFORMAT_RGBx32                (0xE)


// DSI REGISTERS FIELDS

// NOTE: Only the relevant registers reqd for clock configuration
//       is current defined in this file.

// DSS_SYSCONFIG register fields
#define DSI_CLK_CTRL_LP_CLK_DIVISOR(x)           (x << 0)    //[12:0]
#define DSI_CLK_CTRL_DDR_CLK_ALWAYS_ON_ENABLED   (1 << 13)
#define DSI_CLK_CTRL_CIO_CLK_ICG_ENABLED         (1 << 14)
#define DSI_CLK_CTRL_LP_CLK_NULL_PKT_ENABLED     (1 << 15)
#define DSI_CLK_CTRL_LP_CLK_NULL_PKT_SIZE(x)     (x << 16)   //[17:16]
#define DSI_CLK_CTRL_HS_AUTO_STOP_ENABLED        (1 << 18)
#define DSI_CLK_CTRL_HS_MANUEL_STOP_CTRL         (1 << 19)
#define DSI_CLK_CTRL_LP_CLK_ENABLE               (1 << 20)
#define DSI_CLK_CTRL_LP_RX_SYNCHRO_ENABLE        (1 << 21)

#define DSI_CLK_CTRL_PLL_PWR_STATUS_OFF          (0 << 28)
#define DSI_CLK_CTRL_PLL_PWR_STATUS_ON_PLLONLY   (1 << 28)
#define DSI_CLK_CTRL_PLL_PWR_STATUS_ON_PLLANDHS  (2 << 28)
#define DSI_CLK_CTRL_PLL_PWR_STATUS_ON_DISPCONLY (3 << 28)

#define DSI_CLK_CTRL_PLL_PWR_CMD_OFF             (0 << 30)
#define DSI_CLK_CTRL_PLL_PWR_CMD_ON_PLLONLY      (1 << 30)
#define DSI_CLK_CTRL_PLL_PWR_CMD_ON_PLLANDHS     (2 << 30)
#define DSI_CLK_CTRL_PLL_PWR_CMD_ON_DISPCONLY    (3 << 30)
#define DSI_CLK_CTRL_PLL_PWR_CMD_MASK            (3 << 30)


#define DSI_PLL_AUTOMODE                         (1 << 0)
#define DSI_PLL_GATEMODE                         (1 << 1)
#define DSI_PLL_HALTMODE                         (1 << 2)
#define DSI_PLL_SYSRESET                         (1 << 3)
#define DSI_HSDIV_SYSRESET                       (1 << 4) //SYSRESET forced active

#define DSI_PLLCTRL_RESET_DONE_STATUS            (1 << 0)
#define DSI_PLL_LOCK_STATUS                      (1 << 1)
#define DSI_PLL_RECAL_STATUS                     (1 << 2)
#define DSI_PLL_LOSSREF_STATUS                   (1 << 3)
#define DSI_PLL_LIMP_STATUS                      (1 << 4)
#define DSI_PLL_HIGHJITTER_STATUS                (1 << 5)
#define DSI_PLL_BYPASS_STATUS                    (1 << 6)
#define DSS_CLOCK_ACK_STATUS                     (1 << 7)
#define DSIPROTO_CLOCK_ACK_STATUS                (1 << 8)
#define DSI_BYPASSACKZ_STATUS                    (1 << 9)

#define DSI_PLL_GO_CMD                           (1 << 0) //Request PLL (re)locking/locking pending

#define DSI_PLL_STOPMODE                         (1 << 0)
#define DSI_PLL_REGN(x)                          (x << 1) // [7:1]
#define DSI_PLL_REGM(x)                          (x << 8) // [18:8]
#define DSS_CLOCK_DIV(x)                         (x << 19)// [22:19]
#define DSIPROTO_CLOCK_DIV(x)                    (x << 23)// [26:23]



#define DSI_PLL_FREQSEL_1                        (1)
#define DSI_PLL_FREQSEL_2                        (2)
#define DSI_PLL_FREQSEL_7                        (7)


#define DSI_PLL_IDLE                             (1 << 0)
#define DSI_PLL_FREQSEL(x)                       (x << 1)
#define DSI_PLL_PLLLPMODE_REDUCEDPWR             (1 << 5)
#define DSI_PLL_LOWCURRSTBY                      (1 << 6)
#define DSI_PLL_TIGHTPHASELOCK                   (1 << 7)
#define DSI_PLL_DRIFTGUARDEN                     (1 << 8)
#define DSI_PLL_LOCKSEL(x)                       (1 << 9)
#define DSI_PLL_CLKSEL_PCLKFREE                  (1 << 11)
#define DSI_PLL_CLKSEL_DSS2ALWON_FCLK            (0 << 11)
#define DSI_PLL_HIGHFREQ_PIXELCLKBY2             (1 << 12)
#define DSI_PLL_REFEN                            (1 << 13)
#define DSIPHY_CLKINEN                           (1 << 14)
#define DSI_BYPASSEN_USEDSSFCLK                  (1 << 15)
#define DSS_CLOCK_EN                             (1 << 16)
#define DSS_CLOCK_PWDN                           (1 << 17)
#define DSI_PROTO_CLOCK_EN                       (1 << 18)
#define DSI_PROTO_CLOCK_PWDN                     (1 << 19)
#define DSI_HSDIVBYPASS                          (1 << 20)

#define DSS_FCLK_MAX                             (172000000)
#define DSI_HIGHFREQ_MAX                         (32000000)

#ifdef __cplusplus
}
#endif

#endif
