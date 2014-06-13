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


//------------------------------------------------------------------------------
//  Local Functions

static VOID* HWInit(ULONG, VOID*, HWOBJ*);
static BOOL  HWPostInit(VOID*);
static ULONG  HWDeinit(VOID*);
static BOOL  HWOpen(VOID*);
static ULONG HWClose(VOID*);
static INTERRUPT_TYPE HWGetInterruptType(VOID*);
static ULONG HWRxIntr(VOID*, UCHAR*, ULONG*);
static VOID  HWTxIntr(VOID*, UCHAR*, ULONG*);
static VOID  HWModemIntr(VOID*);
static VOID  HWLineIntr(VOID*);
static ULONG HWGetRxBufferSize(VOID*);
static BOOL  HWPowerOff(VOID*);
static BOOL  HWPowerOn(VOID*);
static VOID  HWClearDTR(VOID*);
static VOID  HWSetDTR(VOID*);
static VOID  HWClearRTS(VOID*);
static VOID  HWSetRTS(VOID*);
static BOOL  HWEnableIR(VOID*, ULONG);
static BOOL  HWDisableIR(VOID*);
static VOID  HWClearBreak(VOID*);
static VOID  HWSetBreak(VOID*);
static VOID  HWReset(VOID*);
static VOID  HWGetModemStatus(VOID*, ULONG*);
static BOOL  HWXmitComChar(VOID*, UCHAR);
static ULONG HWGetStatus(VOID*, COMSTAT*);
static VOID  HWGetCommProperties(VOID*, COMMPROP*);
static VOID  HWPurgeComm(VOID*, DWORD);
static BOOL  HWSetDCB(VOID*, DCB*);
static BOOL  HWSetCommTimeouts(VOID*, COMMTIMEOUTS*);
static BOOL  HWIOCtl(VOID*, DWORD, UCHAR*, DWORD, UCHAR*, DWORD, DWORD*);


static ULONG HWRxDMAIntr(VOID *pvContext, UCHAR *pRxBuffer, ULONG *pLength);
static BOOL UpdateDMARxPointer(UARTPDD *pPdd, BOOL bPurge, DWORD  dwOffSet);
static VOID RestoreUARTContext(UARTPDD *pPdd);
static BOOL SetPower(UARTPDD *pPdd, CEDEVICE_POWER_STATE dx);


static VOID UART_RegDump(UARTPDD * pPDD);


static HW_VTBL g_pddVTbl = {
    HWInit,
    HWPostInit,
    HWDeinit,
    HWOpen,
    HWClose,
    HWGetInterruptType,
    HWRxIntr,
    HWTxIntr,
    HWModemIntr,
    HWLineIntr,
    HWGetRxBufferSize,
    HWPowerOff,
    HWPowerOn,
    HWClearDTR,
    HWSetDTR,
    HWClearRTS,
    HWSetRTS,
    HWEnableIR,
    HWDisableIR,
    HWClearBreak,
    HWSetBreak,
    HWXmitComChar,
    HWGetStatus,
    HWReset,
    HWGetModemStatus,
    HWGetCommProperties,
    HWPurgeComm,
    HWSetDCB,
    HWSetCommTimeouts,
    HWIOCtl
};


//------------------------------------------------------------------------------
//
//  Function:  SetDeviceClockState
//
//  This function changes device clock state according to required power state.
//
//  The dx parameter gives the power state to switch to
//
VOID SetDeviceClockState(UARTPDD *pPdd, CEDEVICE_POWER_STATE dx)
{
	switch(dx)
	{
	case D0:
	case D1:
	case D2:
		EnableDeviceClocks(pPdd->DeviceID, TRUE);
		break;
	case D3:
	case D4:
		EnableDeviceClocks(pPdd->DeviceID, FALSE);
		break;
	default:
	break;
	}
}

//------------------------------------------------------------------------------
//
//  Function:  SetDefaultDCB
//
//  This function set DCB to default values
//
//
static
VOID
SetDefaultDCB(
           UARTPDD *pPdd
           )
{

    // Initialize Default DCB
    pPdd->dcb.DCBlength = sizeof(pPdd->dcb);
    pPdd->dcb.BaudRate = 9600;
    pPdd->dcb.fBinary = TRUE;
    pPdd->dcb.fParity = FALSE;
    pPdd->dcb.fOutxCtsFlow = FALSE;
    pPdd->dcb.fOutxDsrFlow = FALSE;
    pPdd->dcb.fDtrControl = DTR_CONTROL_DISABLE; //DTR_CONTROL_ENABLE;
    pPdd->dcb.fDsrSensitivity = FALSE;
    pPdd->dcb.fRtsControl = RTS_CONTROL_DISABLE;  // RTS_CONTROL_ENABLE;
    pPdd->dcb.ByteSize = 8;
    pPdd->dcb.Parity = 0;
    pPdd->dcb.StopBits = 1;

}

//------------------------------------------------------------------------------
//
//  Function:  SetAutoIdle
//
//  This function enable/disable AutoIdle Mode
//
//
static
BOOL
SetAutoIdle(
           UARTPDD *pPdd,
           BOOL enable
           )
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetAutoIdle(%d)\r\n", enable));

    EnterCriticalSection(&pPdd->hwCS);

    // Enable/disable hardware auto Idle
    if (enable)
    {
        if(!pPdd->RxTxRefCount)
            {
                OUTREG8(
                &pUartRegs->SYSC,
                // Disable force idle, to avoid data corruption
                UART_SYSC_IDLE_DISABLED|UART_SYSC_WAKEUP_ENABLE
                );
            }
        pPdd->bDisableAutoIdle = FALSE;
        InterlockedIncrement((LONG*) &pPdd->RxTxRefCount);
    }
    else
    {
        InterlockedDecrement((LONG*) &pPdd->RxTxRefCount);
        if(!pPdd->RxTxRefCount) 
            SetEvent(pPdd->hPowerEvent);
    }

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-SetAutoIdle()\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SetAutoRTS
//
//  This function enable/disable HW auto RTS.
//
//  This function enable/disable auto RTS. It is primary intend to be used
//  for BT, but it should work in most cases. However correct function depend
//  on oposite device, it must be able to stop sending data in timeframe
//  which will not FIFO overflow. Check TCR setting in HWInit.
//
static
BOOL
SetAutoRTS(
           UARTPDD *pPdd,
           BOOL enable
           )
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lcr;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetAutoRTS()\r\n"));

    // Get UART lock
    EnterCriticalSection(&pPdd->hwCS);

    // Save LCR value & enable EFR access
    lcr = INREG8(&pUartRegs->LCR);
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_CONFIG_B);

    // Enable/disable hardware auto RTS
    if (enable)
    {
        SETREG8(&pUartRegs->EFR, UART_EFR_AUTO_RTS_EN);
        pPdd->autoRTS = TRUE;
    }
    else
    {
        // Disable hardware auto RTS
        CLRREG8(&pUartRegs->EFR, UART_EFR_AUTO_RTS_EN);
        pPdd->autoRTS = FALSE;
    }

    // Restore LCR value
    OUTREG8(&pUartRegs->LCR, lcr);

    // Free UART lock.

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-SetAutoRTS()\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SetAutoCTS
//
//  This function enable/disable HW auto CTS.
//
static
BOOL
SetAutoCTS(
           UARTPDD *pPdd,
           BOOL enable
           )
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lcr;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetAutoCTS()\r\n"));

    // Get UART lock
    EnterCriticalSection(&pPdd->hwCS);

    // Save LCR value & enable EFR access
    lcr = INREG8(&pUartRegs->LCR);
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_CONFIG_B);

    // Enable/disable hardware auto CTS/RTS
    if (enable)
    {
        SETREG8(&pUartRegs->EFR, UART_EFR_AUTO_CTS_EN);
    }
    else
    {
        // Disable hardware auto CTS/RTS
        CLRREG8(&pUartRegs->EFR, UART_EFR_AUTO_CTS_EN);
    }

    // Restore LCR value
    OUTREG8(&pUartRegs->LCR, lcr);

    // Free UART lock.
    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-SetAutoCTS()\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ReadLineStat
//
//  This function reads line status register and it calls back MDD if line
//  event occurs. This must be done in this way because register bits are
//  cleared on read.
//
static
UCHAR
ReadLineStat(
             UARTPDD *pPdd
             )
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    ULONG events = 0;
    UCHAR lineStat = 0;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ReadLineStat()\r\n")));

    if (pPdd->open == TRUE)
    {
        EnterCriticalSection(&pPdd->hwCS);

        lineStat = INREG8(&pUartRegs->LSR);
        if ((lineStat & UART_LSR_RX_FE) != 0)
        {
            pPdd->commErrors |= CE_FRAME;
            events |= EV_ERR;
        }
        if ((lineStat & UART_LSR_RX_PE) != 0)
        {
            pPdd->commErrors |= CE_RXPARITY;
            events |= EV_ERR;
        }
        if ((lineStat & UART_LSR_RX_OE) != 0)
        {
            pPdd->overrunCount++;
            pPdd->commErrors |= CE_OVERRUN;
            events |= EV_ERR;

            // UART RX stops working after RX FIFO overrun, must clear RX FIFO and read RESUME register
            OUTREG8(&pUartRegs->FCR, pPdd->CurrentFCR | UART_FCR_RX_FIFO_CLEAR);
            INREG8(&pUartRegs->RESUME);
        }
        if ((lineStat & UART_LSR_RX_BI) != 0)
        {
            events |= EV_BREAK;
            pPdd->bRxBreak = TRUE;
        }

        LeaveCriticalSection(&pPdd->hwCS);

        if ((events & EV_ERR) != 0)
        {
            DEBUGMSG(ZONE_ERROR, (
                L"UART!ReadLineStat: Error detected, LSR: 0x%02x\r\n", lineStat
                ));
        }

        // Let MDD know if something happen
        if (events != 0) EvaluateEventFlag(pPdd->pMdd, events);
    }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("-ReadLineStat(%02x)\r\n"), lineStat));
    return lineStat;
}

//------------------------------------------------------------------------------
//
//  Function:  ReadModemStat
//
//  This function reads modem status register and it calls back MDD if modem
//  event occurs. This must be done in this way  because register bits are
//  cleared on read.
//
static
UCHAR
ReadModemStat(
              UARTPDD *pPdd
              )
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR modemStat;
    ULONG events;

    DEBUGMSG(ZONE_FUNCTION, (L"+ReadModemStat()\r\n"));

    // Nothing happen yet...
    events = 0;

    // Read modem status register (it clear most bits)
    modemStat = INREG8(&pUartRegs->MSR);

    // For changes, we use callback to evaluate the event
    if ((modemStat & UART_MSR_CTS) != 0) events |= EV_CTS;
    if ((modemStat & UART_MSR_DSR) != 0) events |= EV_DSR;
    if ((modemStat & UART_MSR_DCD) != 0) events |= EV_RLSD;

    // Let MDD know if something happen
    if (events != 0) EvaluateEventFlag(pPdd->pMdd, events);

    DEBUGMSG(ZONE_FUNCTION, (L"-ReadModemStat(%02x)\r\n", modemStat));

    return modemStat;
}

//------------------------------------------------------------------------------
//
//  Function:  SetBaudRate
//
//  This function sets baud rate.
//
static
BOOL
SetBaudRate(
            UARTPDD *pPdd,
            ULONG baudRate
            )
{
    BOOL rc = FALSE;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    USHORT divider;
    UCHAR mdr1;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetBaudRate(rate:%d,membase:0x%x)\r\n",baudRate, pPdd->memBase[0]));

    // Calculate mode and divider
    if (baudRate < 300)
    {
        goto cleanUp;
    }
    else if  (baudRate <= 230400 || baudRate == 3000000)
    {
        mdr1 = UART_MDR1_UART16;
        divider = (USHORT)(pPdd->frequency/(baudRate * 16));
    }
    else if (baudRate <= 3686400)
    {
        mdr1 = UART_MDR1_UART13;
        divider = (USHORT)(pPdd->frequency/(baudRate * 13));
    }
    else
    {
        goto cleanUp;
    }
  
    // Get UART lock
    EnterCriticalSection(&pPdd->hwCS);

    // Disable UART
    OUTREG8(&pUartRegs->MDR1, UART_MDR1_DISABLE);

    // Set new divisor
    SETREG8(&pUartRegs->LCR, UART_LCR_DIV_EN);
    OUTREG8(&pUartRegs->DLL, (UCHAR)(divider >> 0));
    OUTREG8(&pUartRegs->DLH, (UCHAR)(divider >> 8));
    CLRREG8(&pUartRegs->LCR, UART_LCR_DIV_EN);
    // Enable UART
    OUTREG8(&pUartRegs->MDR1, mdr1);

    // Free UART lock

    LeaveCriticalSection(&pPdd->hwCS);

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SetBaudRate()=%d\r\n",rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SetWordLength
//
//  This function sets word length.
//
static
BOOL
SetWordLength(
              UARTPDD *pPdd,
              UCHAR wordLength
              )
{
    BOOL rc = FALSE;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lineCtrl;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetWordLength(%d)\r\n",wordLength));

    if ((wordLength < 5) || (wordLength > 8)) goto cleanUp;

    EnterCriticalSection(&pPdd->hwCS);

    lineCtrl = INREG8(&pUartRegs->LCR);
    lineCtrl = (lineCtrl & ~0x03)|(wordLength - 5);
    OUTREG8(&pUartRegs->LCR, lineCtrl);

    LeaveCriticalSection(&pPdd->hwCS);

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SetWordLength()=%d\r\n",rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SetParity
//
//  This function sets parity.
//
static
BOOL
SetParity(
          UARTPDD *pPdd,
          UCHAR parity
          )
{
    BOOL rc = FALSE;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lineCtrl;
    UCHAR mask;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetParity(%d)\r\n",parity));

    switch (parity)
    {
    case NOPARITY:
        mask = 0;
        break;
    case ODDPARITY:
        mask = UART_LCR_PARITY_EN | (0 << 4);
        break;
    case EVENPARITY:
        mask = UART_LCR_PARITY_EN | (1 << 4);
        break;
    case MARKPARITY:
        mask = UART_LCR_PARITY_EN | (2 << 4);
        break;
    case SPACEPARITY:
        mask = UART_LCR_PARITY_EN | (3 << 4);
        break;
    default:
        goto cleanUp;
    }

    EnterCriticalSection(&pPdd->hwCS);

    lineCtrl = INREG8(&pUartRegs->LCR);
    lineCtrl &= ~(UART_LCR_PARITY_EN);
    lineCtrl &= ~(UART_LCR_PARITY_TYPE_1|UART_LCR_PARITY_TYPE_2);
    lineCtrl |= mask;
    OUTREG8(&pUartRegs->LCR, lineCtrl);

    LeaveCriticalSection(&pPdd->hwCS);

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SetParity()=%d\r\n",rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SetStopBits
//
//  This function sets stop bits.
//
static
BOOL
SetStopBits(
            UARTPDD *pPdd,
            UCHAR stopBits
            )
{
    BOOL rc = FALSE;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lineCtrl;
    UCHAR mask;

    DEBUGMSG(ZONE_FUNCTION, (L"+SetStopBits(%d)\r\n",stopBits));

    switch (stopBits)
    {
    case ONESTOPBIT:
        mask = 0;
        break;
    case ONE5STOPBITS:
    case TWOSTOPBITS:
        mask = UART_LCR_NB_STOP;
        break;
    default:
        goto cleanUp;
    }

    EnterCriticalSection(&pPdd->hwCS);

    lineCtrl = INREG8(&pUartRegs->LCR);
    lineCtrl = (lineCtrl & ~UART_LCR_NB_STOP)|mask;
    OUTREG8(&pUartRegs->LCR, lineCtrl);

    LeaveCriticalSection(&pPdd->hwCS);

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SetStopBits()=%d\r\n",rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  GetSerialObject
//
//  This function returns a pointer to a HWOBJ structure, which contains
//  the correct function pointers and parameters for the relevant PDD layer's
//  hardware interface functions.
//
PHWOBJ
GetSerialObject(
                DWORD index
                )
{
    PHWOBJ pHWObj;

    UNREFERENCED_PARAMETER(index);
    DEBUGMSG(ZONE_FUNCTION, (L"+GetSerialObject(%d)\r\n", index));

    // Allocate space for the HWOBJ.
    pHWObj = malloc(sizeof(HWOBJ));
    if (pHWObj == NULL) goto cleanUp;

    // Fill in the HWObj structure
    pHWObj->BindFlags = THREAD_AT_OPEN;
    pHWObj->dwIntID = 0;
    pHWObj->pFuncTbl = &g_pddVTbl;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-GetSerialObject()=0x%x\r\n", pHWObj));
    return pHWObj;
}

//------------------------------------------------------------------------------
//
//  Function:  InitializeUART
//
//  This function initializes a UART register.
//
static VOID InitializeUART(UARTPDD *pPdd)
{
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    // Reset UART & wait until it completes
    OUTREG8(&pUartRegs->SYSC, UART_SYSC_RST);
    while ((INREG8(&pUartRegs->SYSS) & UART_SYSS_RST_DONE) == 0);

    // Enable wakeup
    // REG: turning off Auto Idle and turning on Smart Idle
    OUTREG8(
    &pUartRegs->SYSC,
    // Try turn on force idle, smart idle or turn on no idle
    // Lets configure force idle here we will change this in HWopen.
    UART_SYSC_IDLE_FORCE|UART_SYSC_WAKEUP_ENABLE|UART_SYSC_AUTOIDLE
    );

    // Ensure baud rate generator is off
    OUTREG8(&pUartRegs->LCR, UART_LCR_DLAB);
    OUTREG8(&pUartRegs->DLL, 0);
    OUTREG8(&pUartRegs->DLH, 0);

    // Select UART mode
    OUTREG8(&pUartRegs->MDR1, UART_MDR1_UART16);


    // Line control: configuration mode B
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_CONFIG_B);
    // Enable access to IER bits 4-7, FCR bits 4-5 and MCR bits 5-7
    SETREG8(&pUartRegs->EFR, UART_EFR_ENHANCED_EN);

    // Line control: operational mode
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_OPERATIONAL);

    // Enable sleep mode
    // Do not enable sleep mode hardware flow control will have problem
   // OUTREG8(&pUartRegs->IER, UART_IER_SLEEP_MODE);

    // Enable access to TCR and TLR
    SETREG8(&pUartRegs->MCR, UART_MCR_TCR_TLR);
    // Start receive when 32 bytes in FIFO, halt when 60 byte in FIFO
    OUTREG8(
        &pUartRegs->TCR,
        UART_TCR_RX_FIFO_TRIG_START_24|UART_TCR_RX_FIFO_TRIG_HALT_40
        );

    // This will create a space of 60 bytes in the FIFO for TX
    // Later we set FCR[4:5] so that the space is 63 bytes
    // we adjusted the TX DMA frame size to be 63 so we don't overrun our fifo
    if(pPdd->RxDmaInfo)
    {
        // if RxDMA is enabled, set up the MSBs of RX_FIFO_TRIG according
        // to the value in pPdd->dwRxFifoTriggerLevel
        BYTE    bRxTrigDMA = (BYTE)((pPdd->dwRxFifoTriggerLevel >> 2) << 4);

        OUTREG8(&pUartRegs->TLR, UART_TLR_TX_FIFO_TRIG_DMA_0 | bRxTrigDMA);
    }
    else
    {
        OUTREG8(&pUartRegs->TLR, UART_TLR_TX_FIFO_TRIG_DMA_0);
    }

    // Disable access to TCR and TLR
    CLRREG8(&pUartRegs->MCR, UART_MCR_TCR_TLR);

    pPdd->CurrentSCR = UART_SCR_TX_TRIG_GRANU1 | UART_SCR_RX_TRIG_GRANU1;
    pPdd->CurrentFCR = 0;

    if(pPdd->RxDmaInfo || pPdd->TxDmaInfo)
    {
        //pPdd->CurrentSCR |= UART_SCR_DMA_MODE_CTL;
        pPdd->CurrentSCR |=
                            UART_SCR_DMA_MODE_CTL |
                            UART_SCR_DMA_MODE_2_MODE1 |
                            UART_SCR_TX_EMPTY_CTL;
        pPdd->CurrentFCR |= UART_FCR_DMA_MODE;
    }

    OUTREG8(&pPdd->pUartRegs->SCR, pPdd->CurrentSCR);

    pPdd->intrMask = UART_IER_RHR;
    pPdd->CurrentFCR |= 
                        UART_FCR_TX_FIFO_LSB_1 |
                        UART_FCR_FIFO_EN;
    if (pPdd->RxDmaInfo == NULL)
    {
        pPdd->CurrentFCR |= UART_FCR_RX_FIFO_LSB_1;
    }
    else
    {
        // if RxDMA is enabled, set up the LSBs of RX_FIFO_TRG according
        // to the value in pPdd->dwRxFifoTriggerLevel
        pPdd->CurrentFCR |= ((pPdd->dwRxFifoTriggerLevel & 0x03) << 6);
    }


    OUTREG8(&pUartRegs->FCR, pPdd->CurrentFCR);

    // Line control: configuration mode B
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_CONFIG_B);
    // Disable access to IER bits 4-7, FCR bits 4-5 and MCR bits 5-7
    CLRREG8(&pUartRegs->EFR, UART_EFR_ENHANCED_EN);
    // Line control: operational mode
    OUTREG8(&pUartRegs->LCR, UART_LCR_MODE_OPERATIONAL);

    // Set default LCR 8 bits, 1 stop, no parity
    SETREG8(&pUartRegs->LCR, UART_LCR_CHAR_LENGTH_8BIT);
}

//------------------------------------------------------------------------------
//
//  Function:  SetDCB
//
//  This function sets the COM Port configuration according the DCB.
//
static BOOL SetDCB(UARTPDD *pPdd, DCB *pDCB, BOOL force)
{
    BOOL    bRC = FALSE;

    // If the device is open, scan for changes and do whatever
    // is needed for the changed fields.  if the device isn't
    // open yet, just save the DCB for later use by the open.
    if (pPdd->open)
    {
        if ((force == TRUE) || (pDCB->BaudRate != pPdd->dcb.BaudRate))
        {
            if (!SetBaudRate(pPdd, pDCB->BaudRate)) goto cleanUp;
        }

        if ((force == TRUE) || (pDCB->ByteSize != pPdd->dcb.ByteSize))
        {
            if (!SetWordLength(pPdd, pDCB->ByteSize)) goto cleanUp;
        }

        if ((force == TRUE) || (pDCB->Parity != pPdd->dcb.Parity))
        {
            if (!SetParity(pPdd, pDCB->Parity)) goto cleanUp;
        }

        if ((force == TRUE) || (pDCB->StopBits != pPdd->dcb.StopBits))
        {
            if (!SetStopBits(pPdd, pDCB->StopBits)) goto cleanUp;
        }

        // Enable hardware auto RST/CTS modes...
        if (pPdd->hwMode)
        {
            if (pDCB->fRtsControl == RTS_CONTROL_HANDSHAKE)
            {
                if (!SetAutoRTS(pPdd, TRUE)) goto cleanUp;
            }
            else
            {
                if (!SetAutoRTS(pPdd, FALSE)) goto cleanUp;
            }
            if (pDCB->fOutxCtsFlow)
            {
                if (!SetAutoCTS(pPdd, TRUE)) goto cleanUp;
            }
            else
            {
                if (!SetAutoCTS(pPdd, FALSE)) goto cleanUp;
            }

        }

        bRC = TRUE;
    }

cleanUp:
    return bRC;
}

//------------------------------------------------------------------------------
//
//  Function:  PowerThreadProc
//
//  This Thread checks to see if the power can be disabled.
//
DWORD
PowerThreadProc(
    void *pParam
    )
{
    DWORD nTimeout = INFINITE;
    UARTPDD *pPdd = (UARTPDD *)pParam;


    for(;;)
    {
        WaitForSingleObject(pPdd->hPowerEvent, nTimeout);

        if (pPdd->bExitPowerThread == TRUE) break;
        // serialize access to power state changes
        EnterCriticalSection(&pPdd->hwCS);

        // by the time this thread got the hwCS hPowerEvent may
        // have gotten resignaled.  Clear the event to  make
        // sure the Power thread isn't awaken prematurely
        //
        ResetEvent(pPdd->hPowerEvent);

        // check if we need to reset the timer
        if (pPdd->RxTxRefCount == 0)
        {
            // We disable the AutoIdle and put Uart into power
            // force Idle mode only when this thread wakes-up
            // twice in a row with no RxTxRefCount = 0
            // This is achieved by using the bDisableAutoIdle
            // flag to determine if power state changed since
            // the last time this thread woke-up
            //
            if (pPdd->bDisableAutoIdle == TRUE)
            {
                // check to make sure the uart clocks are on
                if (pPdd->currentDX >= D3) 
                {
                //RETAILMSG(1,(TEXT("UART:Switching to Force Idle \r")));
                OUTREG8(
                        &pPdd->pUartRegs->SYSC,
                        // turn on force idle, to allow full retention
                        UART_SYSC_IDLE_FORCE|UART_SYSC_WAKEUP_ENABLE|UART_SYSC_AUTOIDLE
                        );
                }
                nTimeout = INFINITE;
            }
            else
            {
                // wait for activity time-out before shutting off power.
                pPdd->bDisableAutoIdle = TRUE;
                nTimeout = pPdd->hwTimeout;
            }
        }
        else
        {
            nTimeout = INFINITE;
        }

        LeaveCriticalSection(&pPdd->hwCS);
    }
    return 1;
}

ULONG
HWRxNoDMAIntr(
              VOID *pvContext, UCHAR *pRxBuffer, ULONG *pLength
              )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    ULONG count = *pLength;
    UCHAR lineStat, rxChar;
    BOOL rxFlag, replaceParityError;

    *pLength = 0;
    rxFlag = FALSE;
    replaceParityError = (pPdd->dcb.fErrorChar != '\0') && pPdd->dcb.fParity;
    SetAutoIdle(pPdd, TRUE);
    while (count > 0)
    {
        // Get line status register
        lineStat = ReadLineStat(pPdd);
        // If there isn't more chars exit loop
        if ((lineStat & UART_LSR_RX_FIFO_E) == 0) break;

        // Get received char
        rxChar = INREG8(&pUartRegs->RHR);
        // Ignore char in DSR is low and we care about it
        if (pPdd->dcb.fDsrSensitivity &&
            ((ReadModemStat(pPdd) & UART_MSR_NDSR) == 0))
        {
            continue;
        }

        // Ignore NUL char if requested
        if ((rxChar == '\0') && pPdd->dcb.fNull) continue;

        // Replace char with parity error
        if (replaceParityError && ((lineStat & UART_LSR_RX_PE) != 0))
        {
            rxChar = pPdd->dcb.ErrorChar;
        }
        // See if we need to generate an EV_RXFLAG
        if (rxChar == pPdd->dcb.EvtChar) rxFlag = TRUE;

        // Store it to buffer
        *pRxBuffer++ = rxChar;
        (*pLength)++;
        count--;
    }
    SetAutoIdle(pPdd, FALSE);

    // Send event to MDD
    if (rxFlag) EvaluateEventFlag(pPdd->pMdd, EV_RXFLAG);

    return count;
}

//------------------------------------------------------------------------------
//
//  Function:  HWRxIntr
//
//  This function gets several characters from the hardware receive buffer
//  and puts them in a buffer provided via the second argument. It returns
//  the number of bytes lost to overrun.
//
static
ULONG
HWRxIntr(
         VOID *pvContext, UCHAR *pRxBuffer, ULONG *pLength
         )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    ULONG count;

    DEBUGMSG(ZONE_THREAD, (
        L"+HWRxIntr(0x%08x, 0x%08x, %d)\r\n", pvContext, pRxBuffer,
        *pLength
        ));

    // Somehow BT driver was trying to read data after setting the UART to D3 and causing
    // UART driver to crash in this routine. Check the power state before doing anything.
    if (pPdd->currentDX >= D3)
    {
        *pLength = 0;
        count = 0;
        goto cleanUp;
    }

    // Check to see if we are in wake up mode. If so we will
    // just report one character we have received a byte.
    if (pPdd->wakeUpMode)
    {
        *pRxBuffer = (UCHAR)pPdd->wakeUpChar;
        *pLength = 1;
        count = 0;
        goto cleanUp;
    }

    if (pPdd->RxDmaInfo)
        HWRxDMAIntr(pvContext, pRxBuffer, pLength);
    else
        HWRxNoDMAIntr(pvContext, pRxBuffer, pLength);

    // Clear overrun counter and use this value as return code
    count = pPdd->overrunCount;
    pPdd->overrunCount = 0;

cleanUp:
    DEBUGMSG(ZONE_THREAD, (
        L"-HWRxIntr(count = %d, length = %d)\r\n", count, *pLength
        ));
    return count;
}


//------------------------------------------------------------------------------
//
//  Function:  HWModemIntr
//
//  This function is called from the MDD whenever INTR_MODEM is returned
//  by HWGetInterruptType which indicate change in modem register.
//
static
VOID
HWModemIntr(
            VOID *pvContext
            )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    UCHAR modemStat;


    DEBUGMSG(ZONE_THREAD, (L"+HWModemIntr(0x%08x)\r\n", pvContext));

    // Get actual modem status
    modemStat = ReadModemStat(pPdd);

    // If we are currently flowed off via CTS or DSR, then
    // we better signal the TX thread when one of them changes
    // so that TX can resume sending.

    EnterCriticalSection(&pPdd->hwCS);


    if (pPdd->flowOffDSR && ((modemStat & UART_MSR_NDSR) != 0))
    {
        // Clear flag
        pPdd->flowOffDSR = FALSE;
        // DSR is set, so go ahead and resume sending
        SetEvent( ((PHW_INDEP_INFO)pPdd->pMdd)->hSerialEvent);

        // Then simulate a TX intr to get things moving
        pPdd->addTxIntr = TRUE;
    }

    if (pPdd->flowOffCTS && ((modemStat & UART_MSR_NCTS) != 0))
    {
        pPdd->flowOffCTS = FALSE;
        //CTS is set, so go ahead and resume sending
        //Need to simulate an interrupt
        SetEvent( ((PHW_INDEP_INFO)pPdd->pMdd)->hSerialEvent);

        // Then simulate a TX intr to get things moving
        pPdd->addTxIntr = TRUE;
    }


    LeaveCriticalSection(&pPdd->hwCS);
    DEBUGMSG(ZONE_THREAD, (L"-HWModemIntr\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWLineIntr
//
//  This function is called from the MDD whenever INTR_LINE is returned by
//  HWGetInterruptType which indicate change in line status register.
//
static
VOID
HWLineIntr(
           VOID *pvContext
           )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(ZONE_THREAD, (L"+HWLineIntr(0x%08x)\r\n", pvContext));


    if (pPdd->open == TRUE)
    {
        ReadLineStat(pPdd);

        EnterCriticalSection(&pPdd->hwCS);

        // Reset receiver
        OUTREG8(&pUartRegs->FCR, pPdd->CurrentFCR | UART_FCR_RX_FIFO_CLEAR);

        // Do we need to read RESUME register in case there was an overrun error in the FIFO?
        //INREG8(&pUartRegs->RESUME);

        // Did we not just clear the FIFO?       
        // Read all character in fifo
        while ((ReadLineStat(pPdd) & UART_LSR_RX_FIFO_E) != 0)
        {
            INREG8(&pUartRegs->RHR);
        }

        LeaveCriticalSection(&pPdd->hwCS);
    }

    DEBUGMSG(ZONE_THREAD, (L"-HWLineIntr\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWGetRxBufferSize
//
//  This function returns the size of the hardware buffer passed to
//  the interrupt initialize function.
//
static
ULONG
HWGetRxBufferSize(
                  VOID *pvContext
                  )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+HWGetRxBufferSize()\r\n")));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-HWGetRxBufferSize()\r\n")));
    return pPdd->rxBufferSize;
}

//------------------------------------------------------------------------------
//
//  Function:  HWPowerOff
//
//  This function is called from COM_PowerOff. Implementation support power
//  management IOCTL, so there is nothing to do there.
//
static
BOOL
HWPowerOff(
           VOID *pvContext
           )
{
    UNREFERENCED_PARAMETER(pvContext);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HWPowerOn
//
//  This function is called from COM_PowerOff. Implementation support power
//  management IOCTL, so there is nothing to do there.
//
static
BOOL
HWPowerOn(
          VOID *pvContext
          )
{
    UNREFERENCED_PARAMETER(pvContext);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HWClearDTR
//
//  This function clears DTR.
//
static
VOID
HWClearDTR(
           VOID *pvContext
           )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;


    DEBUGMSG(ZONE_FUNCTION, (L"+HWClearDTR(0x%08x)\r\n", pvContext));

    EnterCriticalSection(&pPdd->hwCS);

    pPdd->currentMCR &= ~UART_MCR_DTR;
    CLRREG8(&pUartRegs->MCR, UART_MCR_DTR);

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWClearDTR\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  HWSetDTR
//
//  This function sets DTR.
//
static
VOID
HWSetDTR(
         VOID *pvContext
         )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;


    DEBUGMSG(ZONE_FUNCTION, (L"+HWSetDTR(0x%08x)\r\n", pvContext));

    EnterCriticalSection(&pPdd->hwCS);

    pPdd->currentMCR |= UART_MCR_DTR;
    SETREG8(&pUartRegs->MCR, UART_MCR_DTR);

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWSetDTR\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  HWClearRTS
//
//  This function clears RTS.
//
static
VOID
HWClearRTS(
           VOID *pvContext
           )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(TRUE||ZONE_FUNCTION, (L"+HWClearRTS(0x%08x)\r\n", pvContext));

    if (pPdd->autoRTS)
    {
        EnterCriticalSection(&pPdd->hwCS);

        // We should disable RX interrupt, this will result in auto RTS
        pPdd->intrMask &= ~UART_IER_RHR;
        OUTREG8(&pUartRegs->IER, pPdd->intrMask);

        LeaveCriticalSection(&pPdd->hwCS);
    }
    else
    {
        EnterCriticalSection(&pPdd->hwCS);

        pPdd->currentMCR &= ~UART_MCR_RTS;
        CLRREG8(&pUartRegs->MCR, UART_MCR_RTS);

        LeaveCriticalSection(&pPdd->hwCS);
    }

    DEBUGMSG(ZONE_FUNCTION, (L"-HWClearRTS\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWSetRTS
//
//  This function sets RTS.
//
static
VOID
HWSetRTS(
         VOID *pvContext
         )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(TRUE||ZONE_FUNCTION, (L"+HWSetRTS(0x%08x)\r\n", pvContext));

    if (pPdd->autoRTS)
    {
        EnterCriticalSection(&pPdd->hwCS);

        // We should enable RX interrupt
        pPdd->intrMask |= UART_IER_RHR;
        OUTREG8(&pUartRegs->IER, pPdd->intrMask);

        LeaveCriticalSection(&pPdd->hwCS);
    }
    else
    {
        EnterCriticalSection(&pPdd->hwCS);

        pPdd->currentMCR |= UART_MCR_RTS;
        SETREG8(&pUartRegs->MCR, UART_MCR_RTS);

        LeaveCriticalSection(&pPdd->hwCS);
    }

    DEBUGMSG(ZONE_FUNCTION, (L"-HWSetRTS\n"));
}

//------------------------------------------------------------------------------

static
BOOL
HWEnableIR(
           VOID *pvContext,
           ULONG baudRate
           )
{
    UNREFERENCED_PARAMETER(pvContext);
    UNREFERENCED_PARAMETER(baudRate);
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+HWEnableIR()\r\n")));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-HWEnableIR()\r\n")));
    return TRUE;
}

//------------------------------------------------------------------------------

static
BOOL
HWDisableIR(
            VOID *pvContext
            )
{

    UNREFERENCED_PARAMETER(pvContext);
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+HWDisableIR()\r\n")));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-HWDisableIR()\r\n")));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HWClearBreak
//
//  This function clears break.
//
static
VOID
HWClearBreak(
             VOID *pvContext
             )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWClearBreak(0x%08x)\r\n", pvContext
        ));

    EnterCriticalSection(&pPdd->hwCS);

    CLRREG8(&pUartRegs->LCR, UART_LCR_BREAK_EN);

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWClearBreak\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWSetBreak
//
//  This function sets break.
//
static
VOID
HWSetBreak(
           VOID *pvContext
           )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;

    DEBUGMSG(ZONE_FUNCTION, (L"+HWSetBreak(0x%08x)\r\n", pvContext));

    EnterCriticalSection(&pPdd->hwCS);

    SETREG8(&pUartRegs->LCR, UART_LCR_BREAK_EN);

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWSetBreak\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWReset
//
//  This function performs any operations associated with a device reset.
//
static
VOID
HWReset(
        VOID *pvContext
        )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;


    DEBUGMSG(ZONE_FUNCTION, (L"+HWReset(0x%08x)\r\n", pvContext));

    EnterCriticalSection(&pPdd->hwCS);

    // Enable interrupts
    pPdd->intrMask = UART_IER_LINE|UART_IER_MODEM|UART_IER_RHR;
    OUTREG8(&pUartRegs->IER, pPdd->intrMask);

    LeaveCriticalSection(&pPdd->hwCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWReset\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWGetModemStatus
//
//  This function retrieves modem status.
//
static
VOID
HWGetModemStatus(
                 VOID *pvContext,
                 ULONG *pModemStat
                 )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    UCHAR modemStat;


    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWGetModemStatus(0x%08x)\r\n", pvContext
        ));

    modemStat = ReadModemStat(pPdd);

    *pModemStat = 0;
    if ((modemStat & UART_MSR_NCTS) != 0) *pModemStat |= MS_CTS_ON;
    if ((modemStat & UART_MSR_NDSR) != 0) *pModemStat |= MS_DSR_ON;
    if ((modemStat & UART_MSR_NCD) != 0) *pModemStat |= MS_RLSD_ON;

    DEBUGMSG(TRUE||ZONE_FUNCTION, (
        L"-HWGetModemStatus(0x%08x/%02x)\r\n", *pModemStat, modemStat
        ));

}

//------------------------------------------------------------------------------
//
//  Function:  HWXmitComChar
//
//  This function transmits a char immediately
//
static
BOOL
HWXmitComChar(
              VOID *pvContext,
              UCHAR ch
              )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    DEBUGMSG(ZONE_FUNCTION, (L"+HWXmitComChar(0x%08x, %d)\r\n", pvContext, ch));

    EnterCriticalSection(&pPdd->txCS);

    // We know THR will eventually empty
    for(;;)
    {
        EnterCriticalSection(&pPdd->hwCS);


        // Write the character if we can
        if ((ReadLineStat(pPdd) & UART_LSR_TX_FIFO_E) != 0)
        {

            // Tell the tx interrupt handler that we are waiting for
            // a TX interrupt
            pPdd->bHWXmitComCharWaiting = TRUE;

            // FIFO is empty, send this character
            OUTREG8(&pPdd->pUartRegs->THR, ch);
            // Enable TX interrupt

            pPdd->intrMask |= UART_IER_THR;

            OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

            LeaveCriticalSection(&pPdd->hwCS);
            break;
        }

        // If we couldn't write the data yet, then wait for a TX interrupt

        pPdd->intrMask |= UART_IER_THR;
        OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

        LeaveCriticalSection(&pPdd->hwCS);

        // Wait until the TX interrupt has signalled
        WaitForSingleObject(pPdd->txEvent, 1000);

    }
    LeaveCriticalSection(&pPdd->txCS);

    DEBUGMSG(ZONE_FUNCTION, (L"-HWXmitComChar\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HWGetStatus
//
//  This function is called by the MDD to retrieve the contents of
//  COMSTAT structure.
//
static
ULONG
HWGetStatus(
            VOID *pvContext,
            COMSTAT *pComStat
            )
{
    ULONG rc = (ULONG)-1;
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWGetStatus(0x%08x, 0x%08x)\r\n", pvContext, pComStat
        ));

    if (pComStat == NULL) goto cleanUp;

    pComStat->fCtsHold = pPdd->flowOffCTS ? 1 : 0;
    pComStat->fDsrHold = pPdd->flowOffDSR ? 1 : 0;
    pComStat->cbInQue  = 0;
    pComStat->cbOutQue = 0;

    rc = pPdd->commErrors;
    pPdd->commErrors = 0;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HWGetStatus(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  HWGetCommProperties
//
static
VOID
HWGetCommProperties(
                    VOID *pvContext,
                    COMMPROP *pCommProp
                    )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWGetCommProperties(0x%08x, 0x%08x)\r\n", pvContext,
        pCommProp
        ));

    memset(pCommProp, 0, sizeof(COMMPROP));
    pCommProp->wPacketLength = 0xffff;
    pCommProp->wPacketVersion = 0xffff;
    pCommProp->dwServiceMask = SP_SERIALCOMM;
    pCommProp->dwMaxTxQueue = 16;
    pCommProp->dwMaxRxQueue = 16;
    pCommProp->dwMaxBaud = BAUD_USER;
    pCommProp->dwProvSubType = PST_RS232;
    pCommProp->dwProvCapabilities =
        // On EVM DTR/DSR are connected together (but cannot be controlled) and RI/CD are not wired.
        // PCF_DTRDSR | PCF_RLSD |
        PCF_INTTIMEOUTS | PCF_PARITY_CHECK |
        PCF_SETXCHAR | PCF_SPECIALCHARS | PCF_TOTALTIMEOUTS |
        PCF_XONXOFF;
    pCommProp->dwSettableParams =
        // On GSample RI/CD are not wired
        // SP_RLSD |
        SP_BAUD | SP_DATABITS | SP_HANDSHAKING | SP_PARITY |
        SP_PARITY_CHECK | SP_STOPBITS;
    pCommProp->dwSettableBaud =
        /* not supported - BAUD_075 | BAUD_110 | BAUD_150 | */ BAUD_300 | BAUD_600 | BAUD_1200 |
        BAUD_1800 | BAUD_2400 | BAUD_4800 | BAUD_7200 | BAUD_9600 | BAUD_14400 |
        BAUD_19200 | BAUD_38400 | BAUD_57600 | BAUD_115200 | BAUD_USER;
    pCommProp->wSettableData =
        DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
    pCommProp->wSettableStopParity =
        STOPBITS_10 | STOPBITS_20 |
        PARITY_NONE | PARITY_ODD | PARITY_EVEN | PARITY_SPACE |
        PARITY_MARK;

    if (pPdd->dwRtsCtsEnable)
        pCommProp->dwProvCapabilities |= PCF_RTSCTS;

    DEBUGMSG(ZONE_FUNCTION, (L"-HWGetCommProperties\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  HWSetDCB
//
//  This function sets new values for DCB. It gets a DCB from the MDD and
//  compare it to the current DCB, and if any fields have changed take
//  appropriate action.
//
static
BOOL
HWSetDCB(
         VOID *pvContext,
         DCB *pDCB
         )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    BOOL rc = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWSetDCB(0x%08x, 0x%08x\r\n", pvContext, pDCB
        ));
    // Check for same XON/XOFF characters...
    if (((pDCB->fOutX != 0) || (pDCB->fInX != 0)) &&
        (pDCB->XonChar == pDCB->XoffChar))
    {
        goto cleanUp;
    }

    // Update COMM port setting according DCB. SetDCB checks the flags in pPdd->pDCB
    // before updating the register value if the 3rd parameter is FALSE.
    rc = SetDCB(pPdd, pDCB, FALSE);

    // Now that we have done the right thing, store this DCB
    if (rc == TRUE)
    {
        pPdd->dcb = *pDCB;
    }


cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HWSetDCB(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  HWSetCommTimeouts
//
//  This function sets new values for the commTimeouts structure.
//
static
BOOL
HWSetCommTimeouts(
                  VOID *pvContext,
                  COMMTIMEOUTS *pCommTimeouts
                  )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    DEBUGMSG(ZONE_FUNCTION, (L"+HWSetCommTimeouts()\r\n"));
    pPdd->commTimeouts = *pCommTimeouts;
    DEBUGMSG(ZONE_FUNCTION, (L"-HWSetCommTimeouts()\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HWIOCtl
//
//  This function process PDD IOCtl calls
//
static BOOL HWIOCtl(
                    VOID *pvContext,
                    DWORD code,
                    UCHAR *pInBuffer,
                    DWORD inSize,
                    UCHAR *pOutBuffer,
                    DWORD outSize,
                    DWORD *pOutSize
                    )
{
    BOOL rc = FALSE;
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+HWIOCtl()\r\n")));

    switch (code)
    {
    case IOCTL_SERIAL_SET_TIMEOUTS:
        // Check input parameters
        if ((pInBuffer == NULL) || (inSize < sizeof(COMMTIMEOUTS)) ||
            !CeSafeCopyMemory(
            &pPdd->commTimeouts, pInBuffer, sizeof(COMMTIMEOUTS)
            ))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;

    case IOCTL_SERIAL_GET_TIMEOUTS:
        if ((pInBuffer == NULL) || (inSize < sizeof(COMMTIMEOUTS)) ||
            !CeSafeCopyMemory(
            pInBuffer, &pPdd->commTimeouts, sizeof(COMMTIMEOUTS)
            ))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;

    case IOCTL_POWER_CAPABILITIES:
        if ((pOutBuffer == NULL) || (outSize < sizeof(POWER_CAPABILITIES)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        __try
        {
            POWER_CAPABILITIES *pCaps = (POWER_CAPABILITIES*)pOutBuffer;
            memset(pCaps, 0, sizeof(POWER_CAPABILITIES));
            pCaps->DeviceDx = DX_MASK(D0)|DX_MASK(D3)|DX_MASK(D4);
            if ((outSize >= sizeof(DWORD)) && (pOutSize != NULL))
            {
                *pOutSize = sizeof(POWER_CAPABILITIES);
            }
            rc = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: UART::HWIOCtl: "
                L"Exception in IOCTL_POWER_CAPABILITIES\r\n"
                ));
        }
        break;

    case IOCTL_POWER_QUERY:
        if ((pOutBuffer == NULL) ||
            (outSize < sizeof(CEDEVICE_POWER_STATE)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        __try
        {
            CEDEVICE_POWER_STATE dx = *(CEDEVICE_POWER_STATE*)pOutBuffer;
            rc = VALID_DX(dx);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: UART::HWIOCtl: "
                L"Exception in IOCTL_POWER_QUERY\r\n"
                ));
        }
        break;

    case IOCTL_POWER_SET:
        if ((pOutBuffer == NULL) ||
            (outSize < sizeof(CEDEVICE_POWER_STATE)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        __try
        {
            CEDEVICE_POWER_STATE dx = *(CEDEVICE_POWER_STATE*)pOutBuffer;
            pPdd->externalDX = dx;
            SetPower(pPdd, dx);
            *(CEDEVICE_POWER_STATE*)pOutBuffer = pPdd->externalDX;
            *pOutSize = sizeof(CEDEVICE_POWER_STATE);
            
            rc = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: UART::HWIOCtl: "
                L"Exception in IOCTL_POWER_SET\r\n"
                ));
        }
        break;

    case IOCTL_POWER_GET:
        if ((pOutBuffer == NULL) ||
            (outSize < sizeof(CEDEVICE_POWER_STATE)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        __try
        {
            *(CEDEVICE_POWER_STATE*)pOutBuffer = pPdd->externalDX;
            rc = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: UART::HWIOCtl: "
                L"Exception in IOCTL_POWER_GET\r\n"
                ));
        }
        break;

    case IOCTL_CONTEXT_RESTORE:
        RestoreUARTContext(pPdd);
        break;
   }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("-HWIOCtl()\r\n")));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  RxDmaStop
//
//  This function stops the Rx DMA, pulls all the data left in the FIFO and .
//  places them into the DMA buffer.
// 
//
static BOOL RxDmaStop(UARTPDD   *pPdd,
                      BOOL      bPurge)                      
{
    DWORD   dwCount     = 0;
    BYTE*   pbDataPtr   = NULL;

    DmaStop(pPdd->RxDmaInfo);

    pbDataPtr = DmaGetLastWritePos(pPdd->RxDmaInfo);
    while ((dwCount < MAX_RX_SERIALDMA_FRAMESIZE) && ((INREG8(&pPdd->pUartRegs->LSR) & UART_LSR_RX_FIFO_E) != 0))
    {
        pbDataPtr[dwCount++] = INREG8(&pPdd->pUartRegs->RHR);
    }
    
    if (dwCount >= MAX_RX_SERIALDMA_FRAMESIZE)
    {
        DEBUGMSG(ZONE_ERROR, (L"+HWGetInterruptType Pulled %d bytes. Shouldn't be here!!!\r\n", dwCount));
        //UART_RegDump(pPdd);
    }

    return UpdateDMARxPointer(pPdd, bPurge, dwCount);
}


//------------------------------------------------------------------------------
//
//  Function:  SetPower
//
//  This function sets power.
//
static BOOL
SetPower(
         UARTPDD *pPdd,
         CEDEVICE_POWER_STATE dx
         )
{
    BOOL rc = FALSE;
    int nTimeout = 1000;
   
    DEBUGMSG(ZONE_FUNCTION, (TEXT("UART:SetPower: D%d (curDx=D%d)\r\n"), dx, pPdd->currentDX));
    EnterCriticalSection(&pPdd->powerCS);
	
    // Device can't be set to lower power state than external
    if (dx < pPdd->externalDX) 
        dx = pPdd->externalDX;

    // Update state only when it is different from actual
    if (pPdd->currentDX != dx)
    {
        if (pPdd->currentDX <= D2 && dx >= D3)
        {
            // going from D0/D1/D2 to D3/D4
            
            // turn off transceiver
            if (pPdd->hGpio != NULL)
            {   
                // turn off xcvr while UART is still in D0/D1/D2 state because xcvr state
                // change can generate interrupts that need to be handled.
                if (pPdd->XcvrEnabledLevel)
                    GPIOClrBit(pPdd->hGpio, pPdd->XcvrEnableGpio);
                else
                    GPIOSetBit(pPdd->hGpio, pPdd->XcvrEnableGpio);
                // Delay to allow time for any interrupt processing due to change in xcvr state
                Sleep(25);
            }

            while((INREG8(&pPdd->pUartRegs->LSR) & UART_LSR_RX_FIFO_E) != 0) 
            {
                if(--nTimeout == 0) 
                {
                    // Timedout so just empty the fifo
                    OUTREG8(&pPdd->pUartRegs->FCR, pPdd->CurrentFCR | UART_FCR_RX_FIFO_CLEAR);
                }
            }

            //set event on power thread and put uart into force Idle

            if ( pPdd->RxTxRefCount )
                pPdd->RxTxRefCount = 0;

            pPdd->bDisableAutoIdle = TRUE;

            // Update current power state before triggering the power thread because
            // the power thread will set the UART's idle mode according to the 
            // current DX
            // We changed power state
            pPdd->currentDX = dx;

            if (pPdd->hPowerThread != NULL)
            {
                DWORD   dwCount = 0;

                SetEvent(pPdd->hPowerEvent);

                // Under some condition, the power thread wouldn't run until few
                // hundreds of ms after the event is set in which case the we ended
                // up turning off the UART before the power thread access the UART 
                // registers and causing the power thread to crash. To avoid the problem,
                // we wait until the power thread actually woke up from the event.
                do
                {
                    Sleep(5);
                } while ((WaitForSingleObject(pPdd->hPowerEvent, 1) != WAIT_TIMEOUT) && (dwCount++ < 1000));

                // Now the power woke up from the event, but we still need to give it
                // a chance to set the UART idle mode before we can turn off the power
                // to the UART module, otherwise we will generate exception in the 
                // power thread.
                Sleep(10);
            }

            if (pPdd->bDmaInitialize == TRUE)
            {
                DEBUGMSG(ZONE_FUNCTION, (TEXT("UART:SetPower: stop DMA\r\n")));
                RxDmaStop(pPdd, FALSE);
            }

            // If we are going to shut down the power (or clock?), we need to disable all 
            // interrupt before we do so otherwise the interrupt will keep kicking if there
            // is a pending interrupt because we wouldn't be able to mask the interrupt once
            // we turn off the power (or clock?).
            pPdd->savedIntrMask = pPdd->intrMask;
            pPdd->intrMask = 0;
            OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

            SOC_SET_DEVICE_POWER(pPdd, dx);
        }
        else if (pPdd->currentDX >= D3 && dx <= D2)
        {
            // going from D3/D4 to D0/D1/D2
            /* force it to D0 so that we can program the registers and restore context */
			/* once that is done, set the power state to requested level */
            SOC_SET_DEVICE_POWER(pPdd, D0);

            pPdd->intrMask = pPdd->savedIntrMask;
            OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

			/* dont know why we need the check dx != currentDX */
            if ((dx != pPdd->currentDX) && (pPdd->bDmaInitialize == TRUE))
            {
                DEBUGMSG(ZONE_FUNCTION, (L"UART:SetPower: Starting DMA\r\n"));
                UpdateDMARxPointer(pPdd, FALSE, 0);

                SetAutoIdle(pPdd, TRUE);
                DmaStart(pPdd->RxDmaInfo);
            }
			if (dx != D0)
                SOC_SET_DEVICE_POWER(pPdd, dx);

            pPdd->currentDX = dx;

            if (pPdd->hGpio != NULL)
            {   
                // Xcvr is off, power it on
                if (pPdd->XcvrEnabledLevel)
                    GPIOSetBit(pPdd->hGpio, pPdd->XcvrEnableGpio);
                else
                    GPIOClrBit(pPdd->hGpio, pPdd->XcvrEnableGpio);
            }
        }
        else if (pPdd->currentDX > dx)
        {
            // going from D4 to D3, D2 to D1, D1 to D0
            SOC_SET_DEVICE_POWER(pPdd, dx);
            pPdd->currentDX = dx;
        }
        else
        {
            // going from D3 to D4, D0 to D1, D1 to D2
            SOC_SET_DEVICE_POWER(pPdd, dx);
            pPdd->currentDX = dx;
        }
        
        rc = TRUE;
    }

    //RETAILMSG(1,(L"UART: -SetPower Device Power state D%d\r\n", pPdd->currentDX));
    LeaveCriticalSection(&pPdd->powerCS);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  IST_RxDMA
//
// Monitoring thread for end of RX DMA data
//
DWORD IST_RxDMA(void* pParam)
{
    UINT32 status;
    UARTPDD *pPdd = (UARTPDD *)pParam;

    //SetProcPermissions(0xFFFFFFFF);
    CeSetThreadPriority(GetCurrentThread(), 100);

    // register dma for interrupts
    if (DmaEnableInterrupts(pPdd->hRxDmaChannel, pPdd->hEventRxIstDma) == FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: IST_RxDMA: "
            L"Failed to enable DMA RX interrupt\r\n"
            ));
        goto cleanUp;
    }
    for(;;)
    {
        if (WaitForSingleObject(pPdd->hEventRxIstDma, INFINITE) == WAIT_OBJECT_0)
        {
            // Check if this thread is to shutdown
            if (pPdd->bExitThread == TRUE) goto cleanUp;
            // get cause of wake-up
            status = DmaGetStatus(pPdd->RxDmaInfo);
            if (status != 0)
            {
                if (status & (DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE))
                {
                    // clear dma interrupts
                    DmaClearStatus(pPdd->RxDmaInfo, status);
                    if (DmaInterruptDone(pPdd->hRxDmaChannel) == FALSE)
                    {
                        DEBUGMSG(ZONE_ERROR, (L"ERROR: IST_RxDMA: "
                            L"Failed to Signal DMA RX interrupt completion\r\n"
                            ));
                        goto cleanUp;
                    }

                    if ((status & (DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE)) == 0)
                    {
                        continue;
                    }

                    EnterCriticalSection(&pPdd->RxUpdatePtrCS);

                    if (status & DMA_CICR_BLOCK_IE )
                    {
                        SetAutoIdle(pPdd, FALSE);

                        if (IsDmaEnable(pPdd->RxDmaInfo))
                        {
                            DEBUGMSG(ZONE_FUNCTION, (L"IST_RxDMA: Got End of block Int stopping DMA\r\n"));
                            DmaStop(pPdd->RxDmaInfo);
                        }

                        if((UpdateDMARxPointer(pPdd, FALSE, 0) == TRUE) && (pPdd->open == TRUE))
                        {
                            SetAutoIdle(pPdd, TRUE);
                            DmaStart(pPdd->RxDmaInfo);
                            DEBUGMSG(TESTENABLE, (L"IST_RxDMA: End of block DMA Start\r\n"));
                        }
                        else
                        {
                            DEBUGMSG(ZONE_ERROR, (L"Error: IST_RxDMA: Shouldn't get to here!!!\r\n"));
                        }
                    }
                    else
                    {
                        DWORD   dwLastDMAWrite = (DWORD)DmaGetLastWritePos(pPdd->RxDmaInfo);

                        if (dwLastDMAWrite < ((DWORD)pPdd->pRxDmaBuffer + pPdd->RxDmaBufferSize))
                        {
                            pPdd->pRxDMALastWrite = (VOID*)dwLastDMAWrite;
                        }
                    }

                    pPdd->bRxDMASignaled = 1;
                    SetEvent( ((PHW_INDEP_INFO)pPdd->pMdd)->hSerialEvent);

                    LeaveCriticalSection(&pPdd->RxUpdatePtrCS);
                }
            }
        }
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-IST_RXDMA:\r\n"));
    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  UpdateDMARxPointer
//
//  This function updates internal dma offsets based on the current
//  read position of the mdd.
//
//  The dwOffset indicates the number of bytes were filled by the interrupt
//  handler when RX timeout occured.
//
static BOOL UpdateDMARxPointer(
                        UARTPDD *pPdd,
                        BOOL bPurge,
                        DWORD  dwOffSet)
{
    BOOL        bRet = FALSE;
    UINT32      nStartPos = 0;
    UINT32      nBufferSize = 0;
    PHW_INDEP_INFO  pSerialHead = (PHW_INDEP_INFO)pPdd->pMdd;
    UINT32      nMddRxPosHead = (UINT32)RxBuffRead(pSerialHead);
    UINT32      nMddRxPosTail = (UINT32)RxBuffWrite(pSerialHead);
    UINT32      nRxDmaBuffer = (UINT32)pPdd->pRxDmaBuffer;
    BOOL        bAllClear = (nMddRxPosHead == nMddRxPosTail);
    BOOL        bConfig = FALSE;

    if(IsDmaEnable(pPdd->RxDmaInfo))
    {
        DEBUGMSG(TESTENABLE, (L"UpdateDMARxPointer: DMA Runing\r\n"));
        goto cleanUp;
    }

    // If we are purging we set the last read pos equal to the last write pos
    if(bPurge)
    {
       nMddRxPosHead = nMddRxPosTail;
       bAllClear = TRUE;
    }

    nStartPos = (UINT32)DmaGetLastWritePos(pPdd->RxDmaInfo) + dwOffSet;
    DEBUGMSG(TESTENABLE, (L"UpdateDMARxPointer: CurrentDMALocation = 0x%x"
        L" nMddRxWIndex= 0x%x   nMddRxRIndex= 0x%x\r\n",
        nStartPos, nMddRxPosTail, nMddRxPosHead));

    // Since we have to setup DMA to transfer multiple of frame size (or Rx Fifo's 
    // trigger level) and we have to fill up the DMA buffer before wrap around, we 
    // setup the DMA to fill few bytes more after the DMA buffer (yes, we did
    // allocate that extra space, but just didn't tell MDD about that) at the end of
    // this routine and we will have to copy the extra data to the beginning of the 
    // DMA buffer when we are call to setup the DMA again.
    if (nStartPos >= (nRxDmaBuffer + pPdd->RxDmaBufferSize))
    {
        DWORD   dwCount = nStartPos - (nRxDmaBuffer + pPdd->RxDmaBufferSize);
        DEBUGMSG(TESTENABLE,
           (L"UpdateDMARxPointer: recv EndOfBlock DMA\r\n"));
        if (dwCount > 0)
        {
            pPdd->bRxWrapped   = TRUE;
            memcpy((BYTE*)nRxDmaBuffer, (BYTE*)(nRxDmaBuffer + pPdd->RxDmaBufferSize), dwCount);
    }
        nStartPos = nRxDmaBuffer + dwCount;
    }

    // check for start position if it is greater then MDD write position
    if(/*(nStartPos >= nMddRxPosTail) && */
        (nStartPos < (nRxDmaBuffer + pPdd->RxDmaBufferSize)) &&
        (nStartPos > nMddRxPosHead))
    {
        DEBUGMSG(TESTENABLE,
            (L"(nStartPos < (nRxDmaBuffer + pPdd->RxDmaBufferSize))&&"
            L"(nStartPos > nMddRxPosHead)\r\n"));
        nStartPos = nStartPos;
        nBufferSize = pPdd->RxDmaBufferSize - (nStartPos - nRxDmaBuffer);
        bConfig = TRUE;
        goto cleanUp;
    }
    // check for start position if less then MDD read position
    if (nStartPos < nMddRxPosHead)
    {
        DEBUGMSG(TESTENABLE, (L"(nStartPos < nMddRxPosHead)\r\n"));
        // adjust the buffer length to only extend to the last read
        // position of the mdd
        nStartPos = nStartPos;
        nBufferSize = (nMddRxPosHead - nStartPos) -1;
        bConfig = TRUE;
        goto cleanUp;
    }
    if((bAllClear == TRUE) && (nStartPos == nMddRxPosHead))
    {
        DEBUGMSG(TESTENABLE,
            (L"(bAllClear == TRUE)&& (nMddRxPosHead = nRxDmaBuffer)\r\n"));
        //check if it is not starting position then inc by 1
        if(nStartPos != nRxDmaBuffer)
            nStartPos =nStartPos;// + 1;
        nBufferSize = pPdd->RxDmaBufferSize - (nStartPos - nRxDmaBuffer);
        bConfig = TRUE;
        goto cleanUp;
    }

cleanUp:
    if(bConfig)
    {
        if ((nBufferSize <= 0) || (nBufferSize > pPdd->RxDmaBufferSize))
            {
                bRet = FALSE;
                return bRet;
            }
        RETAILMSG(0,
            (L"nBufferSize = 0x%x  nStartPos = 0x%x\r\n",
            nBufferSize,
            nStartPos));

        // update registers
        DmaSetDstBuffer(pPdd->RxDmaInfo,
            (UINT8*)nStartPos,
            pPdd->paRxDmaBuffer + (nStartPos - nRxDmaBuffer));

        // Since we have to setup DMA to transfer multiple of frame size (or Rx Fifo's 
        // trigger level) and we have to fill up the DMA buffer before wrap around, we 
        // will setup the DMA to fill few bytes more after the DMA buffer (yes, we did
        // allocate that extra space, but just didn't tell MDD about that) here and we 
        // will copy the extra data to the beginning of the DMA when we update the DMA 
        // pointer next. See beginning of this routine.
        nBufferSize += (pPdd->dwRxFifoTriggerLevel - 1);
        DmaSetElementAndFrameCount(pPdd->RxDmaInfo,
            (UINT16)pPdd->dwRxFifoTriggerLevel, 
            ((UINT16)(nBufferSize / pPdd->dwRxFifoTriggerLevel)));

        pPdd->pRxDMALastWrite = (VOID*)nStartPos;

        bRet = TRUE;
    }
    return bRet;
}



//------------------------------------------------------------------------------
//
//  Function:  HWRxDMAIntr
//
//  This function is called by the upper layer whenever an Rx interrupt occurs.
//  The return code is then checked by the MDD to determine number of char needs to read
//
//
static ULONG
HWRxDMAIntr(
            VOID *pvContext, UCHAR *pRxBuffer, ULONG *pLength
            )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    //    OMAP_UART_REGS *pUartRegs = pPdd->pUartRegs;
    UCHAR lineStat = 0;
    ULONG count = *pLength;
    UCHAR *pCurrentDMAPos;
    ULONG i;
    //  PHW_INDEP_INFO  pSerialHead = (PHW_INDEP_INFO)pPdd->pMdd;

    RETAILMSG(FALSE,(TEXT("HWRxDMAIntr pRxBuffer=0x%x pLength=%d\r\n"),
                pRxBuffer,
                *pLength));

    pCurrentDMAPos = pPdd->pRxDMALastWrite; // DmaGetLastWritePos(pPdd->RxDmaInfo);

    if(pCurrentDMAPos >= ((UCHAR*)pPdd->pRxDmaBuffer + pPdd->RxDmaBufferSize))
    {
        Sleep(0);
        pCurrentDMAPos = DmaGetLastWritePos(pPdd->RxDmaInfo);
    }
    if (pCurrentDMAPos >= pRxBuffer)
    {
        if (pCurrentDMAPos >= ((UCHAR*)pPdd->pRxDmaBuffer + pPdd->RxDmaBufferSize))
        {
            count = 0;
        }
        else
        {
        count = pCurrentDMAPos - pRxBuffer;
    }
    }
    else
    {
        pPdd->bSendSignal = TRUE;
        count = pPdd->RxDmaBufferSize - (pRxBuffer - (UCHAR*)pPdd->pRxDmaBuffer);
    }

    // the mdd will specify maximum space available.  Since we don't account for
    // the last read position at the MDD layer don't go beyond what is in pLength
    *pLength = min(*pLength, count);
    DEBUGMSG(TESTENABLE, (L"+HWRxDMAIntr Count= %d\r\n",count));
    // With DMA we lose synchronization with UART error to an individual byte
    // Could apply disposition to all bytes in buffer?
    if(pPdd->open == TRUE)
    {
        lineStat = ReadLineStat(pPdd);
    }
    // Replace char with parity error
    if ((pPdd->dcb.fErrorChar != '\0') &&
        pPdd->dcb.fParity &&
        ((lineStat & UART_LSR_RX_PE) != 0))
    {
        for (i=0; i < *pLength; i++)
            *(pRxBuffer+i) = pPdd->dcb.ErrorChar;
    }
    else
    {
        for (i=0; i < *pLength; i++)
        {
            // See if we need to generate an EV_RXFLAG
            if (*(pRxBuffer+i) == pPdd->dcb.EvtChar)
            {
                EvaluateEventFlag(pPdd->pMdd, EV_RXFLAG);
                break;
            }
        }

    }

    return count;

}

//------------------------------------------------------------------------------
//
//  Function:  InitializeTxDMA
//
//  This function initiallizes the Tx DMA register.
//
static BOOL InitializeTxDMA(UARTPDD *pPdd)
{
    BOOL rc = TRUE;

    // Tx DMA configuration settings
    DmaConfigure (pPdd->hTxDmaChannel,
        &TxDmaSettings, pPdd->TxDmaRequest, pPdd->TxDmaInfo);

    DmaSetSrcBuffer (pPdd->TxDmaInfo,
        pPdd->pTxDmaBuffer,
        pPdd->paTxDmaBuffer);

    DmaSetDstBuffer(pPdd->TxDmaInfo,
        (UINT8 *)&(pPdd->pUartRegs->THR),
        pPdd->memBase[0] + offset(SOC_UART_REGS, THR));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RestoreUARTContext
//
//  This function restore the UART register.
//
static VOID RestoreUARTContext(UARTPDD *pPdd)
{
    DEBUGMSG(ZONE_FUNCTION, (L"+RestoreUARTContext\r\n"));
    //RETAILMSG(1, (L"+RestoreUARTContext\r\n"));

    // Set up Tx DMA Channel
    if (pPdd->hTxDmaChannel != NULL)
    {
        InitializeTxDMA(pPdd);
    }

    // Set up Rx DMA Channel
    if (pPdd->hRxDmaChannel != NULL)
    {
        PHW_INDEP_INFO  pSerialHead = (PHW_INDEP_INFO)pPdd->pMdd;

        InitializeRxDMA(pPdd);

        // Adjust the DMA Rx buffer pointer to where the MDD currently points to.
        UpdateDMARxPointer(pPdd, TRUE, (UINT32)RxBuffRead(pSerialHead) - (UINT32)(pPdd->pRxDmaBuffer));
    }

    if (pPdd->currentDX != D0) 
    {
        SOC_SET_DEVICE_POWER(pPdd, D0);
    }

    // Initialize the UART registers with default value. 
    InitializeUART(pPdd);

    // Update COMM port setting according DCB. SetDCB checks the flags in pPdd->pDCB
    // before updating the register value. Passing TRUE in the 3rd parameter to force
    // SetDCB to skip the checking.
    SetDCB(pPdd, &pPdd->dcb, TRUE);

    // MCR wasn't part of DCB, so we need to restore it here.
    OUTREG8(&pPdd->pUartRegs->MCR, pPdd->currentMCR);

    /* Clear Modem and Line stats before enable IER */
    INREG8(&pPdd->pUartRegs->MSR);
    INREG8(&pPdd->pUartRegs->LSR);

    // Restore interrupt mask. Since we are restoring to UART to it initialize condition
    // and add the current COMM setting on top it, we will simply initialize the interrupt
    // mask according to if the COMM port is open or closed.
    if (pPdd->open == TRUE)
    {
        // Enable interrupts (no TX interrupt)
        pPdd->intrMask |= UART_IER_LINE|UART_IER_MODEM;
    }
    OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

    if (pPdd->currentDX != D0)
    {
        SOC_SET_DEVICE_POWER(pPdd, pPdd->currentDX);
    }

    // We need to give the Tx function a kick to restart the Tx operation.
    pPdd->addTxIntr = TRUE;
    if ((pPdd->hTxDmaChannel != NULL) && (pPdd->hEventTxIstDma != INVALID_HANDLE_VALUE))
    {
        SetEvent(pPdd->hEventTxIstDma);
    }

    //RETAILMSG(1, (L"-RestoreUARTContext\r\n"));
    DEBUGMSG(ZONE_FUNCTION, (L"-RestoreUARTContext\r\n"));
}



static VOID ResetUART(UARTPDD *pPdd)
{
    DEBUGMSG(ZONE_FUNCTION, (L"+ResetUART\r\n"));

    if (pPdd->currentDX != D0) 
    {
        SOC_SET_DEVICE_POWER(pPdd, D0);
    }

    // Reset UART & wait until it completes
    OUTREG8(&pPdd->pUartRegs->SYSC, UART_SYSC_RST);
    while ((INREG8(&pPdd->pUartRegs->SYSS) & UART_SYSS_RST_DONE) == 0);

    if (pPdd->currentDX != D0)
    {
        SOC_SET_DEVICE_POWER(pPdd, pPdd->currentDX);
    }

    DEBUGMSG(ZONE_FUNCTION, (L"-ResetUART\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  HWInit
//
//  This function initializes a serial device and it returns information about
//  it.
//
static
PVOID
HWInit(
       ULONG context,
       PVOID pMdd,
       PHWOBJ pHWObj
       )
{
    BOOL rc = FALSE;
    UARTPDD *pPdd = NULL;
    PHYSICAL_ADDRESS pa;
    SOC_UART_REGS *pUartRegs;

    DEBUGMSG(ZONE_OPEN||ZONE_FUNCTION, (
        L"+HWInit(%s, 0x%08x, 0x%08x\r\n", context, pMdd, pHWObj
        ));

    // Allocate SER_INFO structure
    pPdd = LocalAlloc(LPTR, sizeof(UARTPDD));
    if (pPdd == NULL) goto cleanUp;

    memset(pPdd, 0x00, sizeof(UARTPDD));

    // Read device parameters
    if (GetDeviceRegistryParams(
        (LPCWSTR)context, pPdd, dimof(s_deviceRegParams), s_deviceRegParams
        ) != ERROR_SUCCESS)

    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
            L"Failed read driver registry parameters\r\n"
            ));
        goto cleanUp;
    }


    rc = CreateParentBus(pPdd, context);
    if (rc == FALSE)
        goto cleanUp;

    rc = FALSE;

	// Retrieve device index
	pPdd->DeviceID = SOCGetUartDeviceByIndex(pPdd->UARTIndex);

	if (pPdd->DeviceID == OMAP_DEVICE_NONE)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
            L"Failed to retrieve valid device ID for this UART\r\n"
            ));
        goto cleanUp;
	}

	// Perform pads configuration
	if (!RequestDevicePads(pPdd->DeviceID))
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
            L"Failed to request pads\r\n"
            ));
        goto cleanUp;
	}

    // Map physical memory
    DEBUGMSG(ZONE_INIT, (L"HWInit: MmMapIoSpace: 0x%08x\n", pPdd->memBase[0]));
    
    pPdd->memBase[0] = GetAddressByDevice(pPdd->DeviceID);
    pPdd->memLen[0] = sizeof(SOC_UART_REGS);

    pPdd->irq = GetIrqByDevice(SOCGetUartDeviceByIndex(pPdd->UARTIndex),NULL);
    // UART registers
    pa.QuadPart = pPdd->memBase[0];
    pUartRegs = MmMapIoSpace(pa, pPdd->memLen[0], FALSE);
    if (pUartRegs == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
            L"Failed map physical memory 0x%08x\r\n", pa.LowPart
            ));
        goto cleanUp;
    }
    pPdd->pUartRegs = pUartRegs;

    if ((pPdd->XcvrEnableGpio != 0xFFFF) && ((pPdd->XcvrEnabledLevel == 0) || (pPdd->XcvrEnabledLevel == 1)))
    {
        DEBUGMSG(ZONE_INIT, (L"HWInit: External transceiver controlled with gpio %d\n", pPdd->XcvrEnableGpio));
        // GPIO used to control external transceiver
        pPdd->hGpio = GPIOOpen();
        if (pPdd->hGpio == INVALID_HANDLE_VALUE)
        {
            pPdd->hGpio = NULL;
     
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Failed to open gpio driver!\r\n"
                ));
            goto cleanUp;
        }
    }
            
        
    pPdd->bHWXmitComCharWaiting = FALSE;

    DEBUGMSG(ZONE_INIT, (L"HWInit: TxDmaRequest= %d\n", pPdd->TxDmaRequest));

    pPdd->TxDmaInfo = NULL;     // init TX DMA as not used

    // Should driver use DMA for TX?
    if (pPdd->TxDmaRequest == -1)
        {
            RETAILMSG(FALSE, (L"TX - DMA Disabled \r\n"));
            goto skipTXDMA;         // use FIFO, not DMA for TX.
        }

    // allocate dma channel
    pPdd->hTxDmaChannel = DmaAllocateChannel(DMA_TYPE_SYSTEM);
    if (pPdd->hTxDmaChannel != NULL)
    {
        DEBUGMSG(ZONE_INIT, (L"TX DMA enabled\r\n"));

        pPdd->TxDmaInfo = LocalAlloc(LMEM_ZEROINIT,sizeof(DmaDataInfo_t));
        if(!pPdd->TxDmaInfo)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Cannot allocate DMA on TX\r\n"
                ));
            goto skipTXDMA;
        }
        if (pPdd->TxDmaBufferSize < MAX_TX_SERIALDMA_FRAMESIZE)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"TxDmaBufferSize must be at least %u bytes\r\n",
                MAX_TX_SERIALDMA_FRAMESIZE
                ));

            LocalFree(pPdd->TxDmaInfo);
            pPdd->TxDmaInfo = NULL;
            goto skipTXDMA;
        }

        // Allocate DMA transfer buffer
        pPdd->pTxDmaBuffer = AllocPhysMem(
            pPdd->TxDmaBufferSize, PAGE_READWRITE|PAGE_NOCACHE, 0, 0, &pPdd->paTxDmaBuffer
            );

        if (pPdd->pTxDmaBuffer == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Failed allocating TX buffer (size %u)\r\n", pPdd->TxDmaBufferSize
                ));
            LocalFree(pPdd->TxDmaInfo);
            pPdd->TxDmaInfo = NULL;
            goto skipTXDMA;
        }
        DEBUGMSG(ZONE_INIT, (L"TX DMA buffer allocated\r\n"));

        pPdd->TxDmaInfo->pSrcBuffer = NULL;
        pPdd->TxDmaInfo->pDstBuffer = NULL;
        pPdd->TxDmaInfo->PhysAddrSrcBuffer = 0;
        pPdd->TxDmaInfo->PhysAddrDstBuffer = 0;
        pPdd->bExitThread = FALSE;
        pPdd->bSendSignal = FALSE;
        pPdd->bRxBreak = FALSE;
        
        //Event for Tx DMA Endofblock
        pPdd->hEventTxIstDma = CreateEvent(NULL, FALSE, FALSE, NULL);

        // register dma for interrupts
        if (DmaEnableInterrupts(pPdd->hTxDmaChannel, pPdd->hEventTxIstDma) == FALSE)
        {
            RETAILMSG(1,(TEXT("ERROR:Failed to enable DMA TX interrupt\r\n")));
            FreePhysMem( pPdd->pTxDmaBuffer );
            pPdd->pTxDmaBuffer = NULL;
            LocalFree(pPdd->TxDmaInfo);
            pPdd->TxDmaInfo = NULL;
            goto skipTXDMA;
        }
        // Tx DMA configuration settings

        InitializeTxDMA(pPdd);

        DEBUGMSG(ZONE_INIT, (L"TX DMA init completed\r\n"));

    }
skipTXDMA:

    pPdd->RxDmaInfo = NULL; // init RX DMA not used
    pPdd->hRxDmaChannel = NULL;

    DEBUGMSG(ZONE_INIT, (L"HWInit: RxDmaRequest= %d\n", pPdd->RxDmaRequest));

    // Should driver use DMA for RX?
    if (pPdd->RxDmaRequest == -1)
        {
            RETAILMSG(FALSE, (L"RX - DMA Disabled\r\n"));
            goto skipRXDMA; // use FIFO, not DMA for RX.
        }

    // allocate dma channel
    pPdd->hRxDmaChannel = DmaAllocateChannel(DMA_TYPE_SYSTEM);
    if (pPdd->hRxDmaChannel != NULL)
    {

        pPdd->RxDmaInfo = LocalAlloc(LMEM_ZEROINIT,sizeof(DmaDataInfo_t));
        if(!pPdd->RxDmaInfo)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Cannot allocate DMA on RX\r\n"
                ));
            goto skipRXDMA;
        }

        if (pPdd->RxDmaBufferSize < MAX_RX_SERIALDMA_FRAMESIZE)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"RxDmaBufferSize must be at least %u bytes\r\n",
                MAX_TX_SERIALDMA_FRAMESIZE
                ));
            LocalFree(pPdd->RxDmaInfo);
            pPdd->RxDmaInfo = NULL;
            goto skipRXDMA;
        }

        if (pPdd->dwRxFifoTriggerLevel > UART_FIFO_SIZE)
        {
            DEBUGMSG(ZONE_INIT, (L"HWInit: "
                L"RxFifoTriggerLevel must be less then FIFO size %d. Reset to default trigger level %d.\r\n",
                UART_FIFO_SIZE, DEFAULT_RX_FIFO_TRIGGER_LEVEL
                ));
            pPdd->dwRxFifoTriggerLevel = DEFAULT_RX_FIFO_TRIGGER_LEVEL;
        }

        // Allocate DMA receive buffer. We need to allocate a few bytes more than we should
        // because the total number of bytes in a DMA transfer must be equal to mutliple of
        // frame size. In our case, the frame size is never greater than the FIFO size, so 
        // we simply allocate extra 64 byte (FIFO size) for the DMA buffer.
        pPdd->pRxDmaBuffer = AllocPhysMem(
            pPdd->RxDmaBufferSize + UART_FIFO_SIZE, PAGE_READWRITE|PAGE_NOCACHE, 0, 0,
            &pPdd->paRxDmaBuffer
            );

        if (pPdd->pRxDmaBuffer == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Failed allocating transfer buffer (size %u)\r\n",
                pPdd->RxDmaBufferSize
                ));
            LocalFree(pPdd->RxDmaInfo);
            pPdd->RxDmaInfo = NULL;
            goto skipRXDMA;
        }
        DEBUGMSG(ZONE_INIT, (L"RX DMA buffer allocated\n"));
        //Rx Event to get End of Block/Frame
        pPdd->hEventRxIstDma = CreateEvent(NULL, FALSE, FALSE, NULL);

        // registering for system interrupt is deferred to the IST thread
        // spawn thread
        pPdd->hRxThread = CreateThread(NULL, 0, IST_RxDMA, pPdd, 0, NULL);
        if (pPdd->hRxThread == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Failed to create DMA RX thread\r\n"
                ));
            goto cleanUp;
        }
        // Rx DMA configuration settings
        InitializeRxDMA(pPdd);
    }
skipRXDMA:
    // Map interrupt
    if (!KernelIoControl(
        IOCTL_HAL_REQUEST_SYSINTR, &(pPdd->irq), sizeof(pPdd->irq), &pPdd->sysIntr,
        sizeof(pPdd->sysIntr), NULL
        )) {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
                L"Failed obtain SYSINTR for IRQ %d\r\n", pPdd->irq
                ));
            goto cleanUp;
    }

    // Save it to HW object
    pHWObj->dwIntID = pPdd->sysIntr;

    // Create sync objects
    InitializeCriticalSection(&pPdd->hwCS);
    InitializeCriticalSection(&pPdd->txCS);
    InitializeCriticalSection(&pPdd->RxUpdatePtrCS);
    InitializeCriticalSection(&pPdd->powerCS);
    
    pPdd->txEvent = CreateEvent(0, FALSE, FALSE, NULL);
    if (pPdd->txEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWInit: "
            L"Failed create event\r\n"
            ));
        goto cleanUp;
    }

    // Device is initially closed
    pPdd->open = FALSE;
    SetDefaultDCB(pPdd);

    // Initialize device power state
    pPdd->externalDX = D0;
    pPdd->currentDX = D4;
    SetPower(pPdd, D0);

    EnterCriticalSection(&pPdd->hwCS);

    pPdd->bRxDMASignaled = 0;
    pPdd->bRxWrapped     = FALSE;
    pPdd->pRxDMALastWrite = 0;
    pPdd->bDmaInitialize = FALSE;

    InitializeUART(pPdd);

    LeaveCriticalSection(&pPdd->hwCS);

    // Set device to D3
    pPdd->externalDX = D3;
    SetPower(pPdd, D3);

    // Save MDD context for callback
    pPdd->pMdd = pMdd;

    // Initialization succeeded
    rc = TRUE;
    DEBUGMSG(ZONE_INIT, (L"HWInit: Initialization succeeded\r\n"));

cleanUp:
    if (!rc && (pPdd != NULL))
    {
        RETAILMSG(1,(TEXT("HWInit Failed!! Calling HWDeinit\r\n")));
        HWDeinit(pPdd);
        pPdd = NULL;
    }
    DEBUGMSG(ZONE_OPEN||ZONE_FUNCTION, (L"-HWInit(pPdd = 0x%08x)\r\n", pPdd));

    return pPdd;
}



//------------------------------------------------------------------------------
//
//  Function:  HWPostInit
//
//  This function is called by the upper layer after hardware independent
//  initialization is done (at end of COM_Init).
//
static
BOOL
HWPostInit(
           VOID *pvContext
           )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    PHW_INDEP_INFO  pSerialHead = (PHW_INDEP_INFO)pPdd->pMdd;

    // if Rx DMA is enabled override the mdd rxbuffer with DMA buffer
    if (pPdd->RxDmaInfo)
    {
        pSerialHead->RxBufferInfo.RxCharBuffer = pPdd->pRxDmaBuffer;
        pSerialHead->RxBufferInfo.Length = pPdd->RxDmaBufferSize;
        pPdd->rxBufferSize = pPdd->RxDmaBufferSize;

        DEBUGMSG(ZONE_OPEN||ZONE_FUNCTION,
            (TEXT("buffer %x %d "),pPdd->pRxDmaBuffer,pPdd->RxDmaBufferSize));

    }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  HWDeinit
//
//  This function is called by the upper layer to de-initialize the hardware
//  when a device driver is unloaded.
//
static
ULONG
HWDeinit(
         VOID *pvContext
         )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    DEBUGMSG(ZONE_CLOSE||ZONE_FUNCTION, (L"+HWDeinit(0x%08x)\r\n", pvContext));

    // stop rx thread
    if (pPdd->hRxThread != NULL)
    {
        pPdd->bExitThread = TRUE;
        SetEvent(pPdd->hRxThread);
        WaitForSingleObject(pPdd->hRxThread, INFINITE);
        CloseHandle(pPdd->hRxThread);
        pPdd->hRxThread = NULL;
    }

    if (pPdd->hEventRxIstDma != NULL)
    {
        CloseHandle(pPdd->hEventRxIstDma);
        pPdd->hEventRxIstDma = NULL;
    }
    // Unmap UART registers
    if (pPdd->pUartRegs != NULL)
    {
        MmUnmapIoSpace((VOID*)pPdd->pUartRegs, pPdd->memLen[0]);
    }

    if (pPdd->hGpio)
    {
        GPIOClose(pPdd->hGpio);
        pPdd->hGpio = NULL;
    }

    // Disconnect the interrupt
    if (pPdd->sysIntr != 0)
    {
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR, &pPdd->sysIntr, sizeof(&pPdd->sysIntr),
            NULL, 0, NULL
            );
    }

    // Disable all clocks
    if (SOC_HAS_PARENT_BUS(pPdd))
    {
        pPdd->externalDX = D3;
        SetPower(pPdd, D3);
        SOC_CLOSE_PARENT_BUS(pPdd);
    }

    // Delete sync objects
    DeleteCriticalSection(&pPdd->hwCS);
    DeleteCriticalSection(&pPdd->txCS);
    DeleteCriticalSection(&pPdd->RxUpdatePtrCS);

    if (pPdd->txEvent != NULL) CloseHandle(pPdd->txEvent);

    if (pPdd->RxDmaInfo)
    {
        DmaStop(pPdd->RxDmaInfo);

        if(!DmaEnableInterrupts(pPdd->hRxDmaChannel, NULL))
        {
            RETAILMSG(1, (L"UART:HWDeinit:DmaEnableInterrupts failed\r\n"));
        }
        LocalFree(pPdd->RxDmaInfo);
    }
    if (pPdd->TxDmaInfo)
    {
        DmaStop(pPdd->TxDmaInfo);
        LocalFree(pPdd->TxDmaInfo);
    }

    // Free any allocated physical memory
    if (pPdd->pTxDmaBuffer)
    {
        FreePhysMem( pPdd->pTxDmaBuffer );
    }
    if (pPdd->pRxDmaBuffer)
    {
        FreePhysMem( pPdd->pRxDmaBuffer );
    }

	// Release pads
	ReleaseDevicePads(pPdd->DeviceID);

    // Free driver object
    LocalFree(pPdd);

    DEBUGMSG(ZONE_CLOSE||ZONE_FUNCTION, (L"-HWDeinit\r\n"));
    return TRUE;
}



//------------------------------------------------------------------------------
static
BOOL
HWOpen(
       VOID *pvContext
       )
{
    BOOL rc = FALSE;
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    //Modem line testing
    DWORD dwStatus = 0;

    DEBUGMSG(ZONE_OPEN||ZONE_FUNCTION, (L"+HWOpen(0x%08x)\r\n", pvContext));
    DEBUGMSG(ZONE_FUNCTION, (L"HWOpen - Membase=0x%08x\r\n", pPdd->memBase[0]));

    if (pPdd->open) goto cleanUp;

    // We have set hardware to D0
    pPdd->externalDX = D0;
    SetPower(pPdd, D0);

    SetDefaultDCB(pPdd);

    pPdd->commErrors    = 0;
    pPdd->overrunCount  = 0;
    pPdd->flowOffCTS    = FALSE;
    pPdd->flowOffDSR    = FALSE;
    pPdd->addTxIntr     = FALSE;
    pPdd->open          = TRUE;
    pPdd->wakeUpMode    = FALSE;
    pPdd->RxTxRefCount  = 0;
    pPdd->bExitPowerThread = FALSE;

    // Event for power thread to check Rx -Tx activity
    pPdd->hPowerEvent = CreateEvent(0,TRUE,FALSE,NULL);

    // spawn power thread
    pPdd->hPowerThread = CreateThread(NULL, 0, PowerThreadProc, pPdd, 0, NULL);

    if (pPdd->hPowerThread == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HWOpen: "
            L"Failed to create Power Thread\r\n"
            ));
        goto cleanUp;
    }

    EnterCriticalSection(&pPdd->hwCS);

    // Set line control register
    SetWordLength(pPdd, pPdd->dcb.ByteSize);
    SetStopBits(pPdd, pPdd->dcb.StopBits);
    SetParity(pPdd, pPdd->dcb.Parity);

    // Set modem control register
    pPdd->currentMCR = 0;
    OUTREG8(&pPdd->pUartRegs->MCR, 0);
    //Clear Tx and Rx FIFO
    OUTREG8(&pPdd->pUartRegs->FCR, pPdd->CurrentFCR | UART_FCR_TX_FIFO_CLEAR | UART_FCR_RX_FIFO_CLEAR);

    // Do we need to read RESUME register in case there was an overrun error in the FIFO?
    INREG8(&pPdd->pUartRegs->RESUME);

    // Reset the RX dma info.
    if(pPdd->RxDmaInfo) {

        // Just to be safe we will make sure DMA is stopped and the status is clear
        DmaStop(pPdd->RxDmaInfo);
        dwStatus = DmaGetStatus(pPdd->RxDmaInfo);
        DmaClearStatus(pPdd->RxDmaInfo, dwStatus);

        //We must do this since the MDD calls RXResetFifo
        DmaSetElementAndFrameCount (pPdd->RxDmaInfo,
            (UINT16)pPdd->dwRxFifoTriggerLevel,
            (UINT16)((pPdd->RxDmaBufferSize + pPdd->dwRxFifoTriggerLevel - 1) / pPdd->dwRxFifoTriggerLevel));
        DmaSetDstBuffer (pPdd->RxDmaInfo,
            pPdd->pRxDmaBuffer,
            pPdd->paRxDmaBuffer);

        pPdd->bRxDMASignaled = 0;
        pPdd->bRxWrapped     = FALSE;
        pPdd->pRxDMALastWrite = pPdd->pRxDmaBuffer;

        UpdateDMARxPointer(pPdd, FALSE, 0);

        SetAutoIdle(pPdd, TRUE);
        DmaStart(pPdd->RxDmaInfo);
        pPdd->bDmaInitialize = TRUE;
    }

    // configure uart port with default settings
    SetBaudRate(pPdd, pPdd->dcb.BaudRate);
    // Enable interrupts (no TX interrupt)
    pPdd->intrMask |= UART_IER_LINE|UART_IER_MODEM;
    OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);
    // Update line & modem status
    ReadModemStat(pPdd);

    LeaveCriticalSection(&pPdd->hwCS);

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_OPEN||ZONE_FUNCTION, (L"-HWOpen(rc = %d)\r\n", rc));

    return rc;
}


//------------------------------------------------------------------------------

static
ULONG
HWClose(
        VOID *pvContext
        )
{
    ULONG rc = (ULONG)-1;
    UARTPDD *pPdd = (UARTPDD*)pvContext;

    DEBUGMSG(ZONE_CLOSE||ZONE_FUNCTION, (L"+HWClose(0x%08x)\r\n", pvContext));

    if (!pPdd->open) goto cleanUp;

    //CETK test case put Device in D4 state. CETK called some API's which is not supported
    //Added check just to Make sure COM_Close is executed in D0 state.
    if(pPdd->externalDX != D0)
    {
        pPdd->externalDX = D0;
        SetPower(pPdd, D0);
    }

    if (pPdd->hwMode)
    {
        if (pPdd->dcb.fRtsControl == RTS_CONTROL_HANDSHAKE) SetAutoRTS(pPdd, FALSE);
        if (pPdd->dcb.fOutxCtsFlow) SetAutoCTS(pPdd, FALSE);
    }

    EnterCriticalSection(&pPdd->hwCS);

    // Disable interrupts
    pPdd->intrMask &= ~(UART_IER_LINE|UART_IER_MODEM);
    OUTREG8(&pPdd->pUartRegs->IER, pPdd->intrMask);

    // Update line & modem status
    ReadModemStat(pPdd);

    // Disable all interrupts and clear modem control register
    // The following line generates a data abort.
    // Unclear why (even if the above code is commented out too)
    //    OUTREG8(&pPdd->pUartRegs->IER, 0);
    //    OUTREG8(&pPdd->pUartRegs->MCR, 0);

    if (pPdd->RxDmaInfo)
    {
        DmaStop(pPdd->RxDmaInfo);
        pPdd->bDmaInitialize = FALSE;
    }

    LeaveCriticalSection(&pPdd->hwCS);

    // We are closed
    pPdd->open = FALSE;
    // stop power thread
    if (pPdd->hPowerThread != NULL)
    {
        pPdd->bExitPowerThread = TRUE;
        SetEvent(pPdd->hPowerEvent);
        WaitForSingleObject(pPdd->hPowerThread, INFINITE);
        CloseHandle(pPdd->hPowerThread);
        pPdd->hPowerThread = NULL;
    }

    if (pPdd->hPowerEvent != NULL)
    {
        CloseHandle(pPdd->hPowerEvent);
    }
    // put uart in force Idle as power thread is exited.
    OUTREG8(
    &pPdd->pUartRegs->SYSC,
    // turn on force idle, to allow full retention
     UART_SYSC_IDLE_FORCE|UART_SYSC_WAKEUP_ENABLE|UART_SYSC_AUTOIDLE
    );

    // Set hardware to D3
    pPdd->externalDX = D3;
    SetPower(pPdd, D3);


    rc = 0;

cleanUp:
    DEBUGMSG(ZONE_CLOSE||ZONE_FUNCTION, (L"-HWClose(%d)\r\n", rc));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function:  HWGetInterruptType
//
//  This function is called by the upper layer whenever an interrupt occurs.
//  The return code is then checked by the MDD to determine which of the four
//  interrupt handling routines are to be called.
//
static
INTERRUPT_TYPE
HWGetInterruptType(
                   VOID *pvContext
                   )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    INTERRUPT_TYPE type = INTR_NONE;
    UCHAR intCause = 0, mask = 0;

    RETAILMSG(FALSE, (L"+HWGetInterruptType(0x%08x)\r\n",pvContext));

    // We must solve special wakeup mode
    if (pPdd->wakeUpMode)
    {
        if (!pPdd->wakeUpSignaled)
        {
            type = INTR_RX;
            pPdd->wakeUpSignaled = TRUE;
        }
        goto cleanUp;
    }
    // if there is no open handle then power is in D3 state
    // avoid exceptions
    if (pPdd->open == FALSE){
        goto cleanUp;
    }

    EnterCriticalSection(&pPdd->hwCS);

    if ((pPdd->currentDX == D3) || (pPdd->currentDX == D4))
    {
       RETAILMSG(FALSE, (L"+HWGetInterruptType D3 (0x%08x)\r\n", pvContext));

       LeaveCriticalSection(&pPdd->hwCS);
    goto cleanUp;
    }

    // Get cause from hardware
    intCause = INREG8(&pPdd->pUartRegs->IIR);
    mask  = INREG8(&pPdd->pUartRegs->IER);
    
    if ((intCause & UART_IIR_IT_PENDING) == 0) {
        switch (intCause & 0x3F)
        {
        case UART_IIR_THR:
            type = INTR_TX;
            break;
        case UART_IIR_RHR:
        type = INTR_RX;
        break;
        case UART_IIR_TO:
            type = INTR_RX;
            break;
        case UART_IIR_MODEM:
        case UART_IIR_CTSRTS:
            type = INTR_MODEM;
            break;
        case UART_IIR_LINE:
            type = INTR_LINE;
        if (IsDmaEnable(pPdd->RxDmaInfo) == FALSE)
        {
            type |= INTR_RX;
        }
            break;
        }

    }

    LeaveCriticalSection(&pPdd->hwCS);

    // Add software TX interrupt to resume send
    if (pPdd->addTxIntr)
    {
        //RETAILMSG(1, (TEXT("Add software TX interrupt to resume send")));
        type |= INTR_TX;
        pPdd->addTxIntr = FALSE;
    }

    if (pPdd->RxDmaInfo == NULL)
    {
        goto cleanUp;
    }

    //RX DMA processing
    if ((intCause & 0x3F) == UART_IIR_TO)
    {
        // UART_RegDump(pPdd);

        EnterCriticalSection(&pPdd->RxUpdatePtrCS);

        SetAutoIdle(pPdd, FALSE);

        pPdd->bRxDMASignaled = FALSE;
        if (RxDmaStop(pPdd, FALSE) == TRUE)
        {
            DEBUGMSG(TESTENABLE, (L"+HWGetInterruptType Starting DMA\r\n"));

            SetAutoIdle(pPdd, TRUE);
            DmaStart(pPdd->RxDmaInfo);
        }
        else
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: HWGetInterruptType: UpdateDMARxPointer failed!!\r\n"));
            }
        LeaveCriticalSection(&pPdd->RxUpdatePtrCS);
        }
        // check if we need to start dma
    else 
        {
        if (!pPdd->RxDmaInfo)
        {
            goto cleanUp;
        }

        if (pPdd->bRxDMASignaled == TRUE)
        {
            pPdd->bRxDMASignaled = FALSE;
            type = INTR_RX;
            goto cleanUp;
        }

        if (pPdd->bRxWrapped == TRUE)
        {
            pPdd->bRxWrapped = FALSE;
            type = INTR_RX;
            goto cleanUp;
        }

        if ((type & INTR_RX) != 0)
        {
            DEBUGMSG(ZONE_FUNCTION, (L"+HWGetInterruptType Starting DMA(INTR_RX)\r\n"));

            EnterCriticalSection(&pPdd->RxUpdatePtrCS);
            if(UpdateDMARxPointer(pPdd, FALSE, 0)== TRUE)
            {
                DEBUGMSG(TESTENABLE, (L"+HWGetInterruptType Starting DMA\r\n"));
                SetAutoIdle(pPdd, TRUE);
                pPdd->bRxDMASignaled = FALSE;
                DmaStart(pPdd->RxDmaInfo);
                type &= ~INTR_RX;
            }
            LeaveCriticalSection(&pPdd->RxUpdatePtrCS);
        }

        if ((type & INTR_LINE) != 0)
        {
            DEBUGMSG(ZONE_FUNCTION, (L"+HWGetInterruptType Starting DMA(INTR_LINE)\r\n"));
            // UNDONE: need to check if DMA has already be started
            // if so then don't start it again
            EnterCriticalSection(&pPdd->RxUpdatePtrCS);
            if(UpdateDMARxPointer(pPdd, FALSE, 0)== TRUE)
            {
                DEBUGMSG(TESTENABLE, (L"+HWGetInterruptType Starting DMA\r\n"));
                SetAutoIdle(pPdd, TRUE);
                pPdd->bRxDMASignaled = FALSE;
                DmaStart(pPdd->RxDmaInfo);

            }
            LeaveCriticalSection(&pPdd->RxUpdatePtrCS);
        }
    }

cleanUp:
    DEBUGMSG(ZONE_THREAD, (
        L"-HWGetInterruptType(type = %d, cause = %02x)\r\n",
        type, intCause
        ));
    return type;
}


////------------------------------------------------------------------------------
////
////  Function:  HWTxIntr
////
////  This function is called from the MDD whenever INTR_TX is returned
////  by HWGetInterruptType which indicate empty place in transmitter FIFO.
////
static
VOID
HWTxIntr(
         VOID *pvContext,
         UCHAR *pTxBuffer,
         ULONG *pLength
         )
{
    UARTPDD *pPdd = (UARTPDD *)pvContext;
    SOC_UART_REGS *pUartRegs = pPdd->pUartRegs;
    DWORD dwDmaStatus;          // DMA status
    DWORD dwStatus;             // Status
    ULONG BytesToTransfer;      // Actual byte to DMA'd
    UCHAR modemStat;            // Modem status
    DWORD dwCount = 0;          // Dma transfer count
    DWORD size = *pLength;      // Actual Buffer Length
    ULONG TotalTimeout;         // The Total Timeout
    ULONG Timeout;              // The Timeout value actually used


    DEBUGMSG(ZONE_THREAD, (
        L"+HWTxIntr(0x%08x, 0x%08x, %d)\r\n", pvContext, pTxBuffer,
        *pLength
        ));

    // Somehow BT driver was trying to send data after setting the UART to D3 and causing
    // UART driver to crash in this routine. Check the power state before doing anything.
    if (pPdd->currentDX >= D3)
    {
        *pLength = 0;
        goto cleanUp;
    }

    SetAutoIdle(pPdd, TRUE);

    EnterCriticalSection(&pPdd->hwCS);
    // If there is nothing to send then disable TX interrupt
    if (*pLength == 0)
    {
        // Disable TX interrupt
        pPdd->intrMask &= ~UART_IER_THR;
        OUTREG8(&pUartRegs->IER, pPdd->intrMask);
        LeaveCriticalSection(&pPdd->hwCS);
        SetAutoIdle(pPdd, FALSE);

        goto cleanUp;
    }


    // Set event to fire HWXmitComChar
    PulseEvent(pPdd->txEvent);

    // If CTS flow control is desired, check it. If deasserted, don't send,
    // but loop.  When CTS is asserted again, the OtherInt routine will
    // detect this and re-enable TX interrupts (causing Flushdone).
    // For finest granularity, we would check this in the loop below,
    // but for speed, I check it here (up to 8 xmit characters before
    // we actually flow off.
    if (pPdd->dcb.fOutxCtsFlow)
    {
        modemStat = ReadModemStat(pPdd);
        if ((modemStat & UART_MSR_NCTS) == 0)
        {
            // Set flag
            pPdd->flowOffCTS = TRUE;
            // Disable TX interrupt
            pPdd->intrMask &= ~UART_IER_THR;
            OUTREG8(&pUartRegs->IER, pPdd->intrMask);
            LeaveCriticalSection(&pPdd->hwCS);
            SetAutoIdle(pPdd, FALSE);
            *pLength = 0;
            goto cleanUp;
        }
    }

    // Same thing applies for DSR
    if (pPdd->dcb.fOutxDsrFlow)
    {
        modemStat = ReadModemStat(pPdd);
        if ((modemStat & UART_MSR_NDSR) == 0)
        {
            // Set flag
            pPdd->flowOffDSR = TRUE;
            // Disable TX interrupt

            pPdd->intrMask &= ~UART_IER_THR;
            OUTREG8(&pUartRegs->IER, pPdd->intrMask);
            LeaveCriticalSection(&pPdd->hwCS);
            SetAutoIdle(pPdd, FALSE);
            *pLength = 0;
            goto cleanUp;
        }
    }

    if ( pPdd->bHWXmitComCharWaiting )
    {
        LeaveCriticalSection(&pPdd->hwCS);

        // Give chance to HWXmitComChar there
        Sleep(pPdd->txPauseTimeMs);

        EnterCriticalSection(&pPdd->hwCS);

        pPdd->bHWXmitComCharWaiting = FALSE;
    }

    if( !pPdd->TxDmaInfo)
    {

//		UART_RegDump(pPdd);

        // 'NO DMA' transmit routine
        // While there are data and there is room in TX FIFO
        dwCount = *pLength;
        *pLength = 0;
        while (dwCount > 0)
        {
            if ((INREG8(&pUartRegs->SSR) & UART_SSR_TX_FIFO_FULL) != 0) break;
            OUTREG8(&pUartRegs->THR, *pTxBuffer++);
            (*pLength)++;
            dwCount--;
        }

        // enable TX interrupt
        pPdd->intrMask |= UART_IER_THR;
        OUTREG8(&pUartRegs->IER, pPdd->intrMask);
        LeaveCriticalSection(&pPdd->hwCS);
        SetAutoIdle(pPdd, FALSE);

        goto cleanUp;
    }

    // clear any pending tx event
    WaitForSingleObject(pPdd->hEventTxIstDma, 0);

    // Make sure an event isn't hanging around from a previous DMA time out.
    // The WaitForSingleObject above handles this...
    //ResetEvent(pPdd->hEventTxIstDma);

    //  Get the length of how much can be DMA'd
    BytesToTransfer = (size < pPdd->TxDmaBufferSize) ? size : pPdd->TxDmaBufferSize;

    //  Write out all the data; loop if necessary
    for( dwCount = 0; dwCount < size; )
    {

        TotalTimeout = ((PHW_INDEP_INFO)pPdd->pMdd)->CommTimeouts.WriteTotalTimeoutMultiplier*BytesToTransfer +
                       ((PHW_INDEP_INFO)pPdd->pMdd)->CommTimeouts.WriteTotalTimeoutConstant;

        if ( !TotalTimeout )
            Timeout = INFINITE;
        else
            Timeout = TotalTimeout;

        dwDmaStatus = DmaGetStatus(pPdd->TxDmaInfo);
        // clear status
        DmaClearStatus(pPdd->TxDmaInfo, dwDmaStatus);

        if (DmaInterruptDone(pPdd->hTxDmaChannel) == FALSE)
        {
            RETAILMSG(1,(TEXT("ERROR: Failed to Signal DMA TX interrupt completion\r\n")));
        }

        DmaSetElementAndFrameCount( pPdd->TxDmaInfo,
            (UINT16)BytesToTransfer,1);

        CeSafeCopyMemory (pPdd->pTxDmaBuffer, pTxBuffer, BytesToTransfer);

        dwDmaStatus = DmaGetStatus(pPdd->TxDmaInfo);

        if(dwDmaStatus) {
            RETAILMSG(1, (TEXT("!!!! DMA status is 0x%x\r\n"), dwDmaStatus));
        }
    
        // Workaround, restart RXDMA to get TX working after break is received
        if (pPdd->bRxBreak)
        {
            if (pPdd->RxDmaInfo)
                DmaStart(pPdd->RxDmaInfo);
            pPdd->bRxBreak = FALSE;
        }
                
        DmaStart(pPdd->TxDmaInfo);
        LeaveCriticalSection(&pPdd->hwCS);

        //The end of transfer is now signalled by the DMA TX IST
        //Adjust to be the correct timeout value
        // need to recalculate every iteration
        dwStatus = WaitForSingleObject(pPdd->hEventTxIstDma, Timeout);

        if(dwStatus == WAIT_FAILED) {
            RETAILMSG(1, (TEXT("TX DMA failed\r\n")));
            DmaStop(pPdd->TxDmaInfo);
        }
        else if(dwStatus == WAIT_TIMEOUT) {
            //EnterCriticalSection(&pPdd->hwCS);
            //UART_RegDump(pPdd);
            //LeaveCriticalSection(&pPdd->hwCS);
            RETAILMSG(1, (TEXT("TX DMA timeout\r\n")));
            DmaStop(pPdd->TxDmaInfo);
        }

        dwDmaStatus = DmaGetStatus(pPdd->TxDmaInfo);
        // clear status
        DmaClearStatus(pPdd->TxDmaInfo, dwDmaStatus);

        if (DmaInterruptDone(pPdd->hTxDmaChannel) == FALSE)
        {
            RETAILMSG(1,(TEXT("ERROR: IST_TxDMA: Failed to Signal DMA TX interrupt completion\r\n")));
            DmaStop(pPdd->TxDmaInfo);
        }

        if(!(dwDmaStatus & DMA_CSR_BLOCK)) {
            RETAILMSG(1,(TEXT("TX DMA Status is 0x%x\r\n"), dwDmaStatus));
            DmaStop(pPdd->TxDmaInfo);
        }

        //  Update amount transferred
        dwCount += BytesToTransfer;

        //calculate remaining data size
        BytesToTransfer = ((size - dwCount)/pPdd->TxDmaBufferSize) ?
                          pPdd->TxDmaBufferSize : (size - dwCount);
    }

    SetEvent(((PHW_INDEP_INFO)pPdd->pMdd)->hTransmitEvent);

    // Report to MDD total bytes transferred
    *pLength = dwCount;
    if(!IsDmaEnable (pPdd->TxDmaInfo)) SetAutoIdle(pPdd, FALSE);

cleanUp:
    DEBUGMSG(ZONE_THREAD, (
        L"-HWTxIntr(*pLength = %d)\r\n", *pLength
        ));
}


//------------------------------------------------------------------------------
//
//  Function:  HWPurgeComm
//
//  This function purges RX and/or TX
//
static
VOID
HWPurgeComm(
            VOID *pvContext,
            DWORD action
            )
{
    UARTPDD *pPdd = (UARTPDD*)pvContext;
    UCHAR mdr1, nDll, nDlh;
    UCHAR fifoCtrl = 0;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HWPurgeComm(0x%08x 0x%08x)\r\n", pvContext, action
        ));
    if ((action & PURGE_TXCLEAR) || (action & PURGE_RXCLEAR))
    {
        EnterCriticalSection(&pPdd->hwCS);

        mdr1 = INREG8(&pPdd->pUartRegs->MDR1);

        // Disable UART
        OUTREG8(&pPdd->pUartRegs->MDR1, UART_MDR1_DISABLE);

        SETREG8(&pPdd->pUartRegs->LCR, UART_LCR_DIV_EN);

        // store baud clock
        nDll = INREG8(&pPdd->pUartRegs->DLL);
        nDlh = INREG8(&pPdd->pUartRegs->DLH);

        // Clear the baud clock
        OUTREG8(&pPdd->pUartRegs->DLL, 0);
        OUTREG8(&pPdd->pUartRegs->DLH, 0);

        // clear FIFOs as requested
        if ((action & PURGE_TXCLEAR) != 0) fifoCtrl |= UART_FCR_TX_FIFO_CLEAR;
        if ((action & PURGE_RXCLEAR) != 0) fifoCtrl |= UART_FCR_RX_FIFO_CLEAR;
        OUTREG8(&pPdd->pUartRegs->FCR, pPdd->CurrentFCR | fifoCtrl);

        // Do we need to read RESUME register in case there was an overrun error in the FIFO?
        //INREG8(&pPdd->pUartRegs->RESUME);

        // set baud clock
        OUTREG8(&pPdd->pUartRegs->DLL, nDll);
        OUTREG8(&pPdd->pUartRegs->DLH, nDlh);

        CLRREG8(&pPdd->pUartRegs->LCR, UART_LCR_DIV_EN);

        // Enable UART
        OUTREG8(&pPdd->pUartRegs->MDR1, mdr1);

        LeaveCriticalSection(&pPdd->hwCS);
    }


    // Reset the RX dma pointers if we are using DMA for RX
    // the mdd pointers are reset by the mdd.
    if(pPdd->RxDmaInfo && (action & PURGE_RXCLEAR))
    {
        DmaStop(pPdd->RxDmaInfo);
        EnterCriticalSection(&pPdd->RxUpdatePtrCS);
        UpdateDMARxPointer(pPdd, TRUE, 0);
        DmaStart(pPdd->RxDmaInfo);
        LeaveCriticalSection(&pPdd->RxUpdatePtrCS);
        DEBUGMSG(ZONE_FUNCTION, (TEXT("PurgeComm: Last Write Pos: 0x%x"),
                DmaGetLastWritePos(pPdd->RxDmaInfo)));
    }
    // purge on TX DMA if Application has not set the timeout value.
    if(pPdd->TxDmaInfo && (action & PURGE_TXCLEAR))
    {
        if(IsDmaEnable(pPdd->TxDmaInfo)) DmaStop(pPdd->TxDmaInfo);
    }
    DEBUGMSG(ZONE_FUNCTION, (L"-HWPurgeComm\r\n"));
}


static VOID UART_RegDump(UARTPDD * pPDD)
{
#ifndef SHIP_BUILD
    DWORD uartID = 0;
#endif

    UCHAR ucRegLCR;
    UCHAR ucRegMDR1;

    if (pPDD)
    {
        RETAILMSG(1, (L"\r\n"));
        RETAILMSG(1, (L"===========================================================================\r\n"));

        if (pPDD->pUartRegs)
        {
            // Preserve original LCR value so we can restore it when we're done.
            ucRegLCR = INREG8(&pPDD->pUartRegs->LCR);

            RETAILMSG(1, (L"UART%d - UART Register Values:\r\n", uartID));
            RETAILMSG(1, (L"UART%d - Original LCR = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->LCR) ));
            RETAILMSG(1, (L"\r\n"));

            // Note: Don't show IIR because reading IIR will clear any outstanding interrupts
            //       Don't show SFLSR because it will advance the status register pointer.
            //       Don't show TCR or TLR because we need to set bits in order to view them.

            // Figure out which mode we are in.
            ucRegMDR1 = INREG8(&pPDD->pUartRegs->MDR1);

            // Display the registers appropriate to our mode

            // Configuration Mode A
            OUTREG8(&pPDD->pUartRegs->LCR, UART_LCR_MODE_CONFIG_A);

            RETAILMSG(1, (L"UART%d Registers - Configuration Mode A:\r\n", uartID));
            RETAILMSG(1, (L"   UART%d DLL    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->DLL)   ));
            RETAILMSG(1, (L"   UART%d DLH    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->DLH)   ));
            RETAILMSG(1, (L"   UART%d LCR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->LCR)   ));
            RETAILMSG(1, (L"   UART%d MCR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->MCR)   ));
            RETAILMSG(1, (L"   UART%d LSR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->LSR)   ));
            RETAILMSG(1, (L"   UART%d MSR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->MSR)   ));
            RETAILMSG(1, (L"   UART%d SPR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->SPR)   ));
            RETAILMSG(1, (L"   UART%d MDR1   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->MDR1)  ));
            RETAILMSG(1, (L"   UART%d MDR2   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->MDR2)  ));
            RETAILMSG(1, (L"   UART%d UASR   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->UASR)  ));
            RETAILMSG(1, (L"   UART%d SCR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->SCR)   ));
            RETAILMSG(1, (L"   UART%d SSR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->SSR)   ));
            RETAILMSG(1, (L"   UART%d MVR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->MVR)   ));
            RETAILMSG(1, (L"   UART%d SYSC   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->SYSC)  ));
            RETAILMSG(1, (L"   UART%d SYSS   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->SYSS)  ));
            RETAILMSG(1, (L"   UART%d WER    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->WER)   ));
            RETAILMSG(1, (L"\r\n"));

            // Configuration Mode B
            OUTREG8(&pPDD->pUartRegs->LCR, UART_LCR_MODE_CONFIG_B);

            RETAILMSG(1, (L"UART%d Registers - Configuration Mode B:\r\n", uartID));
            RETAILMSG(1, (L"   UART%d EFR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->EFR)   ));
            RETAILMSG(1, (L"   UART%d LCR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->LCR)   ));
            RETAILMSG(1, (L"   UART%d XON1   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->XON1_ADDR1)));
            RETAILMSG(1, (L"   UART%d XON2   = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->XON2_ADDR2)));
            RETAILMSG(1, (L"   UART%d XOFF1  = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->XOFF1) ));
            RETAILMSG(1, (L"   UART%d XOFF2  = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->XOFF2) ));
            RETAILMSG(1, (L"\r\n"));

            // Operational Mode
            OUTREG8(&pPDD->pUartRegs->LCR, UART_LCR_MODE_OPERATIONAL);

            RETAILMSG(1, (L"UART%d Registers - Operational Mode:\r\n", uartID));
            RETAILMSG(1, (L"   UART%d IER    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->IER)   ));
            RETAILMSG(1, (L"   UART%d IIR    = 0x%02X\r\n", uartID, pPDD->pUartRegs->FCR));
            RETAILMSG(1, (L"   UART%d FCR    = 0x%02X\r\n", uartID, pPDD->CurrentFCR));            
            RETAILMSG(1, (L"   UART%d LCR    = 0x%02X\r\n", uartID, INREG8(&pPDD->pUartRegs->LCR)   ));
            RETAILMSG(1, (L"\r\n"));

            // Restore the original value of LCR
            OUTREG8(&pPDD->pUartRegs->LCR, ucRegLCR);

            SOC_DUMP_DMA_REGS(pPDD, uartID);
        }
        RETAILMSG(1, (L"===========================================================================\r\n"));
        RETAILMSG(1, (L"\r\n"));
    }
}

