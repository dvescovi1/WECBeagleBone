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
//  File: dma_init.c
//
//#include "am33x.h"
#include <am3xx_mcspi_regs.h>
#include <spi_priv.h>

//#ifndef UNREFERENCED_PARAMETER
//#define UNREFERENCED_PARAMETER(P) (P)

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//  Function:  SpiDmaRestore
//
//
BOOL SpiDmaRestore(SPI_INSTANCE *pInstance)
{
	UNREFERENCED_PARAMETER(pInstance);
	return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  SpiDmaInit
//
//  Called initialize DMA channels for SPI driver instance
//
BOOL SpiDmaInit(SPI_INSTANCE *pInstance)
{
UNREFERENCED_PARAMETER(pInstance);
	return FALSE;
}


//------------------------------------------------------------------------------
//
//  Function:  SpiDmaDeinit
//
//  Called free DMA channels for SPI driver instance
//
BOOL SpiDmaDeinit(SPI_INSTANCE *pInstance)
{
	UNREFERENCED_PARAMETER(pInstance);
	return FALSE;

}

//------------------------------------------------------------------------------

