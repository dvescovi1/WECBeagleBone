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
//  File:  bsp_cfg.c
//
#include "bsp.h"
#include "bsp_cfg.h"
#include "bsp_def.h"
#include "oal_i2c.h"
#include "ceddkex.h"
#include "oalex.h"
#include "am33x_clocks.h"
#include "am33x_irq.h"
#include "omap_devices_map.h"
#include "omap_gpmc_regs.h"

//------------------------------------------------------------------------------
//  DEBUG UART
//------------------------------------------------------------------------------
//  This constants are used to initialize serial debugger output UART.
//  Serial debugger uses 115200-8-N-1
static const DEBUG_UART_CFG debugUartCfg={
    AM_DEVICE_UART0,
    BSP_UART_LCR,
    BSP_UART_DSIUDLL,
    BSP_UART_DSIUDLH
} ;

const DEBUG_UART_CFG* BSPGetDebugUARTConfig()
{
    return &debugUartCfg;
}

//------------------------------------------------------------------------------
//  I2C
//------------------------------------------------------------------------------
DWORD const I2CDefaultI2CTimeout = BSP_I2C_TIMEOUT_INIT;
//-----------------------------------------------------------------------------
I2CScaleTable_t _rgScaleTable[] = {
    { I2C_SS_PSC_VAL, I2C_SS_SCLL_VAL,  I2C_SS_SCLH_VAL }, // 100 KHz mode
    { I2C_FS_PSC_VAL, I2C_FS_SCLL_VAL,  I2C_FS_SCLH_VAL }, // 400 KHz mode (FS)
    { I2C_HS_PSC_VAL, I2C_1P6M_HSSCLL,  I2C_1P6M_HSSCLH }, // 1.6 MHz mode (HS)
    { I2C_HS_PSC_VAL, I2C_2P4M_HSSCLL,  I2C_2P4M_HSSCLH }, // 2.4 MHz mode (HS)
    { I2C_HS_PSC_VAL, I2C_3P2M_HSSCLL,  I2C_3P2M_HSSCLH }, // 3.2 MHz mode (HS)
};

I2CDevice_t _rgI2CDevice[] = 
{
    {
		AM_DEVICE_I2C0, BSP_I2C0_OA_INIT, BSP_I2C0_BAUDRATE_INIT,  BSP_I2C0_MAXRETRY_INIT, 
		BSP_I2C0_RX_THRESHOLD_INIT, BSP_I2C0_TX_THRESHOLD_INIT,
	}, {
        AM_DEVICE_I2C1, BSP_I2C1_OA_INIT, BSP_I2C1_BAUDRATE_INIT,  BSP_I2C1_MAXRETRY_INIT, 
        BSP_I2C1_RX_THRESHOLD_INIT, BSP_I2C1_TX_THRESHOLD_INIT,
	}, {
        AM_DEVICE_I2C2, BSP_I2C2_OA_INIT, BSP_I2C2_BAUDRATE_INIT,  BSP_I2C2_MAXRETRY_INIT, 
        BSP_I2C2_RX_THRESHOLD_INIT, BSP_I2C2_TX_THRESHOLD_INIT,
	}, {
        OMAP_DEVICE_NONE
    }
};

//------------------------------------------------------------------------------
//  NAND Flash
//------------------------------------------------------------------------------

NAND_INFO SupportedNands[] = {
	{	// MT29F2G08ABAEA   --( MT29F2G16AADWP THAT IS FROM HW MANUAL )
		0x2C,   //manufacturerId
		0xDA,   //deviceId
        2048,   //blocks
		64,     //sectorsPerBlock 
		2048,   //sectorSize - pageSize
		2       //wordData
	}
};

DWORD BSPGetNandIrqWait()
{
    return BSP_GPMC_IRQ_WAIT_EDGE;
}
DWORD BSPGetNandCS()
{
    return BSP_GPMC_NAND_CS;
}

const NAND_INFO* BSPGetNandInfo(DWORD manufacturer,DWORD device)
{
    int i;    
    for (i=0;i< dimof(SupportedNands);i++){
        if ((SupportedNands[i].manufacturerId == manufacturer) && (SupportedNands[i].deviceId == device)){
            return &SupportedNands[i];
        }
    }
    RETAILMSG(1,(TEXT("NAND manufacturer %x device %x : no matching device found\r\n"),manufacturer,device));    
    return NULL;
}

//------------------------------------------------------------------------
// GPIO
//------------------------------------------------------------------------
#define NB_GPIO_GRP 4

static HANDLE gpioHandleTable[NB_GPIO_GRP];
static UINT   gpioRangesTable[NB_GPIO_GRP];
static DEVICE_IFC_GPIO* gpioTables[NB_GPIO_GRP];
static WCHAR* gpioNames[NB_GPIO_GRP];
GpioDevice_t BSPGpioTables = {
    0,
    0,
    gpioRangesTable,
    gpioHandleTable,
    gpioTables,
    gpioNames,
};

BOOL BSPInsertGpioDevice(UINT range,DEVICE_IFC_GPIO* fnTbl,WCHAR* name)
{
    int index = BSPGpioTables.nbGpioGrp;
    if (index >= NB_GPIO_GRP) return FALSE;
    BSPGpioTables.rgRanges[index] = range;
    BSPGpioTables.rgGpioTbls[index] = fnTbl;
    BSPGpioTables.name[index] = name;
    BSPGpioTables.nbGpioGrp++;
    return TRUE;
}

GpioDevice_t* BSPGetGpioDevicesTable()
{
    return &BSPGpioTables;
}
DWORD BSPGetGpioIrq(DWORD id)
{
    return id + IRQ_GPIO_0;
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  SDHC
//------------------------------------------------------------------------------
DWORD BSPGetSDHCCardDetect(DWORD slot)
{
    switch (slot)
    {
    default: return (DWORD) -1;
    }
}


AM33X_DEVICE_ID BPSGetWatchdogDevice()
{
    return AM_DEVICE_WDT1;
}

//------------------------------------------------------------------------------
//  Triton
//------------------------------------------------------------------------------
DWORD BSPGetTritonBusID()
{
    return TPS65217_I2C_DEVICE_ID;
}

UINT16 BSPGetTritonSlaveAddress()
{
    return I2C_PMIC_ADDR;
}

DWORD BSPGetSysTimerDeviceId(void)
{
	return AM_DEVICE_TIMER3;
}

DWORD BSPGetGPTPerfDevice(void)
{
	return AM_DEVICE_TIMER2;
}

