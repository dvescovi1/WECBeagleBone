//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Init.c
//! \brief Contains initialisation functions for NDIS..
//!
//! This source File contains the adapter block creation and has NDIS called
//! initialisation function which sets all the characteristics of miniport driver.
//!
//! \version  1.00 Aug 22nd 2006 File Created

// Includes
#include <Ndis.h>
#include <oal.h>
#include <oalex.h>
#include "Emac_Adapter.h"
#include "Emac_Proto.h"
#include "soc_cfg.h"
#include "sdk_padcfg.h"

//! \brief Global  64 Bit Physical Address initialised to all F's
NDIS_PHYSICAL_ADDRESS g_HighestAcceptedMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

//========================================================================
//!  \fn NDIS_STATUS EmacAllocAdapterBlock(PMINIPORT_ADAPTER* pAdapter)
//!  \brief Allocate MINIPORT_ADAPTER  data block and do some initialization
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
NDIS_STATUS
EmacAllocAdapterBlock(
    PMINIPORT_ADAPTER* pAdapter
    )

{
    PMINIPORT_ADAPTER     pTempAdapter;
    NDIS_STATUS           Status;

    DEBUGMSG(DBG_FUNC, (L"--> NICAllocAdapter\r\n"));

    *pAdapter = NULL;

    /* Allocate memory for the adapter block now. */

    Status = NdisAllocateMemory(
                (PVOID *)&pTempAdapter,
                sizeof(MINIPORT_ADAPTER),
                0,              /* System space cached memory */
                g_HighestAcceptedMax);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, (L"EMAC:Initialize: NdisAllocateMemory(EMAC) failed\r\n"));
    }

    /* Clear out the adapter block, which sets all default values to FALSE,
     * or NULL.
     */

    NdisZeroMemory (pTempAdapter, sizeof(MINIPORT_ADAPTER));

    /* Allocating the spin locks */

    NdisAllocateSpinLock(&pTempAdapter->m_Lock);
    NdisAllocateSpinLock(&pTempAdapter->m_SendLock);
    NdisAllocateSpinLock(&pTempAdapter->m_RcvLock);


    *pAdapter = pTempAdapter;

    DEBUGMSG(DBG_FUNC, (L"<-- NICAllocAdapter Status=%x\r\n", Status));

    return Status;

}

//========================================================================
//!  \fn NDIS_STATUS EmacFreeAdapter(PMINIPORT_ADAPTER   *pAdapter)
//!  \brief Fress MINIPORT_ADAPTER  data block and all allocated memories.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return 
//========================================================================
VOID
EmacFreeAdapter(
        PMINIPORT_ADAPTER  pAdapter
    )
{
    PEMAC_RXPKTS    pNextPkt;
    PEMAC_RXPKTS    pCurPkt;
	PEMAC_RXBUFS	pNextBuf;
	PEMAC_RXBUFS	pCurBuf;
    UINT            Count;

    DEBUGMSG(DBG_FUNC, (L"--> EmacFreeAdapter\r\n"));

    /*
    * Check if the Interrupt has been registered, if so then deregister the
    * interrupt for the adapter. Rest, all deallocation is done in reverse way
    */
    if(pAdapter->m_HwStatus != NdisHardwareStatusReset)
    {
        if(pAdapter->m_RxIntrVector)
        {
            NdisMDeregisterInterrupt(&pAdapter->m_RxIntrInfo);
        }
        if(pAdapter->m_TxIntrVector)
        {
            NdisMDeregisterInterrupt(&pAdapter->m_TxIntrInfo);
        }
        if(pAdapter->m_LinkIntrVector)
        {
            NdisMDeregisterInterrupt(&pAdapter->m_LinkIntrInfo);
        }
    }

	pNextBuf = pAdapter->m_pBaseRxBufs;
	for (Count = 0; Count < pAdapter->m_NumEmacRxBufDesc ; Count++)
	{
		pCurBuf = pNextBuf;

		/* Free buffers from buffer pool */
		NdisFreeBuffer((PNDIS_BUFFER)pNextBuf->m_BufHandle);

		pNextBuf->m_BufHandle = NULL;

		pNextBuf++;
		pCurBuf->m_pNext = pNextBuf;
	}

    if(NULL != pAdapter->m_pBaseRxPkts)
    {
        pNextPkt = pAdapter->m_pBaseRxPkts;
        for (Count = 0; Count < pAdapter->m_NumRxIndicatePkts ; Count++)
        {
            pCurPkt = pNextPkt;

            /* Free packets from packet pool */
            NdisFreePacket((PNDIS_PACKET)pNextPkt->m_PktHandle);

            pNextPkt->m_PktHandle = NULL;

            pNextPkt++;
            pCurPkt->m_pNext = pNextPkt;
        }
    }

    if(NULL != pAdapter->m_RecvPacketPool)
    {
        NdisFreePacketPool(pAdapter->m_RecvPacketPool);
        pAdapter->m_RecvPacketPool = NULL;
    }

    NdisFreeBufferPool(pAdapter->m_RecvBufferPool);
    pAdapter->m_RecvBufferPool = NULL;

	if(pAdapter->m_HwStatus != NdisHardwareStatusReset)
	{
		if(NULL != pAdapter->m_pBaseRxPkts)
		{
			NdisFreeMemory(pAdapter->m_pBaseRxPkts,
						pAdapter->m_NumRxIndicatePkts * sizeof(EMAC_RXPKTS),
						0);

			pAdapter->m_pBaseRxPkts = NULL;
		}

		if(NULL != pAdapter->m_pBaseRxBufs)
		{
			NdisFreeMemory(pAdapter->m_pBaseRxBufs,
					pAdapter->m_NumEmacRxBufDesc * sizeof(EMAC_RXBUFS),
					0);

			pAdapter->m_pBaseRxBufs = NULL;
		}

		if(NULL != (VOID *)pAdapter->m_RxBufsBase)
		{
			NdisMUnmapIoSpace(pAdapter->m_AdapterHandle,
					(PVOID)pAdapter->m_RxBufsBase,
					(pAdapter->m_NumEmacRxBufDesc * EMAC_MAX_PKT_BUFFER_SIZE)
					);

			pAdapter->m_RxBufsBase = 0;
		}

		if(NULL != pAdapter->m_pBaseTxPkts)
		{
			NdisFreeMemory(pAdapter->m_pBaseTxPkts,
					pAdapter->m_MaxPacketPerTransmit * sizeof(EMAC_TXPKT),
					0);

			pAdapter->m_pBaseTxPkts = NULL;
		}

		if(NULL != pAdapter->m_pBaseTxBufs)
		{
			NdisFreeMemory(pAdapter->m_pBaseTxBufs,
				pAdapter->m_MaxTxEmacBufs * sizeof(EMAC_TXBUF),
				0);

			pAdapter->m_pBaseTxBufs = NULL;
		}

		if(NULL != (VOID *)pAdapter->m_TxBufBase)
		{
			NdisMUnmapIoSpace(pAdapter->m_AdapterHandle,
					(PVOID)pAdapter->m_TxBufBase,
					(EMAC_MAX_TX_BUFFERS * EMAC_MAX_ETHERNET_PKT_SIZE)
					);

			pAdapter->m_TxBufBase = 0;
		}

		NdisFreeSpinLock(&pAdapter->m_Lock);
		NdisFreeSpinLock(&pAdapter->m_SendLock);
		NdisFreeSpinLock(&pAdapter->m_RcvLock);

		if(pAdapter->m_EmacIRamBase)
		{
			NdisMUnmapIoSpace(pAdapter->m_AdapterHandle,
							  (PVOID)pAdapter->m_EmacIRamBase,
							  g_EmacMemLayout.EMAC_TOTAL_MEMORY);

			pAdapter->m_EmacIRamBase = 0;
		}

		/* Finally free adapter memory itself  */
		NdisFreeMemory((PVOID)pAdapter,
					sizeof(MINIPORT_ADAPTER),
					0);
	}

    DEBUGMSG(DBG_FUNC, (L"<-- EmacFreeAdapter \r\n"));
}

//========================================================================
//!  \fn BOOL EmacReadEEProm( PMINIPORT_ADAPTER  pAdapter)
//!  \brief This routine fetches the Mac address from the EEProm
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return BOOL Either TRUE or FALSE.
//========================================================================
BOOL
EmacReadEEProm(
    PMINIPORT_ADAPTER  pAdapter
)
{
    
    BOOL    RetVal = FALSE;
    UINT8   ReadBuffer[ETH_LENGTH_OF_ADDRESS] = {0x20,0x20,0x30,0x30,0x40,0x40};    
    DWORD   dwKernelRet = 0;

    /* Get the MAC address from the kernel */
    if (!KernelIoControl(IOCTL_HAL_GET_MAC_ADDRESS,
                         NULL, 0, &ReadBuffer, sizeof(ReadBuffer), &dwKernelRet))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read MAC address\r\n")));
        return RetVal;
    }

    /* Fill to Adapter structure */
    memcpy(pAdapter->m_MACAddress, ReadBuffer, ETH_LENGTH_OF_ADDRESS);

    RETAILMSG( TRUE,(TEXT("Adapter's MAC address is %02X:%02X:%02X:%02X:%02X:%02X\r\n"),
                ReadBuffer[0], ReadBuffer[1], ReadBuffer[2],
                ReadBuffer[3], ReadBuffer[4], ReadBuffer[5] ));
   
    RetVal = TRUE;

   return (RetVal);
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
    PMINIPORT_ADAPTER     pAdapter,
    NDIS_HANDLE           WrapperConfigurationContext
    )

{
    DWORD                               phys = 0;
    DWORD                               dwEmacIndex = 0;
    NDIS_STATUS                         Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE                         ConfigurationHandle;
    PNDIS_CONFIGURATION_PARAMETER       pReturnedValue;
    NDIS_STRING                         EmacDeviceRegName = NDIS_STRING_CONST("EmacDevice");
    NDIS_STRING                         RxIntRegName = NDIS_STRING_CONST("RxIntrNum");
    NDIS_STRING                         TxIntRegName = NDIS_STRING_CONST("TxIntrNum");
    NDIS_STRING                         LinkIntRegName = NDIS_STRING_CONST("LinkIntrNum");
    NDIS_STRING                         PhysBufRegName = NDIS_STRING_CONST("EMACBufferBase");

    DEBUGMSG(DBG_FUNC, (L"--> NICReadAdapterInfo\r\n"));

    /* Open the registry for this adapter */
    NdisOpenConfiguration(
        &Status,
        &ConfigurationHandle,
        WrapperConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, (L"NdisOpenConfiguration failed\r\n"));
        return Status;
    }
 
    /* Get the configuration value for a specific parameter for EMAC device number */    
    NdisReadConfiguration(&Status, &pReturnedValue, ConfigurationHandle,
                          &EmacDeviceRegName, NdisParameterInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {        
        dwEmacIndex = pReturnedValue->ParameterData.IntegerData;
    }
    pAdapter->m_device = SOCGetEMACDevice(dwEmacIndex);

#if 0
    /* Get the configuration value for a specific parameter for Interrupt number */
    NdisReadConfiguration(&Status, &pReturnedValue, ConfigurationHandle,
                          &RxIntRegName, NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERRORMSG(TRUE, (L"Failed to read Interrupt number for EMAC.\r\n"));
        return Status;
    }
    else
    {
        RETAILMSG(FALSE, (L"NICReadAdapterInfo: Successfull read Int no.\r\n"));
    }

     /* Populate the interrupt vector */
    pAdapter->m_RxIntrVector = (USHORT)pReturnedValue->ParameterData.IntegerData;
    
    NdisReadConfiguration(&Status, &pReturnedValue, ConfigurationHandle,
                          &TxIntRegName, NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERRORMSG(TRUE, (L"Failed to read Interrupt number for EMAC.\r\n"));
        return Status;
    }
    else
    {
        RETAILMSG(FALSE, (L"NICReadAdapterInfo: Successfull read Int no.\r\n"));
    }

     /* Populate the interrupt vector */
    pAdapter->m_TxIntrVector = (USHORT)pReturnedValue->ParameterData.IntegerData;
    
    NdisReadConfiguration(&Status, &pReturnedValue, ConfigurationHandle,
                          &LinkIntRegName, NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERRORMSG(TRUE, (L"Failed to read Interrupt number for EMAC.\r\n"));
        return Status;
    }
    else
    {
        RETAILMSG(FALSE, (L"NICReadAdapterInfo: Successfull read Int no.\r\n"));
    }

     /* Populate the interrupt vector */
    pAdapter->m_LinkIntrVector = (USHORT)pReturnedValue->ParameterData.IntegerData;
#else
    pAdapter->m_RxIntrVector = (USHORT) GetIrqByDevice(pAdapter->m_device,L"RX");
    pAdapter->m_TxIntrVector = (USHORT) GetIrqByDevice(pAdapter->m_device,L"TX");
    pAdapter->m_LinkIntrVector = (USHORT) GetIrqByDevice(pAdapter->m_device,L"MISC");
#endif

    /* Get the configuration value for a specific parameter for physical buffers */
    NdisReadConfiguration(&Status, &pReturnedValue, ConfigurationHandle,
                          &PhysBufRegName, NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        PVOID* pv = NULL;        

        // TODO : Currently hardcoded, should be replace with a real computation
        pv = AllocPhysMem(768*1024, PAGE_READWRITE, 0, 0,&phys);
        if (pv)
        {                
            RETAILMSG(FALSE, (L"NICReadAdapterInfo: Allocated Buffers 0x%X.\r\n", (DWORD) phys));
            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            RETAILMSG(FALSE, (L"NICReadAdapterInfo:Unable to get physicall buffers\r\n"));
        }
    }
    else
    {
        RETAILMSG(FALSE, (L"NICReadAdapterInfo: Successfull read phys base as 0x%X.\r\n",
                         (DWORD) pReturnedValue->ParameterData.IntegerData));
        phys = (DWORD) pReturnedValue->ParameterData.IntegerData;
    }

    /* Populate the local structures with the Buffer base read from registry */
    NdisSetPhysicalAddressLow (pAdapter->m_RxBufsBasePa,
                               phys);
    NdisSetPhysicalAddressHigh (pAdapter->m_RxBufsBasePa, 0);

    NdisSetPhysicalAddressLow (pAdapter->m_TxBufBasePa,
                               (phys +
                                (EMAC_MAX_RXBUF_DESCS * (EMAC_MAX_PKT_BUFFER_SIZE))));
    NdisSetPhysicalAddressHigh (pAdapter->m_TxBufBasePa, 0);

    DEBUGMSG(DBG_FUNC, (L"pAdapter->m_TxBufBasePa=%p\r\n", pAdapter->m_TxBufBasePa));
    DEBUGMSG(DBG_FUNC, (L"pAdapter->m_RxBufsBasePa=%p\r\n", pAdapter->m_RxBufsBasePa));

     /* Close the registry */
    NdisCloseConfiguration(ConfigurationHandle);

    /* Read MAC information stored in EEPROM */

    if(FALSE == EmacReadEEProm (pAdapter))
    {
        DEBUGMSG(DBG_FUNC, (L"Unable to read MAC address from Adapter \r\n"));
        return (NDIS_STATUS_FAILURE);
    }

    /* Filling Rx information */
    pAdapter->m_NumEmacRxBufDesc        =  EMAC_MAX_RXBUF_DESCS;
    pAdapter->m_NumRxIndicatePkts       =  NDIS_INDICATE_PKTS;

    /* Filling Tx information */
    pAdapter->m_MaxPacketPerTransmit    = MAX_NUM_PACKETS_PER_SEND;
    pAdapter->m_MaxTxEmacBufs           = EMAC_MAX_TXBUF_DESCS;


    DEBUGMSG(DBG_FUNC, (L"<-- NICReadAdapterInfo, Status=%x\r\n", Status));

    return (Status);
}

//========================================================================
//!  \fn UINT32 NICSelfTest(PMP_ADAPTER Adapter)
//!  \brief Does a self test on controller for Rx/Tx before giving NDIS.
//!  \param pAdapter PMINIPORT_ADAPTER Pointer to receive pointer to our adapter
//!  \return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//========================================================================
UINT32
NICSelfTest(
    MINIPORT_ADAPTER *Adapter
    )
{
    /* TBD */

    return (NDIS_STATUS_SUCCESS);
}

//========================================================================
//!  \fn NDIS_STATUS Emac_MiniportInitialize(PNDIS_STATUS    OpenErrorStatus,
//!                                          PUINT           SelectedMediumIndex,
//!                                          PNDIS_MEDIUM    MediumArray,
//!                                          UINT            MediumArraySize,
//!                                          NDIS_HANDLE     MiniportAdapterHandle,
//!                                          NDIS_HANDLE     WrapperConfigurationContext
//!  \brief This function is a required function that sets up a network adapter,
//!         for network I/O operations, claims all hardware resources necessary
//!         to the network adapter in the registry, and allocates resources the
//!         driver needs to carry out network I/O operations.
//!  \param OpenErrorStatus PNDIS_STATUS Additional error status, if return value is error
//!  \param SelectedMediumIndex PUINT Specifies the medium type the driver or its
//!         network adapter uses
//!  \param MediumArray PNDIS_MEDIUM  Specifies an array of NdisMediumXXX values
//!         from which MiniportInitialize selects one that its network adapter supports
//!         or that the driver supports as an interface to higher-level drivers.
//!  \param MediumArraySize UINT Specifies the number of elements at MediumArray
//!  \param MiniportAdapterHandle NDIS_HANDLE Specifies a handle identifying the
//!         miniport’s network adapter,which is assigned by the NDIS library
//!  \param WrapperConfigurationContext NDIS_HANDLE
//!  \return NDIS_STATUS Status values.
//========================================================================

NDIS_STATUS
Emac_MiniportInitialize(
    PNDIS_STATUS    OpenErrorStatus,
    PUINT           SelectedMediumIndex,
    PNDIS_MEDIUM    MediumArray,
    UINT            MediumArraySize,
    NDIS_HANDLE     MiniportAdapterHandle,
    NDIS_HANDLE     WrapperConfigurationContext
    )
{
    NDIS_STATUS       Status;
    PEMAC_ADAPTER     pAdapter = NULL;
    UINT              Index;

    DEBUGMSG (DBG_FUNC,(L"+Emac_MiniportInitialize()\r\n"));

     do
    {
        /* Find the media type we support, we
         * are checking for 802.3 MediaType.
         */
        for (Index = 0; Index < MediumArraySize; ++Index)
        {
            if (MediumArray[Index] == NIC_MEDIA_TYPE)
            {
                break;
            }
        }

        if (Index == MediumArraySize)
        {
            DEBUGMSG(DBG_WARN, (L"Expected media (%x) is not in MediumArray.\r\n",
                                                        NIC_MEDIA_TYPE));
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            break;
        }

        *SelectedMediumIndex = Index;


        /* Allocate Emac Miniport Adapter structure */

        Status = EmacAllocAdapterBlock(&pAdapter);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        pAdapter->m_AdapterHandle = MiniportAdapterHandle;

        /* Initialise Hardware status */
        pAdapter->m_HwStatus = NdisHardwareStatusNotReady;

        /* Inform NDIS of the attributes of our adapter.
         * This has to be done before calling NdisMRegisterXxx or NdisXxxx function
         * that depends on the information supplied to NdisMSetAttributesEx
         * e.g. NdisMAllocateMapRegisters
         */
        NdisMSetAttributes (
            MiniportAdapterHandle,
            (NDIS_HANDLE) pAdapter,
            TRUE,                    /* True since ours is Bus Master */
            NdisInterfaceInternal);  /* We have internal controller */

        /* Reading all relevant information from adapter
         * etc. and filling it in to our adapter structure
         */

        Status = NICReadAdapterInfo(
                    pAdapter,
                    WrapperConfigurationContext);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICReadAdapterInfo is failed.\r\n"));
            break;
        }
        /* Allocate all other memory blocks
         *
         */
        Status = NICMapAdapterRegs(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICMapAdapterRegs is failed.\r\n"));
            break;
        }

        /* Init send data structures */

        Status = NICInitSend(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICInitSend is failed.\r\n"));
            break;
        }
        /* Init receive data structures */

        Status = NICInitRecv(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICInitRecv is failed.\r\n"));
            break;
        }

        /* Disable interrupts here which is as soon as possible */

        Emac_MiniportDisableInterrupt(pAdapter);

         /*  Register the interrupt */

        Status = NdisMRegisterInterrupt(
                     &pAdapter->m_RxIntrInfo,
                     pAdapter->m_AdapterHandle,
                     pAdapter->m_RxIntrVector,
                     0,          //ignored
                     FALSE,      // RequestISR
                     FALSE,      // SharedInterrupt
                     0);         //ignored
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR, (L"Emac_MiniportInitialize: NdisMRegisterInterrupt(m_RxIntrVector) failed\r\n"));
            break;
        }

        if(pAdapter->m_TxIntrVector)
        {
            Status = NdisMRegisterInterrupt(
                         &pAdapter->m_TxIntrInfo,
                         pAdapter->m_AdapterHandle,
                         pAdapter->m_TxIntrVector,
                         0,          //ignored
                         FALSE,      // RequestISR
                         FALSE,      // SharedInterrupt
                         0);         //ignored
            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGMSG(DBG_ERR, (L"Emac_MiniportInitialize: NdisMRegisterInterrupt(m_TxIntrVector) failed\r\n"));
                break;
            }
        }

        if(pAdapter->m_LinkIntrVector)
        {
            Status = NdisMRegisterInterrupt(
                         &pAdapter->m_LinkIntrInfo,
                         pAdapter->m_AdapterHandle,
                         pAdapter->m_LinkIntrVector,
                         0,          //ignored
                         FALSE,      // RequestISR
                         FALSE,      // SharedInterrupt
                         0);         //ignored
            if (Status != NDIS_STATUS_SUCCESS)
            {
                DEBUGMSG(DBG_ERR, (L"Emac_MiniportInitialize: NdisMRegisterInterrupt(m_LinkIntrVector) failed\r\n"));
                break;
            }
        }

        /* About to initialise */

        pAdapter->m_HwStatus = NdisHardwareStatusInitializing;

        /* request and configure the pads used by the EMAC device*/
        if (!RequestDevicePads(pAdapter->m_device))
        {
            Status = NDIS_STATUS_FAILURE;
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: RequestDevicePads failed.\r\n"));
            break;
        }

        /* Init the hardware and set up everything */
        if (!EthHwInit())
        {
            Status = NDIS_STATUS_FAILURE;
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: EthHwInit failed.\r\n"));
            break;
        }

        Status = NICInitializeAdapter(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICInitializeAdapter is failed.\r\n"));
            break;
        }

        /* Test our adapter hardware */

        Status = NICSelfTest(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(TRUE, (L"Emac_MiniportInitialize: NICSelfTest is failed.\r\n"));
            break;
        }

        /* Test is successful , make a status transition */
        pAdapter->m_HwStatus = NdisHardwareStatusReady;

    } while (FALSE);

    if (pAdapter && Status != NDIS_STATUS_SUCCESS)
    {
       /* Free allocated memory and resources held */
       EmacFreeAdapter(pAdapter);
    }

    DEBUGMSG (DBG_FUNC,(L"<--Emac_MiniportInitialize()\r\n"));

    return Status;

}
