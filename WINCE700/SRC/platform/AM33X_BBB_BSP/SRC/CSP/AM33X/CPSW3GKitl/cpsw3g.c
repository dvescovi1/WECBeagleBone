//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include <windows.h>
#include <oal_memory.h>

#include <oal.h>
#if  (_WINCEOSVER<=700)
#include <types.h>
#else
#include <cetypes.h>
#endif
#include <ethdbg.h>
#include "am33x_base_regs.h"
#include "cpsw3g.h"

#define CONFIG_PHY_GIGE

#define HOST_PORT         0
#define MAC_PORT_START    1
#define MAC_PORT_END      2
#define MAC_PORT          MAC_PORT_START

#define RESET_LOOP_COUNT 1000000

/* Global Varible */
CpswCb g_cpsw3gCb;
int    g_cpsw3g_int_enable = 0;
UINT32 g_cpsw3g_reg_base   = 0;

char *rx_buf = (char *)CPSW3G_BUF_BASE;
char *tx_buf;
UINT32 tx_bd_base;
UINT32 rx_bd_base;

BuffDesc *curr_first_bd_pa = NULL;
BuffDesc *curr_last_bd = NULL;
BuffDesc *last_bd = NULL;

#define CPSW3G_DELAY(ms)                           \
{                                                  \
    UINT32 s;                                      \
    s = OALGetTickCount();                         \
    while ((OALGetTickCount() - s) < (ms)) { }     \
}

BOOL Cpsw3g_get_phy_link_state(UINT32 port);
void Phy_init(UINT16 addr);

#define DEVICE_ID  (AM387X_L4_CNTRL_MODULE_PA + 0x0600)

/******************************************
 * get_cpu_rev(void) - extract rev info
 ******************************************/
UINT32 get_cpu_rev(UINT32 *pID)
{
    return 1;
}


/* Utility functions */
UINT32 Cpsw3g_Read_Register(volatile UINT32 *addr)
{
        return *(addr);
}

void Cpsw3g_Write_Register(volatile UINT32 *addr, UINT32 value)
{
        *(addr) = value;
}
void Cpsw3g_Register_SetBit(volatile UINT32 *addr, UINT32 bit)
{
    *(addr) |= (0x1<<bit);
}
void Cpsw3g_Register_ClearBit(volatile UINT32 *addr, UINT32 bit)
{
    *(addr) &= (0x1<<bit);
}

/*------------------------------------------------------------------------------
*
*  Function:  SetALEUnicastEntry
*
*  This helper function is called to add ALE Unicast Entry. 
*
*  It first prepares the 3 table words that together form a table entry.
*  It then initiate a write to the entry indicated by the index.
*--------------------------------------------------------------------------------*/
void SetALEUnicastEntry(int index, int VLANId, unsigned int UniAddrHigh, unsigned int UniAddrLow, int portNumber)
{
    unsigned int entry;

    /* Word 0: addr low[31:0] =UniAddrLow */
    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW0, UniAddrLow);

    if(VLANId)
    {
        /* Word 1: [63:32]
            ucast type [63:62] =00b (not ageable)
            entity type[61:60] =11b (this is an vlan addr entry)
            vlan id    [59:48] =VLANId
            addr high  [47:32] =UniAddrHigh
        */
        entry = (0x30000000 | (VLANId << 16) | UniAddrHigh);
        Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW1, entry);
    }
    else
    {
        /* Word 1: [63:32]
            ucast type [63:62] =00b (not ageable)
            entity type[61:60] =01b (this is an addr entry)
            resv       [59:48]
            addr high  [47:32] =UniAddrHigh
        */
        entry = (0x10000000 | UniAddrHigh);
        Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW1, entry);
    }

    /* Word 3: [71:64]
        port#[67:66]=host port number
        block   [65]=0
        secure  [64]=0    (linux secu=1)
    */
    entry = (portNumber << 2);
    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW2, entry);

    /* Write the entry to the ALE table */
    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLCTL , (0x80000000 | index));
}

/*------------------------------------------------------------------------------
*
*  Function:  SetALEMulticastEntry
*
*  This helper function is called to add ALE Multicast Entry. 
*
*--------------------------------------------------------------------------------*/
void SetALEMulticastEntry(int index, int VLANId, unsigned int AddrHigh, unsigned int AddrLow, int portMask, int Super)
{
    unsigned int entry;

    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW0, AddrLow);
    if(VLANId)
    {
        entry = AddrHigh | 0xF0000000  |(VLANId << 16);
        Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW1, entry);
    }
    else
    {
        entry = AddrHigh | 0xD0000000;
        Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW1, entry);
    }
    entry = portMask | (Super <<3);
    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLW2, entry);

    /* Write the entry to the ALE */
    Cpsw3g_Write_Register((UINT32 *)CPALE_TBLCTL , (0x80000000 | index));
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3g_ale_init
*
*  This helper function is called to intialize ALE module. 
*
*--------------------------------------------------------------------------------*/
void Cpsw3g_ale_init(UINT16 MacAddr[3])
{
    UINT32 i;

    /* Init the ALE registers and clear ALE */
    Cpsw3g_Write_Register((UINT32 *)CPALE_CTRL, 0xC0000000);  // enable ALE (bit 31), clear table (bit 30)
    for(i=0; i<1000000 /*1000*/;i++);  // +++FIXME
    Cpsw3g_Write_Register((UINT32 *)CPALE_CTRL, 0x80000000);  // ALE mode enable, vlan unaware

//    Cpsw3g_Write_Register((UINT32 *)CPALE_UNK_VLAN, 0x07040404);
//    Cpsw3g_Write_Register((UINT32 *)CPALE_PRESCALE, 0x000FFFFF);  // input clk is divided by this for use
                                                                    // in mcast/bcast rate limiters

    /* Set MAC Address */
    SetALEUnicastEntry(
      0,                                                          // index (table)
      0x000,                                                      // VLANId
      (MacAddr[0] & 0xff)<<8 | (MacAddr[0] & 0xff00)>>8,          // UniAddrHigh
      ((MacAddr[1] & 0xff)  << 24 | (MacAddr[1] & 0xff00) << 8 |
       (MacAddr[2] & 0xff00) >> 8 | (MacAddr[2] & 0xff)   << 8 ), // UniAddrLow
      HOST_PORT                                                   // portNumber (host port)
    );
}

#if 0
/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3g_rgmii_dump
*
*  This helper function is called to dump RGMII registers. 
*
*--------------------------------------------------------------------------------*/
void  Cpsw3g_rgmii_dump(pvtp_ctrl   pVtp)
{
    RETAILMSG(TRUE, (L"Cpsw3g_rgmii_dump for port %d.\r\n",
                                   ((pVtp == (pvtp_ctrl)CPSW_VTP_CTRL0_BASE) ? 0 : 1)));

    RETAILMSG(TRUE, (L"    pid:%x \r\n", pVtp->pid));
    RETAILMSG(TRUE, (L"    mode:%x \r\n", pVtp->mode));
    RETAILMSG(TRUE, (L"    wdt:%x \r\n", pVtp->wdt));
    RETAILMSG(TRUE, (L"    np:%x \r\n", pVtp->np));	
    RETAILMSG(TRUE, (L"    control:%x \r\n", pVtp->ctrl));
    RETAILMSG(TRUE, (L"    start:%x \r\n", pVtp->start));
}
#endif

/*------------------------------------------------------------------------------
*
*  Function:  cpsw3g_rgmii_init
*
*  This helper function is called to init RGMII module. 
*
*--------------------------------------------------------------------------------*/
void  cpsw3g_rgmii_init(void)
{
}

/*------------------------------------------------------------------------------
*
*  Function:  BoardSoftReboot
*
*  This function is used to reboot the board in case servious problem happens. 
*
*--------------------------------------------------------------------------------*/
void BoardSoftReboot(void)
{
}


/*------------------------------------------------------------------------------
*
*  Function:  cpsw3g_module_softreset
*
*  This function is a generic funciton to perform ETHSS sub-module soft reset. 
*
*--------------------------------------------------------------------------------*/
BOOL cpsw3g_module_softreset(UINT32 Reg)
{
    UINT32 timeout=0;
    
    Cpsw3g_Register_SetBit((UINT32 *)Reg, 0);

    /* the total time out value is 100ms */
    while(Cpsw3g_Read_Register((UINT32 *)Reg) & 0x1 && timeout < RESET_LOOP_COUNT)  // +++FIXME
    {
        timeout++;
        OALStall(5000);        
    }
    if(Cpsw3g_Read_Register((UINT32 *)Reg) & 0x1 || timeout >= RESET_LOOP_COUNT)
    {
        OALMSGS(OAL_ERROR, ( 
            L"+cpsw3g_module_softreset(reg:%x) failed in %d iterations , board will be soft reset\r\n", 
            Reg, timeout));

#if 0
        BoardSoftReboot(); 
#endif

        return FALSE;
    }
    return TRUE;
}


static void Cpsw3g_slave_init(PCpswCb pCpsw3gCb, UINT32 slave_port)
{
    cpsw3g_module_softreset(CPMAC_MAC_SRST(slave_port));

	/* setup priority mapping */
    Cpsw3g_Write_Register((UINT32 *)CPMAC_MAC_RX_PRI_MAP(slave_port), 0x76543210);	
    Cpsw3g_Write_Register((UINT32 *)CPSW_SL_TX_PRI_MAP(slave_port), 0x33221100);	

    Cpsw3g_Write_Register((UINT32 *)CPSW_SL_MAX_BLK(slave_port), 0xE6);	

	/* setup max packet size */
    Cpsw3g_Write_Register((UINT32 *)CPMAC_MAC_RXMAX_LEN(slave_port), PKT_MAX);	

	/* setup mac address */
    Cpsw3g_Write_Register((UINT32 *)CPSW_SL_SRC_ADDR_LOW(slave_port), pCpsw3gCb->macAddrLo);
    Cpsw3g_Write_Register((UINT32 *)CPSW_SL_SRC_ADDR_HI(slave_port),  pCpsw3gCb->macAddrHi);
    pCpsw3gCb->tx_port_bit = (CPSW3G_CPPI_TO_PORT_BIT << (slave_port - 1));  // send to port n

	pCpsw3gCb->mac_control = 0;	/* no link yet */

	/* set slave port ALE port control: forward port state + no learn */
    // +++FIXME: read first?
    Cpsw3g_Write_Register((UINT32 *)(CPALE_PORT_CTRL_0 + 4 * slave_port), 
                          (CPMAC_PORT_FORWARD | CPMAC_NOLEARN));

//	cpsw_ale_add_mcast(priv, NetBcastAddr, 1 << slave_port);

    Phy_init(pCpsw3gCb->phy_id);
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gInit
*
*  This function is called to intialize CPSW3G hardware. 
*
*--------------------------------------------------------------------------------*/
UINT32 tick_start, tick_end;
BOOL Cpsw3gInit (UINT8 *pBaseAddress, UINT32 offset, UINT16 MacAddr[3])
{
    PCpswCb pCpsw3gCb=&g_cpsw3gCb;
    UINT32 i, startTime;
    UINT32 mac_port = MAC_PORT;  // 1-based on centaurus

    UNREFERENCED_PARAMETER(offset);

    OALMSGS(1, ( L"+Cpsw3gInit(0x%08p, 0x%08x, 0x%08x) v0.3 \r\n", pBaseAddress, mac_port, MacAddr ));

    /* check param */
    if(!pBaseAddress)
    {
        OALMSGS(OAL_ERROR, (L"+Cpsw3gInit-- Base Address is NULL.\r\n"));
        return FALSE;
    }

    /********************************************************************
    **  Save data
    *********************************************************************/

    /* Save Mac Address 2 ports, in bootloader use the one for WAN */
    pCpsw3gCb->macAddrHi = MacAddr[0] + (MacAddr[1]<<16);
    pCpsw3gCb->macAddrLo = MacAddr[2];

    pCpsw3gCb->mac_port = mac_port;

    if (get_cpu_rev(NULL))
    {
        if (mac_port == 1)
            pCpsw3gCb->phy_id =  EXTERNAL_PHY0_ADDR;
        else
            pCpsw3gCb->phy_id =  EXTERNAL_PHY1_ADDR;
    }
    else
    {
        OALMSGS(OAL_ERROR, (L"Cpsw3gInit-- Unknown cpu rev\r\n"));
        return FALSE;
    }

    /* set register base */
    g_cpsw3g_reg_base = (UINT32)OALPAtoUA((UINT32)pBaseAddress);

    pCpsw3gCb->rx_buf_pa = (UINT32)CPSW3G_BUF_BASE;

    pCpsw3gCb->tx_buf_pa = pCpsw3gCb->rx_buf_pa  + 
                               RX_BD_COUNT * RX_BUFF_SZ;

    pCpsw3gCb->tx_bd_pa = pCpsw3gCb->tx_buf_pa +
                               TX_BD_COUNT * RX_BUFF_SZ;

    pCpsw3gCb->rx_bd_pa = pCpsw3gCb->tx_bd_pa + 
                               (TX_BD_COUNT+1) * CPSW3G_TX_BD_ALIGN ;
    
    rx_buf     = (char *)OALPAtoUA(CPSW3G_BUF_BASE);
    tx_buf     = (char *)OALPAtoUA(pCpsw3gCb->tx_buf_pa);
    tx_bd_base = (UINT32)OALPAtoUA(pCpsw3gCb->tx_bd_pa);
    rx_bd_base = (UINT32)OALPAtoUA(pCpsw3gCb->rx_bd_pa);

    /********************************************************************
    **  Reset switch controller and all 3 ports
    *********************************************************************/
    OALMSGS(1, ( L"g_cpsw3g_reg_base = 0x%08x \r\n", g_cpsw3g_reg_base ));
    if(cpsw3g_module_softreset(CPSW_SRST) == TRUE)
    {
        //OALMSG(OAL_INFO, (TEXT("Cpsw3gInit, cpsw reset done. \r\n")));
    }
    else
    {
        OALMSG(OAL_ERROR, (TEXT("Cpsw3gInit, cpsw reset failed. \r\n")));
        return FALSE;
    }

    if(cpsw3g_module_softreset(CPDMA_SRST) == TRUE)
    {
        //OALMSG(OAL_INFO, (TEXT("Cpsw3gInit, cpdma reset done. \r\n")));
    }
    else
    {
        OALMSG(OAL_ERROR, (TEXT("Cpsw3gInit, cpdma reset failed. \r\n")));
        return FALSE;
    }
    
    for(i=MAC_PORT_START; i<= MAC_PORT_END; i++)
    {
        if(cpsw3g_module_softreset(CPMAC_MAC_SRST(i)) == TRUE)
        {
            //OALMSG(OAL_INFO, (TEXT("Cpsw3gInit, cpmac %d reset done. \r\n"), i));
        }
        else
        {
            OALMSG(OAL_ERROR, (TEXT("Cpsw3gInit, cpmac %d reset failed. \r\n"), i));
            return FALSE;
        }
    }

    /********************************************************************
    **  Configure switch
    *********************************************************************/
    MdioEnable();

#if 0
    /* configure Ethernet Rgmii */
    cpsw3g_rgmii_init();    
    //OALMSG(OAL_ERROR, (TEXT("Cpsw3gInit, finished rgmii init. \r\n")));
#endif

	/* enable statistics on all ports */
    Cpsw3g_Write_Register((UINT32 *)CPSW_STAT_PORT_EN, 0x07);

	/* disable priority elevation on all ports */
    Cpsw3g_Write_Register((UINT32 *)CPSW_PRI_TYPE, 0x00);	

    /* Set the Switch Control registers, VLAN unaware */
    Cpsw3g_Write_Register((UINT32 *)CPSW_CTRL, 0 /*0x000002f4*/);

    Cpsw3g_ale_init(MacAddr);

    /* setup host port priority mapping */
    Cpsw3g_Write_Register((UINT32 *)CPDMA_TX_PRI_MAP, 0x76543210);
    Cpsw3g_Write_Register((UINT32 *)CPDMA_RX_CH_MAP, 0);

    // set host port ALE port control: forward port state
    Cpsw3g_Write_Register((UINT32 *)(CPALE_PORT_CTRL_0 + (4 * HOST_PORT)), CPMAC_PORT_FORWARD);

    // slave init and phy init
    Cpsw3g_slave_init(pCpsw3gCb, mac_port);

    /* Clear out the STAT registers */
    for(i=0; i<0x90; i+=4)
        Cpsw3g_Write_Register((UINT32 *)(CPSW_STATS_BASE+i) , 0x00000000);

    Cpsw3g_Register_SetBit((UINT32 *)CPDMA_DMA_CTRL, 0); 

    for(i=MAC_PORT_START; i<=MAC_PORT_END; i++)
    {
        Cpsw3g_Write_Register((UINT32 *)CPMAC_MAC_RX_PRI_MAP(i) , 
                              KITL_CHANNEL<<0  | KITL_CHANNEL<<4  | 
                              KITL_CHANNEL<<8  | KITL_CHANNEL<<12 | 
                              KITL_CHANNEL<<16 | KITL_CHANNEL<<20);
    }

    /* Initialize CPDMA StateRam */	
    for (i=0;i<8;i++)
    {
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_TX_DMAHDP(i), 0x00);
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMAHDP(i), 0x00);
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_TX_DMACP(i),  0x00);
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMACP(i),  0x00);
    }
       
    /*
    * Initialize the TX/RX buffer descriptors (uncached addr).
    */
    pCpsw3gCb->rx = (BuffDesc*)rx_bd_base;          
    pCpsw3gCb->tx = (BuffDesc*)tx_bd_base;          

    /*
    * Initialize pCpsw3gCb, as required, eg link_state, rx_next, et al.
    */
    pCpsw3gCb->link_state = LK_DN;
    pCpsw3gCb->rx_next    = pCpsw3gCb->rx;
    pCpsw3gCb->rx_next_pa = (BuffDesc *)pCpsw3gCb->rx_bd_pa;

    if (TRUE != cpsw3g_rx_bd_init(pCpsw3gCb->rx)) {
        return FALSE;
    }

    if (TRUE != cpsw3g_tx_bd_init(pCpsw3gCb->tx)) {
        return FALSE;
    }
    
    // Host port
    Cpsw3g_Write_Register((UINT32 *)CPDMA_RX_CH_MAP, 
                          (KITL_CHANNEL<<0  | KITL_CHANNEL<<4  | 
                           KITL_CHANNEL<<8  | KITL_CHANNEL<<12 | 
                           KITL_CHANNEL<<16 | KITL_CHANNEL<<20) );

    // Host port
    Cpsw3g_Write_Register((UINT32 *)CPSW_P0_TX_PRI_MAP, 0x76543210);
    
    /* Enable DMA TX/RX in Control registers. */
    Cpsw3g_Write_Register((UINT32 *)CPDMA_TX_CTRL, 1);
    Cpsw3g_Write_Register((UINT32 *)CPDMA_RX_CTRL, 1);

    OALMSGS(OAL_ERROR, (L"Cpsw3gInit, wait link up on mac port:%d.\r\n",pCpsw3gCb->mac_port));
    startTime = OALGetTickCount();
    while ((OALGetTickCount() - startTime) < LINK_TIMEOUT_VALUE)
    {
        if ((Phy_link_state()) == LK_UP) {
            break;
        }
    }

    if( pCpsw3gCb->link_state != LK_UP)
    {
        OALMSGS(OAL_ERROR, (L"Cpsw3gInit, LINK down on port:%d.\r\n",pCpsw3gCb->mac_port));
    }

    OALMSGS(1, (L"-Cpsw3gInit\r\n"));
    return TRUE;
}

/*------------------------------------------------------------------------------
*
*  Function:  dump_packet
*
*  This is helper function to dump packet for debugging purpose. 
*
*--------------------------------------------------------------------------------*/
void dump_packet(UINT8 *addr, UINT16 len)
{
    UINT32 i;
    OALMSGS(OAL_ERROR,(L"packet dump\r\n"));

    for(i=0; i<(len);i++)
    {
        if(!(i%16)) OALMSGS(OAL_ERROR, (L"\r\n 0x%x : ", addr + i));
        OALMSGS(OAL_ERROR,(L"%x ", *(addr+i) ));
    }
    OALMSGS(OAL_ERROR, (L"\r\n"));
    OALMSGS(OAL_ERROR, (L"\r\n"));       
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3g_handlepacket
*
*  This is helper function for packet receiving. 
*
*--------------------------------------------------------------------------------*/
UINT16 Cpsw3g_handlepacket	(UINT8 *pBuffer, BuffDesc *rx_bd)
{
    PCpswCb pCpsw3gCb=&g_cpsw3gCb;
    UINT16  length=0;
    UINT32 temp;

    temp = (UINT32)OALPAtoUA((DWORD)(BD_BufPTR(rx_bd) + BD_BufOFFSET(rx_bd)));

    /* Check packet len */
    if(BD_BufLen(rx_bd) != BD_PktLen(rx_bd))
    {
        OALMSGS(OAL_ERROR, (L"\r\nCpsw3gGetFrame: rx packet and buffer size mismatch !"));
        goto error;
    }
    length  = (UINT16)BD_PktLen(rx_bd);
    length = length - 4;

    if(pBuffer)
       memcpy(pBuffer, (UINT8 *)temp, length);

    if (IS_EOQ(rx_bd))
    {
        /* check if the RX DMA HDP is cleared. */
        if (Cpsw3g_Read_Register((UINT32 *)CPMAC_A_RX_DMAHDP(KITL_CHANNEL)) != 0) 
            OALMSGS(OAL_ERROR,(L"\r\nCpsw3gGetFrame: RX DMA HDP is not cleared still !"));

        /* check for next pointer */
        if (rx_bd->next) 
        {
            Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMAHDP(KITL_CHANNEL), (UINT32)rx_bd->next);
        } 
        else
        {
            Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMAHDP(KITL_CHANNEL), (UINT32)curr_first_bd_pa);
    
            /* Set the Free Buffer Count reg, for flow control. */
            Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_FBUFF(KITL_CHANNEL), RX_BD_COUNT);
        }
    } 
    else 
    {
        // update current-first
        curr_first_bd_pa = (pCpsw3gCb->rx_next_pa);
    
        // re-insert rx_bd to the end of queue
        curr_last_bd->next = (struct BUFF_DESC *)(pCpsw3gCb->rx_next_pa);
        rx_bd->next = NULL;
        curr_last_bd = rx_bd;
    
        /* Set the Free Buffer Count reg, for flow control. */
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_FBUFF(KITL_CHANNEL), 1);

        // update the rx_next to either the next or the beginning of original list
        if (pCpsw3gCb->rx_next == last_bd)
        {
            pCpsw3gCb->rx_next    = pCpsw3gCb->rx;
            pCpsw3gCb->rx_next_pa = (BuffDesc *)pCpsw3gCb->rx_bd_pa;
        }
        else
        {
            pCpsw3gCb->rx_next ++; 
            pCpsw3gCb->rx_next_pa ++; 
        }
    }

    rx_bd->BuffOffsetLength = RX_BUFF_SZ & CPSW3G_CPPI_BUFF_LEN_MASK;
    rx_bd->BuffFlagPktLen = CPSW3G_CPPI_OWNERSHIP_BIT;

error:
    return length;
}


/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gGetFrame
*
*  This function is called to receive KTIL frame.
*
*--------------------------------------------------------------------------------*/
UINT16 Cpsw3gGetFrame(UINT8 *pBuffer, UINT16 *pLength)
{
    PCpswCb pCpsw3gCb=&g_cpsw3gCb;
    BuffDesc  *rx_bd , *rx_bd_pa;

    OALMSGS(OAL_FUNC, (L"+Cpsw3gGetFrame\r\n"));

    *pLength = 0;

    if(pCpsw3gCb->link_state != LK_UP)
    {
        if ((Phy_link_state()) != LK_UP) 
        {
            goto error;
        }
    }

    rx_bd = pCpsw3gCb->rx_next; /* rx_next is always uncached. */
    rx_bd_pa = pCpsw3gCb->rx_next_pa;
    
    if (rx_bd == NULL) 
    {
        OALMSGS(OAL_ERROR, (L"\r\nCpsw3gGetFrame: rx_next not updated \r\n!"));
        goto error;
    }

    if(PORT_OWNS(rx_bd))
    {   
        if (g_cpsw3g_int_enable ) 
            Cpsw3g_Write_Register((UINT32 *)CPDMA_EOI_Vector, 0x0);
        /* No pkts pending. */
        goto error;
    }

    // Host owns the rx_next buffer/desc.

    /*
    * Pactket received. Process the packet and pass it to app.
    */
    /* Acknowledge interrupt */
    *pLength = Cpsw3g_handlepacket(pBuffer, rx_bd);

    if (*pLength)
    {
        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMACP(KITL_CHANNEL), (UINT32)rx_bd_pa);
    }

error:    
    OALMSGS(OAL_FUNC, (L"-Cpsw3gGetFrame, len=%d\r\n", *pLength)); 
    return (*pLength);
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gSendFrame
*
*  This function is called to send KTIL frame.
*
*--------------------------------------------------------------------------------*/
UINT16 Cpsw3gSendFrame(UINT8 *pBuffer, UINT32 length)
{
    PCpswCb pCpsw3gCb=&g_cpsw3gCb;
    BuffDesc  *tx_bd;
    UINT32     startTime;
    UINT16     ret =1;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+Cpsw3gSendFrame \r\n")); 

    tx_bd = g_cpsw3gCb.tx;

    if (!(PORT_OWNS(tx_bd))) {

        /* Start the tx process. */
        memcpy(tx_buf, pBuffer, length);

        if (length < CPSW3G_MIN_ETHERNET_PACKET_SIZE) 
        {
            memset(tx_buf + length, 0, CPSW3G_MIN_ETHERNET_PACKET_SIZE-length);
            length = CPSW3G_MIN_ETHERNET_PACKET_SIZE;
        }
        /* Free. Update and send the buffer. */
        tx_bd->BuffPtr = (UINT32 *)pCpsw3gCb->tx_buf_pa;     
        tx_bd->next = (0);  
        tx_bd->BuffOffsetLength = length & CPSW3G_CPPI_BUFF_LEN_MASK;
        tx_bd->BuffFlagPktLen = length & CPSW3G_CPPI_BUFF_LEN_MASK;
        tx_bd->BuffFlagPktLen |= (BD_SOP | BD_EOP | BD_OWNS |
                                               CPSW3G_CPPI_TO_PORT_EN_BIT |
                                               pCpsw3gCb->tx_port_bit);

        Cpsw3g_Write_Register((UINT32 *)CPMAC_A_TX_DMAHDP(KITL_CHANNEL), (UINT32)(pCpsw3gCb->tx_bd_pa));
    } 
    else 
    {
        OALMSGS(OAL_ERROR, (L"\r\nNo free TX BD found !\r\n"));
        return (ret);
    }

    // Wait until it is sent or an error is generated.
    startTime = OALGetTickCount();
    while ((OALGetTickCount() - startTime) < TIMEOUT_VALUE)
    {
        if (!PORT_OWNS(tx_bd)) {
            ret = 0;
            goto Exit;        
        }
    }
    
    if ( (PORT_OWNS(tx_bd)))
    {
        OALMSGS(OAL_ERROR, (L"ERROR: Cpsw3gSendFrame: Timed out waiting for the transfer to complete.\r\n"));
    }

Exit:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-Cpsw3gSendFrame\r\n")); 

    return (ret);
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gEnableInts
*
*  This function is called to Enable KITL interrupt.
*
*--------------------------------------------------------------------------------*/
VOID Cpsw3gEnableInts(void)
{
    OALMSGS(1, (L"+Cpsw3gEnableInts\r\n"));
//    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+Cpsw3gEnableInts\r\n"));
    /* Enable interrupt on Channel 0 */
    Cpsw3g_Write_Register((UINT32 *)CPDMA_RX_IntMask_Set, 0x1<<KITL_CHANNEL);
    g_cpsw3g_int_enable =1 ;
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-Cpsw3gEnableInts\r\n"));
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gDisableInts
*
*  This function is called to disable KITL interrupt.
*
*--------------------------------------------------------------------------------*/
VOID Cpsw3gDisableInts(void)
{
    OALMSGS(1, (L"+Cpsw3gDisableInts\r\n"));
//    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+Cpsw3gDisableInts\r\n"));
    /* Disable interrupt on Channel 0 */	
    Cpsw3g_Write_Register((UINT32 *)CPDMA_RX_IntMask_Clear, 0x1<<KITL_CHANNEL);
    g_cpsw3g_int_enable =0 ;
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-Cpsw3gDisableInts\r\n"));
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gCurrentPacketFilter
*
*--------------------------------------------------------------------------------*/
VOID Cpsw3gCurrentPacketFilter(UINT32 filter)
{
    UINT32 ctrl=0;

    ctrl = Cpsw3g_Read_Register((UINT32 *)CPALE_CTRL);
    
    OALMSGS(1, (L"+Cpsw3gCurrentPacketFilter(0x%08x)\r\n", filter));

//    OALMSGS(OAL_ETHER&&OAL_FUNC, (
//        L"+Cpsw3gCurrentPacketFilter(0x%08x)\r\n", filter));

    if ((filter & PACKET_TYPE_ALL_MULTICAST ) ||
         (filter & PACKET_TYPE_PROMISCUOUS))
        ctrl |= 0x1 << 4; /* ALE bypass*/ 
        
    Cpsw3g_Write_Register((UINT32 *)CPALE_CTRL, ctrl);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-Cpsw3gCurrentPacketFilter\r\n"));
}

/*------------------------------------------------------------------------------
*
*  Function:  Cpsw3gMulticastList
*
*  void SetALEMulticastEntry(int index, int VLANId, unsigned int AddrHigh, unsigned int AddrLow, int portMask, int Super)

*
*--------------------------------------------------------------------------------*/
BOOL Cpsw3gMulticastList(UINT8 *pAddresses, UINT32 count)
{
    UINT32 i;
    
    OALMSGS(1, (L"+Cpsw3gMulticastList(0x%08x, %d)\r\n", pAddresses, count));

//    OALMSGS(OAL_ETHER&&OAL_FUNC, (
//        L"+Cpsw3gMulticastList(0x%08x, %d)\r\n", pAddresses, count));

    for(i=0;i<count;i++)
    {
        SetALEMulticastEntry(i, 0x0, pAddresses[0] <<8 |pAddresses[1],
                                                      pAddresses[2] <<24 |pAddresses[3] <<16 |
                                                      pAddresses[4] <<8 |pAddresses[5],
                                                      0x2, 0);
    }
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-Cpsw3gMulticastList(rc = 1)\r\n"));
    return TRUE;
}

/*------------------------------------------------------------------------------
*
*  Function:  cpsw3g_tx_bd_init
*
*  This function is called to do receive BD initialization.
*
*--------------------------------------------------------------------------------*/
BOOL cpsw3g_tx_bd_init(BuffDesc *tx_bd_head)
{
    UINT32 i;

    for (i = 0; i < TX_BD_COUNT; i++) {
        /* Clear only the OWNS bit for the cpmac_tx to start smoothly. */
        tx_bd_head[i].BuffFlagPktLen &= ~CPSW3G_CPPI_OWNERSHIP_BIT;
    }        

    return TRUE;
}

/*------------------------------------------------------------------------------
*
*  Function:  cpsw3g_rx_bd_init
*
*  This function is called to do receive BD initialization.
*
*--------------------------------------------------------------------------------*/
BOOL cpsw3g_rx_bd_init(BuffDesc *rx_bd_head)
{
    PCpswCb pCpsw3gCb = &g_cpsw3gCb;
    UINT32 i, bd_pa, buf_pa;

    if (rx_bd_head == NULL) 
        return FALSE;

    bd_pa  = pCpsw3gCb->rx_bd_pa + CPSW3G_RX_BD_ALIGN; /* second BD */
    buf_pa = pCpsw3gCb->rx_buf_pa;
    
    for (i = 0; i < RX_BD_COUNT; i++) {
        rx_bd_head[i].next = (i < (RX_BD_COUNT - 1)) ? (struct BUFF_DESC *)bd_pa : (struct BUFF_DESC *)NULL;
        bd_pa += CPSW3G_RX_BD_ALIGN;
        
        if ((rx_bd_head[i].BuffPtr =  (UINT32 *)buf_pa) == NULL) 
            return FALSE;        

        buf_pa +=  RX_BUFF_SZ;

        rx_bd_head[i].BuffOffsetLength = RX_BUFF_SZ & CPSW3G_CPPI_BUFF_LEN_MASK;
        rx_bd_head[i].BuffFlagPktLen = CPSW3G_CPPI_OWNERSHIP_BIT;
    }

    last_bd = &(rx_bd_head[RX_BD_COUNT - 1]);

    curr_first_bd_pa = NULL;
    curr_last_bd = &(rx_bd_head[RX_BD_COUNT - 1]);

    Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_DMAHDP(KITL_CHANNEL), (UINT32)pCpsw3gCb->rx_bd_pa);
    
    /* Set the Free Buffer Count reg, for flow control. */
    Cpsw3g_Write_Register((UINT32 *)CPMAC_A_RX_FBUFF(KITL_CHANNEL), RX_BD_COUNT);

    return TRUE;
}

/*------------------------------------------------------------------------------
*
*  Function:  Phy_link_state
*
*  This function is called to check link state.
*
*--------------------------------------------------------------------------------*/
BOOL Phy_link_state(void)
{
    PCpswCb pCpsw3gCb=&g_cpsw3gCb;

    if((pCpsw3gCb->mac_port < MAC_PORT_START) || (pCpsw3gCb->mac_port > MAC_PORT_END))
    {
        OALMSGS(OAL_ERROR, (L"ERROR: Phy_link_state: Invald active mac port:%d.\r\n",pCpsw3gCb->mac_port));
        return LK_DN;
    }

    pCpsw3gCb->link_state = Cpsw3g_get_phy_link_state(pCpsw3gCb->mac_port);
    return pCpsw3gCb->link_state;
	
}

/*------------------------------------------------------------------------------
*
*  Function:  Phy_init
*
*  This function is called to configure PHY LEDs.
*
*--------------------------------------------------------------------------------*/
void Phy_init(UINT16 addr)
{
	UINT16 val;
	unsigned int   cntr = 0;

#if 0
	/* Enable PHY to clock out TX_CLK */
	MdioRd(addr, PHY_CONF_REG, 0, &val);
	val |= PHY_CONF_TXCLKEN;
	MdioWr(addr, PHY_CONF_REG, 0, val);  // channel 0
	MdioRd(addr, PHY_CONF_REG, 0, &val);
#endif

	/* Enable Autonegotiation */
	if (MdioRd(addr, PHY_BMCR, 0, &val)) {
        OALMSGS(OAL_ERROR, (L"failed to read bmcr\r\n"));
		return;
	}
	val |= (PHY_BMCR_DPLX | PHY_BMCR_AUTON | PHY_BMCR_100_MBPS);
	MdioWr(addr, PHY_BMCR, 0, val);  // channel 0
    MdioRd(addr, PHY_BMCR, 0, &val);

	/* Setup GIG advertisement */
    MdioRd(addr, PHY_1000BTCR, 0, &val);
	val |= PHY_1000BTCR_1000FD;
	val &= ~PHY_1000BTCR_1000HD;
	MdioWr(addr, PHY_1000BTCR, 0, val);  // channel 0
    MdioRd(addr, PHY_1000BTCR, 0, &val);

	/* Setup general advertisement */
	if (MdioRd(addr, PHY_ANAR, 0, &val)) {
        OALMSGS(OAL_ERROR, (L"failed to read anar\r\n"));
		return;
	}
	val |= (PHY_ANLPAR_10 | PHY_ANLPAR_10FD | PHY_ANLPAR_TX | PHY_ANLPAR_TXFD);
	MdioWr(addr, PHY_ANAR, 0, val);  // channel 0
    MdioRd(addr, PHY_ANAR, 0, &val);

	/* Restart auto negotiation*/
    MdioRd(addr, PHY_BMCR, 0, &val);
	val |= PHY_BMCR_RST_NEG;
	MdioWr(addr, PHY_BMCR, 0, val);  // channel 0

    /*check AutoNegotiate complete - it can take upto 3 secs*/
    do {
        OALStall(40000);  // 40 msec
        cntr++;

        if (!MdioRd(addr, PHY_BMSR, 0, &val)){
            if(val & PHY_BMSR_AUTN_COMP)
                break;
        }
    } while(cntr < 1000);  // +++FIXME

    if (!MdioRd(addr, PHY_BMSR, 0, &val)) {
        if (!(val & PHY_BMSR_AUTN_COMP))
            OALMSGS(OAL_ERROR, (L"Auto negotitation failed\r\n"));
    }

    // Centaurus note:
    // 500 msec delay (not optimized) after auto negotiation complete
    // Without this delay, the first frame to be sent
    // in the first SendFrame() call will not go out to network.
    CPSW3G_DELAY(500);

    OALMSGS(OAL_ERROR, (L"Phy_init: Auto negotitation completed\r\n"));

    return;
}


/*****************************************************************************
 *
 * Return 1 if PHY supports 1000BASE-X, 0 if PHY supports 10BASE-T/100BASE-TX/
 * 1000BASE-T, or on error.
 */
int Phy_is_1000base_x (UINT16 addr)
{
#if defined(CONFIG_PHY_GIGE)
	UINT16 val = 0;
    UINT16 reg = 0;
    UINT16 mask = 0;

    if (get_cpu_rev(NULL))
    {
        reg = PHY_SPEC_STAT;
        mask = PHY_SPEC_STAT_1000;
    }
    else
    {
        OALMSG(OAL_ERROR, (TEXT("Phy_is_1000base_x - Unknown cpu rev\r\n")));
        return 0;
    }

	if (MdioRd(addr, reg, 0, &val)) {
        OALMSG(OAL_ERROR, (TEXT("PHY reg read failed, assuming no 1000BASE-X\r\n")));
		return 0;
	}

	return 0 != (val & mask);
#else
	return 0;
#endif
}


/*****************************************************************************
 *
 * Determine the ethernet speed (10/100/1000).  Return 10 on error.
 */
int Phy_speed (UINT16 addr)
{
	UINT16 bmcr, anlpar;

#if defined(CONFIG_PHY_GIGE)
	UINT16 btsr;

	/*
	 * Check for 1000BASE-X.  If it is supported, then assume that the speed
	 * is 1000.
	 */
	if (Phy_is_1000base_x (addr)) {
		return _1000BASET;
	}
	/*
	 * No 1000BASE-X, so assume 1000BASE-T/100BASE-TX/10BASE-T register set.
	 */
	/* Check for 1000BASE-T. */
	if (MdioRd(addr, PHY_1000BTSR, 0, &btsr)) {
        OALMSG(OAL_ERROR, (L"PHY 1000BT status"));
		goto phy_read_failed;
	}
	if (btsr != 0xFFFF &&
	    (btsr & (PHY_1000BTSR_1000FD | PHY_1000BTSR_1000HD))) {
		return _1000BASET;
	}
#endif /* CONFIG_PHY_GIGE */

	/* Check Basic Management Control Register first. */
	if (MdioRd (addr, PHY_BMCR, 0, &bmcr)) {
        OALMSG(OAL_ERROR, (L"Phy speed"));
		goto phy_read_failed;
	}

	/* Check if auto-negotiation is on. */
	if (bmcr & PHY_BMCR_AUTON) {
		/* Get auto-negotiation results. */
		if (MdioRd (addr, PHY_ANLPAR, 0, &anlpar)) {
            OALMSG(OAL_ERROR, (L"Phy AN speed"));
			goto phy_read_failed;
		}
		return (anlpar & PHY_ANLPAR_100) ? _100BASET : _10BASET;
	}

	/* Get speed from basic control settings. */
	return (bmcr & PHY_BMCR_100MB) ? _100BASET : _10BASET;

phy_read_failed:
    OALMSG(OAL_ERROR, (L" read failed, assuming 10BASE-T\r\n"));
	return _10BASET;
}


/*****************************************************************************
 *
 * Determine full/half duplex.  Return half on error.
 */
int Phy_duplex (UINT16 addr)
{
	UINT16 bmcr, anlpar;

#if defined(CONFIG_PHY_GIGE)
	UINT16 btsr;

	/* Check for 1000BASE-X. */
	if (Phy_is_1000base_x (addr)) {
		/* 1000BASE-X */
		if (MdioRd (addr, PHY_ANLPAR, 0, &anlpar)) {
            OALMSG(OAL_ERROR, (L"1000BASE-X PHY AN duplex"));
			goto phy_read_failed;
		}
	}

	/*
	 * No 1000BASE-X, so assume 1000BASE-T/100BASE-TX/10BASE-T register set.
	 */
	/* Check for 1000BASE-T. */
	if (MdioRd (addr, PHY_1000BTSR, 0, &btsr)) {
        OALMSG(OAL_ERROR, (L"PHY 1000BT status"));
		goto phy_read_failed;
	}

	if (btsr != 0xFFFF) {
		if (btsr & PHY_1000BTSR_1000FD) {
			return FULL;
		} else if (btsr & PHY_1000BTSR_1000HD) {
			return HALF;
		}
	}
#endif /* CONFIG_PHY_GIGE */

	/* Check Basic Management Control Register first. */
	if (MdioRd (addr, PHY_BMCR, 0, &bmcr)) {
        OALMSG(OAL_ERROR, (L"PHY duplex"));
		goto phy_read_failed;
	}
	/* Check if auto-negotiation is on. */
	if (bmcr & PHY_BMCR_AUTON) {
		/* Get auto-negotiation results. */
		if (MdioRd (addr, PHY_ANLPAR, 0, &anlpar)) {
            OALMSG(OAL_ERROR, (L"PHY AN duplex"));
			goto phy_read_failed;
		}
		return ((anlpar & (PHY_ANLPAR_10FD | PHY_ANLPAR_TXFD)) ?  FULL : HALF);
	}
	/* Get speed from basic control settings. */
	return ((bmcr & PHY_BMCR_DPLX) ? FULL : HALF);

phy_read_failed:
    OALMSG(OAL_ERROR, (L" read failed, assuming half duplex\r\n"));
	return HALF;
}


BOOL Cpsw3g_get_phy_link_state(UINT32 port)
{
	UINT16 phy_id;
	int speed = 0, duplex = 0;
	UINT16 reg;
	UINT32 mac_control = 0;

    BOOL link = FALSE;

    PCpswCb pCpsw3gCb=&g_cpsw3gCb;

    phy_id = pCpsw3gCb->phy_id;

	if (MdioRd(phy_id, PHY_BMSR, 0, &reg))
		return link; /* could not read, assume no link */

	if (reg & PHY_BMSR_LS) { /* link up */
		speed = Phy_speed(phy_id);
		duplex = Phy_duplex(phy_id);

		link = TRUE;
		mac_control = (1 << 5); /* MIIEN */
		if (speed == 10)
			mac_control |= (1 << 18);	/* In Band mode	*/
		if (speed == 1000)
			mac_control |= ((1 << 7) | (1 << 17));	/* GIGABITEN | GIGABIT_FORCE */
		if (duplex == FULL)
			mac_control |= (1 << 0);	/* FULLDUPLEXEN	*/
	}

	if (mac_control == pCpsw3gCb->mac_control)
		return link;

	if (mac_control) {
        OALMSGS(OAL_ERROR, ( 
            L"link up on port %d, speed %d, %s duplex\r\n", 
            port, speed, ((duplex == FULL) ?  L"full" : L"half")));
	} else {
        OALMSGS(OAL_ERROR, ( L"link down on port %d\r\n", port));
	}

    Cpsw3g_Write_Register((UINT32 *)CPMAC_MAC_CTRL(port), mac_control);
	pCpsw3gCb->mac_control = mac_control;

    return link;
}


#ifdef DBG_CPSW3G

void Cpsw3g_sl_reg_dump(Cpsw3g_SL_Regs *pRegs, int idx)
{

    OALMSG(1, (L" \r\n"));
    OALMSG(1, (L"  P%d_Max_Blks        : %08X \r\n", idx, pRegs->Max_Blks     ));
    OALMSG(1, (L"  P%d_BLK_CNT         : %08X \r\n", idx, pRegs->BLK_CNT         ));
    OALMSG(1, (L"  P%d_Tx_In_Ctl       : %08X \r\n", idx, pRegs->Tx_In_Ctl     ));
    OALMSG(1, (L"  P%d_Port_VLAN       : %08X \r\n", idx, pRegs->Port_VLAN     ));
    OALMSG(1, (L"  P%d_Tx_Pri_Map      : %08X \r\n", idx, pRegs->Tx_Pri_Map     ));
    OALMSG(1, (L"  P%d_TS_CTL          : %08X \r\n", idx, pRegs->TS_CTL         ));
    OALMSG(1, (L"  P%d_TS_SEQ_LTYPE    : %08X \r\n", idx, pRegs->TS_SEQ_LTYPE ));
    OALMSG(1, (L"  P%d_TS_VLAN         : %08X \r\n", idx, pRegs->TS_VLAN         ));
    OALMSG(1, (L"  SL%d_SA_LO          : %08X \r\n", idx, pRegs->SL_SA_LO     ));
    OALMSG(1, (L"  SL%d_SA_HI          : %08X \r\n", idx, pRegs->SL_SA_HI       ));
    OALMSG(1, (L"  P%d_Send_Percent    : %08X \r\n", idx, pRegs->Send_Percent ));

}

void Cpsw3g_stats_reg_dump(PCPSW3G_REGS pRegs)
{
}

void Cpsw3g_cpts_reg_dump(PCPSW3G_REGS pRegs)
{
}

void Cpsw3g_cpgmac_reg_dump(CpgmacSl_Regs *pRegs, int idx)
{
    OALMSG(1, (L" \r\n"));
    OALMSG(1, (L"  SL%d_IDVER          : %08X \r\n", idx, pRegs->SL_IDVER ));
    OALMSG(1, (L"  SL%d_MacControl     : %08X \r\n", idx, pRegs->SL_MacControl ));
    OALMSG(1, (L"  SL%d_MacStatus      : %08X \r\n", idx, pRegs->SL_MacStatus ));
    OALMSG(1, (L"  SL%d_Soft_Reset     : %08X \r\n", idx, pRegs->SL_Soft_Reset ));
    OALMSG(1, (L"  SL%d_Rx_Maxlen      : %08X \r\n", idx, pRegs->SL_Rx_Maxlen  ));
    OALMSG(1, (L"  SL%d_BoffTest       : %08X \r\n", idx, pRegs->SL_BoffTest   ));
    OALMSG(1, (L"  SL%d_Rx_Pause       : %08X \r\n", idx, pRegs->SL_Rx_Pause   ));
    OALMSG(1, (L"  SL%d_Tx_Pause       : %08X \r\n", idx, pRegs->SL_Tx_Pause   ));
    OALMSG(1, (L"  SL%d_EMControl      : %08X \r\n", idx, pRegs->SL_EMControl  ));
    OALMSG(1, (L"  SL%d_Rx_Pri_Map     : %08X \r\n", idx, pRegs->SL_Rx_Pri_Map ));

}


void Cpsw3g_reg_dump(void)
{
    int i;

    PCPSW3G_REGS pRegs = (PCPSW3G_REGS)CPSW_3G_BASE;

    OALMSG(1, (L"  CPSW_IdVer         : %08X \r\n", pRegs->CPSW_IdVer         ));
    OALMSGS(1, (L"  CPSW_Control       : %08X \r\n", pRegs->CPSW_Control     ));
    OALMSGS(1, (L"  CPSW_Soft_Reset    : %08X \r\n", pRegs->CPSW_Soft_Reset     ));
    OALMSGS(1, (L"  CPSW_Stat_Port_En  : %08X \r\n", pRegs->CPSW_Stat_Port_En));
    OALMSGS(1, (L"  CPSW_Pritype       : %08X \r\n", pRegs->CPSW_Pritype     ));
    
    OALMSGS(1, (L"  CPSW_Soft_Idle     : %08X \r\n", pRegs->CPSW_Soft_Idle     ));
    OALMSGS(1, (L"  CPSW_Thru_Rate     : %08X \r\n", pRegs->CPSW_Thru_Rate     ));
    OALMSGS(1, (L"  CPSW_Gap_Thresh    : %08X \r\n", pRegs->CPSW_Gap_Thresh     ));
    OALMSGS(1, (L"  CPSW_Tx_Start_WDS  : %08X \r\n", pRegs->CPSW_Tx_Start_WDS ));
    OALMSGS(1, (L"  CPSW_Flow_Control  : %08X \r\n", pRegs->CPSW_Flow_Control ));
    
    OALMSGS(1, (L" \r\n"));
    OALMSGS(1, (L"  P0_Max_blks        : %08X \r\n", pRegs->P0_Max_blks         ));
    OALMSGS(1, (L"  P0_BLK_CNT         : %08X \r\n", pRegs->P0_BLK_CNT         ));
    OALMSGS(1, (L"  P0_Tx_In_Ctl       : %08X \r\n", pRegs->P0_Tx_In_Ctl     ));
    OALMSGS(1, (L"  P0_Port_VLAN       : %08X \r\n", pRegs->P0_Port_VLAN     ));
    OALMSGS(1, (L"  P0_Tx_Pri_Map      : %08X \r\n", pRegs->P0_Tx_Pri_Map     ));
    OALMSGS(1, (L"  CPDMA_Tx_Pri_Map   : %08X \r\n", pRegs->CPDMA_Tx_Pri_Map ));
    OALMSGS(1, (L"  CPDMA_Rx_Ch_Map    : %08X \r\n", pRegs->CPDMA_Rx_Ch_Map     ));

    Cpsw3g_sl_reg_dump(&(pRegs->CPSW_SL_Regs[0]), 1);
    Cpsw3g_sl_reg_dump(&(pRegs->CPSW_SL_Regs[1]), 2);

    OALMSGS(1, (L" \r\n"));
    OALMSGS(1, (L"  Tx_Idver           : %08X \r\n", pRegs->Tx_Idver         ));
    OALMSGS(1, (L"  Tx_Control         : %08X \r\n", pRegs->Tx_Control         ));
    OALMSGS(1, (L"  Tx_Teardown        : %08X \r\n", pRegs->Tx_Teardown         ));
    OALMSGS(1, (L"  Rsvd2              : %08X \r\n", pRegs->Rsvd2             ));
    OALMSGS(1, (L"  Rx_Idver           : %08X \r\n", pRegs->Rx_Idver         ));
    OALMSGS(1, (L"  Rx_Control         : %08X \r\n", pRegs->Rx_Control         ));
    OALMSGS(1, (L"  Rx_Teardown        : %08X \r\n", pRegs->Rx_Teardown         ));
    OALMSGS(1, (L"  CPDMA_Soft_Reset   : %08X \r\n", pRegs->CPDMA_Soft_Reset ));
    OALMSGS(1, (L"  DMAControl         : %08X \r\n", pRegs->DMAControl         ));
    OALMSGS(1, (L"  DMAStatus          : %08X \r\n", pRegs->DMAStatus         ));
    OALMSGS(1, (L"  RX_Buffer_Offset   : %08X \r\n", pRegs->RX_Buffer_Offset ));
    OALMSGS(1, (L"  EMControl          : %08X \r\n", pRegs->EMControl         ));

    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)
    OALMSGS(1, (L"  TX_PriN_Rate[%d]    : %08X \r\n", i, pRegs->TX_PriN_Rate[i] ));

    OALMSGS(1, (L" \r\n"));
    OALMSGS(1, (L"  Tx_IntStat_Raw     : %08X \r\n", pRegs->Tx_IntStat_Raw ));
    OALMSGS(1, (L"  Tx_IntStat_Masked  : %08X \r\n", pRegs->Tx_IntStat_Masked ));
    OALMSGS(1, (L"  Tx_IntMask_Set     : %08X \r\n", pRegs->Tx_IntMask_Set ));
    OALMSGS(1, (L"  Tx_IntMask_Clear   : %08X \r\n", pRegs->Tx_IntMask_Clear ));
    OALMSGS(1, (L"  CPDMA_In_Vector    : %08X \r\n", pRegs->CPDMA_In_vector     ));
    OALMSGS(1, (L"  CPDMA_EOI_Vector   : %08X \r\n", pRegs->CPDMA_EOI_vector     ));
    OALMSGS(1, (L"  Rx_IntStat_Raw     : %08X \r\n", pRegs->Rx_IntStat_Raw         ));
    OALMSGS(1, (L"  Rx_IntStat_Masked  : %08X \r\n", pRegs->Rx_IntStat_Masked     ));
    OALMSGS(1, (L"  Rx_IntMask_Set     : %08X \r\n", pRegs->Rx_IntMask_Set         ));
    OALMSGS(1, (L"  Rx_IntMask_Clear   : %08X \r\n", pRegs->Rx_IntMask_Clear     ));
    OALMSGS(1, (L"  DMA_IntStat_Raw    : %08X \r\n", pRegs->DMA_IntStat_Raw     ));
    OALMSGS(1, (L"  DMA_IntStat_Masked : %08X \r\n", pRegs->DMA_IntStat_Masked     ));
    OALMSGS(1, (L"  DMA_IntMask_Set    : %08X \r\n", pRegs->DMA_IntMask_Set     ));
    OALMSGS(1, (L"  DMA_IntMask_Clear  : %08X \r\n", pRegs->DMA_IntMask_Clear     ));

    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)
    OALMSGS(1, (L"  RX_PendThresh[%d]   : %08X \r\n", i, pRegs->RX_PendThresh[i] ));

    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)
    OALMSGS(1, (L"  RX_FreeBuffer[%d]   : %08X \r\n", i, pRegs->RX_FreeBuffer[i] ));

    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)
    OALMSGS(1, (L"  Tx_HDP[%d]          : %08X \r\n", i, pRegs->Tx_HDP[i] ));
                                         
    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)                  
    OALMSGS(1, (L"  Rx_HDP[%d]          : %08X \r\n", i, pRegs->Rx_HDP[i] ));

    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)
    OALMSGS(1, (L"  Tx_CP[%d]           : %08X \r\n", i, pRegs->Tx_CP[i] ));
                                         
    OALMSGS(1, (L" \r\n"));
    for (i=0; i<8; i++)                  
    OALMSGS(1, (L"  Rx_CP[%d]           : %08X \r\n", i, pRegs->Rx_CP[i] ));

    Cpsw3g_stats_reg_dump(pRegs);

    Cpsw3g_cpts_reg_dump(pRegs);

    OALMSGS(1, (L" \r\n"));
    OALMSGS(1, (L"  ALE_IdVer          : %08X \r\n", pRegs->ALE_IdVer ));
    OALMSGS(1, (L"  ALE_Control        : %08X \r\n", pRegs->ALE_Control ));
    OALMSGS(1, (L"  ALE_PreScale       : %08X \r\n", pRegs->ALE_PreScale ));
    OALMSGS(1, (L"  ALE_Unknown_VLAN   : %08X \r\n", pRegs->ALE_Unknown_VLAN ));
    OALMSGS(1, (L"  ALE_TblCtl         : %08X \r\n", pRegs->ALE_TblCtl ));

    for (i=0; i<3; i++)
    OALMSGS(1, (L"  ALE_Tbl[%d]         : %08X \r\n", i, pRegs->ALE_Tbl[i] ));

    for (i=0; i<6; i++)
    OALMSGS(1, (L"  ALE_PortCtl[%d]     : %08X \r\n", i, pRegs->ALE_PortCtl[i] ));

    Cpsw3g_cpgmac_reg_dump(&(pRegs->SL_Regs[0]), 1);
    Cpsw3g_cpgmac_reg_dump(&(pRegs->SL_Regs[1]), 2);

    OALMSGS(1, (L" \r\n"));
}


void Cpsw3g_pkt_dump(UINT8 *pBuffer, UINT32 length)
{
    UINT32 r, R, last_len;
    PUCHAR  pc;

    pc = (PUCHAR)pBuffer;
    R = (length) / 8;
    last_len = length - (R * 8);

    RETAILMSG(1, (L"%d\r\n", length));
 
    for (r=0; r < R; r++, pc += 8)
    {
        if (0 == (r & 0x00000001))
        {
            RETAILMSG(1, (L"  ** %02x %02x %02x %02x %02x %02x %02x %02x  ", 
               *(pc+0), *(pc+1), *(pc+2), *(pc+3), *(pc+4), *(pc+5), *(pc+6), *(pc+7) ));
        }
        else
        {
            RETAILMSG(1, (L"  %02x %02x %02x %02x %02x %02x %02x %02x \r\n", 
               *(pc+0), *(pc+1), *(pc+2), *(pc+3), *(pc+4), *(pc+5), *(pc+6), *(pc+7) ));
        }
    }

    if (last_len == 0)
    {
        // Done.
        if (0 == (r & 0x00000001))
            RETAILMSG(1, (L"\r\n")); 

        return;
    }

    // has incomplete last row.
    if (0 == (r & 0x00000001))
    {
        // left
        RETAILMSG(1, (L"  ** ")); 
    }
    else
    {
        // right
        RETAILMSG(1, (L"  ")); 
    }

    switch(last_len)
    {
        case 7:
            RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x \r\n", 
               *(pc+0), *(pc+1), *(pc+2), *(pc+3), *(pc+4), *(pc+5), *(pc+6)));
        break;

        case 6:
            RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x \r\n", 
               *(pc+0), *(pc+1), *(pc+2), *(pc+3), *(pc+4), *(pc+5) ));
        break;

        case 5:
            RETAILMSG(1, (L"%02x %02x %02x %02x %02x \r\n", 
               *(pc+0), *(pc+1), *(pc+2), *(pc+3), *(pc+4) ));
        break;

        case 4:
            RETAILMSG(1, (L"%02x %02x %02x %02x \r\n", *(pc+0), *(pc+1), *(pc+2), *(pc+3) ));
        break;

        case 3:
            RETAILMSG(1, (L"%02x %02x %02x \r\n", *(pc+0), *(pc+1), *(pc+2) ));
        break;

        case 2:
            RETAILMSG(1, (L"%02x %02x \r\n", *(pc+0), *(pc+1) ));
        break;

        case 1:
            RETAILMSG(1, (L"%02x \r\n", *(pc+0) ));
        break;
    }
}

#endif // DBG_CPSW3G


