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
//  File:  fuse.c
//

#include "bsp.h"
#include "am33x_config.h"
#include "am33x_base_regs.h"

//------------------------------------------------------------------------------
void ReadMacAddressFromFuse(int cpgmac_num, UCHAR mac[6])
{
    DWORD low,high;
    AM33X_DEVICE_CONF_REGS *pSys = (AM33X_DEVICE_CONF_REGS*) OALPAtoUA(AM33X_DEVICE_CONF_REGS_PA);
    
	switch (cpgmac_num){
		case 0:
			low = INREG32(&pSys->MAC_ID0_LO);
			high = INREG32(&pSys->MAC_ID0_HI);
			break;
		case 1:
			low = INREG32(&pSys->MAC_ID1_LO);
			high = INREG32(&pSys->MAC_ID1_HI);
			break;
		default:
			low = 0x0;
			high = 0x0;
	}

    mac[5] = (UCHAR) ((low  >>  8) & 0xFF);
    mac[4] = (UCHAR) ((low  >>  0) & 0xFF);
    mac[3] = (UCHAR) ((high >>  24) & 0xFF);
    mac[2] = (UCHAR) ((high >>  16) & 0xFF);
    mac[1] = (UCHAR) ((high >>   8) & 0xFF);
    mac[0] = (UCHAR) ((high >>   0) & 0xFF);
    RETAILMSG(1,(TEXT("%x %x -> %x %x %x %x %x %x\r\n"),high,low,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]));
}
