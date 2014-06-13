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

//------------------------------------------------------------------------------
//
//  File:  omap_prcm_regs.h
//
//  This header file is comprised of PRCM module register details defined as 
//  structures and macros for configuring and controlling PRCM module.

#ifndef __OMAP_PRCM_REGS_H
#define __OMAP_PRCM_REGS_H

#pragma warning(push)
#pragma warning(disable:4201)
//------------------------------------------------------------------------------
//
//  Define: DVFS_LOW_OPP_STALL
//
//  Software Delay after updating CORE Dpll
//
//  Values should be in sync with omap35xx\inc\omap35xx_const.inc
//
#define DVFS_LOW_OPP_STALL             0x0000
#define DVFS_HIGH_OPP_STALL            0x0010

//------------------------------------------------------------------------------

#define PRCM_CLKSSETUP_T2_DELAY         (0xFF)
#define PRCM_CLKSSETUP_DELAY            PRCM_CLKSSETUP_T2_DELAY

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_CLKEN_PLL;              // offset 0x0000 
    REG32 CM_CLKEN2_PLL;             // offset 0x0004
    REG32 zzzReserved01[6];

    REG32 CM_IDLEST_CKGEN;           // offset 0x0020 
    REG32 CM_IDLEST2_CKGEN;          // offset 0x0024
    REG32 zzzReserved02[2];

    REG32 CM_AUTOIDLE_PLL;           // offset 0x0030 
    REG32 CM_AUTOIDLE2_PLL;          // offset 0x0034
    REG32 zzzReserved03[2];

    REG32 CM_CLKSEL1_PLL;            // offset 0x0040 
    REG32 CM_CLKSEL2_PLL;            // offset 0x0044 
    REG32 CM_CLKSEL3_PLL;            // offset 0x0048 
    REG32 CM_CLKSEL4_PLL;            // offset 0x004C 
    REG32 CM_CLKSEL5_PLL;            // offset 0x0050 
    REG32 zzzReserved04[7];

    REG32 CM_CLKOUT_CTRL;            // offset 0x0070 

} OMAP_PRCM_CLOCK_CONTROL_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN1_CORE;            // offset 0x0000 
    REG32 zzzReserved00[1];           // offset 0x0004
    REG32 CM_FCLKEN3_CORE;            // offset 0x0008
    REG32 zzzReserved01[1];

    REG32 CM_ICLKEN1_CORE;            // offset 0x0010 
    REG32 CM_ICLKEN2_CORE;            // offset 0x0014 
    REG32 CM_ICLKEN3_CORE;            // offset 0x0018
    REG32 zzzReserved02[1];

    REG32 CM_IDLEST1_CORE;            // offset 0x0020 
    REG32 CM_IDLEST2_CORE;            // offset 0x0024 
    REG32 CM_IDLEST3_CORE;            // offset 0x0028
    REG32 zzzReserved03[1];

    REG32 CM_AUTOIDLE1_CORE;          // offset 0x0030 
    REG32 CM_AUTOIDLE2_CORE;          // offset 0x0034 
    REG32 CM_AUTOIDLE3_CORE;          // offset 0x0038
    REG32 zzzReserved04[1];

    REG32 CM_CLKSEL_CORE;             // offset 0x0040 
    REG32 zzzReserved05[1];           // offset 0x0044
    REG32 CM_CLKSTCTRL_CORE;          // offset 0x0048 
    REG32 CM_CLKSTST_CORE;            // offset 0x004C 

} OMAP_PRCM_CORE_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_IVA2;             // offset 0x0000 
    REG32 CM_CLKEN_PLL_IVA2;          // offset 0x0004 
    REG32 zzzReserved01[6];

    REG32 CM_IDLEST_IVA2;             // offset 0x0020 
    REG32 CM_IDLEST_PLL_IVA2;         // offset 0x0024 
    REG32 zzzReserved02[3];

    REG32 CM_AUTOIDLE_PLL_IVA2;       // offset 0x0034 
    REG32 zzzReserved03[2];

    REG32 CM_CLKSEL1_PLL_IVA2;        // offset 0x0040 
    REG32 CM_CLKSEL2_PLL_IVA2;        // offset 0x0044 
    REG32 CM_CLKSTCTRL_IVA2;          // offset 0x0048 
    REG32 CM_CLKSTST_IVA2;            // offset 0x004C 

} OMAP_PRCM_IVA2_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[1];

    REG32 CM_CLKEN_PLL_MPU;           // offset 0x0004 
    REG32 zzzReserved02[6];

    REG32 CM_IDLEST_MPU;              // offset 0x0020 
    REG32 CM_IDLEST_PLL_MPU;          // offset 0x0024 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_PLL_MPU;        // offset 0x0034 
    REG32 zzzReserved04[2];

    REG32 CM_CLKSEL1_PLL_MPU;         // offset 0x0040 
    REG32 CM_CLKSEL2_PLL_MPU;         // offset 0x0044 
    REG32 CM_CLKSTCTRL_MPU;           // offset 0x0048 
    REG32 CM_CLKSTST_MPU;             // offset 0x004C 

} OMAP_PRCM_MPU_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_SGX;               // offset 0x0000 
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_SGX;               // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_SGX;               // offset 0x0020 
    REG32 zzzReserved03[7];

    REG32 CM_CLKSEL_SGX;               // offset 0x0040 
    REG32 CM_SLEEPDEP_SGX;             // offset 0x0044 
    REG32 CM_CLKSTCTRL_SGX;            // offset 0x0048 
    REG32 CM_CLKSTST_SGX;              // offset 0x004C 

} OMAP_PRCM_SGX_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_DSS;               // offset 0x0000 
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_DSS;               // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_DSS;               // offset 0x0020 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_DSS;             // offset 0x0030 
    REG32 zzzReserved04[3];
    
    REG32 CM_CLKSEL_DSS;               // offset 0x0040 
    REG32 CM_SLEEPDEP_DSS;             // offset 0x0044 
    REG32 CM_CLKSTCTRL_DSS;            // offset 0x0048 
    REG32 CM_CLKSTST_DSS;              // offset 0x004C 

} OMAP_PRCM_DSS_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_CAM;               // offset 0x0000     
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_CAM;               // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_CAM;               // offset 0x0020 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_CAM;             // offset 0x0030 
    REG32 zzzReserved04[3];

    REG32 CM_CLKSEL_CAM;               // offset 0x0040 
    REG32 CM_SLEEPDEP_CAM;             // offset 0x0044 
    REG32 CM_CLKSTCTRL_CAM;            // offset 0x0048 
    REG32 CM_CLKSTST_CAM;              // offset 0x004C 

} OMAP_PRCM_CAM_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_PER;               // offset 0x0000 
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_PER;               // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_PER;               // offset 0x0020 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_PER;             // offset 0x0030 
    REG32 zzzReserved04[3];

    REG32 CM_CLKSEL_PER;               // offset 0x0040 
    REG32 CM_SLEEPDEP_PER;             // offset 0x0044 
    REG32 CM_CLKSTCTRL_PER;            // offset 0x0048 
    REG32 CM_CLKSTST_PER;              // offset 0x004C 

} OMAP_PRCM_PER_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_WKUP;              // offset 0x0000     
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_WKUP;              // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_WKUP;              // offset 0x0020 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_WKUP;            // offset 0x0030 
    REG32 zzzReserved04[3];

    REG32 CM_CLKSEL_WKUP;              // offset 0x0040 

} OMAP_PRCM_WKUP_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[16];

    REG32 CM_CLKSEL1_EMU;              // offset 0x0040 
    REG32 zzzReserved02[1];

    REG32 CM_CLKSTCTRL_EMU;            // offset 0x0048 
    REG32 CM_CLKSTST_EMU;              // offset 0x004C 
    REG32 CM_CLKSEL2_EMU;              // offset 0x0050 
    REG32 CM_CLKSEL3_EMU;              // offset 0x0054 

} OMAP_PRCM_EMU_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[8];

    REG32 CM_IDLEST_NEON;               // offset 0x0020 
    REG32 zzzReserved02[9];
    REG32 CM_CLKSTCTRL_NEON;            // offset 0x0048 

} OMAP_PRCM_NEON_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_FCLKEN_USBHOST;            // offset 0x0000     
    REG32 zzzReserved01[3];

    REG32 CM_ICLKEN_USBHOST;            // offset 0x0010 
    REG32 zzzReserved02[3];

    REG32 CM_IDLEST_USBHOST;            // offset 0x0020 
    REG32 zzzReserved03[3];

    REG32 CM_AUTOIDLE_USBHOST;          // offset 0x0030 
    REG32 zzzReserved04[4];

    REG32 CM_SLEEPDEP_USBHOST;          // offset 0x0044 
    REG32 CM_CLKSTCTRL_USBHOST;         // offset 0x0048 
    REG32 CM_CLKSTST_USBHOST;           // offset 0x004C 

} OMAP_PRCM_USBHOST_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 CM_REVISION;                  // offset 0x0000
    REG32 zzzReserved01[3];

    REG32 CM_SYSCONFIG;                 // offset 0x0010

} OMAP_PRCM_OCP_SYSTEM_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[39];

    REG32 CM_POLCTRL;                   // offset 0x009C 

} OMAP_PRCM_GLOBAL_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[1];

    REG32 PRM_REVISION;                 // offset 0x0004 
    REG32 zzzReserved02[3];

    REG32 PRM_SYSCONFIG;                // offset 0x0014 
    REG32 PRM_IRQSTATUS_MPU;            // offset 0x0018 
    REG32 PRM_IRQENABLE_MPU;            // offset 0x001C 

} OMAP_PRCM_OCP_SYSTEM_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[8];

    REG32 PRM_VC_SMPS_SA;               // offset 0x0020 
    REG32 PRM_VC_SMPS_VOL_RA;           // offset 0x0024 
    REG32 PRM_VC_SMPS_CMD_RA;           // offset 0x0028 
    REG32 PRM_VC_CMD_VAL_0;             // offset 0x002C 
    REG32 PRM_VC_CMD_VAL_1;             // offset 0x0030 
    REG32 PRM_VC_CH_CONF;               // offset 0x0034 
    REG32 PRM_VC_I2C_CFG;               // offset 0x0038 
    REG32 PRM_VC_BYPASS_VAL;            // offset 0x003C 
    REG32 zzzReserved02[4];             // offset 0x0040

    REG32 PRM_RSTCTRL;                  // offset 0x0050 
    REG32 PRM_RSTTIME;                  // offset 0x0054 
    REG32 PRM_RSTST;                    // offset 0x0058
    REG32 zzzReserved03[1];             // offset 0x005C
    REG32 PRM_VOLTCTRL;                 // offset 0x0060 
    REG32 PRM_SRAM_PCHARGE;             // offset 0x0064 
    REG32 zzzReserved04[2];             // offset 0x0068

    REG32 PRM_CLKSRC_CTRL;              // offset 0x0070 
    REG32 zzzReserved05[3];             // offset 0x0074

    REG32 PRM_OBS;                      // offset 0x0080
    REG32 zzzReserved06[3];             // offset 0x0084

    REG32 PRM_VOLTSETUP1;               // offset 0x0090 
    REG32 PRM_VOLTOFFSET;               // offset 0x0094 
    REG32 PRM_CLKSETUP;                 // offset 0x0098 
    REG32 PRM_POLCTRL;                  // offset 0x009C 
    REG32 PRM_VOLTSETUP2;               // offset 0x00A0 
    REG32 zzzReserved07[3];

    REG32 PRM_VP1_CONFIG;               // offset 0x00B0 
    REG32 PRM_VP1_VSTEPMIN;             // offset 0x00B4
    REG32 PRM_VP1_VSTEPMAX;             // offset 0x00B8
    REG32 PRM_VP1_VLIMITTO;             // offset 0x00BC
    REG32 PRM_VP1_VOLTAGE;              // offset 0x00C0
    REG32 PRM_VP1_STATUS;               // offset 0x00C4
    REG32 zzzReserved08[2];             // offset 0x00C8
    
    REG32 PRM_VP2_CONFIG;               // offset 0x00D0
    REG32 PRM_VP2_VSTEPMIN;             // offset 0x00D4
    REG32 PRM_VP2_VSTEPMAX;             // offset 0x00D8
    REG32 PRM_VP2_VLIMITTO;             // offset 0x00DC
    REG32 PRM_VP2_VOLTAGE;              // offset 0x00E0
    REG32 PRM_VP2_STATUS;               // offset 0x00E4

    REG32 zzzReserved09[2];             // offset 0x00E8

    // DM3730 only
    REG32 PRM_LDO_ABB_SETUP;            // offset 0x00F0
    REG32 PRM_LDO_ABB_CTRL;             // offset 0x00F4
	
} OMAP_PRCM_GLOBAL_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[20];

    REG32 RM_RSTCTRL_CORE;              // offset 0x0050 
    REG32 zzzReserved02[1];

    REG32 RM_RSTST_CORE;                // offset 0x0058 
    REG32 zzzReserved03[17];

    REG32 PM_WKEN1_CORE;                // offset 0x00A0 
    REG32 PM_MPUGRPSEL1_CORE;           // offset 0x00A4 
    REG32 PM_IVA2GRPSEL1_CORE;          // offset 0x00A8 
    REG32 zzzReserved04[1];

    REG32 PM_WKST1_CORE;                // offset 0x00B0 
    REG32 zzzReserved05[1];             // offset 0x00B4
    REG32 PM_WKST3_CORE;                // offset 0x00B8
    REG32 zzzReserved06[9];             // offset 0x00BC

    REG32 PM_PWSTCTRL_CORE;             // offset 0x00E0 
    REG32 PM_PWSTST_CORE;               // offset 0x00E4 
    REG32 PM_PREPWSTST_CORE;            // offset 0x00E8 

    REG32 zzzReserved07[1];             // offset 0x00EC
    REG32 PM_WKEN3_CORE;                // offset 0x00F0
    REG32 PM_IVA2GRPSEL3_CORE;          // offset 0x00F4
    REG32 PM_MPUGRPSEL3_CORE;           // offset 0x00F8

} OMAP_PRCM_CORE_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[20];

    REG32 RM_RSTCTRL_IVA2;              // offset 0x0050 
    REG32 zzzReserved02[1];

    REG32 RM_RSTST_IVA2;                // offset 0x0058 
    REG32 zzzReserved03[27];

    REG32 PM_WKDEP_IVA2;                // offset 0x00C8 
    REG32 zzzReserved04[5];

    REG32 PM_PWSTCTRL_IVA2;             // offset 0x00E0 
    REG32 PM_PWSTST_IVA2;               // offset 0x00E4 
    REG32 PM_PREPWSTST_IVA2;            // offset 0x00E8 
    REG32 zzzReserved05[3];

    REG32 PRM_IRQSTATUS_IVA2;           // offset 0x00F8 
    REG32 PRM_IRQENABLE_IVA2;           // offset 0x00FC 

} OMAP_PRCM_IVA2_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_PER;                 // offset 0x0058 
    REG32 zzzReserved02[17];

    REG32 PM_WKEN_PER;                  // offset 0x00A0 
    REG32 PM_MPUGRPSEL_PER;             // offset 0x00A4 
    REG32 PM_IVA2GRPSEL_PER;            // offset 0x00A8 
    REG32 zzzReserved03[1];

    REG32 PM_WKST_PER;                  // offset 0x00B0 
    REG32 zzzReserved04[5];

    REG32 PM_WKDEP_PER;                 // offset 0x00C8 
    REG32 zzzReserved05[5];

    REG32 PM_PWSTCTRL_PER;              // offset 0x00E0 
    REG32 PM_PWSTST_PER;                // offset 0x00E4 
    REG32 PM_PREPWSTST_PER;             // offset 0x00E8 

} OMAP_PRCM_PER_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_DSS;                 // offset 0x0058 
    REG32 zzzReserved02[17];

    REG32 PM_WKEN_DSS;                  // offset 0x00A0 
    REG32 zzzReserved03[9];

    REG32 PM_WKDEP_DSS;                 // offset 0x00C8 
    REG32 zzzReserved04[5];

    REG32 PM_PWSTCTRL_DSS;              // offset 0x00E0 
    REG32 PM_PWSTST_DSS;                // offset 0x00E4 
    REG32 PM_PREPWSTST_DSS;             // offset 0x00E8 

} OMAP_PRCM_DSS_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_MPU;                 // offset 0x0058 
    REG32 zzzReserved02[27];

    REG32 PM_WKDEP_MPU;                 // offset 0x00C8 
    REG32 zzzReserved03[2];

    REG32 PM_EVGENCTRL_MPU;             // offset 0x00D4 
    REG32 PM_EVGENONTIM_MPU;            // offset 0x00D8 
    REG32 PM_EVGENOFFTIM_MPU;           // offset 0x00DC 
    REG32 PM_PWSTCTRL_MPU;              // offset 0x00E0 
    REG32 PM_PWSTST_MPU;                // offset 0x00E4 
    REG32 PM_PREPWSTST_MPU;             // offset 0x00E8 
    
} OMAP_PRCM_MPU_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_CAM;                 // offset 0x0058 
    REG32 zzzReserved02[27];

    REG32 PM_WKDEP_CAM;                 // offset 0x00C8 
    REG32 zzzReserved03[5];

    REG32 PM_PWSTCTRL_CAM;              // offset 0x00E0 
    REG32 PM_PWSTST_CAM;                // offset 0x00E4 
    REG32 PM_PREPWSTST_CAM;             // offset 0x00E8 

} OMAP_PRCM_CAM_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_SGX;                 // offset 0x0058 
    REG32 zzzReserved02[27];

    REG32 PM_WKDEP_SGX;                 // offset 0x00C8 
    REG32 zzzReserved03[5];

    REG32 PM_PWSTCTRL_SGX;              // offset 0x00E0 
    REG32 PM_PWSTST_SGX;                // offset 0x00E4 
    REG32 PM_PREPWSTST_SGX;             // offset 0x00E8 

} OMAP_PRCM_SGX_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_NEON;                // offset 0x0058 
    REG32 zzzReserved02[27];

    REG32 PM_WKDEP_NEON;                // offset 0x00C8 
    REG32 zzzReserved03[5];

    REG32 PM_PWSTCTRL_NEON;             // offset 0x00E0 
    REG32 PM_PWSTST_NEON;               // offset 0x00E4 
    REG32 PM_PREPWSTST_NEON;            // offset 0x00E8 

} OMAP_PRCM_NEON_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_USBHOST;             // offset 0x0058 
    REG32 zzzReserved02[17];

    REG32 PM_WKEN_USBHOST;              // offset 0x00A0 
    REG32 PM_MPUGRPSEL_USBHOST;         // offset 0x00A4 
    REG32 PM_IVA2GRPSEL_USBHOST;        // offset 0x00A8 
    REG32 zzzReserved03[1];

    REG32 PM_WKST_USBHOST;              // offset 0x00B0 
    REG32 zzzReserved04[5];

    REG32 PM_WKDEP_USBHOST;             // offset 0x00C8 
    REG32 zzzReserved05[5];

    REG32 PM_PWSTCTRL_USBHOST;          // offset 0x00E0 
    REG32 PM_PWSTST_USBHOST;            // offset 0x00E4 
    REG32 PM_PREPWSTST_USBHOST;         // offset 0x00E8 

} OMAP_PRCM_USBHOST_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[40];

    REG32 PM_WKEN_WKUP;                 // offset 0x00A0 
    REG32 PM_MPUGRPSEL_WKUP;            // offset 0x00A4 
    REG32 PM_IVA2GRPSEL_WKUP;           // offset 0x00A8 
    REG32 zzzReserved02[1];

    REG32 PM_WKST_WKUP;                 // offset 0x00B0 

} OMAP_PRCM_WKUP_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[22];

    REG32 RM_RSTST_EMU;                 // offset 0x0058 
    REG32 zzzReserved02[34];

    REG32 PM_PWSTST_EMU;                // offset 0x00E4 

} OMAP_PRCM_EMU_PRM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[16];

    REG32 PRM_CLKSEL;                   // offset 0x0040 
    REG32 zzzReserved02[11];

    REG32 PRM_CLKOUT_CTRL;              // offset 0x0070 

} OMAP_PRCM_CLOCK_CONTROL_PRM_REGS;


//------------------------------------------------------------------------------

typedef struct {

    union {
        REG32 CM_FCLKEN_xxx;            // offset 0x0000
        REG32 CM_FCLKEN1_xxx;           // offset 0x0000
    };
    
    REG32 CM_CLKEN_PLL_xxx;             // offset 0x0004
    REG32 CM_FCLKEN3_xxx;               // offset 0x0008
    REG32 zzzReserved01[1];             // offset 0x000C
 
    union {
        REG32 CM_ICLKEN_xxx;            // offset 0x0010
        REG32 CM_ICLKEN1_xxx;           // offset 0x0010
    };
    
    REG32 CM_ICLKEN2_xxx;               // offset 0x0014
    REG32 CM_ICLKEN3_xxx;               // offset 0x0018
    REG32 zzzReserved02[1];             // offset 0x001C

    union {
        REG32 CM_IDLEST_xxx;            // offset 0x0020 
        REG32 CM_IDLEST1_xxx;           // offset 0x0020 
    };

    union {
        REG32 CM_IDLEST_PLL_xxx;        // offset 0x0024
        REG32 CM_IDLEST2_xxx;           // offset 0x0024 
    };
    
    REG32 CM_IDLEST3_xxx;               // offset 0x0028
    REG32 zzzReserved03[1];             // offset 0x002C

    union {
        REG32 CM_AUTOIDLE_xxx;          // offset 0x0030 
        REG32 CM_AUTOIDLE1_xxx;         // offset 0x0030 
    };

    REG32 CM_AUTOIDLE2_xxx;             // offset 0x0034
    REG32 CM_AUTOIDLE3_xxx;             // offset 0x0038
    REG32 zzzReserved04[1];             // offset 0x003C

    REG32 CM_CLKSEL_xxx;                // offset 0x0040
    union {
        REG32 CM_SLEEPDEP_xxx;          // offset 0x0044 
        REG32 CM_CLKSEL1_xxx;
    };
    REG32 CM_CLKSTCTRL_xxx;             // offset 0x0048 
    REG32 CM_CLKSTST_xxx;               // offset 0x004C 
    REG32 CM_CLKSEL2_xxx;               // offset 0x0050 
    REG32 CM_CLKSEL3_xxx;               // offset 0x0054

} OMAP_CM_REGS;

//------------------------------------------------------------------------------

typedef struct {

    REG32 zzzReserved01[20];

    REG32 RM_RSTCTRL_xxx;               // offset 0x0050 
    REG32 zzzReserved02[1];

    REG32 RM_RSTST_xxx;                 // offset 0x0058 
    REG32 zzzReserved03[17];            // offset 0x005c

    union {
        REG32 PM_WKEN_xxx;              // offset 0x00A0 
        REG32 PM_WKEN1_xxx;             // offset 0x00A0 
    };
    
    REG32 PM_MPUGRPSEL1_xxx;            // offset 0x00A4 
    REG32 PM_IVA2GRPSEL1_xxx;           // offset 0x00A8 
    REG32 zzzReserved04[1];

    REG32 PM_WKST_xxx;                  // offset 0x00B0 
    REG32 zzzReserved05[1];             // offset 0x00B4
    REG32 PM_WKST3_xxx;                 // offset 0x00B8
    REG32 zzzReserved06[3];             // offset 0X00BC

    REG32 PM_WKDEP_xxx;                 // offset 0x00C8 
    REG32 zzzReserved07[2];             // offset 0x00cc

    REG32 PM_EVGENCTRL_xx;              // offset 0x00D4 
    REG32 PM_EVGENONTIM_xxx;            // offset 0x00D8 
    REG32 PM_EVGENOFFTIM_xxx;           // offset 0x00DC
 
    REG32 PM_PWSTCTRL_xxx;              // offset 0x00E0 
    REG32 PM_PWSTST_xxx;                // offset 0x00E4 
    REG32 PM_PREPWSTST_xxx;             // offset 0x00E8 

    REG32 zzzReserved08[1];             // offset 0x00EC
    REG32 PM_WKEN3_xxx;                 // offset 0x00F0
    REG32 PM_IVA2GRPSEL3_xxx;           // offset 0x00F4
    REG32 PM_MPUGRPSEL3_xxx;            // offset 0x00F8

} OMAP_PRM_REGS;

//------------------------------------------------------------------------------
typedef struct {    
    OMAP_PRCM_WKUP_PRM_REGS             *pOMAP_WKUP_PRM;
    OMAP_PRCM_CORE_PRM_REGS             *pOMAP_CORE_PRM;
    OMAP_PRCM_PER_PRM_REGS              *pOMAP_PER_PRM;
    OMAP_PRCM_USBHOST_PRM_REGS          *pOMAP_USBHOST_PRM;    
    OMAP_PRCM_EMU_PRM_REGS              *pOMAP_EMU_PRM;
    OMAP_PRCM_MPU_PRM_REGS              *pOMAP_MPU_PRM;
    OMAP_PRCM_DSS_PRM_REGS              *pOMAP_DSS_PRM;  
    OMAP_PRCM_NEON_PRM_REGS             *pOMAP_NEON_PRM;
    OMAP_PRCM_IVA2_PRM_REGS             *pOMAP_IVA2_PRM;
    OMAP_PRCM_CAM_PRM_REGS              *pOMAP_CAM_PRM; 
    OMAP_PRCM_SGX_PRM_REGS              *pOMAP_SGX_PRM;
    OMAP_PRCM_GLOBAL_PRM_REGS           *pOMAP_GLOBAL_PRM;
    OMAP_PRCM_OCP_SYSTEM_PRM_REGS       *pOMAP_OCP_SYSTEM_PRM;
    OMAP_PRCM_CLOCK_CONTROL_PRM_REGS    *pOMAP_CLOCK_CONTROL_PRM;
} OMAP_PRCM_PRM;

//------------------------------------------------------------------------------
typedef struct {    
    OMAP_PRCM_WKUP_CM_REGS              *pOMAP_WKUP_CM;
    OMAP_PRCM_CORE_CM_REGS              *pOMAP_CORE_CM;    
    OMAP_PRCM_PER_CM_REGS               *pOMAP_PER_CM;
    OMAP_PRCM_USBHOST_CM_REGS           *pOMAP_USBHOST_CM;
    OMAP_PRCM_EMU_CM_REGS               *pOMAP_EMU_CM;
    OMAP_PRCM_MPU_CM_REGS               *pOMAP_MPU_CM;
    OMAP_PRCM_DSS_CM_REGS               *pOMAP_DSS_CM;
    OMAP_PRCM_NEON_CM_REGS              *pOMAP_NEON_CM;
    OMAP_PRCM_IVA2_CM_REGS              *pOMAP_IVA2_CM;
    OMAP_PRCM_CAM_CM_REGS               *pOMAP_CAM_CM; 
    OMAP_PRCM_SGX_CM_REGS               *pOMAP_SGX_CM;     
    OMAP_PRCM_GLOBAL_CM_REGS            *pOMAP_GLOBAL_CM;
    OMAP_PRCM_OCP_SYSTEM_CM_REGS        *pOMAP_OCP_SYSTEM_CM;
    OMAP_PRCM_CLOCK_CONTROL_CM_REGS     *pOMAP_CLOCK_CONTROL_CM;
} OMAP_PRCM_CM;

//-----------------------------------------------------------------------------
typedef struct {
    REG32 BOOT_CONFIG_ADDR;              // offset 0x0000
    REG32 PUBLIC_RESTORE_ADDR;           // offset 0x0004
    REG32 SECURE_SRAM_RESTORE_ADDR;      // offset 0x0008
    REG32 SDRC_MODULE_SEMAPHORE;         // offset 0x000C
    REG32 PRCM_BLOCK_OFFSET;             // offset 0x0010
    REG32 SDRC_BLOCK_OFFSET;             // offset 0x0014
    REG32 zzzReserved01;                 // offset 0x0018
    REG32 OEM_CPU_INFO_DATA_PA;          // offset 0x001C    // priv
    REG32 OEM_CPU_INFO_DATA_VA;          // offset 0x0020    // priv
} OMAP_CONTEXT_RESTORE_REGS;

//-----------------------------------------------------------------------------
typedef struct {
    REG32 PRM_CLKSRC_CTRL;               // offset 0x0000
    REG32 PRM_CLKSEL;                    // offset 0x0004
    REG32 CM_CLKSEL_CORE;                // offset 0x0008
    REG32 CM_CLKSEL_WKUP;                // offset 0x000c
    REG32 CM_CLKEN_PLL;                  // offset 0x0010
    REG32 CM_AUTOIDLE_PLL;               // offset 0x0014
    REG32 CM_CLKSEL1_PLL;                // offset 0x0018
    REG32 CM_CLKSEL2_PLL;                // offset 0x001c
    REG32 CM_CLKSEL3_PLL;                // offset 0x0020
    REG32 CM_CLKEN_PLL_MPU;              // offset 0x0024
    REG32 CM_AUTOIDLE_PLL_MPU;           // offset 0x0028
    REG32 CM_CLKSEL1_PLL_MPU;            // offset 0x002c
    REG32 CM_CLKSEL2_PLL_MPU;            // offset 0x0030
    REG32 zzzReserved01;                 // offset 0x0034
    REG32 CM_CLKSTCTRL_MPU;              // offset 0x0038    // priv
    REG32 CM_CLKSTCTRL_CORE;             // offset 0x003C    // priv
} OMAP_PRCM_RESTORE_REGS;

//-----------------------------------------------------------------------------
typedef struct {
    unsigned short SYSCONFIG;                   // offset 0x0000
    unsigned short CS_CFG;                      // offset 0x0002
    unsigned short SHARING;                     // offset 0x0004
    unsigned short ERR_TYPE;                    // offset 0x0006
    REG32   DLLA_CTRL;                   // offset 0x0008
    REG32   DLLB_CTRL;                   // offset 0x000C
    REG32   POWER;                       // offset 0x0010
    REG32   zzzReserved01;               // offset 0x0014
    REG32   MCFG_0;                      // offset 0x0018
    unsigned short MR_0;                        // offset 0x001C
    unsigned short EMR1_0;                      // offset 0x001E
    unsigned short EMR2_0;                      // offset 0x0020
    unsigned short EMR3_0;                      // offset 0x0022
    REG32   ACTIM_CTRLA_0;               // offset 0x0024
    REG32   ACTIM_CTRLB_0;               // offset 0x0028
    REG32   RFR_CTRL_0;                  // offset 0x002C
    REG32   zzzReserved02;               // offset 0x0030
    REG32   MCFG_1;                      // offset 0x0034
    unsigned short MR_1;                        // offset 0x0038
    unsigned short EMR1_1;                      // offset 0x003A
    unsigned short EMR2_1;                      // offset 0x003C
    unsigned short EMR3_1;                      // offset 0x003E
    REG32   ACTIM_CTRLA_1;               // offset 0x0040
    REG32   ACTIM_CTRLB_1;               // offset 0x0044
    REG32   RFR_CTRL_1;                  // offset 0x0048
    unsigned short DCDL_1_CTRL;                 // offset 0x004C
    unsigned short DCDL_2_CTRL;                 // offset 0x004E
    REG32   zzzReserved03;               // offset 0x0050
    REG32   zzzReserved04;               // offset 0x0054
} OMAP_SDRC_RESTORE_REGS;

//-----------------------------------------------------------------------------
// Global reset control flags
//
#define RSTCTRL_RST_DPLL3                   (1 << 2)
#define RSTCTRL_RST_GS                      (1 << 1)


//-----------------------------------------------------------------------------
// CORE reset flags
//
#define EMULATION_SEQ_RST                   (1 << 13)
#define EMULATION_VHWA_RST                  (1 << 12)
#define EMULATION_MPU_RST                   (1 << 11)
#define EMULATION_IVA2_RST                  (1 << 11)
#define IVA2_SW_RST3                        (1 << 10)
#define IVA2_SW_RST2                        (1 << 9)
#define IVA2_SW_RST1                        (1 << 8)
#define EXTERNALWARM_RST                    (1 << 6)
#define COREDOMAINWKUP_RST                  (1 << 3)
#define DOMAINWKUP_RST                      (1 << 2)
#define GLOBALWARM_RST                      (1 << 1)
#define GLOBALCOLD_RST                      (1 << 0)

#define RST3_IVA2                           (1 << 2)
#define RST2_IVA2                           (1 << 1)
#define RST1_IVA2                           (1 << 0)

//-----------------------------------------------------------------------------
// GLOBAL reset flags
//
#define ICECRUSHER_RST                      (1 << 10)
#define ICEPICK_RST                         (1 << 9)
#define VDD2_VOLTAGE_MANAGER_RST            (1 << 8)
#define VDD1_VOLTAGE_MANAGER_RST            (1 << 7)
#define EXTERNAL_WARM_RST                   (1 << 6)
#define SECRURE_WD_RST                      (1 << 5)
#define MPU_WD_RST                          (1 << 4)
#define SECURITY_VIOL_RST                   (1 << 3)
#define GLOBAL_SW_RST                       (1 << 1)
#define GLOBAL_COLD_RST                     (1 << 0)

//-----------------------------------------------------------------------------
// sysconfig flags
//
#define SYSCONFIG_AUTOIDLE                  (1 << 0)

#define SYSCONFIG_SOFTRESET                 (1 << 1)
#define SYSSTATUS_RESETDONE                 (1 << 0)

#define SYSCONFIG_ENAWAKEUP                 (1 << 2)

#define SYSCONFIG_FORCEIDLE                 (0 << 3)
#define SYSCONFIG_NOIDLE                    (1 << 3)
#define SYSCONFIG_SMARTIDLE                 (2 << 3)
#define SYSCONFIG_IDLE_MASK                 (3 << 3)

#define SYSCONFIG_CLOCKACTIVITY_AUTOOFF     (0 << 8)
#define SYSCONFIG_CLOCKACTIVITY_F_ON        (1 << 8)
#define SYSCONFIG_CLOCKACTIVITY_I_ON        (2 << 8)
#define SYSCONFIG_CLOCKACTIVITY_IF_ON       (3 << 8)

#define SYSCONFIG_FORCESTANDBY              (0 << 12)
#define SYSCONFIG_NOSTANDBY                 (1 << 12)
#define SYSCONFIG_SMARTSTANDBY              (2 << 12)
#define SYSCONFIG_STANDBY_MASK              (3 << 12)

//-----------------------------------------------------------------------------
// dpll flags
//
#define DPLL_UPDATE_DPLLMODE                (0x00000001)
#define DPLL_UPDATE_LPMODE                  (0x00000002)
#define DPLL_UPDATE_DRIFTGUARD              (0x00000004)
#define DPLL_UPDATE_RAMPTIME                (0x00000008)
#define DPLL_UPDATE_AUTOIDLEMODE            (0x00000010)
#define DPLL_UPDATE_ALL                     (0x0000001F)

#define DOMAIN_UPDATE_WKUPDEP               (0x00000001)
#define DOMAIN_UPDATE_SLEEPDEP              (0x00000002)
#define DOMAIN_UPDATE_POWERSTATE            (0x00000004)
#define DOMAIN_UPDATE_CLOCKSTATE            (0x00000008)
#define DOMAIN_UPDATE_ALL                   (0x0000000F)

//-----------------------------------------------------------------------------
#define DPLL_PER_MODE_SHIFT                 (16)
#define DPLL_PER_STAT_SHIFT                 (1)
#define DPLL_PER_IDLE_SHIFT                 (3)
#define DPLL_CORE_CLKSEL_SHIFT              (8)

#define DPLL_MPU                            1
#define DPLL_IVA                            2
#define DPLL_CORE                           3
#define DPLL_PER                            4
#define DPLL_PER2                           5

#define DPLL_MODE_SHIFT                     (0)
#define DPLL_MODE_MASK                      (0x7 << DPLL_MODE_SHIFT)
#define DPLL_MODE_LOWPOWER_STOP             (0x1 << DPLL_MODE_SHIFT)
#define DPLL_MODE_LOWPOWER_BYPASS           (0x5 << DPLL_MODE_SHIFT)
#define DPLL_MODE_FASTRELOCK                (0x6 << DPLL_MODE_SHIFT)
#define DPLL_MODE_LOCK                      (0x7 << DPLL_MODE_SHIFT)

#define DPLL_STATUS_MASK                    (0x1)
#define DPLL_STATUS_BYPASSED                (0x0)
#define DPLL_STATUS_LOCKED                  (0x1)

#define DPLL_CLK_SRC_SHIFT                  (19)
#define DPLL_CLK_SRC_MASK                   (0x1F << DPLL_CLK_SRC_SHIFT)
#define DPLL_CLK_SRC(x)                     ((0x1F & x) << DPLL_CLK_SRC_SHIFT)

#define DPLL_FREQSEL_SHIFT                  (4)
#define DPLL_FREQSEL_MASK                   (0xF << DPLL_FREQSEL_SHIFT)
#define DPLL_FREQSEL(x)                     ((0xF & (x)) << DPLL_FREQSEL_SHIFT)

#define DPLL_MULT_SHIFT                     (8)
#define DPLL_MULT_MASK                      (0x7FF << DPLL_MULT_SHIFT)
#define DPLL_MULT(x)                        ((0x7FF & (x)) << DPLL_MULT_SHIFT)

#define DPLL_DIV_SHIFT                      (0)
#define DPLL_DIV_MASK                       (0x7F << DPLL_DIV_SHIFT)
#define DPLL_DIV(x)                         ((0x7F & (x)) << DPLL_DIV_SHIFT)

#define EN_DPLL_LPMODE_SHIFT                (10)
#define EN_DPLL_LPMODE_MASK                 (1 << EN_DPLL_LPMODE_SHIFT)
#define EN_DPLL_LPMODE                      (1 << EN_DPLL_LPMODE_SHIFT)

#define DPLL_RAMPTIME_SHIFT                 (8)
#define DPLL_RAMPTIME_MASK                  (3 << DPLL_RAMPTIME_SHIFT)
#define DPLL_RAMPTIME_DISABLE               (0 << DPLL_RAMPTIME_SHIFT)
#define DPLL_RAMPTIME_4                     (1 << DPLL_RAMPTIME_SHIFT)
#define DPLL_RAMPTIME_20                    (2 << DPLL_RAMPTIME_SHIFT)
#define DPLL_RAMPTIME_40                    (3 << DPLL_RAMPTIME_SHIFT)

#define EN_DPLL_DRIFTGUARD_SHIFT            (3)
#define EN_DPLL_DRIFTGUARD_MASK             (1 << EN_DPLL_DRIFTGUARD_SHIFT)
#define EN_DPLL_DRIFTGUARD                  (1 << EN_DPLL_DRIFTGUARD_SHIFT)

#define DPLL_AUTOIDLE_SHIFT                 (0)
#define DPLL_AUTOIDLE_MASK                  (0x7 << DPLL_AUTOIDLE_SHIFT)
#define DPLL_AUTOIDLE_DISABLED              (0 << DPLL_AUTOIDLE_SHIFT)
#define DPLL_AUTOIDLE_LOWPOWERSTOPMODE      (1 << DPLL_AUTOIDLE_SHIFT)
#define DPLL_AUTOIDLE_LOWPOWERBYPASS        (5 << DPLL_AUTOIDLE_SHIFT)
#define DPLL_AUTOIDLE_HIGHPOWERBYPASS       (6 << DPLL_AUTOIDLE_SHIFT)

#define DPLL_MPU_CLKOUT_DIV_SHIFT           (0)
#define DPLL_IVA2_CLKOUT_DIV_SHIFT          (0)
#define DPLL_CORE_CLKOUT_DIV_SHIFT          (27)
#define DPLL_CORE_CLKOUT_DIV_MASK           (0x1F << DPLL_CORE_CLKOUT_DIV_SHIFT)

//-----------------------------------------------------------------------------
// clock state transition control
#define CLKSTCTRL_CORE_D2D_SHIFT            (4)
#define CLKSTCTRL_CORE_L4_SHIFT             (2)
#define CLKSTCTRL_SHIFT                     (0)
#define CLKSTCTRL_MASK                      (3 << CLKSTCTRL_SHIFT)
#define CLKSTCTRL_DISABLED                  (0 << CLKSTCTRL_SHIFT)
#define CLKSTCTRL_SLEEP                     (1 << CLKSTCTRL_SHIFT)
#define CLKSTCTRL_WAKEUP                    (2 << CLKSTCTRL_SHIFT)
#define CLKSTCTRL_AUTOMATIC                 (3 << CLKSTCTRL_SHIFT)

#define POWERSTATE_SHIFT                    (0)
#define POWERSTATE_MASK                     (3 << POWERSTATE_SHIFT)
#define POWERSTATE_OFF                      (0 << POWERSTATE_SHIFT)
#define POWERSTATE_RETENTION                (1 << POWERSTATE_SHIFT)
#define POWERSTATE_INACTIVE                 (2 << POWERSTATE_SHIFT)
#define POWERSTATE_ON                       (3 << POWERSTATE_SHIFT)

#define WKDEP_SHIFT                         (0)
#define WKDEP_MASK                          (0xFF << WKDEP_SHIFT)
#define WKDEP_EN_PER                        (1 << 7)
#define WKDEP_EN_DSS                        (1 << 5)
#define WKDEP_EN_WKUP                       (1 << 4)
#define WKDEP_EN_IVA2                       (1 << 2)
#define WKDEP_EN_MPU                        (1 << 1)
#define WKDEP_EN_CORE                       (1 << 0)

#define SLEEPDEP_SHIFT                      (0)
#define SLEEPDEP_MASK                       (0xFF << SLEEPDEP_SHIFT)
#define SLEEPDEP_EN_PER                     (1 << 7)
#define SLEEPDEP_EN_DSS                     (1 << 5)
#define SLEEPDEP_EN_WKUP                    (1 << 4)
#define SLEEPDEP_EN_IVA2                    (1 << 2)
#define SLEEPDEP_EN_MPU                     (1 << 1)
#define SLEEPDEP_EN_CORE                    (1 << 0)

//-----------------------------------------------------------------------------
// CORE Devices
//
#define CM_CLKEN_MMC3                       (1 << 30)
#define CM_CLKEN_ICR                        (1 << 29)
#define CM_CLKEN_AES2                       (1 << 28)
#define CM_CLKEN_SHA12                      (1 << 27)
#define CM_CLKEN_DES2                       (1 << 26)
#define CM_CLKEN_MMC2                       (1 << 25)
#define CM_CLKEN_MMC1                       (1 << 24)
#define CM_CLKEN_MSPRO                      (1 << 23)
#define CM_CLKEN_HDQ                        (1 << 22)
#define CM_CLKEN_MCSPI4                     (1 << 21)
#define CM_CLKEN_MCSPI3                     (1 << 20)
#define CM_CLKEN_MCSPI2                     (1 << 19)
#define CM_CLKEN_MCSPI1                     (1 << 18)
#define CM_CLKEN_I2C3                       (1 << 17)
#define CM_CLKEN_I2C2                       (1 << 16)
#define CM_CLKEN_I2C1                       (1 << 15)
#define CM_CLKEN_UART2                      (1 << 14)
#define CM_CLKEN_UART1                      (1 << 13)
#define CM_CLKEN_GPT11                      (1 << 12)
#define CM_CLKEN_GPT10                      (1 << 11)
#define CM_CLKEN_MCBSP5                     (1 << 10)
#define CM_CLKEN_MCBSP1                     (1 << 9)
#define CM_CLKEN_FAC                        (1 << 8)
#define CM_CLKEN_MAILBOXES                  (1 << 7)
#define CM_CLKEN_OMAPCTRL                   (1 << 6)
#define CM_CLKEN_FSHOSTUSB                  (1 << 5)
#define CM_CLKEN_HSOTGUSB                   (1 << 4)
#define CM_CLKEN_D2D                        (1 << 3)
#define CM_CLKEN_SDRC                       (1 << 1)
#define CM_CLKEN_SSI                        (1 << 0)

#define CM_CLKEN_PKA                        (1 << 4)
#define CM_CLKEN_AES1                       (1 << 3)
#define CM_CLKEN_RNG                        (1 << 2)
#define CM_CLKEN_SHA11                      (1 << 1)
#define CM_CLKEN_DES1                       (1 << 0)

#define CM_V_CLKEN_VRFB                     (1 << 16)   // virtual bit
#define CM_CLKEN_USBTLL                     (1 << 2)
#define CM_CLKEN_TS                         (1 << 1)
#define CM_CLKEN_EFUSE                      (1 << 0)
#define CM_V_CLKEN_IVA2                     (1 << 16)   // virtual bit

#define CM_IDLEST_ST_MMC3                   (1 << 30)   // ES2.0
#define CM_IDLEST_ST_ICR                    (1 << 29)
#define CM_IDLEST_ST_AES2                   (1 << 28)
#define CM_IDLEST_ST_SHA12                  (1 << 27)
#define CM_IDLEST_ST_DES2                   (1 << 26)
#define CM_IDLEST_ST_MMC2                   (1 << 25)
#define CM_IDLEST_ST_MMC1                   (1 << 24)
#define CM_IDLEST_ST_MSPRO                  (1 << 23)
#define CM_IDLEST_ST_HDQ                    (1 << 22)
#define CM_IDLEST_ST_MCSPI4                 (1 << 21)
#define CM_IDLEST_ST_MCSPI3                 (1 << 20)
#define CM_IDLEST_ST_MCSPI2                 (1 << 19)
#define CM_IDLEST_ST_MCSPI1                 (1 << 18)
#define CM_IDLEST_ST_I2C3                   (1 << 17)
#define CM_IDLEST_ST_I2C2                   (1 << 16)
#define CM_IDLEST_ST_I2C1                   (1 << 15)
#define CM_IDLEST_ST_UART2                  (1 << 14)
#define CM_IDLEST_ST_UART1                  (1 << 13)
#define CM_IDLEST_ST_GPT11                  (1 << 12)
#define CM_IDLEST_ST_GPT10                  (1 << 11)
#define CM_IDLEST_ST_MCBSP5                 (1 << 10)
#define CM_IDLEST_ST_MCBSP1                 (1 << 9)
#define CM_IDLEST_ST_FAC                    (1 << 8)    // ES1.0
#define CM_IDLEST_ST_SSI_IDLE               (1 << 8)    // ES2.0
#define CM_IDLEST_ST_MAILBOXES              (1 << 7)
#define CM_IDLEST_ST_OMAPCTRL               (1 << 6)
#define CM_IDLEST_ST_FSHOSTUSB              (1 << 5)    // ES1.0
#define CM_IDLEST_ST_HSOTGUSB_IDLE          (1 << 5)    // ES2.0
#define CM_IDLEST_ST_HSOTGUSB               (1 << 4)    // ES1.0
#define CM_IDLEST_ST_HSOTGUSB_STDBY         (1 << 4)    // ES2.0
#define CM_IDLEST_ST_D2D                    (1 << 3)    // ES1.0 
#define CM_IDLEST_ST_SDMA                   (1 << 2)
#define CM_IDLEST_ST_SDRC                   (1 << 1)
#define CM_IDLEST_ST_SSI                    (1 << 0)    // ES1.0
#define CM_IDLEST_ST_SSI_STANDBY            (1 << 0)    // ES2.0

#define CM_IDLEST_ST_PKA                    (1 << 4)
#define CM_IDLEST_ST_AES1                   (1 << 3)
#define CM_IDLEST_ST_RNG                    (1 << 2)
#define CM_IDLEST_ST_SHA11                  (1 << 1)
#define CM_IDLEST_ST_DES1                   (1 << 0)

#define CM_IDLEST_ST_USBTLL                 (1 << 2)
#define CM_IDLEST_ST_EFUSE                  (1 << 0)

#define CLKSEL_L3_SHIFT						(0)
#define CLKSEL_L3_MASK						(0x3 << CLKSEL_L3_SHIFT)
#define CLKSEL_L4_SHIFT						(2)
#define CLKSEL_L4_MASK						(0x3 << CLKSEL_L4_SHIFT)

#define CLKSEL_GPT10                        (1 << 6)
#define CLKSEL_GPT11                        (1 << 7)

#define CLKSEL_SSI_MASK                     (0xF << 8)
#define CLKSEL_SSI(x)                       (((0xF) & x) << 8)

#define CLKSEL_96M_SHIFT                    (12)
#define CLKSEL_96M_MASK                     (0x3 << CLKSEL_96M_SHIFT)
#define CLKSEL_96M(x)                       ((x << CLKSEL_96M_SHIFT) & CLKSEL_96M_MASK)

//-----------------------------------------------------------------------------
// PER Devices
//
#define CM_CLKEN_UART4                      (1 << 18) /* 37xx only */
#define CM_CLKEN_GPIO6                      (1 << 17)
#define CM_CLKEN_GPIO5                      (1 << 16)
#define CM_CLKEN_GPIO4                      (1 << 15)
#define CM_CLKEN_GPIO3                      (1 << 14)
#define CM_CLKEN_GPIO2                      (1 << 13)
#define CM_CLKEN_WDT3                       (1 << 12)
#define CM_CLKEN_UART3                      (1 << 11)
#define CM_CLKEN_GPT9                       (1 << 10)
#define CM_CLKEN_GPT8                       (1 << 9)
#define CM_CLKEN_GPT7                       (1 << 8)
#define CM_CLKEN_GPT6                       (1 << 7)
#define CM_CLKEN_GPT5                       (1 << 6)
#define CM_CLKEN_GPT4                       (1 << 5)
#define CM_CLKEN_GPT3                       (1 << 4)
#define CM_CLKEN_GPT2                       (1 << 3)
#define CM_CLKEN_MCBSP4                     (1 << 2)
#define CM_CLKEN_MCBSP3                     (1 << 1)
#define CM_CLKEN_MCBSP2                     (1 << 0)

#define CM_IDLEST_ST_UART4                  (1 << 18) /* 37xx only */
#define CM_IDLEST_ST_GPIO6                  (1 << 17)
#define CM_IDLEST_ST_GPIO5                  (1 << 16)
#define CM_IDLEST_ST_GPIO4                  (1 << 15)
#define CM_IDLEST_ST_GPIO3                  (1 << 14)
#define CM_IDLEST_ST_GPIO2                  (1 << 13)
#define CM_IDLEST_ST_WDT3                   (1 << 12)
#define CM_IDLEST_ST_UART3                  (1 << 11)
#define CM_IDLEST_ST_GPT9                   (1 << 10)
#define CM_IDLEST_ST_GPT8                   (1 << 9)
#define CM_IDLEST_ST_GPT7                   (1 << 8)
#define CM_IDLEST_ST_GPT6                   (1 << 7)
#define CM_IDLEST_ST_GPT5                   (1 << 6)
#define CM_IDLEST_ST_GPT4                   (1 << 5)
#define CM_IDLEST_ST_GPT3                   (1 << 4)
#define CM_IDLEST_ST_GPT2                   (1 << 3)
#define CM_IDLEST_ST_MCBSP4                 (1 << 2)
#define CM_IDLEST_ST_MCBSP3                 (1 << 1)
#define CM_IDLEST_ST_MCBSP2                 (1 << 0)

#define CLKSEL_GPT2                         (1 << 0)
#define CLKSEL_GPT3                         (1 << 1)
#define CLKSEL_GPT4                         (1 << 2)
#define CLKSEL_GPT5                         (1 << 3)
#define CLKSEL_GPT6                         (1 << 4)
#define CLKSEL_GPT7                         (1 << 5)
#define CLKSEL_GPT8                         (1 << 6)
#define CLKSEL_GPT9                         (1 << 7)

//-----------------------------------------------------------------------------
// Wakeup Devices
//
#define CM_CLKEN_IO_CHAIN                   (1 << 16)
#define CM_CLKEN_USIM                       (1 << 9)
#define CM_CLKEN_IO                         (1 << 8)
#define CM_CLKEN_SR2                        (1 << 7)
#define CM_CLKEN_SR1                        (1 << 6)
#define CM_CLKEN_WDT2                       (1 << 5)
#define CM_CLKEN_WDT1                       (1 << 4)
#define CM_CLKEN_GPIO1                      (1 << 3)
#define CM_CLKEN_32KSYNC                    (1 << 2)
#define CM_CLKEN_GPT12                      (1 << 1)
#define CM_CLKEN_GPT1                       (1 << 0)

#define CM_IDLEST_ST_USIM                   (1 << 9)
#define CM_IDLEST_ST_SR2                    (1 << 7)
#define CM_IDLEST_ST_SR1                    (1 << 6)
#define CM_IDLEST_ST_WDT2                   (1 << 5)
#define CM_IDLEST_ST_WDT1                   (1 << 4)
#define CM_IDLEST_ST_GPIO1                  (1 << 3)
#define CM_IDLEST_ST_32KSYNC                (1 << 2)
#define CM_IDLEST_ST_GPT12                  (1 << 1)
#define CM_IDLEST_ST_GPT1                   (1 << 0)

#define CM_ST_IO_CHAIN                      (1 << 16)

#define CLKSEL_GPT1                         (1 << 0)

#define CLKSEL_USIMOCP_MASK                 (0xF << 3)
#define CLKSEL_USIMOCP(x)                   (((0xF)&x) << 3)

//-----------------------------------------------------------------------------
// MPU Devices
//
#define MPU_DPLL_CLKOUT_DIV_MASK            (0x1F)
#define MPU_DPLL_CLKOUT_DIV(x)              (MPU_DPLL_CLKOUT_DIV_MASK & x)

//-----------------------------------------------------------------------------
// IVA2 Devices
//
#define IVA2_DPLL_CLKOUT_DIV_MASK           (0x1F)
#define IVA2_DPLL_CLKOUT_DIV(x)             (IVA2_DPLL_CLKOUT_DIV_MASK & x)

//-----------------------------------------------------------------------------
// GFX Devices
//
#define CM_CLKEN_2D                         (1 << 1)
#define CM_CLKEN_3D                         (1 << 2)
#define CM_CLKEN_FSGX                       (1 << 1)
#define CM_CLKEN_ISGX                       (1 << 0)

#define CM_IDLEST_ST_SGX                    (1 << 0)

#define CLKSEL_SGX_MASK                     (0x7)
#define CLKSEL_SGX(x)                       ((0x7) & x)

//-----------------------------------------------------------------------------
// IVA Devices
//
#define CM_CLKEN_IVA2                       (1 << 0)

#define CM_IDLEST_ST_IVA2                   (1 << 0)

//-----------------------------------------------------------------------------
// DSS Devices
//
#define CM_CLKEN_DSS_MASK                   (0x3)
#define CM_CLKEN_DSS1                       (1 << 0)
#define CM_CLKEN_DSS2                       (1 << 1)
#define CM_CLKEN_TV                         (1 << 2)
#define CM_CLKEN_DSS                        (1 << 0)

#define CM_IDLEST_ST_DSS                    (1 << 0)    // ES1.0
#define CM_IDLEST_ST_DSS_IDLE               (1 << 1)    // ES2.0
#define CM_IDLEST_ST_DSS_STDBY              (1 << 0)    // ES2.0

#define CLKSEL_TV_MASK                      (0x1F << 8)
#define CLKSEL_TV(x)                        (((0x1F) & x) << 8)

#define CLKSEL_DSS1_MASK                    (0x1F)
#define CLKSEL_DSS1(x)                      ((0x1F) & x)

//-----------------------------------------------------------------------------
// CAM Devices
//
#define CM_CLKEN_CSI2                       (1 << 1)
#define CM_CLKEN_CAM                        (1 << 0)

#define CM_IDLEST_ST_CAM                    (1 << 0)

#define CLKSEL_CAM_MASK                     (0x1F)
#define CLKSEL_CAM(x)                       ((0x1F) & x)

//-----------------------------------------------------------------------------
// USBHOST Devices
//
#define CM_CLKEN_HSUSB_MASK                 (0x3)
#define CM_CLKEN_HSUSB2                     (1 << 1)
#define CM_CLKEN_HSUSB1                     (1 << 0)

#define CM_CLKEN_USBHOST                    (1 << 0)

#define CM_IDLEST_ST_USBHOST_STDBY          (1 << 0)
#define CM_IDLEST_ST_USBHOST_IDLE           (1 << 1)

//-----------------------------------------------------------------------------
// Clock Control Registers

#define SOURCE_48M                          (1 << 3)
#define SOURCE_54M                          (1 << 5)
#define SOURCE_96M                          (1 << 6)

#define CORE_DPLL_DIV_SHIFT					(8)
#define CORE_DPLL_DIV_MASK					(0x7F  <<  CORE_DPLL_DIV_SHIFT)
#define CORE_DPLL_MULT_SHIFT				(16)
#define CORE_DPLL_MULT_MASK					(0x7FF << CORE_DPLL_MULT_SHIFT)

#define DIV_96M_SHIFT						(0)
#define DIV_96M_MASK						(0x1F << DIV_96M_SHIFT)
#define DIV_120M_SHIFT						(0)
#define DIV_120M_MASK						(0x1F << DIV_120M_SHIFT)

#define SYS_CKIN_SEL_MASK					(0x7)

//-----------------------------------------------------------------------------
// defines the communication path to external IC about various power states
// 

// DM3730 only
#define PAD_OFF_MODE_OVR_MASK               (1 << 8)

#define SEL_VMODE_I2C                       (0 << 4)
#define SEL_VMODE_SIGNALLINE                (1 << 4)
#define SEL_VMODE                           (1 << 4)

#define SEL_OFF_I2C                         (0 << 3)
#define SEL_OFF_SIGNALLINE                  (1 << 3)
#define SEL_OFF                             (1 << 3)

#define AUTO_OFF_DISABLED                   (0 << 2)
#define AUTO_OFF_ENABLED                    (1 << 2)
#define AUTO_OFF                            (1 << 2)

#define AUTO_RET_DISABLED                   (0 << 1)
#define AUTO_RET_ENABLED                    (1 << 1)
#define AUTO_RET                            (1 << 1)

#define AUTO_SLEEP_DISABLED                 (0 << 0)
#define AUTO_SLEEP_ENABLED                  (1 << 0)
#define AUTO_SLEEP                          (1 << 0)

//-----------------------------------------------------------------------------
// define when to disable ext. clock req
// 
#define AUTOEXTCLKMODE_MASK                 (3 << 3)
#define AUTOEXTCLKMODE_ALWAYSACTIVE         (0 << 3)
#define AUTOEXTCLKMODE_INSLEEP              (1 << 3)
#define AUTOEXTCLKMODE_INRETENTION          (2 << 3)
#define AUTOEXTCLKMODE_INOFF                (3 << 3)

//-----------------------------------------------------------------------------
// define sysclk setup time mask
// 
#define SETUPTIME_MASK                      (0xFFFF << 0)

//-----------------------------------------------------------------------------
// global Control Registers
#define PRM_POLCTRL_MASK                    (0xF)

// defines sys_offmode signal
#define OFFMODE_POL_ACTIVELOW               (0 << 3)
#define OFFMODE_POL_ACTIVEHIGH              (1 << 3)
#define OFFMODE_POL                         (1 << 3)

// defines sys_clkout signal
#define CLKOUT_POL_INACTIVELOW              (0 << 2)
#define CLKOUT_POL_INACTIVEHIGH             (1 << 2)
#define CLKOUT_POL                          (1 << 2)

// defines sys_clkreq signal
#define CLKREQ_POL_ACTIVEHIGH               (1 << 1)
#define CLKREQ_POL_ACTIVELOW                (0 << 1)
#define CLKREQ_POL                          (1 << 1)

// defines sys_vmode signal
#define EXTVOL_POL_ACTIVEHIGH               (1 << 0)
#define EXTVOL_POL_ACTIVELOW                (0 << 0)
#define EXTVOL_POL                          (1 << 0)


#define L2FLATMEMONSTATE_SHIFT              (22)
#define L2FLATMEMONSTATE_MASK               (0x3 << L2FLATMEMONSTATE_SHIFT)
#define L2FLATMEMONSTATE_MEMORYON_DOMAINON  (0x3 << L2FLATMEMONSTATE_SHIFT)
#define L2FLATMEMONSTATE                    (L2FLATMEMONSTATE_MASK)

#define SHAREDL2CACHEFLATONSTATE_SHIFT      (20)
#define SHAREDL2CACHEFLATONSTATE_MASK       (0x3 << SHAREDL2CACHEFLATONSTATE_SHIFT)
#define SHAREDL2CACHEFLATONSTATE_MEMORYOFF_DOMAINON (0x0 << SHAREDL2CACHEFLATONSTATE_SHIFT)
#define SHAREDL2CACHEFLATONSTATE_MEMORYON_DOMAINON  (0x3 << SHAREDL2CACHEFLATONSTATE_SHIFT)
#define SHAREDL2CACHEFLATONSTATE            (SHAREDL2CACHEFLATONSTATE_MASK)

#define L1FLATMEMONSTATE_SHIFT              (18)
#define L1FLATMEMONSTATE_MASK               (0x3 << L1FLATMEMONSTATE_SHIFT)
#define L1FLATMEMONSTATE_MEMORYON_DOMAINON  (0x3 << L1FLATMEMONSTATE_SHIFT)
#define L1FLATMEMONSTATE                    (L1FLATMEMONSTATE_MASK)

#define MEM2ONSTATE_SHIFT                   (18)
#define MEM2ONSTATE_MASK                    (0x3 << MEM2ONSTATE_SHIFT)
#define MEM2ONSTATE_MEMORYOFF_DOMAINON      (0x0 << MEM2ONSTATE_SHIFT)
#define MEM2ONSTATE_MEMORYRET_DOMAINON      (0x1 << MEM2ONSTATE_SHIFT)
#define MEM2ONSTATE_MEMORYON_DOMAINON       (0x3 << MEM2ONSTATE_SHIFT)
#define MEM2ONSTATE                         (MEM2ONSTATE_MASK)

#define SHAREDL1CACHEFLATONSTATE_SHIFT      (16)
#define SHAREDL1CACHEFLATONSTATE_MASK       (0x3 << SHAREDL1CACHEFLATONSTATE_SHIFT)
#define SHAREDL1CACHEFLATONSTATE_MEMORYON_DOMAINON  (0x3 << SHAREDL1CACHEFLATONSTATE_SHIFT)
#define SHAREDL1CACHEFLATONSTATE            (SHAREDL1CACHEFLATONSTATE_MASK)

#define MEM1ONSTATE_SHIFT                   (16)
#define MEM1ONSTATE_MASK                    (0x3 << MEM1ONSTATE_SHIFT)
#define MEM1ONSTATE_MEMORYOFF_DOMAINON      (0x0 << MEM1ONSTATE_SHIFT)
#define MEM1ONSTATE_MEMORYRET_DOMAINON      (0x1 << MEM1ONSTATE_SHIFT)
#define MEM1ONSTATE_MEMORYON_DOMAINON       (0x3 << MEM1ONSTATE_SHIFT)
#define MEM1ONSTATE                         (MEM1ONSTATE_MASK)

#define MEMONSTATE_SHIFT                    (16)
#define MEMONSTATE_MASK                     (0x3 << MEMONSTATE_SHIFT)
#define MEMONSTATE_MEMORYON_DOMAINON        (0x3 << MEMONSTATE_SHIFT)
#define MEMONSTATE                          (MEMONSTATE_MASK)

#define L2CACHEONSTATE_SHIFT                (16)
#define L2CACHEONSTATE_MASK                 (0x3 << L2CACHEONSTATE_SHIFT)
#define L2CACHEONSTATE_MEMORYOFF_DOMAINON   (0x0 << L2CACHEONSTATE_SHIFT)
#define L2CACHEONSTATE_MEMORYON_DOMAINON    (0x3 << L2CACHEONSTATE_SHIFT)
#define L2CACHEONSTATE                      (L2CACHEONSTATE_MASK)

#define L2FLATMEMRETSTATE_SHIFT             (11)
#define L2FLATMEMRETSTATE_MASK              (0x1 << L2FLATMEMRETSTATE_SHIFT)
#define L2FLATMEMRETSTATE_MEMORYOFF_DOMAINRET (0x0 << L2FLATMEMRETSTATE_SHIFT)
#define L2FLATMEMRETSTATE_MEMORYRET_DOMAINRET (0x1 << L2FLATMEMRETSTATE_SHIFT)
#define L2FLATMEMRETSTATE                   (L2FLATMEMRETSTATE_MASK)

#define SHAREDL2CACHEFLATRETSTATE_SHIFT     (10)
#define SHAREDL2CACHEFLATRETSTATE_MASK      (0x1 << SHAREDL2CACHEFLATRETSTATE_SHIFT)
#define SHAREDL2CACHEFLATRETSTATE_MEMORYOFF_DOMAINRET (0x0 << SHAREDL2CACHEFLATRETSTATE_SHIFT)
#define SHAREDL2CACHEFLATRETSTATE_MEMORYRET_DOMAINRET (0x1 << SHAREDL2CACHEFLATRETSTATE_SHIFT)
#define SHAREDL2CACHEFLATRETSTATE           (SHAREDL2CACHEFLATRETSTATE_MASK)

#define L1FLATMEMRETSTATE_SHIFT             (9)
#define L1FLATMEMRETSTATE_MASK              (0x1 << L1FLATMEMRETSTATE_SHIFT)
#define L1FLATMEMRETSTATE_MEMORYOFF_DOMAINRET (0x0 << L1FLATMEMRETSTATE_SHIFT)
#define L1FLATMEMRETSTATE_MEMORYRET_DOMAINRET (0x1 << L1FLATMEMRETSTATE_SHIFT)
#define L1FLATMEMRETSTATE                   (L1FLATMEMRETSTATE_MASK)

#define MEM2RETSTATE_SHIFT                  (9)
#define MEM2RETSTATE_MASK                   (0x1 << MEM2RETSTATE_SHIFT)
#define MEM2RETSTATE_MEMORYOFF_DOMAINRET    (0x0 << MEM2RETSTATE_SHIFT)
#define MEM2RETSTATE_MEMORYRET_DOMAINRET    (0x1 << MEM2RETSTATE_SHIFT)
#define MEM2RETSTATE                        (MEM2RETSTATE_MASK)

#define SHAREDL1CACHEFLATRETSTATE_SHIFT     (8)
#define SHAREDL1CACHEFLATRETSTATE_MASK      (0x1 << SHAREDL1CACHEFLATRETSTATE_SHIFT)
#define SHAREDL1CACHEFLATRETSTATE_MEMORYOFF_DOMAINRET (0x0 << SHAREDL1CACHEFLATRETSTATE_SHIFT)
#define SHAREDL1CACHEFLATRETSTATE_MEMORYRET_DOMAINRET (0x1 << SHAREDL1CACHEFLATRETSTATE_SHIFT)
#define SHAREDL1CACHEFLATRETSTATE           (SHAREDL1CACHEFLATRETSTATE_MASK)

#define MEM1RETSTATE_SHIFT                  (8)
#define MEM1RETSTATE_MASK                   (0x1 << MEM1RETSTATE_SHIFT)
#define MEM1RETSTATE_MEMORYOFF_DOMAINRET    (0x0 << MEM1RETSTATE_SHIFT)
#define MEM1RETSTATE_MEMORYRET_DOMAINRET    (0x1 << MEM1RETSTATE_SHIFT)
#define MEM1RETSTATE                        (MEM1RETSTATE_MASK)

#define MEMRETSTATE_SHIFT                   (8)
#define MEMRETSTATE_MASK                    (0x3 << MEMRETSTATE_SHIFT)
#define MEMRETSTATE_MEMORYRET_DOMAINRET     (0x1 << MEMRETSTATE_SHIFT)
#define MEMRETSTATE                         (MEMRETSTATE_MASK)

#define L2CACHERETSTATE_SHIFT               (8)
#define L2CACHERETSTATE_MASK                (0x1 << L2CACHERETSTATE_SHIFT)
#define L2CACHERETSTATE_MEMORYOFF_DOMAINRET (0x0 << L2CACHERETSTATE_SHIFT)
#define L2CACHERETSTATE_MEMORYRET_DOMAINRET (0x1 << L2CACHERETSTATE_SHIFT)
#define L2CACHERETSTATE                     (L2CACHERETSTATE_MASK)

#define SAVEANDRESTORE_SHIFT                (4)
#define SAVEANDRESTORE_MASK                 (0x1 << SAVEANDRESTORE_SHIFT)
#define SAVEANDRESTORE                      (SAVEANDRESTORE_MASK)

#define MEMORYCHANGE_SHIFT                  (3)
#define MEMORYCHANGE_MASK                   (0x1 << MEMORYCHANGE_SHIFT)
#define MEMORYCHANGE_ENABLE                 (0x1 << MEMORYCHANGE_SHIFT)
#define MEMORYCHANGE                        (MEMORYCHANGE_MASK)

#define LOGICRETSTATE_SHIFT                 (2)
#define LOGICRETSTATE_MASK                  (0x1 << LOGICRETSTATE_SHIFT)
#define LOGICRETSTATE_LOGICOFF_DOMAINRET    (0x0 << LOGICRETSTATE_SHIFT)
#define LOGICRETSTATE_LOGICRET_DOMAINRET    (0x1 << LOGICRETSTATE_SHIFT)
#define LOGICRETSTATE                       (LOGICRETSTATE_MASK)

//-----------------------------------------------------------------------------
// voltage Control Registers

#define SMPS_SA0                            (1 << 0)
#define SMPS_RAV0                           (1 << 1)
#define SMPS_RAC0                           (1 << 2)
#define SMPS_RACEN0                         (1 << 3)
#define SMPS_CMD0                           (1 << 4)
#define SMPS_SA1                            (1 << 16)
#define SMPS_RAV1                           (1 << 17)
#define SMPS_RAC1                           (1 << 18)
#define SMPS_RACEN1                         (1 << 19)
#define SMPS_CMD1                           (1 << 20)

#define SMPS_SA0_SHIFT                      (0)
#define SMPS_SA0_MASK                       (0x7F << SMPS_SA0_SHIFT)
#define SMPS_SA1_SHIFT                      (16)
#define SMPS_SA1_MASK                       (0x7F << SMPS_SA1_SHIFT)

#define SMPS_VOLRA0_SHIFT                   (0)
#define SMPS_VOLRA0_MASK                    (0xFF << SMPS_VOLRA0_SHIFT)
#define SMPS_VOLRA1_SHIFT                   (16)
#define SMPS_VOLRA1_MASK                    (0xFF << SMPS_VOLRA1_SHIFT)

#define SMPS_CMDRA0_SHIFT                   (0)
#define SMPS_CMDRA0_MASK                    (0xFF << SMPS_CMDRA0_SHIFT)
#define SMPS_CMDRA1_SHIFT                   (16)
#define SMPS_CMDRA1_MASK                    (0xFF << SMPS_CMDRA1_SHIFT)

#define SMPS_MCODE_SHIFT                    (0)
#define SMPS_MCODE_MASK                     (0x7 << SMPS_MCODE_SHIFT)
#define SMPS_SREN                           (1 << 3)
#define SMPS_HSEN                           (1 << 4)
#define SMPS_HSMASTER                       (1 << 5)

#define SMPS_ON_SHIFT                       (24)
#define SMPS_ON_MASK                        (0xFF << SMPS_ON_SHIFT)
#define SMPS_ONLP_SHIFT                     (16)
#define SMPS_ONLP_MASK                      (0xFF << SMPS_ONLP_SHIFT)
#define SMPS_RET_SHIFT                      (8)
#define SMPS_RET_MASK                       (0xFF << SMPS_RET_SHIFT)
#define SMPS_OFF_SHIFT                      (0)
#define SMPS_OFF_MASK                       (0xFF << SMPS_OFF_SHIFT)

#define SMPS_ERROROFFSET_SHIFT              (24)
#define SMPS_ERROROFFSET_MASK               (0xFF << SMPS_ERROROFFSET_SHIFT)
#define SMPS_ERRORGAIN_SHIFT                (16)
#define SMPS_ERRORGAIN_MASK                 (0xFF << SMPS_ERRORGAIN_SHIFT)

#define SMPS_INITVOLTAGE_SHIFT              (8)
#define SMPS_INITVOLTAGE_MASK               (0xFF << SMPS_INITVOLTAGE_SHIFT)

#define SMPS_VOLTAGE_SHIFT                  (0)
#define SMPS_VOLTAGE_MASK                   (0xFF << SMPS_VOLTAGE_SHIFT)

#define SMPS_SMPSWAITTIMEMIN_SHIFT          (8)
#define SMPS_SMPSWAITTIMEMIN_MASK           (0xFFFF << SMPS_SMPSWAITTIMEMIN_SHIFT)
#define SMPS_VSTEPMIN_SHIFT                 (0)
#define SMPS_VSTEPMIN_MASK                  (0xFF << SMPS_VSTEPMIN_SHIFT)

#define SMPS_SMPSWAITTIMEMAX_SHIFT          (8)
#define SMPS_SMPSWAITTIMEMAX_MASK           (0xFFFF << SMPS_SMPSWAITTIMEMAX_SHIFT)
#define SMPS_VSTEPMAX_SHIFT                 (0)
#define SMPS_VSTEPMAX_MASK                  (0xFF << SMPS_VSTEPMAX_SHIFT)

#define SMPS_VDDMAX_SHIFT                   (24)
#define SMPS_VDDMAX_MASK                    (0xFF << SMPS_VDDMAX_SHIFT)
#define SMPS_VDDMIN_SHIFT                   (16)
#define SMPS_VDDMIN_MASK                    (0xFF << SMPS_VDDMIN_SHIFT)

#define SMPS_TIMEOUT_SHIFT                  (0)
#define SMPS_TIMEOUT_MASK                   (0xFFFF << SMPS_TIMEOUT_SHIFT)

#define SMPS_VPENABLE                       (1 << 0)
#define SMPS_FORCEUPDATE                    (1 << 1)
#define SMPS_INITVDD                        (1 << 2)
#define SMPS_TIMEOUTEN                      (1 << 3)

#define SMPS_VPINIDLE                       (1 << 0)

//-----------------------------------------------------------------------------
// MPU/IVA Control masks
#define PRM_IRQENABLE_WKUP_EN               (1 << 0)
#define PRM_IRQENABLE_FORCEWKUP_EN          (1 << 1)
#define PRM_IRQENABLE_IVA2_DPLL_RECAL_EN_IVA2 (1 << 2)
#define PRM_IRQENABLE_EVGENON_EN            (1 << 2)
#define PRM_IRQENABLE_EVGENOFF_EN           (1 << 3)
#define PRM_IRQENABLE_TRANSITION_EN         (1 << 4)
#define PRM_IRQENABLE_CORE_DPLL_RECAL_EN    (1 << 5)
#define PRM_IRQENABLE_PERIPH_DPLL_RECAL_EN  (1 << 6)
#define PRM_IRQENABLE_MPU_DPLL_RECAL_EN     (1 << 7)
#define PRM_IRQENABLE_IVA2_DPLL_RECAL_EN_MPU (1 << 8)
#define PRM_IRQENABLE_IO_EN                 (1 << 9)
#define PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN  (1 << 10)
#define PRM_IRQENABLE_VP1_MINVDD_EN         (1 << 11)
#define PRM_IRQENABLE_VP1_MAXVDD_EN         (1 << 12)
#define PRM_IRQENABLE_VP1_NOSMPSACK_EN      (1 << 13)
#define PRM_IRQENABLE_VP1_EQVALUE_EN        (1 << 14)
#define PRM_IRQENABLE_VP1_TRANXDONE_EN      (1 << 15)
#define PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN  (1 << 16)
#define PRM_IRQENABLE_VP2_MINVDD_EN         (1 << 17)
#define PRM_IRQENABLE_VP2_MAXVDD_EN         (1 << 18)
#define PRM_IRQENABLE_VP2_NOSMPSACK_EN      (1 << 19)
#define PRM_IRQENABLE_VP2_EQVALUE_EN        (1 << 20)
#define PRM_IRQENABLE_VP2_TRANXDONE_EN      (1 << 21)
#define PRM_IRQENABLE_VC_SAERR_EN           (1 << 22)
#define PRM_IRQENABLE_VC_RAERR_EN           (1 << 23)
#define PRM_IRQENABLE_VC_TIMEOUTERR_EN      (1 << 24)
#define PRM_IRQENABLE_SND_PERIPH_DPLL_RECAL_EN (1 << 25)
// DM3730 only
#define PRM_IRQENABLE_ABB_LDO_TRANXDONE_EN  (1 << 26)
#define PRM_IRQENABLE_VC_VP1_ACK_EN         (1 << 27)
#define PRM_IRQENABLE_VC_BYPASS_ACK_EN      (1 << 28)

//-----------------------------------------------------------------------------
// SDRC Control masks
//
#define SDRC_RFR_CTRL_ARCV_SHIFT            (8)
#define SDRC_RFR_CTRL_ARCV_MASK             (0xFFFF << SDRC_RFR_CTRL_ARCV_SHIFT)

#pragma warning(pop)

//-----------------------------------------------------------------------------

#endif
