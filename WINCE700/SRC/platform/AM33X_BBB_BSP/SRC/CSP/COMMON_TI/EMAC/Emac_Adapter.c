//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Adapter.c
//! \brief OMAP13x EMAC Controller Initialisation and related file.
//!
//! This source File contains the mapping , initialisation of EMAC controller.
//! Also it contains the data structure initialisation for send and receive.
//!
//! \version  1.00 Aug 22nd 2006 File Created

// Includes
#include <Ndis.h>
#include "Emac_Adapter.h"
#include "Emac_Proto.h"
#include "oal_clock.h"

extern void SocResetEmac();

//========================================================================
//!  \fn NDIS_STATUS NICMapAdapterRegs(PEMAC_ADAPTER    pAdapter)
//!  \brief Maps EMAC adapter memory to process memory.
//!  \param pAdapter PEMAC_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
NICMapAdapterRegs(
    PEMAC_ADAPTER   pAdapter
    )

{
    DWORD temp;
    DWORD                   EmacIRamBase;
    NDIS_STATUS             Status;
    NDIS_PHYSICAL_ADDRESS   PhysicalAddress;

    DEBUGMSG(DBG_FUNC, (L"-->NICMapAdapterRegs\r\n"));

    temp = GetAddressByDevice(pAdapter->m_device);

    NdisSetPhysicalAddressLow (PhysicalAddress, temp);
    NdisSetPhysicalAddressHigh (PhysicalAddress, 0);

   /* Mapping the EMAC controller registers to process memory
    * and initialising the individual register modules
    */
    Status = NdisMMapIoSpace((PVOID*)&EmacIRamBase,
                            pAdapter->m_AdapterHandle,
                            PhysicalAddress,
                            g_EmacMemLayout.EMAC_TOTAL_MEMORY);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\r\n"));
        return Status;
    }

   /* Since EMAC register modules are contiguos assigning
    * mapped memory to our adapter structure elements
    */

    pAdapter->m_EmacIRamBase    = EmacIRamBase;
    pAdapter->m_pEmacRegsBase   = (PEMACREGS)(EmacIRamBase + g_EmacMemLayout.EMAC_REG_OFFSET);
    pAdapter->m_pEmacCtlRegs    = (PEMACCTRLREGS)(EmacIRamBase + g_EmacMemLayout.EMAC_CTRL_OFFSET);
    pAdapter->m_pMdioRegsBase   = (PEMACMDIOREGS)(EmacIRamBase + g_EmacMemLayout.EMAC_MDIO_OFFSET);

    DEBUGMSG(DBG_FUNC, (L"<--NICMapAdapterRegs EmacRegs->TXREVID = %x\r\n", pAdapter->m_pEmacRegsBase->TXREVID));

    return Status;
}

//========================================================================
//!  \fn NDIS_STATUS NICInitSend(PEMAC_ADAPTER  pAdapter)
//!  \brief Allocates and initialises Tx data structures.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
NICInitSend(
    PEMAC_ADAPTER   pAdapter
    )

{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    USHORT          Count;
    DWORD           EmacTxBufDesBase;
    DWORD           EmacTxBufDesBasePa;
    DWORD           EmacTxPhyBuf ;
    DWORD           EmacTxPhyBufPa;
    PEMAC_TXPKT     pCurPkt;
    PEMAC_TXPKT     pNextPkt;
    PEMAC_TXBUF     pCurBuf;
    PEMAC_TXBUF     pNextBuf;


    DEBUGMSG(DBG_FUNC, (L"--> NICInitSend, \r\n" ));

    if(pAdapter->m_HwStatus != NdisHardwareStatusReset)
    {
		/* Setting up Transmit Packets data structures */
		Status = NdisAllocateMemory((PVOID *)&pAdapter->m_pBaseTxPkts,
							pAdapter->m_MaxPacketPerTransmit * sizeof(EMAC_TXPKT),
							0,
							g_HighestAcceptedMax);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
			return Status;
		}

		/* Setting up Transmit Buffers  data structures */
		Status = NdisAllocateMemory((PVOID *)&pAdapter->m_pBaseTxBufs,
								pAdapter->m_MaxTxEmacBufs * sizeof(EMAC_TXBUF),
								0,
								g_HighestAcceptedMax);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
			return Status;
		}

		/* Allocating reserved memory for Tx buffers */
		if(NULL == (VOID *)pAdapter->m_TxBufBase)
		{
			Status = NdisMMapIoSpace((PVOID*)&pAdapter->m_TxBufBase,
							pAdapter->m_AdapterHandle,
							pAdapter->m_TxBufBasePa,
							(EMAC_MAX_TX_BUFFERS * EMAC_MAX_ETHERNET_PKT_SIZE));

			if(Status != NDIS_STATUS_SUCCESS)
			{
				DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\r\n"));
				return Status;
			}
		}
	}

    NdisZeroMemory(pAdapter->m_pBaseTxPkts, pAdapter->m_MaxPacketPerTransmit * sizeof(EMAC_TXPKT));
    NdisZeroMemory(pAdapter->m_pBaseTxBufs, pAdapter->m_MaxTxEmacBufs * sizeof(EMAC_TXBUF));

    pNextPkt = pAdapter->m_pBaseTxPkts;

    QUEUE_INIT(&pAdapter->m_TxPktsInfoPool);

    for (Count = 0; Count < pAdapter->m_MaxPacketPerTransmit ; Count++)
    {
        pCurPkt = pNextPkt;

        /* Add to Free Transmit packets pool */
        QUEUE_INSERT(&pAdapter->m_TxPktsInfoPool,pCurPkt);

        /* Initialise per packet maintained Bufs List */
        QUEUE_INIT(&pCurPkt->m_BufsList);

        pNextPkt++;

        pCurPkt->m_pNext  = pNextPkt;
    }

    pCurPkt->m_pNext = 0;

    DEBUGMSG(DBG_INFO, (L"AptHandle = %p NdisMMapIoSpace TxBuff(size=%d)(%p)=PA:%p\r\n", 
        pAdapter->m_AdapterHandle, 
        (EMAC_MAX_TX_BUFFERS * EMAC_MAX_ETHERNET_PKT_SIZE), 
        pAdapter->m_TxBufBase, 
        pAdapter->m_TxBufBasePa));

    NdisZeroMemory((PVOID)pAdapter->m_TxBufBase, (EMAC_MAX_TX_BUFFERS * EMAC_MAX_ETHERNET_PKT_SIZE));

    EmacTxBufDesBase = pAdapter->m_EmacIRamBase + EMAC_TX_DESC_BASE;
    EmacTxBufDesBasePa = (g_EmacMemLayout.EMAC_PERPECTIVE_RAM_ADDR + EMAC_TX_DESC_BASE);

    EmacTxPhyBuf   = pAdapter->m_TxBufBase;
    EmacTxPhyBufPa = NdisGetPhysicalAddressLow(pAdapter->m_TxBufBasePa);

    pNextBuf = pAdapter->m_pBaseTxBufs;

    QUEUE_INIT(&pAdapter->m_TxBufInfoPool);

    for (Count = 0; Count < pAdapter->m_MaxTxEmacBufs ; Count++)
    {
        pCurBuf = pNextBuf;

        /* Assigning the EMAC buffer descriptors virtual and physical
        * addressses as well
        */
        pCurBuf->m_EmacBufDes   = EmacTxBufDesBase;
        pCurBuf->m_EmacBufDesPa = EmacTxBufDesBasePa;

        /* Assigning corresponding physical and logical addresses */

        pCurBuf->m_BufLogicalAddress    = EmacTxPhyBuf;
        pCurBuf->m_BufPhysicalAddress   = EmacTxPhyBufPa;

        EmacTxBufDesBase   += sizeof(EMAC_DESC);
        EmacTxBufDesBasePa += sizeof(EMAC_DESC);

        EmacTxPhyBuf   += EMAC_MAX_ETHERNET_PKT_SIZE;
        EmacTxPhyBufPa += EMAC_MAX_ETHERNET_PKT_SIZE;

        pNextBuf++;

        QUEUE_INSERT(&pAdapter->m_TxBufInfoPool,pCurBuf);

    }

    /* Initialise posted packets queue */
    QUEUE_INIT(&pAdapter->m_TxPostedPktPool);

    DEBUGMSG(DBG_FUNC, (L"<-- NICInitSend, \r\n" ));

    return Status;
}


//========================================================================
//!  \fn NDIS_STATUS NICInitRecv(PEMAC_ADAPTER  pAdapter)
//!  \brief Allocates and initialises Rx data structures.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
NICInitRecv(
    PEMAC_ADAPTER  pAdapter
    )

{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    USHORT          Count;
    DWORD           EmacRcvBufDesBase;
    DWORD           EmacRcvBufDesBasePa;
    PEMAC_RXPKTS    pCurPkt;
    PEMAC_RXPKTS    pNextPkt;
    PEMAC_RXBUFS    pCurBuf;
    PEMAC_RXBUFS    pNextBuf;
    DWORD           RcvBufLogical;
    DWORD           RcvBufPhysical;

    EmacRcvBufDesBase   = (pAdapter->m_EmacIRamBase +EMAC_RX_DESC_BASE);
    EmacRcvBufDesBasePa = (g_EmacMemLayout.EMAC_PERPECTIVE_RAM_ADDR + EMAC_RX_DESC_BASE);

    DEBUGMSG(DBG_FUNC, (L"---> NICInitRecv \r\n" ));

    if(pAdapter->m_HwStatus != NdisHardwareStatusReset)
    {
		// Setting up Receive Packets data structures */
		Status = NdisAllocateMemory((PVOID *)&pAdapter->m_pBaseRxPkts,
									  pAdapter->m_NumRxIndicatePkts * sizeof(EMAC_RXPKTS),
									  0,
									  g_HighestAcceptedMax);
	                            
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
			return Status;
		}
	    
		/* Setting up Receive Buffers  data structures */
	    
		Status = NdisAllocateMemory((PVOID *)&pAdapter->m_pBaseRxBufs,
										pAdapter->m_NumEmacRxBufDesc * sizeof(EMAC_RXBUFS),
										0,
										g_HighestAcceptedMax);
	                            
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
			return Status;
		}

		if(NULL == (VOID *)pAdapter->m_RxBufsBase)
		{
			Status = NdisMMapIoSpace((PVOID*)&pAdapter->m_RxBufsBase,
							pAdapter->m_AdapterHandle,
							pAdapter->m_RxBufsBasePa,
							(pAdapter->m_NumEmacRxBufDesc * EMAC_MAX_PKT_BUFFER_SIZE));

			if(Status != NDIS_STATUS_SUCCESS)
			{
				DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\r\n"));
				return Status;
			}
		}
	}

    DEBUGMSG(DBG_INFO, (L"AptHandle = %p NdisMMapIoSpace RxBuff(size=%d)(%p)=PA:%p\r\n", 
        pAdapter->m_AdapterHandle, 
        (pAdapter->m_NumEmacRxBufDesc * EMAC_MAX_PKT_BUFFER_SIZE), 
        pAdapter->m_RxBufsBase, 
        pAdapter->m_RxBufsBasePa));

    NdisZeroMemory(pAdapter->m_pBaseRxPkts,
                    pAdapter->m_NumRxIndicatePkts * sizeof(EMAC_RXPKTS));

    NdisZeroMemory(pAdapter->m_pBaseRxBufs,
                    pAdapter->m_NumEmacRxBufDesc * sizeof(EMAC_RXBUFS));

    EmacRcvBufDesBase = (pAdapter->m_EmacIRamBase + EMAC_RX_DESC_BASE);

    NdisZeroMemory((PVOID)pAdapter->m_RxBufsBase,
                    pAdapter->m_NumEmacRxBufDesc * EMAC_MAX_PKT_BUFFER_SIZE);

    /* Allocate Packet pool */
    NdisAllocatePacketPool(&Status,
                           &pAdapter->m_RecvPacketPool,
                           pAdapter->m_NumRxIndicatePkts,
                           MINIPORT_RESERVED_SIZE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisAllocatePacketPool failed\r\n"));
        return Status;
    }


    pNextPkt = pAdapter->m_pBaseRxPkts;

    QUEUE_INIT(&pAdapter->m_RxPktPool);

    for (Count = 0; Count < pAdapter->m_NumRxIndicatePkts ; Count++)
    {
        pCurPkt = pNextPkt;

        /* Allocating packet from packet pool */
        NdisAllocatePacket( &Status,
                            (PNDIS_PACKET *)&pCurPkt->m_PktHandle,
                            pAdapter->m_RecvPacketPool);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR,(L" NdisAllocatePacket Unsucessful\r\n"));
            break;
        }

        QUEUE_INSERT(&pAdapter->m_RxPktPool, pCurPkt);

        NDIS_SET_PACKET_HEADER_SIZE((PNDIS_PACKET)pCurPkt->m_PktHandle, EMAC_HEADER_SIZE);

        pNextPkt++;
        pCurPkt->m_pNext  = pNextPkt;
    }

    pCurPkt->m_pNext=0;

    if(Count != pAdapter->m_NumRxIndicatePkts)
    {
        return Status;
    }

    DEBUGMSG (DBG_INFO,(L"+pAdapter->m_RxPktPool.Head %x\r\npAdapter->m_RxPktPool.Tail %x\r\npAdapter->m_RxPktPool.Count %x \r\n",
        pAdapter->m_RxPktPool.m_pHead,  pAdapter->m_RxPktPool.m_pTail,pAdapter->m_RxPktPool.m_Count));

    /* Allocate  the receive buffer pool */
    NdisAllocateBufferPool( &Status,
                            &pAdapter->m_RecvBufferPool,
                            pAdapter->m_NumEmacRxBufDesc
                          );
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisAllocateBufferPool failed\r\n"));
        return Status;
    }
    pNextBuf = pAdapter->m_pBaseRxBufs;

    RcvBufLogical   = pAdapter->m_RxBufsBase;
    RcvBufPhysical =  NdisGetPhysicalAddressLow(pAdapter->m_RxBufsBasePa);

    QUEUE_INIT(&pAdapter->m_RxBufsPool);


    for (Count = 0; Count < pAdapter->m_NumEmacRxBufDesc ; Count++)
    {
        pCurBuf = pNextBuf;

        pCurBuf->m_BufLogicalAddress  = RcvBufLogical;
        pCurBuf->m_BufPhysicalAddress = RcvBufPhysical;

        /* point our buffer for receives at this Rfd */
        NdisAllocateBuffer(&Status,
                            (PNDIS_BUFFER *)&pCurBuf->m_BufHandle,
                            pAdapter->m_RecvBufferPool,
                            (PVOID)pCurBuf->m_BufLogicalAddress,
                            EMAC_MAX_PKT_BUFFER_SIZE);

        if (Status != NDIS_STATUS_SUCCESS)
        {

            DEBUGMSG(DBG_ERR,(L" NdisAllocateBuffer Unsucessful\r\n"));
            break;
        }
        /* Assigning the EMAC buffer descriptors virtual and physical
        * addressses as well
        */
        pCurBuf->m_EmacBufDes   = EmacRcvBufDesBase;
        pCurBuf->m_EmacBufDesPa = EmacRcvBufDesBasePa;

        /* we will also set up correspondinf EMAC buffer descriptors virtual as
        * well as physical
        */

        RcvBufLogical += EMAC_MAX_PKT_BUFFER_SIZE;
        RcvBufPhysical += EMAC_MAX_PKT_BUFFER_SIZE;

        EmacRcvBufDesBase += sizeof(EMAC_DESC);
        EmacRcvBufDesBasePa += sizeof(EMAC_DESC);

        pNextBuf++;

        QUEUE_INSERT(&pAdapter->m_RxBufsPool,pCurBuf);

     }
        pCurBuf->m_pNext=0;

    if(Count != pAdapter->m_NumEmacRxBufDesc)
    {
        return Status;
    }
    DEBUGMSG(DBG_FUNC, (L"<-- NICInitRecv, Status=%x\r\n", Status));

    return Status;
}


//========================================================================
//!  \fn BOOL EMACModStateChange(OMAP_DEVICE device, UINT32  ModState)
//!  \brief Power on EMAC power-on mudules if they are not.
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================
BOOL
EMACModStateChange(
    OMAP_DEVICE device,
    UINT32  ModState
    )
{
    DEBUGMSG(TRUE, (L"+EMACModStateChange\r\n"));
 
    switch (ModState)
    {   
    case SYNCRST:
        SocResetEmac();
        EnableDeviceClocks(device,TRUE);
        break;
    case ENABLED:
        EnableDeviceClocks(device,TRUE);
        break;
    case DISABLED:
        EnableDeviceClocks(device,FALSE);
        break;
    }

    DEBUGMSG(TRUE, ( L"-EMACModStateChange\r\n" ));

    return (TRUE);
}

//========================================================================
//!  \fn NDIS_STATUS NICInitializeAdapter(PEMAC_ADAPTER     Adapter)
//!  \brief Initialises EMAC and MDIO registers and configures them.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
NICInitializeAdapter(
    PEMAC_ADAPTER  pAdapter
    )

{
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    PEMACREGS               pEmacRegs;
    PEMACMDIOREGS           pMdioRegs;
    PEMACCTRLREGS           pEmacCtlRegs;
    PEMAC_RXBUFS            pEmacRxCur;
    PEMAC_RXBUFS            pEmacRxNext;
    PEMAC_TXBUF             pEmacTxCur;
    PEMAC_TXBUF             pEmacTxNext;
    PEMACDESC               pRxDesc;
    PEMACDESC               pTxDesc;
    UINT32*                 pu32RegPtr = NULL;
    USHORT                  Count = 0;
    UINT32                  clkdiv;
    UINT32                  RetVal = 0;

    DEBUGMSG(DBG_FUNC, (L"--> NICInitializeAdapter \r\n"));

    //Power ON EMAC
    EMACModStateChange(pAdapter->m_device,ENABLED);

    pEmacRegs       = pAdapter->m_pEmacRegsBase;
    pMdioRegs       = pAdapter->m_pMdioRegsBase;
    pEmacCtlRegs    = pAdapter->m_pEmacCtlRegs;

    /*Issuing a reset to EMAC soft reset register and polling till we get 0. */
    pEmacRegs->SOFTRESET = 1;
    while ( pEmacRegs->SOFTRESET != 0 )
    {
    }
    pEmacRegs->SOFTRESET = 1;
    while ( pEmacRegs->SOFTRESET != 0 )
    {
    }

    /* Clear MACCONTROL, RXCONTROL & TXCONTROL registers */
    pEmacRegs->MACCONTROL = 0x0;
    pEmacRegs->TXCONTROL  = 0x0;
    pEmacRegs->RXCONTROL  = 0x0;

    /* Initialise the 8 Rx/Tx Header descriptor pointer registers */
    pu32RegPtr  = (UINT32 *)&pEmacRegs->TX0HDP;
    for (Count = 0; Count < EMAC_MAX_CHAN; Count++)
    {
        *pu32RegPtr ++ = 0;
    }
    pu32RegPtr  = (UINT32 *)&pEmacRegs->RX0HDP;
    for (Count = 0; Count < EMAC_MAX_CHAN; Count++)
    {
        *pu32RegPtr ++ = 0;
    }
    /* Clear 36 Statics registers */
    /* The statics registers start from RXGODFRAMES and continues for the next
     * 36 DWORD locations.
     */
    pu32RegPtr  = (UINT32 *)&pEmacRegs->RXGOODFRAMES;
    for (Count=0; Count < EMAC_STATS_REGS; Count++)
    {
        *pu32RegPtr ++ = 0;
    }

    /* Setup the local MAC address for all 8 Rx Channels */

    for (Count=0; Count < EMAC_MAX_CHAN; Count++)
    {
        pEmacRegs->MACINDEX = Count;

        /* Filling MACADDRHI registers only for first channel */
        if (Count==0 )
        {
            pEmacRegs->MACADDRHI = (
                                       (*(pAdapter->m_MACAddress + 3) << 24) |
                                       (*(pAdapter->m_MACAddress + 2) << 16) |
                                       (*(pAdapter->m_MACAddress + 1) << 8) |
                                       (*(pAdapter->m_MACAddress + 0) << 0)
                                       );
        }
        pEmacRegs->MACADDRLO = (
                                   (*(pAdapter->m_MACAddress + 5) << 8) |
                                   (*(pAdapter->m_MACAddress + 4) << 0) | (0<<16)|(1<<19) |(1<<20)
                                   );
    }


    /* clear the MAC address hash registers to 0 */
    pEmacRegs->MACHASH1 = 0;
    pEmacRegs->MACHASH2 = 0;

    /* Setup the local MAC address for 0th Transmit channel */
    pEmacRegs->MACINDEX = 0;
    pEmacRegs->MACSRCADDRHI = (
                                  (*(pAdapter->m_MACAddress + 3) << 24) |
                                  (*(pAdapter->m_MACAddress + 2) << 16) |
                                  (*(pAdapter->m_MACAddress + 1) << 8) |
                                  (*(pAdapter->m_MACAddress + 0) << 0)
                                  );

    pEmacRegs->MACSRCADDRLO = (
                                  (*(pAdapter->m_MACAddress + 5) << 8) |
                                  (*(pAdapter->m_MACAddress + 4) << 0)
                                  );

    /* Initialize the receive channel free buffer register including the
     * count, threshold, filter low priority frame threshold etc.
     */
    pEmacRegs->RX0FREEBUFFER = EMAC_MAX_RXBUF_DESCS;
    pEmacRegs->RX0FLOWTHRESH = 0x1;
    pEmacRegs->MACCONTROL   |= (EMAC_MACCONTROL_RXBUFFERFLOW_ENABLE);

    /* clear the MAC address hash registers to 0 */
    pEmacRegs->MACHASH1 = 0;
    pEmacRegs->MACHASH2 = 0;

    /* Zero the receive buffer offset register */
    pEmacRegs->RXBUFFEROFFSET = 0;


    /* Clear all the UniCast receive . This will effectively disalbe any packet
     * reception.
     */
    pEmacRegs->RXUNICASTCLEAR = 0xFF;

    /* Setup receive multicast/broadcast/promiscous channel enable on channel 0.
     * We don't need to enable the multicast/promiscous modes. However, we do
     * need the broadcast receive capability.
     */
    pEmacRegs->RXMBPENABLE = EMAC_RXMBPENABLE_RXBROADEN; 

    /* Setup MACCONTROL register with apt value */
    pEmacRegs->MACCONTROL = EMAC_MACCONTROL_FULLDUPLEX_ENABLE;
    
    /* Clear all unused Tx and RX channel interrupts */
    pEmacRegs->TXINTMASKCLEAR = 0xFF;
    pEmacRegs->RXINTMASKCLEAR = 0xFF;

    /* Enable the Rx and Tx channel interrupt mask registers.
     * Setting channel0   RX & TX channel interrupts
     */
    pEmacRegs->RXINTMASKSET = 0x1;
    pEmacRegs->TXINTMASKSET = 0x1;

    /* Setting up Rx buffer descriptors */
    pEmacRxNext = pAdapter->m_pBaseRxBufs;
    for( Count = 0; Count < pAdapter->m_NumEmacRxBufDesc ; Count++)
    {
        pEmacRxCur            = pEmacRxNext;
        pEmacRxNext++;
        pRxDesc               = (PEMACDESC)pEmacRxCur->m_EmacBufDes;
        pRxDesc->pNext        = (PEMACDESC)(pEmacRxNext->m_EmacBufDesPa);
        pRxDesc->pBuffer      = (UINT8 *)(pEmacRxCur->m_BufPhysicalAddress);
        pRxDesc->BufOffLen    = EMAC_MAX_PKT_BUFFER_SIZE;
        pRxDesc->PktFlgLen    = EMAC_DSC_FLAG_OWNER;
     }

     pRxDesc-> pNext = 0;

    /* Setting up Tx buffer descriptors */
    pEmacTxNext = pAdapter->m_pBaseTxBufs;
    for( Count = 0; Count < pAdapter->m_MaxTxEmacBufs ; Count++)
    {
        pEmacTxCur            = pEmacTxNext;
        pEmacTxNext++;
        pTxDesc               = (PEMACDESC)pEmacTxCur->m_EmacBufDes;
        pTxDesc->pNext        = (PEMACDESC)(pEmacTxNext->m_EmacBufDesPa);
        pTxDesc->pBuffer      = 0;
        pTxDesc->BufOffLen    = EMAC_MAX_ETHERNET_PKT_SIZE;
        pTxDesc->PktFlgLen    = 0;
     }

    pTxDesc->pNext = 0;

    /* Adjust RX Length characteristics */
    pEmacRegs->RXMAXLEN = EMAC_RX_MAX_LEN;

    // Todo : Get the clock value using the clock SDK
    clkdiv = 166000000;
    clkdiv = (clkdiv/(EMAC_MDIO_CLOCK_FREQ)) - 1;

    pMdioRegs->CONTROL = ((clkdiv & 0xFF) |
                              (MDIO_CONTROL_ENABLE) |
                              (MDIO_CONTROL_FAULTEN) |
                              (MDIO_CONTROL_FAULT));

    // wait for MDIO to become active
    while (pMdioRegs->CONTROL & MDIO_CONTROL_IDLE) {
        //5ms sleep
        NdisMSleep(1000*5);
    };

    // wait for the phy(s) to indicate they are active
    for(Count = 0;Count < 256; Count++)
    {
        DEBUGMSG(TRUE, (TEXT("NICInitializeAdapter: waiting for active phy... 0x%x\r\n"), pMdioRegs->ALIVE));
        if(pMdioRegs->ALIVE)
            break;
        NdisMSleep(1000* 10);
    }

    // print out the Phy(s) that are active
    if(pMdioRegs->ALIVE)
        DEBUGMSG(TRUE, (TEXT("NICInitializeAdapter: pMdioRegs->ALIVE = 0x%x\r\n"), pMdioRegs->ALIVE));

#ifdef SUPPORT_TWO_ETH_PORT
    
    RetVal = PhyFindLink(pAdapter);
    while(0 == RetVal)
    {
        DEBUGMSG(DBG_WARN, (TEXT("WARN: NICInitializeAdapter --> Could not find the link!!!\r\n")));
        //1sec
        NdisMSleep(1000*1000);

        RetVal = PhyFindLink(pAdapter);
    }

    /* Monitor the PHY address for any link change  */
    pMdioRegs->USERPHYSEL0 |= pAdapter->m_ActivePhy;
    /* Enabling Link change status interrupt for PHY address being monitored */
    pMdioRegs->USERPHYSEL0 |= (BIT(6));
    /* Indicate that link is UP */
    pAdapter->m_LinkStatus = UP;

#else
    /* Supporting only Ethernet Port1 only if ethernet cable is not plug in 
     * while booting up
    */

    if(0 != PhyFindLink(pAdapter))
    {
        /* Indicate that link is UP */
        pAdapter->m_LinkStatus = UP;
    }
	else
	{
        pAdapter->m_LinkStatus = DOWN;
	}

    /* Monitor the PHY address for any link change  */
    pMdioRegs->USERPHYSEL0 |= pAdapter->m_ActivePhy;
    /* Enabling Link change status interrupt for PHY address being monitored */
    pMdioRegs->USERPHYSEL0 |= (BIT(6));

#endif

    /* Only coming here means complete initialisation is over */
    Status = NDIS_STATUS_SUCCESS;

    /* Enable Tx & Rx for channel 0 only */
    pEmacRegs->RXUNICASTSET = 0x01;
    pEmacRegs->TXCONTROL = 0x1;
    pEmacRegs->RXCONTROL = 0x1;
    pEmacRegs->MACCONTROL |= (EMAC_MACCONTROL_MIIEN_ENABLE | EMAC_MACCONTROL_RMII_SPEED);

    /* Enabling statistics and Host error interrupts */
    pEmacRegs->MACINTMASKSET = (BIT(1) | BIT(0));

    /* Start receive process */
    pEmacRegs->RX0HDP = (pAdapter->m_pBaseRxBufs->m_EmacBufDesPa);

    /* Enable the interrupt */
    //  pAdapter->m_EwrapRegsBase->m_Ewinttcnt = 0xFFFF;
    EmacEnableInterrupts(pAdapter);
    pEmacCtlRegs->INTCONTROL = 0x1;


    /* Here we have enabled broadcast and unicast reception */
    pAdapter->m_PacketFilter = (
                                NDIS_PACKET_TYPE_DIRECTED
                              );
    /* Clearing teardown events */
    pAdapter->m_Events = 0x0;

    DEBUGMSG(DBG_FUNC, (L"<-- NICInitializeAdapter, Status=%x \r\n", Status));

    return Status;
}

