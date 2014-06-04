//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Intr.c
//! \brief Contains interrupt related information of miniport.
//! 
//! This source File contains the interrupt routines and necessary handling 
//! functions.
//! 
//! \version  1.00 Aug 22nd 2006 File Created 

// Includes
#include <Ndis.h>
#include "Emac_Adapter.h"
#include "Emac_Queue.h"
#include "Emac_Proto.h"

#ifdef DUMP_FRAME
    #define htons( value ) ((UINT16)((((UINT16)value) << 8) | (((UINT16)((UINT16)value)) >> 8)))
    #define ntohs( value ) htons( value )
    #define htonl( value ) ((((ULONG)value) << 24) | ((0x0000FF00UL & ((ULONG)value)) << 8) | ((0x00FF0000UL & ((ULONG)value)) >> 8) | (((ULONG)value) >> 24))
    #define ntohl( value ) htonl( value )
#endif

#ifdef DUMP_FRAME
//========================================================================
//!  \fn DumpEtherFrame()
//!  \brief Test routine for Dumping the given ethernet Frame
//!  \param void.
//!  \return UINT32 containing the SYSINR.
//========================================================================

void
DumpEtherFrame(
              BYTE*   pFrame,
              USHORT  cwFrameLength
              )
{

    int i,j;
    #if 1
    if(pFrame[0] == 0xFF && pFrame[3]== 0xFF &&  pFrame[5]== 0xFF &&
       pFrame[1] == 0xFF && pFrame[2]== 0xFF && pFrame[4]== 0xFF )
    {
         DEBUGMSG( TRUE ,(L"Broadcast got \r\n"));
         return;
    }   
    #endif
    DEBUGMSG( TRUE ,(L"Frame Buffer Address: 0x%08X\r\n", pFrame ));

    DEBUGMSG( TRUE ,(L"To: %x:%x:%x:%x:%x:%x  From: %x:%x:%x:%x:%x:%x" \
                           L" Type: %x Length: %d \r\n",
                           pFrame[0], pFrame[1], pFrame[2], pFrame[3], pFrame[4], pFrame[5],
                           pFrame[6], pFrame[7], pFrame[8], pFrame[9], pFrame[10], pFrame[11],
                           ntohs(*((UINT16 *)(pFrame + 12))),cwFrameLength ));

    for ( i = 0; i < cwFrameLength / 16; i++ )
        {
        for ( j = 0; j < 16; j++ )
            {
            DEBUGMSG( TRUE ,(L"%x \t", pFrame[i*16 + j]) );
            }

        DEBUGMSG( DBG_INFO,( L"\r\n") );
        }
    for ( j = 0; j < cwFrameLength % 16; j++ )
        {
        DEBUGMSG( TRUE ,(L"%x \t", pFrame[i*16 + j] ));
        }


    DEBUGMSG( TRUE ,(L"\r\n" ));
}

#endif


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
    PEMAC_RXPKTS pTmpRxPkt
    )
{
    PEMAC_ADAPTER   pAdapter;
    PEMAC_RXBUFS    pEmacRxBufsList;
    PEMACDESC       pRXDescPa;
    PEMACDESC       pRXDescVa;
    UINT32          RXTailDescPa;
    UINT32          RXTailDescVa;
    UINT32          TailPktStatus;
    
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;
    
    /* Acquire the Receive lock */
    NdisAcquireSpinLock(&pAdapter->m_RcvLock);

    /* Fetch the current buffer to be added */
    pEmacRxBufsList = (PEMAC_RXBUFS)pTmpRxPkt->m_BufsList.m_pHead;
    
    while(pEmacRxBufsList != 0)
    {
        /* Get the current Rx bufs physical and virtual EMAC buffer descriptor */
        pRXDescPa = (PEMACDESC)pEmacRxBufsList->m_EmacBufDesPa; 
        pRXDescVa = (PEMACDESC)pEmacRxBufsList->m_EmacBufDes; 
        
        /* Terminating the present descriptor to NULL */
        pRXDescVa->pNext = 0;
        
        /* Recylcling buffer length and ownership fields */
        pRXDescVa->BufOffLen    = EMAC_MAX_PKT_BUFFER_SIZE;
        pRXDescVa->PktFlgLen    = EMAC_DSC_FLAG_OWNER; 

        if(0 != QUEUE_COUNT(&pAdapter->m_RxBufsPool))
        {
            RXTailDescPa = ((PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pTail))->m_EmacBufDesPa;
            RXTailDescVa = ((PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pTail))->m_EmacBufDes;

            /* Attaching tail's EMAC buffer descriptor's next to got one */
            ((PEMACDESC)(RXTailDescVa))->pNext = pRXDescPa;
            
             if(0 != (((PEMACDESC)(RXTailDescVa))->PktFlgLen & EMAC_DSC_FLAG_EOQ))
            {
                TailPktStatus = ((PEMACDESC)(RXTailDescVa))->PktFlgLen;
                TailPktStatus &= ~(EMAC_DSC_FLAG_EOQ);
                ((PEMACDESC)(RXTailDescVa))->PktFlgLen = TailPktStatus;
                pAdapter->m_pEmacRegsBase->RX0HDP = (UINT32)pRXDescPa; 
            }
        }
        else 
        {
            
            pAdapter->m_pEmacRegsBase->RX0HDP = (UINT32)pRXDescPa;
            
           // pAdapter->m_RxQueueActive = TRUE;    
        }

        /* Write to increment the register the buffer we got back for
         * hardware flow control support
         */
        pAdapter->m_pEmacRegsBase->RX0FREEBUFFER = 0x1;
      
        /* Clearing memory */
        NdisZeroMemory((PVOID)pEmacRxBufsList->m_BufLogicalAddress,
                                EMAC_MAX_PKT_BUFFER_SIZE);
   
        /* Adjust the Buffer length */
        NdisAdjustBufferLength((PNDIS_BUFFER)pEmacRxBufsList->m_BufHandle,
                                   EMAC_MAX_PKT_BUFFER_SIZE);

         /* Also adding to our Rx bufs pool */
        QUEUE_INSERT(&pAdapter->m_RxBufsPool,pEmacRxBufsList);
                
        /* Getting to next Rx buf if any */
        pEmacRxBufsList = (PEMAC_RXBUFS)((PSLINK_T)pEmacRxBufsList)->m_pLink;
    }       
    
     NdisReleaseSpinLock(&pAdapter->m_RcvLock);
}
//========================================================================
//!  \fn VOID   RxIntrHandler(NDIS_HANDLE MiniportAdapterContext);
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
RxIntrHandler(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER   pAdapter;
    PEMAC_RXBUFS    pCurRxBuf;
    PEMAC_RXBUFS    pNextRxBuf;
    PNDIS_PACKET    NdisPktsArray[NDIS_INDICATE_PKTS];
    UINT32          Status;
    PEMACDESC       pCurDesc;
    USHORT          PacketArrayCount = 0;
    UINT16          BufLen;
    UINT16          Index;
    PEMAC_RXPKTS    pRxPkt;
    PNDIS_BUFFER    NdisBuffer;
    PEMAC_RXPKTS    *TempPtr;
    PEMAC_RXPKTS    pTmpRxPkt;
    BOOL            EOQPktGot = FALSE;
    UINT32          RXTailDescVa;
    
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;      

    if(DOWN == pAdapter->m_LinkStatus)
    {
        return;
    }

    DEBUGMSG (DBG_FUNC,(L"-->RxIntrHandler\r\n"));

    pCurRxBuf =  pAdapter->m_pCurEmacRxBuf;
    
    /* Check if it is teardown interrupt then clear it 
     * and return 
     */
     if(0xfffffffc == pAdapter->m_pEmacRegsBase->RX0CP)
     {
        pAdapter->m_Events |= EMAC_RX_TEARDOWN_EVENT;
        pAdapter->m_pEmacRegsBase->RX0CP = 0xfffffffc;
        return;
     }   
                
    do
    {
      /* Dequeue buffer from available buffers pool */
      QUEUE_REMOVE(&pAdapter->m_RxBufsPool, pCurRxBuf);
  
        pNextRxBuf = (PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pHead);

      /* Extract the EMAC buffer descriptor information */
      pCurDesc = (PEMACDESC)pCurRxBuf->m_EmacBufDes;
      Status   = pCurDesc->PktFlgLen;
      BufLen   = pCurDesc->BufOffLen & 0xFFFF;
      
        /* Process for bad packet received  */
        if ( 0 != (Status & EMAC_DSC_FLAG_SOP ))
        {
            /* Process if a error in packet */
            if ( 0 != (Status & EMAC_DSC_RX_ERROR_FRAME ))
            {
                DEBUGMSG (DBG_INFO,
                    (L"Error in packet \r\n"));
            }
            /* Process if a teardown happened */
            if ( 0 != (Status & EMAC_DSC_FLAG_TDOWNCMPLT ))
            {
                DEBUGMSG (DBG_INFO,
                    (L"Teardown in packet \r\n"));
                    
                pAdapter->m_Events |= EMAC_RX_TEARDOWN_EVENT;
            }
            
        }
        /* Process for single buffer packets */
      
        if ( ( 0 != (Status & EMAC_DSC_FLAG_SOP )) &&
             ( 0 != (Status & EMAC_DSC_FLAG_EOP )) )                            
        {
            /* Deque from available packet pool a fresh packet  */
            QUEUE_REMOVE(&pAdapter->m_RxPktPool, pRxPkt);

            /* Initialise per packet maintained queue of bufs list */
            QUEUE_INIT(&pRxPkt->m_BufsList);
            
            /* Getting the associated NDIS buffer from RxBuffer pool*/
            NdisBuffer = (PNDIS_BUFFER)pCurRxBuf->m_BufHandle;

            /* Adjust the buffer length in the NDIS_BUFFER */
            NdisAdjustBufferLength( NdisBuffer, BufLen);

            /* Also setting header size */
            NDIS_SET_PACKET_HEADER_SIZE((PNDIS_PACKET)pRxPkt->m_PktHandle,
                            EMAC_HEADER_SIZE);
            
            /* Add it to array of packets */                   
            NdisPktsArray[PacketArrayCount] = (PNDIS_PACKET)pRxPkt->m_PktHandle;
        

            /* Also insert in to per packet maintained Buffer queue */
            QUEUE_INSERT(&pRxPkt->m_BufsList,pCurRxBuf);
                  
            /* Get the HALPacket associated with this packet */
             TempPtr = (PEMAC_RXPKTS *)(((PNDIS_PACKET)pRxPkt->m_PktHandle)->MiniportReserved);
             
            /* 
             * Assign back pointer from the NdisPacket to the
             * HALPacket. This will be useful, when NDIS calls
             * MiniportGetReturnedPackets function.
             */
             *TempPtr = pRxPkt;
            
            /* Add it to packets buffer list */
            NdisChainBufferAtFront(NdisPktsArray[PacketArrayCount],
                               NdisBuffer);
          
        
            if(PacketArrayCount >= (pAdapter->m_NumRxIndicatePkts -4))
            {
                     /* Set status as resources */
                    NDIS_SET_PACKET_STATUS(NdisPktsArray[PacketArrayCount],
                                                         NDIS_STATUS_RESOURCES );
                            
            }       
            else
            {
                /* Set status as success */
                NDIS_SET_PACKET_STATUS(NdisPktsArray[PacketArrayCount],
                                                         NDIS_STATUS_SUCCESS );

            }
                 
            PacketArrayCount++;

            /* Acknowledge to channel completion register our last processed buffer */
            pAdapter->m_pEmacRegsBase->RX0CP = pCurRxBuf->m_EmacBufDesPa;
         } 
        
    } while ((NULL != pNextRxBuf) &&
            (0 ==(EMAC_DSC_FLAG_OWNER & ((PEMACDESC)(pNextRxBuf->m_EmacBufDes))->PktFlgLen)) &&
             (PacketArrayCount < NDIS_INDICATE_PKTS));
       
    if (0 == PacketArrayCount)
    {
        return;
    }
    
    /* Indicate to NDIS */                        
    NdisMIndicateReceivePacket(pAdapter->m_AdapterHandle , 
                        NdisPktsArray,
                        PacketArrayCount);

    /* Check if the NDIS has returned the ownership of any of
     * the packets back to us.
     */
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
            TempPtr = (PEMAC_RXPKTS *)(NdisPktsArray[Index]->MiniportReserved);
             
            pTmpRxPkt = *TempPtr; 
                
            /* This has every information about packets information like
             * buffers chained to it. This will be useful when we are adding
             * associated buffers to EMAC buffer descriptor queue
             */
     
            AddBufToRxQueue(pAdapter , pTmpRxPkt);
            
            /* Unchain buffer attached preventing memory leak */
            NdisUnchainBufferAtFront(NdisPktsArray[Index],
                            &NdisBuffer);

             /* 
              * Reinitialize the NDIS packet for later use.
              * This will remove the NdisBuffer Linkage from
              * the NDIS Packet.
              */
            NdisReinitializePacket(NdisPktsArray[Index]);
            
            /* Also clearing OOB data */
            NdisZeroMemory(NDIS_OOB_DATA_FROM_PACKET(NdisPktsArray[Index]),
                           sizeof(NDIS_PACKET_OOB_DATA));
                
            /* Also insert in packet got in to packet pool */
            QUEUE_INSERT(&pAdapter->m_RxPktPool,pTmpRxPkt);
        }
    }  

    /* Check for race condition - null RX0HDP and no EOQ flagged.  This appears to arise 
       when the EOQ flag is set on the tail BD some time after the null 'next' pointer
       is read by the EMAC.  If our EOQ check occurs after the 'next' pointer is set but 
       before the EOQ flag is set by the EMAC then RX0HDP does not get reset. */
    RXTailDescVa = ((PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pTail))->m_EmacBufDes;
    if (pAdapter->m_pEmacRegsBase->RX0HDP == 0 &&
        (((PEMACDESC)(RXTailDescVa))->PktFlgLen & EMAC_DSC_FLAG_EOQ) == 0)
    {
        /* Find the first free packet in the queue */
        pCurRxBuf = (PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pHead);
        while (pCurRxBuf && 0 == (((PEMACDESC)pCurRxBuf->m_EmacBufDes)->PktFlgLen & EMAC_DSC_FLAG_OWNER))
        {
            pCurRxBuf = (PEMAC_RXBUFS)(pCurRxBuf->m_pNext);
        }
        if (!pCurRxBuf)
        {
            pCurRxBuf = (PEMAC_RXBUFS)(pAdapter->m_RxBufsPool.m_pHead);
        }
        pAdapter->m_pEmacRegsBase->RX0HDP = pCurRxBuf->m_EmacBufDesPa;
    }

    DEBUGMSG(DBG_FUNC,(L"<--RxIntrHandler\r\n"));
}

//========================================================================
//!  \fn VOID TxIntrHandler(NDIS_HANDLE MiniportAdapterContext)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
TxIntrHandler(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER       pAdapter;
    PQUEUE_T            QBufsList; 
    PEMACDESC           pHeadTxDescPa;
    PEMACDESC           pTailTxDescPa;
    PEMACDESC           pSOPTxDesc;
    PEMACDESC           pEOPTxDesc;
    PEMAC_TXPKT         pCurPktInfo;
    PEMACDESC           pCurTxDescVa;
    PEMACDESC           pBufTailDescVa;
    PEMAC_TXBUF         pNextTxBuf;
    PEMAC_TXBUF         pCurTxBuf;
    PEMAC_TXBUF         TxHeadBuf;
    ULONG               Count;
    ULONG               Status;

    DEBUGMSG (DBG_FUNC,(L"-->TxIntrHandler\r\n"));

    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;      
    
     /* Acquire the send lock */
    NdisAcquireSpinLock(&pAdapter->m_SendLock); 
    /* Check if it is teardown interrupt then clear it 
     * and return 
     */
    if(0xfffffffc == pAdapter->m_pEmacRegsBase->TX0CP)
    {
        pAdapter->m_Events |= EMAC_TX_TEARDOWN_EVENT;
        pAdapter->m_pEmacRegsBase->TX0CP = 0xfffffffc;
        goto end;
    } 
                
    do
    {
        if(0 == QUEUE_COUNT(&pAdapter->m_TxPostedPktPool))
        {
          DEBUGMSG (DBG_INFO,(L"Got Tx interrupt without packet posted \r\n"));  
          goto end;
        
        }
        /* Dequeue packet from already  posted packets  pool */
        QUEUE_REMOVE(&pAdapter->m_TxPostedPktPool, pCurPktInfo);
            
        /* Extract the EMAC buffer descriptor information */
        QBufsList      = &pCurPktInfo->m_BufsList;
        pHeadTxDescPa  = (PEMACDESC)((PEMAC_TXBUF)(QBufsList->m_pHead))->m_EmacBufDesPa;
        pTailTxDescPa  = (PEMACDESC)((PEMAC_TXBUF)(QBufsList->m_pTail))->m_EmacBufDesPa;
        
        pSOPTxDesc    = (PEMACDESC)((PEMAC_TXBUF)(QBufsList->m_pHead))->m_EmacBufDes;
        pEOPTxDesc    = (PEMACDESC)((PEMAC_TXBUF)(QBufsList->m_pTail))->m_EmacBufDes; 

        Status   = pSOPTxDesc->PktFlgLen;
        /* Extract the flags and see if teardown has happened */
        
        if ( 0 != (Status & EMAC_DSC_FLAG_SOP ))
        {
            /* Process if a teardown happened */
            if ( 0 != (Status & EMAC_DSC_FLAG_TDOWNCMPLT ))
            {
            DEBUGMSG (DBG_INFO,(L"Teardown in packet \r\n"));
        
            pAdapter->m_Events |= EMAC_TX_TEARDOWN_EVENT;
            goto end;
            
            }
        }

        Status   = pEOPTxDesc->PktFlgLen;

        if ( 0 != (Status & EMAC_DSC_FLAG_EOQ ))
        {
            /* Queue has been halted Let others can proceed if there is/are any */
            if(0 != QUEUE_COUNT(&pAdapter->m_TxPostedPktPool))
            {
                TxHeadBuf = (PEMAC_TXBUF)(((PEMAC_TXPKT)(pAdapter->m_TxPostedPktPool.m_pHead))->m_BufsList).m_pHead; 
                pAdapter->m_pEmacRegsBase->TX0HDP = TxHeadBuf->m_EmacBufDesPa;
            }
         }
        /* Acknowledge to channel completion register our last processed buffer */
        
        pAdapter->m_pEmacRegsBase->TX0CP = (UINT32)pTailTxDescPa;

        DEBUGMSG(DBG_INFO, (L"pCurPktInfo->m_PktHandle %08x TX0CP %08x \r\n",
            pCurPktInfo->m_PktHandle,pTailTxDescPa));
    

        /* We can reuse this  packet info structure */                       
        QUEUE_INSERT(&pAdapter->m_TxPktsInfoPool,pCurPktInfo);
        
        /* We can also reuse associated Tx buffers also 
         * But before that we have to recycle the descriptor 
         * information 
         */
        pNextTxBuf=(PEMAC_TXBUF)(QBufsList->m_pHead);
        
        for(Count = QBufsList->m_Count; Count != 0 ;Count--)
        {
            pCurTxBuf                   = pNextTxBuf;
            pCurTxDescVa                = (PEMACDESC)pCurTxBuf->m_EmacBufDes;
            pCurTxDescVa->pBuffer       = 0;
            pCurTxDescVa->BufOffLen     = EMAC_MAX_ETHERNET_PKT_SIZE;
            pCurTxDescVa->PktFlgLen     = 0;

            NdisZeroMemory((PVOID)(pCurTxBuf->m_BufLogicalAddress),
                                EMAC_MAX_ETHERNET_PKT_SIZE);
            pNextTxBuf = (PEMAC_TXBUF)((PSLINK_T)pCurTxBuf)->m_pLink;
            
        }
        pCurTxDescVa->pNext = 0;  
        /* Also relinking chain */
        if(0 != QUEUE_COUNT(&pAdapter->m_TxBufInfoPool))
        {
            pBufTailDescVa =(PEMACDESC)((PEMAC_TXBUF)(pAdapter->m_TxBufInfoPool.m_pTail))->m_EmacBufDes;
            pBufTailDescVa->pNext = pHeadTxDescPa;
        }
        
        QUEUE_INSERT_QUEUE(&pAdapter->m_TxBufInfoPool,QBufsList);
        NdisReleaseSpinLock(&pAdapter->m_SendLock);

        NdisMSendComplete(pAdapter->m_AdapterHandle, 
                        pCurPktInfo->m_PktHandle,
                        NDIS_STATUS_SUCCESS);
                        
        NdisAcquireSpinLock(&pAdapter->m_SendLock); 
        
    } while (0 != (BIT(0) & pAdapter->m_pEmacRegsBase->TXINTSTATMASKED));


    DEBUGMSG (DBG_FUNC,(L"<--TxIntrHandler\r\n"));

end:
    NdisReleaseSpinLock(&pAdapter->m_SendLock);
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
    PEMAC_STATINFO EmacStatInfo, 
    PDWORD  StatRegVal, 
    USHORT StatReg
    )
{
    /* Updating only the driver required statistics info from EMAC statistics
     * Register value using count
     */
      
    
    switch(StatReg)
    {
        case 0:
            EmacStatInfo->m_RxOKFrames += *StatRegVal;   
            break;
        
        case 5:
            EmacStatInfo->m_RxAlignErrorFrames += *StatRegVal;   
        case 4:
        case 6:
        case 7:
        case 8:
             EmacStatInfo->m_RxErrorframes += *StatRegVal;   
             break;
        
        case 13:
            EmacStatInfo->m_TxOKFrames += *StatRegVal;   
            break;
        case 17:
            EmacStatInfo->m_TxDeferred += *StatRegVal;   
            break;
        
                
        case 19:
            EmacStatInfo->m_TxOneColl += *StatRegVal;   
            break;
            
        case 20:    
            EmacStatInfo->m_TxMoreColl += *StatRegVal;   
            break;
            
        case 21:
            EmacStatInfo->m_TxMaxColl += *StatRegVal;
            break;
        case 22:
        case 24:     
            EmacStatInfo->m_TxErrorframes += *StatRegVal;   
            break;  
        case 33:
        case 34:
            EmacStatInfo->m_RxOverRun += *StatRegVal;
            break;
                  
        case 35:
            EmacStatInfo->m_RxOverRun += *StatRegVal;     
            EmacStatInfo->m_RxNoBufFrames += *StatRegVal;   
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
    PEMAC_ADAPTER   pAdapter;
    PDWORD          TempStatReg;  
    PDWORD          StatRegStart;
    USHORT          Count;
    
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;
    
    StatRegStart = (PDWORD)&(pAdapter->m_pEmacRegsBase->RXGOODFRAMES);
    
    TempStatReg = StatRegStart;
     
    for(Count = 0 ;Count < EMAC_STATS_REGS ; Count++)
    {
        if( 0x80000000 <= INREG32(TempStatReg))
        {
            RETAILMSG(1,(TEXT("Stat reg %d: 0x%x\r\n"),Count,INREG32(TempStatReg)));
            break;
        }
        
        TempStatReg++;      
    }
    
    /* Updating driver maintained structures */
    UpdateStatInfoByCount(&pAdapter->m_EmacStatInfo ,TempStatReg , Count);
    
    if(INREG32(&(pAdapter->m_pEmacRegsBase->MACCONTROL)) & EMAC_MACCONTROL_GMII_ENABLE)
    {
        /* Decrement the value to 0 (this a Write-to-decrement register)*/    
         OUTREG32(TempStatReg,INREG32(TempStatReg));
    }
    else
    {
        /* Decrement the value to 0 (this a Read/Writeregister)*/    
        OUTREG32(TempStatReg,0);
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
    PEMAC_ADAPTER   pAdapter;
    DWORD           MacStatus;
    DWORD           TxIntrMask;
    DWORD           RXIntrMask;
    DWORD           IntrVal;
    
    DEBUGMSG (DBG_FUNC,(L"-->HostErrorIntrHandler()\r\n"));

    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext; 
    
    MacStatus   = pAdapter->m_pEmacRegsBase->MACSTATUS;
    DEBUGMSG (DBG_FUNC,(L"status 0x%x\r\n",MacStatus));
    TxIntrMask  = (BIT(23) | BIT(22) | BIT(21) | BIT(20));
    RXIntrMask  = (BIT(15) | BIT(14) | BIT(13) | BIT(12));
    
    if(IntrVal =  ((MacStatus & TxIntrMask) >> 20)) 
    {
        if((0x1 == IntrVal) || 
           (0x2 == IntrVal) ||
           (0x3 == IntrVal) ||
           (0x4 == IntrVal) ||
           (0x5 == IntrVal) ||
           (0x6 == IntrVal))
        {   
            DEBUGMSG (DBG_INFO,(L"Tx error() %d\r\n",IntrVal));  
        }    
    }       
    else if(IntrVal =  ((MacStatus & RXIntrMask) >> 12)) 
    {
        if((0x2 == IntrVal) || 
           (0x4 == IntrVal))
        {
            DEBUGMSG (DBG_INFO,(L"Rx error() %d\r\n",IntrVal));  
        }   
    }    
    
    /* Reset the EMAC modules */
    EMACModStateChange(pAdapter->m_device,SYNCRST);

    /* Initialise once again the EMAC */
    NICInitializeAdapter(pAdapter);
    
}    
        
//========================================================================
//!  \fn VOID   Emac_MiniportIsr(PBOOLEAN    InterruptRecognized,
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
Emac_MiniportIsr(
    PBOOLEAN    InterruptRecognized,
    PBOOLEAN    QueueMiniportHandleInterrupt,
    NDIS_HANDLE MiniportAdapterContext
    )
{
    *InterruptRecognized = TRUE;
    *QueueMiniportHandleInterrupt = TRUE;

//    DEBUGMSG (DBG_FUNC,(L"+Emac_MiniportIsr()\r\n"));

    return;
}

void EmacEnableInterrupts(PEMAC_ADAPTER  pAdapter)
{
    if(pAdapter->m_pEmacCtlRegs)
    {
        pAdapter->m_pEmacRegsBase->MACEOIVECTOR = 0x1;
        pAdapter->m_pEmacRegsBase->MACEOIVECTOR = 0x2;
        pAdapter->m_pEmacRegsBase->MACEOIVECTOR = 0x3;
        pAdapter->m_pEmacCtlRegs->C0RXTHRESHEN=0xff;
        pAdapter->m_pEmacCtlRegs->C0RXEN=0x01;
        pAdapter->m_pEmacCtlRegs->C0TXEN=0x01;
        pAdapter->m_pEmacCtlRegs->C0MISCEN=0xf;
    }
}

void EmacDisableInterrupts(PEMAC_ADAPTER  pAdapter)
{
    if(pAdapter->m_pEmacCtlRegs)
    {
        pAdapter->m_pEmacCtlRegs->C0RXTHRESHEN=0x0;
        pAdapter->m_pEmacCtlRegs->C0RXEN=0x0;
        pAdapter->m_pEmacCtlRegs->C0TXEN=0x0;
        pAdapter->m_pEmacCtlRegs->C0MISCEN=0x0;
    }
}

//========================================================================
//!  \fn VOID   Emac_MiniportHandleInterrupt( NDIS_HANDLE  MiniportAdapterContext)
//!  \brief MiniportHandleInterrupt does the deferred processing of all outstanding 
//!         interrupt operations.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return None.
//========================================================================

VOID        
Emac_MiniportHandleInterrupt( 
    NDIS_HANDLE  MiniportAdapterContext
    )
{
    DWORD ack_flag = 0;
    DWORD           MacStatus;
    PEMAC_ADAPTER   pAdapter;
    
        
    //DEBUGMSG(DBG_FUNC && DBG_INT,(L"-->Emac_MiniportHandleInterrupt()\r\n"));
    
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    /* Since always interrupts are processed by disabling,servicing and enabling 
     * the interrupt in NDIS by calling Emac_MiniportDisableInterrupt and 
     * Emac_MiniportEnableInterrupt we need not take care of them here 
     */  
      
    MacStatus = pAdapter->m_pEmacRegsBase->MACINVECTOR;
    
	/* Receive interrupt */
    if( MacStatus & BIT(0) )
    {
        //DEBUGMSG (DBG_INFO,(L"+Receive Emac_MiniportHandleInterrupt()\r\n")); 
        RxIntrHandler(pAdapter);
        ack_flag |= ACK_RX;
    } 
    
    /* Trasmit interrupt */
    if( MacStatus & BIT(16) )
    {
        DEBUGMSG (DBG_INFO,(L"+ Trasmit Emac_MiniportHandleInterrupt()\r\n"));
        TxIntrHandler(pAdapter);
        ack_flag |= ACK_TX;
    }      
          
    /* Host error interrupt */
    if( MacStatus & BIT(26) )
    {
         DEBUGMSG (DBG_INFO,(L"+Host error Emac_MiniportHandleInterrupt()\r\n"));
         HostErrorIntrHandler(pAdapter);
         ack_flag |= ACK_MISC;
    }       
    
    /* Link change interrupt */
    if( MacStatus & BIT(25) )
    {
        DEBUGMSG (DBG_INFO,(L"+Link change interrupt()\r\n"));
        LinkChangeIntrHandler(pAdapter);
        ack_flag |= ACK_MISC;
    }
    
    /* Statistics interrupt */
    if( MacStatus & BIT(27) )
    {
        DEBUGMSG (DBG_INFO,(L"+Statistics Emac_MiniportHandleInterrupt()\r\n")); 
        StatIntrHandler(pAdapter);
        ack_flag |= ACK_MISC;
    }
    
    SocAckInterrupt(0xF);
    EmacEnableInterrupts(pAdapter);

    //DEBUGMSG(DBG_FUNC && DBG_INT,(L"<--Emac_MiniportHandleInterrupt()\r\n"));

    return;
}


//========================================================================
//!  \fn VOID   Emac_MiniportDisableInterrupt(NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is an optional function, supplied by drivers of network 
//!            adapters that support dynamic enabling and disabling of interrupts 
//!            but do not share an IRQ.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================

VOID        
Emac_MiniportDisableInterrupt(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER  pAdapter;
    
//    DEBUGMSG (DBG_FUNC && DBG_INT,(L"+Emac_MiniportDisableInterrupt()\r\n"));
    
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    EmacDisableInterrupts(pAdapter);

    return;
}
//========================================================================
//!  \fn VOID Emac_MiniportReturnPacket(NDIS_HANDLE MiniportAdapterContext,
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
Emac_MiniportReturnPacket(
    NDIS_HANDLE MiniportAdapterContext,
    PNDIS_PACKET Packet
    )
{
    PEMAC_RXPKTS    *TempPtr;
    PEMAC_RXPKTS    pTmpRxPkt;
    PNDIS_BUFFER    NdisBuffer = NULL;
    PEMAC_ADAPTER   pAdapter;
    
    pAdapter = (PEMAC_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_FUNC && DBG_RX, (L"---> Emac_MiniportReturnPacket\r\n"));

    /* Unchain buffer attached preventing memory leak */
    NdisUnchainBufferAtFront(Packet,&NdisBuffer);
    
    /* Reinitialize the NDIS packet for later use.
     * This will remove the NdisBuffer Linkage from
     * the NDIS Packet.
     */
    NdisReinitializePacket(Packet);    
    
    /* Get the HALPacket associated with this packet */
    TempPtr = (PEMAC_RXPKTS *)(Packet->MiniportReserved);
    
    pTmpRxPkt = *TempPtr; 
    
    DEBUGMSG (DBG_INFO,(L"+ Emac_MiniportReturnPacket 8.2 %X \r\n",pTmpRxPkt));
    
    /* This has every information about packets information like
     * buffers chained to it. This will be useful when we are adding
     * associated buffers to EMAC buffer descriptor queue
     */
    
    AddBufToRxQueue(pAdapter, pTmpRxPkt);
    
    /* Also clearing OOB data */
    NdisZeroMemory(NDIS_OOB_DATA_FROM_PACKET(Packet),
            sizeof(NDIS_PACKET_OOB_DATA));
    
    /* Also insert in packet got in to packet pool */
    QUEUE_INSERT(&pAdapter->m_RxPktPool, pTmpRxPkt);
    
    DEBUGMSG(DBG_FUNC && DBG_RX, (L"<--- Emac_MiniportReturnPacket\r\n"));

}
