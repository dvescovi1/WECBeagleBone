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
//  File:  pdd.c
//
//  This file implements PDD for AM389X serial port.
//
//  Its has two modes of transmission, FIFO and DMA. It is selected via registry:
//  TxDmaRequest= -1 will disable DMA and enable non-DMA FIFO for Tx
//  RxDmaRequest= -1 will disable DMA and enable non-DMA FIFO for Rx
//  For 35xx, the DMA library will dynamically select the DMA channel resources.
//  DMA and non-DMA FIFO usage on Tx and Rx can be mixed, where one channel is DMA and the
//  other is non-DMA FIFO as they are independent.
//
//  Note: For Rx DMA some filtering features have not been implemented and
//  additional limitation exist due to its implementation:
//   1) The DSR Sensitivity feature is disabled - char received are not
//      filtered out with DSR
//   2) NULL character filtering is disabled
//   3) Single character response delay is upto 100 msec regardless of baud rate.
//
//  Improvements: This implementation uses the public MDD layer, which either
//  forces triple buffering to implement the full features.  Using a custom MDD
//  layer would allow double buffering to take full advantage of DMA.
//

#include <serdbg.h>
#include "am33x.h"
#include "soc_cfg.h"
#include "sdk_gpio.h"
#include "am33x_uart.h"
#include "edma_utility.h"
#include "oal_clock.h"
#include "sdk_padcfg.h"
#include <serhw.h>
#include <pegdser.h>
#include <serpriv.h>

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)


extern DmaConfigInfo_t TxDmaSettings;
extern DmaConfigInfo_t RxDmaSettings;

//------------------------------------------------------------------------------

typedef struct {
    DWORD memBase[1];                   // PA of UART and DMA registers
    DWORD memLen[1];                    // Size of register arrays
    DWORD irq;                          // IRQ
    DWORD UARTIndex;					// UART index
	OMAP_DEVICE DeviceID;				// UART device ID

    BOOL  hwMode;                       // Hardware handshake mode
    DWORD rxBufferSize;                 // MDD RX buffer size

    DWORD wakeUpChar;                   // WakeUp character
    DWORD hwTimeout;                    // Hardware timeout

    AM33X_UART_REGS *pUartRegs;          // Mapped VA of UART port
    ULONG sysIntr;                      // Assigned SYSINTR

//    HANDLE hParentBus;                  // Parent bus handler

    CEDEVICE_POWER_STATE currentDX;     // Actual hardware power state
    CEDEVICE_POWER_STATE externalDX;    // External power state
    CRITICAL_SECTION powerCS;           // Guard access to power change

    ULONG frequency;                    // UART module input frequency

    PVOID pMdd;                         // MDD context

    BOOL  open;                         // Is device open?
    DCB   dcb;                          // Serial port DCB

    ULONG commErrors;                   // How many errors occured
    ULONG overrunCount;                 // How many chars was missed

    BOOL  autoRTS;                      // Is auto RTS enabled?
    BOOL  wakeUpMode;                   // Are we in special wakeup mode?
    BOOL  wakeUpSignaled;               // Was wakeup mode signaled already?

    UCHAR intrMask;                     // Actual interrupt mask
    UCHAR CurrentFCR;                   // FCR write only so save TX/RX trigger here
    UCHAR CurrentSCR;                   // SCR write only so save TX/RX trigger here

    BOOL  addTxIntr;                    // Should we add software TX interrupt?
    BOOL  flowOffCTS;                   // Is CTS down?
    BOOL  flowOffDSR;                   // Is DSR down?

    CRITICAL_SECTION hwCS;              // Guard access to HW registers
    CRITICAL_SECTION txCS;              // Guard HWXmitComChar
    CRITICAL_SECTION RxUpdatePtrCS;     // Guard DMA update pointer
    HANDLE txEvent;                     // Signal TX interrupt for HWXmitComChar

    COMMTIMEOUTS commTimeouts;          // Communication Timeouts
    DWORD txPauseTimeMs;                // Time to delay in Tx thread

    DWORD TxDmaRequest;                 // Transmit DMA request
    DWORD TxDmaBufferSize;
    VOID *pTxDmaBuffer;                 // Transmit DMA buffer (virtual address)
    DWORD paTxDmaBuffer;                // Transmit DMA buffer (physical address)
    DmaDataInfo_t *TxDmaInfo;
    HANDLE hTxDmaChannel;               // TX DMA channel allocated by the DMA lib
    HANDLE hEventTxIstDma;              // TX DMA event handle

    DWORD RxDmaRequest;                 // Receive DMA request
    DWORD RxDmaBufferSize;
    VOID *pRxDmaBuffer;                 // Receive DMA buffer (virtual address)
    DWORD paRxDmaBuffer;                // Receive DMA buffer (physical address)
    UINT8* pSavedBufferPos;             // PDD last write position in buffer
    UINT8* lastWritePos;                // last write position of Rx DMA
    DmaDataInfo_t *RxDmaInfo;
    HANDLE hEventRxIstDma;              // RX DMA event handle
    HANDLE hRxDmaChannel;               // RX DMA channel allocated by the DMA lib
    BOOL bHWXmitComCharWaiting;         // true when HWXmitComChar character has been sent.
    BOOL bSendSignal;                   // Flag to indicate no space for DMA
    BOOL  bExitThread;                  // Flag to indicate rx thread shutodown.
    HANDLE hRxThread;                   // IST_RxDMA thread handle
    DWORD RxTxRefCount;                 // to keep track of RX-TX power level

    BOOL  bRxDMASignaled;               // Rx DMA Occured   
    BOOL  bRxWrapped;                   // Rx wrap around occured
    VOID  *pRxDMALastWrite;             // Cached pointer for the last DMA write position.
    BOOL  bDmaInitialize;               // Indicates HWOpen finished DMA setting. Used to
                                        // avoid problem in HWOpen which calls SetPower
                                        // before DMA was initialized and causing SetPower
                                        // to crash when it try to update DMA pointer.
    DWORD dwRxFifoTriggerLevel;         // Rx Fifo trigger level.
    DWORD dwRtsCtsEnable;               // Enables RTS/CTS handshake support

    HANDLE hPowerEvent;                 // Rx Tx Activity tracking event
    HANDLE hPowerThread;                // Process Force Idle / NoIdle Thread
    BOOL bExitPowerThread;              // Signal to exit power thread
    BOOL bDisableAutoIdle;

    HANDLE hGpio;
    DWORD XcvrEnableGpio;               // GPIO pin that enables/disables external transceiver
    DWORD XcvrEnabledLevel;             // Level of GPIO pin that corresponds to xcvr enabled

    BOOL bRxBreak;                      // true if break condition is received
    UINT8   savedIntrMask;              // backup interrupt mask.
    UINT8   currentMCR;                 // MCR register value.
} UARTPDD;

// FIFO size and default RxFifoTriggerLevel
#define UART_FIFO_SIZE  64
#define DEFAULT_RX_FIFO_TRIGGER_LEVEL   32

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"DeviceArrayIndex", PARAM_DWORD, TRUE, offset(UARTPDD, UARTIndex),
            fieldsize(UARTPDD, UARTIndex), NULL
    }, {
        L"HWMode", PARAM_DWORD, FALSE, offset(UARTPDD, hwMode),
            fieldsize(UARTPDD, hwMode), (VOID*)FALSE
    }, {
        L"Frequency", PARAM_DWORD, FALSE, offset(UARTPDD, frequency),
            fieldsize(UARTPDD, frequency), (VOID*)48000000
    }, {
        L"WakeChar", PARAM_DWORD, FALSE, offset(UARTPDD, wakeUpChar),
            fieldsize(UARTPDD, wakeUpChar), (VOID*)0x32
    }, {
        L"RxBuffer", PARAM_DWORD, FALSE, offset(UARTPDD, rxBufferSize),
            fieldsize(UARTPDD, rxBufferSize), (VOID*)8192
    }, {
        L"HWTimeout", PARAM_DWORD, FALSE, offset(UARTPDD, hwTimeout),
            fieldsize(UARTPDD, hwTimeout), (VOID*)1000
    }, {
        L"TxDmaRequest", PARAM_DWORD, FALSE, offset(UARTPDD, TxDmaRequest),
            fieldsize(UARTPDD, TxDmaRequest), (VOID*)-1
    }, {
        L"TxDmaBufferSize", PARAM_DWORD, FALSE, offset(UARTPDD, TxDmaBufferSize),
            fieldsize(UARTPDD, TxDmaBufferSize), (VOID*)0x2000
    }, {
        L"TXPauseTimeMs", PARAM_DWORD, FALSE, offset(UARTPDD, txPauseTimeMs),
            fieldsize(UARTPDD, txPauseTimeMs), (VOID*)0x25
    }, {
        L"RxDmaRequest", PARAM_DWORD, FALSE, offset(UARTPDD, RxDmaRequest),
            fieldsize(UARTPDD, RxDmaRequest), (VOID*)-1
    }, {
        L"RxDmaBufferSize", PARAM_DWORD, FALSE, offset(UARTPDD, RxDmaBufferSize),
            fieldsize(UARTPDD, RxDmaBufferSize), (VOID*)0x2000
    }, {
        L"XcvrEnableGpio", PARAM_DWORD, FALSE, offset(UARTPDD, XcvrEnableGpio),
            fieldsize(UARTPDD, XcvrEnableGpio), (VOID*)0xFFFF
    }, {
        L"XcvrEnabledLevel", PARAM_DWORD, FALSE, offset(UARTPDD, XcvrEnabledLevel),
            fieldsize(UARTPDD, XcvrEnabledLevel), (VOID*)0xFFFF
    }, {
        L"RxFifoTriggerLevel", PARAM_DWORD, FALSE, offset(UARTPDD, dwRxFifoTriggerLevel ),
            fieldsize(UARTPDD, dwRxFifoTriggerLevel), (VOID*)DEFAULT_RX_FIFO_TRIGGER_LEVEL
    }, {
        L"RtsCtsEnable", PARAM_DWORD, FALSE, offset(UARTPDD, dwRtsCtsEnable),
            fieldsize(UARTPDD, dwRtsCtsEnable), (VOID*)FALSE
    }
   };
//------------------------------------------------------------------------------
//  Local Defines

#define MAX_TX_SERIALDMA_FRAMESIZE            63
#define MAX_RX_SERIALDMA_FRAMESIZE            63

///Just for testing
#define TESTENABLE FALSE


//------------------------------------------------------------------------------
//
//  Function:  InitializeRxDMA
//
//  This function initiallizes the Rx DMA register.
//
static BOOL InitializeRxDMA(UARTPDD *pPdd)
{
//    OMAP_DMA_LC_REGS *pDmaLcReg;

    DmaConfigure (pPdd->hRxDmaChannel,
        &RxDmaSettings, pPdd->RxDmaRequest, pPdd->RxDmaInfo);

//    pDmaLcReg = (OMAP_DMA_LC_REGS*)DmaGetLogicalChannel(pPdd->hRxDmaChannel);
//    OUTREG32(&pDmaLcReg->CDAC, pPdd->paRxDmaBuffer);

    // set up for Rx buffer as single frame with Max DMA buffer. Must be multiple of
    // frame size
    DmaSetElementAndFrameCount (pPdd->RxDmaInfo,
        (UINT16)pPdd->dwRxFifoTriggerLevel, 
        (UINT16)((pPdd->RxDmaBufferSize + pPdd->dwRxFifoTriggerLevel - 1) / pPdd->dwRxFifoTriggerLevel));
    DmaSetDstBuffer (pPdd->RxDmaInfo,
        pPdd->pRxDmaBuffer,
        pPdd->paRxDmaBuffer);
    DmaSetSrcBuffer(pPdd->RxDmaInfo,
        (UINT8 *)&(pPdd->pUartRegs->RHR),
        pPdd->memBase[0] + offset(AM33X_UART_REGS, RHR));

    return TRUE;
}

#define SOC_UART_REGS AM33X_UART_REGS

#define SOC_SET_DEVICE_POWER(p, s)  SetDeviceClockState((p), (s))
#define SOC_DUMP_DMA_REGS(p, i)
#define SOC_CLOSE_PARENT_BUS(p)     

// This is just a dummy
#define SOC_HAS_PARENT_BUS(p)       ((p) != NULL)

static BOOL CreateParentBus(UARTPDD *pPdd, ULONG context)
{
    UNREFERENCED_PARAMETER(pPdd);
    UNREFERENCED_PARAMETER(context);
    return TRUE;
}

#include "..\pdd_common.c"

