// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: dma.cpp
//

#include "omap.h"
#include <ceddk.h>
#include <oal_clock.h>
#include "ceddkex.h"
#include "_debug.h"
#include "omap_sdma_regs.h"
#include "dmacontroller.h"
#include "soc_cfg.h"


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

DBGPARAM dpCurSettings = {
    L"DMA", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define DMA_DEVICE_COOKIE       'dmaD'
#define DMA_INSTANCE_COOKIE     'dmaI'

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD                   cookie;
    DWORD                   SdmaDevice;
    DWORD                   SdmaIndex;
    DWORD                   irqCamera;
    DWORD                   priority;
    SystemDmaController    *pSystemController;
    CRITICAL_SECTION        cs;
    LONG                    instances;
} Device_t;

typedef struct {
    DWORD                   cookie;
    Device_t               *pDevice;
    DWORD                   reservedSysDma;
    DWORD                   reservedCamDma;
} Instance_t;


//------------------------------------------------------------------------------
//  Local Functions

BOOL
DMA_Deinit(
    DWORD context
    );


//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"DmaIndex", PARAM_MULTIDWORD, FALSE, offset(Device_t, SdmaIndex),
        fieldsize(Device_t, SdmaIndex), 0
    }, { 
        L"priority256", PARAM_DWORD, TRUE, offset(Device_t, priority),
        fieldsize(Device_t, priority), (VOID*)97
    }
};


//------------------------------------------------------------------------------
//
//  Function:  InitializeDevice
//
//  Initializes the general system dma controller and camera dma controller.
//
BOOL
InitializeDevice(
    Device_t *pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"+InitializeDevice(0x%08x)\r\n", pDevice
        ));

    BOOL rc = FALSE;
    PHYSICAL_ADDRESS pa;
    DWORD SysIntArray[OMAP_DMA_INTERRUPT_COUNT];
    
    // create instance of objects
    pDevice->pSystemController = new SystemDmaController();
    if (pDevice->pSystemController == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR! (DMA) InitializeDevice :"
            L"Unable to allocate system dma controller(err=0x%08X)",
            GetLastError()
            ));
        goto cleanUp;
        }

    SysIntArray[0] = GetIrqByDevice(pDevice->SdmaDevice,L"IRQ0");
    SysIntArray[1] = GetIrqByDevice(pDevice->SdmaDevice,L"IRQ1");
    SysIntArray[2] = GetIrqByDevice(pDevice->SdmaDevice,L"IRQ2");
    SysIntArray[3] = GetIrqByDevice(pDevice->SdmaDevice,L"IRQ3");
    // initialize objects
    pa.HighPart = 0;
    pa.LowPart = GetAddressByDevice(pDevice->SdmaDevice);
    if (pDevice->pSystemController->Initialize(
            pa,
			sizeof(OMAP_SDMA_REGS),
            SysIntArray) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR! (DMA) InitializeDevice :"
            L"Failed to initialize system dma controller(err=0x%08X)",
            GetLastError()
            ));
        goto cleanUp;
        }

    // start interrupt handlers
    if (pDevice->pSystemController->StartInterruptThread(
            SysIntArray[0], pDevice->priority) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR! (DMA) InitializeDevice :"
            L"Failed to start interrupt thread for system dma controller"
            L"(err=0x%08X)",
            GetLastError()
            ));
        goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (
        L"-InitializeDevice(rc=%d)\r\n", rc
        ));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DMA_Init
//
//  Called by device manager to initialize device.
//
DWORD
DMA_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;

    UNREFERENCED_PARAMETER(pBusContext);
    UNREFERENCED_PARAMETER(szContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+DMA_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    Device_t *pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_Init: "
            L"Failed allocate DMA controller structure\r\n"
            ));
        goto cleanUp;
        }

    // initialize memory
    memset(pDevice, 0, sizeof(Device_t));

    // Set cookie
    pDevice->cookie = DMA_DEVICE_COOKIE;

    // Initalize critical section
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_Init: "
            L"Failed read DMA driver registry parameters\r\n"
            ));
        goto cleanUp;
        }
        
	// Retrieve device ID
    pDevice->SdmaDevice = SOCGetDMADevice(pDevice->SdmaIndex);
    
    // Set hardware to full power
	EnableDeviceClocks(pDevice->SdmaDevice, TRUE);
    
    // DMA will be in smart idle mode so we don't need to
    // ever set the power state of the DMA to D4

    // initialize general dma controller
    if (InitializeDevice(pDevice) == FALSE)
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: DMA_Init: "
            L"Failed to initialize device\r\n"
            ));
        goto cleanUp;        
        }

    rc = (DWORD)pDevice;
    
cleanUp:
    if (rc == 0) DMA_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-DMA_Init(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  DMA_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
DMA_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+DMA_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != DMA_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: DMA_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Check for open instances
    if (pDevice->instances > 0)
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: DMA_Deinit: "
            L"Deinit with active instance (%d instances active)\r\n",
            pDevice->instances
            ));
        goto cleanUp;
        }

    if (pDevice->pSystemController != NULL)
        {
        pDevice->pSystemController->Uninitialize();
        delete pDevice->pSystemController;
        }

    // Set hardware to D4
	EnableDeviceClocks(pDevice->SdmaDevice, FALSE);

	// Delete critical section
    DeleteCriticalSection(&pDevice->cs);
 
    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DMA_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  DMA_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
DMA_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = (Device_t*)context;
    Instance_t *pInstance = NULL;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+DMA_Open(0x%08x, 0x%08x, 0x%08x\r\n", context, accessCode, shareMode
        ));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != DMA_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: DMA_Open: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Create device structure
    pInstance = (Instance_t*)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_Open: "
            L"Failed allocate DMA instance structure\r\n"
            ));
        goto cleanUp;
        }

    // Set cookie
    memset(pInstance, 0, sizeof(Instance_t));
    pInstance->cookie = DMA_INSTANCE_COOKIE;

    // Save device reference
    pInstance->pDevice = pDevice;

    // Increment number of open instances
    InterlockedIncrement(&pDevice->instances);

    // Sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Done...
    rc = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DMA_Open(rc = 0x%08x)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  DMA_Close
//
//  This function closes the device context.
//
BOOL
DMA_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice;
    Instance_t *pInstance = (Instance_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+DMA_Close(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != DMA_INSTANCE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: DMA_Read: "
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
    DEBUGMSG(ZONE_FUNCTION, (L"-DMA_Close(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DMA_IOControl
//
//  This function sends a command to a device.
//
BOOL
DMA_IOControl(
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
    Instance_t *pInstance = (Instance_t*)context;
    Device_t *pDevice = pInstance->pDevice;

    UNREFERENCED_PARAMETER(pOutSize);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+DMA_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != DMA_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        { 
        case IOCTL_DMA_INTERRUPTDONE:
            {
            DmaControllerBase *pController;
            IOCTL_DMA_INTERRUPTDONE_IN *pContext;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_INTERRUPTDONE\r\n"));

            if (pInBuffer == NULL || 
                inSize != sizeof(IOCTL_DMA_INTERRUPTDONE_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_INTERRUPTDONE invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            pContext = (IOCTL_DMA_INTERRUPTDONE_IN*)pInBuffer;
            switch (pContext->type)
                {
                case DMA_TYPE_SYSTEM:
                    pController = pDevice->pSystemController;
                    break;

                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanUp;               
                }
            EnterCriticalSection(&pDevice->cs);
            rc = pController->InterruptDone(pContext);
            LeaveCriticalSection(&pDevice->cs);
            }
            break;
            
        case IOCTL_DMA_RESERVE_CHANNEL:
            {
            DmaControllerBase *pController;
            IOCTL_DMA_RESERVE_IN *pDmaType;
            IOCTL_DMA_RESERVE_OUT *pContext;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_RESERVE_CHANNEL\r\n"));
            
            // check for correct parameters
            if (pOutBuffer == NULL || outSize != sizeof(IOCTL_DMA_RESERVE_OUT) ||
                pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_RESERVE_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_RESERVE_CHANNEL invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            // if successful allocate memory            
            pDmaType = (IOCTL_DMA_RESERVE_IN*)pInBuffer;
            pContext = (IOCTL_DMA_RESERVE_OUT*)pOutBuffer;
            switch (*pDmaType)
                {
                case DMA_TYPE_SYSTEM:
                    pController = pDevice->pSystemController;
                    break;

                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanUp;
                }

            // reset context information
            memset(pContext, 0, sizeof(IOCTL_DMA_RESERVE_OUT));

            // copy context information
            pContext->type = *pDmaType;
            EnterCriticalSection(&pDevice->cs);
            if (pController->ReserveChannel(pContext) == FALSE)
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_RESERVE_CHANNEL: failed\r\n"
                    ));
                LeaveCriticalSection(&pDevice->cs);
                break;
                }
            LeaveCriticalSection(&pDevice->cs);
            rc = TRUE;
            }            
            break;

        case IOCTL_DMA_RELEASE_CHANNEL:
            {
            DmaControllerBase *pController;
            IOCTL_DMA_RELEASE_IN *pContext;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_RELEASE_CHANNEL\r\n"));

            if (pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_RELEASE_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_RELEASE_CHANNEL invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            pContext = (IOCTL_DMA_RELEASE_IN*)pInBuffer;
            switch (pContext->type)
                {
                case DMA_TYPE_SYSTEM:
                    pController = pDevice->pSystemController;
                    break;

                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanUp;                
                }

            EnterCriticalSection(&pDevice->cs);
            pController->ReleaseChannel(pContext);
            LeaveCriticalSection(&pDevice->cs);
            rc = TRUE;
            }
            break;

        case IOCTL_DMA_REGISTER_EVENTHANDLE:
            {
            DmaControllerBase *pController;
            IOCTL_DMA_REGISTER_EVENTHANDLE_IN *pRegisterEvent;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_REGISTER_EVENTHANDLE\r\n"));

            if (pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_REGISTER_EVENTHANDLE_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_REGISTER_EVENTHANDLE invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            pRegisterEvent = (IOCTL_DMA_REGISTER_EVENTHANDLE_IN*)pInBuffer;

#if (_WINCEOSVER<600)
            pRegisterEvent->pContext = (DmaChannelContext_t*)MapCallerPtr(
                    pRegisterEvent->pContext, sizeof(DmaChannelContext_t)
                    );
#endif
            
            switch (pRegisterEvent->pContext->type)
                {
                case DMA_TYPE_SYSTEM:
                    pController = pDevice->pSystemController;
                    break;

                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanUp;                
                }

            EnterCriticalSection(&pDevice->cs);
            rc = pController->SetSecondaryInterruptHandler(
                    pRegisterEvent->pContext,
                    pRegisterEvent->hEvent,
                    pRegisterEvent->processId
                    );
            LeaveCriticalSection(&pDevice->cs);
            }
            break; 

        case IOCTL_DMA_DISABLESTANDBY:
            {
            DmaControllerBase *pController;
            IOCTL_DMA_DISABLESTANDBY_IN *pIoctlInput;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_DISABLESTANDBY\r\n"));

            if (pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_DISABLESTANDBY_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_DISABLESTANDBY invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            // Cast pInBuffer
            pIoctlInput = (IOCTL_DMA_DISABLESTANDBY_IN*)pInBuffer;
#if (_WINCEOSVER<600)
            pIoctlInput->pContext = (DmaChannelContext_t*)MapCallerPtr(
                    pIoctlInput->pContext, sizeof(DmaChannelContext_t)
                    );
#endif

            if (!pIoctlInput->bDelicatedChannel)
            {
            switch (pIoctlInput->pContext->type)
                {
                case DMA_TYPE_SYSTEM:
                    pController = pDevice->pSystemController;
                    break;

                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanUp;                
                    }
            }
            else
            {
                pController = pDevice->pSystemController;
                }

            EnterCriticalSection(&pDevice->cs);
            rc = pController->DisableStandby(
                    pIoctlInput->pContext,
                    pIoctlInput->bNoStandby
                    );
            LeaveCriticalSection(&pDevice->cs);
            }
            break;
            
        case IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT:
            {
            SystemDmaController *pController;
            IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT *pIoctlOutput;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT\r\n"));

            if (pOutBuffer == NULL || outSize != sizeof(IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT) ||
                pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            // Cast pOutBuffer
            pIoctlOutput = (IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT*)pOutBuffer;

            //assume this is a system interrupt
            pController = pDevice->pSystemController;
            
            EnterCriticalSection(&pDevice->cs);
            rc = pController->DI_ReserveInterrupt(
                                                *((DWORD*)pInBuffer), 
                                                &pIoctlOutput->IrqNum,
                                                &pIoctlOutput->DmaControllerPhysAddr,
                                                &pIoctlOutput->ffDmaChannels
                                              );
            LeaveCriticalSection(&pDevice->cs);
            }
            break;

        case IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT:
            {
            SystemDmaController *pController;
            DEBUGMSG(ZONE_INFO, (L"DMA: IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT\r\n"));

            if (pInBuffer == NULL || inSize != sizeof(IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: DMA_IOControl: "
                    L"IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT invalid parameters\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            //assume this is a system interrupt
            pController = pDevice->pSystemController;
            
            EnterCriticalSection(&pDevice->cs);
            rc = pController->DI_ReleaseInterrupt(*((DWORD*)pInBuffer));
            LeaveCriticalSection(&pDevice->cs);
            }
            break;
            
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-DMA_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//------------------------------------------------------------------------------


