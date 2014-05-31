// All rights reserved ADENEO EMBEDDED 2010
/*****************************************************************************
   
    Copyright (c) 2004-2008 SMSC. All rights reserved.

    Use of this source code is subject to the terms of the SMSC Software
    License Agreement (SLA) under which you licensed this software product.  
    If you did not accept the terms of the SLA, you are not authorized to use
    this source code. 

    This code and information is provided as is without warranty of any kind,
    either expressed or implied, including but not limited to the implied
    warranties of merchantability and/or fitness for a particular purpose.
     
    File name   : smsc9118.c 
    Description : smsc9118 driver 

    History     :
        03-16-05 WH         First Release
        08-12-05 MDG        ver 1.01 
            - add LED1 inversion, add PHY work around
        11-07-05 WH         ver 1.02
            - Fixed middle buffer handling bug
              (Driver didn't handle middle buffers correctly if it is less than 
               4bytes size)
            - workaround for Multicast bug
            - Workaround for MAC RXEN bug
        11-17-05 WH         ver 1.03
            - 1.02 didn't have 1.01 patches
            - 1.03 is 1.02 + 1.01
        12-06-05 WH         ver 1.04
            - Fixed RX doesn't work on Silicon A1 (REV_ID = 0x011x0002)
            - Support SMSC9118x/117x/116x/115x family
        02-27-06 WH         ver 1.05
            - Fixing External Phy bug that doesn't work with 117x/115x
        03-23-06 WH         ver 1.06
            - Put the variable to avoid PHY_WORKAROUND for External PHY
            - Change product name to 9118x->9218, 9115x->9215
        07-26-06 WH, MDG, NL        ver 1.07
            - Add RXE and TXE interrupt handlers
            - Workaround Code for direct GPIO connection from 9118 family 
              Interrupt (Level Interrupt -> Edge Interrupt)
            - Change GPT interrupt interval to 200mSec from 50mSec
            - clean up un-used SH3 code
        08-25-06  WH, MDG, NL       ver 1.08
            - Fixed RXE and TXE interrupt handlers bug
            - support for direct and nondirect Interrupt
        02-15-07   NL               ver 1.09
            - First version of WinCE 6.0 driver
            - Removed Support for LAN9112
            - Added AutoMdix as modifiable parameter in the Registry
            - Fixed DMA Xmit Bug
        04-17-07   NL               ver 1.10
            - Added Support LAN9211 Chip
            - Changed Register Name ENDIAN to WORD_SWAP According to the Menual
            - Merged CE6.0 & 5.0 Drivers Together
        10-24-07   NL               ver 1.11
            - Added Support LAN9218A/LAN9217A/LAN9216A/LAN9215A Chips
        01-08-08   AH               ver 1.12
            - Added Support for LAN9210 Chip
        06-09-08   AH               ver 1.13
            - Added Support for LAN9220/21 Chip
        06-19-08   MDG              ver 1.13
            - Added IntCfg as modifiable parameter in the Registry
            - Fixed support for Fixed parameters LinkMode
            - Removed Support for early FPGA versions (OLD_REGISTERS)
        -----------------------------------------------------------------------
        09-26-08   WH
            - Move to version 2.00 intentionally
            - From version 2.00, 
               driver drops support chip ID of 0x011x0000 and 0x011x0001
        -----------------------------------------------------------------------
        09-26-08   WH               ver 2.00
            - replace TAB to SPACE
            - Reorder initialization routines to avoid 
               possible unexpect behavior
            - Fixed the issue Flow Control ignores "FlowControl" key 
               if not Auto Negotiation
            - Added "MinLinkChangeWakeUp", "MinMagicPacketWakeUp" and
               "MinPatternWakeUp" registry key for PM
            - Fixed PM issues
                - DHCP doesn't work after wakeup
                - Disable GPTimer while in sleep mode
                - Add routine to wakeup chip during Reset
            - Fix discarding Rx Frame when it is less than 16bytes
            - Enable RXE & RWT interrupt
            - Chip goes to D2(Energy Detect Power Down) when there is no link
               when POWERDOWN_AT_NO_LINK is defined (see smsc9118.h)
            - Clean up Registers which are absolete
            - See relnotes.txt for detail
        11-17-08   WH               ver 2.01
            - Lan_SetMiiRegW() and Lan_GetMiiRegW are changed to return BOOL.
              Caller checks return value to detect error
            - Tx Error Interrupt is enabled
            - Check return from NdisAllocateMemory() is 32bit aligned or not
              Technically it won't happen though
              See comments in source (smsc9118.c)
            - Fixed bug which does't go to AUTOIP when failed to get DHCP addr
		-----------------------------------------------------------------------
		07-24-09   RL
			- Added support for OMAP platform
*****************************************************************************/

#pragma warning(disable:4127 4201 4214 4115 4100 4514 4152 6011)
#include "smsc9118.h"

static const char date_code[] = BUILD_NUMBER;

static void DumpSIMRegs(const SMSC9118_ADAPTER * const pAdapter);
static void DumpMACRegs(const SMSC9118_ADAPTER * const pAdapter);
static void DumpPHYRegs(const SMSC9118_ADAPTER * const pAdapter);
static void DumpAllRegs(const SMSC9118_ADAPTER * const pAdapter);

static DRIVER_BLOCK gSmsc9118MiniportBlock = { (NDIS_HANDLE)0, NULL };  /*lint !e956 */

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("SMSC9118"), {
    TEXT("Errors"),TEXT("Warnings"),TEXT("Functions"),TEXT("Init"),
    TEXT("Interrupts"),TEXT("Receives"),TEXT("Transmits"),TEXT("Link"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
    0
};
#endif

#define OID_NDIS_SMSC_DUMP_ALL_REGS     0xFFFFFF01U

static const UINT Smsc9118SupportedOids[] = {
    (UINT)OID_GEN_SUPPORTED_LIST,
    (UINT)OID_GEN_HARDWARE_STATUS,
    (UINT)OID_GEN_MEDIA_SUPPORTED,
    (UINT)OID_GEN_MEDIA_IN_USE,
    (UINT)OID_GEN_MAXIMUM_LOOKAHEAD,
    (UINT)OID_GEN_MAXIMUM_FRAME_SIZE,
    (UINT)OID_GEN_MAXIMUM_TOTAL_SIZE,
    (UINT)OID_GEN_MAC_OPTIONS,
    (UINT)OID_GEN_PROTOCOL_OPTIONS,
    (UINT)OID_GEN_LINK_SPEED,
    (UINT)OID_GEN_TRANSMIT_BUFFER_SPACE,
    (UINT)OID_GEN_RECEIVE_BUFFER_SPACE,
    (UINT)OID_GEN_TRANSMIT_BLOCK_SIZE,
    (UINT)OID_GEN_RECEIVE_BLOCK_SIZE,
    (UINT)OID_GEN_VENDOR_DESCRIPTION,
    (UINT)OID_GEN_VENDOR_ID,
    (UINT)OID_GEN_DRIVER_VERSION,
    (UINT)OID_GEN_CURRENT_PACKET_FILTER,
    (UINT)OID_GEN_CURRENT_LOOKAHEAD,
    (UINT)OID_GEN_XMIT_OK,
    (UINT)OID_GEN_RCV_OK,
    (UINT)OID_GEN_XMIT_ERROR,
    (UINT)OID_GEN_RCV_ERROR,
    (UINT)OID_GEN_RCV_NO_BUFFER,
    (UINT)OID_802_3_PERMANENT_ADDRESS,
    (UINT)OID_802_3_CURRENT_ADDRESS,
    (UINT)OID_802_3_MULTICAST_LIST,
    (UINT)OID_802_3_MAXIMUM_LIST_SIZE,
    (UINT)OID_GEN_MEDIA_CONNECT_STATUS,
    (UINT)OID_GEN_MAXIMUM_SEND_PACKETS,
    (UINT)OID_GEN_VENDOR_DRIVER_VERSION,
    (UINT)OID_802_3_RCV_ERROR_ALIGNMENT,
    (UINT)OID_802_3_XMIT_ONE_COLLISION,
    (UINT)OID_802_3_XMIT_MORE_COLLISIONS,
    //  Power Management
    (UINT)OID_PNP_CAPABILITIES,
    (UINT)OID_PNP_SET_POWER,
    (UINT)OID_PNP_QUERY_POWER,
    (UINT)OID_PNP_ADD_WAKE_UP_PATTERN,
    (UINT)OID_PNP_REMOVE_WAKE_UP_PATTERN,
    (UINT)OID_PNP_ENABLE_WAKE_UP,
    // SMSC Proprietary
    OID_NDIS_SMSC_DUMP_ALL_REGS,
};
#define TRACE_BUFFER

#ifdef  TRACE_BUFFER
volatile DWORD  dwTxReported = 0UL, dwTxSent = 0UL, dwTxPend = 0UL;
volatile DWORD  dwRxPktToFull = 0UL, dwRxPktToEmpty = 0UL;
volatile DWORD  dwRxPktFromFull = 0UL, dwRxPktFromEmpty = 0UL;
volatile DWORD  dwRxTotalPkt = 0UL, dwRxDiscard = 0UL;
volatile DWORD  dwRxNumIndicate = 0UL;
void DumpStatus(const SMSC9118_ADAPTER * const pAdapter);
#endif

LONG       g_total_pkt_in_ndis = 0;
/*
   Since Windows CE NDIS miniports are implemented as DLLs, 
   a DLL entrypoint isneeded.
*/
BOOL __stdcall DllEntry (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    BOOL    bRet;

    /* Make Lint Happy */
    SMSC_TRACE1(DBG_INIT, "date_code = %a", date_code);

    SMSC_TRACE0(DBG_INIT,"+DllEntry\r\n");
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
#ifdef  DEBUG
            DEBUGREGISTER (hDLL);
#endif
            SMSC_TRACE0(DBG_INIT,"DLL_PROCESS_ATTACH\r\n");
            bRet = DisableThreadLibraryCalls((HMODULE) hDLL);
            if (bRet != TRUE) {
                SMSC_TRACE0(DBG_INIT,"Failed to DisableThreadLibraryCalls()\r\n");
            }
            break;
        case DLL_PROCESS_DETACH:
            SMSC_TRACE0(DBG_INIT,"DLL_PROCESS_DETACH\r\n");
            break;
        default:
            break;
    }

    dwReason = dwReason;
    lpReserved = lpReserved;
    SMSC_TRACE0(DBG_INIT,"-DllEntry\r\n");
    return (TRUE);
}

/*----------------------------------------------------------------------------
    DriverEntry
        NDIS miniport function
*/
NDIS_STATUS DriverEntry (IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NDIS_MINIPORT_CHARACTERISTICS Characteristics;
    NDIS_STATUS Status;
    
    SMSC_TRACE0(DBG_INIT,"+DriverEntry\r\n");

    RETAILMSG(1, (TEXT("DRIVER_VERSION : %X, "), DRIVER_VERSION));
    RETAILMSG(1, (TEXT("DATECODE : %s\r\n"), TEXT(BUILD_NUMBER)));

    PlatformInitialize();
    PlatformDisplayInfo();

    gSmsc9118MiniportBlock.AdapterQueue = (PSMSC9118_ADAPTER)NULL;
    NdisMInitializeWrapper(&gSmsc9118MiniportBlock.NdisWrapperHandle, 
                            DriverObject, RegistryPath, NULL);

    NdisZeroMemory (&Characteristics, sizeof (Characteristics));
    Characteristics.MajorNdisVersion = (UCHAR)SMSC9118_NDIS_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = (UCHAR)SMSC9118_NDIS_MINOR_VERSION;
    Characteristics.CheckForHangHandler = Smsc9118CheckForHang;
    Characteristics.DisableInterruptHandler = NULL;
    Characteristics.EnableInterruptHandler = NULL;
    Characteristics.HaltHandler = Smsc9118Halt;
    Characteristics.HandleInterruptHandler = Smsc9118HandleInterrupt;
    Characteristics.InitializeHandler = Smsc9118Initialize;
    Characteristics.ISRHandler = Smsc9118Isr;
    Characteristics.QueryInformationHandler = Smsc9118QueryInformation;
    Characteristics.ReconfigureHandler = NULL;
    Characteristics.ResetHandler = Smsc9118Reset;
    Characteristics.SendHandler = NULL;
    Characteristics.SetInformationHandler = Smsc9118SetInformation;
    Characteristics.TransferDataHandler = NULL;
    Characteristics.ReturnPacketHandler = Smsc9118GetReturnedPackets;
    Characteristics.SendPacketsHandler = Smsc9118SendPackets;
    Characteristics.AllocateCompleteHandler = NULL;

    Status = NdisMRegisterMiniport (gSmsc9118MiniportBlock.NdisWrapperHandle, 
                                    &Characteristics, 
                                    sizeof (Characteristics));
    if (Status == NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE0(DBG_INIT, "NdisMRegisterMiniport OK.\r\n");
    }
    else 
    {
        // Terminate the wrapper.
        NdisTerminateWrapper (gSmsc9118MiniportBlock.NdisWrapperHandle, NULL);
        gSmsc9118MiniportBlock.NdisWrapperHandle = NULL;
        SMSC_TRACE0(DBG_INIT,"    NdisMRegisterMiniport failed.\r\n");
    }
    
    SMSC_TRACE0(DBG_INIT,"-DriverEntry\r\n");
    return (Status);
}


/*----------------------------------------------------------------------------
    InitializeQueues
        Initialization of data structures used by the driver.

*/
NDIS_STATUS InitializeQueues(IN PSMSC9118_ADAPTER pAdapter)
{
    NDIS_STATUS Status;
    UINT dw;
    PSMSC9118_SHAREDMEM pSharedMem;

    SMSC_TRACE0(DBG_INIT,"+InitializeQueues\r\n");

    // Initialize Tx queues, etc.
    QUEUE_INIT(&(pAdapter->TxRdyToComplete));
    QUEUE_INIT(&(pAdapter->TxWaitToSend));
    QUEUE_INIT(&(pAdapter->TxDeferedPkt));

    // Initialize Rx queues, etc.
    NdisAllocatePacketPool(&Status, 
                           &(pAdapter->hPacketPool),
                           MAX_RXPACKETS_IN_QUEUE,
                           PROTOCOL_RESERVED_LENGTH);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE0(DBG_INIT,"NdisAllocatePacketPool failed.\r\n");
        return (Status);
    }

    for (dw=0U; dw<MAX_RXPACKETS_IN_QUEUE; dw++)
    {
        NdisAllocatePacket(&Status, &(pAdapter->RxPacketArray[dw]), pAdapter->hPacketPool);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            SMSC_TRACE1(DBG_INIT,"NdisAllocatePacket failed. dw=%d\r\n", dw);
            return (Status);
        }
    }

    NdisAllocateBufferPool(&Status,
                           &(pAdapter->hBufferPool),
                           MAX_RXPACKETS_IN_QUEUE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE0(DBG_INIT,"NdisAllocateBufferPool failed.\r\n");
        return (Status);
    }

    pSharedMem = (PSMSC9118_SHAREDMEM)(pAdapter->pSharedMemVA);
    for (dw=0U; dw<MAX_RXPACKETS_IN_QUEUE; dw++)
    {
        if (pAdapter->fRxDMAMode)
        {
            NdisAllocateBuffer(&Status, 
                               &(pAdapter->RxBufferArray[dw]), 
                               pAdapter->hBufferPool,
                               (PVOID)((PUCHAR)(((DWORD)pSharedMem->RxBuffer[dw]+(RX_END_ALIGNMENT-1UL))&~((DWORD)RX_END_ALIGNMENT-1UL))+2UL),   //DMA - 16-bit aligned
                               MAX_PACKET);
        }
        else
        {
            NdisAllocateBuffer(&Status, 
                               &(pAdapter->RxBufferArray[dw]), 
                               pAdapter->hBufferPool,
                               // PIO with 2-byte offset
                               (PVOID)((PUCHAR)(pSharedMem->RxBuffer[dw])+2),   
                               MAX_PACKET);
        }
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            SMSC_TRACE0(DBG_INIT,"NdisAllocateBuffer failed.\r\n");
            return (Status);
        }
    }

    for (dw=0U; dw<MAX_RXPACKETS_IN_QUEUE; dw++) 
    {
        NdisChainBufferAtBack((pAdapter->RxPacketArray[dw]), (pAdapter->RxBufferArray[dw]));
        {
        UINT    ArraySize;
        NDIS_PHYSICAL_ADDRESS_UNIT SegmentArray[MAX_NUM_SEGMENTS];

        NdisMStartBufferPhysicalMapping(pAdapter->hMiniportAdapterHandle,
                pAdapter->RxBufferArray[dw],
                0UL, (BOOLEAN)TRUE, SegmentArray, &ArraySize);
        if (ArraySize != 1U) 
        {
            RETAILMSG(1, (TEXT("Error! ArraySize = %d\r\n"), ArraySize));
        }
        SET_PACKET_RESERVED_PA(pAdapter->RxPacketArray[dw], 
                NdisGetPhysicalAddressLow(SegmentArray[0].PhysicalAddress));
        }
    }

    // Initialize EmptyPktArray
    pAdapter->EmptyPkt.dwRdPtr = pAdapter->EmptyPkt.dwWrPtr = 0UL;
    for (dw=0U; dw<MAX_RXPACKETS_IN_QUEUE; dw++) {
        pAdapter->EmptyPkt.dwPktArray[dw] = pAdapter->RxPacketArray[dw];
    }
    pAdapter->EmptyPkt.dwRdPtr = 0UL;
    pAdapter->EmptyPkt.dwWrPtr = pAdapter->EmptyPkt.dwRdPtr + (DWORD)MAX_RXPACKETS_IN_QUEUE;

    // Initialize FullPktArray
    pAdapter->FullPkt.dwRdPtr = pAdapter->FullPkt.dwWrPtr = 0UL;

    pAdapter->TxDPCNeeded = (BOOLEAN)FALSE;
    pAdapter->RxDPCNeeded = (BOOLEAN)FALSE;
    pAdapter->PhyDPCNeeded = (BOOLEAN)FALSE;
    pAdapter->RxOverRun = (BOOLEAN)FALSE;
    
    SMSC_TRACE0(DBG_INIT,"-InitializeQueues\r\n");
    return (NDIS_STATUS_SUCCESS);

}

/*----------------------------------------------------------------------------
    Smsc9118Initialize
        NDIS miniport function
*/
NDIS_STATUS
Smsc9118Initialize (OUT PNDIS_STATUS OpenErrorStatus, 
                   OUT PUINT puiSelectedMediumIndex, 
                   IN  NDIS_MEDIUM * MediumArray, 
                   IN  UINT uiMediumArraySize, 
                   IN  NDIS_HANDLE hMiniportAdapterHandle, 
                   IN  NDIS_HANDLE hConfigurationHandle)
{
    PSMSC9118_ADAPTER pAdapter;
    BOOL bResult;
    UINT uiIndex;
    NDIS_STATUS Status;
    NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

    SMSC_TRACE0(DBG_INIT,"+Smsc9118Initialize\r\n");

    *OpenErrorStatus = NDIS_STATUS_SUCCESS;

    for (uiIndex = 0U; uiIndex < uiMediumArraySize; uiIndex++)
    {
        if (MediumArray[uiIndex] == NdisMedium802_3) {
            break;
        }
    }

    if (uiIndex == uiMediumArraySize)
    {
        SMSC_TRACE0(DBG_INIT,"  No supported media\r\n");
        return (NDIS_STATUS_UNSUPPORTED_MEDIA);
    }

    *puiSelectedMediumIndex = uiIndex;

    // Allocate adapter context memory.
    HighestAcceptableMax.LowPart = 0xFFFFFFFFUL;        // DWORD
    HighestAcceptableMax.HighPart = -1L;                // LONG

    /**********************************************************************
     * Nov/14/08 WH
        By WinCE document, NdisAllocateMemory() will call either 
        VirtualAlloc() or LocalAlloc() based on MemoryFlags (3rd parameter).
        In our case, it will be LocalAlloc() because 3rd parameter is 0.

        LocalAlloc() will determine the number of bytes allocated using the
        LocalSize() function. From the "Programming Windows Embedded CE 6.0 
        4th Edition" of MS press, it describes that
          "The size of the block is rouned up. The amount rounded up varies
           dependings on the version of CE. CE 6.0 rounds up to 
           the next 32-byte boundary,"

        And, heap which is memory allocated by LocalAlloc() is mamanged with
        32-bytes block for efficiency in CE. It means it is allocated with 
        32-byte aligned virtual address all the time.

        Technically, NdisAllocateMemory() will return 32byte aligned virtual
        address and size will be 32-byte aligned too.

        But, **JUST** in case of NdisAllocateMemory() didn't return 
        32bit aligned virtual address, we ask 4 bytes more size than need to
        align it manually
    ***********************************************************************/
    Status = NdisAllocateMemory ((PVOID *)&pAdapter,
                                 sizeof (SMSC9118_ADAPTER) + 4,
                                 NDIS_ALLOC_FLAG, HighestAcceptableMax);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE0(DBG_INIT,"  NdisAllocateMemory (SMSC9118_ADAPTER) failed.\r\n");
        return NDIS_STATUS_RESOURCES;
    }
    
    SMSC_TRACE1(DBG_INIT,"  pAdapter allocated at 0x%x.\r\n", pAdapter);
    if (((DWORD)pAdapter) & 0x03)
    {
        SMSC_WARNING0("pAdapter is not DWORD aligned. Adjust it manually\r\n");
        pAdapter = ((void *)(((UINT32)pAdapter+3)&~0x03UL));
    }

    NdisZeroMemory(pAdapter, sizeof (SMSC9118_ADAPTER));
    NdisZeroMemory(&pAdapter->lan9118_data, sizeof(LAN9118_DATA));

    // Read configurations from registry
    Status = ReadConfigFromRegistry(pAdapter, hConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
        return Status;
    }

    pAdapter->hMiniportAdapterHandle = hMiniportAdapterHandle;
    pAdapter->lan9118_data.dwLanBase = (DWORD)VirtualAlloc((PVOID)0, 
                                                           (DWORD)REGISTER_PAGE_SIZE, 
                                                           (DWORD)MEM_RESERVE, 
                                                           (DWORD)PAGE_NOACCESS);
    if ((PVOID)(pAdapter->lan9118_data.dwLanBase)==NULL)
    {
        SMSC_TRACE0(DBG_INIT,"VirtualAlloc failed.\r\n");
        return NDIS_STATUS_RESOURCES;
    }
    else {
        SMSC_TRACE1(DBG_INIT,"VirtualAlloc at 0x%x.\r\n", pAdapter->lan9118_data.dwLanBase);   
    }

    SMSC_TRACE1(DBG_INIT,"ulIoBaseAddress = 0x%x.\r\n", pAdapter->ulIoBaseAddress);
    bResult= VirtualCopy((PVOID)pAdapter->lan9118_data.dwLanBase,
                         (PVOID)(pAdapter->ulIoBaseAddress >> 8),
                         (DWORD)REGISTER_PAGE_SIZE,
                         (DWORD)(PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL));
    if (bResult == TRUE)
    {
        // Identify chip
        if (!ChipIdentify(pAdapter))
        {
            SMSC_WARNING0("  ChipIdentify -- Failure\r\n");
            return NDIS_STATUS_ADAPTER_NOT_FOUND;
        }
    }
    else
    {
        SMSC_WARNING0("VirtualCopy ETHERNET_BASE failed\r\n");
        if (pAdapter->lan9118_data.dwLanBase)
        {
            bResult = VirtualFree((PVOID)pAdapter->lan9118_data.dwLanBase, 0UL, (DWORD)MEM_RELEASE);
            pAdapter->lan9118_data.dwLanBase = 0UL;
        }
        return NDIS_STATUS_RESOURCES;
    }

    // Mapping DMA registers. 
    if (pAdapter->fRxDMAMode || pAdapter->fTxDMAMode)
    {
        // REGISTER_PAGE_SIZE is changed to DMAC_REGSIZE
        pAdapter->DMABaseVA = (DWORD)VirtualAlloc ((PVOID)0, 
                                                   (DWORD)DMAC_REGSIZE, 
                                                   (DWORD)MEM_RESERVE, 
                                                   (DWORD)PAGE_NOACCESS);

        if ((PVOID)(pAdapter->DMABaseVA)==NULL)
        {
            // Free VA for 9118
            if (pAdapter->lan9118_data.dwLanBase)
            {
                bResult = VirtualFree ((PVOID)(pAdapter->lan9118_data.dwLanBase - (pAdapter->ulIoBaseAddress & (((DWORD)PAGE_SIZE)-1UL))), 0UL, (DWORD)MEM_RELEASE);
                pAdapter->lan9118_data.dwLanBase = (DWORD)NULL;
            }
            // Free pAdapter.
            NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
            SMSC_TRACE0(DBG_DMA,"DMA VirtualAlloc failed.\r\n");
            return (NDIS_STATUS_FAILURE);
        }
        else
        {
            SMSC_TRACE1(DBG_DMA,"DMA VirtualAlloc OK. pAdapter->DMABaseVA = 0x%x\r\n", pAdapter->DMABaseVA);
        }

        bResult= VirtualCopy ((PVOID)(pAdapter->DMABaseVA),
                              (PVOID)(DMAC_REGBASE >> 8),
                              (DWORD)DMAC_REGSIZE,
                              (DWORD)(PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL));

        if (bResult == FALSE)
        {
            // Free VA for DMA
            if (pAdapter->DMABaseVA)
            {
                bResult = VirtualFree ((PVOID)(pAdapter->DMABaseVA), 0UL, (DWORD)MEM_RELEASE);
                pAdapter->DMABaseVA = (DWORD)NULL;
            }
            // Free VA for 9118
            if (pAdapter->lan9118_data.dwLanBase)
            {
                bResult = VirtualFree ((PVOID)(pAdapter->lan9118_data.dwLanBase - (pAdapter->ulIoBaseAddress & (((DWORD)PAGE_SIZE)-1UL))), 0UL, (DWORD)MEM_RELEASE);
                pAdapter->lan9118_data.dwLanBase = (DWORD)NULL;
            }
            // Free pAdapter.
            NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
            SMSC_TRACE0(DBG_DMA,"DMA VirtualCopy failed.\r\n");
            return (NDIS_STATUS_FAILURE);
        }
        else
        {
            SMSC_TRACE0(DBG_DMA,"DMA VirtualCopy OK.\r\n");
        }
    }
    
    /**********************************************************************
     * Nov/14/08 WH
        By WinCE document, NdisAllocateMemory() will call either 
        VirtualAlloc() or LocalAlloc() based on MemoryFlags (3rd parameter).
        In our case, it will be LocalAlloc() because 3rd parameter is 0.

        LocalAlloc() will determine the number of bytes allocated using the
        LocalSize() function. From the "Programming Windows Embedded CE 6.0 
        4th Edition" of MS press, it describes that
          "The size of the block is rouned up. The amount rounded up varies
           dependings on the version of CE. CE 6.0 rounds up to 
           the next 32-byte boundary,"

        And, heap which is memory allocated by LocalAlloc() is mamanged with
        32-bytes block for efficiency in CE. It means it is allocated with 
        32-byte aligned virtual address all the time.

        Technically, NdisAllocateMemory() will return 32byte aligned virtual
        address and size will be 32-byte aligned too.

        But, **JUST** in case of NdisAllocateMemory() didn't return 
        32bit aligned virtual address, we ask 4 bytes more size than need to
        align it manually
    */
    // Allocate shared memory for Rx.
    Status = NdisAllocateMemory((PVOID *)&pAdapter->pSharedMemVA,
                                (UINT)sizeof(SMSC9118_SHAREDMEM) + 4,
                                NDIS_ALLOC_FLAG, HighestAcceptableMax);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_WARNING0("Failed to allocate memory for DMA Rx\r\n");
        return (NDIS_STATUS_FAILURE);
    }

    // pSharedMemVA is not DWORD aligned
    if (((DWORD)pAdapter->pSharedMemVA) & 0x03)
    {
        SMSC_WARNING0("pSharedMemVA is not DWORD aligned. Adjust it manually\r\n");
        pAdapter->pSharedMemVA=((void *)(((UINT32)pAdapter->pSharedMemVA+3)&~0x03UL));
    }

    if (pAdapter->pSharedMemVA == 0)
    {
        // Free VA for DMA
        if (pAdapter->DMABaseVA)
        {
            bResult = VirtualFree ((PVOID)(pAdapter->DMABaseVA), 0UL, (DWORD)MEM_RELEASE);
            pAdapter->DMABaseVA = (DWORD)NULL;
        }
        // Free VA for 9118
        if (pAdapter->lan9118_data.dwLanBase)
        {
            bResult = VirtualFree ((PVOID)(pAdapter->lan9118_data.dwLanBase - (pAdapter->ulIoBaseAddress & (((DWORD)PAGE_SIZE)-1UL))), 0UL, (DWORD)MEM_RELEASE);
            pAdapter->lan9118_data.dwLanBase = (DWORD)NULL;
        }
        // Free pAdapter.
        NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
        SMSC_TRACE0(DBG_DMA,"  AllocPhysMem failed.\r\n");
        return (NDIS_STATUS_FAILURE);
    }
    else
    {
        SMSC_TRACE2(DBG_DMA,"AllocPhysMem OK. pSharedMemVA = 0x%x; pSharedMemPA = 0x%x.\r\n", pAdapter->pSharedMemVA, pAdapter->pSharedMemPA);
    }
    
    pAdapter->lan9118_data.wAutoMdix = (WORD)(pAdapter->dwAutoMdix & 0xFFFF);

    // Setup the card based on the initialization information
    if (ChipSetup(pAdapter))
    {
        NdisMRegisterAdapterShutdownHandler (pAdapter->hMiniportAdapterHandle, pAdapter, Smsc9118Shutdown);

        ChipStart(pAdapter);

        SMSC_TRACE0(DBG_INIT,"  ChipSetup Success\r\n");
    }
    else 
    {
        SMSC_TRACE0(DBG_INIT,"  ChipSetup Failed\r\n");
        return NDIS_STATUS_ADAPTER_NOT_FOUND;
    }

    pAdapter->fLinkUp = FALSE;

    // Initialize DMA
    if (pAdapter->fRxDMAMode || pAdapter->fTxDMAMode) {
        if (DmaInitialize(pAdapter) != TRUE)
        {
            SMSC_TRACE0(DBG_INIT," DMA Initialization failed.\r\n");
            return (NDIS_STATUS_FAILURE);
        }
    }

    // Initialize queues
    if (InitializeQueues(pAdapter) != NDIS_STATUS_SUCCESS)
    {
        NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
        SMSC_TRACE0(DBG_INIT," Smsc9118InitializeQueues failed.\r\n");
        return (NDIS_STATUS_FAILURE);
    }

    // Set power state to NdisDeviceStateD0
    pAdapter->CurrentPowerState = NdisDeviceStateD0;

    // Make Lint Happy
    MediumArray = MediumArray;
    uiMediumArraySize = uiMediumArraySize;

    // allocate temp buffer for fragmented packet
    NdisMAllocateSharedMemory(hMiniportAdapterHandle,
            2000UL, (BOOLEAN)FALSE, 
            (PVOID *)&(pAdapter->TxTempPktBuf.pUnAlignedVAddr),
            &(pAdapter->TxTempPktBuf.UnAlignedPAddr));
    if (pAdapter->TxTempPktBuf.pUnAlignedVAddr == NULL)
    {
        SMSC_WARNING0("Error! Can not allocate buffer for fragmented packet\r\n");
        return (NDIS_STATUS_FAILURE);
    }
    else
    {
        pAdapter->TxTempPktBuf.pVAddr = (PUCHAR)(((DWORD)(pAdapter->TxTempPktBuf.pUnAlignedVAddr) + 0x03UL) & (~0x03UL));
        pAdapter->TxTempPktBuf.PAddr.LowPart = (((DWORD)NdisGetPhysicalAddressLow(pAdapter->TxTempPktBuf.UnAlignedPAddr) + 0x03UL) & (~0x03UL));
    }

    pAdapter->hSWIntEvt = NULL;

    // Now Register Adapter and initialize Interrupt
    if (RegisterAdapter(pAdapter, hConfigurationHandle) != NDIS_STATUS_SUCCESS)
    {
        // Free VA for 9118
        if (pAdapter->lan9118_data.dwLanBase)
        {
            bResult = VirtualFree((PVOID)(pAdapter->lan9118_data.dwLanBase - (pAdapter->ulIoBaseAddress & (((DWORD)PAGE_SIZE)-1UL))), 0UL, (DWORD)MEM_RELEASE);
            pAdapter->lan9118_data.dwLanBase = (DWORD)NULL;
        }
        // Free pAdapter.
        NdisFreeMemory(pAdapter, (UINT)sizeof(SMSC9118_ADAPTER), NDIS_ALLOC_FLAG);
        SMSC_WARNING0("  RegisterAdapter failed.\r\n");
        return (NDIS_STATUS_FAILURE);
    }

    SMSC_TRACE0(DBG_INIT,"-Smsc9118Initialize\r\n");
    return (NDIS_STATUS_SUCCESS);
}


/*----------------------------------------------------------------------------
    RegisterAdapter
        Register 9118 with NDIS

*/
NDIS_STATUS
RegisterAdapter (IN PSMSC9118_ADAPTER pAdapter, 
                 IN NDIS_HANDLE hConfigurationHandle)
{
    NDIS_STATUS status;

    SMSC_TRACE0(DBG_INIT,"+RegisterAdapter\r\n");

    // avoid Lint error
    hConfigurationHandle = hConfigurationHandle;

    // Inform the wrapper of the physical attributes of this adapter.
    NdisMSetAttributesEx(pAdapter->hMiniportAdapterHandle,
                         (NDIS_HANDLE)pAdapter,
                         // NOTE
                         // Sep/22/08 WH
                         // Depends on host processor power, 
                         // Ndis may get chocked at 10Mbps speed when packets
                         //  are slamming chips high bandwidth.
                         // It causes MiniportReset.
                         // But, it is not practical case in daily life, 
                         //  increasing NDIS timeout to call MiniportReset
                         10U,
                         (DWORD)NDIS_ATTRIBUTE_BUS_MASTER,
                         NdisInterfaceInternal);
    // Link us on to the chain of adapters for this driver.
    pAdapter->NextAdapter = gSmsc9118MiniportBlock.AdapterQueue;
    gSmsc9118MiniportBlock.AdapterQueue = pAdapter;

    SMSC_TRACE1(DBG_INIT,"ucInterruptNumber(before)=0x%x\r\n", pAdapter->ucInterruptNumber);
    status = NdisMRegisterInterrupt (&(pAdapter->Interrupt),
                                     pAdapter->hMiniportAdapterHandle, 
                                     (UINT)pAdapter->ucInterruptNumber, 
                                     (UINT)pAdapter->ucInterruptNumber, 
                                     (BOOLEAN)TRUE, 
                                     (BOOLEAN)TRUE, 
                                     NdisInterruptLatched);
    if (status == NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE0(DBG_INIT,"  NdisMRegisterInterrupt Success\r\n");
    }
    else
    {
        SMSC_TRACE0(DBG_INIT,"  NdisRegisterInterrupt Failed\r\n");
    }

    if (gSmsc9118MiniportBlock.AdapterQueue == pAdapter) 
    {
        gSmsc9118MiniportBlock.AdapterQueue = pAdapter->NextAdapter;
    }
    else
    {
        PSMSC9118_ADAPTER TmpAdapter = gSmsc9118MiniportBlock.AdapterQueue;
        while (TmpAdapter->NextAdapter != pAdapter) 
        {
            TmpAdapter = TmpAdapter->NextAdapter;
        }
        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    SMSC_TRACE0(DBG_INIT,"-RegisterAdapter\r\n");
    return (status);
}


/*----------------------------------------------------------------------------
    ChipIdentify
*/
BOOLEAN ChipIdentify(IN PSMSC9118_ADAPTER pAdapter)
{
    DWORD   dwValue;

    SMSC_TRACE0(DBG_INIT,"+ChipIdentify\r\n");

    if (pAdapter==NULL) 
    {
        return (BOOLEAN)FALSE;
    }

    // Check Chip ID
    // Start with 32bit Mode
    PlatformSetBusWidth(32UL);
    dwValue = AdapterGetCSR(BYTE_TEST);
    if (dwValue != 0x87654321UL)
    {
        // Try with 16bit mode
        PlatformSetBusWidth(16UL);
        dwValue = AdapterGetCSR(BYTE_TEST);
        if (dwValue != 0x87654321UL)
        {
            SMSC_WARNING0("Error! Can not determine 16bit or 32bit mode.\r\n");
            RETAILMSG(1, (TEXT("BYTE_TEST = 0x%08x\r\n"), dwValue));
            return (BOOLEAN)FALSE;
        }
        else
        {
            RETAILMSG(1, (TEXT("16Bit Mode\r\n")));
        }
    }
    else
    {
        RETAILMSG(1, (TEXT("32Bit Mode\r\n")));
    }
    
    // read Chip ID
    pAdapter->lan9118_data.dwIdRev = AdapterGetCSR(ID_REV);

    // This driver won't support ID_REV (0x011x0000 and 0x011x0001) 
    if ((pAdapter->lan9118_data.dwIdRev == 0x01180000UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01170000UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01160000UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01150000UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01180001UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01170001UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01160001UL) ||
        (pAdapter->lan9118_data.dwIdRev == 0x01150001UL))
    {
        SMSC_WARNING1("Driver doesn't support this chip (ID_REV=0x%08x)\r\n", pAdapter->lan9118_data.dwIdRev);
        return (BOOLEAN)FALSE;
    }

    switch ((pAdapter->lan9118_data.dwIdRev)&0xFFFF0000UL)
    {
        case    0x218A0000UL:
                RETAILMSG(1, (TEXT("Lan9218A identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x217A0000UL:
                RETAILMSG(1, (TEXT("Lan9217A identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x216A0000UL:
                RETAILMSG(1, (TEXT("Lan9216A identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x215A0000UL:
                RETAILMSG(1, (TEXT("Lan9215A identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
                /*
                 * 0x93110000 & 0x93120000 are here
                 * But, we don't have device to test them as of Aug/2008
                 */
        case    0x93110000UL:
                RETAILMSG(1, (TEXT("Lan9311 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x93120000UL:
                RETAILMSG(1, (TEXT("Lan9312 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x92200000UL:
                RETAILMSG(1, (TEXT("Lan9220 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x92210000UL:
                RETAILMSG(1, (TEXT("Lan9221 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x92100000UL:
                RETAILMSG(1, (TEXT("Lan9210 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x92110000UL:
                RETAILMSG(1, (TEXT("Lan9211 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x118A0000UL:
                RETAILMSG(1, (TEXT("Lan9218 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x117A0000UL:
                RETAILMSG(1, (TEXT("Lan9217 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x116A0000UL:
                RETAILMSG(1, (TEXT("Lan9216 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x115A0000UL:
                RETAILMSG(1, (TEXT("Lan9215 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
                /* 
                 * ID of 0x011x0000 & 0x011x0001 will be filtered 
                 * code before this switch case routines
                 */
        case    0x01180000UL:
                RETAILMSG(1, (TEXT("Lan9118 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x01170000UL:
                RETAILMSG(1, (TEXT("Lan9117 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x01160000UL:
                RETAILMSG(1, (TEXT("Lan9116 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        case    0x01150000UL:
                RETAILMSG(1, (TEXT("Lan9115 identified. ID_REV = 0x%08lX\r\n"), (pAdapter->lan9118_data.dwIdRev)));
                break;
        default:
                if (CheckExtraID(pAdapter, pAdapter->lan9118_data.dwIdRev) == FALSE)
                {
                    SMSC_WARNING1("Can not Identify LAN9118. (ID_REV = 0x%08x)\r\n", pAdapter->lan9118_data.dwIdRev);
                    return (BOOLEAN)FALSE;
                }
    }

    // Set bus timing for chip
    PlatformSetBusTiming(pAdapter->lan9118_data.dwIdRev);

    SMSC_TRACE0(DBG_INIT,"-ChipIdentify\r\n");
    return (BOOLEAN)TRUE;
}


/*----------------------------------------------------------------------------
    ChipSetup
    Reset the chip, 
*/
BOOLEAN ChipSetup(IN PSMSC9118_ADAPTER pAdapter)
{
    const DWORD dwLanBase = pAdapter->lan9118_data.dwLanBase;
    DWORD dwGpioCfg;
    DWORD dwADDRL, dwADDRH;
    DWORD dwIntCfg;

    SMSC_TRACE0(DBG_INIT,"+ChipSetup\r\n");

    // reset the chip
    if (ChipReset(pAdapter) == FALSE)
    {
        SMSC_WARNING0("Reset failed.\r\n");
        return (BOOLEAN)FALSE;
    }

    if (pAdapter->dIntrDeas != 0)
    {
        dwIntCfg = pAdapter->lan9118_data.dwIntCfg;
        dwIntCfg |= ((pAdapter->dIntrDeas & 0xFF) << 24);
    }
    else
    {
        dwIntCfg = pAdapter->lan9118_data.dwIntCfg | INT_DEAS;
    }
    pAdapter->lan9118_data.dwIntCfg = dwIntCfg;

    // disable all interrupt, clear any pending status and configure INT_CFG
    Lan_InitializeInterrupts(&(pAdapter->lan9118_data), dwIntCfg);

    if (!Lan_InitializePhy(&(pAdapter->lan9118_data), pAdapter->PhyAddress))
    {
        SMSC_TRACE0(DBG_INIT,"Lan_InitializePhy failed.\r\n");
        return (BOOLEAN)FALSE;
    }

    Lan_InitializeTx(&(pAdapter->lan9118_data));

    if (pAdapter->fRxDMAMode)
    {
#if (CACHE_LINE_BYTES==32UL)
        Lan_InitializeRx(&(pAdapter->lan9118_data), 0x80000200UL, 0UL); //DMA
#elif (CACHE_LINE_BYTES==16UL)
        Lan_InitializeRx(&(pAdapter->lan9118_data), 0x40000200UL, 0UL); //DMA
#else
#error  "CACHE_LINE_BYTES should be defined."
#endif
    }
    else
    {
        Lan_InitializeRx(&(pAdapter->lan9118_data), 0x00000200UL, 0UL); // PIO with 2-byte offset
    }

    // Mac Initialize
    if (pAdapter->fSwFlowControlEnabled == (BOOLEAN)TRUE)
    {
        EnableSwFlowControlFD(pAdapter);
    }
    else
    {
        SMSC_TRACE0(DBG_FLOW,"Flow control disabled.\r\n");
    }

#ifdef  USE_GPIO
    // Enable GPIOs, RX_DV and TX_EN
    dwGpioCfg = GetRegDW(dwLanBase, GPIO_CFG);
    dwGpioCfg &= ~GPIO_CFG_EEPR_EN_;
    // Enable GPIO3 & GPIO4
    dwGpioCfg |= 0x00100000UL;
    // Set GPIO0~2 Output
    dwGpioCfg |= (GPIO_CFG_GPIODIR2_|GPIO_CFG_GPIODIR1_|GPIO_CFG_GPIODIR0_);
    // Clear GPIO0~2
    dwGpioCfg &= ~(GPIO_CFG_GPIOD2_|GPIO_CFG_GPIOD1_|GPIO_CFG_GPIOD0_);
    SetRegDW(dwLanBase, GPIO_CFG, dwGpioCfg);
#else
    dwGpioCfg = GPIO_CFG_LED3_EN_ | GPIO_CFG_LED2_EN_ | GPIO_CFG_LED1_EN_ |
                GPIO_CFG_GPIODIR2_ | GPIO_CFG_GPIODIR1_ | GPIO_CFG_GPIODIR0_;
    SetRegDW(dwLanBase, GPIO_CFG, dwGpioCfg);
#endif

    //Save GPIO_CFG register
    dwGpioCfg = GetRegDW(dwLanBase, GPIO_CFG);

    //Configure GPIO as EEPROM Mode
    SetRegDW(dwLanBase, GPIO_CFG, dwGpioCfg & ~GPIO_CFG_EEPR_EN_);
    B2BWriteDelay(dwLanBase, 1);

    //Check automatic load
    if (GetRegDW(dwLanBase, E2P_CMD) & E2P_CMD_E2P_PROG_DONE_)
    {
        //MAC address is successfully loaded from EEPROM.
        SMSC_TRACE0(DBG_EEPROM,"MAC address from EEPROM is loaded.\r\n");

        dwADDRL = Lan_GetMacRegDW(dwLanBase, ADDRL);
        dwADDRH = Lan_GetMacRegDW(dwLanBase, ADDRH);
        SMSC_TRACE2(DBG_EEPROM,"ADDRH=0x%x ADDRL=0x%x\r\n", dwADDRH, dwADDRL);

        // Save the MAC address.
        pAdapter->ucStationAddress[0] = LOBYTE(LOWORD(dwADDRL));
        pAdapter->ucStationAddress[1] = HIBYTE(LOWORD(dwADDRL));
        pAdapter->ucStationAddress[2] = LOBYTE(HIWORD(dwADDRL));
        pAdapter->ucStationAddress[3] = HIBYTE(HIWORD(dwADDRL));
        pAdapter->ucStationAddress[4] = LOBYTE(LOWORD(dwADDRH)); 
        pAdapter->ucStationAddress[5] = HIBYTE(LOWORD(dwADDRH));
        SMSC_TRACE6(DBG_EEPROM,"ucStationAddress=%x %x %x %x %x %x\r\n", 
                               pAdapter->ucStationAddress[0], 
                               pAdapter->ucStationAddress[1],
                               pAdapter->ucStationAddress[2], 
                               pAdapter->ucStationAddress[3],
                               pAdapter->ucStationAddress[4], 
                               pAdapter->ucStationAddress[5] );
    }
    else
    {
        // Set the default MAC address
        Lan_SetMacAddress(&(pAdapter->lan9118_data), 0x00000070UL, 0x110F8000UL);
    
        // Save the MAC address.
        pAdapter->ucStationAddress[0] = (BYTE)0x00;
        pAdapter->ucStationAddress[1] = (BYTE)0x80;
        pAdapter->ucStationAddress[2] = (BYTE)0x0F;
        pAdapter->ucStationAddress[3] = (BYTE)0x11;
        pAdapter->ucStationAddress[4] = (BYTE)0x70; 
        pAdapter->ucStationAddress[5] = (BYTE)0x00;
    
    }
    
    //Restore GPIO_CFG register
    SetRegDW(dwLanBase, GPIO_CFG, dwGpioCfg);

    SMSC_TRACE0(DBG_INIT,"-ChipSetup\r\n");
    return (BOOLEAN)TRUE;
}

DWORD LinkIndicate(PSMSC9118_ADAPTER pAdapter)
{
    DWORD dwLinkStatus = LINK_NO_LINK;
    
    if (pAdapter->CurrentPowerState == NdisDeviceStateD0)
    {
        dwLinkStatus = Lan_GetLinkMode(&pAdapter->lan9118_data);
        if ((dwLinkStatus != LINK_NO_LINK) &&
            (pAdapter->lan9118_data.dwLinkMode == LINK_NO_LINK))
        {
            NdisMIndicateStatus(pAdapter->hMiniportAdapterHandle,
                                NDIS_STATUS_MEDIA_CONNECT, (PVOID)0, 0U);
            NdisMIndicateStatusComplete(pAdapter->hMiniportAdapterHandle);
        }
        else if ((dwLinkStatus == LINK_NO_LINK) &&
            (pAdapter->lan9118_data.dwLinkMode != LINK_NO_LINK))
        {
            // Put the chip into D2 (Energy Detection Power Down) 
            // when there is no link
#ifdef  POWERDOWN_AT_NO_LINK
            // If internal PHY
            if (pAdapter->lan9118_data.bPhyAddress == 1)
            {
                // Set Device into D2 (Energy Detect State)
                if (SetPowerState(pAdapter, NdisDeviceStateD2) == FALSE)
                    return LINK_NO_LINK;
            }
#endif

            NdisMIndicateStatus(pAdapter->hMiniportAdapterHandle,
                                NDIS_STATUS_MEDIA_DISCONNECT, (PVOID)0, 0U);
            NdisMIndicateStatusComplete(pAdapter->hMiniportAdapterHandle);
        }
        pAdapter->lan9118_data.dwLinkMode = dwLinkStatus;
    }
    else
    {
        dwLinkStatus = LINK_NO_LINK;
        pAdapter->lan9118_data.dwLinkMode = dwLinkStatus;
        NdisMIndicateStatus(pAdapter->hMiniportAdapterHandle,
                            NDIS_STATUS_MEDIA_DISCONNECT, (PVOID)0, 0U);
        NdisMIndicateStatusComplete(pAdapter->hMiniportAdapterHandle);
    }

    return dwLinkStatus;
}

static void Smsc9118SetMacFilter(CPCSMSC9118_ADAPTER pAdapter)
{
    const DWORD dwLanBase = pAdapter->lan9118_data.dwLanBase;
    DWORD   dwReg;

    dwReg = Lan_GetMacRegDW(dwLanBase, MAC_CR);
    
    dwReg &= (~(MAC_CR_MCPAS_ | MAC_CR_PRMS_ | MAC_CR_INVFILT_ | MAC_CR_HFILT_ | MAC_CR_HPFILT_ | MAC_CR_BCAST_));

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_ALL_MULTICAST)
    {
        dwReg |= MAC_CR_MCPAS_;
    }

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        dwReg |= MAC_CR_PRMS_;
    }

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_MULTICAST)
    {
        dwReg |= MAC_CR_HPFILT_;
    }

    Lan_SetMacRegDW(dwLanBase, MAC_CR, dwReg);
}

void UpdateFilterAndMacReg(CPCSMSC9118_ADAPTER pAdapter)
{
    const DWORD dwLanBase = pAdapter->lan9118_data.dwLanBase;

    // Now safe to change multicasts hash registers
    Lan_SetMacRegDW(dwLanBase, HASHL, pAdapter->ucNicMulticastRegs[0]);
    Lan_SetMacRegDW(dwLanBase, HASHH, pAdapter->ucNicMulticastRegs[1]);

    Smsc9118SetMacFilter(pAdapter);
}

/*----------------------------------------------------------------------------
    Smsc9118QueryInformation
        NDIS miniport function
*/
NDIS_STATUS
Smsc9118QueryInformation (IN NDIS_HANDLE hMiniportAdapterContext, 
                         IN NDIS_OID Oid, 
                         IN PVOID pInformationBuffer,
                         IN ULONG ulInformationBufferLength, 
                         OUT PULONG pulBytesWritten, 
                         OUT PULONG pulBytesNeeded)
{
    PSMSC9118_ADAPTER const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;
    ULONG ulGeneric;
    USHORT usGeneric;
    UCHAR ucGenericArray[6];
    ULONG ulMoveBytes = (DWORD)sizeof(ULONG);
    const NDIS_MEDIUM Medium = NdisMedium802_3;
    const UCHAR VendorString[] = "SMSC9118 Ethernet Controller.";
    const void *pMoveSource = (PVOID)(&ulGeneric);
#ifdef NDIS50_MINIPORT
    NDIS_PNP_CAPABILITIES   NdisPnpCapabilities;
#endif

    SMSC_TRACE1(DBG_QUERY_OID,"+Smsc9118QueryInformation[0x%x]\r\n", Oid);

    SMSC_ASSERT(sizeof(ULONG) == 4U); /*lint !e506 !e944 !e774 */

    switch (Oid)
    {
        case OID_GEN_MAC_OPTIONS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MAC_OPTIONS\r\n");
            ulGeneric = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND | 
                                NDIS_MAC_OPTION_RECEIVE_SERIALIZED | 
                                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                                NDIS_MAC_OPTION_NO_LOOPBACK);
            break;

        case OID_GEN_SUPPORTED_LIST:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_SUPPORTED_LIST\r\n");
            pMoveSource = (PVOID)(Smsc9118SupportedOids);
            ulMoveBytes = (DWORD)sizeof (Smsc9118SupportedOids);
            break;

        case OID_GEN_HARDWARE_STATUS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_HARDWARE_STATUS\r\n");
            HardwareStatus = NdisHardwareStatusReady;
            pMoveSource = (PVOID)(&HardwareStatus);
            ulMoveBytes = (DWORD)sizeof (NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_MEDIA_SUPPORTED:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MEDIA_SUPPORTED\r\n");
            pMoveSource = (PVOID) (&Medium);
            ulMoveBytes = (DWORD)sizeof (NDIS_MEDIUM);
            break;

        case OID_GEN_MEDIA_IN_USE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MEDIA_IN_USE\r\n");
            pMoveSource = (PVOID) (&Medium);
            ulMoveBytes = (DWORD)sizeof (NDIS_MEDIUM);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_CURRENT_LOOKAHEAD\r\n");
            ulGeneric = (DWORD)pAdapter->ulMaxLookAhead;
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MAXIMUM_LOOKAHEAD\r\n");
            ulGeneric = (DWORD)MAX_LOOKAHEAD;
            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_RECEIVE_BLOCK_SIZE\r\n");
            ulGeneric = (ULONG)MAX_PACKET;
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MAXIMUM_FRAME_SIZE\r\n");
            ulGeneric = (ULONG)(1514U - ETHER_HEADER_SIZE);
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MAXIMUM_TOTAL_SIZE\r\n");
            ulGeneric = 1514UL;
            break;

        case OID_GEN_LINK_SPEED:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_LINK_SPEED\r\n");
            if ((pAdapter->lan9118_data.dwLinkMode == LINK_10MPS_HALF) ||
                (pAdapter->lan9118_data.dwLinkMode == LINK_10MPS_FULL))
            {
                ulGeneric = 100000UL;           // 10Mbps
            }
            else
            {
                ulGeneric = 1000000UL;          // 100Mbps
            }
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_TRANSMIT_BUFFER_SPACE\r\n");
            ulGeneric = TX_DATA_FIFO_SIZE;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_RECEIVE_BUFFER_SPACE\r\n");
            ulGeneric = RX_DATA_FIFO_SIZE;
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_TRANSMIT_BLOCK_SIZE\r\n");
            ulGeneric = (ULONG)MAX_PACKET;
            break;

        case OID_GEN_VENDOR_ID:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_VENDOR_ID\r\n");
            NdisMoveMemory ((PVOID)&ulGeneric, pAdapter->ucStationAddress, 3U);
            ulGeneric &= 0xFFFFFF00UL;
            ulGeneric |= 0x01UL;
            pMoveSource = (PVOID)(&ulGeneric);
            ulMoveBytes = (DWORD)sizeof (ulGeneric);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_VENDOR_DESCRIPTION\r\n");
            pMoveSource = VendorString;
            ulMoveBytes = (DWORD)sizeof (VendorString);
            break;

        case OID_GEN_DRIVER_VERSION:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_DRIVER_VERSION\r\n");
            usGeneric = (USHORT)((SMSC9118_NDIS_MAJOR_VERSION << 8) | SMSC9118_NDIS_MINOR_VERSION);
            pMoveSource = (PVOID)(&usGeneric);
            ulMoveBytes = (DWORD)sizeof (usGeneric);
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_PERMANENT_ADDRESS\r\n");
            NdisMoveMemory ((PCHAR)ucGenericArray, pAdapter->ucStationAddress, ETHER_LENGTH_OF_ADDRESS);
            pMoveSource = (PVOID)ucGenericArray;
            ulMoveBytes = (DWORD)sizeof (pAdapter->ucStationAddress);
            break;

        case OID_802_3_CURRENT_ADDRESS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_CURRENT_ADDRESS\r\n");
            NdisMoveMemory ((PCHAR)ucGenericArray, pAdapter->ucStationAddress, ETHER_LENGTH_OF_ADDRESS);
            pMoveSource = (PVOID)ucGenericArray;
            ulMoveBytes = (DWORD)sizeof (pAdapter->ucStationAddress);
            break;

        case OID_802_3_MULTICAST_LIST:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_MULTICAST_LIST\r\n");
            pMoveSource = (PVOID)(pAdapter->ucAddresses);
            ulMoveBytes = (DWORD)(DEFAULT_MULTICASTLISTMAX*ETHER_LENGTH_OF_ADDRESS);
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_MAXIMUM_LIST_SIZE\r\n");
            ulGeneric = (DWORD)DEFAULT_MULTICASTLISTMAX;
            break;

        case OID_GEN_XMIT_OK:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_XMIT_OK\r\n");
            ulGeneric = (pAdapter->ulFramesXmitGood);
            break;

        case OID_GEN_RCV_OK:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_RCV_OK\r\n");
            ulGeneric = (pAdapter->ulFramesRcvGood);
            break;

        case OID_GEN_XMIT_ERROR:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_XMIT_ERROR\r\n");
            ulGeneric = (pAdapter->ulFramesXmitBad);
            break;

        case OID_GEN_RCV_ERROR:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_RCV_ERROR\r\n");
            ulGeneric = (pAdapter->ulFramesRcvBad);
            break;

        case OID_GEN_RCV_NO_BUFFER:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_RCV_NO_BUFFER\r\n");
            ulGeneric = (pAdapter->ulMissedPackets);
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MEDIA_CONNECT_STATUS\r\n");
            if (LinkIndicate(pAdapter) == LINK_NO_LINK)
            {
                ulGeneric = (ULONG)NdisMediaStateDisconnected; /*lint !e930 */
            }
            else
            {
                ulGeneric = (ULONG)NdisMediaStateConnected; /*lint !e930 */
            }
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_GEN_MAXIMUM_SEND_PACKETS\r\n");
            ulGeneric = MAX_NUM_PACKETS_PER_SEND;
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            usGeneric = (USHORT)DRIVER_VERSION;
            pMoveSource = (PVOID)(&usGeneric);
            ulMoveBytes = (DWORD)sizeof (usGeneric);
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_RCV_ERROR_ALIGNMENT\r\n");
            ulGeneric = (pAdapter->ulFrameAlignmentErrors);
            break;

        case OID_802_3_XMIT_ONE_COLLISION:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_XMIT_ONE_COLLISION\r\n");
            ulGeneric = (pAdapter->ulFramesXmitOneCollision);
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:
            SMSC_TRACE0(DBG_QUERY_OID,"  OID_802_3_XMIT_MORE_COLLISIONS\r\n");
            ulGeneric = (pAdapter->ulFramesXmitManyCollisions);
            break;

        //
        //  Power Management
        //
        case OID_PNP_CAPABILITIES:
            SMSC_TRACE0(DBG_POWER,"  OID_PNP_CAPABILITIES\r\n");
            NdisZeroMemory(&NdisPnpCapabilities, sizeof(NdisPnpCapabilities));
            NdisPnpCapabilities.WakeUpCapabilities.MinPatternWakeUp = 
                pAdapter->WakeUpCap.MinPatternWakeUp;
            NdisPnpCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp =
                pAdapter->WakeUpCap.MinLinkChangeWakeUp;
            NdisPnpCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp =
                pAdapter->WakeUpCap.MinMagicPacketWakeUp;
            pMoveSource = (PVOID)&NdisPnpCapabilities; 
            ulMoveBytes = (DWORD)sizeof(NdisPnpCapabilities);
            break;

        case OID_PNP_QUERY_POWER:
            SMSC_TRACE0(DBG_POWER,"  OID_PNP_QUERY_POWER\r\n");
            /*
             *  We do nothing here.
             *
             *  An OID_PNP_QUERY_POWER request is not used to request a 
             *  transition to a device state of D0. NDIS simply sends an 
             *  OID_PNP_SET_POWER request that specifies a device state of D0.
             *  By returning NDIS_STATUS_SUCCESS to this request, 
             *  the miniport guarantees that it will transition the NIC to 
             *  the specified device power state on receipt of a subsequent 
             *  OID_PNP_SET_POWER request. 
             *  The miniport, in this case, must do nothing to jeopardize the 
             *  transition
             */
            break;

        case OID_PNP_ENABLE_WAKE_UP:
            SMSC_TRACE0(DBG_POWER,"  OID_PNP_ENABLE_WAKE_UP (QUERY)\r\n");
            ulGeneric = pAdapter->dwWakeUpSource;
            if((ulGeneric & (DWORD)(NDIS_PNP_WAKE_UP_LINK_CHANGE | NDIS_PNP_WAKE_UP_MAGIC_PACKET | NDIS_PNP_WAKE_UP_PATTERN_MATCH)) ==
               (DWORD)(NDIS_PNP_WAKE_UP_LINK_CHANGE | NDIS_PNP_WAKE_UP_MAGIC_PACKET | NDIS_PNP_WAKE_UP_PATTERN_MATCH)
              )
            {
                SMSC_WARNING0("NDIS_PNP_WAKE_UP_LINK_CHANGE is set with NDIS_PNP_WAKE_UP_MAGIC_PACKET, NDIS_PNP_WAKE_UP_PATTERN_MATCH.\r\n");
            }
            break;                

        case OID_NDIS_SMSC_DUMP_ALL_REGS:
            DumpAllRegs(pAdapter);
            break;      

        default:
            SMSC_TRACE0(DBG_QUERY_OID,"  Unrecognized OID\r\n");
            StatusToReturn = NDIS_STATUS_INVALID_OID;
            break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS)
    {
        if (ulMoveBytes > ulInformationBufferLength)
        {
            SMSC_WARNING0("  Invalid Buffer Length\r\n");
            *pulBytesNeeded = ulMoveBytes;
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            SMSC_TRACE0(DBG_QUERY_OID,"  Information Moved Successfully\r\n");
            if ((ulMoveBytes > 0x00UL) && (pInformationBuffer != NULL)) {
                NdisMoveMemory (pInformationBuffer, pMoveSource, (UINT)ulMoveBytes);
                (*pulBytesWritten) += ulMoveBytes;
            }
        }
    }

    // Make Lint Happy
    ulInformationBufferLength = ulInformationBufferLength;

    SMSC_TRACE0(DBG_QUERY_OID,"-Smsc9118QueryInformation\r\n");
    return (StatusToReturn);
}



/*----------------------------------------------------------------------------
    ComputeCrc
        Calculate CRC32 for multicast function.
*/
DWORD ComputeCrc(IN const CPUCHAR pBuffer, IN const UINT uiLength)
{
    UINT i;
    DWORD crc = 0xFFFFFFFFUL;
    DWORD result = 0UL;
    const DWORD poly = 0xEDB88320UL;

    SMSC_TRACE0(DBG_MULTICAST,"+ComputeCrc\r\n");

    SMSC_TRACE1(DBG_MULTICAST,"uiLength=%d\r\n", uiLength);

    for(i=0U; i<uiLength; i++) 
    {
        int bit;
        DWORD data=((DWORD)pBuffer[i]);
        for(bit=0; bit<8; bit++) 
        {
            const DWORD p = (crc^((DWORD)data))&1UL;
            crc >>= 1;
            if(p != 0UL) {
                crc ^= poly;
            }
            data >>=1;
        }
    }
    result=((crc&0x01UL)<<5)|
           ((crc&0x02UL)<<3)|
           ((crc&0x04UL)<<1)|
           ((crc&0x08UL)>>1)|
           ((crc&0x10UL)>>3)|
           ((crc&0x20UL)>>5);

    SMSC_TRACE0(DBG_MULTICAST,"-ComputeCrc\r\n");
    return (result);

}


/*----------------------------------------------------------------------------
    GetMulticastBit
        Calculate multicase table.
*/
VOID
GetMulticastBit(IN const UCHAR * const ucAddress, OUT PUCHAR const pTable, OUT PULONG const pValue)
{
    DWORD uiBitNumber;

    SMSC_TRACE0(DBG_MULTICAST,"+GetMulticastBit\r\n");

    uiBitNumber = ComputeCrc(ucAddress, (UINT)ETHER_LENGTH_OF_ADDRESS);
    *pTable = (UCHAR)(((uiBitNumber & 0x20UL) >> 5) & 1UL);
    *pValue = (DWORD)(1UL << (uiBitNumber & 0x1FUL));

    SMSC_TRACE0(DBG_MULTICAST,"-GetMulticastBit\r\n");
}

void DisableMacRxEn(CPCSMSC9118_ADAPTER pAdapter)
{
    volatile DWORD  dwReg;

    // Disable RX 
    dwReg = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR);
    if (dwReg & MAC_CR_RXEN_)
    {
        volatile long   counter;

        SetRegDW(pAdapter->lan9118_data.dwLanBase, INT_STS, INT_STS_RXSTOP_INT_);
        dwReg = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR);
        dwReg &= ~MAC_CR_RXEN_;
        Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR, dwReg);

        counter = (long)GetRegDW(pAdapter->lan9118_data.dwLanBase, FREE_RUN);
        while (!((dwReg=GetRegDW(pAdapter->lan9118_data.dwLanBase, INT_STS)) & INT_STS_RXSTOP_INT_))
        {
            if (((long)GetRegDW(pAdapter->lan9118_data.dwLanBase, FREE_RUN) - counter) > (25L*2000L))
            {
                break;
            }
        }

        SetRegDW(pAdapter->lan9118_data.dwLanBase, INT_STS, INT_STS_RXSTOP_INT_);
    }
}

void EnableMacRxEn(CPCSMSC9118_ADAPTER pAdapter)
{
    volatile DWORD  dwReg;

    // Enable RX 
    dwReg = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR);
    dwReg |= MAC_CR_RXEN_;
    Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR, dwReg);

    // Wait for at least 64uSec
    DelayUsingFreeRun(pAdapter, 64L);
}

void DelayUsingFreeRun(CPCSMSC9118_ADAPTER pAdapter, const LONG uSec)
{
    volatile long   counter;

    counter = (long)GetRegDW(pAdapter->lan9118_data.dwLanBase, FREE_RUN);
    while (((long)GetRegDW(pAdapter->lan9118_data.dwLanBase, FREE_RUN) - counter) < (25L*uSec))
    {
    }
}

/*----------------------------------------------------------------------------
    SetMulticastAddressList

*/
NDIS_STATUS
SetMulticastAddressList (IN PSMSC9118_ADAPTER pAdapter)
{
    UINT uiIndex;
    UCHAR Table;
    DWORD dwBit;
    NDIS_STATUS Ret = NDIS_STATUS_SUCCESS;

    SMSC_TRACE0(DBG_INIT,"+SetMulticastAddressList\r\n");

    pAdapter->ucNicMulticastRegs[0] = 0UL;
    pAdapter->ucNicMulticastRegs[1] = 0UL;

    for (uiIndex = 0U; uiIndex < DEFAULT_MULTICASTLISTMAX; uiIndex++)
    {
        SMSC_TRACE6(DBG_MULTICAST,"%x-%x-%x-%x-%x-%x\r\n", 
                pAdapter->ucAddresses[uiIndex][0], 
                pAdapter->ucAddresses[uiIndex][1], 
                pAdapter->ucAddresses[uiIndex][2], 
                pAdapter->ucAddresses[uiIndex][3], 
                pAdapter->ucAddresses[uiIndex][4], 
                pAdapter->ucAddresses[uiIndex][5]);

        GetMulticastBit (pAdapter->ucAddresses[uiIndex], &Table, &dwBit);
        SMSC_TRACE2(DBG_MULTICAST,"Table=0x%x, dwBit=0x%x\r\n", Table, dwBit);

        pAdapter->ucNicMulticastRegs[Table] |= dwBit;
        SMSC_TRACE2(DBG_MULTICAST,"ucNicMulticastRegs[0]=0x%x, ucNicMulticastRegs[1]=0x%x\r\n", pAdapter->ucNicMulticastRegs[0], pAdapter->ucNicMulticastRegs[1]);
    }

    Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, HASHL, 
                    pAdapter->ucNicMulticastRegs[0]);
    Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, HASHH, 
                    pAdapter->ucNicMulticastRegs[1]);

    SMSC_TRACE2(DBG_MULTICAST,"HASHH=0x%x, HASHL=0x%x\r\n", 
            Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, HASHH),
            Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, HASHL));
    
    SMSC_TRACE0(DBG_INIT,"-SetMulticastAddressList\r\n");
    return Ret;
}

/*----------------------------------------------------------------------------
    SetPacketFilter

*/
NDIS_STATUS
SetPacketFilter(IN PSMSC9118_ADAPTER pAdapter)
{
    DWORD   dwReg;
    NDIS_STATUS Ret = NDIS_STATUS_SUCCESS;

    SMSC_TRACE0(DBG_MULTICAST,"+SetPacketFilter\r\n");

    // Disable Rx on Mac
    DisableMacRxEn(pAdapter);

    dwReg = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR);

    dwReg &= (~(MAC_CR_MCPAS_ | MAC_CR_PRMS_ | MAC_CR_INVFILT_ | MAC_CR_HFILT_ | MAC_CR_HPFILT_ | MAC_CR_BCAST_));

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_ALL_MULTICAST)
    {
        dwReg |= MAC_CR_MCPAS_;
    }

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        dwReg |= MAC_CR_PRMS_;
    }

    if (pAdapter->ulPacketFilter & (DWORD)NDIS_PACKET_TYPE_MULTICAST)
    {
        dwReg |= MAC_CR_HPFILT_;
    }

    // Enable Rx on Mac
    EnableMacRxEn(pAdapter);

    SMSC_TRACE1(DBG_MULTICAST,"***(after) MAC_CR=0x%x\r\n", 
            Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR));

    SMSC_TRACE1(DBG_MULTICAST,"*** ADDRH=0x%x\r\n", 
            Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, ADDRH));
    SMSC_TRACE1(DBG_MULTICAST,"*** ADDRL=0x%x\r\n", 
            Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, ADDRL));

    SMSC_TRACE0(DBG_MULTICAST,"-SetPacketFilter\r\n");
    return Ret;
}


/*----------------------------------------------------------------------------
    Smsc9118SetInformation
        NDIS miniport function
*/
NDIS_STATUS
Smsc9118SetInformation (IN NDIS_HANDLE hMiniportAdapterContext, 
                       IN NDIS_OID Oid, 
                       IN PVOID pInformationBuffer,
                       IN ULONG ulInformationBufferLength, 
                       OUT PULONG pulBytesRead, 
                       OUT PULONG pulBytesNeeded)
{
    SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);
    const UCHAR * const pucInfoBuffer = (PUCHAR)(pInformationBuffer);
    ULONG ulPacketFilter, ulLookAhead;
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;
    const NDIS_PM_PACKET_PATTERN *pPattern;
    NDIS_DEVICE_POWER_STATE  PowerState;   //power management

    SMSC_TRACE1(DBG_SET_OID,"+Smsc9118SetInformation[0x%x]\r\n", Oid);

    switch (Oid)
    {
        case OID_802_3_MULTICAST_LIST:
            SMSC_TRACE0(DBG_SET_OID | DBG_MULTICAST,"  OID_802_3_MULTICAST_LIST\r\n");
            if ((ulInformationBufferLength % (DWORD)ETHER_LENGTH_OF_ADDRESS) != 0UL)
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *pulBytesRead = 0UL;
                *pulBytesNeeded = 0UL;
                break;
            }
            if (sizeof(pAdapter->ucAddresses) < (UINT)ulInformationBufferLength)
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *pulBytesRead = 0UL;
                *pulBytesNeeded = 0UL;
                break;
            }
            NdisZeroMemory(pAdapter->ucAddresses, DEFAULT_MULTICASTLISTMAX*ETHER_LENGTH_OF_ADDRESS);
            // Set Multicast addresses
            NdisMoveMemory (pAdapter->ucAddresses, pInformationBuffer, (UINT)ulInformationBufferLength);
            StatusToReturn = SetMulticastAddressList(pAdapter);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            SMSC_TRACE0(DBG_SET_OID | DBG_MULTICAST,"  OID_GEN_CURRENT_PACKET_FILTER\r\n");
            if (ulInformationBufferLength != 4UL)
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *pulBytesRead = 0UL;
                *pulBytesNeeded = 0UL;
                break;
            }
            NdisMoveMemory (&ulPacketFilter, pucInfoBuffer, 4U);

            if (ulPacketFilter & (DWORD)(NDIS_PACKET_TYPE_SOURCE_ROUTING | 
                                         NDIS_PACKET_TYPE_SMT | 
                                         NDIS_PACKET_TYPE_MAC_FRAME | 
                                         NDIS_PACKET_TYPE_FUNCTIONAL | 
                                         NDIS_PACKET_TYPE_ALL_FUNCTIONAL | 
                                         NDIS_PACKET_TYPE_GROUP))
            {
                StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
                *pulBytesRead = 4UL;
                *pulBytesNeeded = 0UL;
                RETAILMSG(1, (TEXT("Error! PacketFilter = 0x%08x\r\n"), ulPacketFilter));
                break;
            }
            pAdapter->ulPacketFilter = ulPacketFilter;
            StatusToReturn = SetPacketFilter(pAdapter);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            SMSC_TRACE0(DBG_SET_OID,"  OID_GEN_CURRENT_LOOKAHEAD\r\n");
            if (ulInformationBufferLength != 4UL)
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *pulBytesRead = 0UL;
                *pulBytesNeeded = 0UL;
                break;
            }
            NdisMoveMemory (&ulLookAhead, pucInfoBuffer, 4U);
            if (ulLookAhead <= (DWORD)MAX_LOOKAHEAD) {
                pAdapter->ulMaxLookAhead = ulLookAhead;
            }
            else {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
            }
            break;

        //
        //  Power Management
        //
        case OID_PNP_SET_POWER:
            SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"  OID_PNP_SET_POWER\r\n");
            PowerState = *(PNDIS_DEVICE_POWER_STATE)pInformationBuffer;
            if (SetPowerState(pAdapter, PowerState) == FALSE)
                StatusToReturn = NDIS_STATUS_FAILURE;
            break;

        case OID_PNP_ADD_WAKE_UP_PATTERN:
            SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"  OID_PNP_ADD_WAKE_UP_PATTERN\r\n");
            pPattern = (PNDIS_PM_PACKET_PATTERN)pInformationBuffer;
            SMSC_TRACE1(DBG_SET_OID | DBG_POWER,"MaskSize=%d\r\n", pPattern->MaskSize);
            SMSC_TRACE2(DBG_SET_OID | DBG_POWER,"PatternOffset=%d(0x%x)\r\n", pPattern->PatternOffset, pPattern->PatternOffset);
            SMSC_TRACE1(DBG_SET_OID | DBG_POWER,"PatternSize=%d\r\n", pPattern->PatternSize);
            if ((pPattern->PatternSize < 13UL) || 
                (pPattern->MaskSize < 2UL) || 
                (pPattern->MaskSize > 6UL))   //pPattern->MaskSize always <6(?)
            {
                // Pattern is too short, since 9118 start checking after MAC dst/src address.
                if (pPattern->PatternSize < 13UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER, "WAKEUP Pattern Error: PatternSize is smaller than 13 bytes.\r\n");
                }

                if (pPattern->MaskSize < 2UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER, "WAKEUP Pattern Error: MaskSize is smaller than 2 bytes.\r\n");
                }
                else if (pPattern->MaskSize > 6UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER, "WAKEUP Pattern Error: MaskSize is larger than 6 bytes.\r\n");
                }
                else {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER, "WAKEUP Pattern Error: Error! 2 <= MaskSize <= 6\r\n");
                }
                StatusToReturn = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
            else
            {
                if ( ((pAdapter->Wuff.FilterCommands)&(FILTER3_ENABLE|FILTER2_ENABLE|FILTER1_ENABLE|FILTER0_ENABLE)) == 
                     (FILTER3_ENABLE|FILTER2_ENABLE|FILTER1_ENABLE|FILTER0_ENABLE) )
                {
                    //All Wuff slots are used.
                    SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"No filter is available(all 4 filters in use).\r\n");
                    StatusToReturn = NDIS_STATUS_RESOURCES;
                    break;
                }
                else
                {
                    //There is at least one filter available.
                    if( !((pAdapter->Wuff.FilterCommands)&FILTER0_ENABLE) )
                    {
                        SetWakeUpFrameFilter(pAdapter, pPattern, 0UL);
                        (pAdapter->Wuff.FilterCommands) |= FILTER0_ENABLE;
                        (pAdapter->Wuff.FilterCommands) &= ~FILTER0_ADDR_TYPE;
                    }
                    else
                    {
                        if( !((pAdapter->Wuff.FilterCommands)&FILTER1_ENABLE) )
                        {
                            SetWakeUpFrameFilter(pAdapter, pPattern, 1UL);
                            (pAdapter->Wuff.FilterCommands) |= FILTER1_ENABLE;
                            (pAdapter->Wuff.FilterCommands) &= ~FILTER1_ADDR_TYPE;
                        }
                        else
                        {
                            if( !((pAdapter->Wuff.FilterCommands)&FILTER2_ENABLE) )
                            {
                                SetWakeUpFrameFilter(pAdapter, pPattern, 2UL);
                                (pAdapter->Wuff.FilterCommands) |= FILTER2_ENABLE;
                                (pAdapter->Wuff.FilterCommands) &= ~FILTER2_ADDR_TYPE;
                            }
                            else
                            {
                                if( !((pAdapter->Wuff.FilterCommands)&FILTER3_ENABLE) )
                                {
                                    SetWakeUpFrameFilter(pAdapter, pPattern, 3UL);
                                    (pAdapter->Wuff.FilterCommands) |= FILTER3_ENABLE;
                                    (pAdapter->Wuff.FilterCommands) &= ~FILTER3_ADDR_TYPE;
                                }
                            }
                        }
                    }

                }
            }
            break;

        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
            SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"  OID_PNP_REMOVE_WAKE_UP_PATTERN\r\n");

            pPattern = (PNDIS_PM_PACKET_PATTERN)pInformationBuffer;
            SMSC_TRACE1(DBG_SET_OID | DBG_POWER,"MaskSize=%d\r\n", pPattern->MaskSize);
            SMSC_TRACE2(DBG_SET_OID | DBG_POWER,"PatternOffset=%d(0x%x)\r\n", pPattern->PatternOffset, pPattern->PatternOffset);
            SMSC_TRACE1(DBG_SET_OID | DBG_POWER,"PatternSize=%d\r\n", pPattern->PatternSize);

            if ((pPattern->PatternSize < 13UL) || 
                (pPattern->MaskSize < 2UL) || 
                (pPattern->MaskSize > 6UL))   //pPattern->MaskSize always <6(?)
            {
                // Pattern is too short, since 9118 start checking after MAC dst/src address.
                if (pPattern->PatternSize < 13UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"PatternSize is smaller than 13 bytes.\r\n");
                }

                if (pPattern->MaskSize < 2UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"MaskSize is smaller than 2 bytes.\r\n");
                }
                else if (pPattern->MaskSize > 6UL) {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"MaskSize is larger than 6 bytes.\r\n");
                }
                else {
                   SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"Error! 2 <= MaskSize <= 6\r\n");
                }

                SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"No pattern is removed.\r\n");
                StatusToReturn = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
            else
            {
                if ((pAdapter->Wuff.FilterCommands & 
                    (FILTER3_ENABLE|FILTER2_ENABLE|FILTER1_ENABLE|FILTER0_ENABLE)) == 0UL )
                {
                    //No pattern is enabled(nothing to remove). Return success.
                    SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"No filter is enabled(all 4 filters NOT in use).\r\n");
                    break;
                }
                else
                {
                    //There is at least one filter enabled.
                    ResetWakeUpFrameFilter(pAdapter, pPattern);
                }
            }
            break;

        case OID_PNP_ENABLE_WAKE_UP:
            SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"  OID_PNP_ENABLE_WAKE_UP (SET)\r\n");
            // Check if all three modes are requested to set.
            if( ((pAdapter->dwWakeUpSource) & 
                 (DWORD)(NDIS_PNP_WAKE_UP_LINK_CHANGE | 
                         NDIS_PNP_WAKE_UP_MAGIC_PACKET | 
                         NDIS_PNP_WAKE_UP_PATTERN_MATCH)) == 
                 (DWORD)(NDIS_PNP_WAKE_UP_LINK_CHANGE | 
                         NDIS_PNP_WAKE_UP_MAGIC_PACKET | 
                         NDIS_PNP_WAKE_UP_PATTERN_MATCH) )
            {
                SMSC_WARNING0("NDIS_PNP_WAKE_UP_LINK_CHANGE is set with NDIS_PNP_WAKE_UP_MAGIC_PACKET, NDIS_PNP_WAKE_UP_PATTERN_MATCH.\r\n");
            }

            pAdapter->dwWakeUpSource = *(PDWORD)pInformationBuffer;
            if((pAdapter->dwWakeUpSource) & (DWORD)NDIS_PNP_WAKE_UP_MAGIC_PACKET) {
                SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"NDIS_PNP_WAKE_UP_MAGIC_PACKET is set.\r\n");
            }
            if((pAdapter->dwWakeUpSource) & (DWORD)NDIS_PNP_WAKE_UP_PATTERN_MATCH) {
                SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"NDIS_PNP_WAKE_UP_PATTERN_MATCH is set.\r\n");
            }
            if((pAdapter->dwWakeUpSource) & (DWORD)NDIS_PNP_WAKE_UP_LINK_CHANGE) {
                SMSC_TRACE0(DBG_SET_OID | DBG_POWER,"NDIS_PNP_WAKE_UP_LINK_CHANGE is set.\r\n");
            }
            break;

        default:
            SMSC_TRACE0(DBG_SET_OID,"  Unrecognozed OID\r\n");
            StatusToReturn = NDIS_STATUS_INVALID_OID;
            *pulBytesRead = 0UL;
            *pulBytesNeeded = 0UL;
            break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE1(DBG_SET_OID,"Smsc9118SetInformation[0x%x] Successful\r\n", Oid);
        *pulBytesRead = ulInformationBufferLength;
        *pulBytesNeeded = 0UL;
    }

    // Make Lint Happy
    pInformationBuffer = pInformationBuffer;

    SMSC_TRACE1(DBG_SET_OID,"-Smsc9118SetInformation[0x%x]\r\n", Oid);
    return (StatusToReturn);
}


/*----------------------------------------------------------------------------
    ChipStart
        Start the chip by autonegotiating the link.
*/
VOID ChipStart(IN PSMSC9118_ADAPTER pAdapter)
{
    BOOL    bRet;
    DWORD   dwTemp;

    SMSC_TRACE0(DBG_INIT,"+ChipStart\r\n");

    // we don't care about return value now.
    bRet = Lan_EstablishLink(&(pAdapter->lan9118_data), pAdapter->LinkMode, pAdapter->fSwFlowControlEnabled);

    // Make Lint Happy
    bRet = bRet;

    if (pAdapter->lan9118_data.dwLinkMode != LINK_NO_LINK)
    {
        SMSC_TRACE0(DBG_INIT,"Link Established !\r\n");
    }

    //Enable the GPT timer
    dwTemp = GetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG);
    dwTemp &= ~0xFFFFUL;
    dwTemp |= (GPT_INT_INTERVAL | GPT_CFG_TIMER_EN_);
    SetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG, dwTemp);
    Lan_EnableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_GPT_INT_EN_);

    // Enable Rx Overrun Interrupt
    Lan_EnableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_RDFO_EN_);
    // Enable Rx Error Interrupt
    Lan_EnableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_RXE_EN_);

    // Enable Tx Error Interrupt
    Lan_EnableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_TXE_EN_);

    // Enable RWT Interrupt
    Lan_EnableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_RWT_EN_);

    SMSC_TRACE0(DBG_INIT,"-ChipStart\r\n");
}


/*----------------------------------------------------------------------------
    ChipReset
*/
BOOLEAN ChipReset(IN SMSC9118_ADAPTER * const pAdapter)
{
    SMSC_TRACE0(DBG_INIT,"+ChipReset\r\n");

    if (!Lan_Initialize(&(pAdapter->lan9118_data), pAdapter->lan9118_data.dwLanBase))
    {
        SMSC_TRACE0(DBG_INIT,"Lan_Initialize failed.\r\n");
        SMSC_TRACE0(DBG_INIT,"-ChipReset\r\n");
        return (BOOLEAN)FALSE;
    }

    SMSC_TRACE0(DBG_INIT,"-ChipReset\r\n");
    return (BOOLEAN)TRUE;
}


/*----------------------------------------------------------------------------
    Smsc9118Shutdown
        NDIS miniport function
*/
VOID Smsc9118Shutdown (IN NDIS_HANDLE hMiniportAdapterContext)
{
    BOOLEAN bRet;
    SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    SMSC_TRACE0(DBG_INIT,"+Smsc9118Shutdown\r\n");

    // we don't care about return value now
    bRet = ChipReset(pAdapter);

    // Make Lint Happy
    bRet = bRet;

    SMSC_TRACE0(DBG_INIT,"-Smsc9118Shutdown\r\n");
}

void Reset118(IN const PSMSC9118_ADAPTER pAdapter)
{
    PNDIS_PACKET    pPacket;
    DMA_XFER        dmaXfer;

    // Disable Global Interrupt
    NicDisableInterrupt(pAdapter);

    if (pAdapter->fRxDMAMode)
    {
        dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
        DmaComplete(&dmaXfer, DMA_RX_CH);
    }
    if (pAdapter->fTxDMAMode)
    {
        dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
        DmaComplete(&dmaXfer, DMA_TX_CH);
    }

    // Empty Pending Packets
    while (QUEUE_COUNT(&pAdapter->TxDeferedPkt) != 0UL)
    {
        pPacket = pAdapter->TxDeferedPkt.FirstPacket;
        DequeuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket);
        pAdapter->TxDeferedPkt.Count --;
        NdisMSendComplete(pAdapter->hMiniportAdapterHandle, pPacket, NDIS_STATUS_FAILURE);
    }

    pAdapter->fLinkUp = FALSE;

    // Setup (include Reset Chip) chip again
    ChipSetup(pAdapter);

    ChipStart(pAdapter);

    // Enable Global Interrupt
    NicEnableInterrupt(pAdapter);
}

/*----------------------------------------------------------------------------
    Smsc9118Reset
        NDIS miniport function
*/
NDIS_STATUS Smsc9118Reset(OUT PBOOLEAN pbAddressingReset, IN NDIS_HANDLE hMiniportAdapterContext)
{
    SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    SMSC_TRACE0(DBG_INIT,"+Smsc9118Reset\r\n");

    if (pAdapter != NULL)
    {
        //DumpAllRegs(pAdapter);
        Reset118(pAdapter);
        *pbAddressingReset = (BOOLEAN)TRUE;
    }
    else
    {
        SMSC_WARNING0("Error! Smsc9118Reset: pAdapter == NULL\r\n");
        *pbAddressingReset = (BOOLEAN)FALSE;
    }

    SMSC_TRACE0(DBG_INIT,"-Smsc9118Reset\r\n");

    return NDIS_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------
    Smsc9118Halt
        NDIS miniport function
*/
VOID Smsc9118Halt(IN NDIS_HANDLE hMiniportAdapterContext)
{
    DWORD       dw;
    SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    SMSC_TRACE0(DBG_INIT,"+Smsc9118Halt\r\n");

    // Disable Global Interrupt
    NicDisableInterrupt(pAdapter);

    //De-register shutdown handler.
    NdisMDeregisterAdapterShutdownHandler(pAdapter->hMiniportAdapterHandle);

    //De-register interrupt.
    NdisMDeregisterInterrupt (&(pAdapter->Interrupt));
    NdisMSleep(25000UL);        // 25000uSec = 25mSec

    if (gSmsc9118MiniportBlock.AdapterQueue == pAdapter) {
        gSmsc9118MiniportBlock.AdapterQueue = pAdapter->NextAdapter;
    }
    else
    {
        PSMSC9118_ADAPTER TmpAdapter = gSmsc9118MiniportBlock.AdapterQueue;

        while (TmpAdapter->NextAdapter != pAdapter) {
            TmpAdapter = TmpAdapter->NextAdapter;
        }
        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    if ((gSmsc9118MiniportBlock.AdapterQueue == NULL) && gSmsc9118MiniportBlock.NdisWrapperHandle)
    {
        NdisTerminateWrapper(gSmsc9118MiniportBlock.NdisWrapperHandle, NULL);
        gSmsc9118MiniportBlock.NdisWrapperHandle = NULL;
    }

    // Unchain Buffer and Free Buffer
    for (dw = 0UL; dw < (DWORD)MAX_RXPACKETS_IN_QUEUE; dw++)
    {
        NdisUnchainBufferAtBack((pAdapter->RxPacketArray[dw]), &(pAdapter->RxBufferArray[dw]));
        NdisFreeBuffer(pAdapter->RxBufferArray[dw]);
    }
    // Free BufferPool
    NdisFreeBufferPool(pAdapter->hBufferPool);
    
    // Free Packets
    for (dw = 0UL; dw < (DWORD)MAX_RXPACKETS_IN_QUEUE; dw++)
    {
        NdisFreePacket(pAdapter->RxPacketArray[dw]);
    }

    // Free PacketPool
    NdisFreePacketPool(pAdapter->hPacketPool);

    // Free shared memory.
    if (pAdapter->pSharedMemVA)
    {
        NdisFreeMemory(pAdapter->pSharedMemVA, (UINT)sizeof(SMSC9118_SHAREDMEM), NDIS_ALLOC_FLAG);
        pAdapter->pSharedMemVA = NULL;
    }

    // Free VA for DMA
    if ((pAdapter->fRxDMAMode || pAdapter->fTxDMAMode) && pAdapter->DMABaseVA)
    {
        if (VirtualFree ((PVOID)(pAdapter->DMABaseVA), 0UL, (DWORD)MEM_RELEASE) != TRUE) 
        {
            SMSC_TRACE0(DBG_INIT,"Failed! at Smsc9118Halt() to free DMABaseVA\r\n");
        }
        pAdapter->DMABaseVA = 0x00UL;
    }

    // Free VA for 9118
    if (pAdapter->lan9118_data.dwLanBase)
    {
        if (VirtualFree ((PVOID)(pAdapter->lan9118_data.dwLanBase - (pAdapter->ulIoBaseAddress & (((DWORD)PAGE_SIZE)-1UL))), 0UL, (DWORD)MEM_RELEASE) != TRUE)
        {
            SMSC_TRACE0(DBG_INIT,"Failed! at Smsc9118Halt() to free dwLanBase\r\n");
        }
        pAdapter->lan9118_data.dwLanBase = 0x00UL;
    }

    // Free pAdapter.
    NdisFreeMemory(pAdapter, (UINT)sizeof(PSMSC9118_ADAPTER), NDIS_ALLOC_FLAG);

    SMSC_TRACE0(DBG_INIT,"-Smsc9118Halt\r\n");
    return;
}

__inline PNDIS_PACKET GetPktFromEmptyArray(PSMSC9118_ADAPTER pAdapter)
{
    PNDIS_PACKET    pPkt;

    if (!IS_ARRAY_EMPTY(pAdapter->EmptyPkt))
    {
#ifdef  TRACE_BUFFER
        dwRxPktFromEmpty ++;
#endif
        pPkt = pAdapter->EmptyPkt.dwPktArray[pAdapter->EmptyPkt.dwRdPtr];
        INC_PTR(pAdapter->EmptyPkt.dwRdPtr);
        return pPkt;
    }
    else 
    {
        return NULL;
    }
}

__inline PNDIS_PACKET GetPktFromFullArray(PSMSC9118_ADAPTER pAdapter)
{
    PNDIS_PACKET    pPkt;

    if (!IS_ARRAY_EMPTY(pAdapter->FullPkt))
    {
#ifdef  TRACE_BUFFER
        dwRxPktFromFull ++;
#endif
        pPkt = pAdapter->FullPkt.dwPktArray[pAdapter->FullPkt.dwRdPtr];
        INC_PTR(pAdapter->FullPkt.dwRdPtr);
        return pPkt;
    }
    else 
    {
        return NULL;
    }
}

__inline BOOL PutPktToEmptyArray(PSMSC9118_ADAPTER pAdapter, PNDIS_PACKET pPkt)
{
    if (!IS_ARRAY_FULL(pAdapter->EmptyPkt))
    {
#ifdef  TRACE_BUFFER
        dwRxPktToEmpty ++;
#endif
        pAdapter->EmptyPkt.dwPktArray[pAdapter->EmptyPkt.dwWrPtr] = pPkt;
        INC_PTR(pAdapter->EmptyPkt.dwWrPtr);
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}

__inline BOOL PutPktToFullArray(PSMSC9118_ADAPTER pAdapter, PNDIS_PACKET pPkt)
{
    if (!IS_ARRAY_FULL(pAdapter->FullPkt))
    {
#ifdef  TRACE_BUFFER
        dwRxPktToFull ++;
#endif
        pAdapter->FullPkt.dwPktArray[pAdapter->FullPkt.dwWrPtr] = pPkt;
        INC_PTR(pAdapter->FullPkt.dwWrPtr);
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}


/*----------------------------------------------------------------------------
    HandlerRxDPC
        Rx DPC function for processing time consuming tasks out of ISR.
*/
DPC_STATUS HandlerRxDPC(PSMSC9118_ADAPTER pAdapter)
{
    PNDIS_PACKET    RxPacketArrayIndicate[MAX_RXPACKETS_IN_QUEUE];  // Packet array for indication
    PNDIS_PACKET    pPacket;
#ifdef  FOR_CETK
    PNDIS_BUFFER    pBuffer;
    PUCHAR          pucBufAddress;
    UINT            uiBufLen;
#endif
    DWORD           iy, numRxToIndicate;
    DWORD           dwDeferedTxPktCount;
    DPC_STATUS      RetStatus = DPC_STATUS_DONE;

    SMSC_TRACE0(DBG_RX,"+HandlerRxDPC\r\n");

    numRxToIndicate=0UL;
    while ((pPacket = GetPktFromFullArray(pAdapter)) != NULL)
    {
#ifdef  FOR_CETK
        // If Multicast Packet and MULTICAST flag is ON
        if ((GET_PACKET_RESERVED_RXSTS(pPacket) & RX_STS_MULTICAST) &&
            ((pAdapter->ulPacketFilter & (DWORD)(NDIS_PACKET_TYPE_ALL_MULTICAST|NDIS_PACKET_TYPE_PROMISCUOUS|NDIS_PACKET_TYPE_MULTICAST)) == (DWORD)NDIS_PACKET_TYPE_MULTICAST))
        {
            UINT    ix;

            NdisQueryPacket(pPacket, NULL, NULL, &pBuffer, NULL);
            NdisQueryBuffer(pBuffer, (PVOID*)(&pucBufAddress), &uiBufLen);
            if ((pucBufAddress[0] & 0x03) == 0x01) 
            {
                for (ix = 0U; ix < DEFAULT_MULTICASTLISTMAX; ix++) 
                {
                    if (NdisEqualMemory(pucBufAddress, pAdapter->ucAddresses[ix], ETHER_LENGTH_OF_ADDRESS))
                    {
                        // matched to address in list
                        break;
                    }
                }
                if (ix == DEFAULT_MULTICASTLISTMAX)
                {
                    // nothing matched
                    // Put Packet back to EmptyArray
                    if (PutPktToEmptyArray(pAdapter, pPacket) == FALSE)
                    {
                        SMSC_TRACE0(DBG_RX, "Empty Array Full\r\n");
                    }
                    continue;       // continue to while
                }
            }
        }
#endif
        RxPacketArrayIndicate[numRxToIndicate] = pPacket;
        numRxToIndicate++;
        if (numRxToIndicate > (DWORD)MAX_RXPACKETS_IN_QUEUE)
        {
            SMSC_WARNING0("Exceed MAX_RXPACKETS_IN_QUEUE!!!\r\n");
            break;
        }

        // If too many packets are defered in Tx, 
        // get out from Rx loop to give chance to Tx path
        dwDeferedTxPktCount = QUEUE_COUNT(&pAdapter->TxDeferedPkt);
        if (dwDeferedTxPktCount > BALANCE_TX_DEFER_PACKET_LIMIT)
        {
            RetStatus = DPC_STATUS_PENDING;
            break;
        }
    }

    // Indicate packets.
    if (numRxToIndicate)
    {
        NdisMIndicateReceivePacket(pAdapter->hMiniportAdapterHandle,
                                   RxPacketArrayIndicate,
                                   (UINT)numRxToIndicate);

        for (iy=0UL; iy<numRxToIndicate; iy++)
        {
            if (NDIS_GET_PACKET_STATUS(RxPacketArrayIndicate[iy]) == NDIS_STATUS_SUCCESS) 
            {
#ifdef  TRACE_BUFFER
                dwRxNumIndicate++;
#endif
                if (PutPktToEmptyArray(pAdapter, RxPacketArrayIndicate[iy]) == FALSE) 
                {
                    SMSC_TRACE0(DBG_RX, "Empty Array Full\r\n");
                    SMSC_WARNING0("EmptyBuffer is Full\r\n");
                }
            }
            else
            {
                if (NDIS_GET_PACKET_STATUS(RxPacketArrayIndicate[iy]) != NDIS_STATUS_PENDING)
                    SMSC_WARNING0("Packet is returned to Non-NDIS_STATUS_SUCCESS.\r\n");
            }

        }
        SMSC_TRACE1(DBG_RX,"numRxToIndicate = %d\r\n", numRxToIndicate);

    }

    SMSC_TRACE0(DBG_RX,"-HandlerRxDPC\r\n");

    return RetStatus;
}


/*----------------------------------------------------------------------------
    RxFastForward
*/
void RxFastForward(IN const SMSC9118_ADAPTER * const pAdapter, IN DWORD dwDWCount)
{
    DWORD   dwLanBase, dwTimeOut;

    dwLanBase = pAdapter->lan9118_data.dwLanBase;

    if (dwDWCount >= 4)
    {
        SetRegDW(dwLanBase, RX_DP_CTL, RX_DP_CTL_FFWD_BUSY_);
        B2BWriteDelay(dwLanBase, 1);

        dwTimeOut = 10000UL;
        while ((GetRegDW(dwLanBase, RX_DP_CTL) & RX_DP_CTL_FFWD_BUSY_) &&
               (dwTimeOut > 0UL)) 
        {
            SMSC_MICRO_DELAY(1U);
            dwTimeOut--;
        }

        if(dwTimeOut == 0UL) {
            SMSC_WARNING1("Timeout in RX FFWD. RX_DP_CTRL=0x%x", GetRegDW(dwLanBase, RX_DP_CTL));
        }
    }   
    else
    {
        DWORD   dummy[4];

        // in case dwDWCount < 4, read RX_FIFO out to dump data
        Lan_ReadRxFifo(dwLanBase, dummy, dwDWCount);
    }
}

/*----------------------------------------------------------------------------
    DMA and PIO mode HandlerRxISR(...)
*/
__inline void HandlerRxISR(PSMSC9118_ADAPTER pAdapter)
{
    PUCHAR pucBufAddress;
    UINT uiBufLen;
    PNDIS_BUFFER pBuffer;
    PNDIS_PACKET pPacket;
    DWORD RxStatus, packet_len, dw;
    DWORD numPacketReceived;
    DWORD RxFFCountDW;
    DMA_XFER dmaXfer;

    SMSC_TRACE0(DBG_RX,"+HandlerRxISR\r\n");

    dw=GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_FIFO_INF);
    numPacketReceived = (dw & RX_FIFO_INF_RXSUSED_) >> 16;
    SMSC_TRACE1(DBG_RX,"numPacketReceived = %d\r\n", numPacketReceived);

    pAdapter->RxDPCNeeded = (BOOLEAN)FALSE;
    while(numPacketReceived)
    {
#ifdef  TRACE_BUFFER
        dwRxTotalPkt++;
#endif
        // Pop out the status DW.
        RxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_STATUS_FIFO_PORT);
        packet_len = (RxStatus & RX_STATUS_PACKET_LEN) >> 16;

        // If no error, make the transfer.
#ifdef  FOR_CETK
        // For CETK
        // NOTE CETK sends wrong length 802.3 Frame :-(
        // So, ignore that error when CETK is running
        if (!(RxStatus & RX_STS_ES))
#else
        // Should discard packet with length error
        if (!(RxStatus & RX_STS_ES) && 
            !(((RxStatus & RX_STS_LENGTH_ERR) == RX_STS_LENGTH_ERR) && 
              ((RxStatus & RX_STS_FRAME_TYPE) == 0UL)))
#endif
        {
            if ((pPacket = GetPktFromEmptyArray(pAdapter)) != NULL)
            {
                SMSC_TRACE1(DBG_RX,"pPacket = 0x%x\r\n", pPacket);

                // Do the transfer.
          
                // Prepare the descripter:
                //      set the HeaderSize of OOB data to 14
                //      SizeMediaSpecificInfo = 0
                //      MediaSpecificInformation = NULL
                //      Set status to: Resources or Success
                NDIS_SET_PACKET_HEADER_SIZE(pPacket, ETHER_HEADER_SIZE);
                NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);

                // Query packet and buffer to get the buffer addr.
                NdisQueryPacket (pPacket,
                                 NULL, 
                                 NULL, 
                                 &pBuffer, 
                                 NULL);

                NdisQueryBuffer (pBuffer, 
                                 (PVOID*)(&pucBufAddress),
                                 &uiBufLen);

                SET_PACKET_RESERVED_RXSTS(pPacket, RxStatus);

                // Read data from nic
                if (pAdapter->fRxDMAMode)
                {
                    // DMA
                    dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
                    // Wait for DMA to complete.
                    DmaComplete(&dmaXfer, DMA_RX_CH);
                    dmaXfer.pdwBuf = (DWORD *)CACHELINE_ALIGN((((DWORD)GET_PACKET_RESERVED_PA(pPacket))&~3UL));
                    dmaXfer.dwDwCnt = CACHELINE_ALIGN(packet_len+2UL)>>2;
                    dmaXfer.fMemWr = (BOOLEAN)TRUE;
                    dmaXfer.fMemAddrInc = (BOOLEAN)TRUE;
                    dmaXfer.dwDmaCh = DMA_RX_CH;
                    dmaXfer.fClBurst = (BOOLEAN)TRUE;
                    dmaXfer.dwLanReg = pAdapter->ulIoBaseAddress;  // Phys address of 9118 I/O base
                    dmaXfer.fLanAddrInc = (BOOLEAN)FALSE;

                    SMSC_TRACE1(DBG_DMA,"pucBufAddress=0x%x\r\n", pucBufAddress);
                    SMSC_TRACE1(DBG_DMA,"packet_len=%d\r\n", packet_len);
                    SMSC_TRACE1(DBG_DMA,"dwDwCnt=%d\r\n", dmaXfer.dwDwCnt);

                    // Kick off DMA
                    if (!DmaStartXfer(&dmaXfer)) {
                        SMSC_WARNING0("Rx DMA Failed.\r\n");
                    }
                }
                else
                {
                    //
                    // PIO
                    //
                    ReadFifo(pAdapter->lan9118_data.dwLanBase, 
                             RX_DATA_FIFO_PORT, 
                             (DWORD*)(((DWORD)pucBufAddress)&~3UL),   // There is 2-byte offset in buffer.
                             (((packet_len+2UL+3UL)>>2)));
                }

                // Adjust the buffer length
                // Remove CRC size (4bytes)
                NdisAdjustBufferLength(pBuffer, (UINT)packet_len-4U);

                if (PutPktToFullArray(pAdapter, pPacket) == FALSE)
                {
                    SMSC_TRACE0(DBG_RX, "Full Array Full!\r\n");
                    SMSC_WARNING0("Full Array Full!\r\n");
                }
                else 
                {
                    //Update Rx stats
                    (pAdapter->ulFramesRcvGood)++;
                    SMSC_TRACE0(DBG_RX,"1 packet read successfully\r\n");
                }
            }
            else
            {
#ifdef  TRACE_BUFFER
                dwRxDiscard++;
#endif
                // Out of buffer. Read off and discard the packet.
                if (pAdapter->fRxDMAMode)
                {
                    // DMA
                    // Wait for DMA to complete.
                    dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
                    DmaComplete(&dmaXfer, DMA_RX_CH);

                    RxFFCountDW = CACHELINE_ALIGN(packet_len+2UL)>>2;
                    RxFastForward(pAdapter, RxFFCountDW);
                }
                else
                {
                    // PIO
                    RxFFCountDW = (packet_len+2UL+3UL)>>2;
                    RxFastForward(pAdapter, RxFFCountDW);
                }

                // Count the dropped packet.
                (pAdapter->ulMissedPackets)++;
            }
            pAdapter->RxDPCNeeded = (BOOLEAN)TRUE;
        }
        else
        {
            //
            // Packet has error. Discard it and update stats.
            //

            if (pAdapter->fRxDMAMode)
            {
                // DMA
                // Wait for DMA to complete.
                dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
                DmaComplete(&dmaXfer, DMA_RX_CH);
                RxFFCountDW = CACHELINE_ALIGN(packet_len+2UL)>>2;
                RxFastForward(pAdapter, RxFFCountDW);
            }
            else
            {
                // PIO
                RxFFCountDW = (packet_len+2UL+3UL)>>2;
                RxFastForward(pAdapter, RxFFCountDW);
            }
            
            // Count the dropped packet.
            (pAdapter->ulFramesRcvBad)++;
        }

        // Update numPacketReceived for loop.
        dw=GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_FIFO_INF);
        numPacketReceived = (dw&0x00FF0000UL)>>16;
    }

    SMSC_TRACE0(DBG_RX,"-HandlerRxISR\r\n");
}

/*----------------------------------------------------------------------------
    Smsc9118GetReturnedPackets
        NDIS miniport function
*/
VOID
Smsc9118GetReturnedPackets(IN NDIS_HANDLE hMiniportAdapterContext,
                          IN NDIS_PACKET * pPacketReturned)
{
    SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);
    
    SMSC_TRACE0(DBG_RX,"+Smsc9118GetReturnedPackets\r\n");

    SMSC_ASSERT(pAdapter);
    
    NDIS_SET_PACKET_STATUS(pPacketReturned, NDIS_STATUS_SUCCESS);
    if (PutPktToEmptyArray(pAdapter, pPacketReturned) == FALSE)
    {
        SMSC_TRACE0(DBG_RX, "Empty Array Full\r\n");
    }

    SMSC_TRACE0(DBG_RX,"-Smsc9118GetReturnedPackets\r\n");
}


/*----------------------------------------------------------------------------
    Smsc9118XmitOnePacket
*/
static
__inline BOOL Smsc9118XmitOnePacket(IN NDIS_HANDLE hMiniportAdapterContext, IN PNDIS_PACKET pPacket)
{
    //PUCHAR    pucBufAddress;
    PDWORD  pucBufAddress;
    UINT    uiBufLen, uiPacketLength, uiBufCount, uiTemp;
    volatile DWORD  dwTxCmdA, dwTxCmdB;
    DWORD   PhysAddrLo, dwCount, TxStatus;
    DWORD   *PhysAddr;
    BOOL    fFistNonZeroBuf = TRUE;
    UINT    i, ArraySize;
    NDIS_PHYSICAL_ADDRESS_UNIT SegmentArray[MAX_NUM_SEGMENTS];
    PNDIS_BUFFER pBuffer, pBufferStart;
    const NDIS_BUFFER *pBufferLast = NULL;
    DMA_XFER dmaXfer;

    SMSC9118_ADAPTER * const pAdapter = (SMSC9118_ADAPTER *)(hMiniportAdapterContext);

    // Make Lint happy
    hMiniportAdapterContext = hMiniportAdapterContext;

    SMSC_TRACE0(DBG_TX,"+Smsc9118XmitOnePacket\r\n");

    NdisQueryPacket (pPacket, NULL, &uiBufCount, &pBuffer, &uiPacketLength);
    pBufferStart = pBuffer;
    SMSC_TRACE1(DBG_TX,"Transmit Packet length is %d bytes.\r\n", uiPacketLength);
    SMSC_TRACE1(DBG_TX,"BufferCount is %d.\r\n", uiBufCount);
    
    //
    // Find the last non-zero buffer segment.
    //
    for (uiTemp=0U; pBuffer && (uiTemp<uiBufCount); uiTemp++)
    {
        NdisQueryBuffer (pBuffer, (PVOID*)&pucBufAddress, &uiBufLen);

        if (uiBufLen)
        {
            pBufferLast = pBuffer;
            NdisGetNextBuffer (pBuffer, &pBuffer);
        }
        else
        {
            NdisGetNextBuffer (pBuffer, &pBuffer);
        }
    }

    // Null pBuffer; return error.
    if ((uiTemp < uiBufCount) && (pBuffer==0))
    {
        SMSC_TRACE0(DBG_TX,"Null pBuffer before end of packet error.\r\n");
        return FALSE;
    }
    
    SMSC_TRACE1(DBG_TX,"pBufferLast = 0x%x\r\n", pBufferLast);

    //
    // Transmit the buffers.
    //
    dwTxCmdB = (((DWORD)uiPacketLength)<<16) | ((DWORD)uiPacketLength);
    SMSC_TRACE1(DBG_TX,"dwTxCmdB = 0x%x\r\n", dwTxCmdB);

    pBuffer = pBufferStart;

    if (((uiBufCount*MAX_NUM_SEGMENTS) <= 86U) || (uiBufCount <= 86U))
    {
        DWORD  pkt_len = 0UL;
        PUCHAR pkt_buf = (PUCHAR)(pAdapter->TxTempPktBuf.pVAddr);

        do
        {
            NdisQueryBuffer (pBuffer, (PVOID*)&pucBufAddress, &uiBufLen);
            SMSC_TRACE1(DBG_TX,"pBuffer = 0x%x\r\n", pBuffer);
            SMSC_TRACE1(DBG_TX,"uiBufLen = 0x%x\r\n", uiBufLen);
            SMSC_TRACE1(DBG_TX,"pucBufAddress = 0x%x.\r\n", pucBufAddress);

            if (pBuffer && (uiBufLen == 0U))
            {
                NdisGetNextBuffer (pBuffer, &pBuffer);
                continue;
            }

            if (pkt_len)
            {
                // there is a buffer which is not sent last iteration
                NdisMoveMemory((PVOID)pkt_buf, (PVOID)pucBufAddress, uiBufLen);
                pkt_buf += uiBufLen;
                pkt_len += (DWORD)uiBufLen;

                if ((pAdapter->fTxDMAMode) &&
                    (pkt_len > (DWORD)TX_DMA_THRESHOLD))
                {
                    // DMA Mode
                    PhysAddr = (DWORD*)pAdapter->TxTempPktBuf.PAddr.LowPart;

                    dwTxCmdA=(((CACHE_LINE_BYTES/16UL)<<24) |
                              ((((DWORD)PhysAddr)&(CACHE_LINE_BYTES-1UL))<<16) | //Data Start Offset adjustment
                              ((DWORD)pkt_len));
                    // Set Last/First Segment flag in TxCmdA.
                    if (pBuffer == pBufferLast)    //last non-zero buffer
                    {
                        dwTxCmdA |= TX_CMD_A_INT_LAST_SEG_;

                        // Check if it is also the first buffer, i.e. single buffer packet.
                        if (fFistNonZeroBuf == TRUE)   //first segment of the first non-zero buffer.
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                    }
                    else
                    {
                        // Check if it is the first segment of the first buffer.
                        if (fFistNonZeroBuf == TRUE)
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                    }

                    dwCount = CACHELINE_ALIGN((DWORD)(pkt_len) + (((DWORD)PhysAddr)&(CACHE_LINE_BYTES-1UL))) >> 2;
                    dmaXfer.pdwBuf = (DWORD*)(((DWORD)PhysAddr)&(~(CACHE_LINE_BYTES-1UL)));
                    dmaXfer.fClBurst = (BOOLEAN)TRUE;
                    dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
                    dmaXfer.dwDwCnt = dwCount;
                    dmaXfer.fMemWr = (BOOLEAN)FALSE;
                    dmaXfer.fMemAddrInc = (BOOLEAN)TRUE;
                    dmaXfer.fLanAddrInc = (BOOLEAN)FALSE;
                    dmaXfer.dwDmaCh = DMA_TX_CH;
                    dmaXfer.dwLanReg = pAdapter->ulIoBaseAddress;  // Phys address of 9118 I/O base

                    SMSC_TRACE0(DBG_TX,"Initiate DMA Tx\r\n");
                    SMSC_TRACE2(DBG_TX,"TxCmdA = 0x%x, TxCmdB = 0x%x\r\n", dwTxCmdA, dwTxCmdB);
                    // Start xfer...
                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdA);
                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdB);

                    // Kick off DMA
                    if (!DmaStartXfer(&dmaXfer)) {
                        SMSC_TRACE0(DBG_TX,"Tx DMA Failed.\r\n");
                    }

                    // Wait for DMA to complete.
                    DmaComplete(&dmaXfer, DMA_TX_CH);
                }
                else
                {
                    // PIO
                    const DWORD dwPktPtr = (DWORD)(pAdapter->TxTempPktBuf.pVAddr);
                    if (pBuffer == pBufferLast)    //last non-zero buffer
                    {
                        SMSC_TRACE1(DBG_TX,"At pBufferLast (pBuffer = 0x%x)\r\n", pBuffer);
                        dwTxCmdA=( ((dwPktPtr)<<16) |
                               TX_CMD_A_INT_LAST_SEG_ | ((DWORD)pkt_len));

                        // Check if it is also the first buffer, i.e. single buffer packet.
                        if (fFistNonZeroBuf)    //first non-zero buffer
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                    }
                    else
                    {
                        dwTxCmdA=(((dwPktPtr)<<16) | (pkt_len));

                        // Check if it is the first buffer.
                        if (fFistNonZeroBuf)    //first non-zero buffer
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                        else
                        {
                            // Middle buffers should be 
                            // greater than or equal to 4bytes in length
                            if (pkt_len < 4UL)
                            {
                                // Do not transmit this buffer.
                                // This should be merged to next buffer
                                if (pBuffer) 
                                {
                                    NdisGetNextBuffer (pBuffer, &pBuffer);
                                }
                                continue;
                            }
                        }
                    }

                    SMSC_TRACE0(DBG_TX,"Initiate PIO Tx\r\n");

                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdA);
                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdB);
                    Lan_WriteTxFifo((PLAN9118_DATA const)&pAdapter->lan9118_data,
				    pAdapter->lan9118_data.dwLanBase,
                                (DWORD *)((dwPktPtr)&~0x03UL),
                                ((DWORD)pkt_len+3UL+(dwPktPtr&0x03UL))>>2);
                }
                pkt_len = 0UL;
                pkt_buf = (PUCHAR)(pAdapter->TxTempPktBuf.pVAddr);
                if (pBuffer) 
                {
                    NdisGetNextBuffer (pBuffer, &pBuffer);
                }
                continue;
            }

            if ((pAdapter->fTxDMAMode) && 
                (uiBufLen>TX_DMA_THRESHOLD) && 
                ((uiBufCount*MAX_NUM_SEGMENTS) <= 86U))
            {
                // DMA Mode

                NdisMStartBufferPhysicalMapping(pAdapter->hMiniportAdapterHandle,
                                                pBuffer,
                                                0UL,
                                                (BOOLEAN)TRUE,
                                                SegmentArray,
                                                &ArraySize);

                SMSC_ASSERT(ArraySize <= MAX_NUM_SEGMENTS);

                // Aug/1/2008 WH
                BufferCacheFlush((PUCHAR)pucBufAddress, uiBufLen);

                for (i=0U; i<ArraySize; i++)
                {
                    SMSC_ASSERT(SegmentArray[i].Length);

                    // Calculate dwTxCmdA
                    SMSC_ASSERT(NdisGetPhysicalAddressHigh(SegmentArray[i].PhysicalAddress) == 0L);
                    PhysAddrLo = NdisGetPhysicalAddressLow(SegmentArray[i].PhysicalAddress);
                    PhysAddr = (DWORD*)PhysAddrLo;

                    {
                        dwTxCmdA=( 
                                   ((CACHE_LINE_BYTES/16UL)<<24) |
                                   ((((DWORD)PhysAddr)&(CACHE_LINE_BYTES-1UL))<<16) | //Data Start Offset adjustment
                                   ((DWORD)(SegmentArray[i].Length)) );
                    }

                    // Set Last/First Segment flag in TxCmdA.
                    if (pBuffer == pBufferLast)    //last non-zero buffer
                    {
                        // Check if it is the last segment for the last buffer.
                        if (i == (ArraySize-1U)) {
                            dwTxCmdA |= TX_CMD_A_INT_LAST_SEG_;
                        }

                        // Check if it is also the first buffer, i.e. single buffer packet.
                        if ((fFistNonZeroBuf == TRUE) && (i == 0U))    //first segment of the first non-zero buffer.
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                    }
                    else
                    {
                        // Check if it is the first segment of the first buffer.
                        if ((fFistNonZeroBuf == TRUE) && (i == 0U))
                        {
                            fFistNonZeroBuf = FALSE;
                            dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                        }
                    }

                    {
                        dwCount = CACHELINE_ALIGN((DWORD)(SegmentArray[i].Length) + (((DWORD)PhysAddr)&(CACHE_LINE_BYTES-1UL))) >> 2;
                        dmaXfer.pdwBuf = (DWORD*)(((DWORD)PhysAddr)&(~(CACHE_LINE_BYTES-1UL)));
                        dmaXfer.fClBurst = (BOOLEAN)TRUE;
                    }
                    dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
                    dmaXfer.dwDwCnt = dwCount;
                    dmaXfer.fMemWr = (BOOLEAN)FALSE;
                    dmaXfer.fMemAddrInc = (BOOLEAN)TRUE;
                    dmaXfer.fLanAddrInc = (BOOLEAN)FALSE;
                    dmaXfer.dwDmaCh = DMA_TX_CH;
                    dmaXfer.dwLanReg = pAdapter->ulIoBaseAddress;  // Phys address of 9118 I/O base

                    SMSC_TRACE0(DBG_TX,"Initiate DMA Tx\r\n");
                    SMSC_TRACE2(DBG_TX,"TxCmdA = 0x%x, TxCmdB = 0x%x\r\n", dwTxCmdA, dwTxCmdB);
                    // Start xfer...
                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdA);
                    AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdB);

                    // Kick off DMA
                    if (!DmaStartXfer(&dmaXfer)) {
                        SMSC_TRACE0(DBG_TX,"Tx DMA Failed.\r\n");
                    }

                    // Wait for DMA to complete.
                    DmaComplete(&dmaXfer, DMA_TX_CH);
                }

                NdisMCompleteBufferPhysicalMapping(pAdapter->hMiniportAdapterHandle,
                                                   pBuffer,
                                                   0UL);
            }
            else
            {
                // PIO Mode

                // Calculate dwTxCmdA
                if (pBuffer == pBufferLast)    //last non-zero buffer
                {
                    SMSC_TRACE1(DBG_TX,"At pBufferLast (pBuffer = 0x%x)\r\n", pBuffer);
                    dwTxCmdA=( ((((DWORD)pucBufAddress)&0x03UL)<<16) | //DWORD alignment adjustment
                           TX_CMD_A_INT_LAST_SEG_ | 
                           ((DWORD)uiBufLen));

                    // Check if it is also the first buffer, i.e. single buffer packet.
                    if (fFistNonZeroBuf)    //first non-zero buffer
                    {
                        fFistNonZeroBuf = FALSE;
                        dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                    }
                }
                else
                {
                    dwTxCmdA=( ((((DWORD)pucBufAddress)&0x03UL)<<16) | //DWORD alignment adjustment
                           ((DWORD)uiBufLen) );

                    // Check if it is the first buffer.
                    if (fFistNonZeroBuf)    //first non-zero buffer
                    {
                        fFistNonZeroBuf = FALSE;
                        dwTxCmdA |= TX_CMD_A_INT_FIRST_SEG_;
                    }
                    else
                    {
                        // Middle buffers should be 
                        // greater than or equal to 4bytes in length
                        if (uiBufLen < 4U)
                        {
                            NdisMoveMemory((PVOID)pkt_buf, (PVOID)pucBufAddress, uiBufLen);
                            pkt_buf += uiBufLen;
                            pkt_len += (DWORD)uiBufLen;
                            // Do not transmit this buffer.
                            // This should be merged to next buffer
                            if (pBuffer) 
                            {
                                NdisGetNextBuffer (pBuffer, &pBuffer);
                            }
                            continue;
                        }
                    }
                }

                SMSC_TRACE0(DBG_TX,"Initiate PIO Tx\r\n");
                AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdA);
                AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdB);
                Lan_WriteTxFifo((PLAN9118_DATA const)&pAdapter->lan9118_data,
				pAdapter->lan9118_data.dwLanBase,
                            (DWORD *)(((DWORD)pucBufAddress)&~0x03UL),
                            ( (DWORD)uiBufLen+3UL+(((DWORD)pucBufAddress)&0x03UL) )>>2);
            }

            if (pBuffer) {
                NdisGetNextBuffer (pBuffer, &pBuffer);
            }
            else {
                SMSC_TRACE0(DBG_WARNING, "Invalid pBuffer\r\n");
            }

        } while (pBuffer);
    }
    else
    {
        // Packet is too fragmented. 
        // Copy the buffers to a driver buffer and xfer using the driver buffer.
        DWORD  pkt_len = 0UL;
        PUCHAR pkt_buf = (PUCHAR)(pAdapter->TxPktBuffer);

        do
        {
            NdisQueryBuffer (pBuffer, (PVOID*)&pucBufAddress, &uiBufLen);

            while (pBuffer && (uiBufLen == 0U))
            {
                NdisGetNextBuffer (pBuffer, &pBuffer);
                NdisQueryBuffer (pBuffer, (PVOID*)&pucBufAddress, &uiBufLen);
            }

            SMSC_TRACE2(DBG_TX,"(Highly Fragmented)pucBufAddress %x = %d byte.\r\n", pucBufAddress, uiBufLen);

            NdisMoveMemory((PVOID)pkt_buf, (PVOID)pucBufAddress, uiBufLen);
            pkt_buf += uiBufLen;
            pkt_len += (DWORD)uiBufLen;

            if (pBuffer) {
                NdisGetNextBuffer(pBuffer, &pBuffer);
            }
        } while (pBuffer);

        if (pkt_len != (DWORD)uiPacketLength)
            RETAILMSG(1, (TEXT("pkt_len = %d, uiPacketLength = %d\r\n"), pkt_len, uiPacketLength));
        
        // Send it using PIO Mode

        // Calculate dwTxCmdA
        dwTxCmdA=( ((((DWORD)(pAdapter->TxPktBuffer))&0x03UL)<<16) | //DWORD alignment adjustment
                   TX_CMD_A_INT_LAST_SEG_ | TX_CMD_A_INT_FIRST_SEG_ |
                   (pkt_len) );

        SMSC_TRACE1(DBG_TX,"(Highly Fragmented)dwTxCmdA = 0x%x\r\n", dwTxCmdA);

        AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdA);
        AdapterSetCSR(TX_DATA_FIFO_PORT, dwTxCmdB);
        Lan_WriteTxFifo((PLAN9118_DATA const)&pAdapter->lan9118_data,
			   pAdapter->lan9118_data.dwLanBase,
                        (DWORD *)(((DWORD)(pAdapter->TxPktBuffer))&~0x03UL),
                        (pkt_len+3UL+(((DWORD)(pAdapter->TxPktBuffer))&0x03UL) )>>2);
    }    

    while (Lan_GetTxStatusCount(&(pAdapter->lan9118_data)))
    {
        TxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_STATUS_FIFO_PORT);
        if (!(TxStatus&TX_STS_ES))
        {
            // Update the stats
            (pAdapter->ulFramesXmitGood)++;
            SMSC_TRACE0(DBG_TX,"Tx Success\r\n");
        }
        else
        {
            // Update the stats
            (pAdapter->ulFramesXmitBad)++;
            SMSC_TRACE1(DBG_TX,"Tx Error! TxStatus = 0x%x\r\n", TxStatus);
            return FALSE;
        }
    }

    SMSC_TRACE0(DBG_TX,"-Smsc9118XmitOnePacket\r\n");
    return TRUE;
}


#if 1
/*----------------------------------------------------------------------------
    Smsc9118TransmitPackets
        Transmit packets in Tx queue, which calls Smsc9118XmitOnePacket.
*/
static __inline VOID
Smsc9118TransmitPackets(IN  PSMSC9118_ADAPTER pAdapter,
                       IN  PPNDIS_PACKET ppPacket,
                       IN  UINT uiNumOfPkts)
{
    DWORD   dw, TxDataFreeSpace, TxSpaceNeeded, TxStatus;
    UINT    packet_len;
    UINT   BufferCount, dwFreeRunCounter;
    BOOL    status = TRUE;
    DWORD   dwDeferedPktCnt;
    PNDIS_PACKET pPacket;

    SMSC_TRACE0(DBG_TX,"+Smsc9118TransmitPackets\r\n");

    dwDeferedPktCnt = QUEUE_COUNT(&pAdapter->TxDeferedPkt);

    if (dwDeferedPktCnt)
    {
        SMSC_TRACE1(DBG_TX, "%d TxDeferedPkts\r\n", QUEUE_COUNT(&pAdapter->TxDeferedPkt));
        for(dw=0UL; dw<(DWORD)uiNumOfPkts; dw++)
        {
            pPacket = ppPacket[dw];
            SMSC_ASSERT(pPacket);

            NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
            dwFreeRunCounter = AdapterGetCSR(FREE_RUN);
            EnqueuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket, pPacket, dwFreeRunCounter);
            (pAdapter->TxDeferedPkt.Count)++;
#ifdef  TRACE_BUFFER
            dwTxPend++;
#endif
        }

        while (dwDeferedPktCnt--)
        {
            pPacket = pAdapter->TxDeferedPkt.FirstPacket;
            SMSC_ASSERT(pPacket);

            // Read the actual data.
            TxDataFreeSpace = (GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_FIFO_INF)) & TX_FIFO_INF_TDFREE_;
            NdisQueryPacket(pPacket, NULL, &BufferCount, NULL, &packet_len);
            if ((pAdapter->fTxDMAMode) && (BufferCount <= 30UL))
            {
                // worst case for burst mode DMA
                TxSpaceNeeded = ( ( (DWORD)packet_len + 
                                    (BufferCount*((DWORD)MAX_NUM_SEGMENTS*TX_BURST_DMA_END_ALIGNMENT)) +  CACHE_LINE_BYTES) & ~(CACHE_LINE_BYTES-1UL));

            }
            else
            {
                // if (BufferCount > 30), TxSpaceNeeded will be larger than
                // max available TX FIFO.
                // If then, Smsc9118XmitOnePacket() will handle the packet
                //  as PIO mode.
                // We can check as PIO mode here
                TxSpaceNeeded = (DWORD)((packet_len+3U)&(~0x03U)) + (BufferCount<<4);
            }

            if (TxSpaceNeeded < TxDataFreeSpace)
            {
                status = Smsc9118XmitOnePacket(pAdapter, pPacket);
                DequeuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket);
                if (status == TRUE)
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_SUCCESS);
                    SMSC_TRACE0(DBG_TX,"One packet xmitted successfully.\r\n");
                }
                else
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_FAILURE);
                    SMSC_TRACE0(DBG_TX,"One packet failed xmitting.\r\n");
                }
#ifdef  TRACE_BUFFER
                dwTxSent++;
                dwTxPend--;
#endif
                (pAdapter->TxDeferedPkt.Count)--;

            }
            else
            {
                while (Lan_GetTxStatusCount(&(pAdapter->lan9118_data)))
                {
                    TxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_STATUS_FIFO_PORT);
                    if (!(TxStatus&TX_STS_ES))
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitGood)++;
                        SMSC_TRACE0(DBG_TX,"Tx Success\r\n");
                    }
                    else
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitBad)++;
                        SMSC_TRACE1(DBG_TX,"Tx Failure! TxStatus = 0x%x\r\n", TxStatus);
                    }
                }
                break;  // break from while
            }
        }
        // Set the TDFA interrupt for size of a minimum packet size.
        pAdapter->TDFALevel = 0x20UL;
        if (NdisMSynchronizeWithInterrupt(&(pAdapter->Interrupt), SyncSetTDFALevel, (PVOID)pAdapter) != TRUE) 
        {
            SMSC_WARNING(TEXT("Failed! at NdisMSynchronizeWithInterrupt()\r\n"));
        }
    }
    else 
    {
        SMSC_TRACE0(DBG_TX, "No TxDeferedPkts\r\n");

        for(dw=0UL; dw<(DWORD)uiNumOfPkts; dw++)
        {
            pPacket = ppPacket[dw];
            SMSC_ASSERT(pPacket);
            // Read the actual data.
            TxDataFreeSpace = (GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_FIFO_INF)) & TX_FIFO_INF_TDFREE_;
            NdisQueryPacket(pPacket, NULL, &BufferCount, NULL, &packet_len);
            if ((pAdapter->fTxDMAMode) && (BufferCount < 30UL))
            {
                // worst case for burst mode DMA
                TxSpaceNeeded = ( ( (DWORD)packet_len + 
                                    (BufferCount*((DWORD)MAX_NUM_SEGMENTS*TX_BURST_DMA_END_ALIGNMENT)) +  CACHE_LINE_BYTES) & ~(CACHE_LINE_BYTES-1UL));
            }
            else
            {
                // if (BufferCount > 30), TxSpaceNeeded will be larger than
                // max available TX FIFO.
                // If then, Smsc9118XmitOnePacket() will handle the packet
                //  as PIO mode.
                // We can check as PIO mode here
                TxSpaceNeeded = (DWORD)((packet_len+3U)&(~0x03U)) + (BufferCount<<4);
            }

            if (TxSpaceNeeded < TxDataFreeSpace)
            {
                status = Smsc9118XmitOnePacket(pAdapter, pPacket);
                if (status == FALSE)
                {
                    NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_FAILURE);
                    SMSC_TRACE0(DBG_TX,"One packet failed xmitting.\r\n");
                }
                else
                {
                    NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);
                    SMSC_TRACE0(DBG_TX,"One packet xmitted successfully.\r\n");
#ifdef  TRACE_BUFFER
                    dwTxSent++;
#endif
                    
                }
            }
            else
            {
                // No room on FIFO. Save now and send them later
                SMSC_TRACE2(DBG_TX, "No room on TX FIFO. SpaceNeeded = %d, FreeSpace = %d\r\n", TxSpaceNeeded, TxDataFreeSpace);

                NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
                dwFreeRunCounter = AdapterGetCSR(FREE_RUN);
                EnqueuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket, pPacket, dwFreeRunCounter);
                (pAdapter->TxDeferedPkt.Count)++;
#ifdef  TRACE_BUFFER
                dwTxPend++;
#endif
                while (Lan_GetTxStatusCount(&(pAdapter->lan9118_data)))
                {
                    TxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_STATUS_FIFO_PORT);
                    if (!(TxStatus&TX_STS_ES))
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitGood)++;
                        SMSC_TRACE0(DBG_TX,"Tx Success\r\n");
                    }
                    else
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitBad)++;
                        SMSC_TRACE1(DBG_TX,"Tx Failure! TxStatus = 0x%x\r\n", TxStatus);
                    }
                }
            }
            pAdapter->TDFALevel = 0x20UL;
            if (NdisMSynchronizeWithInterrupt(&(pAdapter->Interrupt), SyncSetTDFALevel, (PVOID)pAdapter) != TRUE) 
            {
                SMSC_WARNING(TEXT("Failed! at NdisMSynchronizeWithInterrupt()\r\n"));
            }
        }
    }

    // Make Lint Happy
    ppPacket = ppPacket;
    uiNumOfPkts = uiNumOfPkts;

    SMSC_TRACE0(DBG_TX,"-Smsc9118TransmitPackets\r\n");
}
#else
/*----------------------------------------------------------------------------
    Smsc9118TransmitPackets
        Transmit packets in Tx queue, which calls Smsc9118XmitOnePacket.
*/
static __inline VOID
Smsc9118TransmitPackets(IN  PSMSC9118_ADAPTER pAdapter,
                       IN  PPNDIS_PACKET ppPacket,
                       IN  UINT uiNumOfPkts)
{
    DWORD   dw, TxDataFreeSpace, TxSpaceNeeded, TxStatus;
    UINT    packet_len;
    DWORD   BufferCount, dwFreeRunCounter;
    BOOL    status = TRUE;
    DWORD   dwDeferedPktCnt;
    PNDIS_PACKET pPacket;

    SMSC_TRACE0(DBG_TX,"+Smsc9118TransmitPackets\r\n");

    dwDeferedPktCnt = QUEUE_COUNT(&pAdapter->TxDeferedPkt);

    if (dwDeferedPktCnt)
    {
        SMSC_TRACE1(DBG_TX, "%d TxDeferedPkts\r\n", QUEUE_COUNT(&pAdapter->TxDeferedPkt));
        for(dw=0UL; dw<(DWORD)uiNumOfPkts; dw++)
        {
            pPacket = ppPacket[dw];
            SMSC_ASSERT(pPacket);

            NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
            dwFreeRunCounter = AdapterGetCSR(FREE_RUN);
            EnqueuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket, pPacket, dwFreeRunCounter);
            (pAdapter->TxDeferedPkt.Count)++;
#ifdef  TRACE_BUFFER
            dwTxPend++;
#endif
        }

        while (dwDeferedPktCnt--)
        {
            pPacket = pAdapter->TxDeferedPkt.FirstPacket;
            SMSC_ASSERT(pPacket);

            // Read the actual data.
            TxDataFreeSpace = (GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_FIFO_INF)) & TX_FIFO_INF_TDFREE_;
            NdisQueryPacket(pPacket, NULL, &BufferCount, NULL, &packet_len);
            if ((pAdapter->fTxDMAMode) && (BufferCount <= 30UL))
            {
                // worst case for burst mode DMA
                TxSpaceNeeded = ( ( (DWORD)packet_len + 
                                    (BufferCount*((DWORD)MAX_NUM_SEGMENTS*TX_BURST_DMA_END_ALIGNMENT)) +  CACHE_LINE_BYTES) & ~(CACHE_LINE_BYTES-1UL));

            }
            else
            {
                // if (BufferCount > 30), TxSpaceNeeded will be larger than
                // max available TX FIFO.
                // If then, Smsc9118XmitOnePacket() will handle the packet
                //  as PIO mode.
                // We can check as PIO mode here
                TxSpaceNeeded = (DWORD)((packet_len+3U)&(~0x03U)) + (BufferCount<<4);
            }

            if (TxSpaceNeeded < TxDataFreeSpace)
            {
                status = Smsc9118XmitOnePacket(pAdapter, pPacket);
                DequeuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket);
                if (status == TRUE)
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_SUCCESS);
                    SMSC_TRACE0(DBG_TX,"One packet xmitted successfully.\r\n");
                }
                else
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_FAILURE);
                    SMSC_TRACE0(DBG_TX,"One packet failed xmitting.\r\n");
                }
#ifdef  TRACE_BUFFER
                dwTxSent++;
                dwTxPend--;
#endif
                (pAdapter->TxDeferedPkt.Count)--;
            }
            else
            {
                while (Lan_GetTxStatusCount(&(pAdapter->lan9118_data)))
                {
                    TxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_STATUS_FIFO_PORT);
                    if (!(TxStatus&TX_STS_ES))
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitGood)++;
                        SMSC_TRACE0(DBG_TX,"Tx Success\r\n");
                    }
                    else
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitBad)++;
                        SMSC_TRACE1(DBG_TX,"Tx Failure! TxStatus = 0x%x\r\n", TxStatus);
                    }
                }
                break;  // break from while
            }
        }
        // Set the TDFA interrupt for size of a minimum packet size.
        pAdapter->TDFALevel = 0x20UL;
        if (NdisMSynchronizeWithInterrupt(&(pAdapter->Interrupt), SyncSetTDFALevel, (PVOID)pAdapter) != TRUE) 
        {
            SMSC_WARNING(TEXT("Failed! at NdisMSynchronizeWithInterrupt()\r\n"));
        }
    }
    else 
    {
        SMSC_TRACE0(DBG_TX, "No TxDeferedPkts\r\n");

        for(dw=0UL; dw<(DWORD)uiNumOfPkts; dw++)
        {
            pPacket = ppPacket[dw];
            SMSC_ASSERT(pPacket);
            // Read the actual data.
            TxDataFreeSpace = (GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_FIFO_INF)) & TX_FIFO_INF_TDFREE_;
            NdisQueryPacket(pPacket, NULL, &BufferCount, NULL, &packet_len);
            if ((pAdapter->fTxDMAMode) && (BufferCount < 30UL))
            {
                // worst case for burst mode DMA
                TxSpaceNeeded = ( ( (DWORD)packet_len + 
                                    (BufferCount*((DWORD)MAX_NUM_SEGMENTS*TX_BURST_DMA_END_ALIGNMENT)) +  CACHE_LINE_BYTES) & ~(CACHE_LINE_BYTES-1UL));
            }
            else
            {
                // if (BufferCount > 30), TxSpaceNeeded will be larger than
                // max available TX FIFO.
                // If then, Smsc9118XmitOnePacket() will handle the packet
                //  as PIO mode.
                // We can check as PIO mode here
                TxSpaceNeeded = (DWORD)((packet_len+3U)&(~0x03U)) + (BufferCount<<4);
            }

            if (TxSpaceNeeded < TxDataFreeSpace)
            {
                status = Smsc9118XmitOnePacket(pAdapter, pPacket);
                if (status == FALSE)
                {
                    NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_FAILURE);
                    SMSC_TRACE0(DBG_TX,"One packet failed xmitting.\r\n");
                }
                else
                {
                    NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);
                    SMSC_TRACE0(DBG_TX,"One packet xmitted successfully.\r\n");
                }
#ifdef  TRACE_BUFFER
                dwTxSent++;
#endif
            }
            else
            {
                // No room on FIFO. Save now and send them later
                SMSC_TRACE2(DBG_TX, "No room on TX FIFO. SpaceNeeded = %d, FreeSpace = %d\r\n", TxSpaceNeeded, TxDataFreeSpace);

                NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
                dwFreeRunCounter = AdapterGetCSR(FREE_RUN);
                EnqueuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket, pPacket, dwFreeRunCounter);
                (pAdapter->TxDeferedPkt.Count)++;
#ifdef  TRACE_BUFFER
                dwTxPend++;
#endif
                while (Lan_GetTxStatusCount(&(pAdapter->lan9118_data)))
                {
                    TxStatus = GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_STATUS_FIFO_PORT);
                    if (!(TxStatus&TX_STS_ES))
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitGood)++;
                        SMSC_TRACE0(DBG_TX,"Tx Success\r\n");
                    }
                    else
                    {
                        // Update the stats
                        (pAdapter->ulFramesXmitBad)++;
                        SMSC_TRACE1(DBG_TX,"Tx Failure! TxStatus = 0x%x\r\n", TxStatus);
                    }
                }
            }
            pAdapter->TDFALevel = 0x20UL;
            if (NdisMSynchronizeWithInterrupt(&(pAdapter->Interrupt), SyncSetTDFALevel, (PVOID)pAdapter) != TRUE) 
            {
                SMSC_WARNING(TEXT("Failed! at NdisMSynchronizeWithInterrupt()\r\n"));
            }
        }
    }

    // Make Lint Happy
    ppPacket = ppPacket;
    uiNumOfPkts = uiNumOfPkts;

    SMSC_TRACE0(DBG_TX,"-Smsc9118TransmitPackets\r\n");
}
#endif


/*----------------------------------------------------------------------------
    SyncSetTDFALevel
*/
BOOLEAN SyncSetTDFALevel(const void *const SynchronizeContext)
{
    DWORD dw;
    const SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(SynchronizeContext);

    // Set the TDFA level.
    dw = GetRegDW(pAdapter->lan9118_data.dwLanBase, FIFO_INT);
    dw &= (~FIFO_INT_TX_AVAIL_LEVEL_);
    dw |= (pAdapter->TDFALevel) << 24;
    SetRegDW(pAdapter->lan9118_data.dwLanBase, FIFO_INT, dw);
    return (BOOLEAN)TRUE;
}


/*----------------------------------------------------------------------------
    HandlerTxISR
*/
__inline VOID HandlerTxISR(PSMSC9118_ADAPTER pAdapter)
{
    SMSC_TRACE0(DBG_TX,"+HandlerTxISR\r\n");
    SMSC_ASSERT(pAdapter);

    if (pAdapter)
    {
        // Disable the TDFA iterrupt by setting 0xFF to TDFA
        pAdapter->TDFALevel = 0xFFUL;
        if (SyncSetTDFALevel((PVOID)pAdapter) != TRUE)
        {
            SMSC_WARNING(TEXT("SyncSetTDFALevel() returns FALSE\r\n"));
        }

        pAdapter->TxDPCNeeded = (BOOLEAN)TRUE;
    }

    SMSC_TRACE0(DBG_TX,"-HandlerTxISR\r\n");
}


/*----------------------------------------------------------------------------
    HandlerTxDPC
        DPC handler for Tx ISR, which processes the time consuming tasks out 
        of Tx ISR.
*/

DPC_STATUS HandlerTxDPC(PSMSC9118_ADAPTER pAdapter)
{
    PNDIS_PACKET    pPacket;
    volatile DWORD  TxDataFreeSpace, TxSpaceNeeded;
    UINT            packet_len;
    UINT            BufferCount;
    BOOL            status = TRUE;
    DPC_STATUS      RetStatus = DPC_STATUS_DONE;
    BOOLEAN         bRet;
    DWORD           dwDeferedPktCnt;

    SMSC_TRACE0(DBG_TX,"+HandlerTxDPC\r\n");
    SMSC_ASSERT(pAdapter);

    if (pAdapter) 
    {
        dwDeferedPktCnt = QUEUE_COUNT(&(pAdapter->TxDeferedPkt));

        // limit number of packet to handle in this loop
        if (dwDeferedPktCnt > BALANCE_TX_DEFER_PACKET_LIMIT)
            dwDeferedPktCnt = BALANCE_TX_DEFER_PACKET_LIMIT;

        while (dwDeferedPktCnt--)
        {
            pPacket = pAdapter->TxDeferedPkt.FirstPacket;
            // Read the actual data.
            TxDataFreeSpace = (GetRegDW(pAdapter->lan9118_data.dwLanBase, TX_FIFO_INF)) & TX_FIFO_INF_TDFREE_;
            NdisQueryPacket(pPacket, NULL, &BufferCount, NULL, &packet_len);
            if ((pAdapter->fTxDMAMode) && (BufferCount < 30UL))
            {
                // worst case for burst mode DMA
                TxSpaceNeeded = ( ( (DWORD)packet_len + 
                                    (BufferCount*((DWORD)MAX_NUM_SEGMENTS*TX_BURST_DMA_END_ALIGNMENT)) +  CACHE_LINE_BYTES) & ~(CACHE_LINE_BYTES-1UL));
            }
            else
            {
                // if (BufferCount > 30), TxSpaceNeeded will be larger than
                // max available TX FIFO.
                // If then, Smsc9118XmitOnePacket() will handle the packet
                //  as PIO mode.
                // We can check as PIO mode here
                TxSpaceNeeded = (DWORD)((packet_len+3U)&(~0x03U)) + (BufferCount<<4);
            }
            if (TxSpaceNeeded < TxDataFreeSpace)
            {
                status = Smsc9118XmitOnePacket(pAdapter, pPacket);
                DequeuePacket(pAdapter->TxDeferedPkt.FirstPacket, pAdapter->TxDeferedPkt.LastPacket);
                if (status == FALSE)
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_FAILURE);
                    SMSC_TRACE0(DBG_TX,"One packet failed xmitting.\r\n");
                }
                else
                {
                    NdisMSendComplete(pAdapter->hMiniportAdapterHandle, 
                            pPacket, NDIS_STATUS_SUCCESS);
                    SMSC_TRACE0(DBG_TX,"One packet xmitted successfully.\r\n");
                }
                (pAdapter->TxDeferedPkt.Count)--;
#ifdef  TRACE_BUFFER
                dwTxSent++;
                dwTxPend--;
#endif
            }
            else
            {
                RetStatus = DPC_STATUS_PENDING;
                break;  // exit from while(pPacket...)
            }
        } // end of while

        // if there is defered Tx packet, set TDFALevel to 0x20
        if (QUEUE_COUNT(&(pAdapter->TxDeferedPkt)))
        {
            pAdapter->TDFALevel = 0x20UL;
            bRet = NdisMSynchronizeWithInterrupt(&(pAdapter->Interrupt),
                                                SyncSetTDFALevel,
                                                (PVOID)pAdapter);
            // SyncSetTDFALevel() always returns TRUE
            // Make Lint Happy
            bRet = bRet;
        }
    }

    SMSC_TRACE0(DBG_TX,"-HandlerTxDPC\r\n");
    return RetStatus;
}


DPC_STATUS HandlerPhyDPC(PSMSC9118_ADAPTER pAdapter)
{
    if (pAdapter)
    {
        switch(LinkIndicate(pAdapter))
        {
            case LINK_NO_LINK:
                SMSC_TRACE0(DBG_PHY,"LINK_NO_LINK\n");
                break;
            case LINK_10MPS_HALF:
                SMSC_TRACE0(DBG_PHY,"LINK_10MPS_HALF\n");
                break;
            case LINK_10MPS_FULL:
                SMSC_TRACE0(DBG_PHY,"LINK_10MPS_FULL\n");
                break;
            case LINK_100MPS_HALF:
                SMSC_TRACE0(DBG_PHY,"LINK_100MPS_HALF\n");
                break;
            case LINK_100MPS_FULL:
                SMSC_TRACE0(DBG_PHY,"LINK_100MPS_FULL\n");
                break;
            default:
                SMSC_WARNING1("Unknown Link Mode, dwLinkMode=0x%08lX\n", pAdapter->lan9118_data.dwLinkMode);
                break;
        }

        pAdapter->PhyDPCNeeded = (BOOLEAN)FALSE;
    }

    return DPC_STATUS_DONE;
}

/*----------------------------------------------------------------------------
    Smsc9118SendPackets
        NDIS miniport function
*/
VOID 
Smsc9118SendPackets (IN NDIS_HANDLE hMiniportAdapterContext,
                    IN PPNDIS_PACKET ppPacketArray,
                    IN UINT uiNumberOfPackets)
{
    PSMSC9118_ADAPTER const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    SMSC_TRACE0(DBG_TX,"+Smsc9118SendPackets\r\n");
    SMSC_ASSERT(ppPacketArray);

    SMSC_TRACE1(DBG_TX,"uiNumberOfPackets = %d\r\n", uiNumberOfPackets);

    if (pAdapter == (PSMSC9118_ADAPTER)NULL)
    {
        SMSC_TRACE0(DBG_TX,"Invalid Adapter pointer\r\n");
        return;
    }

#ifdef  TRACE_BUFFER
    dwTxReported += (DWORD)uiNumberOfPackets;
#endif

    Smsc9118TransmitPackets(pAdapter, ppPacketArray, uiNumberOfPackets);

    SMSC_TRACE0(DBG_TX,"-Smsc9118SendPackets\r\n");

    return;
}

VOID CheckPhyStatus(PSMSC9118_ADAPTER pAdapter)
{
    static volatile WORD    wOldPhyBSR = (WORD)0x0;
    WORD                    wPhyBSR;
    DWORD                   dwTemp, dwRegVal;

    SMSC_TRACE0(DBG_PHY,"+CheckPhyStatus\r\n");

    if (AdapterReadPhy(PHY_BSR, &wPhyBSR) == FALSE)
    {
        SMSC_WARNING0("Timeout waiting flow busy bit.\r\n");
        return;
    }

    if ((pAdapter->fLinkUp == FALSE) &&
        ((wPhyBSR & PHY_BSR_LINK_STATUS_) == PHY_BSR_LINK_STATUS_))
    {
        // Link Up
        if(wPhyBSR & PHY_BSR_AUTO_NEG_COMP_) 
        {
            SMSC_TRACE0(DBG_PHY,"Auto Negotiation Complete.\r\n");
            dwTemp = Lan_GetLinkMode(&pAdapter->lan9118_data);
            if (dwTemp != LINK_NO_LINK)
            {
                dwRegVal = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR);
                dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);
                switch (dwTemp) 
                {
                    case LINK_10MPS_HALF:
                    case LINK_100MPS_HALF:
                        dwRegVal |= MAC_CR_RCVOWN_;
                        break;

                    case LINK_10MPS_FULL:
                    case LINK_100MPS_FULL:
                        dwRegVal |= MAC_CR_FDPX_;
                        break;
                    default:
                        break;
                }
                Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR, dwRegVal);
                pAdapter->PhyDPCNeeded = (BOOLEAN)TRUE;
#ifdef USE_LED1_WORK_AROUND
                // workaround for 118/117/116/115 family
                if ((pAdapter->lan9118_data.dwIdRev & 0xFFF0FFFFUL) == 0x01100002UL)
                {
                    if(pAdapter->lan9118_data.bPhyAddress == (BYTE)1)   // internal PHY only
                    {
                        DWORD   dwOldGpioCfg=GetRegDW(pAdapter->lan9118_data.dwLanBase, GPIO_CFG);

                        // Link is now UP (in some link state fashion)
                        if (pAdapter->lan9118_data.dwOriginalGpioCfg & GPIO_CFG_LED1_EN_)
                        {
                            /* If we only restore the LED1 state when we
                             * expected to, other processes can muck with
                             * the GPIO_CFG register the rest of the time.
                             * You could also think of this as a one-shot.
                             */
                            if (pAdapter->lan9118_data.LED1NotYetRestored == TRUE)
                            {
                                SMSC_TRACE2(DBG_PHY,"LED1 on GPIO_CFG:0x%08x <-- 0x%08x\n\r",
                                    pAdapter->lan9118_data.dwOriginalGpioCfg, dwOldGpioCfg);
                                dwOldGpioCfg = pAdapter->lan9118_data.dwOriginalGpioCfg;
                                pAdapter->lan9118_data.LED1NotYetRestored = (BOOLEAN)FALSE;
                                SetRegDW(pAdapter->lan9118_data.dwLanBase, GPIO_CFG,
                                    dwOldGpioCfg);
                            }
                        }
                    }
                }
#endif // USE_LED1_WORK_AROUND
            }
        }
        else
        {
            // Fixed parameter Link Up
            dwTemp = Lan_GetLinkMode(&pAdapter->lan9118_data);
            dwRegVal = Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase,MAC_CR);
            dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);
            switch (dwTemp) 
            {
                case LINK_10MPS_HALF:
                case LINK_100MPS_HALF:
                    dwRegVal |= MAC_CR_RCVOWN_;
                    break;

                case LINK_10MPS_FULL:
                case LINK_100MPS_FULL:
                    dwRegVal |= MAC_CR_FDPX_;
                    break;
                default:
                    break;
            }
            Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, MAC_CR, dwRegVal);
            pAdapter->PhyDPCNeeded = (BOOLEAN)TRUE;     // need to LinkIndicate
        }
        pAdapter->fLinkUp = TRUE;
    }

    if ((pAdapter->fLinkUp == TRUE) &&
        ((wPhyBSR & PHY_BSR_LINK_STATUS_) == (WORD)0x0))
    {
        // Link Down
        pAdapter->PhyDPCNeeded = (BOOLEAN)TRUE;
#ifdef USE_LED1_WORK_AROUND
        // workaround for 118/117/116/115 family
        if ((pAdapter->lan9118_data.dwIdRev & 0xFFF0FFFFUL) == 0x01100002UL)
        {
            if(pAdapter->lan9118_data.bPhyAddress == (BYTE)1)   // on internal PHY use only
            {
                DWORD   dwOldGpioCfg=GetRegDW(pAdapter->lan9118_data.dwLanBase, GPIO_CFG);

                // Check global setting that LED1 usage is 10/100 indicator
                if (dwOldGpioCfg & GPIO_CFG_LED1_EN_)
                {
                    //Force 10/100 LED off, after saving orginal GPIO config
                    pAdapter->lan9118_data.dwOriginalGpioCfg = dwOldGpioCfg;
                    pAdapter->lan9118_data.LED1NotYetRestored = (BOOLEAN)TRUE;

                    dwOldGpioCfg &= ~GPIO_CFG_LED1_EN_;
                    dwOldGpioCfg |=  GPIO_CFG_LED1_DIS_;
                    SetRegDW(pAdapter->lan9118_data.dwLanBase, GPIO_CFG,
                        dwOldGpioCfg);
                    SMSC_TRACE2(DBG_PHY,"LED1 off GPIO_CFG:0x%08x-->0x%08x\n\r",
                        pAdapter->lan9118_data.dwOriginalGpioCfg, dwOldGpioCfg);
                }
            }
        }
#endif // USE_LED1_WORK_AROUND

        pAdapter->fLinkUp = FALSE;
    }

    wOldPhyBSR = wPhyBSR;
    SMSC_TRACE0(DBG_PHY,"-CheckPhyStatus\r\n");
}

/*----------------------------------------------------------------------------
    HandlerGptISR
*/
VOID HandlerGptISR(PSMSC9118_ADAPTER pAdapter)
{
    DWORD   dw;

    SMSC_TRACE0(DBG_ISR, "+HandlerGptISR\r\n");

    // Reload GPT Timer
    dw = GetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG);
    dw &= ~0xFFFFUL;
    dw |= GPT_INT_INTERVAL;
    SetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG, dw);

    CheckPhyStatus(pAdapter);

    if ((pAdapter->fLoadBalancing == 0) &&
        (QUEUE_COUNT(&(pAdapter->TxDeferedPkt)) > 200))
    {
        pAdapter->fLoadBalancing = 1;
        DisableMacRxEn(pAdapter);
    }
    else if ((pAdapter->fLoadBalancing == 1) &&
             (QUEUE_COUNT(&(pAdapter->TxDeferedPkt)) < 50))
    {
        pAdapter->fLoadBalancing = 0;
        EnableMacRxEn(pAdapter);
    }

    SMSC_TRACE0(DBG_ISR, "-HandlerGptISR\r\n");
}


/*----------------------------------------------------------------------------
    HandlerPmeISR
*/
VOID
HandlerPmeISR(CPCSMSC9118_ADAPTER pAdapter)
{
    if (SetPowerState((PSMSC9118_ADAPTER)pAdapter, NdisDeviceStateD0) == FALSE)
    {
        SMSC_WARNING0("HandlerPmeISR(): SetPowerState to NdisDeviceStateD0 failed\r\n");
    }
}

/*----------------------------------------------------------------------------
    RxForceReceiverDiscard
        reset the RX alone
        NOTE: assumes that the reciever was already running
*/
static VOID RxForceReceiverDiscard(PSMSC9118_ADAPTER pAdapter)
{
    DMA_XFER    dmaXfer;
    DWORD       dwReg;
    int         i;

    // Wait until DMA stop
    if (pAdapter->fRxDMAMode)
    {
        dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
        DmaComplete(&dmaXfer, DMA_RX_CH);
    }

    // Disable Mac Rx
    DisableMacRxEn(pAdapter);

    /* discard all RX FIFO data via the DUMP cmd, and
     * wait for DUMP to self-clear
     */
    dwReg = GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG);
    dwReg |= RX_CFG_RX_DUMP_;
    SetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG, dwReg);
    B2BWriteDelay(pAdapter->lan9118_data.dwLanBase, 1);

    for (i=0; i < 100; i++)
    {
        if ((GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG) & RX_CFG_RX_DUMP_) == 0)
            break;
    }

    /* re-start the reciever (RXEN) */
    EnableMacRxEn(pAdapter);
}

/*----------------------------------------------------------------------------
    Smsc9118Isr
        NDIS miniport function
*/
VOID Smsc9118Isr(OUT PBOOLEAN pbInterruptRecognized, OUT PBOOLEAN pbQueueDpc, IN PVOID pContext)
{
    SMSC9118_ADAPTER * const pAdapter = ((PSMSC9118_ADAPTER)pContext);
    volatile DWORD dwIntCfg, dwIntStatus, dwIntEn;
    const DWORD dwLanBase=pAdapter->lan9118_data.dwLanBase;

    // Wake 9118 up if it is in bed
    while (!(GetRegDW(dwLanBase, PMT_CTRL) & PMT_CTRL_READY_))
    {
        SetRegDW(dwLanBase, BYTE_TEST, 0xFFFFFFFF);
    }

    dwIntCfg = GetRegDW(dwLanBase, INT_CFG);
    SMSC_ASSERT(!(dwIntCfg&INT_CFG_IRQ_RESERVED_));

#ifdef  SMSC_DIRECT_INTR
    // Disable 9118 global Interrupt
    NicDisableInterrupt(pAdapter);
#endif

    if (dwIntCfg & INT_CFG_IRQ_INT_)
    {
        dwIntStatus = Lan_GetInterruptStatus((PLAN9118_DATA)&(pAdapter->lan9118_data));
        dwIntEn = GetRegDW(dwLanBase, INT_EN);
        dwIntStatus &= dwIntEn;

        if (dwIntStatus & INT_STS_RSFL_)
        {
            HandlerRxISR((PSMSC9118_ADAPTER)pAdapter);
            // Clear the RSFL interrupt after processing packets.
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_RSFL_);
        }

        if (dwIntStatus & INT_STS_TDFA_)
        {
            // Clear the TDFA interrupt STS.
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_TDFA_);
            HandlerTxISR((PSMSC9118_ADAPTER)pAdapter);
        }

        if (dwIntStatus & INT_STS_GPT_INT_)
        {
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_GPT_INT_);
            HandlerGptISR((PSMSC9118_ADAPTER)pAdapter);
        }

        if (dwIntStatus & INT_STS_PME_INT_)
        {
            // Clear INT_STS_PEM_INT_ will be cleared in HandlerPmeISR()
            //  (actually, in SetPowerState())
            HandlerPmeISR((PSMSC9118_ADAPTER)pAdapter);
        }

        if (dwIntStatus & INT_STS_RXE_)
        {
            RxForceReceiverDiscard(pAdapter);
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_RXE_);
#ifdef DEBUG
            RETAILMSG(1, (TEXT("Smsc9118Isr: INT_STS_RXE_ handled !!!\r\n")));
#endif
        }

        if (dwIntStatus & INT_STS_TXE_)
        {
            /* Transmitter error.  Transmit stops because the
             * TX_FIFOs (TXS_DUMP_:15, TXD_DUMP_:14, TX_ON_:1)
             * became misaligned.  There should not be any
             * need to wait afterwards.
             */
            SetRegDW(dwLanBase, TX_CFG, 
                    TX_CFG_TXS_DUMP_ | TX_CFG_TXD_DUMP_ | TX_CFG_STOP_TX_);

            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_TXE_);
#ifdef DEBUG
            RETAILMSG(1, (TEXT("Smsc9118Isr: INT_STS_TXE_ handled !!!\r\n")));
#endif
        }

        if (dwIntStatus & INT_STS_RDFO_)
        {
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_RDFO_);
            pAdapter->RxOverRun = (BOOLEAN)TRUE;
        }

        if (dwIntStatus & INT_STS_SW_INT_)
        {
            Lan_DisableInterrupt((PLAN9118_DATA)&pAdapter->lan9118_data, INT_EN_SW_INT_EN_);
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_SW_INT_);
        }

        if (dwIntStatus & INT_STS_RWT_)
        {
            RxForceReceiverDiscard(pContext);
            Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_RWT_);
#ifdef  DEBUG
            pAdapter->IntrCount.dwIntWRT++;
            RETAILMSG(1, (TEXT("Smsc9118Isr: INT_STS_RWT_ handled !!!\r\n")));
#endif            
        }

        // The interrupt is from 9118, schedule a DPC if needed.
        if ((pAdapter->RxDPCNeeded == (BOOLEAN)TRUE) || 
            (pAdapter->TxDPCNeeded == (BOOLEAN)TRUE) ||
            (pAdapter->PhyDPCNeeded == (BOOLEAN)TRUE))
        {
            *pbInterruptRecognized = (BOOLEAN)TRUE;
            *pbQueueDpc = (BOOLEAN)TRUE;
            //pAdapter->RxDPCNeeded = pAdapter->TxDPCNeeded = (BOOLEAN)FALSE;
        }
        else
        {
            *pbInterruptRecognized = (BOOLEAN)TRUE;
            *pbQueueDpc = (BOOLEAN)FALSE;
        }
    }
    else
    {
        // The interrupt is not from 9118. Do not schedule DPC.
        *pbInterruptRecognized = (BOOLEAN)FALSE;
        *pbQueueDpc = (BOOLEAN)FALSE;
    }

#ifdef  SMSC_DIRECT_INTR
    // Reenable GPIO Interrupt 
    PlatformEnableGpioInterrupt();

    //NdisMSleep(1);        // 1uSec Delay
    
    // ReEnable global Interrupt
    SetRegDW(dwLanBase, INT_CFG, 
            GetRegDW(dwLanBase, INT_CFG) | INT_CFG_IRQ_EN_ | 
            ((pAdapter->dIntrDeas & 0xFF) << 24));
#endif

    return;
}


/*----------------------------------------------------------------------------
    Smsc9118HandleInterrupt
        NDIS miniport function
*/
VOID
Smsc9118HandleInterrupt (IN NDIS_HANDLE hMiniportAdapterContext)
{
	SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);
	DPC_STATUS	TxStatus = DPC_STATUS_DONE;
	DPC_STATUS	RxStatus = DPC_STATUS_DONE;
	DPC_STATUS	PhyStatus;

    SMSC_TRACE0(DBG_ISR,"+Smsc9118HandleInterrupt\r\n");
	SMSC_ASSERT(pAdapter);

	if (pAdapter)
	{
		do 
		{
			if (!IS_ARRAY_EMPTY(pAdapter->FullPkt) && RxStatus == DPC_STATUS_DONE)
			{
                SMSC_TRACE0(DBG_RX,"DPC for RSFL interrupt\r\n");
				RxStatus = HandlerRxDPC((PSMSC9118_ADAPTER)pAdapter);
				// Make Lint Happy
				RxStatus = RxStatus;
			}

			if (QUEUE_COUNT(&(pAdapter->TxDeferedPkt)) != 0UL && TxStatus == DPC_STATUS_DONE)
			{
                SMSC_TRACE0(DBG_TX,"DPC for TDFA interrupt\r\n");
				TxStatus = HandlerTxDPC((PSMSC9118_ADAPTER)pAdapter);
				// Make Lint Happy
				TxStatus = TxStatus;
			}
		} while ((!IS_ARRAY_EMPTY(pAdapter->FullPkt) && RxStatus == DPC_STATUS_DONE) || 
		         (QUEUE_COUNT(&(pAdapter->TxDeferedPkt)) != 0UL && TxStatus == DPC_STATUS_DONE));

		if (pAdapter->PhyDPCNeeded == (BOOLEAN)TRUE)
		{
            SMSC_TRACE0(DBG_ISR,"   PhyDPCNeeded\r\n");
            PhyStatus = HandlerPhyDPC(pAdapter);
			// Make Lint Happy
			PhyStatus = PhyStatus;
		}

		if (pAdapter->RxOverRun == (BOOLEAN)TRUE)
		{
			DMA_XFER 	dmaXfer;
			DWORD		dwReg, dwRegOld, dwCount;

            SMSC_TRACE0(DBG_ISR,"   RxOverRun\r\n");

			if (pAdapter->fRxDMAMode)
			{
				dmaXfer.DMABaseVA = pAdapter->DMABaseVA;
				DmaComplete(&dmaXfer, DMA_RX_CH);
			}
			DisableMacRxEn(pAdapter);

			dwRegOld = dwReg = GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG);
			dwReg &= 0x3FFFFFFFUL;
			dwReg |= RX_CFG_RX_DUMP_;
			SetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG, dwReg);
			dwCount = 0UL;
			while (GetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG) & RX_CFG_RX_DUMP_)
			{
				NdisStallExecution(10U);		// 10uSec
				if (dwCount++ > 100UL)	// 10u * 1000 = 1mSec
				{
					SMSC_WARNING0("Timeout waiting RX_DUMP\n");
					break;
				}
			}
			SetRegDW(pAdapter->lan9118_data.dwLanBase, RX_CFG, dwRegOld);

			EnableMacRxEn(pAdapter);
			pAdapter->RxOverRun = (BOOLEAN)FALSE;
		}

	}
    SMSC_TRACE0(DBG_ISR,"-Smsc9118HandleInterrupt\r\n");
}


/*----------------------------------------------------------------------------
    Smsc9118CheckForHang
        NDIS miniport function
*/
BOOLEAN Smsc9118CheckForHang(IN NDIS_HANDLE hMiniportAdapterContext)
{
    BOOLEAN bRet = (BOOLEAN)FALSE;
    const SMSC9118_ADAPTER * const pAdapter = (PSMSC9118_ADAPTER)(hMiniportAdapterContext);

    SMSC_TRACE0(DBG_TX,"+Smsc9118CheckForHang\r\n");

	//RETAILMSG(1,(L"+Smsc9118CheckForHang\r\n"));

    SMSC_ASSERT(pAdapter);

    if (pAdapter)
    {
        // Make Lint Happy
        hMiniportAdapterContext = hMiniportAdapterContext;

        // 1500 is a kind of heuristic value.
        // It might be different from systems.
        if (QUEUE_COUNT(&pAdapter->TxDeferedPkt) > 1500UL)
        {
            RETAILMSG(1, (TEXT("Oops! Too many Pkts are defered. I assume H/W is not operating. It will cause MiniportReset()\r\n")));
            bRet = (BOOLEAN)TRUE;
        }
    }

    SMSC_TRACE0(DBG_TX,"-Smsc9118CheckForHang\r\n");
    return bRet;
}

/*---------------------------------------------------------------------------
    EnableSwFlowControlFD
        Enable software flow control in full duplex mode.
*/
VOID EnableSwFlowControlFD(IN const SMSC9118_ADAPTER * const pAdapter)
{
    DWORD dwTimeout=1000UL;

    SMSC_TRACE0(DBG_FLOW,"EnableSwFlowControlFD\r\n");

    // Enable flow control.
    SetRegDW(pAdapter->lan9118_data.dwLanBase, AFC_CFG, DEFAULT_AFC_CFG);

    //Wait for busy bit to clear.
    while ((Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, FLOW)&FLOW_FCBSY_)  && (dwTimeout > 0UL)) 
    {
        SMSC_MICRO_DELAY(1U);
        dwTimeout--;
    }

    if(!dwTimeout)
    {
        SMSC_WARNING0("Timeout waiting flow busy bit.\r\n");
    }

    // Set FLOW register in MAC to send out a pause frame.
    Lan_SetMacRegDW(pAdapter->lan9118_data.dwLanBase, 
                    FLOW, 
                    (FLOW_FCEN_ | (0xFFFFUL<<16UL) | FLOW_FCBSY_));
}

//
// Power Management
//

/*---------------------------------------------------------------------------
    CalculateCrc16
        Calculate CRC16 for WUFF register.
*/
WORD
CalculateCrc16(const BYTE * bpData, const DWORD dwLen, const BOOL fBitReverse)
{
    const WORD wCrc16Poly = (WORD)0x8005;   // s/b international standard for CRC-16
                                // x^16 + x^15 + x^2 + 1
    //WORD crc16_poly = 0xA001; // reverse
    WORD i, j, bit;
    WORD wCrc = (WORD)0xFFFF;
    WORD wMsb;
    BYTE bCurrentByte;
    WORD wNumOfBits = (WORD)16;
    WORD wCrcOut = (WORD)0;

    wNumOfBits = wNumOfBits; // to avoid lint warning

    for (i=(WORD)0; i<(WORD)dwLen; i++)
    {
        bCurrentByte = *bpData++;

        for (bit=(WORD)0U; bit<(WORD)8U; bit++)
        {
            wMsb = (WORD)(wCrc >> 15);
            wCrc = (WORD)(wCrc << 1);

            if (wMsb ^ (WORD)(bCurrentByte & 1))
            {
                wCrc = (WORD)(wCrc ^ wCrc16Poly);
                wCrc = (WORD)(wCrc | (WORD)1);
            }
            bCurrentByte = (BYTE)(bCurrentByte >> 1);
        }
    }

    //bit reverse if needed
    // so far we do not need this for 117
    // but the standard CRC-16 seems to require this.
    if (fBitReverse)
    {
        j = (WORD)1;
        for (i=(WORD)(1<<(wNumOfBits-(WORD)1U)); i; i = (WORD)(i>>1)) {
            if (wCrc & i)
            {
                wCrcOut =  (WORD)(wCrcOut | j);
            }
            // j <<= 1;
            j = (WORD)(j << 1);
        }
        wCrc = wCrcOut;

    }

    return wCrc;
}


/*---------------------------------------------------------------------------
    SetWakeUpFrameFilter
*/
VOID SetWakeUpFrameFilter(IN PSMSC9118_ADAPTER pAdapter, const NDIS_PM_PACKET_PATTERN  * const pPattern, const DWORD FilterNo)
{
    DWORD MaskSize, PatternSize, dw, Crc16Length, tempMask;
    const UCHAR * const pPatternStart = (PUCHAR)pPattern+(pPattern->PatternOffset);
    const UCHAR * const pMaskStart = (PUCHAR)pPattern + sizeof(NDIS_PM_PACKET_PATTERN);

    SMSC_TRACE0(DBG_POWER,"+SetWakeUpFrameFilter\r\n");

    SMSC_ASSERT(FilterNo < 4UL);
    if(FilterNo >= 4UL)
    {
        return;
    }

    MaskSize = pPattern->MaskSize;
    PatternSize = pPattern->PatternSize;

    //
    // Calculate Byte Mask
    //
    tempMask = (((DWORD)*(pMaskStart + 1UL)) >> 4) & 0x0FUL; //Take the first 4-bit mask after dst/src addresses.
    for (dw=2UL; dw<MaskSize; dw++) //MaskSize must be >=2 && <6, otherwise we won't be here.
    {
        tempMask |= (((DWORD)*(pMaskStart+dw))<<((dw-2UL)*8UL+4UL));
    }
    pAdapter->Wuff.FilterByteMask[FilterNo] = tempMask;

    tempMask = 0xFFFFFFFFUL >> (32UL-(PatternSize-12UL)); //bits to mask out in the Byte Mask.  
    tempMask &=0x7FFFFFFFUL;  //Make sure the bit-31 is reset according to Synopsys Databook.
    (pAdapter->Wuff.FilterByteMask[FilterNo]) &= tempMask;
    SMSC_TRACE2(DBG_POWER,"Wuff.FilterByteMask[%d] = 0x%x\r\n", FilterNo, pAdapter->Wuff.FilterByteMask[FilterNo]);

    //
    // Calculate CRC16
    //
    Crc16Length=0UL;
    tempMask = (pAdapter->Wuff.FilterByteMask[FilterNo]);
    for (dw=0UL; dw<(PatternSize-12UL); dw++)
    {
        if (tempMask&1UL)
        {
            pAdapter->Wuff.FilterBufferToCRC16[FilterNo][Crc16Length] = *(pPatternStart+12+dw);
            Crc16Length++;
        }
        tempMask >>= 1;
    }
    pAdapter->Wuff.FilterBufferLength[FilterNo] = Crc16Length;

    SMSC_TRACE2(DBG_POWER,"Wuff.FilterBufferLength[%d] = 0x%x\r\n", FilterNo, pAdapter->Wuff.FilterBufferLength[FilterNo]);

    {
        DWORD dw1;
        SMSC_TRACE0(DBG_POWER," FilterBufferToCRC16\r\n");
        for(dw1=0UL; dw1<Crc16Length; dw1++)
        {
            SMSC_TRACE1(DBG_POWER,"0x%x\r\n", pAdapter->Wuff.FilterBufferToCRC16[FilterNo][dw1]);
        }
    }

    pAdapter->Wuff.FilterCRC16[FilterNo] = CalculateCrc16(pAdapter->Wuff.FilterBufferToCRC16[FilterNo],
                                                          pAdapter->Wuff.FilterBufferLength[FilterNo],
                                                          FALSE);

    SMSC_TRACE2(DBG_POWER,"Wuff.FilterCRC16[%d] = 0x%x\r\n", FilterNo, pAdapter->Wuff.FilterCRC16[FilterNo]);


    //
    // Claculate Offset
    //

    //(It seems that WinCE always(?) sends down the sample wakeup frame starting from dst address. So, set the offset to 12.)
    pAdapter->Wuff.FilterOffsets[FilterNo] = (BYTE)12;

    SMSC_TRACE0(DBG_POWER,"-SetWakeUpFrameFilter\r\n");

}


/*---------------------------------------------------------------------------
    ResetWakeUpFrameFilter
*/
VOID
ResetWakeUpFrameFilter(IN PSMSC9118_ADAPTER pAdapter, const NDIS_PM_PACKET_PATTERN * const pPattern)
{
    DWORD PatternSize, dw;
    WORD  FilterCRC16;
    BYTE FilterBufferToCRC16[31];
    DWORD FilterBufferLength;
    const UCHAR * const pPatternStart = (PUCHAR)pPattern+(pPattern->PatternOffset);

    SMSC_TRACE0(DBG_POWER,"+ResetWakeUpFrameFilter\r\n");

    PatternSize = pPattern->PatternSize;

    //
    // Calculate CRC16
    //
    NdisFillMemory((PVOID)FilterBufferToCRC16, (ULONG)(31*sizeof(BYTE)), (UCHAR)0);
    for (dw=0UL; dw<(PatternSize-12UL); dw++)
    {
        FilterBufferToCRC16[dw] = *(pPatternStart+12+dw);
    }
    FilterBufferLength = PatternSize - 12UL;

    FilterCRC16 = CalculateCrc16(FilterBufferToCRC16, FilterBufferLength, FALSE);

    SMSC_TRACE1(DBG_POWER,"FilterCRC16 = 0x%x\r\n", FilterCRC16);

    //There is at least one filter enabled. Search for and compare with them.
    if (((pAdapter->Wuff.FilterCommands)&FILTER0_ENABLE) &&
       (pAdapter->Wuff.FilterCRC16[0] == FilterCRC16))
    {
        (pAdapter->Wuff.FilterCommands) &= ~FILTER0_ENABLE;
        SMSC_TRACE0(DBG_POWER,"FILTER0 disabled.\r\n");
    }
    else
    {
        if (((pAdapter->Wuff.FilterCommands)&FILTER1_ENABLE) &&
            (pAdapter->Wuff.FilterCRC16[1] == FilterCRC16))
        {
            (pAdapter->Wuff.FilterCommands) &= ~FILTER1_ENABLE;
            SMSC_TRACE0(DBG_POWER,"FILTER1 disabled.\r\n");
        }
        else
        {
            if (((pAdapter->Wuff.FilterCommands)&FILTER2_ENABLE) &&
                (pAdapter->Wuff.FilterCRC16[2] == FilterCRC16))
            {
                (pAdapter->Wuff.FilterCommands) &= ~FILTER2_ENABLE;
                SMSC_TRACE0(DBG_POWER,"FILTER2 disabled.\r\n");
            }
            else
            {
                if (((pAdapter->Wuff.FilterCommands)&FILTER3_ENABLE) &&
                    (pAdapter->Wuff.FilterCRC16[3] == FilterCRC16))
                {
                    (pAdapter->Wuff.FilterCommands) &= ~FILTER3_ENABLE;
                    SMSC_TRACE0(DBG_POWER,"FILTER3 disabled.\r\n");
                }
                else
                {
                    SMSC_TRACE0(DBG_POWER,"No matching filter set.\r\n");
                }
            }
        }
    }

    SMSC_TRACE0(DBG_POWER,"-ResetWakeUpFrameFilter\r\n");
}

void NicEnableInterrupt(IN const PSMSC9118_ADAPTER pAdapter)
{
    DWORD   dwTemp, dwLanBase;

    dwLanBase = pAdapter->lan9118_data.dwLanBase;

    dwTemp = GetRegDW(dwLanBase, INT_CFG);
    dwTemp |= INT_CFG_IRQ_EN_;
    SetRegDW(dwLanBase, INT_CFG, dwTemp);
}

void NicDisableInterrupt(IN const PSMSC9118_ADAPTER pAdapter)
{
    DWORD   dwTemp, dwLanBase;

    dwLanBase = pAdapter->lan9118_data.dwLanBase;

    dwTemp = GetRegDW(dwLanBase, INT_CFG);
    dwTemp &= ~INT_CFG_IRQ_EN_;
    SetRegDW(dwLanBase, INT_CFG, dwTemp);
}

/*---------------------------------------------------------------------------
    SetPowerState
*/
BOOL
SetPowerState(IN PSMSC9118_ADAPTER pAdapter, const NDIS_DEVICE_POWER_STATE PowerState)
{
    BOOL    ret;
    volatile DWORD dw;
    const DWORD dwLanBase = pAdapter->lan9118_data.dwLanBase;
    WORD wd;

    //
    // Set device into D0 mode.
    //
    if ((PowerState == NdisDeviceStateD0) &&
        (pAdapter->CurrentPowerState != NdisDeviceStateD0))
    {
        if (Lan_Wakeup9118((PLAN9118_DATA)&pAdapter->lan9118_data) != TRUE)
        {
            SMSC_TRACE0(DBG_WARNING, "Failed! at Wakeup118()\r\n");
            return FALSE;
        }
        else 
        {
            if (pAdapter->CurrentPowerState == NdisDeviceStateD3) 
            {
                WORD    wValue;

                SetRegDW(dwLanBase, PMT_CTRL, 
                        GetRegDW(dwLanBase, PMT_CTRL) & ~PMT_CTRL_PM_MODE_);
                if (AdapterReadPhy(PHY_BCR, &wValue) == FALSE)
                {
                    SMSC_WARNING0("SetPowerState(): Failed to read PHY_BCR\r\n");
                    return FALSE;
                }
                if (AdapterWritePhy(PHY_BCR, wValue & ~PHY_BCR_POWER_DOWN_) == FALSE)
                {
                    SMSC_WARNING0("SetPowerState(): Failed to write to PHY_BCR\r\n");
                    return FALSE;
                }
                NicEnableInterrupt(pAdapter);
            }
        }

        // This will work on Internal Phy
        if (pAdapter->lan9118_data.bPhyAddress == 1)
        {
            // Disable PHY Energy On Int
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_MASK, &wd);
            if (ret == FALSE)
            {
                SMSC_WARNING0("SetPowerState(): Failed to read PHY_INT_MASK\r\n");
                return FALSE;
            }

            Lan_SetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_MASK, wd & ~PHY_INT_MASK_ENERGY_ON_);

            // clear PHY Int Source
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_SRC, &wd);

            // Put PHY out from EnergyOn mode
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_MODE_CTRL_STS, &wd);
            wd = (WORD)(wd & ~MODE_CTRL_STS_EDPWRDOWN_);
            Lan_SetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_MODE_CTRL_STS, wd);
        }

        // reset 118 and restore packet filter & multicast list
        ResetAndRestore(pAdapter);

        // Disable PME interrupt & clear pending status
        Lan_DisableInterrupt(&pAdapter->lan9118_data, INT_EN_PME_INT_EN_);
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_PME_INT_);

        // Clear pending status & Enable GPT interrupt
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_GPT_INT_);
        Lan_EnableInterrupt(&pAdapter->lan9118_data, INT_EN_GPT_INT_EN_);

        pAdapter->CurrentPowerState = NdisDeviceStateD0;
        return TRUE;
    }

    // Set device into D1 mode.
    if ((PowerState == NdisDeviceStateD1) &&
        (pAdapter->CurrentPowerState != NdisDeviceStateD1) &&
        (pAdapter->dwWakeUpSource & (DWORD)(NDIS_PNP_WAKE_UP_PATTERN_MATCH|NDIS_PNP_WAKE_UP_MAGIC_PACKET)) )
    {
        if (pAdapter->dwWakeUpSource & (DWORD)NDIS_PNP_WAKE_UP_PATTERN_MATCH)
        {
            // Config WUFF
            ConfigWUFFReg(pAdapter);
        }

        // Clear Remote-Wakeup-Frame-Received(WUFR)/Magic-Packet_Received(MPR).
        dw = Lan_GetMacRegDW(dwLanBase, WUCSR);
        dw &= (~(WUCSR_WUFR_ | WUCSR_MPR_));

        // Enable WUEN/MPEN in WUCSR.
        dw &= (~(WUCSR_WAKE_EN_ | WUCSR_MPEN_));

        if (pAdapter->dwWakeUpSource & (DWORD)NDIS_PNP_WAKE_UP_PATTERN_MATCH)
        {
            dw |= WUCSR_WAKE_EN_;
        }

        if (pAdapter->dwWakeUpSource & (DWORD)NDIS_PNP_WAKE_UP_MAGIC_PACKET)
        {
            dw |= WUCSR_MPEN_;
        }
        Lan_SetMacRegDW(dwLanBase, WUCSR, dw);

        // Disbale GP Timer in case that 9118 goes to D1
        // I.e, system is still running
        DisableGPTimer(pAdapter);

        // Disable the GPT timer
        SetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG, 
            GetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG) & 
            ~GPT_CFG_TIMER_EN_);

        Lan_DisableInterrupt(&pAdapter->lan9118_data, INT_EN_GPT_INT_EN_);
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_GPT_INT_);

        // Clear WUPS
        dw = GetRegDW(dwLanBase, PMT_CTRL);
        SetRegDW(dwLanBase, PMT_CTRL, dw|PMT_CTRL_WUPS_);
        B2BWriteDelay(dwLanBase, 7);

        // Clear pending PME interrupt
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_PME_INT_);
        // Enable PME interrupt.
        Lan_EnableInterrupt(&pAdapter->lan9118_data, INT_EN_PME_INT_EN_);
        
        // Set PME_CTRL to put chip into D1 mode.
        dw = GetRegDW(dwLanBase, PMT_CTRL)&~PMT_CTRL_PM_MODE_;
        dw |= (PMT_CTRL_WOL_EN_ | PMT_CTRL_PM_MODE_WOL_);
        SetRegDW(dwLanBase, PMT_CTRL, dw);
        B2BWriteDelay(dwLanBase, 7);

        pAdapter->CurrentPowerState = NdisDeviceStateD1;
        // set Link to DOWN status
        pAdapter->fLinkUp = FALSE;

        SMSC_TRACE0(DBG_POWER, "Device Power State = NdisDeviceStateD1.\r\n");
        return TRUE;
    }

    // Set device into D2 mode.
    if ((PowerState == NdisDeviceStateD2) &&
        (pAdapter->CurrentPowerState != NdisDeviceStateD2))
    {
        // This only works on Internal Phy
        if (pAdapter->lan9118_data.bPhyAddress == 1)
        {
            WORD    wValue;

            // Disable Auto Neg
            if (AdapterReadPhy(PHY_BCR, &wValue) == FALSE)
            {
                SMSC_WARNING0("SetPowerState(): Failed to read PHY_BCR\r\n");
                return FALSE;
            }
            if (AdapterWritePhy(PHY_BCR, wValue & ~PHY_BCR_AUTO_NEG_ENABLE_) == FALSE)
            {
                SMSC_WARNING0("SetPowerState(): Failed to write to PHY_BCR\r\n");
                return FALSE;
            }

            // Disable Auto MDIX
            if (AdapterReadPhy(SPECIAL_CTRL_STS, &wValue) == FALSE)
            {
                SMSC_WARNING0("SetPowerState(): Failed to read SPECIAL_CTRL_STS\r\n");
                return FALSE;
            }

            if (AdapterWritePhy(SPECIAL_CTRL_STS, 
                    wValue & 
                    ~(SPECIAL_CTRL_STS_AMDIX_ENABLE_ | 
                      SPECIAL_CTRL_STS_AMDIX_STATE_)) == FALSE)
            {
                SMSC_WARNING0("SetPowerState(): Failed to write to SPECIAL_CTRL_STS\r\n");
                return FALSE;
            }

            // clear PHY Int Source
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_SRC, &wd);

            // Enable PHY Energy On Int
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_MASK, &wd);
            Lan_SetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_INT_MASK, wd | PHY_INT_MASK_ENERGY_ON_);

            // Put PHY to Energy On mode
            ret = Lan_GetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_MODE_CTRL_STS, &wd);
            Lan_SetPhyRegW(dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, PHY_MODE_CTRL_STS, wd | MODE_CTRL_STS_EDPWRDOWN_);
        }
        else
        {
            /*
             * Energy Detect works on Internal PHY only.
             * But we keep move on here with warning message.
             */
            SMSC_WARNING0("Energy Detection PowerDown will ONLY work on Internal PHY\r\n");
            return FALSE;
        }

        // Disbale GP Timer
        DisableGPTimer(pAdapter);

        // Clear WUPS
        dw = GetRegDW(dwLanBase, PMT_CTRL);
        SetRegDW(dwLanBase, PMT_CTRL, dw | PMT_CTRL_WUPS_);
        B2BWriteDelay(dwLanBase, 7);

        // Enable Energy Detect
        dw = GetRegDW(dwLanBase, PMT_CTRL) & ~PMT_CTRL_PM_MODE_;
        SetRegDW(dwLanBase, PMT_CTRL, dw | PMT_CTRL_ED_EN_);
        B2BWriteDelay(dwLanBase, 7);

        // Clear pending PME interrupt
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_PME_INT_);
        // Enable PME interrupt.
        Lan_EnableInterrupt(&pAdapter->lan9118_data, INT_EN_PME_INT_EN_);
        
        dw = GetRegDW(dwLanBase, PMT_CTRL);
        // Set PME_CTRL to put chip into D2 mode.
        SetRegDW(dwLanBase, PMT_CTRL, dw | PMT_CTRL_PM_MODE_ED_);

        pAdapter->CurrentPowerState = NdisDeviceStateD2;

        // set Link to DOWN status
        pAdapter->fLinkUp = FALSE;

        SMSC_TRACE0(DBG_POWER, "Device Power State = NdisDeviceStateD2.\r\n");
        return TRUE;
    }


    // Set device into D3 mode.
    if ((PowerState == NdisDeviceStateD3) &&
        (pAdapter->CurrentPowerState != NdisDeviceStateD3))
    {
        WORD    wValue;

        // This is called when Device is disabled..
        
        // disable 9118 interrupt
        NicDisableInterrupt(pAdapter);

        // 9118 doesn't support D3.
        // So, when D3 is requested, put 9118 to D2 mode
        if (AdapterReadPhy(PHY_BCR, &wValue) == FALSE)
        {
            SMSC_WARNING0("SetPowerState(): Failed to read PHY_BCR\r\n");
            return FALSE;
        }
        if (AdapterWritePhy(PHY_BCR, wValue | PHY_BCR_POWER_DOWN_) == FALSE)
        {
            SMSC_WARNING0("SetPowerState(): Failed to write to PHY_BCR\r\n");
            return FALSE;
        }

        dw = GetRegDW(dwLanBase, PMT_CTRL) & ~PMT_CTRL_PM_MODE_;
        SetRegDW(dwLanBase, PMT_CTRL, dw|PMT_CTRL_PM_MODE_ED_);
        B2BWriteDelay(dwLanBase, 7);

        // Disbale GP Timer in case that 9118 goes to D1
        // I.e, system is still running
        DisableGPTimer(pAdapter);

        pAdapter->CurrentPowerState = NdisDeviceStateD3;
        // set Link to DOWN status
        pAdapter->fLinkUp = FALSE;

        SMSC_TRACE0(DBG_POWER, "Device Power State = NdisDeviceStateD3.\r\n");
        return TRUE;
    }

    return TRUE;
}    
    

/*---------------------------------------------------------------------------
    ConfigWUFFReg
*/
VOID
ConfigWUFFReg(IN const SMSC9118_ADAPTER * const pAdapter)
{
    const DWORD dwLanBase = pAdapter->lan9118_data.dwLanBase;
    const DWORD FilterOffsets = 
                         ((DWORD)(pAdapter->Wuff.FilterOffsets[3])<<24) + 
                          ((DWORD)(pAdapter->Wuff.FilterOffsets[2])<<16) +
                          ((DWORD)(pAdapter->Wuff.FilterOffsets[1])<<8) + 
                          (DWORD)(pAdapter->Wuff.FilterOffsets[0]);
    const DWORD Filter10CRC16 = 
                         ((DWORD)(pAdapter->Wuff.FilterCRC16[1])<<16) + 
                          (DWORD)(pAdapter->Wuff.FilterCRC16[0]);
    const DWORD Filter32CRC16 = 
                         ((DWORD)(pAdapter->Wuff.FilterCRC16[3])<<16) + 
                          (DWORD)(pAdapter->Wuff.FilterCRC16[2]);

    // Write #1 Filter 0 byte mask
    Lan_SetMacRegDW(dwLanBase, WUFF, pAdapter->Wuff.FilterByteMask[0]);
    SMSC_TRACE1(DBG_POWER, "Filter0 ByteMask = 0x%x\r\n", pAdapter->Wuff.FilterByteMask[0]);
    // Write #2 Filter 1 byte mask
    Lan_SetMacRegDW(dwLanBase, WUFF, pAdapter->Wuff.FilterByteMask[1]);
    SMSC_TRACE1(DBG_POWER, "Filter1 ByteMask = 0x%x\r\n", pAdapter->Wuff.FilterByteMask[1]);
    // Write #3 Filter 2 byte mask
    Lan_SetMacRegDW(dwLanBase, WUFF, pAdapter->Wuff.FilterByteMask[2]);
    SMSC_TRACE1(DBG_POWER, "Filter2 ByteMask = 0x%x\r\n", pAdapter->Wuff.FilterByteMask[2]);
    // Write #4 Filter 3 byte mask
    Lan_SetMacRegDW(dwLanBase, WUFF, pAdapter->Wuff.FilterByteMask[3]);
    SMSC_TRACE1(DBG_POWER, "Filter3 ByteMask = 0x%x\r\n", pAdapter->Wuff.FilterByteMask[3]);
    // Write #5 command
    Lan_SetMacRegDW(dwLanBase, WUFF, pAdapter->Wuff.FilterCommands);
    SMSC_TRACE1(DBG_POWER, "Commands = 0x%x\r\n", pAdapter->Wuff.FilterCommands);
    // Write #6 offset
    Lan_SetMacRegDW(dwLanBase, WUFF, FilterOffsets);
    SMSC_TRACE1(DBG_POWER, "Offsets = 0x%x\r\n", FilterOffsets);
    // Write #7 filter 1 & 0 CRC-16
    Lan_SetMacRegDW(dwLanBase, WUFF, Filter10CRC16);
    SMSC_TRACE1(DBG_POWER, "Filter10CRC16 = 0x%x\r\n", Filter10CRC16);
    // Write #8 filter 3 & 2 CRC-16
    Lan_SetMacRegDW(dwLanBase, WUFF, Filter32CRC16);
    SMSC_TRACE1(DBG_POWER, "Filter32CRC16 = 0x%x\r\n", Filter32CRC16);
}

/*
 * Back to Back Read after Write Delay
 *   See Ch6 Timing Diagrams of Chip manual for details
 * Assume reading BYTE_TEST register takes 45nSec or longer
 */
void B2BWriteDelay(DWORD dwLanBase, DWORD iter)
{
    volatile DWORD dwTemp;

    while (iter > 0)
    {
        dwTemp = GetRegDW(dwLanBase, BYTE_TEST);
        iter --;
    }
}

/*
 * Disable GP Timer, Clear Pending GPT Interrupt and Disable GPT Interrupt
 */
void DisableGPTimer(IN const PSMSC9118_ADAPTER pAdapter)
{
    // Disable the GPT timer
    SetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG, 
        GetRegDW(pAdapter->lan9118_data.dwLanBase, GPT_CFG) & 
        ~GPT_CFG_TIMER_EN_);

    Lan_DisableInterrupt(&pAdapter->lan9118_data, INT_EN_GPT_INT_EN_);

    while (GetRegDW(pAdapter->lan9118_data.dwLanBase, INT_STS) & INT_STS_GPT_INT_)
    {
        Lan_ClearInterruptStatus(&pAdapter->lan9118_data, INT_STS_GPT_INT_);
        B2BWriteDelay(pAdapter->lan9118_data.dwLanBase, 2);
    }
}

/*
 * Reset 118 and Restore Packet Filter / Multicast Address after wakeup
 *
 *  As of CE 6.0, NDIS (or Windows CE) calls MiniportReset(),
 *  MiniportSetInformationHandler(OID_GEN_CURRENT_PACKET_FILTER, ..) and
 *  MiniportSetInformationHandler(OID_802_3_MULTICAST_LIST,..)
 *  after wakeup (Dx -> D0)
 *  This routine is redundant when system wakeup by system but
 *   this will put system into working status after 
 *   forced calls to D1/D2/D3 -> D0 from application 
 */
void ResetAndRestore(IN const PSMSC9118_ADAPTER pAdapter)
{
    NDIS_STATUS StatusToReturn;

    SMSC_TRACE0(DBG_POWER, "+ResetAndRestore()\r\n");

    if (pAdapter != NULL)
    {
        Reset118(pAdapter);
        // We assume pAdapter has previous status before entering sleep
        // ignore return value
        StatusToReturn = SetPacketFilter(pAdapter);
        // ignore return value
        StatusToReturn = SetMulticastAddressList(pAdapter);
    }
    else
    {
        SMSC_WARNING0("Error! ResetAndRestore: pAdapter == NULL\r\n");
    }

    SMSC_TRACE0(DBG_POWER, "-ResetAndRestore()\r\n");
}

/*
 * Decode Registry of ExtraChipIDs
 * Decoded IDs are stored in dwExtraChipIDs[] as Hex
 * Max Extra Chip ID is 10 (see MAX_EXTRA_CHIP_ID)
 */
void DecodeExtraChipId(IN const PSMSC9118_ADAPTER pAdapter, BINARY_DATA *pBD)
{
    LPCTSTR pszStr;
    DWORD   dwID;
    int     i;

    pszStr = pBD->Buffer;
    for (i=0; i<MAX_EXTRA_CHIP_ID; i++)
    {
        if (wcslen(pszStr) == 0)
            break;
        dwID = wcstoul(pszStr, NULL, 16);
        pAdapter->dwExtraChipIDs[i] = dwID;
        SMSC_TRACE1(DBG_INIT, "ID = 0x%08x\r\n", pAdapter->dwExtraChipIDs[i]);
        pszStr+=(wcslen(pszStr)+1);
    }

    if ((i == MAX_EXTRA_CHIP_ID) && (wcslen(pszStr)))
    {
        SMSC_WARNING0("Support upto Max 10 Extra ID\r\n");
    }
}

/*
 * Check chip id from ExtraID of registry key
 */
BOOL CheckExtraID(IN const PSMSC9118_ADAPTER pAdapter, const DWORD IdRev)
{
    int i;

    for (i=0; i<MAX_EXTRA_CHIP_ID; i++)
    {
        if (pAdapter->dwExtraChipIDs[i] == 0x00)
            return FALSE;

        if (IdRev == pAdapter->dwExtraChipIDs[i])
        {
            RETAILMSG(1, (TEXT("ID_REV(0x%08lX) identified from ExtraID\r\n"), IdRev));

            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Read configuration parameters from Registry
 */
NDIS_STATUS ReadConfigFromRegistry(IN const PSMSC9118_ADAPTER pAdapter, IN  NDIS_HANDLE hConfigurationHandle)
{
    NDIS_HANDLE hConfigHandle;
    NDIS_STATUS Status;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
    NDIS_STRING IOAddressStr = IOADDRESS;
    NDIS_STRING IntCfg = INTCFG;
    NDIS_STRING PhyAddressStr = PHYADDRESS;
    NDIS_STRING InterruptStr = INTERRUPT;
    NDIS_STRING RxDMAMode = RXDMAMODE;
    NDIS_STRING TxDMAMode = TXDMAMODE;
    NDIS_STRING LinkMode = LINKMODE;
    NDIS_STRING FlowControl = FLOWCONTROL;
    NDIS_STRING AutoMdix = AMDIX;
    NDIS_STRING ExtraChipIDs = EXTRACHIPID;
    NDIS_STRING IntrDeas = INTRDEAS;
    NDIS_STRING LinkChangeWakeup = MINLINKCHANGEWAKEUP;
    NDIS_STRING MagicPacketWakeup = MINMAGICPACKETWAKEUP;
    NDIS_STRING PatternWakeup = MINPATTERNWAKEUP;

    // Read configuration from registry.
    NdisOpenConfiguration (&Status, &hConfigHandle, hConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE1(DBG_INIT,"  NdisOpenConfiguration failed 0x%x\r\n", Status);
        return NDIS_STATUS_RESOURCES;
    }

    pAdapter->ulMaxLookAhead = (ULONG)MAX_LOOKAHEAD;

    //Read flow control flag.
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &FlowControl,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS) 
    {
        if (ReturnedValue->ParameterData.IntegerData == 1UL) {
            pAdapter->fSwFlowControlEnabled = (BOOLEAN)TRUE;
        }
        else {
            pAdapter->fSwFlowControlEnabled = (BOOLEAN)FALSE;
        }
    }
    else
    {
        pAdapter->fSwFlowControlEnabled = (BOOLEAN)TRUE; //Defaultly set the flow control on.
    }

    //Read the LinkMode
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &LinkMode,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS) {
        pAdapter->LinkMode = (ReturnedValue->ParameterData.IntegerData);
    }
    else {
        pAdapter->LinkMode = LINKMODE_DEFAULT;
    }

   //Read the Auto Mdix Status
   NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &AutoMdix,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS) 
    {
        pAdapter->dwAutoMdix = (ReturnedValue->ParameterData.IntegerData);
    }
    else
    {
        pAdapter->dwAutoMdix = 3UL;
    }

    //Read Rx DMA mode.
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &RxDMAMode,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS) 
    {
        pAdapter->fRxDMAMode = (ReturnedValue->ParameterData.IntegerData);
    }
    else
    {
        pAdapter->fRxDMAMode = 0UL;
    }

    //Read Tx DMA mode.
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &TxDMAMode,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS) 
    {
        pAdapter->fTxDMAMode = (ReturnedValue->ParameterData.IntegerData);
    }
    else
    {
        pAdapter->fTxDMAMode = 0UL;
    }

    //Display Tx/Rx modes
    if (pAdapter->fRxDMAMode)
    {
        RETAILMSG(1, (TEXT("Rx DMA\r\n")));
    }
    else 
    {
        RETAILMSG(1, (TEXT("Rx PIO\r\n")));
    }

    if (pAdapter->fTxDMAMode)
    {
        RETAILMSG(1, (TEXT("Tx DMA\r\n")));
    }
    else
    {
        RETAILMSG(1, (TEXT("Tx PIO\r\n")));
    }

    //Read I/O address of 9118.
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &IOAddressStr,
        NdisParameterHexInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        pAdapter->ulIoBaseAddress= (ReturnedValue->ParameterData.IntegerData);

        NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            hConfigHandle,
            &PhyAddressStr,
            NdisParameterHexInteger);
        
        if (Status == NDIS_STATUS_SUCCESS)
        {
            pAdapter->PhyAddress= (BYTE)(ReturnedValue->ParameterData.IntegerData);
            
            NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                hConfigHandle,
                &InterruptStr,
                NdisParameterInteger);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                pAdapter->ucInterruptNumber = (UINT)(ReturnedValue->ParameterData.IntegerData);
                NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    hConfigHandle,
                    &IntCfg,
                    NdisParameterInteger);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    pAdapter->lan9118_data.dwIntCfg = (DWORD)(ReturnedValue->ParameterData.IntegerData & (INT_CFG_IRQ_POL_|INT_CFG_IRQ_TYPE_));
                }
            }
        }

    }

    // if any of above three "if" are failed..
    if (Status != NDIS_STATUS_SUCCESS)
    {
        SMSC_TRACE1(DBG_INIT,"  NdisReadConfiguration failed 0x%x\r\n", Status);
        return NDIS_STATUS_RESOURCES;
    }

    SMSC_TRACE4(DBG_INIT,"  PhyAddress=0x%lx, IoBaseAddress=0x%lx, Intcfg=0x%lx, Interrupt=0x%lx\r\n", 
                            pAdapter->PhyAddress,
                            pAdapter->ulIoBaseAddress,
                            pAdapter->lan9118_data.dwIntCfg,
                            pAdapter->ucInterruptNumber);

    SMSC_TRACE2(DBG_INIT,"  RxDMAMode=0x%lx, TxDMAMode=0x%lx\r\n", 
                            pAdapter->fRxDMAMode,
                            pAdapter->fTxDMAMode);

    // Read Extra Chip IDs in registry which driver can support 
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &ExtraChipIDs,
        NdisParameterMultiString);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        DecodeExtraChipId(pAdapter, &ReturnedValue->ParameterData.BinaryData);
    }

    // Read Interrupt Deassertion Time
    // Unit : 10uSec
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &IntrDeas,
        NdisParameterInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        // unit: 10uSec
        pAdapter->dIntrDeas = (int)ReturnedValue->ParameterData.IntegerData;
    }
    else
    {
        pAdapter->dIntrDeas = 0;
    }

    // Read Wakeup Capabilities
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &LinkChangeWakeup,
        NdisParameterInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (ReturnedValue->ParameterData.IntegerData == 2)
            pAdapter->WakeUpCap.MinLinkChangeWakeUp = NdisDeviceStateD1;
        else if (ReturnedValue->ParameterData.IntegerData == 3)
            pAdapter->WakeUpCap.MinLinkChangeWakeUp = NdisDeviceStateD2;
        else if (ReturnedValue->ParameterData.IntegerData == 4)
            pAdapter->WakeUpCap.MinLinkChangeWakeUp = NdisDeviceStateD3;
        else
            pAdapter->WakeUpCap.MinLinkChangeWakeUp = NdisDeviceStateD1;
    }
    else
    {
        // Set default state to NdisDeviceStateD1
        pAdapter->WakeUpCap.MinLinkChangeWakeUp = NdisDeviceStateD1;
    }

    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &MagicPacketWakeup,
        NdisParameterInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (ReturnedValue->ParameterData.IntegerData == 2)
            pAdapter->WakeUpCap.MinMagicPacketWakeUp = NdisDeviceStateD1;
        else if (ReturnedValue->ParameterData.IntegerData == 3)
            pAdapter->WakeUpCap.MinMagicPacketWakeUp = NdisDeviceStateD2;
        else if (ReturnedValue->ParameterData.IntegerData == 4)
            pAdapter->WakeUpCap.MinMagicPacketWakeUp = NdisDeviceStateD3;
        else
            pAdapter->WakeUpCap.MinMagicPacketWakeUp = NdisDeviceStateD1;
    }
    else
    {
        // Set default state to WOL (NdisDeviceStateD1)
        pAdapter->WakeUpCap.MinMagicPacketWakeUp = NdisDeviceStateD1;
    }
    NdisReadConfiguration(
        &Status,
        &ReturnedValue,
        hConfigHandle,
        &PatternWakeup,
        NdisParameterInteger);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (ReturnedValue->ParameterData.IntegerData == 2)
            pAdapter->WakeUpCap.MinPatternWakeUp = NdisDeviceStateD1;
        else if (ReturnedValue->ParameterData.IntegerData == 3)
            pAdapter->WakeUpCap.MinPatternWakeUp = NdisDeviceStateD2;
        else if (ReturnedValue->ParameterData.IntegerData == 4)
            pAdapter->WakeUpCap.MinPatternWakeUp = NdisDeviceStateD3;
        else
            pAdapter->WakeUpCap.MinPatternWakeUp = NdisDeviceStateD1;
    }
    else
    {
        // Set default state to NdisDeviceStateD1
        pAdapter->WakeUpCap.MinPatternWakeUp = NdisDeviceStateD1;
    }


    NdisCloseConfiguration (hConfigHandle);

    return NDIS_STATUS_SUCCESS;
}

typedef struct  _SHOW_REG   {
    WCHAR   wszName[20];
    DWORD   dwOffset;
}   SHOW_REG;

static const SHOW_REG   sysCsr[] = {
    { L"ID_REV",        ID_REV      },
    { L"INT_CFG",       INT_CFG     },
    { L"INT_STS",       INT_STS     },
    { L"INT_EN",        INT_EN      },
    { L"DMA_CFG",       DMA_CFG     },
    { L"BYTE_TEST",     BYTE_TEST   },
    { L"FIFO_INT",      FIFO_INT    },
    { L"RX_CFG",        RX_CFG      },
    { L"TX_CFG",        TX_CFG      },
    { L"HW_CFG",        HW_CFG      },
    { L"RX_DP_CTRL",    RX_DP_CTL   },
    { L"RX_FIFO_INF",   RX_FIFO_INF },
    { L"TX_FIFO_INF",   TX_FIFO_INF },
    { L"PMT_CTRL",      PMT_CTRL    },
    { L"GPIO_CFG",      GPIO_CFG    },
    { L"GPT_CFG",       GPT_CFG     },
    { L"GPT_CNT",       GPT_CNT     },
    { L"WORD_SWAP",     WORD_SWAP   },
    { L"FREE_RUN",      FREE_RUN    },
    { L"RX_DROP",       RX_DROP     },
    { L"MAC_CSR_CMD",   MAC_CSR_CMD },
    { L"MAC_CSR_DATA",  MAC_CSR_DATA},
    { L"AFC_CFG",       AFC_CFG     },
    { L"TEST_REG_A",    TEST_REG_A  }
};

static const SHOW_REG macCsr[] = {
    { L"MAC_CR",        MAC_CR      },
    { L"MAC_ADDRH",     ADDRH       },
    { L"MAC_ADDRL",     ADDRL       },
    { L"MAC_HASHH",     HASHH       },
    { L"MAC_HASHL",     HASHL       },
    { L"MAC_MII_ACC",   MII_ACC     },
    { L"MAC_MII_DATA",  MII_DATA    },
    { L"MAC_FLOW",      FLOW        },
    { L"MAC_VLAN1",     VLAN1       },
    { L"MAC_VLAN2",     VLAN2       },
    { L"MAC_WUFF",      WUFF        },
    { L"MAC_WUCSR",     WUCSR       }
};

static const SHOW_REG phyCsr[] = {
    { L"PHY_BCR",       PHY_BCR     },
    { L"PHY_BSR",       PHY_BSR     },
    { L"PHY_ID_1",      PHY_ID_1    },
    { L"PHY_ID_2",      PHY_ID_2    },
    { L"PHY_ANEG_ADV",  PHY_ANEG_ADV},
    { L"PHY_ANEG_LPA",  PHY_ANEG_LPA},
    { L"PHY_ANEG_EXP",  6UL         },
    { L"PHY_SI_REV",    16UL        },
    { L"PHY_MODE_CTRL_STS", PHY_MODE_CTRL_STS   },
    { L"PHY_SPECIAL_MODE",  18UL    },
    { L"PHY_TSTCNTL",       20UL    },
    { L"PHY_TSTREAD1",      21UL    },
    { L"PHY_TSTREAD1",      22UL    },
    { L"PHY_TSTWRITE",      23UL    },
    { L"PHY_CONTROL",       SPECIAL_CTRL_STS},
    { L"PHY_SITC",          28UL    },
    { L"PHY_INT_SRC",       PHY_INT_SRC         },
    { L"PHY_INT_MASK",      PHY_INT_MASK        },
    { L"PHY_SPECIAL",       PHY_SPECIAL         },
};


static void DumpSIMRegs(const SMSC9118_ADAPTER * const pAdapter)
{
    UINT    i;

    RETAILMSG(1, (TEXT("Dump 9118 Slave Interface Module Registers\r\n")));
    for (i=0U;i<(sizeof(sysCsr)/sizeof(SHOW_REG));i++) {
        RETAILMSG(1, (TEXT("%-20s = 0x%08x\r\n"), 
                    sysCsr[i].wszName, 
                    GetRegDW(pAdapter->lan9118_data.dwLanBase, sysCsr[i].dwOffset)));
    }
}

static void DumpMACRegs(const SMSC9118_ADAPTER * const pAdapter)
{
    UINT    i;

    RETAILMSG(1, (TEXT("Dump 9118 MAC Registers\r\n")));
    for (i=0U;i<(sizeof(macCsr)/sizeof(SHOW_REG));i++) {
        RETAILMSG(1, (TEXT("%-20s = 0x%08x\r\n"), 
                    macCsr[i].wszName, 
                    Lan_GetMacRegDW(pAdapter->lan9118_data.dwLanBase, macCsr[i].dwOffset)));
    }
}

static void DumpPHYRegs(const SMSC9118_ADAPTER * const pAdapter)
{
    UINT    i;
    WORD    wValue;

    RETAILMSG(1, (TEXT("Dump 9118 PHY Registers\r\n")));
    for (i=0U;i<(sizeof(phyCsr)/sizeof(SHOW_REG));i++) 
    {
        if (AdapterReadPhy(phyCsr[i].dwOffset, &wValue) == FALSE)
        {
            RETAILMSG(1, (TEXT("Failed to read %s\r\n"), 
                        phyCsr[i].wszName));
        }
        else
        {
            RETAILMSG(1, (TEXT("%-20s = 0x%04x\r\n"), 
                        phyCsr[i].wszName, wValue));
        }
    }
}

static void DumpAllRegs(const SMSC9118_ADAPTER * const pAdapter)
{
    RETAILMSG(1, (TEXT("===================================================================\r\n")));
    RETAILMSG(1, (TEXT("Dump All 9118 Registers\r\n")));
    DumpSIMRegs(pAdapter);
    DumpMACRegs(pAdapter);
    DumpPHYRegs(pAdapter);
#ifdef  TRACE_BUFFER
    DumpStatus(pAdapter);
#endif
    RETAILMSG(1, (TEXT("===================================================================\r\n")));
}

#ifdef  TRACE_BUFFER
void DumpStatus(const SMSC9118_ADAPTER * const pAdapter)
{
    DWORD       dwRdPtr, dwWrPtr;

    RETAILMSG(1, (TEXT("dwRxNumIndicate = %d\r\n"), dwRxNumIndicate));
    RETAILMSG(1, (TEXT("TxReported = %d, TxSent = %d, TxPend = %d\r\n"), dwTxReported, dwTxSent, dwTxPend));
    RETAILMSG(1, (TEXT("%d are in the Defered Queue\r\n"), QUEUE_COUNT(&pAdapter->TxDeferedPkt)));

    RETAILMSG(1, (TEXT("Total Rx Pkt = %d, RxPktToFull = %d, RxPktToEmpty = %d, RxPktFromFull = %d, RxPktFromEmpty = %d, RxDiscard = %d\r\n"),
                dwRxTotalPkt, dwRxPktToFull, dwRxPktToEmpty, dwRxPktFromFull, dwRxPktFromEmpty, dwRxDiscard));
    dwRdPtr = pAdapter->EmptyPkt.dwRdPtr;
    dwWrPtr = pAdapter->EmptyPkt.dwWrPtr;
    RETAILMSG(1, (TEXT("RxPkt In EmptyQueue = %d\r\n"), (dwWrPtr >= dwRdPtr) ? (dwWrPtr - dwRdPtr) : (MAX_RXPACKETS_IN_QUEUE+1 - (dwRdPtr - dwWrPtr))));
    RETAILMSG(1, (TEXT("pAdapter->EmptyPkt.dwRdPtr = %d, pAdapter->EmptyPkt.dwWrPtr = %d\r\n"), pAdapter->EmptyPkt.dwRdPtr, pAdapter->EmptyPkt.dwWrPtr));
    dwRdPtr = pAdapter->FullPkt.dwRdPtr;
    dwWrPtr = pAdapter->FullPkt.dwWrPtr;
    RETAILMSG(1, (TEXT("RxPkt In FullQueue = %d\r\n"), (dwWrPtr >= dwRdPtr) ? (dwWrPtr - dwRdPtr) : (MAX_RXPACKETS_IN_QUEUE+1 - (dwRdPtr - dwWrPtr))));
    RETAILMSG(1, (TEXT("pAdapter->FullPkt.dwRdPtr = %d, pAdapter->FullPkt.dwWrPtr = %d\r\n"), pAdapter->FullPkt.dwRdPtr, pAdapter->FullPkt.dwWrPtr));
}
#endif

