//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2011-2012
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#ifndef _CPSW3G_H_
#define _CPSW3G_H_

#include "oal_memory.h"

#define TIMEOUT_VALUE  2000    
#define LINK_TIMEOUT_VALUE 6000
#define KITL_CHANNEL 0

/* PHY configuration */
#define CONF_NUM_MAC_PORT     2
#define MAC0_EXT_PHY_MASK     0x1 <<EXTERNAL_PHY0_ADDR 
#define MAC1_EXT_PHY_MASK     0x1 <<EXTERNAL_PHY1_ADDR 
#define EXTERNAL_PHY0_ADDR    0
#define EXTERNAL_PHY1_ADDR    1  
#define LK_DN                 0
#define LK_UP                 1

/* phy seed setup */
#define AUTO			99
#define _1000BASET		1000
#define _100BASET		100
#define _10BASET		10
#define HALF			22
#define FULL			44

/* phy register offsets */
#define PHY_BMCR		0x00
#define PHY_BMSR		0x01
#define PHY_PHYIDR1		0x02
#define PHY_PHYIDR2		0x03
#define PHY_ANAR		0x04
#define PHY_ANLPAR		0x05
#define PHY_ANER		0x06
#define PHY_ANNPTR		0x07
#define PHY_ANLPNP		0x08
#define PHY_1000BTCR	0x09
#define PHY_1000BTSR	0x0A
#define PHY_EXSR		0x0F

#define PHY_SPEC_STAT	0x11

#define PHY_PHYCTRL2	0x12
#define PHY_LBR			0x13
#define PHY_RECR		0x14
#define PHY_MICR		0x15
#define PHY_CONF_REG	0x16
#define PHY_CTRL_REG	0x17

#define PHY_CONF_TXCLKEN       (1 << 5)

/* PHY BMCR */
#define PHY_BMCR_RESET		0x8000
#define PHY_BMCR_LOOP		0x4000
#define PHY_BMCR_100MB		0x2000
#define PHY_BMCR_AUTON		0x1000
#define PHY_BMCR_POWD		0x0800
#define PHY_BMCR_ISO		0x0400
#define PHY_BMCR_RST_NEG	0x0200
#define PHY_BMCR_DPLX		0x0100
#define PHY_BMCR_COL_TST	0x0080

#define PHY_BMCR_SPEED_MASK	0x2040
#define PHY_BMCR_1000_MBPS	0x0040
#define PHY_BMCR_100_MBPS	0x2000
#define PHY_BMCR_10_MBPS	0x0000

/* phy BMSR */
#define PHY_BMSR_100T4		0x8000
#define PHY_BMSR_100TXF		0x4000
#define PHY_BMSR_100TXH		0x2000
#define PHY_BMSR_10TF		0x1000
#define PHY_BMSR_10TH		0x0800
#define PHY_BMSR_EXT_STAT	0x0100
#define PHY_BMSR_PRE_SUP	0x0040
#define PHY_BMSR_AUTN_COMP	0x0020
#define PHY_BMSR_RF		0x0010
#define PHY_BMSR_AUTN_ABLE	0x0008
#define PHY_BMSR_LS		0x0004
#define PHY_BMSR_JD		0x0002
#define PHY_BMSR_EXT		0x0001

/*phy ANLPAR */
#define PHY_ANLPAR_NP		0x8000
#define PHY_ANLPAR_ACK		0x4000
#define PHY_ANLPAR_RF		0x2000
#define PHY_ANLPAR_ASYMP	0x0800
#define PHY_ANLPAR_PAUSE	0x0400
#define PHY_ANLPAR_T4		0x0200
#define PHY_ANLPAR_TXFD		0x0100
#define PHY_ANLPAR_TX		0x0080
#define PHY_ANLPAR_10FD		0x0040
#define PHY_ANLPAR_10		0x0020
#define PHY_ANLPAR_100		0x0380	/* we can run at 100 */
/* phy ANLPAR 1000BASE-X */
#define PHY_X_ANLPAR_NP		0x8000
#define PHY_X_ANLPAR_ACK	0x4000
#define PHY_X_ANLPAR_RF_MASK	0x3000
#define PHY_X_ANLPAR_PAUSE_MASK	0x0180
#define PHY_X_ANLPAR_HD		0x0040
#define PHY_X_ANLPAR_FD		0x0020

#define PHY_ANLPAR_PSB_MASK	0x001f
#define PHY_ANLPAR_PSB_802_3	0x0001
#define PHY_ANLPAR_PSB_802_9	0x0002

/* phy 1000BTCR */
#define PHY_1000BTCR_1000FD	0x0200
#define PHY_1000BTCR_1000HD	0x0100

/* phy 1000BTSR */
#define PHY_1000BTSR_MSCF	0x8000
#define PHY_1000BTSR_MSCR	0x4000
#define PHY_1000BTSR_LRS	0x2000
#define PHY_1000BTSR_RRS	0x1000
#define PHY_1000BTSR_1000FD	0x0800
#define PHY_1000BTSR_1000HD	0x0400

/* phy EXSR */
#define PHY_EXSR_1000XF		0x8000
#define PHY_EXSR_1000XH		0x4000
#define PHY_EXSR_1000TF		0x2000
#define PHY_EXSR_1000TH		0x1000

#define PHY_SPEC_STAT_1000	0x8000

/* TLK110 PHY registers */
#define TLK110_COARSEGAIN_REG	0x00A3
#define TLK110_LPFHPF_REG		0x00AC
#define TLK110_SPAREANALOG_REG	0x00B9
#define TLK110_VRCR_REG			0x00D0
#define TLK110_SETFFE_REG		0x0107
#define TLK110_FTSP_REG			0x0154
#define TLK110_ALFATPIDL_REG	0x002A
#define TLK110_PSCOEF21_REG		0x0096
#define TLK110_PSCOEF3_REG		0x0097
#define TLK110_ALFAFACTOR1_REG	0x002C
#define TLK110_ALFAFACTOR2_REG	0x0023
#define TLK110_CFGPS_REG		0x0095
#define TLK110_FTSPTXGAIN_REG	0x0150
#define TLK110_SWSCR3_REG		0x000B
#define TLK110_SCFALLBACK_REG	0x0040
#define TLK110_PHYRCR_REG		0x001F

/* TLK110 register writes values */
#define TLK110_COARSEGAIN_VAL	0x0000
#define TLK110_LPFHPF_VAL		0x8000
#define TLK110_SPAREANALOG_VAL	0x0000
#define TLK110_VRCR_VAL			0x0008
#define TLK110_SETFFE_VAL		0x0605
#define TLK110_FTSP_VAL			0x0255
#define TLK110_ALFATPIDL_VAL	0x7998
#define TLK110_PSCOEF21_VAL		0x3A20
#define TLK110_PSCOEF3_VAL		0x003F
#define TLK110_ALFAFACTOR1_VAL	0xFF80
#define TLK110_ALFAFACTOR2_VAL	0x021C
#define TLK110_CFGPS_VAL		0x0000
#define TLK110_FTSPTXGAIN_VAL	0x6A88
#define TLK110_SWSCR3_VAL		0x0000
#define TLK110_SCFALLBACK_VAL	0xC11D
#define TLK110_PHYRCR_VAL		0x4000
#define TLK110_PHYIDR1			0x2000
#define TLK110_PHYIDR2			0xA201

typedef enum link_speed
{
    SPEED_10M = 0,
    SPEED_100M,
    SPEED_1000M
}LINK_SPEED;


/**
 *  \brief TX Buffer Descriptor
 *
 *  CPPI 3.0 BD structure specific to CPSWITCH.
 */
typedef volatile struct BUFF_DESC {
	struct BUFF_DESC	*next;
	UINT32				*BuffPtr;
	UINT32				BuffOffsetLength;
	UINT32				BuffFlagPktLen;
} BuffDesc, *PBUFFDESC;


typedef struct CPSW_CB {
    UINT32           BaseAddress;
    UINT32           macAddrLo;        	/* Mac (ethernet) address */       
    UINT32           macAddrHi;        	/* Mac (ethernet) address */  
    UINT32           mac_port;
    UINT16           phy_id;
    UINT32           Phy_mode[CONF_NUM_MAC_PORT];        /* 1: internal, 0:external */
    BuffDesc         *tx;
    BuffDesc         *rx;
    BuffDesc        *rx_next;         /* this should always be uncached. */
    BuffDesc        *rx_next_pa;       
    UINT32           rx_next_buf;
    
    UINT32           rx_bd_pa;
    UINT32           rx_buf_pa;
    
    UINT32           tx_buf_pa;
    UINT32           tx_bd_pa;
    
    UINT32           tx_port_bit;
    BOOL             link_state;

    UINT32           mac_control;

} CpswCb, *PCpswCb;

#define CPSW_VTP_CTRL0_BASE		(0x44E10E0C)
#define CPSW_VTP_CTRL1_BASE		(0x48140E10)

#define VTP_SINGLE_MODE			0x0
#define VTP_CONT_MODE			0x1
#define VTP_GZ_ALL				0xfff
#define VTP_SINGLE_MODE_START	(0x1<<1)


#define CPSW_3G_BASE			g_cpsw3g_reg_base
#define CPMAC_1_BASE			(CPSW_3G_BASE + 0x0D80)   // sliver
#define CPMAC_2_BASE			(CPSW_3G_BASE + 0x0DC0)   // sliver
#define CPDMA_BASE				(CPSW_3G_BASE + 0x0800)   // cpdma
#define CPSW_STATS_BASE			(CPSW_3G_BASE + 0x0900)

#define CPSW3G_RX_BD_ALIGN		16  
#define CPSW3G_TX_BD_ALIGN		16  
#define RX_BUFF_SZ				1536
#define TX_BD_COUNT				1
#define RX_BD_COUNT				512
#define PKT_MIN					60
#define PKT_MAX					(1500 + 14 + 4 + 4)

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
#define CPSW3G_MIN_ETHERNET_PACKET_SIZE     60

#define BD_SOP    CPSW3G_CPPI_SOP_BIT
#define BD_EOP    CPSW3G_CPPI_EOP_BIT
#define BD_OWNS   CPSW3G_CPPI_OWNERSHIP_BIT
#define BD_EOQ    CPSW3G_CPPI_EOQ_BIT

/* BD Macros. */
#define IS_SOP(bd)   		((bd)->BuffFlagPktLen & CPSW3G_CPPI_SOP_BIT)
#define IS_EOP(bd)   		((bd)->BuffFlagPktLen & CPSW3G_CPPI_EOP_BIT)
#define PORT_OWNS(bd)		((bd)->BuffFlagPktLen & CPSW3G_CPPI_OWNERSHIP_BIT)
#define HOST_OWNS(bd)		(!(PORT_OWNS((bd))))
#define IS_TRDN(bd)  		((bd)->BuffFlagPktLen & CPSW3G_CPPI_TEARDOWN_COMPLETE_BIT)
#define IS_EOQ(bd)   		((bd)->BuffFlagPktLen & CPSW3G_CPPI_EOQ_BIT)
#define BD_BufPTR(bd)    	((bd)->BuffPtr)
#define BD_BufOFFSET(bd)    (((bd)->BuffOffsetLength & CPSW3G_CPPI_BUFF_OFFSET_MASK) >> 16)
#define BD_BufLen(bd)    	((bd)->BuffOffsetLength & CPSW3G_CPPI_BUFF_LEN_MASK)
#define BD_PktLen(bd)		((bd)->BuffFlagPktLen & CPSW3G_CPPI_PKT_LEN_MASK)

/* CPMAC */
#define CPMAC_A_RXFREE(ch)       (CPSW_3G_BASE + 0x8E0 + ((ch) * 4))
#define CPMAC_A_TX_DMAHDP(ch)    (CPSW_3G_BASE + 0xA00 + ((ch) * 4))
#define CPMAC_A_RX_DMAHDP(ch)    (CPSW_3G_BASE + 0xA20 + ((ch) * 4))
#define CPMAC_A_TX_DMACP(ch)     (CPSW_3G_BASE + 0xA40 + ((ch) * 4))
#define CPMAC_A_RX_DMACP(ch)     (CPSW_3G_BASE + 0xA60 + ((ch) * 4))

/* ALE Registers */
#define CPALE_VER_ID             (CPSW_3G_BASE + 0xD00)
#define CPALE_CTRL               (CPSW_3G_BASE + 0xD08)
#define CPALE_PRESCALE           (CPSW_3G_BASE + 0xD10)
#define CPALE_UNK_VLAN           (CPSW_3G_BASE + 0xD18)
#define CPALE_TBLCTL             (CPSW_3G_BASE + 0xD20)
#define CPALE_TBLW2              (CPSW_3G_BASE + 0xD34)
#define CPALE_TBLW1              (CPSW_3G_BASE + 0xD38)
#define CPALE_TBLW0              (CPSW_3G_BASE + 0xD3C)
#define CPALE_PORT_CTRL_0        (CPSW_3G_BASE + 0xD40)
#define CPALE_PORT_CTRL_1        (CPSW_3G_BASE + 0xD44)
#define CPALE_PORT_CTRL_2        (CPSW_3G_BASE + 0xD48)
#define CPALE_PORT_CTRL_3        (CPSW_3G_BASE + 0xD4C)
#define CPALE_PORT_CTRL_4        (CPSW_3G_BASE + 0xD50)
#define CPALE_PORT_CTRL_5        (CPSW_3G_BASE + 0xD54)
/* ALE Registers ends */

/* CPSW_3G switch registers */
#define CPSW_VER_ID              (CPSW_3G_BASE)
#define CPSW_CTRL                (CPSW_3G_BASE + 0x04)
#define CPSW_SRST                (CPSW_3G_BASE + 0x08)
#define CPSW_STAT_PORT_EN        (CPSW_3G_BASE + 0x0C)
#define CPSW_PRI_TYPE            (CPSW_3G_BASE + 0x10)
#define CPSW_FLOW_CONTROL        (CPSW_3G_BASE + 0x24)
//
#define CPSW_P0_CONTROL          (CPSW_3G_BASE + 0x100)
#define CPSW_P0_MAX_BLK          (CPSW_3G_BASE + 0x108)
#define CPSW_P0_BLK_CNT          (CPSW_3G_BASE + 0x10C)
#define CPSW_P0_VLAN             (CPSW_3G_BASE + 0x114)
#define CPSW_P0_TX_PRI_MAP       (CPSW_3G_BASE + 0x118)
#define CPDMA_TX_PRI_MAP         (CPSW_3G_BASE + 0x11C)
#define CPDMA_RX_CH_MAP          (CPSW_3G_BASE + 0x120)
/* P0_RX_DSCP_PRI_MAP_n, n=0...7 */
//
#define CPSW_P1_CONTROL          (CPSW_3G_BASE + 0x200)
#define CPSW_P1_MAX_BLK          (CPSW_3G_BASE + 0x208)
#define CPSW_P1_BLK_CNT          (CPSW_3G_BASE + 0x20C)
#define CPSW_P1_VLAN             (CPSW_3G_BASE + 0x214)
#define CPSW_P1_TX_PRI_MAP       (CPSW_3G_BASE + 0x218)
#define CPSW_P1_SRC_ADDR_LOW     (CPSW_3G_BASE + 0x220)
#define CPSW_P1_SRC_ADDR_HI      (CPSW_3G_BASE + 0x224)
/* P1_RX_DSCP_PRI_MAP_n, n=0...7 */
//
#define CPSW_P2_CONTROL          (CPSW_3G_BASE + 0x300)
#define CPSW_P2_MAX_BLK          (CPSW_3G_BASE + 0x308)
#define CPSW_P2_BLK_CNT          (CPSW_3G_BASE + 0x30C)
#define CPSW_P2_VLAN             (CPSW_3G_BASE + 0x314)
#define CPSW_P2_TX_PRI_MAP       (CPSW_3G_BASE + 0x318)
#define CPSW_P2_SRC_ADDR_LOW     (CPSW_3G_BASE + 0x320)
#define CPSW_P2_SRC_ADDR_HI      (CPSW_3G_BASE + 0x324)
/* P1_RX_DSCP_PRI_MAP_n, n=0...7 */

/* Slave port registers (for convenience) == CPSW_P1/2_... */
// n = 1, 2
#define CPSW_SL_MAX_BLK(n)			(CPSW_P1_MAX_BLK      + ((n - 1) * 0x100))
#define CPSW_SL_BLK_CNT(n)			(CPSW_P1_BLK_CNT      + ((n - 1) * 0x100))
#define CPSW_SL_VLAN(n)				(CPSW_P1_VLAN         + ((n - 1) * 0x100))
#define CPSW_SL_TX_PRI_MAP(n)		(CPSW_P1_TX_PRI_MAP   + ((n - 1) * 0x100))
#define CPSW_SL_SRC_ADDR_LOW(n)		(CPSW_P1_SRC_ADDR_LOW + ((n - 1) * 0x100))
#define CPSW_SL_SRC_ADDR_HI(n)		(CPSW_P1_SRC_ADDR_HI  + ((n - 1) * 0x100))

#define CPMAC_A_RX_FBUFF(ch)     (CPSW_3G_BASE + 0x8E0 + ((ch) * 4))


/* CPMAC registers */
// n = 1, 2
#define CPMAC_VER_ID(n)           (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x00)
#define CPMAC_MAC_CTRL(n)         (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x04)
#define CPMAC_MAC_STS(n)          (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x08)
#define CPMAC_MAC_SRST(n)         (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x0C)
#define CPMAC_MAC_RXMAX_LEN(n)    (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x10)
#define CPMAC_MAC_BOFF_TEST(n)    (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x14)
#define CPMAC_MAC_RX_PAUSE(n)     (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x18)
#define CPMAC_MAC_TX_PAUSE(n)     (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x1C)
#define CPMAC_MAC_EM_CTRL(n)      (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x20)
#define CPMAC_MAC_RX_PRI_MAP(n)   (CPMAC_1_BASE + ((n - 1) * 0x40)+ 0x24)


/* CPDMA registers */
#define CPDMA_TX_VER_ID           	(CPDMA_BASE)
#define CPDMA_TX_CTRL             	(CPDMA_BASE + 0x04)                       
#define CPDMA_TX_TRDN             	(CPDMA_BASE + 0x08)
#define CPDMA_RX_VER_ID           	(CPDMA_BASE + 0x10)
#define CPDMA_RX_CTRL             	(CPDMA_BASE + 0x14)
#define CPDMA_RX_TRDN             	(CPDMA_BASE + 0x18)
#define CPDMA_SRST                	(CPDMA_BASE + 0x1C)
#define CPDMA_DMA_CTRL           	(CPDMA_BASE + 0x20)
#define CPDMA_DMA_STS             	(CPDMA_BASE + 0x24)
#define CPDMA_RX_BUF_OFF          	(CPDMA_BASE + 0x28)
#define CPDMA_EM_CTRL             	(CPDMA_BASE + 0x2C)
//
#define CPDMA_TX_IntStat_Raw		(CPDMA_BASE + 0x80)
#define CPDMA_TX_IntStat_Masked  	(CPDMA_BASE + 0x84)
#define CPDMA_TX_IntMask_Set	  	(CPDMA_BASE + 0x88)
#define CPDMA_TX_IntMask_Clear		(CPDMA_BASE + 0x8c)
#define CPDMA_In_Vector		  		(CPDMA_BASE + 0x90)
#define CPDMA_EOI_Vector	  		(CPDMA_BASE + 0x94)
//
#define CPDMA_RX_IntStat_Raw   		(CPDMA_BASE + 0xa0)
#define CPDMA_RX_IntStat_Masked  	(CPDMA_BASE + 0xa4)
#define CPDMA_RX_IntMask_Set  		(CPDMA_BASE + 0xa8)
#define CPDMA_RX_IntMask_Clear		(CPDMA_BASE + 0xac)
#define CPDMA_IntStat_Raw			(CPDMA_BASE + 0xb0)
#define CPDMA_IntStat_Masked		(CPDMA_BASE + 0xb4)
#define CPDMA_IntMask_Set			(CPDMA_BASE + 0xb8)
#define CPDMA_IntMask_Clear			(CPDMA_BASE + 0xbc)


/* CPDMA Statistics registers */
#define CPMAC_A_ST_RX_GOODFRAMES  (CPSW_STATS_BASE + 0x000)
#define CPMAC_A_ST_RX_BROADCAST   (CPSW_STATS_BASE + 0x004)
#define CPMAC_A_ST_RX_MULTICAST   (CPSW_STATS_BASE + 0x008)
#define CPMAC_A_ST_RX_PAUSE       (CPSW_STATS_BASE + 0x00c)
#define CPMAC_A_ST_RX_ERR_CRC     (CPSW_STATS_BASE + 0x010)
#define CPMAC_A_ST_RX_ERR_ALIGNC  (CPSW_STATS_BASE + 0x014)
#define CPMAC_A_ST_RX_OVERSIZED   (CPSW_STATS_BASE + 0x018)
#define CPMAC_A_ST_RX_JABBER      (CPSW_STATS_BASE + 0x01c)
#define CPMAC_A_ST_RX_UNDERSIZED  (CPSW_STATS_BASE + 0x020)
#define CPMAC_A_ST_RX_FRAGMENTS   (CPSW_STATS_BASE + 0x024)
//
#define CPMAC_A_ST_RX_OCTETS      (CPSW_STATS_BASE + 0x030)
#define CPMAC_A_ST_TX_GOODFRAMES  (CPSW_STATS_BASE + 0x034)
#define CPMAC_A_ST_TX_BROADCAST   (CPSW_STATS_BASE + 0x038)
#define CPMAC_A_ST_TX_MULTICAST   (CPSW_STATS_BASE + 0x03c)
#define CPMAC_A_ST_TX_PAUSE       (CPSW_STATS_BASE + 0x040)
#define CPMAC_A_ST_TX_DEFFERED    (CPSW_STATS_BASE + 0x044)
#define CPMAC_A_ST_TX_COLLISION   (CPSW_STATS_BASE + 0x048)
#define CPMAC_A_ST_TX_SINGLECOLL  (CPSW_STATS_BASE + 0x04c)
#define CPMAC_A_ST_TX_MULTICOLL   (CPSW_STATS_BASE + 0x050)
#define CPMAC_A_ST_TX_EXCESSCOLL  (CPSW_STATS_BASE + 0x054)
#define CPMAC_A_ST_TX_LATECOLL    (CPSW_STATS_BASE + 0x058)
#define CPMAC_A_ST_TX_UNDERRUN    (CPSW_STATS_BASE + 0x05c)
#define CPMAC_A_ST_TX_CARRSENSEER (CPSW_STATS_BASE + 0x060)
#define CPMAC_A_ST_TX_OCTETS      (CPSW_STATS_BASE + 0x064)
#define CPMAC_A_ST_TX_OC_64       (CPSW_STATS_BASE + 0x068)
#define CPMAC_A_ST_TX_OC_65_127   (CPSW_STATS_BASE + 0x06c)
#define CPMAC_A_ST_TX_OC_128_255  (CPSW_STATS_BASE + 0x070)
#define CPMAC_A_ST_TX_OC_256_511  (CPSW_STATS_BASE + 0x074)
#define CPMAC_A_ST_TX_OC_512_1023 (CPSW_STATS_BASE + 0x078)
#define CPMAC_A_ST_TX_OC_1024_U   (CPSW_STATS_BASE + 0x07c)
#define CPMAC_A_ST_TX_NETOCTETS   (CPSW_STATS_BASE + 0x080)
#define CPMAC_A_ST_RX_SOF_OVRUN   (CPSW_STATS_BASE + 0x084)
#define CPMAC_A_ST_TX_MOF_OVRUN   (CPSW_STATS_BASE + 0x088)
#define CPMAC_A_ST_TX_DMA_OVRUN   (CPSW_STATS_BASE + 0x08c)

#define CPMAC_PORT_DISABLED       0x00
#define CPMAC_PORT_BLOCKED        0x01
#define CPMAC_PORT_LEARN          0x02
#define CPMAC_PORT_FORWARD        0x03
#define CPMAC_NOLEARN             0x10

#define CPMAC_CTL_FD              (0x1<<0)
#define CPMAC_CTL_RX_FLOW_EN      (0x1<<3)
#define CPMAC_CTL_TX_FLOW_EN      (0x1<<4)
#define CPMAC_CTL_GMII_EN         (0x1<<5)
#define CPMAC_CTL_GIG             (0x1<<7)
#define CPMAC_CTL_IFCTL_A         (0x1<<15)
#define CPMAC_CTL_IFCTL_B         (0x1<<16)
#define CPMAC_CTL_EXT_EN          (0x1<<18)
#define CPMAC_RX_CEF_EN           (0x1<<22)
#define CPMAC_RX_CSF_EN           (0x1<<23)
#define CPMAC_RX_CMF_EN           (0x1<<24)


/* prototypes from Cpsw3g.c */
BOOL cpsw3g_rx_bd_init(BuffDesc *rx_bd_head);
BOOL cpsw3g_tx_bd_init(BuffDesc *tx_bd_head);  
BOOL cpsw3g_phy_init(void);
BOOL Phy_link_state(void);
void config_mavell_phy_init(void);
BOOL Cpsw3g_get_mavell_phy_link_state(UINT32 port);

extern BOOL Cpsw3gInit (UINT8 *pBaseAddress, UINT32 offset, UINT16 MacAddr[3]);

extern UINT16 Cpsw3gGetFrame(UINT8 *pBuffer, UINT16 *pLength);

extern UINT16 Cpsw3gSendFrame(UINT8 *pBuffer, UINT32 length);

extern VOID Cpsw3gEnableInts(void);
extern VOID Cpsw3gDisableInts(void);
extern VOID Cpsw3gCurrentPacketFilter(UINT32 filter);
extern BOOL Cpsw3gMulticastList(UINT8 *pAddresses, UINT32 count);

/* prototype from mdio.c */
extern void MdioWr(UINT16 phyAddr, UINT16 regNum, int channel, UINT16 data);

extern int MdioRd(UINT16 PhyAddr, UINT16 RegNum, int channel, UINT16 *pData);
extern int MdioGetLinkState(unsigned int phy_mask, int channel);
extern void MdioEnable(void);

//! \def EMAC_BUF
//! \brief EMAC buffer regions allocated by OEM
#ifdef EMAC_BUF_BASE
#undef EMAC_BUF_BASE
#endif
// +++FIXME:
#define EMAC_BUF_BASE       (0x8F000000)
#define EMAC_RX_BUFS_SIZE   (N4KB + N8KB)
#define EMAC_TX_BUFS_BASE   (EMAC_BUF_BASE + EMAC_RX_BUFS_SIZE)

#define CPSW3G_BUF_BASE     EMAC_BUF_BASE

#endif /* _CPSW3G_H_ */



