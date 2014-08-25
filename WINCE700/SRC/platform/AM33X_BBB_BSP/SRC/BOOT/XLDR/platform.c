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
//
//  File:  platform.c
//
//  This file contains X-Loader configuration code for AM33X
//
#include "bsp.h"
#include "bsp_cfg.h"
#include "bsp_def.h"

#include "sdk_i2c.h"
#include "sdk_padcfg.h"
#include "oal_padcfg.h"
#include "oal_i2c.h"
#include "oal_clock.h"

#include "bsp_padcfg.h"
#include "omap_cpuver.h"

#include "am33x_config.h"
#include "am33x_base_regs.h"
#include "am33x_gpmc.h"
#include "am33x_clocks.h"
#include "am33x_prcm.h"
#include "am33x_dmm.h"

#include "tps65217.h"

extern BOOL detect_baseboard_id_info();
extern DEVICE_IFC_GPIO Am3xx_Gpio;

/****************************************************************/

/* Module Offsets */
#define CM_PER					(AM33X_PRCM_REGS_PA + 0x0)
#define CM_WKUP					(AM33X_PRCM_REGS_PA + 0x400)
#define CM_DPLL					(AM33X_PRCM_REGS_PA + 0x500)
#define CM_DEVICE				(AM33X_PRCM_REGS_PA + 0x0700)
#define CM_CEFUSE				(AM33X_PRCM_REGS_PA + 0x0A00)
#define PRM_DEVICE				(AM33X_PRCM_REGS_PA + 0x0F00)

/* Register Offsets */
/* Core PLL ADPLLS */
#define CM_CLKSEL_DPLL_CORE		(CM_WKUP + 0x68)
#define CM_CLKMODE_DPLL_CORE	(CM_WKUP + 0x90)

/* Core HSDIV */
#define CM_DIV_M4_DPLL_CORE		(CM_WKUP + 0x80)
#define CM_DIV_M5_DPLL_CORE		(CM_WKUP + 0x84)
#define CM_DIV_M6_DPLL_CORE		(CM_WKUP + 0xD8)
#define CM_IDLEST_DPLL_CORE		(CM_WKUP + 0x5c)

/* Peripheral PLL */
#define CM_CLKSEL_DPLL_PER		(CM_WKUP + 0x9c)
#define CM_CLKMODE_DPLL_PER		(CM_WKUP + 0x8c)
#define CM_DIV_M2_DPLL_PER		(CM_WKUP + 0xAC)
#define CM_IDLEST_DPLL_PER		(CM_WKUP + 0x70)

/* Display PLL */
#define CM_CLKSEL_DPLL_DISP		(CM_WKUP + 0x54)
#define CM_CLKMODE_DPLL_DISP	(CM_WKUP + 0x98)
#define CM_DIV_M2_DPLL_DISP		(CM_WKUP + 0xA4)
#define CM_IDLEST_DPLL_DISP		(CM_WKUP + 0x48)

/* DDR PLL */
#define CM_CLKSEL_DPLL_DDR		(CM_WKUP + 0x40)
#define CM_CLKMODE_DPLL_DDR		(CM_WKUP + 0x94)
#define CM_DIV_M2_DPLL_DDR		(CM_WKUP + 0xA0)
#define CM_IDLEST_DPLL_DDR		(CM_WKUP + 0x34)

/* MPU PLL */
#define CM_CLKSEL_DPLL_MPU		(CM_WKUP + 0x2c)
#define CM_CLKMODE_DPLL_MPU		(CM_WKUP + 0x88)
#define CM_DIV_M2_DPLL_MPU		(CM_WKUP + 0xA8)
#define CM_IDLEST_DPLL_MPU		(CM_WKUP + 0x20)

/* Domain Wake UP */
#define CM_WKUP_CLKSTCTRL		(CM_WKUP + 0)	/* UART0 */
#define CM_PER_L4LS_CLKSTCTRL	(CM_PER + 0x0)	/* TIMER2 */
#define CM_PER_L3_CLKSTCTRL		(CM_PER + 0x0c)	/* EMIF */
#define CM_PER_L4FW_CLKSTCTRL	(CM_PER + 0x08)	/* EMIF FW */
#define CM_PER_L3S_CLKSTCTRL	(CM_PER + 0x4)
#define CM_PER_L4HS_CLKSTCTRL	(CM_PER + 0x011c)
#define CM_CEFUSE_CLKSTCTRL		(CM_CEFUSE + 0x0)

#define PRCM_FORCE_WAKEUP		0x2

#define PRCM_EMIF_CLK_ACTIVITY	(0x1 << 2)
#define PRCM_L3_GCLK_ACTIVITY	(0x1 << 4)

#define PLL_BYPASS_MODE			0x4
#define PLL_LOCK_MODE			0x7
#define PLL_MULTIPLIER_SHIFT	8

#define LCDC_PIXEL_CLK_DISP_CLKOUTM2    0x0
#define LCDC_PIXEL_CLK_CORE_CLKOUTM5    0x1
#define LCDC_PIXEL_CLK_PER_CLKOUTM2     0x2

#define CM_CLKSEL_LCDC_PIXEL_CLK        (CM_PER + 0x34)


/*****************************************************************/

#define VTP0_CTRL_REG					0x44E10E0C
#define VTP1_CTRL_REG					0x48140E10

#define EMIF_MOD_ID_REV					(AM33X_EMIF4_0_CFG_REGS_PA + 0x00)
#define EMIF4_0_SDRAM_STATUS            (AM33X_EMIF4_0_CFG_REGS_PA + 0x04)
#define EMIF4_0_SDRAM_CONFIG            (AM33X_EMIF4_0_CFG_REGS_PA + 0x08)
#define EMIF4_0_SDRAM_CONFIG2           (AM33X_EMIF4_0_CFG_REGS_PA + 0x0C)
#define EMIF4_0_SDRAM_REF_CTRL          (AM33X_EMIF4_0_CFG_REGS_PA + 0x10)
#define EMIF4_0_SDRAM_REF_CTRL_SHADOW   (AM33X_EMIF4_0_CFG_REGS_PA + 0x14)
#define EMIF4_0_SDRAM_TIM_1             (AM33X_EMIF4_0_CFG_REGS_PA + 0x18)
#define EMIF4_0_SDRAM_TIM_1_SHADOW      (AM33X_EMIF4_0_CFG_REGS_PA + 0x1C)
#define EMIF4_0_SDRAM_TIM_2             (AM33X_EMIF4_0_CFG_REGS_PA + 0x20)
#define EMIF4_0_SDRAM_TIM_2_SHADOW      (AM33X_EMIF4_0_CFG_REGS_PA + 0x24)
#define EMIF4_0_SDRAM_TIM_3             (AM33X_EMIF4_0_CFG_REGS_PA + 0x28)
#define EMIF4_0_SDRAM_TIM_3_SHADOW      (AM33X_EMIF4_0_CFG_REGS_PA + 0x2C)
#define EMIF4_0_SDRAM_MGMT_CTRL         (AM33X_EMIF4_0_CFG_REGS_PA + 0x38)
#define EMIF4_0_SDRAM_MGMT_CTRL_SHD     (AM33X_EMIF4_0_CFG_REGS_PA + 0x3C)
#define EMIF4_0_SDRAM_ZQ_CONFIG         (AM33X_EMIF4_0_CFG_REGS_PA + 0xC8)
#define EMIF4_0_DDR_PHY_CTRL_1          (AM33X_EMIF4_0_CFG_REGS_PA + 0xE4)
#define EMIF4_0_DDR_PHY_CTRL_1_SHADOW   (AM33X_EMIF4_0_CFG_REGS_PA + 0xE8)
#define EMIF4_0_DDR_PHY_CTRL_2          (AM33X_EMIF4_0_CFG_REGS_PA + 0xEC)
#define EMIF4_0_IODFT_TLGC              (AM33X_EMIF4_0_CFG_REGS_PA + 0x60)


/****************************************************************************/

//------------------------------------------------------------------------------
//  Function Prototypes
//


//-----------------------------------------
//
// Function: OALGetTickCount
//
// Stub routine
//
UINT32 OALGetTickCount()
{
    return 1;
}

static VOID xdelay(UINT32 loops)
{
    OALStall(loops);
    return;
}


/****************************************************************************/

/* MAIN PLL Fdll = 1 GHZ, */
#define MPUPLL_N	23	/* (n -1 ) */
#define MPUPLL_M2	1

/* Core PLL Fdll = 1 GHZ, */
#define COREPLL_M      1000     /* 125 * n */
#define COREPLL_N      23       /* (n -1 ) */

#define COREPLL_M4     10      /* CORE_CLKOUTM4 = 200 MHZ */
#define COREPLL_M5     8       /* CORE_CLKOUTM5 = 250 MHZ */
#define COREPLL_M6     4       /* CORE_CLKOUTM6 = 500 MHZ */

/*
 * USB PHY clock is 960 MHZ. Since, this comes directly from Fdll, Fdll
 * frequency needs to be set to 960 MHZ. Hence,
 * For clkout = 192 MHZ, Fdll = 960 MHZ, divider values are given below
 */
#define PERPLL_M		960      /* M = 40 * (N + 1) */
#define PERPLL_N		23
#define PERPLL_M2		5

/* DDR Freq */
/* Set Fdll = 400 MHZ , Fdll = M * 2 * CLKINP/ N + 1; clkout = Fdll /(2 * M2) */
#define DDRPLL_N		23
#define DDRPLL_M2		1

/* DISP Freq is 300 MHZ */
/* Set Fdll = 600 MHZ , Fdll = M * 2 * CLKINP/ N + 1; clkout = Fdll /(2 * M2) */
#define DISPPLL_M		25	
#define DISPPLL_N		1
#define DISPPLL_M2		1


//------------------------------------------------------------------------------
//  OPP mode related table
#define DEFAULT_OPP 1
#define VDD2_INIT   0x2E //1.1375

typedef struct CPU_OPP_SETTINGS
{
    UINT32 MPUMult;
    UINT32 VDD1Init;
}CPU_OPP_Settings, *pCPU_OPP_Settings;

CPU_OPP_Settings AM33x_OPP_Table[AM33x_OPP_NUM]=
{
    // MPU[275Mhz @ 0.95V], 
    {275, 0x1f},
    // MPU[500Mhz @ 1.1V], 
    {500, 0x2b},
    // MPU[600Mhz @ 1.2V], 
    {600, 0x33},
    // MPU[720Mhz @ 1.26V], 
    {720, 0x38},     
    // MPU[1000Mhz @ 1.325], 
    {1000, 0x3D}     
};


typedef struct CTRL_DDR
{
    UINT32 CMD0_CSRATIO;
    UINT32 CMD0_DLDIFF;
	UINT32 CMD0_INVCLKOUT;
    UINT32 CMD1_CSRATIO;
    UINT32 CMD1_DLDIFF;
	UINT32 CMD1_INVCLKOUT;
    UINT32 CMD2_CSRATIO;
    UINT32 CMD2_DLDIFF;
	UINT32 CMD2_INVCLKOUT;
}CTRL_ddr, *pCTRL_ddr;


typedef struct DATA_DDR
{
	BOOL   IsDDR2;
    UINT32 RD_DQS;
    UINT32 WR_DQS;
	UINT32 PHY_WRLVL;
    UINT32 PHY_GATELVL;
    UINT32 PHY_FIFO_WE;
	UINT32 PHY_WR_DATA;
}DATA_ddr, *pDATA_ddr;


typedef struct EMIF_DDR
{
	BOOL   IsDDR3;
    UINT32 READ_LATENCY;
    UINT32 TIM1;
	UINT32 TIM2;
    UINT32 TIM3;
    UINT32 SDCFG;
	UINT32 SDCFG2;
	UINT32 ZQCR;
	UINT32 SDREF;
}EMIF_ddr, *pEMIF_ddr;



void interface_clocks_enable(void)
{
	/* Enable all the Interconnect Modules */
    EnableDeviceClocks(AM_DEVICE_L3,TRUE);

    EnableDeviceClocks(AM_DEVICE_L4LS,TRUE);

    EnableDeviceClocks(AM_DEVICE_L4FW,TRUE);

    EnableDeviceClocks(AM_DEVICE_L4WKUP,TRUE);

    EnableDeviceClocks(AM_DEVICE_L3_INSTR,TRUE);

    EnableDeviceClocks(AM_DEVICE_L4_HS,TRUE);
}


void gpio_init(void)
{
	EnableDeviceClocks(AM_DEVICE_GPIO0,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO1,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO2,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO3,TRUE);

	BSPInsertGpioDevice(0,&Am3xx_Gpio,NULL);
}


void power_domain_transition_enable(void)
{
	/*
	 * Force power domain wake up transition
	 * Ensure that the corresponding interface clock is active before
	 * using the peripheral
	 */
	OUTREG32(CM_PER_L3_CLKSTCTRL,   PRCM_FORCE_WAKEUP);
	OUTREG32(CM_PER_L4LS_CLKSTCTRL, PRCM_FORCE_WAKEUP);
	OUTREG32(CM_WKUP_CLKSTCTRL,     PRCM_FORCE_WAKEUP);
	OUTREG32(CM_PER_L4FW_CLKSTCTRL, PRCM_FORCE_WAKEUP);
	OUTREG32(CM_PER_L3S_CLKSTCTRL,  PRCM_FORCE_WAKEUP);
}

/*
 * Enable the module clock and the power domain for required peripherals
 */
void per_clocks_enable(void)
{
	/* GPMC */
    EnableDeviceClocks(AM_DEVICE_GPMC,TRUE);

	/* ELM */
    EnableDeviceClocks(AM_DEVICE_ELM,TRUE);

	/* Ethernet */
    EnableDeviceClocks(AM_DEVICE_CPGMAC0, TRUE);

	/* Enable the control module though RBL would have done it*/
    EnableDeviceClocks(AM_DEVICE_CONTROL, TRUE);

}

void mpu_pll_config(pCPU_OPP_Settings opp_setting)
{
	UINT32 clkmode, clksel, div_m2;

	clkmode = INREG32(OALPAtoUA(CM_CLKMODE_DPLL_MPU));
	clksel = INREG32(OALPAtoUA(CM_CLKSEL_DPLL_MPU));
	div_m2 = INREG32(OALPAtoUA(CM_DIV_M2_DPLL_MPU));

	/* Set the PLL to bypass Mode */
	OUTREG32(CM_CLKMODE_DPLL_MPU, PLL_BYPASS_MODE);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_MPU)) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((opp_setting->MPUMult<< 0x8) | MPUPLL_N);
	OUTREG32(CM_CLKSEL_DPLL_MPU, clksel);

	div_m2 = div_m2 & ~0x1f;
	div_m2 = div_m2 | MPUPLL_M2;
	OUTREG32(CM_DIV_M2_DPLL_MPU, div_m2);

	clkmode = clkmode | 0x7;
	OUTREG32(CM_CLKMODE_DPLL_MPU, clkmode);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_MPU)) != 0x1);
}

void core_pll_config(void)
{
	UINT32 clkmode, clksel, div_m4, div_m5, div_m6;

	clkmode = INREG32(OALPAtoUA(CM_CLKMODE_DPLL_CORE));
	clksel = INREG32(OALPAtoUA(CM_CLKSEL_DPLL_CORE));
	div_m4 = INREG32(OALPAtoUA(CM_DIV_M4_DPLL_CORE));
	div_m5 = INREG32(OALPAtoUA(CM_DIV_M5_DPLL_CORE));
	div_m6 = INREG32(OALPAtoUA(CM_DIV_M6_DPLL_CORE));

	/* Set the PLL to bypass Mode */
	OUTREG32(CM_CLKMODE_DPLL_CORE, PLL_BYPASS_MODE);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_CORE)) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((COREPLL_M << 0x8) | COREPLL_N);
	OUTREG32(CM_CLKSEL_DPLL_CORE, clksel);

	div_m4 = div_m4 & ~0x1f;
	div_m4 = div_m4 | COREPLL_M4;
	OUTREG32(CM_DIV_M4_DPLL_CORE, div_m4);

	div_m5 = div_m5 & ~0x1f;
	div_m5 = div_m5 | COREPLL_M5;
	OUTREG32(CM_DIV_M5_DPLL_CORE, div_m5);

	div_m6 = div_m6 & ~0x1f;
	div_m6 = div_m6 | COREPLL_M6;
	OUTREG32(CM_DIV_M6_DPLL_CORE, div_m6);


	clkmode = clkmode | 0x7;
	OUTREG32(CM_CLKMODE_DPLL_CORE, clkmode);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_CORE)) != 0x1);
}

void per_pll_config(void)
{
	UINT32 clkmode, clksel, div_m2;

	clkmode = INREG32(OALPAtoUA(CM_CLKMODE_DPLL_PER));
	clksel = INREG32(OALPAtoUA(CM_CLKSEL_DPLL_PER));
	div_m2 = INREG32(OALPAtoUA(CM_DIV_M2_DPLL_PER));

	/* Set the PLL to bypass Mode */
	OUTREG32(CM_CLKMODE_DPLL_PER, PLL_BYPASS_MODE);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_PER)) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((PERPLL_M << 0x8) | PERPLL_N);
	OUTREG32(CM_CLKSEL_DPLL_PER, clksel);

	div_m2 = div_m2 & ~0x7f;
	div_m2 = div_m2 | PERPLL_M2;
	OUTREG32(CM_DIV_M2_DPLL_PER, div_m2);

	clkmode = clkmode | 0x7;
	OUTREG32(CM_CLKMODE_DPLL_PER, clkmode);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_PER)) != 0x1);

}

void disp_pll_config(void)
{
	UINT32 clkmode, clksel, div_m2;

	clkmode = INREG32(OALPAtoUA(CM_CLKMODE_DPLL_DISP));
	clksel = INREG32(OALPAtoUA(CM_CLKSEL_DPLL_DISP));
	div_m2 = INREG32(OALPAtoUA(CM_DIV_M2_DPLL_DISP));

	/* Set the PLL to bypass Mode */
	OUTREG32(CM_CLKMODE_DPLL_DISP, PLL_BYPASS_MODE);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_DISP)) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((DISPPLL_M << 0x8) | DISPPLL_N);
	OUTREG32(CM_CLKSEL_DPLL_DISP, clksel);

	clkmode = clkmode | 0x7;
	OUTREG32(CM_CLKMODE_DPLL_DISP, clkmode);

	while(INREG32(OALPAtoUA(CM_IDLEST_DPLL_DISP)) != 0x1);

	/* SELECT the MUX  line for LCDC PIXEL clock*/
	OUTREG32(CM_CLKSEL_LCDC_PIXEL_CLK, LCDC_PIXEL_CLK_DISP_CLKOUTM2);  //250MHz

}


void ddr_pll_config(unsigned int ddrpll_m)
{
	UINT32 clkmode, clksel, div_m2;

	clkmode = INREG32(OALPAtoUA(CM_CLKMODE_DPLL_DDR));
	clksel = INREG32(OALPAtoUA(CM_CLKSEL_DPLL_DDR));
	div_m2 = INREG32(OALPAtoUA(CM_DIV_M2_DPLL_DDR));

	/* Set the PLL to bypass Mode */
	clkmode = (clkmode & 0xfffffff8) | 0x00000004;
	OUTREG32(CM_CLKMODE_DPLL_DDR, clkmode);

	while ((INREG32(OALPAtoUA(CM_IDLEST_DPLL_DDR)) & 0x00000100) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((ddrpll_m << 0x8) | DDRPLL_N);
	OUTREG32(CM_CLKSEL_DPLL_DDR, clksel);

	div_m2 = div_m2 & 0xFFFFFFE0;
	div_m2 = div_m2 | DDRPLL_M2;
	OUTREG32(CM_DIV_M2_DPLL_DDR, div_m2);

	clkmode = (clkmode & 0xfffffff8) | 0x7;
	OUTREG32(CM_CLKMODE_DPLL_DDR, clkmode);

	while ((INREG32(OALPAtoUA(CM_IDLEST_DPLL_DDR)) & 0x00000001) != 0x1);
}


/*
 * Configure the PLL/PRCM for necessary peripherals
 */
void pll_init()
{
    mpu_pll_config(&AM33x_OPP_Table[DEFAULT_OPP]);
	core_pll_config();
	per_pll_config();
//	ddr_pll_config();
    disp_pll_config();	
	/* Enable the required interconnect clocks */
	interface_clocks_enable();
	/* Enable power domain transition */
	power_domain_transition_enable();
	/* Enable the required peripherals */
	per_clocks_enable();
}

/****************************************************************************/

/* DDR offsets */
#define	DDR_PHY_BASE_ADDR		0x44E12000
#define	DDR_IO_CTRL				0x44E10E04
#define	DDR_CKE_CTRL			0x44E1131C
#define	CONTROL_BASE_ADDR		AM33X_CONTROL_MODULE_REGS_PA

#define	DDR_CMD0_IOCTRL			(CONTROL_BASE_ADDR + 0x1404)
#define	DDR_CMD1_IOCTRL			(CONTROL_BASE_ADDR + 0x1408)
#define	DDR_CMD2_IOCTRL			(CONTROL_BASE_ADDR + 0x140C)
#define	DDR_DATA0_IOCTRL		(CONTROL_BASE_ADDR + 0x1440)
#define	DDR_DATA1_IOCTRL		(CONTROL_BASE_ADDR + 0x1444)

#define	CMD0_CTRL_SLAVE_RATIO_0		(DDR_PHY_BASE_ADDR + 0x01C)
#define	CMD0_CTRL_SLAVE_FORCE_0		(DDR_PHY_BASE_ADDR + 0x020)
#define	CMD0_CTRL_SLAVE_DELAY_0		(DDR_PHY_BASE_ADDR + 0x024)
#define	CMD0_DLL_LOCK_DIFF_0		(DDR_PHY_BASE_ADDR + 0x028)
#define	CMD0_INVERT_CLKOUT_0		(DDR_PHY_BASE_ADDR + 0x02C)

#define	CMD1_CTRL_SLAVE_RATIO_0		(DDR_PHY_BASE_ADDR + 0x050)
#define	CMD1_CTRL_SLAVE_FORCE_0		(DDR_PHY_BASE_ADDR + 0x054)
#define	CMD1_CTRL_SLAVE_DELAY_0		(DDR_PHY_BASE_ADDR + 0x058)
#define	CMD1_DLL_LOCK_DIFF_0		(DDR_PHY_BASE_ADDR + 0x05C)
#define	CMD1_INVERT_CLKOUT_0		(DDR_PHY_BASE_ADDR + 0x060)

#define	CMD2_CTRL_SLAVE_RATIO_0		(DDR_PHY_BASE_ADDR + 0x084)
#define	CMD2_CTRL_SLAVE_FORCE_0		(DDR_PHY_BASE_ADDR + 0x088)
#define	CMD2_CTRL_SLAVE_DELAY_0		(DDR_PHY_BASE_ADDR + 0x08C)
#define	CMD2_DLL_LOCK_DIFF_0		(DDR_PHY_BASE_ADDR + 0x090)
#define	CMD2_INVERT_CLKOUT_0		(DDR_PHY_BASE_ADDR + 0x094)

#define DATA0_RD_DQS_SLAVE_RATIO_0	(DDR_PHY_BASE_ADDR + 0x0C8)
#define DATA0_RD_DQS_SLAVE_RATIO_1	(DDR_PHY_BASE_ADDR + 0x0CC)
#define	DATA0_WR_DQS_SLAVE_RATIO_0	(DDR_PHY_BASE_ADDR + 0x0DC)

#define	DATA0_WR_DQS_SLAVE_RATIO_1	(DDR_PHY_BASE_ADDR + 0x0E0)
#define	DATA0_WRLVL_INIT_RATIO_0	(DDR_PHY_BASE_ADDR + 0x0F0)

#define	DATA0_WRLVL_INIT_RATIO_1	(DDR_PHY_BASE_ADDR + 0x0F4)
#define	DATA0_GATELVL_INIT_RATIO_0	(DDR_PHY_BASE_ADDR + 0x0FC)

#define	DATA0_GATELVL_INIT_RATIO_1	(DDR_PHY_BASE_ADDR + 0x100)
#define	DATA0_FIFO_WE_SLAVE_RATIO_0	(DDR_PHY_BASE_ADDR + 0x108)

#define	DATA0_FIFO_WE_SLAVE_RATIO_1	(DDR_PHY_BASE_ADDR + 0x10C)
#define	DATA0_WR_DATA_SLAVE_RATIO_0	(DDR_PHY_BASE_ADDR + 0x120)

#define	DATA0_WR_DATA_SLAVE_RATIO_1	(DDR_PHY_BASE_ADDR + 0x124)
#define DATA0_DLL_LOCK_DIFF_0		(DDR_PHY_BASE_ADDR + 0x138)

#define DATA0_RANK0_DELAYS_0		(DDR_PHY_BASE_ADDR + 0x134)
#define	DATA1_RANK0_DELAYS_0		(DDR_PHY_BASE_ADDR + 0x1D8)


/* AM335X EMIF Register values */
#define EMIF_SDMGT				0x80000000
#define EMIF_SDRAM				0x00004650
#define EMIF_PHYCFG				0x2
#define DDR_PHY_RESET			(0x1 << 10)
#define DDR_FUNCTIONAL_MODE_EN	0x1
#define DDR_PHY_READY			(0x1 << 2)
#define	VTP_CTRL_READY			(0x1 << 5)
#define VTP_CTRL_ENABLE			(0x1 << 6)
#define VTP_CTRL_LOCK_EN		(0x1 << 4)
#define VTP_CTRL_START_EN		(0x1)
#define DDR2_RATIO				0x80	/* for mDDR */
#define DDR3_RATIO				0x80	/* for mDDR */
#define CMD_FORCE				0x00	/* common #def */
#define CMD_DELAY				0x00

#define DDR2_EMIF_READ_LATENCY	0x04
#define DDR2_EMIF_TIM1			0x0666B3D6
#define DDR2_EMIF_TIM2			0x143731DA
#define	DDR2_EMIF_TIM3			0x00000347
#define DDR2_EMIF_SDCFG			0x43805332
#define DDR2_EMIF_SDCFG2		0x43805332
#define DDR2_EMIF_SDREF			0x0000081a

#define DDR3_EMIF_READ_LATENCY	0x100007
#define DDR3_EMIF_TIM1			0x0AAAD4DB
#define DDR3_EMIF_TIM2			0x266B7FDA
#define	DDR3_EMIF_TIM3			0x501F867F
#define DDR3_EMIF_SDCFG			0x61C05332
#define DDR3_EMIF_SDCFG2		0x61C05332
#define DDR3_EMIF_ZQCR			0x50074BE4
#define DDR3_EMIF_SDREF			0xC30

#define	PHY_RANK0_DELAY		0x01
#define PHY_DLL_LOCK_DIFF	0x0
#define DDR_IOCTRL_VALUE	0x18B

#define DDR2_RD_DQS			0x12
#define	DDR2_WR_DQS			0x00
#define DDR2_PHY_FIFO_WE	0x80
#define DDR2_DLL_LOCK_DIFF	0x0
#define	DDR2_INVERT_CLKOUT	0x00
#define	DDR2_PHY_WR_DATA	0x40
#define	DDR2_PHY_WRLVL		0x00
#define	DDR2_PHY_GATELVL	0x00

#define DDR3_RD_DQS			0x38
#define	DDR3_WR_DQS			0x44
#define DDR3_PHY_FIFO_WE	0x94
#define DDR3_DLL_LOCK_DIFF	0x1
#define	DDR3_INVERT_CLKOUT	0x00
#define	DDR3_PHY_WR_DATA	0x7D


static CTRL_ddr ctrl_ddr2 = {
	DDR2_RATIO,
	DDR2_DLL_LOCK_DIFF,
	DDR2_INVERT_CLKOUT,
	DDR2_RATIO,
	DDR2_DLL_LOCK_DIFF,
	DDR2_INVERT_CLKOUT,
	DDR2_RATIO,
	DDR2_DLL_LOCK_DIFF,
	DDR2_INVERT_CLKOUT,
};

static CTRL_ddr ctrl_ddr3 = {
	DDR3_RATIO,
	DDR3_DLL_LOCK_DIFF,
	DDR3_INVERT_CLKOUT,
	DDR3_RATIO,
	DDR3_DLL_LOCK_DIFF,
	DDR3_INVERT_CLKOUT,
	DDR3_RATIO,
	DDR3_DLL_LOCK_DIFF,
	DDR3_INVERT_CLKOUT,
};


static DATA_ddr data_ddr2 = {
	TRUE,
    DDR2_RD_DQS,
    DDR2_WR_DQS,
	DDR2_PHY_WRLVL,
    DDR2_PHY_GATELVL,
    DDR2_PHY_FIFO_WE,
	DDR2_PHY_WR_DATA,
};

static DATA_ddr data_ddr3 = {
	FALSE,
    DDR3_RD_DQS,
    DDR3_WR_DQS,
	0,
    0,
    DDR3_PHY_FIFO_WE,
	DDR3_PHY_WR_DATA,
};


static EMIF_ddr emif_ddr2 = {
	FALSE,
	DDR2_EMIF_READ_LATENCY,
	DDR2_EMIF_TIM1,
	DDR2_EMIF_TIM2,
	DDR2_EMIF_TIM3,
	DDR2_EMIF_SDCFG,
	DDR2_EMIF_SDCFG2,
	0,
	DDR2_EMIF_SDREF,
};

static EMIF_ddr emif_ddr3 = {
	TRUE,
	DDR3_EMIF_READ_LATENCY,
	DDR3_EMIF_TIM1,
	DDR3_EMIF_TIM2,
	DDR3_EMIF_TIM3,
	DDR3_EMIF_SDCFG,
	DDR3_EMIF_SDCFG2,
	DDR3_EMIF_ZQCR,
	DDR3_EMIF_SDREF,
};



void enable_ddr_clocks(void)
{
	/* Enable the  EMIF_FW Functional clock */
    EnableDeviceClocks(AM_DEVICE_EMIF_FW, TRUE);

	/* Enable EMIF0 Clock */
    EnableDeviceClocks(AM_DEVICE_EMIF, TRUE);

	/* Poll for emif_gclk  & L3_G clock  are active */
	while ((INREG32(OALPAtoUA(CM_PER_L3_CLKSTCTRL)) & 
       (PRCM_EMIF_CLK_ACTIVITY | PRCM_L3_GCLK_ACTIVITY)) != 
       (PRCM_EMIF_CLK_ACTIVITY | PRCM_L3_GCLK_ACTIVITY));
}

static void config_vtp(void)
{
	OUTREG32(VTP0_CTRL_REG, INREG32(OALPAtoUA(VTP0_CTRL_REG)) | VTP_CTRL_ENABLE);
	OUTREG32(VTP0_CTRL_REG, INREG32(OALPAtoUA(VTP0_CTRL_REG)) & (~VTP_CTRL_START_EN));
	OUTREG32(VTP0_CTRL_REG, INREG32(OALPAtoUA(VTP0_CTRL_REG)) | VTP_CTRL_START_EN);

	/* Poll for READY */
	while ((INREG32(OALPAtoUA(VTP0_CTRL_REG)) & VTP_CTRL_READY) != VTP_CTRL_READY);
}

static void Cmd_Macro_Config(CTRL_ddr *pctrl_ddr)
{
	OUTREG32(OALPAtoUA(CMD0_CTRL_SLAVE_RATIO_0), pctrl_ddr->CMD0_CSRATIO);
	OUTREG32(OALPAtoUA(CMD0_CTRL_SLAVE_FORCE_0), CMD_FORCE);
	OUTREG32(OALPAtoUA(CMD0_CTRL_SLAVE_DELAY_0), CMD_DELAY);
	OUTREG32(OALPAtoUA(CMD0_DLL_LOCK_DIFF_0),    pctrl_ddr->CMD0_DLDIFF);
	OUTREG32(OALPAtoUA(CMD0_INVERT_CLKOUT_0),    pctrl_ddr->CMD0_INVCLKOUT);

	OUTREG32(OALPAtoUA(CMD1_CTRL_SLAVE_RATIO_0), pctrl_ddr->CMD1_CSRATIO);
	OUTREG32(OALPAtoUA(CMD1_CTRL_SLAVE_FORCE_0), CMD_FORCE);
	OUTREG32(OALPAtoUA(CMD1_CTRL_SLAVE_DELAY_0), CMD_DELAY);
	OUTREG32(OALPAtoUA(CMD1_DLL_LOCK_DIFF_0),    pctrl_ddr->CMD1_DLDIFF);
	OUTREG32(OALPAtoUA(CMD1_INVERT_CLKOUT_0),    pctrl_ddr->CMD1_INVCLKOUT);

	OUTREG32(OALPAtoUA(CMD2_CTRL_SLAVE_RATIO_0), pctrl_ddr->CMD2_CSRATIO);
	OUTREG32(OALPAtoUA(CMD2_CTRL_SLAVE_FORCE_0), CMD_FORCE);
	OUTREG32(OALPAtoUA(CMD2_CTRL_SLAVE_DELAY_0), CMD_DELAY);
	OUTREG32(OALPAtoUA(CMD2_DLL_LOCK_DIFF_0),    pctrl_ddr->CMD2_DLDIFF);
	OUTREG32(OALPAtoUA(CMD2_INVERT_CLKOUT_0),    pctrl_ddr->CMD2_INVCLKOUT);
}

static void Data_Macro_Config(int dataMacroNum, DATA_ddr *pdata_ddr)
{
	UINT32 BaseAddrOffset = 0x00;

	if (dataMacroNum == 0)
		BaseAddrOffset = 0x00;
	else if (dataMacroNum == 1)
		BaseAddrOffset = 0xA4;

	OUTREG32(
		OALPAtoUA(DATA0_RD_DQS_SLAVE_RATIO_0 + BaseAddrOffset),
        ((pdata_ddr->RD_DQS<<30)|(pdata_ddr->RD_DQS<<20)|(pdata_ddr->RD_DQS<<10)|(pdata_ddr->RD_DQS<<0))
    );

	OUTREG32(
        OALPAtoUA(DATA0_RD_DQS_SLAVE_RATIO_1 + BaseAddrOffset),
        pdata_ddr->RD_DQS>>2
    );

	OUTREG32(
        OALPAtoUA(DATA0_WR_DQS_SLAVE_RATIO_0 + BaseAddrOffset),
        ((pdata_ddr->WR_DQS<<30)|(pdata_ddr->WR_DQS<<20)|(pdata_ddr->WR_DQS<<10)|(pdata_ddr->WR_DQS<<0))
    );

	OUTREG32(
        OALPAtoUA(DATA0_WR_DQS_SLAVE_RATIO_1 + BaseAddrOffset),
        pdata_ddr->WR_DQS>>2
    );


	if (TRUE == pdata_ddr->IsDDR2)
	{
		OUTREG32(
			OALPAtoUA(DATA0_WRLVL_INIT_RATIO_0 + BaseAddrOffset),
			((pdata_ddr->PHY_WRLVL<<30)|(pdata_ddr->PHY_WRLVL<<20)|(pdata_ddr->PHY_WRLVL<<10)|(pdata_ddr->PHY_WRLVL<<0))
		);

		OUTREG32(
			OALPAtoUA(DATA0_WRLVL_INIT_RATIO_1 + BaseAddrOffset),
			pdata_ddr->PHY_WRLVL>>2
		);

		OUTREG32(
			OALPAtoUA(DATA0_GATELVL_INIT_RATIO_0 + BaseAddrOffset),
			((pdata_ddr->PHY_GATELVL<<30)|(pdata_ddr->PHY_GATELVL<<20)|(pdata_ddr->PHY_GATELVL<<10)|(pdata_ddr->PHY_GATELVL<<0))
		);

		OUTREG32(
			OALPAtoUA(DATA0_GATELVL_INIT_RATIO_1 + BaseAddrOffset),
			pdata_ddr->PHY_GATELVL>>2
		);
	}

	OUTREG32(
        OALPAtoUA(DATA0_FIFO_WE_SLAVE_RATIO_0 + BaseAddrOffset),
        ((pdata_ddr->PHY_FIFO_WE<<30)|(pdata_ddr->PHY_FIFO_WE<<20)|(pdata_ddr->PHY_FIFO_WE<<10)|(pdata_ddr->PHY_FIFO_WE<<0))
    );

	OUTREG32(
        OALPAtoUA(DATA0_FIFO_WE_SLAVE_RATIO_1 + BaseAddrOffset),
        pdata_ddr->PHY_FIFO_WE>>2
    );

	OUTREG32(
		OALPAtoUA(DATA0_WR_DATA_SLAVE_RATIO_0 + BaseAddrOffset),
        ((pdata_ddr->PHY_WR_DATA<<30)|(pdata_ddr->PHY_WR_DATA<<20)|(pdata_ddr->PHY_WR_DATA<<10)|(pdata_ddr->PHY_WR_DATA<<0))
    );

	OUTREG32(
        OALPAtoUA(DATA0_WR_DATA_SLAVE_RATIO_1 + BaseAddrOffset),
        pdata_ddr->PHY_WR_DATA>>2
    );

	if (TRUE == pdata_ddr->IsDDR2)
	{
		OUTREG32(
			OALPAtoUA(DATA0_DLL_LOCK_DIFF_0 + BaseAddrOffset), 
			PHY_DLL_LOCK_DIFF
		);
	}
}

static void config_emif_ddr(EMIF_ddr *pemif_ddr)
{
	UINT32 i;

	/*Program EMIF0 CFG Registers*/
	OUTREG32(OALPAtoUA(EMIF4_0_DDR_PHY_CTRL_1),        pemif_ddr->READ_LATENCY);
	OUTREG32(OALPAtoUA(EMIF4_0_DDR_PHY_CTRL_1_SHADOW), pemif_ddr->READ_LATENCY);
	OUTREG32(OALPAtoUA(EMIF4_0_DDR_PHY_CTRL_2),        pemif_ddr->READ_LATENCY);

	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_1),        pemif_ddr->TIM1);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_1_SHADOW), pemif_ddr->TIM1);

	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_2),        pemif_ddr->TIM2);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_2_SHADOW), pemif_ddr->TIM2);

	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_3),        pemif_ddr->TIM3);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_TIM_3_SHADOW), pemif_ddr->TIM3);

	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_CONFIG),  pemif_ddr->SDCFG);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_CONFIG2), pemif_ddr->SDCFG2);

	/*
    OUTREG32(OALPAtoUA(EMIF0_0_SDRAM_MGMT_CTRL),     pemif_ddr->SDMGT);
 	OUTREG32(OALPAtoUA(EMIF0_0_SDRAM_MGMT_CTRL_SHD), pemif_ddr->SDMGT);
    */
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_REF_CTRL),        0x00004650);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_REF_CTRL_SHADOW), 0x00004650);

	if (TRUE == pemif_ddr->IsDDR3)
	{
		OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_ZQ_CONFIG), pemif_ddr->ZQCR);
	}


	for (i = 0; i < 5000; i++) {

	}

	/* 
    OUTREG32(OALPAtoUA(EMIF0_0_SDRAM_MGMT_CTRL),     pemif_ddr->SDMGT);
	OUTREG32(OALPAtoUA(EMIF0_0_SDRAM_MGMT_CTRL_SHD), pemif_ddr->SDMGT); 
    */
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_REF_CTRL), pemif_ddr->SDREF);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_REF_CTRL_SHADOW), pemif_ddr->SDREF);

	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_CONFIG),  pemif_ddr->SDCFG);
	OUTREG32(OALPAtoUA(EMIF4_0_SDRAM_CONFIG2), pemif_ddr->SDCFG2);
}

static void config_am33x_ddr(void)
{
	int data_macro_0 = 0;
	int data_macro_1 = 1;

	enable_ddr_clocks();

	if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
	{
		ddr_pll_config(400);
		config_vtp();
		Cmd_Macro_Config(&ctrl_ddr3);
		Data_Macro_Config(data_macro_0, &data_ddr3);
		Data_Macro_Config(data_macro_1, &data_ddr3);
	}
	else
	{
		ddr_pll_config(266);
//		ddr_pll_config(303);
		config_vtp();
		Cmd_Macro_Config(&ctrl_ddr2);
		Data_Macro_Config(data_macro_0, &data_ddr2);
		Data_Macro_Config(data_macro_1, &data_ddr2);
		OUTREG32(OALPAtoUA(DATA0_RANK0_DELAYS_0), PHY_RANK0_DELAY);
		OUTREG32(OALPAtoUA(DATA1_RANK0_DELAYS_0), PHY_RANK0_DELAY);
	}


	OUTREG32(OALPAtoUA(DDR_CMD0_IOCTRL),  DDR_IOCTRL_VALUE);
	OUTREG32(OALPAtoUA(DDR_CMD1_IOCTRL),  DDR_IOCTRL_VALUE);
	OUTREG32(OALPAtoUA(DDR_CMD2_IOCTRL),  DDR_IOCTRL_VALUE);

	OUTREG32(OALPAtoUA(DDR_DATA0_IOCTRL), DDR_IOCTRL_VALUE);
	OUTREG32(OALPAtoUA(DDR_DATA1_IOCTRL), DDR_IOCTRL_VALUE);

	OUTREG32(OALPAtoUA(DDR_IO_CTRL),  INREG32(OALPAtoUA(DDR_IO_CTRL))  & 0x0fffffff);
	OUTREG32(OALPAtoUA(DDR_CKE_CTRL), INREG32(OALPAtoUA(DDR_CKE_CTRL)) | 0x00000001);

	if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
	{
		config_emif_ddr(&emif_ddr3);
	}
	else
	{
		config_emif_ddr(&emif_ddr2);
	}
}

/****************************************************************************/

void configure_pin_mux()
{
    /* do nothing here since UART0, and boot device (ex: NAND, NOR, SPI, MMC) pinmux 
               will be enabled by bootrom. Except for I2C0, there shouldnt be any other device pinmux needed 
               in xldr. For all other devices needed by eboot and kernel, set the pinmux in respective places */    
	const PAD_INFO UART0Pads[]  =	{UART0_PADS END_OF_PAD_ARRAY};
	ConfigurePadArray(UART0Pads);
}

void enable_i2c0_pin_mux(void)
{
    // since board and profile info is not available until we have I2C0 functional, 
    // we cannot use BSPGetDevicePadInfo  
    const PAD_INFO I2C0Pads[]   =   {I2C0_PADS END_OF_PAD_ARRAY};
	ConfigurePadArray(I2C0Pads);
}

/****************************************************************************/

/* RGMII mode define */
#define RGMII_MODE_ENABLE	0xA
#define RMII_MODE_ENABLE	0x5
#define MII_MODE_ENABLE		0x0
#define GMII_SEL_REG        (AM33X_CONTROL_MODULE_REGS_PA+0x0650)
#define MAC_MII_SEL         GMII_SEL_REG

// TODO: read from EEPROM
#define HEADER_VERION_00A1  0
#define HEADER_VERION_00A2  0
#define HEADER_VERION_00A3  1

int board_eth_init(void)
{
    /* For beaglebone > Rev A2 , enable MII mode, for others enable RMII */
#if HEADER_VERION_00A1
	OUTREG32(MAC_MII_SEL, RMII_MODE_ENABLE);
#elif HEADER_VERION_00A2
	OUTREG32(MAC_MII_SEL, RMII_MODE_ENABLE);
#elif HEADER_VERION_00A3
	OUTREG32(MAC_MII_SEL, MII_MODE_ENABLE);
#endif

    return 0;
}


/****************************************************************************/

#define CONFIG_CMD_NAND

#define PISMO1_NAND_BASE GPMC_NAND_BASE
#define PISMO1_NAND_SIZE GPMC_NAND_SIZE

static VOID sdelay(UINT32 loops)
{
    OALStall(loops);
    return;
}

#if defined(CONFIG_CMD_NAND)
static const UINT32 gpmc_m_nand[GPMC_MAX_REG] = {
	M_NAND_GPMC_CONFIG1,
	M_NAND_GPMC_CONFIG2,
	M_NAND_GPMC_CONFIG3,
	M_NAND_GPMC_CONFIG4,
	M_NAND_GPMC_CONFIG5,
	M_NAND_GPMC_CONFIG6, 0
};

#define GPMC_CS 0

void enable_gpmc_cs_config(
    const UINT32 *gpmc_config, AM3XX_GPMC_CS *cs, UINT32 base, UINT32 size
)
{
	OUTREG32(&cs->GPMC_CONFIG7, 0);
	sdelay(1000);
	/* Delay for settling */
	OUTREG32(&cs->GPMC_CONFIG1, gpmc_config[0]);
	OUTREG32(&cs->GPMC_CONFIG2, gpmc_config[1]);
	OUTREG32(&cs->GPMC_CONFIG3, gpmc_config[2]);
	OUTREG32(&cs->GPMC_CONFIG4, gpmc_config[3]);
	OUTREG32(&cs->GPMC_CONFIG5, gpmc_config[4]);
	OUTREG32(&cs->GPMC_CONFIG6, gpmc_config[5]);
	/* Enable the config */
	OUTREG32(&cs->GPMC_CONFIG7, (((size & 0xF) << 8) | ((base >> 24) & 0x3F) | (1 << 6)));
	sdelay(2000);
}
#endif

/*****************************************************
 * gpmc_init(): init gpmc bus
 * Init GPMC for x16, MuxMode (SDRAM in x32).
 * This code can only be executed from SRAM or SDRAM.
 *****************************************************/
void gpmc_init(void)
{
	/* putting a blanket check on GPMC based on ZeBu for now */
    AM3XX_GPMC_REGS *gpmc_cfg = (AM3XX_GPMC_REGS *)AM33X_GPMC_REGS_PA;

#if defined(CONFIG_CMD_NAND) || defined(CONFIG_CMD_ONENAND)
	const UINT32 *gpmc_config = NULL;
	UINT32 base = 0;
	UINT32 size = 0;
#endif
	/* global settings */
	OUTREG32(&gpmc_cfg->GPMC_SYSCONFIG, 0x00000008);
	OUTREG32(&gpmc_cfg->GPMC_IRQSTATUS, 0x00000100);
	OUTREG32(&gpmc_cfg->GPMC_IRQENABLE, 0x00000200);
	OUTREG32(&gpmc_cfg->GPMC_CONFIG,    0x00000012);
	/*
	 * Disable the GPMC0 config set by ROM code
	 */
	OUTREG32(&gpmc_cfg->CS[0].GPMC_CONFIG7, 0);
	sdelay(1000);

#if defined(CONFIG_CMD_NAND)	/* CS 0 */
	gpmc_config = gpmc_m_nand;

	base = PISMO1_NAND_BASE;
	size = PISMO1_NAND_SIZE;
	enable_gpmc_cs_config(gpmc_config, &gpmc_cfg->CS[0], base, size);
#endif
}

/****************************************************************************/

int board_init()
{
    pCPU_OPP_Settings opp_setting = &AM33x_OPP_Table[BSP_OPM_SELECT-1];    
    unsigned char pmic_status_reg;
	
	/* Configure the i2c0 pin mux */
	enable_i2c0_pin_mux();
    OALI2CInit(AM_DEVICE_I2C0);

    detect_baseboard_id_info(); 

    if (g_dwBoardId == AM33X_BOARDID_BBONE_BOARD)
    {
        if (TWLGetStatusReg(&pmic_status_reg)==FALSE)
			goto exit_pmic_config;

        /* Increase USB current limit to 1300mA */
		TWLProtWriteRegs(PROT_LEVEL_NONE, POWER_PATH,
				       PMIC_POWER_PATH_IUSB_LIMIT_1300MA,
				       PMIC_POWER_PATH_IUSB_LIMIT_MASK);

        /* Set DCDC2 (MPU) voltage to 1.275V */
        TWLUpdateVoltage(DEFDCDC2,
					     DCDC_VOLT_SEL_1275MV);

        /* Set LDO3, LDO4 output voltage to 3.3V */
		TWLProtWriteRegs(PROT_LEVEL_2, DEFLS1,
				       LDO_VOLTAGE_OUT_3_3, PMIC_DEFLS1_MASK);

		TWLProtWriteRegs(PROT_LEVEL_2, DEFLS2,
				       LDO_VOLTAGE_OUT_3_3, PMIC_DEFLS2_MASK);

		if (!(pmic_status_reg & PWR_SRC_AC_BITMASK)) {			
			goto exit_pmic_config;
		}

		/* Set MPU Frequency to 720MHz if BSP_OPM_SELECT = 4 */
		mpu_pll_config(opp_setting);
        
    }
    else if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
    {
        if (TWLGetStatusReg(&pmic_status_reg)==FALSE)
			goto exit_pmic_config;

        /* Increase USB current limit to 1800mA */
		TWLProtWriteRegs(PROT_LEVEL_NONE, POWER_PATH,
				       PMIC_POWER_PATH_IUSB_LIMIT_1800MA,
				       PMIC_POWER_PATH_IUSB_LIMIT_MASK);

        /* Set DCDC2 (MPU) voltage to 1.325V */
        TWLUpdateVoltage(DEFDCDC2,
					     DCDC_VOLT_SEL_1325MV);

        /* Set LDO3 output voltage to 1.8V */
		TWLProtWriteRegs(PROT_LEVEL_2, DEFLS1,
				       LDO_VOLTAGE_OUT_1_8, PMIC_DEFLS1_MASK);

        /* Set LDO4 output voltage to 3.3V */
		TWLProtWriteRegs(PROT_LEVEL_2, DEFLS2,
				       LDO_VOLTAGE_OUT_3_3, PMIC_DEFLS2_MASK);

		/* Set MPU Frequency to 1Ghz if BSP_OPM_SELECT = 5 */
		mpu_pll_config(opp_setting);
        
    }
    else
    {
        TWLSetOPVoltage(kVdd2,VDD2_INIT);

        if (TWLSetOPVoltage(kVdd1,opp_setting->VDD1Init))
        {
            mpu_pll_config(opp_setting);   
        } 
    }   

exit_pmic_config:        
	configure_pin_mux();
	
	gpio_init();

	gpmc_init();
    
	return 0;
}

/****************************************************************************/

/* WDT related */
#define WDT_BASE		AM33X_WDT1_REGS_PA /*0x44E35000*/
#define WDT_WDSC		(WDT_BASE + 0x010)
#define WDT_WDST		(WDT_BASE + 0x014)
#define WDT_WISR		(WDT_BASE + 0x018)
#define WDT_WIER		(WDT_BASE + 0x01C)
#define WDT_WWER		(WDT_BASE + 0x020)
#define WDT_WCLR		(WDT_BASE + 0x024)
#define WDT_WCRR		(WDT_BASE + 0x028)
#define WDT_WLDR		(WDT_BASE + 0x02C)
#define WDT_WTGR		(WDT_BASE + 0x030)
#define WDT_WWPS		(WDT_BASE + 0x034)
#define WDT_WDLY		(WDT_BASE + 0x044)
#define WDT_WSPR		(WDT_BASE + 0x048)
#define WDT_WIRQEOI		(WDT_BASE + 0x050)
#define WDT_WIRQSTATRAW	(WDT_BASE + 0x054)
#define WDT_WIRQSTAT	(WDT_BASE + 0x058)
#define WDT_WIRQENSET	(WDT_BASE + 0x05C)
#define WDT_WIRQENCLR	(WDT_BASE + 0x060)

#define WDT_UNFREEZE	(AM33X_CONTROL_MODULE_REGS_PA + 0x100)


//------------------------------------------------------------------------------
//
//  Function:  PlatformSetup
//
//  Initializes platform settings.  Stack based initialization only - no 
//  global variables allowed.
//
VOID PlatformSetup()
{
	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	OUTREG32(WDT_WSPR, 0xAAAA);
	while(INREG32(OALPAtoUA(WDT_WWPS)) != 0x0);

	OUTREG32(WDT_WSPR, 0x5555);
	while(INREG32(OALPAtoUA(WDT_WWPS)) != 0x0);

    pll_init();

    board_init();

	config_am33x_ddr();	/* Do DDR settings */

    board_eth_init();
}


