// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  bsp_def.h
//
#ifndef __BSP_DEF_H
#define __BSP_DEF_H

#ifdef __cplusplus
extern "C" {
#endif


#define BSP_DEVICE_AM33x_PREFIX       "BBB33X-"

#define AM33x_OPP_NUM    5

//------------------------------------------------------------------------------
// I2C slave addresses
//Note: We mostly use I2C0 on the base board (BUS 1)
#define I2C_BASE_BOARD_ADDR			(0x50)
#define I2C_BASE_BOARD_BUS			(1)
//Note: We mostly use I2C2 for daughter bd detect (BUS 3)
#define I2C_DAUGHTER_BOARD1_ADDR	(0x54)
#define I2C_DAUGHTER_BOARD2_ADDR	(0x55)
#define I2C_DAUGHTER_BOARD3_ADDR	(0x56)
#define I2C_DAUGHTER_BOARD4_ADDR	(0x57)
#define I2C_DAUGHTER_BOARD_BUS		(3)
// TLV320AIC3106 (not used)
#define I2C_AUDIO_ADDR				(0x1b)
#define I2C_AUDIO_BUS				(1)
// TPS65217
#define I2C_PMIC_ADDR				(0x24)
#define I2C_PMIC_BUS				(1)
// some daughter boards (not used)
#define I2C_CPLD_ADDR				(0x35)
#define I2C_CPLD_BUS				(1)
// TDA19988
#define I2C_HDMI_ADDR				(0x70)
#define I2C_HDMI_BUS				(1)
#define I2C_CEC_ADDR				(0x34)
#define I2C_CEC_BUS					(1)

//------------------------------------------------------------------------------
//
//  Define:  TPS65217_I2C_DEVICE_ID
//
//  i2c bus twl is on
//      AM_DEVICE_I2C0
//      AM_DEVICE_I2C1
//      
//
#define TPS65217_I2C_DEVICE_ID              (AM_DEVICE_I2C0)

//------------------------------------------------------------------------
// LCD/DVI Resolutions
typedef enum OMAP_LCD_DVI_RES {
	OMAP_RES_DEFAULT=0,
    OMAP_LCD_320W_240H,
    OMAP_LCD_480W_272H,
    OMAP_LCD_640W_480H,
    OMAP_LCD_800W_480H,
    OMAP_DVI_800W_600H,		// first DVI supported mode see IsDVIMode()
    OMAP_DVI_1024W_768H,
    OMAP_DVI_1280W_720H,
    OMAP_RES_INVALID
}OMAP_LCD_DVI_RES;


//------------------------------------------------------------------------------
//  default timeout in tick count units (milli-seconds)
#define BSP_I2C_TIMEOUT_INIT            (500)

#define BSP_I2C0_OA_INIT                (0x0E)
#define BSP_I2C0_BAUDRATE_INIT          (1)
#define BSP_I2C0_MAXRETRY_INIT          (5)
#define BSP_I2C0_RX_THRESHOLD_INIT      (5)
#define BSP_I2C0_TX_THRESHOLD_INIT      (5)

#define BSP_I2C1_OA_INIT                (0x0E)
#define BSP_I2C1_BAUDRATE_INIT          (1)
#define BSP_I2C1_MAXRETRY_INIT          (5)
#define BSP_I2C1_RX_THRESHOLD_INIT      (5)
#define BSP_I2C1_TX_THRESHOLD_INIT      (5)

#define BSP_I2C2_OA_INIT                (0x0E)
#define BSP_I2C2_BAUDRATE_INIT          (1)
#define BSP_I2C2_MAXRETRY_INIT          (5)
#define BSP_I2C2_RX_THRESHOLD_INIT      (5)
#define BSP_I2C2_TX_THRESHOLD_INIT      (5)

#define BSP_I2C3_OA_INIT                (0x0E)
#define BSP_I2C3_BAUDRATE_INIT          (1)
#define BSP_I2C3_MAXRETRY_INIT          (5)
#define BSP_I2C3_RX_THRESHOLD_INIT      (5)
#define BSP_I2C3_TX_THRESHOLD_INIT      (5)

//------------------------------------------------------------------------------
//
//  Select initial XLDR CPU and IVA speed and VDD1 voltage using BSP_OPM_SELECT 
//
    // MPU[720hz @ 1.35V], IVA2[520Mhz @ 1.35V]
    #define BSP_SPEED_CPUMHZ                720
    #define BSP_SPEED_IVAMHZ                520
    #define VDD1_INIT_VOLTAGE_VALUE         0x3c

//------------------------------------------------------------------------------
//
//  Define:  BSP_DEVICE_PREFIX
//
//  This define is used as device name prefix when KITL creates device name.
//
#define BSP_DEVICE_PREFIX       BSP_DEVICE_AM33x_PREFIX


//------------------------------------------------------------------------------
#if 0

#define BSP_B32NOT16_1                 (1 << 4)     // Ext. SDRAM is x32 bit.
#define BSP_DEEPPD_1                   (1 << 3)     // supports deep-power down
#define BSP_DDRTYPE_1                  (0 << 2)     // SDRAM is MobileDDR
#define BSP_RAMTYPE_1                  (1 << 0)     // SDRAM is DDR

#define BSP_SDRC_MCFG_1                (BSP_RASWIDTH_1 | \
                                        BSP_CASWIDTH_1 | \
                                        BSP_ADDRMUXLEGACY_1 | \
                                        BSP_RAMSIZE_1 | \
                                        BSP_BANKALLOCATION_1 | \
                                        BSP_B32NOT16_1 | \
                                        BSP_DEEPPD_1 | \
                                        BSP_DDRTYPE_1 | \
                                        BSP_RAMTYPE_1)



//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_SHARING
//
//  Determines the SDRC module attached memory size and position on the SDRC 
//  module I/Os..  Used to update SDRC_SHARING
//
//  Allowed values:
//
#define BSP_CS1MUXCFG                  (0 << 12)    // 32-bit SDRAM on [31:0]
#define BSP_CS0MUXCFG                  (0 << 9)     // 32-bit SDRAM on [31:0]
#define BSP_SDRCTRISTATE               (1 << 8)     // Normal mode

#define BSP_SDRC_SHARING               (BSP_CS1MUXCFG | \
                                        BSP_CS0MUXCFG | \
                                        BSP_SDRCTRISTATE)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLA_0
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLA_0
//
//  Allowed values:
//
// NOTE - Settings below are based on CORE DPLL = 332MHz, L3 = CORE/2 (166MHz)

/* Samsung version of EVM3530 [K5W1G1GACM-DL60](166MHz optimized) ~ 6.0ns
 * Micron version of EVM3530 [MT29C2G24MAKLAJG-6](166MHz optimized) ~ 6.0ns
 *
 * ACTIM_CTRLA -
 *  TWR = 12/6  = 2 (samsung)
 *  TWR = 15/6  = 3 (micron)
 *  TDAL = Twr/Tck + Trp/tck = 12/6 + 18/6 = 2 + 3 = 5  (samsung)
 *  TDAL = Twr/Tck + Trp/tck = 15/6 + 18/6 = 3 + 3 = 6  (micron)
 *  TRRD = 12/6 = 2
 *  TRCD = 18/6 = 3
 *  TRP = 18/6  = 3
 *  TRAS = 42/6 = 7
 *  TRC = 60/6  = 10
 *  TRFC = 72/6 = 12 (samsung)
 *  TRFC = 125/6 = 21 (micron)
 *
 * ACTIM_CTRLB -
 *  TCKE            = 2 (samsung)
 *  TCKE            = 1 (micron)
 *  XSR = 120/6   = 20  (samsung)
 *  XSR = 138/6   = 23  (micron)
 */

// Choose more conservative of memory timings when they differ between vendors
#define BSP_TRFC_0                     (21 << 27)   // Autorefresh to active
#define BSP_TRC_0                      (10 << 22)   // Row cycle time
#define BSP_TRAS_0                     (7 << 18)    // Row active time
#define BSP_TRP_0                      (3 << 15)    // Row precharge time
#define BSP_TRCD_0                     (3 << 12)    // Row to column delay time
#define BSP_TRRD_0                     (2 << 9)     // Active to active cmd per.
#define BSP_TWR_0                      (3 << 6)     // Data-in to precharge cmd
#define BSP_TDAL_0                     (6 << 0)     // Data-in to active command

#define BSP_SDRC_ACTIM_CTRLA_0         (BSP_TRFC_0 | \
                                        BSP_TRC_0 | \
                                        BSP_TRAS_0 | \
                                        BSP_TRP_0 | \
                                        BSP_TRCD_0 | \
                                        BSP_TRRD_0 | \
                                        BSP_TWR_0 | \
                                        BSP_TDAL_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLA_1
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLA_1
//
//  Allowed values:
//
#define BSP_SDRC_ACTIM_CTRLA_1          BSP_SDRC_ACTIM_CTRLA_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLB_0
//
//  Determines ac timing control register B.  Used to update SDRC_ACTIM_CTRLB_0
//
//  Allowed values:
//
#define BSP_TWTR_0                     (0x1 << 16)  // 1-cycle write to read delay
#define BSP_TCKE_0                     (2 << 12)    // CKE minimum pulse width
#define BSP_TXP_0                      (0x5 << 8)   // 5 minimum cycles
#define BSP_TXSR_0                     (20 << 0)    // Self Refresh Exit to Active period

#define BSP_SDRC_ACTIM_CTRLB_0         (BSP_TCKE_0 | \
                                        BSP_TXSR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLB_1
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLB_1
//
//  Allowed values:
//
#define BSP_SDRC_ACTIM_CTRLB_1          BSP_SDRC_ACTIM_CTRLB_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_RFR_CTRL_0
//
//  SDRAM memory autorefresh control.  Used to update SDRC_RFR_CTRL_0
//
//  Allowed values:
//
#define BSP_ARCV                       (0x4E2)
#define BSP_ARCV_0                     (BSP_ARCV << 8)  // Autorefresh counter val
#define BSP_ARE_0                      (1 << 0)         // Autorefresh on counter x1

#define BSP_SDRC_RFR_CTRL_0            (BSP_ARCV_0 | \
                                        BSP_ARE_0)

//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_RFR_CTRL_1
//
//  SDRAM memory autorefresh control.  Used to update SDRC_RFR_CTRL_1
//
//  Allowed values:
//
#define BSP_SDRC_RFR_CTRL_1             BSP_SDRC_RFR_CTRL_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MR_0
//
//  Corresponds to the JEDEC SDRAM MR register.  Used to update SDRC_MR_0
//
//  Allowed values:
//
#define BSP_CASL_0                     (3 << 4)    // CAS latency = 3
#define BSP_SIL_0                      (0 << 3)    // Serial mode
#define BSP_BL_0                       (2 << 0)    // Burst Length = 4(DDR only)

#define BSP_SDRC_MR_0                  (BSP_CASL_0 | \
                                        BSP_SIL_0 | \
                                        BSP_BL_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MR_0
//
//  Corresponds to the JEDEC SDRAM MR register.  Used to update SDRC_MR_1
//
//  Allowed values:
//
#define BSP_SDRC_MR_1                  (BSP_SDRC_MR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_EMR2_0
//
//  Corresponds to the low-power EMR register, as defined in the mobile DDR 
//  JEDEC standard.  Used to update SDRC_EMR2_0
//
//  Allowed values:
//
#define BSP_DS_0                       (0 << 5)    // Strong-strength driver
#define BSP_TCSR_0                     (0 << 3)    // 70 deg max temp
#define BSP_PASR_0                     (0 << 0)    // All banks

#define BSP_SDRC_EMR2_0                (BSP_DS_0 | \
                                        BSP_TCSR_0 | \
                                        BSP_PASR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_EMR2_1
//
//  Corresponds to the low-power EMR register, as defined in the mobile DDR 
//  JEDEC standard.  Used to update SDRC_EMR2_1
//
//  Allowed values:
//
#define BSP_SDRC_EMR2_1                (BSP_SDRC_EMR2_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_DLLA_CTRL
//
//  Used to fine-tune DDR timings.  Used to update SDRC_DLLA_CTRL
//
//  Allowed values:
//
#define BSP_FIXEDELAY                  (38 << 24)
#define BSP_MODEFIXEDDELAYINITLAT      (0 << 16)
#define BSP_DLLMODEONIDLEREQ           (0 << 5)
#define BSP_ENADLL                     (1 << 3)     // enable DLLs
#define BSP_LOCKDLL                    (0 << 2)     // run in unlock mode
#define BSP_DLLPHASE                   (1 << 1)     // 72 deg phase
#define BSP_SDRC_DLLA_CTRL             (BSP_FIXEDELAY | \
                                        BSP_MODEFIXEDDELAYINITLAT | \
                                        BSP_DLLMODEONIDLEREQ | \
                                        BSP_ENADLL | \
                                        BSP_LOCKDLL | \
                                        BSP_DLLPHASE)

//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_POWER_REG
//
//  Set SDRAM power management mode.  Used to update SDRC_POWER_REG
//
//  Allowed values:
//
#define BSP_WAKEUPPROC                 (0 << 26)    // don't stall 500 cycles on 1st access
#define BSP_AUTOCOUNT                  (4370 << 8)  // SDRAM idle count down
//#define BSP_SRFRONRESET                (0 << 7)     // disable idle on reset
#define BSP_SRFRONRESET                (1 << 7)     // enable self refresh on warm reset
#define BSP_SRFRONIDLEREQ              (1 << 6)     // hw idle on request
#define BSP_CLKCTRL                    (2 << 4)     // self-refresh on auto_cnt
#define BSP_EXTCLKDIS                  (1 << 3)     // disable ext clock
#define BSP_PWDENA                     (1 << 2)     // active power-down mode 
#define BSP_PAGEPOLICY                 (1 << 0)     // must be 1
#define BSP_SDRC_POWER_REG             (BSP_WAKEUPPROC | \
                                        BSP_AUTOCOUNT | \
                                        BSP_SRFRONRESET | \
                                        BSP_SRFRONIDLEREQ | \
                                        BSP_CLKCTRL | \
                                        BSP_EXTCLKDIS | \
                                        BSP_PWDENA | \
                                        BSP_PAGEPOLICY)

#endif
//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_DLLB_CTRL
//
//  Used to fine-tune DDR timings.  Used to update SDRC_DLLB_CTRL
//
//  Allowed values:
//
#define BSP_SDRC_DLLB_CTRL             (BSP_SDRC_DLLA_CTRL)

//------------------------------------------------------------------------------
//
//  Define:  BSP_GPMC_xxx
//
//  These constants are used to initialize general purpose memory configuration 
//  registers
//
// NOTE - Settings below are based on CORE DPLL = 332MHz, L3 = CORE/2 (166MHz)

// ONE NAND settings, not optimized
#define BSP_GPMC_ONENAND_CONFIG1        0x00001200
#define BSP_GPMC_ONENAND_CONFIG2        0x000F0F01
#define BSP_GPMC_ONENAND_CONFIG3        0x00030301
#define BSP_GPMC_ONENAND_CONFIG4        0x0F040F04
#define BSP_GPMC_ONENAND_CONFIG5        0x010F1010
#define BSP_GPMC_ONENAND_CONFIG6        0x1F060000
#define BSP_GPMC_ONENAND_CONFIG7        0x00000C4C     // Base address 0x0C000000, 64 MB window

//  NAND settings, not optimized
#define GPMC_SIZE_256M		0x0
#define GPMC_SIZE_128M		0x8
#define GPMC_SIZE_64M		0xC
#define GPMC_SIZE_32M		0xE
#define GPMC_SIZE_16M		0xF

#define GPMC_MAX_REG        7

#define GPMC_NAND_BASE     0x08000000
#define GPMC_NAND_SIZE     GPMC_SIZE_128M

#if 1 /*BSP_AM33X*/   /* SA 8-Bit Nand */
#define M_NAND_GPMC_CONFIG1	0x00000800
#else
#define M_NAND_GPMC_CONFIG1	0x00001810
#endif
#define M_NAND_GPMC_CONFIG2    0x001e1e00
#define M_NAND_GPMC_CONFIG3    0x001e1e00
#define M_NAND_GPMC_CONFIG4    0x16051807
#define M_NAND_GPMC_CONFIG5    0x00151e1e
#define M_NAND_GPMC_CONFIG6	   0x16000f80
#define M_NAND_GPMC_CONFIG7	   0x00000008

//------------------------------------------------------------------------------
//
//  Define:  BSP_UART_DSIUDLL & BSP_UART_DSIUDLH
//
//  This constants are used to initialize serial debugger output UART.
//  Serial debugger uses 115200-8-N-1
//
#define BSP_UART_LCR                   (0x03)
#define BSP_UART_DSIUDLL               (26)
#define BSP_UART_DSIUDLH               (0)

BOOL BSPInsertGpioDevice(UINT range,void* fnTbl,WCHAR* name);


// nand pin connection information
#define BSP_GPMC_NAND_CS            (0)      // NAND is on CHIP SELECT 0
#define BSP_GPMC_IRQ_WAIT_EDGE      (GPMC_IRQENABLE_WAIT0_EDGEDETECT)


#define BSP_WATCHDOG_TIMEOUTPERIOD_MILLISECONDS    (20000)
#define BSP_WATCHDOG_REFRESH_MILLISECONDS   (500)
#define BSP_WATCHDOG_THREAD_PRIORITY        (100)
#define TIME_BOMB_PERIOD					(3600000 / BSP_WATCHDOG_REFRESH_MILLISECONDS) // 1hour

/* NLED GPIO settings */
#define NOTIFICATION_LED0_GPIO      (53)
#define NOTIFICATION_LED1_GPIO      (54)
#define NOTIFICATION_LED2_GPIO      (55)
#define NOTIFICATION_LED3_GPIO      (56)

#ifdef __cplusplus
}
#endif

#endif
