//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

#include "cpsw3g_miniport.h"
#include "cpsw3g_proto.h"
#include "cpsw3g_oid.h"

#define htonl( value ) ((((ULONG)value) << 24) | ((0x0000FF00UL & ((ULONG)value)) << 8) | ((0x00FF0000UL & ((ULONG)value)) >> 8) | (((ULONG)value) >> 24))

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

    OID_GEN_VLAN_ID,
    
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

    OID_GEN_BROADCAST_FRAMES_XMIT,
    OID_GEN_BROADCAST_FRAMES_RCV,    
    OID_GEN_MULTICAST_FRAMES_XMIT,
    OID_GEN_MULTICAST_FRAMES_RCV,

    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
};

//========================================================================
//!  \fn VOID   Cpsw3g_MiniportHalt(NDIS_HANDLE MiniportAdapterContext)
//!  \brief MiniportHalt is a required function that de-allocates resources 
//!         when the network adapter is removed and halts the network adapter.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return None
//========================================================================

VOID        
Cpsw3g_MiniportHalt(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER       pAdapter;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    
    pAdapter = (PCPSW3G_ADAPTER) MiniportAdapterContext;

    DEBUGMSG(DBG_FUNC, (L"  ->Cpsw3g_MiniportHalt \r\n"));
    
RETAILMSG(TRUE, (L"***** Cpsw3g_MiniportHalt calling Cpsw3g_StopAdapter() \r\n"));
    if(!Cpsw3g_StopAdapter(pAdapter))
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        RETAILMSG(TRUE, (L"  <- Stop CPSW3G Adapter failed. \r\n"));
    }

    /* Free allocated memory and resources held */
    Cpsw3g_FreeAdapter(pAdapter, 3);
    DEBUGMSG(DBG_FUNC, (L"  <-Cpsw3g_MiniportHalt \r\n"));

    return;
}

//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_MiniportQueryInformation(NDIS_HANDLE MiniportAdapterContext,
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
Cpsw3g_MiniportQueryInformation(
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
    static const UCHAR      VendorDescriptor[] = "TI-CPSW3G ADAPTER";
    ULONG                       ulInfo = 0;
    ULONG64                     ul64Info = 0;
    PVOID                       pInfo = (PVOID) &ulInfo;
    ULONG                       ulInfoLen = sizeof(ulInfo);
    ULONG                       ulBytesAvailable = ulInfoLen;
    PCPSW3G_ADAPTER               pAdapter;
    PCPSW3G_STATINFO              pCpsw3gStatInfo;
    NDIS_PNP_CAPABILITIES       PMCaps;
    PNDIS_DEVICE_POWER_STATE    pPowerState;
    
    pAdapter = (PCPSW3G_ADAPTER) MiniportAdapterContext;
    
    DEBUGMSG(DBG_FUNC && DBG_OID, 
            (L"  -> Cpsw3g_MiniportQueryInformation 0x%08X \r\n",Oid));
   
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
            pInfo = (PVOID) &pAdapter->HwStatus;
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
            ulInfo = (ULONG) CPSW3G_MAX_ETHERNET_PKT_SIZE;
            break;
        
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
            ulInfo = CPSW3G_MAX_DATA_SIZE;
            break;
             
       case OID_GEN_TRANSMIT_BUFFER_SPACE:
            ulInfo = (ULONG) CPSW3G_MAX_ETHERNET_PKT_SIZE * pAdapter->MaxTxMacBufDesc;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            ulInfo = (ULONG) CPSW3G_MAX_ETHERNET_PKT_SIZE * pAdapter->NumRxIndicatePkts;
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
            if(pAdapter->Cfg.vlanAware)
            {
                ulInfo |= NDIS_MAC_OPTION_8021Q_VLAN;    
            }
            break;
            
        case OID_GEN_VLAN_ID:
            ulInfo = pAdapter->Cfg.dmaCfg.hw_config.portVID & 0xfff;
            break;
        
        case OID_GEN_VENDOR_ID:
            /* A 3-byte IEEE vendor code, followed by a single byte the vendor
             * assigns to identify a particular vendor-supplied network
             * interface card driver. The IEEE code uniquely identifies the
             * vendor and is the same as the three bytes appearing at the
             * beginning of the NIC hardware address. 
             */ 
            NdisMoveMemory(VendorId, pAdapter->MACAddress, 3);
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

            ulInfo  =  CPSW3G_NDIS_DRIVER_VERSION;
            break;
                
        case OID_GEN_MEDIA_CONNECT_STATUS:
            
            
            if(LINK_UP == pAdapter->link)
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
            ulInfo = pAdapter->link_speed;
            break;
            
       
        case OID_GEN_CURRENT_PACKET_FILTER:
            ulInfo = pAdapter->PacketFilter;
            break;               
            
        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
            pInfo = pAdapter->MACAddress;
            DEBUGMSG(DBG_INFO, (L"Mac addr is %x:%x:%x:%x:%x:%x.\r\n",
                pAdapter->MACAddress[0],pAdapter->MACAddress[1],pAdapter->MACAddress[2],
                pAdapter->MACAddress[3],pAdapter->MACAddress[4],pAdapter->MACAddress[5]));
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            /* Todo how to decide number of mcast entries ? */
            ulInfo = CPSW3G_MAX_MCAST_ENTRIES;
            break;
            
        case OID_802_3_MULTICAST_LIST:
             pInfo = pAdapter->MulticastTable;
             ulBytesAvailable = ulInfoLen = pAdapter->NumMulticastEntries *
                                                     ETH_LENGTH_OF_ADDRESS;   
            break;     

        case OID_TI_REG_DUMP:
            if(InformationBufferLength < 0x600) {
                Status = NDIS_STATUS_INVALID_LENGTH;
            }
            else{
                pInfo = (PVOID)pAdapter->pCpsw3gRegsBase;

                ulBytesAvailable = ulInfoLen = 0x600;
                
            }
            break;

        case OID_TI_HW_STATS_DUMP:
            if(InformationBufferLength < CPSW3G_NUM_STATS_REGS * 4) {
                Status = NDIS_STATUS_INVALID_LENGTH;
            }
            else{
                pInfo = (PVOID)((UINT8 *)pAdapter->pCpsw3gRegsBase + 0x400);
                ulBytesAvailable = ulInfoLen = CPSW3G_NUM_STATS_REGS * 4;
            }
            break;
            
        case OID_TI_SW_STATS_DUMP:
            if(InformationBufferLength < sizeof(CPSW3G_SW_STATS)) {
                Status = NDIS_STATUS_INVALID_LENGTH;
            }
            else{
                pInfo = (PVOID)&pAdapter->Cpsw3g_sw_stats;
                ulBytesAvailable = ulInfoLen = sizeof(CPSW3G_SW_STATS);

            }
            break;            

        case OID_PNP_CAPABILITIES:
            NdisZeroMemory(&PMCaps, sizeof(PMCaps));
            PMCaps.WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
            PMCaps.WakeUpCapabilities.MinPatternWakeUp     = NdisDeviceStateUnspecified;
            PMCaps.WakeUpCapabilities.MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;
            pInfo = (PVOID)&PMCaps;
            ulBytesAvailable = ulInfoLen = sizeof(PMCaps);
            break;

        case OID_PNP_QUERY_POWER:
            if (!InformationBuffer || InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
            }
            else
            {
                pPowerState = (PNDIS_DEVICE_POWER_STATE)InformationBuffer;
                switch (*pPowerState)
                {
                case NdisDeviceStateD0:
                case NdisDeviceStateD3:
                    // Supported
                    break;
                default:
                    // Unsupported
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
            }
            break;

         default:
                /*
                 * This is a query for statistics information from the NDIS
                 * wrapper. Let us call Cpsw3gStatistics and retrieve the latest set
                 * of statistical information retrieved by the CPSW3G.
                 */
                 pCpsw3gStatInfo = &pAdapter->Cpsw3gStatInfo;
                 
                
                pInfo = (PVOID) &ul64Info;
                ulBytesAvailable = ulInfoLen = sizeof(ul64Info);
                if (NdisDeviceStateD3 == pAdapter->CurrentPowerState)
                {
                    ulInfoLen = MIN(InformationBufferLength, ulBytesAvailable);
                    break;
                }
                    
                switch (Oid)
                {
    
                    case OID_GEN_XMIT_OK:
    
                        ul64Info = pCpsw3gStatInfo->TxOKFrames + 
                            pAdapter->pCpsw3gRegsBase->TxGoodFrames;
                        break;
    
                    case OID_GEN_RCV_OK:
    
                        ul64Info = pCpsw3gStatInfo->RxOKFrames +
                            pAdapter->pCpsw3gRegsBase->RxGoodFrames;
                            
                        break;
    
                    case OID_GEN_XMIT_ERROR:
    
                        ul64Info = pCpsw3gStatInfo->TxErrorframes + 
                                            (pAdapter->pCpsw3gRegsBase->TxExcessiveCollision +
                                            pAdapter->pCpsw3gRegsBase->TxLateCollision +
                                            pAdapter->pCpsw3gRegsBase->TxCarrierSenseError
                                            );                             
                        break;
    
                    case OID_GEN_RCV_ERROR:
                        ul64Info = pCpsw3gStatInfo->RxErrorframes +
                                            (pAdapter->pCpsw3gRegsBase->RxCRCErrors +
                                            pAdapter->pCpsw3gRegsBase->RxAlignCodeErrors +
                                            pAdapter->pCpsw3gRegsBase->RxOversizedFrames +
                                            pAdapter->pCpsw3gRegsBase->RxJabberFrames +
                                            pAdapter->pCpsw3gRegsBase->RxUndersizedFrames
                                            ); 
                        break;
    
                    case OID_GEN_RCV_NO_BUFFER:
                        ul64Info = pCpsw3gStatInfo->RxNoBufFrames + 
                                        pAdapter->pCpsw3gRegsBase->RxDmaOverruns;
                        break;
    
                  
                    case OID_802_3_RCV_ERROR_ALIGNMENT:
                        ul64Info = pCpsw3gStatInfo->RxAlignErrorFrames + 
                                        pAdapter->pCpsw3gRegsBase->RxAlignCodeErrors;
                        break;
    
                    case OID_802_3_XMIT_ONE_COLLISION:
                        ul64Info = pCpsw3gStatInfo->TxOneColl + 
                                        pAdapter->pCpsw3gRegsBase->TxSinglecollFrames;
                        break;
    
                    case OID_802_3_XMIT_MORE_COLLISIONS:
                        ul64Info = pCpsw3gStatInfo->TxMoreColl +
                                        pAdapter->pCpsw3gRegsBase->TxMulticollFrames;
                            
                        break;
    
                    case OID_802_3_XMIT_DEFERRED:
                        ul64Info = pCpsw3gStatInfo->TxDeferred +
                                        pAdapter->pCpsw3gRegsBase->TxDeferredFrames;
                            
                        break;
    
                    case OID_802_3_XMIT_MAX_COLLISIONS:
                        ul64Info = pCpsw3gStatInfo->TxMaxColl + 
                                        pAdapter->pCpsw3gRegsBase->TxExcessiveCollision;
                            
                        break;
    
                    case OID_802_3_RCV_OVERRUN:
                        ul64Info = pCpsw3gStatInfo->RxOverRun +
                                        pAdapter->pCpsw3gRegsBase->RxDmaOverruns;
                            
                        break;
                        
                    case OID_GEN_BROADCAST_FRAMES_RCV:
                        ul64Info = pCpsw3gStatInfo->RxBroadcastFrame +
                                        pAdapter->pCpsw3gRegsBase->RxBcastFrames;
                            
                        break;

                    case OID_GEN_BROADCAST_FRAMES_XMIT:
                        ul64Info = pCpsw3gStatInfo->TxBroadcastFrame +
                                        pAdapter->pCpsw3gRegsBase->TxBcastFrames;
                            
                        break;

                    case OID_GEN_MULTICAST_FRAMES_RCV:
                        ul64Info = pCpsw3gStatInfo->RxMulticastFrame +
                                        pAdapter->pCpsw3gRegsBase->RxMcastFrames;
                            
                        break;

                    case OID_GEN_MULTICAST_FRAMES_XMIT:
                        ul64Info = pCpsw3gStatInfo->TxMulticastFrame +
                                        pAdapter->pCpsw3gRegsBase->TxMcastFrames;
                            
                        break;
                  
                    default:
                        Status = NDIS_STATUS_NOT_SUPPORTED;
                        break;
                }
                if(Status == NDIS_STATUS_SUCCESS)
                {
                    if (InformationBufferLength < sizeof(ULONG))
                    {
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        *BytesNeeded = ulBytesAvailable;
                        break;
                    }
                    ulInfoLen = MIN(InformationBufferLength, ulBytesAvailable);
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
            (L"  <- Cpsw3g_MiniportQueryInformation status 0x%08X \r\n",Status));
   
    return(Status);

}


ULONG CopyPacketToBuffer(
    PNDIS_BUFFER FirstBuffer,
    PUCHAR buffer,
    ULONG BytesToCopy
    )
{
    PNDIS_BUFFER       CurrBuffer = FirstBuffer;
    PUCHAR             BufferVirtAddr;
    ULONG              BufferLen, BytesOnThisCopy;
    PUCHAR             DestAddr = (PUCHAR) buffer;
    ULONG              BytesCopied =0;

    if(BytesToCopy == 0) return BytesCopied;

    do
    {
        NdisQueryBuffer( CurrBuffer, &BufferVirtAddr, &BufferLen);
        BytesOnThisCopy = MIN(BytesToCopy, BufferLen);
        //RETAILMSG(TRUE, (L"  -> CopyPacketToBuffer, BytesToCopy%d, BytesOnThisCopy %d,  \r\n", BytesToCopy, BytesOnThisCopy));
        NdisMoveMemory(DestAddr, BufferVirtAddr, BytesOnThisCopy);

        (PUCHAR)DestAddr += BytesOnThisCopy;

        BytesCopied += BytesOnThisCopy;
        BytesToCopy -= BytesOnThisCopy;
        NdisGetNextBuffer( CurrBuffer, &CurrBuffer);

    }while(BytesToCopy && CurrBuffer);
    
    return BytesCopied;
} 

VOID cpsw3g_move_memory(PUCHAR dst, PUCHAR src, UINT32 len)
{
    UINT32 i;
    dst += len -1;
    src += len - 1;
    for(i=0;i<len;i++){
        *dst-- = *src--;         
    }
}

VOID cpsw3g_dump_packet(PULONG addr, UINT32 length)
{
    UINT32 i;
    RETAILMSG(TRUE,  (L"Dump Packet:\r\n"));

    for (i=0;i<length/4+1;i++)
    {
        if(!(i % 4)) RETAILMSG(TRUE,  (L"\r\n%x:", addr + i));
        RETAILMSG(TRUE,  (L" %x", addr[i]));
        
    }
    RETAILMSG(TRUE,  (L"\r\n"));        

}

//========================================================================
//!  \fn VOID Cpsw3g_MiniportSendPacketsHandler(NDIS_HANDLE  MiniportAdapterContext,
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
Cpsw3g_MiniportSendPacketsHandler(
    NDIS_HANDLE    MiniportAdapterContext,
    PPNDIS_PACKET  PacketArray,
    UINT           NumberOfPackets
    )
{  
    USHORT          Count;
    PNDIS_PACKET    CurPacket;
    PCPSW3G_ADAPTER   pAdapter;
    UINT            PhysicalBufCount;
    UINT            BufCount;
    UINT            TotalPktLength;
    UINT            CurBufAddress = 0;
    UINT	        CurBufAddressPa;
    PNDIS_BUFFER    FirstBuf;
    PCPSW3G_TXPKT     pTxPktInfo = NULL;
    PCPSW3G_TXBUF     TxHeadBuf;
    PCPSW3G_TXBUF     TxNextHeadBuf;
    PCPSW3G_TXBUF     TxPrevTailBuf;
    PCPSW3G_TXBUF	  TxTailBuf;	
    PCPSW3G_TXBUF     pTxBufInfo;
    QUEUE_T         TmpPerSendPktPool;    
    ULONG BytesCopied = 0;

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;

    DEBUGMSG(DBG_FUNC && DBG_TX, (L"  -> Cpsw3g_MiniportSendPacketsHandler %d \r\n",NumberOfPackets));

    /* Acquire the Send lock */
    NdisAcquireSpinLock(&pAdapter->SendLock); 
    
    if(LINK_DOWN == pAdapter->link)
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
            &PhysicalBufCount,  //the max num of physical breaks mapped by the buffer descriptors
            &BufCount,          //the number of buffer descriptors chained to the given packet
            &FirstBuf,          //a ptr to the initial buf descriptor chained to the given pkt
            &TotalPktLength);   //num of bytes of pkt data mapped by all chained buf descriptors

        if(TotalPktLength > CPSW3G_MAX_ETHERNET_PKT_SIZE)
                pAdapter->Cpsw3g_sw_stats.tx_oversize_pkt++;

       /*  Number of Physical buffers, could be returned with invalid 
        *  higher order bits.
        */
        PhysicalBufCount &= 0x0000FFFF;
        
        DEBUGMSG(DBG_TX, 
          (L"PhysicalBufCount %d pAdapter->TxBufInfoPool.Count %d BufCount %d TotalPktLength %d\r\n",
          PhysicalBufCount, pAdapter->TxBufInfoPool.m_Count,
          BufCount, TotalPktLength)); 

        pAdapter->Cpsw3g_sw_stats.tx_in++;

        if (NdisDeviceStateD3 == pAdapter->CurrentPowerState)
        {
            /* Set status as aborted */
            NDIS_SET_PACKET_STATUS(CurPacket, NDIS_STATUS_REQUEST_ABORTED );
            
            /* we will return to indicate NDIS to abort the packets */
            continue;
        }
        
        if(0 == QUEUE_COUNT(&pAdapter->TxPktsInfoPool) )
        {
            RETAILMSG(TRUE,  
                (L"Unable to send packet %d \r\n",
                QUEUE_COUNT(&pAdapter->TxPktsInfoPool)));

            pAdapter->Cpsw3g_sw_stats.tx_out_of_descs++;
            /* Set status as resources */
            NDIS_SET_PACKET_STATUS(CurPacket,NDIS_STATUS_RESOURCES );
            
            /* we will return to indicate NDIS to resubmit remaining packets */
            continue;
        }
                
        /* we can transmit this packet 
         * Dequeue  from available packet pool a packet  
         */
        QUEUE_REMOVE(&pAdapter->TxPktsInfoPool, pTxPktInfo);
        if(!pTxPktInfo)
        {
            /* queue count is checked alreay, this condition should not be hit , double checking */
            DEBUGMSG (DBG_INFO, 
                (L"  -> Cpsw3g_MiniportSendPacketsHandler, Tx Pkt is not available  \r\n"));

            pAdapter->Cpsw3g_sw_stats.tx_out_of_descs++;
            break;
        }
        
        /* Initialise per packet maintained queue of bufs list */
        QUEUE_INIT(&pTxPktInfo->BufsList);
        
        /* Pkt handle will be current packet */
        pTxPktInfo->PktHandle = CurPacket;
        
        /*
        * Dequeue a TxBufInfoPool structure, and assert that one is
        * available.
        */
        QUEUE_REMOVE(&pAdapter->TxBufInfoPool, pTxBufInfo);
        if(!pTxPktInfo)
        {
            /* queue count is checked alreay, this condition should not be hit , double checking */
            DEBUGMSG (DBG_INFO, 
                (L"  -> Cpsw3g_MiniportSendPacketsHandler, Tx Buf is not available  \r\n"));

            pAdapter->Cpsw3g_sw_stats.tx_out_of_descs++;
            break;
        }
        
        CurBufAddress  = pTxBufInfo->BufLogicalAddress;
        CurBufAddressPa = pTxBufInfo->BufPhysicalAddress;

        BytesCopied = CopyPacketToBuffer(FirstBuf, (PUCHAR)CurBufAddress, TotalPktLength); 

        if(pAdapter->Cfg.vlanAware && pAdapter->Cfg.sw_vlantagging)
        {
            NDIS_PACKET_8021Q_INFO VlanPriInfo;
            UINT32 vlan_id, packet_pri, temp;
            PUCHAR        temp_addr, DestAddr; 

            VlanPriInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(CurPacket, Ieee8021QInfo);
            packet_pri = VlanPriInfo.TagHeader.UserPriority;
            vlan_id = VlanPriInfo.TagHeader.VlanId;

            temp = htonl (packet_pri<<13 | vlan_id | 0x8100<<16);

            /* move data 4 bytes forward */
            temp_addr = DestAddr = (PUCHAR)pTxBufInfo->BufLogicalAddress ;
            DestAddr += ETH_LENGTH_OF_ADDRESS *2 + 4;
            temp_addr += ETH_LENGTH_OF_ADDRESS *2;
            
            cpsw3g_move_memory((PUCHAR)DestAddr, (PUCHAR)temp_addr, 
                                TotalPktLength - ETH_LENGTH_OF_ADDRESS *2);
            
            DestAddr = (PUCHAR)CurBufAddress + ETH_LENGTH_OF_ADDRESS *2;
            /* Insert 4 bytes VLAN tag */
            NdisMoveMemory (DestAddr, &temp, 4);
            TotalPktLength += 4;
            pAdapter->Cpsw3g_sw_stats.tx_sw_vlan_process++;
        }

        if(TotalPktLength < CPSW3G_MIN_ETHERNET_PKT_SIZE)
        {
            NdisZeroMemory((UCHAR *)(CurBufAddress + TotalPktLength), 
                            CPSW3G_MIN_ETHERNET_PKT_SIZE - TotalPktLength);

            TotalPktLength = CPSW3G_MIN_ETHERNET_PKT_SIZE;
            pAdapter->Cpsw3g_sw_stats.tx_undersized ++;
        }
        
        ((PCPSW3G_DESC)(pTxBufInfo->BufDes))->pNext = 0;
        
        ((PCPSW3G_DESC)(pTxBufInfo->BufDes))->PktFlgLen = (
                                                               CPSW3G_CPPI_SOP_BIT |
                                                               CPSW3G_CPPI_EOP_BIT | 
                                                               CPSW3G_CPPI_OWNERSHIP_BIT |
                                                               (TotalPktLength & 0x7FF)
                                                           );

        if(pAdapter->Cfg.cpsw3g_mode != CPSW3G_ESWITCH)  
        {
            /* Use directed packet */
            ((PCPSW3G_DESC)(pTxBufInfo->BufDes))->PktFlgLen |= 
                                         (CPSW3G_CPPI_TO_PORT_EN_BIT |
                                          pAdapter->curr_port_info->PortNum <<16);
        }

        ((PCPSW3G_DESC)(pTxBufInfo->BufDes))->BufOffLen = (TotalPktLength & 0xFFFF);
        
        ((PCPSW3G_DESC)(pTxBufInfo->BufDes))->pBuffer = (UINT8 *)CurBufAddressPa;

        CacheRangeFlush((VOID *)CurBufAddress, TotalPktLength, CACHE_SYNC_WRITEBACK );

        /* Insert in to per packet maintained buffer pool */
        QUEUE_INSERT(&pTxPktInfo->BufsList,pTxBufInfo);
        
        if(0 != QUEUE_COUNT(&TmpPerSendPktPool))
        {
            /* Since there are some other packets in to be posted send pkt pool
             * we have to link the current head of the to be posted send packet pool 
             * to the tail of temporary send packets pool
             */

            /* First we will fetch the tail buffer of temp send pkts pool */
            
            TxPrevTailBuf = (PCPSW3G_TXBUF)(((PCPSW3G_TXPKT)(TmpPerSendPktPool.m_pTail))->BufsList).m_pTail;
            TxNextHeadBuf = (PCPSW3G_TXBUF)((pTxPktInfo->BufsList).m_pHead);
            
            ((PCPSW3G_DESC)(TxPrevTailBuf->BufDes))->pNext = (PCPSW3G_DESC)(TxNextHeadBuf->BufDesPa);
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
  
    if(0 == QUEUE_COUNT(&pAdapter->TxPostedPktPool))
    {
        /* Insert in to posted packets pool */
        QUEUE_INSERT_QUEUE(&pAdapter->TxPostedPktPool, &TmpPerSendPktPool);
        
        TxHeadBuf = (PCPSW3G_TXBUF)(((PCPSW3G_TXPKT)(pAdapter->TxPostedPktPool.m_pHead))->BufsList).m_pHead;
       
        /* Submit formed TX buffers queue to EMAC TX DMA */  
        pAdapter->pCpsw3gRegsBase->Tx_HDP[pAdapter->curr_port_info->Tx_channel] = TxHeadBuf->BufDesPa;
    }
    else   /* Posted packets is non zero */
    {    
        TxTailBuf = (PCPSW3G_TXBUF)(((PCPSW3G_TXPKT)(pAdapter->TxPostedPktPool.m_pTail))->BufsList).m_pTail;
        TxHeadBuf = (PCPSW3G_TXBUF)(((PCPSW3G_TXPKT)(TmpPerSendPktPool.m_pHead))->BufsList).m_pHead;
         
        /* Insert in to posted packets pool */
        QUEUE_INSERT_QUEUE(&pAdapter->TxPostedPktPool,&TmpPerSendPktPool);
            
        /* Insert in to posted packets pool */
        ((PCPSW3G_DESC)(TxTailBuf->BufDes))->pNext = ((PCPSW3G_DESC)(TxHeadBuf->BufDesPa));
    }
end:   
    NdisReleaseSpinLock(&pAdapter->SendLock);
    
    DEBUGMSG(DBG_FUNC && DBG_TX, (L"  <- Cpsw3g_MiniportSendPacketsHandler \r\n"));
}




//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_MiniportReset(PBOOLEAN AddressingReset,
//!                                        NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is a required function that issues a hardware reset 
//!         to the network adapter and/or resets the driver's software state.
//!  \param AddressingReset PBOOLEAN Pointer to receive pointer to our adapter
//!  \param MiniportAdapterContext NDIS_HANDLE Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS 
Cpsw3g_MiniportReset(
    PBOOLEAN AddressingReset,
    NDIS_HANDLE MiniportAdapterContext
    ) 
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PCPSW3G_ADAPTER   pAdapter = (PCPSW3G_ADAPTER) MiniportAdapterContext;
    //BOOLEAN ret;
    
    DEBUGMSG(DBG_CRITICAL || DBG_FUNC, (L"  -> Cpsw3g_MiniportReset\n")); 
  
    /*
     * Perform validity checks on the MiniportAdapterContext -> adapter
     * pointer.
     */
   
    NdisAcquireSpinLock(&pAdapter->Lock);

    if(pAdapter == (PCPSW3G_ADAPTER) NULL)
    {
        DEBUGMSG(DBG_ERR, (L"Cpsw3g_MiniportReset: Invalid Adapter pointer"));
        return NDIS_STATUS_FAILURE;
    }

    if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL) 
    {
        *AddressingReset = FALSE;

        DEBUGMSG(DBG_INFO, (L"  Cpsw3g_MiniportReset skipped. \r\n"));

        goto skip;
    }
                
    /* Test is successful , make a status transition */     
    pAdapter->HwStatus = NdisHardwareStatusReset;
    *AddressingReset = TRUE;

    if(!Cpsw3g_StopAdapter(pAdapter))
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        DEBUGMSG(DBG_INFO, (L"  <- Stop CPSW3G Adapter failed. \r\n"));

        goto skip;
    }
    Cpsw3g_free_Adapter_memory(pAdapter);
    
    NdisAcquireSpinLock(&pAdapter->SendLock);
    NdisAcquireSpinLock(&pAdapter->RcvLock);

    Status = Cpsw3g_InitializeAdapter(pAdapter);    
    
    if (Status != NDIS_STATUS_SUCCESS) 
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        goto fail;
    }
   
    Status = Cpsw3g_SelfCheck(pAdapter);
    
    if (Status == NDIS_STATUS_SUCCESS) 
        /* Test is successful , make a status transition */     
        pAdapter->HwStatus = NdisHardwareStatusReady; 
    else
        Status = NDIS_STATUS_HARD_ERRORS;       

    if(pAdapter->link != LINK_UP)
    {
        Status = NDIS_STATUS_PENDING;
        pAdapter->ResetPending=TRUE;
    }
fail:    
    NdisReleaseSpinLock(&pAdapter->RcvLock);
    NdisReleaseSpinLock(&pAdapter->SendLock); 
skip:
    NdisReleaseSpinLock(&pAdapter->Lock);

    DEBUGMSG(DBG_CRITICAL || DBG_FUNC, (L"  <- Cpsw3g_MiniportReset\n"));
    
    return(Status);
}

//========================================================================
//!  \fn BOOLEAN    Cpsw3g_MiniportCheckForHang(NDIS_HANDLE MiniportAdapterContext)
//!  \brief This function is an optional function that reports the state of the 
//!         network adapter or monitors the responsiveness of an underlying device driver.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated 
//!         context area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize. 
//!  \return Return True or False.
//========================================================================

BOOLEAN 
Cpsw3g_MiniportCheckForHang(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PCPSW3G_ADAPTER     pAdapter = (PCPSW3G_ADAPTER) MiniportAdapterContext;
    UINT32 channel;
    
    DEBUGMSG(DBG_FUNC && DBG_MSG, (L"  -> Cpsw3g_MiniportCheckForHang\r\n"));
    /* We are indicating link change interrupt asynchonioulsly 
     * No need to take care of link down state. Also host error events
     * are interrupts here and reset will be done there also.
     */

    for(channel = 0;channel < CPDMA_MAX_CHANNELS; channel++)
    {
    
        if(((0x1 << channel) & pAdapter->curr_port_info->RxPortChanMask) &&
            (QUEUE_COUNT(&pAdapter->RxBufsPool[channel]) < 10))
        {
        
            RETAILMSG (TRUE, (L"  -> Cpsw3g_MiniportCheckForHang, channel :%d, rx_buf_queue_count=%d, need to reset driver\r\n", 
                QUEUE_COUNT(&pAdapter->RxBufsPool[channel])));
            return TRUE;
        }
        else if((0x1 << channel) & pAdapter->curr_port_info->RxPortChanMask) 
        {
            DEBUGMSG (DBG_INFO, (L"  -> Cpsw3g_MiniportCheckForHang, queue count=%d, head=%x, tail=%x, rx_int=%x, rx_set=%x\r\n", 
                QUEUE_COUNT(&pAdapter->RxBufsPool[channel]),
                pAdapter->RxBufsPool[channel].m_pHead, 
                pAdapter->RxBufsPool[channel].m_pTail, 
                pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked, 
                pAdapter->pCpsw3gRegsBase->Rx_IntMask_Set));
        }
    }
    
    /* Need to see any other way we can monitor hungups */
    DEBUGMSG(DBG_FUNC && DBG_MSG, (L"  <- Cpsw3g_MiniportCheckForHang\r\n"));
     
    return (FALSE);  
}     


 
