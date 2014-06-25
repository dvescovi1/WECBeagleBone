//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#ifndef __AM33XCPSW3GREGS_H_INCLUDED__
#define __AM33XCPSW3GREGS_H_INCLUDED__

#include <windows.h>
#include "constants.h"

#define TIMEOUT_VALUE        30000       //30seconds

#define CPSW3G_MEMORY_SIZE   N16KB
#define MDIO_MEMORY_SIZE     0x00000100  //256
#define SS_MEMORY_SIZE       0x0000008C  //140

typedef struct __CPSW_SS_REGS__
{// offset from base: 0x900(centaurus); 0x1200(subarctic)
  volatile UINT32   IDVER;				// 0x000  Subsystem ID Version
  volatile UINT32   Soft_Reset;			// 0x004  Subsystem Soft Rest
  volatile UINT32   Control;			// 0x008  Subsystem Control
  volatile UINT32   Int_Control;		// 0x00C  Subsystem Interrupt Control
  volatile UINT32   C0_Rx_Thresh_En;	// 0x010  Subsystem Core 0 Receive Threshold Int Enable
  volatile UINT32   C0_Rx_En;	 		// 0x014  Subsystem Core 0 Receive Interrupt Enable
  volatile UINT32   C0_Tx_En;			// 0x018  Subsystem Core 0 Transmit Interrupt Enable
  volatile UINT32   C0_Misc_En; 		// 0x01C  Subsystem Core 0 Misc Interrupt Enable

  volatile UINT32   Rsvd0[8];   		// 0x020 - 0x03c

  volatile UINT32   C0_Rx_Thresh_STAT;	// 0x040  Subsystem Core 0 Receive Threshold Int Enable
  volatile UINT32   C0_Rx_STAT;	 		// 0x044  Subsystem Core 0 Receive Interrupt Enable
  volatile UINT32   C0_Tx_STAT;			// 0x048  Subsystem Core 0 Transmit Interrupt Enable
  volatile UINT32   C0_Misc_STAT; 		// 0x04C  Subsystem Core 0 Misc Interrupt Enable
  
  volatile UINT32   Rsvd1[8];   		// 0x050 - 0x06C

  volatile UINT32   C0_Rx_Imax;			// 0x070 Subsystem Core 0 Receive Intrs Per msec
  volatile UINT32   C0_Tx_Imax;	 		// 0x074 Subsystem Core 0 Transmit Intrs Per msec

  volatile UINT32   Rsvd2[4];   		// 0x078 - 0x084
  
  volatile UINT32   RGMII_CTL;	 		// 0x088 RGMII Control Signal Register 

} CPSW_SS_REGS, *PCPSW_SS_REGS;


// CPGMAC SLIVER
typedef struct __CPGMACSL_REGS__
{// offset from base: (centaurus)SL1: 0x700, SL2: 0x740
 //                   (subarctic)SL1: 0xd80, SL2: 0xdc0
  volatile UINT32   SL_IDVER;				// 0x000 CPGMAC_SL ID/Version
  volatile UINT32   SL_MacControl;			// 0x004 CPGMAC_SL Mac Control
  volatile UINT32   SL_MacStatus;			// 0x008 CPGMAC_SL Mac Status
  volatile UINT32   SL_Soft_Reset;			// 0x00c CPGMAC_SL Soft Reset
  volatile UINT32   SL_Rx_Maxlen;			// 0x010 CPGMAC_SL Receive Maximum Length 
  volatile UINT32   SL_BoffTest;			// 0x014 CPGMAC_SL Backoff Test Register
  volatile UINT32   SL_Rx_Pause;			// 0x018 CPGMAC_SL Receive Pause Timer Register
  volatile UINT32   SL_Tx_Pause;	 		// 0x01c CPGMAC_SL Transmit Pause Timer Register
  volatile UINT32   SL_EMControl;			// 0x020 CPGMAC_SL Emulation Control
  volatile UINT32   SL_Rx_Pri_Map; 			// 0x024 CPGMAC_SL Rx Pkt Pri to Header Pri Mapping
  volatile UINT32   Rsvd0[6];      			// 0x028 - 0x03c
} CpgmacSl_Regs;


// SLAVE PORT
typedef struct __CPSW3G_SL_REGS__
{
  // Offset from base: (Centaurus) P1: 0x050, P2: 0x090
  //                   (Subarctic) P1: 0x200, P2: 0x300

#ifdef AM33X_BSP
  volatile UINT32   Control;			// CPSW_3G Port Control
  volatile UINT32   Rsvd0_AM33X;		// 
#endif

  volatile UINT32   Max_Blks;			// CPSW_3G Port Maximum FIFO blocks Register
  volatile UINT32   BLK_CNT;			// CPSW_3G Port FIFO Block Usage Count (read only)
  volatile UINT32   Tx_In_Ctl;			// CPSW_3G Port Transmit FIFO Control
  volatile UINT32   Port_VLAN;			// CPSW_3G Port VLAN Register
  volatile UINT32   Tx_Pri_Map;			// CPSW_3G Port Tx Header Pri to Switch Pri Mapping Register

#ifdef AM387X_BSP
  volatile UINT32   TS_CTL;				// CPSW_3G Port Time Sync Control Register
  volatile UINT32   TS_SEQ_LTYPE;		// CPSW_3G Port Time Sync LTYPE (and SEQ_ID_OFFSET)
  volatile UINT32   TS_VLAN;			// CPSW_3G Port Time Sync VLAN2 and VLAN2 Register
#endif

#ifdef AM33X_BSP
  volatile UINT32   TS_SEQ_MTYPE;		// CPSW_3G Port Time Sync Seq ID Ofset and MSG type
#endif

  volatile UINT32   SL_SA_LO;	 		// CPSW_3G CPGMAC_SL Source Address Low Register
  volatile UINT32   SL_SA_HI;  			// CPSW_3G CPGMAC_SL Source Address High Register
  volatile UINT32   Send_Percent;		// CPSW_3G Port Transmit Queue Send Percentages

#ifdef AM387X_BSP
  volatile UINT32   Rsvd0[5];			// 0x02C - 0x03C
#endif

#ifdef AM33X_BSP
  volatile UINT32   Rsvd1_AM33X;		// 
  volatile UINT32   Rx_DSCP_Pri_Map[8];	// CPSW_3G Port Rx DSCP Priority to Rx Pkt Mapping 
  volatile UINT32   Idle2LPI;			// CPSW_3G Port EEE Idle to LPI counter load value
  volatile UINT32   LPI2Wake;			// CPSW_3G Port EEE LPI to wake counter load value
  volatile UINT32   Rsvd2_AM33X[42];	// 
#endif

} Cpsw3g_SL_Regs;


typedef struct __CPSW3G_REGS__
{
    volatile UINT32    CPSW_IdVer;			// 0x000 CPSW_3G ID Version Register
    volatile UINT32    CPSW_Control;		// 0x004 CPSW_3G Switch Control Register
    volatile UINT32    CPSW_Soft_Reset;		// 0x008 CPSW_3G Soft Reset Register
    volatile UINT32    CPSW_Stat_Port_En;	// 0x00c CPSW_3G Statistics Port Enable Register
    volatile UINT32    CPSW_Pritype;		// 0x010 CPSW_3G Transmit Priority Type Register

    volatile UINT32    CPSW_Soft_Idle;		// 0x014 CPSW_3G Software Idle
    volatile UINT32    CPSW_Thru_Rate;		// 0x018 CPSW_3G Throughput Rate
    volatile UINT32    CPSW_Gap_Thresh;		// 0x01C CPSW_3G CPGMAC_SL Short Gap Threshold
    volatile UINT32    CPSW_Tx_Start_WDS;	// 0x020 CPSW_3G Transmit Start Words
    volatile UINT32    CPSW_Flow_Control;	// 0x024 CPSW_3G Flow Control

#ifdef AM33X_BSP 
    volatile UINT32    CPSW_VLAN_Ltype;		// 0x028 CPSW LTYPE1 and LTYPE2
    volatile UINT32    CPSW_TS_Ltype;		// 0x02C CPSW VLAN LTYPE1 and LTYPE2
    volatile UINT32    CPSW_DLR_Ltype;		// 0x030 CPSW DLR LTYPE
    volatile UINT32    Rsvd0_AM33X[51];		// 0x034 - 0x0FC
#endif // AM33X_BSP

    // Host Port 0
    // (centaurus) 0x028 - 0x04C
    // (subarctic) 0x100 - 0x1FC 
#ifdef AM33X_BSP
    volatile UINT32    P0_Control;			// CPSW Port 0 Control
    volatile UINT32    Rsvd1_AM33X;			// 
#endif // AM33X_BSP
    volatile UINT32    P0_Max_blks;			// CPSW_3G Port 0 Maximum FIFO blocks Register
    volatile UINT32    P0_BLK_CNT;			// CPSW_3G Port 0 FIFO Block Usage Count (read only)
    volatile UINT32    P0_Tx_In_Ctl;		// CPSW_3G Port 0 Transmit FIFO Control
    volatile UINT32    P0_Port_VLAN;		// CPSW_3G Port 0 VLAN Register
    volatile UINT32    P0_Tx_Pri_Map;		// CPSW_3G Port 0 Tx Header Pri to Switch Pri Mapping Register
    volatile UINT32    CPDMA_Tx_Pri_Map;	// CPSW_3G CPDMA TX (Port 0 Rx) Pkt Pri to Header Pri Mapping
    volatile UINT32    CPDMA_Rx_Ch_Map;		// CPSW_3G CPDMA RX (Port 0 Tx) switch Pri to DMA channel Pri Mapping
    volatile UINT32    Rsvd0[3];			// (centaurus) 0x01c-0x024; (subarctic) 0x024-0x02c
#ifdef AM33X_BSP
    volatile UINT32    P0_Rx_DSCP_Pri_Map[8];	// 0x030 - 0x04C
    volatile UINT32    Rsvd2_AM33X[44];			// 0x050 - 0x0FC
#endif // AM33X_BSP

    // SLAVE PORT 
    // (centaurus) P1: 0x050 - 0x08C, P2: 0x090 - 0x0CC
    // (subarctic) P1: 0x800 - 0x2FC, P2: 0x300 - 0x3FC 
    Cpsw3g_SL_Regs     CPSW_SL_Regs[2];

#ifdef AM387X_BSP
    volatile UINT32    Rsvd1[12];			// 0x0D0 - 0x0FC
#endif

#ifdef AM33X_BSP
    volatile UINT32    Rsvd3_AM33X[256];	// 0x400 - 0x7FC
#endif

    // CPDMA 
    // (centaurus) 0x100 -
    // (subarctic) 0x800 -  
    volatile UINT32    Tx_Idver;			// 0x000 CPDMA_REGS TX Identification and Version
    volatile UINT32    Tx_Control;			// 0x004 CPDMA_REGS Transmit Control Register
    volatile UINT32    Tx_Teardown;			// 0x008 CPDMA_REGS TX Teardown Register
    volatile UINT32    Rsvd2;				// 0x00c
    volatile UINT32    Rx_Idver;			// 0x010 CPDMA_REGS RX Identification and Version
    volatile UINT32    Rx_Control;			// 0x014 CPDMA_REGS RX Control Register
    volatile UINT32    Rx_Teardown;			// 0x018 CPDMA_REGS RX Teardown Register
    volatile UINT32    CPDMA_Soft_Reset;	// 0x01c CPDMA_REGS CPDMA Soft Reset Register
    volatile UINT32    DMAControl;			// 0x020 CPDMA_REGS CPDMA CPDMA Control Register
    volatile UINT32    DMAStatus;			// 0x024 CPDMA_REGS CPDMA CPDMA Status Register 
    volatile UINT32    RX_Buffer_Offset;	// 0x028 CPDMA_REGS CPDMA Receive Buffer Offset  Register
    volatile UINT32    EMControl;			// 0x02c CPDMA_REGS CPDMA Emulation Control Register
    volatile UINT32    TX_PriN_Rate[8];		// 0x030-04C CPDMA_REGS Transmit (ingress) Priority N=0,...,7 Rate
    volatile UINT32    Rsvd3[12];			// 0x050-0x07C  

    volatile UINT32    Tx_IntStat_Raw;		// 0x080 CPDMA_INT Transmit Interrupt Status (Unmasked) Register
    volatile UINT32    Tx_IntStat_Masked;	// 0x084 CPDMA_INT Transmit Interrupt Status (Masked) Register
    volatile UINT32    Tx_IntMask_Set;		// 0x088 CPDMA_INT Transmit Interrupt Mask Set Register
    volatile UINT32    Tx_IntMask_Clear;	// 0x08c CPDMA_INT Transmit Interrupt Mask Clear Register
    volatile UINT32    CPDMA_In_Vector;		// 0x090 CPDMA_INT Input Vector Register (Read Only)
    volatile UINT32    CPDMA_EOI_Vector;	// 0x094 CPDMA_INT End of Interrupt Vector Register 
    volatile UINT32    Rsvd4[2];			// 0x098-0x09C  

    volatile UINT32    Rx_IntStat_Raw;		// 0x0a0 CPDMA_INT RX Interrupt Status (Unmasked) Register
    volatile UINT32    Rx_IntStat_Masked;	// 0x0a4 CPDMA_INT RX Interrupt Status (Masked) Register
    volatile UINT32    Rx_IntMask_Set;		// 0x0a8 CPDMA_INT RX Interrupt Mask Set Register
    volatile UINT32    Rx_IntMask_Clear;	// 0x0ac CPDMA_INT RX Interrupt Mask Clear Register
    volatile UINT32    DMA_IntStat_Raw;		// 0x0b0 CPDMA_INT DMA Interrupt Status (Unmasked) Register
    volatile UINT32    DMA_IntStat_Masked;	// 0x0b4 CPDMA_INT DMA Interrupt Status (Masked) Register
    volatile UINT32    DMA_IntMask_Set;		// 0x0b8 CPDMA_INT DMA Interrupt Mask Set Register
    volatile UINT32    DMA_IntMask_Clear;	// 0x0bc CPDMA_INT DMA Interrupt Mask Clear Register
    volatile UINT32    RX_PendThresh[8];	// 0x0c0-0x0dc CPDMA_INT Rx Threshold Pending Register Channel 0-7
    volatile UINT32    RX_FreeBuffer[8];	// 0x0e0-0x0fc CPDMA_INT Free Buffer Register Channel 0-7

#ifdef AM33X_BSP
	// CPSW STATS (Subarctic) 0x900 - 0x9FC
    volatile UINT32    RxGoodFrames;		// 0x000 CPSW_STATS  Total number of good frames received
    volatile UINT32    RxBcastFrames;		// 0x004 CPSW_STATS  Total number of good broadcast frames received
    volatile UINT32    RxMcastFrames;		// 0x008 CPSW_STATS  Total number of good multicast frames received
    volatile UINT32    RxPauseFrames;		// 0x00c CPSW_STATS  Pause Receive Frames Register
    volatile UINT32    RxCRCErrors;			// 0x010 CPSW_STATS  Total number of CRC errors frames received
    volatile UINT32    RxAlignCodeErrors;	// 0x014 CPSW_STATS  Total number of alignment/code frames received
    volatile UINT32    RxOversizedFrames;	// 0x018 CPSW_STATS  Total number of oversized frames received
    volatile UINT32    RxJabberFrames;		// 0x01c CPSW_STATS  Total number of jabber frames received
    volatile UINT32    RxUndersizedFrames;	// 0x020 CPSW_STATS  Total number of undersized frames received
    volatile UINT32    RxFragments;			// 0x024 CPSW_STATS  Receive Frame Fragments Register
    volatile UINT32    Rsvd4_AM33X[2];		// 0x028-
    volatile UINT32    RxOctets;			// 0x030 CPSW_STATS  Total number of RX bytes in good frames Register
    volatile UINT32    TxGoodFrames;		// 0x034 CPSW_STATS  Good Transmit Frames
    volatile UINT32    TxBcastFrames;		// 0x038 CPSW_STATS  Broadcast Transmit Frames Register
    volatile UINT32    TxMcastFrames;		// 0x03c CPSW_STATS  Multicast Transmit Frames Register
    volatile UINT32    TxPauseFrames;		// 0x040 CPSW_STATS  Pause Transmit Frames Register
    volatile UINT32    TxDeferredFrames;	// 0x044 CPSW_STATS  Deferred Transmit Frames Register
    volatile UINT32    TxCollisionFrames;	// 0x048 CPSW_STATS  Tx Collision Frames Register
    volatile UINT32    TxSinglecollFrames;	// 0x04c CPSW_STATS  Tx Single Collision Frames Register
    volatile UINT32    TxMulticollFrames;	// 0x050 CPSW_STATS  Tx Multiple Collision Frames Register
    volatile UINT32    TxExcessiveCollision;// 0x054 CPSW_STATS  Tx Excessive Collision Frames Register
    volatile UINT32    TxLateCollision;		// 0x058 CPSW_STATS  Tx Late Collision Frames Register
    volatile UINT32    TxUnderrun;			// 0x05c CPSW_STATS  Tx Underrun Error Register
    volatile UINT32    TxCarrierSenseError; // 0x060 CPSW_STATS  Tx Carrier Sense Errors Register
    volatile UINT32    TxOcets;				// 0x064 CPSW_STATS  Tx Octet Frames Register
    volatile UINT32    Frame64;				// 0x068 CPSW_STATS  Tx and Rx 64 Octet Frames Register
    volatile UINT32    Frame65t127;			// 0x06c CPSW_STATS  Tx and Rx 65 to 127 Octet Frames Register
    volatile UINT32    Frame128t255;		// 0x070 CPSW_STATS  Tx and Rx 128 to 255 Octet Frames Register
    volatile UINT32    Frame256t511;		// 0x074 CPSW_STATS  Tx and Rx 256 to 511 Octet Frames Register
    volatile UINT32    Frame512t1023;		// 0x078 CPSW_STATS  Tx and Rx 512 to 1023 Octet Frames Register
    volatile UINT32    Frame1024tup;		// 0x07c CPSW_STATS  Tx and Rx 1024 to 1518 Octet Frames Register
    volatile UINT32    NetOctets;			// 0x080 CPSW_STATS  Network Octet Frames Register
    volatile UINT32    RxSofOverruns;		// 0x084 CPSW_STATS  Rx FIFO or DMA Start of Frame Overruns Register
    volatile UINT32    RxMofOverruns;		// 0x088 CPSW_STATS  Rx FIFO or DMA Middle of Frame Overruns Register
    volatile UINT32    RxDmaOverruns;		// 0x08c CPSW_STATS  Rx DMA Start and Mid of Frame Overruns Reg
    volatile UINT32    Rsvd5_AM33X[28];		// 0x090-0x0fc
#endif	// AM33X_BSP

    // CPDMA STATE RAM
    // (centaurus) 0x200 -
    // (subarctic) 0xA00 -  
    volatile UINT32    Tx_HDP[8];			// 0x000-0x01c CPDMA_STATERAM TX Channel 0-7 Head Desc Pointer
    volatile UINT32    Rx_HDP[8];			// 0x020-0x02c CPDMA_STATERAM RX Channel 0-7 Head Desc Pointer
    volatile UINT32    Tx_CP[8];			// 0x040-0x05c CPDMA_STATERAM TX Channel 0-7 Completion Pointer
    volatile UINT32    Rx_CP[8];			// 0x060-0x07c CPDMA_STATERAM RX Channel 0-7 Completion Pointer
    volatile UINT32    Rsvd5[96];       	// 0x080-0x1FC

#ifdef AM387X_BSP
    // CPSW STATS (Centaurus) 0x400 - 0x4FC
    volatile UINT32    RxGoodFrames;		// 0x000 CPSW_STATS  Total number of good frames received
    volatile UINT32    RxBcastFrames;		// 0x004 CPSW_STATS  Total number of good broadcast frames received
    volatile UINT32    RxMcastFrames;		// 0x008 CPSW_STATS  Total number of good multicast frames received
    volatile UINT32    RxPauseFrames;		// 0x00c CPSW_STATS  Pause Receive Frames Register
    volatile UINT32    RxCRCErrors;			// 0x010 CPSW_STATS  Total number of CRC errors frames received
    volatile UINT32    RxAlignCodeErrors;	// 0x014 CPSW_STATS  Total number of alignment/code frames received
    volatile UINT32    RxOversizedFrames;	// 0x018 CPSW_STATS  Total number of oversized frames received
    volatile UINT32    RxJabberFrames;		// 0x01c CPSW_STATS  Total number of jabber frames received
    volatile UINT32    RxUndersizedFrames;	// 0x020 CPSW_STATS  Total number of undersized frames received
    volatile UINT32    RxFragments;			// 0x024 CPSW_STATS  Receive Frame Fragments Register
    volatile UINT32    Rsvd6[2];			// 0x028-0x42f
    volatile UINT32    RxOctets;			// 0x030 CPSW_STATS  Total number of RX bytes in good frames Register
    volatile UINT32    TxGoodFrames;		// 0x034 CPSW_STATS  Good Transmit Frames
    volatile UINT32    TxBcastFrames;		// 0x038 CPSW_STATS  Broadcast Transmit Frames Register
    volatile UINT32    TxMcastFrames;		// 0x03c CPSW_STATS  Multicast Transmit Frames Register
    volatile UINT32    TxPauseFrames;		// 0x040 CPSW_STATS  Pause Transmit Frames Register
    volatile UINT32    TxDeferredFrames;	// 0x044 CPSW_STATS  Deferred Transmit Frames Register
    volatile UINT32    TxCollisionFrames;	// 0x048 CPSW_STATS  Tx Collision Frames Register
    volatile UINT32    TxSinglecollFrames;	// 0x04c CPSW_STATS  Tx Single Collision Frames Register
    volatile UINT32    TxMulticollFrames;	// 0x050 CPSW_STATS  Tx Multiple Collision Frames Register
    volatile UINT32    TxExcessiveCollision;// 0x054 CPSW_STATS  Tx Excessive Collision Frames Register
    volatile UINT32    TxLateCollision;		// 0x058 CPSW_STATS  Tx Late Collision Frames Register
    volatile UINT32    TxUnderrun;			// 0x05c CPSW_STATS  Tx Underrun Error Register
    volatile UINT32    TxCarrierSenseError; // 0x060 CPSW_STATS  Tx Carrier Sense Errors Register
    volatile UINT32    TxOcets;				// 0x064 CPSW_STATS  Tx Octet Frames Register
    volatile UINT32    Frame64;				// 0x068 CPSW_STATS  Tx and Rx 64 Octet Frames Register
    volatile UINT32    Frame65t127;			// 0x06c CPSW_STATS  Tx and Rx 65 to 127 Octet Frames Register
    volatile UINT32    Frame128t255;		// 0x070 CPSW_STATS  Tx and Rx 128 to 255 Octet Frames Register
    volatile UINT32    Frame256t511;		// 0x074 CPSW_STATS  Tx and Rx 256 to 511 Octet Frames Register
    volatile UINT32    Frame512t1023;		// 0x078 CPSW_STATS  Tx and Rx 512 to 1023 Octet Frames Register
    volatile UINT32    Frame1024tup;		// 0x07c CPSW_STATS  Tx and Rx 1024 to 1518 Octet Frames Register
    volatile UINT32    NetOctets;			// 0x080 CPSW_STATS  Network Octet Frames Register
    volatile UINT32    RxSofOverruns;		// 0x084 CPSW_STATS  Rx FIFO or DMA Start of Frame Overruns Register
    volatile UINT32    RxMofOverruns;		// 0x088 CPSW_STATS  Rx FIFO or DMA Middle of Frame Overruns Register
    volatile UINT32    RxDmaOverruns;		// 0x08c CPSW_STATS  Rx DMA Start and Mid of Frame Overruns Reg
    volatile UINT32    Rsvd7[28];			// 0x090-0x0fc
#endif // AM387X_BSP

    // CPTS 
    // (centaurus) 0x500 -
    // (subarctic) 0xC00 -  
    volatile UINT32    CPTS_IdVer;			// 0x000
    volatile UINT32    CPTS_Control;		// 0x004
    volatile UINT32    CPTS_RFTCLK_Sel;		// 0x008
    volatile UINT32    CPTS_TS_Push;		// 0x00c
    volatile UINT32    CPTS_TS_Load_Val;	// 0x010
    volatile UINT32    CPTS_TS_Load_en;		// 0x014
    volatile UINT32    Rsvd8[2];			// 0x018-0x51c
    volatile UINT32    CPTS_Instat_raw;		// 0x020
    volatile UINT32    CPTS_Instat_masked;	// 0x024
    volatile UINT32    CPTS_Int_enable;		// 0x028
    volatile UINT32    Rsvd9;				// 0x02c
    volatile UINT32    CPTS_Event_Pop;		// 0x030
    volatile UINT32    CPTS_Event_low;		// 0x034
    volatile UINT32    CPTS_Event_High;		// 0x038
    volatile UINT32    Rsvd10[49];			// 0x03c-0x0fc

    // ALE 
    // (centaurus) 0x600 -
    // (subarctic) 0xD00 -  
    volatile UINT32    ALE_IdVer;			// 0x000 ALE  id/Version Register
    volatile UINT32    Rsvd11;				// 0x004
    volatile UINT32    ALE_Control;			// 0x008 ALE  Control Register
    volatile UINT32    Rsvd12;				// 0x00c
    volatile UINT32    ALE_PreScale;		// 0x010 ALE  PreScale Register
    volatile UINT32    Rsvd13;				// 0x014
    volatile UINT32    ALE_Unknown_VLAN;	// 0X018
    volatile UINT32    Rsvd14;				// 0x01c
    volatile UINT32    ALE_TblCtl;			// 0x020 ALE  Table Control Register
    volatile UINT32    Rsvd15[4];			// 0x024-0x030
    volatile UINT32    ALE_Tbl[3];			// 0x034-0x03c ALE  Table Word 2 Register -Word0
    volatile UINT32    ALE_PortCtl[6];		// 0x040-0x054 ALE Port N=0,...,5 Control Register

#ifdef AM387X_BSP
    volatile UINT32    Rsvd16[42];			// 0x058-0x0fc (ofs from centaurus ALE)
#endif

#ifdef AM33X_BSP
    volatile UINT32    Rsvd6_AM33X[10];		// 0x058-0x07c  (ofs from subarctic ALE)
#endif

    // SLIVER 
    // (centaurus) 0x700 -
    // (subarctic) 0xD80 -  
    CpgmacSl_Regs      SL_Regs[2];			// (Centaurus) 0x700-0x73c, 0x740-0x77c
											// (Subarctic) 0xD80-0xDBC, 0xDC0-0xDFC

//    Cpsw_SS_Regs       SS_Regs;             // 0x900 SubSystem Regs

}CPSW3G_REGS, *PCPSW3G_REGS;


//***********************************
//
//! \typedef EMAC_DESC
//! \brief EMAC descriptor overlay structure
//
//************************************
typedef struct __CPSW3G_DESC__ 
{
    volatile struct  __CPSW3G_DESC__*  pNext;      /// Pointer to next descriptor in chain 
    volatile UINT8*                  pBuffer;      /// Pointer to data buffer 
    volatile UINT32                  BufOffLen;    /// Buffer Offset(MSW) and Length(LSW) 
    volatile UINT32                  PktFlgLen;    /// Packet Flags(MSW) and Length(LSW) 
} CPSW3G_DESC,*PCPSW3G_DESC;

#define KITL_CHANNEL 0

/* CPPI bit positions */
#define CPSW3G_CPPI_SOP_BIT					0x80000000  /*(1 << 31)*/
#define CPSW3G_CPPI_EOP_BIT					0x40000000  /*(1 << 30)*/
#define CPSW3G_CPPI_OWNERSHIP_BIT			0x20000000  /*(1 << 29)*/
#define CPSW3G_CPPI_EOQ_BIT					0x10000000  /*(1 << 28)*/
#define CPSW3G_CPPI_TEARDOWN_COMPLETE_BIT	0x08000000  /*(1 << 27)*/
#define CPSW3G_CPPI_PASS_CRC_BIT			0x04000000  /*(1 << 26)*/
#define CPSW3G_CPPI_TO_PORT_EN_BIT			0x00100000  /*(1 << 20)*/
#define CPSW3G_CPPI_TO_PORT_BIT				0x00010000  /*(1 << 16)*/

#define CPSW3G_CPPI_BUFF_OFFSET_MASK		0xffff0000
#define CPSW3G_CPPI_BUFF_LEN_MASK			0x0000ffff
#define CPSW3G_CPPI_PKT_LEN_MASK			0x0000ffff

#define BD_SOP    CPSW3G_CPPI_SOP_BIT
#define BD_EOP    CPSW3G_CPPI_EOP_BIT
#define BD_OWNS   CPSW3G_CPPI_OWNERSHIP_BIT
#define BD_EOQ    CPSW3G_CPPI_EOQ_BIT

/* BD Macros. */
#define IS_SOP(bd)			((bd)->BuffFlagPktLen & CPSW3G_CPPI_SOP_BIT)
#define IS_EOP(bd)			((bd)->BuffFlagPktLen & CPSW3G_CPPI_EOP_BIT)
#define IS_OWNS(bd)			((bd)->BuffFlagPktLen & CPSW3G_CPPI_OWNERSHIP_BIT)
#define IS_TRDN(bd)			((bd)->BuffFlagPktLen & CPSW3G_CPPI_TEARDOWN_COMPLETE_BIT)
#define IS_EOQ(bd)			((bd)->BuffFlagPktLen & CPSW3G_CPPI_EOQ_BIT)
#define BD_BufPTR(bd)		((bd)->BuffPtr)
#define BD_BufOFFSET(bd)	(((bd)->BuffOffsetLength & CPSW3G_CPPI_BUFF_OFFSET_MASK) >> 16)
#define BD_BufLen(bd)		((bd)->BuffOffsetLength & CPSW3G_CPPI_BUFF_LEN_MASK)
#define BD_PktLen(bd)		((bd)->BuffFlagPktLen & CPSW3G_CPPI_PKT_LEN_MASK)

/* Macro definition for MDIO */
#define MDIO_GO			0x80000000
#define MDIO_READ		0x00000000
#define MDIO_WRITE		0x40000000
#define MDIO_ENABLE		0x40000000
#define MDIO_ACK		0x20000000
#define MII_STATUS_REG	1


//***********************************
//
//! \typedef CPSW3G_MDIO_REGS
//! \brief CPSW3G_MDIO_REGS Register Overlay Structure 
//
//************************************
typedef struct __MDIO_USERACCESS__
{
    volatile UINT32 access;
    volatile UINT32 physel;
}MDIO_USERACCESS;

typedef struct __CPSW3G_MDIO_REGS__
{
    volatile UINT32    Version;      		// 0x0 MDIO Version Register
    volatile UINT32    Control;       		// 0x4 MDIO Control Register
    volatile UINT32    Alive;      			// 0x8 MDIO PHY Alive Status Register
    volatile UINT32    Link;       			// 0xc MDIO PHY Link Status Register
    volatile UINT32    Linkintraw;     		// 0x10 MDIO Link Status Change Interrupt (Unmasked) Register
    volatile UINT32    Linkintmasked;     	// 0x14 MDIO Link Status Change Interrupt (Masked) Register
    volatile UINT32    Rsvd1[2];	       	// 0x18 -0x1c Reserved
    volatile UINT32    Userintraw;     		// 0x20 MDIO User Command Complete Interrupt (Unmasked) Register
    volatile UINT32    Userintmasked;       // 0x24 MDIO User Command Complete Interrupt (Masked) Register
    volatile UINT32    Userintmaskset;    	// 0x28 MDIO User Command Complete Interrupt Mask Set Register
    volatile UINT32    Userintmaskclear;  	// 0x2c MDIO User Command Complete Interrupt Mask Clear Register
    volatile UINT32    Rsvd2[20];      		// 0x30-0x7c Reserved
    MDIO_USERACCESS    Useraccess[2];
}CPSW3G_MDIO_REGS, *PCPSW3G_MDIO_REGS;


/* PHY related Macro definition */
#define MII_CONTROL_REG                  0
#define MII_STATUS_REG                   1

//#define CPSW_VTP_CTRL0_BASE              (0x0803D800)
//#define CPSW_VTP_CTRL1_BASE              (0x0803D900)

typedef struct __CPSW3G_VTP_CTRL_REGS__
{
    volatile UINT32 pid;
    volatile UINT32 mode;
    volatile UINT32 wdt;
    volatile UINT32 np;
    volatile UINT32 ctrl;
    volatile UINT32 start;
	
}CPSW3G_VTP_REGS, *PCPSW3G_VTP_REGS;


#define VTP_SINGLE_MODE			0x0
#define VTP_CONT_MODE			0x1
#define VTP_GZ_ALL				0xfff
#define VTP_SINGLE_MODE_START	(0x1<<1)

/*
 * CSL Macros
*/
#define CSL_FMK(PER_REG_FIELD, val) \
    (((val) << CSL_##PER_REG_FIELD##_SHIFT) & CSL_##PER_REG_FIELD##_MASK)

#define CSL_FEXT(reg, PER_REG_FIELD) \
    (((reg) & CSL_##PER_REG_FIELD##_MASK) >> CSL_##PER_REG_FIELD##_SHIFT)
    
#define CSL_FINS(reg, PER_REG_FIELD, val) \
    ((reg) = ((reg) & ~CSL_##PER_REG_FIELD##_MASK)\
    | CSL_FMK(PER_REG_FIELD, val))

/* Port_VLAN register fields */
#define CPSW3G_PORTVLAN_PRI_SHIFT               13
#define CPSW3G_PORTVLAN_PRI_MASK                (0x7 << 13)
#define CPSW3G_PORTVLAN_CFI_SHIFT               12
#define CPSW3G_PORTVLAN_CFI_MASK                (0x1 << 12)
#define CPSW3G_PORTVLAN_VID_SHIFT               0
#define CPSW3G_PORTVLAN_VID_MASK                (0xfff << 0)

/* ALE Port Control register fields */
#define CPSW3G_ALEPORTCONTROL_BCASTLIMIT_SHIFT  24
#define CPSW3G_ALEPORTCONTROL_BCASTLIMIT_MASK   (0xff << 24)
#define CPSW3G_ALEPORTCONTROL_MCASTLIMIT_SHIFT  16
#define CPSW3G_ALEPORTCONTROL_MCASTLIMIT_MASK   (0xff << 16)
#define CPSW3G_ALEPORTCONTROL_NOLEARN_SHIFT     4
#define CPSW3G_ALEPORTCONTROL_NOLEARN_MASK      (0x1  << 4)
#define CPSW3G_ALEPORTCONTROL_VIDINGRESSCHECK   (0x1  << 3)
#define CPSW3G_ALEPORTCONTROL_DROPUNTAGGED      (0x1  << 2)
#define CPSW3G_ALEPORTCONTROL_PORTSTATE_SHIFT   0
#define CPSW3G_ALEPORTCONTROL_PORTSTATE_MASK    (0x3  << 0)

/* ALE Control register fields */
#define CPSW3G_ALECONTROL_ENABLEALE        (0x1 << 31)
#define CPSW3G_ALECONTROL_CLEARTABLE       (0x1 << 30)
#define CPSW3G_ALECONTROL_AGEOUTNOW        (0x1 << 29)
#define CPSW3G_ALECONTROL_LEARNNOVID       (0x1 << 7)
#define CPSW3G_ALECONTROL_ENVID0MODE       (0x1 << 6)
#define CPSW3G_ALECONTROL_ENABLEOUIDENY    (0x1 << 5)
#define CPSW3G_ALECONTROL_ALEBYPASS        (0x1 << 4)
#define CPSW3G_ALECONTROL_RATELIMITTX      (0x1 << 3)
#define CPSW3G_ALECONTROL_ALEVLANAWARE     (0x1 << 2)
#define CPSW3G_ALECONTROL_ENABLEAUTHMODE   (0x1 << 1)
#define CPSW3G_ALECONTROL_ENABLERATELIMIT  (0x1 << 0)

/* ALE Unknown VLAN Register Fields */
#define CPSW3G_ALEUNKNOWNVLAN_FORCEUNTAGGEDEGRESS   (0x7 << 24)
#define CPSW3G_ALEUNKNOWNVLAN_REGMCASTFLOODMASK     (0x7 << 16)
#define CPSW3G_ALEUNKNOWNVLAN_MCASTFLOODMASK        (0x7 << 8 )
#define CPSW3G_ALEUNKNOWNVLAN_VLANMEMBERLIST        (0x7 << 0)

/* ALE PRESCALE Register Fields */
#define CPSW3G_ALEPRESCALE_MASK                     0xFFFFF

/* DMA Control register bit fields */
#define CSL_CPSW3G_DMACONTROL_RXCEF_SHIFT           4
#define CSL_CPSW3G_DMACONTROL_RXCEF_MASK            (0x1 << 4)
#define CSL_CPSW3G_DMACONTROL_CMDIDLE_SHIFT         3
#define CSL_CPSW3G_DMACONTROL_CMDIDLE_MASK          (0x1 << 3)
#define CSL_CPSW3G_DMACONTROL_RXOFFLENBLOCK_SHIFT   2
#define CSL_CPSW3G_DMACONTROL_RXOFFLENBLOCK_MASK    (0x1 << 2)
#define CSL_CPSW3G_DMACONTROL_RXOWNERSHIP_SHIFT     1
#define CSL_CPSW3G_DMACONTROL_RXOWNERSHIP_MASK      (0x1 << 1)
#define CSL_CPSW3G_DMACONTROL_TXPTYPE_SHIFT         0
#define CSL_CPSW3G_DMACONTROL_TXPTYPE_MASK          (0x1 << 0)

/* CPDMA Rx Ch Map register bit fields */
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH3_SHIFT         28
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH3_MASK          (0x7 << 28)
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH2_SHIFT         24
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH2_MASK          (0x7 << 24)
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH1_SHIFT         20
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH1_MASK          (0x7 << 20)
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH0_SHIFT         16
#define CSL_CPSW3G_CPDMARXCHMAP_SUCH0_MASK          (0x7 << 16)
#define CSL_CPSW3G_CPDMARXCHMAP_CH3_SHIFT           12
#define CSL_CPSW3G_CPDMARXCHMAP_CH3_MASK            (0x7 << 12)
#define CSL_CPSW3G_CPDMARXCHMAP_CH2_SHIFT           8
#define CSL_CPSW3G_CPDMARXCHMAP_CH2_MASK            (0x7 << 8)
#define CSL_CPSW3G_CPDMARXCHMAP_CH1_SHIFT           4
#define CSL_CPSW3G_CPDMARXCHMAP_CH1_MASK            (0x7 << 4)
#define CSL_CPSW3G_CPDMARXCHMAP_CH0_SHIFT           0
#define CSL_CPSW3G_CPDMARXCHMAP_CH0_MASK            (0x7 << 0)


/* Pri_Map register bit fields */
#define CSL_CPSW3G_PRIMAP_PRI7_SHIFT         28
#define CSL_CPSW3G_PRIMAP_PRI7_MASK          (0x7 << 28)
#define CSL_CPSW3G_PRIMAP_PRI6_SHIFT         24
#define CSL_CPSW3G_PRIMAP_PRI6_MASK          (0x7 << 24)
#define CSL_CPSW3G_PRIMAP_PRI5_SHIFT         20
#define CSL_CPSW3G_PRIMAP_PRI5_MASK          (0x7 << 20)
#define CSL_CPSW3G_PRIMAP_PRI4_SHIFT         16
#define CSL_CPSW3G_PRIMAP_PRI4_MASK          (0x7 << 16)
#define CSL_CPSW3G_PRIMAP_PRI3_SHIFT         12
#define CSL_CPSW3G_PRIMAP_PRI3_MASK          (0x7 << 12)
#define CSL_CPSW3G_PRIMAP_PRI2_SHIFT         8
#define CSL_CPSW3G_PRIMAP_PRI2_MASK          (0x7 << 8)
#define CSL_CPSW3G_PRIMAP_PRI1_SHIFT         4
#define CSL_CPSW3G_PRIMAP_PRI1_MASK          (0x7 << 4)
#define CSL_CPSW3G_PRIMAP_PRI0_SHIFT         0
#define CSL_CPSW3G_PRIMAP_PRI0_MASK          (0x7 << 0)

/* CPSW Control register bit fields */
#define CPSW3G_CPSWCONTROL_P2PASSPRITAGGED_SHIFT  12
#define CPSW3G_CPSWCONTROL_P2PASSPRITAGGED_MASK   (0x1 << 12)
#define CPSW3G_CPSWCONTROL_P1PASSPRITAGGED_SHIFT  11
#define CPSW3G_CPSWCONTROL_P1PASSPRITAGGED_MASK   (0x1 << 11)
#define CPSW3G_CPSWCONTROL_P0PASSPRITAGGED_SHIFT  10
#define CPSW3G_CPSWCONTROL_P0PASSPRITAGGED_MASK   (0x1 << 10)
#define CPSW3G_CPSWCONTROL_VLANAWARE_SHIFT        9
#define CPSW3G_CPSWCONTROL_VLANAWARE_MASK         (0x1 << 9)
#define CPSW3G_CPSWCONTROL_RXVLANENCAP_SHIFT      8
#define CPSW3G_CPSWCONTROL_RXVLANENCAP_MASK       (0x1 << 8)
#define CPSW3G_CPSWCONTROL_P2P2RXFLOWEN_SHIFT     7
#define CPSW3G_CPSWCONTROL_P2P2RXFLOWEN_MASK      (0x1 << 7)
#define CPSW3G_CPSWCONTROL_P2P1TXFLOWEN_SHIFT     6
#define CPSW3G_CPSWCONTROL_P2P1TXFLOWEN_MASK      (0x1 << 6)
#define CPSW3G_CPSWCONTROL_P2P0TXFLOWEN_SHIFT     5
#define CPSW3G_CPSWCONTROL_P2P0TXFLOWEN_MASK      (0x1 << 5)
#define CPSW3G_CPSWCONTROL_P1P2TXFLOWEN_SHIFT     4
#define CPSW3G_CPSWCONTROL_P1P2TXFLOWEN_MASK      (0x1 << 4)
#define CPSW3G_CPSWCONTROL_P1P0TXFLOWEN_SHIFT     3
#define CPSW3G_CPSWCONTROL_P1P0TXFLOWEN_MASK      (0x1 << 3)
#define CPSW3G_CPSWCONTROL_P0P2TXFLOWEN_SHIFT     2
#define CPSW3G_CPSWCONTROL_P0P2TXFLOWEN_MASK      (0x1 << 2)
#define CPSW3G_CPSWCONTROL_P0P1TXFLOWEN_SHIFT     1
#define CPSW3G_CPSWCONTROL_P0P1TXFLOWEN_MASK      (0x1 << 1)
#define CPSW3G_CPSWCONTROL_FIFOLOOPBACK_SHIFT     0
#define CPSW3G_CPSWCONTROL_FIFOLOOPBACK_MASK      (0x1 << 0)

/* Flow Thresh register fields */
#define CSL_CPSW3G_FLOWTHRESH_FIELD2_SHIFT        16
#define CSL_CPSW3G_FLOWTHRESH_FIELD2_MASK         (0xf << 16)
#define CSL_CPSW3G_FLOWTHRESH_FIELD1_SHIFT        8
#define CSL_CPSW3G_FLOWTHRESH_FIELD1_MASK         (0x1f << 8)
#define CSL_CPSW3G_FLOWTHRESH_FIELD0_SHIFT        0
#define CSL_CPSW3G_FLOWTHRESH_FIELD0_MASK         (0x1f << 0)

/* PTYPE Register fields */
#define CSL_CPSW3G_PTYPE_P2PTYPEESC_SHIFT         10
#define CSL_CPSW3G_PTYPE_P2PTYPEESC_MASK          (0x1 << 10)
#define CSL_CPSW3G_PTYPE_P1PTYPEESC_SHIFT         9
#define CSL_CPSW3G_PTYPE_P1PTYPEESC_MASK          (0x1 << 9)
#define CSL_CPSW3G_PTYPE_P0PTYPEESC_SHIFT         8
#define CSL_CPSW3G_PTYPE_P0PTYPEESC_MASK          (0x1 << 8)
#define CSL_CPSW3G_PTYPE_ESCPRIIDVAL_SHIFT        0
#define CSL_CPSW3G_PTYPE_ESCPRIIDVAL_MASK         (0x1f << 0)

/* CPSW Stat Port En register bit fields */
#define CSL_CPSW3G_CPSWSTATPORTEN_P2STATEN_SHIFT  2
#define CSL_CPSW3G_CPSWSTATPORTEN_P2STATEN_MASK   (0x1 << 2)
#define CSL_CPSW3G_CPSWSTATPORTEN_P1STATEN_SHIFT  1
#define CSL_CPSW3G_CPSWSTATPORTEN_P1STATEN_MASK   (0x1 << 1)
#define CSL_CPSW3G_CPSWSTATPORTEN_P0STATEN_SHIFT  0
#define CSL_CPSW3G_CPSWSTATPORTEN_P0STATEN_MASK   (0x1 << 0)

/* Mac Control register bit fields */
#define CPMAC_MACCONTROL_RXCMFEN_SHIFT          24
#define CPMAC_MACCONTROL_RXCMFEN_MASK           (0x1 << 24)
#define CPMAC_MACCONTROL_RXCSFEN_SHIFT          23
#define CPMAC_MACCONTROL_RXCSFEN_MASK           (0x1 << 23)
#define CPMAC_MACCONTROL_RXCEFEN_SHIFT          22
#define CPMAC_MACCONTROL_RXCEFEN_MASK           (0x1 << 22)
#define CPMAC_MACCONTROL_EXTEN_SHIFT            18
#define CPMAC_MACCONTROL_EXTEN_MASK             (0x1 << 18)
#define CPMAC_MACCONTROL_GIGFORCE_SHIFT         17
#define CPMAC_MACCONTROL_GIGFORCE_MASK          (0x1 << 17)
#define CPMAC_MACCONTROL_IFCTLB_SHIFT           16
#define CPMAC_MACCONTROL_IFCTLB_MASK            (0x1 << 16)
#define CPMAC_MACCONTROL_IFCTLA_SHIFT           15
#define CPMAC_MACCONTROL_IFCTLA_MASK            (0x1 << 15)
#define CPMAC_MACCONTROL_CMDIDLE_SHIFT          11
#define CPMAC_MACCONTROL_CMDIDLE_MASK           (0x1 << 11)
#define CPMAC_MACCONTROL_TXSHORTGAPEN_SHIFT     10
#define CPMAC_MACCONTROL_TXSHORTGAPEN_MASK      (0x1 << 10)
#define CPMAC_MACCONTROL_GIGABITEN_SHIFT        7
#define CPMAC_MACCONTROL_GIGABITEN_MASK         (0x1 << 7)
#define CPMAC_MACCONTROL_TXPACEEN_SHIFT         6
#define CPMAC_MACCONTROL_TXPACEEN_MASK          (0x1 << 6)
#define CPMAC_MACCONTROL_MIIEN_SHIFT            5
#define CPMAC_MACCONTROL_MIIEN_MASK             (0x1 << 5)
#define CPMAC_MACCONTROL_TXFLOWEN_SHIFT         4
#define CPMAC_MACCONTROL_TXFLOWEN_MASK          (0x1 << 4)
#define CPMAC_MACCONTROL_RXFLOWEN_SHIFT         3
#define CPMAC_MACCONTROL_RXFLOWEN_MASK          (0x1 << 3)
#define CPMAC_MACCONTROL_LOOPBKEN_SHIFT         1
#define CPMAC_MACCONTROL_LOOPBKEN_MASK          (0x1 << 1)
#define CPMAC_MACCONTROL_FULLDUPLEXEN_SHIFT     0
#define CPMAC_MACCONTROL_FULLDUPLEXEN_MASK      (0x1)

/* Mac Status register bit fields */
#define CPMAC_MACSTATUS_IDLE_MASK               (0x1<<31)

/* DmaStatus register */
#define CSL_CPDMA_DMASTATUS_IDLE_MASK               (0x1<<31)
#define CSL_CPDMA_DMASTATUS_IDLE_SHIFT              31
#define CSL_CPDMA_DMASTATUS_TXERRCODE_MASK          (0xf << 20)
#define CSL_CPDMA_DMASTATUS_TXERRCODE_SHIFT         20
#define CSL_CPDMA_DMASTATUS_TXERRCH_MASK            (0x7 << 16)
#define CSL_CPDMA_DMASTATUS_TXERRCH_SHIFT           16
#define CSL_CPDMA_DMASTATUS_RXERRCODE_MASK          (0xf << 12)
#define CSL_CPDMA_DMASTATUS_RXERRCODE_SHIFT         12
#define CSL_CPDMA_DMASTATUS_RXERRCH_MASK            (0x7 << 8)
#define CSL_CPDMA_DMASTATUS_RXERRCH_SHIFT           8

/* CPDMA_IN_VECTOR (0x190) register bit fields */
#define CSL_CPDMA_IN_VECTOR_SPF_INT_MASK            (1 << 22)
#define CSL_CPDMA_IN_VECTOR_SPF_INT_SHIFT           (22)
#define CSL_CPDMA_IN_VECTOR_STATUS_INT_MASK         (1 << 19)
#define CSL_CPDMA_IN_VECTOR_STATUS_INT_SHIFT        19
#define CSL_CPDMA_IN_VECTOR_HOST_INT_MASK           (1 << 18)
#define CSL_CPDMA_IN_VECTOR_HOST_INT_SHIFT          18
#define CSL_CPDMA_IN_VECTOR_RX_INT_OR_MASK          (1 << 17)
#define CSL_CPDMA_IN_VECTOR_RX_INT_OR_SHIFT         17
#define CSL_CPDMA_IN_VECTOR_TX_INT_OR_MASK          (1 << 16)
#define CSL_CPDMA_IN_VECTOR_TX_INT_OR_SHIFT         16
#define CSL_CPDMA_IN_VECTOR_RX_INT_VEC_MASK         (0x7 << 8)
#define CSL_CPDMA_IN_VECTOR_RX_INT_VEC_SHIFT        8
#define CSL_CPDMA_IN_VECTOR_TX_INT_VEC_MASK         (0x7)
#define CSL_CPDMA_IN_VECTOR_TX_INT_VEC_SHIFT        0

/* Miscellaneous register related defines */
#define CPSW3G_SOFT_RESET_BIT                   (0x1 << 0)
#define CPSW3G_TX_CONTROL_TX_ENABLE_VAL         (0x1 << 0)
#define CPSW3G_RX_CONTROL_RX_ENABLE_VAL         (0x1 << 0)
#define CPSW3G_HOST_ERR_INTMASK_VAL             (0x1 << 1)
#define CPSW3G_STAT_INTMASK_VAL                 (0x1 << 0)

#define CPSW3G_DEFAULT_PORTSTATE                3

//flow enable
#define CPSW3G_DEFAULT_P2_P2RXFLOWEN            1
#define CPSW3G_DEFAULT_P2_P1TXFLOWEN            1
#define CPSW3G_DEFAULT_P2_P0TXFLOWEN            1
#define CPSW3G_DEFAULT_P1_P2TXFLOWEN            1
#define CPSW3G_DEFAULT_P1_P0TXFLOWEN            0
#define CPSW3G_DEFAULT_P0_P2TXFLOWEN            1
#define CPSW3G_DEFAULT_P0_P1TXFLOWEN            0

#define CPSW3G_DEFAULT_P2STATEN                 1
#define CPSW3G_DEFAULT_P1STATEN                 1
#define CPSW3G_DEFAULT_P0STATEN                 1


/* CPSW3G configuration definition */
#define CPSW3G_NUM_STATS_REGS		36
#define CPSW3G_MAX_MCAST_ENTRIES	64
#define CPSW3G_TEARDOWN_VALUE		0xFFFFFFFC
#define CPDMA_MAX_CHANNELS			8           /**< Maximum num of channels supported by the DMA */
#define CPDMA_MAX_DIR				2           /**< Number of DMA data directions (tx and rx) */
#define CPSW3G_NUM_PORTS			3           /**< CPSW3G Port Count */
#define CPSW3G_NUM_MAC_PORTS		2           /**< MAC Port Count */


/* Ethernet Packet definiation */
#define CPSW3G_MAX_DATA_SIZE			1500
#define CPSW3G_MIN_DATA_SIZE			46
#define CPSW3G_ETHERNET_HEADER_SIZE		14
#define CPSW3G_MIN_ETHERNET_PKT_SIZE	(CPSW3G_MIN_DATA_SIZE + CPSW3G_ETHERNET_HEADER_SIZE)
#define CPSW3G_MAX_ETHERNET_PKT_SIZE	(CPSW3G_MAX_DATA_SIZE + CPSW3G_ETHERNET_HEADER_SIZE)
#define CPSW3G_PKT_ALIGN				32       
#define CPSW3G_MAX_PKT_BUFFER_SIZE		((CPSW3G_MAX_ETHERNET_PKT_SIZE + (CPSW3G_PKT_ALIGN -1)) & ~(CPSW3G_PKT_ALIGN -1))



typedef struct __MAC_ID_REGS__ {
	volatile UINT32	MAC_ID_LO;
	volatile UINT32	MAC_ID_HI;
}  MAC_ID_REGS, *PMAC_ID_REGS;

#define PORT_OWN(bd)		((bd)->PktFlgLen & CPSW3G_CPPI_OWNERSHIP_BIT)
#define HOST_OWN(bd)		(!(PORT_OWN((bd))))


/*********************************************************************************
 *    SOC Specifics
 *********************************************************************************/

#ifdef AM387X_BSP
/* Centaurus */
#error "Compiling the wrong bsp: AM387X_BSP!!!"
#define CPSW_3G_BASE			0x4A100000
#define CPMDIO_BASE				0x4A100800
#define CPSW_SS_BASE			0x4A100900
#define CPSW3G_BD_ALIGN			16 
#define AM387X_MAC_ID0_OFFSET	0x0230 //offset from AM387X_DEVICE_CONF_REGS_PA  0x48140400
#define AM387X_MAC_ID1_OFFSET	0x0238

#define CPSW3G_HOST_PORT		0           /**< DMA Port number */
#define CPGMAC_PORT_START		1
#define CPGMAC_PORT_END			2

/* LSI PHY configuration */
#define EXTERNAL_PHY0_ADDR		1
#define EXTERNAL_PHY1_ADDR		0

#define PORT2IDX(n)				((n)-1)	// Slave port number to array index conversion 
#define OTHER_MAC_PORT(n)		((CPGMAC_PORT_START + CPGMAC_PORT_END) - (n))
#endif // AM387X_BSP

#ifdef AM33X_BSP
/* Subarctic */
#define CPSW_3G_BASE			0x4A100000  // AM33X_EMACSW_REGS_PA
#define CPMDIO_BASE				0x4A101000
#define CPSW_SS_BASE			0x4A101200
#define CPSW3G_BD_ALIGN			16 
#define AM33X_MAC_ID0_OFFSET	0x0030  //offset from AM33X_DEVICE_CONF_REGS_PA  0x44E10600
#define AM33X_MAC_ID1_OFFSET	0x0038

#define CPSW3G_HOST_PORT		0           /**< DMA Port number */
#define CPGMAC_PORT_START		1
#define CPGMAC_PORT_END			2

/* PHY configuration */
#define EXTERNAL_PHY0_ADDR		0
#define EXTERNAL_PHY1_ADDR		1

#define PORT2IDX(n)				((n)-1) // Slave port number to array index conversion 
#define OTHER_MAC_PORT(n)		((CPGMAC_PORT_START + CPGMAC_PORT_END) - (n))
#endif  // AM33X_BSP

#define P2I(n)				PORT2IDX((n))

#endif /* #ifndef __AM33XCPSW3GREGS_H_INCLUDED__ */
