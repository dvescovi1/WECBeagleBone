// All rights reserved ADENEO EMBEDDED 2010
#include "omap.h"
#include "omap_cpgmac_regs.h"
#include <cmnintrin.h>

static EMAC_MODULE_REGS*	g_pEmacMod;
static EMAC_SUBSYS_REGS*	g_pEmacSys;
static EMAC_MDIO_REGS*		g_pMdio;

// MDIO module input frequency 
#define EMAC_MDIO_BUS_FREQ		166000000		// 166 MHZ check

#define EMAC_HW_RAM_ADDR		0x01E20000

#define EMAC_MAX_RX_BUFFERS			8
#define EMAC_MAX_TX_BUFFERS         1

#define BD_TO_HW(x) \
    ( ( (x) == 0) ? 0 : ( (x) - (DWORD) startOfCPPIArea + EMAC_HW_RAM_ADDR ))
#define HW_TO_BD(x) \
    ( ( (x) == 0) ? 0 : ( (x) - EMAC_HW_RAM_ADDR + startOfCPPIArea ))

#define SPEED_10MBPS	10
#define SPEED_100MBPS	100
//------------------------------------------------------------------------------
UINT8 active_phy_addr;
EMAC_DESC*	emac_rx_desc;
static EMAC_DESC*	emac_rx_active_head = 0;
static EMAC_DESC*	emac_rx_active_tail = 0;

EMAC_DESC*	emac_tx_desc;
UINT8*		dmaBuffer;
UINT8*		startOfCPPIArea;

//------------------------------------------------------------------------------

extern void SocAckInterrupt(DWORD flag);

//------------------------------------------------------------------------------

VOID EMACEnableInts()
{
    g_pEmacSys->C0RXEN = 0x01;
}

VOID EMACDisableInts()
{
    g_pEmacSys->C0RXTHRESHEN = 0x0;
    g_pEmacSys->C0RXEN = 0x0;
    g_pEmacSys->C0TXEN = 0x0;
    g_pEmacSys->C0MISCEN = 0x0;
}

BOOL EMACMulticastList(UINT8 *pAddresses, UINT32 count)
{
    // Not supported	 
    UNREFERENCED_PARAMETER(pAddresses);
    UNREFERENCED_PARAMETER(count);
    return FALSE;
}

VOID EMACCurrentPacketFilter(UINT32 filter)
{
    // Not supported
    UNREFERENCED_PARAMETER(filter);
}



// Read a PHY register via MDIO inteface.
BOOL PhyReadReg(UINT8 phy_addr, UINT8 reg_num, UINT16 *data)
{
    int	tmp;

    // Wait for MDIO to be ready
    while (g_pMdio->USERACCESS0 & MDIO_USERACCESS0_GO);

    g_pMdio->USERACCESS0 = MDIO_USERACCESS0_GO |
        MDIO_USERACCESS0_WRITE_READ |
        ((reg_num & 0x1f) << 21) |
        ((phy_addr & 0x1f) << 16);

    // Wait for command to complete
    while ((tmp = g_pMdio->USERACCESS0) & MDIO_USERACCESS0_GO);

    if (tmp & MDIO_USERACCESS0_ACK) {
        *data = (UINT16) tmp;
        return TRUE;
    }

    *data = (UINT16) -1;
    return FALSE;
}

// Write to a PHY register via MDIO inteface.
BOOL PhyWriteReg(UINT8 phy_addr, UINT8 reg_num, UINT16 data)
{

    // Wait for MDIO to be ready
    while (g_pMdio->USERACCESS0 & MDIO_USERACCESS0_GO);

    g_pMdio->USERACCESS0 = MDIO_USERACCESS0_GO |
        MDIO_USERACCESS0_WRITE_WRITE |
        ((reg_num & 0x1f) << 21) |
        ((phy_addr & 0x1f) << 16) |
        (data & 0xffff);

    // Wait for command to complete
    while (g_pMdio->USERACCESS0 & MDIO_USERACCESS0_GO);

    return TRUE;
}




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
#define PHY_PHYSTS		0x10
#define PHY_MIPSCR		0x11
#define PHY_MIPGSR		0x12
#define PHY_DCR			0x13
#define PHY_FCSCR		0x14
#define PHY_RECR		0x15
#define PHY_PCSR		0x16
#define PHY_LBR			0x17
#define PHY_10BTSCR		0x18
#define PHY_PHYCTRL		0x19 

/* Generic phy definitions */
#define GEN_PHY_STATUS_SPEED100_MASK	((1 << 13) | (1 << 14))
#define GEN_PHY_STATUS_FD_MASK		((1 << 11) | (1 << 13))

#define GEN_PHY_ANEG_100DUP		(1 << 8)
#define GEN_PHY_ANEG_100TX		(1 << 7)
#define GEN_PHY_ANEG_10DUP		(1 << 6)
#define GEN_PHY_ANEG_10TX		(1 << 5)

#define GEN_PHY_CTRL_RST_ANEG		(1 << 9)
#define GEN_PHY_CTRL_DUP			(1 << 8)
#define GEN_PHY_CTRL_ENA_ANEG		(1 << 12)
#define GEN_PHY_CTRL_SPD_SEL		(1 << 13)


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

void EnableMDIO()
{
    UINT32 clkdiv;
    clkdiv = (EMAC_MDIO_BUS_FREQ / EMAC_MDIO_CLOCK_FREQ) - 1;
    if (clkdiv > 0xFF)
    {
        clkdiv = 0xFF;
    }
    g_pMdio->CONTROL = (clkdiv & 0xff) |
        MDIO_CONTROL_ENABLE |
        MDIO_CONTROL_FAULT |
        MDIO_CONTROL_FAULTEN;

    while (g_pMdio->CONTROL & MDIO_CONTROL_IDLE) ;
}
UINT8  PhyInit()
{

    UINT32 i;
    UINT8 phyAddr = (UINT8)-1;
    //Initialize MDIO
    EnableMDIO();

    for (i = 0; i < 256; i++) {
        if (g_pMdio->ALIVE)
            break;
        OALStall(10);
    }

    if (i >= 256) {
        RETAILMSG(1, (L"No ETH PHY detected!!!\n"));
        return  (UINT8) -1;
    }
    phyAddr = (UINT8) (31 - _CountLeadingZeros(g_pMdio->ALIVE));

    return phyAddr;
}

DWORD PhyReadIds()
{
    UINT16 id1;
    UINT16 id2;
    if (!PhyReadReg(active_phy_addr, PHY_PHYIDR1, &id1)) 
    {
        return 0;
    }
    if (!PhyReadReg(active_phy_addr, PHY_PHYIDR2, &id2)) 
    {
        return 0;
    }
    return (id1 << 16) | id2;
}

void PhySoftReset()
{
    UINT16 tmp;
    PhyWriteReg(active_phy_addr, PHY_BMCR, (1 << 15));
    do
    {
        PhyReadReg(active_phy_addr, PHY_BMCR , &tmp);

    }while (tmp & (1 << 15));	
}


static BOOL LinkIsOK()
{
    UINT16	tmp;

    if (PhyReadReg(active_phy_addr, PHY_BMSR, &tmp) && (tmp & 0x04)) 
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL GetLinkStatus(DWORD *pSpeed,BOOL *pFullDuplex)
{
    UINT16	anlpar;

    if (LinkIsOK() == FALSE)
        return FALSE;

    if (PhyReadReg(active_phy_addr,PHY_ANLPAR,&anlpar) == FALSE)
        return FALSE;

    if (pFullDuplex)
    {
        if (anlpar & (GEN_PHY_ANEG_100DUP | GEN_PHY_ANEG_10DUP ) ) 
        {
            *pFullDuplex = TRUE;			
        } 
        else 
        {
            *pFullDuplex = FALSE;
        }
    }

    if (pSpeed)
    {
        if (anlpar & (GEN_PHY_ANEG_100DUP | GEN_PHY_ANEG_100TX ) ) 
        {
            *pSpeed = SPEED_100MBPS;

        } else 
        {
            *pSpeed = SPEED_10MBPS;
        }
    }
    return TRUE;
}


static int PhyAutoNegociate()
{
    UINT16	tmp,val;
    unsigned long cntr =0;
    if (!PhyReadReg(active_phy_addr, PHY_BMCR, &tmp))
    {
        return(0);
    }

    val = tmp | GEN_PHY_CTRL_DUP | GEN_PHY_CTRL_ENA_ANEG | GEN_PHY_CTRL_SPD_SEL ;
    PhyWriteReg(active_phy_addr, PHY_BMCR, val);
    PhyReadReg(active_phy_addr, PHY_BMCR, &val);

    PhyReadReg(active_phy_addr,PHY_ANAR, &val);
    val |= ( GEN_PHY_ANEG_100DUP | GEN_PHY_ANEG_100TX | GEN_PHY_ANEG_10DUP | GEN_PHY_ANEG_10TX );
    PhyWriteReg(active_phy_addr, PHY_ANAR, val);
    PhyReadReg(active_phy_addr,PHY_ANAR, &val);


    PhyReadReg(active_phy_addr, PHY_BMCR, &tmp);

    /* Restart Auto_negotiation  */
    tmp |= PHY_BMCR_RST_NEG;
    PhyWriteReg(active_phy_addr, PHY_BMCR, tmp);

    /*check AutoNegotiate complete */
    do{
        OALStall(10000);
        cntr++;

        if (PhyReadReg(active_phy_addr, PHY_BMSR, &tmp)){
            if(tmp & PHY_BMSR_AUTN_COMP)
            {
                //autoneg complete
                break;
            }
        }
    }while(cntr < 200 );

    if (!PhyReadReg(active_phy_addr, PHY_BMSR, &tmp))
        return(0);

    PhyReadReg(active_phy_addr,PHY_BMCR,&val);

    PhyReadReg(active_phy_addr,PHY_ANAR,&val);

    PhyReadReg(active_phy_addr,PHY_ANLPAR,&val);

    PhyReadReg(active_phy_addr,PHY_ANER,&val);


    if (!(tmp & PHY_BMSR_AUTN_COMP))
        return(0);

    return(LinkIsOK());
}


BOOL EMACInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    DWORD phyID;
    REG32* addr;
    int i;
    BYTE* byteMac = (BYTE*) mac;
    DWORD oldMac[2];
    EMAC_DESC* rx_desc;
    DWORD speed;
    BOOL fullduplex;

    UNREFERENCED_PARAMETER(offset);

    if ((DWORD) pAddress < 0x80000000)
    {
        pAddress = OALPAtoUA((DWORD)pAddress);
    }

    g_pEmacMod	= (EMAC_MODULE_REGS *)	(pAddress + 0x10000);
    g_pMdio		= (EMAC_MDIO_REGS *)	(pAddress + 0x30000);
    g_pEmacSys	= (EMAC_SUBSYS_REGS *)	(pAddress);
    startOfCPPIArea = (pAddress + 0x20000);
    emac_rx_desc = (EMAC_DESC*)	(startOfCPPIArea);
    emac_tx_desc = (EMAC_DESC*)	(startOfCPPIArea + sizeof(EMAC_DESC)*EMAC_MAX_RX_BUFFERS);
    memset(startOfCPPIArea,0,8*1024);

    g_pEmacMod->MACINDEX = 0;
    oldMac[0] = g_pEmacMod->MACSRCADDRLO;
    oldMac[1] = g_pEmacMod->MACSRCADDRHI;

    if ((oldMac[0]!=0)||(oldMac[1]!=0))
    {
        byteMac[0]=(BYTE) (oldMac[1] >> 0);
        byteMac[1]=(BYTE) (oldMac[1] >> 8);
        byteMac[2]=(BYTE) (oldMac[1] >> 16);
        byteMac[3]=(BYTE) (oldMac[1] >> 24);
        byteMac[4]=(BYTE) (oldMac[0] >> 0);
        byteMac[5]=(BYTE) (oldMac[0] >> 8);
    }

    active_phy_addr = PhyInit();
    if (active_phy_addr == -1)
    {
        return FALSE;
    }
    phyID = PhyReadIds();

    switch (phyID) {
        default:
            break;
    }

    if (LinkIsOK() == FALSE)
    {
        /* Soft reset the PHY */
        PhySoftReset();
    }

    /* Reset EMAC module and disable interrupts in wrapper */
    g_pEmacMod->SOFTRESET = 1;
    while (g_pEmacMod->SOFTRESET != 0);
    g_pEmacSys->SOFTRESET = 1;
    while (g_pEmacSys->SOFTRESET != 0);

    g_pEmacSys->C0RXEN = g_pEmacSys->C1RXEN = g_pEmacSys->C2RXEN = 0;
    g_pEmacSys->C0TXEN = g_pEmacSys->C1TXEN = g_pEmacSys->C2TXEN = 0;
    g_pEmacSys->C0MISCEN = g_pEmacSys->C1MISCEN = g_pEmacSys->C2MISCEN = 0;

    g_pEmacMod->TXCONTROL = 0x01;
    g_pEmacMod->RXCONTROL = 0x01;

    /* Set MAC Addresses & Init multicast Hash to 0 (disable any multicast receive) */
    /* Using channel 0 only - other channels are disabled */
    for (i = 0; i < 8; i++) {
        g_pEmacMod->MACINDEX = i;
        g_pEmacMod->MACADDRLO =	(byteMac[5] << 8) |(byteMac[4] << 0) | (1 << 19) | (1 << 20); /* bits 8-0 */;
        g_pEmacMod->MACADDRHI = (byteMac[3] << 24) |(byteMac[2] << 16) | 
            (byteMac[1] << 8)  |(byteMac[0] << 0);
    }

    g_pEmacMod->MACHASH1 = 0;
    g_pEmacMod->MACHASH2 = 0;

    /* Set source MAC address - REQUIRED for pause frames */
    g_pEmacMod->MACSRCADDRLO=	(byteMac[5] << 8) |(byteMac[4] << 0);
    g_pEmacMod->MACSRCADDRHI = (byteMac[3] << 24) |(byteMac[2] << 16) | 
        (byteMac[1] << 8)  |(byteMac[0] << 0);


    /* Set DMA 8 TX / 8 RX Head pointers to 0 */
    addr = &g_pEmacMod->TX0HDP;
    for(i = 0; i < 16; i++)
        *addr++ = 0;

    addr = &g_pEmacMod->RX0HDP;
    for(i = 0; i < 16; i++)
        *addr++ = 0;

    /* Clear Statistics (do this before setting MacControl register) */
    addr = &g_pEmacMod->RXGOODFRAMES;
    for(i = 0; i < EMAC_STATS_REGS; i++)
        *addr++ = 0;

    g_pEmacMod->MACHASH1 = 0;
    g_pEmacMod->MACHASH2 = 0;



    /* Create RX queue and set receive process in place */
    rx_desc = emac_rx_desc;
    emac_rx_active_head = emac_rx_desc;
    for (i = 0; i < EMAC_MAX_RX_BUFFERS; i++) {
        if (i != (EMAC_MAX_RX_BUFFERS -1))
        {
            rx_desc->next = BD_TO_HW( (UINT32)(rx_desc + 1) );
        }
        else
        {
            emac_rx_active_tail = rx_desc;
            /* Set the last descriptor's "next" parameter to 0 to end the RX desc list */
            rx_desc->next = 0;
        }
        rx_desc->buffer = OALVAtoPA(dmaBuffer + (i * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)));
        rx_desc->BufOffLen = EMAC_MAX_ETHERNET_PKT_SIZE;
        rx_desc->PktFlgLen = EMAC_DSC_FLAG_OWNER;
        rx_desc++;
    }



    /* Enable TX/RX */
    g_pEmacMod->RXMAXLEN = EMAC_MAX_ETHERNET_PKT_SIZE;
    g_pEmacMod->RXBUFFEROFFSET = 0;

    /* No fancy configs - Use this for promiscous for debug - EMAC_RXMBPENABLE_RXCAFEN_ENABLE */
    g_pEmacMod->RXMBPENABLE = EMAC_RXMBPENABLE_RXBROADEN;

    /* Enable ch 0 only */
    g_pEmacMod->RXUNICASTSET = 0x01;

    /* Clear all unused RX channel interrupts */
    g_pEmacMod->RXINTMASKCLEAR = 0xFF;

    /* Enable the Rx channel interrupt mask registers.
     * Setting channel0   RX channel interrupts
     */
    g_pEmacMod->RXINTMASKSET = 0x1;

    /* Enable MII interface and Full duplex mode */

    EnableMDIO();
    if (LinkIsOK() == FALSE)
    {
        /* Init MDIO & get link state */
        if (!PhyAutoNegociate())
        {
            RETAILMSG(1,(TEXT("Autonegociation failed. Check the ethernet cable\r\n")));
            return FALSE;
        }
    }

    if (GetLinkStatus(&speed,&fullduplex) == FALSE)
    {
        return FALSE;
    }
    g_pEmacMod->MACCONTROL = EMAC_MACCONTROL_MIIEN_ENABLE |
        (fullduplex ? EMAC_MACCONTROL_FULLDUPLEX_ENABLE : 0) |
        ((speed == SPEED_100MBPS) ? EMAC_MACCONTROL_RMII_SPEED : 0);

    RETAILMSG(1,(TEXT("Link : %s-duplex %d MBits/s\r\n"),fullduplex?L"Full":L"Half",speed));

    /* Start receive process */
    g_pEmacMod->RX0HDP = BD_TO_HW((UINT32)emac_rx_desc);


    return TRUE;
}

void DumpDesc(EMAC_DESC* pDdesc)
{
	UNREFERENCED_PARAMETER(pDdesc);

    RETAILMSG(1,(TEXT("descriptor 0x%x\r\n"),pDdesc));
    RETAILMSG(1,(TEXT("next 0x%x\r\n"),pDdesc->next));
    RETAILMSG(1,(TEXT("buffer 0x%x\r\n"),pDdesc->buffer));
    RETAILMSG(1,(TEXT("BufOffLen 0x%x\r\n"),pDdesc->BufOffLen));
    RETAILMSG(1,(TEXT("PktFlgLen 0x%x\r\n"),pDdesc->PktFlgLen));
}
UINT16 EMACSendFrame(UCHAR *src_buffer, ULONG length)
{
    int tx_send_loop = 0;
    UCHAR* buffer = (dmaBuffer + (EMAC_MAX_RX_BUFFERS * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)));
    //	RETAILMSG(1,(TEXT("sending %d bytes (0x%x)(0x%x)\r\n"),length,src_buffer,OALVAtoPA((UINT8 *) src_buffer)));

    /* Check packet size and if < EMAC_MIN_ETHERNET_PKT_SIZE, pad it up */
    if (length < EMAC_MIN_ETHERNET_PKT_SIZE) 
    {
        length = EMAC_MIN_ETHERNET_PKT_SIZE;
    }

    memcpy(buffer,src_buffer,length);
    /* Populate the TX descriptor */
    emac_tx_desc->next = 0;
    emac_tx_desc->buffer = OALVAtoPA((UINT8 *) buffer);
    emac_tx_desc->BufOffLen = (length & 0xffff);
    emac_tx_desc->PktFlgLen = ((length & 0xffff) |
        EMAC_DSC_FLAG_SOP |
        EMAC_DSC_FLAG_OWNER |
        EMAC_DSC_FLAG_EOP);

    //	DumpDesc(emac_tx_desc);


    /* Send the packet */
    g_pEmacMod->TX0HDP = BD_TO_HW((unsigned int) emac_tx_desc);

    /* Wait for packet to complete or link down */
    while (!(g_pEmacMod->TXINTSTATRAW & 0x01))
    {
        tx_send_loop++;
    }

    return 0;
}

UINT16 EMACGetFrame(UCHAR *dest_buffer, USHORT *length)
{
    EMAC_DESC *rx_curr_desc;
    EMAC_DESC *curr_desc;
    EMAC_DESC *tail_desc;
    int status, ret = -1;
    static int			emac_rx_queue_active = 1;


    *length = 0;
    rx_curr_desc = emac_rx_active_head;
    status = rx_curr_desc->PktFlgLen;
    if ((rx_curr_desc) && ((status & EMAC_DSC_FLAG_OWNER) == 0)) 
    {
        if (status & EMAC_DSC_RX_ERROR_FRAME) 
        {
            /* Error in packet - discard it and requeue desc */
            RETAILMSG (1, (L"WARN: emac_rcv_pkt: Error in packet\n"));
        } else 
        {
            ret = rx_curr_desc->BufOffLen & 0xffff;
            // packet is good...copy to input buffer.
            *length = (USHORT)(rx_curr_desc->BufOffLen & 0xffff);
            memcpy(dest_buffer, OALPAtoUA(rx_curr_desc->buffer), *length);
            //RETAILMSG(1,(TEXT("received 1 frame : %d  bytes\r\n"), *length));
            //			dumpPacket(dest_buffer, *length);
        }

        /* Ack received packet descriptor */
        g_pEmacMod->RX0CP = BD_TO_HW((unsigned int) rx_curr_desc);
        curr_desc = rx_curr_desc;
        emac_rx_active_head =
            (EMAC_DESC *) (HW_TO_BD(rx_curr_desc->next));


        if (status & EMAC_DSC_FLAG_EOQ) 
        {
            if (emac_rx_active_head) 
            {
                g_pEmacMod->RX0HDP =
                    BD_TO_HW((unsigned int) emac_rx_active_head);
            } 
            else 
            {
                emac_rx_queue_active = 0;
            }
        }

        /* Recycle RX descriptor */
        rx_curr_desc->BufOffLen = EMAC_MAX_ETHERNET_PKT_SIZE;
        rx_curr_desc->PktFlgLen = EMAC_DSC_FLAG_OWNER;
        rx_curr_desc->next = 0;

        if (emac_rx_active_head == 0) 
        {
            emac_rx_active_head = curr_desc;
            emac_rx_active_tail = curr_desc;
            if (emac_rx_queue_active == 0) 
            {
                g_pEmacMod->RX0HDP =
                    BD_TO_HW((unsigned int) emac_rx_active_head);
                emac_rx_queue_active = 1;
            }
        } 
        else 
        {
            tail_desc = emac_rx_active_tail;
            emac_rx_active_tail = curr_desc;
            tail_desc->next = BD_TO_HW((unsigned int) curr_desc);
            status = tail_desc->PktFlgLen;
            if (status & EMAC_DSC_FLAG_EOQ) 
            {
                status &= ~EMAC_DSC_FLAG_EOQ;
                tail_desc->PktFlgLen = status;
                g_pEmacMod->RX0HDP = BD_TO_HW((unsigned int) curr_desc);
            }
        }
    }

	SocAckInterrupt(0xF);
	g_pEmacMod->MACEOIVECTOR = 0x1;

    return  *length;
}



BOOL EMACInitDMABuffer(UINT32 address, UINT32 size)
{
    dmaBuffer = (UINT8*) address;
    if (size < ((EMAC_MAX_RX_BUFFERS + EMAC_MAX_TX_BUFFERS) * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)))
    {
        return FALSE;
    }
    return TRUE;
}
