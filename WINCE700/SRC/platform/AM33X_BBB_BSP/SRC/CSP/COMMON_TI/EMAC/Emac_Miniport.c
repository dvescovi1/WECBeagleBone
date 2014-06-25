//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Miniport.c
//! \brief Contains NDIS called other routines. 
//! 
//! This source File contains the OID query function as well as other NDIS
//! functions.
//! 
//! \version  1.00 Aug 22nd 2006 File Created 


// Includes
#include <Ndis.h>

#include "Emac_Adapter.h"
#include "Emac_Proto.h"
#include "Emac_Queue.h"

ULONG VendorDriverVersion = NIC_VENDOR_DRIVER_VERSION;

//------------------------ GLOBAL OID ------------------------------
//Supported OIDs
NDIS_OID NICSupportedOids[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,
    
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    
    
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,

    OID_PNP_CAPABILITIES,
    OID_PNP_QUERY_POWER,
    OID_PNP_SET_POWER
    
};

//========================================================================
//!  \fn VOID   Emac_MiniportHalt(NDIS_HANDLE MiniportAdapterContext)
//!  \brief MiniportHalt is a required function that de-allocates resources 
//!         when the network adapter is removed and halts the network adapter.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return None
//========================================================================

VOID        
Emac_MiniportHalt(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER       pAdapter;
    
    pAdapter = (PEMAC_ADAPTER) MiniportAdapterContext;

    DEBUGMSG(DBG_FUNC, (L"Emac_MiniportHalt \r\n"));
    
    /* Tear down receive and transmit channels so that Rx/Tx are completelly
     * stopped immediately
     */
    pAdapter->m_pEmacRegsBase->RXTEARDOWN = 0x0;
    pAdapter->m_pEmacRegsBase->TXTEARDOWN = 0x0;
     
    /* wait for teardowm completion */
    while(0 != (pAdapter->m_Events & EMAC_RX_TEARDOWN_EVENT) &&
          0 != (pAdapter->m_Events & EMAC_TX_TEARDOWN_EVENT))
    {
       ;
    }
    /*
     * Disable the interrupts in the card, so that the interrupt routine would
     * not be called.
     */
    Emac_MiniportDisableInterrupt(pAdapter);

    /* Free allocated memory and resources held */
    EmacFreeAdapter(pAdapter);
    
    return;
}
//========================================================================
//!  \fn EmacUpdateStatistics(PEMAC_STATINFO EmacStatInfo,PEMACREGS    EmacRegs) 
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================

VOID
EmacUpdateStatistics(
    PEMAC_STATINFO EmacStatInfo,
    PEMACREGS     EmacRegs 
    )
{
    /* Updating software statistics information from Network statistics registers
     * from EMAC
     */
    EmacStatInfo->m_TxOKFrames          += EmacRegs->TXGOODFRAMES;
    
    EmacStatInfo->m_RxOKFrames          += EmacRegs->RXGOODFRAMES;
    
    EmacStatInfo->m_TxErrorframes       += (
                                            EmacRegs->TXEXCESSIVECOLL +
                                            EmacRegs->TXLATECOLL +
                                            EmacRegs->TXCARRIERSENSE
                                            ); 
    
    EmacStatInfo->m_RxErrorframes       += (
                                            EmacRegs->RXCRCERRORS +
                                            EmacRegs->RXALIGNCODEERRORS +
                                            EmacRegs->RXOVERSIZED +
                                            EmacRegs->RXJABBER +
                                            EmacRegs->RXUNDERSIZED
                                            ); 
    
    EmacStatInfo->m_RxNoBufFrames       += EmacRegs->RXDMAOVERRUNS; //needtocheck
    
    EmacStatInfo->m_RxAlignErrorFrames  += EmacRegs->RXALIGNCODEERRORS;
    
    EmacStatInfo->m_TxOneColl           += EmacRegs->TXSINGLECOLL;
    
    EmacStatInfo->m_TxMoreColl          += EmacRegs->TXMULTICOLL;
    
    EmacStatInfo->m_TxDeferred          += EmacRegs->TXDEFERRED;
    
    EmacStatInfo->m_TxMaxColl           += EmacRegs->TXEXCESSIVECOLL;
    
    EmacStatInfo->m_RxOverRun           += (
                                             EmacRegs->RXSOFOVERRUNS +
                                             EmacRegs->RXMOFOVERRUNS +
                                             EmacRegs->RXDMAOVERRUNS 
                                           );
    
    
    
}    
//========================================================================
//!  \fn NDIS_STATUS Emac_MiniportQueryInformation(NDIS_HANDLE MiniportAdapterContext,
//!                                                        NDIS_OID Oid,
//!                                                        PVOID InformationBuffer,
//!                                                        ULONG InformationBufferLength,
//!                                                        PULONG BytesWritten,
//!                                                        PULONG BytesNeeded
//!                                                        )
//!  \brief This function is a required function that returns information 
//!         about the capabilities and status of the driver and/or its network adapter.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \param Oid NDIS_OID Specifies the system-defined OID_XXX code designating 
//!         the global query operation the driver should carry out. 
//!  \param InformationBuffer PVOID  Points to a buffer in which MiniportQueryInformation 
//!         should return the OID-specific information. 
//!  \param InformationBufferLength ULONG Specifies the number of bytes at InformationBuffer. 
//!  \param BytesWritten PULONG Points to a variable that MiniportQueryInformation 
//!         sets to the number of bytes it is returning at InformationBuffer. 
//!  \param BytesNeeded PULONG Points to a variable that MiniportQueryInformation 
//!         sets to the number of additional bytes it needs to satisfy the request 
//!         if InformationBufferLength is less than Oid requires.
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS 
Emac_MiniportQueryInformation(
    NDIS_HANDLE MiniportAdapterContext,
    NDIS_OID Oid,
    PVOID InformationBuffer,
    ULONG InformationBufferLength,
    PULONG BytesWritten,
    PULONG BytesNeeded
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    NDIS_MEDIUM                 Medium = NIC_MEDIA_TYPE;
    UCHAR                       VendorId[4];
    static const UCHAR          VendorDescriptor[] = "TI- EMAC ADAPTER";
    ULONG                       TempRegVal;
    ULONG                       ulInfo = 0;
    PVOID                       pInfo = (PVOID) &ulInfo;
    ULONG                       ulInfoLen = sizeof(ulInfo);
    ULONG                       ulBytesAvailable = ulInfoLen;
    PEMAC_ADAPTER               pAdapter;
    PEMAC_STATINFO              pEmacStatInfo;
    NDIS_PNP_CAPABILITIES       Capabilites;
    NDIS_DEVICE_POWER_STATE     PowerState;
    
    pAdapter = (PEMAC_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_FUNC && DBG_OID, 
            (L"---> Emac_MiniportQueryInformation 0x%08X \r\n",Oid));
   
    //
    // Process different type of requests
    //
    switch(Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            pInfo = (PVOID) NICSupportedOids;
            ulBytesAvailable = ulInfoLen = sizeof(NICSupportedOids);
            break;
        
        case OID_GEN_HARDWARE_STATUS:
            pInfo = (PVOID) &pAdapter->m_HwStatus;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;     
           
        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            pInfo = (PVOID) &Medium;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_MEDIUM);
            break;
            
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            ulInfo = (ULONG) EMAC_MAX_ETHERNET_PKT_SIZE;
            break;
        
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
            ulInfo = EMAC_MAX_ETHERNET_PKT_SIZE - EMAC_HEADER_SIZE;;
            break;
             
       case OID_GEN_TRANSMIT_BUFFER_SPACE:
            ulInfo = (ULONG) EMAC_MAX_ETHERNET_PKT_SIZE * pAdapter->m_MaxTxEmacBufs;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            ulInfo = (ULONG) EMAC_MAX_ETHERNET_PKT_SIZE * pAdapter->m_NumRxIndicatePkts;
            break;
            
        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = MAX_NUM_PACKETS_PER_SEND;
            break;
            
        case OID_GEN_MAC_OPTIONS:
            // Notes: 
            // The protocol driver is free to access indicated data by any means. 
            // Some fast-copy functions have trouble accessing on-board device 
            // memory. NIC drivers that indicate data out of mapped device memory 
            // should never set this flag. If a NIC driver does set this flag, it 
            // relaxes the restriction on fast-copy functions. 

            // This miniport indicates receive with NdisMIndicateReceivePacket 
            // function. It has no MiniportTransferData function. Such a driver 
            // should set this flag. 

            ulInfo = (
                      NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                      NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                      NDIS_MAC_OPTION_NO_LOOPBACK 
                     );
#if 0
            ulInfo |= NDIS_MAC_OPTION_FULL_DUPLEX    
#endif
            break;     
        
        case OID_GEN_VENDOR_ID:
            /* A 3-byte IEEE vendor code, followed by a single byte the vendor
             * assigns to identify a particular vendor-supplied network
             * interface card driver. The IEEE code uniquely identifies the
             * vendor and is the same as the three bytes appearing at the
             * beginning of the NIC hardware address. 
             */ 
            NdisMoveMemory(VendorId, pAdapter->m_MACAddress, 3);
            VendorId[3] = 0x0;
            pInfo = VendorId;
            ulBytesAvailable = ulInfoLen = sizeof(VendorId);
            break; 
                   
        case OID_GEN_VENDOR_DESCRIPTION:

            pInfo = (PVOID) &VendorDescriptor;
            ulBytesAvailable = ulInfoLen = sizeof(VendorDescriptor);
            break;
            
        case OID_GEN_VENDOR_DRIVER_VERSION:
            ulInfo = VendorDriverVersion;
            break;
        
        case OID_GEN_DRIVER_VERSION:

            ulInfo  =  EMAC_NDIS_DRIVER_VERSION;
            break;
                
        case OID_GEN_MEDIA_CONNECT_STATUS:
            
            
            if(UP == pAdapter->m_LinkStatus)
            {
                ulInfo = NdisMediaStateConnected;
            }
            else 
            {
                ulInfo  = NdisMediaStateDisconnected; 
            }
            DEBUGMSG (DBG_OID , (L"OID_GEN_MEDIA_CONNECT_STATUS is called %u \r\n",ulInfo));
            break;
       
        case OID_GEN_LINK_SPEED:
            
            TempRegVal = ReadPhyRegister(pAdapter->m_ActivePhy, MII_CONTROL_REG);
            if((0 == (TempRegVal & BIT(6))) && (0 == (TempRegVal & BIT(13))))
            {
                ulInfo = 100000;
            }
            else if((0 == (TempRegVal & BIT(6))) && (0 != (TempRegVal & BIT(13))))
            {
                ulInfo = 1000000;
            }
            else
            {
                DEBUGMSG(DBG_WARN,(L"WARN: Link is Down!!!!\r\n"));
            }

            DEBUGMSG (DBG_OID, (L"OID_GEN_LINK_SPEED is called %u\r\n",ulInfo));
            break;
        
        case OID_GEN_CURRENT_PACKET_FILTER:
            ulInfo = pAdapter->m_PacketFilter;
            break;               
            
        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
            pInfo = pAdapter->m_MACAddress;
            DEBUGMSG (DBG_OID, (L"Mac addr is %x:%x:%x:%x:%x:%x.\r\n",
            pAdapter->m_MACAddress[0],pAdapter->m_MACAddress[1],pAdapter->m_MACAddress[2],
            pAdapter->m_MACAddress[3],pAdapter->m_MACAddress[4],pAdapter->m_MACAddress[5]));
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            ulInfo = EMAC_MAX_MCAST_ENTRIES;
            break;
            
        case OID_802_3_MULTICAST_LIST:
             pInfo = pAdapter->m_MulticastTable;
             ulBytesAvailable = ulInfoLen = pAdapter->m_NumMulticastEntries *
                                                     ETH_LENGTH_OF_ADDRESS;   
            break;     
            
        case OID_PNP_CAPABILITIES:
            DEBUGMSG(1, (TEXT ("Emac_Miniport: QueryInformation: Got OID_PNP_CAPABILITIES\r\n")));
            Capabilites.WakeUpCapabilities.MinMagicPacketWakeUp = 
                Capabilites.WakeUpCapabilities.MinPatternWakeUp =
                Capabilites.WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
            pInfo = &Capabilites;
            ulBytesAvailable = sizeof(Capabilites);
            break;

        case OID_PNP_QUERY_POWER:
            DEBUGMSG(1, (TEXT ("Emac_Miniport: QueryInformation: Got OID_PNP_QUERY_POWER\r\n")));
            if (!InformationBuffer || InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE))
            {
                // Buffer not big enough
                Status = NDIS_STATUS_INVALID_LENGTH;
            }
            else
            {
                NdisMoveMemory((PVOID)&PowerState, InformationBuffer, sizeof(PowerState));
                ulBytesAvailable = 0;
                switch (PowerState)
                {
                case NdisDeviceStateD0:
                case NdisDeviceStateD3:
                    // Supported. Leave error code as success
                    break;
                default:
                    // Unsupported
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                };
            }
            break;

         default:
                /*
                 * This is a query for statistics information from the NDIS
                 * wrapper. Let us call EmacStatistics and retrieve the latest set
                 * of statistical information retrieved by the EMAC.
                 */
                 pEmacStatInfo = &pAdapter->m_EmacStatInfo;
                 
                 EmacUpdateStatistics(pEmacStatInfo , pAdapter->m_pEmacRegsBase);

                switch (Oid)
                {
    
                    case OID_GEN_XMIT_OK:
    
                        ulInfo = pEmacStatInfo->m_TxOKFrames;
                        break;
    
                    case OID_GEN_RCV_OK:
    
                        ulInfo = pEmacStatInfo->m_RxOKFrames;
                        break;
    
                    case OID_GEN_XMIT_ERROR:
    
                        ulInfo = pEmacStatInfo->m_TxErrorframes;
                        break;
    
                    case OID_GEN_RCV_ERROR:
                        ulInfo = pEmacStatInfo->m_RxErrorframes;
                        break;
    
                    case OID_GEN_RCV_NO_BUFFER:
                        ulInfo = pEmacStatInfo->m_RxNoBufFrames;
                        break;
    
                  
                    case OID_802_3_RCV_ERROR_ALIGNMENT:
                        ulInfo = pEmacStatInfo->m_RxAlignErrorFrames;
                        break;
    
                    case OID_802_3_XMIT_ONE_COLLISION:
                        ulInfo = pEmacStatInfo->m_TxOneColl;
                        break;
    
                    case OID_802_3_XMIT_MORE_COLLISIONS:
                        ulInfo = pEmacStatInfo->m_TxMoreColl;
                        break;
    
                    case OID_802_3_XMIT_DEFERRED:
                        ulInfo = pEmacStatInfo->m_TxDeferred;
                        break;
    
                    case OID_802_3_XMIT_MAX_COLLISIONS:
                        ulInfo = pEmacStatInfo->m_TxMaxColl;
                        break;
    
                    case OID_802_3_RCV_OVERRUN:
                        ulInfo = pEmacStatInfo->m_RxOverRun;
                        break;
    
                  
                    default:
                        Status = NDIS_STATUS_NOT_SUPPORTED;
                        break;
                }
    }

   if (Status == NDIS_STATUS_SUCCESS)
    {
        *BytesNeeded = ulBytesAvailable;
        if (ulInfoLen <= InformationBufferLength)
        {
            //
            // Copy result into InformationBuffer
            //
            *BytesWritten = ulInfoLen;
            if (ulInfoLen)
            {
                NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
            }
        }
        else
        {
            //
            // too short
            //
            *BytesNeeded = ulInfoLen;
            Status = NDIS_STATUS_INVALID_LENGTH;
        }
    }

    DEBUGMSG(DBG_FUNC && DBG_OID, 
            (L"<--- Emac_MiniportQueryInformation status 0x%08X \r\n",Status));
 
    return(Status);
}

#if 0
VOID
EmacAddPhyBufs(
    NDIS_HANDLE MiniportAdapterContext, 
    PNDIS_BUFFER PacketCurBuf,
    PEMAC_TXPKT  TxPktInfo
    )
{
    PEMAC_ADAPTER               Adapter;
    UINT                        ArraySize;
    NDIS_PHYSICAL_ADDRESS_UNIT  PhysicalUnits[MAX_DESC_PER_BUFFER];
    USHORT                      Count;
    PEMAC_TXBUF                 pTxBufInfo;
    
    Adapter = (PEMAC_ADAPTER)MiniportAdapterContext;
    DEBUGMSG(DBG_FUNC && DBG_TX, (L"---> EmacAddPhyBufs  \r\n"));
    
    /*
     * The NdisMStartBufferPhysicalMapping() call returns an array of
     * starting address of the physical addresses of the fragments in
     * the buffer.
     */
    NdisMStartBufferPhysicalMapping(Adapter->m_AdapterHandle,
                                    PacketCurBuf,
                                    0, //Not used 
                                    FALSE,
                                    &PhysicalUnits[0],
                                    &ArraySize);

   
    /*
     * For Each of the Physical Address element returned by the
     * NdisMStartBufferPhysicalMapping
     */
    for(Count = 0; Count < ArraySize; Count++)
    {
        /*
         * Dequeue a TxBufInfoPool structure, and assert that one is
         * available.
         */
        QUEUE_REMOVE(&Adapter->TxBufInfoPool, pTxBufInfo);
        
        
        QUEUE_INSERT(&Adapter->TxPerPktBufPool,pTxBufInfo); 
        
        /* Assign the physical address to the EMAC buffer descriptor element.*/
        DEBUGMSG(DBG_TX, (L"1.1.1 %x %x \r\n",pTxBufInfo->EmacBufDesPa,PhysicalUnits[Count].PhysicalAddress));
        /* clear all flags */
        ((PEMACDESC)(pTxBufInfo->EmacBufDes))->m_PktFlgLen = 0;
        /* Assign the physical buffer */
        ((PEMACDESC)(pTxBufInfo->EmacBufDes))->m_pBuffer = (UINT8* )NdisGetPhysicalAddressLow(
                                    PhysicalUnits[Count].PhysicalAddress
                                                    );
        pTxBufInfo->BufHandle = PacketCurBuf;
        if(0 == Count)
        {
            pTxBufInfo->Flags = TXBUF_FIRST_FRAGMENT;
            
        }
        /* Adjust the length */
        ((PEMACDESC)(pTxBufInfo->EmacBufDes))->m_BufOffLen = PhysicalUnits[Count].Length;
        
        /* Link the fragment to the Transmit packet info. structure. */
        QUEUE_INSERT(&TxPktInfo->BufsList, pTxBufInfo);
        
    }

    DEBUGMSG(DBG_FUNC && DBG_TX, (L"<--- EmacAddPhyBufs  \r\n"));
}
#endif

//========================================================================
//!  \fn VOID   CopyPacketToBuffer(PNDIS_PACKET Packet, 
//!                                        PNDIS_BUFFER FirstBuffer,
//!                                        PUCHAR buffer,
//!                                        ULONG BytesToCopy,
//!                                        PULONG BytesCopied
//!                                        )
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
VOID
CopyPacketToBuffer(
    PNDIS_PACKET Packet, 
    PNDIS_BUFFER FirstBuffer,
    PUCHAR buffer,
    ULONG BytesToCopy,
    PULONG BytesCopied
    )
{
    PNDIS_BUFFER        CurrBuffer;
    PUCHAR              BufferVirtAddr;
    ULONG               BufferLen;
    ULONG               BytesOnThisCopy;
    PUCHAR              DestAddr;
    
    
    *BytesCopied = 0;

    if(BytesToCopy == 0)
    {
            return;
    }
        
    DestAddr   = (PUCHAR) buffer;
    CurrBuffer = FirstBuffer;
    
    NdisQueryBuffer( CurrBuffer, &BufferVirtAddr, &BufferLen);

    do
    {
        while(!BufferLen)
        {
            NdisGetNextBuffer( CurrBuffer, &CurrBuffer);

            /* Check if we have reached the end of packet, if so return. */
            if (!CurrBuffer)
            {
                /* Assert BytesToCopy = 0 */
                ASSERT(BytesToCopy == 0);
                return;
            }
            NdisQueryBuffer(CurrBuffer, &BufferVirtAddr, &BufferLen);
        }

        BytesOnThisCopy = MIN(BytesToCopy, BufferLen);
        NdisMoveMemory(DestAddr, BufferVirtAddr, BytesOnThisCopy);

        (PUCHAR)DestAddr += BytesOnThisCopy;

        *BytesCopied += BytesOnThisCopy;
        BytesToCopy -= BytesOnThisCopy;

        BufferLen = 0;

    }while(BytesToCopy);
    
    return;
}    

//========================================================================
//!  \fn VOID Emac_MiniportSendPacketsHandler(NDIS_HANDLE  MiniportAdapterContext,
//!                                                PPNDIS_PACKET  PacketArray,
//!                                                UINT  NumberOfPackets);
//!
//!  \brief MiniportSendPackets transfers some number of packets, 
//!         specified as an array of packet pointers, over the network. 
//!  \param MiniportAdapterContext NDIS_HANDLE Pointer to receive pointer to our adapter
//!  \param PacketArray PPNDIS_PACKET Pointer to receive pointer to our adapter
//!  \param NumberOfPackets UINT Pointer to receive pointer to our adapter
//!  \return None.
//========================================================================    
VOID 
Emac_MiniportSendPacketsHandler(
    NDIS_HANDLE  MiniportAdapterContext,
    PPNDIS_PACKET  PacketArray,
    UINT  NumberOfPackets
    )
{  
    USHORT          Count;
    PNDIS_PACKET    CurPacket;
    PEMAC_ADAPTER   pAdapter;
    UINT            PhysicalBufCount;
    UINT            BufCount;
    UINT            TotalPktLength;
    UINT            CurBufAddress;
    UINT            CurBufAddressPa;
    PNDIS_BUFFER    FirstBuf;
    PEMAC_TXPKT     pTxPktInfo = NULL;
    PEMAC_TXBUF     TxHeadBuf;
    PEMAC_TXBUF     TxNextHeadBuf;
    PEMAC_TXBUF     TxPrevTailBuf;
    PEMAC_TXBUF     TxTailBuf;  
    PEMAC_TXBUF     pTxBufInfo;
    QUEUE_T         TmpPerSendPktPool;    

    ULONG BytesCopied;

    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    DEBUGMSG(DBG_FUNC && DBG_TX, (L"---> Emac_MiniportSendPacketsHandler %d \r\n",NumberOfPackets));
  
     /* Acquire the Send lock */
    NdisAcquireSpinLock(&pAdapter->m_SendLock); 

    if(DOWN == pAdapter->m_LinkStatus)
    {
        goto end;  
    }


    /* Initialise total packets maintained queue of bufs list */
    QUEUE_INIT(&TmpPerSendPktPool);
              
    for( Count = 0 ; Count < NumberOfPackets ; Count++)
    {
        /* Point to current packet */
        CurPacket = PacketArray[Count];
        
        /* Querying information about packet */
        NdisQueryPacket(CurPacket,
                       &PhysicalBufCount,
                       &BufCount,
                       &FirstBuf,
                       &TotalPktLength);
        
       /*  Number of Physical buffers, could be returned with invalid 
        *  higher order bits.
        */
        PhysicalBufCount &= 0x0000FFFF;
        
        DEBUGMSG(DBG_TX, (L"PhysicalBufCount %d\r\npAdapter->TxBufInfoPool.Count %d\r\nBufCount %d\r\nTotalPktLength %d\r\n",
            PhysicalBufCount,pAdapter->m_TxBufInfoPool.m_Count,BufCount,TotalPktLength)); 
   
        
        if(0 == QUEUE_COUNT(&pAdapter->m_TxPktsInfoPool))
        {
            DEBUGMSG(DBG_WARN, (L"Unable to send packet %d \r\n",QUEUE_COUNT(&pAdapter->m_TxPktsInfoPool)));
            /* Set status as resources */
            NDIS_SET_PACKET_STATUS(CurPacket,NDIS_STATUS_RESOURCES );
            
            /* we will return to indicate NDIS to resubmit remaining packets */
            continue;
        }

        /* we can transmit this packet 
         * Dequeue  from available packet pool a packet  
         */
        QUEUE_REMOVE(&pAdapter->m_TxPktsInfoPool, pTxPktInfo);
        
        /* Initialise per packet maintained queue of bufs list */
        QUEUE_INIT(&pTxPktInfo->m_BufsList);
        
        /* Pkt handle will be current packet */
        pTxPktInfo->m_PktHandle = CurPacket;
        
        /*
        * Dequeue a TxBufInfoPool structure, and assert that one is
        * available.
        */
        QUEUE_REMOVE(&pAdapter->m_TxBufInfoPool, pTxBufInfo);
        
        CurBufAddress  = pTxBufInfo->m_BufLogicalAddress;
        CurBufAddressPa = pTxBufInfo->m_BufPhysicalAddress;
        
        CopyPacketToBuffer(CurPacket,FirstBuf,
                    (PUCHAR)CurBufAddress,
                    TotalPktLength,
                    &BytesCopied);
        
        if(TotalPktLength < EMAC_MIN_ETHERNET_PKT_SIZE)
        {
            TotalPktLength = EMAC_MIN_ETHERNET_PKT_SIZE;
        }
        
        ((PEMACDESC)(pTxBufInfo->m_EmacBufDes))->pNext = 0;
        
        ((PEMACDESC)(pTxBufInfo->m_EmacBufDes))->PktFlgLen = (
                                                               EMAC_DSC_FLAG_SOP |
                                                               EMAC_DSC_FLAG_EOP | 
                                                               EMAC_DSC_FLAG_OWNER |
                                                               (TotalPktLength & 0xFFFF)
                                                               );
        ((PEMACDESC)(pTxBufInfo->m_EmacBufDes))->BufOffLen =  (TotalPktLength & 0xFFFF);
        
        ((PEMACDESC)(pTxBufInfo->m_EmacBufDes))->pBuffer = (UINT8 *)CurBufAddressPa;
        
        
        /* Insert in to per packet maintained buffer pool */
        QUEUE_INSERT(&pTxPktInfo->m_BufsList,pTxBufInfo);
        
        
        if(0 != QUEUE_COUNT(&TmpPerSendPktPool))
        {
            /* Since there are some other packets in to be posted send pkt pool
             * we have to link the current head of the to be posted send packet pool 
             * to the tail of temporary send packets pool
             */

            /* First we will fetch the tail buffer of temp send pkts pool */
            
            TxPrevTailBuf = (PEMAC_TXBUF)(((PEMAC_TXPKT)(TmpPerSendPktPool.m_pTail))->m_BufsList).m_pTail;
            TxNextHeadBuf = (PEMAC_TXBUF)((pTxPktInfo->m_BufsList).m_pHead);
            
            ((PEMACDESC)(TxPrevTailBuf->m_EmacBufDes))->pNext = (PEMACDESC)(TxNextHeadBuf->m_EmacBufDesPa);
         }
        /* add it to the to be posted packet pool */
       
        QUEUE_INSERT(&TmpPerSendPktPool,pTxPktInfo);
           
        /* Set status as pending as we are indicating asynchronously */
        
        NDIS_SET_PACKET_STATUS(CurPacket,NDIS_STATUS_PENDING );
        
    }/* end of for loop */
    
    if(0 == QUEUE_COUNT(&TmpPerSendPktPool))
    {
        goto end;   
    }
  
    if(0 == QUEUE_COUNT(&pAdapter->m_TxPostedPktPool))
    {
        /* Insert in to posted packets pool */
        QUEUE_INSERT_QUEUE(&pAdapter->m_TxPostedPktPool,&TmpPerSendPktPool);
        
        TxHeadBuf = (PEMAC_TXBUF)(((PEMAC_TXPKT)(pAdapter->m_TxPostedPktPool.m_pHead))->m_BufsList).m_pHead;
       
        /* Submit formed TX buffers queue to EMAC TX DMA */  
        pAdapter->m_pEmacRegsBase->TX0HDP = TxHeadBuf->m_EmacBufDesPa;
        
    }
    else   /* Posted packets is non zero */
    {    
        TxTailBuf = (PEMAC_TXBUF)(((PEMAC_TXPKT)(pAdapter->m_TxPostedPktPool.m_pTail))->m_BufsList).m_pTail;
        TxHeadBuf = (PEMAC_TXBUF)(((PEMAC_TXPKT)(TmpPerSendPktPool.m_pHead))->m_BufsList).m_pHead;
         
        /* Insert in to posted packets pool */
        QUEUE_INSERT_QUEUE(&pAdapter->m_TxPostedPktPool,&TmpPerSendPktPool);
            
        /* Insert in to posted packets pool */
        ((PEMACDESC)(TxTailBuf->m_EmacBufDes))->pNext = ((PEMACDESC)(TxHeadBuf->m_EmacBufDesPa));
        
    }

end:   
    NdisReleaseSpinLock(&pAdapter->m_SendLock);
    
    DEBUGMSG(DBG_FUNC && DBG_TX, (L"<--- Emac_MiniportSendPacketsHandler \r\n"));
}




//========================================================================
//!  \fn NDIS_STATUS Emac_MiniportReset(PBOOLEAN AddressingReset,
//!                                        NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is a required function that issues a hardware reset 
//!         to the network adapter and/or resets the driver's software state.
//!  \param AddressingReset PBOOLEAN Pointer to receive pointer to our adapter
//!  \param MiniportAdapterContext NDIS_HANDLE Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS 
Emac_MiniportReset(
    PBOOLEAN AddressingReset,
    NDIS_HANDLE MiniportAdapterContext
    ) 
{
	NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PEMAC_ADAPTER   pAdapter = (PEMAC_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_CRITICAL || DBG_FUNC, (L"---> Emac_MiniportReset\r\n")); 
  
    /*
     * Perform validity checks on the MiniportAdapterContext -> adapter
     * pointer.
     */

    do
    {
        if(pAdapter == (PEMAC_ADAPTER) NULL)
        {
            DEBUGMSG(DBG_ERR, (L"Emac_MiniportReset: Invalid Adapter pointer"));
            return NDIS_STATUS_FAILURE;
        }
                    
        /* Test is successful , make a status transition */     
        pAdapter->m_HwStatus = NdisHardwareStatusReset;
        
        *AddressingReset = TRUE;
      
        /* Tear down receive and transmit channels so that Rx/Tx are completelly
         * stopped immediately
         */
        pAdapter->m_pEmacRegsBase->RXTEARDOWN = 0x0;
        pAdapter->m_pEmacRegsBase->TXTEARDOWN = 0x0;
         
        /* wait for teardowm completion */
        while(0 != (pAdapter->m_Events & EMAC_RX_TEARDOWN_EVENT) &&
              0 != (pAdapter->m_Events & EMAC_TX_TEARDOWN_EVENT))
        {

        }

        /*
         * Disable the interrupts in the card, so that the interrupt routine would
         * not be called.
         */
        Emac_MiniportDisableInterrupt(pAdapter);

        /* Free allocated memory */
        EmacFreeAdapter(pAdapter);

        /* Init send data structures */
        Status = NICInitSend(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportReset:NICInitSend is failed.\r\n"));
            break;
        }   

        /* Init receive data structures */
        Status = NICInitRecv(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportReset:NICInitRecv is failed.\r\n"));
            break;
        }   

		/* Init the hardware and set up everything */
        pAdapter->m_HwStatus = NdisHardwareStatusInitializing;
        
        /* Power on the EMAC subsystem */
        EMACModStateChange(pAdapter->m_device,SYNCRST);

        Status = NICInitializeAdapter(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            RETAILMSG(TRUE, (L"Emac_MiniportReset:NICInitializeAdapter is failed.\r\n"));
            Status = NDIS_STATUS_HARD_ERRORS;
            break;
        }

        Status = NICSelfTest(pAdapter);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            /* Test is successful , make a status transition */     
            pAdapter->m_HwStatus = NdisHardwareStatusReady; 
        }  
        else
        {
            Status = NDIS_STATUS_HARD_ERRORS;
            break;
        }

    }while(0);

    DEBUGMSG(DBG_CRITICAL || DBG_FUNC, (L"<--- Emac_MiniportReset Status = %X\r\n", Status));
  
    return(Status);
}

//========================================================================
//!  \fn BOOLEAN    Emac_MiniportCheckForHang(NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is an optional function that reports the state of the 
//!         network adapter or monitors the responsiveness of an underlying device driver.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return Return True or False.
//========================================================================

BOOLEAN 
Emac_MiniportCheckForHang(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER     pAdapter = (PEMAC_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_FUNC && DBG_MSG, (L"---> Emac_MiniportCheckForHang\r\n"));
    
    /* We are indicating link change interrupt asynchonioulsly 
     * No need to take care of link down state. Also host error events
     * are interrupts here and reset will be done there also.
     */
     
    /* Need to see any other way we can monitor hungups */
     
    return (FALSE);  
}     
