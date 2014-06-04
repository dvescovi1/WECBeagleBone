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
//  Header:  am3xx_i2c_regs.h
//
#ifndef __AM3XX_I2C_REGS_H
#define __AM3XX_I2C_REGS_H

#pragma warning(push)
#pragma warning(disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>
#pragma warning(pop)

//------------------------------------------------------------------------------

typedef volatile struct {
    UINT16 REVNB_LO;            // 0000
    UINT16 zzzReserved0002;
    UINT16 REVNB_HI;            // 0004
    UINT16 zzzReserved0006[5];
    UINT16 SYSC;                // 0010
    UINT16 zzzReserved0012[9];
    UINT16 IRQSTATUS_RAW;       // 0024
    UINT16 zzzReserved0026;
    UINT16 IRQSTATUS;           // 0028
    UINT16 zzzReserved002A;
    UINT16 IRQENABLE_SET;       // 002C
    UINT16 zzzReserved002E;
    UINT16 IRQENABLE_CLR;       // 0030
    UINT16 zzzReserved0032;
    UINT16 WE;                  // 0034
    UINT16 zzzReserved0036;
    UINT16 DMARXENABLE_SET;     // 0038
    UINT16 zzzReserved003A;
    UINT16 DMATXENABLE_SET;     // 003C
    UINT16 zzzReserved003E;
    UINT16 DMARXENABLE_CLR;     // 0040
    UINT16 zzzReserved0042;
    UINT16 DMATXENABLE_CLR;     // 0044
    UINT16 zzzReserved0046;
    UINT16 DMARXWAKE_EN;        // 0048
    UINT16 zzzReserved004A;
    UINT16 DMATXWAKE_EN;        // 004C
    UINT16 zzzReserved004E[27];
    UINT16 IE;                  // 0084
    UINT16 zzzReserved0086;
    UINT16 STAT;                // 0088
    UINT16 zzzReserved008A;
    UINT16 zzzReserved008C;     // 008C
    UINT16 zzzReserved008E;
    UINT16 SYSS;                // 0090
    UINT16 zzzReserved0092;
    UINT16 BUF;                 // 0094
    UINT16 zzzReserved0096;
    UINT16 CNT;                 // 0098
    UINT16 zzzReserved009A;
    UINT16 DATA;                // 009C
    UINT16 zzzReserved009E;
    UINT16 zzzReserved00A0;     // 00A0
    UINT16 zzzReserved00A2;
    UINT16 CON;                 // 00A4
    UINT16 zzzReserved00A6;
    UINT16 OA0;                 // 00A8
    UINT16 zzzReserved00AA;
    UINT16 SA;                  // 00AC
    UINT16 zzzReserved00AE;
    UINT16 PSC;                 // 00B0
    UINT16 zzzReserved00B2;
    UINT16 SCLL;                // 00B4
    UINT16 zzzReserved00B6;
    UINT16 SCLH;                // 00B8
    UINT16 zzzReserved00BA;
    UINT16 SYSTEST;             // 00BC
    UINT16 zzzReserved00BE;
    UINT16 BUFSTAT;             // 00C0
    UINT16 zzzReserved00C2;
    UINT16 OA1;                 // 00C4
    UINT16 zzzReserved00C6;
    UINT16 OA2;                 // 00C8
    UINT16 zzzReserved00CA;
    UINT16 OA3;                 // 00CC
    UINT16 zzzReserved00CE;
    UINT16 ACTOA;               // 00D0
    UINT16 zzzReserved00D2;
    UINT16 SBLOCK;              // 00D4
} AM3XX_I2C_REGS;

//------------------------------------------------------------------------------

#define I2C_STAT_XDR                            (1 << 14)
#define I2C_STAT_RDR                            (1 << 13)
#define I2C_STAT_BB                             (1 << 12)
#define I2C_STAT_ROVR                           (1 << 11)
#define I2C_STAT_XUDF                           (1 << 10)
#define I2C_STAT_AAS                            (1 << 9)
#define I2C_STAT_BF                             (1 << 8)
#define I2C_STAT_AERR                           (1 << 7)
#define I2C_STAT_STC                            (1 << 6)
#define I2C_STAT_GC                             (1 << 5)
#define I2C_STAT_XRDY                           (1 << 4)
#define I2C_STAT_RRDY                           (1 << 3)
#define I2C_STAT_DRDY                           (1 << 3)
#define I2C_STAT_ARDY                           (1 << 2)
#define I2C_STAT_NACK                           (1 << 1)
#define I2C_STAT_AL                             (1 << 0)

#define I2C_SYSS_RDONE                          (1 << 0)

#define I2C_SYSC_CLKACTY_FCLK                   (1 << 9)
#define I2C_SYSC_CLKACTY_ICLK                   (1 << 8)
#define I2C_SYSC_SRST                           (1 << 1)
#define I2C_SYSC_AUTOIDLE                       (1 << 0)

#define I2C_CON_EN                              (1 << 15)
#define I2C_CON_OPMODE_FS                       (0 << 12)
#define I2C_CON_OPMODE_HS                       (1 << 12)
#define I2C_CON_OPMODE_SCCB                     (2 << 12)
#define I2C_CON_STB                             (1 << 11)
#define I2C_CON_MST                             (1 << 10)
#define I2C_CON_TRX                             (1 << 9)
#define I2C_CON_XSA                             (1 << 8)
#define I2C_CON_XOA0                            (1 << 7)
#define I2C_CON_XOA1                            (1 << 6)
#define I2C_CON_XOA2                            (1 << 5)
#define I2C_CON_XOA3                            (1 << 4)
#define I2C_CON_STP                             (1 << 1)
#define I2C_CON_STT                             (1 << 0)
#define I2C_CON_DISABLE	                        (0 << 0)

#define I2C_BUF_RDMA_EN                         (1 << 15)
#define I2C_BUF_RXFIFO_CLR                      (1 << 14)
#define I2C_BUF_RTRSH_MASK                      (0x3F00)
#define I2C_BUF_RTRSH_SHIFT                     (8)
#define I2C_BUF_RTRSH(x)                        (((x) << I2C_BUF_RTRSH_SHIFT) & I2C_BUF_RTRSH_MASK)
#define I2C_BUF_XDMA_EN                         (1 << 7)
#define I2C_BUF_TXFIFO_CLR                      (1 << 6)
#define I2C_BUF_XTRSH_MASK                      (0x3F)
#define I2C_BUF_XTRSH_SHIFT                     (0)
#define I2C_BUF_XTRSH(x)                        (((x) << I2C_BUF_XTRSH_SHIFT) & I2C_BUF_XTRSH_MASK)

#define I2C_BUFSTAT_FIFODEPTH_MASK              (0xC000)
#define I2C_BUFSTAT_FIFODEPTH_SHIFT             (14)
#define I2C_BUFSTAT_RXSTAT_MASK                 (0x3F00)
#define I2C_BUFSTAT_RXSTAT_SHIFT                (8)
#define I2C_BUFSTAT_TXSTAT_MASK                 (0x003F)
#define I2C_BUFSTAT_TXSTAT_SHIFT                (0)

#define I2C_OP_FULLSPEED_MODE                   (0)
#define I2C_OP_HIGHSPPED_MODE                   (1)
#define I2C_OP_SCCB_MODE                        (2)

//------------------------------------------------------------------------------
// useful defines

#define I2C_MASTER_CODE                         (0x4000)
#define I2C_SLAVE_MASK                          (0x007f)

//------------------------------------------------------------------------------
//Internal clock was connected to I2C is 96mhz fclk;
//100kbps
#define I2C_SS_PSC_VAL                          (23)
#define I2C_SS_SCLL_VAL                         (13)
#define I2C_SS_SCLH_VAL                         (15)

//400kbps
#define I2C_FS_PSC_VAL				            (9)
#define I2C_FS_SCLL_VAL                         (5)
#define I2C_FS_SCLH_VAL                         (7)

//>400kbps, first phase
#define I2C_HS_PSC_VAL                          (4)
#define I2C_HS_SCLL                             (17)
#define I2C_HS_SCLH                             (19)

//second phase
#define I2C_1P6M_HSSCLL                         ((23 << 8) | I2C_HS_SCLL)
#define I2C_1P6M_HSSCLH                         ((25 << 8) | I2C_HS_SCLH)

#define I2C_2P4M_HSSCLL                         ((13 << 8) | I2C_HS_SCLL)
#define I2C_2P4M_HSSCLH                         ((15 << 8) | I2C_HS_SCLH)

#define I2C_3P2M_HSSCLL                         ((8 << 8) | I2C_HS_SCLL)
#define I2C_3P2M_HSSCLH                         ((10 << 8) | I2C_HS_SCLH)

#define I2C_DEFAULT_HSSCLL                      I2C_3P2M_HSSCLL
#define I2C_DEFAULT_HSSCLH                      I2C_3P2M_HSSCLH


#define I2C_SYSTEST_ST_EN                       (1<<15)
#define I2C_SYSTEMTEST_TMODE3                   (3<<12)
#define I2C_SYSTEST_SCL_O                       (1<<2)
#define I2C_SYSTEST_SDA_O                       (1<<0)

#endif // __AM3XX_I2C_REGS_H
