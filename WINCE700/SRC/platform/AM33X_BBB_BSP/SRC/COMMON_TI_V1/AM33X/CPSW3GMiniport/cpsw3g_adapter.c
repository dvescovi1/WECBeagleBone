//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "am33x.h"
#include "am33x_config.h"
#include "am33x_oal_prcm.h"

#include "Am33xCpsw3gRegs.h"
#include "cpsw3g_miniport.h"
#include "cpsw3g_cfg.h"
#include "cpsw3g_proto.h"

#include "oal_clock.h"

   #pragma data_seg(".MYSEC")
   int ActiveCpgmac = 0;
PNDIS_MINIPORT_TIMER ale_timer=0;
   #pragma data_seg()

  #pragma comment(linker, "/SECTION:.MYSEC,RWS")

//! \brief Global  64 Bit Physical Address initialised to all F's
NDIS_PHYSICAL_ADDRESS g_HighestAcceptedMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

#define CPSW3G_MAX_NUM_STATIC_ENTRY     4
#define CPSW3G_MAX_NUM_VLAN_ENTRY       8

/* !!! Warning:
   This workaround is for avoiding using dma channel 1 in Subarctic
   in which case NO interrupt is received from INTC.  Only channel 0
   receives interrupts from INTC
*/
#define POLAR_1_0

typedef struct __CPSW3G_ALE_STATIC_ENTRY__
{
    UINT8 valid;
    UINT8 mac[ETH_LENGTH_OF_ADDRESS];
    UINT8 fwd_port;
    UINT8 vlan_id;
    
}CPSW3G_ALE_STATIC_ENTRY;

typedef struct __CPSW3G_ALE_VLAN_ENTRY__
{
    UINT32 VID;
    UINT8 MemPort;
    UINT8 UntagPort;
    UINT8 RegMcastFloodMask;
    UINT8 UnregMcastFloodMask;
    UINT8 valid;    
    
}CPSW3G_ALE_VLAN_ENTRY;

#define CFG_StaticEntryField(field)     ((UINT32)FIELD_OFFSET(CPSW3G_ALE_STATIC_ENTRY, field)) 
#define CFG_StaticEntryFieldSize(field) sizeof(((CPSW3G_ALE_STATIC_ENTRY *)0)->field) 

CPSW3G_REG_ALE_CFG Cpsw3g_Ale_staticTbl[]=
{
    {NDIS_STRING_CONST("ForwardingPort"),   
         CFG_StaticEntryField(fwd_port), CFG_StaticEntryFieldSize(fwd_port)},

    {NDIS_STRING_CONST("VID"),   
         CFG_StaticEntryField(vlan_id), CFG_StaticEntryFieldSize(vlan_id)},
        
};

#define CPSW3G_NUM_STATIC_PARAMS       (sizeof (Cpsw3g_Ale_staticTbl) / sizeof(CPSW3G_REG_ALE_CFG))

#define CFG_VlanEntryField(field)      ((UINT32)FIELD_OFFSET(CPSW3G_ALE_VLAN_ENTRY, field)) 
#define CFG_VlanEntryFieldSize(field)  sizeof(((CPSW3G_ALE_VLAN_ENTRY *)0)->field) 

CPSW3G_REG_VLAN_CFG Cpsw3g_Ale_vlanTbl[]=
{
    {NDIS_STRING_CONST("VID"),  
         CFG_VlanEntryField(VID),  CFG_VlanEntryFieldSize(VID), 0, 4095},

    {NDIS_STRING_CONST("memberPort"), 
         CFG_VlanEntryField(MemPort), CFG_VlanEntryFieldSize(MemPort), 0, 7},

    {NDIS_STRING_CONST("UntagPort"),  
         CFG_VlanEntryField(UntagPort), CFG_VlanEntryFieldSize(UntagPort), 0, 7},

    {NDIS_STRING_CONST("RegMcastFloodMask"),  
         CFG_VlanEntryField(RegMcastFloodMask), CFG_VlanEntryFieldSize(RegMcastFloodMask), 0, 7},

    {NDIS_STRING_CONST("UnregMcastFloodMask"),  
         CFG_VlanEntryField(UnregMcastFloodMask), CFG_VlanEntryFieldSize(UnregMcastFloodMask), 0, 7},
};

#define CPSW3G_NUM_VLAN_PARAMS (sizeof (Cpsw3g_Ale_vlanTbl) / sizeof(CPSW3G_REG_VLAN_CFG))

CPSW3G_ALE_STATIC_ENTRY Ale_entry[CPSW3G_MAX_NUM_STATIC_ENTRY];
CPSW3G_ALE_VLAN_ENTRY Vlan_entry[CPSW3G_MAX_NUM_VLAN_ENTRY];

void Cpsw_add_vlan_ALE_entry(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 index=0;
    
    /* Add Vlan entry */
    for(index=0; index<CPSW3G_MAX_NUM_VLAN_ENTRY; index++)
    {
        if(Vlan_entry[index].valid != TRUE) continue;

        /* Added entry in ALE hardware */
        cpsw_ale_add_vlan_entry(
                pAdapter, Vlan_entry[index].VID, 
                Vlan_entry[index].MemPort, 
                Vlan_entry[index].UntagPort, 
                Vlan_entry[index].RegMcastFloodMask, 
                Vlan_entry[index].UnregMcastFloodMask
        );

        RETAILMSG(TRUE, (L"Cpsw_add_vlan_ALE_entry:  vlan entry %d, member: %x, untag %x is added.\r\n", 
                Vlan_entry[index].VID, Vlan_entry[index].MemPort, Vlan_entry[index].UntagPort,
                Vlan_entry[index].RegMcastFloodMask, Vlan_entry[index].UnregMcastFloodMask));

    }
}

void Cpsw_read_vlan_ALE_entry(PCPSW3G_ADAPTER pAdapter)
{
    NDIS_STATUS                         Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE                         ConfigurationHandle, SubKeyHandle;
    PNDIS_CONFIGURATION_PARAMETER       pReturnedValue;    
    NDIS_STRING nszKeyword  = {0};    
    CPSW3G_REG_VLAN_CFG *pRegEntry;
    wchar_t wstr[16];
    UINT32 index=0, i, value;
    PUCHAR param;

    /* Open registry */
    NdisOpenConfiguration(
        &Status,
        &ConfigurationHandle,
        pAdapter->ConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"cpsw_add_vlan_ALE_entry: Open Registry failed\n"));
        return ;
    }

    /* Read Vlan entry */
    for(index=0; index<CPSW3G_MAX_NUM_VLAN_ENTRY; index++)
    {
        Vlan_entry[index].valid = FALSE;

        /* Open port registry */
        wsprintf(wstr, L"%s%d", L"VlanTable", index);
        DEBUGMSG(DBG_INFO, (L"Cpsw_add_vlan_ALE_entry: wstr %s ", wstr ));
        
        NdisInitUnicodeString (&nszKeyword, wstr);
        DEBUGMSG(DBG_INFO, (L"Cpsw_add_vlan_ALE_entry: subkey %s ", nszKeyword.Buffer ));
        
        NdisOpenConfigurationKeyByName(
                &Status,
                ConfigurationHandle,
                &nszKeyword,
                &SubKeyHandle
        );
        
        // If the parameter was not present, break
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_WARN, (L"Cpsw_add_vlan_ALE_entry: entry %d does not exist ", index));
            continue;
        }
        // read all the registry values 
        for (i = 0, pRegEntry = Cpsw3g_Ale_vlanTbl; i < CPSW3G_NUM_VLAN_PARAMS; i++, pRegEntry++)
        {
            param = (PUCHAR)&Vlan_entry[index] + pRegEntry->Cfg_field;
            DEBUGMSG(DBG_INFO, (L"Cpsw_add_vlan_ALE_entry: name: %s, ", pRegEntry->keyword.Buffer));

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
                    DEBUGMSG(DBG_WARN, 
                        (L"Cpsw_add_vlan_ALE_entry: Value out of range: %d, ", 
                        pReturnedValue->ParameterData.IntegerData));

                    DEBUGMSG(DBG_WARN, (L"entry dropped\r\n"));
                    break;
                }
                else
                {
                    value = pReturnedValue->ParameterData.IntegerData;
                }
                DEBUGMSG(DBG_INFO, (L"Cpsw_add_vlan_ALE_entry: value = 0x%x.\r\n", value));

            }
            else
            {
                DEBUGMSG(DBG_WARN, 
                    (L"Cpsw_add_vlan_ALE_entry: read registry failed for : %s, entry dropped\r\n", 
                    pRegEntry->keyword.Buffer));

                break;
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
                    RETAILMSG(TRUE, (L"Bogus field size %d\n", pRegEntry->FieldSize));
                    break;
            }
        }
        NdisCloseConfiguration(SubKeyHandle);
        DEBUGMSG(DBG_INFO,  (L"Cpsw_add_vlan_ALE_entry: read vlan table %d.\r\n", index));

        /* Added entry in ALE hardware */
        Vlan_entry[index].valid = TRUE;
        
    }
    DEBUGMSG(DBG_INFO,  (L"Cpsw_add_vlan_ALE_entry: finished reading all Vlan table %d.\r\n", index));

    NdisCloseConfiguration(ConfigurationHandle);

}


void cpsw3g_add_dynamic_vlan(PCPSW3G_ADAPTER pAdapter)
{

    /* set port VLAN cfg */
    pAdapter->pCpsw3gRegsBase->P0_Port_VLAN &= ~CPSW3G_PORTVLAN_VID_MASK;
    pAdapter->pCpsw3gRegsBase->P0_Port_VLAN |= 
        (pAdapter->Cfg.dmaCfg.hw_config.portVID << CPSW3G_PORTVLAN_VID_SHIFT);
    
    cpsw_ale_add_vlan_ucast_entry(
        pAdapter, pAdapter->Cfg.dmaCfg.hw_config.portVID, 
        pAdapter->MACAddress, 2);   

    if(Vlan_entry[0].valid == TRUE){ 

        cpsw_ale_add_vlan_entry(
            pAdapter, pAdapter->Cfg.dmaCfg.hw_config.portVID, 
            Vlan_entry[0].MemPort, 
            Vlan_entry[0].UntagPort, 
            Vlan_entry[0].RegMcastFloodMask, 
            Vlan_entry[0].UnregMcastFloodMask
        );
    }
    else{

        cpsw_ale_add_vlan_entry(
            pAdapter, pAdapter->Cfg.dmaCfg.hw_config.portVID, 
            CPSW3G_DEFAULT_UNKNOWN_MEMBER_LIST, 
            CPSW3G_DEFAULT_UNKNOWN_UNTAG_EGRESS, 
            CPSW3G_DEFAULT_UNKNOWN_REG_MCAST_MASK, 
            CPSW3G_DEFAULT_UNKNOWN_MCAST_MASK
        );
    }
}

void Cpsw_add_static_ALE_entry(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 index=0;

    for(index=0; index < CPSW3G_MAX_NUM_STATIC_ENTRY; index++)
    {
        if(Ale_entry[index].valid != TRUE) continue;

        /* Added entry in ALE hardware */
        cpsw_ale_add_vlan_ucast_entry(
            pAdapter, Ale_entry[index].vlan_id, 
            Ale_entry[index].mac,  Ale_entry[index].fwd_port);   
        
        RETAILMSG(TRUE,  
            (L"Added static entry- %02x-%02x-%02x-%02x-%02x-%02x, vlan: %d fwd port:%d\r\n", 
            Ale_entry[index].mac[0], Ale_entry[index].mac[1], Ale_entry[index].mac[2],
            Ale_entry[index].mac[3], Ale_entry[index].mac[4], Ale_entry[index].mac[5],
            Ale_entry[index].vlan_id, Ale_entry[index].fwd_port));
    }
}

void    Cpsw_read_static_ALE_entry(PCPSW3G_ADAPTER pAdapter)
{
    NDIS_STATUS                         Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE                         ConfigurationHandle, SubKeyHandle;
    PNDIS_CONFIGURATION_PARAMETER       pReturnedValue;    
    CPSW3G_REG_ALE_CFG *pRegEntry;
    //CPSW3G_ALE_STATIC_ENTRY Ale_entry;
    NDIS_STRING nszKeyword  = {0};
    NDIS_STRING temp;
    wchar_t wstr[16];
    UINT32 index=0, i, value;
    PUCHAR param;

    /* Open registry */
    NdisOpenConfiguration(
        &Status,
        &ConfigurationHandle,
        pAdapter->ConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"cpsw_add_static_ALE_entry: Open Registry failed\n"));
        return ;
    }
    /* Read Static entry */
    for(index=0; index < CPSW3G_MAX_NUM_STATIC_ENTRY; index++)
    {
        /* Open Static table registry */
        wsprintf(wstr, L"%s%d", L"StaticTable", index);
        
        NdisInitUnicodeString (&nszKeyword, wstr);
        
        NdisOpenConfigurationKeyByName(
                &Status,
                ConfigurationHandle,
                &nszKeyword,
                &SubKeyHandle
        );
        // If the parameter was not present, break
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_WARN,  (L"cpsw_add_static_ALE_entry: reading entry %d failed\r\n ", index));
            continue;
        }
        NdisZeroMemory(&Ale_entry[index], sizeof (Ale_entry));
        NdisInitUnicodeString (&nszKeyword, L"NetworkAddress");

        // Get the configuration value for a specific parameter.  
        NdisReadConfiguration(
            &Status,
            &pReturnedValue,
            SubKeyHandle,
            &nszKeyword,
            NdisParameterString);

        // If the parameter was present, then check its value for validity.
        if (Status == NDIS_STATUS_SUCCESS)
        {
            ASSERT(pReturnedValue->ParameterType == NdisParameterString);

            if (pReturnedValue->ParameterData.StringData.Length !=0)
            {
                temp.MaximumLength = 
                    pReturnedValue->ParameterData.StringData.Length + sizeof(WCHAR);

                swscanf(
                    pReturnedValue->ParameterData.StringData.Buffer, 
                    L"%x-%x-%x-%x-%x-%x", 
                    &Ale_entry[index].mac[0], &Ale_entry[index].mac[1], 
                    &Ale_entry[index].mac[2], &Ale_entry[index].mac[3], 
                    &Ale_entry[index].mac[4], &Ale_entry[index].mac[5]
                );

                RETAILMSG(TRUE,  
                    (L"NetworkAddress  - %02x-%02x-%02x-%02x-%02x-%02x\n", 
                    Ale_entry[index].mac[0], Ale_entry[index].mac[1], 
                    Ale_entry[index].mac[2], Ale_entry[index].mac[3], 
                    Ale_entry[index].mac[4], Ale_entry[index].mac[5])
                );
            }
        }
        else
            continue;

        // read all the registry values 
        for (i = 0, pRegEntry = Cpsw3g_Ale_staticTbl; i < CPSW3G_NUM_STATIC_PARAMS; i++, pRegEntry++)
        {
            param = (PUCHAR)&Ale_entry[index] + pRegEntry->Cfg_field;
            
            DEBUGMSG(DBG_INFO, (L"cpsw_add_static_ALE_entry: name: %s, ", pRegEntry->keyword.Buffer));

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
                value = pReturnedValue->ParameterData.IntegerData;
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
                        DEBUGMSG(DBG_INFO, (L"Bogus field size %d\n", pRegEntry->FieldSize));
                        break;
                }
                        
                DEBUGMSG(DBG_INFO,  (L"Cpsw3g_ReadRegParameters: name: value = 0x%x.\r\n", value));

            }
            else
            {
                DEBUGMSG(DBG_WARN,  (L"Read registry keyword %s failed, static entry is dropped\r\n", 
                            pRegEntry->keyword.Buffer));
                break;
            }
        }

        if(Ale_entry[index].fwd_port >2 || Ale_entry[index].fwd_port <0 )
        {
            DEBUGMSG(DBG_WARN,  
                (L"Invalid forwarding port %d, static entry is dropped\r\n",  
                Ale_entry[index].fwd_port));
        }
        else if (Ale_entry[index].vlan_id > 4095 || Ale_entry[index].vlan_id < 1)
        {
            DEBUGMSG(DBG_WARN,  
                (L"Invalid vlan ID %d, static entry is dropped\r\n",  
                Ale_entry[index].vlan_id));
        }
        else    
        {
            /* Added entry in ALE hardware */
            Ale_entry[index].valid = TRUE;
        }
        NdisCloseConfiguration(SubKeyHandle);
    }

    NdisCloseConfiguration(ConfigurationHandle);
}

NDIS_STATUS Cpsw3g_MapAdapterRegs(PCPSW3G_ADAPTER   pAdapter)
{
    NDIS_STATUS Status;
    DWORD       Cpsw3gCtlRegsBase;
    DWORD       Cpsw3gMdioBase;
    DWORD       Cpsw3gSsBase;
//    DWORD Cpsw3gVtpBase;
	
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;

    NdisSetPhysicalAddressLow (PhysicalAddress, CPSW_3G_BASE);
    NdisSetPhysicalAddressHigh(PhysicalAddress, 0);

    /* Mapping the CPSW3G controller registers */
    Status = NdisMMapIoSpace((VOID*)&Cpsw3gCtlRegsBase,
                    pAdapter->AdapterHandle,
                    PhysicalAddress,
                    CPSW3G_MEMORY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\n"));
        return Status;
    }

    pAdapter->pCpsw3gRegsBase  = (PCPSW3G_REGS) Cpsw3gCtlRegsBase;


    NdisSetPhysicalAddressLow (PhysicalAddress, CPMDIO_BASE);
    NdisSetPhysicalAddressHigh (PhysicalAddress, 0);
    /* Mapping the MDIO controller registers */
    Status = NdisMMapIoSpace((VOID*)&Cpsw3gMdioBase,
                    pAdapter->AdapterHandle,
                    PhysicalAddress,
                    MDIO_MEMORY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\n"));
        return Status;
    }

    pAdapter->pMdioRegsBase  = (PCPSW3G_MDIO_REGS)Cpsw3gMdioBase;


    NdisSetPhysicalAddressLow (PhysicalAddress, CPSW_SS_BASE);
    NdisSetPhysicalAddressHigh (PhysicalAddress, 0);
    /* Mapping the Subsystem registers */
    Status = NdisMMapIoSpace((VOID*)&Cpsw3gSsBase,
                    pAdapter->AdapterHandle,
                    PhysicalAddress,
                    SS_MEMORY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace CPSW_SS_BASE 0x%08X failed\n", CPSW_SS_BASE));
        return Status;
    }

    pAdapter->pSsRegsBase  = (PCPSW_SS_REGS)Cpsw3gSsBase;

// +++FIXME: not for Centaurus, Subarctic
#if 0  
    NdisSetPhysicalAddressLow (PhysicalAddress, CPSW_VTP_CTRL0_BASE);
    NdisSetPhysicalAddressHigh (PhysicalAddress, 0);
    /* Mapping the MDIO controller registers */
    Status = NdisMMapIoSpace((VOID*)&Cpsw3gVtpBase,
                    pAdapter->AdapterHandle,
                    PhysicalAddress,
                    MDIO_MEMORY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\n"));
        return Status;
    }

    pAdapter->pVtp0CtrlBase  = (PCPSW3G_VTP_REGS)Cpsw3gVtpBase;

    NdisSetPhysicalAddressLow (PhysicalAddress, CPSW_VTP_CTRL1_BASE);
    NdisSetPhysicalAddressHigh (PhysicalAddress, 0);
    /* Mapping the MDIO controller registers */
    Status = NdisMMapIoSpace((VOID*)&Cpsw3gVtpBase,
                    pAdapter->AdapterHandle,
                    PhysicalAddress,
                    MDIO_MEMORY_SIZE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisMMapIoSpace failed\n"));
        return Status;
    }

    pAdapter->pVtp1CtrlBase  = (PCPSW3G_VTP_REGS)Cpsw3gVtpBase;
#endif

    return Status;
}

static __inline BOOL cpsw_soft_reset(volatile UINT32* reg) 
{
    UINT32 timeout=0;
    *reg |= CPSW3G_SOFT_RESET_BIT;

    /* Wait for 100 ms then time out */
    while ( ((*reg & CPSW3G_SOFT_RESET_BIT) != 0) &&
                (timeout < 1000))  // +++FIXME
    {
        timeout++;
        /* Delay for 100us */
        NdisMSleep(100); 
    }

    if((*reg & CPSW3G_SOFT_RESET_BIT) != 0)
    {
        return FALSE;
    }
    return TRUE;
}

//========================================================================
//!  \fn BOOL Cpsw3g_ModStateChange(UINT32  ModState)
//!  \brief Power on/off CPSW3G modules.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================
BOOL Cpsw3g_ModStateChange(UINT32  ModState)
{
    BOOL rc = TRUE;

    DEBUGMSG(TRUE, (L"+Cpsw3g_ModStateChange\r\n"));
 
    switch (ModState)
    {   
    case MOD_SYNCRST:
//        SocResetEmac();
        rc = EnableDeviceClocks(AM_DEVICE_CPGMAC0, TRUE);
        break;
    case MOD_ENABLE:
        rc = EnableDeviceClocks(AM_DEVICE_CPGMAC0, TRUE);
        break;
    case MOD_DISABLE:
        rc = EnableDeviceClocks(AM_DEVICE_CPGMAC0, FALSE);
        break;
    }

    DEBUGMSG(TRUE, ( L"-Cpsw3g_ModStateChange\r\n" ));

    return (rc);
}


//========================================================================
//!  \fn BOOL   Cpsw3g_Ethss_power_on(void)
//!  \brief Power on CPSW3G modules.
//!  \param None
//!  \return True/False
//========================================================================
BOOL   Cpsw3g_Ethss_power_on(void)
{
    if(FALSE == Cpsw3g_ModStateChange(MOD_DISABLE))
    {
        DEBUGMSG(ZONE_ERRORS, (L"Cpsw3g_Ethss_power_on: Disable ETHSS modules failed \n"));
        return FALSE;
    }
    
    if(FALSE == Cpsw3g_ModStateChange(MOD_ENABLE))
    {
        DEBUGMSG(ZONE_ERRORS, (L"Cpsw3g_Ethss_power_on: Enable ETHSS modules failed \n"));
        return FALSE;
    }
    return TRUE;
}

//========================================================================
//!  \fn BOOL   Cpsw3g_Ethss_power_off(void)
//!  \brief Power off CPSW3G modules.
//!  \param None
//!  \return True/False
//========================================================================
BOOL   Cpsw3g_Ethss_power_off(void)
{
    if(FALSE == Cpsw3g_ModStateChange(MOD_DISABLE))
    {
        DEBUGMSG(ZONE_ERRORS, (L"Cpsw3g_Ethss_power_off: Disable ETHSS modules failed \n"));
        return FALSE;
    }
    return TRUE;
}


// +++FIXME:  not for centaurus
#if 0
//========================================================================
//!  \fn void  Cpsw3g_rgmii_dump(PCPSW3G_ADAPTER   pAdapter, PCPSW3G_VTP_REGS   pVtp)
//!  \brief RGMII register dump.
//!  \param PCPSW3G_ADAPTER pAdapter, PCPSW3G_VTP_REGS   pVtp
//!  \return None.
//========================================================================
void  Cpsw3g_rgmii_dump(PCPSW3G_ADAPTER   pAdapter, PCPSW3G_VTP_REGS   pVtp)
{

#ifdef DEBUG
    DEBUGMSG(DBG_INFO,  (L"Cpsw3g_rgmii_dump for port %d.\r\n",
                                   ((pVtp == (PCPSW3G_VTP_REGS)pAdapter->pVtp0CtrlBase) ? 0 : 1)));

    DEBUGMSG(DBG_INFO, (L"    pid:%x \r\n", pVtp->pid));
    DEBUGMSG(DBG_INFO, (L"    mode:%x \r\n", pVtp->mode));
    DEBUGMSG(DBG_INFO, (L"    wdt:%x \r\n", pVtp->wdt));
    DEBUGMSG(DBG_INFO, (L"    np:%x \r\n", pVtp->np));	
    DEBUGMSG(DBG_INFO, (L"    control:%x \r\n", pVtp->ctrl));
    DEBUGMSG(DBG_INFO, (L"    start:%x \r\n", pVtp->start));
#else
    UNREFERENCED_PARAMETER(pAdapter);
    UNREFERENCED_PARAMETER(pVtp);
#endif
}
#endif

//========================================================================
//!  \fn void  Cpsw3g_rgmii_init(PCPSW3G_ADAPTER   pAdapter, BOOL reset)
//!  \brief RGMII register Init.
//!  \param PCPSW3G_ADAPTER pAdapter, BOOL reset
//!  \return None.
//========================================================================
void  Cpsw3g_rgmii_init(PCPSW3G_ADAPTER   pAdapter, BOOL reset)
{
// +++FIXME:  not for centaurus
#if 0
    PCPSW3G_VTP_REGS vtp;

    if(reset)
    {
        /* Power on vtp and RGMII gasket */
        if (!PSCSetModuleState(MOD_ETHSS_RGMII, MOD_ENABLE))
        {
            DEBUGMSG (TRUE, (TEXT("PSCSetModuleState For MDIO Failed \r\n")));
            return ;
        }
    }

    vtp = (PCPSW3G_VTP_REGS) pAdapter->pVtp1CtrlBase;

    vtp->mode &= ~(0x3);
    vtp->mode |= VTP_SINGLE_MODE;

    /* clear oe_n */
    vtp->ctrl &= ~(VTP_GZ_ALL);

    /* start single mode */
    vtp->start |= VTP_SINGLE_MODE_START;	

    /* setup VTP  address */
    vtp = (PCPSW3G_VTP_REGS) pAdapter->pVtp0CtrlBase;

    vtp->mode &= ~(0x3);
    vtp->mode |= VTP_SINGLE_MODE;

    /* clear oe_n */
    vtp->ctrl &= ~(VTP_GZ_ALL);

    /* start single mode */
    vtp->start |= VTP_SINGLE_MODE_START;	

    Cpsw3g_rgmii_dump(pAdapter, (PCPSW3G_VTP_REGS) pAdapter->pVtp0CtrlBase);	
    Cpsw3g_rgmii_dump(pAdapter, (PCPSW3G_VTP_REGS) pAdapter->pVtp1CtrlBase);	
#else
    return;
#endif
}

//========================================================================
//!  \fn NDIS_STATUS    Cpsw3g_Init_Tx_Buffer_memory(PCPSW3G_ADAPTER pAdapter)
//!  \brief Transmit buffer init function
//!  \param PCPSW3G_ADAPTER pAdapter
//!  \return None.
//========================================================================
NDIS_STATUS    Cpsw3g_Init_Tx_Buffer_memory(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 len;
    NDIS_STATUS     Status=NDIS_STATUS_SUCCESS;

    len = ((sizeof(CPSW3G_DESC)+(CPSW3G_BD_ALIGN-1)) & ~(CPSW3G_BD_ALIGN-1)) * CPSW3G_MAX_TX_BUFFERS;
    
    /* Tx buffer */
    NdisMAllocateSharedMemory(pAdapter->AdapterHandle, 
        CPSW3G_MAX_TX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
        TRUE, //cached
        (PVOID)&pAdapter->TxBufBase, 
        &pAdapter->TxBufBasePa);

    if (!pAdapter->TxBufBase)
    {
        RETAILMSG(TRUE, (L"Failed to allocate a TX buffer\n"));
        Status = NDIS_STATUS_RESOURCES;
        goto Fail;
    }
    else
    {
        DEBUGMSG(DBG_INFO, (L"init_memory for TX Buffer,  va=%x, pa=%x\r\n", 
            pAdapter->TxBufBase, pAdapter->TxBufBasePa)); 
    }
    
    /* Tx buffer desc */
    NdisMAllocateSharedMemory(pAdapter->AdapterHandle, 
        len,
        FALSE, 
        (PVOID)&pAdapter->TxBDBase, 
        &pAdapter->TxBDBasePa);

    if (!pAdapter->TxBDBase)
    {
            RETAILMSG(TRUE, (L"Failed to allocate a TX BD buffer\n"));
            Status = NDIS_STATUS_RESOURCES;
            goto Fail;
    }
    else
    {

        DEBUGMSG(DBG_INFO, (L"init_memory for TX BD,  va=%x, pa=%x\r\n", 
            pAdapter->TxBDBase, pAdapter->TxBDBasePa)); 
    }

    return Status;
    
Fail:

    /* Free Allocated buffer */
    if(NULL != (VOID *)pAdapter->TxBufBase)
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                              CPSW3G_MAX_TX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
                                              TRUE, //cached
                                              (PVOID)pAdapter->TxBufBase, 
                                              pAdapter->TxBufBasePa);
    }
    if(NULL != (VOID *)pAdapter->TxBDBase)
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                              len,
                                              FALSE, 
                                              (PVOID)pAdapter->TxBDBase, 
                                              pAdapter->TxBDBasePa);    
    }
    return Status;

}

//========================================================================
//!  \fn NDIS_STATUS    Cpsw3g_Init_Rx_Buffer_memory(PCPSW3G_ADAPTER pAdapter, UINT32 channel)
//!  \brief Receive buffer init function
//!  \param PCPSW3G_ADAPTER pAdapter, UINT32 channel
//!  \return None.
//========================================================================
NDIS_STATUS    Cpsw3g_Init_Rx_Buffer_memory(PCPSW3G_ADAPTER pAdapter, UINT32 channel)
{
    UINT32 desc_len=0;
    NDIS_STATUS     Status=NDIS_STATUS_SUCCESS;

    /* Rx buffer */
    NdisMAllocateSharedMemory(pAdapter->AdapterHandle, 
        CPSW3G_MAX_RX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
        TRUE, //cached
        (PVOID)&pAdapter->RxBufsBase[channel], 
        &pAdapter->RxBufsBasePa[channel]);

    if (!pAdapter->RxBufsBase[channel])
    {
        RETAILMSG(TRUE, (L"Failed to allocate RX buffer\n"));
        Status = NDIS_STATUS_RESOURCES;
        goto Fail;
    }
    else
    {
        DEBUGMSG(DBG_INFO, (L"init_memory for RX buffer,  va=%x, pa=%x\r\n", 
            pAdapter->RxBufsBase[channel], pAdapter->RxBufsBasePa[channel])); 
    }

    /* Rx buffer desc */
    desc_len = ((sizeof(CPSW3G_DESC)+(CPSW3G_BD_ALIGN-1)) &
                     ~(CPSW3G_BD_ALIGN-1)) * 
                CPSW3G_MAX_RX_BUFFERS;

    NdisMAllocateSharedMemory(pAdapter->AdapterHandle, 
        desc_len,
        FALSE, //uncached
        (PVOID)&pAdapter->RxBDBase[channel], 
        &pAdapter->RxBDBasePa[channel]);

    if (!pAdapter->RxBufsBase[channel])
    {
        RETAILMSG(TRUE, (L"Failed to allocate RX BD buffer\n"));
        Status = NDIS_STATUS_RESOURCES;
        goto Fail;
    }
    else
    {
        DEBUGMSG(DBG_INFO, (L"init_memory for RX BD buffer,  va=%x, pa=%x\r\n", 
            pAdapter->RxBDBase[channel], pAdapter->RxBDBasePa[channel])); 
    }

    return Status;
    
Fail:

    /* Free Allocated buffer */
    if(NULL != (VOID *)pAdapter->RxBufsBase[channel])
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
            CPSW3G_MAX_RX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
            TRUE, //cached
            (PVOID)pAdapter->RxBufsBase[channel], 
            pAdapter->RxBufsBasePa[channel]);    
    }

    if(NULL != (VOID *)pAdapter->RxBDBase[channel])
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
            desc_len,
            FALSE, //uncached
            (PVOID)pAdapter->RxBDBase[channel], 
            pAdapter->RxBDBasePa[channel]);    
    }

    return Status;

}

//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_InitSend(PCPSW3G_ADAPTER  pAdapter)
//!  \brief Allocates and initialises Tx data structures.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
Cpsw3g_InitSend(PCPSW3G_ADAPTER pAdapter)
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    USHORT          Count;
    DWORD           TxBufDesBase ;
    DWORD           TxBufDesBasePa;
    DWORD           TxPhyBuf ;
    DWORD           TxPhyBufPa;
    PCPSW3G_TXPKT   pCurPkt;
    PCPSW3G_TXPKT   pNextPkt;
    PCPSW3G_TXBUF   pCurBuf;
    PCPSW3G_TXBUF   pNextBuf;

    DEBUGMSG(DBG_FUNC, (L"  -> Cpsw3g_InitSend, \r\n" ));

    if((VOID *)pAdapter->pBaseTxPkts == NULL)
    {
        /* Setting up Transmit Packets data structures */
        Status = NdisAllocateMemory((PVOID *)&pAdapter->pBaseTxPkts,
                     pAdapter->MaxPacketPerTransmit * sizeof(CPSW3G_TXPKT),
                     0,
                     g_HighestAcceptedMax);
    }
    else
    {
        RETAILMSG(TRUE, (L"pAdapter->pBaseTxPkts(%x) is allocated already.\r\n",
                                    pAdapter->pBaseTxPkts));
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
        return Status;
    }

    if((VOID *)pAdapter->pBaseTxBufs == NULL)
    {
        /* Setting up Transmit Buffers data structures */
        Status = NdisAllocateMemory((PVOID *)&pAdapter->pBaseTxBufs,
                     pAdapter->MaxTxMacBufDesc * sizeof(CPSW3G_TXBUF),
                     0,
                     g_HighestAcceptedMax);
    }
    else
    {
        RETAILMSG(TRUE, (L"pAdapter->pBaseTxBufs(%x) is allocated already.\r\n",
                                    pAdapter->pBaseTxBufs));
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR,(L" NdisAllocateMemory Unsucessful\r\n"));
        return Status;
    }

    NdisZeroMemory(pAdapter->pBaseTxPkts, pAdapter->MaxPacketPerTransmit * sizeof(CPSW3G_TXPKT));
    NdisZeroMemory(pAdapter->pBaseTxBufs, pAdapter->MaxTxMacBufDesc * sizeof(CPSW3G_TXBUF));

    pCurPkt = pNextPkt = pAdapter->pBaseTxPkts;

    QUEUE_INIT(&pAdapter->TxPktsInfoPool);

    for (Count = 0; Count < pAdapter->MaxPacketPerTransmit ; Count++)
    {
        pCurPkt = pNextPkt;

        /* Add to Free Transmit packets pool */
        QUEUE_INSERT(&pAdapter->TxPktsInfoPool,pCurPkt);

        /* Initialise per packet maintained Bufs List */
        QUEUE_INIT(&pCurPkt->BufsList);

        pNextPkt++;

        pCurPkt->pNext  = pNextPkt;
    }

    pCurPkt->pNext=0;

    Status = Cpsw3g_Init_Tx_Buffer_memory(pAdapter);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"Allocate RX buffer memory failed\n"));
        return Status;
    }

    ASSERT(pAdapter->TxBufBase);
    ASSERT(pAdapter->TxBDBase);

    NdisZeroMemory((PVOID)pAdapter->TxBufBase, (CPSW3G_MAX_TX_BUFFERS * CPSW3G_MAX_PKT_BUFFER_SIZE));
    NdisZeroMemory((PVOID)pAdapter->TxBDBase, (CPSW3G_MAX_TX_BUFFERS * sizeof(CPSW3G_DESC)));
    
    TxBufDesBase   = pAdapter->TxBDBase ;
    TxBufDesBasePa = NdisGetPhysicalAddressLow(pAdapter->TxBDBasePa);

    TxPhyBuf   = pAdapter->TxBufBase;
    TxPhyBufPa = NdisGetPhysicalAddressLow(pAdapter->TxBufBasePa);

    pNextBuf = pAdapter->pBaseTxBufs;

    QUEUE_INIT(&pAdapter->TxBufInfoPool);

    for (Count = 0; Count < pAdapter->MaxTxMacBufDesc ; Count++)
    {
        pCurBuf = pNextBuf;

        /* Assigning the buffer descriptors virtual and physical
        * addressses as well
        */
        pCurBuf->BufDes   = TxBufDesBase;
        pCurBuf->BufDesPa = TxBufDesBasePa;

        /* Assigning corresponding physical and logical addresses */

        pCurBuf->BufLogicalAddress    = TxPhyBuf;
        pCurBuf->BufPhysicalAddress   = TxPhyBufPa;

        /*DEBUGMSG(DBG_ERR, (L"InitSend::init BD buffer::count:%d ->CurBuf:%x, BufDes:%x, BufDesPa:%x\r\n",
                                             Count, pCurBuf,  pCurBuf->BufDes, pCurBuf->BufDesPa));*/

        TxBufDesBase   += sizeof(CPSW3G_DESC);
        TxBufDesBasePa += sizeof(CPSW3G_DESC);

        TxPhyBuf   += CPSW3G_MAX_PKT_BUFFER_SIZE;
        TxPhyBufPa += CPSW3G_MAX_PKT_BUFFER_SIZE;

        pNextBuf++;

        QUEUE_INSERT(&pAdapter->TxBufInfoPool, pCurBuf);
    }

    /* Initialise posted packets queue */
    QUEUE_INIT(&pAdapter->TxPostedPktPool);
    DEBUGMSG(DBG_FUNC, (L"<-- Cpsw3g_InitSend, \r\n" ));

    return Status;
}

//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_InitRecv(PEMAC_ADAPTER  pAdapter)
//!  \brief Allocates and initialises Rx data structures.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
Cpsw3g_InitRecv(PCPSW3G_ADAPTER  pAdapter, UINT32 channel )

{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    USHORT          Count;
    DWORD           RcvBufDesBase;
    DWORD           RcvBufDesBasePa;
    PCPSW3G_RXPKTS  pCurPkt;
    PCPSW3G_RXPKTS  pNextPkt;
    PCPSW3G_RXBUFS  pCurBuf;
    PCPSW3G_RXBUFS  pNextBuf;
    DWORD           RcvBufLogical;
    DWORD           RcvBufPhysical;
    PCPSW3G_DESC    pRxBD;

    /* Before calling this function, BD memory blocks are already pre-allocated */
    DEBUGMSG(DBG_FUNC, (L"---> Cpsw3g_InitRecv, \r\n" ));

    if((VOID *)pAdapter->pBaseRxPkts[channel] == NULL)
    {
        // Setting up Receive Packets data structures */
        Status = NdisAllocateMemory((PVOID *)&pAdapter->pBaseRxPkts[channel],
                     pAdapter->NumRxIndicatePkts * sizeof(CPSW3G_RXPKTS),
                     0,
                     g_HighestAcceptedMax);
    }
    else
        DEBUGMSG(DBG_INIT,  (L"pAdapter->pBaseRxPkts[%d] (%x) is allocated already.\r\n",
            channel, pAdapter->pBaseRxPkts[channel]));
        
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_INIT,  (L" NdisAllocateMemory for RxPkts Unsucessful\r\n"));
        return Status;
    }
    
    /* Setting up Receive Buffers  data structures */
    if((VOID *)pAdapter->pBaseRxBufs[channel] == NULL)
    {
        Status = NdisAllocateMemory((PVOID *)&pAdapter->pBaseRxBufs[channel],
                     pAdapter->NumRxMacBufDesc * sizeof(CPSW3G_RXBUFS),
                     0,
                     g_HighestAcceptedMax);
    }
    else
        DEBUGMSG(DBG_INIT, (L"pAdapter->pBaseRxBufs[%d] (%x) is allocated already.\r\n",
            channel, pAdapter->pBaseRxBufs[channel]));
                            
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_INIT, (L" NdisAllocateMemory for RxBufs Unsucessful\r\n"));
        return Status;
    }
    
    NdisZeroMemory(pAdapter->pBaseRxPkts[channel], pAdapter->NumRxIndicatePkts * sizeof(CPSW3G_RXPKTS));
    NdisZeroMemory(pAdapter->pBaseRxBufs[channel], pAdapter->NumRxMacBufDesc * sizeof(CPSW3G_RXBUFS));

    Status = Cpsw3g_Init_Rx_Buffer_memory(pAdapter, channel);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"Allocate RX buffer memory failed\n"));
        return Status;
    }

    ASSERT(pAdapter->RxBufsBase[channel]);
    ASSERT(pAdapter->RxBDBase[channel]);

    NdisZeroMemory((PVOID)pAdapter->RxBufsBase[channel], 
        pAdapter->NumRxMacBufDesc * CPSW3G_MAX_PKT_BUFFER_SIZE);

    NdisZeroMemory((PVOID)pAdapter->RxBDBase[channel], 
        pAdapter->NumRxMacBufDesc * sizeof(CPSW3G_DESC));

    /* Allocate Packet pool */
    NdisAllocatePacketPool(&Status,
                           &pAdapter->RecvPacketPool[channel],
                           pAdapter->NumRxIndicatePkts,
                           //PROTOCOL_RESERVED_SIZE_IN_PACKET);                           
                           MINIPORT_RESERVED_SIZE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"NdisAllocatePacketPool failed\n"));
        return Status;
    }

    pCurPkt = pNextPkt = pAdapter->pBaseRxPkts[channel];

    QUEUE_INIT(&pAdapter->RxPktPool[channel]);

    for (Count = 0; Count < pAdapter->NumRxIndicatePkts ; Count++)
    {
        pCurPkt = pNextPkt;
        pCurPkt->RxChannel = channel;

        /* Allocating packet from packet pool */
        NdisAllocatePacket( &Status,
                            (PNDIS_PACKET *)&pCurPkt->PktHandle,
                            pAdapter->RecvPacketPool[channel]);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR,(L" NdisAllocatePacket Unsucessful\r\n"));
            break;
        }

        QUEUE_INSERT(&pAdapter->RxPktPool[channel],pCurPkt);

        NDIS_SET_PACKET_HEADER_SIZE((PNDIS_PACKET)pCurPkt->PktHandle, CPSW3G_ETHERNET_HEADER_SIZE);

        pNextPkt++;
        pCurPkt->pNext  = pNextPkt;
    }

    pCurPkt->pNext=0;

    if(Count != pAdapter->NumRxIndicatePkts)
    {
        return Status;
    }

    DEBUGMSG (DBG_INFO,
       (L"+pAdapter->RxPktPool.Head %x  pAdapter->RxPktPool.Tail %x pAdapter->RxPktPool.Count %x \r\n",
       pAdapter->RxPktPool[channel].m_pHead,  
       pAdapter->RxPktPool[channel].m_pTail,
       pAdapter->RxPktPool[channel].m_Count));
    
    /* Allocate  the receive buffer pool */
    NdisAllocateBufferPool( &Status,
                            &pAdapter->RecvBufferPool[channel],
                            pAdapter->NumRxMacBufDesc
                          );

    if (Status != NDIS_STATUS_SUCCESS)
    {
        RETAILMSG(TRUE, (L"NdisAllocateBufferPool failed\n"));
        return Status;
    }

    pCurBuf = pNextBuf = pAdapter->pBaseRxBufs[channel];

    RcvBufLogical   = pAdapter->RxBufsBase[channel];
    RcvBufPhysical =  NdisGetPhysicalAddressLow(pAdapter->RxBufsBasePa[channel]);
    
    RcvBufDesBase   = (pAdapter->RxBDBase[channel]);
    RcvBufDesBasePa = NdisGetPhysicalAddressLow(pAdapter->RxBDBasePa[channel]);

    QUEUE_INIT(&pAdapter->RxBufsPool[channel]);

    for (Count = 0; Count < pAdapter->NumRxMacBufDesc ; Count++)
    {
        pCurBuf = pNextBuf;
        //RETAILMSG(TRUE, (L"Cpsw3g_InitRecv: adding buf %x for channel %d\r\n", pCurBuf, channel));

        pCurBuf->BufLogicalAddress  = RcvBufLogical;
        pCurBuf->BufPhysicalAddress = RcvBufPhysical;

        /* point our buffer for receives at this Rfd */
        NdisAllocateBuffer(&Status,
                           (PNDIS_BUFFER *)&pCurBuf->BufHandle,
                           pAdapter->RecvBufferPool[channel],
                           (PVOID)pCurBuf->BufLogicalAddress,
                           CPSW3G_MAX_PKT_BUFFER_SIZE);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR,(L" NdisAllocateBuffer Unsucessful\r\n"));
            break;
        }

        /* Assigning the buffer descriptors virtual and physical
        * addressses as well
        */
        pCurBuf->BufDes   = RcvBufDesBase;
        pCurBuf->BufDesPa = RcvBufDesBasePa;

        /* Setting up Rx buffer descriptors */
        pRxBD = (PCPSW3G_DESC)pCurBuf->BufDes;
        pRxBD->pNext        = (PCPSW3G_DESC)((UINT8 *)RcvBufDesBasePa + sizeof(CPSW3G_DESC));
        pRxBD->pBuffer      = (UINT8 *)(RcvBufPhysical);
        pRxBD->BufOffLen    = CPSW3G_MAX_PKT_BUFFER_SIZE;
        pRxBD->PktFlgLen    = CPSW3G_CPPI_OWNERSHIP_BIT;

        /*DEBUGMSG(DBG_ERR, (L"InitRecv::init BD buffer::count:%d ->CurBuf:%x, BufDes:%x, BufDesPa:%x\n",
                                             Count, pCurBuf,  pCurBuf->BufDes, pCurBuf->BufDesPa)); */

        /* we will also set up correspondinG  buffer descriptors virtual as
        * well as physical ADDRESS
        */
        RcvBufLogical += CPSW3G_MAX_PKT_BUFFER_SIZE;
        RcvBufPhysical += CPSW3G_MAX_PKT_BUFFER_SIZE;

        RcvBufDesBase += sizeof(CPSW3G_DESC);
        RcvBufDesBasePa += sizeof(CPSW3G_DESC);

        pNextBuf++;

        QUEUE_INSERT(&pAdapter->RxBufsPool[channel], pCurBuf);

    }
    pCurBuf->pNext=0;
    pRxBD = (PCPSW3G_DESC)pCurBuf->BufDes;
    pRxBD->pNext  = 0;

    if(Count != pAdapter->NumRxMacBufDesc)
    {
        return Status;
    }

    DEBUGMSG(DBG_FUNC, (L"<-- Cpsw3g_InitRecv, Status=%x\n", Status));

    return Status;
}

//========================================================================
//!  \fn void Cpsw3g_Init_SendRecv(PCPSW3G_ADAPTER pAdapter, UINT32 ch_mask)
//!  \brief main function to allocate resource for send/receive.
//!  \param PCPSW3G_ADAPTER pAdapter, UINT32 ch_mask
//!  \return None.
//========================================================================
static void Cpsw3g_Init_SendRecv(PCPSW3G_ADAPTER pAdapter, UINT32 ch_mask)
{
    UINT8  channel=0;
    ch_mask &= 0xff;
    
    if (Cpsw3g_InitSend(pAdapter) != NDIS_STATUS_SUCCESS)
    {
        RETAILMSG(TRUE, (L"Cpsw3g_Init_SendRecv:  Cpsw3g_InitSend Failed\r\n"));
    }

    for(channel=0; channel<CPDMA_MAX_CHANNELS; channel++)
    {
        if(ch_mask & (0x1 << channel))  
        {
            if ( Cpsw3g_InitRecv(pAdapter, channel) != NDIS_STATUS_SUCCESS )
            {
                 RETAILMSG(TRUE, (L"Cpsw3g_Init_SendRecv:  Cpsw3g_InitRecv Failed\r\n"));
            }
        }
    }    
}

void Cpsw3g_Clear_hw_stats(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 i;
    UINT32 *RegAddr = (UINT32 *)&(pAdapter->pCpsw3gRegsBase->RxGoodFrames);
    volatile UINT32 val;

    for(i=0;i<CPSW3G_NUM_STATS_REGS;i++)
    {
        val = RegAddr[i];
        RegAddr[i] = val;
    }
}

// (slave)port = 1, 2
int Cpsw3g_ConfigMacAddress(PCPSW3G_ADAPTER pAdapter, UINT32 port)
{
    UINT32 idx = P2I(port); // for array indexing purpose 

    /* Configure Source mac address */
    pAdapter->pCpsw3gRegsBase->CPSW_SL_Regs[idx].SL_SA_LO = 
        (pAdapter->MACAddress[5]<< 8) | pAdapter->MACAddress[4] ;

    pAdapter->pCpsw3gRegsBase->CPSW_SL_Regs[idx].SL_SA_HI = 
        (pAdapter->MACAddress[3]<< 24)|(pAdapter->MACAddress[2]<< 16)|
        (pAdapter->MACAddress[1]<< 8) | pAdapter->MACAddress[0];

    return (CPSW3G_SUCCESS);
}

// (slave)port = 1, 2
void Cpsw3g_Rxchannel_Map(PCPSW3G_ADAPTER pAdapter, UINT32 port, UINT32 channel_mask)
{
    UINT8 ch1=0, ch2=0, i;
    UINT32 idx = P2I(port); // for array indexing purpose 

    channel_mask &=0xff;

    for(i=0; i<CPDMA_MAX_CHANNELS;i++)
        if(channel_mask & (0x1 << i)) break;

    ch1 = i++;

    for(; i<CPDMA_MAX_CHANNELS;i++)
        if(channel_mask & (0x1 << i)) break;

    if (i != CPDMA_MAX_CHANNELS)
        ch2 = i;
    else 
        ch2 = ch1;
    
    pAdapter->pCpsw3gRegsBase->SL_Regs[idx].SL_Rx_Pri_Map = 
        ((ch1 << 0)  | (ch1 << 4)  | (ch1 << 8)  | (ch1 << 12) |
         (ch2 << 16) | (ch2 << 20) | (ch2 << 24) | (ch2 << 28)
        );

    if((pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL) && pAdapter->KITL_enable)
    {
        pAdapter->pCpsw3gRegsBase->CPDMA_Rx_Ch_Map = 
            ((KITL_CHANNEL << 0)  | (ch1 << 4)  | (ch1 << 8)  | (ch1 << 12) |
             (KITL_CHANNEL << 16) | (ch2 << 20) | (ch2 << 24) | (ch2 << 28));
    }
    else if (pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC)
    {
        if(pAdapter->ActiveCpgmac == 1) 
            pAdapter->pCpsw3gRegsBase->CPDMA_Rx_Ch_Map = 0;

        pAdapter->pCpsw3gRegsBase->CPDMA_Rx_Ch_Map |= ((ch1 << ch1*4) | (ch1 << (ch1 +4)*4));
        pAdapter->pCpsw3gRegsBase->CPDMA_Rx_Ch_Map |= ((ch2 << ch2*4) | (ch2 << (ch2 +4)*4));
    }
    else
        pAdapter->pCpsw3gRegsBase->CPDMA_Rx_Ch_Map = 
            ((ch1 << 0)  | (ch1 << 4)  | (ch1 << 8)  | (ch1 << 12) |
             (ch2 << 16) | (ch2 << 20) | (ch2 << 24) | (ch2 << 28));
       
    pAdapter->pCpsw3gRegsBase->P0_Tx_Pri_Map = 0x76543210;
}

/* Since ActiveCpgmac will be changed in InitializeAdapter(), 
   this function should be called before calling InitializeAdapter() */
UINT32 Cpsw3g_get_current_mac_port (PCPSW3G_ADAPTER  pAdapter)
{
    UINT32 port=0;
    switch(pAdapter->Cfg.cpsw3g_mode)
    {
        case CPSW3G_ESWITCH:
            port = CPGMAC_PORT_START;
            break;
        case CPSW3G_CPGMAC:
            /* if this miniport instance is up already, use saved port number */
            if(pAdapter->curr_port_info)
            {
                port = pAdapter->curr_port_info->PortNum;
            }
            else
            {
                /* first time up, use ActiveCpgmac */
                port = ActiveCpgmac + 1;
            }
            break;
        case CPSW3G_CPGMAC_KITL:
            // kitl=1 => port=2
            // kitl=2 => port=1
            port = OTHER_MAC_PORT(pAdapter->Cfg.KITL_port);
            break;

        default:
            RETAILMSG(TRUE, (L"Cpsw3g_get_current_mac_port: CPSW3G driver mode%d is not supported.\r\n",
                            pAdapter->Cfg.cpsw3g_mode));
            return (0xff);
    }
    return (port);
}


// Note: depending on platform, port may be 1-bases
static void Cpsw3g_Enable_DMAchannel(PCPSW3G_ADAPTER pAdapter, UINT32 port)
{
    UINT8 channel=0;
    CPSW3G_PORT_INFO *port_info = &pAdapter->Cpsw3gPort[port];
    UINT32 ch_mask = port_info->RxPortChanMask & 0xff; 
    
    pAdapter->pCpsw3gRegsBase->Tx_HDP[port_info->Tx_channel] = 0;
    pAdapter->pCpsw3gRegsBase->Tx_CP[port_info->Tx_channel] = 0;

    pAdapter->pSsRegsBase->C0_Tx_En = 0xFF;
    pAdapter->pCpsw3gRegsBase->Tx_IntMask_Set |= 0x1 << port_info->Tx_channel; 

    /* Enable Rx Channel */
    for(channel=0; channel<CPDMA_MAX_CHANNELS; channel++)
    {
        if((ch_mask & (0x1 << channel)) && pAdapter->RxBufsPool[channel].m_pHead) 
        {
            pAdapter->pCpsw3gRegsBase->Rx_HDP[channel] = 
                      ((PCPSW3G_RXBUFS)(pAdapter->RxBufsPool[channel].m_pHead))->BufDesPa;

            DEBUGMSG(DBG_INFO, (L"Cpsw3g_Enable_DMAchannel: channel %d rx_hdp:%x.\r\n",
                            channel, pAdapter->pCpsw3gRegsBase->Rx_HDP[channel] ));

            pAdapter->pCpsw3gRegsBase->Rx_CP[channel] = 0;

            /* Enable Channel interrupt */
            pAdapter->pSsRegsBase->C0_Rx_En |= (0x1 << channel);
            pAdapter->pCpsw3gRegsBase->Rx_IntMask_Set |= (0x1 << channel);
        }
    }    

    DEBUGMSG(DBG_INFO, (L"Cpsw3g_Enable_DMAchannel: tx_intmask_set:%x, rx_intmask_set:%x.\r\n",
        pAdapter->pCpsw3gRegsBase->Tx_IntMask_Set , pAdapter->pCpsw3gRegsBase->Rx_IntMask_Set));
}

void Cpsw3g_Set_AlePortCfg(PCPSW3G_ADAPTER pAdapter, UINT32 port)
{
    UINT32 portCfg;
    Cpsw_PortHwConfig *hw_cfg;

    if((port >= CPGMAC_PORT_START) && (port <= CPGMAC_PORT_END))
        hw_cfg = &pAdapter->Cfg.macCfg[P2I(port)].hw_config;
    else if(port==CPSW3G_HOST_PORT) 
        hw_cfg = &pAdapter->Cfg.dmaCfg.hw_config;
    else 
        return;

    if(port == CPSW3G_HOST_PORT)
    {
        portCfg = CPSW3G_DEFAULT_PORTSTATE << CPSW3G_ALEPORTCONTROL_PORTSTATE_SHIFT;
    }
    else
    {
        switch(pAdapter->Cfg.cpsw3g_mode)
        {
            case CPSW3G_CPGMAC:
            case CPSW3G_CPGMAC_KITL:
                portCfg = CPSW3G_DEFAULT_PORTSTATE << CPSW3G_ALEPORTCONTROL_PORTSTATE_SHIFT |
                               0x1<<CPSW3G_ALEPORTCONTROL_NOLEARN_SHIFT;
                break;

            case CPSW3G_ESWITCH:
                portCfg = CPSW3G_DEFAULT_PORTSTATE << CPSW3G_ALEPORTCONTROL_PORTSTATE_SHIFT;
                break;
            default:
                break;
        }
    }
    
    if(hw_cfg->VIDIngressCheck)
        portCfg |= CPSW3G_ALEPORTCONTROL_VIDINGRESSCHECK;
    if(hw_cfg->DropUntagged)
        portCfg |= CPSW3G_ALEPORTCONTROL_DROPUNTAGGED;

    if(hw_cfg->bcastLimit)
        portCfg |= (hw_cfg->bcastLimit & 0xff) << CPSW3G_ALEPORTCONTROL_BCASTLIMIT_SHIFT;

    if(hw_cfg->mcastLimit)
        portCfg |= (hw_cfg->mcastLimit & 0xff) << CPSW3G_ALEPORTCONTROL_MCASTLIMIT_SHIFT;

    pAdapter->pCpsw3gRegsBase->ALE_PortCtl[port] = portCfg;

}

/* Set Configuration - ALE_Control register
   First call should set the clearTable bit */
void Cpsw3g_SetAleCfg(PCPSW3G_ADAPTER pAdapter)
{
	PCPSW3G_CONFIG    iCfg=&pAdapter->Cfg;
    
    pAdapter->pCpsw3gRegsBase->ALE_Control = (UINT32)(
            CPSW3G_ALECONTROL_ENABLEALE    |
            CPSW3G_ALECONTROL_CLEARTABLE  );
    
    if(pAdapter->Cfg.vlanAware)
        pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_ALEVLANAWARE;
    

    if(pAdapter->Cfg.MacAuth)
        pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_ENABLEAUTHMODE;
    
    if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC)
    {
       if (pAdapter->Cfg.ale_bypass)
            pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_ALEBYPASS;

        pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_LEARNNOVID;
    }    

    /* Configure ALE Prescale register */
    pAdapter->pCpsw3gRegsBase->ALE_PreScale = iCfg->ale_prescale & CPSW3G_ALEPRESCALE_MASK;

    /* enable rate limitation on RX*/
    pAdapter->pCpsw3gRegsBase->ALE_Control &= ~CPSW3G_ALECONTROL_RATELIMITTX;
    /* enable rate limitation */
    pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_ENABLERATELIMIT;
    
    /* Configure Unknown VLAN items */
    pAdapter->pCpsw3gRegsBase->ALE_Unknown_VLAN = 
                            (iCfg->UnknownUntagEgress & 0x7)       << 24 |
                            (iCfg->UnknownRegMcastFloodMask & 0x7) << 16 |
                            (iCfg->UnknownMcastFloodMask & 0x7)    <<  8 |
                            (iCfg->UnknownMemberList & 0x7)        <<  0;
}


void Cpsw3g_Init_ALE(PCPSW3G_ADAPTER pAdapter, BOOL reset, UINT32 port)
{
    if(reset)
       Cpsw3g_SetAleCfg(pAdapter);

    if(pAdapter->Cfg.vlanAware)
    {
        cpsw_ale_add_ucast_entry(pAdapter, pAdapter->MACAddress, CPSW3G_HOST_PORT);
        Cpsw_add_vlan_ALE_entry(pAdapter);
    }
    else        
        cpsw_ale_add_ucast_entry(pAdapter, pAdapter->MACAddress, CPSW3G_HOST_PORT);

    Cpsw_add_static_ALE_entry(pAdapter);

    /* Add 802.1x multicast entry when enabled */
    if(pAdapter->Cfg.Enable_8021x)
        cpsw_ale_add_mcast_entry(pAdapter, mcast_802_1x_mac, 0, 0x04 | (1 << port));

}

BOOL Cpsw3g_Init_Cpdma(PCPSW3G_ADAPTER pAdapter, BOOL reset)
{
    CpdmaPortConfig *dmaCfg = &pAdapter->Cfg.dmaCfg;

    if(reset)
    {
        if (cpsw_soft_reset(&pAdapter->pCpsw3gRegsBase->CPDMA_Soft_Reset) == FALSE)
            return FALSE;

        pAdapter->pCpsw3gRegsBase->DMAControl = CPSW3G_DEFAULT_TXPTYPE;

        /* set port VLAN cfg */
        pAdapter->pCpsw3gRegsBase->P0_Port_VLAN = 
            dmaCfg->hw_config.portPri << CPSW3G_PORTVLAN_PRI_SHIFT |
            dmaCfg->hw_config.portCfi << CPSW3G_PORTVLAN_CFI_SHIFT |
            dmaCfg->hw_config.portVID << CPSW3G_PORTVLAN_VID_SHIFT;

        /* Set the rx buffer offset */
        pAdapter->pCpsw3gRegsBase->RX_Buffer_Offset = 0;

        /* Enable TX/RX DMA */
        pAdapter->pCpsw3gRegsBase->Tx_Control |= CPSW3G_TX_CONTROL_TX_ENABLE_VAL;
        pAdapter->pCpsw3gRegsBase->Rx_Control |= CPSW3G_RX_CONTROL_RX_ENABLE_VAL;
    }

    /* Enable Adapter check interrupts - disable stats interupt */
    pAdapter->pCpsw3gRegsBase->DMA_IntMask_Set = CPSW3G_HOST_ERR_INTMASK_VAL |
                                                 CPSW3G_STAT_INTMASK_VAL;

    return (TRUE);
}

// phyChan is 0 based. in this case it is already the right index. no 
// need to convert using P2I().
static void Cpsw3g_cpgmac_control(PCPSW3G_ADAPTER pAdapter, UINT32 phyChan)
{
    UINT32 macControlVal;

    macControlVal = pAdapter ->pCpsw3gRegsBase->SL_Regs[phyChan].SL_MacControl;
    if (pAdapter->PhyDevice[phyChan].link_duplex == FULL_DUPLEX)
        macControlVal |= (0x1 << CPMAC_MACCONTROL_FULLDUPLEXEN_SHIFT);
    else 
        macControlVal &= ~(0x1 << CPMAC_MACCONTROL_FULLDUPLEXEN_SHIFT);
        
    if (pAdapter->PhyDevice[phyChan].link_speed == 1000000)
        macControlVal |= ((0x1 << CPMAC_MACCONTROL_GIGABITEN_SHIFT) |
                          (0x1 << CPMAC_MACCONTROL_GIGFORCE_SHIFT));
    else
        macControlVal &= ~((0x1 << CPMAC_MACCONTROL_GIGABITEN_SHIFT) |
                           (0x1 << CPMAC_MACCONTROL_GIGFORCE_SHIFT));
        
    pAdapter ->pCpsw3gRegsBase->SL_Regs[phyChan].SL_MacControl = macControlVal;
        
}

// Note: depending on platform, port may not be 0 based
BOOL Cpsw3g_Init_Cpgmac(PCPSW3G_ADAPTER pAdapter, UINT32 port)
{
    UINT32 macControlVal;
    CpgmacPortConfig *macCfg = NULL;
    UINT32 idx;

    // convert port number to array index
    idx = P2I(port);

    macCfg = &pAdapter->Cfg.macCfg[idx];

    macControlVal = 0x1 << CPMAC_MACCONTROL_MIIEN_SHIFT        |
                    0x1 << CPMAC_MACCONTROL_TXFLOWEN_SHIFT     |
                    0x1 << CPMAC_MACCONTROL_FULLDUPLEXEN_SHIFT |
                    0x1 << CPMAC_MACCONTROL_GIGABITEN_SHIFT;
    
    pAdapter ->pCpsw3gRegsBase->SL_Regs[idx].SL_MacControl = macControlVal;
    pAdapter ->pCpsw3gRegsBase->CPSW_SL_Regs[idx].Tx_Pri_Map = 0x33221100;

    Cpsw3g_ConfigMacAddress(pAdapter, port);
    
    pAdapter ->pCpsw3gRegsBase->CPSW_SL_Regs[idx].Port_VLAN = 
                                macCfg->hw_config.portPri << CPSW3G_PORTVLAN_PRI_SHIFT |
                                macCfg->hw_config.portCfi << CPSW3G_PORTVLAN_CFI_SHIFT |
                                macCfg->hw_config.portVID << CPSW3G_PORTVLAN_VID_SHIFT;

    pAdapter ->pCpsw3gRegsBase->SL_Regs[idx].SL_Rx_Maxlen = macCfg->rxMaxlen;

    /* FIFO allocation for TX:14, RX:6. This is recommended value in spec */
    pAdapter ->pCpsw3gRegsBase->CPSW_SL_Regs[idx].Max_Blks = 0xE6;

    return (CPSW3G_SUCCESS);
}
void Cpsw3g_disable_interrupt(PCPSW3G_ADAPTER pAdapter)
{
    pAdapter->pCpsw3gRegsBase->Tx_IntMask_Clear = 0xff;
    
    /* if KITL is enabled, then do not disable KITL RX interrupt */    
    if(pAdapter->KITL_enable && pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL )
       pAdapter->pCpsw3gRegsBase->Rx_IntMask_Clear = (~(0x1 << KITL_CHANNEL)) & 0xff;
    else
       pAdapter->pCpsw3gRegsBase->Rx_IntMask_Clear = 0xff;
        
    pAdapter->pCpsw3gRegsBase->DMA_IntMask_Clear= 0x03;

}

void Cpsw3g_enable_interrupt(PCPSW3G_ADAPTER pAdapter)
{
    UINT8 channel=0;
    UINT32 ch_mask = pAdapter->curr_port_info->RxPortChanMask & 0xff;

    pAdapter->pSsRegsBase->C0_Tx_En = 0xFF;
    pAdapter->pCpsw3gRegsBase->Tx_IntMask_Set |= 0x1 << pAdapter->curr_port_info->Tx_channel; 

    /* Enable Rx Channel interrupt */
    pAdapter->pSsRegsBase->C0_Rx_En = 0xFF;
    for(channel=0; channel<CPDMA_MAX_CHANNELS; channel++)
    {
        if((ch_mask & (0x1 << channel))) 
        {
            pAdapter->pCpsw3gRegsBase->Rx_IntMask_Set |= (0x1 << channel);
        }
    }

    /* Enable Adapter check interrupts - disable stats interupt */
    pAdapter->pCpsw3gRegsBase->DMA_IntMask_Set = CPSW3G_HOST_ERR_INTMASK_VAL |
                                                 CPSW3G_STAT_INTMASK_VAL;

    RETAILMSG(1, (L"Cpsw3g_enable_interrupt: tx_intmask_set:%x, rx_intmask_set:%x.\r\n",
        pAdapter->pCpsw3gRegsBase->Tx_IntMask_Set , pAdapter->pCpsw3gRegsBase->Rx_IntMask_Set));

}

void Cpsw3g_teardown_tx_channel(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 channel = pAdapter->curr_port_info->Tx_channel;

    pAdapter->pCpsw3gRegsBase->Tx_Teardown = channel;
    DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_tx_channel:  %d\r\n", channel)); 

    while(pAdapter->pCpsw3gRegsBase->Tx_CP[channel] != CPSW3G_TEARDOWN)
    {
        NdisMSleep(1000);
        DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_tx_channel: waiting for teardown  %x\r\n", 
                pAdapter->pCpsw3gRegsBase->Tx_CP[channel])); 
    }
    DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_tx_channel %d done! \r\n", channel)); 

}

void Cpsw3g_teardown_rx_channel(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 channel;

    for(channel = 0;channel < CPDMA_MAX_CHANNELS; channel++)
    {
        if((0x1 << channel) & pAdapter->curr_port_info->RxPortChanMask)
        {
        
            pAdapter->pCpsw3gRegsBase->Rx_Teardown = channel;
            DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_rx_channel:  %d\r\n", channel)); 

            while(pAdapter->pCpsw3gRegsBase->Rx_CP[channel] != CPSW3G_TEARDOWN)
            {
                NdisMSleep(1000);
                DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_rx_channel: waiting for teardown  %x\r\n", 
                        pAdapter->pCpsw3gRegsBase->Rx_CP[channel])); 
            }
            DEBUGMSG(DBG_INFO, (L"  -> Cpsw3g_teardown_rx_channel %d done! \r\n", channel)); 
            
        }
    }
}

void Cpsw3g_free_Adapter_memory(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 i,j, len;
    PCPSW3G_RXPKTS     pNextPkt;
    PCPSW3G_RXBUFS     pNextBuf;
    
    
    /* Free Tx Pkts and Bufs */
    if(NULL != pAdapter->pBaseTxPkts)
    {
        NdisFreeMemory(pAdapter->pBaseTxPkts,
                pAdapter->MaxPacketPerTransmit * sizeof(CPSW3G_TXPKT),
                0);
        pAdapter->pBaseTxPkts = NULL;
    }
    if(NULL != pAdapter->pBaseTxBufs)
    {
        NdisFreeMemory(pAdapter->pBaseTxBufs,
            pAdapter->MaxTxMacBufDesc * sizeof(CPSW3G_TXBUF),
            0);
        pAdapter->pBaseTxBufs = NULL;
    }
    /* Free Allocated buffer */
    if(NULL != (VOID *)pAdapter->TxBufBase)
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                              CPSW3G_MAX_TX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
                                              TRUE, //cached
                                              (PVOID)pAdapter->TxBufBase, 
                                              pAdapter->TxBufBasePa);
    }
    len = ((sizeof(CPSW3G_DESC)+(CPSW3G_BD_ALIGN-1)) & ~(CPSW3G_BD_ALIGN-1)) * CPSW3G_MAX_TX_BUFFERS;

    if(NULL != (VOID *)pAdapter->TxBDBase)
    {
        NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                              len,
                                              FALSE, 
                                              (PVOID)pAdapter->TxBDBase, 
                                              pAdapter->TxBDBasePa);    
    }

    /* Free Rx Pkts and Bufs */
    for(i=0; i<CPDMA_MAX_CHANNELS; i++)
    {
        if(!((0x1 << i) & pAdapter->curr_port_info->RxPortChanMask)) 
            continue;

        /* Free buffer Pool */
        {
            pNextBuf = pAdapter->pBaseRxBufs[i];
            
            for(j=0;j<pAdapter->NumRxIndicatePkts && pNextBuf;j++, pNextBuf++)
            {
                if(pNextBuf->BufHandle)
                    NdisFreeBuffer(pNextBuf->BufHandle);
            }
            NdisFreeBufferPool(pAdapter->RecvBufferPool[i]);
            pAdapter->RecvBufferPool[i] = NULL;
        }

        /* Free Packet Pool */
        if(NULL != pAdapter->RecvPacketPool[i])
        {
            pNextPkt = pAdapter->pBaseRxPkts[i];

            for(j=0;j<pAdapter->NumRxIndicatePkts && pNextPkt; j++, pNextPkt++)
            {
                if(pNextPkt->PktHandle)
                    NdisFreePacket(pNextPkt->PktHandle);
            }
            NdisFreePacketPool(pAdapter->RecvPacketPool[i]);
            pAdapter->RecvPacketPool[i] = NULL;
        }
        
        if(NULL != pAdapter->pBaseRxPkts[i])
        {
            NdisFreeMemory(pAdapter->pBaseRxPkts[i],
                        pAdapter->NumRxIndicatePkts * sizeof(CPSW3G_RXPKTS),
                        0);
            pAdapter->pBaseRxPkts[i] = NULL;

        }
        if(NULL != pAdapter->pBaseRxBufs[i])
        {
            NdisFreeMemory(pAdapter->pBaseRxBufs[i],
                    pAdapter->NumRxMacBufDesc * sizeof(CPSW3G_RXBUFS),
                    0);
            pAdapter->pBaseRxBufs[i] = NULL;
        }
        /* Free Allocated buffer */
        if(NULL != (VOID *)pAdapter->RxBufsBase[i])
        {
            NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                                      CPSW3G_MAX_RX_BUFFERS * (CPSW3G_MAX_PKT_BUFFER_SIZE),
                                                      TRUE, //cached
                                                      (PVOID)pAdapter->RxBufsBase[i], 
                                                      pAdapter->RxBufsBasePa[i]);    
        }
        len = ((sizeof(CPSW3G_DESC)+(CPSW3G_BD_ALIGN-1)) & ~(CPSW3G_BD_ALIGN-1)) * CPSW3G_MAX_RX_BUFFERS;
        
        if(NULL != (VOID *)pAdapter->RxBDBase[i])
        {
            NdisMFreeSharedMemory(pAdapter->AdapterHandle, 
                                                      len,
                                                      FALSE, //UNcached
                                                      (PVOID)pAdapter->RxBDBase[i], 
                                                      pAdapter->RxBDBasePa[i]);    
        }
        
    }
}
BOOL Cpsw3g_Interrupt_pending(PCPSW3G_ADAPTER pAdapter)
{
    /* Check TX interrupt */
    if (pAdapter->pCpsw3gRegsBase->Tx_IntStat_Masked & (0x1 <<pAdapter->curr_port_info->Tx_channel))
        return TRUE;

    /* Check RX interrupt */
    if (pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked & (pAdapter->curr_port_info->RxPortChanMask))
        return TRUE;

    /* Check RX Thresh interrupt */
    if (pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked & ((pAdapter->curr_port_info->RxPortChanMask) << 8))
        return TRUE;

    /* Check Host Err and Statistics interrupt */
    if (pAdapter->pCpsw3gRegsBase->DMA_IntStat_Masked & 0x3)
        return TRUE;

    return FALSE;
}

void Cpsw3g_tx_complete_packet(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 channel = pAdapter->curr_port_info->Tx_channel;
    while(pAdapter->pCpsw3gRegsBase->Tx_IntStat_Masked & (0x1 << channel))
    {
        pAdapter->pCpsw3gRegsBase->Tx_CP[channel] = 
            pAdapter->pCpsw3gRegsBase->Tx_CP[channel];

        DEBUGMSG(DBG_INFO, (L"Cpsw3g_tx_complete_packet: channel %d CP=%x\r\n", 
            channel, pAdapter->pCpsw3gRegsBase->Tx_CP[channel]));
    }
}

void Cpsw3g_rx_complete_packet(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 channel ;

    for(channel = 0;channel < CPDMA_MAX_CHANNELS; channel++)
    {
        if((0x1 << channel) & pAdapter->curr_port_info->RxPortChanMask)
        {
            while(pAdapter->pCpsw3gRegsBase->Rx_IntStat_Masked & (0x1 << channel))
            {
                pAdapter->pCpsw3gRegsBase->Rx_CP[channel] = 
                    pAdapter->pCpsw3gRegsBase->Rx_CP[channel];

                DEBUGMSG(DBG_INFO,(L"Cpsw3g_rx_complete_packet: channel %d CP=%x\r\n", 
                    channel, pAdapter->pCpsw3gRegsBase->Rx_CP[channel]));
                
            }
        }
    }
}

BOOL Cpsw3g_StopAdapter(PCPSW3G_ADAPTER pAdapter)
{
    BOOLEAN ret;
    UINT32  teardown_event, i;

    teardown_event = 0x1<<(8+pAdapter->curr_port_info->Tx_channel) | 
                               pAdapter->curr_port_info->RxPortChanMask;

   
    /* tear down channels */
    Cpsw3g_teardown_tx_channel(pAdapter);
    Cpsw3g_teardown_rx_channel(pAdapter);

    Cpsw3g_tx_complete_packet(pAdapter);
    Cpsw3g_rx_complete_packet(pAdapter);

    /* Wait until all interrupts are handled */

    while(Cpsw3g_Interrupt_pending(pAdapter)){
        NdisMSleep(10);
    }

    /* Write EOI */
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x2; // TX
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x1; // RX
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x3; // host err and stat
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x0; // RX_THRESH

    DEBUGMSG(DBG_INFO, (L"Cpsw3g_StopAdapter: All interrupts are cleared\r\n"));

    if( pAdapter->Cfg.cpsw3g_mode != CPSW3G_CPGMAC || ActiveCpgmac==1)
    {
        /* Cancel timers */  
        if(((PNDIS_MINIPORT_TIMER)ale_timer) != NULL)
        {
            NdisMCancelTimer((PNDIS_MINIPORT_TIMER)ale_timer, &ret);
            DEBUGMSG(DBG_INFO, (L"Cpsw3g_StopAdapter: ALE timer is %s cancelled\r\n", ret?L"":L"not"));
            
            if(ret) {
                (PNDIS_MINIPORT_TIMER)ale_timer = 0;
            }
        }
    }

    if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_ESWITCH)    
    {
        for(i=CPGMAC_PORT_START; i<=CPGMAC_PORT_END; i++)   
        {
	        Cpsw3g_Phy_Stop(&pAdapter->PhyDevice[P2I(i)]);
        }
    }
    else
    {
	     Cpsw3g_Phy_Stop(&pAdapter->PhyDevice[P2I(pAdapter->curr_port_info->PortNum)]);
    }

    if(ActiveCpgmac > 0) ActiveCpgmac --;
    pAdapter->ActiveCpgmac = ActiveCpgmac;

    return TRUE;
}
void Cpsw3g_Set_CpswCfg(PCPSW3G_ADAPTER pAdapter)
{
    UINT32 cpswControlVal;
    PCPSW3G_CONFIG cpswCfg = &pAdapter->Cfg ;  

    /* Use default values for flow control */
    cpswControlVal = (cpswCfg->vlanAware   << CPSW3G_CPSWCONTROL_VLANAWARE_SHIFT) |
                     (cpswCfg->rxVlanEncap << CPSW3G_CPSWCONTROL_RXVLANENCAP_SHIFT);

    pAdapter->pCpsw3gRegsBase->CPSW_Control = cpswControlVal;

    pAdapter->pCpsw3gRegsBase->CPSW_Pritype = 0;

}


/* cpsw3g soft reset 
         Reset of CPSW need to be called for SW mode and 1st of the two CPMAC mode 
*/

BOOL Cpsw3g_Init_Cpsw(PCPSW3G_ADAPTER pAdapter, BOOL reset)
{
    UINT32 ch;
    if(reset)
    {

        /* Power on the ETH subsystem */
        if(FALSE == Cpsw3g_Ethss_power_on())
        {
            DEBUGMSG(DBG_ERR, (L"Fail to power on ETHSS modules  \n"));
            return FALSE;
        }
        else
        {
            pAdapter->CurrentPowerState = NdisDeviceStateD0;

            if(cpsw_soft_reset(&pAdapter->pCpsw3gRegsBase->CPSW_Soft_Reset) == FALSE)
                return FALSE;
        }
        
        Cpsw3g_rgmii_init(pAdapter, reset);

        /* Program TX/RX HDP's to 0 */
        for (ch = 0; ch < CPDMA_MAX_CHANNELS; ch++)
        {
            pAdapter->pCpsw3gRegsBase->Tx_HDP[ch] = 0;
            pAdapter->pCpsw3gRegsBase->Rx_HDP[ch] = 0;
            pAdapter->pCpsw3gRegsBase->Tx_CP[ch] = 0;
            pAdapter->pCpsw3gRegsBase->Rx_CP[ch] = 0;      
        }
        Cpsw3g_Set_CpswCfg(pAdapter);

    }

    /* Configure use of statistics*/
    pAdapter->pCpsw3gRegsBase->CPSW_Stat_Port_En = pAdapter->Cfg.stats_port_mask;

    /* Enable MDIO */
    MdioEnable((UINT32)pAdapter->pMdioRegsBase);

    return TRUE;
}


VOID  Cpsw3g_ALE_age_out
(
    IN  PVOID	SystemSpecific1,
    IN  PVOID	FunctionContext,
    IN  PVOID	SystemSpecific2, 
    IN  PVOID	SystemSpecific3
    )
{
    PCPSW3G_ADAPTER     pAdapter = (PCPSW3G_ADAPTER)FunctionContext;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
    
    pAdapter->pCpsw3gRegsBase->ALE_Control |= CPSW3G_ALECONTROL_AGEOUTNOW;

    if(!ale_timer){
        RETAILMSG(TRUE, (L"  ->Cpsw3g_ALE_age_out, timer alreay cancelled!\r\n"));
        return;
    }
    else
        /* restart timer */
        NdisMSetTimer(&pAdapter->ALE_timer, pAdapter->Cfg.ALE_Agingtimer);

}

void Cpsw3g_Update_LinkStatus(PHY_DEVICE *phydev)
{
    LINKSTATUS link_status = LINK_DOWN;
    UINT32 Status, i;
    PCPSW3G_ADAPTER  pAdapter = (PCPSW3G_ADAPTER )phydev->handle;
    PHY_DEVICE *phydev_change = (PHY_DEVICE *)NULL, *phydev_lan_speed = (PHY_DEVICE *)NULL;
    UINT32 LANport=0;
    UINT32 Otherport=0;

    if(pAdapter->Cfg.cpsw3g_mode == CPSW3G_ESWITCH)
    {
        for(i=CPGMAC_PORT_START;i<=CPGMAC_PORT_END;i++)
        {
            if( pAdapter->PhyDevice[P2I(i)].link == LINK_UP)
            {
                link_status = LINK_UP;
                pAdapter->link_speed = pAdapter->PhyDevice[P2I(i)].link_speed;
            }
        }

        if(pAdapter->Cfg.Speed_match)
        {
            /* In eswitch mode, use KITL port as LAN port, since it is read from MAC_PORT */
            LANport = pAdapter->Cfg.KITL_port;
            Otherport = OTHER_MAC_PORT(LANport);
            
            if( (pAdapter->PhyDevice[0].link == LINK_UP) && 
                (pAdapter->PhyDevice[1].link == LINK_UP) )
            {
                /* PC speed > LAN speed */
                if (pAdapter->PhyDevice[P2I(Otherport)].link_speed > 
                    pAdapter->PhyDevice[P2I(LANport)].link_speed)
                {
                    phydev_change = &pAdapter->PhyDevice[P2I(Otherport)];
                    phydev_lan_speed = &pAdapter->PhyDevice[P2I(LANport)];
                }

                /* Different speed, speed request to make them same */
                if (phydev_change != NULL && (phydev_change->speed_force_default != TRUE))
                {
                   
                    /* re-set speed_cap */
                    if(phydev_lan_speed->link_speed == 100000 )
                    {
                        phydev_change->speed_cap &= ~((0x1 <<PHY_1000_FD) | (0x1 <<PHY_1000_HD));
                    }
                    else if (phydev_lan_speed->link_speed == 10000)
                    {
                        phydev_change->speed_cap &= 
                            ~((0x1 <<PHY_100_FD)  | (0x1 <<PHY_100_HD) |
                              (0x1 <<PHY_1000_FD) | (0x1 <<PHY_1000_HD));
                    }
                    phydev_change->speed_change_request = TRUE;
                } 
                
            }
            else if(phydev->link == LINK_DOWN)
            {
                for(i=CPGMAC_PORT_START;i<=CPGMAC_PORT_END;i++)
                {
                    if(pAdapter->PhyDevice[P2I(i)].speed_cap != CPSW3G_SPEED_CAP_DEFAULT)
                    {
                        pAdapter->PhyDevice[P2I(i)].speed_cap = CPSW3G_SPEED_CAP_DEFAULT;
                        pAdapter->PhyDevice[P2I(i)].speed_change_request = TRUE;                
                    }
                    pAdapter->PhyDevice[P2I(i)].speed_force_default = FALSE;
                }
            }
        }
        
    }	
    else
    {
        link_status = phydev->link;
	    pAdapter->link_speed = phydev->link_speed;
    }
	
    if(link_status != pAdapter->link)
    {
        DEBUGMSG(DBG_INFO, (L"Cpsw3g_Update_LinkStatus: link %d is %s\r\n", 
               phydev->channel,
               (link_status == LINK_UP) ?  L"UP" : L"DOWN"));
		
        pAdapter->link = link_status;
        Status = (link_status == LINK_UP) ? NDIS_STATUS_MEDIA_CONNECT:
		     NDIS_STATUS_MEDIA_DISCONNECT;

        Cpsw3g_cpgmac_control(pAdapter, phydev->channel);

        // sleep for microsec
        NdisMSleep(500 * 1000); 

        /* Link was active last time, now it is up. */
        NdisMIndicateStatus(pAdapter->AdapterHandle,
                        Status, 
                        (PVOID)0, 
                        0);

        NdisMIndicateStatusComplete(pAdapter->AdapterHandle);

        if(pAdapter->ResetPending == TRUE && link_status == LINK_UP)
        {
            NdisMResetComplete(pAdapter->AdapterHandle, NDIS_STATUS_SUCCESS, TRUE);
            pAdapter->ResetPending = FALSE;
        }
    }	
}

BOOL Cpsw3g_eswitch_setup(PCPSW3G_ADAPTER  pAdapter, UINT32 current_port)
{
    UINT32 i;

        RETAILMSG(TRUE, 
            (L"  ++ Cpsw3g_eswitch_setup : port %d\r\n", 
            current_port
        ));
		
    if (Cpsw3g_Init_Cpsw(pAdapter, TRUE) == FALSE)
        return FALSE;

    for(i=CPGMAC_PORT_START; i <= CPGMAC_PORT_END; i++)
    {
        Cpsw3g_Init_Cpgmac(pAdapter, i);
        pAdapter->PhyDevice[P2I(i)].handle = pAdapter;
        pAdapter->PhyDevice[P2I(i)].channel = P2I(i);
        pAdapter->PhyDevice[P2I(i)].speed_cap = CPSW3G_SPEED_CAP_DEFAULT;
        Cpsw3g_Phy_Init(&pAdapter->PhyDevice[P2I(i)]);
    }

    Cpsw3g_Init_Cpdma(pAdapter, TRUE);

    Cpsw3g_Rxchannel_Map(pAdapter, current_port,  pAdapter->curr_port_info->RxPortChanMask);

    Cpsw3g_Init_ALE(pAdapter, TRUE, current_port);
    
    for(i=CPGMAC_PORT_START; i<=CPGMAC_PORT_END; i++)
    {
        Cpsw3g_Set_AlePortCfg(pAdapter, i);
    }

    Cpsw3g_Set_AlePortCfg(pAdapter, CPSW3G_HOST_PORT);

    Cpsw3g_Enable_DMAchannel(pAdapter, current_port);
    return (TRUE);
}

BOOL Cpsw3g_cpgmac_setup(PCPSW3G_ADAPTER  pAdapter, UINT32 current_port)
{

    pAdapter->Cfg.Speed_match = FALSE;
    if (Cpsw3g_Init_Cpsw(pAdapter, pAdapter->ActiveCpgmac == 1) == FALSE)
        return FALSE;

    Cpsw3g_Init_Cpgmac(pAdapter, current_port);
    pAdapter->PhyDevice[P2I(current_port)].handle = pAdapter;
    pAdapter->PhyDevice[P2I(current_port)].channel = P2I(current_port);
    pAdapter->PhyDevice[P2I(current_port)].speed_cap = CPSW3G_SPEED_CAP_DEFAULT;
    Cpsw3g_Phy_Init(&pAdapter->PhyDevice[P2I(current_port)]);

    Cpsw3g_Init_Cpdma(pAdapter, pAdapter->ActiveCpgmac == 1);

    Cpsw3g_Rxchannel_Map(pAdapter, current_port,  pAdapter->curr_port_info->RxPortChanMask);

    Cpsw3g_Init_ALE(pAdapter, pAdapter->ActiveCpgmac == 1, current_port);
    
    Cpsw3g_Set_AlePortCfg(pAdapter, current_port);
    Cpsw3g_Set_AlePortCfg(pAdapter, CPSW3G_HOST_PORT);

    Cpsw3g_Enable_DMAchannel(pAdapter, current_port);

#if 0
    Cpsw3g_disable_interrupt(pAdapter);
    Cpsw3g_disable_irq();
    Cpsw3g_enable_interrupt(pAdapter);
    Cpsw3g_enable_irq();
#endif

    /* Write EOI */
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x2; // TX
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x1; // RX
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x3; // host err and stat
    pAdapter->pCpsw3gRegsBase->CPDMA_EOI_Vector = 0x0; // RX_THRESH

    return (TRUE);

}

BOOL Cpsw3g_cpgmac_KITL_setup(PCPSW3G_ADAPTER  pAdapter, UINT32 current_port)
{
    pAdapter->Cfg.Speed_match = FALSE;

    if (Cpsw3g_Init_Cpsw(pAdapter, FALSE) == FALSE)
        return FALSE;

    Cpsw3g_Init_Cpgmac(pAdapter, current_port);
    pAdapter->PhyDevice[P2I(current_port)].handle = pAdapter;
    pAdapter->PhyDevice[P2I(current_port)].channel = P2I(current_port);
    pAdapter->PhyDevice[P2I(current_port)].speed_cap = CPSW3G_SPEED_CAP_DEFAULT;   
    Cpsw3g_Phy_Init(&pAdapter->PhyDevice[P2I(current_port)]);

    Cpsw3g_Init_Cpdma(pAdapter, FALSE);
    Cpsw3g_Rxchannel_Map(pAdapter, current_port,  pAdapter->curr_port_info->RxPortChanMask);

    Cpsw3g_Init_ALE(pAdapter, FALSE, current_port);
    
    Cpsw3g_Set_AlePortCfg(pAdapter, current_port);
    Cpsw3g_Set_AlePortCfg(pAdapter, CPSW3G_HOST_PORT);

    Cpsw3g_Enable_DMAchannel(pAdapter, current_port);

    return (TRUE);

}


NDIS_STATUS Cpsw3g_Init_hardware(PCPSW3G_ADAPTER  pAdapter, UINT32 current_port)
{
    switch(pAdapter->Cfg.cpsw3g_mode){
        case CPSW3G_ESWITCH:
            if(Cpsw3g_eswitch_setup(pAdapter, current_port) == FALSE)
            {
                goto Error;
            }
            break;
            
        case CPSW3G_CPGMAC:
            if(Cpsw3g_cpgmac_setup(pAdapter, current_port) == FALSE)
            {
                goto Error;
            }               
            break;
            
        case CPSW3G_CPGMAC_KITL:
            if(Cpsw3g_cpgmac_KITL_setup(pAdapter, current_port) == FALSE)
            {
                goto Error;
            }
            break;
            
        default:
            break;
        }
        
    return (CPSW3G_SUCCESS);

Error:
    return (NDIS_STATUS_FAILURE);
}

NDIS_STATUS Cpsw3g_InitializeAdapter(PCPSW3G_ADAPTER  pAdapter)
{
    UINT32  current_port=0; 
    UINT32  tx_chan=0;
    UINT32  rx_chan_mask=0;

    DEBUGMSG(DBG_FUNC, (L"-> Cpsw3g_InitializeAdapter \r\n"));

    current_port = Cpsw3g_get_current_mac_port(pAdapter);

#ifdef POLAR_1_0
    tx_chan=0;
    rx_chan_mask=(0x1<< tx_chan);
#else
    tx_chan=current_port; 
    rx_chan_mask=(0x1<< tx_chan) | (0x1 << (tx_chan+2));
#endif

    if((current_port < CPGMAC_PORT_START) || (current_port > CPGMAC_PORT_END)) 
        return NDIS_STATUS_FAILURE;

    pAdapter->curr_port_info = &pAdapter->Cpsw3gPort[current_port];

    pAdapter->curr_port_info->PortNum        = current_port;
    pAdapter->curr_port_info->RxPortChanMask = rx_chan_mask;
    pAdapter->curr_port_info->Tx_channel     = tx_chan;
    ActiveCpgmac ++;

    RETAILMSG(TRUE, (L"Cpsw3g_InitializeAdapter: mode=%d, port=%d, txCh=%d, rxChMask=0x%02X, ActiveCpgmac=%d\r\n", 
        pAdapter->Cfg.cpsw3g_mode,
        current_port, 
        pAdapter->curr_port_info->Tx_channel,
        pAdapter->curr_port_info->RxPortChanMask,
        ActiveCpgmac));
    
    /* Special handling for KITL mode */
    if (pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC_KITL)
    {
        pAdapter->curr_port_info->RxPortChanMask = (0x1 << 2 | 0x1 << 4); /* channel 2 and 4 */
        pAdapter->curr_port_info->Tx_channel = 2;
    }

    pAdapter->ActiveCpgmac = ActiveCpgmac;
    Cpsw3g_Init_SendRecv(pAdapter, pAdapter->curr_port_info->RxPortChanMask);

    pAdapter->link = NDIS_STATUS_MEDIA_DISCONNECT;

    memset(&pAdapter->Cpsw3g_sw_stats, 0, sizeof(CPSW3G_SW_STATS));
    memset(&pAdapter->Cpsw3gStatInfo, 0, sizeof(CPSW3G_STATINFO));

    if(Cpsw3g_Init_hardware(pAdapter, current_port) != CPSW3G_SUCCESS)
    {
        return NDIS_STATUS_FAILURE;    
    }

    pAdapter->PacketFilter = NDIS_PACKET_TYPE_DIRECTED;

    if(pAdapter->Cfg.cpsw3g_mode != CPSW3G_CPGMAC || ActiveCpgmac == 1 )
    {
        /* Start periodic timer for ALE age out */
        NdisMInitializeTimer(
                            &pAdapter->ALE_timer,
                            pAdapter->AdapterHandle,
                            Cpsw3g_ALE_age_out,
                            pAdapter);

        (PNDIS_MINIPORT_TIMER)ale_timer = &pAdapter->ALE_timer;

        NdisMSetTimer(&pAdapter->ALE_timer, pAdapter->Cfg.ALE_Agingtimer);
    }
 
    pAdapter->ResetPending = FALSE;
    
    DEBUGMSG(ZONE_INFO, (L"<-- Cpsw3g_InitializeAdapter.\r\n")); 

    return (CPSW3G_SUCCESS);
}

//========================================================================
//!  \fn NDIS_STATUS Cpsw3g_SetPowerState(...)
//!  \brief Sets the power state of the adapter.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to adapter
//!  \param PowerState New power state
//!  \return NDIS_STATUS Returns success or error states.
//========================================================================

NDIS_STATUS
Cpsw3g_SetPowerState(
    PCPSW3G_ADAPTER pAdapter,
    NDIS_DEVICE_POWER_STATE PowerState)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    if (pAdapter->CurrentPowerState != PowerState)
    {
        switch (PowerState)
        {
        case NdisDeviceStateD0:
            
            if( (pAdapter->Cfg.cpsw3g_mode == CPSW3G_ESWITCH) ||
                ((pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC) && (ActiveCpgmac == 0))
                )
            {
            
                if (!Cpsw3g_Ethss_power_on())
                {
                    Status = NDIS_STATUS_HARD_ERRORS;
                    break;
                }
            }          
            NdisAcquireSpinLock(&pAdapter->SendLock);
            NdisAcquireSpinLock(&pAdapter->RcvLock);

            Status = Cpsw3g_InitializeAdapter(pAdapter);
            if (Status == NDIS_STATUS_SUCCESS) 
            {
                Status = Cpsw3g_SelfCheck(pAdapter);
            }
            if (Status == NDIS_STATUS_SUCCESS) 
            {
                pAdapter->HwStatus = NdisHardwareStatusReady; 
                pAdapter->CurrentPowerState = PowerState;
            }

            NdisReleaseSpinLock(&pAdapter->RcvLock);
            NdisReleaseSpinLock(&pAdapter->SendLock); 
            break;
            
        case NdisDeviceStateD3:
            
            NdisDprAcquireSpinLock(&pAdapter->Lock);
           
            pAdapter->HwStatus = NdisHardwareStatusReset;

            if (!Cpsw3g_StopAdapter(pAdapter))
            {
                Status = NDIS_STATUS_HARD_ERRORS;
            }
            else
            {
                Cpsw3g_free_Adapter_memory(pAdapter);

                if( (pAdapter->Cfg.cpsw3g_mode == CPSW3G_ESWITCH) ||
                    ((pAdapter->Cfg.cpsw3g_mode == CPSW3G_CPGMAC) && (ActiveCpgmac == 0))
                    )
                {
                    if (!Cpsw3g_Ethss_power_off())
                    {
                        Status = NDIS_STATUS_HARD_ERRORS;
                    }
                    else
                    {
                        pAdapter->CurrentPowerState = PowerState;
                    }
                }
                else
                {
                    pAdapter->CurrentPowerState = PowerState;

                    RETAILMSG(TRUE, (L"Cpsw3g_SetPowerState -> down, mode=%d, ActiveCpgmac=%d\r\n", 
                        pAdapter->Cfg.cpsw3g_mode,  ActiveCpgmac));
                                    
                }
            }

            NdisReleaseSpinLock(&pAdapter->Lock);
            break;
            
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    }

    return Status;
}


#ifdef DEBUG_REG_DUMP

#define REG_OFS(r)       (&(((PCPSW3G_REGS)0)->r))
#define MAC_REG_OFS(i, r)  (((UINT32)REG_OFS(CPSW_SL_Regs[P2I(i)])) + (UINT32)(&(((Cpsw3g_SL_Regs *)0)->r)))
#define SL_REG_OFS(i, r)   (((UINT32)REG_OFS(SL_Regs[P2I(i)])) + (UINT32)(&(((CpgmacSl_Regs *)0)->r)))
#define SS_REG_OFS(r)      (CPSW_SS_BASE - CPSW_3G_BASE + (UINT32)(&(((PCPSW_SS_REGS)0)->r)))

void Cpsw3g_sl_port_reg_dump(Cpsw3g_SL_Regs *pRegs, int idx)
{

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  SLAVE PORT %d\r\n", idx));
#ifdef AM33X_BSP
    RETAILMSG(1, (L"  P%d_Control         : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Control),    pRegs->Control  ));
#endif
    RETAILMSG(1, (L"  P%d_Max_Blks        : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Max_Blks),   pRegs->Max_Blks ));
    RETAILMSG(1, (L"  P%d_BLK_CNT         : %08X %08X \r\n", idx, MAC_REG_OFS(idx, BLK_CNT),    pRegs->BLK_CNT  ));
    RETAILMSG(1, (L"  P%d_Tx_In_Ctl       : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Tx_In_Ctl),  pRegs->Tx_In_Ctl));
    RETAILMSG(1, (L"  P%d_Port_VLAN       : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Port_VLAN),  pRegs->Port_VLAN));
    RETAILMSG(1, (L"  P%d_Tx_Pri_Map      : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Tx_Pri_Map), pRegs->Tx_Pri_Map));

#ifdef AM387X_BSP
    RETAILMSG(1, (L"  P%d_TS_CTL          : %08X %08X \r\n", idx, MAC_REG_OFS(idx, TS_CTL      ), pRegs->TS_CTL      ));
    RETAILMSG(1, (L"  P%d_TS_SEQ_LTYPE    : %08X %08X \r\n", idx, MAC_REG_OFS(idx, TS_SEQ_LTYPE), pRegs->TS_SEQ_LTYPE));
    RETAILMSG(1, (L"  P%d_TS_VLAN         : %08X %08X \r\n", idx, MAC_REG_OFS(idx, TS_VLAN     ), pRegs->TS_VLAN     ));
#endif

#ifdef AM33X_BSP
    RETAILMSG(1, (L"  P%d_TS_SEQ_MTYPE    : %08X %08X \r\n", idx, MAC_REG_OFS(idx, TS_SEQ_MTYPE), pRegs->TS_SEQ_MTYPE));
#endif

    RETAILMSG(1, (L"  SL%d_SA_LO          : %08X %08X \r\n", idx, MAC_REG_OFS(idx, SL_SA_LO), pRegs->SL_SA_LO    ));
    RETAILMSG(1, (L"  SL%d_SA_HI          : %08X %08X \r\n", idx, MAC_REG_OFS(idx, SL_SA_HI), pRegs->SL_SA_HI    ));
    RETAILMSG(1, (L"  P%d_Send_Percent    : %08X %08X \r\n", idx, MAC_REG_OFS(idx, Send_Percent), pRegs->Send_Percent));

}


void Cpsw3g_port_reg_dump(PCPSW3G_REGS pRegs)
{
    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  HOST PORT:\r\n"));
#ifdef AM33X_BSP
    RETAILMSG(1, (L"  P0_Control         : %08X %08X \r\n", REG_OFS(P0_Control), pRegs->P0_Control         ));
#endif
    RETAILMSG(1, (L"  P0_Max_blks        : %08X %08X \r\n", REG_OFS(P0_Max_blks), pRegs->P0_Max_blks         ));
    RETAILMSG(1, (L"  P0_BLK_CNT         : %08X %08X \r\n", REG_OFS(P0_BLK_CNT), pRegs->P0_BLK_CNT         ));
    RETAILMSG(1, (L"  P0_Tx_In_Ctl       : %08X %08X \r\n", REG_OFS(P0_Tx_In_Ctl), pRegs->P0_Tx_In_Ctl     ));
    RETAILMSG(1, (L"  P0_Port_VLAN       : %08X %08X \r\n", REG_OFS(P0_Port_VLAN), pRegs->P0_Port_VLAN     ));
    RETAILMSG(1, (L"  P0_Tx_Pri_Map      : %08X %08X \r\n", REG_OFS(P0_Tx_Pri_Map), pRegs->P0_Tx_Pri_Map     ));
    RETAILMSG(1, (L"  CPDMA_Tx_Pri_Map   : %08X %08X \r\n", REG_OFS(CPDMA_Tx_Pri_Map), pRegs->CPDMA_Tx_Pri_Map ));
    RETAILMSG(1, (L"  CPDMA_Rx_Ch_Map    : %08X %08X \r\n", REG_OFS(CPDMA_Rx_Ch_Map), pRegs->CPDMA_Rx_Ch_Map     ));

    Cpsw3g_sl_port_reg_dump(&(pRegs->CPSW_SL_Regs[0]), 1);
    Cpsw3g_sl_port_reg_dump(&(pRegs->CPSW_SL_Regs[1]), 2);
}


void Cpsw3g_dma_reg_dump(PCPSW3G_REGS pRegs)
{
    int i;

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  CPDMA:\r\n"));
    RETAILMSG(1, (L"  Tx_Idver           : %08X %08X \r\n", REG_OFS(Tx_Idver        ), pRegs->Tx_Idver         ));
    RETAILMSG(1, (L"  Tx_Control         : %08X %08X \r\n", REG_OFS(Tx_Control      ), pRegs->Tx_Control         ));
    RETAILMSG(1, (L"  Tx_Teardown        : %08X %08X \r\n", REG_OFS(Tx_Teardown     ), pRegs->Tx_Teardown         ));
    RETAILMSG(1, (L"  Rsvd2              : %08X %08X \r\n", REG_OFS(Rsvd2           ), pRegs->Rsvd2             ));
    RETAILMSG(1, (L"  Rx_Idver           : %08X %08X \r\n", REG_OFS(Rx_Idver        ), pRegs->Rx_Idver         ));
    RETAILMSG(1, (L"  Rx_Control         : %08X %08X \r\n", REG_OFS(Rx_Control      ), pRegs->Rx_Control         ));
    RETAILMSG(1, (L"  Rx_Teardown        : %08X %08X \r\n", REG_OFS(Rx_Teardown     ), pRegs->Rx_Teardown         ));
    RETAILMSG(1, (L"  CPDMA_Soft_Reset   : %08X %08X \r\n", REG_OFS(CPDMA_Soft_Reset), pRegs->CPDMA_Soft_Reset ));
    RETAILMSG(1, (L"  DMAControl         : %08X %08X \r\n", REG_OFS(DMAControl      ), pRegs->DMAControl         ));
    RETAILMSG(1, (L"  DMAStatus          : %08X %08X \r\n", REG_OFS(DMAStatus       ), pRegs->DMAStatus         ));
    RETAILMSG(1, (L"  RX_Buffer_Offset   : %08X %08X \r\n", REG_OFS(RX_Buffer_Offset), pRegs->RX_Buffer_Offset ));
    RETAILMSG(1, (L"  EMControl          : %08X %08X \r\n", REG_OFS(EMControl       ), pRegs->EMControl         ));

    RETAILMSG(1, (L" \r\n"));
    for (i=0; i<8; i++)
    RETAILMSG(1, (L"  TX_PriN_Rate[%d]    : %08X %08X \r\n", i,  REG_OFS(TX_PriN_Rate[i]), pRegs->TX_PriN_Rate[i] ));

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  Tx_IntStat_Raw     : %08X %08X \r\n", REG_OFS(Tx_IntStat_Raw    ), pRegs->Tx_IntStat_Raw ));
    RETAILMSG(1, (L"  Tx_IntStat_Masked  : %08X %08X \r\n", REG_OFS(Tx_IntStat_Masked ), pRegs->Tx_IntStat_Masked ));
    RETAILMSG(1, (L"  Tx_IntMask_Set     : %08X %08X \r\n", REG_OFS(Tx_IntMask_Set    ), pRegs->Tx_IntMask_Set ));
    RETAILMSG(1, (L"  Tx_IntMask_Clear   : %08X %08X \r\n", REG_OFS(Tx_IntMask_Clear  ), pRegs->Tx_IntMask_Clear ));
    RETAILMSG(1, (L"  CPDMA_In_Vector    : %08X %08X \r\n", REG_OFS(CPDMA_In_Vector   ), pRegs->CPDMA_In_Vector     ));
    RETAILMSG(1, (L"  CPDMA_EOI_Vector   : %08X %08X \r\n", REG_OFS(CPDMA_EOI_Vector  ), pRegs->CPDMA_EOI_Vector     ));
    RETAILMSG(1, (L"  Rx_IntStat_Raw     : %08X %08X \r\n", REG_OFS(Rx_IntStat_Raw    ), pRegs->Rx_IntStat_Raw         ));
    RETAILMSG(1, (L"  Rx_IntStat_Masked  : %08X %08X \r\n", REG_OFS(Rx_IntStat_Masked ), pRegs->Rx_IntStat_Masked     ));
    RETAILMSG(1, (L"  Rx_IntMask_Set     : %08X %08X \r\n", REG_OFS(Rx_IntMask_Set    ), pRegs->Rx_IntMask_Set         ));
    RETAILMSG(1, (L"  Rx_IntMask_Clear   : %08X %08X \r\n", REG_OFS(Rx_IntMask_Clear  ), pRegs->Rx_IntMask_Clear     ));
    RETAILMSG(1, (L"  DMA_IntStat_Raw    : %08X %08X \r\n", REG_OFS(DMA_IntStat_Raw   ), pRegs->DMA_IntStat_Raw     ));
    RETAILMSG(1, (L"  DMA_IntStat_Masked : %08X %08X \r\n", REG_OFS(DMA_IntStat_Masked), pRegs->DMA_IntStat_Masked     ));
    RETAILMSG(1, (L"  DMA_IntMask_Set    : %08X %08X \r\n", REG_OFS(DMA_IntMask_Set   ), pRegs->DMA_IntMask_Set     ));
    RETAILMSG(1, (L"  DMA_IntMask_Clear  : %08X %08X \r\n", REG_OFS(DMA_IntMask_Clear ), pRegs->DMA_IntMask_Clear     ));

    RETAILMSG(1, (L" \r\n"));
    for (i=0; i<8; i++)
    RETAILMSG(1, (L"  RX_PendThresh[%d]   : %08X %08X \r\n", i, REG_OFS(RX_PendThresh[i]), pRegs->RX_PendThresh[i] ));
                                                                
    RETAILMSG(1, (L" \r\n"));                                   
    for (i=0; i<8; i++)                                         
    RETAILMSG(1, (L"  RX_FreeBuffer[%d]   : %08X %08X \r\n", i, REG_OFS(RX_FreeBuffer[i]), pRegs->RX_FreeBuffer[i] ));
}


void Cpsw3g_stats_reg_dump(PCPSW3G_REGS pRegs)
{
    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  STATS:\r\n"));
    RETAILMSG(1, (L"  RxGoodFrames       : %08X %08X \r\n", REG_OFS(RxGoodFrames) , pRegs->RxGoodFrames));
    RETAILMSG(1, (L"  RxBcastFrames      : %08X %08X \r\n", REG_OFS(RxBcastFrames), pRegs->RxBcastFrames));
    RETAILMSG(1, (L"  RxMcastFrames      : %08X %08X \r\n", REG_OFS(RxMcastFrames), pRegs->RxMcastFrames));
    RETAILMSG(1, (L"  RxPauseFrames      : %08X %08X \r\n", REG_OFS(RxPauseFrames), pRegs->RxPauseFrames));
    RETAILMSG(1, (L"  RxCRCErrors        : %08X %08X \r\n", REG_OFS(RxCRCErrors)  , pRegs->RxCRCErrors));
    RETAILMSG(1, (L"  RxAlignCodeErrors  : %08X %08X \r\n", REG_OFS(RxAlignCodeErrors), pRegs->RxAlignCodeErrors));
    RETAILMSG(1, (L"  RxOversizedFrames  : %08X %08X \r\n", REG_OFS(RxOversizedFrames), pRegs->RxOversizedFrames));
    RETAILMSG(1, (L"  RxJabberFrames     : %08X %08X \r\n", REG_OFS(RxJabberFrames),    pRegs->RxJabberFrames));
    RETAILMSG(1, (L"  RxUndersizedFrames : %08X %08X \r\n", REG_OFS(RxUndersizedFrames),pRegs->RxUndersizedFrames));
    RETAILMSG(1, (L"  RxFragments        : %08X %08X \r\n", REG_OFS(RxFragments),       pRegs->RxFragments));
    RETAILMSG(1, (L"  RxOctets           : %08X %08X \r\n", REG_OFS(RxOctets),          pRegs->RxOctets));
    RETAILMSG(1, (L"  TxGoodFrames       : %08X %08X \r\n", REG_OFS(TxGoodFrames) , pRegs->TxGoodFrames));
    RETAILMSG(1, (L"  TxBcastFrames      : %08X %08X \r\n", REG_OFS(TxBcastFrames), pRegs->TxBcastFrames));
    RETAILMSG(1, (L"  TxMcastFrames      : %08X %08X \r\n", REG_OFS(TxMcastFrames), pRegs->TxMcastFrames));
    RETAILMSG(1, (L"  TxPauseFrames      : %08X %08X \r\n", REG_OFS(TxPauseFrames), pRegs->TxPauseFrames));
    RETAILMSG(1, (L"  TxDeferredFrames   : %08X %08X \r\n", REG_OFS(TxDeferredFrames),  pRegs->TxDeferredFrames));
    RETAILMSG(1, (L"  TxCollisionFrames  : %08X %08X \r\n", REG_OFS(TxCollisionFrames), pRegs->TxCollisionFrames));
    RETAILMSG(1, (L"  TxSinglecollFrames : %08X %08X \r\n", REG_OFS(TxSinglecollFrames),pRegs->TxSinglecollFrames));
    RETAILMSG(1, (L"  TxMulticollFrames  : %08X %08X \r\n", REG_OFS(TxMulticollFrames), pRegs->TxMulticollFrames));
    RETAILMSG(1, (L"  TxExcessCollision  : %08X %08X \r\n", REG_OFS(TxExcessiveCollision), pRegs->TxExcessiveCollision));
    RETAILMSG(1, (L"  TxLateCollision    : %08X %08X \r\n", REG_OFS(TxLateCollision), pRegs->TxLateCollision));
    RETAILMSG(1, (L"  TxUnderrun         : %08X %08X \r\n", REG_OFS(TxUnderrun),      pRegs->TxUnderrun));
    RETAILMSG(1, (L"  TxCarrierSenseError: %08X %08X \r\n", REG_OFS(TxCarrierSenseError), pRegs->TxCarrierSenseError));
    RETAILMSG(1, (L"  TxOcets            : %08X %08X \r\n", REG_OFS(TxOcets)    , pRegs->TxOcets));
    RETAILMSG(1, (L"  Frame64            : %08X %08X \r\n", REG_OFS(Frame64)    , pRegs->Frame64));
    RETAILMSG(1, (L"  Frame65t127        : %08X %08X \r\n", REG_OFS(Frame65t127), pRegs->Frame65t127));
    RETAILMSG(1, (L"  Frame128t255       : %08X %08X \r\n", REG_OFS(Frame128t255),pRegs->Frame128t255));
    RETAILMSG(1, (L"  Frame256t511       : %08X %08X \r\n", REG_OFS(Frame256t511),pRegs->Frame256t511));
    RETAILMSG(1, (L"  Frame512t1023      : %08X %08X \r\n", REG_OFS(Frame512t1023), pRegs->Frame512t1023));
    RETAILMSG(1, (L"  Frame1024tup       : %08X %08X \r\n", REG_OFS(Frame1024tup),  pRegs->Frame1024tup));
    RETAILMSG(1, (L"  NetOctets          : %08X %08X \r\n", REG_OFS(NetOctets),     pRegs->NetOctets));
    RETAILMSG(1, (L"  RxSofOverruns      : %08X %08X \r\n", REG_OFS(RxSofOverruns), pRegs->RxSofOverruns));
    RETAILMSG(1, (L"  RxMofOverruns      : %08X %08X \r\n", REG_OFS(RxMofOverruns), pRegs->RxMofOverruns));
    RETAILMSG(1, (L"  RxDmaOverruns      : %08X %08X \r\n", REG_OFS(RxDmaOverruns), pRegs->RxDmaOverruns));

}


void Cpsw3g_stateram_reg_dump(PCPSW3G_REGS pRegs)
{
    int i;

    RETAILMSG(1, (L" \r\n"));                                   
    RETAILMSG(1, (L"  STATE RAM:\r\n"));                                   
    for (i=0; i<8; i++)                                         
    RETAILMSG(1, (L"  Tx_HDP[%d]          : %08X %08X \r\n", i, REG_OFS(Tx_HDP[i]), pRegs->Tx_HDP[i] ));
                                                                            
    RETAILMSG(1, (L" \r\n"));                                               
    for (i=0; i<8; i++)                                                     
    RETAILMSG(1, (L"  Rx_HDP[%d]          : %08X %08X \r\n", i, REG_OFS(Rx_HDP[i]), pRegs->Rx_HDP[i] ));
                                                                            
    RETAILMSG(1, (L" \r\n"));                                               
    for (i=0; i<8; i++)                                                     
    RETAILMSG(1, (L"  Tx_CP[%d]           : %08X %08X \r\n", i, REG_OFS(Tx_CP[i]), pRegs->Tx_CP[i] ));
                                                                            
    RETAILMSG(1, (L" \r\n"));                                               
    for (i=0; i<8; i++)                                                     
    RETAILMSG(1, (L"  Rx_CP[%d]           : %08X %08X \r\n", i, REG_OFS(Rx_CP[i]), pRegs->Rx_CP[i] ));
}


void Cpsw3g_ale_reg_dump(PCPSW3G_REGS pRegs)
{
    int i;

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  ALE:\r\n"));
    RETAILMSG(1, (L"  ALE_IdVer          : %08X %08X \r\n", REG_OFS(ALE_IdVer       ), pRegs->ALE_IdVer ));
    RETAILMSG(1, (L"  ALE_Control        : %08X %08X \r\n", REG_OFS(ALE_Control     ), pRegs->ALE_Control ));
    RETAILMSG(1, (L"  ALE_PreScale       : %08X %08X \r\n", REG_OFS(ALE_PreScale    ), pRegs->ALE_PreScale ));
    RETAILMSG(1, (L"  ALE_Unknown_VLAN   : %08X %08X \r\n", REG_OFS(ALE_Unknown_VLAN), pRegs->ALE_Unknown_VLAN ));
    RETAILMSG(1, (L"  ALE_TblCtl         : %08X %08X \r\n", REG_OFS(ALE_TblCtl      ), pRegs->ALE_TblCtl ));

    for (i=0; i<3; i++)
    RETAILMSG(1, (L"  ALE_Tbl[%d]         : %08X %08X \r\n", i, REG_OFS(ALE_Tbl[i]), pRegs->ALE_Tbl[i] ));

    for (i=0; i<6; i++)
    RETAILMSG(1, (L"  ALE_PortCtl[%d]     : %08X %08X \r\n", i, REG_OFS(ALE_PortCtl[i]), pRegs->ALE_PortCtl[i] ));
}


void Cpsw3g_cpts_reg_dump(PCPSW3G_REGS pRegs)
{
    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  CPTS:\r\n"));
    RETAILMSG(1, (L"  CPTS_IdVer         : %08X %08X \r\n", REG_OFS(CPTS_IdVer) , pRegs->CPTS_IdVer));
    RETAILMSG(1, (L"  CPTS_Control       : %08X %08X \r\n", REG_OFS(CPTS_Control) , pRegs->CPTS_Control));
    RETAILMSG(1, (L"  CPTS_RFTCLK_Sel    : %08X %08X \r\n", REG_OFS(CPTS_RFTCLK_Sel) , pRegs->CPTS_RFTCLK_Sel));
    RETAILMSG(1, (L"  CPTS_TS_Push       : %08X %08X \r\n", REG_OFS(CPTS_TS_Push) , pRegs->CPTS_TS_Push));
    RETAILMSG(1, (L"  CPTS_TS_Load_Val   : %08X %08X \r\n", REG_OFS(CPTS_TS_Load_Val) , pRegs->CPTS_TS_Load_Val));
    RETAILMSG(1, (L"  CPTS_TS_Load_en    : %08X %08X \r\n", REG_OFS(CPTS_TS_Load_en) , pRegs->CPTS_TS_Load_en));

    RETAILMSG(1, (L"  CPTS_Event_Pop     : %08X %08X \r\n", REG_OFS(CPTS_Event_Pop) , pRegs->CPTS_Event_Pop));
    RETAILMSG(1, (L"  CPTS_Event_low     : %08X %08X \r\n", REG_OFS(CPTS_Event_low) , pRegs->CPTS_Event_low));
    RETAILMSG(1, (L"  CPTS_Event_High    : %08X %08X \r\n", REG_OFS(CPTS_Event_High) , pRegs->CPTS_Event_High));
}

void Cpsw3g_cpgmac_sliver_reg_dump(CpgmacSl_Regs *pRegs, int idx)
{
    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  SLIVER %d:\r\n", idx));
    RETAILMSG(1, (L"  SL%d_IDVER          : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_IDVER      ), pRegs->SL_IDVER ));
    RETAILMSG(1, (L"  SL%d_MacControl     : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_MacControl ), pRegs->SL_MacControl ));
    RETAILMSG(1, (L"  SL%d_MacStatus      : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_MacStatus  ), pRegs->SL_MacStatus ));
    RETAILMSG(1, (L"  SL%d_Soft_Reset     : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_Soft_Reset ), pRegs->SL_Soft_Reset ));
    RETAILMSG(1, (L"  SL%d_Rx_Maxlen      : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_Rx_Maxlen  ), pRegs->SL_Rx_Maxlen  ));
    RETAILMSG(1, (L"  SL%d_BoffTest       : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_BoffTest   ), pRegs->SL_BoffTest   ));
    RETAILMSG(1, (L"  SL%d_Rx_Pause       : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_Rx_Pause   ), pRegs->SL_Rx_Pause   ));
    RETAILMSG(1, (L"  SL%d_Tx_Pause       : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_Tx_Pause   ), pRegs->SL_Tx_Pause   ));
    RETAILMSG(1, (L"  SL%d_EMControl      : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_EMControl  ), pRegs->SL_EMControl  ));
    RETAILMSG(1, (L"  SL%d_Rx_Pri_Map     : %08X %08X \r\n", idx, SL_REG_OFS(idx, SL_Rx_Pri_Map ), pRegs->SL_Rx_Pri_Map ));

}


void Cpsw3g_sw_stats_dump(PCPSW3G_ADAPTER pAdapter)
{
    CPSW3G_SW_STATS *pStats = &(pAdapter->Cpsw3g_sw_stats);

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  tx_in               : %08X \r\n", pStats->tx_in));
    RETAILMSG(1, (L"  tx_complete         : %08X \r\n", pStats->tx_complete));
    RETAILMSG(1, (L"  tx_out_of_descs     : %08X \r\n", pStats->tx_out_of_descs));
    RETAILMSG(1, (L"  tx_eoq              : %08X \r\n", pStats->tx_eoq));
    RETAILMSG(1, (L"  tx_false_eoq        : %08X \r\n", pStats->tx_false_eoq));
    RETAILMSG(1, (L"  tx_undersized       : %08X \r\n", pStats->tx_undersized));
    RETAILMSG(1, (L"  tx_teardown_discard : %08X \r\n", pStats->tx_teardown_discard));
    RETAILMSG(1, (L"  tx_copies           : %08X \r\n", pStats->tx_copies));
    RETAILMSG(1, (L"  tx_sw_vlan_process  : %08X \r\n", pStats->tx_sw_vlan_process));
    RETAILMSG(1, (L"  tx_oversize_pkt     : %08X \r\n", pStats->tx_oversize_pkt));

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  rx_in               : %08X \r\n", pStats->rx_in));
    RETAILMSG(1, (L"  rx_out_of_descs     : %08X \r\n", pStats->rx_out_of_descs));
    RETAILMSG(1, (L"  rx_complete         : %08X \r\n", pStats->rx_complete));
    RETAILMSG(1, (L"  rx_eoq              : %08X \r\n", pStats->rx_eoq));
    RETAILMSG(1, (L"  rx_teardown_discard : %08X \r\n", pStats->rx_teardown_discard));
    RETAILMSG(1, (L"  rx_copies           : %08X \r\n", pStats->rx_copies));
    RETAILMSG(1, (L"  rx_indicated        : %08X \r\n", pStats->rx_indicated));
    RETAILMSG(1, (L"  rx_vlan_tagged_pkt  : %08X \r\n", pStats->rx_vlan_tagged_pkt));
    RETAILMSG(1, (L"  rx_out_of_pkt       : %08X \r\n", pStats->rx_out_of_pkt));
    RETAILMSG(1, (L"  rx_invalid_desc     : %08X \r\n", pStats->rx_invalid_desc));

}

void Cpsw3g_ss_reg_dump(PCPSW3G_ADAPTER pAdapter)
{
    PCPSW_SS_REGS pSsRegs = pAdapter->pSsRegsBase;

    RETAILMSG(1, (L" \r\n"));
    RETAILMSG(1, (L"  SS:\r\n"));
    RETAILMSG(1, (L"  IDVER               : %08X %08X \r\n", SS_REG_OFS(IDVER), pSsRegs->IDVER));
    RETAILMSG(1, (L"  Soft_Reset          : %08X %08X \r\n", SS_REG_OFS(Soft_Reset), pSsRegs->Soft_Reset));
    RETAILMSG(1, (L"  Control             : %08X %08X \r\n", SS_REG_OFS(Control  ), pSsRegs->Control));
    RETAILMSG(1, (L"  Int_Control         : %08X %08X \r\n", SS_REG_OFS(Int_Control  ), pSsRegs->Int_Control));

    RETAILMSG(1, (L"  C0_Rx_Thresh_En     : %08X %08X \r\n", SS_REG_OFS(C0_Rx_Thresh_En  ), pSsRegs->C0_Rx_Thresh_En));
    RETAILMSG(1, (L"  C0_Rx_En            : %08X %08X \r\n", SS_REG_OFS(C0_Rx_En         ), pSsRegs->C0_Rx_En));
    RETAILMSG(1, (L"  C0_Tx_En            : %08X %08X \r\n", SS_REG_OFS(C0_Tx_En         ), pSsRegs->C0_Tx_En));
    RETAILMSG(1, (L"  C0_Misc_En          : %08X %08X \r\n", SS_REG_OFS(C0_Misc_En       ), pSsRegs->C0_Misc_En));
                                            
    RETAILMSG(1, (L"  C0_Rx_Thresh_STAT   : %08X %08X \r\n", SS_REG_OFS(C0_Rx_Thresh_STAT), pSsRegs->C0_Rx_Thresh_STAT));
    RETAILMSG(1, (L"  C0_Rx_STAT          : %08X %08X \r\n", SS_REG_OFS(C0_Rx_STAT       ), pSsRegs->C0_Rx_STAT));
    RETAILMSG(1, (L"  C0_Tx_STAT          : %08X %08X \r\n", SS_REG_OFS(C0_Tx_STAT       ), pSsRegs->C0_Tx_STAT));
    RETAILMSG(1, (L"  C0_Misc_STAT        : %08X %08X \r\n", SS_REG_OFS(C0_Misc_STAT     ), pSsRegs->C0_Misc_STAT));

    RETAILMSG(1, (L"  C0_Rx_Imax          : %08X %08X \r\n", SS_REG_OFS(C0_Rx_Imax     ), pSsRegs->C0_Rx_Imax));
    RETAILMSG(1, (L"  C0_Tx_Imax          : %08X %08X \r\n", SS_REG_OFS(C0_Tx_Imax     ), pSsRegs->C0_Tx_Imax));
    RETAILMSG(1, (L"  RGMII_CTL           : %08X %08X \r\n", SS_REG_OFS(RGMII_CTL      ), pSsRegs->RGMII_CTL));
}

void Cpsw3g_reg_dump(PCPSW3G_ADAPTER pAdapter)
{
    int i;

    PCPSW3G_REGS pRegs = pAdapter->pCpsw3gRegsBase;

    RETAILMSG(1, (L"  CPSW_IdVer         : %08X %08X \r\n", REG_OFS(CPSW_IdVer), pRegs->CPSW_IdVer         ));
    RETAILMSG(1, (L"  CPSW_Control       : %08X %08X \r\n", REG_OFS(CPSW_Control), pRegs->CPSW_Control     ));
    RETAILMSG(1, (L"  CPSW_Soft_Reset    : %08X %08X \r\n", REG_OFS(CPSW_Soft_Reset), pRegs->CPSW_Soft_Reset     ));
    RETAILMSG(1, (L"  CPSW_Stat_Port_En  : %08X %08X \r\n", REG_OFS(CPSW_Stat_Port_En), pRegs->CPSW_Stat_Port_En));
    RETAILMSG(1, (L"  CPSW_Pritype       : %08X %08X \r\n", REG_OFS(CPSW_Pritype), pRegs->CPSW_Pritype     ));
    
    RETAILMSG(1, (L"  CPSW_Soft_Idle     : %08X %08X \r\n", REG_OFS(CPSW_Soft_Idle), pRegs->CPSW_Soft_Idle     ));
    RETAILMSG(1, (L"  CPSW_Thru_Rate     : %08X %08X \r\n", REG_OFS(CPSW_Thru_Rate), pRegs->CPSW_Thru_Rate     ));
    RETAILMSG(1, (L"  CPSW_Gap_Thresh    : %08X %08X \r\n", REG_OFS(CPSW_Gap_Thresh), pRegs->CPSW_Gap_Thresh     ));
    RETAILMSG(1, (L"  CPSW_Tx_Start_WDS  : %08X %08X \r\n", REG_OFS(CPSW_Tx_Start_WDS), pRegs->CPSW_Tx_Start_WDS ));
    RETAILMSG(1, (L"  CPSW_Flow_Control  : %08X %08X \r\n", REG_OFS(CPSW_Flow_Control), pRegs->CPSW_Flow_Control ));

#ifdef AM33X_BSP 
    RETAILMSG(1, (L"  CPSW_VLAN_Ltype    : %08X %08X \r\n", REG_OFS(CPSW_VLAN_Ltype), pRegs->CPSW_VLAN_Ltype ));
    RETAILMSG(1, (L"  CPSW_TS_Ltype      : %08X %08X \r\n", REG_OFS(CPSW_TS_Ltype), pRegs->CPSW_TS_Ltype ));
    RETAILMSG(1, (L"  CPSW_DLR_Ltype     : %08X %08X \r\n", REG_OFS(CPSW_DLR_Ltype), pRegs->CPSW_DLR_Ltype ));
#endif // AM33X_BSP
    
    Cpsw3g_port_reg_dump(pRegs);

    Cpsw3g_dma_reg_dump(pRegs);
                                                                 
    Cpsw3g_stats_reg_dump(pRegs);

    Cpsw3g_stateram_reg_dump(pRegs);

    Cpsw3g_cpts_reg_dump(pRegs);

    Cpsw3g_ale_reg_dump(pRegs);

    Cpsw3g_cpgmac_sliver_reg_dump(&(pRegs->SL_Regs[0]), 1);
    Cpsw3g_cpgmac_sliver_reg_dump(&(pRegs->SL_Regs[1]), 2);

    Cpsw3g_ss_reg_dump(pAdapter);

    Cpsw3g_sw_stats_dump(pAdapter);
}

#endif // DEBUG_REG_DUMP

