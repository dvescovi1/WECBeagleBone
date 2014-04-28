//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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
//  File:  debug.c
//
#include <bsp.h>
#include <am33x.h>
#include <am33x_clocks.h>

//#include <oal_prcm.h>

extern BOOL EnableDeviceClocks( UINT devId, BOOL bEnable);

static AM33X_UART_REGS  *s_pUartRegs = NULL; 
static BOOL bEnableDebugMessages = TRUE;
static BOOL bInitializationDone = FALSE;

VOID OEMInitDebugSerial( )
//  Initialize debug serial port.
{
	UINT8 tmp;

//	EnableDeviceClocks(BSPGetDebugUARTConfig()->dev, TRUE);

    //  Initialize UART0
    if (BSPGetDebugUARTConfig()->dev == AM_DEVICE_UART0){
    	s_pUartRegs = OALPAtoUA(AM33X_UART0_REGS_PA);
    } else {
    	for(;;); // spin forever
    }

    // Reset UART & wait until it completes
    tmp = INREG8(&s_pUartRegs->SYSC);
    tmp |= UART_SYSC_RST;
    OUTREG8(&s_pUartRegs->SYSC, tmp);
    while ((INREG8(&s_pUartRegs->SYSS) & UART_SYSS_RST_DONE) == 0);

	/* Disable smart idle */
    tmp = INREG8(&s_pUartRegs->SYSC);
	tmp |= (1<<3);
    OUTREG8(&s_pUartRegs->SYSC, tmp);

    // Set baud rate
    OUTREG8(&s_pUartRegs->LCR, UART_LCR_DLAB);
    OUTREG8(&s_pUartRegs->DLL, BSPGetDebugUARTConfig()->DLL /* 26 = 48000000 / 16/ 115200 */ );
    OUTREG8(&s_pUartRegs->DLH, BSPGetDebugUARTConfig()->DLH);
    // 8 bit, 1 stop bit, no parity
    OUTREG8(&s_pUartRegs->LCR, 0x3);
    // Enable FIFO
    OUTREG8(&s_pUartRegs->FCR, UART_FCR_FIFO_EN);
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

VOID OEMDeinitDebugSerial()
//  Function:  OEMDeinitDebugSerial
{
    // Wait while FIFO isn't empty
    if (s_pUartRegs != NULL){
        while ((INREG8(&s_pUartRegs->LSR) & UART_LSR_TX_SR_E) == 0);
    }
    s_pUartRegs = NULL;
    EnableDeviceClocks(AM_DEVICE_UART0, FALSE);
	bInitializationDone = FALSE;
}

VOID OEMWriteDebugByte(UINT8 ch)
//  Write byte to debug serial port.
{
	if (!bInitializationDone) return;

	if (s_pUartRegs != NULL && bEnableDebugMessages){
		// Wait while FIFO is full
		while ((INREG8(&s_pUartRegs->SSR) & UART_SSR_TX_FIFO_FULL) != 0);
		// Send byte
		OUTREG8(&s_pUartRegs->THR, ch);
	}        
}

INT OEMReadDebugByte()
//  Input character/byte from debug serial port
{
    UINT8 ch = (UINT8)OEM_DEBUG_READ_NODATA;
    UINT8 status;

	if (!bInitializationDone)
		return ch;

    if ((s_pUartRegs == NULL) || !bEnableDebugMessages)
        return OEM_DEBUG_READ_NODATA;

    status = INREG8(&s_pUartRegs->LSR);
    if ((status & UART_LSR_RX_FIFO_E) == 0)
        return OEM_DEBUG_READ_NODATA;

    ch = INREG8(&s_pUartRegs->RHR);
    if ((status & UART_LSR_RX_ERROR) != 0)
        return OEM_DEBUG_COM_ERROR;

	return (INT)ch;
}

void OEMEnableDebugOutput(BOOL bEnable)
{
    bEnableDebugMessages = bEnable;
}

VOID OEMWriteDebugString(UINT16 * string)
{
	while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
}
