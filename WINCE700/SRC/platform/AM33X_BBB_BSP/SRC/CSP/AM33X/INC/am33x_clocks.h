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
//
//  File:  am33x_clocks.h
//
//  This header defines all relevant clocks and power domains for am33x.
//
#ifndef __AM33X_CLOCKS_H
#define __AM33X_CLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------


#define AM33X_GENERIC        0xFFFFFFFF

typedef enum {    
    AM_DEVICE_ADC_TSC = 0, /* WKUP */ /* 0 */
    AM_DEVICE_CONTROL,
    AM_DEVICE_DEBUGSS,
    AM_DEVICE_GPIO0,
    AM_DEVICE_I2C0,
    AM_DEVICE_L4WKUP,
    AM_DEVICE_SMARTREFLEX0,
    AM_DEVICE_SMARTREFLEX1,
    AM_DEVICE_TIMER0,
    AM_DEVICE_TIMER1,
    AM_DEVICE_UART0,               /* 10 */
    AM_DEVICE_WDT0,
    AM_DEVICE_WDT1,
    AM_DEVICE_WKUP_M3,
    AM_DEVICE_AES0,         /* PER */
    AM_DEVICE_AES1,
    AM_DEVICE_CLKDIV32K,
    AM_DEVICE_CPGMAC0,
    AM_DEVICE_DCAN0,
    AM_DEVICE_DCAN1,
    AM_DEVICE_ELM,               /* 20 */
    AM_DEVICE_EMIF,
    AM_DEVICE_EMIF_FW,
    AM_DEVICE_EPWM0,
    AM_DEVICE_EPWM1,
    AM_DEVICE_EPWM2,
    AM_DEVICE_GPIO1,
    AM_DEVICE_GPIO2,
    AM_DEVICE_GPIO3,
    AM_DEVICE_GPMC,
    AM_DEVICE_I2C1,               /* 30 */
    AM_DEVICE_I2C2,
    AM_DEVICE_ICSS,
    AM_DEVICE_IEEE5000,
    AM_DEVICE_L3,
    AM_DEVICE_L3_INSTR,
    AM_DEVICE_L4FW,
    AM_DEVICE_L4LS,
    AM_DEVICE_L4_HS,
    AM_DEVICE_LCDC,
    AM_DEVICE_MAILBOX0,             /* 40 */
    AM_DEVICE_MCASP0,
    AM_DEVICE_MCASP1,
    AM_DEVICE_MMCHS0,
    AM_DEVICE_MMCHS1,
    AM_DEVICE_MMCHS2,
    AM_DEVICE_MSTR_EXPS,
    AM_DEVICE_OCMCRAM,
    AM_DEVICE_OCPWP,
    AM_DEVICE_PCIE,
    AM_DEVICE_PKA,                  /* 50 */
    AM_DEVICE_RNG,
    AM_DEVICE_SHA0,
    AM_DEVICE_SLV_EXPS,
    AM_DEVICE_SPARE0,
    AM_DEVICE_SPARE1,
    AM_DEVICE_MCSPI0,
    AM_DEVICE_MCSPI1,
    AM_DEVICE_SPINLOCK,
    AM_DEVICE_TIMER2,
    AM_DEVICE_TIMER3,                  /* 60 */
    AM_DEVICE_TIMER4,
    AM_DEVICE_TIMER5,
    AM_DEVICE_TIMER6,
    AM_DEVICE_TIMER7,
    AM_DEVICE_TPCC,
    AM_DEVICE_TPTC0,
    AM_DEVICE_TPTC1,
    AM_DEVICE_TPTC2,
    AM_DEVICE_UART1,
    AM_DEVICE_UART2,               /* 70 */
    AM_DEVICE_UART3,
    AM_DEVICE_UART4,
    AM_DEVICE_UART5,
    AM_DEVICE_USB0,
    AM_DEVICE_BITBLT,           /* GFX */
    AM_DEVICE_GFX,
    AM_DEVICE_MMU,
    AM_DEVICE_MMUCFG,
    AM_DEVICE_MPU,              /* MPU */
    AM_DEVICE_RTC,                /* 80 */
    AM_DEVICE_CEFUSE,
    AM_DEVICE_EFUSE,   
    AM_DEVICE_COUNT,
	/* following devices are needed for pinmux */    
    /* when adding new device here, dont forget to fill the s_rgDeviceRemapTable in prcm_device.h */
	AM_DEVICE_NAND,            /* same as GPMC */
	AM_DEVICE_NOR,             /* same as GPMC */    
	AM_DEVICE_BKL,             /* GPIO */
	AM_DEVICE_FRAMER,
	AM_DEVICE_END         /* this should be the last one */
} AM33X_DEVICE_ID;

//-----------------------------------------------------------------------------
typedef enum {
    POWERDOMAIN_WKUP       = 0,
    POWERDOMAIN_PER,
    POWERDOMAIN_GFX,        
    POWERDOMAIN_MPU,
    POWERDOMAIN_RTC,    
    POWERDOMAIN_CEFUSE,
    POWERDOMAIN_EFUSE,
	POWERDOMAIN_COUNT
} PowerDomain_e;

typedef enum {
	CLKDMN_CLK_24MHZ,
	CLKDMN_CPSW_125MHZ,
	CLKDMN_GFX_L3,	
	CLKDMN_GFX_L4LS,
	CLKDMN_ICSS_OCP,
	CLKDMN_L3_AON,
	CLKDMN_L3,
	CLKDMN_L3S,
	CLKDMN_L4_CEFUSE,
	CLKDMN_L4FW,
	CLKDMN_L4HS,
	CLKDMN_L4LS,
	CLKDMN_L4_RTC,
	CLKDMN_L4_WKUP_AON,
	CLKDMN_L4_WKUP,
	CLKDMN_LCD_L3,
	CLKDMN_MPU,	
	CLKDMN_OCPWP,       /* NOT USED */
    CLKDMN_DMA_L3,      /* NOT USED */
	CLKDMN_COUNT,
    CLKDMN_NULL = 0xfff,
    CLKDMN_UNKNOWN = 0xffff
} ClockDomain_e;

typedef enum {    
	kVDD_CORE,
	kVDD_MPU,
	kVDD_RTC,
    kVDD_COUNT,
} Vdd_e;

//------------------------------------------------------------------------------

typedef enum {    
	kDPLL_CORE =0,
	kDPLL_PER,
	kDPLL_MPU,
	kDPLL_DDR,
    kDPLL_DISP,
	kDPLL_UNKNOWN,
    kDPLL_COUNT,
} Dpll_e;

//------------------------------------------------------------------------------
typedef enum {
    kDPLL_CORE_M4=0,
    kDPLL_CORE_M5,
    kDPLL_CORE_M6,
    kDPLL_PER_M2,
    kDPLL_PER_CLKDCOLDO,
    kDPLL_MPU_M2,
    kDPLL_DDR_M2,
    kDPLL_DISP_M2,	
    kCLK_32768_CK,			// [    32768]	
	kCLK_RC32K_CK,			// [    32000]
	kSYS_CLKIN_CK,			// [ 24000000]
	kTCLKIN_CK,				// [ 12000000]
//--------------------------------------------------------------
	kDPLL_CLKOUT_COUNT
} DpllClkOut_e;

//------------------------------------------------------------------------------

typedef enum {
//    clock name                parent clock
//  ----------------------  ----------------------
    /* DPLL CLKOUT CLOCKS */
    kSYSCLK1,               //kDPLL_CORE_M4
    kSYSCLK2,               //kDPLL_CORE_M5
    k192MHZ_CLK,            //kDPLL_PER_M2
    kUSB_PLL_CLK,           //kDPLL_PER_CLKDCOLDO    
    kMPU_CLK,               //kDPLL_MPU_M2
    kDDR_PLL_CLK,           //kDPLL_DDR_M2
    kDISP_DLL_CLK,          //kDPLL_DISP_M2

    /* WKUP */
    kL4_WKUP_GCLK,          //kSYSCLK1
    kL3_AON_GCLK,           //kSYSCLK1
    kL4_WKUP_AON_GCLK,      //kSYSCLK1
    kADC_FCLK,			    //kSYS_CLKIN_CK
    kDEBUG_CLKA_GCLK,       //kSYSCLK1
    kDBGSYSCLK,             //kSYS_CLKIN_CK
    kGPIO0_GDBCLK,          //kCLK_RC32K_CK or kCLK_32768_CK or kCLK_32KHZ_CK
    kI2C0_GFCLK,            //k192MHZ_CLK
    kSR_SYSCLK,             //kSYS_CLKIN_CK
    kTIMER0_GCLK,           //kCLK_RC32K_CK or kSYS_CLKIN_CK or kCLK_32KHZ_CK OR kTCLKIN_CK
    kTIMER1_GCLK,           //kCLK_RC32K_CK or kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK or kCLK_32768_CK
    kUART0_GFCLK,           //k192MHZ_CLK
    kWDT0_GCLK,             //kCLK_RC32K_CK or kCLK_32KHZ_CK 
	kWDT1_GCLK,             //kCLK_RC32K_CK or kCLK_32KHZ_CK 

    /* PER */
    kL3_GCLK,               //kSYSCLK1
    kCLK_24MHZ,             //k192MHZ_CLK
    kCLK_32KHZ_CK,			// [    32768]	
    kCPSW_125MHZ_CLK,       //kSYSCLK2
    kL4LS_GCLK,             //kSYSCLK1
    kDMA_L3_GCLK,           //kSYSCLK1
    kICSS_OCP_GCLK,         //kL3_GCLK or kDISP_DLL_CLK
    kL3S_GCLK,              //kSYSCLK1
    kL4HS_GCLK,             //kSYSCLK1
    kLCD_L3_GCLK,           //kSYSCLK1
    kCPSW_CPTS_RFT_CLK,     //kSYSCLK2 or kSYSCLK1
    kCPSW_250MHZ_CLK,       //kSYSCLK2
    kCPSW_5MHZ_CLK,         //kSYSCLK2
    kCPSW_50MHZ_CLK,        //kSYSCLK2
    kCAN_CLK,               //kSYS_CLKIN_CK
    kEMIF_GCLK,             //kDDR_PLL_CLK
    kL4FW_GCLK,             //kSYSCLK1
    kGPIO1_DBCLK,           //kCLK_32KHZ_CK
    kGPIO2_DBCLK,           //kCLK_32KHZ_CK
    kGPIO3_DBCLK,           //kCLK_32KHZ_CK
    kGPIO4_DBCLK,           //kCLK_32KHZ_CK
    kGPIO5_DBCLK,           //kCLK_32KHZ_CK
    kGPIO6_DBCLK,           //kCLK_32KHZ_CK
    kI2C_FCLK,              //k192MHZ_CLK
    kICSS_IEP_GCLK,         //kL3_GCLK
    kICSS_UART_GCLK,        //k192MHZ_CLK
    kLCDC_GLCK,             //kDISP_DLL_CLK or kSYSCLK2 or k192MHZ_CLK
    kMCASP_FCLK,            //kSYS_CLKIN_CK
    kMMC_FCLK,              //k192MHZ_CLK
    kSPI_GCLK,              //k192MHZ_CLK
    kTIMER2_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kTIMER3_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kTIMER4_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kTIMER5_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kTIMER6_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kTIMER7_GCLK,           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    kUART_GFCLK,            //k192MHZ_CLK

    /* GFX */
    kL4LS_GFX_GCLK,         //kSYSCLK1
    kGFX_L3_GCLK,           //kSYSCLK1
    kGFX_FCLK,              //k192MHZ_CLK or kSYSCLK1

    /* RTC */
    kL4_RTC_GCLK,           //kSYSCLK1
    kRTC_32KCLK,            //kCLK_32KHZ_CK

    /* CEFUSE */
    kL4_CEFUSE_GCLK,        //kSYSCLK1
    kCUST_EFUSE_SYSCLK,     //kSYS_CLKIN_CK

    /* EFUSE */
    kSTD_EFUSE_SYSCLK,      //kSYS_CLKIN_CK

    // end of clock definitions
    kSOURCE_CLOCK_COUNT,       
    kCLK_NULL,
} SourceClock_e;

#define NOCLOCK  9999

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
