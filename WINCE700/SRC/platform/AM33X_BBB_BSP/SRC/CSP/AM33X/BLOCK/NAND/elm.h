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
//  File:  gpmc_ecc.h
//
#ifndef __ELM_H
#define __ELM_H


#ifdef __cplusplus
extern "C" {
#endif


VOID
BCH8_ELM_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    );

VOID
BCH8_ELM_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );

BOOL BCH8_ELM_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    );


VOID
BCH8_ELMX_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    );

VOID
BCH8_ELMX_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );

BOOL BCH8_ELMX_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    );

void elm_init( void );

#ifdef __cplusplus
}
#endif

#endif // __ELM_H
//-----------------------------------------------------------------------------
