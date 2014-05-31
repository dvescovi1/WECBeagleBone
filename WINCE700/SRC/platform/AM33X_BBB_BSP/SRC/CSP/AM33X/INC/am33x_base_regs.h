/*
================================================================================
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  Header:  am33x_base_regs.h
//

//------------------------------------------------------------------------------
//  This header file defines the addresses of the base registers for
//  the System on Chip (SoC) components.
//
//  The following abbreviations are used for different addressing type:
//
//                PA - physical address
//                UA - uncached virtual address
//                CA - cached virtual address
//                OA - offset address
//
//  The naming convention for CPU base registers is:
//
//                <SoC>_<SUBSYSTEM>_REGS_<ADDRTYPE>
//
#ifndef __AM33X_BASE_REGS_H
#define __AM33X_BASE_REGS_H

//---------- L4 Peripheral

#define AM33X_L4S_CONFIG_PA                      0x48000000

#define AM33X_L4S_LINK_AGENT_PA                  0x48000800
#define AM33X_L4S_IP0_PA                         0x48001000  // initiator port 0
#define AM33X_L4S_IP1_PA                         0x48001400  // initiator port 1
#define AM33X_L4S_IP2_PA                         0x48001800  // initiator port 2
#define AM33X_L4S_IP3_PA                         0x48001C00  // initiator port 3

#define AM33X_EFUSE_CTRL_PA                      0x48008000  
#define AM33X_EFUSE_CTRL_L4_INTERCONNECT_PA      0x48009000  

#define AM33X_UART1_REGS_PA                      0x48022000
#define AM33X_UART2_REGS_PA                      0x48024000

#define AM33X_I2C1_REGS_PA                       0x4802A000

#define AM33X_MCSPI0_REGS_PA                     0x48030000

#define AM33X_MCASP0_CFG_REGS_PA                 0x48038000
#define AM33X_MCASP1_CFG_REGS_PA                 0x4803C000

#define AM33X_GPTIMER2_REGS_PA                   0x48040000
#define AM33X_GPTIMER3_REGS_PA                   0x48042000
#define AM33X_GPTIMER4_REGS_PA                   0x48044000
#define AM33X_GPTIMER5_REGS_PA                   0x48046000
#define AM33X_GPTIMER6_REGS_PA                   0x48048000
#define AM33X_GPTIMER7_REGS_PA                   0x4804A000
#define AM33X_GPIO1_REGS_PA                      0x4804C000

#define AM33X_MMCHS0_REGS_PA                     0x48060000
#define AM33X_ELM_REGS_PA                        0x48080000
#define AM33X_SPARE0_REGS_PA                     0x480A0000

#define AM33X_MBOX_REGS_PA                       0x480C8000
#define AM33X_SPINLOCK_REGS_PA                   0x480CA000

#define AM33X_OCP_WATCHPT_REGS_PA                0x4818C000
#define AM33X_P1500_CFG_REGS_PA                  0x4818E000

#define AM33X_I2C2_REGS_PA                       0x4819C000
#define AM33X_MCSPI1_REGS_PA                     0x481A0000

#define AM33X_UART3_REGS_PA                      0x481A6000
#define AM33X_UART4_REGS_PA                      0x481A8000
#define AM33X_UART5_REGS_PA                      0x481AA000

#define AM33X_GPIO2_REGS_PA                      0x481AC000
#define AM33X_GPIO3_REGS_PA                      0x481AE000

#define AM33X_SPARE1_REGS_PA                     0x481B0000

#define AM33X_DCAN0_REGS_PA                      0x481CC000
#define AM33X_DCAN1_REGS_PA                      0x481D0000

#define AM33X_MMCHS1_REGS_PA                     0x481D8000 

//------------------------------------------------------------------------------
//  Interrupt Units
#define AM33X_INTC_MPU_REGS_PA                   0x48200000
#define AM33X_MPUSS_CONG_REGS_PA                 0x48240000

//===== new 
#define AM33X_ECAP_EQEP_EPWM0_REGS_PA            0x48300000
#define AM33X_ECAP_EQEP_EPWM1_REGS_PA            0x48302000
#define AM33X_ECAP_EQEP_EPWM2_REGS_PA            0x48304000

#define AM33X_LCDC_REGS_PA                       0x4830E000
#define AM33X_RNG_REGS_PA                        0x48310000

#define AM33X_PKA_REGS_PA                        0x48318000

//==============================================================================

//---------- L4 Fast Peripheral
#define AM33X_L4F_CFG_REGS_PA				     0x4A000000
#define AM33X_L4F_LINK_AGENT_PA                  0x4A000800
#define AM33X_L4F_IP0_PA                         0x4A001000  // initiator port 0
#define AM33X_EMACSW_REGS_PA					 0x4A100000
#define AM33X_ICSS_PRUSS1_REGS_PA				 0x4A300000


//---------- L3 memory
#define AM33X_SRAM_INTERNAL_PA                    0x402F0000
#define AM33X_OCMC0_PA                            0x40300000
#define AM33X_L3F_CFG_REGS_PA                     0x44800000
#define AM33X_L4_WKUP_REGS_PA                     0x44C00000
#define AM33X_EXP_PORT_REGS_PA                    0x45000000
#define AM33X_MCASP0_DATA_REGS_PA				  0x46000000
#define AM33X_MCASP1_DATA_REGS_PA				  0x46400000
#define AM33X_USBSS_PA							  0x47400000
#define AM33X_MMCHS2_REGS_PA                      0x47810000 

//----------L4 FW
#define AM33X_L4FW_REGS_PA                        0x47C00000 
#define AM33X_EMIFFW_REGS_PA                      0x47C0C000 


#define AM33X_TPCC_REGS_PA                        0x49000000 
#define AM33X_TPTC0_REGS_PA                       0x49800000 
#define AM33X_TPTC1_REGS_PA                       0x49900000 
#define AM33X_TPTC2_REGS_PA                       0x49A00000 

#define AM33X_DEBUGSS_REGS_PA                     0x4B000000
#define AM33X_EMIF4_0_CFG_REGS_PA		          0x4C000000
//------------------------------------------------------------------------------
//  GPMC Module Register base address
//  (see file omap_gpmc.h for offset definitions for this base address)
//  VA: Netra has a newer version of the GPMC with more registers vs. omap
//      but I hope the omap works for now !!!! 
//------------------------------------------------------------------------------
#define AM33X_GPMC_REGS_PA                       0x50000000
#define AM33X_GPMC_REGS_SIZE                     0x00001000

#define AM33X_SHA_REGS_PA                        0x53000000
#define AM33X_AES0_REGS_PA                       0x53400000

#define AM33X_ADC_TSC_DMA_REGS_PA                0x54C00000
#define AM33X_SGX530_REGS_PA                     0x56000000

//=== L4_WKUP =======================================================
#define AM33X_L4WKUP_CFG_REGS_PA				 0x44C00000
#define AM33X_M3_UMEM_REGS_PA				     0x44D00000
#define AM33X_M3_DMEM_REGS_PA				     0x44D80000
#define AM33X_PRCM_REGS_PA                       0x44E00000
#define AM33X_SPARE_REGS_PA                      0x44E03000

#define AM33X_GPTIMER0_REGS_PA                   0x44E05000
#define AM33X_GPIO0_REGS_PA                      0x44E07000
#define AM33X_UART0_REGS_PA                      0x44E09000
#define AM33X_I2C0_REGS_PA                       0x44E0B000
#define AM33X_ADC_TSC_REGS_PA                    0x44E0D000
#define AM33X_CONTROL_MODULE_REGS_PA             0x44E10000
#define AM33X_DDR_REGS_PA                        0x44E12000
#define AM33X_GPTIMER1_1MS_REGS_PA               0x44E31000
#define AM33X_WDT0_REGS_PA                       0x44E33000
#define AM33X_WDT1_REGS_PA                       0x44E35000
#define AM33X_SMRFLX0_REGS_PA                    0x44E37000
#define AM33X_SMRFLX1_REGS_PA                    0x44E39000

#define AM33X_RTCSS_REGS_PA                      0x44E3E000

//=============================================================================
#define AM33X_L4_CNTRL_MODULE_PA                 0x44E10000
//------------------------------------------------------------------------------
#define AM33X_OCP_CONF_REGS_PA				(AM33X_L4_CNTRL_MODULE_PA + 0x0000)
#define AM33X_DEVICE_BOOT_REGS_PA			(AM33X_L4_CNTRL_MODULE_PA + 0x0040)
#define AM33X_SECnFUSE_KEY_REGS_PA			(AM33X_L4_CNTRL_MODULE_PA + 0x0100)
#define AM33X_SECURE_SM_REGS_PA			    (AM33X_L4_CNTRL_MODULE_PA + 0x0300)
#define AM33X_SUPPL_DEVICE_CTRL_REGS_PA		(AM33X_L4_CNTRL_MODULE_PA + 0x0400)
#define AM33X_DEVICE_CONF_REGS_PA			(AM33X_L4_CNTRL_MODULE_PA + 0x0600)
//#define AM33X_STATUS_CONTROL_REGS_PA		 AM33X_DEVICE_CONF_REGS_PA
#define AM33X_SYSC_PADCONFS_REGS_PA		    (AM33X_L4_CNTRL_MODULE_PA + 0x0800)
#define AM33X_PINCTRL_REGS_PA				 AM33X_SYSC_PADCONFS_REGS_PA
#define AM33X_SYSC_MISC_REGS_PA			    (AM33X_L4_CNTRL_MODULE_PA + 0x0E00)
#define AM33X_SYSC_INTR_DMA_MUX_REGS_PA	    (AM33X_L4_CNTRL_MODULE_PA + 0x0F00)
#define AM33X_SYSC_MISC2_REGS_PA			(AM33X_L4_CNTRL_MODULE_PA + 0x1000)

//------------------------------------------------------------------------------
#endif // __AM33X_BASE_REGS_H
