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

//-----------------------------------------------------------------------------
//
//  File:  omap35xx_sr.h
//
#ifndef __OMAP35XX_SR_H
#define __OMAP35XX_SR_H

//-----------------------------------------------------------------------------
//
// SmartReflex register
//
typedef volatile struct {

   union{
   	struct{
	   UINT SRCONFIG;               // offset 0x00
	   UINT SRSTATUS;               // offset 0x04
	   UINT SENVAL;                 // offset 0x08
	   UINT SENMIN;                 // offset 0x0c
	   UINT SENMAX;                 // offset 0x10
	   UINT SENAVG;                 // offset 0x14
	   UINT AVGWEIGHT;              // offset 0x18
	   UINT NVALUERECIPROCAL;       // offset 0x1c
	   UINT SENERROR;               // offset 0x20
	   UINT ERRCONFIG;              // offset 0x24
       } omap_35xx;
	struct{
	   UINT SRCONFIG;               // offset 0x00
	   UINT SRSTATUS;               // offset 0x04
	   UINT SENVAL;                 // offset 0x08
	   UINT SENMIN;                 // offset 0x0c
	   UINT SENMAX;                 // offset 0x10
	   UINT SENAVG;                 // offset 0x14
	   UINT AVGWEIGHT;              // offset 0x18
	   UINT NVALUERECIPROCAL;       // offset 0x1c
	   UINT RESERVED;               // offset 0x20
	   UINT IRQSTATUS_RAW;   // offset 0x24
	   UINT IRQSTATUS;            // offset 0x28
	   UINT IRQENABLE_SET;    // offset 0x2C
	   UINT IRQENABLE_CLR;    // offset 0x30
	   UINT SENERROR;             // offset 0x34
	   UINT ERRCONFIG;           // offset 0x38
	}omap_37xx;
    }sr_regs;
} OMAP_SMARTREFLEX;


//-----------------------------------------------------------------------------
#define SRCONFIG_CLKCTRL_SHIFT      (0)
#define SRCONFIG_CLKCTRL_MASK       (0x3 << SRCONFIG_CLKCTRL_SHIFT)
#define SRCONFIG_DELAYCTRL          (1 << 2)
#define SRCONFIG_SENPENABLE_SHIFT   (3)
#define SRCONFIG_SENPENABLE_MASK    (0x3 << SRCONFIG_SENPENABLE_SHIFT)
#define SRCONFIG_SENNENABLE_SHIFT   (5)
#define SRCONFIG_SENNENABLE_MASK    (0x3 << SRCONFIG_SENNENABLE_SHIFT)
#define SRCONFIG_MINMAXAVG_EN       (1 << 8)
#define SRCONFIG_ERRORGEN_EN        (1 << 9)
#define SRCONFIG_SENENABLE          (1 << 10)
#define SRCONFIG_SRENABLE           (1 << 11)
#define SRCONFIG_SRCLKLENGTH_SHIFT  (12)
#define SRCONFIG_SRCLKLENGTH_MASK   (0x3FF << SRCONFIG_SRCLKLENGTH_SHIFT)
#define SRCONFIG_ACCUMDATA_SHIFT    (22)
#define SRCONFIG_ACCUMDATA_MASK     (0x3FF << SRCONFIG_ACCUMDATA_SHIFT)

#define SRSTATUS_MINMAXAVGACCUMVALID (1 << 0)
#define SRSTATUS_ERRGEN_VALID       (1 << 1)
#define SRSTATUS_MINMAXAVGVALID     (1 << 2)
#define SRSTATUS_AVGERRVALID        (1 << 3)

#define SENVAL_SENNVAL_SHIFT        (0)
#define SENVAL_SENNVAL_MASK         (0xFFFF << SENVAL_SENNVAL_SHIFT)
#define SENVAL_SENPVAL_SHIFT        (16)
#define SENVAL_SENPVAL_MASK         (0xFFFF << SENVAL_SENPVAL_SHIFT)

#define SENMIN_SENNMIN_SHIFT        (0)
#define SENMIN_SENNMIN_MASK         (0xFFFF << SENMIN_SENNMIN_SHIFT)
#define SENMIN_SENPMIN_SHIFT        (16)
#define SENMIN_SENPMIN_MASK         (0xFFFF << SENMIN_SENPMIN_SHIFT)

#define SENMAX_SENNMAX_SHIFT        (0)
#define SENMAX_SENNMAX_MASK         (0xFFFF << SENMAX_SENNMAX_SHIFT)
#define SENMAX_SENPMAX_SHIFT        (16)
#define SENMAX_SENPMAX_MASK         (0xFFFF << SENMAX_SENPMAX_SHIFT)

#define SENAVG_SENNAVG_SHIFT        (0)
#define SENAVG_SENNAVG_MASK         (0xFFFF << SENAVG_SENNAVG_SHIFT)
#define SENAVG_SENPAVG_SHIFT        (16)
#define SENAVG_SENPAVG_MASK         (0xFFFF << SENAVG_SENPAVG_SHIFT)

#define AVGWEIGHT_SENNAVGWEIGHT_SHIFT (0)
#define AVGWEIGHT_SENNAVGWEIGHT_MASK (0x3 << AVGWEIGHT_SENNAVGWEIGHT_SHIFT)
#define AVGWEIGHT_SENPAVGWEIGHT_SHIFT (2)
#define AVGWEIGHT_SENPAVGWEIGHT_MASK (0x3 << AVGWEIGHT_SENPAVGWEIGHT_SHIFT)

#define NVALUERECIPROCAL_RNSENN_SHIFT (0)
#define NVALUERECIPROCAL_RNSENN_MASK (0xFF << NVALUERECIPROCAL_RNSENN_SHIFT)
#define NVALUERECIPROCAL_RNSENP_SHIFT (8)
#define NVALUERECIPROCAL_RNSENP_MASK (0xFF << NVALUERECIPROCAL_RNSENP_SHIFT)
#define NVALUERECIPROCAL_SENNGAIN_SHIFT (16)
#define NVALUERECIPROCAL_SENNGAIN_MASK (0xF << NVALUERECIPROCAL_SENNGAIN_SHIFT)
#define NVALUERECIPROCAL_SENPGAIN_SHIFT (20)
#define NVALUERECIPROCAL_SENPGAIN_MASK (0xF << NVALUERECIPROCAL_SENPGAIN_SHIFT)

#define SENERROR_SENERR0R_SHIFT     (0)
#define SENERROR_SENERROR_MASK      (0xFF << SENERROR_SENERR0R_SHIFT)
#define SENERROR_AVGERR0R_SHIFT     (8)
#define SENERROR_AVGERROR_MASK      (0xFF << SENERROR_AVGERR0R_SHIFT)

#define ERRCONFIG_ERRMINLIMIT_SHIFT (0)
#define ERRCONFIG_ERRMINLIMIT_MASK  (0xFF << ERRCONFIG_ERRMINLIMIT_SHIFT)
#define ERRCONFIG_ERRMAXLIMIT_SHIFT (8)
#define ERRCONFIG_ERRMAXLIMIT_MASK  (0xFF << ERRCONFIG_ERRMAXLIMIT_SHIFT)
#define ERRCONFIG_ERRWEIGHT_SHIFT   (16)
#define ERRCONFIG_ERRWEIGHT_MASK    (0x7 << ERRCONFIG_ERRWEIGHT_SHIFT)
#define ERRCONFIG_CLKACTIVITY_SHIFT (20)
#define ERRCONFIG_CLKACTIVITY_MASK  (0x3 << ERRCONFIG_CLKACTIVITY_SHIFT)
#define ERRCONFIG_MCU_DISACKINT_ST  (1 << 22)
#define ERRCONFIG_MCU_DISACKINT_EN  (1 << 23)
#define ERRCONFIG_MCU_BOUNDINT_ST   (1 << 24)
#define ERRCONFIG_MCU_BOUNDINT_EN   (1 << 25)
#define ERRCONFIG_MCU_VALIDINT_ST   (1 << 26)
#define ERRCONFIG_MCU_VALIDINT_EN   (1 << 27)
#define ERRCONFIG_MCU_ACCUMINT_ST   (1 << 28)
#define ERRCONFIG_MCU_ACCUMINT_EN   (1 << 29)
#define ERRCONFIG_VP_BOUNDINT_ST    (1 << 30)
#define ERRCONFIG_VP_BOUNDINT_EN    (1 << 31)
#define ERRCONFIG_INTR_SR_MASK      (ERRCONFIG_MCU_DISACKINT_ST | \
                                     ERRCONFIG_MCU_BOUNDINT_ST |  \
                                     ERRCONFIG_MCU_VALIDINT_ST |  \
                                     ERRCONFIG_MCU_ACCUMINT_ST |  \
                                     ERRCONFIG_VP_BOUNDINT_ST)

#define SR_CLKACTIVITY_NOIDLE       (0x2)
#define SR_CLKACTIVITY_IDLE         (0x0)

/* 37xx specific SR registers */
#define SRCONFIG_SENPENABLE_SHIFT_37XX   (0)
#define SRCONFIG_SENPENABLE_MASK_37XX    (0x1 << SRCONFIG_SENPENABLE_SHIFT_37XX)
#define SRCONFIG_SENNENABLE_SHIFT_37XX   (1)
#define SRCONFIG_SENNENABLE_MASK_37XX    (0x1 << SRCONFIG_SENNENABLE_SHIFT_37XX)

#define IRQENABLE_SET_MCUACCUMINTENA (0x1<<3)
#define IRQENABLE_SET_MCUVALIDINTENA (0x1<<2)
#define IRQENABLE_SET_MCUBOUNDSINTENA (0x1<<1)
#define IRQENABLE_SET_MCUDISABLEACKINTSTATENA (0x1<<0)

#define IRQENABLE_CLR_MCUACCUMINTENA (0x1<<3)
#define IRQENABLE_CLR_MCUVALIDINTENA (0x1<<2)
#define IRQENABLE_CLR_MCUBOUNDSINTENA (0x1<<1)
#define IRQENABLE_CLR_MCUDISABLEACKINTSTATENA (0x1<<0)

#define IRQENABLE_MASK (IRQENABLE_CLR_MCUACCUMINTENA | \
	                                            IRQENABLE_CLR_MCUVALIDINTENA | \
	                                            IRQENABLE_CLR_MCUBOUNDSINTENA | \
	                                            IRQENABLE_CLR_MCUDISABLEACKINTSTATENA)

#define ERRCONFIG_CLKACTIVITY_SHIFT_37XX (24)
#define ERRCONFIG_CLKACTIVITY_MASK_37XX  (0x3 << ERRCONFIG_CLKACTIVITY_SHIFT_37XX)
#define ERRCONFIG_VP_BOUNDINT_ST_37XX    (1 << 23)
#define ERRCONFIG_VP_BOUNDINT_EN_37XX    (1 << 22)

#endif // __OMAP35XX_SR_H
