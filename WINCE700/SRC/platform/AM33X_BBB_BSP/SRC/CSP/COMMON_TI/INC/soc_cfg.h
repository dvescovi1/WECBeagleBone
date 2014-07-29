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
//
//  File:  soc_cfg.h
//
#ifndef __SOC_CFG_H
#define __SOC_CFG_H

#include "sdk_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------
//  Interrupt Ctrl
//------------------------------------------------------------------------
DWORD SOCGetIntCtrlAddr();

//------------------------------------------------------------------------
//  GPMC
//------------------------------------------------------------------------
DWORD SOCGetGPMCAddress(DWORD index);

//------------------------------------------------------------------------
//  GPIO
//------------------------------------------------------------------------
DWORD SOCGetGPIODeviceByBank(DWORD index);

//------------------------------------------------------------------------
//  MCSPI
//------------------------------------------------------------------------
DWORD SOCGetMCSPIDeviceByBus(DWORD index);

//------------------------------------------------------------------------
//  MCBSP
//------------------------------------------------------------------------
DWORD SOCGetMCBSPDeviceByBus(DWORD index);
DWORD SOCGetMCBSPSidetoneAddress(OMAP_DEVICE deviceID);

//------------------------------------------------------------------------
//  I2C
//------------------------------------------------------------------------
DWORD SOCGetI2CDeviceByBus(DWORD index);

//------------------------------------------------------------------------
//  UART
//------------------------------------------------------------------------
DWORD SOCGetUartDeviceByIndex(DWORD index);

//------------------------------------------------------------------------
//  SDHC
//------------------------------------------------------------------------
DWORD SOCGetSDHCDeviceBySlot(DWORD slot);

//------------------------------------------------------------------------
//  DSS
//------------------------------------------------------------------------
typedef struct {
    OMAP_DEVICE DSSDevice;
    OMAP_DEVICE TVEncoderDevice;

    DWORD DSS1_REGS_PA;
    DWORD DISC1_REGS_PA;  
    DWORD VENC1_REGS_PA;
    DWORD DSI_REGS_PA;    
    DWORD DSI_PLL_REGS_PA;
} DSS_INFO;

void SOCGetDSSInfo(DSS_INFO* pInfo);

//------------------------------------------------------------------------
//  CPGMAC (or EMAC)
//------------------------------------------------------------------------
DWORD SOCGetEMACDevice(DWORD index);


//------------------------------------------------------------------------
//  HDQ (1-wire)
//------------------------------------------------------------------------
DWORD SOCGetHDQDevice(DWORD index);

//------------------------------------------------------------------------
//  SDMA
//------------------------------------------------------------------------
DWORD SOCGetDMADevice(DWORD index);

//------------------------------------------------------------------------
//  CAN
//------------------------------------------------------------------------
DWORD SOCGetCANDevice(DWORD index);

//------------------------------------------------------------------------
//  VRFB
//------------------------------------------------------------------------
DWORD SOCGetVRFBDevice();

//------------------------------------------------------------------------
//  Pad configuration
//------------------------------------------------------------------------
BOOL SOCSetPadConfig(UINT16 padId, UINT16 cfg);
BOOL SOCGetPadConfig(UINT16 padId, UINT16* pCfg);

//------------------------------------------------------------------------
//  USB Host
//------------------------------------------------------------------------
typedef struct {
	OMAP_DEVICE EHCIDevice;
	OMAP_DEVICE	TLLDevice;
	OMAP_DEVICE Host1Device;
	OMAP_DEVICE Host2Device;
	OMAP_DEVICE Host3Device;

	DWORD	UHH_REGS_PA;
	DWORD	USBTLL_REGS_PA;
	DWORD	EHCI_REGS_PA;
	DWORD	PRCM_USBHOST_REGS_PA;
	DWORD	PRCM_CORE_REGS_PA;
}USBH_INFO;

void SOCGetUSBHInfo(USBH_INFO* pInfo);

//------------------------------------------------------------------------
//  USB OTG
//------------------------------------------------------------------------
DWORD SOCGetUSBOTGAddress();
DWORD SOCGetOTGDevice();

//------------------------------------------------------------------------
//  CPU 
//------------------------------------------------------------------------
DWORD SOCGetIDCodeAddress();

//------------------------------------------------------------------------
//  System Frequence KHz 
//------------------------------------------------------------------------
UINT32 SOCGetSysFreqKHz(void);

//------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif // __SOC_CFG_H

