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
#ifndef __GPMC_ECC_H
#define __GPMC_ECC_H

//------------------------------------------------------------------------------
//  ECC Macros
#define ECC_P1_128_E(val)       ((val) & 0xFF)          /*Bit 0 to 7 */
#define ECC_P1_128_O(val)       ((val >> 16) & 0xFF)    /* Bit 16 to Bit 23*/
#define ECC_P512_2048_E(val)    ((val >> 8) & 0x0F)     /* Bit 8 to 11 */
#define ECC_P512_2048_O(val)    ((val >> 24) & 0x0F)    /* Bit 24 to Bit 27 */

#define ECC_BYTES_HAMMING   (12)
#define ECC_BYTES_BCH4          (28)
#define ECC_BYTES_BCH8          (52)
#define ECC_BYTES                    ECC_BYTES_BCH8    // number of bytes used for ecc calculation. 
#define ECC_OFFSET                   (2)     //ecc location in oob area expected by BootROM in bytes
#define SKIP_ECC_WRITE_MAGIC_NUMBER    0x0f
//-----------------------------------------------------------------------------
// defines a set of gpmc based ecc routines

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    Hamming1bit=0,
    BCH4bit,
    BCH8bit
}EccType_e;

typedef enum {
    NAND_ECC_READ=0,
    NAND_ECC_WRITE
}EccXfer_mode_e;

typedef struct {
	
void (*ecc_init)(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    );

void (*ecc_calculate)(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );	

void (*ecc_reset)(
    OMAP_GPMC_REGS *pGpmcRegs
    );

BOOL (*ecc_correct_data)(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    );
}omap_gpmc_ecc_Func_t;
	
VOID
ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    EccType_e ECC_mode,
    UINT xfer_mode
    );

VOID
ECC_Result(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );

VOID
ECC_Reset(
    OMAP_GPMC_REGS *pGpmcRegs
    );

BOOL 
ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    );


int decode_bch(int select_4_8, unsigned char *ecc, unsigned int *err_loc);


#ifdef __cplusplus
}
#endif

#endif // __GPMC_ECC_H
//-----------------------------------------------------------------------------
