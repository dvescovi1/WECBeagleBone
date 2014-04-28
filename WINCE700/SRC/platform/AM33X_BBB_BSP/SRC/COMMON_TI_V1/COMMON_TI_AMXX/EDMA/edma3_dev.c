//
//   Copyright (c) MPC Data Limited 2009. All Rights Reserved.
//

#include <windows.h>
#include <oal_log.h>
#include <ceddk.h>
#include <devload.h>
#include <nkintr.h>
#include <windev.h>
#include "am389x_edma.h"
//#include <psc.h>
#include "edma3_dev.h"
//#include <oal_prcm.h>

#define EDMA_ALWAYS_ON 

//------------------------------------------------------------------------------
//  Set debug zones names and initial setting for the EDMA driver. 
//
#ifdef DEBUG

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)
#define ZONE_IOCTL          DEBUGZONE(6)
#define ZONE_VERBOSE        DEBUGZONE(7)

DBGPARAM dpCurSettings = {
    L"EDMA Driver", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"IST",         L"IOCTL",       L"Verbose",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#else
#define ZONE_ERROR          1
#define ZONE_WARN           1
#define ZONE_FUNCTION       0
#define ZONE_INIT           0
#define ZONE_INFO           0
#define ZONE_IST            0
#define ZONE_IOCTL          0
#define ZONE_VERBOSE        0
#endif

//------------------------------------------------------------------------------
//  Globals
extern const unsigned int g_EDMA3_NUM_INSTANCES;

// Driver context
static EDMA_CONTEXT *g_pEDMACtx[EDMA3_MAX_EDMA3_INSTANCES] = {NULL};

// Table of TC error interrupt events
EDMA_TC_ERROR_EVENT g_EDMATccErrIntEvt[EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_TC] = {
    {
        {0, 0},     // Instance 0, TC 0
        {0, 1},     // Instance 0, TC 1
        {0, 2},     // Instance 0, TC 2
        {0, 3}      // Instance 0, TC 3
    }
};

// Table of TC error interrupt handlers
typedef void (*Edma3TCErrHandler)(unsigned int arg); 
static Edma3TCErrHandler lisrEdma3TCErrHandler[8] = {
    lisrEdma3TC0ErrHandler0,
    lisrEdma3TC1ErrHandler0,
    lisrEdma3TC2ErrHandler0,
    lisrEdma3TC3ErrHandler0,
    lisrEdma3TC4ErrHandler0,
    lisrEdma3TC5ErrHandler0,
    lisrEdma3TC6ErrHandler0,
    lisrEdma3TC7ErrHandler0
}; 

// Channel to TCC mapping
extern EDMA3_DRV_ChBoundResources edma3DrvChBoundRes [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_LOGICAL_CH];

// External Global Configuration Structure
extern EDMA3_DRV_GblConfigParams platformEdma3GblCfgParams[EDMA3_MAX_EDMA3_INSTANCES];

// External Instance Specific Configuration Structure
extern EDMA3_DRV_InstanceInitConfig platformInstInitConfig[EDMA3_MAX_EDMA3_INSTANCES][8];


//------------------------------------------------------------------------------
//  Local Functions

static BOOL EDMAInitHardware(unsigned int instanceId);
static BOOL EDMAInitEventTables(unsigned int instanceId);
static BOOL EDMAInitInterrupts(unsigned int instanceId);
static BOOL EDMAInitDriver(unsigned int instanceId);
static void EDMACleanup(unsigned int instanceId);

static void EDMAIstTransferComplete(PVOID pContext);
static void EDMAIstCCError(PVOID pContext);
static void EDMAIstTCError(PVOID pContext);

static unsigned int EDMAFindChannelForTcc(unsigned int instanceId, unsigned int tcc);
static BOOL EDMACheckForChannelError(unsigned int instanceId, unsigned int edmaChannel);
static unsigned int EDMAHandleToInstance(EDMA3_DRV_Handle hEDMA);


void edma3TccCallback(unsigned int tcc, EDMA3_RM_TccStatus status, void *appData);
void edma3GblErrCallback(EDMA3_RM_GlobalError deviceStatus, unsigned int instanceId, void *gblerrData);


//------------------------------------------------------------------------------
//
//  Function:  DllEntry
//
//  Standard Windows DLL entry point.
//
BOOL WINAPI DllEntry(HANDLE hInstance, DWORD dwReason, VOID *pReserved)
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER((HMODULE)hInstance);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        PRINTMSG(ZONE_INIT, (TEXT("EDMA: DLL_PROCESS_ATTACH\r\n")));
        break;

    case DLL_PROCESS_DETACH:
        PRINTMSG(ZONE_INIT, (TEXT("EDMA: DLL_PROCESS_DETACH\r\n")));
        break;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  EDM_Init
//
DWORD EDM_Init(LPCTSTR szContext, LPCVOID pBusContext)
{
    BOOL rc = FALSE;
    HKEY hKey;
    DWORD dwValueType;
    DWORD dwDataSize;
    unsigned int instanceId, tcNum;

    PRINTMSG(ZONE_INIT, (TEXT("EDMA: +EDM_Init\r\n")));

    for (instanceId = 0; instanceId < g_EDMA3_NUM_INSTANCES; instanceId++)
    {
        g_pEDMACtx[instanceId] = (EDMA_CONTEXT*)LocalAlloc(LPTR, sizeof(EDMA_CONTEXT));
        if (g_pEDMACtx[instanceId] == NULL)
        {
            ERRORMSG(TRUE, (L"EDMA: EDM_Init: Failed allocate EDMA context structure\r\n"));
            goto cleanUp;
        }

        InitializeCriticalSection(&g_pEDMACtx[instanceId]->csEdma);

        // Store global configuration data
        g_pEDMACtx[instanceId]->pGlobalConfig = &platformEdma3GblCfgParams[instanceId];
        g_pEDMACtx[instanceId]->pInstanceConfig = &platformInstInitConfig[instanceId][0];

        g_pEDMACtx[instanceId]->dwEDMACCSysIntr     = (DWORD)SYSINTR_UNDEFINED;
        g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr  = (DWORD)SYSINTR_UNDEFINED;
        for (tcNum = 0; tcNum < platformEdma3GblCfgParams[instanceId].numTcs; tcNum++)
        {
            g_pEDMACtx[instanceId]->dwEDMATCErrSysIntr[tcNum]  = (DWORD)SYSINTR_UNDEFINED;
        }

        g_pEDMACtx[instanceId]->intrThreadExit = FALSE;

        // Read registry settings
        hKey = OpenDeviceKey((LPCTSTR)szContext);
        if (!hKey) 
        {
            DEBUGMSG(ZONE_ERROR, (L"EDMA: ERROR: Failed to open registry key\r\n"));
            goto cleanUp;
        }

        dwDataSize = sizeof(g_pEDMACtx[instanceId]->dwEDMAIstThreadPriority);
        if (RegQueryValueEx(hKey, L"Priority256", NULL, &dwValueType,
                        (LPBYTE)&g_pEDMACtx[instanceId]->dwEDMAIstThreadPriority, &dwDataSize))
        {
            DEBUGMSG(ZONE_WARN,
                 (L"EDMA: WARNING: Failed to read thread priority from registry\r\n"));
            g_pEDMACtx[instanceId]->dwEDMAIstThreadPriority = 70; // Default value
        }
        RegCloseKey(hKey);
    
        rc = EDMAInitHardware(instanceId);
        if (!rc)
            goto cleanUp;

        rc = EDMAInitEventTables(instanceId);
        if (!rc)
            goto cleanUp;

        rc = EDMAInitInterrupts(instanceId);
        if (!rc)
            goto cleanUp;

        rc = EDMAInitDriver(instanceId);
        if (!rc)
            goto cleanUp;
    }
    
    rc = TRUE;

cleanUp:
    if (!rc)
    {
        EDMACleanup(instanceId);
    }

    PRINTMSG(ZONE_INIT, (TEXT("EDMA: -EDM_Init: Returning 0x%X\r\n"), g_pEDMACtx));
    return (DWORD)g_pEDMACtx;
}

//------------------------------------------------------------------------------
//
//  Function:  EDM_Deinit
//
BOOL EDM_Deinit(DWORD context)
{
    unsigned int instanceId;

    for (instanceId = 0; instanceId < g_EDMA3_NUM_INSTANCES; instanceId++)
    {
        EDMACleanup(instanceId);
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  EDM_Open
//
DWORD EDM_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{
    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  EDM_Close
//
BOOL EDM_Close(DWORD context)
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  EDM_IOControl
//
BOOL EDM_IOControl(
    DWORD context, DWORD dwCode,
    BYTE *pInBuffer, DWORD inSize,
    BYTE *pOutBuffer, DWORD outSize, DWORD *pOutSize)
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;
    BOOL   rc = FALSE;

    PRINTMSG(ZONE_IOCTL,
             (L"EDMA: +EDM_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
              context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize));

    // No IOCTLs supported
    PRINTMSG(ZONE_IOCTL, (L"EDMA: EDM_IOControl: Unsupported IOCTL code 0x%08x\r\n", dwCode));
    dwError = ERROR_NOT_SUPPORTED;
    SetLastError(dwError);

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAInitHardware
//
BOOL EnableDeviceClocks(UINT	devID, BOOL bEnable);

static BOOL EDMAInitHardware(unsigned int instanceId)
{
    BOOL rc = FALSE;
    int i;
    PHYSICAL_ADDRESS physicalAddress;
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    
    pGlobalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;
    
#ifdef EDMA_ALWAYS_ON
	EnableDeviceClocks(pGlobalConfig->devId_CC, TRUE);

	for (i=0; i<(int)pGlobalConfig->numTcs; i++)
		EnableDeviceClocks(pGlobalConfig->devId_Tc[i], TRUE);
#endif

    // Map channel controller register space
    physicalAddress.HighPart = 0;
    physicalAddress.LowPart  = (DWORD)pGlobalConfig->globalRegs;
    pGlobalConfig->globalRegs = (PEDMACCREGS)MmMapIoSpace(
        physicalAddress, sizeof(AM389X_EDMACC_REGS), FALSE);
    if (!pGlobalConfig->globalRegs)
    {
        ERRORMSG(TRUE, (_T("EDMA: EDMAInitHardware: Unable to map memory region.\r\n")));
        goto done;
    }

    PRINTMSG(ZONE_INIT,(_T("EDMA: EDMAInitHardware: CCREGS Phys 0x%X Mapped to 0x%X Size 0x%X\r\n"),
                        physicalAddress.LowPart, pGlobalConfig->globalRegs,
                        sizeof (AM389X_EDMACC_REGS) ));

    PRINTMSG(ZONE_INIT,(_T("EDMA: PID 0x%X \r\n"), 
                         ((PEDMACCREGS)pGlobalConfig->globalRegs)->REV
             ));

    // Map transfer controller register space
    for (i = 0; i < (int)pGlobalConfig->numTcs; ++i)
    {
        physicalAddress.HighPart = 0;
        physicalAddress.LowPart = (DWORD)pGlobalConfig->tcRegs[i];
        pGlobalConfig->tcRegs[i] = (PEDMATCREGS)MmMapIoSpace(
            physicalAddress, sizeof(AM389X_EDMATC_REGS), FALSE);
        if (!pGlobalConfig->tcRegs[i])
        {
            ERRORMSG(TRUE,(_T("EDMA: EDMAInitHardware: Unable to map TC%d memory region.\r\n"), i));
            goto done;
        }

        PRINTMSG(ZONE_INIT,(_T("EDMA: EDMAInitHardware: TC%d REGS Phys 0x%X Mapped to 0x%X Size 0x%X\r\n"),
                            i, physicalAddress.LowPart, pGlobalConfig->tcRegs[i],
                            sizeof(AM389X_EDMATC_REGS)));
    }

    rc = TRUE;

done:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  EDMAInitEventTables
//
static BOOL EDMAInitEventTables(unsigned int instanceId)
{
    BOOL rc = TRUE;
    TCHAR szEventName[EDMA_MAX_EVENT_NAME_LEN];
    int i;
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    
    pGlobalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;
    
    ZeroMemory(g_pEDMACtx[instanceId]->DMAEventTable, sizeof(EDMA_DMA_EVENT) * EDMA3_MAX_DMA_CH);
    ZeroMemory(g_pEDMACtx[instanceId]->QDMAEventTable, sizeof(EDMA_DMA_EVENT) * EDMA3_MAX_QDMA_CH);
    ZeroMemory(g_pEDMACtx[instanceId]->CCEventTable, sizeof(EDMA_CC_ERROR_EVENT) * EDMA3_NUM_CCERRORS);

    for (i = 0; rc == TRUE && i < (int)pGlobalConfig->numDmaChannels; i++)
    {
        /* Create the Unique Event Name First */
        _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"), EDMA_EVENT_NAME, instanceId, i);

        /* Create the Event Object and Initialize the Event Tbl Member */
        g_pEDMACtx[instanceId]->DMAEventTable[i].hEvent = CreateEvent(NULL, FALSE, FALSE, szEventName);
        if (g_pEDMACtx[instanceId]->DMAEventTable[i].hEvent == NULL)
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitEventTables: EventObject Create Failed Index 0x%x\n"), i));
            rc = FALSE;
            break;
        }
        PRINTMSG(ZONE_VERBOSE, (TEXT("EDMA: EDMAInitEventTables: %s Transfer Complete Handle 0x%X\n"),
                                szEventName, g_pEDMACtx[instanceId]->DMAEventTable[i].hEvent));

        /* Create the Event Object for errors */
        _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"), EDMA_ERROR_NAME, instanceId, i);
        g_pEDMACtx[instanceId]->DMAEventTable[i].hError = CreateEvent(NULL, FALSE, FALSE, szEventName);
        if (g_pEDMACtx[instanceId]->DMAEventTable[i].hError == NULL)
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitEventTables: ErrorObject Create Failed Index 0x%x\n"), i));
            rc = FALSE;
            break;
        }
        PRINTMSG(ZONE_VERBOSE, (TEXT("EDMA: EDMAInitEventTables: %s Error Handle 0x%X\n"),
                                szEventName, g_pEDMACtx[instanceId]->DMAEventTable[i].hError));
    }

    for (i = 0; rc == TRUE && i < (int)pGlobalConfig->numQdmaChannels; i++)
    {
        /* Create the Unique Event Name First */
        _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"),
                    EDMA_EVENT_NAME, instanceId, EDMA3_DRV_QDMA_CHANNEL_0 + i);

        /* Create the Event Object and Initialize the Event Tbl Member */
        g_pEDMACtx[instanceId]->QDMAEventTable[i].hEvent = CreateEvent(NULL, FALSE, FALSE, szEventName);
        if (g_pEDMACtx[instanceId]->QDMAEventTable[i].hEvent == NULL)
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitEventTables: EventObject Create Failed Index 0x%x\n"), i));
            rc = FALSE;
            break;
        }
        PRINTMSG(ZONE_VERBOSE, (TEXT("EDMA: EDMAInitEventTables: %s QDMA Complete Handle 0x%X\n"),
                                szEventName, g_pEDMACtx[instanceId]->QDMAEventTable[i].hEvent));

        /* Create the Event Object for errors */
        _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"),
                    EDMA_ERROR_NAME, instanceId, EDMA3_DRV_QDMA_CHANNEL_0 + i);
        g_pEDMACtx[instanceId]->QDMAEventTable[i].hError = CreateEvent(NULL, FALSE, FALSE, szEventName);
        if (g_pEDMACtx[instanceId]->QDMAEventTable[i].hError == NULL)
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitEventTables: QDMA ErrorObject Create Failed Index 0x%x\n"), i));
            rc = FALSE;
            break;
        }
        PRINTMSG(ZONE_VERBOSE, (TEXT("EDMA: EDMAInitEventTables: %s Error Handle 0x%X\n"),
                                szEventName, g_pEDMACtx[instanceId]->QDMAEventTable[i].hError));
    }

    for (i = 0; rc == TRUE && i < EDMA3_NUM_CCERRORS; i++)
    {
        /* Create the Event Object for errors */
        _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"), EDMA_CC_ERROR_NAME, instanceId, i);
        g_pEDMACtx[instanceId]->CCEventTable[i].hError = CreateEvent(NULL, FALSE, FALSE, szEventName);
        if (g_pEDMACtx[instanceId]->CCEventTable[i].hError == NULL)
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitEventTables: CC ErrorObject Create Failed Index 0x%x\n"), i));
            rc = FALSE;
            break;
        }
        PRINTMSG(ZONE_VERBOSE, (TEXT("EDMA: EDMAInitEventTables: %s Error Handle 0x%X\n"),
                                szEventName, g_pEDMACtx[instanceId]->CCEventTable[i].hError));
    }

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAInitHardware
//
static BOOL EDMAInitInterrupts(unsigned int instanceId)
{
    BOOL rc = FALSE;
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    unsigned int tcNum;

    EDMA_CONTEXT *pInstCtx = NULL;
    
    pInstCtx = g_pEDMACtx[instanceId];

    pGlobalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;
    
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
                         &(pGlobalConfig->xferCompleteInt), 
                    sizeof(pGlobalConfig->xferCompleteInt),
                         &(pInstCtx->dwEDMACCSysIntr), 
                    sizeof(pInstCtx->dwEDMACCSysIntr), 
                           NULL))
    {
        ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Error obtaining CC SYSINTR value!\n")));
        pInstCtx->dwEDMACCSysIntr = (DWORD)SYSINTR_UNDEFINED;
        goto done;
    }

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
                        &(pGlobalConfig->ccError), 
                   sizeof(pGlobalConfig->ccError),
                        &(pInstCtx->dwEDMACCErrSysIntr), 
                   sizeof(pInstCtx->dwEDMACCErrSysIntr), 
                          NULL))
    {
        ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Error obtaining TC error SYSINTR value!\n")));
        pInstCtx->dwEDMACCErrSysIntr = (DWORD)SYSINTR_UNDEFINED;
        goto done;
    }

    for (tcNum = 0; tcNum < pGlobalConfig->numTcs; tcNum++)
    {   
        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
                        &(pGlobalConfig->tcError[tcNum]), 
                   sizeof(pGlobalConfig->tcError[tcNum]),
                        &(pInstCtx->dwEDMATCErrSysIntr[tcNum]), 
                   sizeof(pInstCtx->dwEDMATCErrSysIntr[tcNum]), 
                          NULL))
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Error obtaining TC error SYSINTR value!\n")));
            pInstCtx->dwEDMATCErrSysIntr[tcNum] = (DWORD)SYSINTR_UNDEFINED;
            goto done;
        }
        
        // Create interrupt event
        pInstCtx->hEDMAIntrEventTCError[tcNum] = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!pInstCtx->hEDMAIntrEventTCError[tcNum])
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Failed to create TC interrupt event!\n")));
            goto done;
        }
        
        if(!InterruptInitialize(pInstCtx->dwEDMATCErrSysIntr[tcNum], 
                                pInstCtx->hEDMAIntrEventTCError[tcNum], 
                                0, 0))
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: InterruptInitialize failed!\r\n")));
            goto done;
        }
        
        // Create interrupt thread        
        pInstCtx->hEDMAIntrThreadTCError[tcNum] = CreateThread(NULL, 0,
                                                      (LPTHREAD_START_ROUTINE)EDMAIstTCError,
                                                      (PVOID)&g_EDMATccErrIntEvt[instanceId][tcNum], 0, NULL);
        
        if(!pInstCtx->hEDMAIntrThreadTCError[tcNum])
        {
            ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Failed to create TC interrupt thread\r\n")));
            goto done;
        }
        
        CeSetThreadPriority(pInstCtx->hEDMAIntrThreadTCError[tcNum], 
                            pInstCtx->dwEDMAIstThreadPriority);
    }

    // Create interrupt events
    pInstCtx->hEDMAIntrEventTransferComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
    pInstCtx->hEDMAIntrEventCCError = CreateEvent(NULL, FALSE, FALSE, NULL);
    
    if (!pInstCtx->hEDMAIntrEventTransferComplete ||
        !pInstCtx->hEDMAIntrEventCCError)
    {
        ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Failed to create interrupt events!\n")));
        goto done;
    }
    
    if( !InterruptInitialize(pInstCtx->dwEDMACCSysIntr,    
                             pInstCtx->hEDMAIntrEventTransferComplete, 0, 0) 
         ||
        !InterruptInitialize(pInstCtx->dwEDMACCErrSysIntr, 
                             pInstCtx->hEDMAIntrEventCCError, 0, 0))
    {
        ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: InterruptInitialize failed!\r\n")));
        goto done;
    }

    // Create interrupt threads
    pInstCtx->hEDMAIntrThreadTransferComplete = CreateThread(NULL, 0,
                                                               (LPTHREAD_START_ROUTINE)EDMAIstTransferComplete,
                                                               (PVOID)instanceId, 0, NULL);
    pInstCtx->hEDMAIntrThreadCCError = CreateThread(NULL, 0,
                                                      (LPTHREAD_START_ROUTINE)EDMAIstCCError,
                                                      (PVOID)instanceId, 0, NULL);

    if( !pInstCtx->hEDMAIntrThreadTransferComplete ||
        !pInstCtx->hEDMAIntrThreadCCError)
    {
        ERRORMSG(TRUE, (TEXT("EDMA: EDMAInitInterrupts: Failed to create interrupt threads\r\n")));
        goto done;
    }

    CeSetThreadPriority(pInstCtx->hEDMAIntrThreadTransferComplete, 
                        pInstCtx->dwEDMAIstThreadPriority);

    CeSetThreadPriority(pInstCtx->hEDMAIntrThreadCCError, 
                        pInstCtx->dwEDMAIstThreadPriority);

    rc = TRUE;

done:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAInitDriver
//
static BOOL EDMAInitDriver(unsigned int instanceId)
{
    BOOL rc = FALSE;
    EDMA3_DRV_InitConfig initCfg;
    EDMA3_DRV_Result edma3Result = EDMA3_DRV_SOK;
    EDMA3_DRV_GblConfigParams *globalConfig; 
    EDMA3_DRV_InstanceInitConfig *instanceConfig;
    EDMA3_RM_MiscParam miscParam;

    PRINTMSG(ZONE_INIT, (_T("EDMA: +EDMAInitDriver\r\n")));

    globalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;
    instanceConfig = g_pEDMACtx[instanceId]->pInstanceConfig;

    /* Configuration structures for the Driver */
    initCfg.isMaster = TRUE;
    initCfg.regionId = (EDMA3_RM_RegionId)0u;
    initCfg.drvSemHandle = NULL;
    initCfg.drvInstInitConfig = instanceConfig;
    initCfg.gblerrCb = edma3GblErrCallback;
    initCfg.gblerrData = (void*)instanceId;
    miscParam.isSlave = FALSE;

    /* Create EDMA3 Driver Object */
    edma3Result = EDMA3_DRV_create(instanceId, globalConfig, (void*)&miscParam);
    if (edma3Result != EDMA3_DRV_SOK)
    {
        ERRORMSG(TRUE,(_T("EDMA: EDMA3_DRV_create failed\r\n")));
        goto done;
    }

    /* Create a semaphore now for driver instance. */
    initCfg.drvSemHandle = CreateSemaphore(NULL, 1, 100, NULL);
    if (initCfg.drvSemHandle == NULL)
    {
        ERRORMSG(TRUE,(_T("EDMA: CreateSemaphore failed\r\n")));
        goto done;
    }

    /* Open the Driver Instance */
    g_pEDMACtx[instanceId]->hEDMA = EDMA3_DRV_open(instanceId, (void*)&initCfg, &edma3Result);
    if (g_pEDMACtx[instanceId]->hEDMA == NULL)
    {
        ERRORMSG(TRUE, (_T("EDMA: EDMA3_DRV_open failed\r\n")));
        goto done;
    }

    rc = TRUE;

done:
    PRINTMSG(ZONE_INIT, (_T("EDMA: -EDMAInitDriver rc %d\r\n"), rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMACleanup
//
static void EDMACleanup(unsigned int instanceId)
{
    int i;
    unsigned int tcNum;
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    
    if (g_pEDMACtx[instanceId] == NULL)
        goto done;

    pGlobalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;

    if (g_pEDMACtx[instanceId]->hEDMA != NULL)
    {
        EDMA3_DRV_close(g_pEDMACtx[instanceId]->hEDMA, NULL);
    }
    EDMA3_DRV_delete(instanceId, NULL);

    g_pEDMACtx[instanceId]->intrThreadExit = TRUE;
    if (g_pEDMACtx[instanceId]->hEDMAIntrThreadTransferComplete)
    {
        SetEvent(g_pEDMACtx[instanceId]->hEDMAIntrEventTransferComplete);
        WaitForSingleObject(g_pEDMACtx[instanceId]->hEDMAIntrThreadTransferComplete, INFINITE);
    }
    if (g_pEDMACtx[instanceId]->hEDMAIntrThreadCCError)
    {
        SetEvent(g_pEDMACtx[instanceId]->hEDMAIntrEventCCError);
        WaitForSingleObject(g_pEDMACtx[instanceId]->hEDMAIntrThreadCCError, INFINITE);
    }
    
    for (tcNum = 0; tcNum < pGlobalConfig->numTcs; tcNum++)
    {
        if (g_pEDMACtx[instanceId]->hEDMAIntrThreadTCError)
        {
            SetEvent(g_pEDMACtx[instanceId]->hEDMAIntrEventTCError[tcNum]);
            WaitForSingleObject(g_pEDMACtx[instanceId]->hEDMAIntrThreadTCError[tcNum], INFINITE);
        }
    
        if (g_pEDMACtx[instanceId]->dwEDMATCErrSysIntr[tcNum] != (DWORD)SYSINTR_UNDEFINED)
        {
            InterruptDisable(g_pEDMACtx[instanceId]->dwEDMATCErrSysIntr[tcNum]);
            KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_pEDMACtx[instanceId]->dwEDMATCErrSysIntr[tcNum],
                        sizeof(g_pEDMACtx[instanceId]->dwEDMATCErrSysIntr[tcNum]), NULL, 0, NULL);
        }    
        
        CloseHandle(g_pEDMACtx[instanceId]->hEDMAIntrEventTCError[tcNum]);
        
    }

    if (g_pEDMACtx[instanceId]->dwEDMACCSysIntr != (DWORD)SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_pEDMACtx[instanceId]->dwEDMACCSysIntr);
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_pEDMACtx[instanceId]->dwEDMACCSysIntr,
                        sizeof(g_pEDMACtx[instanceId]->dwEDMACCSysIntr), NULL, 0, NULL);
    }
    if (g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr != (DWORD)SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr);
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr,
                        sizeof(g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr), NULL, 0, NULL);
    }
    

    CloseHandle(g_pEDMACtx[instanceId]->hEDMAIntrEventTransferComplete);
    CloseHandle(g_pEDMACtx[instanceId]->hEDMAIntrEventCCError);

    for (i = 0; i < EDMA3_MAX_DMA_CH; i++)
    {
        CloseHandle(g_pEDMACtx[instanceId]->DMAEventTable[i].hEvent);
        CloseHandle(g_pEDMACtx[instanceId]->DMAEventTable[i].hError);
    }
    for (i = 0; i < EDMA3_MAX_QDMA_CH; i++)
    {
        CloseHandle(g_pEDMACtx[instanceId]->QDMAEventTable[i].hEvent);
        CloseHandle(g_pEDMACtx[instanceId]->QDMAEventTable[i].hError);
    }
    for (i = 0; i < EDMA3_NUM_CCERRORS; i++)
    {
        CloseHandle(g_pEDMACtx[instanceId]->CCEventTable[i].hError);
    }

    if (pGlobalConfig->globalRegs != NULL)
    {
        MmUnmapIoSpace(pGlobalConfig->globalRegs, sizeof(AM389X_EDMACC_REGS));
    }
    for (i = 0; i < (int)pGlobalConfig->numTcs; ++i)
    {
        if (pGlobalConfig->tcRegs[i] != NULL)
        {
            MmUnmapIoSpace(pGlobalConfig->tcRegs[i], sizeof(AM389X_EDMATC_REGS));
        }
    }

    DeleteCriticalSection(&g_pEDMACtx[instanceId]->csEdma);

    LocalFree(g_pEDMACtx[instanceId]);
    g_pEDMACtx[instanceId] = NULL;

done:
    return;
}


//------------------------------------------------------------------------------
//
//  Function:  EDMAIstTransferComplete
//
static void EDMAIstTransferComplete(PVOID pContext)
{
    unsigned int intState = 0;
    unsigned int instanceId = (unsigned int)pContext;
    
    for(;;)
    {
        PRINTMSG(ZONE_IST, (TEXT("EDMA: Waiting for DMA Completion ...\r\n")));
        WaitForSingleObject(g_pEDMACtx[instanceId]->hEDMAIntrEventTransferComplete, INFINITE);

        if (g_pEDMACtx[instanceId]->intrThreadExit)
            break;

        // Call EDMA3 ISR handler
        edma3OsProtectEntry(instanceId, EDMA3_OS_PROTECT_INTERRUPT, &intState);
        lisrEdma3ComplHandler0(instanceId);
        edma3OsProtectExit (instanceId, EDMA3_OS_PROTECT_INTERRUPT, intState);

        InterruptDone(g_pEDMACtx[instanceId]->dwEDMACCSysIntr);
    }
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAIstCCError
//
static void EDMAIstCCError(PVOID pContext)
{
    unsigned int intState = 0;
    unsigned int instanceId = (unsigned int)pContext;
    
    for(;;)
    {
        PRINTMSG(ZONE_IST, (TEXT("EDMA: Waiting for CC Errors ...\r\n")));
        WaitForSingleObject(g_pEDMACtx[instanceId]->hEDMAIntrEventCCError, INFINITE);

        if (g_pEDMACtx[instanceId]->intrThreadExit)
            break;

        // Call EDMA3 ISR handler
        edma3OsProtectEntry(instanceId, EDMA3_OS_PROTECT_INTERRUPT, &intState);
        lisrEdma3CCErrHandler0(instanceId);
        edma3OsProtectExit (instanceId, EDMA3_OS_PROTECT_INTERRUPT, intState);

        InterruptDone(g_pEDMACtx[instanceId]->dwEDMACCErrSysIntr);
    }
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAIstTCError
//
static void EDMAIstTCError(PVOID pContext)
{
    unsigned int intState = 0;
    EDMA_TC_ERROR_EVENT *pTcEvent = (EDMA_TC_ERROR_EVENT*)pContext;
    
    for(;;)
    {
        PRINTMSG(ZONE_IST, (TEXT("EDMA: Waiting for TC Errors ...\r\n")));
        WaitForSingleObject(g_pEDMACtx[pTcEvent->instanceId]->hEDMAIntrEventTCError[pTcEvent->tcNum], INFINITE);

        if (g_pEDMACtx[pTcEvent->instanceId]->intrThreadExit)
            break;

        // Call EDMA3 ISR handler
        edma3OsProtectEntry(pTcEvent->instanceId, EDMA3_OS_PROTECT_INTERRUPT, &intState);
        lisrEdma3TCErrHandler[pTcEvent->tcNum](pTcEvent->instanceId);
        edma3OsProtectExit (pTcEvent->instanceId, EDMA3_OS_PROTECT_INTERRUPT, intState);

        InterruptDone(g_pEDMACtx[pTcEvent->instanceId]->dwEDMATCErrSysIntr[pTcEvent->tcNum]);
    }
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAFindChannelForTcc
//
static unsigned int EDMAFindChannelForTcc(unsigned int instanceId, unsigned int tcc)
{
    unsigned int chNum;

    for (chNum = 0; chNum < EDMA3_MAX_DMA_CH; chNum++)
    {
        if (edma3DrvChBoundRes[instanceId][chNum].paRAMId != -1 &&
            edma3DrvChBoundRes[instanceId][chNum].tcc == tcc)
            return chNum;
    }

    for (chNum = EDMA3_DRV_QDMA_CHANNEL_0; chNum < EDMA3_DRV_QDMA_CHANNEL_7; chNum++)
    {
        if (edma3DrvChBoundRes[instanceId][chNum].paRAMId != -1 &&
            edma3DrvChBoundRes[instanceId][chNum].tcc == tcc)
            return chNum;
    }

    return EDMA3_DRV_CH_NO_TCC_MAP;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMAHandleToInstance
//
static unsigned int EDMAHandleToInstance(EDMA3_DRV_Handle hEDMA)
{
    unsigned int instanceId;
    
    for (instanceId = 0; instanceId < EDMA3_MAX_EDMA3_INSTANCES; instanceId++)
    {
        if (g_pEDMACtx[instanceId]->hEDMA == hEDMA)
        {
            break;
        }
    }
    
    if (instanceId == EDMA3_MAX_EDMA3_INSTANCES)
    {
        // Invalid instance handle
        instanceId = (unsigned int)-1;
    }
    
    return instanceId;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMACheckForChannelError
//
static BOOL EDMACheckForChannelError(unsigned int instanceId, unsigned int chNum)
{
    BOOL err = FALSE;
    EDMA3_DRV_GblConfigParams *pGlobalConfig;
    
    pGlobalConfig = g_pEDMACtx[instanceId]->pGlobalConfig;
    
    if (chNum < EDMA3_MAX_DMA_CH)
    {
        if (chNum < 32)
        {
            if (((PEDMACCREGS)pGlobalConfig->globalRegs)->EMR & (1 << chNum))
                err = TRUE;
        }
        else
        {
            if (((PEDMACCREGS)pGlobalConfig->globalRegs)->EMRH & (1 << (chNum - 32)))
                err = TRUE;
        }
    }
    else if (chNum >= EDMA3_DRV_QDMA_CHANNEL_0 &&
             chNum <= EDMA3_DRV_QDMA_CHANNEL_7)
    {
        if (((PEDMACCREGS)pGlobalConfig->globalRegs)->QEMR & (1 << (chNum - EDMA3_DRV_QDMA_CHANNEL_0)))
            err = TRUE;
    }

    return err;
}

//------------------------------------------------------------------------------
//
//  Function:  edma3TccCallback
//
void edma3TccCallback(unsigned int tcc,
                      EDMA3_RM_TccStatus status,
                      void *appData)
{
    unsigned int chNum, instanceId;
    EDMA_DMA_EVENT *pEvent;
    EDMA3_DRV_Handle hEDMA;
    
    PRINTMSG(ZONE_IST, (L"EDMA: edma3TccCallback: tcc %d, status 0x%x\r\n", tcc, status));

    // Get handle to EDMA
    hEDMA = (EDMA3_DRV_Handle)appData;
    
    // Convert handle to instance ID
    instanceId = EDMAHandleToInstance(hEDMA);

    // Convert from TCC to channel number
    chNum = EDMAFindChannelForTcc(instanceId, tcc);
    if (chNum != EDMA3_DRV_CH_NO_TCC_MAP)
    {
        switch (status)
        {
        case EDMA3_RM_XFER_COMPLETE:
            if (chNum >= EDMA3_DRV_QDMA_CHANNEL_0 && chNum <= EDMA3_DRV_QDMA_CHANNEL_7)
                pEvent = &g_pEDMACtx[instanceId]->QDMAEventTable[chNum - EDMA3_DRV_QDMA_CHANNEL_0];
            else
                pEvent = &g_pEDMACtx[instanceId]->DMAEventTable[chNum];

            pEvent->transStatus = EDMA_STAT_TRANSFER_COMPLETE;

            // Check for error condition.
            // Caller may read status before event miss error callback is performed.
            if (EDMACheckForChannelError(instanceId, chNum))
                pEvent->transStatus |= EDMA_STAT_EVENT_MISSED;

            SetEvent(pEvent->hEvent);
            break;

        case EDMA3_RM_E_CC_DMA_EVT_MISS:
            g_pEDMACtx[instanceId]->DMAEventTable[chNum].transStatus |= EDMA_STAT_EVENT_MISSED;
            SetEvent(g_pEDMACtx[instanceId]->DMAEventTable[chNum].hError);
            break;

        case EDMA3_RM_E_CC_QDMA_EVT_MISS:
            g_pEDMACtx[instanceId]->QDMAEventTable[chNum - EDMA3_DRV_QDMA_CHANNEL_0].transStatus |= EDMA_STAT_EVENT_MISSED;
            SetEvent(g_pEDMACtx[instanceId]->QDMAEventTable[chNum - EDMA3_DRV_QDMA_CHANNEL_0].hError);
            break;

        default:
            ERRORMSG(TRUE, (L"EDMA: edma3TccCallback: Invalid status %d for tcc %d\r\n", status, tcc));
            break;
        }
    }
    else
    {
        PRINTMSG(ZONE_ERROR, (L"EDMA: edma3TccCallback: No channel for TCC 0x%x\r\n", tcc));
    }
}

//------------------------------------------------------------------------------
//
//  Function:  edma3GblErrCallback
//
void edma3GblErrCallback(EDMA3_RM_GlobalError deviceStatus,
                         unsigned int instanceId,
                         void *gblerrData)
{
    unsigned int ccInstanceId;
    
    PRINTMSG(ZONE_IST, (L"EDMA: edma3GblErrCallback: err 0x%x, instance 0x%x\r\n", deviceStatus, instanceId));

    // Get instance ID of channel controller associated with this callback
    ccInstanceId = (unsigned int)gblerrData;

    switch (deviceStatus)
    {
    case EDMA3_RM_E_CC_QUE_THRES_EXCEED:
    case EDMA3_RM_E_TC_MEM_LOCATION_READ_ERROR:
    case EDMA3_RM_E_TC_MEM_LOCATION_WRITE_ERROR:
    case EDMA3_RM_E_TC_INVALID_ADDR:
    case EDMA3_RM_E_TC_TR_ERROR:
        // instanceId = event queue num / TC num
        if (instanceId < g_pEDMACtx[ccInstanceId]->pGlobalConfig->numTcs)
        {
            ERRORMSG(TRUE, (L"EDMA: edma3GblErrCallback: CC error 0x%x for event queue %d\r\n",
                            deviceStatus, instanceId));

            SetEvent(g_pEDMACtx[ccInstanceId]->CCEventTable[instanceId].hError);
        }
        else
        {
            ERRORMSG(TRUE, (L"EDMA: edma3GblErrCallback: Invalid instanceId %d for status %d\r\n",
                            instanceId, deviceStatus));
        }
        break;

    case EDMA3_RM_E_CC_TCC:
        // instanceId not used
        ERRORMSG(TRUE, (L"EDMA: edma3GblErrCallback: CC TCC error\r\n"));
        SetEvent(g_pEDMACtx[ccInstanceId]->CCEventTable[g_pEDMACtx[ccInstanceId]->pGlobalConfig->numTcs].hError);
        break;

    default:
        ERRORMSG(TRUE, (L"EDMA: edma3GblErrCallback: Invalid status %d\r\n", deviceStatus));
        break;
    }
}


//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_getTransferEvent
//
//  Returns the transfer complete event for an EDMA channel.
//  The event handle should be closed when finised with.
//
HANDLE EDMA3_DRV_getTransferEvent(EDMA3_DRV_Handle hEdma,
                                  unsigned int edmaChannel)
{
    HANDLE hEvent;
    unsigned int instanceId;
    TCHAR szEventName[EDMA_MAX_EVENT_NAME_LEN];
    
    instanceId = EDMAHandleToInstance(hEdma);
    
    _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"),
                EDMA_EVENT_NAME, instanceId, edmaChannel);
    hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName);
    return hEvent;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_getErrorEvent
//
//  Returns the error event for an EDMA channel.
//  The event handle should be closed when finised with.
//
HANDLE EDMA3_DRV_getErrorEvent(EDMA3_DRV_Handle hEdma,
                               unsigned int edmaChannel)
{
    HANDLE hEvent;
    unsigned int instanceId;
    TCHAR szEventName[EDMA_MAX_EVENT_NAME_LEN];
    
    instanceId = EDMAHandleToInstance(hEdma);
    
    _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"),
                EDMA_ERROR_NAME, instanceId, edmaChannel);
    hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName);
    return hEvent;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_getCCErrorEvent
//
//  Returns the CC error event.
//  The event handle should be closed when finised with.
//
HANDLE EDMA3_DRV_getCCErrorEvent(EDMA3_DRV_Handle hEdma,
                                 unsigned int instanceId)
{
    HANDLE hEvent;
    unsigned int ccInstanceId;
    TCHAR szEventName[EDMA_MAX_EVENT_NAME_LEN];
    
    ccInstanceId = EDMAHandleToInstance(hEdma);
    
    _stprintf_s(szEventName, EDMA_MAX_EVENT_NAME_LEN, TEXT("%s%02x_%04x"),
                EDMA_CC_ERROR_NAME, ccInstanceId, instanceId);
    hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName);
    return hEvent;
}

//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_getTransferStatus
//
//  Read and reset the transfer status for an EDMA channel.
//
EDMA_TRANS_STATUS EDMA3_DRV_getTransferStatus(
    EDMA3_DRV_Handle hEdma, unsigned int edmaChannel)
{
    unsigned int intState = 0;
    EDMA_TRANS_STATUS transStatus = EDMA_STAT_INVALID_CHANNEL_NUM;
    unsigned int instanceId;

    PRINTMSG(ZONE_FUNCTION, (TEXT("EDMA: +EDMA3_DRV_getTransferStatus(%d)\r\n"), edmaChannel));

    instanceId = EDMAHandleToInstance(hEdma);

    if ( !((edmaChannel < EDMA3_MAX_DMA_CH) ||
           (edmaChannel >= EDMA3_DRV_QDMA_CHANNEL_0 &&
            edmaChannel <= EDMA3_DRV_QDMA_CHANNEL_7) ))
    {
        PRINTMSG(ZONE_ERROR, (_T("EDMA: EDMA3_DRV_getTransferStatus: Invalid channel passed: %d\r\n"),
                              edmaChannel));
    }
    else
    {
        edma3OsProtectEntry(instanceId, EDMA3_OS_PROTECT_INTERRUPT, &intState);

        if (edmaChannel < EDMA3_MAX_DMA_CH)
        {
            transStatus = g_pEDMACtx[instanceId]->DMAEventTable[edmaChannel].transStatus;
            g_pEDMACtx[instanceId]->DMAEventTable[edmaChannel].transStatus = 0;
            ResetEvent(g_pEDMACtx[instanceId]->DMAEventTable[edmaChannel].hEvent);
            ResetEvent(g_pEDMACtx[instanceId]->DMAEventTable[edmaChannel].hError);
        }
        else if (edmaChannel >= EDMA3_DRV_QDMA_CHANNEL_0 &&
                 edmaChannel <= EDMA3_DRV_QDMA_CHANNEL_7)
        {
            transStatus = g_pEDMACtx[instanceId]->QDMAEventTable[edmaChannel - EDMA3_DRV_QDMA_CHANNEL_0].transStatus;
            g_pEDMACtx[instanceId]->QDMAEventTable[edmaChannel - EDMA3_DRV_QDMA_CHANNEL_0].transStatus = 0;
            ResetEvent(g_pEDMACtx[instanceId]->QDMAEventTable[edmaChannel - EDMA3_DRV_QDMA_CHANNEL_0].hEvent);
            ResetEvent(g_pEDMACtx[instanceId]->QDMAEventTable[edmaChannel - EDMA3_DRV_QDMA_CHANNEL_0].hError);
        }
        
        edma3OsProtectExit(instanceId, EDMA3_OS_PROTECT_INTERRUPT, intState);
    }

    PRINTMSG(ZONE_FUNCTION, (TEXT("EDMA: -EDMA3_DRV_getTransferStatus(%d)\r\n"), transStatus));
    return transStatus;
}


//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_resetCCErrors
//
//  Resets the CC error events.
//
void EDMA3_DRV_resetCCErrors(unsigned int instanceId)
{
    int i;
    PRINTMSG(ZONE_FUNCTION, (TEXT("EDMA: +EDMA3_DRV_resetCCErrors\r\n")));

    for (i = 0; i <= EDMA3_NUM_TC; ++i)
    {
        ResetEvent(g_pEDMACtx[instanceId]->CCEventTable[i].hError);
    }

    PRINTMSG(ZONE_FUNCTION, (TEXT("EDMA: -EDMA3_DRV_resetCCErrors\r\n")));
}


//------------------------------------------------------------------------------
//
//  Function:  EDMA3_DRV_releaseInstHandle
//
//  Releases an EDMA3 handle previous obtained via EDMA3_DRV_getInstHandle().
//
void EDMA3_DRV_releaseInstHandle(EDMA3_DRV_Handle hEdma)
{
    // Do nothing - for future use
}


//------------------------------------------------------------------------------
//
//  Function:  edma3OsProtectEntry
//
void edma3OsProtectEntry(unsigned int instanceId, int level, unsigned int *intState)
{
    DEBUGCHK(g_pEDMACtx[instanceId] != NULL);
    if (g_pEDMACtx[instanceId])
        EnterCriticalSection(&g_pEDMACtx[instanceId]->csEdma);
}

//------------------------------------------------------------------------------
//
//  Function:  edma3OsProtectExit
//
void edma3OsProtectExit(unsigned int instanceId, int level, unsigned int intState)
{
    DEBUGCHK(g_pEDMACtx[instanceId] != NULL);
    if (g_pEDMACtx[instanceId])
        LeaveCriticalSection(&g_pEDMACtx[instanceId]->csEdma);
}

//------------------------------------------------------------------------------
//
//  Function:  edma3OsSemTake
//
EDMA3_DRV_Result edma3OsSemTake(EDMA3_OS_Sem_Handle hSem, int mSecTimeout)
{
    EDMA3_DRV_Result drvRes = EDMA3_DRV_SOK;

    if (hSem == NULL)
    {
        drvRes = EDMA3_DRV_E_INVALID_PARAM;
    }
    else
    {
        if (WaitForSingleObject(hSem, mSecTimeout) != WAIT_OBJECT_0)
            drvRes = EDMA3_DRV_E_SEMAPHORE;
    }

    return drvRes;
}

//------------------------------------------------------------------------------
//
//  Function:  edma3OsSemGive
//
EDMA3_DRV_Result edma3OsSemGive(EDMA3_OS_Sem_Handle hSem)
{
    EDMA3_DRV_Result drvRes = EDMA3_DRV_SOK;

    if (NULL == hSem)
    {
        drvRes = EDMA3_DRV_E_INVALID_PARAM;
    }
    else
    {
        if (ReleaseSemaphore(hSem, 1, NULL) == 0)
            drvRes = EDMA3_DRV_E_SEMAPHORE;
    }

    return drvRes;
}

