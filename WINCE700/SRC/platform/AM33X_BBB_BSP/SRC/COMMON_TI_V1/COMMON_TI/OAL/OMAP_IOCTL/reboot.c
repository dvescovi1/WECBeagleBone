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
//  File:  reboot.c
//
//  This file implement OMAP35XX SoC specific OALIoCtlHalReboot function.
//
#include "omap.h"
#include "oalex.h"
#include "omap_prcm_regs.h"
#include "oal_prcm.h"
#include <nkintr.h>


extern OMAP_PRCM_PRM              *g_pPrcmPrm;
extern void PrcmClearContextRegisters();

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalReboot
//
BOOL OALIoCtlHalReboot(
    UINT32 code, 
    VOID *pInpBuffer, 
    UINT32 inpSize, 
    VOID *pOutBuffer,
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{    
    BOOL    bPowerOn = FALSE;

    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalReboot\r\n"));

    // Perform a global SW reset
    OALMSG(TRUE, (L"*** RESET ***\r\n"));

    // Disable KITL
#if (_WINCEOSVER<600)
    OALKitlPowerOff();
#else    
    KITLIoctl(IOCTL_KITL_POWER_CALL, &bPowerOn, sizeof(bPowerOn), NULL, 0, NULL);    
#endif

    PrcmClearContextRegisters();

    // Flush the cache
    OEMCacheRangeFlush( NULL, 0, CACHE_SYNC_ALL );

    // Do warm reset
    PrcmGlobalReset();

    // Should never get to this point...
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalReboot\r\n"));

    return TRUE;
}

//------------------------------------------------------------------------------
