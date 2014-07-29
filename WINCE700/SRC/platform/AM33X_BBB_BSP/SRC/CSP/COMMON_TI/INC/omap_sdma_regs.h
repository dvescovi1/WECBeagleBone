// All rights reserved ADENEO EMBEDDED 2010
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
//  File:  omap_sdma_regs.h
//
#ifndef __OMAP_SDMA_REGS_H
#define __OMAP_SDMA_REGS_H

#define OMAP_DMA_LOGICAL_CHANNEL_COUNT          (32)
#define OMAP_DMA_INTERRUPT_COUNT                (4)

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_DMATYPE_e
//
//  defines a dma channel type.
//
#define DMA_TYPE_SYSTEM                         0
#define DMA_TYPE_CAMERA                         1


//------------------------------------------------------------------------------
// Module      Base
// Name        Address
typedef volatile struct {
   UINT32 CCR;              //offset 0x0,chnl ctrl
   UINT32 CLNK_CTRL;        //offset 0x4,chnl link ctrl
   UINT32 CICR;             //offset 0x8,chnl intr ctrl
   UINT32 CSR;              //offset 0xC,chnl status
   UINT32 CSDP;             //offset 0x10,chnl src,dest params
   UINT32 CEN;              //offset 0x14,chnl element number
   UINT32 CFN;              //offset 0x18,chnl frame number
   UINT32 CSSA;             //offset 0x1C,chnl src start addr
   UINT32 CDSA;             //offset 0x20,chnl dest start addr
   UINT32 CSEI;             //offset 0x24,chnl src element index
   UINT32 CSFI;             //offset 0x28,chnl src frame index
   UINT32 CDEI;             //offset 0x2C,chnl destination element index
   UINT32 CDFI;             //offset 0x30,chnl destination frame index
   UINT32 CSAC;             //offset 0x34,chnl src address counter
   UINT32 CDAC;             //offset 0x38,chnl dest address counter
   UINT32 CCEN;             //offset 0x3C,chnl cur trans element no
   UINT32 CCFN;             //offset 0x40,chnl cur trans frame no
   UINT32 COLOR;            //offset 0x44,chnl DMA color key/solid color
   UINT32 ulRESERVED_1[6];  // 48-60 Reserved
}
OMAP_DMA_LC_REGS;


typedef volatile struct {
   UINT32 DMA4_REVISION;     //offset 0x0, Revision code
   UINT32 zzzReserved01;
   UINT32 DMA4_IRQSTATUS_L0; //offset 0x08,intr status over line L0
   UINT32 DMA4_IRQSTATUS_L1; //offset 0x0C,intr status over line L1
   UINT32 DMA4_IRQSTATUS_L2; //offset 0x10,intr status over line L2
   UINT32 DMA4_IRQSTATUS_L3; //offset 0x14,intr status over line L3
   UINT32 DMA4_IRQENABLE_L0; //offset 0x18,intr enable over line L0
   UINT32 DMA4_IRQENABLE_L1; //offset 0x1C,intr enable over line L1
   UINT32 DMA4_IRQENABLE_L2; //offset 0x20,intr enable over line L2
   UINT32 DMA4_IRQENABLE_L3; //offset 0x24,intr enable over line L3
   UINT32 DMA4_SYSSTATUS;    //offset 0x28,module status
   UINT32 DMA4_OCP_SYSCONFIG;//offset 0x2C,OCP i/f params control
   UINT32 zzzReserved02[13];
   UINT32 DMA4_CAPS_0;       //offset 0x64,DMA capabilities reg 0 LSW
   UINT32 zzzReserved03;
   UINT32 DMA4_CAPS_2;       //offset 0x6C,DMA capabilities reg 2
   UINT32 DMA4_CAPS_3;       //offset 0x70,DMA capabilities reg 3
   UINT32 DMA4_CAPS_4;       //offset 0x74,DMA capabilities reg 4
   UINT32 DMA4_GCR;          //offset 0x78,chnl DMA global register
   UINT32 zzzReserved04;
   OMAP_DMA_LC_REGS CHNL_CTRL[32];//offset 0x80-0x920,chnl
}
OMAP_SDMA_REGS;

typedef volatile struct {
   UINT32 DMA4_REVISION;              //offset 0x0, Revision code
   UINT32 zzzReserved01;
   UINT32 DMA4_IRQSTATUS_L0; //offset 0x08,intr status over line L0
   UINT32 DMA4_IRQSTATUS_L1; //offset 0x0C,intr status over line L1
   UINT32 DMA4_IRQSTATUS_L2; //offset 0x10,intr status over line L2
   UINT32 DMA4_IRQSTATUS_L3; //offset 0x14,intr status over line L3
   UINT32 DMA4_IRQENABLE_L0; //offset 0x18,intr enable over line L0
   UINT32 DMA4_IRQENABLE_L1; //offset 0x1C,intr enable over line L1
   UINT32 DMA4_IRQENABLE_L2; //offset 0x20,intr enable over line L2
   UINT32 DMA4_IRQENABLE_L3; //offset 0x24,intr enable over line L3
   UINT32 DMA4_SYSSTATUS;    //offset 0x28,module status
   UINT32 DMA4_OCP_SYSCONFIG;//offset 0x2C,OCP i/f params control
   UINT32 zzzReserved02[13];
   UINT32 DMA4_CAPS_0;       //offset 0x64,DMA capabilities reg 0 LSW
   UINT32 zzzReserved03;
   UINT32 DMA4_CAPS_2;       //offset 0x6C,DMA capabilities reg 2
   UINT32 DMA4_CAPS_3;       //offset 0x70,DMA capabilities reg 3
   UINT32 DMA4_CAPS_4;       //offset 0x74,DMA capabilities reg 4
   UINT32 DMA4_GCR;          //offset 0x78,chnl DMA global register
   UINT32 zzzReserved04;
   OMAP_DMA_LC_REGS CHNL_CTRL[4];//offset 0x80-0x920,chnl
}
OMAP_CAMDMA_REGS;


//------------------------------------------------------------------------------
// System DMA request mappings

#define SDMA_REQ_XTI                    1
#define SDMA_REQ_EXT_DMA_REQ_0          2
#define SDMA_REQ_EXT_DMA_REQ_1          3
#define SDMA_REQ_GPMC                   4
#define SDMA_REQ_GFX                    5
#define SDMA_REQ_DSS                    6
#define SDMA_REQ_RESERVED1              7
#define SDMA_REQ_CWT                    8
#define SDMA_REQ_AES_TX                 9
#define SDMA_REQ_AES_RX                 10
#define SDMA_REQ_DES_TX                 11
#define SDMA_REQ_DES_RX                 12
#define SDMA_REQ_SHA1MD5_RX             13
#define SDMA_REQ_RESERVED2              14
#define SDMA_REQ_SPI3_TX0               15
#define SDMA_REQ_SPI3_RX0               16
#define SDMA_REQ_MCBSP3_TX              17
#define SDMA_REQ_MCBSP3_RX              18
#define SDMA_REQ_MCBSP4_TX              19
#define SDMA_REQ_MCBSP4_RX              20
#define SDMA_REQ_MCBSP5_TX              21
#define SDMA_REQ_MCBSP5_RX              22
#define SDMA_REQ_SPI3_TX1               23
#define SDMA_REQ_SPI3_RX1               24
#define SDMA_REQ_RESERVED3              25
#define SDMA_REQ_RESERVED4              26
#define SDMA_REQ_I2C1_TX                27
#define SDMA_REQ_I2C1_RX                28
#define SDMA_REQ_I2C2_TX                29
#define SDMA_REQ_I2C2_RX                30
#define SDMA_REQ_MCBSP1_TX              31
#define SDMA_REQ_MCBSP1_RX              32
#define SDMA_REQ_MCBSP2_TX              33
#define SDMA_REQ_MCBSP2_RX              34
#define SDMA_REQ_SPI1_TX0               35
#define SDMA_REQ_SPI1_RX0               36
#define SDMA_REQ_SPI1_TX1               37
#define SDMA_REQ_SPI1_RX1               38
#define SDMA_REQ_SPI1_TX2               39
#define SDMA_REQ_SPI1_RX2               40
#define SDMA_REQ_SPI1_TX3               41
#define SDMA_REQ_SPI1_RX3               42
#define SDMA_REQ_SPI2_TX0               43
#define SDMA_REQ_SPI2_RX0               44
#define SDMA_REQ_SPI2_TX1               45
#define SDMA_REQ_SPI2_RX1               46
#define SDMA_REQ_MMC2_TX                47
#define SDMA_REQ_MMC2_RX                48
#define SDMA_REQ_UART1_TX               49
#define SDMA_REQ_UART1_RX               50
#define SDMA_REQ_UART2_TX               51
#define SDMA_REQ_UART2_RX               52
#define SDMA_REQ_UART3_TX               53
#define SDMA_REQ_UART3_RX               54
#define SDMA_REQ_USB0_TX0               55
#define SDMA_REQ_USB0_RX0               56
#define SDMA_REQ_USB0_TX1               57
#define SDMA_REQ_USB0_RX1               58
#define SDMA_REQ_USB0_TX2               59
#define SDMA_REQ_USB0_RX2               60
#define SDMA_REQ_MMC1_TX                61
#define SDMA_REQ_MMC1_RX                62
#define SDMA_REQ_RESERVED5              63
#define SDMA_REQ_REQ3                   64
#define SDMA_REQ_RESERVED6              65
#define SDMA_REQ_RESERVED7              66
#define SDMA_REQ_RESERVED8              67
#define SDMA_REQ_RESERVED9              68
#define SDMA_REQ_RESERVED10             69
#define SDMA_REQ_SPI4_TX0               70
#define SDMA_REQ_SPI4_RX0               71
#define SDMA_REQ_DSS_REQ0               72
#define SDMA_REQ_DSS_REQ1               73
#define SDMA_REQ_DSS_REQ2               74
#define SDMA_REQ_DSS_REQ3               75
#define SDMA_REQ_RESERVED11             76
#define SDMA_REQ_MMC3_TX                77
#define SDMA_REQ_MMC3_RX                78
// 37xx only
#define SDMA_REQ_UART4_TX               81
#define SDMA_REQ_UART4_RX               82


//------------------------------------------------------------------------------
// dma interrupt enable masks
#define DMA_INTERRUPT_MASKALL               (0xFFFFFFFF)

//------------------------------------------------------------------------------
// GCR register fields
#define DMA_GCR_ARBITRATION_RATE(x)         (x << 16)
#define DMA_GCR_HITHREAD_RSVP(x)            (x << 12)
#define DMA_GCR_FIFODEPTH(x)                (x << 0)

//------------------------------------------------------------------------------
// CCR register fields
#define DMA_CCR_WRITE_PRIO                  (1 << 26)
#define DMA_CCR_BUFFERING_DISABLE           (1 << 25)
#define DMA_CCR_SEL_SRC_DST_SYNCH           (1 << 24)
#define DMA_CCR_PREFETCH                    (1 << 23)
#define DMA_CCR_SUPERVISOR                  (1 << 22)
#define DMA_CCR_SECURE                      (1 << 21)
#define DMA_CCR_BS                          (1 << 18)
#define DMA_CCR_TRANSPARENT_COPY_ENABLE     (1 << 17)
#define DMA_CCR_CONST_FILL_ENABLE           (1 << 16)

#define DMA_CCR_DST_AMODE_MASK              (3 << 14)
#define DMA_CCR_DST_AMODE_DOUBLE            (3 << 14)
#define DMA_CCR_DST_AMODE_SINGLE            (2 << 14)
#define DMA_CCR_DST_AMODE_POST_INC          (1 << 14)
#define DMA_CCR_DST_AMODE_CONST             (0 << 14)

#define DMA_CCR_SRC_AMODE_MASK              (3 << 12)
#define DMA_CCR_SRC_AMODE_DOUBLE            (3 << 12)
#define DMA_CCR_SRC_AMODE_SINGLE            (2 << 12)
#define DMA_CCR_SRC_AMODE_POST_INC          (1 << 12)
#define DMA_CCR_SRC_AMODE_CONST             (0 << 12)

#define DMA_CCR_WR_ACTIVE                   (1 << 10)
#define DMA_CCR_RD_ACTIVE                   (1 << 9)
#define DMA_CCR_SUSPEND_SENSITIVE           (1 << 8)
#define DMA_CCR_ENABLE                      (1 << 7)
#define DMA_CCR_READ_PRIO                   (1 << 6)
#define DMA_CCR_FS                          (1 << 5)

#define DMA_CCR_SYNC(req)                   (((req) & 0x1F) | (((DWORD)(req) & 0x60) << 14))

//------------------------------------------------------------------------------
// CICR register fields

#define DMA_CICR_MISALIGNED_ERR_IE          (1 << 11)
#define DMA_CICR_SUPERVISOR_ERR_IE          (1 << 10)
#define DMA_CICR_SECURE_ERR_IE              (1 << 9)
#define DMA_CICR_TRANS_ERR_IE               (1 << 8)
#define DMA_CICR_PKT_IE                     (1 << 7)
#define DMA_CICR_BLOCK_IE                   (1 << 5)
#define DMA_CICR_LAST_IE                    (1 << 4)
#define DMA_CICR_FRAME_IE                   (1 << 3)
#define DMA_CICR_HALF_IE                    (1 << 2)
#define DMA_CICR_DROP_IE                    (1 << 1)

//------------------------------------------------------------------------------
// CSR register fields

#define DMA_CSR_MISALIGNED_ERR              (1 << 11)
#define DMA_CSR_SUPERVISOR_ERR              (1 << 10)
#define DMA_CSR_SECURE_ERR                  (1 << 9)
#define DMA_CSR_TRANS_ERR                   (1 << 8)
#define DMA_CSR_PKT                         (1 << 7)
#define DMA_CSR_SYNC                        (1 << 6)
#define DMA_CSR_BLOCK                       (1 << 5)
#define DMA_CSR_LAST                        (1 << 4)
#define DMA_CSR_FRAME                       (1 << 3)
#define DMA_CSR_HALF                        (1 << 2)
#define DMA_CSR_DROP                        (1 << 1)

//------------------------------------------------------------------------------
// CSR register fields

#define DMA_CSDP_SRC_ENDIAN_BIG                 0x00200000
#define DMA_CSDP_SRC_ENDIAN_LOCK                0x00100000
#define DMA_CSDP_DST_ENDIAN_BIG                 0x00080000
#define DMA_CSDP_DST_ENDIAN_LOCK                0x00040000
#define DMA_CSDP_WRITE_MODE_MASK                0x00030000
#define DMA_CSDP_WRITE_MODE_NOPOST              0x00000000
#define DMA_CSDP_WRITE_MODE_POSTED              0x00010000
#define DMA_CSDP_WRITE_MODE_POSTED_EXCEPT       0x00020000
#define DMA_CSDP_DST_BURST_MASK                 0x0000C000
#define DMA_CSDP_DST_BURST_NONE                 0x00000000
#define DMA_CSDP_DST_BURST_16BYTES_4x32_2x64    0x00004000
#define DMA_CSDP_DST_BURST_32BYTES_8x32_4x64    0x00008000
#define DMA_CSDP_DST_BURST_64BYTES_16x32_8x64   0x0000C000
#define DMA_CSDP_DST_PACKED                     0x00002000
#define DMA_CSDP_WR_ADDR_TRSLT_MASK             0x00001E00
#define DMA_CSDP_SRC_BURST_MASK                 0x00000180
#define DMA_CSDP_SRC_BURST_NONE                 0x00000000
#define DMA_CSDP_SRC_BURST_16BYTES_4x32_2x64    0x00000080
#define DMA_CSDP_SRC_BURST_32BYTES_8x32_4x64    0x00000100
#define DMA_CSDP_SRC_BURST_64BYTES_16x32_8x64   0x00000180
#define DMA_CSDP_SRC_PACKED                     0x00000040
#define DMA_CSDP_RD_ADDR_TRSLT_MASK             0x0000003C
#define DMA_CSDP_DATATYPE_MASK                  0x00000003
#define DMA_CSDP_DATATYPE_S8                    0x00000000
#define DMA_CSDP_DATATYPE_S16                   0x00000001
#define DMA_CSDP_DATATYPE_S32                   0x00000002


//------------------------------------------------------------------------------
// CLNK_CTRL register fields

#define DMA_CLNK_CTRL_ENABLE_LINK           (1 << 15)

//------------------------------------------------------------------------------

#define DMA_LCD_CSDP_B2_BURST_MASK          (3 << 14)
#define DMA_LCD_CSDP_B2_BURST_4             (2 << 14)
#define DMA_LCD_CSDP_B2_BURST_NONE          (0 << 14)

#define DMA_LCD_CSDP_B2_PACK                (1 << 13)

#define DMA_LCD_CSDP_B2_DATA_TYPE_MASK      (3 << 11)
#define DMA_LCD_CSDP_B2_DATA_TYPE_S32       (2 << 11)
#define DMA_LCD_CSDP_B2_DATA_TYPE_S16       (1 << 11)
#define DMA_LCD_CSDP_B2_DATA_TYPE_S8        (0 << 11)

#define DMA_LCD_CSDP_B1_BURST_MASK          (3 << 7)
#define DMA_LCD_CSDP_B1_BURST_4             (2 << 7)
#define DMA_LCD_CSDP_B1_BURST_NONE          (0 << 7)

#define DMA_LCD_CSDP_B1_PACK                (1 << 6)

#define DMA_LCD_CSDP_B1_DATA_TYPE_MASK      (3 << 0)
#define DMA_LCD_CSDP_B1_DATA_TYPE_S32       (2 << 0)
#define DMA_LCD_CSDP_B1_DATA_TYPE_S16       (1 << 0)
#define DMA_LCD_CSDP_B1_DATA_TYPE_S8        (0 << 0)

//------------------------------------------------------------------------------

#define DMA_LCD_CCR_B2_AMODE_MASK           (3 << 14)
#define DMA_LCD_CCR_B2_AMODE_DOUBLE         (3 << 14)
#define DMA_LCD_CCR_B2_AMODE_SINGLE         (2 << 14)
#define DMA_LCD_CCR_B2_AMODE_INC            (1 << 14)

#define DMA_LCD_CCR_B1_AMODE_MASK           (3 << 12)
#define DMA_LCD_CCR_B1_AMODE_DOUBLE         (3 << 12)
#define DMA_LCD_CCR_B1_AMODE_SINGLE         (2 << 12)
#define DMA_LCD_CCR_B1_AMODE_INC            (1 << 12)

#define DMA_LCD_CCR_END_PROG                (1 << 11)
#define DMA_LCD_CCR_OMAP32                  (1 << 10)
#define DMA_LCD_CCR_REPEAT                  (1 << 9)
#define DMA_LCD_CCR_AUTO_INIT               (1 << 8)
#define DMA_LCD_CCR_EN                      (1 << 7)
#define DMA_LCD_CCR_PRIO                    (1 << 6)
#define DMA_LCD_CCR_SYNC_PR                 (1 << 4)

//------------------------------------------------------------------------------

#define DMA_LCD_CTRL_EXTERNAL_LCD           (1 << 8)

#define DMA_LCD_CTRL_SOURCE_PORT_MASK       (3 << 6)
#define DMA_LCD_CTRL_SOURCE_PORT_SDRAM      (0 << 6)
#define DMA_LCD_CTRL_SOURCE_PORT_OCP_T1     (1 << 6)
#define DMA_LCD_CTRL_SOURCE_PORT_OCP_T2     (2 << 6)

#define DMA_LCD_CTRL_BLOCK_MODE             (1 << 0)

//------------------------------------------------------------------------------

#define DMA_LCD_LCH_CTRL_TYPE_D             (4 << 0)

//------------------------------------------------------------------------------

#endif
