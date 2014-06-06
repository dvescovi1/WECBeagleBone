//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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
//  File:  kitl.c
//

//-------------------------------------------------------------------------------

#include <bsp.h>
#include <bsp_def.h>
//#include <bus.h>
#include <kitlprot.h>
#include <kitl_cfg.h>
#include <devload.h>
#include <oal_kitlex.h>
#include <am33x.h>

//------------------------------------------------------------------------------
//  Local definition

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE          ((HKEY)(ULONG_PTR)0x80000002)
#endif

static BOOL s_bKitlActive = FALSE;

//------------------------------------------------------------------------------

BOOL OALIoCtlVBridge(
    UINT32 code, VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);



//------------------------------------------------------------------------------
//  External functions

void OEMEthernetDriverEnable(BOOL bEnable);
void OEMUsbDriverEnable(BOOL bEnable);
    
//------------------------------------------------------------------------------
BOOL OEMKitlEnableClocks(BOOL bEnable)
{
    KITL_RETAILMSG(ZONE_WARNING, ("OEMKitlEnableClocks: %d", bEnable));
    return TRUE;
}

BOOL OEMKitlStartup()
//Currently only support Ethernet Kitl, and we didn't coniser wake-up
{
    BOOL rc = FALSE;

    OAL_KITL_ARGS *pArgs, args;
    CHAR *szDeviceId;
//    OMAP_GPIO_REGS *pGPIORegs = OALPAtoUA(BSP_ETHER_GPIO_PA);

    KITLSetDebug(ZONE_ERROR | ZONE_WARNING | ZONE_INIT | ZONE_KITL_OAL | ZONE_KITL_ETHER );

	KITL_RETAILMSG(ZONE_KITL_OAL, ("+OALKitlStart\r\n"));
    // First get boot args and device id
    pArgs = (OAL_KITL_ARGS*)OALArgsQuery(OAL_ARGS_QUERY_KITL);

    // If we don't get kitl arguments use default
    if (pArgs == NULL){
        KITL_RETAILMSG(ZONE_WARNING, (
            "WARN: Boot arguments not found, use defaults\r\n"
            ));
        memset(&args, 0, sizeof(args));
        args.flags = OAL_KITL_FLAGS_ENABLED|OAL_KITL_FLAGS_POLL;
        args.devLoc.IfcType = Internal;
        args.devLoc.BusNumber = 0;
        args.devLoc.LogicalLoc = AM33X_EMACSW_REGS_PA;
		args.ipAddress = 192 << 24 | 168 << 16 | 205 << 8 |  3; 
//		args.ipMask    = 0xffffff00
        pArgs = &args;
	}

    // We always create device name
    szDeviceId = BSP_DEVICE_PREFIX;
    pArgs->flags |= OAL_KITL_FLAGS_EXTNAME;
    
    // Initialize debug device
    //switch (pArgs->devLoc.IfcType){
    //    case Internal:
    //        switch (pArgs->devLoc.LogicalLoc){
    //            case BSP_LAN9115_REGS_PA:
    //                // enable clocks to GPIO banks that have KITL dependencies so we can do initialization
    //                OEMKitlEnableClocks(TRUE);
    //                
    //                // Reset LAN chip
    //                pGPIORegs = OALPAtoUA(BSP_RESET_ETHER_KITL_GPIO_PA);
    //                CLRREG32(&pGPIORegs->DATAOUT, 1 << (BSP_RESET_ETHER_KITL_GPIO % 32));
    //                OALStall(1000);
    //                SETREG32(&pGPIORegs->DATAOUT, 1 << (BSP_RESET_ETHER_KITL_GPIO % 32));
    //                OALStall(1000);

    //                // Prepare interrupt
    //                pGPIORegs = OALPAtoUA(BSP_ETHER_GPIO_PA);
    //                SETREG32(&pGPIORegs->OE, 1 << (BSP_IRQ_ETHER_KITL % 32));
    //                // Interrupt on falling edge
    //                SETREG32(&pGPIORegs->FALLINGDETECT, 1 << (BSP_IRQ_ETHER_KITL % 32));

    //                OEMKitlEnableClocks(FALSE);
    //                break;
    //            }
    //        break;
    //    }

    // Finally call KITL library
    rc = OALKitlInit(szDeviceId, pArgs, g_kitlDevices);

    // If it failed or KITL is disabled
    if (!rc || (pArgs->flags & OAL_KITL_FLAGS_ENABLED) == 0) goto cleanUp;

    // enable kitl interrupts
    s_bKitlActive = TRUE;
    OEMKitlEnable(TRUE);
    
cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlStart(rc = %d)\r\n", rc));

    return rc;
}

VOID OALKitlInitRegistry( )
//  Function:  OALKitlInitRegistry
{
    DEVICE_LOCATION devLoc;

    // Get KITL device location
    if (!OALKitlGetDevLoc(&devLoc)) goto cleanUp;

    // Depending on device bus
    switch (devLoc.IfcType){
        case Internal:
            switch (devLoc.LogicalLoc)
                {
                case AM33X_EMACSW_REGS_PA:
                    // Disable ethernet, enable USB
                    OEMEthernetDriverEnable(FALSE);
//                    OEMUsbDriverEnable(TRUE);
                    break;   
                case 0x44332211/*OMAP_USBHS_REGS_PA*/:
                    // Disable USB, enable ethernet
                    OEMEthernetDriverEnable(TRUE);
//                    OEMUsbDriverEnable(FALSE);
                    break;   
                default:
                    // Enable both USB and ethernet
                    OEMEthernetDriverEnable(TRUE);
//                    OEMUsbDriverEnable(TRUE);
                    break;   
                }
            break;
        }

cleanUp:
    return;
}

//------------------------------------------------------------------------------
DWORD OEMKitlGetSecs()
{
    return OALGetTickCount()/1000;
}

//------------------------------------------------------------------------------
//
//  Function:     OEMKitlEnable
//
//  Enables/disables kitl.  Necessary to enable/disable gpio pin for kitl
//  interrupts. 
//
BOOL OEMKitlEnable( BOOL bEnable )
{
    if (s_bKitlActive == FALSE) return FALSE;

    OEMKitlEnableClocks(bEnable);

    return TRUE;
}

//------------------------------------------------------------------------------
UINT32 OALGetTickCount( )
{
    return CurMSec;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMKitlIoctl
//
//  This function handles KITL IOCTL codes.
//
//
BOOL OEMKitlIoctl (DWORD code, VOID * pInBuffer, DWORD inSize, VOID * pOutBuffer, DWORD outSize, DWORD * pOutSize)
{
    BOOL fRet = FALSE;

    switch (code) {
    case IOCTL_HAL_INITREGISTRY:
        OALKitlInitRegistry();
        // Leave return code false and set last error to ERROR_NOT_SUPPORTED
        // This allows code to fall through to OEMIoctl so IOCTL_HAL_INITREGISTRY can be 
        // handled there as well.
        NKSetLastError(ERROR_NOT_SUPPORTED);
        break;
    default:
        fRet = OALIoCtlVBridge (code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize);
    }

    return fRet;
}
