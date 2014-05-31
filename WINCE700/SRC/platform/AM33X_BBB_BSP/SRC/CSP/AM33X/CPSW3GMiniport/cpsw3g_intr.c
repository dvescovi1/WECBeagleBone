//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "cpsw3g_miniport.h"
#include <halether.h>
#include "cpgmacisr.h"
#include "cpsw3g_proto.h"

extern NDIS_STATUS LoadISR( PCPSW3G_ADAPTER   pAdapter, DWORD SysIntr);

//========================================================================
//!  \fn VOID AddBufToRxQueue(NDIS_HANDLE MiniportAdapterContext,
//!                                PEMAC_RXPKTS pTmpRxPkt);
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
AddBufToRxQueue(
    NDIS_HANDLE MiniportAdapterContext,
    PCPSW3G_RXPKTS pTmpRxPkt,
    UINT32 channel
    )
{
    PCPSW3G_ADAPTER   pAdapter;
    PCPSW3G_RXBUFS    pRxBufsList;
    PCPSW3G_DESC      pRXDescPa;
    PCPSW3G_DESC      pRXDescVa;
    UINT32          RXTailDescPa;
    UINT32          RXTailDescVa;
    UINT32          BufStatus;
    
    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;
    
    /* Fetch the current buffer to be added */
    pRxBufsList = (PCPSW3G_RXBUFS)pTmpRxPkt->BufsList.m_pHead;

    /* the Queue handling in this function should be protected, assume the 
        spinlock is acquired outside of this function */
    while(pRxBufsList != 0)
    {

        /* Get the current Rx bufs physical and virtual buffer descriptor */
        pRXDescPa = (PCPSW3G_DESC)pRxBufsList->BufDesPa; 
        pRXDescVa = (PCPSW3G_DESC)pRxBufsList->BufDes; 
        BufStatus = pRXDescVa->PktFlgLen;

        /* Terminating the present descriptor to NULL */
        pRXDescVa->pNext = 0;
        
        /* Recylcling buffer length and ownership fields */
        pRXDescVa->BufOffLen    = CPSW3G_MAX_PKT_BUFFER_SIZE;
        pRXDescVa->PktFlgLen    = CPSW3G_CPPI_OWNERSHIP_BIT; 

        if(0 != QUEUE_COUNT(&pAdapter->RxBufsPool[channel]))
        {
            RXTailDescPa = ((PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pTail))->BufDesPa;
            RXTailDescVa = ((PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pTail))->BufDes;

            /* Attaching tail's buffer descriptor's next to got one */
            ((PCPSW3G_DESC)(RXTailDescVa))->pNext = pRXDescPa;
        }

        /* Write to increment the register the buffer we got back for
         * hardware flow control support
         */
        pAdapter->pCpsw3gRegsBase->RX_FreeBuffer[channel] = 1;
        
        /* Adjust the Buffer length */
        NdisAdjustBufferLength((PNDIS_BUFFER)pRxBufsList->BufHandle,
                                   CPSW3G_MAX_PKT_BUFFER_SIZE);

         /* Also adding to our Rx bufs pool */
        QUEUE_INSERT(&pAdapter->RxBufsPool[channel], pRxBufsList);
        pAdapter->Cpsw3g_sw_stats.rx_complete++;
         
        if(0 != (BufStatus & CPSW3G_CPPI_EOQ_BIT))
        {
            pAdapter->pCpsw3gRegsBase->Rx_HDP[channel] = 
                    (UINT32)(((PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pHead))->BufDesPa);
            pAdapter->Cpsw3g_sw_stats.rx_eoq++;

        }
                        
        /* Getting to next Rx buf if any */
        pRxBufsList = (PCPSW3G_RXBUFS)((PSLINK_T)pRxBufsList)->m_pLink;
    }
}

//========================================================================
//!  \fn VOID   UpdateStatInfoByCount(PEMAC_STATINFO EmacStatInfo, 
//!                                        PDWORD  StatRegVal, 
//!                                        USHORT StatReg)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
UpdateStatInfoByCount(
    PCPSW3G_STATINFO pCpsw3gStatInfo, 
    PDWORD  StatRegVal, 
    USHORT StatReg
    )
{
    /* Updating only the driver required statistics info from CPSW3G statistics
     * Register 
     */
      
    DWORD stats;

    stats = (*StatRegVal & 0x80000000)?
               (*StatRegVal):0;

    switch(StatReg)
    {
        case 0:
            pCpsw3gStatInfo->RxOKFrames += stats;   
            break;
        
        case 5:
            pCpsw3gStatInfo->RxAlignErrorFrames += stats;   
        case 4:
        case 6:
        case 7:
        case 8:
             pCpsw3gStatInfo->RxErrorframes += stats;   
             break;

        case 1:
            pCpsw3gStatInfo->RxBroadcastFrame += stats;   
            break;
        case 2:
            pCpsw3gStatInfo->RxMulticastFrame += stats;   
            break;

        case 13:
            pCpsw3gStatInfo->TxOKFrames += stats;   
            break;

        case 14:
            pCpsw3gStatInfo->TxBroadcastFrame += stats;   
            break;
            
        case 15:
            pCpsw3gStatInfo->TxMulticastFrame += stats;   
            break;
            
        case 17:
            pCpsw3gStatInfo->TxDeferred += stats;   
            break;
        
                
        case 19:
            pCpsw3gStatInfo->TxOneColl += stats;   
            break;
            
        case 20:    
            pCpsw3gStatInfo->TxMoreColl += stats;   
            break;
            
        case 21:
            pCpsw3gStatInfo->TxMaxColl += stats;
            break;
        case 22:
        case 24:     
            pCpsw3gStatInfo->TxErrorframes += stats;   
            break;
            
        case 35:
            pCpsw3gStatInfo->RxOverRun += stats;     
            pCpsw3gStatInfo->RxNoBufFrames += stats;   
            break;  

       default:
		   break;
	}  
        
}    
    
//========================================================================
//!  \fn VOID  StatIntrHandler(NDIS_HANDLE MiniportAdapterContext)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================

VOID
StatIntrHandler(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER   pAdapter;
    PDWORD          TempStatReg;  
    PDWORD          StatRegStart;
    USHORT          Count;
   
    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;
    
    StatRegStart = (PDWORD)&(pAdapter->pCpsw3gRegsBase->RxGoodFrames);
    
    TempStatReg = StatRegStart;
     
    for(Count = 0 ;Count < CPSW3G_NUM_STATS_REGS ; Count++)
    {
        if( 0x80000000 == (*TempStatReg & (0x80000000)))
        {
            /* Updating driver maintained structures */
            UpdateStatInfoByCount(&pAdapter->Cpsw3gStatInfo ,TempStatReg , Count);
            
            /* Writing any value greater  than  0x80000000 */
            *TempStatReg = 0x80000000; //*TempStatReg ; 
            break;
            
        }
        
        TempStatReg++;      
    }
    
}

//========================================================================
//!  \fn VOID   HostErrorIntrHandler(NDIS_HANDLE MiniportAdapterContext)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================    

VOID
HostErrorIntrHandler(
    NDIS_HANDLE MiniportAdapterContext
    )
{ 

    PCPSW3G_ADAPTER   pAdapter;
    DWORD DmaStatus;
    BOOLEAN reset;
    
    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext; 
    DmaStatus = pAdapter->pCpsw3gRegsBase->DMAStatus;
    if((DmaStatus >> 12) & 0xf) 
    {
        RETAILMSG (TRUE, (L"->HostErrorIntrHandler: RX error %x on channel %d\r\n", 
                               (DmaStatus >> 12) & 0xf,            
                               (DmaStatus >> 8) & 0x7));
    }
    if ((DmaStatus >> 20) & 0xf)
    {
        RETAILMSG (TRUE, (L"->HostErrorIntrHandler: TX error %x on channel %d\r\n", 
                               (DmaStatus >> 20) & 0xf,            
                               (DmaStatus >> 16) & 0x7));
    }

    /* Reset the CPSW3G modules */
    if(FALSE == Cpsw3g_Ethss_power_on())
    {
       DEBUGMSG (DBG_INFO, (L"Reset failure for CPSW3G modules \n"));
    }
    
    /* reset CPSW3g driver */
    Cpsw3g_MiniportReset(&reset, pAdapter);
}    

 //========================================================================
//!  \fn VOID   RxIntrHandler(NDIS_HANDLE MiniportAdapterContext);
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID RxIntrHandler(NDIS_HANDLE MiniportAdapterContext, UINT32 channel)
{
    PCPSW3G_ADAPTER   pAdapter;
    PCPSW3G_RXBUFS    pCurRxBuf;
    PNDIS_PACKET      NdisPktsArray[NDIS_INDICATE_PKTS];
    UINT32            Status;
    PCPSW3G_DESC      pCurDesc;
    USHORT            PacketArrayCount = 0;
    UINT16            BufLen;
    UINT16            Index;
    PCPSW3G_RXPKTS    pRxPkt;
    PNDIS_BUFFER      NdisBuffer;
    PCPSW3G_RXPKTS    *TempPtr;
    PCPSW3G_RXPKTS    pTmpRxPkt;
    PUCHAR            temp_addr, DestAddr; 
    NDIS_PACKET_8021Q_INFO VlanPriInfo;
    UINT16                 temp, vlan_type=0x8100;
    PVLAN_ETHHDR           vlan_hdr;
    UINT32                 RXTailDescVa;    

    PCPSW3G_DESC      pNextBdVa = NULL;
    DWORD             RcvBufDesBase;
    DWORD             RcvBufDesBasePa;

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;      
    DEBUGMSG(DBG_FUNC && DBG_TX, (L"  -> RxIntrHandler for channel %d \r\n",channel));

    /* Check if it is teardown interrupt then clear it 
     * and return 
     */
     if(CPSW3G_TEARDOWN == pAdapter->pCpsw3gRegsBase->Rx_CP[channel])
     {
        pAdapter->Events |= CPSW3G_RX_TEARDOWN_EVENT(channel);
        pAdapter->pCpsw3gRegsBase->Rx_CP[channel] = CPSW3G_TEARDOWN;
        DEBUGMSG(DBG_FUNC && DBG_TX, (L"RxIntrHandler: RX tear down, channel = %d! \r\n", channel));  
        return;
     }   
     
    // Prepare virt and phy address of buf desc base for 
    // calculating buf desc virt addr later.
    RcvBufDesBase   = (pAdapter->RxBDBase[channel]);
    RcvBufDesBasePa = NdisGetPhysicalAddressLow(pAdapter->RxBDBasePa[channel]);

    // Check rx buffer ownership before removing from the pool's head.
    pCurRxBuf = (PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pHead);
    if (PORT_OWN((PCPSW3G_DESC)(pCurRxBuf->BufDes)))
        return;

    do
    {
        /* Dequeue buffer from available buffers pool */
        NdisAcquireSpinLock(&pAdapter->RcvLock);         
        QUEUE_REMOVE(&pAdapter->RxBufsPool[channel], pCurRxBuf);
        NdisReleaseSpinLock(&pAdapter->RcvLock);    
        
        if(!pCurRxBuf)
        {
            /* This should not happen */
            RETAILMSG (TRUE, (L"  -> RxIntrHandler, NULL Receive Buffer\r\n"));
            NdisMSleep(100);
            break;
        }
            
        pAdapter->Cpsw3g_sw_stats.rx_in++;
        pCurDesc = (PCPSW3G_DESC)pCurRxBuf->BufDes;

        Status   = pCurDesc->PktFlgLen;
        BufLen   = (UINT16)(pCurDesc->BufOffLen & 0xFFFF);

        /* At this moment, there should be an valid packet */
        ASSERT(HOST_OWN(pCurDesc));

        /* Remove CRC */
        if(Status & CPSW3G_CPPI_PASS_CRC_BIT) BufLen -= 4;
        
        /* Process for bad packet received  */
        if ( 0 != (Status & CPSW3G_CPPI_SOP_BIT ))
        {
            /* Process if a teardown happened */
            if ( 0 != (Status & CPSW3G_CPPI_TEARDOWN_COMPLETE_BIT ))
            {
                DEBUGMSG (DBG_INFO, (L"Teardown in packet \r\n"));
                
                pAdapter->Events |= CPSW3G_RX_TEARDOWN_EVENT(channel);
            }
        }
        /* Process for single buffer packets */
        if ( ( 0 != (Status & CPSW3G_CPPI_SOP_BIT )) &&
             ( 0 != (Status & CPSW3G_CPPI_EOP_BIT )) )                            
        {
            /* Deque from available packet pool a fresh packet  */
            NdisAcquireSpinLock(&pAdapter->RcvLock); 
            QUEUE_REMOVE(&pAdapter->RxPktPool[channel], pRxPkt);

            if(!pRxPkt)
            {
                RETAILMSG(TRUE, (L"  -> RxIntrHandler, pRxPkt is not available \r\n"));
                //QUEUE_INSERT(&pAdapter->RxBufsPool[channel],pCurRxBuf);
                pAdapter->Cpsw3g_sw_stats.rx_out_of_pkt++;
                break;
            }

            /* For debugging purpose */
            ASSERT(pRxPkt->RxChannel == channel);
            
            /* Initialise per packet maintained queue of bufs list */
            QUEUE_INIT(&pRxPkt->BufsList);
            
            /* Getting the associated NDIS buffer from RxBuffer pool*/
            NdisBuffer = (PNDIS_BUFFER)pCurRxBuf->BufHandle;

            /* check Vlan header and remove tag */
            if(pAdapter->Cfg.vlanAware)
            {
                temp_addr = (PUCHAR)(pCurRxBuf->BufLogicalAddress) ;
                vlan_hdr =     (PVLAN_ETHHDR) temp_addr;

                if(vlan_hdr->vlan_proto == ntohs(vlan_type))
                {
                    temp = ntohs(vlan_hdr->vlan_tag);
                    DEBUGMSG (DBG_INFO, (L"  -> RxIntrHandler, temp data %x \r\n", temp));
                    VlanPriInfo.TagHeader.UserPriority = (temp>>13) & 0x7;
                    VlanPriInfo.TagHeader.VlanId = temp & 0xfff;
                    VlanPriInfo.TagHeader.CanonicalFormatId = 0;
                    
                    NDIS_PER_PACKET_INFO_FROM_PACKET((PNDIS_PACKET)pRxPkt->PktHandle, Ieee8021QInfo) = VlanPriInfo.Value; 
                    DestAddr = temp_addr + ETH_LENGTH_OF_ADDRESS *2;
                    temp_addr += ETH_LENGTH_OF_ADDRESS *2 + 4;
                    /* remove VLAN tag from packet */
                    BufLen -= 4;

                    NdisMoveMemory (DestAddr, (PUCHAR)temp_addr, BufLen - ETH_LENGTH_OF_ADDRESS *2);
                    pAdapter->Cpsw3g_sw_stats.rx_vlan_tagged_pkt++;

                }
                else
                {
                    DEBUGMSG (DBG_INFO, (L"  -> RxIntrHandler, raw data %x,  \r\n", vlan_hdr->vlan_proto));
                }
            }
            /* Adjust the buffer length in the NDIS_BUFFER */
            NdisAdjustBufferLength( NdisBuffer, BufLen);

            /* Also setting header size */
            NDIS_SET_PACKET_HEADER_SIZE((PNDIS_PACKET)pRxPkt->PktHandle,  CPSW3G_ETHERNET_HEADER_SIZE);
            
            /* Add it to array of packets */                   
            NdisPktsArray[PacketArrayCount] = (PNDIS_PACKET)pRxPkt->PktHandle;
        

            /* Also insert in to per packet maintained Buffer queue */
            QUEUE_INSERT(&pRxPkt->BufsList, pCurRxBuf);
                  
            /* Get the HALPacket associated with this packet */
             TempPtr = (PCPSW3G_RXPKTS *)(((PNDIS_PACKET)pRxPkt->PktHandle)->MiniportReserved);
             
            /* 
             * Assign back pointer from the NdisPacket to the
             * HALPacket. This will be useful, when NDIS calls
             * MiniportGetReturnedPackets function.
             */
             *TempPtr = pRxPkt;
            
            /* Add it to packets buffer list */
            NdisChainBufferAtFront(NdisPktsArray[PacketArrayCount],  NdisBuffer);

            /* Set status as success */
            NDIS_SET_PACKET_STATUS(NdisPktsArray[PacketArrayCount], NDIS_STATUS_SUCCESS );
            NdisReleaseSpinLock(&pAdapter->RcvLock);    

            PacketArrayCount++;
                       
            CacheRangeFlush((VOID *)pCurRxBuf->BufLogicalAddress, BufLen, CACHE_SYNC_DISCARD );
            
            /* Acknowledge to channel completion register our last processed buffer */
            pAdapter->pCpsw3gRegsBase->Rx_CP[channel] = pCurRxBuf->BufDesPa;
         } 
         else
         {

             RETAILMSG (TRUE, (L"  -> RxIntrHandler, invalid desc =%x, status=%x, dma_status=%x\r\n",
                pCurRxBuf->BufDesPa, Status, pAdapter->pCpsw3gRegsBase->DMAStatus));
             pAdapter->Cpsw3g_sw_stats.rx_invalid_desc++;
         
             /* got an invalid status in descriptor , re-queue the descriptor */
             if(pCurRxBuf)
            {
                pCurDesc = (PCPSW3G_DESC)pCurRxBuf->BufDes;

                pCurDesc->pNext = 0;

                /* Recylcling buffer length and ownership fields */
                pCurDesc->BufOffLen    = CPSW3G_MAX_PKT_BUFFER_SIZE;
                pCurDesc->PktFlgLen    = CPSW3G_CPPI_OWNERSHIP_BIT; 
                
                NdisAcquireSpinLock(&pAdapter->RcvLock);                 
                QUEUE_INSERT(&pAdapter->RxBufsPool[channel],pCurRxBuf);
                
                if(0 != QUEUE_COUNT(&pAdapter->RxBufsPool[channel]))
                {
                    RXTailDescVa = ((PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pTail))->BufDes;

                    /* Attaching tail's buffer descriptor's next to pCurRxBuf*/
                    ((PCPSW3G_DESC)(RXTailDescVa))->pNext = (PCPSW3G_DESC)pCurRxBuf->BufDesPa;
                }
                NdisReleaseSpinLock(&pAdapter->RcvLock);    
               
            }
             
            /* Acknowledge to channel completion register our last processed buffer */
            pAdapter->pCpsw3gRegsBase->Rx_CP[channel] = pCurRxBuf->BufDesPa;

         }

         // Calculate the VA of pCurDesc->pNext for checking next bd ownership.
         // Note: Here we assume pCurDesc->pNext is not NULL in the calculation.
         //       The non-NULL check of pCurDesc->pNext in the while conditional
         //       is necessary before using the result of this calculation
         pNextBdVa = (PCPSW3G_DESC)(((DWORD)(pCurDesc->pNext) - RcvBufDesBasePa) + RcvBufDesBase);

    } while (
                (pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked & (0x1<<channel)) &&
                (NULL != pCurDesc->pNext) &&  // there is a BD next
                HOST_OWN(pNextBdVa) &&        // using corresponding VA to check its ownership
                (PacketArrayCount <= pAdapter->Cfg.rx_serviceMax)
            );

    if (0 == PacketArrayCount)
    {
        return;
    }

    /* Indicate to NDIS */                        
    NdisMIndicateReceivePacket(pAdapter->AdapterHandle , NdisPktsArray, PacketArrayCount);
    pAdapter->Cpsw3g_sw_stats.rx_indicated+= PacketArrayCount;
    /* Check if the NDIS has returned the ownership of any of
     * the packets back to us.
     */
    NdisAcquireSpinLock(&pAdapter->RcvLock); 
    for (Index = 0; Index < PacketArrayCount ; Index++ )
    {
        NDIS_STATUS ReturnStatus;
        
        ReturnStatus = NDIS_GET_PACKET_STATUS(NdisPktsArray[Index]);
         
       
        /* we can regain ownership of packets only for which pending status is
         * not set
         */
        
        if (ReturnStatus != NDIS_STATUS_PENDING)
        {
            
            /* Get the HALPacket associated with this packet */
            TempPtr = (PCPSW3G_RXPKTS *)(NdisPktsArray[Index]->MiniportReserved);
             
            pTmpRxPkt = *TempPtr; 
                
            /* This has every information about packets information like
             * buffers chained to it. This will be useful when we are adding
             * associated buffers to EMAC buffer descriptor queue
             */
     
            
            /* Unchain buffer attached preventing memory leak */
            NdisUnchainBufferAtFront(NdisPktsArray[Index],
                            &NdisBuffer);

             /* 
              * Reinitialize the NDIS packet for later use.
              * This will remove the NdisBuffer Linkage from
              * the NDIS Packet.
              */
            NdisReinitializePacket(NdisPktsArray[Index]);
            AddBufToRxQueue(pAdapter , pTmpRxPkt, channel);
            /* Also clearing OOB data */
            NdisZeroMemory(NDIS_OOB_DATA_FROM_PACKET(NdisPktsArray[Index]),
                           sizeof(NDIS_PACKET_OOB_DATA));
            
            /* Also insert in packet got in to packet pool */
            QUEUE_INSERT(&pAdapter->RxPktPool[channel],pTmpRxPkt);
    
        }
    }  
    NdisReleaseSpinLock(&pAdapter->RcvLock);    
}

//========================================================================
//!  \fn VOID TxIntrHandler(NDIS_HANDLE MiniportAdapterContext)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
TxIntrHandler(NDIS_HANDLE MiniportAdapterContext, UINT32 channel )
{
    PCPSW3G_ADAPTER    pAdapter;
    PQUEUE_T                   QBufsList; 
    PCPSW3G_DESC          pHeadTxDescPa;
    PCPSW3G_DESC          pTailTxDescPa;
    PCPSW3G_DESC          pSOPTxDesc;
    PCPSW3G_DESC          pEOPTxDesc;
    PCPSW3G_TXPKT         pCurPktInfo;
    PCPSW3G_DESC          pCurTxDescVa;
    PCPSW3G_DESC          pBufTailDescVa;
    PCPSW3G_TXBUF         pNextTxBuf;
    PCPSW3G_TXBUF         pCurTxBuf;
    PCPSW3G_TXBUF         TxHeadBuf;
    ULONG                         Count;
    ULONG                         Status;

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;      
    
     /* Acquire the send lock */
    NdisAcquireSpinLock(&pAdapter->SendLock); 

    if(channel != pAdapter->curr_port_info->Tx_channel)
    {
        /* Error condition */
        RETAILMSG(TRUE, (L"Tx interrupt happened on channel %d, not the reserved channel %d", channel, 
                pAdapter->curr_port_info->Tx_channel));
        return;
    }
    /* Check if it is teardown interrupt then clear it 
    * and return 
    */
    if(0xfffffffc == pAdapter->pCpsw3gRegsBase->Tx_CP[channel])
    {
        pAdapter->Events |= CPSW3G_TX_TEARDOWN_EVENT(channel);
        pAdapter->pCpsw3gRegsBase->Tx_CP[channel] = 0xfffffffc;
        RETAILMSG(TRUE,(L"TxIntrHandler: TX tear down! \r\n"));  

        return;
    }   

    do
    {
        if(0 == QUEUE_COUNT(&pAdapter->TxPostedPktPool))
        {
          DEBUGMSG (DBG_INFO,(L"Got Tx interrupt without packet posted \r\n"));  
          break;
        }
        /* Dequeue packet from already  posted packets  pool */
        QUEUE_REMOVE(&pAdapter->TxPostedPktPool, pCurPktInfo);
        if(!pCurPktInfo)
        {
            DEBUGMSG (DBG_INFO, (L"  -> TxIntrHandler, Tx Pkt is not available from Posted Pool \r\n"));
            break;
        }
            
        /* Extract the buffer descriptor information */
        QBufsList      = &pCurPktInfo->BufsList;
        pHeadTxDescPa  = (PCPSW3G_DESC)((PCPSW3G_TXBUF)(QBufsList->m_pHead))->BufDesPa;
        pTailTxDescPa  = (PCPSW3G_DESC)((PCPSW3G_TXBUF)(QBufsList->m_pTail))->BufDesPa;
        
        pSOPTxDesc    = (PCPSW3G_DESC)((PCPSW3G_TXBUF)(QBufsList->m_pHead))->BufDes;
        pEOPTxDesc    = (PCPSW3G_DESC)((PCPSW3G_TXBUF)(QBufsList->m_pTail))->BufDes; 

        Status   = pSOPTxDesc->PktFlgLen;
        /* Extract the flags and see if teardown has happened */

        if ( 0 != (Status & CPSW3G_CPPI_SOP_BIT ))
        {
            /* Process if a teardown happened */
            if ( 0 != (Status & CPSW3G_CPPI_TEARDOWN_COMPLETE_BIT ))
            {
                DEBUGMSG (DBG_INFO,(L"Teardown in packet \r\n"));
            
                pAdapter->Events |= CPSW3G_TX_TEARDOWN_EVENT(channel);
                goto end;
            
            }
        }
        Status   = pEOPTxDesc->PktFlgLen;

        if ( 0 != (Status & CPSW3G_CPPI_EOQ_BIT ))
        {
            pAdapter->Cpsw3g_sw_stats.tx_eoq++;
            /* Queue has been halted Let others can proceed if there is/are any */
            if(0 != QUEUE_COUNT(&pAdapter->TxPostedPktPool))
            {
                TxHeadBuf = (PCPSW3G_TXBUF)(((PCPSW3G_TXPKT)(pAdapter->TxPostedPktPool.m_pHead))->BufsList).m_pHead; 
                pAdapter->pCpsw3gRegsBase->Tx_HDP[channel] = TxHeadBuf->BufDesPa;
                pAdapter->Cpsw3g_sw_stats.tx_false_eoq++;
            }

         }
        /* Acknowledge to channel completion register our last processed buffer */
        
        pAdapter->pCpsw3gRegsBase->Tx_CP[channel] = (UINT32)pTailTxDescPa;
        pAdapter->Cpsw3g_sw_stats.tx_complete++;


        DEBUGMSG(DBG_INFO, (L"pCurPktInfo->PktHandle %08x Tx_cp %08x \r\n",pCurPktInfo->PktHandle,pTailTxDescPa));

        /* We can reuse this  packet info structure */                       
        QUEUE_INSERT(&pAdapter->TxPktsInfoPool,pCurPktInfo);
        
        /* We can also reuse associated Tx buffers also 
         * But before that we have to recycle the descriptor 
         * information 
         */
        pNextTxBuf = (PCPSW3G_TXBUF)(QBufsList->m_pHead);
        pCurTxDescVa  = (PCPSW3G_DESC)pNextTxBuf->BufDes;
        
        for(Count = QBufsList->m_Count; Count != 0 ;Count--)
        {

            pCurTxBuf                   = pNextTxBuf;
            pCurTxDescVa                = (PCPSW3G_DESC)pCurTxBuf->BufDes;
            pCurTxDescVa->pBuffer     = 0;
            pCurTxDescVa->BufOffLen   = CPSW3G_MAX_PKT_BUFFER_SIZE;
            pCurTxDescVa->PktFlgLen   = 0;

            /*NdisZeroMemory((PVOID)(pCurTxBuf->BufLogicalAddress),
                                CPSW3G_MAX_PKT_BUFFER_SIZE);*/
            pNextTxBuf = (PCPSW3G_TXBUF)((PSLINK_T)pCurTxBuf)->m_pLink;
            
        }
        pCurTxDescVa->pNext =0;  
        /* Also relinking chain */
        if(0 != QUEUE_COUNT(&pAdapter->TxBufInfoPool))
        {
        
            pBufTailDescVa =(PCPSW3G_DESC)((PCPSW3G_TXBUF)(pAdapter->TxBufInfoPool.m_pTail))->BufDes;
        
            pBufTailDescVa->pNext = pHeadTxDescPa;
        }
        
        QUEUE_INSERT_QUEUE(&pAdapter->TxBufInfoPool,QBufsList);
        DEBUGMSG(DBG_TX, (L"tx intr, re-queue tx buffer: pAdapter->TxBufInfoPool.Count %d \r\n",
                                            pAdapter->TxBufInfoPool.m_Count)); 

        NdisReleaseSpinLock(&pAdapter->SendLock);

        NdisMSendComplete(pAdapter->AdapterHandle, pCurPktInfo->PktHandle,  NDIS_STATUS_SUCCESS);

        NdisAcquireSpinLock(&pAdapter->SendLock); 
        
    } while (0 != ((BIT0<<pAdapter->curr_port_info->Tx_channel) & pAdapter->pCpsw3gRegsBase->Tx_IntStat_Masked));
    
       
end:
     
    NdisReleaseSpinLock(&pAdapter->SendLock);    
}

VOID SpfIntrHandler(NDIS_HANDLE MiniportAdapterContext)
{
    PCPSW3G_ADAPTER   pAdapter;

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;      

    SetEvent(pAdapter->Spf_intr_event);

    *(UINT32 *)(pAdapter->pCpsw3gRegsBase + 0x8A4) = 0x1;

}
//========================================================================
//!  \fn VOID   Cpsw3g_MiniportIsr(PBOOLEAN    InterruptRecognized,
//!                                    PBOOLEAN    QueueMiniportHandleInterrupt,
//!                                    NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is a required function if the driver's network 
//!         adapter generates interrupts.
//!  \param InterruptRecognized PBOOLEAN Points to a variable in which MiniportISR 
//!         returns whether the network adapter actually generated the interrupt. 
//!         MiniportISR sets this to TRUE if it detects that the interrupt came 
//!         from the network adapter designated at MiniportAdapterContext. 
//!  \param QueueMiniportHandleInterrupt PBOOLEAN Points to a variable that MiniportISR 
//!         sets to TRUE if the MiniportHandleInterrupt function should be called 
//!         to complete the interrupt-driven I/O operation. 
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return None.
//========================================================================

VOID        
Cpsw3g_MiniportIsr(
    PBOOLEAN    InterruptRecognized,
    PBOOLEAN    QueueMiniportHandleInterrupt,
    NDIS_HANDLE MiniportAdapterContext
    )
{
    //PCPSW3G_ADAPTER   pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;
    UNREFERENCED_PARAMETER(MiniportAdapterContext);

    *InterruptRecognized = TRUE;
    *QueueMiniportHandleInterrupt = TRUE;

    DEBUGMSG(DBG_INT,(L"  ->Cpsw3g_MiniportIsr\r\n"));

    DEBUGMSG(DBG_INT,(L"  <-Cpsw3g_MiniportIsr\r\n"));

    return;
}

//========================================================================
//!  \fn VOID   Cpsw3g_MiniportDisableInterrupt(NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is an optional function, supplied by drivers of network 
//!            adapters that support dynamic enabling and disabling of interrupts 
//!            but do not share an IRQ.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================

VOID        
Cpsw3g_MiniportDisableInterrupt(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER  pAdapter;

    DEBUGMSG (DBG_FUNC && DBG_INT,(L"  ->Cpsw3g_MiniportDisableInterrupt()\r\n"));

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;
    DEBUGMSG (DBG_FUNC && DBG_INT,(L"  <-Cpsw3g_MiniportDisableInterrupt()\r\n"));

    return;
}




VOID        
Cpsw3g_MiniportEnableInterrupt(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER  pAdapter;

    DEBUGMSG (DBG_FUNC && DBG_INT,(L"  ->Cpsw3g_MiniportEnableInterrupt()\r\n"));
    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;
    DEBUGMSG (DBG_FUNC && DBG_INT,(L"  <-Cpsw3g_MiniportEnableInterrupt()\r\n"));

    return;
}
//========================================================================
//!  \fn VOID   Cpsw3g_MiniportHandleInterrupt( NDIS_HANDLE  MiniportAdapterContext)
//!  \brief MiniportHandleInterrupt does the deferred processing of all outstanding 
//!         interrupt operations.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return None.
//========================================================================

VOID        
Cpsw3g_MiniportHandleInterrupt( 
    NDIS_HANDLE  MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER   pAdapter;
    UINT32 channel;
 //   UINT32  vector = 0;
    UINT32 eoi = 0xffffffff;

    DEBUGMSG(DBG_FUNC && DBG_INT,(L"  ->Cpsw3g_MiniportHandleInterrupt()for %x\r\n", 
        MiniportAdapterContext));

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;

    if (NdisDeviceStateD3 == pAdapter->CurrentPowerState){
        return ;
    }

//    vector = pAdapter->pCpsw3gRegsBase->CPDMA_In_Vector;

    /* Check TX pending */
    channel = 0;
    while (channel < CPDMA_MAX_CHANNELS)
    {
        if ((pAdapter->pCpsw3gRegsBase->Tx_IntStat_Masked & (0x1 << channel)) && 
           (channel == pAdapter->curr_port_info->Tx_channel))
        {
            TxIntrHandler(pAdapter, channel);        
            eoi = 0x2;
            break;
        }

        ++channel;
    }

    if (eoi == 0x2)
        pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = eoi;

    /* Check RX pending */
    channel = 0;
    while (channel < CPDMA_MAX_CHANNELS)
    {
        if((pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked & (0x1<<channel)))
        {
            if(pAdapter->KITL_enable && (pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL)  && 
                (channel == KITL_CHANNEL))
            {
                //RETAILMSG(TRUE, (L"  ->Cpsw3g_MiniportHandleInterrupt(), KITL intr, channel = %d!!!\r\n", 
                //                 channel));
            }
            else if( (0x1<<channel) & pAdapter->curr_port_info->RxPortChanMask)
            {
                RxIntrHandler(pAdapter, channel);
                eoi = 0x1;
            }

            // Handler handles other channels also.  So break.
            break;
        }

        ++channel;
    }

    if (eoi == 0x1)
        pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = eoi;

    /* Check host interrupts */
    if (pAdapter->pCpsw3gRegsBase->DMA_IntStat_Masked & 0x2)
    {
        HostErrorIntrHandler(pAdapter);
        eoi = 0x3;
    }

    if (pAdapter->pCpsw3gRegsBase->DMA_IntStat_Masked & 0x1)
    {
        StatIntrHandler(pAdapter);
        eoi = 0x3;
    }

    /* Write EOI */
    if (eoi == 0x3)
        pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = eoi;

    DEBUGMSG(DBG_FUNC && DBG_INT,(L"  <- Cpsw3g_MiniportHandleInterrupt()\r\n"));

    return;
}


//========================================================================
//!  \fn VOID Cpsw3g_MiniportReturnPacket(NDIS_HANDLE MiniportAdapterContext,
//!                                        PNDIS_PACKET Packet)
//!  \brief This function is a required function in drivers 
//!         that indicate receives with NdisMIndicateReceivePacket.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \param Packet PNDIS_PACKET Points to a packet descriptor being returned to the 
//!         miniport, which previously indicated a packet array that contained 
//!         this pointer. 
//!  \return None.
//========================================================================

VOID 
Cpsw3g_MiniportReturnPacket(
    NDIS_HANDLE MiniportAdapterContext,
    PNDIS_PACKET Packet
    )
{
    
    PCPSW3G_RXPKTS    pTmpRxPkt;
    PNDIS_BUFFER	NdisBuffer = NULL;
    PCPSW3G_ADAPTER   pAdapter;
    
    pAdapter = (PCPSW3G_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_FUNC && DBG_RX, (L"  -> Cpsw3g_MiniportReturnPacket\r\n"));

    /* Unchain buffer attached preventing memory leak */
    NdisUnchainBufferAtFront(Packet,&NdisBuffer);  
    
    /* Reinitialize the NDIS packet for later use.
     * This will remove the NdisBuffer Linkage from
     * the NDIS Packet.
     */
    NdisReinitializePacket(Packet);    
    
    /* Get the HALPacket associated with this packet */
    pTmpRxPkt = *(PCPSW3G_RXPKTS *)(Packet->MiniportReserved);
    
    DEBUGMSG (DBG_INFO,(L"+ Cpsw3g_MiniportReturnPacket 8.2 %X \r\n",pTmpRxPkt));

    NdisAcquireSpinLock(&pAdapter->RcvLock); 
    /* This has every information about packets information like
     * buffers chained to it. This will be useful when we are adding
     * associated buffers to buffer descriptor queue
     */
    
    AddBufToRxQueue(pAdapter , pTmpRxPkt, pTmpRxPkt->RxChannel);

    /* Also clearing OOB data */
    NdisZeroMemory(NDIS_OOB_DATA_FROM_PACKET(Packet),
            sizeof(NDIS_PACKET_OOB_DATA));
    
    /* Also insert in packet got in to packet pool */
    QUEUE_INSERT(&pAdapter->RxPktPool[pTmpRxPkt->RxChannel],pTmpRxPkt);
    NdisReleaseSpinLock(&pAdapter->RcvLock);

    DEBUGMSG(DBG_FUNC && DBG_RX, (L"  <- Cpsw3g_MiniportReturnPacket\r\n"));
}


NDIS_STATUS Cpsw3g_MiniportRegisterOneInterrupt(
    PCPSW3G_ADAPTER          pAdapter,
    PNDIS_MINIPORT_INTERRUPT InterruptInfo, 
    UINT                     Interrupt
)
{
    NDIS_STATUS   Status;
//    UINT32        SysIntr;

    /*  Register the interrupt */
    Status = NdisMRegisterInterrupt(
                 InterruptInfo,
                 pAdapter->AdapterHandle,
                 Interrupt,
                 0,          //ignored
                 FALSE,      // RequestISR
                 FALSE,      // SharedInterrupt
                 0);         //ignored

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(ZONE_ERRORS,
            (TEXT("Cpsw3g_MiniportRegisterInterrupts:: Failed to register interrupt %d\r\n"), 
             Interrupt)
        );

        return Status;
    }

    // Clear interrupt in INTC just in case ...
    InterruptDone(InterruptInfo->InterruptObject->InterruptId);

    DEBUGMSG(DBG_FUNC, 
        (TEXT("  ** cpsw3g_miniport: The sysintr chosen for %d by NDIS is %d\r\n"),  
        Interrupt, InterruptInfo->InterruptObject->InterruptId) );

    if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC)
    {
//+++FIXME: needed for 2 MAC ports
#if 0
        SysIntr = InterruptInfo->InterruptObject->InterruptId;
        Status = LoadISR(pAdapter, SysIntr);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(ZONE_ERRORS,
                (TEXT("CPGMAC:: Failed LoadISR() for Interrupt %d (SysIntr%d).\r\n"), 
                 Interrupt, InterruptInfo->InterruptObject->InterruptId));
        }
#endif
    }
  
    return Status;
}


NDIS_STATUS Cpsw3g_MiniportRegisterInterrupts(PCPSW3G_ADAPTER  pAdapter)
{
    NDIS_STATUS       Status;

    Status = Cpsw3g_MiniportRegisterOneInterrupt(
                 pAdapter, 
                 &pAdapter->RxThreshInterruptInfo, 
                 pAdapter->Cfg.RxThreshInterrupt);

    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = Cpsw3g_MiniportRegisterOneInterrupt(
                 pAdapter, 
                 &pAdapter->RxInterruptInfo, 
                 pAdapter->Cfg.RxInterrupt);

    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = Cpsw3g_MiniportRegisterOneInterrupt(
                 pAdapter, 
                 &pAdapter->TxInterruptInfo, 
                 pAdapter->Cfg.TxInterrupt);

    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = Cpsw3g_MiniportRegisterOneInterrupt(
                 pAdapter, 
                 &pAdapter->MiscInterruptInfo, 
                 pAdapter->Cfg.MiscInterrupt);

    return Status;
}

