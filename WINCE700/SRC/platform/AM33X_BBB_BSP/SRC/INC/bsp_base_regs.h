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
//  File:  bsp_base_regs.h
//
//  This header file defines the addresses of the base registers for
//  the board components.
//  The mask addresses are used to define the size of the Chip-selects mapping.
//  Be careful not to overlap the base addresses for the different Chip-selects.
//  Also, make sure the base addresses are properly aligned depending on the CS size.

#ifndef __BSP_BASE_REGS_H
#define __BSP_BASE_REGS_H

//------------------------------------------------------------------------------
//  NAND flash memory location
//------------------------------------------------------------------------------

#define BSP_NAND_REGS_PA			0x08000000
#define BSP_NAND_MASKADDRESS		GPMC_MASKADDRESS_16MB

#endif
