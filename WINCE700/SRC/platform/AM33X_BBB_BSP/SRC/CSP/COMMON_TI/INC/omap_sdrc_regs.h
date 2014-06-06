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
//  File:  omap_sdrc_regs.h
//
//  This header file is comprised of SDRAM module register structure and
//  definitions.

#ifndef __OMAP_SDRC_REGS_H
#define __OMAP_SDRC_REGS_H

//------------------------------------------------------------------------------
//
// SMS
//
typedef struct {

   REG32    SMS_REVISION;             //offset 0x0
   REG32    zzzReserved_1[3];
   REG32    SMS_SYSCONFIG;            //offset 0x10, system config
   REG32    SMS_SYSSTATUS;            //offset 0x14, system status
   REG32    zzzReserved_2[(0x48 - 0x18) / 4];

   REG32	SMS_RG_ATT[8];			  //offset 0x48, region attributes
   REG32    zzzReserved_3[(0x150 - 0x68) / 4];
   
   REG32    SMS_CLASS_ARBITER0;       //offset 0x150
   REG32    SMS_CLASS_ARBITER1;       //offset 0x154
   REG32    SMS_CLASS_ARBITER2;       //offset 0x158
   REG32    zzzReserved_4;            //offset 0x15c
   REG32    SMS_INTERCLASS_ARBITER;   //offset 0x160
   
} OMAP_SMS_REGS;



//------------------------------------------------------------------------------
//
// SDRC
//
typedef struct {

   REG32    SDRC_REVISION;            //offset 0x00,IP revision code
   REG32    zzzReserved_1[3];
   REG32    SDRC_SYSCONFIG;           //offset 0x10,OCP i/f control
   REG32    SDRC_SYSSTATUS;           //offset 0x14,module status
   REG32    zzzReserved_2[10];
   REG32    SDRC_CS_CFG;              //offset 0x40,config of start addr of cs1
   REG32    SDRC_SHARING;             //offset 0x44,controls GPMC /access wrt SDRC
   REG32    SDRC_ERR_ADDR;            //offset 0x48,addr of last illegal access received on OCP i/f.
   REG32    SDRC_ERR_TYPE;            //offset 0x4C,info abt last illegal access
   REG32    zzzReserved_3[4];
   REG32    SDRC_DLLA_CTRL;           //offset 0x60,controls SDRC DLL A resrce for fine tuning a DDR I/F
   REG32    SDRC_DLLA_STATUS;         //offset 0x64,reflects the current status of DLL A
   REG32    SDRC_DLLB_CTRL;           //offset 0x68,controls SDRC DLL B resrce for fine tuning a DDR I/F
   REG32    SDRC_DLLB_STATUS;         //offset 0x6C,reflects the curr status of DLL B
   REG32    SDRC_POWER;               //offset 0x70,defn of global pwr mgmt policy
   REG32    zzzReserved_4[3];
   REG32    SDRC_MCFG_0;              //offset 0x80,memory config reg0
   REG32    SDRC_MR_0;                //offset 0x84,
   REG32    SDRC_EMR1_0;              //offset 0x88,DDR1 EMR reg
   REG32    SDRC_EMR2_0;              //offset 0x8C,low-power EMR reg
   REG32    zzzReserved_0x90;         //offset 0x90,
   REG32    SDRC_DCDL1_CTRL;          //offset 0x94, DCDL delay adj ctrl
   REG32    SDRC_DCDL2_CTRL;          //offset 0x98, DCDL delay adj ctrl
   REG32    SDRC_ACTIM_CTRLA_0;       //offset 0x9C, sets the ac params 
                                               //values in the clock cycle unit 
                                               //to match memory characteristics
   REG32    SDRC_ACTIM_CTRLB_0;       //offset 0xA0, sets the ac params  
                                               //values in the clock cycle unit 
                                               //to match memory characteristics
   REG32    SDRC_RFR_CTRL_0;          //offset 0xA4,SDRAM autorefresh control
   REG32    SDRC_MANUAL_0;            //offset 0xA8,cmd to ext mem devices
   REG32    zzzReserved_0xAC;
   REG32    SDRC_MCFG_1;              //offset 0xB0,memory config reg1
   REG32    SDRC_MR_1;                //offset 0xB4,SDRAM MR reg
   REG32    SDRC_EMR1_1;              //offset 0xB8, DDR1 EMR reg
   REG32    SDRC_EMR2_1;              //offset 0xBC, low-power EMR reg
   REG32    zzzReserved_0xC0;         //offset 0xC0,
   REG32    SDRC_ACTIM_CTRLA_1;       //offset 0xC4, sets the ac params 
                                               //values in the clock cycle unit 
                                               //to match memory characteristics
   REG32    SDRC_ACTIM_CTRLB_1;       //offset 0xC8, sets the ac params 
                                               //values in the clock cycle unit 
                                               //to match memory characteristics
   REG32    zzzReserved_5[2];
   REG32    SDRC_RFR_CTRL_1;          //offset 0xD4,SDRAM autorefreshcontrol
   REG32    SDRC_MANUAL_1;            //offset 0xD8,cmd to ext mem devs
   
} OMAP_SDRC_REGS;


//------------------------------------------------------------------------------

//  SDRC Modes
#define SDRC_MODE_NORMAL                       0x00000010
#define SDRC_MODE_RESET                        0x00000012

// SDRC SYSCONFIG
#define SDRC_SYSCONFIG_SOFTRESET			  (1 << 1)

// SDRC SYSCONFIG
#define SDRC_SYSSTATUS_RESETDONE			  (1 << 0)

//  SDRC Manual command codes
#define SDRC_CMDCODE_NOP                       0x00
#define SDRC_CMDCODE_PRECHARGE                 0x01
#define SDRC_CMDCODE_AUTOREFRESH               0x02
#define SDRC_CMDCODE_ENTER_DEEP_POWER_DOWN     0x03
#define SDRC_CMDCODE_EXIT_DEEP_POWER_DOWN      0x04
#define SDRC_CMDCODE_ENTER_SELF_REFRESH        0x05
#define SDRC_CMDCODE_EXIT_SELF_REFRESH         0x06
#define SDRC_CMDCODE_SET_CKE_HIGH              0x07
#define SDRC_CMDCODE_SET_CKE_LOW               0x08

//  SDRC DLL Control codes
#define SDRC_DLL_ENABLE                        (1 << 3)
#define SDRC_DLL_LOCK                          (1 << 2)
#define SDRC_DLL_PHASE_72                      (0 << 1)
#define SDRC_DLL_PHASE_90                      (1 << 1)
#define SDRC_72DEG_PHASE_LIMIT                 (133)

//  SDRC POWER modes
#define SDRC_POWER_DELAY                       (6 << 8)
#define SDRC_POWER_SRFRONRESET                 (1 << 7)
#define SDRC_POWER_SRFRONIDLEREQ               (1 << 6)
#define SDRC_POWER_EXTCLKDIS                   (1 << 3)
#define SDRC_POWER_PWDENA                      (1 << 2)
#define SDRC_POWER_CLKCTRL_1                   (1 << 4)
#define SDRC_POWER_CLKCTRL_2                   (2 << 4)
#define SDRC_POWER_AUTOCOUNT(x)                (x << 8)

// SMS SYSCONFIG
#define SMS_SYSCONFIG_AUTOIDLE				   (1 << 0)
#define SMS_SYSCONFIG_SOFTRESET				   (1 << 1)

// SMS SYSSTATUS
#define SMS_SYSSTATUS_RESETDONE				   (1 << 0)

#endif
