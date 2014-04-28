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
//  File:  soccfg.c
//
//  Configuration file for the device list
//
#include "am33x.h"
#include "soc_cfg.h"
#include "omap_devices_map.h"
#include "am33x_clocks.h"
#include "omap_cpgmac_regs.h"

DWORD SOCGetGPIODeviceByBank(DWORD index)
{
    switch (index)
    {
        case 1: return AM_DEVICE_GPIO0;
        case 2: return AM_DEVICE_GPIO1;
        case 3: return AM_DEVICE_GPIO2;
        case 4: return AM_DEVICE_GPIO3;
        default: return OMAP_DEVICE_NONE;   
    }
}

DWORD SOCGetSDHCDeviceBySlot(DWORD index)
{
    switch (index)
    {
    	case 1: return AM_DEVICE_MMCHS0;
    	case 2: return AM_DEVICE_MMCHS1;
    	case 3: return AM_DEVICE_MMCHS2;
    	default: return OMAP_DEVICE_NONE;   
    }
}

DWORD SOCGetMCSPIDeviceByBus(DWORD index)
{
	switch(index)
	{
		case 1:	return AM_DEVICE_MCSPI0;
		case 2:	return AM_DEVICE_MCSPI1;
	}
	return OMAP_DEVICE_NONE;
}

DWORD SOCGetI2CDeviceByBus(DWORD index)
{
	switch(index)
	{
		case 1:	return AM_DEVICE_I2C0;
		case 2:	return AM_DEVICE_I2C1;
		case 3:	return AM_DEVICE_I2C2;
	}
	return OMAP_DEVICE_NONE;
}

DWORD SOCGetUartDeviceByIndex(DWORD index)
{
	switch(index)
	{
		case 1:	return AM_DEVICE_UART0;
		case 2:	return AM_DEVICE_UART1;
		case 3:	return AM_DEVICE_UART2;
		case 4:	return AM_DEVICE_UART3;
		case 5:	return AM_DEVICE_UART4;
		case 6:	return AM_DEVICE_UART5;
	}
	return OMAP_DEVICE_NONE;
}

DWORD SOCGetGPMCAddress(DWORD index)
{
    UNREFERENCED_PARAMETER(index);
    return AM33X_GPMC_REGS_PA;
}

void SOCGetDSSInfo(DSS_INFO* pInfo)
{
    pInfo->DSSDevice = AM_DEVICE_LCDC;
    pInfo->TVEncoderDevice = (DWORD)NULL;
    pInfo->DSS1_REGS_PA     = (DWORD)NULL;
    pInfo->DISC1_REGS_PA    = (DWORD)NULL; 
    pInfo->VENC1_REGS_PA    = (DWORD)NULL;
    pInfo->DSI_REGS_PA      = (DWORD)NULL;
    pInfo->DSI_PLL_REGS_PA  = (DWORD)NULL;
}

UINT32 SOCGetSysFreqKHz(void)
{
	return 24000;
}

