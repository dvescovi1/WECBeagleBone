// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  devicemap.c
//
//  Configuration file for the device list
//
#include "am33x.h"
#include "omap_devices_map.h"
#include "am33x_clocks.h"

const DEVICE_ADDRESS_MAP s_DeviceAddressMap[] = {
//  DeviceAddress    Device id
//------------------------------------------------------------------------------
    {AM33X_ADC_TSC_REGS_PA,AM_DEVICE_ADC_TSC}, 
    {AM33X_CONTROL_MODULE_REGS_PA,AM_DEVICE_CONTROL},
    {AM33X_DEBUGSS_REGS_PA,AM_DEVICE_DEBUGSS},
    {AM33X_GPIO0_REGS_PA,AM_DEVICE_GPIO0},
    {AM33X_I2C0_REGS_PA,AM_DEVICE_I2C0},
    {AM33X_L4_WKUP_REGS_PA,AM_DEVICE_L4WKUP},
    {AM33X_SMRFLX0_REGS_PA,AM_DEVICE_SMARTREFLEX0},
    {AM33X_SMRFLX1_REGS_PA,AM_DEVICE_SMARTREFLEX1},
    {AM33X_GPTIMER0_REGS_PA,AM_DEVICE_TIMER0},
    {AM33X_GPTIMER1_1MS_REGS_PA,AM_DEVICE_TIMER1},
    {AM33X_UART0_REGS_PA,AM_DEVICE_UART0}, 
    {AM33X_WDT0_REGS_PA,AM_DEVICE_WDT0},
    {AM33X_WDT1_REGS_PA,AM_DEVICE_WDT1},
    {AM33X_AES0_REGS_PA,AM_DEVICE_AES0},  
    {AM33X_EMACSW_REGS_PA,AM_DEVICE_CPGMAC0},
    {AM33X_DCAN0_REGS_PA,AM_DEVICE_DCAN0},
    {AM33X_DCAN1_REGS_PA,AM_DEVICE_DCAN1},
    {AM33X_ELM_REGS_PA,AM_DEVICE_ELM},
    {AM33X_EMIF4_0_CFG_REGS_PA,AM_DEVICE_EMIF},
    {AM33X_EMIFFW_REGS_PA,AM_DEVICE_EMIF_FW},
    {AM33X_ECAP_EQEP_EPWM0_REGS_PA,AM_DEVICE_EPWM0},
    {AM33X_ECAP_EQEP_EPWM1_REGS_PA,AM_DEVICE_EPWM1},
    {AM33X_ECAP_EQEP_EPWM2_REGS_PA,AM_DEVICE_EPWM2},
    {AM33X_GPIO1_REGS_PA,AM_DEVICE_GPIO1},
    {AM33X_GPIO2_REGS_PA,AM_DEVICE_GPIO2},
    {AM33X_GPIO3_REGS_PA,AM_DEVICE_GPIO3},
    {AM33X_GPMC_REGS_PA,AM_DEVICE_GPMC},
    {AM33X_I2C1_REGS_PA,AM_DEVICE_I2C1},
    {AM33X_I2C2_REGS_PA,AM_DEVICE_I2C2},
    {AM33X_ICSS_PRUSS1_REGS_PA,AM_DEVICE_ICSS},
    {AM33X_P1500_CFG_REGS_PA,AM_DEVICE_IEEE5000},
    {AM33X_L4FW_REGS_PA,AM_DEVICE_L4FW},  
    {AM33X_LCDC_REGS_PA,AM_DEVICE_LCDC},
    {AM33X_MBOX_REGS_PA,AM_DEVICE_MAILBOX0},
    {AM33X_MCASP0_CFG_REGS_PA,AM_DEVICE_MCASP0},
    {AM33X_MCASP1_CFG_REGS_PA,AM_DEVICE_MCASP1},
    {AM33X_MMCHS0_REGS_PA+0x0100,AM_DEVICE_MMCHS0},// See comment below
    {AM33X_MMCHS1_REGS_PA+0x0100,AM_DEVICE_MMCHS1},// See comment below
    {AM33X_MMCHS2_REGS_PA+0x0100,AM_DEVICE_MMCHS2},// See comment below
    {AM33X_OCP_WATCHPT_REGS_PA,AM_DEVICE_OCPWP},
    {AM33X_PKA_REGS_PA,AM_DEVICE_PKA},
    {AM33X_RNG_REGS_PA,AM_DEVICE_RNG},
    {AM33X_SHA_REGS_PA,AM_DEVICE_SHA0},
    {AM33X_SPARE0_REGS_PA,AM_DEVICE_SPARE0},
    {AM33X_SPARE1_REGS_PA,AM_DEVICE_SPARE1},
    {AM33X_MCSPI0_REGS_PA,AM_DEVICE_MCSPI0},
    {AM33X_MCSPI1_REGS_PA,AM_DEVICE_MCSPI1},
    {AM33X_SPINLOCK_REGS_PA,AM_DEVICE_SPINLOCK},
    {AM33X_GPTIMER2_REGS_PA,AM_DEVICE_TIMER2},
    {AM33X_GPTIMER3_REGS_PA,AM_DEVICE_TIMER3},
    {AM33X_GPTIMER4_REGS_PA,AM_DEVICE_TIMER4},  
    {AM33X_GPTIMER5_REGS_PA,AM_DEVICE_TIMER5},
    {AM33X_GPTIMER6_REGS_PA,AM_DEVICE_TIMER6},
    {AM33X_GPTIMER7_REGS_PA,AM_DEVICE_TIMER7},
    {AM33X_TPCC_REGS_PA,AM_DEVICE_TPCC},
    {AM33X_TPTC0_REGS_PA,AM_DEVICE_TPTC0},
    {AM33X_TPTC1_REGS_PA,AM_DEVICE_TPTC1},
    {AM33X_TPTC2_REGS_PA,AM_DEVICE_TPTC2},
    {AM33X_UART1_REGS_PA,AM_DEVICE_UART1},
    {AM33X_UART2_REGS_PA,AM_DEVICE_UART2},
    {AM33X_UART3_REGS_PA,AM_DEVICE_UART3},   
    {AM33X_UART4_REGS_PA,AM_DEVICE_UART4},
    {AM33X_UART5_REGS_PA,AM_DEVICE_UART5},
    {AM33X_USBSS_PA,AM_DEVICE_USB0},
    {AM33X_SGX530_REGS_PA,AM_DEVICE_GFX},
    {AM33X_RTCSS_REGS_PA,AM_DEVICE_RTC},
    /* Note: for Centaurus MMC, the actual base PA should be AM387X_MMCHS_REGS_PA.
             0x0100 is added before returning so that the struct OMAP_MMCHS_REGS
             defined in COMMON_TI\INC\omap_sdhc_regs.h and the sdhc.c source code 
             can be re-used without modifications.

             However the first three Centaurus SD/SDIO registers, 
                 SD_HL_REV       (0x0000  resp to AM387X_MMCHS_REGS_PA)
                 SD_HL_HWINFO    (0x0004)
                 SD_HL_SYSCONFIG (0x0010) 
             will not be covered by the OMAP_MMCHS_REGS structure.

             Also, OMAP_MMCHS_REGS does not include the following 4 Centaurus regs
                 SD_FE      (0x0250)
                 SD_ADMAES  (0x0254)
                 SD_ADMASAL (0x0258)
                 SD_ADMASAH (0x025C)

             Ref: uboot/mmc_host_def.h/OMAP_HSMMC_BASE
     */
    {        0,                             OMAP_DEVICE_NONE        }
};

const DEVICE_IRQ_MAP s_DeviceIrqMap[]=
{
    // IRQ              Device              Extra
	//{40,	AM_DEVICE_EMAC0,	L"THRS"		},
	//{41,	AM_DEVICE_EMAC0,	L"RX"		},
	//{42,	AM_DEVICE_EMAC0,	L"TX"		},
	//{43,	AM_DEVICE_EMAC0,	L"MISC"		},
	{IRQ_I2CINT0,	AM_DEVICE_I2C0,	    NULL		},
	{IRQ_I2CINT1,	AM_DEVICE_I2C1,	    NULL		},
	{IRQ_I2CINT2,	AM_DEVICE_I2C2,	    NULL		},
	{IRQ_SDINT0,	AM_DEVICE_MMCHS0,	NULL		},
	{IRQ_SDINT1,	AM_DEVICE_MMCHS1,	NULL		},
	{IRQ_SDINT2,	AM_DEVICE_MMCHS2,	NULL		},
	{IRQ_LCDCINT,	AM_DEVICE_LCDC,	    NULL		},
	{IRQ_ADC_TSC_GENINT,	AM_DEVICE_ADC_TSC,	    NULL		},
	{IRQ_UART0INT,	AM_DEVICE_UART0, 	NULL		},
	{IRQ_UART1INT,	AM_DEVICE_UART1, 	NULL		},
	{IRQ_UART2INT,	AM_DEVICE_UART2, 	NULL		},
	{IRQ_UART3INT,	AM_DEVICE_UART3,	NULL		},
	{IRQ_UART4INT,	AM_DEVICE_UART4,	NULL		},
	{IRQ_UART5INT,	AM_DEVICE_UART5,	NULL		},
	{IRQ_TIMER2,	AM_DEVICE_TIMER2,	NULL		},
	{IRQ_TIMER3,	AM_DEVICE_TIMER3,	NULL		},
	{IRQ_TIMER4,	AM_DEVICE_TIMER4,	NULL		},
	{IRQ_TIMER5,	AM_DEVICE_TIMER5,	NULL		},
	{IRQ_TIMER6,	AM_DEVICE_TIMER6,	NULL		},
	{IRQ_TIMER7,	AM_DEVICE_TIMER7,	NULL		},
    {0,             OMAP_DEVICE_NONE,   NULL        }
};
