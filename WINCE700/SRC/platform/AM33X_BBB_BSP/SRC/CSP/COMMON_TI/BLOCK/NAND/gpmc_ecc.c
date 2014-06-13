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
//  File:  gpmc_ecc.c
//
#include "omap.h"
#include "omap_gpmc_regs.h"
#include "omap_prcm_regs.h"
#include "gpmc_ecc.h"


#define DATA_BLOCK_LEN  512 // Ecc calculations is done in multiple of 512 B.

#define ECC_BUFF_LEN    3   // 24-bit ECC

// Count of bits set in xor determines the type of error
#define NO_ERRORS                0
#define ECC_ERROR                1
#define CORRECTABLE_ERROR       12
#define ERASED_SECTOR           24

omap_gpmc_ecc_Func_t* gpmc_ecc_func = NULL;

static VOID
BCH4_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );
//-----------------------------------------------------------------------------
static VOID
BCH8_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    );

//-----------------------------------------------------------------------------
// Function:       CountNumberOfOnes()
//
// Description:    Counts the number of bits that are "1" in a byte.
//
// Returns:        Number of bits that are "1".
//
static __inline 
UCHAR 
CountNumberOfOnes(
    DWORD num
    )
{
    UCHAR count = 0;
    while(num)
        {
        num=num&(num-1);
        count++;
        }

    return count;
}

//-----------------------------------------------------------------------------
static VOID
Hamming_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    )
{
    UNREFERENCED_PARAMETER(xfer_mode); 

    //  Configure ECC calculator engine for NAND part
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONFIG, configMask);

    //  Set ECC field sizes
    OUTREG32(&pGpmcRegs->GPMC_ECC_SIZE_CONFIG, 0x3fcff000);

    //  Select result reg 1 and clear results
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_CLEAR);
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_POINTER1);

    //  Enable ECC engine
    SETREG32(&pGpmcRegs->GPMC_ECC_CONFIG, GPMC_ECC_CONFIG_ENABLE);
}

//-----------------------------------------------------------------------------
static VOID
Hamming_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    UINT    regIndex = 0;
    UINT8   eccIndex;
    UINT32  regVal;

    // the ecc engine is setup encode 512 bytes at a time
    // so reading a sectore of 2048 bytes will require 4 sets of encoded
    // groups
    
    for (eccIndex=0; eccIndex < size;)
        {
        regVal = INREG32(((UINT32*)&pGpmcRegs->GPMC_ECC1_RESULT) + regIndex);

        // ECC-x[0] where x is from A-D
        pEcc[eccIndex++] = (BYTE) ECC_P1_128_E(regVal);

        // ECC-x[1] where x is from A-D
        pEcc[eccIndex++] = (BYTE) ECC_P1_128_O(regVal);

        // ECC-x[2] where x is from A-D
        pEcc[eccIndex++] = (BYTE) (ECC_P512_2048_E(regVal)|ECC_P512_2048_O(regVal)<<4);

        // read next ecc register
        regIndex++;
        }

    return;
}

//-----------------------------------------------------------------------------
static VOID
Hamming_ECC_Reset(
    OMAP_GPMC_REGS *pGpmcRegs
    )
{
    //  Disable ECC engine
    CLRREG32(&pGpmcRegs->GPMC_ECC_CONFIG, GPMC_ECC_CONFIG_ENABLE);
}

//-----------------------------------------------------------------------------
// Function:    ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
static BOOL Hamming_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{
    BOOL rc = FALSE;
    int   i;
    int numOnes;
    DWORD ECCxor[ECC_BUFF_LEN];
    UCHAR mask;
    DWORD byteLocation;
    DWORD bitLocation;
    UCHAR count;
    BOOL  bCorrect;

    //  ECC calculated for every 512 bytes of data
    UINT numberOfSectors = (sizeData/DATA_BLOCK_LEN);

    UNREFERENCED_PARAMETER(pGpmcRegs);


    //----- 1. Check passed parameters -----
    for(count=0; count < numberOfSectors; count++ )
        {
        //----- 2. XOR the existing ECC info with the new ECC info -----
        for(i = 0; i < ECC_BUFF_LEN; i++)
            {
                ECCxor[i] = *(pEccNew+i) ^ *(pEccOld+i);
            }

        //----- 3. Determine if this is a single-bit error that can be corrected -----
        //         NOTE: The total number of bits equal to '1' in the XORed Hamming
        //               Codes determines if the error can be corrected.
        numOnes = 0;
        for(i = 0; i < ECC_BUFF_LEN; i++)
            {
            numOnes += CountNumberOfOnes(ECCxor[i]);
            }

        switch( numOnes )
            {
            case NO_ERRORS:
            case ECC_ERROR:
            case ERASED_SECTOR:
                //  No error in the data
                bCorrect = FALSE;
                break;
            
            case CORRECTABLE_ERROR:
                //  Single bit error; correctable
                DEBUGMSG(ZONE_ERROR, (L" 1bit error is detected\r\n"));
                
                bCorrect = TRUE;
                break;
            
            default:
                //  More than 1 bit error
                DEBUGMSG(ZONE_ERROR, (L"more than 1bit errors are detected\r\n"));
                rc = FALSE;
                goto cleanUp;
                break;
            }
            
            
        //----- 4. Compute the location of the single-bit error -----
        if( bCorrect )
            {
            // Note: This is how the ECC is layed out in the ECC buffers.
            // ECCxor[0] = P128e  P64e   P32e   P16e   P8e    P4e    P2e    P1e
            // ECCxor[1] = P128o  P64o   P32o   P16o   P8o    P4o    P2o    P1o
            // ECCxor[2] = P2048o P1024o P512o  P256o  P2048e P1024e P512e  P256e

            // Combine the 'o' xor'ed values to get row and column
            byteLocation = ((ECCxor[2] & 0xF0) << 1) | (ECCxor[1] >> 3);
            bitLocation = ECCxor[1] & 0x7;

            //----- 5. Correct the single-bit error (set the bit to its complementary value) -----
            mask = (UCHAR) (0x01 << bitLocation);
            if(pData[byteLocation] & mask)
                {
                pData[byteLocation] &= ~mask;       // 0->1 error, set bit to 0
                }
            else
                {
                pData[byteLocation] |= mask;        // 1->0 error, set bit to 1
                }
            }

            
        //  Advance pointers
        pEccOld += ECC_BUFF_LEN;                // Pointer to the ECC on flash
        pEccNew += ECC_BUFF_LEN;                // Pointer to the ECC the caller calculated
        pData += DATA_BLOCK_LEN;
        }

    rc = TRUE;
    
cleanUp:
    return rc;
}

/* Implementation for 4b/8b BCH correction.  Pass either 4 or 8 into the correct_bits
   parameter. */
static int BCH_correct_data(OMAP_GPMC_REGS *pGpmcRegs, UINT8 *dat,
				 UINT8 *read_ecc, UINT8 *calc_ecc, int correct_bits)
{
    int i=0, blockCnt=4, j, eccCheck, count, corrected=0;
    int eccsize = (correct_bits == 8) ? ECC_BYTES_BCH8/4 : ECC_BYTES_BCH4/4;
    int mode = (correct_bits == 8) ? 1 : 0;
    unsigned int err_loc[8];

    if(correct_bits == 8)
        BCH8_ECC_Calculate(pGpmcRegs, calc_ecc, ECC_BYTES_BCH8);	
    else
        BCH4_ECC_Calculate(pGpmcRegs, calc_ecc, ECC_BYTES_BCH4);	

    for (i = 0; i < blockCnt; i++) 
    {
        /* check if any ecc error */
        eccCheck = 0;
        for (j = 0; (j < eccsize) && (eccCheck == 0); j++)
    	 {
            if (calc_ecc[j] != 0)
                eccCheck = 1;
    	 }
        if (eccCheck == 1) 
        {
            eccCheck = 0;
            for (j = 0; (j < eccsize) && (eccCheck == 0); j++)
                if (read_ecc[j] != 0xFF)
                    eccCheck = 1;
        }
        if (eccCheck == 1) 
        {
            count = decode_bch(mode, calc_ecc, err_loc);
            DEBUGMSG(ZONE_ERROR, (L"...bch correct(%d 512 byte) done, count=%d\r\n", i+1, count));
            if (count > correct_bits)
                return -1;

            /* When the error bits are in ECC bytes itself, decode_bch() returns -1, this condition will be
                 ignored until we get improved BCH decoder  */
	     if ((count < 0) && (count !=-1))
                return count;
            
            for (j = 0; j < count; j++) 
            {
                DEBUGMSG(ZONE_ERROR, (L"err_loc=%x\r\n", err_loc[j]));
		  if((err_loc[j] / 8)<DATA_BLOCK_LEN)		
                    dat[err_loc[j] / 8] ^=  (0x01 << (err_loc[j] % 8));
                corrected++;
            }
        }
        
        calc_ecc = calc_ecc + eccsize;
        read_ecc = read_ecc + eccsize;
        dat += DATA_BLOCK_LEN;
    }
    
    return corrected;
}



/* BCH4_ECC_Init 

BCH4 configuration - ECC is located at offset 2 bytes from the beginning of OOB
           Wrapmode       size0         size1 
Read          9                0x4           0xd
Write          6                0x0           0x20

BCH4 configuration - ECC is located at the end of OOB (offset 36 bytes)
           Wrapmode       size0         size1 
Read          9                0x48         0xd
Write          6                0x0           0x20

*/

//-----------------------------------------------------------------------------
static VOID
BCH4_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask, /* bus_width and cs configuration */
    UINT xfer_mode
    )
{
    UINT32 ecc_conf , ecc_size_conf=0;

    ecc_conf = configMask | GPMC_ECC_CONFIG_BCH | GPMC_ECC_CONFIG_BCH4; 
	
    switch (xfer_mode) 
    {
        case NAND_ECC_READ:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0xD << 22) | (0x4 << 12);
          ecc_conf |= ( (0x09 << 8) | GPMC_ECC_CONFIG_ENABLE);
          break;
		  
        case NAND_ECC_WRITE:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0x20 << 22) | (0x00 << 12);
          ecc_conf |= ((0x06 << 8) |GPMC_ECC_CONFIG_ENABLE);
          break;
		  
        default:
          RETAILMSG(1, (L"Error: Unrecognized Mode[%d]!\r\n", xfer_mode));
          break;
    }

    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_POINTER1);
    //  Set ECC field sizes
    OUTREG32(&pGpmcRegs->GPMC_ECC_SIZE_CONFIG, ecc_size_conf);

    //  Select result reg 1 and clear results
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_CLEAR | GPMC_ECC_CONTROL_POINTER1);


    //  Configure ECC calculator engine for NAND part
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONFIG, ecc_conf );

}

//-----------------------------------------------------------------------------
static VOID
BCH4_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    UINT8   eccIndex=0;
    UINT32  regVal1, regVal2, i;

    if (size < ECC_BYTES_BCH4) return;
	
    // the ecc engine is setup encode 512 bytes at a time
    // so reading a sectore of 2048 bytes will require 4 sets of encoded
    // groups
    
    for (i=0; i < 4; i++)
    {
        /* Reading HW ECC_BCH_Results
         * 0x240-0x24C, 0x250-0x25C, 0x260-0x26C, 0x270-0x27C
         */
        regVal1 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT0);
        regVal2 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT1);
        

        pEcc[eccIndex++] =  (BYTE)((regVal2 >> 16) & 0xFF);
        pEcc[eccIndex++] = (BYTE)((regVal2 >> 8) & 0xFF);
        pEcc[eccIndex++] = (BYTE)(regVal2 & 0xFF);
        pEcc[eccIndex++] = (BYTE)((regVal1 >> 24) & 0xFF);
        pEcc[eccIndex++] = (BYTE)((regVal1 >> 16) & 0xFF);
        pEcc[eccIndex++] = (BYTE)((regVal1 >> 8) & 0xFF);
        pEcc[eccIndex++] = (BYTE)(regVal1 & 0xFF);

    }

    return;
}

//-----------------------------------------------------------------------------
static VOID
BCH4_ECC_Reset(
    OMAP_GPMC_REGS *pGpmcRegs
    )
{
    //  Disable ECC engine
    CLRREG32(&pGpmcRegs->GPMC_ECC_CONFIG, GPMC_ECC_CONFIG_ENABLE);
}

//-----------------------------------------------------------------------------
// Function:    ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
static BOOL BCH4_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{

    int ret;
    UNREFERENCED_PARAMETER(sizeData); 

    ret = BCH_correct_data(pGpmcRegs, pData, (BYTE *)pEccOld, (BYTE *)pEccNew , 4);

    return (ret>=0? TRUE : FALSE);

}

/* BCH8_ECC_Init 

BCH8 configuration - ECC is located at offset 2 bytes from the beginning of OOB
           Wrapmode       size0         size1 
Read          4                0x4           0x1A
Write          6                0x0           0x20

BCH8 configuration - ECC is located at the end of OOB (offset 36 bytes)
           Wrapmode       size0         size1 
Read          4                0x18         0x1A
Write          6                0x0           0x20

*/

//-----------------------------------------------------------------------------
static VOID
BCH8_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    )
{
    UINT32 ecc_conf , ecc_size_conf=0;

    ecc_conf = configMask | GPMC_ECC_CONFIG_BCH | GPMC_ECC_CONFIG_BCH8; 
	
    switch (xfer_mode) 
    {
        case NAND_ECC_READ:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0x1A << 22) | (0x4 << 12);  
          ecc_conf |= ((0x04 << 8)  | (0x1));
          break;
		  
        case NAND_ECC_WRITE:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0x20 << 22) | (0x00 << 12);
          ecc_conf |= ((0x06 << 8)  |(0x1));
          break;
		  
        default:
          RETAILMSG(1, (L"Error: Unrecognized Mode[%d]!\r\n", xfer_mode));
          break;
    }


    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_POINTER1);
    //  Set ECC field sizes
    OUTREG32(&pGpmcRegs->GPMC_ECC_SIZE_CONFIG, ecc_size_conf);

    //  Configure ECC calculator engine for NAND part
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONFIG, ecc_conf );

    //  Select result reg 1 and clear results
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_CLEAR | GPMC_ECC_CONTROL_POINTER1);
}


//-----------------------------------------------------------------------------
static VOID
BCH8_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    UINT8   eccIndex=0;
    UINT32  regVal1, regVal2, regVal3, regVal4, i;

    if(size < ECC_BYTES_BCH8) return;
	
    // the ecc engine is setup encode 512 bytes at a time
    // so reading a sectore of 2048 bytes will require 4 sets of encoded
    // groups
    
    for (i=0;i<4;i++)
    {
        /* Reading HW ECC_BCH_Results
         * 0x240-0x24C, 0x250-0x25C, 0x260-0x26C, 0x270-0x27C
         */
        regVal1 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT0);
        regVal2 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT1);
        regVal3 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT2);
        regVal4 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT3);

        pEcc[eccIndex++]  = (BYTE)(regVal4 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal3 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 24) & 0xFF);
   
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal2 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal1 & 0xFF);

    }

    return;
}

//-----------------------------------------------------------------------------
static VOID
BCH8_ECC_Reset(
    OMAP_GPMC_REGS *pGpmcRegs
    )
{
    //  Disable ECC engine
    CLRREG32(&pGpmcRegs->GPMC_ECC_CONFIG, GPMC_ECC_CONFIG_ENABLE);
}

//-----------------------------------------------------------------------------
// Function:    ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
static BOOL BCH8_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{
    int ret;
    BYTE *read_ecc, *cal_ecc;
    
	
    UNREFERENCED_PARAMETER(sizeData); 

    read_ecc = (BYTE *)pEccOld;
    cal_ecc = (BYTE *)pEccNew; 

    ret = BCH_correct_data(pGpmcRegs, pData, read_ecc, cal_ecc, 8);

    return (ret>=0? TRUE : FALSE);
}

//-----------------------------------------------------------------------------


omap_gpmc_ecc_Func_t gpmc_ecc_hamming_code =
{
    &Hamming_ECC_Init,
    &Hamming_ECC_Calculate,	
    &Hamming_ECC_Reset,    
    &Hamming_ECC_CorrectData
};

omap_gpmc_ecc_Func_t gpmc_ecc_bch_4bits =
{
    &BCH4_ECC_Init,
    &BCH4_ECC_Calculate,	
    &BCH4_ECC_Reset,    
    &BCH4_ECC_CorrectData
};

omap_gpmc_ecc_Func_t gpmc_ecc_bch_8bits =
{
    &BCH8_ECC_Init,
    &BCH8_ECC_Calculate,	
    &BCH8_ECC_Reset,    
    &BCH8_ECC_CorrectData
};


//-----------------------------------------------------------------------------
VOID
ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    EccType_e ECC_mode,
    UINT xfer_mode
    )
{
    /* Initialize function table then call ecc_init */
    switch(ECC_mode)
    {
        case Hamming1bit:
            gpmc_ecc_func = &gpmc_ecc_hamming_code;
	    break;

        case BCH4bit:
            gpmc_ecc_func = &gpmc_ecc_bch_4bits;
	    break;

        case BCH8bit:
            gpmc_ecc_func = &gpmc_ecc_bch_8bits;
	    break;

        default:
           RETAILMSG(TRUE, (L"ECC_Init: unsupported ecc mode:%d\r\n", ECC_mode));
	    break;
    }
    if(gpmc_ecc_func != NULL)
        gpmc_ecc_func->ecc_init(pGpmcRegs, configMask, xfer_mode);
}

//-----------------------------------------------------------------------------
VOID
ECC_Result(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    if(gpmc_ecc_func != NULL)
        gpmc_ecc_func->ecc_calculate(pGpmcRegs, pEcc, size);
}

//-----------------------------------------------------------------------------
VOID
ECC_Reset(
    OMAP_GPMC_REGS *pGpmcRegs
    )
{
    if(gpmc_ecc_func != NULL)
        gpmc_ecc_func->ecc_reset(pGpmcRegs);
}

//-----------------------------------------------------------------------------
// Function:    ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
BOOL ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{
    BOOL rc=FALSE;
	
    if(gpmc_ecc_func != NULL)
        rc = gpmc_ecc_func->ecc_correct_data(pGpmcRegs, pData, sizeData, pEccOld, pEccNew);
	
    return rc;
}

//-----------------------------------------------------------------------------


