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
//  File:  am3xx_mcspi_regs.h
//
//  This header file is comprised of register details of Mutli Channel Serial
//  Port Interface module
//

#ifndef __AM3XX_MCSPI_REGS_H
#define __AM3XX_MCSPI_REGS_H

//------------------------------------------------------------------------------
typedef volatile struct {
	unsigned long MCSPI_CHCONF; 	   //offset 0x12C + (ch * N)
	unsigned long MCSPI_CHSTATUS;	   //offset 0x130 + (ch * N)
	unsigned long MCSPI_CHCTRL; 	   //offset 0x134 + (ch * N)
	unsigned long MCSPI_TX; 		   //offset 0x138 + (ch * N)
	unsigned long MCSPI_RX; 		   //offset 0x13C + (ch * N)
} SPI_CH_REGS;

typedef volatile struct
{
	unsigned long RESERVED_1[68];
	unsigned long MCSPI_SYSCONFIG;         //offset 0x110
	unsigned long MCSPI_SYSSTATUS;         //offset 0x114
	unsigned long MCSPI_IRQSTATUS;         //offset 0x118
	unsigned long MCSPI_IRQENABLE;         //offset 0x11C
	unsigned long RESERVED_2;
	unsigned long MCSPI_SYST;              //offset 0x124, system test
	unsigned long MCSPI_MODULCTRL;         //offset 0x128, SPI config
	SPI_CH_REGS   ch[4];
	unsigned long MCSPI_XFERLEVEL;         //offset 0x17C
}
AM3XX_MCSPI_REGS;

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


#endif //__AM3XX_MCSPI_REGS_H
