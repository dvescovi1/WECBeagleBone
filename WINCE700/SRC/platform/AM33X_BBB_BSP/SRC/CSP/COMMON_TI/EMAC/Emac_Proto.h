//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Proto.h
//! \brief Various functions prototypes used in miniport.
//! 
//! This header File contains the function prototypes being exposed and used
//! by all the source files.
//! 
//! \version  1.00 Aug 22nd 2006 File Created 

// Includes
#ifndef __EMAC_PROTO_H_INCLUDED__
#define __EMAC_PROTO_H_INCLUDED__

#include <Ndis.h>

extern NDIS_PHYSICAL_ADDRESS g_HighestAcceptedMax;

BOOLEAN     
Emac_MiniportCheckForHang(
    NDIS_HANDLE AdapterContext
    );

VOID
Emac_MiniportHalt(
    NDIS_HANDLE MiniportAdapterContext
    );


VOID        
Emac_MiniportHandleInterrupt(
    NDIS_HANDLE  AdapterContext
    );


NDIS_STATUS 
Emac_MiniportInitialize(
    PNDIS_STATUS    OpenErrorStatus,
    PUINT           SelectedMediumIndex,
    PNDIS_MEDIUM    MediumArray,
    UINT            MediumArraySize,
    NDIS_HANDLE     MiniportAdapterHandle,
    NDIS_HANDLE     WrapperConfigurationContext
    );
    
    
VOID
Emac_MiniportISR(
    PBOOLEAN    InterruptRecognized,
    PBOOLEAN    QueueMiniportHandleInterrupt,
    NDIS_HANDLE MiniportAdapterContext
    );
    
    
NDIS_STATUS 
Emac_MiniportQueryInformation(
    NDIS_HANDLE AdapterContext,
    NDIS_OID    Oid,
    PVOID       InformationBuffer,
    ULONG       InformationBufferLength,
    PULONG      BytesWritten,
    PULONG      BytesNeeded
    );

NDIS_STATUS 
Emac_MiniportReset(
    PBOOLEAN    AddressingReset,
    NDIS_HANDLE AdapterContext
    );
    

VOID 
Emac_MiniportSendPacketsHandler(        
    NDIS_HANDLE  MiniportAdapterContext,
    PPNDIS_PACKET  PacketArray,
    UINT  NumberOfPackets
    ); 

NDIS_STATUS 
Emac_MiniportSetInformation(
    NDIS_HANDLE MiniportAdapterContext,
    NDIS_OID Oid,
    PVOID InformationBuffer,
    ULONG InformationBufferLength,
    PULONG BytesRead,
    PULONG BytesNeeded
    );


VOID        
Emac_MiniportDisableInterrupt(
    NDIS_HANDLE MiniportAdapterContext
    );

VOID 
Emac_MiniportReturnPacket(
    NDIS_HANDLE MiniportAdapterContext,
    PNDIS_PACKET Packet
    );

VOID 
Emac_MiniportIsr(
    PBOOLEAN InterruptRecognized,
    PBOOLEAN QueueMiniportHandleInterrupt,
    NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS 
NICMapAdapterRegs(
    PEMAC_ADAPTER     Adapter
    );
    
NDIS_STATUS 
NICInitSend(
    PEMAC_ADAPTER     Adapter
    );
    
NDIS_STATUS 
NICInitRecv(
    PEMAC_ADAPTER  Adapter
    );

NDIS_STATUS 
NICInitializeAdapter(
    PEMAC_ADAPTER  Adapter
    );

UINT32 
ReadPhyRegister( 
    UINT32 PhyAddr,     
    UINT32 RegNum
    );
    
VOID    
WritePhyRegister( 
    UINT32 PhyAddr,     
    UINT32 RegNum,      
    UINT32 Data 
    );
                        
NDIS_STATUS 
NICInitializeAdapter(
    PEMAC_ADAPTER  Adapter
    );            

enum { SYNCRST, ENABLED, DISABLED};
BOOL
EMACModStateChange(
    OMAP_DEVICE device,
    UINT32  ModState 
    );

VOID
LinkChangeIntrHandler(
    MINIPORT_ADAPTER* Adapter
    );

UINT32 
PhyFindLink(
    MINIPORT_ADAPTER *Adapter
    );            

VOID
EmacFreeAdapter(
        PMINIPORT_ADAPTER  pAdapter
    );        

UINT32 
NICSelfTest(
    MINIPORT_ADAPTER *Adapter
    );


                              
#endif /* #ifndef __EMAC_PROTO_H_INCLUDED__*/
