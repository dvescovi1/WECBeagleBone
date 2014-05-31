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
//#include "am33x.h"
#include <am3xx_mcspi_regs.h>
#include <edma_utility.h>

#include "soc_cfg.h"
#include <initguid.h>
#include "sdk_padcfg.h"
#include <sdk_spi.h>
#include <spi_priv.h>
#include <oal_clock.h>
#include <ceddkex.h>

#define CHECK_INSTANCE_CONTEXT(msg) \
if (pInstance == NULL || pInstance->cookie != SPI_INSTANCE_COOKIE) { \
	DEBUGMSG (ZONE_ERROR, (L"ERROR: %s: Incorrect context paramer\r\n", msg)); \
	goto cleanUp; \
}

#define CHECK_EXCLUSIVE_ACCESS(msg) \
if (!pInstance->exclusiveAccess) {  \
	DEBUGMSG (ZONE_ERROR, (L"ERROR: %s: Don't own SPI lock!\r\n", msg)); \
	goto cleanUp; \
}

//------------------------------------------------------------------------------
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
        L"ActivityTimeout", PARAM_DWORD, FALSE, offset(SPI_DEVICE, nActivityTimeout),
        fieldsize(SPI_DEVICE, nActivityTimeout), (VOID*)TIMERTHREAD_TIMEOUT
    }
};

//------------------------------------------------------------------------------
DWORD SPIPowerTimerThread(void *pv)
//	timeout thread, checks to see if the power can be disabled.
{
    DWORD nTimeout = INFINITE;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)(pv);

    for (;;){
        WaitForSingleObject(pDevice->hTimerEvent, nTimeout);

        if (pDevice->bExitThread == TRUE) break;

		WaitForSingleObject(pDevice->hControllerMutex, INFINITE); // serialize access to power state changes

        // by the time this thread got the lock hTimerEvent may 
        // have gotten resignaled.  Clear the event to  make
        // sure the activity timer thread isn't awaken prematurely
        ResetEvent(pDevice->hTimerEvent);

        // check if we need to reset the timer
        if (pDevice->nPowerCounter == 0){
            // We disable the power only when this thread
            // wakes-up twice in a row with no power state
            // change to D0.  This is achieved by using the
            // bDisablePower flag to determine if power state
            // changed since the last time this thread woke-up
            if (pDevice->bDisablePower == TRUE){
                // Clear interrupts.
                OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF); // Clear interrupts.

                EnableDeviceClocks(pDevice->deviceID, FALSE);

                pDevice->powerState = D4;
                nTimeout = INFINITE;
                SetEvent(pDevice->hDeviceOffEvent);
            } else {
                // wait for activity time-out before shutting off power.
                pDevice->bDisablePower = TRUE;
                nTimeout = pDevice->nActivityTimeout;
            }
        } else {
            // disable power and wait for timer to get restarted
            nTimeout = INFINITE;
        }
		ReleaseMutex(pDevice->hControllerMutex);
    }

    return 1;
}


//------------------------------------------------------------------------------
BOOL SetSPIPower(SPI_DEVICE *pDevice, CEDEVICE_POWER_STATE state )
//	central function to enable power to SPI bus.
{
    // enable power when the power state request is D0-D2
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    if (state < D3){
        if (pDevice->powerState >= D3){
			EnableDeviceClocks(pDevice->deviceID, TRUE);
            pDevice->powerState = D0;            

            OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG,
                MCSPI_SYSCONFIG_AUTOIDLE | MCSPI_SYSCONFIG_SMARTIDLE );
        }
        pDevice->bDisablePower = FALSE;
        pDevice->nPowerCounter++;
    } else {
        pDevice->nPowerCounter--;
        if (pDevice->nPowerCounter == 0){
            if ((pDevice->hTimerEvent != NULL) && (pDevice->systemState != D4)) {
                // Reset the device OFF event, set
                // after the device is put to D4 state
                ResetEvent(pDevice->hDeviceOffEvent);
                SetEvent(pDevice->hTimerEvent);
            } else {
                OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);// Clear interrupts.

                EnableDeviceClocks(pDevice->deviceID, FALSE);
                pDevice->powerState = D4;
            }
        }
    }
	ReleaseMutex(pDevice->hControllerMutex);
    return TRUE;
}


//------------------------------------------------------------------------------
DWORD SPI_Init(LPCTSTR szContext, LPCVOID pBusContext)
//	Called by device manager to initialize device.
{
    DWORD rc = (DWORD)NULL;
    SPI_DEVICE *pDevice = NULL;
    PHYSICAL_ADDRESS pa;
    DWORD dwCount=0;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Init(%s, 0x%08x)\r\n", szContext, pBusContext));

    // Create device structure
    pDevice = (SPI_DEVICE *)LocalAlloc(LPTR, sizeof(SPI_DEVICE));
    if (pDevice == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed allocate SPI controller structure\r\n"));
        goto cleanUp;
    }
    memset(pDevice, 0, sizeof(SPI_DEVICE));

    // Set cookie
    pDevice->cookie = SPI_DEVICE_COOKIE;
    pDevice->powerState = D4;

	// Initialize controller mutex (needs to be mutex to lock access for multiple transactions)
	pDevice->hControllerMutex = CreateMutex(NULL, FALSE, NULL);
	if (pDevice->hControllerMutex == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Error creating mutex!\r\n"));
		goto cleanUp;
	}

    // Read device parameters
    if (GetDeviceRegistryParams(
        szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams
    ) != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed read SPI driver registry parameters\r\n"));
        goto cleanUp;
    }

    // start timer thread
    pDevice->hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hTimerEvent != NULL){
        pDevice->hTimerThread = CreateThread(NULL, 0, SPIPowerTimerThread,pDevice, 0, NULL);
        
        if (pDevice->hTimerThread != NULL){
            CeSetThreadPriority(pDevice->hTimerThread, TIMERTHREAD_PRIORITY);
        }
    }

    // Create an Event to wait for Device OFF on Suspend
    pDevice->hDeviceOffEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hDeviceOffEvent == NULL){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed to create Device Off Event\r\n"));        
	}
	
	// Retrieve device ID
	pDevice->deviceID = SOCGetMCSPIDeviceByBus(pDevice->dwPort);
	if (pDevice->deviceID == OMAP_DEVICE_NONE){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed to find device ID for this SPI controller\r\n"));
        goto cleanUp;
	}
    
	// Retrieve IRQ from device
	pDevice->irq = GetIrqByDevice(pDevice->deviceID, NULL);
	if (pDevice->irq == (DWORD)-1){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed to find IRQ number for this SPI controller\r\n"));
        goto cleanUp;
	}

    // Map SPI controller
	pa.QuadPart = GetAddressByDevice(pDevice->deviceID);
    pDevice->pSPIRegs = MmMapIoSpace(pa, sizeof(AM3XX_MCSPI_REGS), FALSE);
    if (pDevice->pSPIRegs == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed map SPI controller registers\r\n"));
        goto cleanUp;
    }

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &pDevice->irq, sizeof(pDevice->irq), 
        &pDevice->sysIntr, sizeof(pDevice->sysIntr), NULL)) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed map SPI controller interrupt\r\n"));
        goto cleanUp;
    }
    
    SetSPIPower(pDevice, D0);

    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Init: Failed create interrupt event\r\n"));
        goto cleanUp;
    }

    if (!InterruptInitialize(pDevice->sysIntr, pDevice->hIntrEvent, NULL, 0)) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Init: InterruptInitialize failed\r\n"));
        goto cleanUp;
    }
	
    // Reset the SPI controller
    SETREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_SOFTRESET);

    // Wait until resetting is done
    while ( !(INREG32(&pDevice->pSPIRegs->MCSPI_SYSSTATUS) & MCSPI_SYSSTATUS_RESETDONE)) {
        Sleep (1);
        if (dwCount++>0x100){
            ERRORMSG (TRUE, (TEXT("SPI: ERROR holding in reset.\n")));
            return FALSE;
        }
    }

    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQENABLE, 0);       // Disable all interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF);  // Clear interrupts.

    // Setup Module Control as master and eSpiMode to unknown so we can change it later.
    pDevice->eSpiMode = UNKNOWN;
    OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, 0);

    rc = (DWORD)pDevice;    // Return non-null value

    // Set this driver to internal suspend mode
    OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_AUTOIDLE | MCSPI_SYSCONFIG_SMARTIDLE); 
    SetSPIPower(pDevice, D4);

    if (!RequestDevicePads(pDevice->deviceID)){
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
BOOL SPI_Deinit(DWORD context)
//	Called by device manager to uninitialize device.
{
    BOOL rc = FALSE;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != SPI_DEVICE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Deinit: Incorrect context paramer\r\n"));
        goto cleanUp;
    }

    // Check for open instances
    if (pDevice->instances > 0) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Deinit: "
            L"Deinit with active instance (%d instances active)\r\n",pDevice->instances));
        goto cleanUp;
    }

    // stop interrupt thread
    if (pDevice->hTimerThread != NULL){
        pDevice->bExitThread = TRUE;
        SetEvent(pDevice->hTimerEvent);
        WaitForSingleObject(pDevice->hTimerThread, INFINITE);
        CloseHandle(pDevice->hTimerThread);
        pDevice->hTimerThread = NULL;
    }

	SetSPIPower(pDevice,  D4);

    if (pDevice->pSPIRegs != NULL){
        MmUnmapIoSpace((VOID*)pDevice->pSPIRegs, sizeof(AM3XX_MCSPI_REGS));
    }

    if (pDevice->sysIntr != 0){
        InterruptDisable(pDevice->sysIntr);
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &pDevice->sysIntr,
            sizeof(pDevice->sysIntr), NULL, 0, NULL);
    }

    if (pDevice->hIntrEvent != NULL) CloseHandle(pDevice->hIntrEvent);

	CloseHandle(pDevice->hControllerMutex);
    LocalFree(pDevice);
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
DWORD SPI_Open(DWORD context, DWORD accessCode, DWORD shareMode)
//	Called by device manager to open a device for reading and/or writing.
{
    DWORD rc = (DWORD)NULL;
    SPI_DEVICE *pDevice = (SPI_DEVICE*)context;
    SPI_INSTANCE *pInstance = NULL;

	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Open(0x%08x, 0x%08x, 0x%08x\r\n", context, accessCode, shareMode));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != SPI_DEVICE_COOKIE) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Open: Incorrect context parameter\r\n"));
        goto cleanUp;
    }

    // Create device structure
    pInstance = (SPI_INSTANCE*)LocalAlloc(LPTR, sizeof(SPI_INSTANCE));
    if (pInstance == NULL) {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: SPI_Open: Failed allocate SPI instance structure\r\n"));
        goto cleanUp;
    }
	
    memset(pInstance, 0, sizeof(SPI_INSTANCE));

    pInstance->cookie = SPI_INSTANCE_COOKIE;
    pInstance->pDevice = pDevice;             // Save device reference
    InterlockedIncrement(&pDevice->instances);// Increment number of open instances 
    ASSERT(pDevice->instances > 0);           // sanity check number of instances

    rc = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Open(rc = 0x%08x)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
BOOL SPI_Close(DWORD context)
//	This function closes the device context.
{
    BOOL rc = FALSE;
    SPI_DEVICE *pDevice;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Close(0x%08x)\r\n", context));

    CHECK_INSTANCE_CONTEXT(L"SPI_Transfer");

    SpiDmaDeinit(pInstance); // Shutdown DMA channels

    pDevice = pInstance->pDevice;
    ASSERT(pDevice->instances > 0); // sanity check number of instances

    // Decrement number of open instances
    if (InterlockedDecrement(&pDevice->instances) == 0){
		WaitForSingleObject(pDevice->hControllerMutex, INFINITE);  // Get hardware
        SetSPIPower(pDevice, D0);                                  // Set this driver to Active Mode
        // If number of open instances is 0, reset the eSpiMode to unknown and 
        // MCSPI_MODULCTRL to 0 so we can change it later.
        pDevice->eSpiMode = UNKNOWN;
        OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, 0);
        SetSPIPower(pDevice, D4);                                  // Set this driver to Suspend Mode
		ReleaseMutex(pDevice->hControllerMutex);                   // Release hardware
    }

    LocalFree(pInstance);
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Close(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
BOOL SPI_Configure(DWORD context, DWORD address, DWORD config)
//	This function configures a SPI channel
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_Configure Addr = 0x%x  Config = 0x%x\r\n", address, config));

    CHECK_INSTANCE_CONTEXT(L"SPI_Configure");

    if (address >= MCSPI_MAX_CHANNELS) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: Incorrect address paramer\r\n"));
        goto cleanUp;
    }

    pDevice = pInstance->pDevice;
    if (address > 0){
        switch (pDevice->eSpiMode){
            case SLAVE:
                DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: Incorrect address paramer for slave device\r\n"));
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

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

    // Setting active instance which is needed for context restore
    pDevice->pActiveInstance = (void*)pInstance;

    SetSPIPower(pDevice, D0);
    
    // Configure the channel
    if (address < 4){
		OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCONF, config);
    }

    //  Clear out current DMA configuration
    SpiDmaDeinit(pInstance);

    //  Configure DMA if needed
    if( config & (MCSPI_CHCONF_DMAR_ENABLE|MCSPI_CHCONF_DMAW_ENABLE) ){
        if( SpiDmaInit(pInstance) == FALSE ) {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_Configure: DMA initialization failed\r\n"));
            SetSPIPower(pDevice, D4);
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }
    }

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);

    rc = TRUE;

cleanUp:    
    return rc;
}

//------------------------------------------------------------------------------
BOOL SPI_SetSlaveMode(DWORD context)
//	This function set the SPI port to act as a slave
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SSPI_SetSlaveMode.\r\n"));

    CHECK_INSTANCE_CONTEXT(L"SPI_SetSlaveMode");

    pDevice = pInstance->pDevice;

    // once we set eSpiMode to MASTER(someone configured channel 1 to N), we can't change it back
    if (pDevice->eSpiMode == MASTER)
        goto cleanUp;
   
    if (pDevice->eSpiMode == UNKNOWN){
		WaitForSingleObject(pDevice->hControllerMutex, INFINITE);

        // Setting active instance which is needed for context restore
        pDevice->pActiveInstance = (void*)pInstance;

        SetSPIPower(pDevice, D0);
        OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_MS_BIT);
        SetSPIPower(pDevice, D4);
		ReleaseMutex(pDevice->hControllerMutex);
    }

    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
DWORD SPI_Read(DWORD context, VOID *pBuffer, DWORD size)
//	This function reads data from the device identified by the open context.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_RXONLY channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Read(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    CHECK_INSTANCE_CONTEXT(L"SPI_Read");

    if( SpiDmaRxEnabled(pInstance) ){
        return SPI_DmaRead(context, pBuffer, size);
    }

    pDevice   = pInstance->pDevice;
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config); // Get the word length of the data

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);
    
	if (!pInstance->exclusiveAccess)     // Enable the channel
		SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; ){
        //  Wait for RX register to fill
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHSTATUS) & 
			                                                    MCSPI_CHSTAT_RX_FULL)){
            StallExecution(1);
            dwWait--;
        }
        
        if( dwWait == 0 ){          //  Check if timeout occured
            DEBUGMSG(ZONE_ERROR, (L"SPI_Read timeout\r\n"));
 	        
			SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);
 			if (!pInstance->exclusiveAccess)       // Disable the channel.
	        	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL,
	        	                                                 MCSPI_CHCONT_EN);

            SetSPIPower(pDevice, D4);
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }

        //  Read in data on byte/word/dword boundaries
        if( dwWordLen > 16 ){
            *(UINT32*)(&pData[dwCount]) = INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT32);
        } else if( dwWordLen > 8 ) {
            *(UINT16*)(&pData[dwCount]) = (UINT16) INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT16);
        } else {
            *(UINT8*)(&pData[dwCount]) = (UINT8) INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT8);
        }   
    }
    
	if (!pInstance->exclusiveAccess)          // Disable the channel.
    	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_Read(rc = %d)\r\n", dwCount));
	return dwCount;
}

//------------------------------------------------------------------------------
DWORD SPI_DmaRead(DWORD context, VOID *pBuffer, DWORD size)
//	This function reads data from the device using DMA.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_RXONLY channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwWordSize;
    DWORD dwCount = 0;
    DWORD dwDmaSize;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_DMA, (L"+SPI_DmaRead(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // We don't need to check instance since it's already checked by caller    
    ASSERT(pInstance->cookie == SPI_INSTANCE_COOKIE);

    pDevice = pInstance->pDevice;
    
    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config); // Get the word length of the data

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 ){
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    } else if( dwWordLen > 8 ){
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    } else {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pInstance->pDevice->dwRxBufferSize) ? size : pInstance->pDevice->dwRxBufferSize;
    dwDmaSize /= dwWordSize;

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);
    
	if (!pInstance->exclusiveAccess)         // Enable the channel
    	SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    //  Write out all the data; loop if necessary
    for( dwCount = 0; dwCount < size; ){
        // Set the DMA transfer size
        DmaSetElementAndFrameCount(&pInstance->rxDmaInfo, (UINT16) dwDmaSize, 1);
    
        DEBUGMSG(ZONE_DMA, (L" SPI_DmaRead: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));

        DmaStart(&pInstance->rxDmaInfo);

        //  Wait for DMA done interrupt or timeout
        if( WaitForSingleObject(pInstance->hRxDmaIntEvent, pInstance->pDevice->timeout) != WAIT_OBJECT_0)
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaRead: DMA interrupt timeout\r\n"));
            DmaStop(&pInstance->rxDmaInfo);              //  Cleanup
            DmaInterruptDone(pInstance->hRxDmaChannel);
            goto cleanUp;
        }

        dwDmaStatus = DmaGetStatus(&pInstance->rxDmaInfo);
        DmaClearStatus(&pInstance->rxDmaInfo, dwDmaStatus);

        DEBUGMSG(ZONE_DMA, (L" SPI_DmaRead: DMA Status = %x\r\n", dwDmaStatus));

        DmaInterruptDone(pInstance->hRxDmaChannel);
        DmaStop(&pInstance->rxDmaInfo);

        // Copy the data from the DMA buffer
        memcpy(&pData[dwCount], pInstance->pRxDmaBuffer, dwDmaSize*dwWordSize);

        dwCount += dwDmaSize*dwWordSize;   //  Update amount transferred
    }

cleanUp:
	if (!pInstance->exclusiveAccess)   // Disable the channel.
    	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);
    
    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);

    DEBUGMSG(ZONE_DMA, (L"-SPI_DmaRead(rc = %d)\r\n", dwCount));
	return dwCount;
}


//------------------------------------------------------------------------------
DWORD SPI_Write(DWORD context, VOID *pBuffer, DWORD size)
//	This function writes data to the device.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_TXONLY channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_Write(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    CHECK_INSTANCE_CONTEXT(L"SPI_Write");

    if( SpiDmaTxEnabled(pInstance) ){
        return SPI_DmaWrite(context, pBuffer, size);
    }

    pDevice = pInstance->pDevice;

    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config);   // Get the word length of the data

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);
    
  	if (!pInstance->exclusiveAccess)   // Enable the channel
    	SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; ){
        //  Write out data on byte/word/dword boundaries
        if( dwWordLen > 16 ){
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT32*)(&pData[dwCount]));   
            dwCount += sizeof(UINT32);
        } else if( dwWordLen > 8 ){
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT16*)(&pData[dwCount]));   
            dwCount += sizeof(UINT16);
        } else {
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT8*)(&pData[dwCount]));   
            dwCount += sizeof(UINT8);
        }   

        dwWait = pDevice->timeout;    //  Wait for TX register to empty out
        while(dwWait && !(INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHSTATUS) & MCSPI_CHSTAT_TX_EMPTY)){
			StallExecution(1);
            dwWait--;
        }

        if( dwWait == 0 ) { //  Check if timeout occured
            DEBUGMSG(ZONE_ERROR, (L"SPI_Write timeout\r\n"));
			if (!pInstance->exclusiveAccess) // Disable the channel.
	        	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            SetSPIPower(pDevice, D4);
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }
    }
    
	if (!pInstance->exclusiveAccess) // Disable the channel.
    	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);

cleanUp:
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
DWORD SPI_DmaWrite(DWORD context, VOID *pBuffer, DWORD size)
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    UCHAR* pData = (UCHAR*)pBuffer;
    DWORD dwWordLen;
    DWORD dwWordSize;
    DWORD dwCount = 0;
    DWORD dwDmaSize;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_DMA, (L"+SPI_DmaWrite(0x%08x, 0x%08x, 0x%08x)\r\n", context, pBuffer, size));

    // We don't need to check instance since it's already checked by caller    
    ASSERT(pInstance->cookie == SPI_INSTANCE_COOKIE);

    pDevice = pInstance->pDevice;

    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config); // Get the word length of the data

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 ) {
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    } else if( dwWordLen > 8 ) {
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    } else {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pInstance->pDevice->dwTxBufferSize) ? size : pInstance->pDevice->dwTxBufferSize;
    dwDmaSize /= dwWordSize;

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    //  Write out all the data; loop if necessary
    for( dwCount = 0; dwCount < size; ){
        // Copy the data to the DMA buffer
        memcpy(pInstance->pTxDmaBuffer, &pData[dwCount], dwDmaSize*dwWordSize);
        // Set the DMA transfer size
        DmaSetElementAndFrameCount(&pInstance->txDmaInfo, (UINT16) dwDmaSize, 1);
    
        DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));
        DmaStart(&pInstance->txDmaInfo);

        //  Wait for DMA done interrupt or timeout
        if( WaitForSingleObject(pInstance->hTxDmaIntEvent, pInstance->pDevice->timeout) != WAIT_OBJECT_0)
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaWrite: DMA interrupt timeout, ABC\r\n"));
            DmaStop(&pInstance->txDmaInfo);
            DmaInterruptDone(pInstance->hTxDmaChannel);
            goto cleanUp;
        }

        // Get and clear the status
        dwDmaStatus = DmaGetStatus(&pInstance->txDmaInfo);
        DmaClearStatus(&pInstance->txDmaInfo, dwDmaStatus);

        DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Status = %x\r\n", dwDmaStatus));

        // Stop the DMA
        DmaInterruptDone(pInstance->hTxDmaChannel);
        DmaStop(&pInstance->txDmaInfo);

        //  Update amount transferred
        dwCount += dwDmaSize*dwWordSize;
    }
    
cleanUp:
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);

    DEBUGMSG(ZONE_DMA, (L"-SPI_DmaWrite(rc = %d)\r\n", dwCount));
    return dwCount;
}


//------------------------------------------------------------------------------
DWORD SPI_AsyncWriteRead(DWORD context, DWORD size,
                                   VOID *pOutBuffer, VOID *pInBuffer )
//	This function setup DMA writes and reads data to/from the device.
//	Both buffers must be the same length.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
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

    pDevice = pInstance->pDevice;

    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config); // Get the word length of the data

    //  Ensure that only SPI words elements are DMA'd
    //  Adjust the DMAsize
    if( dwWordLen > 16 ) {
        size = (size/sizeof(UINT32)) * sizeof(UINT32);
        dwWordSize = sizeof(UINT32);
    } else if( dwWordLen > 8 ) {
        size = (size/sizeof(UINT16)) * sizeof(UINT16);
        dwWordSize = sizeof(UINT16);
    } else {
        dwWordSize = sizeof(UINT8);
    }

    //  Get the length of how much can be DMA'd
    dwDmaSize = (size < pDevice->dwTxBufferSize) ? size : pDevice->dwTxBufferSize;
    dwDmaSize /= dwWordSize;

	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);

    SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN); // Enable the channel

   	// Setup transmit 
    memcpy(pInstance->pTxDmaBuffer, pOutData, dwDmaSize*dwWordSize); // Copy the data to the DMA buffer
    DmaSetElementAndFrameCount(&pInstance->txDmaInfo, (UINT16) dwDmaSize, 1); // Set the DMA transfer size

    DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Start (# elements, elementSize) = %d, %d\r\n", dwDmaSize, dwWordSize));

    DmaSetElementAndFrameCount(&pInstance->rxDmaInfo, (UINT16) dwDmaSize, 1); // Setup receive DMA
	
    DmaStart(&pInstance->rxDmaInfo);
    DmaStart(&pInstance->txDmaInfo);

UNREFERENCED_PARAMETER(pInBuffer);

    return size;
}

//------------------------------------------------------------------------------
DWORD SPI_WaitForAsyncWriteReadComplete(
    DWORD context, DWORD size, VOID *pInBuffer)
//	This function waits for DMA writes and reads data to/from the device to be completed.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    DWORD dwDmaStatus;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_WaitForAsyncWriteReadComplete(0x%08x)\r\n", context));

    pDevice = pInstance->pDevice;

    //  Wait for DMA done interrupt or timeout
    if( WaitForSingleObject(pInstance->hRxDmaIntEvent, pDevice->timeout) != WAIT_OBJECT_0)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_DmaRead: DMA interrupt timeout\r\n"));

        DUMP_DMA_REGS(pInstance->hRxDmaChannel, L"--Rx->");

        //  Cleanup
        DmaStop(&pInstance->rxDmaInfo);
        DmaInterruptDone(pInstance->hRxDmaChannel);
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
        goto cleanUp;
    }

    // Get and clear the status
    dwDmaStatus = DmaGetStatus(&pInstance->txDmaInfo);
    DmaClearStatus(&pInstance->txDmaInfo, dwDmaStatus);

    DEBUGMSG(ZONE_DMA, (L" SPI_DmaWrite: DMA Status = %x\r\n", dwDmaStatus));

    DmaInterruptDone(pInstance->hTxDmaChannel);
    DmaStop(&pInstance->txDmaInfo);

    // Copy the data from the DMA buffer
    memcpy(pInBuffer, pInstance->pRxDmaBuffer, size);
	
cleanUp:
    CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN); // Disable the channel.

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);
	
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_DmaWriteRead(rc = %d)\r\n", size));

    return size;
}


//------------------------------------------------------------------------------
DWORD SPI_DmaWriteRead(DWORD context, DWORD size, VOID *pOutBuffer, VOID *pInBuffer)
//	This function writes and reads data to/from the device using DMA.
//	Both buffers must be the same length.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
{
    DWORD   dwCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_DmaWriteRead(0x%08x, 0x%08x)\r\n", context, size));

    dwCount = SPI_AsyncWriteRead(context, size, pOutBuffer, pInBuffer);
    if (dwCount != 0){
        dwCount = SPI_WaitForAsyncWriteReadComplete(context, size, pInBuffer);
    }
    return dwCount;	
}


//------------------------------------------------------------------------------
DWORD SPI_WriteRead(DWORD context, DWORD size, VOID *pOutBuffer, VOID *pInBuffer)
//	This function writes and reads data to/from the device.  
//	Both buffers must be the same length.
//	Note that this is only allowed for MCSPI_CHCONF_TRM_TXRX channels
{
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
    UCHAR* pInData = (UCHAR*)pInBuffer;
    UCHAR* pOutData = (UCHAR*)pOutBuffer;
    DWORD dwWordLen;
    DWORD dwCount = 0;
    DWORD dwWait;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_WriteRead(0x%08x, 0x%08x)\r\n", context, size));

    CHECK_INSTANCE_CONTEXT(L"SPI_WriteRead");

    //  Call DMA function if DMA enabled
    if( SpiDmaTxEnabled(pInstance) && SpiDmaRxEnabled(pInstance)){
        return SPI_DmaWriteRead(context, size, pOutBuffer, pInBuffer);
    }

    pDevice = pInstance->pDevice;

    dwWordLen = MCSPI_CHCONF_GET_WL(pInstance->config); // Get the word length of the data
    
	WaitForSingleObject(pDevice->hControllerMutex, INFINITE);
    SetSPIPower(pDevice, D0);
    
    // Enable the channel
	if (!pInstance->exclusiveAccess)
    	SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    // Write out the data
    for( dwCount = 0; dwCount < size; ) {
        //  Write out data on byte/word/dword boundaries
        if( dwWordLen > 16 ){
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT32*)(&pOutData[dwCount]));   
        } else if( dwWordLen > 8 ){
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT16*)(&pOutData[dwCount]));   
        } else {
            OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_TX, *(UINT8*)(&pOutData[dwCount]));   
        }   

        //  Wait for TX register to empty out
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHSTATUS) & MCSPI_CHSTAT_TX_EMPTY)){
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 ) {
            DEBUGMSG(ZONE_ERROR, (L"SPI_WriteRead write timeout\r\n"));
            
            // Disable the channel.
			if (!pInstance->exclusiveAccess)
        	    CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            SetSPIPower(pDevice, D4);
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }

        //  Wait for RX register to fill
        dwWait = pDevice->timeout;
        while(dwWait && !(INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHSTATUS) & MCSPI_CHSTAT_RX_FULL)) {
            StallExecution(1);
            dwWait--;
        }

        //  Check if timeout occured
        if( dwWait == 0 ){
            DEBUGMSG(ZONE_ERROR, (L"SPI_WriteRead read timeout\r\n"));
            
            // Disable the channel.
			if (!pInstance->exclusiveAccess)
            	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

            SetSPIPower(pDevice, D4);
			ReleaseMutex(pDevice->hControllerMutex);
            goto cleanUp;
        }

       //  Read in data on byte/word/dword boundaries
        if( dwWordLen > 16 ){
            *(UINT32*)(&pInData[dwCount]) =
				INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT32);
        } else if( dwWordLen > 8 ){
            *(UINT16*)(&pInData[dwCount]) = 
				(UINT16) INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT16);
        } else {
            *(UINT8*)(&pInData[dwCount]) = 
				(UINT8) INREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_RX);   
            dwCount += sizeof(UINT8);
        }   
    }
    
    // Disable the channel.
	if (!pInstance->exclusiveAccess)
    	CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);

    SetSPIPower(pDevice, D4);
	ReleaseMutex(pDevice->hControllerMutex);
    
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_WriteRead(rc = %d)\r\n", dwCount));
    return dwCount;
}

// STOPPED HERE
//------------------------------------------------------------------------------
BOOL ContextRestore(SPI_INSTANCE *pInstance)
//	Function:  CentextRestore()
{
    SPI_DEVICE *pDevice = pInstance->pDevice;
    DWORD	dwCount=0;

    pDevice = pInstance->pDevice;

    SETREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_SOFTRESET);
    // Wait until resetting is done
    while ( !(INREG32(&pDevice->pSPIRegs->MCSPI_SYSSTATUS) & MCSPI_SYSSTATUS_RESETDONE)){
        Sleep (1);
        if (dwCount++>0x100){
            // Break out dead lock, something is wrong.
            ERRORMSG (TRUE, (TEXT("SPI: ERROR holding in reset.\n")));
            return FALSE;
        }
    }

    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQENABLE, 0);      // Disable all interrupts.
    OUTREG32(&pDevice->pSPIRegs->MCSPI_IRQSTATUS, 0xFFFF); // Clear interrupts.
    // Setup Module Control as master or slave
    OUTREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, (pDevice->eSpiMode==SLAVE) ? MCSPI_MS_BIT : 0);
    OUTREG32(&pDevice->pSPIRegs->MCSPI_SYSCONFIG, MCSPI_SYSCONFIG_AUTOIDLE | MCSPI_SYSCONFIG_SMARTIDLE);

    // Configure the channel
	OUTREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCONF, pInstance->config);

    //  Restore DMA if needed
    if (pInstance->config & (MCSPI_CHCONF_DMAR_ENABLE | MCSPI_CHCONF_DMAW_ENABLE)){
        SpiDmaRestore(pInstance);
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL SPI_LockController(DWORD context, DWORD dwTimeout)
//	Called by external clients to place the controller in single access mode
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;
	UINT32 dwStatus;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_LockController\r\n"));
	
    CHECK_INSTANCE_CONTEXT(L"SPI_LockController");

    pDevice = pInstance->pDevice;
	dwStatus = WaitForSingleObject(pDevice->hControllerMutex, dwTimeout); // Get lock
	if (dwStatus != WAIT_OBJECT_0){
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_LockController: Failed getting the hardware lock!  Status 0x%x\r\n", dwStatus));
		goto cleanUp;
	}
	
    SetSPIPower(pDevice, D0);
	// Set controller in single access mode
    SETREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_SINGLE_BIT);
    SetSPIPower(pDevice, D4);
	
	pInstance->exclusiveAccess = TRUE;   // Set flag indicating we own the controller lock
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_LockController\r\n"));
	return rc;
}
		
//------------------------------------------------------------------------------
BOOL SPI_UnlockController(DWORD context)
//	Called by external clients to release controller from single access mode
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"SPI_UnlockController\r\n"));
	
    CHECK_INSTANCE_CONTEXT(L"SPI_UnlockController");
	CHECK_EXCLUSIVE_ACCESS(L"SPI_UnlockController");

    pDevice = pInstance->pDevice;
    SetSPIPower(pDevice, D0);
    CLRREG32(&pDevice->pSPIRegs->MCSPI_MODULCTRL, MCSPI_SINGLE_BIT); // Release single access mode
    SetSPIPower(pDevice, D4);
	
	pInstance->exclusiveAccess = FALSE;
	if (!ReleaseMutex(pDevice->hControllerMutex)){
        DEBUGMSG (ZONE_ERROR, (L"ERROR: SPI_UnlockController: Error releasing lock!  Error code 0x%x\r\n", GetLastError()));
		goto cleanUp;
	}
	
    rc = TRUE;
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_UnlockController\r\n"));
	return rc;
}
		
//------------------------------------------------------------------------------
BOOL SPI_EnableChannel(DWORD context)
//	Called by external clients to enable a channel in single access mode
//	This function both enables the channel and asserts the chip select
//	to the target device
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_EnableChannel\r\n"));
	
    CHECK_INSTANCE_CONTEXT(L"SPI_EnableChannel");
	CHECK_EXCLUSIVE_ACCESS(L"SPI_EnableChannel");

    pDevice = pInstance->pDevice;
    
    SetSPIPower(pDevice, D0);

    SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);      // Enable the channel
    SETREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCONF, MCSPI_CHCONF_FORCE);   // Assert the chip select

    SetSPIPower(pDevice, D4);
	
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_EnableChannel\r\n"));
	return rc;	
}

//------------------------------------------------------------------------------
BOOL SPI_DisableChannel(DWORD context)
//	Called by external clients to disable a channel in single access mode
//	This function both disables the channel and de-asserts the chip select
//	to the target device
{
    BOOL rc = FALSE;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    SPI_DEVICE *pDevice;

    DEBUGMSG(ZONE_FUNCTION, (L"+SPI_DisableChannel\r\n"));
	
    CHECK_INSTANCE_CONTEXT(L"SPI_DisableChannel");
	CHECK_EXCLUSIVE_ACCESS(L"SPI_DisableChannel");

    pDevice = pInstance->pDevice;
	
    SetSPIPower(pDevice, D0);
	
    CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCONF, MCSPI_CHCONF_FORCE);  // De-assert the chip select
    CLRREG32(&pDevice->pSPIRegs->ch[pInstance->address].MCSPI_CHCTRL, MCSPI_CHCONT_EN);     // Disable the channel

    SetSPIPower(pDevice, D4);
	
    rc = TRUE;
	
cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_DisableChannel\r\n"));
	return rc;	
}

//------------------------------------------------------------------------------
BOOL SPI_IOControl(
    DWORD context, DWORD dwCode, BYTE *pInBuffer, DWORD inSize, BYTE *pOutBuffer,
    DWORD outSize, DWORD *pOutSize
)
//	This function sends a command to a device.
{
    BOOL bRetVal = FALSE;
    SPI_DEVICE *pDevice = NULL;
    SPI_INSTANCE *pInstance = (SPI_INSTANCE*)context;
    DEVICE_IFC_SPI ifc;
    IOCTL_SPI_CONFIGURE_IN *pConfig;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+SPI_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    ));

    CHECK_INSTANCE_CONTEXT(L"SPI_IOControl");
    
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
            goto cleanUp;
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
            ifc.context             = context;
            ifc.pfnConfigure        = SPI_Configure;
            ifc.pfnRead             = SPI_Read;
            ifc.pfnWrite            = SPI_Write;
            ifc.pfnWriteRead        = SPI_WriteRead;
            ifc.pfnAsyncWriteRead   = SPI_AsyncWriteRead;
            ifc.pfnWaitForAsyncWriteReadComplete = SPI_WaitForAsyncWriteReadComplete;
            ifc.pfnSetSlaveMode     = SPI_SetSlaveMode;
            ifc.pfnLockController   = SPI_LockController;
            ifc.pfnUnlockController = SPI_UnlockController;
            ifc.pfnEnableChannel    = SPI_EnableChannel;
            ifc.pfnDisableChannel   = SPI_DisableChannel;
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
        bRetVal = FALSE;
        break;

    case IOCTL_CONTEXT_RESTORE:
        if (pDevice->pActiveInstance != NULL)
            ContextRestore((SPI_INSTANCE*)pDevice->pActiveInstance);
        else
            DEBUGMSG( ZONE_ERROR, (L"SPI: IOCTL_CONTEXT_RESTORE_NOTIFY FAILED\r\n"));
        break;

    case IOCTL_POWER_CAPABILITIES: 
        if (pOutBuffer && outSize >= sizeof (POWER_CAPABILITIES) && pOutSize){
            __try {
                PPOWER_CAPABILITIES pPowerCaps;
                pPowerCaps = (PPOWER_CAPABILITIES)pOutBuffer;
     
                // Only supports D0 and D4 states
                memset(pPowerCaps, 0, sizeof(*pPowerCaps));
                pPowerCaps->DeviceDx = DX_MASK(D0)|DX_MASK(D4);;
                *pOutSize = sizeof(*pPowerCaps);
                    
                bRetVal = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                RETAILMSG(ZONE_ERROR, (L"exception in spi ioctl\r\n"));
            }
        }
        break;

    case IOCTL_POWER_QUERY: 
        if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) {
            __try {
                CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pOutBuffer;

                if (VALID_DX(ReqDx)) 
                    bRetVal = TRUE;

            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
            }
        }
        break;

    case IOCTL_POWER_SET: 
        if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) {
            __try {
                CEDEVICE_POWER_STATE newPowerState = *(PCEDEVICE_POWER_STATE)pOutBuffer;

                // SPI clocks are enabled during read/write and disabled after
                // nActivityTimeout duration. 
                // In D4 request wait untill the SPIPowerTimerThread puts the device in D4 state
                if (newPowerState == D4) {
                    if (pDevice->powerState != D4) {
                        // Wait till the SPI device is put to OFF state
                        WaitForSingleObject(pDevice->hDeviceOffEvent, INFINITE);
                    }
                }
                
                pDevice->systemState = newPowerState;
                bRetVal = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER){
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
            }
        }
        break;

    // gets the current device power state
    case IOCTL_POWER_GET: 
        if (pOutBuffer != NULL && outSize >= sizeof(CEDEVICE_POWER_STATE)){
            __try {
                *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
                bRetVal = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER){
                RETAILMSG(ZONE_ERROR, (L"Exception in spi ioctl\r\n"));
            }
        }     
        break;
  
    default:
        ERRORMSG(1, (TEXT("SPI: Unknown IOCTL_xxx(0x%0.8X) \r\n"), dwCode));
        break;
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-SPI_IOControl(rc = %d)\r\n", bRetVal));
    return bRetVal;
}

VOID SPI_PowerUp(DWORD context){UNREFERENCED_PARAMETER(context);}
void SPI_PowerDown(DWORD context){	UNREFERENCED_PARAMETER(context);}

//------------------------------------------------------------------------------
BOOL __stdcall DllMain(HANDLE hDLL, DWORD reason, VOID *pReserved )
{
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    UNREFERENCED_PARAMETER(pReserved);
    return TRUE;
}
