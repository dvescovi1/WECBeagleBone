//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "am33x_base_regs.h"
#include "cpsw3g_miniport.h"
#include "cpsw3g_proto.h"
#include "cpsw3g_cfg.h"
#include "Am33XCpsw3gRegs.h"
#include <nkintr.h>

// ImageCfg.h is included for CPSW3G_BUF_BASE/SIZE
//#include "imagecfg.h"

extern int  ActiveCpgmac;


#ifdef DEBUG
DBGPARAM dpCurSettings = 
{
    TEXT("CPSW3G"), 
    {
        TEXT("Init"),TEXT("Critical"),TEXT("Interrupt"),TEXT("Message"),
        TEXT("Send"),TEXT("Receive"),TEXT("Info"),TEXT("Function"),
        TEXT("Oid"),TEXT(""),TEXT(""),TEXT(""),
        TEXT(""),TEXT(""),TEXT("Warnings"), TEXT("Errors")
    },
    0xC005
};

#endif  // DEBUG

#define CFG_Field(field)      ((UINT32)FIELD_OFFSET(CPSW3G_CONFIG, field)) 
#define CFG_FieldSize(field)  sizeof(((PCPSW3G_CONFIG)0)->field) 


CPSW3G_REG_CFG CPSW_RegTable[]=
{
    {NDIS_STRING_CONST("RxThreshInterrupt"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(RxThreshInterrupt),  CFG_FieldSize(RxThreshInterrupt), 
         CPSW3G_DEFAULT_RX_THRESH_INTERRUPT, 0, 127},       

    {NDIS_STRING_CONST("RxInterrupt"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(RxInterrupt),  CFG_FieldSize(RxInterrupt), 
         CPSW3G_DEFAULT_RX_INTERRUPT, 0, 127},       

    {NDIS_STRING_CONST("TxInterrupt"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(TxInterrupt),  CFG_FieldSize(TxInterrupt), 
         CPSW3G_DEFAULT_TX_INTERRUPT, 0, 127},       

    {NDIS_STRING_CONST("MiscInterrupt"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(MiscInterrupt),  CFG_FieldSize(MiscInterrupt), 
         CPSW3G_DEFAULT_MISC_INTERRUPT, 0, 127},       

    {NDIS_STRING_CONST("VlanAware"),  
         CFG_OPTIONAL,  NdisParameterInteger, 
         CFG_Field(vlanAware), CFG_FieldSize(vlanAware), 
         CPSW3G_DEFAULT_VLANAWARE, 0, 1},      

    {NDIS_STRING_CONST("mode"),  
         CFG_REQUIRED, NdisParameterInteger, 
         CFG_Field(cpsw3g_mode), CFG_FieldSize(cpsw3g_mode), 
         CPSW3G_DEFAULT_MODE, 0, 2},       

    {NDIS_STRING_CONST("ALEagingtimer"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(ALE_Agingtimer), CFG_FieldSize(ALE_Agingtimer), 
         CPSW3G_AGE_OUT_DELAY, 10, 1000000},       

    {NDIS_STRING_CONST("MacAuthorization"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(MacAuth), CFG_FieldSize(MacAuth),
         CPSW3G_DEFAULT_MAC_AUTH, 0, 1},       

    {NDIS_STRING_CONST("UnknownUntagEgress"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(UnknownUntagEgress), CFG_FieldSize(UnknownUntagEgress), 
         CPSW3G_DEFAULT_UNKNOWN_UNTAG_EGRESS, 0, 7},       

    {NDIS_STRING_CONST("UnknownRegMcastFloodMask"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(UnknownRegMcastFloodMask), CFG_FieldSize(UnknownRegMcastFloodMask), 
         CPSW3G_DEFAULT_UNKNOWN_REG_MCAST_MASK, 0, 7},       

    {NDIS_STRING_CONST("UnknownMcastFloodMask"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(UnknownMcastFloodMask), CFG_FieldSize(UnknownMcastFloodMask), 
         CPSW3G_DEFAULT_UNKNOWN_MCAST_MASK, 0, 7},       

    {NDIS_STRING_CONST("UnknownMemberList"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(UnknownMemberList), CFG_FieldSize(UnknownMemberList), 
         CPSW3G_DEFAULT_UNKNOWN_MEMBER_LIST, 0, 7},       

    {NDIS_STRING_CONST("8021xSupport"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(Enable_8021x), CFG_FieldSize(Enable_8021x), 
         CPSW3G_DEFAULT_8021X, 0, 1},          

    {NDIS_STRING_CONST("Software_VLANTagging"), 
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(sw_vlantagging), CFG_FieldSize(sw_vlantagging), 
         CPSW3G_DEFAULT_SW_VLANTAGGING, 0, 1},
                                                                        
    {NDIS_STRING_CONST("RX_serviceMax"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(rx_serviceMax), CFG_FieldSize(rx_serviceMax), 
         CPSW3G_DEFAULT_RX_SERVICE_MAX, 4, CPSW3G_DEFAULT_RX_SERVICE_MAX},

    {NDIS_STRING_CONST("ALE_bypass"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(ale_bypass), CFG_FieldSize(ale_bypass), 
         CPSW3G_DEFAULT_ALE_BYPASS, 0, 1},
                                                                        
    {NDIS_STRING_CONST("KITL_enable"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(KITL_enable), CFG_FieldSize(KITL_enable), 
         CPSW3G_DEFAULT_KITL_ENABLE, 0, 1},       
                                                                        
    {NDIS_STRING_CONST("Stats_port_mask"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(stats_port_mask), CFG_FieldSize(stats_port_mask), 
         CPSW3G_DEFAULT_STATS_PORT_MASK, 0, 7},       

    {NDIS_STRING_CONST("ALE_prescale"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_Field(ale_prescale), CFG_FieldSize(ale_prescale), 
         CPSW3G_DEFAULT_ALE_PRESCALE, 0, 0xfffff},       
};

    
#define CFG_PortHWCfgField(field)      ((UINT32)FIELD_OFFSET(Cpsw_PortHwConfig, field)) 
#define CFG_PortHWCfgFieldSize(field)  sizeof(((Cpsw_PortHwConfig *)0)->field) 
#define CFG_MacCfgField(field)         ((UINT32)FIELD_OFFSET(CpgmacPortConfig, field)) 
#define CFG_MacCfgFieldSize(field)     sizeof(((CpgmacPortConfig *)0)->field) 

CPSW3G_REG_CFG CPSW_PortRegTable[] =
{
    {NDIS_STRING_CONST("vlan_id"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(portVID), CFG_PortHWCfgFieldSize(portVID), 
         CPSW3G_DEFAULT_MAC_PORTVID, 0, 4095},       

    {NDIS_STRING_CONST("priority"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(portPri), CFG_PortHWCfgFieldSize(portPri), 
         CPSW3G_DEFAULT_MAC_PORTPRI, 0, 7},       

    {NDIS_STRING_CONST("BroadcastRateLimit"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(bcastLimit), CFG_PortHWCfgFieldSize(bcastLimit), 
         CPSW3G_DEFAULT_BCAST_RATELIMIT, 0, 255},       

    {NDIS_STRING_CONST("MulticastRateLimit"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(mcastLimit), CFG_PortHWCfgFieldSize(mcastLimit), 
         CPSW3G_DEFAULT_MCAST_RATELIMIT, 0, 255},     

    {NDIS_STRING_CONST("VIDIngressCheck"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(VIDIngressCheck), CFG_PortHWCfgFieldSize(VIDIngressCheck), 
         CPSW3G_DEFAULT_VID_INGRESS_CHECK, 10, 1000000},       

    {NDIS_STRING_CONST("DropUntagged"),  
         CFG_OPTIONAL, NdisParameterInteger, 
         CFG_PortHWCfgField(DropUntagged), CFG_PortHWCfgFieldSize(DropUntagged), 
         CPSW3G_DEFAULT_DROP_UNTAGGED, 10, 1000000},                                                                               
    {NDIS_STRING_CONST("LinkSpeed"),  
         CFG_NOT_SUPPORTED, NdisParameterInteger, 
         CFG_MacCfgField(LinkSpeed), CFG_MacCfgFieldSize(LinkSpeed), 
         0x0, 0, 1000},       

    {NDIS_STRING_CONST("LinkDuplex"),  
         CFG_NOT_SUPPORTED, NdisParameterInteger, 
         CFG_MacCfgField(LinkDuplex), CFG_MacCfgFieldSize(LinkDuplex), 
         0x0, 0, 1},       
};

#define CPSW3G_NUM_REG_PARAMS        (sizeof (CPSW_RegTable)     / sizeof(CPSW3G_REG_CFG))
#define CPSW3G_NUM_PORT_REG_PARAMS   (sizeof (CPSW_PortRegTable) / sizeof(CPSW3G_REG_CFG))

UINT16 phyaddr[2]={EXTERNAL_PHY0_ADDR, EXTERNAL_PHY1_ADDR};


/*******************************************************************
Function Name: DriverEntry
Description: Creates an association between the minoport driver and the NDIS 
                  library, and registers the miniport driver's version number and entry
                  points with NDIS.

Arguments:

    DriverObject    -   pointer to the driver object
    RegistryPath    -   pointer to the driver registry path
     
Return Value:
    
    NDIS_STATUS - the value returned by NdisMRegisterMiniport 
    
********************************************************************/
NDIS_STATUS    
DriverEntry(
    PVOID  DriverObject, 
    PVOID  RegistryPath
    )
{
    NDIS_STATUS                   Status;
    NDIS_HANDLE                   NdisWrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS Cpsw3gChar;
           
    DEBUGMSG(DBG_FUNC, (L"  -> CPSW3G DriverEntry\r\n"));

    /*  Notify the  NDIS library that driver is about to register itself as
     *  a miniport. NDIS sets up the state it needs to track the driver and
     *  returns an NDISWrapperHandle which driver uses for subsequent calls.
     */
    
    NdisMInitializeWrapper(
        &NdisWrapperHandle,
        DriverObject,
        RegistryPath,
        NULL);

    if (NdisWrapperHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;

        DEBUGMSG(Status, (L"  <- DriverEntry failed to InitWrapper, Status=%x \r\n", Status));
        return Status;
    }

    /* Fill in the Miniport characteristics structure with the version numbers 
     * and the entry points for driver-supplied MiniportXxx 
     */
     
    NdisZeroMemory(&Cpsw3gChar, sizeof(Cpsw3gChar));

    Cpsw3gChar.MajorNdisVersion				= CPSW3G_NDIS_MAJOR_VERSION;
    Cpsw3gChar.MinorNdisVersion				= CPSW3G_NDIS_MINOR_VERSION;

    Cpsw3gChar.CheckForHangHandler			= Cpsw3g_MiniportCheckForHang;
    Cpsw3gChar.DisableInterruptHandler		= Cpsw3g_MiniportDisableInterrupt;
    Cpsw3gChar.EnableInterruptHandler		= NULL; 
    Cpsw3gChar.HaltHandler					= Cpsw3g_MiniportHalt;
    Cpsw3gChar.HandleInterruptHandler		= Cpsw3g_MiniportHandleInterrupt;  
    Cpsw3gChar.InitializeHandler			= Cpsw3g_MiniportInitialize;
    Cpsw3gChar.ISRHandler					= Cpsw3g_MiniportIsr;  
    Cpsw3gChar.QueryInformationHandler		= Cpsw3g_MiniportQueryInformation;
    Cpsw3gChar.ReconfigureHandler			= NULL;  
    Cpsw3gChar.ResetHandler					= Cpsw3g_MiniportReset;
    Cpsw3gChar.SendHandler					= NULL;
    Cpsw3gChar.SetInformationHandler		= Cpsw3g_MiniportSetInformation;
    Cpsw3gChar.TransferDataHandler			= NULL;  
    Cpsw3gChar.ReturnPacketHandler			= Cpsw3g_MiniportReturnPacket;
    Cpsw3gChar.SendPacketsHandler			= Cpsw3g_MiniportSendPacketsHandler;
    Cpsw3gChar.AllocateCompleteHandler		= NULL;
    
    DEBUGMSG(DBG_INFO, (L"Calling NdisMRegisterMiniport with NDIS_MINIPORT_MAJOR_VERSION %d" \
                     L" & NDIS_MINIPORT_MINOR_VERSION %d\r\n",
                      NDIS_MINIPORT_MAJOR_VERSION,NDIS_MINIPORT_MINOR_VERSION));

    DEBUGMSG(DBG_INFO, (L"CPSW3G Miniport driver version:%d.%d\r\n", \
        CPSW3G_DRIVER_MAJOR_VERSION, CPSW3G_DRIVER_MINOR_VERSION));
    
    /* Calling NdisMRegisterMiniport causes driver's MiniportInitialise
     * function to run in the context of NdisMRegisterMiniport.
     */
    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &Cpsw3gChar,
                 sizeof(NDIS_MINIPORT_CHARACTERISTICS));
    
    if(Status != NDIS_STATUS_SUCCESS)
    {
        /* Call NdisTerminateWrapper, and return the error code to the OS. */
        NdisTerminateWrapper(
            NdisWrapperHandle, 
            NULL);          /* Ignored */            
    }
        
    DEBUGMSG(DBG_INFO, (L"  <- DriverEntry, Status= %x\r\n", Status));

    return Status;
    
}

/*******************************************************************
Function Name: DllEntry
Description: 

Arguments:

    DllhDLL    -   handle
    dwReason   -   
    lpReserved 
     
Return Value: 
    
********************************************************************/
BOOL __stdcall DllEntry (HANDLE DllhDLL, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
                DEBUGREGISTER(DllhDLL);
                DEBUGMSG(DBG_INIT, (L"Cpsw3g Miniport : DLL Process Attach\r\n"));
                DisableThreadLibraryCalls((HMODULE) DllhDLL);
                break;
                
        case DLL_PROCESS_DETACH:
                DEBUGMSG(DBG_INIT, (L"Cpsw3g Miniport :  DLL Process Detach\r\n"));
                break;
    }
    return TRUE;
}

INT32 Cpsw3g_read_KITL_port(UINT32 *port)
{
// +++FIXME
#if 0
    DWORD           ret_value;

    if (!KernelIoControl(IOCTL_HAL_QUERY_KITL_MAC_PORT, NULL, 0, port, sizeof(UINT32), &ret_value)){
        ret_value=0;
    }
    return (INT32)ret_value;
#else
    return 0;  // 0: FAIL
#endif
}

INT32 Cpsw3g_read_Speed_Match(UINT32 *speed_match)
{
// +++FIXME
#if 0
    DWORD           ret_value;

    if (!KernelIoControl(IOCTL_HAL_QUERY_SPEED_MATCH, NULL, 0, speed_match, sizeof(UINT32), &ret_value)){
        ret_value=0;
    }
    return (INT32)ret_value;
#else
    return 0;
#endif
}

BOOL Cpsw3gReadMACAddrFromBootArgs(PCPSW3G_ADAPTER pAdapter,  UINT32 port)
{
// +++FIXME
#if 0
    BOOL            ret = TRUE;
    UINT8           mac[ETH_LENGTH_OF_ADDRESS];
    UINT8           zero_mac[ETH_LENGTH_OF_ADDRESS];
    DWORD          ret_value;
    
    memset(zero_mac, 0, ETH_LENGTH_OF_ADDRESS);
    if (!KernelIoControl(IOCTL_HAL_QUERY_KITL_MAC_ADDRESS, &port, sizeof(UINT32), mac, ETH_LENGTH_OF_ADDRESS, &ret_value)){
        ret = FALSE;
    }
    
    if( ret==FALSE ||
        !memcmp(mac, broadcast_mac, ETH_LENGTH_OF_ADDRESS) ||
        !memcmp(mac, zero_mac, ETH_LENGTH_OF_ADDRESS) )
    {
        /* Upon EEPROM read failure, use pre-defined default MAC address */
        if(port>1) port = 1;
        memcpy(pAdapter->MACAddress, g_default_mac_address[port], ETH_LENGTH_OF_ADDRESS);      
    }
    else
    {      
        memcpy(pAdapter->MACAddress, mac, ETH_LENGTH_OF_ADDRESS);
    }
    RETAILMSG(TRUE,  (L"READ mac address from Boot args,   %x %x %x %x %x %x\r\n",
                            pAdapter->MACAddress[0], pAdapter->MACAddress[1], pAdapter->MACAddress[2], 
                            pAdapter->MACAddress[3], pAdapter->MACAddress[4], pAdapter->MACAddress[5]));

    return TRUE;
#else
    return FALSE;
#endif
}


#ifdef MAC_IN_EEPROM
BOOL Cpsw3gReadEEProm(PCPSW3G_ADAPTER  pAdapter, UINT32 port)
{
    UINT8           ReadBuffer[ETH_LENGTH_OF_ADDRESS];
    UINT8           offset;

    offset = (port == 1)? MAC1_EEPROM_OFFSET : MAC0_EEPROM_OFFSET; 
    
    if(EEPROM_read(offset, ReadBuffer, ETH_LENGTH_OF_ADDRESS) != ETH_LENGTH_OF_ADDRESS)
    {
        RETAILMSG(TRUE,  (L"Read MAC address from EEPROM failed\r\n"));

        /* Upon EEPROM read failure, use pre-defined default MAC address */
        if(port>1) port = 1;
        memcpy(pAdapter->MACAddress, g_default_mac_address[port], ETH_LENGTH_OF_ADDRESS);
    }
    else
    {
        RETAILMSG(TRUE,  (L"READ mac address from EEPROM,   %x %x %x %x %x %x\r\n",
            ReadBuffer[0], ReadBuffer[1], ReadBuffer[2], ReadBuffer[3], ReadBuffer[4], ReadBuffer[5]));

        /* Fill to Adapter structure */
        memcpy(pAdapter->MACAddress, ReadBuffer, ETH_LENGTH_OF_ADDRESS);
    }
    return (TRUE);
}
#endif


#ifdef MAC_IN_FUSE
BOOL Cpsw3gReadMacAddrFromFuse(PCPSW3G_ADAPTER pAdapter, UINT32 port)
{
    DWORD                 low, high;
    DWORD                 MacIdRegsBase;
    PMAC_ID_REGS          pMacIdRegs;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
    NDIS_STATUS           Status;
    BOOL                  rc = TRUE;

    low = 0x0; 
    high= 0x0;
    NdisSetPhysicalAddressHigh(PhysicalAddress, 0);

	switch (port){
		case CPGMAC_PORT_START:
            NdisSetPhysicalAddressLow (PhysicalAddress, AM33X_DEVICE_CONF_REGS_PA + AM33X_MAC_ID0_OFFSET);
			break;
		case CPGMAC_PORT_END:
            NdisSetPhysicalAddressLow (PhysicalAddress, AM33X_DEVICE_CONF_REGS_PA + AM33X_MAC_ID1_OFFSET);
			break;
		default:
            rc = FALSE;
	}

    if (rc)
    {
        /* Mapping the MAC_ID registers */
        Status = NdisMMapIoSpace((VOID*)&MacIdRegsBase,
                        pAdapter->AdapterHandle,
                        PhysicalAddress,
                        sizeof(MAC_ID_REGS));

        if(Status == NDIS_STATUS_SUCCESS)
        {
            pMacIdRegs = (PMAC_ID_REGS)MacIdRegsBase;
			low  = pMacIdRegs->MAC_ID_LO;
			high = pMacIdRegs->MAC_ID_HI;
        }
        else
        {
            low = 0; high= 0;
            rc = FALSE;
        }
    }

    pAdapter->MACAddress[0] = (UCHAR) ((high >>  0) & 0xFF);
    pAdapter->MACAddress[1] = (UCHAR) ((high >>  8) & 0xFF);
    pAdapter->MACAddress[2] = (UCHAR) ((high >> 16) & 0xFF);
    pAdapter->MACAddress[3] = (UCHAR) ((high >> 24) & 0xFF);
    pAdapter->MACAddress[4] = (UCHAR) ((low  >>  0) & 0xFF);
    pAdapter->MACAddress[5] = (UCHAR) ((low  >>  8) & 0xFF);

    DEBUGMSG(DBG_INFO,(L"Cpsw3gReadMacAddrFromFuse: %x %x -> %x %x %x %x %x %x\r\n", high, low,
        pAdapter->MACAddress[0], pAdapter->MACAddress[1],
        pAdapter->MACAddress[2], pAdapter->MACAddress[3],
        pAdapter->MACAddress[4], pAdapter->MACAddress[5]
    ));

    return rc;
}
#endif


#ifdef DEBUG

void    Cpsw3g_dump_config(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 i;
    CPSW3G_CONFIG     *pSwCfg = NULL;
    CpgmacPortConfig  *pMacPortCfg = NULL;
    Cpsw_PortHwConfig *pDmaPortHwCfg = NULL;

    pSwCfg = &(pAdapter->Cfg);
    RETAILMSG(TRUE, (L"Cpsw3g Configuration: \r\n"));
    RETAILMSG(TRUE, (L"    RxThreshInterrupt: %d\r\n",        pSwCfg->RxThreshInterrupt));
    RETAILMSG(TRUE, (L"    RxInterrupt: %d\r\n",              pSwCfg->RxInterrupt));
    RETAILMSG(TRUE, (L"    TxInterrupt: %d\r\n",              pSwCfg->TxInterrupt));
    RETAILMSG(TRUE, (L"    MiscInterrupt: %d\r\n",            pSwCfg->MiscInterrupt));
    RETAILMSG(TRUE, (L"    cpsw3g_mode: %d\r\n",              pSwCfg->cpsw3g_mode));
    RETAILMSG(TRUE, (L"    KITL_port: %d\r\n",                pSwCfg->KITL_port));
    RETAILMSG(TRUE, (L"    ALE_Agingtimer: %d\r\n",           pSwCfg->ALE_Agingtimer));
    RETAILMSG(TRUE, (L"    MacAuth: %d\r\n",                  pSwCfg->MacAuth));    
    RETAILMSG(TRUE, (L"    UnknownUntagEgress: %d\r\n",       pSwCfg->UnknownUntagEgress));
    RETAILMSG(TRUE, (L"    UnknownRegMcastFloodMask: %d\r\n", pSwCfg->UnknownRegMcastFloodMask));
    RETAILMSG(TRUE, (L"    UnknownMcastFloodMask: %d\r\n",    pSwCfg->UnknownMcastFloodMask));
    RETAILMSG(TRUE, (L"    UnknownMemberList: %d\r\n",        pSwCfg->UnknownMemberList));
    RETAILMSG(TRUE, (L"    Enable_8021x: %d\r\n",             pSwCfg->Enable_8021x));
    RETAILMSG(TRUE, (L"    rxVlanEncap: %d\r\n",              pSwCfg->rxVlanEncap));
    RETAILMSG(TRUE, (L"    sw_vlantagging: %d\r\n",           pSwCfg->sw_vlantagging));
    RETAILMSG(TRUE, (L"    KITL_enable: %d\r\n",              pSwCfg->KITL_enable));
    RETAILMSG(TRUE, (L"    stats_port_mask: %d\r\n",          pSwCfg->stats_port_mask));
    
    for(i=0;i<CPSW3G_NUM_MAC_PORTS;i++)
    {
    pMacPortCfg = &(pAdapter->Cfg.macCfg[i]);
    RETAILMSG(TRUE, (L"Cpsw3g MAC portConfiguration: port %d\r\n",i+1));
    RETAILMSG(TRUE, (L"    PhyAddr: %d\r\n",         pMacPortCfg->PhyAddr));
    RETAILMSG(TRUE, (L"    LinkSpeed: %d\r\n",       pMacPortCfg->LinkSpeed));
    RETAILMSG(TRUE, (L"    LinkDuplex: %d\r\n",      pMacPortCfg->LinkDuplex));
    RETAILMSG(TRUE, (L"    portPriority: %d\r\n",    pMacPortCfg->hw_config.portPri));
    RETAILMSG(TRUE, (L"    portVID: %d\r\n",         pMacPortCfg->hw_config.portVID));
    RETAILMSG(TRUE, (L"    bcastLimit: %d\r\n",      pMacPortCfg->hw_config.bcastLimit));
    RETAILMSG(TRUE, (L"    mcastLimit: %d\r\n",      pMacPortCfg->hw_config.mcastLimit));
    RETAILMSG(TRUE, (L"    VIDIngressCheck: %d\r\n", pMacPortCfg->hw_config.VIDIngressCheck));
    RETAILMSG(TRUE, (L"    DropUntagged: %d\r\n",    pMacPortCfg->hw_config.DropUntagged));
    RETAILMSG(TRUE, (L"    rxMaxlen: %d\r\n",        pMacPortCfg->rxMaxlen));
    RETAILMSG(TRUE, (L"    passCrc: %d\r\n",         pMacPortCfg->passCrc));
    RETAILMSG(TRUE, (L"    phy addr: %d\r\n",        pMacPortCfg->PhyAddr));
    }

    pDmaPortHwCfg = &(pAdapter->Cfg.dmaCfg.hw_config);
    RETAILMSG(TRUE, (L"Cpsw3g MAC portConfiguration: port %d\r\n", CPSW3G_HOST_PORT));
    RETAILMSG(TRUE, (L"    portPriority: %d\r\n",    pDmaPortHwCfg->portPri));
    RETAILMSG(TRUE, (L"    portVID: %d\r\n",         pDmaPortHwCfg->portVID));
    RETAILMSG(TRUE, (L"    bcastLimit: %d\r\n",      pDmaPortHwCfg->bcastLimit));
    RETAILMSG(TRUE, (L"    mcastLimit: %d\r\n",      pDmaPortHwCfg->mcastLimit));
    RETAILMSG(TRUE, (L"    VIDIngressCheck: %d\r\n", pDmaPortHwCfg->VIDIngressCheck));
    RETAILMSG(TRUE, (L"    DropUntagged: %d\r\n",    pDmaPortHwCfg->DropUntagged));
}

#endif  // DEBUG

NDIS_STATUS   Cpsw3g_read_cpsw_setting(PCPSW3G_ADAPTER pAdapter, NDIS_HANDLE ConfigurationHandle)
{
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER   pReturnedValue;    
    CPSW3G_REG_CFG                  *pRegEntry;
    UINT32                          i, value;
    PUCHAR                          param;

    /* Open the registry for this adapter */
    if (!ConfigurationHandle )
    {
        DEBUGMSG(DBG_WARN, (L"Cpsw_read_cpsw_setting:: NULL  ConfigurationHandle\n"));
        return NDIS_STATUS_FAILURE;
    }

    // read all the registry values 
    for (i = 0, pRegEntry = CPSW_RegTable; i < CPSW3G_NUM_REG_PARAMS; i++, pRegEntry++)
    {
        param = (PUCHAR)&pAdapter->Cfg + pRegEntry->Cfg_field;

        if(pRegEntry->cfg_type == CFG_NOT_SUPPORTED)
        {
            DEBUGMSG(DBG_WARN, (L" is not currently supported.\r\n"));
            continue;
        }
        
        // Get the configuration value for a specific parameter.  
        NdisReadConfiguration(
            &Status,
            &pReturnedValue,
            ConfigurationHandle,
            &pRegEntry->keyword,
            NdisParameterInteger);

        // If the parameter was present, then check its value for validity.
        if (Status == NDIS_STATUS_SUCCESS)
        {
            // Check that param value is not too small or too large
            if (pReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
                pReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
            {
                value = pRegEntry->Default;
            }
            else
            {
                value = pReturnedValue->ParameterData.IntegerData;
            }

        }
        else if (pRegEntry->cfg_type == CFG_REQUIRED)
        {
            ASSERT(FALSE);

            Status = NDIS_STATUS_FAILURE;
            break;
        }
        else
        {
            value = pRegEntry->Default;
            Status = NDIS_STATUS_SUCCESS;
        }

        // Store the value in the adapter structure.
        switch(pRegEntry->FieldSize)
        {
            case 1:
                *((PUCHAR) param) = (UCHAR) value;
                break;

            case 2:
                *((PUSHORT) param) = (USHORT) value;
                break;

            case 4:
                *((PULONG) param) = (ULONG) value;
                break;

            default:
                DEBUGMSG(DBG_WARN,(L"Bogus field size %d\r\n", pRegEntry->FieldSize));
                break;
        }
    }

    return Status;
}

NDIS_STATUS   Cpsw3g_read_port_setting(PCPSW3G_ADAPTER pAdapter, NDIS_HANDLE ConfigurationHandle)
{
    NDIS_STATUS                         Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE                         SubKeyHandle;
    PNDIS_CONFIGURATION_PARAMETER       pReturnedValue;    
    CPSW3G_REG_CFG *pRegEntry;
    NDIS_STRING nszKeyword  = {0};
    UINT32 i,j, value;
    PUCHAR param;

    /* Open the registry for this adapter */
    if (!ConfigurationHandle)
    {
        DEBUGMSG(DBG_WARN, (L"Cpsw_read_port_setting:: NULL ConfigurationHandle\n"));
        return NDIS_STATUS_FAILURE;
    }

    // read PORT based registry values 
    for(j=0;j<CPSW3G_NUM_PORTS;j++)
    {
        UINT32 num_entry = CPSW3G_NUM_PORT_REG_PARAMS;
        wchar_t wstr[10];

        /* dma port only read vlan_id , priority, and bcast/mcast rate limit value */
        if(j == CPSW3G_HOST_PORT) num_entry = 4;
        
        /* Open port registry */
        wsprintf(wstr, L"%s%d", L"Port", j);
        
        NdisInitUnicodeString (&nszKeyword, wstr);
        
        NdisOpenConfigurationKeyByName(
                &Status,
                ConfigurationHandle,
                &nszKeyword,
                &SubKeyHandle
        );
        // If the parameter was present, then check its value for validity.
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_WARN,(L"Cpsw_read_port_setting: read subkey failed \r\n"));
            continue;
        }

        for (i = 0, pRegEntry = CPSW_PortRegTable; i < num_entry; i++, pRegEntry++)
        {
            if (j == CPSW3G_HOST_PORT)
                param = (PUCHAR)&pAdapter->Cfg.dmaCfg.hw_config + pRegEntry->Cfg_field;
            else
                param = (PUCHAR)&pAdapter->Cfg.macCfg[P2I(j)].hw_config + pRegEntry->Cfg_field;
            
            if(pRegEntry->cfg_type == CFG_NOT_SUPPORTED)
            {
                DEBUGMSG(DBG_WARN, (L" %s is not currently supported.\r\n", pRegEntry->keyword.Buffer));
                continue;
            }

            // Get the configuration value for a specific parameter.  
            NdisReadConfiguration(
                &Status,
                &pReturnedValue,
                SubKeyHandle,
                &pRegEntry->keyword,
                NdisParameterInteger);

            // If the parameter was present, then check its value for validity.
            if (Status == NDIS_STATUS_SUCCESS)
            {
                // Check that param value is not too small or too large
                if (pReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
                    pReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
                {
                    value = pRegEntry->Default;
                }
                else
                {
                    value = pReturnedValue->ParameterData.IntegerData;
                }

            }
            else if (pRegEntry->cfg_type == CFG_REQUIRED)
            {

                ASSERT(FALSE);

                Status = NDIS_STATUS_FAILURE;
                break;
            }
            else
            {
                value = pRegEntry->Default;
                Status = NDIS_STATUS_SUCCESS;
            }

            // Store the value in the adapter structure.
            switch(pRegEntry->FieldSize)
            {
                case 1:
                    *((PUCHAR) param) = (UCHAR) value;
                    break;

                case 2:
                    *((PUSHORT) param) = (USHORT) value;
                    break;

                case 4:
                    *((PULONG) param) = (ULONG) value;
                    break;

                default:
                    DEBUGMSG(DBG_WARN, (L"Bogus field size %d\r\n", pRegEntry->FieldSize));
                    break;
            }
        }
        NdisCloseConfiguration(SubKeyHandle);
    }

    return Status;
}

NDIS_STATUS Cpsw3g_ReadRegParameters(
    PCPSW3G_ADAPTER  pAdapter,
    NDIS_HANDLE      WrapperConfigurationContext)
{
    NDIS_STATUS    Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE    ConfigurationHandle;

    PCPSW3G_CONFIG iCfg = &pAdapter->Cfg;
    UINT32 i; 

    /* Open the registry for this adapter */
    NdisOpenConfiguration(
        &Status,
        &ConfigurationHandle,
        WrapperConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"NdisOpenConfiguration failed\n"));
        goto Exit;
    }
    
    if ((Status = Cpsw3g_read_cpsw_setting(pAdapter, ConfigurationHandle)) != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"Read CPSW settings failed\n"));
        goto Exit;
    }

    if ((Status = Cpsw3g_read_port_setting(pAdapter, ConfigurationHandle)) != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"Read port settings failed\n"));
        goto Exit;
    }
        
    Cpsw_read_static_ALE_entry(pAdapter);
    Cpsw_read_vlan_ALE_entry(pAdapter);    

    /* Read KITL port from BSP args */
    if(!Cpsw3g_read_KITL_port(&pAdapter->Cfg.KITL_port)){
        DEBUGMSG(DBG_WARN, (L"    Cpsw3g_read_KITL_port failed!\r\n"));
        pAdapter->Cfg.KITL_port = CPGMAC_PORT_START;
//        Status= NDIS_STATUS_FAILURE;
    }

    /* Read option value for Ethernet ports speed match */
    if(!Cpsw3g_read_Speed_Match((UINT32 *)&pAdapter->Cfg.Speed_match)){
        DEBUGMSG(DBG_WARN, (L"    Cpsw3g_read_Speed_Match failed!\r\n"));
        pAdapter->Cfg.Speed_match = FALSE;
    }
    
    pAdapter->Cfg.rxVlanEncap = 0;

    // Default MAC sliver config
    for (i=CPGMAC_PORT_START; i <= CPGMAC_PORT_END; i++)
    {
  
        iCfg->macCfg[P2I(i)].hw_config.portCfi = CPSW3G_DEFAULT_MAC_PORTCFI;

        if(iCfg->vlanAware)
            iCfg->macCfg[P2I(i)].rxMaxlen = CPSW3G_DEFAULT_RXMAXLEN + 4;
        else
            iCfg->macCfg[P2I(i)].rxMaxlen = CPSW3G_DEFAULT_RXMAXLEN;

        /* Always set to use External Phy */
        iCfg->macCfg[P2I(i)].internalPhy = FALSE;
        iCfg->macCfg[P2I(i)].PhyAddr = phyaddr[P2I(i)];
    }

    // Default Dma config
    iCfg->dmaCfg.hw_config.portCfi = CPSW3G_DEFAULT_DMA_PORTCFI;
    
    iCfg->mdioTickMSec = AVALANCHE_DEFAULT_MDIOTICK;
    iCfg->resetLine = 0;
    iCfg->mdioBusFrequency = AVALANCHE_DEFAULT_MDIOBUSFREQ;
    iCfg->mdioClockFrequency = AVALANCHE_DEFAULT_MDIOCLOCKFREQ;

#ifdef DEBUG
    Cpsw3g_dump_config(pAdapter);
#endif  // DEBUG

Exit:
     /* Close the registry */
    NdisCloseConfiguration(ConfigurationHandle);
    return Status;
}

//========================================================================
//!  \fn NDIS_STATUS NICReadAdapterInfo(PMP_ADAPTER Adapter,
//!                                       NDIS_HANDLE  WrapperConfigurationContext)
//!  \brief Fetching information from registry and filling information.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
NDIS_STATUS
NICReadAdapterInfo(
    PCPSW3G_ADAPTER     pAdapter,
    NDIS_HANDLE           WrapperConfigurationContext
    )

{
    NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (L"  -> NICReadAdapterInfo\n"));

    // Read configuration from registry.
    Status = Cpsw3g_ReadRegParameters(pAdapter, WrapperConfigurationContext);

    /* Filling Rx information */
    pAdapter->NumRxMacBufDesc		= CPSW3G_MAX_RXBUF_DESCS;
    pAdapter->NumRxIndicatePkts		= NDIS_INDICATE_PKTS;

    /* Filling Tx information */
    pAdapter->MaxPacketPerTransmit	= MAX_NUM_PACKETS_PER_SEND;
    pAdapter->MaxTxMacBufDesc		= CPSW3G_MAX_TXBUF_DESCS;

#if defined(MAC_IN_FUSE)
    // Read MAC addr from fuse.
    if (FALSE == Cpsw3gReadMacAddrFromFuse(pAdapter, Cpsw3g_get_current_mac_port(pAdapter)))
    {
        DEBUGMSG(DBG_FUNC, (L"Unable to read MAC address from FUSE\r\n"));
        return NDIS_STATUS_FAILURE;
    }
#elif defined(MAC_IN_EEPROM)
    /* Read MAC information stored in EEPROM, save in local structure */
    if (FALSE == Cpsw3gReadEEProm(pAdapter, Cpsw3g_get_current_mac_port(pAdapter)))
    {
        DEBUGMSG(DBG_FUNC, (L"Unable to read MAC address from EEPROM\r\n"));
        return NDIS_STATUS_FAILURE;
    }
#else
    if (FALSE == Cpsw3gReadMACAddrFromBootArgs(pAdapter, Cpsw3g_get_current_mac_port(pAdapter)))
    {
        RETAILMSG(TRUE, (L"Unable to read MAC address from BootArgs \r\n"));
        return NDIS_STATUS_FAILURE;      
    }
#endif   

    DEBUGMSG(DBG_FUNC, (L"  <- NICReadAdapterInfo\r\n"));

    return (Status);
}


/*******************************************************************
Function Name: Cpsw3g_NICAllocAdapterBlock
Description: Allocate Cpsw3g_ADAPTER data block

Arguments:

    pAdapter    -   handle of MINIPORT_ADAPTER

Return Value: 
    NDIS_STATUS
    
********************************************************************/
NDIS_STATUS
Cpsw3g_NICAllocAdapterBlock(
    PCPSW3G_ADAPTER* pAdapter
    )
{
    PCPSW3G_ADAPTER     pNewAdapter;
    NDIS_STATUS           Status;

    DEBUGMSG(DBG_FUNC, (L"  -> NICAllocAdapter\n"));

    *pAdapter = NULL;

    /* Allocate memory */
    Status = NdisAllocateMemoryWithTag(
            (PVOID *)&pNewAdapter,
            (UINT)sizeof(CPSW3G_ADAPTER),
            NIC_TAG);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        /* Bail Out */	
        DEBUGMSG(DBG_ERR, (L"Cpsw3g_NICAllocAdapterBlock: NdisAllocateMemory failed\n"));
        goto ErrorExit;
    }

    /* Clear out the adapter block */
    NdisZeroMemory (pNewAdapter, sizeof(CPSW3G_ADAPTER));

    /* Allocating the spin locks */
    NdisAllocateSpinLock(&pNewAdapter->Lock);
    NdisAllocateSpinLock(&pNewAdapter->SendLock);
    NdisAllocateSpinLock(&pNewAdapter->RcvLock);

    *pAdapter = pNewAdapter;

    DEBUGMSG(DBG_FUNC, (L"  <- NICAllocAdapter, Status=%x\n", Status));


ErrorExit:    
    return Status;

}

/*******************************************************************
Function Name: Cpsw3g_FreeAdapter
Description: Free CPGMAC_ADAPTER data block

Arguments:

    pAdapter    -   handle of MINIPORT_ADAPTER

Return Value: 
    None
    
********************************************************************/
VOID
Cpsw3g_FreeAdapter(PCPSW3G_ADAPTER  pAdapter, UINT32 stage)
{

    DEBUGMSG(DBG_FUNC, (L"  -> Cpsw3g_FreeAdapter\n"));

    NdisDprAcquireSpinLock(&pAdapter->Lock);

    /*
    * Check if the Interrupt has been registered, if so then deregister the
    * interrupt for the adapter. Rest, all deallocation is done in reverse way
    */
  
       if(stage<=0 || stage>3) goto Exit;
            
        if(NULL != pAdapter->pCpsw3gRegsBase)
        {
            NdisMUnmapIoSpace(pAdapter->AdapterHandle,
                              (PVOID)pAdapter->pCpsw3gRegsBase,
                              CPSW3G_MEMORY_SIZE);
        }

        if(stage==1) goto Exit;
            
        if(0 != pAdapter->Cfg.RxThreshInterrupt)
        {
            NdisMDeregisterInterrupt(&pAdapter->RxThreshInterruptInfo);
        }

        if(0 != pAdapter->Cfg.RxInterrupt)
        {
            NdisMDeregisterInterrupt(&pAdapter->RxInterruptInfo);
        }

        if(0 != pAdapter->Cfg.TxInterrupt)
        {
            NdisMDeregisterInterrupt(&pAdapter->TxInterruptInfo);
        }

        if(0 != pAdapter->Cfg.MiscInterrupt)
        {
            NdisMDeregisterInterrupt(&pAdapter->MiscInterruptInfo);
        }

        if(stage==2) goto Exit;
            
        Cpsw3g_free_Adapter_memory(pAdapter);
            
Exit:
    NdisFreeSpinLock(&pAdapter->Lock);

    /* Finally free adapter memory itself  */
    NdisFreeMemory((PVOID *)&pAdapter,
                sizeof(CPSW3G_ADAPTER),
                0);
    DEBUGMSG(DBG_FUNC, (L"  <- Cpsw3g_FreeAdapter \r\n"));
  
    return;

}

//========================================================================
//!  \fn UINT32 NICSelfTest(PMP_ADAPTER Adapter)
//!  \brief Does a self test on controller for Rx/Tx before giving NDIS.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
UINT32 Cpsw3g_SelfCheck(CPSW3G_ADAPTER *pAdapter)
{
    UNREFERENCED_PARAMETER(pAdapter);

    /* TBD */
    return (NDIS_STATUS_SUCCESS);
}

/* For 2 cpgmac mode only*/
//========================================================================
//  LoadISR()
//
//  Routine Description:
//
//      This routine is going to load Interrupt Service Handler into nk.exe
//      space.   
//      The ISR (cpgmacISR.DLL) decides if given interrupt is our interrupt.
//
//  Arguments:
//  
//      Adapter :: PCPSW3G_ADAPTER strucuture..
//
//
//  Return Value:
//
//      NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_FAILURE otherwise
//========================================================================

NDIS_STATUS
LoadISR( PCPSW3G_ADAPTER   pAdapter, DWORD SysIntr)
{
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    
    DEBUGMSG(DBG_FUNC, (L"  ->LoadISR\r\n"));
    
    {

        pAdapter->hISR = LoadIntChainHandler(
                            TEXT("cpgmacisr.dll"),
                            TEXT("ISRHandler"),
                            (BYTE)pAdapter->Cfg.InterruptVector);

        if (pAdapter->hISR == NULL)
        {
            DEBUGMSG(DBG_WARN, (L"  ->LoadIntChainHandler failed\r\n"));

            Status = NDIS_STATUS_FAILURE;
            goto Exit;
        }  
        DEBUGMSG(DBG_INFO, (L"  ->LoadIntChainHandler succeeded!hIST=%d \r\n", pAdapter->hISR));
        
        DEBUGMSG(DBG_INFO, (L"  SysIntr=%d\r\n", SysIntr));

        pAdapter->IsrInfo.RxIntPortAddr = (UINT32)&(pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked);
        pAdapter->IsrInfo.TxIntPortAddr = (UINT32)&pAdapter->pCpsw3gRegsBase->Tx_IntStat_Masked;
        pAdapter->IsrInfo.DMAIntAddr    = (UINT32)&pAdapter->pCpsw3gRegsBase->DMA_IntStat_Masked;
        pAdapter->IsrInfo.Cpdma_Vector  = (UINT32)&pAdapter->pCpsw3gRegsBase->CPDMA_In_Vector;

        pAdapter->IsrInfo.RxIntMask  = 0x1<<ActiveCpgmac | 0x1<<(ActiveCpgmac+2);
        pAdapter->IsrInfo.TxIntMask  = 0x1 << ActiveCpgmac;
        pAdapter->IsrInfo.DMAIntMask = 0x3;

        pAdapter->IsrInfo.SysIntr = SysIntr; 

        ASSERT(pAdapter->IsrInfo.RxIntPortAddr);
        ASSERT(pAdapter->IsrInfo.TxIntPortAddr);
        ASSERT(pAdapter->IsrInfo.DMAIntAddr);
        
        if (!KernelLibIoControl(
                pAdapter->hISR,
                IOCTL_CPGMACISR_INFO,
                &(pAdapter->IsrInfo),
                sizeof(CPGMAC_ISR_INFO),
                NULL,
                0x00,
                NULL))
        {  
            DEBUGMSG(DBG_WARN, (L"CPGMAC:LoadISR - Failed KernelLibIoControl\r\n"));
            goto Exit;
        }   
        DEBUGMSG(DBG_WARN, (L"  CPGMAC:LoadISR - KernelLibIoControl Succeeded\r\n"));
        
        Status = NDIS_STATUS_SUCCESS;
    }
    
Exit:
    return Status;

}   //  LoadISR();



//=====================================================================
//  UnloadISR()
//
//  Routine Description:
//
//      Unload the ISR loaded via LoadISR()
//       
//  Arguments:
//  
//      Adapter :: ADAPTER strucuture..
//
//
//  Return Value:
//
//      NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_FAILURE otherwise
//======================================================================

NDIS_STATUS
UnloadISR( PCPSW3G_ADAPTER   pAdapter)
{
    //
    //  If the ISR has been loaded, unload it.    
    //

    if (pAdapter->hISR)
    {
        FreeIntChainHandler(pAdapter->hISR);
        pAdapter->hISR = NULL;
    }

    return NDIS_STATUS_SUCCESS;

}   //  UnloadISR()




/*******************************************************************
Function : Cpsw3g_MiniportInitialize
Description: NDIS Calls this function once for each NIC managed by the miniport 
                  driver.
                  
Arguments:

    OpenErrorStatus -- Error status when return value is an error.
    SelectedMediumIndex -- the index of the medium miniport driver selects.
    MediumArray -- An array of NdisMediumXxx. the miniport driver must choose 
                            the medium it supports pr perfers and must return the index 
                            of that medium.
    MediumArraySize -- Specifies the number of elements of MediumArray.
    MiniportAdapterHandle -- Specifies a NDIS handle that NDIS uses to refer to the
                                          miniport driver. 
    WrapperConfigurationContext -- Specifies a configuration handle that identifies
                             the registry key containing NIC-specific information associated
                             with this miniport driver.
     
Return Value: 
    NDIS_STATUS
    
********************************************************************/
NDIS_STATUS
Cpsw3g_MiniportInitialize(
    PNDIS_STATUS    OpenErrorStatus,
    PUINT           SelectedMediumIndex,
    PNDIS_MEDIUM    MediumArray,
    UINT            MediumArraySize,
    NDIS_HANDLE     MiniportAdapterHandle,
    NDIS_HANDLE     WrapperConfigurationContext
    )
{
    NDIS_STATUS       Status;
    PCPSW3G_ADAPTER   pAdapter = NULL;
    UINT              Index;
    UINT32            stage=0;

    UNREFERENCED_PARAMETER(OpenErrorStatus);

    DEBUGMSG (DBG_FUNC,(L"  ->Cpsw3g_MiniportInitialize\r\n"));

    /* Scan for the Supported Media Type */
    for (Index = 0; Index < MediumArraySize; ++Index)
    {
        if (MediumArray[Index] == NIC_MEDIA_TYPE)
        {
            break;
        }
    }

    if (Index == MediumArraySize)
    {
        DEBUGMSG(DBG_WARN, (L"Expected media (%x) is not in MediumArray.\r\n", NIC_MEDIA_TYPE));
        Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
        /* We dont have much to do in case of an unsuppported media type */
        goto ErrorExit;
    }

    *SelectedMediumIndex = Index;

    /* Allocate Cpsw3g Miniport Adapter structure */
    Status = Cpsw3g_NICAllocAdapterBlock(&pAdapter);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        /* Bail Out */    
        goto Exit;
    }

    pAdapter->AdapterHandle = MiniportAdapterHandle;
    pAdapter->ConfigurationContext = WrapperConfigurationContext;

    /* Initialise Hardware status */
    pAdapter->HwStatus = NdisHardwareStatusNotReady;

    Status = Cpsw3g_MapAdapterRegs(pAdapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        /* Bail Out */    
        goto ErrorExit;
    }
    stage = 1;

    /* Reading all relevant information from adapter
     * etc. and filling it in to our adapter structure
     */
    Status = NICReadAdapterInfo(
                 pAdapter,
                 WrapperConfigurationContext);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        /* Bail Out */    
        goto ErrorExit;
    }

    /* 
       NdisMSetAttributes informs the NDIS library about significant 
       features of the caller's NIC during initialization.
    */
    NdisMSetAttributes (
         MiniportAdapterHandle,
         (NDIS_HANDLE) pAdapter,
         TRUE,                    /* True since ours is Bus Master */
         NdisInterfaceInternal);  /* We have internal controller */

    if((pAdapter->KITL_enable = pAdapter->Cfg.KITL_enable) == TRUE)
    {
        RETAILMSG(TRUE, (L"  ->Cpsw3g_MiniportInitialize(),  KITL is enabled!!!\r\n"));
    }

    /* Disable interrupts here which is as soon as possible */
    Cpsw3g_MiniportDisableInterrupt(pAdapter);

    Status = Cpsw3g_MiniportRegisterInterrupts(pAdapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        RETAILMSG(TRUE,  (L"NdisMRegisterInterrupt failed: status=%d\n", Status));
        /* Bail Out */    
        goto ErrorExit;
    }
    stage = 2;

    pAdapter->HwStatus = NdisHardwareStatusInitializing;

    pAdapter->Events =  0;
    
    /* Special handling for KITL mode */
    if (pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL)
        ActiveCpgmac ++;

    /* Init the hardware and set up everything */
    Status = Cpsw3g_InitializeAdapter(pAdapter);
    if ( (Status != NDIS_STATUS_SUCCESS) )
    {
        /* Bail Out */    
        goto ErrorExit;
    }
    stage = 3;

    Cpsw3g_SPF_init(pAdapter);

    /* Test our adapter hardware */
    Status = Cpsw3g_SelfCheck(pAdapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        /* Bail Out */    
        goto ErrorExit;
    }

   /* Test is successful , make a status transition */
    pAdapter->HwStatus = NdisHardwareStatusReady;

ErrorExit:    

    /* We should be reporting the specific error / return
    * conditions to the wrapper layer.
    */
    if (Status == NDIS_STATUS_UNSUPPORTED_MEDIA ||
         Status == NDIS_STATUS_FAILURE )
    {
        /* Free allocated memory and resources held */
        Cpsw3g_FreeAdapter(pAdapter, stage);
    }

Exit:
    DEBUGMSG (DBG_FUNC,(L"  <-Cpsw3g_MiniportInitialize()\r\n"));

    return Status;
}


