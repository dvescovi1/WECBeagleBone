// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: spi_priv.h
//

#ifndef __SPI_PRIV_H
#define __SPI_PRIV_H

//#include <dvfs.h>


//------------------------------------------------------------------------------
//
//  Set debug zones names
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IST            DEBUGZONE(4)
#define ZONE_DMA            DEBUGZONE(5)
#define ZONE_DVFS           DEBUGZONE(6)

#endif


//------------------------------------------------------------------------------
//  Local Structures

typedef enum {
    UNKNOWN = 0,
    MASTER,
    SLAVE,
} SPI_MODE;

typedef struct {
    DWORD cookie;
    DWORD memBase[1];
    DWORD memLen[1];
    DWORD irq;
    LONG instances;
    HANDLE hParentBus;
    OMAP_MCSPI_REGS *pSPIRegs;
    HANDLE hControllerMutex;
    DWORD sysIntr;
    HANDLE hIntrEvent;
    DWORD timeout;
    DWORD dwTxBufferSize;
    DWORD dwRxBufferSize;
    CEDEVICE_POWER_STATE powerState;
    DWORD dwPort;
	OMAP_DEVICE deviceID;

    SPI_MODE eSpiMode;

    // dvfs related variables
    BOOL bDVFSActive;
    LONG  nActiveDmaCount;
    HANDLE hDVFSInactiveEvent;
    HANDLE hDVFSAsyncEvent;
    _TCHAR szDVFSAsyncEventName[MAX_PATH];
    CRITICAL_SECTION csDVFS;

    // activity time-out related items
    DWORD  nActivityTimeout;
    HANDLE hTimerThread;
    HANDLE hTimerEvent;
    BOOL   bDisablePower;
    BOOL   bExitThread;
    DWORD  nPowerCounter;
    void *pActiveInstance;
    HANDLE hDeviceOffEvent;
    CEDEVICE_POWER_STATE systemState;
} SPI_DEVICE;

typedef struct {
    DWORD cookie;
    SPI_DEVICE *pDevice;
    DWORD address;
    DWORD config;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
	BOOL exclusiveAccess;

    HANDLE  hTxDmaChannel;
    HANDLE  hRxDmaChannel;

    HANDLE  hTxDmaIntEvent;
    HANDLE  hRxDmaIntEvent;

    DmaConfigInfo_t txDmaConfig;
    DmaConfigInfo_t rxDmaConfig;

    DmaDataInfo_t   txDmaInfo;
    DmaDataInfo_t   rxDmaInfo;

    VOID*   pTxDmaBuffer;
    ULONG  paTxDmaBuffer;

    VOID*   pRxDmaBuffer;
    ULONG  paRxDmaBuffer;
} SPI_INSTANCE;

#define MCSPI_MAX_PORTS                 4

#define SPIPORT_1                       0
#define SPIPORT_2                       1
#define SPIPORT_3                       2
#define SPIPORT_4                       3

#define DMA_TX                          0
#define DMA_RX                          1


//------------------------------------------------------------------------------
//  Local Functions

BOOL 
SpiDmaRestore(
    SPI_INSTANCE *pInstance
    );

BOOL
SpiDmaInit(
    SPI_INSTANCE *pInstance
    );

BOOL
SpiDmaDeinit(
    SPI_INSTANCE *pInstance
    );

__inline BOOL 
SpiDmaTxEnabled(
    SPI_INSTANCE *pInstance
    )
{
    //  Return enabled BOOL
    return( pInstance->hTxDmaChannel != NULL );
}

__inline BOOL 
SpiDmaRxEnabled(
    SPI_INSTANCE *pInstance
    )
{
    //  Return enabled BOOL
    return( pInstance->hRxDmaChannel != NULL );
}

BOOL 
ContextRestore(
    SPI_INSTANCE *pInstance
    );

DWORD 
SPI_Init(
    LPCTSTR szContext, 
    LPCVOID pBusContext
    );

BOOL
SPI_Deinit(
    DWORD context
    );

DWORD 
SPI_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    );

BOOL
SPI_Close(
    DWORD context
    );

BOOL
SPI_Configure(
    DWORD context, 
    DWORD address, 
    DWORD config
    );

DWORD
SPI_Read(
    DWORD context, 
    VOID *pBuffer, 
    DWORD size
    );

DWORD
SPI_Write(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    );

DWORD
SPI_DmaRead(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    );

DWORD
SPI_DmaWrite(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    );

DWORD
SPI_WriteRead(
    DWORD context, 
    DWORD size,
    VOID *pOutBuffer, 
    VOID *pInBuffer
    );

DWORD
SPI_AsyncWriteRead(
    DWORD context,
    DWORD size,
    VOID *pOutBuffer,
    VOID *pInBuffer
    );

DWORD
SPI_WaitForAsyncWriteReadComplete(
    DWORD context,
    DWORD size,
    VOID *pInBuffer
    );

DWORD
SPI_DmaWriteRead(
    DWORD context,
    DWORD size,
    VOID *pOutBuffer,
    VOID *pInBuffer
    );

BOOL 
SPI_SetSlaveMode(
	DWORD context
	);


BOOL
SPI_IOControl(
    DWORD context,
    DWORD dwCode,
    BYTE *pInBuffer,
    DWORD inSize,
    BYTE *pOutBuffer,
    DWORD outSize,
    DWORD *pOutSize
    );


#endif
