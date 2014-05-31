// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2009.  All rights reserved.
//
// Test modes for USB.org Embedded Hi-Speed Host Electrical Test
// Procedure, Revision 1.01.
//

#ifdef ENABLE_TESTMODE_SUPPORT

#pragma warning(push)
#pragma warning(disable: 4510 4512 4610)
#include "cohcd.hpp"
#include "drvcommon.h"
#include "testmode.h"
#pragma warning(pop)

static CSL_UsbRegs * g_pUsbRegs = NULL;

static const UCHAR g_TstPacket[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0x7E
};


void InitUsbTestMode(CSL_UsbRegs * pUsbRegs)
{
    g_pUsbRegs = pUsbRegs;
}

void SetUsbTestMode(UCHAR mode)
{
    if (mode == 0)
    {
        WRITE_PORT_UCHAR(g_pUsbRegs+USB_TESTMODE_REG_OFFSET, 0);
    }
    else if (mode == 4)
    {
        // Load 53 byte test packet into EP0 FIFO
        for (int i = 0; i < sizeof(g_TstPacket); ++i)
            WRITE_PORT_UCHAR(g_pUsbRegs+MGC_FIFO_OFFSET(0), g_TstPacket[i]);

        WRITE_PORT_UCHAR(g_pUsbRegs+USB_TESTMODE_REG_OFFSET, (1 << (mode-1)));

        WRITE_PORT_USHORT(g_pUsbRegs+MGC_END_OFFSET(0, MGC_O_HDRC_CSR0), MGC_M_CSR0_TXPKTRDY);
    }
    else
    {
        WRITE_PORT_UCHAR(g_pUsbRegs+USB_TESTMODE_REG_OFFSET, (1 << (mode-1)));
    }
}

void SetUsbTestModeSuspend(void)
{
    WRITE_PORT_UCHAR(g_pUsbRegs+USB_POWER_REG_OFFSET, BIT1|BIT5);
}

void SetUsbTestModeResume(void)
{
    WRITE_PORT_UCHAR(g_pUsbRegs+USB_POWER_REG_OFFSET, BIT2|BIT5);
    Sleep(20);
    WRITE_PORT_UCHAR(g_pUsbRegs+USB_POWER_REG_OFFSET, BIT0|BIT5);
}

#endif // ENABLE_TESTMODE_SUPPORT
