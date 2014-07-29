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
//  File:  eth.c
//
//  This file implements bootloader functions related to ethernet download
//  
#include "eboot.h"
#include "am33x_base_regs.h"
#include "am33x_clocks.h"


extern BOOL EnableDeviceClocks( UINT devId, BOOL bEnable);
//------------------------------------------------------------------------------
//  This variable specifies level of debug ouptup from BLCommon library.
ULONG EdbgDebugZone = 0;
extern CHAR  *gDevice_prefix;

//------------------------------------------------------------------------------
//  This structure contains local state variables.
static struct {
    EDBG_ADDR edbgAddress;
    OAL_KITL_ETH_DRIVER *pDriver;
    UINT8 buffer[OAL_KITL_BUFFER_SIZE];
} g_ethState;

UINT32 BLEthDownload( BOOT_CFG *pConfig, OAL_KITL_DEVICE *pBootDevices)
//  This function initialize ethernet adapter and call download function from
//  bootloader common library.
{
    UINT32 rc = (UINT32) BL_ERROR;
    OAL_KITL_DEVICE *pDevice;
    UINT32 subnetMask, *pLeaseTime, leaseTime = 0;
    UINT16 mac[3];
    BOOL jumpToImage = FALSE;
    UINT8 deviceId[OAL_KITL_ID_SIZE];

    // Do any necessary initialization
    switch (pConfig->bootDevLoc.IfcType){
        case Internal:
            switch (pConfig->bootDevLoc.LogicalLoc){
                case AM33X_EMACSW_REGS_PA:
					EnableDeviceClocks(AM_DEVICE_CPGMAC0, TRUE);
                    break;
            }
            break;
        }

    // Find boot/KITL device driver
    pDevice = OALKitlFindDevice(&pConfig->bootDevLoc, pBootDevices);        
    if ((pDevice == NULL) || (pDevice->type != OAL_KITL_TYPE_ETH)){
        OALMSG(OAL_ERROR, (L"ERROR: Boot device doesn't exist or it is unsupported\r\n"));
        goto cleanUp;
    }
    
    g_ethState.pDriver = (OAL_KITL_ETH_DRIVER*)pDevice->pDriver; // Get device driver

    // Call InitDmaBuffer if there is any
    if (g_ethState.pDriver->pfnInitDmaBuffer != NULL){
        if (!g_ethState.pDriver->pfnInitDmaBuffer((UINT32)g_ethState.buffer, sizeof(g_ethState.buffer))){
            OALMSG(OAL_ERROR, (L"ERROR: Boot device driver InitDmaBuffer call failed\r\n"));
            goto cleanUp;
        }
    }      

    // RNDIS_MDD (public code) attempts to map devLoc.PhysicalLoc with
    // NKCreateStaticMapping.  NKCreateStaticMapping requires a true
    // physical address.  OALKitlFindDevice fills in devLoc.PhysicalLoc
    // with the kernel mode virtual address which causes NKCreateStaticMapping
    // to fail.
    // Overwrite devLoc.PhysicalLoc with the actual physical address so 
    // this function succeeds.  Note that all kitl transports need to 
    // handle a true physical address in this location.
    pConfig->bootDevLoc.PhysicalLoc = (PVOID)OALVAtoPA(pConfig->bootDevLoc.PhysicalLoc);

    // Call Init
	memcpy(mac,pConfig->mac,sizeof(mac));
    if (!g_ethState.pDriver->pfnInit(pConfig->bootDevLoc.PhysicalLoc,
		                             pConfig->bootDevLoc.LogicalLoc, mac))
	{
        OALMSG(OAL_ERROR, (L"ERROR: Boot device driver Init call failed\r\n"));
        goto cleanUp;
    }         

    OALMSG(OAL_INFO, (L"INFO: Boot device uses MAC %s\r\n", OALKitlMACtoString(mac)));

    // Generate device name
    OALKitlCreateName((CHAR *)gDevice_prefix, mac, (CHAR *)deviceId);
    OALMSG(1 /*OAL_INFO*/, (L"INFO: *** Device Name %S ***\r\n", deviceId));

    // Initialize ethernet
    memset(&g_ethState.edbgAddress, 0, sizeof(g_ethState.edbgAddress));

    // Set lease time pointer to activate DHCP or update global structure
    if ((pConfig->kitlFlags & OAL_KITL_FLAGS_DHCP) != 0){
        pLeaseTime = &leaseTime;
    } else {
        pLeaseTime = NULL;
        g_ethState.edbgAddress.dwIP = pConfig->ipAddress;
    }
    memcpy(g_ethState.edbgAddress.wMAC, mac, sizeof(g_ethState.edbgAddress.wMAC));
    subnetMask = 0xFFFFFF;
       
    // Initialize TFTP transport
    // Note that first parameter must be pointer to global variable...
    if (!EbootInitEtherTransport(
            &g_ethState.edbgAddress, (LPDWORD) &subnetMask, &jumpToImage, (DWORD*) pLeaseTime,
            EBOOT_VERSION_MAJOR, EBOOT_VERSION_MINOR, gDevice_prefix, 
            (char*) deviceId, EDBG_CPU_TYPE_ARM|0x0F, 0
            ))
    {
        OALMSG(OAL_ERROR, (L"ERROR: EbootInitEtherTransport call failed\r\n"));
        goto cleanUp;
    }

    // If DHCP was used update lease time and address
    if (pLeaseTime != NULL) pConfig->ipAddress = g_ethState.edbgAddress.dwIP;

    // Depending on what we get from TFTP
    rc = jumpToImage ? BL_JUMP : BL_DOWNLOAD;

cleanUp:
    return rc;
}

VOID BLEthConfig( BSP_ARGS *pArgs)
//  This function receives connect message from host and it updates arguments
//  based on it. Most part of connect message isn't used by KITL anymore, but
//  we want set passive KITL mode when no-KITL is select on PB.
{
    EDBG_OS_CONFIG_DATA *pConfig;

    // Get host connect info
    pConfig = EbootWaitForHostConnect(&g_ethState.edbgAddress, NULL);

    pArgs->kitl.flags &= ~OAL_KITL_FLAGS_PASSIVE;
    if ((pConfig->KitlTransport & KTS_PASSIVE_MODE) != 0) {
        pArgs->kitl.flags |= OAL_KITL_FLAGS_PASSIVE;
    }        
}

BOOL OEMEthGetFrame( BYTE *pData, UINT16 *pLength)
//  Check to see if a frame has been received, and if so copy to buffer.
//  An optimization  which may be performed in the Ethernet driver is to 
//  filter out all received broadcast packets except for ARPs.
{
    return g_ethState.pDriver->pfnGetFrame(pData, pLength) > 0;
}

BOOL OEMEthSendFrame( BYTE *pData, DWORD length)
//  Send Ethernet frame.  
{
    return g_ethState.pDriver->pfnSendFrame(pData, length) == 0;
}

BOOL BLEthReadData(ULONG size, UCHAR *pData)
//  This function is called to read data from the transport during
//  the download process.
{
    // Save read data size and location. It is used in workaround
    // for download BIN DIO images larger than RAM.
    g_eboot.readSize = size;
    g_eboot.pReadBuffer = pData;
    return EbootEtherReadData(size, pData);
}

//------------------------------------------------------------------------------

