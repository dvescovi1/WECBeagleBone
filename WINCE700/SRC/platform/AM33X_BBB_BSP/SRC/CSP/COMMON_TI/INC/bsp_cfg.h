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
//  File:  bsp_cfg.h
//
#ifndef __BSP_CFG_H
#define __BSP_CFG_H

#include "sdk_gpio.h"
#include "sdk_padcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------
// Serial debug port
//------------------------------------------------------------------------
typedef struct {
    OMAP_DEVICE dev;
    DWORD LCR;
    DWORD DLL;
    DWORD DLH;
} DEBUG_UART_CFG;

const DEBUG_UART_CFG* BSPGetDebugUARTConfig();

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// NAND Flash
//------------------------------------------------------------------------
typedef struct {
    UINT8  manufacturerId;
    UINT8  deviceId;
    UINT32 blocks;
    UINT32 sectorsPerBlock;
    UINT32 sectorSize;
    UINT32 wordData;
} NAND_INFO;

DWORD BSPGetNandIrqWait();
DWORD BSPGetNandCS();
const NAND_INFO* BSPGetNandInfo(DWORD manufacturer,DWORD device);

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// GPIO
//------------------------------------------------------------------------
typedef struct {
    DWORD       cookie;
    int         nbGpioGrp;
    UINT        *rgRanges;
    HANDLE      *rgHandles;
    DEVICE_IFC_GPIO **rgGpioTbls;
    WCHAR       **name;
} GpioDevice_t;

void BSPGpioInit();
GpioDevice_t* BSPGetGpioDevicesTable();
DWORD BSPGetGpioIrq(DWORD id);
//------------------------------------------------------------------------


//------------------------------------------------------------------------
//  Triton Access
//------------------------------------------------------------------------
DWORD BSPGetTritonBusID();
UINT16 BSPGetTritonSlaveAddress();

//------------------------------------------------------------------------------
//  SDHC
//------------------------------------------------------------------------------
DWORD BSPGetSDHCCardDetect(DWORD slot);


//------------------------------------------------------------------------------
//  System Timer
//------------------------------------------------------------------------------
OMAP_DEVICE BSPGetSysTimerDevice();
OMAP_CLOCK BSPGetSysTimer32KClock();

//------------------------------------------------------------------------------
//  Profiler and Performance counter
//------------------------------------------------------------------------------
OMAP_DEVICE BSPGetGPTPerfDevice();
OMAP_CLOCK BSPGetGPTPerfHighFreqClock(UINT32* pFreq);

//------------------------------------------------------------------------------
//  Pad configuration
//------------------------------------------------------------------------------
PAD_INFO* BSPGetAllPadsInfo();
const PAD_INFO* BSPGetDevicePadInfo(OMAP_DEVICE);

//------------------------------------------------------------------------------
//  Watchdog. returns the device used as the system watchdog
//------------------------------------------------------------------------------
OMAP_DEVICE BPSGetWatchdogDevice();

//------------------------------------------------------------------------------
//  Camera
//------------------------------------------------------------------------------
OMAP_DEVICE BSPGetCameraDevice();
DWORD BSPGetCameraIrq();
OMAP_DEVICE BSPGetCameraI2CDevice();

//------------------------------------------------------------------------------
DWORD BSPGetSysTimerDeviceId(void);

void BSPCPUIdle();

DWORD BSPGetSMSCLanIRQ();


#ifdef __cplusplus
}
#endif

#endif // __BSP_CFG_H

