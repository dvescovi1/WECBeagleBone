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
//  Header: omp_cpgmac_regs.h
//

#ifndef __OMAP_CPGMAC_REGS_H
#define  __OMAP_CPGMAC_REGS_H

#define BIT(x) (1<<(x))

typedef struct {
    REG32  TXREVID;            // 0x000 Transmit Revision ID Register
    REG32  TXCONTROL;          // 0x004 Transmit Control Register
    REG32  TXTEARDOWN;         // 0x008 Transmit Teardown Register
    REG32  RSVD_00C;           // 0x00C Reserved
    REG32  RXREVID;            // 0x010 Receive Revision ID Register
    REG32  RXCONTROL;          // 0x014 Receive Control Register
    REG32  RXTEARDOWN;         // 0x018 Receive Teardown Register
    REG32  RSVD_01C_07C[25];   // 0x01C-0x07C Reserved
    REG32  TXINTSTATRAW;       // 0x080 Transmit Interrupt Status (Unmasked) Register
    REG32  TXINTSTATMASKED;    // 0x084 Transmit Interrupt Status (Masked) Register
    REG32  TXINTMASKSET;       // 0x088 Transmit Interrupt Mask Set Register
    REG32  TXINTMASKCLEAR;     // 0x08C Transmit Interrupt Clear Register
    REG32  MACINVECTOR;        // 0x090 MAC Input Vector Register
    REG32  MACEOIVECTOR;       // 0x094 MAC End Of Interrupt Vector Register
    REG32  RSVD_098_9C[2];     // 0x098 - 0x09C Reserved
    REG32  RXINTSTATRAW;       // 0x0A0 Receive Interrupt Status (Unmasked) Register
    REG32  RXINTSTATMASKED;    // 0x0A4 Receive Interrupt Status (Masked) Register
    REG32  RXINTMASKSET;       // 0x0A8 Receive Interrupt Mask Set Register
    REG32  RXINTMASKCLEAR;     // 0X0AC Receive Interrupt Mask Clear Register
    REG32  MACINTSTATRAW;      // 0x0B0 MAC Interrupt Status (Unmasked) Register
    REG32  MACINTSTATMASKED;   // 0x0B4 MAC Interrupt Status (Masked) Register
    REG32  MACINTMASKSET;      // 0x0B8 MAC Interrupt Mask Set Register
    REG32  MACINTMASKCLEAR;    // 0x0BC MAC Interrupt Mask Clear Register
    REG32  RSVD_0C0_0FC[16];   // 0x0C0 - 0x0FC Reserved
    REG32  RXMBPENABLE;        // 0x100 Receive Multicast/Broadcast/Promiscuous Channel Enable Register
    REG32  RXUNICASTSET;       // 0x104 Receive Unicast Enable Set Register
    REG32  RXUNICASTCLEAR;     // 0x108 Receive Unicast Clear Register
    REG32  RXMAXLEN;           // 0x10C Receive Maximum Length Register
    REG32  RXBUFFEROFFSET;     // 0x110 Receive Buffer Offset Register
    REG32  RXFILTERLOWTHRESH;  // 0x114 Receive Filter Low Priority Frame Threshold Register
    REG32  RSVD_118_11C[2];    // 0x118 - 0x11C Reserved
    REG32  RX0FLOWTHRESH;      // 0x120 Receive Channel 0 Flow Control Threshold Register
    REG32  RX1FLOWTHRESH;      // 0x124 Receive Channel 1 Flow Control Threshold Register
    REG32  RX2FLOWTHRESH;      // 0x128 Receive Channel 2 Flow Control Threshold Register
    REG32  RX3FLOWTHRESH;      // 0x12C Receive Channel 3 Flow Control Threshold Register
    REG32  RX4FLOWTHRESH;      // 0x130 Receive Channel 4 Flow Control Threshold Register
    REG32  RX5FLOWTHRESH;      // 0x134 Receive Channel 5 Flow Control Threshold Register
    REG32  RX6FLOWTHRESH;      // 0x138 Receive Channel 6 Flow Control Threshold Register
    REG32  RX7FLOWTHRESH;      // 0x13C Receive Channel 7 Flow Control Threshold Register
    REG32  RX0FREEBUFFER;      // 0x140 Receive Channel 0 Free Buffer Count Register
    REG32  RX1FREEBUFFER;      // 0x144 Receive Channel 1 Free Buffer Count Register
    REG32  RX2FREEBUFFER;      // 0x148 Receive Channel 2 Free Buffer Count Register
    REG32  RX3FREEBUFFER;      // 0x14C Receive Channel 3 Free Buffer Count Register
    REG32  RX4FREEBUFFER;      // 0x150 Receive Channel 4 Free Buffer Count Register
    REG32  RX5FREEBUFFER;      // 0x154 Receive Channel 5 Free Buffer Count Register
    REG32  RX6FREEBUFFER;      // 0x158 Receive Channel 6 Free Buffer Count Register
    REG32  RX7FREEBUFFER;      // 0x15C Receive Channel 7 Free Buffer Count Register
    REG32  MACCONTROL;         // 0x160 MAC Control Register
    REG32  MACSTATUS;          // 0x164 MAC Status Register
    REG32  EMCONTROL;          // 0x168 Emulation Control Register
    REG32  FIFOCONTROL;        // 0x16C FIFO Control Register
    REG32  MACCONFIG;          // 0x170 MAC Configuration Register
    REG32  SOFTRESET;          // 0x174 Soft Reset Register
    REG32  RSVD_178_1CC[22];   // 0x178 - 0x1CC Reserved
    REG32  MACSRCADDRLO;       // 0x1D0 MAC Source Address Low Bytes Register
    REG32  MACSRCADDRHI;       // 0x1D4 MAC Source Address High Bytes Register
    REG32  MACHASH1;           // 0x1D8 MAC Hash Address Register 1
    REG32  MACHASH2;           // 0x1DC MAC Hash Address Register 2
    REG32  BOFFTEST;           // 0x1E0 Back Off Test Register
    REG32  TPACETEST;          // 0x1E4 Transmit Pacing Algorithm Test Register
    REG32  RXPAUSE;            // 0x1E8 Receive Pause Timer Register
    REG32  TXPAUSE;            // 0x1EC Transmit Pause Timer Register
    REG32  RSVD_1F0_1FC[4];  // 0x1F0 - 0x1FC Reserved
    REG32  RXGOODFRAMES;       // 0x200 Good Receive Frames Register
    REG32  RXBCASTFRAMES;      // 0x204 Broadcast Receive Frames Register
    REG32  RXMCASTFRAMES;      // 0x208 Multicast Receive Frames Register
    REG32  RXPAUSEFRAMES;      // 0x20C Pause Receive Frames Register
    REG32  RXCRCERRORS;        // 0x210 Receive CRC Errors Register
    REG32  RXALIGNCODEERRORS;  // 0x214 Receive Alignment/Code Errors Register
    REG32  RXOVERSIZED;        // 0x218 Receive Oversized Frames Register
    REG32  RXJABBER;           // 0x21C Receive Jabber Frames Register
    REG32  RXUNDERSIZED;       // 0x220 Receive Undersized Frames Register
    REG32  RXFRAGMENTS;        // 0x224 Receive Frame Fragments Register
    REG32  RXFILTERED;         // 0x228 Filtered Receive Frames Register
    REG32  RXQOSFILTERED;      // 0X22C Receive QOS Filtered Frames Register
    REG32  RXOCTETS;           // 0x230 Receive Octet Frames Register
    REG32  TXGOODFRAMES;       // 0x234 Good Transmit Frames Register
    REG32  TXBCASTFRAMES;      // 0x238 Broadcast Transmit Frames Register
    REG32  TXMCASTFRAMES;      // 0x23C Multicast Transmit Frames Register
    REG32  TXPAUSEFRAMES;      // 0x240 Pause Transmit Frames Register
    REG32  TXDEFERRED;         // 0x244 Deferred Transmit Frames Register
    REG32  TXCOLLISION;        // 0x248 Transmit Collision Frames Register
    REG32  TXSINGLECOLL;       // 0x24C Transmit Single Collision Frames Register
    REG32  TXMULTICOLL;        // 0x250 Transmit Multiple Collision Frames Register
    REG32  TXEXCESSIVECOLL;    // 0x254 Transmit Excessive Collision Frames Register
    REG32  TXLATECOLL;         // 0x258 Transmit Late Collision Frames Register
    REG32  TXUNDERRUN;         // 0x25C Transmit Underrun Error Register
    REG32  TXCARRIERSENSE;     // 0x260 Transmit Carrier Sense Errors Register
    REG32  TXOCTETS;           // 0x264 Transmit Octet Frames Register
    REG32  FRAME64;            // 0x268 Transmit and Receive 64 Octet Frames Register
    REG32  FRAME65T127;        // 0x26C Transmit and Receive 65 to 127 Octet Frames Register
    REG32  FRAME128T255;       // 0x270 Transmit and Receive 128 to 255 Octet Frames Register
    REG32  FRAME256T511;       // 0x274 Transmit and Receive 256 to 511 Octet Frames Register
    REG32  FRAME512T1023;      // 0x278 Transmit and Receive 512 to 1023 Octet Frames Register
    REG32  FRAME1024TUP;       // 0x27C Transmit and Receive 1024 to RXMAXLEN Octet Frames Register
    REG32  NETOCTETS;          // 0x280 Network Octet Frames Register
    REG32  RXSOFOVERRUNS;      // 0x284 Receive FIFO or DMA Start of Frame Overruns Register
    REG32  RXMOFOVERRUNS;      // 0x288 Receive FIFO or DMA Middle of Frame Overruns Register
    REG32  RXDMAOVERRUNS;      // 0X28C Receive DMA Overruns Register
    REG32  RSVD_0290_4FC[156]; // 0x290 - 0x4FC Reserved
    REG32  MACADDRLO;          // 0x500 MAC Address Low Bytes Register, Used in Receive Address Matching
    REG32  MACADDRHI;          // 0x504 MAC Address High Bytes Register, Used in Receive Address Matching
    REG32  MACINDEX;           // 0x508 MAC Index Register
    REG32  RSVD_50C_5FC[61];   // 0x50C - 0x5FC Reserved
    REG32  TX0HDP;             // 0x600 Transmit Channel 0 DMA Head Descriptor Pointer Register
    REG32  TX1HDP;             // 0x604 Transmit Channel 1 DMA Head Descriptor Pointer Register
    REG32  TX2HDP;             // 0x608 Transmit Channel 2 DMA Head Descriptor Pointer Register
    REG32  TX3HDP;             // 0x60C Transmit Channel 3 DMA Head Descriptor Pointer Register
    REG32  TX4HDP;             // 0x610 Transmit Channel 4 DMA Head Descriptor Pointer Register
    REG32  TX5HDP;             // 0x614 Transmit Channel 5 DMA Head Descriptor Pointer Register
    REG32  TX6HDP;             // 0x618 Transmit Channel 6 DMA Head Descriptor Pointer Register
    REG32  TX7HDP;             // 0x61C Transmit Channel 7 DMA Head Descriptor Pointer Register
    REG32  RX0HDP;             // 0x620 Receive Channel 0 DMA Head Descriptor Pointer Register
    REG32  RX1HDP;             // 0x624 Receive Channel 1 DMA Head Descriptor Pointer Register
    REG32  RX2HDP;             // 0x628 Receive Channel 2 DMA Head Descriptor Pointer Register
    REG32  RX3HDP;             // 0x62C Receive Channel 3 DMA Head Descriptor Pointer Register
    REG32  RX4HDP;             // 0x630 Receive Channel 4 DMA Head Descriptor Pointer Register
    REG32  RX5HDP;             // 0x634 Receive Channel 5 DMA Head Descriptor Pointer Register
    REG32  RX6HDP;             // 0x638 Receive Channel 6 DMA Head Descriptor Pointer Register
    REG32  RX7HDP;             // 0x63C Receive Channel 7 DMA Head Descriptor Pointer Register
    REG32  TX0CP;              // 0x640 Transmit Channel 0 Completion Pointer Register
    REG32  TX1CP;              // 0x644 Transmit Channel 1 Completion Pointer Register
    REG32  TX2CP;              // 0x648 Transmit Channel 2 Completion Pointer Register
    REG32  TX3CP;              // 0x64C Transmit Channel 3 Completion Pointer Register
    REG32  TX4CP;              // 0x650 Transmit Channel 4 Completion Pointer Register
    REG32  TX5CP;              // 0x654 Transmit Channel 5 Completion Pointer Register
    REG32  TX6CP;              // 0x658 Transmit Channel 6 Completion Pointer Register
    REG32  TX7CP;              // 0x65C Transmit Channel 7 Completion Pointer Register
    REG32  RX0CP;              // 0x660 Receive Channel 0 Completion Pointer Register
    REG32  RX1CP;              // 0x664 Receive Channel 1 Completion Pointer Register
    REG32  RX2CP;              // 0x668 Receive Channel 2 Completion Pointer Register
    REG32  RX3CP;              // 0x66C Receive Channel 3 Completion Pointer Register
    REG32  RX4CP;              // 0x670 Receive Channel 4 Completion Pointer Register
    REG32  RX5CP;              // 0x674 Receive Channel 5 Completion Pointer Register
    REG32  RX6CP;              // 0x678 Receive Channel 6 Completion Pointer Register
    REG32  RX7CP;              // 0x67C Receive Channel 7 Completion Pointer Register
} EMAC_MODULE_REGS, *PEMACREGS;

typedef struct __EMACCTRLREGS__ {
    REG32  REVID;              // 0x000 EMAC Control Module Revision ID Register
    REG32  SOFTRESET;          // 0x004 EMAC Control Module Software Reset Register
    REG32  RSVD_008;           // 0x008 Reserved
    REG32  INTCONTROL;         // 0x00C EMAC Control Module Interrupt Control Register
    REG32  C0RXTHRESHEN;       // 0x010 EMAC Control Module Interrupt Core 0 Receive Threshold Interrupt Enable Register
    REG32  C0RXEN;             // 0x014 EMAC Control Module Interrupt Core 0 Receive Interrupt Enable Registe
    REG32  C0TXEN;             // 0x018 EMAC Control Module Interrupt Core 0 Transmit Interrupt Enable Register
    REG32  C0MISCEN;           // 0x01C EMAC Control Module Interrupt Core 0 Miscellaneous Interrupt Enable Register
    REG32  C1RXTHRESHEN;       // 0x020 EMAC Control Module Interrupt Core 1 Receive Threshold Interrupt Enable Register
    REG32  C1RXEN;             // 0x024 EMAC Control Module Interrupt Core 1 Receive Interrupt Enable Register
    REG32  C1TXEN;             // 0x028 EMAC Control Module Interrupt Core 1 Transmit Interrupt Enable Register
    REG32  C1MISCEN;           // 0x02C EMAC Control Module Interrupt Core 1 Miscellaneous Interrupt Enable Register
    REG32  C2RXTHRESHEN;       // 0x030 EMAC Control Module Interrupt Core 2 Receive Threshold Interrupt Enable Register
    REG32  C2RXEN;             // 0x034 EMAC Control Module Interrupt Core 2 Receive Interrupt Enable Register
    REG32  C2TXEN;             // 0x038 EMAC Control Module Interrupt Core 2 Transmit Interrupt Enable Register
    REG32  C2MISCEN;           // 0x03C EMAC Control Module Interrupt Core 2 Miscellaneous Interrupt Enable Register
    REG32  C0RXTHRESHSTAT;     // 0x040 EMAC Control Module Interrupt Core 0 Receive Threshold Interrupt Status Register
    REG32  C0RXSTAT;           // 0x044 EMAC Control Module Interrupt Core 0 Receive Interrupt Status Register
    REG32  C0TXSTAT;           // 0x048 EMAC Control Module Interrupt Core 0 Transmit Interrupt Status Register
    REG32  C0MISCSTAT;         // 0x04C EMAC Control Module Interrupt Core 0 Miscellaneous Interrupt Status Register
    REG32  C1RXTHRESHSTAT;     // 0x050 EMAC Control Module Interrupt Core 1 Receive Threshold Interrupt Status Register
    REG32  C1RXSTAT;           // 0x054 EMAC Control Module Interrupt Core 1 Receive Interrupt Status Register
    REG32  C1TXSTAT;           // 0x058 EMAC Control Module Interrupt Core 1 Transmit Interrupt Status Register
    REG32  C1MISCSTAT;         // 0x05C EMAC Control Module Interrupt Core 1 Miscellaneous Interrupt Status Register
    REG32  C2RXTHRESHSTAT;     // 0x060 EMAC Control Module Interrupt Core 2 Receive Threshold Interrupt Status Register
    REG32  C2RXSTAT;           // 0x064 EMAC Control Module Interrupt Core 2 Receive Interrupt Status Register
    REG32  C2TXSTAT;           // 0x068 EMAC Control Module Interrupt Core 2 Transmit Interrupt Status Register
    REG32  C2MISCSTAT;         // 0x06C EMAC Control Module Interrupt Core 2 Miscellaneous Interrupt Status Register
    REG32  C0RXIMAX;           // 0x070 EMAC Control Module Interrupt Core 0 Receive Interrupts Per Millisecond Register
    REG32  C0TXIMAX;           // 0x074 EMAC Control Module Interrupt Core 0 Transmit Interrupts Per Millisecond Register
    REG32  C1RXIMAX;           // 0x078 EMAC Control Module Interrupt Core 1 Receive Interrupts Per Millisecond Register
    REG32  C1TXIMAX;           // 0x07C EMAC Control Module Interrupt Core 1 Transmit Interrupts Per Millisecond Register
    REG32  C2RXIMAX;           // 0x080 EMAC Control Module Interrupt Core 2 Receive Interrupts Per Millisecond Regist
    REG32  C2TXIMAX;           // 0x084 EMAC Control Module Interrupt Core 2 Transmit Interrupts Per Millisecond Register
} EMAC_SUBSYS_REGS, *PEMACCTRLREGS;

typedef struct __EMACMDIOREGS__ {
    REG32  REVID;              // 0x000 MDIO Revision ID Register
    REG32  CONTROL;            // 0x004 MDIO Control Register
    REG32  ALIVE;              // 0x008 PHY Alive Status register
    REG32  LINK;               // 0x00C PHY Link Status Register
    REG32  LINKINTRAW;         // 0x010 MDIO Link Status Change Interrupt (Unmasked) Register
    REG32  LINKINTMASKED;      // 0x014 MDIO Link Status Change Interrupt (Masked) Register
    REG32  RSVD_018_01C[2];    // 0x018 - 0x01C Reserved
    REG32  USERINTRAW;         // 0x020 MDIO User Command Complete Interrupt (Unmasked) Register
    REG32  USERINTMASKED;      // 0x024 MDIO User Command Complete Interrupt (Masked) Register
    REG32  USERINTMASKSET;     // 0x028 MDIO User Command Complete Interrupt Mask Set Register
    REG32  USERINTMASKCLEAR;   // 0x02C MDIO User Command Complete Interrupt Mask Clear Register
    REG32  RSVD_030_07C[20];   // 0x030 - 0x07C Reserved
    REG32  USERACCESS0;        // 0x080 MDIO User Access Register 0
    REG32  USERPHYSEL0;        // 0x084 MDIO User PHY Select Register 0
    REG32  USERACCESS1;        // 0x088 MDIO User Access Register 1
    REG32  USERPHYSEL1;        // 0x08C MDIO User PHY Select Register 1
} EMAC_MDIO_REGS, *PEMACMDIOREGS;

#pragma warning(push)
#pragma warning(disable:4201)
typedef struct __EMACDESC__ {
    union {
        struct  __EMACDESC__*   pNext;        // Pointer to next descriptor in chain 
        DWORD next;
    };
    union {
        UINT8*                  pBuffer;      // Pointer to data buffer 
        DWORD buffer;
    };
    UINT32                  BufOffLen;    // Buffer Offset(MSW) and Length(LSW) 
    UINT32                  PktFlgLen;    // Packet Flags(MSW) and Length(LSW) 
    
}EMAC_DESC,*PEMACDESC;
#pragma warning(pop)

//------------------------------------------------------------------------------

// EMAC_MAX_CHAN
// Maximum number of channels supported in EMAC
#define EMAC_MAX_CHAN               8

// EMAC_STATS_REGS
// Maximum number of Statics registers in EMAC
#define EMAC_STATS_REGS             36

// EMAC_MULTICAST_REGS
// Maximum number of Multicast entries in EMAC                         
#define EMAC_MAX_MCAST_ENTRIES      64

// EMAC_RXMBPENABL 
// Bit defines for RXMBPENABL register
#define EMAC_RXMBPENABLE_RXCAFEN_ENABLE         BIT(21)
#define EMAC_RXMBPENABLE_RXBROADEN              BIT(13)
#define EMAC_RXMBPENABLE_RXMULTIEN              BIT(5)

// EMAC_MACCONTROL 
// Bit defines for MACCONTROL register
#define EMAC_MACCONTROL_MIIEN_ENABLE            BIT(5)
#define EMAC_MACCONTROL_FULLDUPLEX_ENABLE       BIT(0)
#define EMAC_MACCONTROL_LOOPBACK_ENABLE         BIT(1)
#define EMAC_MACCONTROL_RXBUFFERFLOW_ENABLE     BIT(3)
#define EMAC_MACCONTROL_TXFLOW_EMABLE           BIT(4)
#define EMAC_MACCONTROL_GMII_ENABLE             BIT(5)
#define EMAC_MACCONTROL_TXPACING_ENABLE         BIT(6)
#define EMAC_MACCONTROL_TX_PRIO_ROUND_ROBIN     0x0
#define EMAC_MACCONTROL_TX_PRIO_FIXED           BIT(9)
#define EMAC_MACCONTROL_RXOFFLENBLOCK_ENABLE    BIT(14)
#define EMAC_MACCONTROL_RMII_SPEED              BIT(15)

// EMAC_MDIO
// Bit defines for EMAC_MDIO register
#define MDIO_USERACCESS0_GO                     BIT(31)
#define MDIO_USERACCESS0_WRITE_READ             0x0
#define MDIO_USERACCESS0_WRITE_WRITE            BIT(30)
#define MDIO_USERACCESS0_ACK			        BIT(29)


// MDIO_CONTROL
// Bit defines for MDIO_CONTROL register
#define MDIO_CONTROL_IDLE                       BIT(31)
#define MDIO_CONTROL_ENABLE                     BIT(30)
#define MDIO_CONTROL_FAULT                      BIT(19)
#define MDIO_CONTROL_FAULTEN                    BIT(18)

// EMAC_DSC
// Various Emac descriptor flags 
#define EMAC_DSC_FLAG_SOP                   BIT(31)
#define EMAC_DSC_FLAG_EOP                   BIT(30)
#define EMAC_DSC_FLAG_OWNER                 BIT(29)
#define EMAC_DSC_FLAG_EOQ                   BIT(28)
#define EMAC_DSC_FLAG_TDOWNCMPLT            BIT(27)
#define EMAC_DSC_FLAG_PASSCRC               BIT(26)
#define EMAC_DSC_FLAG_JABBER                BIT(25)
#define EMAC_DSC_FLAG_OVERSIZE              BIT(24)
#define EMAC_DSC_FLAG_FRAGMENT              BIT(23)
#define EMAC_DSC_FLAG_UNDERSIZED            BIT(22)
#define EMAC_DSC_FLAG_CONTROL               BIT(21)
#define EMAC_DSC_FLAG_OVERRUN               BIT(20)
#define EMAC_DSC_FLAG_CODEERROR             BIT(19)
#define EMAC_DSC_FLAG_ALIGNERROR            BIT(18)
#define EMAC_DSC_FLAG_CRCERROR              BIT(17)
#define EMAC_DSC_FLAG_NOMATCH               BIT(16)
#define EMAC_DSC_RX_ERROR_FRAME             0x03FC0000

// EMAC_MDIO_CLOCK_FREQ clock
// Emac MDIO clock defines
#define EMAC_MDIO_CLOCK_FREQ                500000  // 500KHz

// EMAC_PACKET
// Emac Ethernet packet defines
#define EMAC_MAX_DATA_SIZE               1500
#define EMAC_MIN_DATA_SIZE               46
#define EMAC_HEADER_SIZE                 14
#define EMAC_RX_MAX_LEN                  1520   
#define EMAC_MIN_ETHERNET_PKT_SIZE       (EMAC_MIN_DATA_SIZE + EMAC_HEADER_SIZE)
#define EMAC_MAX_ETHERNET_PKT_SIZE       (EMAC_MAX_DATA_SIZE + EMAC_HEADER_SIZE)
#define EMAC_PKT_ALIGN                   22      // (1514 + 22 = 1536)packet aligned on 32 byte boundry
#define EMAC_MAX_PKT_BUFFER_SIZE         (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)


// EMAC_MII
// MII Register numbers
#define MII_CONTROL_REG             0
#define MII_STATUS_REG              1
#define MII_PHY_ID_0                1
#define MII_PHY_ID_1                2
#define MII_PHY_ID_2                4

// EMAC_PHY
// Bit defines for Intel PHY Registers
#define PHY_LINK_STATUS             BIT(2)

typedef struct {
    DWORD EMAC_CTRL_OFFSET;
    DWORD EMAC_RAM_OFFSET;
    DWORD EMAC_MDIO_OFFSET;
    DWORD EMAC_REG_OFFSET;
    DWORD EMAC_TOTAL_MEMORY;
    DWORD EMAC_PERPECTIVE_RAM_ADDR;
} EMAC_MEM_LAYOUT;

#endif //__OMAP_CPGMAC_REGS_H