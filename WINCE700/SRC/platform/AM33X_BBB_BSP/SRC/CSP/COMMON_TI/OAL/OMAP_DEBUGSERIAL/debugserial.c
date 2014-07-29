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
//  File:  debugserial.c
//
#include "omap.h"
#include "omap_uart_regs.h"
#include "oal_clock.h"
#include "bsp_cfg.h"
#include <nkintr.h>

#ifndef SHIP_BUILD

//------------------------------------------------------------------------------
//  Static variables

// Debug UART registers
static OMAP_UART_REGS  *s_pUartRegs = NULL; 
static const DEBUG_UART_CFG* g_pCfg = NULL;
static BOOL bEnableDebugMessages = TRUE;
static BOOL bInitializationDone = FALSE;

//------------------------------------------------------------------------------
//
//  Function:  OEMInitDebugSerial
//
//  Initialize debug serial port.
//
VOID
OEMInitDebugSerial(
    )
{    
    g_pCfg = BSPGetDebugUARTConfig();
    //----------------------------------------------------------------------
    //  Initialize UART
    //----------------------------------------------------------------------

    s_pUartRegs = OALPAtoUA(GetAddressByDevice(g_pCfg->dev));

    // Reset UART & wait until it completes
    OUTREG8(&s_pUartRegs->SYSC, UART_SYSC_RST);
    while ((INREG8(&s_pUartRegs->SYSS) & UART_SYSS_RST_DONE) == 0);
    // Set baud rate
    OUTREG8(&s_pUartRegs->LCR, UART_LCR_DLAB);
    OUTREG8(&s_pUartRegs->DLL, g_pCfg->DLL);
    OUTREG8(&s_pUartRegs->DLH, g_pCfg->DLH);
    // 8 bit, 1 stop bit, no parity
    OUTREG8(&s_pUartRegs->LCR, g_pCfg->LCR);
    // Enable FIFO
    OUTREG8(&s_pUartRegs->FCR, UART_FCR_FIFO_EN|UART_FCR_RX_FIFO_CLEAR|UART_FCR_TX_FIFO_CLEAR);
    // Pool
    OUTREG8(&s_pUartRegs->IER, 0);
    // Set DTR/RTS signals
    OUTREG8(&s_pUartRegs->MCR, UART_MCR_DTR|UART_MCR_RTS);
    // Configuration complete so select UART 16x mode
    OUTREG8(&s_pUartRegs->MDR1, UART_MDR1_UART16);

    bInitializationDone = TRUE;
}

BOOL OEMDebugInit()
{
    OEMInitDebugSerial();
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMDeinitDebugSerial
//
VOID OEMDeinitDebugSerial()
{
    // Wait while FIFO isn't empty
    if (s_pUartRegs != NULL)
        {
        while ((INREG8(&s_pUartRegs->LSR) & UART_LSR_TX_SR_E) == 0);
        }

    s_pUartRegs = NULL;
    EnableDeviceClocks(g_pCfg->dev, FALSE);

    bInitializationDone = FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugByte
//
//  Write byte to debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{
    if (bInitializationDone)
    {
      if (s_pUartRegs != NULL && bEnableDebugMessages)
            {
            // Wait while FIFO is full
            while ((INREG8(&s_pUartRegs->SSR) & UART_SSR_TX_FIFO_FULL) != 0);
            // Send byte
            OUTREG8(&s_pUartRegs->THR, ch);
            }
    }
}

//------------------------------------------------------------------------------
//
//  Function:  OEMReadDebugByte
//
//  Input character/byte from debug serial port
//
INT OEMReadDebugByte()
{
    UINT8 ch = (UINT8)OEM_DEBUG_READ_NODATA;
    UINT8 status;

    if (bInitializationDone)
    {
        if ((s_pUartRegs == NULL) || !bEnableDebugMessages) return OEM_DEBUG_READ_NODATA;

        status = INREG8(&s_pUartRegs->LSR);
	    if ((status & UART_LSR_RX_FIFO_E) == 0) return OEM_DEBUG_READ_NODATA;

        ch = INREG8(&s_pUartRegs->RHR);
	    if ((status & UART_LSR_RX_ERROR) != 0)  return OEM_DEBUG_COM_ERROR;
    }

	return (INT)ch;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMEnableDebugOutput
//
//  Controls debug messages, used to temporarily stop messages during 
//  power management setup. It must not try to disable the clock as it may be called before
//  the clock module is intialized.
//
//
void OEMEnableDebugOutput(BOOL bEnable)
{
    bEnableDebugMessages = bEnable;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
VOID OEMWriteDebugString(UINT16 *string)
{
    while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugLED
//
//  This function is called via OALLED macro to display debug information
//  on debug LEDs.
//
//  Mapping to 16 character LCD
//
//      15:         Idle (1 = in idle, 0 = not in idle)
//      14-4:       Unused
//      3-0:        Timer tick in seconds
//
VOID OEMWriteDebugLED(WORD index, DWORD data)
{
    UNREFERENCED_PARAMETER(index);
    UNREFERENCED_PARAMETER(data);
}

#else // SHIP_BUILD

//------------------------------------------------------------------------------
//
//  Function:  OEMInitDebugSerial
//
//  Initialize debug serial port.
//
VOID
OEMInitDebugSerial(
    )
{
}

BOOL OEMDebugInit()
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMDeinitDebugSerial
//
VOID OEMDeinitDebugSerial()
{
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugByte
//
//  Write byte to debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{
	UNREFERENCED_PARAMETER(ch);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
VOID OEMWriteDebugString(UINT16 *string)
{
	UNREFERENCED_PARAMETER(string);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMReadDebugByte
//
//  Input character/byte from debug serial port
//
INT OEMReadDebugByte()
{
    return OEM_DEBUG_READ_NODATA;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMEnableDebugOutput
//
//  Controls debug messages, used to temporarily stop messages during 
//  power management setup.
//
void OEMEnableDebugOutput(BOOL bEnable)
{
	UNREFERENCED_PARAMETER(bEnable);
}


#endif // SHIP_BUILD
