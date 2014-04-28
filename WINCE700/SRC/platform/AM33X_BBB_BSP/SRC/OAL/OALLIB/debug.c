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
//#include <bus.h>
#include <am33x.h>
//#include <oal_prcm.h>

//------------------------------------------------------------------------------
//  Static variables

// Debug UART registers
static AM33X_UART_REGS  *s_pUartRegs = NULL; 
static BOOL bEnableDebugMessages = TRUE;

//------------------------------------------------------------------------------
//
//  Function:  OEMInitDebugSerial
//
//  Initialize debug serial port.
//
VOID OEMInitDebugSerial( )
{
	UINT32 tmp;
#if 1 // there is a problem to use PRCM API here. Probably because it is not initialiazed yet
	volatile UINT32*  clock_ctrl = (UINT32*)OALPAtoUA(0x44E00000 + 0x04B4); // CM_WKUP_UART0_CLKCTRL

	*clock_ctrl = 0x2;
	while(*clock_ctrl & 0x30000);
#else
	PrcmDeviceEnableClocks(AM_DEVICE_UART0, TRUE);
#endif


    //----------------------------------------------------------------------
    //  Initialize UART0
    //----------------------------------------------------------------------


    s_pUartRegs = OALPAtoUA(AM33X_UART0_REGS_PA);

    // Reset UART & wait until it completes
    OUTREG8(&s_pUartRegs->SYSC, UART_SYSC_RST);
    while ((INREG8(&s_pUartRegs->SYSS) & UART_SYSS_RST_DONE) == 0);
    // Set baud rate
    OUTREG8(&s_pUartRegs->LCR, UART_LCR_DLAB);
    OUTREG8(&s_pUartRegs->DLL, BSP_UART_DSIUDLL /* 48000000 / 16/ 115200 */ );
    OUTREG8(&s_pUartRegs->DLH, BSP_UART_DSIUDLH);
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

    PrcmDeviceEnableClocks(AM_DEVICE_UART0, FALSE);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugByte
//
//  Write byte to debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{

  if (s_pUartRegs != NULL && bEnableDebugMessages)
        {
        // Wait while FIFO is full
        while ((INREG8(&s_pUartRegs->SSR) & UART_SSR_TX_FIFO_FULL) != 0);
        // Send byte
        OUTREG8(&s_pUartRegs->THR, ch);
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

    if (s_pUartRegs != NULL && bEnableDebugMessages)
        {    
        status = INREG8(&s_pUartRegs->LSR);
        if ((status & UART_LSR_RX_FIFO_E) != 0)
            {
            ch = INREG8(&s_pUartRegs->RHR);
            if ((status & UART_LSR_RX_ERROR) != 0)
                {
                ch = (UINT8)OEM_DEBUG_COM_ERROR;
                }                            
            }
        }        
 return (INT)ch;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMEnableDebugOutput
//
//  Controls debug messages, used to temporarily stop messages during 
//  power management setup.
//
//  Called only from PLATFORM\COMMON\SRC\SOC\OMAP35XX_TPS659XX_TI_V1\omap35xx\OAL\PRCM\prcm_device.c
//
void OEMEnableDebugOutput(BOOL bEnable)
{
    bEnableDebugMessages = bEnable;
}

//------------------------------------------------------------------------------

#ifndef SHIP_BUILD
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
}

#endif // SHIP_BUILD
