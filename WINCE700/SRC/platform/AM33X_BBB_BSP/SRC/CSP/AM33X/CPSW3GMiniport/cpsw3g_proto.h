//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================
#ifndef __CPSW3G_PROTO_H_INCLUDED__
#define __CPSW3G_PROTO_H_INCLUDED__

#include <Ndis.h>
#include "Am33xCpsw3gRegs.h"

extern NDIS_PHYSICAL_ADDRESS g_HighestAcceptedMax;

VOID LinkChangeIntrHandler(CPSW3G_ADAPTER* Adapter);
//UINT32 PhyFindLink(CPSW3G_ADAPTER *Adapter);            

//cpsw3g_intr.c
VOID Cpsw3g_MiniportHandleInterrupt( NDIS_HANDLE  AdapterContext );
NDIS_STATUS Cpsw3g_MiniportInitialize(
    PNDIS_STATUS	OpenErrorStatus,
    PUINT			SelectedMediumIndex,
    PNDIS_MEDIUM	MediumArray,
    UINT			MediumArraySize,
    NDIS_HANDLE		MiniportAdapterHandle,
    NDIS_HANDLE		WrapperConfigurationContext
    );
VOID Cpsw3g_MiniportIsr(
    PBOOLEAN    InterruptRecognized,
    PBOOLEAN    QueueMiniportHandleInterrupt,
    NDIS_HANDLE MiniportAdapterContext
    );
    

NDIS_STATUS Cpsw3g_MiniportSetInformation(
    NDIS_HANDLE MiniportAdapterContext,
    NDIS_OID Oid,
    PVOID InformationBuffer,
    ULONG InformationBufferLength,
    PULONG BytesRead,
    PULONG BytesNeeded
    );

VOID Cpsw3g_MiniportDisableInterrupt(NDIS_HANDLE MiniportAdapterContext);
VOID Cpsw3g_MiniportEnableInterrupt(NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS Cpsw3g_MiniportRegisterOneInterrupt(
    PCPSW3G_ADAPTER          pAdapter,
    PNDIS_MINIPORT_INTERRUPT InterruptInfo, 
    UINT                     Interrupt
);

NDIS_STATUS Cpsw3g_MiniportRegisterInterrupts(PCPSW3G_ADAPTER  pAdapter);

VOID Cpsw3g_MiniportReturnPacket(
    NDIS_HANDLE MiniportAdapterContext,
    PNDIS_PACKET Packet
    );


//cpsw3g_adapter.c
NDIS_STATUS Cpsw3g_InitializeAdapter(PCPSW3G_ADAPTER  pAdapter);

VOID  Cpsw3g_ALE_age_out
(
    IN  PVOID	SystemSpecific1,
    IN  PVOID	FunctionContext,
    IN  PVOID	SystemSpecific2, 
    IN  PVOID	    SystemSpecific3
);
BOOL Cpsw3g_StopAdapter(PCPSW3G_ADAPTER pAdapter);
NDIS_STATUS Cpsw3g_InitSend(PCPSW3G_ADAPTER pAdapter);
NDIS_STATUS Cpsw3g_InitRecv(PCPSW3G_ADAPTER  pAdapter, UINT32 channel);
NDIS_STATUS Cpsw3g_MapAdapterRegs(PCPSW3G_ADAPTER   pAdapter);

enum { MOD_SYNCRST, MOD_ENABLE, MOD_DISABLE};
BOOL   Cpsw3g_ModStateChange( UINT32  ModState );
BOOL   Cpsw3g_Ethss_power_on(void);
BOOL   Cpsw3g_Ethss_power_off(void);

UINT32 Cpsw3g_get_current_mac_port (PCPSW3G_ADAPTER  pAdapter);
void Cpsw3g_free_Adapter_memory(PCPSW3G_ADAPTER pAdapter);
void Cpsw3g_Update_LinkStatus(PHY_DEVICE *phydev);
void Cpsw_read_vlan_ALE_entry(PCPSW3G_ADAPTER pAdapter);
void Cpsw_read_static_ALE_entry(PCPSW3G_ADAPTER pAdapter);
void cpsw3g_add_dynamic_vlan(PCPSW3G_ADAPTER pAdapter);
void Cpsw3g_Clear_hw_stats(PCPSW3G_ADAPTER pAdapter);
NDIS_STATUS Cpsw3g_SetPowerState(PCPSW3G_ADAPTER pAdapter, NDIS_DEVICE_POWER_STATE PowerState);
void Cpsw3g_reg_dump(PCPSW3G_ADAPTER pAdapter);


//cpsw3g_ndis.c
VOID Cpsw3g_FreeAdapter(PCPSW3G_ADAPTER  pAdapter, UINT32 stage);
UINT32 Cpsw3g_SelfCheck(CPSW3G_ADAPTER *pAdapter);

//cpsw3g_miniport.c
BOOLEAN	 Cpsw3g_MiniportCheckForHang( NDIS_HANDLE AdapterContext );
VOID Cpsw3g_MiniportHalt( NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS Cpsw3g_MiniportQueryInformation(
    NDIS_HANDLE AdapterContext,
    NDIS_OID    Oid,
    PVOID       InformationBuffer,
    ULONG       InformationBufferLength,
    PULONG      BytesWritten,
    PULONG      BytesNeeded
    );

NDIS_STATUS	 Cpsw3g_MiniportReset(
    PBOOLEAN    AddressingReset,
    NDIS_HANDLE AdapterContext
    );

VOID Cpsw3g_MiniportSendPacketsHandler(		
    NDIS_HANDLE  MiniportAdapterContext,
    PPNDIS_PACKET  PacketArray,
    UINT  NumberOfPackets
    ); 

//cpsw3g_ALE.c
int cpsw_ale_add_ucast_entry(PCPSW3G_ADAPTER pAdapter, unsigned char* mac, int port);
int cpsw_ale_del_ucast_entry(PCPSW3G_ADAPTER pAdapter, unsigned char* mac, int port);
int cpsw_ale_add_mcast_entry(PCPSW3G_ADAPTER pAdapter, unsigned char* mac, int vlan, int port_mask);
int cpsw_ale_del_mcast_entry(PCPSW3G_ADAPTER pAdapter, unsigned char* mac, int port_mask);
int cpsw_ale_flush_mcast_entry(PCPSW3G_ADAPTER pAdapter);
int cpsw_ale_dump(PCPSW3G_ADAPTER pAdapter);
int cpsw_ale_add_vlan_ucast_entry(PCPSW3G_ADAPTER pAdapter, int vlan_id, unsigned char* mac, int port) ;
int cpsw_ale_add_vlan_entry(PCPSW3G_ADAPTER pAdapter, 
                                                        int vlan_id, int member, int force_untag, 
                                                        int reg_mcast_flood_mask, int unreg_mcast_flood_mask);


//cpsw3g_mdio.c
void MdioWr(UINT16 phyAddr, UINT16 regNum, UINT16 data, int channel);
int MdioRd(int PhyAddr, int RegNum, int channel, UINT16 *pData);
UINT32 MdioAlive(void);
void MdioEnable(UINT32 RegsBase);


//cpsw3g_phy.c
LINKSTATUS Cpsw3g_Phy_Get_LinkStatus(PHY_DEVICE *phy_device);
VOID Cpsw3g_Phy_Init(PHY_DEVICE *phy_device);
VOID Cpsw3g_Phy_Stop(PHY_DEVICE *phy_device);

//cpsw3g_spf.c
VOID Cpsw3g_SPF_init( PCPSW3G_ADAPTER pAdater);

#endif /* #ifndef __CPSW3G_PROTO_H_INCLUDED__*/
