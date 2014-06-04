// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  McBSP register mapping and definitions.
//
#ifndef __OMAP35XX_MCBSP_H
#define __OMAP35XX_MCBSP_H

//------------------------------------------------------------------------------
//  memory layout of McBSP hardware registers
//
typedef volatile struct {
    UINT32 DRR;                 //0x00   (r)
    UINT32 RESV_1;
    UINT32 DXR;                 //0x08  (r/w)
    UINT32 RESV_2;
    UINT32 SPCR2;               //0x10  (r/w)
    UINT32 SPCR1;               //0x14  (r/w)
    UINT32 RCR2;                //0x18  (r/w)
    UINT32 RCR1;                //0x1C  (r/w)
    UINT32 XCR2;                //0x20  (r/w)
    UINT32 XCR1;                //0x24  (r/w)
    UINT32 SRGR2;               //0x28  (r/w)
    UINT32 SRGR1;               //0x2C  (r/w)
    UINT32 MCR2;                //0x30  (r/w)
    UINT32 MCR1;                //0x34  (r/w)
    UINT32 RCERA;               //0x38  (r/w)
    UINT32 RCERB;               //0x3C  (r/w)
    UINT32 XCERA;               //0x40  (r/w)
    UINT32 XCERB;               //0x44  (r/w)
    UINT32 PCR;                 //0x48  (r/w)
    UINT32 RCERC;               //0x4C  (r/w)
    UINT32 RCERD;               //0x50  (r/w)
    UINT32 XCERC;               //0x54  (r/w)
    UINT32 XCERD;               //0x58  (r/w)
    UINT32 RCERE;               //0x5C  (r/w)
    UINT32 RCERF;               //0x60  (r/w)
    UINT32 XCERE;               //0x64  (r/w)
    UINT32 XCERF;               //0x68  (r/w)
    UINT32 RCERG;               //0x6C  (r/w)
    UINT32 RCERH;               //0x70  (r/w)
    UINT32 XCERG;               //0x74  (r/w)
    UINT32 XCERH;               //0x78  (r/w)
    UINT32 REV;                 //0x7C   (r)
    UINT32 RINTCLR;             //0x80  (r/w)
    UINT32 XINTCLR;             //0x84  (r/w)
    UINT32 ROVFLCLR;            //0x88  (r/w)
    UINT32 SYSCONFIG;           //0x8C  (r/w)
    UINT32 THRSH2;              //0x90  (r/w)
    UINT32 THRSH1;              //0x94  (r/w)
    UINT32 RESV_3;
    UINT32 RESV_4;
    UINT32 IRQSTATUS;           //0xA0  (r/w)
    UINT32 IRQENABLE;           //0xA4  (r/w)
    UINT32 WAKEUPEN;            //0xA8  (r/w)
    UINT32 XCCR;                //0xAC  (r/w)
    UINT32 RCCR;                //0xB0  (r/w)
    UINT32 XBUFFSTAT;           //0xB4   (r)
    UINT32 RBUFFSTAT;           //0xB8   (r)
    UINT32 SSELCR;              //0xBC   (r/w)
    UINT32 STATUS;              //0xC0   (r)
} OMAP35XX_MCBSP_REGS_t;

//------------------------------------------------------------------------------
//  memory layout of McBSP Sidetone hardware registers
//
typedef volatile struct {
    UINT32 REV;                 //0x00   (r)
    UINT32 RESV_1;
    UINT32 RESV_2;
    UINT32 RESV_3;
    UINT32 SYSCONFIG;           //0x10   (r/w)
    UINT32 RESV_4;
    UINT32 IRQSTATUS;           //0x18   (r/w)
    UINT32 IRQENABLE;           //0x1C   (r/w)
    UINT32 RESV_5;
    UINT32 SGAINCR;             //0x24   (r/w)
    UINT32 SFIRCR;              //0x28   (r/w)
    UINT32 SSELCR;              //0x2C   (r/w)
} OMAP35XX_MCBSP_REGS_ST_t;


// Serial Port Control Register 2
//
#define MCBSP_SPCR2_FREE                    (1 << 9)
#define MCBSP_SPCR2_SOFT                    (1 << 8)
#define MCBSP_SPCR2_FRST_RSTCLR             (1 << 7)
#define MCBSP_SPCR2_GRST_RSTCLR             (1 << 6)
#define MCBSP_SPCR2_XINTM_XSYNCERR          (3 << 4)
#define MCBSP_SPCR2_XINTM_MASK              (3 << 4)
#define MCBSP_SPCR2_XSYNCERR                (1 << 3)
#define MCBSP_SPCR2_XEMPTY                  (1 << 2)    // (r)
#define MCBSP_SPCR2_XRDY                    (1 << 1)    // (r)
#define MCBSP_SPCR2_XRST_RSTCLR             (1 << 0)

// Serial Port Control Register 1
//
#define MCBSP_SPCR1_ALB_ENABLE              (1 << 15)
#define MCBSP_SPCR1_RJUST_RJ_ZEROFILL       (0 << 13)
#define MCBSP_SPCR1_RJUST_RJ_SIGNFILL       (1 << 13)
#define MCBSP_SPCR1_RJUST_LJ_ZEROFILL       (2 << 13)
#define MCBSP_SPCR1_RJUST_MASK              (3 << 13)
#define MCBSP_SPCR1_DXENA                   (1 << 7)
#define MCBSP_SPCR1_RINTM_RSYNCERR          (3 << 4)
#define MCBSP_SPCR1_RINTM_MASK              (3 << 4)
#define MCBSP_SPCR1_RSYNCERR                (1 << 3)
#define MCBSP_SPCR1_RFULL                   (1 << 2)    // (r)
#define MCBSP_SPCR1_RRDY                    (1 << 1)    // (r)
#define MCBSP_SPCR1_RRST_RSTCLR             (1 << 0)


// These values applies to
//  RCR2 - Receive Control Register 2
//  XCR2 - Transmit Control Register 2
//
#define MCBSP_REVERSE_MASK                  (3 << 3)
#define MCBSP_REVERSE_MSB_FIRST             (0 << 3)
#define MCBSP_REVERSE_LSB_FIRST             (1 << 3)

#define MCBSP_DATDLY_MASK                   (3 << 0)
#define MCBSP_DATDLY_0BIT                   (0 << 0)
#define MCBSP_DATDLY_1BIT                   (1 << 0)
#define MCBSP_DATDLY_2BIT                   (2 << 0)

#define MCBSP_PHASE_MASK                    (1 << 15)
#define MCBSP_PHASE_SINGLE                  (0 << 15)
#define MCBSP_PHASE_DUAL                    (1 << 15)

// These values applies to
//  RCR2 - Receive Control Register 2
//  XCR2 - Transmit Control Register 2
//  RCR1 - Receive Control Register 1
//  XCR1 - Transmit Control Register 1
//
#define MCBSP_FRAME_LENGTH_MASK             (0x7F << 8)
#define MCBSP_FRAME_LENGTH(x)               (((x - 1) << 8) & MCBSP_FRAME_LENGTH_MASK)

#define MCBSP_WORD_LENGTH_MASK              (7 << 5)
#define MCBSP_WORD_LENGTH_8                 (0 << 5)
#define MCBSP_WORD_LENGTH_12                (1 << 5)
#define MCBSP_WORD_LENGTH_16                (2 << 5)
#define MCBSP_WORD_LENGTH_20                (3 << 5)
#define MCBSP_WORD_LENGTH_24                (4 << 5)
#define MCBSP_WORD_LENGTH_32                (5 << 5)
#define MCBSP_WORD_LENGTH(x)                (((x) << 5) & MCBSP_WORD_LENGTH_MASK)

// Receive Control Register 2
//
// #define MCBSP_RCR2_RFIG                     (1 << 2)    // does not exist on 2430


// Transmit ControlRegister 2
//
// #define MCBSP_XCR2_XFIG                     (1 << 2)    // does not exist on 2430

// Sample Rate Generator Register 2
//
#define MCBSP_SRGR2_GSYNC                   (1 << 15)
#define MCBSP_SRGR2_CLKSP                   (1 << 14)
#define MCBSP_SRGR2_CLKSM                   (1 << 13)
#define MCBSP_SRGR2_FSGM                    (1 << 12)
#define MCBSP_SRGR2_FPER_MASK               (0x7FF << 0)
#define MCBSP_SRGR2_FPER(x)                 ((x) & MCBSP_SRGR2_FPER_MASK)

// Sample Rate Generator Register 1
//
#define MCBSP_SRGR1_FWID_MASK               (0xFF << 8)
#define MCBSP_SRGR1_FWID(x)                 (((x - 1) << 8) & MCBSP_SRGR1_FWID_MASK)
#define MCBSP_SRGR1_CLKGDV_MASK             (0xFF << 0)
#define MCBSP_SRGR1_CLKGDV(x)               ((x) & MCBSP_SRGR1_CLKGDV_MASK)

// Pin Control Register
//
#define MCBSP_PCR_IDLE_EN                   (1 << 14)
#define MCBSP_PCR_XIOEN                     (1 << 13)
#define MCBSP_PCR_RIOEN                     (1 << 12)
#define MCBSP_PCR_FSXM                      (1 << 11)
#define MCBSP_PCR_FSRM                      (1 << 10)
#define MCBSP_PCR_CLKXM                     (1 << 9)
#define MCBSP_PCR_CLKRM                     (1 << 8)
#define MCBSP_PCR_SCLKME                    (1 << 7)
#define MCBSP_PCR_CLKS_STAT                 (1 << 6)    // (r)
#define MCBSP_PCR_DX_STAT                   (1 << 5)
#define MCBSP_PCR_DR_STAT                   (1 << 4)    // (r)
#define MCBSP_PCR_FSXP                      (1 << 3)
#define MCBSP_PCR_FSRP                      (1 << 2)
#define MCBSP_PCR_CLKXP                     (1 << 1)
#define MCBSP_PCR_CLKRP                     (1 << 0)

// Pin Revision Number Register
//
#define MCBSP_REV_MASK                      (0xFF << 0)

// Multi channel Register MCR1/MCR2
//
#define MCBSP_PARTITION_MODE                (1 << 9)
#define MCBSP_PARTITION_A_BLOCK(blk)        ((blk) << 5)
#define MCBSP_PARTITION_B_BLOCK(blk)        ((blk) << 7)
#define MCBSP_MCR2_RMCM_TX(chnl)            ((chnl) << 0)
#define MCBSP_MCR1_RMCM_RX                  (1 << 0)

// SYSCONFIG Register Bits
//
#define MCBSP_SYSCONFIG_SOFTRESET           (1<<1)
#define MCBSP_SYSCONFIG_ENAWAKEUP           (1<<2)
#define MCBSP_SYSCONFIG_FORCEIDLE           (0<<3)
#define MCBSP_SYSCONFIG_DISABLEIDLE         (1<<3)
#define MCBSP_SYSCONFIG_SMARTIDLE           (2<<3)
#define MCBSP_SYSCONFIG_CLOCKACTIVITY(clk)  ((clk&0x03)<<8)

// IRQENABLE Register Bits
#define MCBSP_IRQENABLE_XEMPTYEOFEN          (1<<14)
#define MCBSP_IRQENABLE_XOVFLEN              (1<<12)
#define MCBSP_IRQENABLE_XUNDFLEN             (1<<11)
#define MCBSP_IRQENABLE_XRDYEN               (1<<10)
#define MCBSP_IRQENABLE_XEOFEN               (1<<9)
#define MCBSP_IRQENABLE_XFSXEN               (1<<8)
#define MCBSP_IRQENABLE_XSYNCERREN           (1<<7)
#define MCBSP_IRQENABLE_ROVFLEN              (1<<5)
#define MCBSP_IRQENABLE_RUNDFLEN             (1<<4)
#define MCBSP_IRQENABLE_RRDYEN               (1<<3)
#define MCBSP_IRQENABLE_REOFEN               (1<<2)
#define MCBSP_IRQENABLE_RFSREN               (1<<1)
#define MCBSP_IRQENABLE_RSYNCERREN           (1<<0)


// WAKEUPEN Register Bits
#define MCBSP_WAKEUPEN_XEMPTYEOFEN          (1<<14)
#define MCBSP_WAKEUPEN_XRDYEN               (1<<10)
#define MCBSP_WAKEUPEN_XEOFEN               (1<<9)
#define MCBSP_WAKEUPEN_XFSXEN               (1<<8)
#define MCBSP_WAKEUPEN_XSYNCERREN           (1<<7)
#define MCBSP_WAKEUPEN_RRDYEN               (1<<3)
#define MCBSP_WAKEUPEN_REOFEN               (1<<2)
#define MCBSP_WAKEUPEN_RFSREN               (1<<1)
#define MCBSP_WAKEUPEN_RSYNCERREN           (1<<0)

// XCCR Register bits
#define MCBSP_XCCR_XDISABLE					(1<<0)
#define MCBSP_XCCR_XDMAEN					(1<<3)
#define MCBSP_XCCR_DLB						(1<<5)
#define MCBSP_XCCR_XDISABLE					(1<<0)
#define MCBSP_XCCR_XFULL_CYCLE				(1<<11)
#define MCBSP_XCCR_DXENDLY_18_NS			(1<<12)
#define MCBSP_XCCR_DXENDLY_26_NS			(2<<12)
#define MCBSP_XCCR_DXENDLY_35_NS			(3<<12)
#define MCBSP_XCCR_DXENDLY_42_NS			(4<<12)
#define MCBSP_XCCR_PPCONNECT				(1<<14)
#define MCBSP_XCCR_EXTCLK_GATE				(1<<15)

// McBSP Side Tone Register SSELCR
//
#define MCBSP_SSELCR_SIDETONEEN             (1 << 10)
#define MCBSP_SSELCR_OCH1ASSIGN(chnl)       ((chnl) << 7)
#define MCBSP_SSELCR_OCH0ASSIGN(chnl)       ((chnl) << 4)
#define MCBSP_SSELCR_ICH1ASSIGN(chnl)       ((chnl) << 2)
#define MCBSP_SSELCR_ICH0ASSIGN(chnl)       ((chnl) << 0)

// Side Tone Registers
//

// SSELCR Register
//
#define ST_SSELCR_SIDETONEEN                (1 << 0)
#define ST_SSELCR_COEFFWREN                 (1 << 1)
#define ST_SSELCR_COEFFWRDONE               (1 << 2)

// FIR control regsiter
//
#define ST_SFIRCR_FIRCOEFF(coeff)           ((coeff) << 0)

// Gain control register
//
#define ST_SGAINCR_CH0GAIN(gain)            (((gain) << 0) & 0x0000FFFF)
#define ST_SGAINCR_CH1GAIN(gain)            (((gain) << 16) & 0xFFFF0000)

// SYSCONFIG register
//
#define ST_SYSCONFIG_AUTOIDLE               (1 << 0)

// Interrupt status register
//
#define ST_IRQSTATUS_OVRRERROR              (1 << 0)

// Interrupt enable register
//
#define ST_IRQENABLE_OVRRERROREN            (1 << 0)

#endif // __OMAP35XX_MCBSP_H
