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
#include "tps6591x.h"

static HANDLE g_hTwl = NULL;

BOOL ValidateHandle()
{
	BYTE twlDevCtrlValue;

	if ( g_hTwl == NULL)
	{
		g_hTwl = TWLOpen();

		// access smartreflex registers via ctl I2C
		if (TWLReadRegs(g_hTwl, PMIC_DEVCTRL_REG, &twlDevCtrlValue, sizeof(twlDevCtrlValue)))
		{
			twlDevCtrlValue |=  PMIC_DEVCTRL_REG_SR_CTL_I2C_SEL_CTL_I2C;
			TWLWriteRegs(g_hTwl, PMIC_DEVCTRL_REG, &twlDevCtrlValue, sizeof(twlDevCtrlValue));
		}
		else
		{
			g_hTwl = NULL;
		}
	}
	return (g_hTwl != NULL);
}


BOOL TWLSetOPVoltage(UINT voltage,UINT32 mv)
{
    
    BOOL rc = FALSE;  
    BYTE buf;
    UINT32 reg=0;
    
	if (!ValidateHandle())
		goto cleanup;
	
	switch (voltage) {
        case kVdd1:
            reg = PMIC_VDD1_OP_REG;
        break;
        case kVdd2:
            reg = PMIC_VDD2_OP_REG;
        break;
    }

    /* Select VDD1 OP   */
    buf=0;
	if (TWLReadByteReg(g_hTwl,reg,&buf)==FALSE)
		goto cleanup;
	buf &= ~PMIC_OP_REG_CMD_MASK;
	if (TWLWriteByteReg(g_hTwl, reg, buf)==FALSE)  
		goto cleanup;

	/* Configure VDD1 OP  Voltage */    
    buf=0;
	if (TWLReadByteReg(g_hTwl,reg,&buf)==FALSE)
		goto cleanup;
	buf &= ~PMIC_OP_REG_SEL_MASK;
    buf |= mv;
	if (TWLWriteByteReg(g_hTwl, reg, buf)==FALSE)  
		goto cleanup;

    /* check VDD1_OP_REG setting */
	buf=0;
	if (TWLReadByteReg(g_hTwl,reg,&buf)==FALSE)
		goto cleanup;
	
	if( (UINT32)(buf & PMIC_OP_REG_SEL_MASK ) != mv) 
		goto cleanup;
	
    rc = TRUE;

cleanup:    
    
    return rc;
}

