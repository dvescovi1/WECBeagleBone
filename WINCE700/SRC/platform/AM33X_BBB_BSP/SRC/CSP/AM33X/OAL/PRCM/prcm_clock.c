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
//  File: prcm_clock.c
//
#include "omap.h"
#include "am33x_oal_prcm.h"
//#include "omap_prof.h"
#include "am33x.h"
//#include "omap_led.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"

static
DpllState_t _dpll_core = {
    DPLL_TYPE_ADPLLS,
    0,  
    0,
    DPLL_RAMPTIME_REFCLK_2 >> DPLL_RAMPTIME_SHIFT,
    DPLL_RAMPLEVEL_DISABLE >> DPLL_RAMPLEVEL_SHIFT,  
    0,  
    DPLL_MODE_LOWPOWER_BYPASS >> DPLL_MODE_SHIFT,  
    0,    
    0,   
    0, 
    DPLL_AUTOIDLE_DISABLED >> DPLL_AUTOIDLE_SHIFT,
    PRCM_OFS(CM_CLKMODE_DPLL_CORE),
    PRCM_OFS(CM_CLKSEL_DPLL_CORE),
    PRCM_OFS(CM_AUTOIDLE_DPLL_CORE)
};

static
DpllState_t _dpll_per = {
    DPLL_TYPE_ADPLLJ,
    0,  
    0,
    0,
    0,  
    0,  
    DPLL_MODE_LOWPOWER_BYPASS >> DPLL_MODE_SHIFT,  
    0,
    0,   
    0, 
    DPLL_AUTOIDLE_DISABLED >> DPLL_AUTOIDLE_SHIFT,
    PRCM_OFS(CM_CLKMODE_DPLL_PER),
    PRCM_OFS(CM_CLKSEL_DPLL_PERIPH),
    PRCM_OFS(CM_AUTOIDLE_DPLL_PER)
};

static
DpllState_t _dpll_mpu = {
    DPLL_TYPE_ADPLLS,
    0,  
    0,
    DPLL_RAMPTIME_REFCLK_2 >> DPLL_RAMPTIME_SHIFT,
    DPLL_RAMPLEVEL_DISABLE >> DPLL_RAMPLEVEL_SHIFT,  
    0,  
    DPLL_MODE_LOWPOWER_BYPASS >> DPLL_MODE_SHIFT,  
    DPLL_BYP_SELECT_CLKINP >> DPLL_BYP_SELECT_SHIFT,    
    0,   
    0, 
    DPLL_AUTOIDLE_DISABLED >> DPLL_AUTOIDLE_SHIFT,
    PRCM_OFS(CM_CLKMODE_DPLL_MPU),
    PRCM_OFS(CM_CLKSEL_DPLL_MPU),
    PRCM_OFS(CM_AUTOIDLE_DPLL_MPU)
};

static
DpllState_t _dpll_ddr = {
    DPLL_TYPE_ADPLLS,
    0,  
    0,
    DPLL_RAMPTIME_REFCLK_2 >> DPLL_RAMPTIME_SHIFT,
    DPLL_RAMPLEVEL_DISABLE >> DPLL_RAMPLEVEL_SHIFT,  
    0,  
    DPLL_MODE_LOWPOWER_BYPASS >> DPLL_MODE_SHIFT,  
    DPLL_BYP_SELECT_CLKINP >> DPLL_BYP_SELECT_SHIFT,    
    0,   
    0, 
    DPLL_AUTOIDLE_DISABLED >> DPLL_AUTOIDLE_SHIFT,
    PRCM_OFS(CM_CLKMODE_DPLL_DDR),
    PRCM_OFS(CM_CLKSEL_DPLL_DDR),
    PRCM_OFS(CM_AUTOIDLE_DPLL_DDR)
};

static
DpllState_t _dpll_disp = {
    DPLL_TYPE_ADPLLS,
    0,  
    0,
    DPLL_RAMPTIME_REFCLK_2 >> DPLL_RAMPTIME_SHIFT,
    DPLL_RAMPLEVEL_DISABLE >> DPLL_RAMPLEVEL_SHIFT,  
    0,  
    DPLL_MODE_LOWPOWER_BYPASS >> DPLL_MODE_SHIFT,  
    DPLL_BYP_SELECT_CLKINP >> DPLL_BYP_SELECT_SHIFT,    
    0,   
    0, 
    DPLL_AUTOIDLE_DISABLED >> DPLL_AUTOIDLE_SHIFT,
    PRCM_OFS(CM_CLKMODE_DPLL_DISP),
    PRCM_OFS(CM_CLKSEL_DPLL_DISP),
    PRCM_OFS(CM_AUTOIDLE_DPLL_DISP)
};



// initialize voltage domain ref count
VddRefCountTable s_VddTable = {
    0,    // kVDD_CORE,
    0,    // kVDD_MPU,
    0,    // kVDD_RTC,
};

//-----------------------------------------------------------------------------
// initialize dpll domain ref count
DpllMap s_DpllTable = {
	{ kVDD_CORE,   0, &_dpll_core }, // kDPLL_CORE,
	{ kVDD_CORE,   0, &_dpll_per }, // kDPLL_PER,
	{ kVDD_CORE,   0, &_dpll_mpu }, // kDPLL_MPU,
    { kVDD_CORE,   0, &_dpll_ddr }, // kDPLL_DDR,
	{ kVDD_CORE,   0, &_dpll_disp }, // kDPLL_DISP,
	{ kVDD_CORE,   0, NULL }, // kDPLL_UNKNOWN,
};

static DpllClockOutState_t  _dpll_core_m4 = {
    DPLL_CLKOUT_DIV_M4,
    DPLL_CLKOUT_PWDN_AUTO_PWDN >> DPLL_CLKOUT_PWDN_SHIFT,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M4_DPLL_CORE)
};

static DpllClockOutState_t  _dpll_core_m5 = {
    DPLL_CLKOUT_DIV_M5,
    DPLL_CLKOUT_PWDN_AUTO_PWDN >> DPLL_CLKOUT_PWDN_SHIFT,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M5_DPLL_CORE)
};

static DpllClockOutState_t  _dpll_core_m6 = {
    DPLL_CLKOUT_DIV_M6,
    DPLL_CLKOUT_PWDN_AUTO_PWDN >> DPLL_CLKOUT_PWDN_SHIFT,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M6_DPLL_CORE)
};

static DpllClockOutState_t  _dpll_per_m2 = {
    DPLL_CLKOUT_DIV_M2,
    0,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M2_DPLL_PER)
};

static DpllClockOutState_t  _dpll_per_clkdcoldo = {
    DPLL_CLKOUT_DIV_CLKDCOLDO,
    DPLL_CLKOUT_PWDN_AUTO_PWDN >> DPLL_CLKOUT_PWDN_SHIFT,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_CLKDCOLDO_DPLL_PER)
};

static DpllClockOutState_t  _dpll_mpu_m2 = {
    DPLL_CLKOUT_DIV_M2,
    0,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M2_DPLL_MPU)
};

static DpllClockOutState_t  _dpll_ddr_m2 = {
    DPLL_CLKOUT_DIV_M2,
    0,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M2_DPLL_DDR)
};

static DpllClockOutState_t  _dpll_disp_m2 = {
    DPLL_CLKOUT_DIV_M2,
    0,
    DPLL_CLKOUT_GATECTRL_AUTOGATE >> DPLL_CLKOUT_GATECTRL_SHIFT,
    0,
    PRCM_OFS(CM_DIV_M2_DPLL_DISP)
};


//-----------------------------------------------------------------------------
// initialize dpll clkout ref count
//
DpllClkOutMap s_DpllClkOutTable = {
	{ kDPLL_CORE, 0, &_dpll_core_m4},    //kDPLL_CORE_M4=0,
	{ kDPLL_CORE, 0, &_dpll_core_m5},    //kDPLL_CORE_M5,
	{ kDPLL_CORE, 0, &_dpll_core_m6},    //kDPLL_CORE_M6,
	{ kDPLL_PER, 0, &_dpll_per_m2},    //kDPLL_PER_M2,
	{ kDPLL_PER, 0, &_dpll_per_clkdcoldo},    //kDPLL_PER_CLKDCOLDO,
	{ kDPLL_MPU, 0, &_dpll_mpu_m2},    //kDPLL_MPU_M2,
	{ kDPLL_DDR, 0, &_dpll_ddr_m2},    //kDPLL_DDR_M2,
	{ kDPLL_DISP, 0, &_dpll_disp_m2},    //kDPLL_DISP_M2,	
	{ kDPLL_UNKNOWN, 0, NULL},	//kCLK_32768_CK,				
	{ kDPLL_UNKNOWN, 0, NULL},	//kCLK_RC32K_CK,				
	{ kDPLL_UNKNOWN, 0, NULL},	//kSYS_CLKIN_CK,		
	{ kDPLL_UNKNOWN, 0, NULL},	//kTCLKIN_CK,				
};


// source clocks
//divisors
SrcClockDivisorTable_t _timer2_to_7_fclk = {3, {{kTCLKIN_CK, 1}, {kSYS_CLKIN_CK, 1}, {kCLK_32KHZ_CK, 1}}};
SrcClockDivisorTable_t _mac_fclk = {1, {{kSYSCLK2, 5}}};
SrcClockDivisorTable_t _cpts_rft_fclk = {2, {{kSYSCLK2, 1}, {kSYSCLK1, 1}}};
SrcClockDivisorTable_t _timer1_fclk = {5, {{kSYS_CLKIN_CK, 1},{kCLK_32KHZ_CK, 1},{kTCLKIN_CK, 1}, {kCLK_RC32K_CK, 1}, {kCLK_32768_CK, 1}}};
SrcClockDivisorTable_t _gfx_fclk = {2, {{kSYSCLK1, 1}, {k192MHZ_CLK, 1}}};
SrcClockDivisorTable_t _icss_ocp_clk = {2, {{kL3_GCLK, 1}, {kDISP_DLL_CLK, 1}}};
SrcClockDivisorTable_t _lcd_fclk = {3, {{kDISP_DLL_CLK, 1}, {kSYSCLK2, 1}, {k192MHZ_CLK, 1}}};
SrcClockDivisorTable_t _wdt1_fclk = {2, {{kCLK_RC32K_CK, 1}, {kCLK_32KHZ_CK, 1}}};
SrcClockDivisorTable_t _gpio0_dbclk = {3, {{kCLK_RC32K_CK, 1},{kCLK_32768_CK, 1}, {kCLK_32KHZ_CK, 1}}};
SrcClockDivisorTable_t _wdt0_fclk = {2, {{kCLK_RC32K_CK, 1}, {kCLK_32KHZ_CK, 1}}};
SrcClockDivisorTable_t _timer0_fclk = {4, {{kCLK_RC32K_CK, 1},{kCLK_32KHZ_CK, 1},{kSYS_CLKIN_CK, 1},{kTCLKIN_CK, 1}}};
SrcClockDivisorTable_t _32k_clk = {1, {{kCLK_24MHZ, 0}}}; // here 0 is 732.4219 and 1 is 366.2109



SrcClockMap s_SrcClockTable = {
/* Order of the table elements MUST match the SourceClock_e list */ 	
/*   this_clk   Parent Clock      RefCount  isDPLL  	  clockdomain   Divisor   clockactivity    */
/* DPLL CLKOUT CLOCKS */
    {kSYSCLK1,          kDPLL_CORE_M4,      0,  TRUE,   CLKDMN_NULL,        NULL,               0},
    {kSYSCLK2,          kDPLL_CORE_M5,      0,  TRUE,   CLKDMN_NULL,        NULL,               0},
    {k192MHZ_CLK,       kDPLL_PER_M2,       0,  TRUE,   CLKDMN_NULL,        NULL,               0},
    {kUSB_PLL_CLK,      kDPLL_PER_CLKDCOLDO,0,  TRUE,   CLKDMN_NULL,        NULL,               0},
    {kMPU_CLK,          kDPLL_MPU_M2,       0,  TRUE,   CLKDMN_MPU,         NULL,               (1<<2)},
    {kDDR_PLL_CLK,      kDPLL_DDR_M2,       0,  TRUE,   CLKDMN_NULL,        NULL,               0},
    {kDISP_DLL_CLK,     kDPLL_DISP_M2,      0,  TRUE,   CLKDMN_NULL,        NULL,               0},

/* WKUP */
    {kL4_WKUP_GCLK,     kSYSCLK1,           0, FALSE,   CLKDMN_L4_WKUP,     NULL,               (1<<2)},
    {kL3_AON_GCLK,      kSYSCLK1,           0, FALSE,   CLKDMN_L3_AON,      NULL,               (1<<3)},
    {kL4_WKUP_AON_GCLK, kSYSCLK1,           0, FALSE,   CLKDMN_L4_WKUP_AON, NULL,               (1<<2)},
    {kADC_FCLK,         kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4_WKUP,     NULL,               (1<<14)},
    {kDEBUG_CLKA_GCLK,  kSYSCLK1,           0, FALSE,   CLKDMN_L3_AON,      NULL,               (1<<4)},
    {kDBGSYSCLK,        kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L3_AON,      NULL,               (1<<2)},
    {kGPIO0_GDBCLK,     kCLK_RC32K_CK,      0, TRUE,    CLKDMN_L4_WKUP,     &_gpio0_dbclk,      (1<<8)},
    {kI2C0_GFCLK,       k192MHZ_CLK,        0, FALSE,   CLKDMN_L4_WKUP,     NULL,               (1<<11)},
    {kSR_SYSCLK,        kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4_WKUP,     NULL,               (1<<3)},
    {kTIMER0_GCLK,      kCLK_RC32K_CK,      0, TRUE,    CLKDMN_L4_WKUP,     &_timer0_fclk,      (1<<10)},
    {kTIMER1_GCLK,      kCLK_RC32K_CK,      0, TRUE,    CLKDMN_L4_WKUP,     &_timer1_fclk,      (1<<13)},
    {kUART0_GFCLK,      k192MHZ_CLK,        0, FALSE,   CLKDMN_L4_WKUP,     NULL,               (1<<12)},
    {kWDT0_GCLK,        kCLK_RC32K_CK,      0, TRUE,    CLKDMN_L4_WKUP,     &_wdt0_fclk,        (1<<9)},
    {kWDT1_GCLK,        kCLK_RC32K_CK,      0, TRUE,    CLKDMN_L4_WKUP,     &_wdt1_fclk,        (1<<4)},

/* PER */
    {kL3_GCLK,          kSYSCLK1,           0, FALSE,   CLKDMN_L3,          NULL,               (1<<4)},
    {kCLK_24MHZ,        k192MHZ_CLK,        0, FALSE,   CLKDMN_CLK_24MHZ,   NULL,               (1<<4)},    
    {kCLK_32KHZ_CK,     kCLK_24MHZ,         0, FALSE,   CLKDMN_NULL,        &_32k_clk,           0},
    {kCPSW_125MHZ_CLK,  kSYSCLK2,           0, FALSE,   CLKDMN_CPSW_125MHZ, NULL,               (1<<4)},
    {kL4LS_GCLK,        kSYSCLK1,           0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<8)},
    {kDMA_L3_GCLK,      kSYSCLK1,           0, FALSE,   CLKDMN_DMA_L3,      NULL,                0},
    {kICSS_OCP_GCLK,    kL3_GCLK,           0, FALSE,   CLKDMN_ICSS_OCP,    &_icss_ocp_clk,     (1<<4)},
    {kL3S_GCLK,         kSYSCLK1,           0, FALSE,   CLKDMN_L3S,         NULL,               (1<<3)},
    {kL4HS_GCLK,        kSYSCLK1,           0, FALSE,   CLKDMN_L4HS,        NULL,               (1<<3)},
    {kLCD_L3_GCLK,      kSYSCLK1,           0, FALSE,   CLKDMN_LCD_L3,      NULL,               (1<<4)},
    {kCPSW_CPTS_RFT_CLK,kSYSCLK2,           0, FALSE,   CLKDMN_L3,          &_cpts_rft_fclk,    (1<<6)},
    {kCPSW_250MHZ_CLK,  kSYSCLK2,           0, FALSE,   CLKDMN_L4HS,        NULL,               (1<<4)},
    {kCPSW_5MHZ_CLK,    kSYSCLK2,           0, FALSE,   CLKDMN_L4HS,        &_mac_fclk,         (1<<6)},
    {kCPSW_50MHZ_CLK,   kSYSCLK2,           0, FALSE,   CLKDMN_L4HS,        &_mac_fclk,         (1<<5)},
    {kCAN_CLK,          kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        NULL,               (1<<11)},
    {kEMIF_GCLK,        kDDR_PLL_CLK,       0, FALSE,   CLKDMN_L3,          NULL,               (1<<2)},
    {kL4FW_GCLK,        kSYSCLK1,           0, FALSE,   CLKDMN_L4FW,        NULL,               (1<<8)},
    {kGPIO1_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<19)},
    {kGPIO2_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<20)},
    {kGPIO3_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<21)},
    {kGPIO4_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<22)},
    {kGPIO5_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<26)},
    {kGPIO6_DBCLK,      kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<18)},
    {kI2C_FCLK,         k192MHZ_CLK,        0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<24)},
    {kICSS_IEP_GCLK,    kL3_GCLK,           0, FALSE,   CLKDMN_ICSS_OCP,    NULL,               (1<<5)},
    {kICSS_UART_GCLK,   k192MHZ_CLK,        0, FALSE,   CLKDMN_ICSS_OCP,    NULL,               (1<<6)},
    {kLCDC_GLCK,        kDISP_DLL_CLK,      0, FALSE,   CLKDMN_L4LS,        &_lcd_fclk,         (1<<17)},
    {kMCASP_FCLK,       kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L3,          NULL,               (1<<7)},
    {kMMC_FCLK,         k192MHZ_CLK,        0, FALSE,   CLKDMN_L3,          NULL,               (1<<3)},
    {kSPI_GCLK,         k192MHZ_CLK,        0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<25)},
    {kTIMER2_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<14)},
    {kTIMER3_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<15)},
    {kTIMER4_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<16)},
    {kTIMER5_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<27)},
    {kTIMER6_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<28)},
    {kTIMER7_GCLK,      kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4LS,        &_timer2_to_7_fclk, (1<<13)},
    {kUART_GFCLK,       k192MHZ_CLK,        0, FALSE,   CLKDMN_L4LS,        NULL,               (1<<10)},

/* GFX */
    {kL4LS_GFX_GCLK,    kSYSCLK1,           0, FALSE,   CLKDMN_GFX_L4LS,    NULL,               (1<<8)},
    {kGFX_L3_GCLK,      kSYSCLK1,           0, FALSE,   CLKDMN_GFX_L3,      NULL,               (1<<8)},
    {kGFX_FCLK,         kSYSCLK1,           0, FALSE,   CLKDMN_GFX_L3,      &_gfx_fclk,         (1<<9)},
    
/* RTC */
    {kL4_RTC_GCLK,      kSYSCLK1,           0, FALSE,   CLKDMN_L4_RTC,      NULL,               (1<<8)},
    {kRTC_32KCLK,       kCLK_32KHZ_CK,      0, FALSE,   CLKDMN_L4_RTC,      NULL,               (1<<9)},

/* CEFUSE */
    {kL4_CEFUSE_GCLK,   kSYSCLK1,           0, FALSE,   CLKDMN_L4_CEFUSE,   NULL,               (1<<8)},
    {kCUST_EFUSE_SYSCLK,kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_L4_CEFUSE,   NULL,               (1<<9)},

/* EFUSE */
    {kSTD_EFUSE_SYSCLK, kSYS_CLKIN_CK,      0, TRUE,    CLKDMN_NULL,        NULL,               0},

//====================================================================================================
};

ClockDomainLookupEntry s_rgClkDomainLookupTable[] = {
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_CLK_24MHZ_CLKSTCTRL)	}, // CLKDMN_CLK_24MHZ, 
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_CPSW_CLKSTCTRL)		}, // CLKDMN_CPSW_125MHZ,
	{ POWERDOMAIN_GFX,          0, FALSE, PRCM_OFS(CM_GFX_L3_CLKSTCTRL)			}, // CLKDMN_GFX_L3,	
	{ POWERDOMAIN_GFX,          0, FALSE, PRCM_OFS(CM_GFX_L4LS_GFX_CLKSTCTRL)	}, // CLKDMN_GFX_L4LS,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_ICSS_CLKSTCTRL)		}, // CLKDMN_ICSS_OCP,
	{ POWERDOMAIN_WKUP,         0, FALSE, PRCM_OFS(CM_L3_AON_CLKSTCTRL)			}, // CLKDMN_L3_AON,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_L3_CLKSTCTRL)			}, // CLKDMN_L3,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_L3S_CLKSTCTRL)		}, // CLKDMN_L3S,
	{ POWERDOMAIN_CEFUSE,       0, FALSE, PRCM_OFS(CM_CEFUSE_CLKSTCTRL)			}, // CLKDMN_L4_CEFUSE,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_L4FW_CLKSTCTRL)		}, // CLKDMN_L4FW,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_L4HS_CLKSTCTRL)		}, // CLKDMN_L4HS,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_L4LS_CLKSTCTRL)       }, // CLKDMN_L4LS,
	{ POWERDOMAIN_RTC,          0, FALSE, PRCM_OFS(CM_RTC_CLKSTCTRL)			}, // CLKDMN_L4_RTC,
	{ POWERDOMAIN_WKUP,         0, FALSE, PRCM_OFS(CM_L4_WKUP_AON_CLKSTCTRL)	}, // CLKDMN_L4_WKUP_AON,
	{ POWERDOMAIN_WKUP,         0, FALSE, PRCM_OFS(CM_WKUP_CLKSTCTRL)			}, // CLKDMN_L4_WKUP,
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_LCDC_CLKSTCTRL)		}, // CLKDMN_LCD_L3,
	{ POWERDOMAIN_MPU,          0, FALSE, PRCM_OFS(CM_MPU_CLKSTCTRL)			}, // CLKDMN_MPU,	
	{ POWERDOMAIN_PER,          0, FALSE, PRCM_OFS(CM_PER_OCPWP_L3_CLKSTCTRL)   }, // CLKDMN_OCPWP,       /* NOT USED */
};																	   

BOOL _ClockHwUpdate32KOpp(UINT32 div)
{    
    if (div>1) return FALSE;
    OUTREG32(&g_pSupplDeviceCtrlRegs->CLK32KDIVRATIO_CTRL,div);
    return TRUE;
}

BOOL _ClockHwEnableClkDomain(ClockDomain_e clkDomain, BOOL bEnable)
{
	volatile unsigned int *pclkstctrl;

	if (clkDomain >= CLKDMN_COUNT || clkDomain < 0) return FALSE;

	pclkstctrl = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          s_rgClkDomainLookupTable[clkDomain].CLKSTCTRL_REG);

    OALMSG(OAL_FUNC,(L"_ClockHwEnableClkDomain: clkDomain=%d, %d, %08X\r\n",
	                                    clkDomain, bEnable, pclkstctrl));
	if (bEnable) 
        OUTREG32(pclkstctrl, CLKSTCTRL_CLKTRCTRL_SW_WKUP); 
    else
        OUTREG32(pclkstctrl, CLKSTCTRL_CLKTRCTRL_SW_SLEEP); 
    
	return TRUE;	
}

BOOL ClockEnableClkDomain(ClockDomain_e clkDomain, BOOL bEnable)
{
	if (clkDomain >= CLKDMN_COUNT || clkDomain < 0) return FALSE;

	if (bEnable == TRUE){
		if (InterlockedIncrement(&s_rgClkDomainLookupTable[clkDomain].refCount) == 1 )        
		{
			_ClockHwEnableClkDomain(clkDomain, bEnable);
		}
	} else if (s_rgClkDomainLookupTable[clkDomain].refCount > 0){
		if (InterlockedDecrement(&s_rgClkDomainLookupTable[clkDomain].refCount) == 0)
        {
			_ClockHwEnableClkDomain(clkDomain, bEnable);
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
static BOOL _ClockHwUpdateParentClock( UINT clockId )
// There are several clocks which have multiple parent clocks. The function 
// sets the HW register to connect the clock to a parent accordingly to the
// s_SrcClockTable[clockId].parentClk 
{
    BOOL rc = FALSE;
    UINT parentClock;    
    SrcClockDivisorTable_t  *pDivisors;
    UINT i=0;
    UINT divisor;
    UINT val=0;
    
    OALMSG(OAL_FUNC, (L"+_ClockHwUpdateParentClock(clockId=%d)\r\n", clockId));

    if (clockId > kSOURCE_CLOCK_COUNT) goto cleanUp;

    parentClock = s_SrcClockTable[clockId].parentClk;
    
    pDivisors = s_SrcClockTable[clockId].pDivisors;
    if (pDivisors==NULL) goto cleanUp;    
    for (i=0; i<pDivisors->count; i++)
        if (parentClock == pDivisors->SourceClock[i].id) break;
    if (i==pDivisors->count) goto cleanUp;
    divisor = pDivisors->SourceClock[i].divisor;

    switch (clockId) {
        case kTIMER1_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER1MS_CLK, (i<<CLKSEL_TIMER1MS_CLK_SHIFT)); 
            break;
        case kTIMER2_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER2_CLK, (i<<CLKSEL_TIMER2_CLK_SHIFT)); 
            break;            
        case kTIMER3_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER3_CLK, (i<<CLKSEL_TIMER3_CLK_SHIFT)); 
            break;            
        case kTIMER4_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER4_CLK, (i<<CLKSEL_TIMER4_CLK_SHIFT)); 
            break;            
        case kTIMER5_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER5_CLK, (i<<CLKSEL_TIMER5_CLK_SHIFT)); 
            break;            
        case kTIMER6_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER6_CLK, (i<<CLKSEL_TIMER6_CLK_SHIFT)); 
            break;            
        case kTIMER7_GCLK:
            OUTREG32(&g_pPrcmRegs->CLKSEL_TIMER7_CLK, (i<<CLKSEL_TIMER7_CLK_SHIFT)); 
            break;            
        case kTIMER0_GCLK: 
            {
                //TODO: [madhvi] Do we need to worry about unlocking the regs 
                UINT32 regVal = INREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL) & ~SEC_CLK_CTRL_SECTIMERCLKSEL_MASK;
                regVal |= (i<<SEC_CLK_CTRL_SECTIMERCLKSEL_SHIFT);
                OUTREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL,regVal);
            }
            break;
        case kWDT0_GCLK: 
            {                
                //TODO: [madhvi] Do we need to worry about unlocking the regs 
                UINT32 regVal = INREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL) & ~SEC_CLK_CTRL_SECWDCLKSEL_MASK;
                regVal |= (i<<SEC_CLK_CTRL_SECWDCLKSEL_SHIFT);
                OUTREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL,regVal);
            }
            break;
        case kWDT1_GCLK: 
            OUTREG32(&g_pPrcmRegs->CLKSEL_WDT1_CLK, (i<<CLKSEL_WDT1_CLK_SHIFT)); 
            break;
        case kCPSW_50MHZ_CLK: 
        case kCPSW_5MHZ_CLK:
            if (divisor > 1) goto cleanUp;
            OUTREG32(&g_pPrcmRegs->CM_MAC_CLKSEL, (divisor<<CM_MAC_CLKSEL_SHIFT)); 
            break;        
        case kGFX_FCLK: 
            if (divisor > 1) goto cleanUp;
            val = (i<<CLKSEL_GFX_FCLK_SHIFT) | (divisor<<CLKDIV_SEL_GFX_FCLK_SHIFT);
            OUTREG32(&g_pPrcmRegs->CLKSEL_GFX_FCLK, val); 
            break;            
        case kLCDC_GLCK: 
            OUTREG32(&g_pPrcmRegs->CLKSEL_LCDC_PIXEL_CLK, (i<<CLKSEL_LCDC_PIXEL_CLK_SHIFT)); 
            break;            
        case kICSS_OCP_GCLK:             
            OUTREG32(&g_pPrcmRegs->CLKSEL_ICSS_OCP_CLK, (i<<CLKSEL_ICSS_OCP_CLK_SHIFT)); 
            break;
        case kGPIO0_GDBCLK:             
            OUTREG32(&g_pPrcmRegs->CLKSEL_GPIO0_DBCLK, (i<<CLKSEL_GPIO0_DBCLK_SHIFT)); 
            break;            
        case kCPSW_CPTS_RFT_CLK:
            OUTREG32(&g_pPrcmRegs->CM_CPTS_RFT_CLKSEL,(i<<CM_CPTS_RFT_CLKSEL_SHIFT)); 
            break;             
        case kCLK_32KHZ_CK:
            if (_ClockHwUpdate32KOpp(divisor) == FALSE) goto cleanUp;
            break;
		default:
			goto cleanUp;
	}

    rc = TRUE;
            
cleanUp:    
    OALMSG(OAL_FUNC, (L"-_ClockHwUpdateParentClock()=%d\r\n", rc));
    return rc;
}

static UINT _ClockHwGetParentClock( UINT clockId )
// If clock has multiple parents (has sel register) we read the register and return  
// the corresponding parent clockId, otherwise NOCLOCK 
{
//	volatile unsigned int *reg_addr;
    UINT parentClock = NOCLOCK;
	UINT32	reg_val = 0xFFFF;
    SrcClockDivisorTable_t  *pDivisors;

    if (clockId > kSOURCE_CLOCK_COUNT) goto cleanUp;                  // clockId out of range
    pDivisors = s_SrcClockTable[clockId].pDivisors;
    if (pDivisors==NULL) goto cleanUp;

	switch (clockId){
        case kTIMER1_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER1MS_CLK) & CLKSEL_TIMER1MS_CLK_MASK) >> CLKSEL_TIMER1MS_CLK_SHIFT;
            break;
        case kTIMER2_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER2_CLK) & CLKSEL_TIMER2_CLK_MASK) >> CLKSEL_TIMER2_CLK_SHIFT;
			break;
        case kTIMER3_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER3_CLK) & CLKSEL_TIMER3_CLK_MASK) >> CLKSEL_TIMER3_CLK_SHIFT;
			break;
        case kTIMER4_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER4_CLK) & CLKSEL_TIMER4_CLK_MASK) >> CLKSEL_TIMER4_CLK_SHIFT;
			break;
        case kTIMER5_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER5_CLK) & CLKSEL_TIMER5_CLK_MASK) >> CLKSEL_TIMER5_CLK_SHIFT;
			break;
        case kTIMER6_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER6_CLK) & CLKSEL_TIMER6_CLK_MASK) >> CLKSEL_TIMER6_CLK_SHIFT;
			break;
        case kTIMER7_GCLK:
			reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_TIMER7_CLK) & CLKSEL_TIMER7_CLK_MASK) >> CLKSEL_TIMER7_CLK_SHIFT;
			break;
        case kTIMER0_GCLK: 
            reg_val = (INREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL) & SEC_CLK_CTRL_SECTIMERCLKSEL_MASK)>>SEC_CLK_CTRL_SECTIMERCLKSEL_SHIFT;
            break;
        case kWDT0_GCLK: 
            reg_val = (INREG32(&g_pSecnFuseRegs->SEC_CLK_CTRL) & SEC_CLK_CTRL_SECWDCLKSEL_MASK)>>SEC_CLK_CTRL_SECWDCLKSEL_SHIFT;
            break;
        case kWDT1_GCLK: 
            reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_WDT1_CLK)& CLKSEL_WDT1_CLK_MASK) >> CLKSEL_WDT1_CLK_SHIFT;
            break;
        case kCPSW_50MHZ_CLK: 
        case kCPSW_5MHZ_CLK:       
            pDivisors->SourceClock[0].divisor = (INREG32(&g_pPrcmRegs->CM_MAC_CLKSEL)& CM_MAC_CLKSEL_MASK) >> CM_MAC_CLKSEL_SHIFT;
            break;        
        case kGFX_FCLK: 
            reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_GFX_FCLK)& CLKSEL_GFX_FCLK_MASK) >> CLKSEL_GFX_FCLK_SHIFT;
            pDivisors->SourceClock[reg_val].divisor = (INREG32(&g_pPrcmRegs->CLKSEL_GFX_FCLK)& CLKDIV_SEL_GFX_FCLK_MASK) >> CLKDIV_SEL_GFX_FCLK_SHIFT;
            break;
        case kLCDC_GLCK: 
            reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_LCDC_PIXEL_CLK)& CLKSEL_LCDC_PIXEL_CLK_MASK) >> CLKSEL_LCDC_PIXEL_CLK_SHIFT;
            break;
        case kICSS_OCP_GCLK:             
            reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_ICSS_OCP_CLK)& CLKSEL_ICSS_OCP_CLK_MASK) >> CLKSEL_ICSS_OCP_CLK_SHIFT;
            break;
        case kGPIO0_GDBCLK:             
            reg_val = (INREG32(&g_pPrcmRegs->CLKSEL_GPIO0_DBCLK)& CLKSEL_GPIO0_DBCLK_MASK) >> CLKSEL_GPIO0_DBCLK_SHIFT;
            break;
        case kCPSW_CPTS_RFT_CLK:             
            reg_val = (INREG32(&g_pPrcmRegs->CM_CPTS_RFT_CLKSEL)& CM_CPTS_RFT_CLKSEL_MASK) >> CM_CPTS_RFT_CLKSEL_SHIFT;
            break;
        case kCLK_32KHZ_CK:       
            pDivisors->SourceClock[0].divisor = INREG32(&g_pSupplDeviceCtrlRegs->CLK32KDIVRATIO_CTRL);
            break;        
        
	}

    // all those that do not have pDivisors will not be in the case statement above - so regVal is valid */
    if (reg_val != 0xFFFF) parentClock = pDivisors->SourceClock[reg_val].id;                			

cleanUp:
	return parentClock;
}

//-----------------------------------------------------------------------------
static
BOOL
_ClockHwUpdateDpllState(
    UINT dpll,
    UINT ffMask
    )
{
    BOOL rc = TRUE;
    DpllState_t * pDpll = s_DpllTable[dpll].pDpllInfo;      
    volatile unsigned int *pCM_CLKMODE_PLL;
    volatile unsigned int *pCM_CLKSEL_PLL;
    volatile unsigned int *pCM_AUTOIDLE_PLL;    
    UINT            cm_clkmode_pll;
    UINT            cm_clksel_pll;
    UINT            cm_autoidle_pll;

    
    if (pDpll == NULL) return FALSE;

    pCM_CLKMODE_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
                                  pDpll->CLKMODE_REG);
    pCM_CLKSEL_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
                                  pDpll->CLKSEL_REG);
    pCM_AUTOIDLE_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
                                  pDpll->AUTOIDLE_REG);

    cm_clkmode_pll  = INREG32(pCM_CLKMODE_PLL);
    cm_clksel_pll  = INREG32(pCM_CLKSEL_PLL);
    cm_autoidle_pll  = INREG32(pCM_AUTOIDLE_PLL);
    
    
    // update the following hw registers
    // CM_AUTOIDLE_PLL_xxx
    // CM_CLKMODE_PLL_xxx

    if (ffMask & DPLL_UPDATE_LPMODE)
    {
        switch (dpll)
        {                
            case kDPLL_CORE:     
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_LPMODE_EN_MASK;
                cm_clkmode_pll |= pDpll->lowPowerEnabled << DPLL_LPMODE_EN_SHIFT;
            break;
        }
    }

    if (ffMask & DPLL_UPDATE_DRIFTGUARD)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_DRIFTGUARD_EN_MASK;
                cm_clkmode_pll |= pDpll->driftGuard << DPLL_DRIFTGUARD_EN_SHIFT;
            break;
        }
    }

    if (ffMask & DPLL_UPDATE_RAMPTIME)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_RAMPTIME_MASK;
                cm_clkmode_pll |= pDpll->rampTime << DPLL_RAMPTIME_SHIFT;                
            break;
        }
    }

    if (ffMask & DPLL_UPDATE_DPLLMODE)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_PER:    
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_MODE_MASK;
                cm_clkmode_pll |= pDpll->dpllMode << DPLL_MODE_SHIFT; 
            break;
        }
    }

    if (ffMask & DPLL_UPDATE_AUTOIDLEMODE)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_PER:               
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_autoidle_pll &= ~DPLL_AUTOIDLE_MASK;
                cm_autoidle_pll |= pDpll->dpllAutoidleState << DPLL_AUTOIDLE_SHIFT;
            break;
        }
    }
    
    if (ffMask & DPLL_UPDATE_RAMPLEVEL)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_RAMPLEVEL_MASK;
                cm_clkmode_pll |= pDpll->rampLevel << DPLL_RAMPLEVEL_SHIFT;                
            break;
        }
    }
    
    if (ffMask & DPLL_UPDATE_RAMPONRELOCK)
    {
        switch (dpll)
        {
            case kDPLL_CORE:
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:               
                cm_clkmode_pll &= ~DPLL_RAMP_RELOCK_MASK;
                cm_clkmode_pll |= pDpll->rampOnRelock << DPLL_RAMP_RELOCK_SHIFT;                
            break;
        }
    }

    if (ffMask & DPLL_UPDATE_BYPSELECT)
    {
        switch (dpll)
        {
            case kDPLL_MPU:               
            case kDPLL_DISP:               
            case kDPLL_DDR:  
                cm_clksel_pll &= ~DPLL_BYP_SELECT_MASK;
                cm_clksel_pll |= pDpll->bypassSelect << DPLL_BYP_SELECT_SHIFT;                
            break;
        }
    }
    
    OUTREG32(pCM_CLKMODE_PLL, cm_clkmode_pll);
    OUTREG32(pCM_AUTOIDLE_PLL, cm_autoidle_pll);
    OUTREG32(pCM_CLKSEL_PLL, cm_clksel_pll);
    
    return rc;
}


static BOOL _ClockUpdateDpllOutput(int dpllClkId, BOOL bEnable)
{
    int addRef;
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"+_ClockUpdateDpllOutput"
            L"(srcClkId=%d, bEnable=%d)\r\n", dpllClkId, bEnable));

    if ((UINT)dpllClkId > kDPLL_CLKOUT_COUNT) goto cleanUp;

    addRef = (bEnable != FALSE) ? 1 : -1;

    if ((s_DpllClkOutTable[dpllClkId].refCount == 0 && bEnable == TRUE) ||
        (s_DpllClkOutTable[dpllClkId].refCount == 1 && bEnable == FALSE))
    {
        int dpllId = s_DpllClkOutTable[dpllClkId].dpllDomain;

        // update vdd refCount
        if ((s_DpllTable[dpllId].refCount == 0 && bEnable == TRUE) ||
            (s_DpllTable[dpllId].refCount == 1 && bEnable == FALSE))
        {
            // increment vdd refCount
            int vddId = s_DpllTable[dpllId].vddDomain;            
            s_VddTable[vddId] += addRef;   
        }
        
        // incrment dpll refCount
        s_DpllTable[dpllId].refCount += addRef;
    }

    // increment dpllClkSrc refCount
    s_DpllClkOutTable[dpllClkId].refCount += addRef;    

cleanUp:    
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"-_ClockUpdateDpllOutput()=%d\r\n", TRUE));
    return TRUE;
}

//-----------------------------------------------------------------------------
void
_ClockHwUpdateDpllFrequency(
    UINT dpllId
    )
{
    UINT cm_clksel_pll;
    volatile unsigned int *clksel_reg = NULL;    
    DpllState_t * pDpll = s_DpllTable[dpllId].pDpllInfo;  
    
    if (pDpll) 
    {
        clksel_reg = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs + pDpll->CLKSEL_REG);
        cm_clksel_pll = INREG32(clksel_reg);    
        cm_clksel_pll &= ~( DPLL_MULT_MASK | DPLL_DIV_MASK);
        cm_clksel_pll |= DPLL_MULT(pDpll->multiplier) | DPLL_DIV(pDpll->divisor);
        OUTREG32(clksel_reg, cm_clksel_pll);
    }    
    return;
}
//-----------------------------------------------------------------------------

static 
BOOL 
_ClockHwUpdateDpllClkOutState(
    UINT dpllClkOut,
    UINT ffMask
)
{
    BOOL rc = FALSE;    
    volatile unsigned int *pCM_CLKOUT_PLL;
    UINT                    cm_clkout_pll;    
    DpllClockOutState_t * pDpllClkOut = s_DpllClkOutTable[dpllClkOut].pDpllClkOutInfo;      

    if (pDpllClkOut == NULL) goto cleanUp;
    
    pCM_CLKOUT_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          pDpllClkOut->CLKOUT_REG);
    cm_clkout_pll  = INREG32(pCM_CLKOUT_PLL);    
    
    OALMSG(OAL_FUNC, (L"+_ClockOutInitialize("
        L"pDpllClkOut=0x%08X, cm_clkout_pll=0x%08X)\r\n", pDpllClkOut,
        cm_clkout_pll)
        );

    if(ffMask & DPLL_CLKOUT_UPDATE_GATECTRL) 
    {
        cm_clkout_pll &= ~DPLL_CLKOUT_GATECTRL_MASK;
        cm_clkout_pll |= pDpllClkOut->autoGateEn << DPLL_CLKOUT_GATECTRL_SHIFT;
    }
    if((ffMask & DPLL_CLKOUT_UPDATE_PWDN) && (pDpllClkOut->clkOutDivType != DPLL_CLKOUT_DIV_M2))
    {
        cm_clkout_pll &= ~DPLL_CLKOUT_PWDN_MASK;
        cm_clkout_pll |= pDpllClkOut->powerDownEn << DPLL_CLKOUT_PWDN_SHIFT;
    }
    if((ffMask & DPLL_CLKOUT_UPDATE_DIV) && (pDpllClkOut->clkOutDivType != DPLL_CLKOUT_DIV_CLKDCOLDO))
    {
        if (s_DpllTable[s_DpllClkOutTable[dpllClkOut].dpllDomain].pDpllInfo->dpllType==DPLL_TYPE_ADPLLS)
        {
            cm_clkout_pll &= ~DPLL_ADPLLS_CLKOUT_DIV_MASK;
            cm_clkout_pll |= pDpllClkOut->divisor << DPLL_ADPLLS_CLKOUT_DIV_SHIFT;
        }
        else 
        {           
            cm_clkout_pll &= ~DPLL_ADPLLJ_CLKOUT_DIV_MASK;
            cm_clkout_pll |= pDpllClkOut->divisor << DPLL_ADPLLJ_CLKOUT_DIV_SHIFT;
        }
        
    }
    
    OUTREG32(pCM_CLKOUT_PLL, cm_clkout_pll);
    
    rc = TRUE;

cleanUp:    
    OALMSG(OAL_FUNC, (L"-_ClockOutInitialize()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------

static 
BOOL 
_ClockOutInitialize(
    DpllClockOutState_t    *pDpllClkOut,
    UINT                    dpllDomain
)
{
    BOOL rc = TRUE;    
    volatile unsigned int *pCM_CLKOUT_PLL;
    UINT                    cm_clkout_pll;
    
    pCM_CLKOUT_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          pDpllClkOut->CLKOUT_REG);
    cm_clkout_pll  = INREG32(pCM_CLKOUT_PLL);    
    
    OALMSG(OAL_FUNC, (L"+_ClockOutInitialize("
        L"pDpllClkOut=0x%08X, cm_clkout_pll=0x%08X)\r\n", pDpllClkOut,
        cm_clkout_pll)
        );
    
    // all values are normalized and then cached in SDRAM
    pDpllClkOut->autoGateEn = (cm_clkout_pll & DPLL_CLKOUT_GATECTRL_MASK) >> DPLL_CLKOUT_GATECTRL_SHIFT;
    if (pDpllClkOut->clkOutDivType != DPLL_CLKOUT_DIV_M2)
        pDpllClkOut->powerDownEn = (cm_clkout_pll & DPLL_CLKOUT_PWDN_MASK) >> DPLL_CLKOUT_PWDN_SHIFT;
    if (pDpllClkOut->clkOutDivType != DPLL_CLKOUT_DIV_CLKDCOLDO) {
        if (s_DpllTable[dpllDomain].pDpllInfo->dpllType == DPLL_TYPE_ADPLLS)
            pDpllClkOut->divisor = (cm_clkout_pll & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT;
        else
            pDpllClkOut->divisor = (cm_clkout_pll & DPLL_ADPLLJ_CLKOUT_DIV_MASK) >> DPLL_ADPLLJ_CLKOUT_DIV_SHIFT;
    }
    OALMSG(OAL_FUNC, (L"-_ClockOutInitialize()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
static
BOOL
_ClockInitialize(
    DpllState_t    *pDpll
    )
{
    volatile unsigned int *pCM_CLKMODE_PLL;
    volatile unsigned int *pCM_CLKSEL_PLL;
    volatile unsigned int *pCM_AUTOIDLE_PLL;    
    UINT            cm_clkmode_pll;
    UINT            cm_clksel_pll;
    UINT            cm_autoidle_pll;
    BOOL rc = TRUE;
    
    pCM_CLKMODE_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          pDpll->CLKMODE_REG);
    pCM_CLKSEL_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          pDpll->CLKSEL_REG);
    pCM_AUTOIDLE_PLL = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          pDpll->AUTOIDLE_REG);

    cm_clkmode_pll  = INREG32(pCM_CLKMODE_PLL);
    cm_clksel_pll  = INREG32(pCM_CLKSEL_PLL);
    cm_autoidle_pll  = INREG32(pCM_AUTOIDLE_PLL);

    
    OALMSG(OAL_FUNC, (L"+_ClockInitialize("
        L"pDpll=0x%08X, cm_clken_pll=0x%08X, cm_clksel_pll=0x%08X"
        L"cm_autoidle_pll=0x%08X)\r\n", pDpll,
        cm_clkmode_pll, cm_clksel_pll, cm_autoidle_pll)
        );
    
    // all values are normalized and then cached in SDRAM

    // save autoidle modes
    pDpll->dpllAutoidleState = (cm_autoidle_pll & DPLL_AUTOIDLE_MASK) >> DPLL_AUTOIDLE_SHIFT;

    // save dpll modes
    pDpll->dpllMode = (cm_clkmode_pll & DPLL_MODE_MASK) >> DPLL_MODE_SHIFT;
    if (pDpll->dpllType == DPLL_TYPE_ADPLLS)
    {
        pDpll->lowPowerEnabled = (cm_clkmode_pll & DPLL_LPMODE_EN_MASK) >> DPLL_LPMODE_EN_SHIFT;
        pDpll->driftGuard = (cm_clkmode_pll & DPLL_DRIFTGUARD_EN_MASK) >> DPLL_DRIFTGUARD_EN_SHIFT;
        pDpll->rampTime = (cm_clkmode_pll & DPLL_RAMPTIME_MASK) >> DPLL_RAMPTIME_SHIFT;
        pDpll->rampLevel = (cm_clkmode_pll & DPLL_RAMPLEVEL_MASK) >> DPLL_RAMPLEVEL_SHIFT;
        pDpll->rampOnRelock = (cm_clkmode_pll & DPLL_RAMP_RELOCK_MASK) >> DPLL_RAMP_RELOCK_SHIFT;
        pDpll->bypassSelect = (cm_clksel_pll & DPLL_BYP_SELECT_MASK) >> DPLL_BYP_SELECT_SHIFT;    
    }
    
    // frequency info
    pDpll->divisor = (cm_clksel_pll & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT;
    pDpll->multiplier = (cm_clksel_pll & DPLL_MULT_MASK) >> DPLL_MULT_SHIFT;

    OALMSG(OAL_FUNC, (L"-_ClockInitialize()=%d\r\n", rc));
    return rc;    
}


//-----------------------------------------------------------------------------
BOOL ClockInitialize()
{
    BOOL rc = TRUE;
    UINT i;
	UINT parentClkId;    
    
    OALMSG(OAL_FUNC, (L"+ClockInitialize()\r\n"));

    // initialize dpll and parent clock settings with what's current set in hw
    for (i =0; i<=kDPLL_DISP; i++)
        if (s_DpllTable[i].pDpllInfo) 
            _ClockInitialize(s_DpllTable[i].pDpllInfo);    
        
    for (i =0; i<kDPLL_CLKOUT_COUNT; i++)
        if (s_DpllClkOutTable[i].pDpllClkOutInfo) 
            _ClockOutInitialize(s_DpllClkOutTable[i].pDpllClkOutInfo,s_DpllClkOutTable[i].dpllDomain);    
 
	for (i=0; i<kSOURCE_CLOCK_COUNT; i++){
		parentClkId = _ClockHwGetParentClock(i);
		if (parentClkId == NOCLOCK)
			continue;
		s_SrcClockTable[i].parentClk = parentClkId;
	}
    OALMSG(OAL_FUNC, (L"-ClockInitialize()=%d\r\n", rc));
    return rc;    
}

//-----------------------------------------------------------------------------
// The function walks through the clock tree and updates reference counters 
BOOL ClockUpdateParentClock( int srcClkId, BOOL bEnable )
{
    BOOL rc = FALSE;
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"+ClockUpdateParentClock (srcClkId=%d, bEnable=%d)\r\n",
                srcClkId, bEnable));
    
    // quick check for valid source clock id's
    if ((UINT)srcClkId > kSOURCE_CLOCK_COUNT) goto cleanUp;
    
    // check if clock is being enabled/disabled
    if ((s_SrcClockTable[srcClkId].refCount == 0 && bEnable == TRUE) ||
        (s_SrcClockTable[srcClkId].refCount == 1 && bEnable == FALSE)){         
        if (s_SrcClockTable[srcClkId].bIsDpllSrcClk == TRUE){
            _ClockUpdateDpllOutput(s_SrcClockTable[srcClkId].parentClk, bEnable);
        } else {
            ClockUpdateParentClock(s_SrcClockTable[srcClkId].parentClk, bEnable);
        }
    }
    s_SrcClockTable[srcClkId].refCount += (bEnable != FALSE) ? 1 : -1;
    
    rc = TRUE;
    
cleanUp:    
    if (!g_bSingleThreaded)
        OALMSG(OAL_FUNC, (L"-ClockUpdateParentClock()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmClockEnableClockDomain(UINT clockId,BOOL bEnable)
{    
    BOOL rc = FALSE;
    OALMSG(OAL_FUNC, (L"+PrcmClockEnableClockDomain (clockId=%d, bEnable=%d)\r\n", 
        clockId, bEnable) );

    if (s_SrcClockTable[clockId].clockDomain != CLKDMN_NULL)
        rc = ClockEnableClkDomain(s_SrcClockTable[clockId].clockDomain,bEnable);
    
    OALMSG(OAL_FUNC, (L"-PrcmClockEnableClockDomain()=%d\r\n", rc));
    return rc;   
}

//-----------------------------------------------------------------------------
BOOL PrcmClockSetParent(UINT clockId, UINT newParentClockId )
{    
    BOOL rc = FALSE;
    UINT oldParentClockId;    
    SrcClockDivisorTable_t  *pDivisors;    
    UINT i=0;
    
    OALMSG(OAL_FUNC, (L"+PrcmClockSetParent"
        L"(clockId=%d, newParentClockId=%d)\r\n", clockId, newParentClockId));

    if (clockId > kSOURCE_CLOCK_COUNT) goto cleanUp;

    Lock(Mutex_Clock);
    oldParentClockId = s_SrcClockTable[clockId].parentClk;    
    
    //check for valid parent clock id
    pDivisors = s_SrcClockTable[clockId].pDivisors;
    if (pDivisors==NULL) goto cleanUp;
    for (i=0; i<pDivisors->count; i++)
        if (newParentClockId == pDivisors->SourceClock[i].id) break;
    if (i==pDivisors->count) goto cleanUp;

    // check if clock is enabled.  If so then release old src clocks
    // and enable new ones
    if (s_SrcClockTable[clockId].refCount > 0){
        ClockUpdateParentClock(newParentClockId, TRUE);
        ClockUpdateParentClock(oldParentClockId, FALSE);
    }

    // update state tree
    s_SrcClockTable[clockId].parentClk = newParentClockId;
    rc = _ClockHwUpdateParentClock(clockId);

    // release sync handle    
    Unlock(Mutex_Clock);
            
cleanUp:    
    OALMSG(OAL_FUNC, (L"-PrcmClockSetParent()=%d\r\n", TRUE));
    return TRUE;
}


//-----------------------------------------------------------------------------
BOOL
PrcmClockSetDivisor(
    UINT clockId,
    UINT parentClockId,
    UINT divisor
    )
{
    static BYTE _sgx_div[]           = {(BYTE)-1, (BYTE)0, (BYTE)1}; // valid values div/1 and div/2
    static BYTE _mac_div[]           = {(BYTE)-1, (BYTE)-1, (BYTE)0, (BYTE)-1, (BYTE)-1, (BYTE)1}; //valid values div/2 and div/5
    
    UINT i;
    BYTE real_divisor = 0;
    BOOL rc = FALSE;
    SrcClockDivisorTable_t *pDivisors;
    
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDivisor"
        L"(clockId=%d, parentClockId=%d, divisor=%d)\r\n", 
        clockId, parentClockId, divisor)
        );

    // get sync handle
    Lock(Mutex_Clock);

    pDivisors = s_SrcClockTable[clockId].pDivisors;
    if (pDivisors==NULL) goto cleanUp;

    switch (clockId)
    {            
        case kGFX_FCLK:
            if (divisor >= sizeof(_sgx_div) || _sgx_div[divisor] == -1) goto cleanUp;
            real_divisor = _sgx_div[divisor];
            // store divisor settings
            for (i = 0; i < pDivisors->count; ++i)
            {
                if (parentClockId == pDivisors->SourceClock[i].id)
                    {
                        pDivisors->SourceClock[i].divisor = real_divisor;
                        break;
                    }
            }
            rc = _ClockHwUpdateParentClock(clockId);
            break;  
        
        case kCPSW_50MHZ_CLK:
        case kCPSW_5MHZ_CLK:
            // validate parameters
            if (parentClockId != pDivisors->SourceClock[0].id || divisor >= sizeof(_mac_div)) goto cleanUp;
            real_divisor = _mac_div[divisor];
            // store divisor settings
            pDivisors->SourceClock[0].divisor = real_divisor;
            rc = _ClockHwUpdateParentClock(clockId);
            break;
            
        case kCLK_32KHZ_CK:
            // validate parameters
            if (parentClockId != pDivisors->SourceClock[0].id) goto cleanUp;
            // store divisor settings
            pDivisors->SourceClock[0].divisor = divisor;
            rc = _ClockHwUpdateParentClock(clockId);
        break;
    }    
    

cleanUp: 
    // release sync handle    
    Unlock(Mutex_Clock);
    OALMSG(OAL_FUNC, (L"-PrcmClockSetDivisor()=%d\r\n", rc));
    return rc;
}


//-----------------------------------------------------------------------------
BOOL PrcmClockGetParentClockRefcount(UINT clockId, UINT nLevel, LONG *pRefCount )
{
    BOOL rc = FALSE;
    OALMSG(OAL_FUNC, (L"+PrcmClockGetParentClockRefcount (clockId=%d, nLevel=%d)\r\n", clockId, nLevel));

    switch (nLevel){
        case 1:
            if (clockId < kSOURCE_CLOCK_COUNT){
                *pRefCount = s_SrcClockTable[clockId].refCount;
                rc = TRUE;
            }
            break;

        case 2:
            if (clockId < kDPLL_CLKOUT_COUNT){
                *pRefCount = s_DpllClkOutTable[clockId].refCount;
                rc = TRUE;
            }
            break;

        case 3:
            if (clockId < kDPLL_COUNT){
                *pRefCount = s_DpllTable[clockId].refCount;
                rc = TRUE;
            }
            break;

        case 4:
            if (clockId < kVDD_COUNT){
                *pRefCount = s_VddTable[clockId];
                rc = TRUE;
            }
            break;
        }

    OALMSG(OAL_FUNC, (L"+PrcmClockGetParentClockRefcount()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmClockGetParentClockInfo(UINT clockId, UINT nLevel, SourceClockInfo_t *pInfo)
{
    BOOL rc = FALSE;
    UINT parentClock;
    OALMSG(OAL_FUNC, (L"+PrcmClockGetParentClockInfo"
        L"(clockId=%d, nLevel=%d, pInfo=0x%08X)\r\n", clockId, nLevel, pInfo));

    // get parent clock
    switch (nLevel) {
        case 1:            
            if (clockId >= kSOURCE_CLOCK_COUNT)
				goto cleanUp;
            parentClock = s_SrcClockTable[clockId].parentClk;
            if (s_SrcClockTable[clockId].bIsDpllSrcClk)
				++nLevel;                    
            break;

        case 2:
            if (clockId >= kDPLL_CLKOUT_COUNT)
				goto cleanUp;
            parentClock = s_DpllClkOutTable[clockId].dpllDomain;
            ++nLevel;                    
            break;

        case 3:
            if (clockId >= kDPLL_COUNT)
                goto cleanUp;
            parentClock = s_DpllTable[clockId].vddDomain;
            ++nLevel;                    
            break;

        default:
            goto cleanUp;
        }


    // get parent information
    pInfo->nLevel = nLevel;
    pInfo->clockId = parentClock;
    rc = PrcmClockGetParentClockRefcount(parentClock, nLevel, &pInfo->refCount);

cleanUp:    
    OALMSG(OAL_FUNC, (L"+PrcmClockGetParentClockInfo()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmClockSetDpllState(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN *pInfo )
{
    BOOL rc = FALSE;
    DpllState_t *pDpll;
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllState(pInfo=0x%08X)\r\n", pInfo));

    Lock(Mutex_DeviceClock);
    if (pInfo->dpllId > kDPLL_DISP) goto cleanUp;

    // get dpll pointer
    pDpll = s_DpllTable[pInfo->dpllId].pDpllInfo;
    if (pDpll == NULL) goto cleanUp;

    if ((pInfo->ffMask & DPLL_UPDATE_LPMODE) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->lowPowerEnabled = pInfo->lowPowerEnabled ? 0x01 : 0;
        }

    if ((pInfo->ffMask & DPLL_UPDATE_DRIFTGUARD) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->driftGuard = pInfo->driftGuardEnabled ? 0x01 : 0;    
        }

    if ((pInfo->ffMask & DPLL_UPDATE_RAMPTIME) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->rampTime = (pInfo->rampTime & DPLL_RAMPTIME_MASK) >> DPLL_RAMPTIME_SHIFT;               
        }

    if (pInfo->ffMask & DPLL_UPDATE_DPLLMODE)
        {
        pDpll->dpllMode = (pInfo->dpllMode & DPLL_MODE_MASK) >> DPLL_MODE_SHIFT;
        }

    if (pInfo->ffMask & DPLL_UPDATE_AUTOIDLEMODE)
        {
        pDpll->dpllAutoidleState = (pInfo->dpllAutoidleState & DPLL_AUTOIDLE_MASK) >> DPLL_AUTOIDLE_SHIFT;
        }

    if ((pInfo->ffMask & DPLL_UPDATE_RAMPLEVEL) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->rampLevel = (pInfo->rampLevel & DPLL_RAMPLEVEL_MASK) >> DPLL_RAMPLEVEL_SHIFT;               
        }

    if ((pInfo->ffMask & DPLL_UPDATE_RAMPONRELOCK) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->rampOnRelock = pInfo->rampOnRelock ? 0x1 : 0;               
        }

    if ((pInfo->ffMask & DPLL_UPDATE_BYPSELECT) && (pDpll->dpllType == DPLL_TYPE_ADPLLS))
        {
        pDpll->bypassSelect = (pInfo->bypassSelect & DPLL_BYP_SELECT_MASK) >> DPLL_BYP_SELECT_SHIFT;               
        }

    
    rc = _ClockHwUpdateDpllState(pInfo->dpllId, pInfo->ffMask);
    

cleanUp:    
    Unlock(Mutex_DeviceClock);
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllState()=%d\r\n", rc));
    return rc;
}


//-----------------------------------------------------------------------------
BOOL PrcmClockSetDpllFrequency(UINT dpllId, UINT m,
    UINT n, UINT freqSel, UINT outputDivisor  )
{
    BOOL rc = FALSE;
    DpllState_t *pDpll;
    
    UNREFERENCED_PARAMETER(freqSel);
    UNREFERENCED_PARAMETER(outputDivisor);
    
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllFrequency"
        L"(dpllId=%d, m=%d, n=%d, freqSel=%d)\r\n", 
        dpllId, m, n, freqSel
        ));

    Lock(Mutex_DeviceClock);
    if (dpllId > kDPLL_DISP) goto cleanUp;
    pDpll = s_DpllTable[dpllId].pDpllInfo;

    // check if parameters are already set
    if (pDpll->divisor != n             ||
        pDpll->multiplier != m          )
    {
        // frequency info
        pDpll->divisor = n;
        pDpll->multiplier = m;
    
        _ClockHwUpdateDpllFrequency(dpllId);
    }

    rc = TRUE;
    
cleanUp:    
    Unlock(Mutex_DeviceClock);
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllFrequency()=%d\r\n", rc));
    return rc;
}    

//-----------------------------------------------------------------------------
BOOL PrcmClockSetDpllAutoIdleState( UINT dpllId, UINT dpllAutoidleState )
{
    BOOL rc = FALSE;
    DpllState_t *pDpll;
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllAutoIdleState"
        L"(dpllId=%d, dpllAutoidleState=0x%08X)\r\n", 
        dpllId, dpllAutoidleState)
        );

    if (dpllId > kDPLL_DISP) goto cleanUp;
    
    Lock(Mutex_DeviceClock);
    pDpll = s_DpllTable[dpllId].pDpllInfo;
    pDpll->dpllAutoidleState = (dpllAutoidleState & DPLL_AUTOIDLE_MASK) >> DPLL_AUTOIDLE_SHIFT;
    rc = _ClockHwUpdateDpllState(dpllId, DPLL_UPDATE_AUTOIDLEMODE);    
    Unlock(Mutex_DeviceClock);
    
cleanUp: 
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllAutoIdleState()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmClockSetDpllClkOutState(IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE_IN *pInfo )
{
    BOOL rc = FALSE;
    DpllClockOutState_t *pDpllClkOut;
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllClkOutState(pInfo=0x%08X)\r\n", pInfo));

    Lock(Mutex_DeviceClock);
    if (pInfo->dpllClkOutId > kDPLL_CLKOUT_COUNT) goto cleanUp;

    // get dpll pointer
    pDpllClkOut = s_DpllClkOutTable[pInfo->dpllClkOutId].pDpllClkOutInfo;
    if (pDpllClkOut == NULL) goto cleanUp;

    if (pInfo->ffMask & DPLL_CLKOUT_UPDATE_GATECTRL)
    {
        pDpllClkOut->autoGateEn = pInfo->autoGateEn ? 0x01 : 0;
    }

    if (pInfo->ffMask & DPLL_CLKOUT_UPDATE_PWDN)
    {
        pDpllClkOut->powerDownEn = pInfo->pwdnEn ? 0x01 : 0;    
    }

    if (pInfo->ffMask & DPLL_CLKOUT_UPDATE_DIV)
    {
        pDpllClkOut->divisor = pInfo->divisor;               
    }

    rc = _ClockHwUpdateDpllClkOutState(pInfo->dpllClkOutId, pInfo->ffMask);
    

cleanUp:    
    Unlock(Mutex_DeviceClock);
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllClkOutState()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmClockSetDpllClkOutDivisor(UINT dpllClkOutId, UINT div)
{
    BOOL rc = FALSE;
    DpllClockOutState_t *pDpllClkOut;
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllClkOutDivisor(dpllClkOutId=%d, div=%d)\r\n", dpllClkOutId,div));

    Lock(Mutex_DeviceClock);
    if (dpllClkOutId > kDPLL_CLKOUT_COUNT) goto cleanUp;

    // get dpll clk out pointer
    pDpllClkOut = s_DpllClkOutTable[dpllClkOutId].pDpllClkOutInfo;
    if (pDpllClkOut == NULL) goto cleanUp;
    
    if (pDpllClkOut->divisor != div) {
        pDpllClkOut->divisor = div;                   
        rc = _ClockHwUpdateDpllClkOutState(dpllClkOutId, DPLL_CLKOUT_UPDATE_DIV);
    }    

cleanUp:    
    Unlock(Mutex_DeviceClock);
    OALMSG(OAL_FUNC, (L"+PrcmClockSetDpllClkOutDivisor()=%d\r\n", rc));
    return rc;
}    


//------------------------------------------------------------------------------
BOOL PrcmClockSetSystemClockSetupTime( USHORT  setupTime )
{    
    UNREFERENCED_PARAMETER(setupTime);

    // acquire sync handle   
 	// Lock(Mutex_Clock);

    // release sync handle    
    // Unlock(Mutex_Clock);
    
    return TRUE;
}

//-----------------------------------------------------------------------------
float PrcmClockGetSystemClockFrequency()
{
    return 24.0f * 1000000.0f;
}

UINT32 PrcmClockGetClockRate(OMAP_CLOCKID clock_id)
{
    float freq=0;
    float sys_clk = PrcmClockGetSystemClockFrequency()/1000000.0f;
    DWORD val;
    DWORD val2;
    DWORD freqSel;
    
    switch(clock_id)
    {
        case SYS_CLK:
		freq = sys_clk;
		break;
		
	    case MPU_CLK:                        
            val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_MPU);
            val2 = INREG32(&g_pPrcmRegs->CM_DIV_M2_DPLL_MPU);
            // fdpll/2
            freq = ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
            // fdll/(2*M2)
	        freq = freq / ((float)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));            
        break;
        
	    case SGX_CLK:
            freqSel = (INREG32(&g_pPrcmRegs->CLKSEL_GFX_FCLK)&&CLKSEL_GFX_FCLK_MASK)>>CLKSEL_GFX_FCLK_SHIFT;
            switch (freqSel)
            {
                case 0:
                case 1:
                    val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_CORE);
                    val2 = INREG32(&g_pPrcmRegs->CM_DIV_M4_DPLL_CORE);
                    //fdpll
                    freq = 2 * ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
                    //fdpll/(M4)
        	        freq = freq / ((float)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));                    
                    // opt: fdpll/(2*(M4))
                    if (freqSel == 1) freq = freq/2;
                break;

                case 2:
                case 3:
                    val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_PERIPH);
                    val2 = INREG32(&g_pPrcmRegs->CM_DIV_M2_DPLL_PER);
                    // fdpll
                    freq = ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
                    //fdpll/M2
        	        freq = freq / ((float)((val2 & DPLL_ADPLLJ_CLKOUT_DIV_MASK) >> DPLL_ADPLLJ_CLKOUT_DIV_SHIFT) + 1.0f);                    
                    //opt: fdpll/(2*M2)
                    if (freqSel == 3) freq = freq/2;
                break;
            }
        break;
        
	    case LCD_PCLK:
            freqSel = (INREG32(&g_pPrcmRegs->CLKSEL_LCDC_PIXEL_CLK)&&CLKSEL_LCDC_PIXEL_CLK_MASK)>>CLKSEL_LCDC_PIXEL_CLK_SHIFT;
            switch (freqSel)
            {
                case 0: //disp DPLL M2
                    val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_DISP);
                    val2 = INREG32(&g_pPrcmRegs->CM_DIV_M2_DPLL_DISP);
                    // fdpll/2
                    freq = ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
                    // fdll/(2*M2)
        	        freq = freq / ((float)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));            
                break;
                
                case 1: //CORE DPLL M5->SYSCLK2                    
                    val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_CORE);
                    val2 = INREG32(&g_pPrcmRegs->CM_DIV_M5_DPLL_CORE);
                    //fdpll
                    freq = 2 * ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
                    //fdpll/(M5)
        	        freq = freq / ((float)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));                    
                break;
                
                case 2: //PER DPLL M2 -> 192MHz                    
                    val = INREG32(&g_pPrcmRegs->CM_CLKSEL_DPLL_PERIPH);
                    val2 = INREG32(&g_pPrcmRegs->CM_DIV_M2_DPLL_PER);
                    // fdpll
                    freq = ((float)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        	                    ((float)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1.0f);
                    //fdpll/M2
        	        freq = freq / ((float)((val2 & DPLL_ADPLLJ_CLKOUT_DIV_MASK) >> DPLL_ADPLLJ_CLKOUT_DIV_SHIFT) + 1.0f);                    
                break;
            }
        break;

        default:
            freq = 0;
        break;
            
    }
		
    OALMSG(OAL_FUNC, (L"+PrcmClockGetClockRate(clock_id=%d)\r\n", clock_id));
    return (UINT32)freq;
}

PTCHAR ClockNames[] = {
    L"SYSCLK1",               //kDPLL_CORE_M4
    L"SYSCLK2",               //kDPLL_CORE_M5
    L"192MHZ_CLK",            //kDPLL_PER_M2
    L"USB_PLL_CLK",           //kDPLL_PER_CLKDCOLDO    
    L"MPU_CLK",               //kDPLL_MPU_M2
    L"DDR_PLL_CLK",           //kDPLL_DDR_M2
    L"DISP_DLL_CLK",          //kDPLL_DISP_M2
    L"L4_WKUP_GCLK",          //kSYSCLK1
    L"L3_AON_GCLK",           //kSYSCLK1
    L"L4_WKUP_AON_GCLK",      //kSYSCLK1
    L"ADC_FCLK",			    //kSYS_CLKIN_CK
    L"DEBUG_CLKA_GCLK",       //kSYSCLK1
    L"DBGSYSCLK",             //kSYS_CLKIN_CK
    L"GPIO0_GDBCLK",          //kCLK_RC32K_CK or kCLK_32768_CK or kCLK_32KHZ_CK
    L"I2C0_GFCLK",            //k192MHZ_CLK
    L"SR_SYSCLK",             //kSYS_CLKIN_CK
    L"TIMER0_GCLK",           //kCLK_RC32K_CK or kSYS_CLKIN_CK or kCLK_32KHZ_CK OR kTCLKIN_CK
    L"TIMER1_GCLK",           //kCLK_RC32K_CK or kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK or kCLK_32768_CK
    L"UART0_GFCLK",           //k192MHZ_CLK
    L"WDT0_GCLK",             //kCLK_RC32K_CK or kCLK_32KHZ_CK 
	L"WDT1_GCLK",             //kCLK_RC32K_CK or kCLK_32KHZ_CK 
    L"L3_GCLK",               //kSYSCLK1
    L"CLK_24MHZ",             //k192MHZ_CLK
    L"CPSW_125MHZ_CLK",       //kSYSCLK2
    L"L4LS_GCLK",             //kSYSCLK1
    L"DMA_L3_GCLK",           //kSYSCLK1
    L"ICSS_OCP_GCLK",         //kL3_GCLK or kDISP_DLL_CLK
    L"L3S_GCLK",              //kSYSCLK1
    L"L4HS_GCLK",             //kSYSCLK1
    L"LCD_L3_GCLK",           //kSYSCLK1
    L"CPSW_CPTS_RFT_CLK",     //kSYSCLK2 or kSYSCLK1
    L"CPSW_250MHZ_CLK",       //kSYSCLK2
    L"CPSW_5MHZ_CLK",         //kSYSCLK2
    L"CPSW_50MHZ_CLK",        //kSYSCLK2
    L"CAN_CLK",               //kSYS_CLKIN_CK
    L"EMIF_GCLK",             //kDDR_PLL_CLK
    L"L4FW_GCLK",             //kSYSCLK1
    L"GPIO1_DBCLK",           //kCLK_32KHZ_CK
    L"GPIO2_DBCLK",           //kCLK_32KHZ_CK
    L"GPIO3_DBCLK",           //kCLK_32KHZ_CK
    L"GPIO4_DBCLK",           //kCLK_32KHZ_CK
    L"GPIO5_DBCLK",           //kCLK_32KHZ_CK
    L"GPIO6_DBCLK",           //kCLK_32KHZ_CK
    L"I2C_FCLK",              //k192MHZ_CLK
    L"ICSS_IEP_GCLK",         //kL3_GCLK
    L"ICSS_UART_GCLK",        //k192MHZ_CLK
    L"LCDC_GLCK",             //kDISP_DLL_CLK or kSYSCLK2 or k192MHZ_CLK
    L"MCASP_FCLK",            //kSYS_CLKIN_CK
    L"MMC_FCLK",              //k192MHZ_CLK
    L"SPI_GCLK",              //k192MHZ_CLK
    L"TIMER2_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"TIMER3_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"TIMER4_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"TIMER5_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"TIMER6_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"TIMER7_GCLK",           //kSYS_CLKIN_CK or kCLK_32KHZ_CK or kTCLKIN_CK 
    L"UART_GFCLK",            //k192MHZ_CLK
    L"L4LS_GFX_GCLK",         //kSYSCLK1
    L"GFX_L3_GCLK",           //kSYSCLK1
    L"GFX_FCLK",              //k192MHZ_CLK or kSYSCLK1
    L"L4_RTC_GCLK",           //kSYSCLK1
    L"RTC_32KCLK",            //kCLK_32KHZ_CK
    L"L4_CEFUSE_GCLK",        //kSYSCLK1
    L"CUST_EFUSE_SYSCLK",     //kSYS_CLKIN_CK
    L"STD_EFUSE_SYSCLK",      //kSYS_CLKIN_CK
};

PTCHAR ClockDmnNames[] = {
	L"CLKDMN_CLK_24MHZ",
	L"CLKDMN_CPSW_125MHZ",
	L"CLKDMN_GFX_L3",	
	L"CLKDMN_GFX_L4LS",
	L"CLKDMN_ICSS_OCP",
	L"CLKDMN_L3_AON",
	L"CLKDMN_L3",
	L"CLKDMN_L3S",
	L"CLKDMN_L4_CEFUSE",
	L"CLKDMN_L4FW",
	L"CLKDMN_L4HS",
	L"CLKDMN_L4LS",
	L"CLKDMN_L4_RTC",
	L"CLKDMN_L4_WKUP_AON",
	L"CLKDMN_L4_WKUP",
	L"CLKDMN_LCD_L3",
	L"CLKDMN_MPU",	
	L"CLKDMN_OCPWP",       /* NOT USED */
    L"CLKDMN_DMA_L3",      /* NOT USED */
};

PTCHAR ClkDomainCtrlStr[] = {
    L"NO_SLEEP",
    L"SW_SLEEP",
    L"SW_WKUP ",
    L"N/A     ",
};
void DumpClockStatus(UINT clock_id)
{

    UINT32 val = 0;
    UINT32 clkAct;
    UINT32 dmnCtrl;
    UINT32 clkdmn;

    if (clock_id >= kSOURCE_CLOCK_COUNT) return;
    clkdmn = s_SrcClockTable[clock_id].clockDomain;
    if (clkdmn >= CLKDMN_COUNT) return;
    
	val = INREG32((volatile unsigned int*)(volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          s_rgClkDomainLookupTable[clkdmn].CLKSTCTRL_REG));
    clkAct = (val&s_SrcClockTable[clock_id].clkActMask);    
    dmnCtrl = (val&CLKSTCTRL_CLKTRCTRL_MASK)>>CLKSTCTRL_CLKTRCTRL_SHIFT;
    
    
    RETAILMSG(1,(L"CLOCK:\t%s(%d)\tState:%s\tRefCount:%d\r\n",
        ClockNames[clock_id],
        clock_id,
        (clkAct==0x0)? L"GATED":L"ACTIVE",
        s_SrcClockTable[clock_id].refCount));
    RETAILMSG(1,(L"CLOCK DOMAIN:\t%s(%d)\tState:%s\tRefCount:%d\r\n",
                ClockDmnNames[clkdmn],
                clkdmn,
                ClkDomainCtrlStr[dmnCtrl],
                s_rgClkDomainLookupTable[clkdmn].refCount));
    
}


void DumpClockDomainStatus(ClockDomain_e clkdmn)
{
    UINT32 val = 0;
    UINT32 dmnCtrl;

    if (clkdmn >= CLKDMN_COUNT) return;
    
	val = INREG32((volatile unsigned int*)(volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
		                          s_rgClkDomainLookupTable[clkdmn].CLKSTCTRL_REG));
    dmnCtrl = (val&CLKSTCTRL_CLKTRCTRL_MASK)>>CLKSTCTRL_CLKTRCTRL_SHIFT;

    RETAILMSG(1,(L"CLOCK DOMAIN:\t%s(%d)\tState:%s\tRefCount:%d\r\n",
                    ClockDmnNames[clkdmn],
                    clkdmn,
                    ClkDomainCtrlStr[dmnCtrl],
                    s_rgClkDomainLookupTable[clkdmn].refCount));

    
}


void DumpAllClockStatus()
{
    int i=0;
    for(i=0;i<kSOURCE_CLOCK_COUNT;i++) {
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));    
        DumpClockStatus(i);
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));
    }
}

void DumpAllClockDomainStatus()
{
    int i=0;
    for(i=0;i<CLKDMN_COUNT;i++) {
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));    
        DumpClockDomainStatus(i);
        RETAILMSG(1,(L"-----------------------------------------------------------------\r\n"));
    }
}

