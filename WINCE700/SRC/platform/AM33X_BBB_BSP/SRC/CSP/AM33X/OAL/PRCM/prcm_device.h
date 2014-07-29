// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//  File: prcm_device.h

#include "prcm_priv.h"

#define IDLEST_MASK         (0x3<<16)
#define STBYST_MASK         (0x1<<18)


DeviceLookupEntry s_rgDeviceLookupTable[] =
{
//	{POWERDOMAIN      FCLK             ICLK        ref count   CLKCTRL      pidleStatus     pStdbyStatus }
    
    { POWERDOMAIN_WKUP, {1,{kADC_FCLK}},                    {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_ADC_TSC_CLKCTRL),       IDLEST_MASK, 0},//AM_DEVICE_ADC_TSC = 0, /* WKUP */ /* 0 */
    { POWERDOMAIN_WKUP, {0,{kCLK_NULL}},                    {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_CONTROL_CLKCTRL),       IDLEST_MASK, 0},//AM_DEVICE_CONTROL,
    { POWERDOMAIN_WKUP, {2,{kDEBUG_CLKA_GCLK, kDBGSYSCLK}}, {1,{kL3_AON_GCLK}},     0, PRCM_OFS(CM_WKUP_DEBUGSS_CLKCTRL),       IDLEST_MASK, STBYST_MASK},//AM_DEVICE_DEBUGSS,
    { POWERDOMAIN_WKUP, {1,{kGPIO0_GDBCLK}},                {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_GPIO0_CLKCTRL),         IDLEST_MASK, 0},//AM_DEVICE_GPIO0,
    { POWERDOMAIN_WKUP, {1,{kI2C0_GFCLK}},                  {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_I2C0_CLKCTRL),          IDLEST_MASK, 0},//AM_DEVICE_I2C0,
    { POWERDOMAIN_WKUP, {0,{kCLK_NULL}},                    {1,{kL4_WKUP_AON_GCLK}},0, PRCM_OFS(CM_WKUP_L4WKUP_CLKCTRL),        IDLEST_MASK, 0},//AM_DEVICE_L4WKUP,
    { POWERDOMAIN_WKUP, {1,{kSR_SYSCLK}},                   {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_SMARTREFLEX0_CLKCTRL),  IDLEST_MASK, 0},//AM_DEVICE_SMARTREFLEX0,
    { POWERDOMAIN_WKUP, {1,{kSR_SYSCLK}},                   {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_SMARTREFLEX1_CLKCTRL),  IDLEST_MASK, 0},//AM_DEVICE_SMARTREFLEX1,
    { POWERDOMAIN_WKUP, {1,{kTIMER0_GCLK}},                 {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_TIMER0_CLKCTRL),        IDLEST_MASK, 0},//AM_DEVICE_TIMER0,
    { POWERDOMAIN_WKUP, {1,{kTIMER1_GCLK}},                 {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_TIMER1_CLKCTRL),        IDLEST_MASK, 0},//AM_DEVICE_TIMER1,
    { POWERDOMAIN_WKUP, {1,{kUART0_GFCLK}},                 {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_UART0_CLKCTRL),         IDLEST_MASK, 0},//AM_DEVICE_UART0,               /* 10 */
    { POWERDOMAIN_WKUP, {1,{kWDT0_GCLK}},                   {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_WDT0_CLKCTRL),          IDLEST_MASK, 0},//AM_DEVICE_WDT0,
    { POWERDOMAIN_WKUP, {1,{kWDT1_GCLK}},                   {1,{kL4_WKUP_GCLK}},    0, PRCM_OFS(CM_WKUP_WDT1_CLKCTRL),          IDLEST_MASK, 0},//AM_DEVICE_WDT1,
    { POWERDOMAIN_WKUP, {0,{kCLK_NULL}},                    {1,{kL4_WKUP_AON_GCLK}},0, PRCM_OFS(CM_WKUP_WKUP_M3_CLKCTRL),       0,           STBYST_MASK},//AM_DEVICE_WKUP_M3,
    { POWERDOMAIN_PER,  {1,{kL3_GCLK}},                     {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_AES0_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_AES0,         /* PER */
    { POWERDOMAIN_PER,  {1,{kL3_GCLK}},                     {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_AES1_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_AES1,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kCLK_24MHZ}},       0, PRCM_OFS(CM_PER_CLKDIV32K_CLKCTRL),      IDLEST_MASK, 0}, //AM_DEVICE_CLKDIV32K,
    { POWERDOMAIN_PER,  {4,{kCPSW_CPTS_RFT_CLK, kCPSW_250MHZ_CLK, kCPSW_5MHZ_CLK, kCPSW_50MHZ_CLK}},
                                                            {1,{kCPSW_125MHZ_CLK}}, 0, PRCM_OFS(CM_PER_CPGMAC0_CLKCTRL),        IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_CPGMAC0,
    { POWERDOMAIN_PER,  {1,{kCAN_CLK}},                     {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_DCAN0_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_DCAN0,
    { POWERDOMAIN_PER,  {1,{kCAN_CLK}},                     {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_DCAN1_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_DCAN1,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_ELM_CLKCTRL),            IDLEST_MASK, 0}, //AM_DEVICE_ELM,              /* 20 */
    { POWERDOMAIN_PER,  {1,{kEMIF_GCLK}},                   {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_EMIF_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_EMIF,
    { POWERDOMAIN_PER,  {1,{kL4FW_GCLK}},                   {1,{kL4FW_GCLK}},       0, PRCM_OFS(CM_PER_EMIF_FW_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_EMIF_FW,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_EPWMSS0_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_EPWM0,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_EPWMSS1_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_EPWM1,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_EPWMSS2_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_EPWM2,
    { POWERDOMAIN_PER,  {1,{kGPIO1_DBCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_GPIO1_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_GPIO1,
    { POWERDOMAIN_PER,  {1,{kGPIO2_DBCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_GPIO2_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_GPIO2,
    { POWERDOMAIN_PER,  {1,{kGPIO3_DBCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_GPIO3_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_GPIO3,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3S_GCLK}},        0, PRCM_OFS(CM_PER_GPMC_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_GPMC,
    { POWERDOMAIN_PER,  {1,{kI2C_FCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_I2C1_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_I2C1,                /* 30 */
    { POWERDOMAIN_PER,  {1,{kI2C_FCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_I2C2_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_I2C2,
    { POWERDOMAIN_PER,  {2,{kICSS_IEP_GCLK, kICSS_UART_GCLK}}, {1,{kICSS_OCP_GCLK}},0, PRCM_OFS(CM_PER_ICSS_CLKCTRL),           IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_ICSS,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3S_GCLK}},        0, PRCM_OFS(CM_PER_IEEE5000_CLKCTRL),       IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_IEEE5000,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_L3_CLKCTRL),             IDLEST_MASK, 0}, //AM_DEVICE_L3,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_L3_INSTR_CLKCTRL),       IDLEST_MASK, 0}, //AM_DEVICE_L3_INSTR,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4FW_GCLK}},       0, PRCM_OFS(CM_PER_L4FW_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_L4FW,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_L4LS_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_L4LS,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4HS_GCLK}},       0, PRCM_OFS(CM_PER_L4HS_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_L4_HS,
    { POWERDOMAIN_PER,  {1,{kLCDC_GLCK}},                   {1,{kLCD_L3_GCLK}},     0, PRCM_OFS(CM_PER_LCDC_CLKCTRL),           IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_LCDC,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_MAILBOX0_CLKCTRL),       IDLEST_MASK, 0}, //AM_DEVICE_MAILBOX0,             /* 40 */
    { POWERDOMAIN_PER,  {1,{kMCASP_FCLK}},                  {1,{kL3S_GCLK}},        0, PRCM_OFS(CM_PER_MCASP0_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_MCASP0,
    { POWERDOMAIN_PER,  {1,{kMCASP_FCLK}},                  {1,{kL3S_GCLK}},        0, PRCM_OFS(CM_PER_MCASP1_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_MCASP1,
    { POWERDOMAIN_PER,  {1,{kMMC_FCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_MMC0_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_MMCHS0,
    { POWERDOMAIN_PER,  {1,{kMMC_FCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_MMC1_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_MMCHS1,
    { POWERDOMAIN_PER,  {1,{kMMC_FCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_MMC2_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_MMCHS2,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_MSTR_EXPS_CLKCTRL),      IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_MSTR_EXPS,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_OCMCRAM_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_OCMCRAM,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_OCPWP_CLKCTRL),          IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_OCPWP,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_PCIE_CLKCTRL),           IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_PCIE,
    { POWERDOMAIN_PER,  {1,{kL4LS_GCLK}},                   {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_PKA_CLKCTRL),            IDLEST_MASK, 0}, //AM_DEVICE_PKA,             /* 50 */
    { POWERDOMAIN_PER,  {1,{kL4LS_GCLK}},                   {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_RNG_CLKCTRL),            IDLEST_MASK, 0}, //AM_DEVICE_RNG,
    { POWERDOMAIN_PER,  {1,{kL3_GCLK}},                     {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_SHA0_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_SHA0,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_SLV_EXPS_CLKCTRL),       IDLEST_MASK, 0}, //AM_DEVICE_SLV_EXPS,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_SPARE0_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_SPARE0,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_SPARE1_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_SPARE1,
    { POWERDOMAIN_PER,  {1,{kSPI_GCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_SPI0_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_MCSPI0,
    { POWERDOMAIN_PER,  {1,{kSPI_GCLK}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_SPI1_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_MCSPI1,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_SPINLOCK_CLKCTRL),       IDLEST_MASK, 0}, //AM_DEVICE_SPINLOCK,
    { POWERDOMAIN_PER,  {1,{kTIMER2_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER2_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER2,
    { POWERDOMAIN_PER,  {1,{kTIMER3_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER3_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER3,               /* 60 */
    { POWERDOMAIN_PER,  {1,{kTIMER4_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER4_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER4,
    { POWERDOMAIN_PER,  {1,{kTIMER5_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER5_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER5,
    { POWERDOMAIN_PER,  {1,{kTIMER6_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER6_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER6,
    { POWERDOMAIN_PER,  {1,{kTIMER7_GCLK}},                 {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_TIMER7_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_TIMER7,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_TPCC_CLKCTRL),           IDLEST_MASK, 0}, //AM_DEVICE_TPCC,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_TPTC0_CLKCTRL),          IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_TPTC0,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_TPTC1_CLKCTRL),          IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_TPTC1,
    { POWERDOMAIN_PER,  {0,{kCLK_NULL}},                    {1,{kL3_GCLK}},         0, PRCM_OFS(CM_PER_TPTC2_CLKCTRL),          IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_TPTC2,
    { POWERDOMAIN_PER,  {1,{kUART_GFCLK}},                  {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_UART1_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_UART1,
    { POWERDOMAIN_PER,  {1,{kUART_GFCLK}},                  {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_UART2_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_UART2,                /* 70 */
    { POWERDOMAIN_PER,  {1,{kUART_GFCLK}},                  {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_UART3_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_UART3,
    { POWERDOMAIN_PER,  {1,{kUART_GFCLK}},                  {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_UART4_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_UART4,
    { POWERDOMAIN_PER,  {1,{kUART_GFCLK}},                  {1,{kL4LS_GCLK}},       0, PRCM_OFS(CM_PER_UART5_CLKCTRL),          IDLEST_MASK, 0}, //AM_DEVICE_UART5,
    { POWERDOMAIN_PER,  {2,{kL3S_GCLK, kUSB_PLL_CLK}},      {1,{kL3S_GCLK}},        0, PRCM_OFS(CM_PER_USB0_CLKCTRL),           IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_USB0,
    { POWERDOMAIN_GFX,  {0,{kCLK_NULL}},                    {1,{kGFX_L3_GCLK}},     0, PRCM_OFS(CM_GFX_BITBLT_CLKCTRL),         IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_BITBLT,           /* GFX */
    { POWERDOMAIN_GFX,  {1,{kGFX_FCLK}},                    {1,{kGFX_L3_GCLK}},     0, PRCM_OFS(CM_GFX_GFX_CLKCTRL),            IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_GFX,
    { POWERDOMAIN_GFX,  {0,{kCLK_NULL}},                    {1,{kGFX_L3_GCLK}},     0, PRCM_OFS(CM_GFX_MMUDATA_CLKCTRL),        IDLEST_MASK, 0}, //AM_DEVICE_MMU,
    { POWERDOMAIN_GFX,  {0,{kCLK_NULL}},                    {1,{kL4LS_GFX_GCLK}},   0, PRCM_OFS(CM_GFX_MMUCFG_CLKCTRL),         IDLEST_MASK, 0}, //AM_DEVICE_MMUCFG,
    { POWERDOMAIN_MPU,  {1,{kMPU_CLK}},                     {1,{kMPU_CLK}},         0, PRCM_OFS(CM_MPU_MPU_CLKCTRL),            IDLEST_MASK, STBYST_MASK}, //AM_DEVICE_MPU,              /* MPU */
    { POWERDOMAIN_RTC,  {1,{kRTC_32KCLK}},                  {1,{kL4_RTC_GCLK}},     0, PRCM_OFS(CM_RTC_RTC_CLKCTRL),            IDLEST_MASK, 0}, //AM_DEVICE_RTC,                  /* 80 */
    { POWERDOMAIN_CEFUSE,  {1,{kCUST_EFUSE_SYSCLK}},        {1,{kL4_CEFUSE_GCLK}},  0, PRCM_OFS(CM_CEFUSE_CEFUSE_CLKCTRL),      IDLEST_MASK, 0}, //AM_DEVICE_CEFUSE,
    { POWERDOMAIN_EFUSE,   {1,{kSTD_EFUSE_SYSCLK}},         {0,{kCLK_NULL}},        0, 0,                                       0,           0}, //AM_DEVICE_EFUSE,   

};

// this only has devices after  AM_DEVICE_COUNT. If the remap doesnt exist, use AM_DEVICE_END 
static UINT s_rgDeviceRemapTable[] = 
{
    AM_DEVICE_GPMC, //AM_DEVICE_NAND
    AM_DEVICE_GPMC, //AM_DEVICE_NOR
    AM_DEVICE_END,  //AM_DEVICE_FRAMER
    AM_DEVICE_END,  //AM_DEVICE_GPIO
    AM_DEVICE_END   //AM_DEVICE_END
};

//-----------------------------------------------------------------------------
static UINT s_rgActiveDomainDeviceCount[POWERDOMAIN_COUNT] = 
{
	0,	// POWERDOMAIN_WKUP       
	0,	// POWERDOMAIN_PER,
	0,	// POWERDOMAIN_GFX,
	0,	// POWERDOMAIN_MPU,
	0,	// POWERDOMAIN_RTC,       
	0,	// POWERDOMAIN_CEFUSE,
	0,	// POWERDOMAIN_EFUSE,
};

