// All rights reserved ADENEO EMBEDDED 2010
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
//  DMA helper routines.
//
#ifndef __DMA_UTILITY_H
#define __DMA_UTILITY_H

/* DMA is used in several common drivers, such as SDHC and display
    However OMAP35x and AM35x SOCs use SDMA IP module , and 
    AM387x, AM389x and AM335x SOCs use EDMA IP module 

    This file contains the two different implementation interface to SDMA and
    EDMA

    Unfortunately, it uses BSP_XXX flags here.  If better solution is available,
    this should be cleaned up. */

    
#ifdef BSP_EDMA
#include "edma_utility.h"
#elif BSP_OMAP_SDMA
#include "omap_sdma_utility.h"
#include "omap_sdma_regs.h"
#else
#include "omap_sdma_utility.h"
#include "omap_sdma_regs.h"
//If the chip has different DMA IP module, please modify this file to use the new DMA engine. 
//#error "Neither EDMA nor SDMA is selected to use DMA utilities."
#endif

#endif //__DMA_UTILITY_H

