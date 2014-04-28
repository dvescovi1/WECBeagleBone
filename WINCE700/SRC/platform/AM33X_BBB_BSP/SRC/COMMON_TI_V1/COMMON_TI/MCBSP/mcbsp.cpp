// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File: mcsbp.c
//

#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <Pkfuncs.h>
#include <pm.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <memtxapi.h>
#include <initguid.h>
#include <mcbsp.h>
#include <omap_sdma_utility.h>
#include <omap.h>
#include <oal_clock.h>

#include "sdk_padcfg.h"
#include "soc_cfg.h"
#include "debug.h"
#include "mcbspprofile.h"
#include "dataport.h"
#include "mcbsptypes.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

extern void SocMcbspDevConf(McBSPDevice_t* pDevice);

//------------------------------------------------------------------------------
//  DMA settings

static DmaConfigInfo_t s_TxDmaSettings =
{
    // NOTE refer omap_sdma_utility.h file to know each individual struct member info

    DMA_CSDP_DATATYPE_S32,                  // Element width
    0,                                      // Source element Index
    0,                                      // Source frame Index
    DMA_CCR_SRC_AMODE_POST_INC,             // Source addressing mode
    0,                                      // Destination element index
    1024,                                   // Destination frame index
    DMA_CCR_DST_AMODE_CONST,                // Destination addressing mode
    DMA_PRIORITY | DMA_CCR_PREFETCH,        // Dma priority level
    DMA_SYNCH_TRIGGER_DST,                  // Synch Trigger
    DMA_SYNCH_PACKET,                       // Sync Mode
    DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE   // Dma interrupt mask
};


static DmaConfigInfo_t s_RxDmaSettings =
{
    // NOTE refer omap_sdma_utility.h file to know each individual struct member info

    DMA_CSDP_DATATYPE_S32,                  // Element width
    0,                                      // Source element Index
    16,                                     // Source frame Index
    DMA_CCR_SRC_AMODE_CONST,                // Source addressing mode
    0,                                      // Destination element index
    0,                                      // Destination frame index
    DMA_CCR_DST_AMODE_POST_INC,             // Destination addressing mode
    DMA_PRIORITY,                           // Dma priority level
    DMA_SYNCH_TRIGGER_SRC,                  // Synch Trigger
    DMA_SYNCH_PACKET ,                      // Sync Mode
    DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE   // Dma interrupt mask
};

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"McBSPProfile", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, mcbspProfile),
        fieldsize(McBSPDevice_t, mcbspProfile), NULL
    }, {
        L"MemBase", PARAM_MULTIDWORD, TRUE,
        offset(McBSPDevice_t, memBase),
        fieldsize(McBSPDevice_t, memBase), NULL
    }, {
        L"MemLen", PARAM_MULTIDWORD, TRUE,
        offset(McBSPDevice_t, memLen),
        fieldsize(McBSPDevice_t, memLen), NULL
    }, {
        L"DmaTxSyncMap", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, dmaTxSyncMap),
        fieldsize(McBSPDevice_t, dmaTxSyncMap), NULL
    }, {
        L"DmaRxSyncMap", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, dmaRxSyncMap),
        fieldsize(McBSPDevice_t, dmaRxSyncMap), NULL
    }, {
        L"TxPriority", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, priorityDmaTx),
        fieldsize(McBSPDevice_t, priorityDmaTx), (void*)DEFAULT_THREAD_PRIORITY
    }, {
        L"RxPriority", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, priorityDmaRx),
        fieldsize(McBSPDevice_t, priorityDmaRx), (void*)DEFAULT_THREAD_PRIORITY
    }, {
        L"TxBufferSize", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, sizeTxBuffer),
        fieldsize(McBSPDevice_t, sizeTxBuffer), (void*)DEFAULT_DMA_BUFFER_SIZE
    }, {
        L"RxBufferSize", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, sizeRxBuffer),
        fieldsize(McBSPDevice_t, sizeRxBuffer), (void*)DEFAULT_DMA_BUFFER_SIZE
    }, {
        L"UseRegistryForMcbsp", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, useRegistryForMcbsp),
        fieldsize(McBSPDevice_t, useRegistryForMcbsp), NULL
    }, {
        L"LoopbackMode", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, loopbackMode),
        fieldsize(McBSPDevice_t, loopbackMode), (void*)0x0
    }, {
        L"WordsPerFrame", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, wordsPerFrame),
        fieldsize(McBSPDevice_t, wordsPerFrame), (void*)0x1
    }, {
        L"WordLength", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, wordLength),
        fieldsize(McBSPDevice_t, wordLength), (void*)0x20
    }, {
        L"WordLength2", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, wordLength2),
        fieldsize(McBSPDevice_t, wordLength2), (void*)0x20
    }, {
        L"ReverseModeTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, reverseModeTx),
        fieldsize(McBSPDevice_t, reverseModeTx), (void*)0x0
    }, {
        L"DataDelayTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, dataDelayTx),
        fieldsize(McBSPDevice_t, dataDelayTx), (void*)0x1
    }, {
        L"ClockModeTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockModeTx),
        fieldsize(McBSPDevice_t, clockModeTx), (void*)0x0
    }, {
        L"FrameSyncSourceTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, frameSyncSourceTx),
        fieldsize(McBSPDevice_t, frameSyncSourceTx), (void*)0x0
    }, {
        L"PhaseTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, phaseTx),
        fieldsize(McBSPDevice_t, phaseTx), (void*)0x1
    }, {
        L"ClockPolarityTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockPolarityTx),
        fieldsize(McBSPDevice_t, clockPolarityTx), (void*)0x1
    }, {
        L"FrameSyncPolarityTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, frameSyncPolarityTx),
        fieldsize(McBSPDevice_t, frameSyncPolarityTx), (void*)0x1
    }, {
        L"FifoThresholdTx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, fifoThresholdTx),
        fieldsize(McBSPDevice_t, fifoThresholdTx), (void*)0x7F
    }, {
        L"ReverseModeRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, reverseModeRx),
        fieldsize(McBSPDevice_t, reverseModeRx), (void*)0x0
    }, {
        L"DataDelayRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, dataDelayRx),
        fieldsize(McBSPDevice_t, dataDelayRx), (void*)0x1
    }, {
        L"ClockModeRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockModeRx),
        fieldsize(McBSPDevice_t, clockModeRx), (void*)0x0
    }, {
        L"FrameSyncSourceRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, frameSyncSourceRx),
        fieldsize(McBSPDevice_t, frameSyncSourceRx), (void*)0x0
    }, {
        L"PhaseRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, phaseRx),
        fieldsize(McBSPDevice_t, phaseRx), (void*)0x1
    }, {
        L"ClockPolarityRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockPolarityRx),
        fieldsize(McBSPDevice_t, clockPolarityRx), (void*)0x0
    }, {
        L"FrameSyncPolarityRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, frameSyncPolarityRx),
        fieldsize(McBSPDevice_t, frameSyncPolarityRx), (void*)0x0
    }, {
        L"FifoThresholdRx", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, fifoThresholdRx),
        fieldsize(McBSPDevice_t, fifoThresholdRx), (void*)0x7F
    }, {
        L"ClockSourceSrg", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockSourceSRG),
        fieldsize(McBSPDevice_t, clockSourceSRG), (void*)0x4
    }, {
        L"CLKSPinSource", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clksPinSrc),
        fieldsize(McBSPDevice_t, clksPinSrc), (void*)0x0
    }, {
        L"FrameWidthSrg", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, frameWidthSRG),
        fieldsize(McBSPDevice_t, frameWidthSRG), (void*)0x10
    }, {
        L"ClockDividerSrg", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockDividerSRG),
        fieldsize(McBSPDevice_t, clockDividerSRG), (void*)0x0
    }, {
        L"ClockResyncSrg", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, clockResyncSRG),
        fieldsize(McBSPDevice_t, clockResyncSRG), (void*)0x0
    }, {
        L"JustificationMode", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, justificationMode),
        fieldsize(McBSPDevice_t, justificationMode), (void*)0x0
    }, {
        L"TDMWordsPerFrame", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, tdmWordsPerFrame),
        fieldsize(McBSPDevice_t, tdmWordsPerFrame), (void*)0x2
    }, {
        L"PartitionMode", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, partitionMode),
        fieldsize(McBSPDevice_t, partitionMode), NULL
    }, {
        L"NumOfTxChannels", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, numOfTxChannels),
        fieldsize(McBSPDevice_t, numOfTxChannels), NULL
    }, {
        L"EnableMcBSPTxChannels", PARAM_MULTIDWORD, FALSE,
        offset(McBSPDevice_t, requestedTxChannels),
        fieldsize(McBSPDevice_t, requestedTxChannels), NULL
    }, {
        L"NumOfRxChannels", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, numOfRxChannels),
        fieldsize(McBSPDevice_t, numOfRxChannels), NULL
    }, {
        L"EnableMcBSPRxChannels", PARAM_MULTIDWORD, FALSE,
        offset(McBSPDevice_t, requestedRxChannels),
        fieldsize(McBSPDevice_t, requestedRxChannels), NULL
    }, {
        L"SideToneFIRCoeff", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, sideToneFIRCoeffWrite),
        fieldsize(McBSPDevice_t, sideToneFIRCoeffWrite), NULL
    }, {
        L"SideToneGain", PARAM_DWORD, FALSE,
        offset(McBSPDevice_t, sideToneGain),
        fieldsize(McBSPDevice_t, sideToneGain), NULL
    }
};


//------------------------------------------------------------------------------
//
//  Function:  MCP_Init
//
//  Called by device manager to initialize device.
//
EXTERN_C
DWORD
MCP_Init(
    LPCWSTR szContext,
    LPCVOID pBusContext
    )
{
    DWORD rc = 0;
    HRESULT status = ERROR_SUCCESS;
    McBSPDevice_t *pDevice = NULL;
    PHYSICAL_ADDRESS pa;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x, 0x%08x)\r\n", __FUNCTION__,
        szContext, pBusContext
        ));

    // Create device structure
    //
    pDevice = (McBSPDevice_t *)LocalAlloc(LPTR, sizeof(McBSPDevice_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed allocate McBSP controller structure\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    // Set McBSP device structure with 0
    memset(pDevice, 0, sizeof(McBSPDevice_t));

    // Set cookie
    pDevice->cookie = MCBSP_DEVICE_COOKIE;

	// Initialize flags
	pDevice->bRxExitThread = FALSE;
	pDevice->bTxExitThread = FALSE;

    // Read device parameters
    //
    if (GetDeviceRegistryParams(
        szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
        ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed to read McBSP driver registry parameters\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    // Allocate memory to store the configuration information to device
    //
    pDevice->pConfigInfo = new McBSPDeviceConfiguration_t;
    if (pDevice->pConfigInfo == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed memory allocation \r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }
    // Set ConfigInfo structure with 0
    memset(pDevice->pConfigInfo, 0, sizeof(McBSPDeviceConfiguration_t));

    // Open parent bus
    //
    pDevice->hParentBus = CreateBusAccessHandle(szContext);
    if (pDevice->hParentBus == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed open parent bus driver\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    // Device address - Physical & Virtual
    //
    pDevice->pPhysAddrMcBSP = (OMAP35XX_MCBSP_REGS_t*)pDevice->memBase[0];
    pa.QuadPart = pDevice->memBase[0];
    pDevice->pMcbspRegs = (OMAP35XX_MCBSP_REGS_t*)MmMapIoSpace(
        pa, pDevice->memLen[0], FALSE
        );
    if (pDevice->pMcbspRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed map McBSP controller registers\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    // Get the device ID
    //
	pDevice->deviceID = GetDeviceByAddress(pDevice->memBase[0]);
	if (pDevice->deviceID == OMAP_DEVICE_NONE)
	{
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed to get McBSP controller's device ID\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
	}

   
	// Request pads 
    if (!RequestDevicePads(pDevice->deviceID))
    {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"RequestDevicePads failed\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
    }

    // Request clocks (interface & functional) to access the device registers
    //
    //EnableDeviceClocks(pDevice->deviceID, TRUE);
    EnableClocks(pDevice, TRUE);

    // Create the profile based on the setting in the mcbsp registry
    //
    if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave)
        {
        pDevice->pMcbspProfile = new I2SSlaveProfile_t(pDevice);
        }
    else if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Master)
        {
        pDevice->pMcbspProfile = new I2SMasterProfile_t(pDevice);
        }
    else if (pDevice->mcbspProfile == kMcBSPProfile_TDM)
        {
        pDevice->pMcbspProfile = new TDMProfile_t(pDevice);
        }
    if (pDevice->pMcbspProfile == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed memory allocation or Unknown Profile Request\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    // Initialise the Mcbsp profile handler object
    //
    pDevice->pMcbspProfile->Initialize();

	// Perform SOC-specific configuration
	SocMcbspDevConf(pDevice);

    // Allocate & Initialize the device port objects
    //
    pDevice->pTxPort = new DataPort_t(pDevice);
    pDevice->pRxPort = new DataPort_t(pDevice);

    if ((pDevice->pRxPort == NULL) ||
        (pDevice->pTxPort == NULL))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed to allocate rx/tx data port object\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    if (pDevice->pTxPort->Initialize(&s_TxDmaSettings,
        pDevice->sizeTxBuffer,(WORD)pDevice->dmaTxSyncMap, IST_TxDMA
        ) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed to initialize tx port\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    if (pDevice->pRxPort->Initialize(&s_RxDmaSettings,
        pDevice->sizeRxBuffer,(WORD)pDevice->dmaRxSyncMap, IST_RxDMA
        ) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Init: "
            L"Failed to initialize rx port\r\n"
            ));
        status = E_FAIL;
        goto cleanUp;
        }

    pDevice->pTxPort->SetDstPhysAddr(
        (DWORD)pDevice->pPhysAddrMcBSP + offset(OMAP35XX_MCBSP_REGS_t, DXR));

    pDevice->pRxPort->SetSrcPhysAddr(
        (DWORD)pDevice->pPhysAddrMcBSP + offset(OMAP35XX_MCBSP_REGS_t, DRR));

    //EnableDeviceClocks(pDevice->deviceID, FALSE);
    EnableClocks(pDevice, FALSE);
    ASSERT(pDevice->nActivityRefCount == 0);

    // Done...
    rc = (DWORD)pDevice;

cleanUp:
    if (status == E_FAIL)
        {
        MCP_Deinit((DWORD)pDevice);
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(rc = 0x%08x)\r\n", __FUNCTION__, rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  MCP_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
MCP_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    McBSPDevice_t *pDevice = (McBSPDevice_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, context));

    // Check if we get correct context
    if ((pDevice == NULL) ||
        (pDevice->cookie != MCBSP_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Check for open instances
    if (pDevice->instances > 0)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Deinit: "
            L"Deinit with active instance (%d instances active)\r\n",
            pDevice->instances
            ));
        goto cleanUp;
        }

    if (pDevice->pTxPort != NULL)
        {
        delete pDevice->pTxPort;
        }

    if (pDevice->pRxPort != NULL)
        {
        delete pDevice->pRxPort;
        }

    if( pDevice->pMcbspProfile != NULL)
        {
        delete pDevice->pMcbspProfile;
        }

	// Release pads
	ReleaseDevicePads(pDevice->deviceID);

	// Disable Clocks
    //EnableDeviceClocks(pDevice->deviceID, FALSE);
    EnableClocks(pDevice, FALSE);

    // Unmap BSP controller registers
    if (pDevice->pMcbspRegs != NULL)
        {
        MmUnmapIoSpace((VOID*)pDevice->pMcbspRegs, pDevice->memLen[0]);
        }

    // Unmap BSP controller registers
    if (pDevice->pSideToneRegs != NULL)
        {
        MmUnmapIoSpace((VOID*)pDevice->pSideToneRegs, pDevice->memLen[2]);
        }

    // Close parent bus
    if (pDevice->hParentBus != NULL)
        {
        CloseBusAccessHandle(pDevice->hParentBus);
        }

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(rc = 0x%08x)\r\n", __FUNCTION__, rc));
    return rc;
}



//------------------------------------------------------------------------------
//
//  Function:  MCP_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
MCP_Open(
    DWORD context,
    DWORD accessCode,
    DWORD shareMode
    )
{
    DWORD rc = (DWORD)NULL;
    McBSPDevice_t *pDevice = (McBSPDevice_t*)context;
    McBSPInstance_t *pInstance = NULL;

	UNREFERENCED_PARAMETER(shareMode);
	UNREFERENCED_PARAMETER(accessCode);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x,0x%08x, 0x%08x)\r\n",
        __FUNCTION__, context, accessCode, shareMode
        ));

    // Check if we get correct context
    if ((pDevice == NULL) ||
        (pDevice->cookie != MCBSP_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Open: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Create device structure
    pInstance = (McBSPInstance_t*)LocalAlloc(LPTR, sizeof(McBSPInstance_t));
    if (pInstance == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Open: "
            L"Failed allocate BSP instance structure\r\n"
            ));
        goto cleanUp;
        }

    // Set cookie
    memset(pInstance, 0, sizeof(McBSPInstance_t));
    pInstance->cookie = MCBSP_INSTANCE_COOKIE;

    // Save device reference
    pInstance->pDevice = pDevice;

    // Increment number of open instances
    InterlockedIncrement(&pDevice->instances);

    // Sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Done...
    rc = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(rc = 0x%08x)\r\n", __FUNCTION__, rc));
    return rc;
}




//------------------------------------------------------------------------------
//
//  Function:  MCP_Close
//
//  This function closes the device context.
//
BOOL
MCP_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    McBSPDevice_t *pDevice;
    McBSPInstance_t *pInstance = (McBSPInstance_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, context));

    // Check if we get correct context
    if ((pInstance == NULL) ||
        (pInstance->cookie != MCBSP_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_Close: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Get device context
    pDevice = pInstance->pDevice;

    // Sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Decrement number of open instances
    InterlockedDecrement(&pDevice->instances);

    // Free instance structure
    LocalFree(pInstance);

    // Done...
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(rc = 0x%08x)\r\n", __FUNCTION__, rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  MCP_IOControl
//
//  This function sends a command to a device.
//
BOOL
MCP_IOControl(
    DWORD context,
    DWORD code,
    UCHAR *pInBuffer,
    DWORD inSize,
    UCHAR *pOutBuffer,
    DWORD outSize,
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    McBSPInstance_t *pInstance = (McBSPInstance_t*)context;
    McBSPDevice_t  *pDevice = pInstance->pDevice;
    DEVICE_IFC_MCBSP ifc;

    DEBUGMSG(ZONE_FUNCTION, (
        L"MCP:+%S(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        __FUNCTION__, context, code, pInBuffer, inSize, pOutBuffer, outSize,
         pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL) ||
        (pInstance->cookie != MCBSP_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_EXTERNAL_DRVR_REGISTER_TRANSFERCALLBACKS:
            CopyTransferInfo(pInstance,
                             (EXTERNAL_DRVR_DATA_TRANSFER_IN*)pInBuffer,
                             (EXTERNAL_DRVR_DATA_TRANSFER_OUT*)pOutBuffer);
            rc = TRUE;
            break;

        case IOCTL_EXTERNAL_DRVR_UNREGISTER_TRANSFERCALLBACKS:
            ClearTransferInfo(pInstance);
            rc = TRUE;
            break;

        case IOCTL_DDK_GET_DRIVER_IFC:
            // We can give interface only to our peer in device process
            if (GetCurrentProcessId() != (DWORD)GetCallerProcess())
                {
                DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: MCP_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                    ));
                SetLastError(ERROR_ACCESS_DENIED);
                break;
                }

            // Check input parameters
            if ((pInBuffer == NULL) ||
                (inSize < sizeof(GUID)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_MCBSP_GUID))
                {
                if (pOutSize != NULL)
                    {
                    *pOutSize = sizeof(DEVICE_IFC_MCBSP);
                    }
                if ((pOutBuffer == NULL) ||
                    (outSize < sizeof(DEVICE_IFC_MCBSP)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                ifc.context = context;
                ifc.pfnIOctl = MCP_IOControl;

                if (!CeSafeCopyMemory(pOutBuffer, &ifc,
                    sizeof(DEVICE_IFC_MCBSP)
                    ))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }

            SetLastError(ERROR_INVALID_PARAMETER);
            break;

        case IOCTL_CONTEXT_RESTORE:
            if(pDevice != NULL)
                {
                pDevice->pMcbspProfile->ContextRestore();

                pDevice->pTxPort->RestoreDMAcontext(&s_TxDmaSettings,
                    pDevice->sizeTxBuffer, (WORD)pDevice->dmaTxSyncMap
                    );

                pDevice->pRxPort->RestoreDMAcontext(&s_RxDmaSettings,
                    pDevice->sizeRxBuffer,(WORD)pDevice->dmaRxSyncMap
                    );

                pDevice->pTxPort->SetDstPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DXR)
                    );

                pDevice->pRxPort->SetSrcPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DRR)
                    );
                }
            else
                {
                DEBUGMSG( ZONE_ERROR, (L"MCP: ERROR: MCP_IOControl: "
                    L"IOCTL_CONTEXT_RESTORE_NOTIFY Failed\r\n"));
                }
            rc = TRUE;
            break;
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S(rc = 0x%08x)\r\n", __FUNCTION__, rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  CopyTransferInfo
//
//  copies direct data transfer information.
//
void
CopyTransferInfo(
    McBSPInstance_t *pInstance,
    EXTERNAL_DRVR_DATA_TRANSFER_IN *pTransferDataIn,
    EXTERNAL_DRVR_DATA_TRANSFER_OUT *pTransferDataOut
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    pInstance->pTransferCallback = pTransferDataIn->pInData;
    pInstance->fnRxCommand = pTransferDataIn->pfnInRxCommand;
    pInstance->fnRxPopulateBuffer = pTransferDataIn->pfnInRxPopulateBuffer;
    pInstance->fnTxCommand = pTransferDataIn->pfnInTxCommand;
    pInstance->fnTxPopulateBuffer = pTransferDataIn->pfnInTxPopulateBuffer;
    pInstance->fnMutexLock = pTransferDataIn->pfnMutexLock;

    pTransferDataOut->pOutData = pInstance;
    pTransferDataOut->pfnOutRxCommand = RxCommand;
    pTransferDataOut->pfnOutTxCommand = TxCommand;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  ClearTransferInfo
//
//  clear direct data transfer information.
//
void
ClearTransferInfo(
    McBSPInstance_t *pInstance
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    pInstance->pTransferCallback = NULL;
    pInstance->fnRxCommand = NULL;
    pInstance->fnRxPopulateBuffer = NULL;
    pInstance->fnTxCommand = NULL;
    pInstance->fnTxPopulateBuffer = NULL;
    pInstance->fnMutexLock = NULL;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
//  Function:  EnableClocks
//
//  enables/disables the i/f clocks for mcbsps.
//
void
EnableClocks(
    McBSPDevice_t *pDevice,
    BOOL bEnable
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x, %d)\r\n", __FUNCTION__,
        pDevice, bEnable
        ));

    if (bEnable)
        {
        if (pDevice->nActivityRefCount == 0)
            {
            SetDevicePowerState(pDevice->hParentBus, D0, NULL);
            }
        InterlockedIncrement(&pDevice->nActivityRefCount);
        }
    else
        {
        InterlockedDecrement(&pDevice->nActivityRefCount);
        if (pDevice->nActivityRefCount == 0)
            {
            SetDevicePowerState(pDevice->hParentBus, D4, NULL);
            }
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  StartTransmit
//
//  starts transmission.
//
void
StartTransmit(
    McBSPInstance_t *pInstance
    )
{
    INT nActiveStreams = 0;
    DataPort_t *pDataPort = pInstance->pDevice->pTxPort;
    McBSPDevice_t *pDevice = pInstance->pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pInstance));

    pDataPort->Lock();

    // always set the loop counter to maximum loop count on
    // start of transmit.  This is to handle the case when
    // the last page of data is being rendered while another
    // start request is made.  Effectively, prevents the
    // the audio from stopping and continues on with new
    // data stream
    //
    pDataPort->SetMaxLoopCount();

    if (pDataPort->GetState() == kMcBSP_Port_Idle)
        {
        // start rendering from beginning of buffer
        //
        pDataPort->ResetDataBuffer();

        // retrieve 2 pages worth of audio data to render
        //
        pDataPort->SetActiveInstance(pInstance);
        nActiveStreams = pInstance->fnTxPopulateBuffer(
            pDataPort->GetDataBuffer(DataPort_t::kBufferStart),
            pInstance->pTransferCallback,
            pDataPort->GetDataBufferSize()
            );

        // do any preprocessing required if we're in left justification
        // mode
        //
        pDataPort->PreprocessDataForRender(
            DataPort_t::kBufferStart,
            pDataPort->GetSamplesPerPage() * 2
            );

        // enable interface clock
        //
        EnableClocks(pDevice, TRUE);
        if (pDevice->fMcBSPActivity == 0)
            {
            // turn-on SRG (Sample rate generator)
            //
            pDevice->pMcbspProfile->EnableSampleRateGenerator();
            }
        pDevice->fMcBSPActivity |= MCBSP_TX_ACTIVE;

        // Enable transmitter
        //
        pDevice->pMcbspProfile->EnableTransmitter();

        // inform client of activation
        //
        pInstance->fnTxCommand(kExternalDrvrDx_Start,
            pInstance->pTransferCallback, NULL
            );

        // Enable DMA for transmitter
        //
        pDataPort->StartDma(TRUE/*transmit mode*/);
        }

    pDataPort->Unlock();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  StartReceive
//
//  start receiving data.
//
void
StartReceive(
    McBSPInstance_t *pInstance
    )
{
    DataPort_t *pDataPort = pInstance->pDevice->pRxPort;
    McBSPDevice_t *pDevice = pInstance->pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pInstance));

    pDataPort->Lock();

    pDataPort->SetMaxLoopCount();

    if (pDataPort->GetState() == kMcBSP_Port_Idle)
        {
        // start recording from beginning of buffer
        //
        pDataPort->ResetDataBuffer();

        // enable interface clock
        //
        EnableClocks(pDevice, TRUE);
        pDataPort->SetActiveInstance(pInstance);
        if (pDevice->fMcBSPActivity == 0)
            {
            // turn-on SRG (Sample rate generator)
            pDevice->pMcbspProfile->EnableSampleRateGenerator();
            }
        pDevice->fMcBSPActivity |= MCBSP_RX_ACTIVE;

        // Enable recevier
        //
        pDevice->pMcbspProfile->EnableReceiver();

        // inform client of activation
        //
        pInstance->fnRxCommand(kExternalDrvrDx_Start,
            pInstance->pTransferCallback, NULL
            );

        // Enable dma for receiver
        //
        pDataPort->StartDma(FALSE/*receiver mode*/);
        }

    pDataPort->Unlock();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  StopTransmit
//
//  stops transmission.
//
void
StopTransmit(
    McBSPDevice_t *pDevice
    )
{
    DataPort_t *pDataPort = pDevice->pTxPort;
    McBSPInstance_t *pInstance = pDataPort->GetActiveInstance();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    pDataPort->Lock();

    if (pDataPort->GetState() == kMcBSP_Port_Active)
        {
        // Stop DMA
        //
        pDataPort->StopDma();
        pDevice->pMcbspProfile->ResetTransmitter();

        // Clear McBSP IRQ Status register
        //
        pDevice->pMcbspProfile->ClearIRQStatus();

        // if no activity then turn-off power
        //
        pDevice->fMcBSPActivity &= ~MCBSP_TX_ACTIVE;
        if (pDevice->fMcBSPActivity == 0)
            {
            // turn-off SRG (Sample rate generator)
            //pDevice->pMcbspProfile->ResetSampleRateGenerator();
            }

        pDataPort->SetActiveInstance(NULL);

        // disable interface clock
        EnableClocks(pDevice, FALSE);

        // inform client of state change
        //
        pInstance->fnTxCommand(kExternalDrvrDx_Stop,
            pInstance->pTransferCallback, NULL
            );
        }

    pDataPort->Unlock();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  StopReceive
//
//  stop receiving data.
//
void
StopReceive(
    McBSPDevice_t *pDevice
    )
{
    DataPort_t *pDataPort = pDevice->pRxPort;
    McBSPInstance_t *pInstance = pDataPort->GetActiveInstance();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    pDataPort->Lock();

    if (pDataPort->GetState() == kMcBSP_Port_Active)
        {
        // Stop DMA
        //
        pDataPort->StopDma();
        pDevice->pMcbspProfile->ResetReceiver();

        // Clear McBSP IRQ Status register
        //
        pDevice->pMcbspProfile->ClearIRQStatus();

        // if no activity then turn-off power
        //
        pDevice->fMcBSPActivity &= ~MCBSP_RX_ACTIVE;
        if (pDevice->fMcBSPActivity == 0)
            {
            // turn-off SRG (Sample rate generator)
            //pDevice->pMcbspProfile->ResetSampleRateGenerator();
            }

        pDataPort->SetActiveInstance(NULL);

        // disable interface clock
        //
        EnableClocks(pDevice, FALSE);

        // inform client of state change
        //
        pInstance->fnRxCommand(kExternalDrvrDx_Stop,
            pInstance->pTransferCallback, NULL
            );
        }
    pDataPort->Unlock();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
//  Function:  TxCommand
//
//  handle tx commands.
//
int
TxCommand(
    ExternalDrvrCommand cmd,
    void* pData,
    PortConfigInfo_t* pPortConfigInfo
    )
{
    McBSPInstance_t *pInstance = (McBSPInstance_t*)pData;
    McBSPDevice_t *pDevice = pInstance->pDevice;
    UINT nCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    if (pPortConfigInfo != NULL)
        {
        // Populate the requested configuration information receieved from
        // mcbsp client
        //
        pDevice->mcbspProfile = pPortConfigInfo->portProfile;
        pDevice->numOfTxChannels = pPortConfigInfo->numOfChannels;
        for (nCount = 0; nCount < pDevice->numOfTxChannels; nCount++)
            {
            pDevice->requestedTxChannels[nCount] =
                pPortConfigInfo->requestedChannels[nCount];
            }
        }

    switch (cmd)
        {
        case kExternalDrvrDx_Start:
            DEBUGMSG(ZONE_INFO, (L"MCP: TxCommand - Starting transmit\r\n"));

            if ((pDevice->pMcbspProfile != NULL) &&
                ((DWORD)pDevice->pMcbspProfile->GetMode() != pDevice->mcbspProfile))
                {
                // Request clocks (interface & functional) to access the
                // device registers
                //
                EnableClocks(pDevice, TRUE);

                delete pDevice->pMcbspProfile;
                if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave)
                    {
                    pDevice->pMcbspProfile = new I2SSlaveProfile_t(pDevice);
                    }
                else if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Master)
                    {
                    pDevice->pMcbspProfile = new I2SMasterProfile_t(pDevice);
                    }
                else if (pDevice->mcbspProfile == kMcBSPProfile_TDM)
                    {
                    pDevice->pMcbspProfile = new TDMProfile_t(pDevice);
                    }
                if (pDevice->pMcbspProfile == NULL)
                    {
                    DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: TxCommand: "
                        L"Failed memory allocation or Unknown Profile Request\r\n"
                        ));
                    break;
                    }

                // Initialise the Mcbsp profile handler object
                //
                pDevice->pMcbspProfile->Initialize();

                if ((pDevice->mcbspProfile == kMcBSPProfile_TDM) &&
                    (pPortConfigInfo != NULL))
                    {
                    pDevice->pMcbspProfile->SetTxChannelsRequested();
                    }

                // Restore DMA contexts
                //
                pDevice->pTxPort->RestoreDMAcontext(&s_TxDmaSettings,
                    pDevice->sizeTxBuffer, (WORD)pDevice->dmaTxSyncMap
                    );

                pDevice->pRxPort->RestoreDMAcontext(&s_RxDmaSettings,
                    pDevice->sizeRxBuffer,(WORD)pDevice->dmaRxSyncMap
                    );

                pDevice->pTxPort->SetDstPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DXR)
                    );

                pDevice->pRxPort->SetSrcPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DRR)
                    );

                EnableClocks(pDevice, FALSE);
                }

            StartTransmit(pInstance);

            break;

        case kExternalDrvrDx_Stop:
            DEBUGMSG(ZONE_INFO, (L"MCP: TxCommand - Stopping transmit\r\n"));

            pDevice->pTxPort->Lock();
            if (pDevice->pTxPort->GetState() == kMcBSP_Port_Active)
                {
                pDevice->pTxPort->RequestDmaStop();
                }
            pDevice->pTxPort->Unlock();
            break;

        case kExternalDrvrDx_ImmediateStop:
            DEBUGMSG(ZONE_INFO, (L"MCP: TxCommand - Abort transmit\r\n"));

            pDevice->pTxPort->Lock();
            if (pDevice->pTxPort->GetState() == kMcBSP_Port_Active)
                {
                StopTransmit(pDevice);
                }
            pDevice->pTxPort->Unlock();
            break;

        case kExternalDrvrDx_Reconfig:
            DEBUGMSG(ZONE_INFO, (L"MCP: TxCommand - McBSP reconfig\r\n"));

            pDevice->pTxPort->Lock();

            // Reconfigure the McBSP registers
            //
            pDevice->pMcbspProfile->Initialize();

            // Note: Need to check if we need to re-initialize the dataports

            pDevice->pTxPort->Unlock();
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return 1;
}


//------------------------------------------------------------------------------
//
//  Function:  RxCommand
//
//  handle rx messages.
//
int
RxCommand(
    ExternalDrvrCommand cmd,
    void* pData,
    PortConfigInfo_t* pPortConfigInfo
    )
{
    McBSPInstance_t *pInstance = (McBSPInstance_t*)pData;
    McBSPDevice_t *pDevice = pInstance->pDevice;
    UINT nCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S\r\n", __FUNCTION__));

    if (pPortConfigInfo != NULL)
        {
        // Populate the requested configuration information receieved from
        // mcbsp client
        //
        pDevice->mcbspProfile = pPortConfigInfo->portProfile;
        pDevice->numOfRxChannels = pPortConfigInfo->numOfChannels;
        for (nCount = 0; nCount < pDevice->numOfRxChannels; nCount++)
            {
            pDevice->requestedRxChannels[nCount] =
                pPortConfigInfo->requestedChannels[nCount];
            }
        }

    switch (cmd)
        {
        case kExternalDrvrDx_Start:
            DEBUGMSG(ZONE_INFO, (L"MCP: RxCommand - Starting Receive\r\n"));

            if ((pDevice->pMcbspProfile != NULL) &&
                ((DWORD)pDevice->pMcbspProfile->GetMode() != pDevice->mcbspProfile))
                {
                // Request clocks (interface & functional) to access the
                // device registers
                //
                EnableClocks(pDevice, TRUE);

                delete pDevice->pMcbspProfile;
                if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Slave)
                    {
                    pDevice->pMcbspProfile = new I2SSlaveProfile_t(pDevice);
                    }
                else if (pDevice->mcbspProfile == kMcBSPProfile_I2S_Master)
                    {
                    pDevice->pMcbspProfile = new I2SMasterProfile_t(pDevice);
                    }
                else if (pDevice->mcbspProfile == kMcBSPProfile_TDM)
                    {
                    pDevice->pMcbspProfile = new TDMProfile_t(pDevice);
                    }
                if (pDevice->pMcbspProfile == NULL)
                    {
                    DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: RxCommand: "
                        L"Failed memory  or Unknown Profile Request\r\n"
                        ));
                    break;
                    }

                // Initialise the Mcbsp profile handler object
                //
                pDevice->pMcbspProfile->Initialize();

                if ((pDevice->mcbspProfile == kMcBSPProfile_TDM) &&
                    (pPortConfigInfo != NULL))
                    {
                    pDevice->pMcbspProfile->SetRxChannelsRequested();
                    }

                // Restore DMA contexts
                //
                pDevice->pTxPort->RestoreDMAcontext(&s_TxDmaSettings,
                    pDevice->sizeTxBuffer, (WORD)pDevice->dmaTxSyncMap
                    );

                pDevice->pRxPort->RestoreDMAcontext(&s_RxDmaSettings,
                    pDevice->sizeRxBuffer,(WORD)pDevice->dmaRxSyncMap
                    );

                pDevice->pTxPort->SetDstPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DXR)
                    );

                pDevice->pRxPort->SetSrcPhysAddr(
                    (DWORD)pDevice->pPhysAddrMcBSP +
                    offset(OMAP35XX_MCBSP_REGS_t, DRR)
                    );

                EnableClocks(pDevice, FALSE);
                }

            StartReceive(pInstance);

            break;

        case kExternalDrvrDx_Stop:
            DEBUGMSG(ZONE_INFO, (L"MCP: RxCommand - Stopping Receive\r\n"));

            pDevice->pRxPort->Lock();
            if (pDevice->pRxPort->GetState() == kMcBSP_Port_Active)
                {
                pDevice->pRxPort->RequestDmaStop();
                }
            pDevice->pRxPort->Unlock();
            break;

        case kExternalDrvrDx_ImmediateStop:
            DEBUGMSG(ZONE_INFO, (L"MCP: RxCommand - Abort Receive\r\n"));

            pDevice->pRxPort->Lock();
            if (pDevice->pRxPort->GetState() == kMcBSP_Port_Active)
                {
                StopReceive(pDevice);
                }
            pDevice->pRxPort->Unlock();
            break;

        case kExternalDrvrDx_Reconfig:
            DEBUGMSG(ZONE_INFO, (L"MCP: RxCommand - McBSP reconfig\r\n"));

            pDevice->pRxPort->Lock();

            // Reconfigure the McBSP registers
            //
            pDevice->pMcbspProfile->Initialize();

            // Note: Need to check if we need to re-initialize the dataports

            pDevice->pRxPort->Unlock();
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  IST_TxDMA
//
//  Receive DMA handler.
//
DWORD
IST_TxDMA(
    void* pParam
    )
{
    DWORD status = 0;
    McBSPInstance_t* pInstance = NULL;
    McBSPDevice_t* pDevice = (McBSPDevice_t*)pParam;
    DataPort_t *pDataPort = pDevice->pTxPort;
    DmaDataInfo_t *pDMAInfo = pDataPort->GetDmaInfo();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

#if (UNDER_CE < 600)
    SetProcPermissions((ULONG)-1);
#endif
    CeSetThreadPriority(GetCurrentThread(), pDevice->priorityDmaTx);

	for(;;)
        {
        if (WaitForSingleObject(pDataPort->m_hEvent, INFINITE) == WAIT_OBJECT_0)
            {

			if (pDevice->bTxExitThread)
			{
				break;
			}

            // check active buffer
            //
            status = DmaGetStatus(pDMAInfo);

            DEBUGMSG(ZONE_IST,
                (L"MCP: Transmit DMA Interrupt(status=0x%04x)\r\n", status)
                );

            DmaClearStatus(pDMAInfo, status);

            // clear interrupt
            //
            DmaInterruptDone(pDataPort->m_hDmaChannel);

            pInstance = pDataPort->GetActiveInstance();
            if (pInstance == NULL)
                {
                StopTransmit(pDevice);
                continue;
                }

            __try
                {
                DEBUGMSG(ZONE_IST, (L"MCP: DMA Tx IST loopCounter=%d\r\n",
                    pDataPort->GetLoopCount())
                    );

                pInstance->fnMutexLock(TRUE, INFINITE,
                    pInstance->pTransferCallback
                    );

                pDataPort->Lock();

                if (pDataPort->DecrementLoopCount() <= 0)
                    {
                    StopTransmit(pDevice);
                    }
                else if (status & (DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE))
                    {
                    // Clear McBSP IRQ Status register
                    //
                    pDevice->pMcbspProfile->ClearIRQStatus();

                    //  swap buffer
                    //
                    pDataPort->SwapBuffer(TRUE /*tx mode*/);

                    // populate the non-active buffer
                    //
                    pInstance->fnTxPopulateBuffer(
                        pDataPort->GetDataBuffer(DataPort_t::kBufferInactive),
                        pInstance->pTransferCallback,
                        ((pDataPort->GetDataBufferSize()) >> 1)
                        );

                    // perform preprocessing
                    //
                    pDataPort->PreprocessDataForRender(
                        DataPort_t::kBufferInactive,
                        pDataPort->GetSamplesPerPage()
                        );
                    }

                pDataPort->Unlock();
                pInstance->fnMutexLock(FALSE, INFINITE,
                    pInstance->pTransferCallback
                    );

                DEBUGMSG(ZONE_IST, (L"MCP: DMA Tx IST interrupt done\r\n"));

                }
            __except(EXCEPTION_EXECUTE_HANDLER)
                {
                DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR - exception in TxIST\r\n"));
                }
            }
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return 0;
}


//------------------------------------------------------------------------------
//
//  Function:  IST_RxDMA
//
//  Receive DMA handler.
//
DWORD
IST_RxDMA(
    void* pParam
    )
{
    DWORD status = 0;
    McBSPInstance_t* pInstance = NULL;
    McBSPDevice_t* pDevice = (McBSPDevice_t*)pParam;
    DataPort_t *pDataPort = pDevice->pRxPort;
    DmaDataInfo_t *pDMAInfo = pDataPort->GetDmaInfo();

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

#if (UNDER_CE < 600)
    SetProcPermissions((ULONG)-1);
#endif
    CeSetThreadPriority(GetCurrentThread(), pDevice->priorityDmaRx);

    for(;;)
    {
        if (WaitForSingleObject(pDataPort->m_hEvent, INFINITE) == WAIT_OBJECT_0)
        {

		if (pDevice->bRxExitThread)
		{
			break;
		}

        // check active buffer
        //
        status = DmaGetStatus(pDMAInfo);

        DEBUGMSG(ZONE_IST,
            (L"MCP:Receive DMA Interrupt(status=0x%04x)\r\n", status)
            );

        DmaClearStatus(pDMAInfo, status);

        // clear interrupt
        //
        DmaInterruptDone(pDataPort->m_hDmaChannel);

        pInstance = pDataPort->GetActiveInstance();
        if (pInstance == NULL)
            {
            StopReceive(pDevice);
            continue;
            }

        __try
            {
            DEBUGMSG(ZONE_IST, (L"MCP: DMA Rx IST loopCounter=%d\r\n",
                pDataPort->GetLoopCount())
                );

            pInstance->fnMutexLock(TRUE, INFINITE,
                pInstance->pTransferCallback);

            pDataPort->Lock();

            if (pDataPort->DecrementLoopCount() <= 0)
                {
                StopReceive(pDevice);
                }
            else if (status & (DMA_CICR_BLOCK_IE | DMA_CICR_FRAME_IE))
                {
                // Clear McBSP IRQ Status register
                //
                pDevice->pMcbspProfile->ClearIRQStatus();

                //  swap buffer
                //
                pDataPort->SwapBuffer(FALSE /*rx mode*/);

                // perform preprocessing
                //
                pDataPort->PostprocessDataForCapture(
                    DataPort_t::kBufferInactive,
                    pDataPort->GetSamplesPerPage());

                // populate the non-active buffer
                //
                pInstance->fnRxPopulateBuffer(
                    pDataPort->GetDataBuffer(DataPort_t::kBufferInactive),
                    pInstance->pTransferCallback,
                    ((pDataPort->GetDataBufferSize()) >> 1)
                    );

                }

            pDataPort->Unlock();

            pInstance->fnMutexLock(FALSE, INFINITE,
                pInstance->pTransferCallback
                );

            DEBUGMSG(ZONE_IST, (L"MCP: DMA Rx IST interrupt done\r\n"));

            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR - exception in RxIST\r\n"));
            }
        }
    }


    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));

    return 0;
}

