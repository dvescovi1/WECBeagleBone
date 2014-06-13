//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Filter.C
//! \brief Contains OID set functions and muticasting related information.
//! 
//! This source File contains the OID set functions and muticast related
//! hashing alogorithm implementation and related functions.
//! 
//! \version  1.00 Aug 22nd 2006 File Created 

// Includes
#include <Ndis.h>
#include "Emac_Adapter.h"
#include "Emac_Queue.h"
#include "Emac_Proto.h"

//========================================================================
//!  \fn UINT32 EmacClearMulticastAddressTable(NDIS_HANDLE MiniportAdapterContext)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
UINT32
EmacClearMulticastAddressTable(
    NDIS_HANDLE MiniportAdapterContext
    )
{
    PEMAC_ADAPTER   pAdapter;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    
    /*
     * Recover the Adapter pointer from the MiniportAdapterContext.
     */
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    DEBUGMSG(DBG_FUNC, 
            (L"Entering EmacClearMulticastAddressTable Function \r\n"));
   
    /* 
     * Clear the receive Multicast table in the hardware first. 
     * Disable the receive Filter before writing into the receive Filter
     * memory.
     */
    pAdapter->m_pEmacRegsBase->MACHASH1 = 0x0;
    pAdapter->m_pEmacRegsBase->MACHASH2 = 0x0;

    /* Clear the receive multicast table in the software */
    NdisZeroMemory(pAdapter->m_MulticastTable,
                  sizeof(pAdapter->m_MulticastTable));
                  
    pAdapter->m_NumMulticastEntries = 0;

    
    /* Return success*/
    return Status;
}

//========================================================================
//!  \fn USHORT ComputeHashValue(UCHAR* pAddress)
//!  \brief Implements a hashing function for multicast addresses.
//!  \param UCHAR* pAddress Six bytes MAC address
//!  \return USHORT Six bit hash value.
//========================================================================

USHORT 
ComputeHashValue(
    UCHAR* pAddress
    )
{
   UINT     OutIndex;
   UINT     InIndex;
   UINT     Result=0;
   USHORT   HashFunVal;
   UCHAR    BitPos;
   UCHAR    MACAddrIndex;
   
    for(OutIndex = 0; OutIndex < 6; OutIndex++)
        {
        
        /* After each of 6 bits calculation value should be reset to low */
        HashFunVal =0;
      
        for(InIndex = 0; InIndex < 8; InIndex++)
            {
            /* Calculate the bit position among 48 bits in mac address acc. to
             * algorithm in TRM 
             */
            
            BitPos = OutIndex + 6*InIndex;
            
            /* Here we need to calculate the MAC array index where the particular
             * BitPos exists also pAddress[0] indicates 47-40 bits and so on
             * so reversing is taken care by substracting 5
             */
             
            MACAddrIndex = 5 - BitPos/8 ;
            
            /* This expression calulates the BitPos value (either high or low)
             * and subsequently XOR ing with 8 BitPos and storing it in HashFunVal
             * which should be also 0 or 1 acc. to TRM
             */
             
            HashFunVal ^= ( pAddress[MACAddrIndex] >> (BitPos % 8)) & BIT(0) ;
            
            }
            
        /* Storing value each of 6 bits in the Result by ORRing each bit 
         * of it by the value obtained 
         */
                
        Result |= HashFunVal << OutIndex;
        
        }
              
   /* Since we are interested only in last 6 bits returning the same */
   
   return (USHORT)(Result & 0x3F);
   
}

//========================================================================
//!  \fn UINT32 EmacAddMulticastAddress(NDIS_HANDLE MiniportAdapterContext, 
//!                                            UINT8 MacAddress[]);
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
UINT32
EmacAddMulticastAddress(
    NDIS_HANDLE MiniportAdapterContext, 
    UINT8 MacAddress[]
    )
{
    
    PEMAC_ADAPTER   pAdapter;
    UINT32          HashValue;
    /*
     * Recover the Adapter pointer from the MiniportAdapterContext.
     */
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    
    /* 
     * Check if the MacAddress is a multicast address.
     */ 
    if((MacAddress[0] & BIT(0)) == 0) 
    {
        return NDIS_STATUS_INVALID_DATA;
    }
    /* Compute the Hash value which is number between 0 to 63 is generated */
    
    HashValue = ComputeHashValue(MacAddress);   
    
    if (HashValue < 32 ) 
    {
        pAdapter->m_pEmacRegsBase->MACHASH1 = (0x1 << HashValue);
    }
    else if ((HashValue >= 32 ) && (HashValue < 64 ))  
    {    
        pAdapter->m_pEmacRegsBase->MACHASH2 = (0x1 << (HashValue-32));
    }
    
    /* Return success*/
    return NDIS_STATUS_SUCCESS;
}

//========================================================================
//!  \fn NDIS_STATUS Emac_MiniportSetInformation(NDIS_HANDLE MiniportAdapterContext,
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
Emac_MiniportSetInformation(
    NDIS_HANDLE MiniportAdapterContext,
    NDIS_OID Oid,
    PVOID InformationBuffer,
    ULONG InformationBufferLength,
    PULONG BytesRead,
    PULONG BytesNeeded
    )
{
    PEMAC_ADAPTER           pAdapter;
    NDIS_STATUS             Status;
    DWORD                   PacketFilter;
    DWORD                   EmacPktFilter;
    PUCHAR                  MacAddress;
    DWORD                   Index;
    NDIS_DEVICE_POWER_STATE PowerState;
    
    DEBUGMSG(DBG_FUNC && DBG_OID, 
            (L"--->Emac_MiniportSetInformation %08x \r\n",Oid));

    /*
     * Recover the Adapter pointer from the MiniportAdapterContext.
     */
    pAdapter = (PEMAC_ADAPTER)MiniportAdapterContext;

    if(pAdapter == (PEMAC_ADAPTER) NULL)
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
            
            /* 
             * Clear old multicast entries, if any, before accepting a 
             * set of new ones.
             */
             
            EmacClearMulticastAddressTable(pAdapter);

            if (InformationBufferLength == 0)
            {
                return NDIS_STATUS_SUCCESS;
            }
                
            /*
             * For each of the MAC address in the Information Buffer, call
             * HALAddMulticastAddress. The HALAddMulticastAddress handles any
             * errors in the Multicast Address/Duplicates etc.
             */
             
            MacAddress = (PUCHAR) InformationBuffer;
            
            for( Index = 0; 
                 Index < (InformationBufferLength /ETH_LENGTH_OF_ADDRESS); 
                 Index ++
               )
            {
                Status = EmacAddMulticastAddress( pAdapter, MacAddress); 
                if(NDIS_STATUS_SUCCESS != Status)
                {
                    return(Status);
                }    
                MacAddress += ETH_LENGTH_OF_ADDRESS;
                
                NdisMoveMemory(pAdapter->m_MulticastTable[Index], 
                        (PUINT8) MacAddress,
                        ETH_LENGTH_OF_ADDRESS);
                     
                pAdapter->m_NumMulticastEntries ++;
            }   
                
            /* 
             * Set Bytes Read as InformationBufferLength,  & status as
             * success 
             */
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
                         (L"Emac_MiniportSetInformation: Invalid PacketFilter\r\n"));
                Status = NDIS_STATUS_NOT_SUPPORTED;
                *BytesRead = 4;
                break;
            }
            
            /* 
             * Convert the NDIS Packet filter mapping into the EMAC
             * Receive packet Filter mapping. 
             */
            
            EmacPktFilter = 0;
            
            if((PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
               (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS))
            {    
                EmacPktFilter |= EMAC_RXMBPENABLE_RXCAFEN_ENABLE;
            }    
            
            if(PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
            {
                EmacPktFilter |= EMAC_RXMBPENABLE_RXBROADEN;
            }
            
            if(PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
            {
                /* Always supported */
            }
            if(PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
            { 
                EmacPktFilter |= EMAC_RXMBPENABLE_RXMULTIEN;
            }
            /* Set the current packet filter to EMAC Filter register */
            pAdapter->m_pEmacRegsBase->RXMBPENABLE = EmacPktFilter;
            
            /* Set Bytes Read as InformationBufferLength, & status as success */
            *BytesRead = InformationBufferLength;
            
            /* Update PacketFilter but we always have NDIS_PACKET_TYPE_DIRECTED set */
            pAdapter->m_PacketFilter = (PacketFilter|NDIS_PACKET_TYPE_DIRECTED);
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

        case OID_PNP_SET_POWER:
            DEBUGMSG(DBG_FUNC, (TEXT ("Emac_Filter: SetInformation: Got OID_PNP_SET_POWER\r\n")));
            if (!InformationBuffer || InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE))
            {
                // Buffer not big enough
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = sizeof(PowerState);
            }
            else
            {
                NdisMoveMemory(&PowerState, (PUCHAR)InformationBuffer, sizeof(PowerState));
                switch (PowerState)
                {
                case NdisDeviceStateD0:
                    // Power on device
                    DEBUGMSG(1, (TEXT ("Emac_Filter: SetInformation: Setting power state to D0\r\n")));
                    EMACModStateChange(pAdapter->m_device,ENABLED);
                    Status = NDIS_STATUS_SUCCESS;
                    break;
                case NdisDeviceStateD3:
                    // Power down device
                    DEBUGMSG(1, (TEXT ("Emac_Filter: SetInformation: Setting power state to D3\r\n")));
					EmacDisableInterrupts(pAdapter);
                    EMACModStateChange(pAdapter->m_device,DISABLED);
                    Status = NDIS_STATUS_SUCCESS;
                    break;
                default:
                    // Unsupported
                    DEBUGMSG(DBG_ERR, (TEXT ("Emac_Filter: SetInformation: Unsupported power state\r\n")));
                    *BytesRead = sizeof(PowerState);
                    *BytesNeeded = 0;
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                };
            }
            break;

        default:
            /* Set the status as NDIS_STATUS_INVALID_OID */
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }
    
    DEBUGMSG(DBG_FUNC && DBG_OID, (L"<-- Emac_MiniportSetInformation 0x%08X \r\n",Status));
    return(Status);
}
