// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: spi.c
//
#include "omap.h"
#include "omap_dvfs.h"
#include "omap_sdma_regs.h"
#include <omap_mcspi_regs.h>
#include <omap_sdma_utility.h>

#include "soc_cfg.h"
#include <initguid.h>
#include "sdk_padcfg.h"
#include <sdk_spi.h>
#include <spi_priv.h>
#include <oal_clock.h>

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

DBGPARAM dpCurSettings = {
    L"SPI", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST",         L"DMA",         L"DVFS",        L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define SPI_DEVICE_COOKIE       'spiD'
#define SPI_INSTANCE_COOKIE     'spiI'
#define PM_TRACE 0
#define TIMERTHREAD_TIMEOUT     5
#define TIMERTHREAD_PRIORITY    152


//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
	{ 
        L"Port", PARAM_DWORD, TRUE, offset(SPI_DEVICE, dwPort),
        fieldsize(SPI_DEVICE, dwPort), (VOID*)NULL
    }, { 
        L"Timeout", PARAM_DWORD, FALSE, offset(SPI_DEVICE, timeout),
        fieldsize(SPI_DEVICE, timeout), (VOID*)500
    }, { 
        L"TxBufferSize", PARAM_DWORD, FALSE, offset(SPI_DEVICE, dwTxBufferSize),
        fieldsize(SPI_DEVICE, dwTxBufferSize), (VOID*)0x4000
    }, { 
        L"RxBufferSize", PARAM_DWORD, FALSE, offset(SPI_DEVICE, dwRxBufferSize),
        fieldsize(SPI_DEVICE, dwRxBufferSize), (VOID*)0x4000
    }, { 
        L"DVFSAsyncEventName", PARAM_DWORD, FALSE, offset(SPI_DEVICE, szDVFSAsyncEventName),
        fieldsize(SPI_DEVICE, szDVFSAsyncEventName), (VOID*)NULL
    }, {
        L"ActivityTimeout", PARAM_DWORD, FALSE, offset(SPI_DEVICE, nActivityTimeout),
        fieldsize(SPI_DEVICE, nActivityTimeout), (VOID*)TIMERTHREAD_TIMEOUT
    }
};

//------------------------------------------------------------------------------
//
//  Function:  CheckAndHaltAllDma
//
//  Called by dma read/write routines to synchronize dma activity
//  with current DVFS transitions.
//
VOID CheckAndHaltAllDma(SPI_INSTANCE *pInstance, BOOL bHalt)
{
    SPI_DEVICE *pDevice = pInstance->pDevice;
    
    DEBUGMSG(ZONE_FUNCTION, (
        L"+CheckAndHaltAllDma(0x%08x, %d)\r\n", 
        pInstance, bHalt
        ));

    EnterCriticalSection(&pDevice->csDVFS);

    if (pDevice->nActiveDmaCount == 0 && bHalt == TRUE)
        {
        if (pDevice->hDVFSAsyncEvent != NULL)
            {
            SetEvent(pDevice->hDVFSAsyncEvent);
            }
        }
    
    LeaveCriticalSection(&pDevice->csDVFS);

    DEBUGMSG(ZONE_FUNCTION, (L"-CheckAndHaltAllDma()\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  PreDmaActivation
//
//  Called by dma read/write routines to synchronize dma activity
//  with current DVFS transitions.
//
void PreDmaActivation(SPI_INSTANCE *pInstance)
{
    SPI_DEVICE *pDevice = pInstance->pDevice;
    
    DEBUGMSG(ZONE_FUNCTION, (
        L"+PreDmaActivation(0x%08x)\r\n", pInstance
    ));
    
    // this operation needs to be atomic to handle a corner case
    EnterCriticalSection(&pDevice->csDVFS);
    
    // check and wait for DVFS activity to complete
    if (pDevice->bDVFSActive == TRUE)
        {
        // avoid deadlock's while waiting for dvfs transitions
        //
        LeaveCriticalSection(&pDevice->csDVFS);
        WaitForSingleObject(pDevice->hDVFSInactiveEvent, INFINITE);
        EnterCriticalSection(&pDevice->csDVFS);
        }
    InterlockedIncrement(&pDevice->nActiveDmaCount);

    LeaveCriticalSection(&pDevice->csDVFS);

    DEBUGMSG(ZONE_FUNCTION, (
        L"-PreDmaActivation()\r\n"
        ));
}

//------------------------------------------------------------------------------
//
//  Function:  PostDmaDeactivation
//
//  Called by dma read/write routines to synchronize dma activity
//  with current DVFS transitions.
//
void PostDmaDeactivation(SPI_INSTANCE *pInstance)
{
    SPI_DEVICE *pDevice = pInstance->pDevice;
    
    DEBUGMSG(ZONE_FUNCTION, (
        L"+PostDmaDeactivation(0x%08x)\r\n", pInstance
    ));
    
    ASSERT(pDevice->nActiveDmaCount > 0);

    // this operation needs to be atomic to handle a corner case
    EnterCriticalSection(&pDevice->csDVFS);
    
    // check if all dma's are inactive and signal ack event if so
    InterlockedDecrement(&pDevice->nActiveDmaCount);
    if (pDevice->bDVFSActive == TRUE && pDevice->nActiveDmaCount <= 0)
        {
        if (pDevice->hDVFSAsyncEvent != NULL) 
        {
            SetEvent(pDevice->hDVFSAsyncEvent);
        }
        }

    LeaveCriticalSection(&pDevice->csDVFS);

    DEBUGMSG(ZONE_FUNCTION, (
        L"-PostDmaDeactivation()\r\n"
        ));
}


//------------------------------------------------------------------------------
//
//  Function:  SPIPowerTimerThread
//
//  timeout thread, checks to see if the power can be disabled.
//
DWORD 
SPIPowerTimerThread(
    void *pv
    )
{
    DWORD nTimeout = INFINITE;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)(pv);
	BOOL exp = TRUE;

    while (exp)
        {
        WaitForSingleObject(pDevice->hTimerEvent, nTimeout);

        if (pDevice->bExitThread == TRUE) break;

        // serialize access to power state changes
		WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

        // by the time this thread got the lock hTimerEvent may 
        // have gotten resignaled.  Clear the event to  make
        // sure the activity timer thread isn't awaken prematurely
        //
        ResetEvent(pDevice->hTimerEvent);

        // check if we need to reset the timer
        if (pDevice->nPowerCounter == 0)
            {
            // We disable the power only when this thread
            // wakes-up twice in a row with no power state
            // change to D0.  This is achieved by using the
            // bDisablePower flag to determine if power state
            // changed since the last time this thread woke-up
            //
            if (pDevice->bDisablePower == TRUE)
                {
                // force idle
                OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG,
                       MCSPI_SYSCONFIG_ENAWAKEUP);

                // Clear interrupts.
                OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);
                
                //EnableDeviceClocks(pDevice->deviceID, FALSE);

                SetDevicePowerState(pDevice->hParentBus, D4, 0);                
                pDevice->powerState = D4;
                nTimeout = INFINITE;
                SetEvent(pDevice->hDeviceOffEvent);
                }
            else
                {
                // wait for activity time-out before shutting off power.
                pDevice->bDisablePower = TRUE;
                nTimeout = pDevice->nActivityTimeout;
                }
            }
        else
            {
            // disable power and wait for timer to get restarted
            nTimeout = INFINITE;
            }
		ReleaseMutex(pDevice->hControllerMutex);
        }

    return 1;
}


//------------------------------------------------------------------------------
//
//  Function:  SetSPIPower
//
//  centeral function to enable power to SPI bus.
//
BOOL
SetSPIPower(
	SPI_DEVICE              *pDevice,
	CEDEVICE_POWER_STATE    state
	)
{
    // enable power when the power state request is D0-D2
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    if (state < D3)
        {
        if (pDevice->powerState >= D3)
            {
			//EnableDeviceClocks(pDevice->deviceID, TRUE);
            SetDevicePowerState(pDevice->hParentBus, D0, NULL);
            pDevice->powerState = D0;            

            // smart idle
            OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG,
                MCSPI_SYSCONFIG_AUTOIDLE |
                MCSPI_SYSCONFIG_SMARTIDLE |
                MCSPI_SYSCONFIG_ENAWAKEUP);
            }
        pDevice->bDisablePower = FALSE;
        pDevice->nPowerCounter++;
        }
    else
        {
        pDevice->nPowerCounter--;
        if (pDevice->nPowerCounter == 0)
            {
            if ((pDevice->hTimerEvent != NULL) && (pDevice->systemState != D4))
                {
                // Reset the device OFF event, set
                // after the device is put to D4 state
                ResetEvent(pDevice->hDeviceOffEvent);

                SetEvent(pDevice->hTimerEvent);
                }
            else
                {
                // force idle
                OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG,
                       MCSPI_SYSCONFIG_ENAWAKEUP);

                // Clear interrupts.
                OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);

                //EnableDeviceClocks(pDevice->deviceID, FALSE);
                SetDevicePowerState(pDevice->hParentBus, D4, NULL);

                pDevice->powerState = D4;
                }
            }
        }
    
	ReleaseMutex(pDevice->hControllerMutex);
    return TRUE;
}



//------------------------------------------------------------------------------
//
//  Function:  SPI_Init
//
//  Called by device manager to initialize device.
//
DWORD SPI_Init(LPCTSTR szContext, LPCVOID pBusContext)
{
    DWORD rc = (DWORD)NULL;
    SPI_DEVICE *pDevice = NULL;
    PHYSICAL_ADDRESS pa;
    DWORD dwCount=0;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+SPI_Init(%s, 0x%08x)\r\n", szContext, pBusContext
    ));

    // Create device structure
    pDevice = (SPI_DEVICE *)LocalAlloc(LPTR, sizeof(SPI_DEVICE));
    if (pDevice == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed allocate SPI controller structure\r\n"
        ));
        goto cleanUp;
    }
    memset(pDevice, 0, sizeof(SPI_DEVICE));

    // Set cookie
    pDevice->cookie = SPI_DEVICE_COOKIE;
    pDevice->powerState = D4;

    // initialize dvfs variables
    pDevice->bDVFSActive = FALSE;
    pDevice->nActiveDmaCount = 0;
    pDevice->hDVFSAsyncEvent = NULL;
    pDevice->szDVFSAsyncEventName[0] = _T('\0');

    // Initalize DVFS critical section
    InitializeCriticalSection(&pDevice->csDVFS);
	// Initialize controller mutex (needs to be mutex to lock access for multiple transactions)
	pDevice->hControllerMutex = CreateMutex(NULL, FALSE, NULL);
	if (pDevice->hControllerMutex == NULL)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Error creating mutex!\r\n"));
		goto cleanUp;
	}

    // Read device parameters
    if (GetDeviceRegistryParams(
        szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams
    ) != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed read SPI driver registry parameters\r\n"
        ));
        goto cleanUp;
    }

    // Create DVFS async dvfs handles if necessary
    pDevice->hDVFSInactiveEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (_tcslen(pDevice->szDVFSAsyncEventName) > 0)
        {
        pDevice->hDVFSAsyncEvent = CreateEvent(NULL, TRUE, FALSE, 
                                        pDevice->szDVFSAsyncEventName
                                        );        
        }

    // Open parent bus
    pDevice->hParentBus = CreateBusAccessHandle(szContext);
    if (pDevice->hParentBus == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed open parent bus driver\r\n"
        ));
        goto cleanUp;
    }

    // start timer thread
    pDevice->hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hTimerEvent != NULL)
        {
        pDevice->hTimerThread = CreateThread(NULL, 0, SPIPowerTimerThread, 
            pDevice, 0, NULL
            );
        
        if (pDevice->hTimerThread != NULL)
            {
            CeSetThreadPriority(pDevice->hTimerThread, TIMERTHREAD_PRIORITY);
            }
        }

    // Create an Event to wait for Device OFF on Suspend
    pDevice->hDeviceOffEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hDeviceOffEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed to create Device Off Event\r\n"
        ));        
	}
	
	// Retrieve device ID
	pDevice->deviceID = SOCGetMCSPIDeviceByBus(pDevice->dwPort);
	if (pDevice->deviceID == OMAP_DEVICE_NONE)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed to find device ID for this SPI controller\r\n"
        ));
        goto cleanUp;
	}
	

    
	// Retrieve IRQ from device
	pDevice->irq = GetIrqByDevice(pDevice->deviceID, NULL);
	if (pDevice->irq == (DWORD)-1)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed to find IRQ number for this SPI controller\r\n"
        ));
        goto cleanUp;
	}

    // Map SPI controller
	pa.QuadPart = GetAddressByDevice(pDevice->deviceID);
    pDevice->pSPIRegs = MmMapIoSpace(pa, sizeof(OMAP_MCSPI_REGS), FALSE);
    if (pDevice->pSPIRegs == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed map SPI controller registers\r\n"
        ));
        goto cleanUp;
    }

    // Map SPI interrupt
    if (!KernelIoControl(
        IOCTL_HAL_REQUEST_SYSINTR, &pDevice->irq, sizeof(pDevice->irq), 
        &pDevice->sysIntr, sizeof(pDevice->sysIntr), NULL
    )) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed map SPI controller interrupt\r\n"
        ));
        goto cleanUp;
    }
    
    // Set hardware to full power    
    SetSPIPower(pDevice, D0);

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"Failed create interrupt event\r\n"
        ));
        goto cleanUp;
    }

    // Initialize interrupt
    if (!InterruptInitialize(pDevice->sysIntr, pDevice->hIntrEvent, NULL, 0)) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Init: "
            L"InterruptInitialize failed\r\n"
        ));
        goto cleanUp;
    }
	
    // Reset the SPI controller
    SETREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_SOFTRESET);

    // Wait until resetting is done
    while ( !(INREG32(&pDevice->pSPIRegs->MCSPI_SYSSTATUS) & MCSPI_SYSSTATUS_RESETDONE)) {
        Sleep (1);

        if (dwCount++>0x100)
        {
            // Break out dead lock, something is wrong.
            ERRORMSG (TRUE, (TEXT("SPI: ERROR holding in reset.\n")));
            return FALSE;
        }
    }

    // Disable all interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQENABLE, 0);

    // Clear interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);


    // Setup Module Control as master and eSpiMode to unknown so we can change it later.
    pDevice->eSpiMode = UNKNOWN;
    OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, 0);

    // Return non-null value
    rc = (DWORD)pDevice;

    // Set this driver to internal suspend mode
    OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_AUTOIDLE |
        MCSPI_SYSCONFIG_SMARTIDLE | MCSPI_SYSCONFIG_ENAWAKEUP); 
    SetSPIPower(pDevice, D4);

    if (!RequestDevicePads(pDevice->deviceID))
    {
        ERRORMSG (TRUE, (TEXT("SPI: RequestDevicePads failed.\n")));
        goto cleanUp;
    }

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0) SPI_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Init(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL SPI_Deinit(DWORD context)
{
    BOOL rc = FALSE;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != SPI_DEVICE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Deinit: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    // Check for open instances
    if (pDevice->instances > 0) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Deinit: "
            L"Deinit with active instance (%d instances active)\r\n",
            pDevice->instances
        ));
        goto cleanUp;
    }

    // stop interrupt thread
    if (pDevice->hTimerThread != NULL)
        {
        pDevice->bExitThread = TRUE;
        SetEvent(pDevice->hTimerEvent);
        WaitForSingleObject(pDevice->hTimerThread, INFINITE);
        CloseHandle(pDevice->hTimerThread);
        pDevice->hTimerThread = NULL;
        }

    // Set hardware to D4 and close parent bus driver
    if (pDevice->hParentBus!= NULL) 
    {
        SetSPIPower(pDevice,  D4);
        CloseBusAccessHandle(pDevice->hParentBus);
    }

    // Unmap SPI controller registers
    if (pDevice->pSPIRegs != NULL)
    {
        MmUnmapIoSpace((VOID*)pDevice->pSPIRegs, sizeof(OMAP_MCSPI_REGS));
    }

    // Release SPI controller interrupt
    if (pDevice->sysIntr != 0) 
	{
        InterruptDisable(pDevice->sysIntr);
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR, &pDevice->sysIntr,
            sizeof(pDevice->sysIntr), NULL, 0, NULL
        );
    }

    // Close interrupt handler
    if (pDevice->hIntrEvent != NULL) CloseHandle(pDevice->hIntrEvent);

    // release dvfs resources
    if (pDevice->hDVFSAsyncEvent != NULL) CloseHandle(pDevice->hDVFSAsyncEvent);

    // Delete critical section
    DeleteCriticalSection(&pDevice->csDVFS);
	
	// Delete mutex
	CloseHandle(pDevice->hControllerMutex);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD SPI_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{
    DWORD rc = (DWORD)NULL;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)context;
    SPI_INSTANCE *pInstance = NULL;

	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+SPI_Open(0x%08x, 0x%08x, 0x%08x\r\n", context, accessCode, shareMode
    ));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != SPI_DEVICE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Open: "
            L"Incorrect context parameter\r\n"
        ));
        goto cleanUp;
    }

    // Create device structure
    pInstance = (SPI_INSTANCE*)LocalAlloc(LPTR, sizeof(SPI_INSTANCE));
    if (pInstance == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Open: "
            L"Failed allocate SPI instance structure\r\n"
        ));
        goto cleanUp;
    }
	
    memset(pInstance, 0, sizeof(SPI_INSTANCE));

    // Set cookie
    pInstance->cookie = SPI_INSTANCE_COOKIE;

    // Save device reference
    pInstance->pDevice = pDevice;

    // Increment number of open instances
    InterlockedIncrement(&pDevice->instances);

    // sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Done...
    rc = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Open(rc = 0x%08x)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_Close
//
//  This function closes the device context.
//
BOOL SPI_Close(DWORD context)
{
    BOOL rc = FALSE;
    SPI_DEVICE *pDevice;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Close(0x%08x)\r\n", context));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Transfer: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    // Shutdown DMA channels
    SpiDmaDeinit(pInstance);

    // Get device context
    pDevice = pInstance->pDevice;

    // sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Decrement number of open instances
    if (InterlockedDecrement(&pDevice->instances) == 0)
    {
        // Get hardware
		WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

        // Set this driver to Active Mode
        SetSPIPower(pDevice, D0);

        // If number of open instances is 0, reset the eSpiMode to unknown and 
        // MCSPI_MODULCTRL to 0 so we can change it later.
        pDevice->eSpiMode = UNKNOWN;
        OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, 0);

        // Set this driver to Suspend Mode
        SetSPIPower(pDevice, D4);

        // Release hardware
		ReleaseMutex(pDevice->hControllerMutex);
    }

    // Free instance structure
    LocalFree(pInstance);

    // Done...
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Close(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_Configure
//
//  This function configures a SPI channel
//
BOOL SPI_Configure(DWORD context, DWORD address, DWORD config)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_Configure Addr = 0x%x  Config = 0x%x\r\n", address, config));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    //  Check channel
    if (address >= MCSPI_MAX_CHANNELS) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: "
            L"Incorrect address paramer\r\n"
        ));
        goto cleanUp;
    }

    // Get Device
    pDevice = pInstance->pDevice;

    if (address > 0)
    {
        switch (pDevice->eSpiMode)
        {
            case SLAVE:
                DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: "
                    L"Incorrect address paramer for slave device\r\n"
                ));
                goto cleanUp;
                break;

            case UNKNOWN:
                pDevice->eSpiMode = MASTER;
                break;

            default:
                break;
        }

    }

    // Set Address and config
    pInstance->address = address;
    pInstance->config  = config;

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    // Setting active instance which is needed for context restore
    pDevice->pActiveInstance = (void*)pInstance;

    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Configure the channel
    switch( address )
    {
        case 0:
            //  Channel 0 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF0);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, config);
            break;

        case 1:
            //  Channel 1 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF1);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, config);
            break;

        case 2:
            //  Channel 2 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF2);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, config);
            break;

        case 3:
            //  Channel 3 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF3);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, config);
            break;

        default:
            break;
    }

    //  Clear out current DMA configuration
    SpiDmaDeinit(pInstance);

    //  Configure DMA if needed
    if( config & (MCSPI_CHCONF_DMAR_ENABLE|MCSPI_CHCONF_DMAW_ENABLE) )
    {
        if( SpiDmaInit(pInstance) == FALSE ) 
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: "
                L"DMA initialization failed\r\n"
            ));

            // Set this driver to Suspend Mode
            SetSPIPower(pDevice, D4);

            // Release hardware
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }
    }

    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
    
    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);

    // Success
    rc = TRUE;

cleanUp:    
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_SetSlaveMode
//
//  This function set the SPI port to act as a slave
//
BOOL SPI_SetSlaveMode(DWORD context)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SSPI_SetSlaveMode.\r\n"));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_SetSlaveMode: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    // Get Device
    pDevice = pInstance->pDevice;

    // once we set eSpiMode to MASTER(someone configured channel 1 to N), we can't 
    // change it back
    if (pDevice->eSpiMode == MASTER)
    {
        goto cleanUp;
    }
   
    if (pDevice->eSpiMode == UNKNOWN)
    {
        // Get hardware
		WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

        // Setting active instance which is needed for context restore
        pDevice->pActiveInstance = (void*)pInstance;


        // Set this driver to Active Mode
        SetSPIPower(pDevice, D0);

        OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_MS_BIT);

        // Set this driver to Suspend Mode
        SetSPIPower(pDevice, D4);

        // Release hardware
		ReleaseMutex(pDevice->hControllerMutex);
    }

    // Success
    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_Read
//
//  This function reads data from the device identified by the open context.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_RXONLY channels
//
DWORD
SPI_Read(
    DWORD context, 
    VOID *pBuffer, 
    DWORD size
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Read(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Read: "
            L"Incorrect context paramer\r\n"
        ));
        return 0;
    }

    //  Call DMA function if DMA enabled
    if( SpiDmaRxEnabled(pInstance) )
    {
        return SPI_DmaRead(context, pBuffer, size);
    }

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;

    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    
    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; )
    {
        //  Wait for RX register to fill
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pSPIChannelRegs->MCSPI_CHSTATUS) & MCSPI_CHSTAT_RX_FULL))
        {
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 )
        {
            DEBUGMSG(ZONE_ERROR, (L"SPI_Read timeout\r\n"));

	        // Disable the channel.
			if (!pInstance->exclusiveAccess)
	        	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            // Set this driver to Suspend Mode
            SetSPIPower(pDevice, D4);

            // Release hardware
			ReleaseMutex(pDevice->hControllerMutex);

            goto clean;
        }


        //  Read in data on byte/word/dword boundaries
        if( dwWordLen > 16 )
        {
            *(UINT32*)(&pData[dwCount]) = INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT32);
        }
        else if( dwWordLen > 8 )
        {
            *(UINT16*)(&pData[dwCount]) = (UINT16) INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT16);
        }
        else
        {
            *(UINT8*)(&pData[dwCount]) = (UINT8) INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT8);
        }   
    }
    
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);

    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);

clean:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Read(rc = %d)\r\n", dwCount));
	return dwCount;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_DmaRead
//
//  This function reads data from the device using DMA.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_RXONLY channels
//
DWORD
SPI_DmaRead(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwWordSize;
    DWORD dwCount = 0;
    DWORD dwDmaSize;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_DMA, (L"+SPI_DmaRead(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // We don't need to check instance since it's already checked by caller    
    ASSERT(pInstance->cookie == SPI_INSTANCE_COOKIE);

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;


    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 )
    {
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    }
    else if( dwWordLen > 8 )
    {
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    }
    else
    {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pInstance->pDevice->dwRxBufferSize) ? size : pInstance->pDevice->dwRxBufferSize;
    dwDmaSize /= dwWordSize;

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);


    //  Write out all the data; loop if necessary
    for( dwCount = 0; dwCount < size; )
    {
        // Set the DMA transfer size
        DmaSetElementAndFrameCount(&pInstance->rxDmaInfo, (UINT16) dwDmaSize, 1);
    
        DEBUGMSG(ZONE_DMA, (L" SPI_DmaRead: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));


        // Start the DMA
        PreDmaActivation(pInstance);
        DmaStart(&pInstance->rxDmaInfo);

        //  Wait for DMA done interrupt or timeout
        if( WaitForSingleObject(pInstance->hRxDmaIntEvent, pInstance->pDevice->timeout) != WAIT_OBJECT_0)
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaRead: "
                L"DMA interrupt timeout\r\n"
            ));

            //  Cleanup
            DmaStop(&pInstance->rxDmaInfo);
            DmaInterruptDone(pInstance->hRxDmaChannel);
            PostDmaDeactivation(pInstance);
            goto cleanUp;
        }

        // Get and clear the status
        dwDmaStatus = DmaGetStatus(&pInstance->rxDmaInfo);
        DmaClearStatus(&pInstance->rxDmaInfo, dwDmaStatus);

        DEBUGMSG(ZONE_DMA, (L" SPI_DmaRead: DMA Status = %x\r\n", dwDmaStatus));

        // Stop the DMA
        DmaInterruptDone(pInstance->hRxDmaChannel);
        DmaStop(&pInstance->rxDmaInfo);
        PostDmaDeactivation(pInstance);


        // Copy the data from the DMA buffer
        memcpy(&pData[dwCount], pInstance->pRxDmaBuffer, dwDmaSize*dwWordSize);

        //  Update amount transferred
        dwCount += dwDmaSize*dwWordSize;
    }

cleanUp:
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);
    
    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
    
    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);


    DEBUGMSG(ZONE_DMA, (L"-SPI_DmaRead(rc = %d)\r\n", dwCount));
	return dwCount;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_Write
//
//  This function writes data to the device.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXONLY channels
//
DWORD
SPI_Write(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Write(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Write: "
            L"Incorrect context paramer\r\n"
        ));
        return 0;
    }

    //  Call DMA function if DMA enabled
    if( SpiDmaTxEnabled(pInstance) )
    {
        return SPI_DmaWrite(context, pBuffer, size);
    }

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;


    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    
    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; )
    {
        //  Write out data on byte/word/dword boundaries
        if( dwWordLen > 16 )
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT32*)(&pData[dwCount]));   
            dwCount += sizeof(UINT32);
        }
        else if( dwWordLen > 8 )
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT16*)(&pData[dwCount]));   
            dwCount += sizeof(UINT16);
        }
        else
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT8*)(&pData[dwCount]));   
            dwCount += sizeof(UINT8);
        }   

        //  Wait for TX register to empty out
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pSPIChannelRegs->MCSPI_CHSTATUS) & MCSPI_CHSTAT_TX_EMPTY))
        {
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 )
        {
            DEBUGMSG(ZONE_ERROR, (L"SPI_Write timeout\r\n"));

	        // Disable the channel.
			if (!pInstance->exclusiveAccess)
	        	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            // Set this driver to Suspend Mode
            SetSPIPower(pDevice, D4);

            // Release hardware
			ReleaseMutex(pDevice->hControllerMutex);

            goto clean;
        }
    }
    
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);

    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);


clean:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Write(rc = %d)\r\n", dwCount));
	return dwCount;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_DmaWrite
//
//  This function writes data to the device using DMA.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXONLY channels
//
DWORD
SPI_DmaWrite(
    DWORD context, 
    VOID *pBuffer,
    DWORD size
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwWordSize;
    DWORD dwCount = 0;
    DWORD dwDmaSize;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_DMA, (L"+SPI_DmaWrite(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // We don't need to check instance since it's already checked by caller    
    ASSERT(pInstance->cookie == SPI_INSTANCE_COOKIE);

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;


    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 )
    {
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    }
    else if( dwWordLen > 8 )
    {
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    }
    else
    {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pInstance->pDevice->dwTxBufferSize) ? size : pInstance->pDevice->dwTxBufferSize;
    dwDmaSize /= dwWordSize;

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    //  Write out all the data; loop if necessary
    for( dwCount = 0; dwCount < size; )
    {
        // Copy the data to the DMA buffer
        memcpy(pInstance->pTxDmaBuffer, &pData[dwCount], dwDmaSize*dwWordSize);

        // Set the DMA transfer size
        DmaSetElementAndFrameCount(&pInstance->txDmaInfo, (UINT16) dwDmaSize, 1);
    
        DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));

        // Start the DMA
        PreDmaActivation(pInstance);
        DmaStart(&pInstance->txDmaInfo);

        //  Wait for DMA done interrupt or timeout
        if( WaitForSingleObject(pInstance->hTxDmaIntEvent, pInstance->pDevice->timeout) != WAIT_OBJECT_0)
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaWrite: "
                L"DMA interrupt timeout, ABC\r\n"
            ));

            //  Cleanup
            DmaStop(&pInstance->txDmaInfo);
            DmaInterruptDone(pInstance->hTxDmaChannel);
            PostDmaDeactivation(pInstance);
            goto cleanUp;
        }

        // Get and clear the status
        dwDmaStatus = DmaGetStatus(&pInstance->txDmaInfo);
        DmaClearStatus(&pInstance->txDmaInfo, dwDmaStatus);

        DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Status = %x\r\n", dwDmaStatus));

        // Stop the DMA
        DmaInterruptDone(pInstance->hTxDmaChannel);
        DmaStop(&pInstance->txDmaInfo);
        PostDmaDeactivation(pInstance);

        //  Update amount transferred
        dwCount += dwDmaSize*dwWordSize;
    }
    
cleanUp:
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);

    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);

    DEBUGMSG(ZONE_DMA, (L"-SPI_DmaWrite(rc = %d)\r\n", dwCount));
    return dwCount;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_AsyncWriteRead
//
//  This function setup DMA writes and reads data to/from the device.
//  Both buffers must be the same length.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
//
DWORD
SPI_AsyncWriteRead(
    DWORD context,
    DWORD size,
    VOID *pOutBuffer,
    VOID *pInBuffer
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pOutData = (UCHAR*)pOutBuffer;
    DWORD dwWordLen;
    DWORD dwWordSize;
    DWORD dwDmaSize;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_AsyncWriteRead(0x%08x, 0x%08x)\r\n", context, size));
  
    // We don't need to check instance since it's already checked by caller
    ASSERT(pInstance->cookie == SPI_INSTANCE_COOKIE);

    if ((SpiDmaTxEnabled(pInstance) == FALSE) || (SpiDmaRxEnabled(pInstance) == FALSE) ||
        (pInstance->pDevice->dwTxBufferSize < size) || (pInstance->pDevice->dwRxBufferSize < size))
    {
        return 0;
    }

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;


    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 )
    {
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    }
    else if( dwWordLen > 8 )
    {
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    }
    else
    {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pDevice->dwTxBufferSize) ? size : pDevice->dwTxBufferSize;
    dwDmaSize /= dwWordSize;

    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);

    // Enable the channel
    SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

   	// Setup transmit
    // Copy the data to the DMA buffer
    memcpy(pInstance->pTxDmaBuffer, pOutData, dwDmaSize*dwWordSize);

    // Set the DMA transfer size
    DmaSetElementAndFrameCount(&pInstance->txDmaInfo, (UINT16) dwDmaSize, 1);

    DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));

	// Setup receive DMA
    DmaSetElementAndFrameCount(&pInstance->rxDmaInfo, (UINT16) dwDmaSize, 1);
	
    // Start the DMA
    PreDmaActivation(pInstance);

    DmaStart(&pInstance->rxDmaInfo);
    DmaStart(&pInstance->txDmaInfo);

UNREFERENCED_PARAMETER(pInBuffer);

    return size;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_WaitForTransferComplete
//
//  This function waits for DMA writes and reads data to/from the device to be completed.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
//
DWORD
SPI_WaitForAsyncWriteReadComplete(
    DWORD context,
    DWORD size,
    VOID *pInBuffer
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_WaitForAsyncWriteReadComplete(0x%08x)\r\n", context));

    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;

    //  Wait for DMA done interrupt or timeout
    if( WaitForSingleObject(pInstance->hRxDmaIntEvent, pDevice->timeout) != WAIT_OBJECT_0)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaRead: DMA interrupt timeout\r\n"));

        DUMP_DMA_REGS(pInstance->hRxDmaChannel, L"--Rx->");

        //  Cleanup
        DmaStop(&pInstance->rxDmaInfo);
        DmaInterruptDone(pInstance->hRxDmaChannel);
        PostDmaDeactivation(pInstance);
        goto cleanUp;
    }

    // Get and clear the status
    dwDmaStatus = DmaGetStatus(&pInstance->rxDmaInfo);
    DmaClearStatus(&pInstance->rxDmaInfo, dwDmaStatus);

    DEBUGMSG(ZONE_DMA, (L" SPI_DmaRead: DMA Status = %x\r\n", dwDmaStatus));

    // Stop the DMA
    DmaInterruptDone(pInstance->hRxDmaChannel);
    DmaStop(&pInstance->rxDmaInfo);

    //  Wait for DMA done interrupt or timeout
    if( WaitForSingleObject(pInstance->hTxDmaIntEvent, pDevice->timeout) != WAIT_OBJECT_0)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaWrite: DMA interrupt timeout, ABC\r\n"));

        DUMP_DMA_REGS(pInstance->hTxDmaChannel, L"--Tx->");

        DmaStop(&pInstance->txDmaInfo);
        DmaInterruptDone(pInstance->hTxDmaChannel);
        PostDmaDeactivation(pInstance);
        goto cleanUp;
    }

    // Get and clear the status
    dwDmaStatus = DmaGetStatus(&pInstance->txDmaInfo);
    DmaClearStatus(&pInstance->txDmaInfo, dwDmaStatus);

    DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Status = %x\r\n", dwDmaStatus));

    // Stop the DMA
    DmaInterruptDone(pInstance->hTxDmaChannel);
    DmaStop(&pInstance->txDmaInfo);

    // Copy the data from the DMA buffer
    memcpy(pInBuffer, pInstance->pRxDmaBuffer, size);
	
    PostDmaDeactivation(pInstance);

cleanUp:
    // Disable the channel.
    CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);

    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);
	
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_DmaWriteRead(rc = %d)\r\n", size));

    return size;
	
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_DmaWriteRead
//
//  This function writes and reads data to/from the device using DMA.
//  Both buffers must be the same length.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
//
DWORD
SPI_DmaWriteRead(
    DWORD context,
    DWORD size,
    VOID *pOutBuffer,
    VOID *pInBuffer
    )
{
    DWORD   dwCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_DmaWriteRead(0x%08x, 0x%08x)\r\n", context, size));

    dwCount = SPI_AsyncWriteRead(context, size, pOutBuffer, pInBuffer);
    if (dwCount != 0)
    {
        dwCount = SPI_WaitForAsyncWriteReadComplete(context, size, pInBuffer);
    }

    return dwCount;	
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_WriteRead
//
//  This function writes and reads data to/from the device.  
//  Both buffers must be the same length.
//  Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
//
DWORD
SPI_WriteRead(
    DWORD context, 
    DWORD size,
    VOID *pOutBuffer, 
    VOID *pInBuffer
    )
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    OMAP_MCSPI_CHANNEL_REGS *pSPIChannelRegs;
    UCHAR* pInData = (UCHAR*)pInBuffer;
    UCHAR* pOutData = (UCHAR*)pOutBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_WriteRead(0x%08x, 0x%08x)\r\n", context, size));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Write: "
            L"Incorrect context paramer\r\n"
        ));
        return 0;
    }

    //  Call DMA function if DMA enabled
    if( SpiDmaTxEnabled(pInstance) && SpiDmaRxEnabled(pInstance))
    {
        return SPI_DmaWriteRead(context, size, pOutBuffer, pInBuffer);
    }

    // Get pointers to registers
    pDevice = pInstance->pDevice;
    pSPIChannelRegs = pInstance->pSPIChannelRegs;


    // Get the word length of the data
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);
    
    // Get hardware
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    
    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; )
    {
        //  Write out data on byte/word/dword boundaries
        if( dwWordLen > 16 )
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT32*)(&pOutData[dwCount]));   
        }
        else if( dwWordLen > 8 )
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT16*)(&pOutData[dwCount]));   
        }
        else
        {
            OUTREG32(&pSPIChannelRegs->MCSPI_TX, *(UINT8*)(&pOutData[dwCount]));   
        }   

        //  Wait for TX register to empty out
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pSPIChannelRegs->MCSPI_CHSTATUS) & MCSPI_CHSTAT_TX_EMPTY))
        {
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 )
        {
            DEBUGMSG(ZONE_ERROR, (L"SPI_WriteRead write timeout\r\n"));
            
            // Disable the channel.
			if (!pInstance->exclusiveAccess)
        	    CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            // Set this driver to Suspend Mode
            SetSPIPower(pDevice, D4);

            // Release hardware
			ReleaseMutex(pDevice->hControllerMutex);
            goto clean;
        }


        //  Wait for RX register to fill
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pSPIChannelRegs->MCSPI_CHSTATUS) & MCSPI_CHSTAT_RX_FULL))
        {
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 )
        {
            DEBUGMSG(ZONE_ERROR, (L"SPI_WriteRead read timeout\r\n"));
            
            // Disable the channel.
			if (!pInstance->exclusiveAccess)
            	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            // Set this driver to Suspend Mode
            SetSPIPower(pDevice, D4);

            // Release hardware
			ReleaseMutex(pDevice->hControllerMutex);

            goto clean;
        }


        //  Read in data on byte/word/dword boundaries
        if( dwWordLen > 16 )
        {
            *(UINT32*)(&pInData[dwCount]) = INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT32);
        }
        else if( dwWordLen > 8 )
        {
            *(UINT16*)(&pInData[dwCount]) = (UINT16) INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT16);
        }
        else
        {
            *(UINT8*)(&pInData[dwCount]) = (UINT8) INREG32(&pSPIChannelRegs->MCSPI_RX);   
            dwCount += sizeof(UINT8);
        }   
    }
    
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);


    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);

    // Release hardware
	ReleaseMutex(pDevice->hControllerMutex);
    
clean:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_WriteRead(rc = %d)\r\n", dwCount));
    return dwCount;
}

//------------------------------------------------------------------------------
//
//  Function:  CentextRestore()
//
//
BOOL ContextRestore(SPI_INSTANCE *pInstance)
{
    SPI_DEVICE *pDevice = pInstance->pDevice;
    DWORD	dwCount=0;

    // Get Device
    pDevice = pInstance->pDevice;

    SETREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_SOFTRESET);
    // Wait until resetting is done
    while ( !(INREG32(&pDevice->pSPIRegs->MCSPI_SYSSTATUS) & MCSPI_SYSSTATUS_RESETDONE))
        {
        Sleep (1);
        if (dwCount++>0x100)
            {
            // Break out dead lock, something is wrong.
            ERRORMSG (TRUE, (TEXT("SPI: ERROR holding in reset.\n")));
            return FALSE;
            }
        }

    // Disable all interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQENABLE, 0);
    // Clear interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);
    // Setup Module Control as master or slave
    OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, (pDevice->eSpiMode==SLAVE) ? MCSPI_MS_BIT : 0);

    OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_AUTOIDLE |
        MCSPI_SYSCONFIG_SMARTIDLE | MCSPI_SYSCONFIG_ENAWAKEUP);

    // Configure the channel
    switch( pInstance->address)
    {
        case 0:
            //  Channel 0 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF0);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, pInstance->config);
            break;

        case 1:
            //  Channel 1 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF1);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, pInstance->config);
            break;

        case 2:
            //  Channel 2 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF2);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, pInstance->config);
            break;

        case 3:
            //  Channel 3 configuration
            pInstance->pSPIChannelRegs = (OMAP_MCSPI_CHANNEL_REGS*)(&pDevice->pSPIRegs->MCSPI_CHCONF3);
            OUTREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, pInstance->config);
            break;

        default:
            break;
    }

    //  Restore DMA if needed
    if (pInstance->config & (MCSPI_CHCONF_DMAR_ENABLE | MCSPI_CHCONF_DMAW_ENABLE))
        {
        SpiDmaRestore(pInstance);
        }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  SPI_LockController
//
//  Called by external clients to place the controller in single access mode
//
BOOL SPI_LockController(DWORD context, DWORD dwTimeout)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
	UINT32 dwStatus;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_LockController\r\n"));
	
    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_LockController: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }

    // Get Device
    pDevice = pInstance->pDevice;
	
	// Get lock
	dwStatus = WaitForSingleObject(pDevice->hControllerMutex, dwTimeout);
	if (dwStatus != WAIT_OBJECT_0)
	{
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_LockController: Failed getting the hardware lock!  Status 0x%x\r\n", dwStatus));
		goto cleanUp;
	}
	
    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
	
	// Set controller in single access mode
    SETREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_SINGLE_BIT);
	
    // Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
	
	// Set flag indicating we own the controller lock
	pInstance->exclusiveAccess = TRUE;
	
    // Done
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_LockController\r\n"));
	return rc;
}
		
//------------------------------------------------------------------------------
//
//  Function:  SPI_UnlockController
//
//  Called by external clients to release controller from single access mode
//
BOOL SPI_UnlockController(DWORD context)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_UnlockController\r\n"));
	
    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_UnlockController: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }
	
	// Check to see if we have exclusive access
	if (!pInstance->exclusiveAccess)
	{
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_UnlockController: Don't own SPI lock!\r\n"));
		goto cleanUp;
	}

    // Get Device
    pDevice = pInstance->pDevice;
	
    // Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
	
	// Release single access mode
    CLRREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_SINGLE_BIT);
    
	// Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
	
	// Clear lock flag
	pInstance->exclusiveAccess = FALSE;
	
	// Release lock
	if (!ReleaseMutex(pDevice->hControllerMutex))
	{
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_UnlockController: Error releasing lock!  Error code 0x%x\r\n", GetLastError()));
		goto cleanUp;
	}
	
    // Done
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_UnlockController\r\n"));
	return rc;
}
		
//------------------------------------------------------------------------------
//
//  Function:  SPI_EnableChannel
//
//  Called by external clients to enable a channel in single access mode
//	This function both enables the channel and asserts the chip select
//	to the target device
//
BOOL SPI_EnableChannel(DWORD context)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_EnableChannel\r\n"));
	
    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_EnableChannel: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }
	
	// Check to see if we have exclusive access
	if (!pInstance->exclusiveAccess)
	{
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_EnableChannel: Not in single access mode!!\r\n"));
		goto cleanUp;
	}

    // Get Device
    pDevice = pInstance->pDevice;
    
	// Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
	
    // Enable the channel
    SETREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);
	// Assert the chip select
    SETREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, MCSPI_CHCONF_FORCE);
    
	// Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
	
    // Done
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_EnableChannel\r\n"));
	return rc;	
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_DisableChannel
//
//  Called by external clients to disable a channel in single access mode
//	This function both disables the channel and de-asserts the chip select
//	to the target device
//
BOOL SPI_DisableChannel(DWORD context)
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_DisableChannel\r\n"));
	
    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DisableChannel: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
    }
	
	// Check to see if we have exclusive access
	if (!pInstance->exclusiveAccess)
	{
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DisableChannel: Not in single access mode!!\r\n"));
		goto cleanUp;
	}

    // Get Device
    pDevice = pInstance->pDevice;
	
	// Set this driver to Active Mode
    SetSPIPower(pDevice, D0);
	
	// De-assert the chip select
    CLRREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCONF, MCSPI_CHCONF_FORCE);
    // Disable the channel
    CLRREG32(&pInstance->pSPIChannelRegs->MCSPI_CHCTRL, MCSPI_CHCONT_EN);

	// Set this driver to Suspend Mode
    SetSPIPower(pDevice, D4);
	
    // Done
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_DisableChannel\r\n"));
	return rc;	
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_IOControl
//
//  This function sends a command to a device.
//
BOOL SPI_IOControl(
    DWORD context, DWORD dwCode, BYTE *pInBuffer, DWORD inSize, BYTE *pOutBuffer,
    DWORD outSize, DWORD *pOutSize
) {
    BOOL bRetVal = FALSE;
    SPI_DEVICE *pDevice = NULL;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    DEVICE_IFC_SPI ifc;
    IOCTL_SPI_CONFIGURE_IN *pConfig;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+SPI_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    ));

    // Check if we get correct context
    if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_IOControl: "
            L"Incorrect context paramer\r\n"
        ));
        goto clean;
    }
    
    //Get Device
    pDevice = pInstance->pDevice;

    switch (dwCode) {
    case IOCTL_DDK_GET_DRIVER_IFC:
        // We can give interface only to our peer in device process
        if (GetCurrentProcessId() != (DWORD)GetCallerProcess()) {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_IOControl: "
                L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                L"device process (caller process id 0x%08x)\r\n",
                GetCallerProcess()
            ));
            SetLastError(ERROR_ACCESS_DENIED);
            goto clean;
        }
        // Check input parameters
        if (pInBuffer == NULL || inSize < sizeof(GUID)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        if (IsEqualGUID(pInBuffer, &DEVICE_IFC_SPI_GUID)) {
            if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_SPI);
            if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_SPI)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            ifc.context = context;
            ifc.pfnConfigure = SPI_Configure;
            ifc.pfnRead = SPI_Read;
            ifc.pfnWrite = SPI_Write;
            ifc.pfnWriteRead = SPI_WriteRead;
            ifc.pfnAsyncWriteRead = SPI_AsyncWriteRead;
            ifc.pfnWaitForAsyncWriteReadComplete = SPI_WaitForAsyncWriteReadComplete;
            ifc.pfnSetSlaveMode = SPI_SetSlaveMode;
            ifc.pfnLockController = SPI_LockController;
            ifc.pfnUnlockController = SPI_UnlockController;
            ifc.pfnEnableChannel = SPI_EnableChannel;
            ifc.pfnDisableChannel = SPI_DisableChannel;
            if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_SPI))) {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            bRetVal = TRUE;
            break;
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        
    case IOCTL_SPI_CONFIGURE:
        if (pOutSize != NULL) *pOutSize = sizeof(DWORD);
        if (pInBuffer == NULL || inSize < sizeof(IOCTL_SPI_CONFIGURE_IN)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

        pConfig = (IOCTL_SPI_CONFIGURE_IN*) pInBuffer;

        bRetVal = SPI_Configure(context, pConfig->address, pConfig->config);
        break;
    
    case IOCTL_SPI_WRITEREAD:
        if (pOutSize != NULL) *pOutSize = inSize;
        if (pInBuffer == NULL || pOutBuffer == NULL || inSize != outSize ) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

        bRetVal = SPI_WriteRead(context, inSize, pOutBuffer, pInBuffer);
        break;

    case IOCTL_SPI_ASYNC_WRITEREAD:
        if (pOutSize != NULL) *pOutSize = inSize;
        if (pInBuffer == NULL || pOutBuffer == NULL || inSize != outSize ) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

        bRetVal = SPI_AsyncWriteRead(context, inSize, pInBuffer, pOutBuffer);
        break;

    case IOCTL_SPI_ASYNC_WRITEREAD_COMPLETE:
        if (pOutSize != NULL) *pOutSize = inSize;
        if (pOutBuffer == NULL || outSize == 0) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

        bRetVal = SPI_WaitForAsyncWriteReadComplete(context, outSize, pOutBuffer);
        break;

    case IOCTL_SPI_SET_SLAVEMODE:
        bRetVal = SPI_SetSlaveMode(context);
        break;

    case IOCTL_SPI_LOCK_CTRL: 
        if (pInBuffer == NULL || inSize != sizeof(DWORD)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        bRetVal = SPI_LockController(context,*(DWORD*)pInBuffer);
        break;

    case IOCTL_SPI_UNLOCK_CTRL: 
        bRetVal = SPI_UnlockController(context);
        break;
    case IOCTL_SPI_ENABLE_CHANNEL: 
        bRetVal = SPI_EnableChannel(context);
        break;
    case IOCTL_SPI_DISABLE_CHANNEL: 
        bRetVal = SPI_DisableChannel(context);
        break;
  

    case IOCTL_DVFS_OPPNOTIFY:
        {
            DWORD i = 0;
            IOCTL_DVFS_OPPNOTIFY_IN *pIn = (IOCTL_DVFS_OPPNOTIFY_IN*)pInBuffer;

            if (pIn->ffInfo & DVFS_CORE1_PRE_NOTICE)
            {
            pDevice->bDVFSActive = TRUE;
                ResetEvent(pDevice->hDVFSInactiveEvent);                
            CheckAndHaltAllDma(pInstance, TRUE);
            }
            else
            {
            pDevice->bDVFSActive = FALSE;
                SetEvent(pDevice->hDVFSInactiveEvent);
            CheckAndHaltAllDma(pInstance, FALSE);
            }
        bRetVal = TRUE;

            
            DEBUGMSG( ZONE_ERROR, (L"SPI: DVFS Notification: ffInfo=0x%08X, dwCount=%d\r\n",
                pIn->ffInfo, pIn->dwCount));

            while (i < pIn->dwCount)
        {
                pIn->dwCount--;
                DEBUGMSG( ZONE_ERROR, (L"...SPI(new): voltDomain=%d, opp=%d\r\n",
                    pIn->rgOppInfo[i].domain, pIn->rgOppInfo[i].newOpp));

                DEBUGMSG( ZONE_ERROR, (L"...SPI(old): voltDomain=%d, opp=%d\r\n",
                    pIn->rgOppInfo[i].domain, pIn->rgOppInfo[i].oldOpp));
                ++i;
            }
            break;
        }
        break;

    case IOCTL_CONTEXT_RESTORE:
        if (pDevice->pActiveInstance != NULL)
            {
            ContextRestore((SPI_INSTANCE*)pDevice->pActiveInstance);
            }
        else
            {
            DEBUGMSG( ZONE_ERROR, (L"SPI: IOCTL_CONTEXT_RESTORE_NOTIFY FAILED\r\n"));
            }
        break;

    case IOCTL_POWER_CAPABILITIES: 
        if (pOutBuffer && outSize >= sizeof (POWER_CAPABILITIES) && 
            pOutSize) 
            {
                __try 
                    {
                    PPOWER_CAPABILITIES pPowerCaps;
                    pPowerCaps = (PPOWER_CAPABILITIES)pOutBuffer;
     
                    // Only supports D0 and D4 states
                    memset(pPowerCaps, 0, sizeof(*pPowerCaps));
                    pPowerCaps->DeviceDx = DX_MASK(D0)|DX_MASK(D4);;
                    *pOutSize = sizeof(*pPowerCaps);
                    
                    bRetVal = TRUE;
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"exception in spi ioctl\r\n"));
                    }
            }
        break;
#if (_WINCEOSVER<700)
    case IOCTL_POWER_QUERY: 
        if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
            {
            __try 
                {
                CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pOutBuffer;

                if (VALID_DX(ReqDx)) 
                    {
                    bRetVal = TRUE;
                    }

                }
            __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
                }
            }
        break;
#endif
    case IOCTL_POWER_SET: 
        if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
            {
            __try 
                {
                CEDEVICE_POWER_STATE newPowerState = *(PCEDEVICE_POWER_STATE)pOutBuffer;

                // SPI clocks are enabled during read/write and disabled after
                // nActivityTimeout duration. 
                // In D4 request wait untill the SPIPowerTimerThread puts the device 
                // in D4 state
                if (newPowerState == D4)
                    {
                    if (pDevice->powerState != D4)
                        {
                        // Wait till the SPI device is put to OFF state
                        WaitForSingleObject(pDevice->hDeviceOffEvent, INFINITE);
                        }
                    }
                
                pDevice->systemState = newPowerState;
                bRetVal = TRUE;
                }
            __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
                }
        }
        break;

    // gets the current device power state
    case IOCTL_POWER_GET: 
        if (pOutBuffer != NULL && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
            {
            __try 
                {
                *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;

                bRetVal = TRUE;
                }
            __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
                }
            }     
        break;
  
    default:
        ERRORMSG(1, (TEXT("SPI: Unknown IOCTL_xxx(0x%0.8X) \r\n"), dwCode));
        break;
    }

clean:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_IOControl(rc = %d)\r\n", bRetVal));
    return bRetVal;
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_PowerUp
//
//  This function restores power to a device.
//
VOID SPI_PowerUp(DWORD context)
{
	UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  SPI_PowerDown
//
//  This function suspends power to the device.
//
void SPI_PowerDown(DWORD context)
{
	UNREFERENCED_PARAMETER(context);
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
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }

UNREFERENCED_PARAMETER(pReserved);
    return TRUE;
}

//------------------------------------------------------------------------------

