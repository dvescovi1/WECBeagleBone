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
//  File: tps6591x_voltage.c
//
//

#include "triton.h"
#include "twl.h"
#include "tps65217.h"

static HANDLE g_hTwl = NULL;

BOOL ValidateTWLHandle()
{
	if ( g_hTwl == NULL)
	{
		g_hTwl = TWLOpen();
	}
	return (g_hTwl != NULL);
}


/**
 *  TWLProtWriteRegs() - Generic function that can write a TPS65217 PMIC
 *                         register or bit field regardless of protection
 *                         level.
 *
 *  @prot_level:        Register password protection.
 *                      use PROT_LEVEL_NONE, PROT_LEVEL_1, or PROT_LEVEL_2
 *  @dest_reg:          Register address to write.
 *  @dest_val:          Value to write.
 *  @mask:              Bit mask (8 bits) to be applied.  Function will only
 *                      change bits that are set in the bit mask.
 *
 *  @return:            TRUE for success, FALSE for failure.
 */
BOOL TWLProtWriteRegs(unsigned char prot_level, unsigned char dest_reg,
        unsigned char dest_val, unsigned char mask)
{
    unsigned char read_val;
    unsigned char xor_reg = 0;

    if (ValidateTWLHandle()==FALSE) return FALSE;

    /* if we are affecting only a bit field, read dest_reg and apply the mask */
    if (mask != MASK_ALL_BITS) {                
        if (TWLReadByteReg(g_hTwl,dest_reg,&read_val)==FALSE)
            return FALSE;
        read_val &= (~mask);
        read_val |= (dest_val & mask);
        dest_val = read_val;
    }

    if (prot_level > 0) {
        xor_reg = dest_reg ^ PASSWORD_UNLOCK;                
        if (TWLWriteByteReg(g_hTwl, PASSWORD, xor_reg)==FALSE)
            return FALSE;
    }
    
    if (TWLWriteByteReg(g_hTwl, dest_reg, dest_val)==FALSE)
        return FALSE;

    if (prot_level == PROT_LEVEL_2) {
        if (TWLWriteByteReg(g_hTwl, PASSWORD, xor_reg)==FALSE)
            return FALSE;

        if (TWLWriteByteReg(g_hTwl, dest_reg, dest_val)==FALSE)
            return FALSE;
    }

    return TRUE;
}


BOOL TWLUpdateVoltage(unsigned char dc_cntrl_reg, unsigned char volt_sel)
{
    
    BOOL rc = FALSE;  
    
    if (ValidateTWLHandle()==FALSE) return FALSE;

    if ((dc_cntrl_reg != DEFDCDC1) && (dc_cntrl_reg != DEFDCDC2) && (dc_cntrl_reg != DEFDCDC3)) {
        //OALMSG(1,(L"TWLUpdateVoltage: wrong dc_cntrl_reg\r\n"));                
        return rc;
    }    

    /* set voltage level */
    if (TWLProtWriteRegs(PROT_LEVEL_2, dc_cntrl_reg, volt_sel, MASK_ALL_BITS)==FALSE){
        //OALMSG(1,(L"TWLUpdateVoltage: dc_cntrl_reg not set\r\n"));                
        return rc;
    }

    /* set GO bit to initiate voltage transition */
    if (TWLProtWriteRegs(PROT_LEVEL_2, DEFSLEW, DCDC_GO, DCDC_GO)==FALSE){
        //OALMSG(1,(L"TWLUpdateVoltage: DEFSLEW not set\r\n"));                
        return rc;
    }
    
    return TRUE;
}


BOOL TWLGetStatusReg(unsigned char * buf)
{
    if (ValidateTWLHandle()==FALSE) return FALSE;
    return TWLReadByteReg(g_hTwl,STATUS,buf);
}

