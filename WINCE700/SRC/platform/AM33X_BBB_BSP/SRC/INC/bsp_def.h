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
//
//  Define:  BSP_GPMC_xxx
//
//  These constants are used to initialize general purpose memory configuration 
//  registers
//
// NOTE - Settings below are based on CORE DPLL = 332MHz, L3 = CORE/2 (166MHz)

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
