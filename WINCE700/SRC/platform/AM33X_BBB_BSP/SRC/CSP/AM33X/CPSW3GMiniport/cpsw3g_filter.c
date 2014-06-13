//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "cpsw3g_miniport.h"
#include "cpsw3g_queue.h"
#include "cpsw3g_proto.h"
#include "cpsw3g_oid.h"

NDIS_STATUS Cpsw3gAddMulticastAddress(
    NDIS_HANDLE MiniportAdapterContext, 
    UINT8 MacAddress[]
    )
{
    
    PCPSW3G_ADAPTER   pAdapter;
    /*
     * Recover the Adapter pointer from the MiniportAdapterContext.
     */
    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;

    /* 
     * Check if the MacAddress is a multicast address.
     */ 
    if((MacAddress[0] & BIT0) == 0) 
    {
        return NDIS_STATUS_INVALID_DATA;
    }

    switch (pAdapter->Cfg.cpsw3g_mode)
    {
            case CPSW3G_ESWITCH:
                cpsw_ale_add_mcast_entry(pAdapter, MacAddress, 0, 0x7);
               
            break;
        case CPSW3G_CPGMAC:
        case CPSW3G_CPGMAC_KITL:
                cpsw_ale_add_mcast_entry(pAdapter, MacAddress, 0, 0x1<< CPSW_UNIT_SWITCH);
            break;
        default:
            RETAILMSG(TRUE, (L"Cpsw3gAddMulticastAddress: CPSW3G  mode %d is not supported.\r\n", 
				pAdapter->Cfg.cpsw3g_mode));
            break;
    }

    /* Return success*/
    return NDIS_STATUS_SUCCESS;
}

//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_MiniportSetInformation(NDIS_HANDLE MiniportAdapterContext,
//!                                                    NDIS_OID Oid,
//!                                                    PVOID InformationBuffer,
//!                                                    ULONG InformationBufferLength,
//!                                                    PULONG BytesRead,
//!                                                    PULONG BytesNeeded
//!                                                    );
//!  \brief This function is a required function that allows bound protocol drivers, 
//!         or NDIS, to request changes in the state information that the miniport 
//!         maintains for particular object identifiers, such as changes in multicast addresses.
//!  \param MiniportAdapterContext NDIS_HANDLE Specifies the handle to a miniport-allocated context 
//!         area in which the driver maintains per-network adapter state, 
//!         set up by MiniportInitialize.
//!  \param Oid PMINIPORT_ADAPTER NDIS_OID Specifies the system-defined OID_XXX code 
//!         designating the set operation the driver should carry out.
//!  \param InformationBuffer PVOID Points to a buffer containing the OID-specific 
//!         data used by MiniportSetInformation for the set. 
//!  \param InformationBufferLength ULONG Specifies the number of bytes at InformationBuffer.
//!  \param BytesRead PULONG Points to a variable that MiniportSetInformation sets 
//!         to the number of bytes it read from the buffer at InformationBuffer.
//!  \param BytesNeeded PULONG Points to a variable that MiniportSetInformation 
//!         sets to the number of additional bytes it needs to satisfy the 
//!         request if InformationBufferLength is less than Oid requires. 
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS 
Cpsw3g_MiniportSetInformation(
    NDIS_HANDLE MiniportAdapterContext,
    NDIS_OID Oid,
    PVOID InformationBuffer,
    ULONG InformationBufferLength,
    PULONG BytesRead,
    PULONG BytesNeeded
    )
{
    PCPSW3G_ADAPTER           pAdapter;
    NDIS_STATUS             Status;
    DWORD                   PacketFilter;
    PUCHAR                  MacAddress;
    DWORD                   Index;
    UINT32                  port_mask;
    PNDIS_DEVICE_POWER_STATE pPowerState;
    
    
    DEBUGMSG(DBG_FUNC && DBG_OID, 
            (L"  ->Cpsw3g_MiniportSetInformation %08x \r\n",Oid));

    pAdapter = (PCPSW3G_ADAPTER)MiniportAdapterContext;

    if(pAdapter == (PCPSW3G_ADAPTER) NULL)
    {
        return NDIS_STATUS_NOT_ACCEPTED;
    }
    /*
     * Set the Bytes read and Bytes Needed to be 0, so that if we return due
     * to failure, it will be valid.
     */
    *BytesRead = 0;
    *BytesNeeded  = 0;

    switch( Oid )
    {
        case OID_802_3_MULTICAST_LIST:
            /*
             * Check if the InformationBufferLength is multiple of Ethernet
             * Mac address size.
             */
            if ((InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
            {
                return (NDIS_STATUS_INVALID_LENGTH);
            }
            
            /* Clear multicast enties in ALE */
            cpsw_ale_flush_mcast_entry(pAdapter);

            if (InformationBufferLength == 0)
            {
                return NDIS_STATUS_SUCCESS;
            }
                
            MacAddress = (PUCHAR) InformationBuffer;
            for( Index = 0; Index < (InformationBufferLength /ETH_LENGTH_OF_ADDRESS); Index ++ )
            {
                Status = Cpsw3gAddMulticastAddress( pAdapter, MacAddress); 

                if(NDIS_STATUS_SUCCESS != Status)
                {
                    return(Status);
                }    
                MacAddress += ETH_LENGTH_OF_ADDRESS;
            }   
                            
            /* 
             * Set Bytes Read as InformationBufferLength,  & status as
             * success 
             */
            *BytesRead = InformationBufferLength;
             Status = NDIS_STATUS_SUCCESS;
             break;
             
        case OID_GEN_VLAN_ID:
            /* Check that the InformationBufferLength is sizeof(ULONG) */
            if (InformationBufferLength != sizeof(ULONG))
            {
                return (NDIS_STATUS_INVALID_LENGTH);
            }            

            pAdapter->Cfg.dmaCfg.hw_config.portVID = 
                *(PULONG)InformationBuffer & CPSW3G_PORTVLAN_VID_MASK;
            

            if(pAdapter->Cfg.vlanAware)
            {
                cpsw3g_add_dynamic_vlan(pAdapter);
            }
            
            *BytesRead = InformationBufferLength;
             Status = NDIS_STATUS_SUCCESS;

            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            
            /* Check that the InformationBufferLength is sizeof(ULONG) */
            if (InformationBufferLength != sizeof(ULONG))
            {
                return (NDIS_STATUS_INVALID_LENGTH);
            }            
            /*
             * Now check if there are any unsupported Packet filter types that
             * have been requested. If so return NDIS_STATUS_NOT_SUPPORTED.
             */
            NdisMoveMemory((PVOID)&PacketFilter, InformationBuffer,
                           sizeof(ULONG));
            
            if (PacketFilter & (NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
                                NDIS_PACKET_TYPE_SOURCE_ROUTING |
                                NDIS_PACKET_TYPE_SMT |
                                NDIS_PACKET_TYPE_MAC_FRAME |
                                NDIS_PACKET_TYPE_FUNCTIONAL |
                                NDIS_PACKET_TYPE_GROUP |
                                NDIS_PACKET_TYPE_ALL_LOCAL
                                ))  
                                
            {
                DEBUGMSG(DBG_WARN, 
                         (L"Cpsw3g_MiniportSetInformation: Invalid PacketFilter\n"));
                Status = NDIS_STATUS_NOT_SUPPORTED;
                *BytesRead = 4;
                break;
            }
            
            /* 
             * Convert the NDIS Packet filter mapping into the MAC
             * Receive packet Filter mapping. 
             */
            
            if((PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
               (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS))
            {    
                /*  put ALE in bypass mode */
                pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_ALEBYPASS;
            }    
            
            if(PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
            {
                /* Add broadcast entry in ALE table if such entry does not exist */
                if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_ESWITCH)
                    port_mask = 0x7;
                else
                    port_mask = 0x4 ;
                cpsw_ale_add_mcast_entry(pAdapter, broadcast_mac,0, port_mask);
            }
            
            if(PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
            {
                /* Always supported */
            }
            if(PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
            { 
                /* How to support this? */
                
            }

            /* Set Bytes Read as InformationBufferLength, & status as success */
            *BytesRead = InformationBufferLength;
            
            /* Update PacketFilter but we always have NDIS_PACKET_TYPE_DIRECTED set */
            pAdapter->PacketFilter = (PacketFilter|NDIS_PACKET_TYPE_DIRECTED);
            Status = NDIS_STATUS_SUCCESS;
            
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            /*
             * We are going to indicate the full ethernet frame, and hence if
             * somebody tries to set the lookahead, then set the status as
             * success.
             */
            if (InformationBufferLength != 4)
            {
                return (NDIS_STATUS_INVALID_LENGTH);
            }	
            /* Set Bytes Read as 4, & status as success */
            *BytesRead = 4;
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_TI_ALE_DUMP:
            cpsw_ale_dump(pAdapter);
            Status = NDIS_STATUS_SUCCESS;
            
            break;
            
        case OID_TI_HW_STATS_CLEAR:
            Cpsw3g_Clear_hw_stats (pAdapter);           
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_TI_SW_STATS_CLEAR:
            NdisZeroMemory(&pAdapter->Cpsw3g_sw_stats, sizeof(CPSW3G_SW_STATS));
            Status = NDIS_STATUS_SUCCESS;
            break;
            
        case OID_PNP_SET_POWER:
            if (!InformationBuffer || InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
            }
            else
            {
                pPowerState = (PNDIS_DEVICE_POWER_STATE)InformationBuffer;
                Status = Cpsw3g_SetPowerState(pAdapter, *pPowerState);
            }
            break;

        default:
            /* Set the status as NDIS_STATUS_INVALID_OID */
            Status = NDIS_STATUS_INVALID_OID;
            break;
	}
	
    DEBUGMSG(DBG_FUNC && DBG_OID, (L"  <- Cpsw3g_MiniportSetInformation 0x%08X \r\n",Status));
    return(Status);
}


