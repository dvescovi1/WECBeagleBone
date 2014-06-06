//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Phy.c
//! \brief Contains EMAC controller PHY related functions.
//! 
//! This source file contains Intel PHY related init,read and write routines.
//! These are being called for link handling.
//! 
//! \version  1.00 Aug 22nd 2006 File Created

// Includes
#include <Ndis.h>
#include "Emac_Adapter.h"
#include "Emac_Queue.h"

PEMACMDIOREGS f_pMdioRegs;

//========================================================================
//!  \fn UINT32 ReadPhyRegister( UINT32 PhyAddr, UINT32 RegNum)
//!  \brief Read a Phy register via MDIO inteface.
//!  \param PhyAddr UINT32 Bit mask of link
//!  \param RegNum  UINT32 Registe number.
//!  \return UINT32  Register value.
//========================================================================
UINT32 
ReadPhyRegister( 
    UINT32 PhyAddr,     
    UINT32 RegNum
    )
{
    /* Wait for any previous command to complete */ 
    while ( (f_pMdioRegs->USERACCESS0 & MDIO_USERACCESS0_GO) != 0 )
    {
    }
    f_pMdioRegs->USERACCESS0 =(
                                (MDIO_USERACCESS0_GO) |
                                (MDIO_USERACCESS0_WRITE_READ)|
                                ((RegNum & 0x1F) << 21) |
                                ((PhyAddr & 0x1F) << 16)
                                );
    /* Wait for command to complete */

    while ( (f_pMdioRegs->USERACCESS0 & MDIO_USERACCESS0_GO) != 0 )
    {
    }

    return (f_pMdioRegs->USERACCESS0 & 0xFFFF);
}



//========================================================================
//!  \fn void    WritePhyRegister( UINT32 PhyAddr,UINT32 RegNum,UINT32 Data )
//!  \brief Write to a Phy register via MDIO inteface
//!  \param PhyAddr UINT32 Bit mask of link.
//!  \param RegNum  UINT32 Register number to be written.
//!  \param Data   UINT32 Data to be written.
//!  \return VOID Returns none.
//========================================================================
void    
WritePhyRegister( 
    UINT32 PhyAddr,     
    UINT32 RegNum,      
    UINT32 Data 
    )
{
    /* Wait for User access register to be ready */

    while ( (f_pMdioRegs->USERACCESS0 & MDIO_USERACCESS0_GO) != 0 )
    {
    }

    f_pMdioRegs->USERACCESS0 =(
                                (MDIO_USERACCESS0_GO) |
                                (MDIO_USERACCESS0_WRITE_WRITE) |
                                ((RegNum & 0x1F) << 21) |
                                ((PhyAddr & 0x1F) << 16) |
                                (Data & 0xFFFF)
                                );

}

//========================================================================
//!  \fn UINT32 OMAPEmacGetLink( MINIPORT_ADAPTER *Adapter)
//!  \brief  Function whether link is present.
//!  \param  Phy_mask Mask of the links to be tested.
//!  \return UINT32 Returns the bit mask of link.
//========================================================================
UINT32
PhyFindLink(
    MINIPORT_ADAPTER *Adapter
    )
{
    UINT32 ActPhy    = 0;
    UINT32 LinkState = 0;   /* To maintain link state  */
    UINT32 PhyAddr   = 0;   /* Phy number to be probed */
    UINT32 Regval    = 0;

    f_pMdioRegs = Adapter->m_pMdioRegsBase;
    
    ActPhy = f_pMdioRegs->ALIVE;
    
    DEBUGMSG(DBG_FUNC,(L"--->PHYFindLink \r\n")); 
    
    while(ActPhy != 0)
    {
        /* find the next active phy */
        while (0 == (ActPhy & BIT(0)))
        {
            PhyAddr++;
            ActPhy >>= 1;
        }

        /* Read the status register from Phy */
        Regval = ReadPhyRegister(PhyAddr, MII_STATUS_REG);
        DEBUGMSG(DBG_INFO, (L"Regval %x, PhyAddr %u, LinkState %d \r\n",Regval, PhyAddr, LinkState));

        /* update link status mask */
        if (Regval != 0xFFFF)
        {
			if (Regval & PHY_LINK_STATUS)
			{
				LinkState |= 1 << PhyAddr;
			}
            break;
        }

        /* go to next phy */
        PhyAddr++;
        ActPhy >>= 1;
    }

	Adapter->m_ActivePhy = PhyAddr;

    DEBUGMSG(DBG_FUNC, (L"<---PHYFindLink\r\n")); 

    return (LinkState);
}

//========================================================================
//!  \fn VOID LinkChangeIntrHandler(MINIPORT_ADAPTER* Adapter)
//!  \brief Handles link change interrupt from MiniporthandleInterrrupt
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
LinkChangeIntrHandler(
    MINIPORT_ADAPTER* Adapter
    )
{
    f_pMdioRegs = Adapter->m_pMdioRegsBase;
    
    DEBUGMSG(DBG_FUNC, (L"LinkChangeIntrHandler-->m_ActivePhy = 0x%X\r\n", Adapter->m_ActivePhy));
    /* Extract the Link status for the active PHY */
    if(0 == (f_pMdioRegs->LINK & (BIT(0) << Adapter->m_ActivePhy)))
    {
        Adapter->m_LinkStatus = DOWN;
         /* Link was active last time, now it is down. */
        NdisMIndicateStatus(Adapter->m_AdapterHandle,
                                NDIS_STATUS_MEDIA_DISCONNECT, 
                                (PVOID)0, 
                                0);
                    
        NdisMIndicateStatusComplete(Adapter->m_AdapterHandle);   
        DEBUGMSG(DBG_INFO, (L"LinkChangeIntrHandler NDIS_STATUS_MEDIA_DISCONNECT\r\n"));
    } 
    else 
    {   
        Adapter->m_LinkStatus = UP;
        /* Link was active last time, now it is up. */
        NdisMIndicateStatus(Adapter->m_AdapterHandle,
                                NDIS_STATUS_MEDIA_CONNECT, 
                                (PVOID)0, 
                                0);
        
        NdisMIndicateStatusComplete(Adapter->m_AdapterHandle);
        DEBUGMSG(DBG_INFO, (L"LinkChangeIntrHandler NDIS_STATUS_MEDIA_CONNECT\r\n"));
    } 
    /* Acknowledge interrupt */
    f_pMdioRegs->LINKINTMASKED = 0x1;
}
