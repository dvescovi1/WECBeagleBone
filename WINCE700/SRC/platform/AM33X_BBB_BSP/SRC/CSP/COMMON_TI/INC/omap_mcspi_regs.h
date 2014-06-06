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
//  File:  omap_mcspi_regs.h
//
//  This header file is comprised of register details of Mutli Channel Serial
//  Port Interface module
//

#ifndef __OMAP_MCSPI_REGS_H
#define __OMAP_MCSPI_REGS_H

//------------------------------------------------------------------------------


typedef volatile struct
{
   unsigned long MCSPI_CHCONF;            //channel config
   unsigned long MCSPI_CHSTATUS;          //channel status
   unsigned long MCSPI_CHCTRL;            //channel control
   unsigned long MCSPI_TX;                //Transmit register
   unsigned long MCSPI_RX;                //Receive register
}
OMAP_MCSPI_CHANNEL_REGS;


typedef volatile struct
{
   unsigned long MCSPI_REVISION;          //offset 0x00, Revision
   unsigned long RESERVED_1[3];
   unsigned long MCSPI_SYSCONFIG;         //offset 0x10, system config
   unsigned long MCSPI_SYSSTATUS;         //offset 0x14, system status
   unsigned long MCSPI_IRQSTATUS;         //offset 0x18, IRQ status
   unsigned long MCSPI_IRQENABLE;         //offset 0x1C, IRQ enable
   unsigned long MCSPI_WAKEUPENABLE;      //offset 0x20, WKUP enable
   unsigned long MCSPI_SYST;              //offset 0x24, system test
   unsigned long MCSPI_MODULCTRL;         //offset 0x28, SPI config

   unsigned long MCSPI_CHCONF0;           //offset 0x2C, channel 0 config
   unsigned long MCSPI_CHSTATUS0;         //offset 0x30, channel 0 status
   unsigned long MCSPI_CHCTRL0;           //offset 0x34, channel 0 control
   unsigned long MCSPI_TX0;               //offset 0x38, Transmit0 register
   unsigned long MCSPI_RX0;               //offset 0x3C, Receive0 register

   unsigned long MCSPI_CHCONF1;           //offset 0x40, channel 1 config
   unsigned long MCSPI_CHSTATUS1;         //offset 0x44, channel 1 status
   unsigned long MCSPI_CHCTRL1;           //offset 0x48, channel 1 control
   unsigned long MCSPI_TX1;               //offset 0x4C, Transmit1 register
   unsigned long MCSPI_RX1;               //offset 0x50, Receive 1 register

   unsigned long MCSPI_CHCONF2;           //offset 0x54, channel 2 config
   unsigned long MCSPI_CHSTATUS2;         //offset 0x58, channel 2 status
   unsigned long MCSPI_CHCTRL2;           //offset 0x5C, channel 2 control
   unsigned long MCSPI_TX2;               //offset 0x60, Transmit2 register
   unsigned long MCSPI_RX2;               //offset 0x64, Receive2 register

   unsigned long MCSPI_CHCONF3;           //offset 0x68, channel 3 config
   unsigned long MCSPI_CHSTATUS3;         //offset 0x6C, channel 3 status
   unsigned long MCSPI_CHCTRL3;           //offset 0x70, channel 3 control
   unsigned long MCSPI_TX3;               //offset 0x74, Transmit3 register
   unsigned long MCSPI_RX3;               //offset 0x78, Receive3 register
}
OMAP_MCSPI_REGS;


/* Max channels for SPI */
#define MCSPI_MAX_CHANNELS                      4

/* SYSSTATUS Register Bits */
#define MCSPI_SYSSTATUS_RESETDONE               (1<<0)

/* SYSCONFIG Register Bits */
#define MCSPI_SYSCONFIG_AUTOIDLE                (1<<0)
#define MCSPI_SYSCONFIG_SOFTRESET               (1<<1)
#define MCSPI_SYSCONFIG_ENAWAKEUP               (1<<2)
#define MCSPI_SYSCONFIG_FORCEIDLE               (0<<3)
#define MCSPI_SYSCONFIG_DISABLEIDLE             (1<<3)
#define MCSPI_SYSCONFIG_SMARTIDLE               (2<<3)
#define MCSPI_SYSCONFIG_CLOCKACTIVITY(clk)      ((clk&0x03)<<8)

/* SYSMODULCTRL Register Bits */
#define MCSPI_SINGLE_BIT                        (1<<0)
#define MCSPI_MS_BIT                            (1<<2)
#define MCSPI_SYSTEMTEST_BIT                    (1<<3)


/* CHCONF Register Bit values */
#define MCSPI_PHA_ODD_EDGES                     (0 << 0)
#define MCSPI_PHA_EVEN_EDGES                    (1 << 0)

#define MCSPI_POL_ACTIVEHIGH                    (0 << 1)
#define MCSPI_POL_ACTIVELOW                     (1 << 1)

#define MCSPI_CHCONF_CLKD(x)                    ((x & 0x0F) << 2)

#define MCSPI_CSPOLARITY_ACTIVEHIGH             (0 << 6)
#define MCSPI_CSPOLARITY_ACTIVELOW              (1 << 6)

#define MCSPI_CHCONF_WL(x)                      (((x-1) & 0x1F) << 7)
#define MCSPI_CHCONF_GET_WL(x)                  (((x >> 7) & 0x1F) +1)

#define MCSPI_CHCONF_TRM_TXRX                   (0 << 12)
#define MCSPI_CHCONF_TRM_RXONLY                 (1 << 12)
#define MCSPI_CHCONF_TRM_TXONLY                 (2 << 12)

#define MCSPI_CHCONF_DMAW_ENABLE                (1 << 14)
#define MCSPI_CHCONF_DMAW_DISABLE               (0 << 14)

#define MCSPI_CHCONF_DMAR_ENABLE                (1 << 15)
#define MCSPI_CHCONF_DMAR_DISABLE               (0 << 15)

#define MCSPI_CHCONF_DPE0                       (1 << 16)
#define MCSPI_CHCONF_DPE1                       (1 << 17)
#define MCSPI_CHCONF_IS                         (1 << 18)
#define MCSPI_CHCONF_TURBO                      (1 << 19)
#define MCSPI_CHCONF_FORCE                      (1 << 20)

#define MCSPI_CHCONF_SPIENSLV(CSn)              ((CSn & 0x03) << 21)
#define MCSPI_CHCONF_TCS(x)                     ((x & 0x03) << 25)

/* CHSTAT Register Bit values */
#define MCSPI_CHSTAT_RX_FULL                    (1 << 0)
#define MCSPI_CHSTAT_TX_EMPTY                   (1 << 1)
#define MCSPI_CHSTAT_EOT                        (1 << 2)


/* CHCONT Register Bit values */
#define MCSPI_CHCONT_EN                         (1 << 0)


#endif //__OMAP_MCSPI_REGS_H
