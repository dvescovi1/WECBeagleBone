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
//  This file implements the OEM's IO Control (IOCTL) functions and declares
//  global variables used by the IOCTL component.
//
#include <windows.h>
#include <bsp.h>
#include <bldver.h>
//#include <i2c.h>
//#include <oal_i2c.h>
#include <pkfuncs.h>
#include "am33x.h"


//------------------------------------------------------------------------------
//  Global:  g_oalIoCtlPlatformType/OEM    
//  Platform Type/OEM

LPCWSTR g_oalIoCtlPlatformType = L"BSP_AM33X_TYPE";
LPCWSTR g_oalIoCtlPlatformOEM  = L"Texas Instruments";

//------------------------------------------------------------------------------
//
// Global: g_oalIoctlPlatformManufacturer/Name
//
//

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlProcessorVendor/Name/Core
//
//  Processor information
//

LPCWSTR g_oalIoCtlProcessorVendor = L"Texas Instruments";
LPCWSTR g_oalIoCtlProcessorName   = L"AM33X";
LPCWSTR g_oalIoCtlProcessorCore   = L"Cortex-A8";

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoctlInstructionSet/ClockSpeed
//
//  Processor instruction set identifier and maximal CPU speed
//
UINT32 g_oalIoCtlInstructionSet = PROCESSOR_FLOATINGPOINT;
UINT32 g_oalIoCtlClockSpeed = BSP_SPEED_CPUMHZ;


//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalInitRegistry
//
//  Implements the IOCTL_HAL_INITREGISTRY handler.

BOOL OALIoCtlHalInitRegistry( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);

    if (g_oalKitlEnabled == FALSE)
    {
        // If KITL isn't enabled, ensure that USB and Ethernet drivers are not
        // blocked.  This logic prevents a persistent registry from inadvertently blocking
        // these drivers when KITL has been removed from an image.
//        OEMEthernetDriverEnable(TRUE);
//        OEMUsbDriverEnable(TRUE);
    }
        
    // call RTC init
    OALIoCtlHalInitRTC(IOCTL_HAL_INIT_RTC, NULL, 0, NULL, 0, NULL);

	OEMRegInit();

    return TRUE;
}
//------------------------------------------------------------------------------
// Function: OALIoCtlHalGetPowerDisposition
//
// IOCTL_HAL_GET_POWER_DISPOSITION IOCTL Handler
//

BOOL OALIoCtlHalGetPowerDisposition(UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    if ((!pOutBuffer) || (outSize<sizeof(DWORD)))
    {
        OALMSG(TRUE, (L"***Error: Incorrect use of IOCTL_HAL_GET_POWER_DISPOSITION.\r\n"));
        return FALSE;
    }

    /* see pkfuncs.h for meaning of this value */
    *((DWORD *)pOutBuffer) = (DWORD)POWER_DISPOSITION_SUSPENDRESUME_MANUAL;
    if (pOutSize)
        *pOutSize = sizeof(DWORD);
    
    return(TRUE);
}

//------------------------------------------------------------------------------
BOOL OALIoCtlNKPhysToVirt(UINT32 code, VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize)
{
    BOOL rc = FALSE;
    PVOID vAddr = NULL;
    DWORD pAddr;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlNKPhysToVirt\r\n"));
    
    if (pInBuffer  == NULL || inSize  != sizeof(DWORD) ||
		pOutBuffer == NULL || outSize != sizeof(UINT32) )
        goto cleanUp;

	pAddr = *((DWORD*)pInBuffer);
	vAddr = NKPhysToVirt(pAddr >> 8, FALSE);
	OALMSG(OAL_IOCTL&&OAL_FUNC, (L">>>> OALIoCtlNKPhysToVirt %08X %p\r\n", pAddr, vAddr));

	*((PVOID*)pOutBuffer) = vAddr;

    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlNKPhysToVirt(rc = %d)\r\n", rc));
    return rc;
}


extern void DumpAllDeviceStatus();
//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalDumpRegisters
//
//
BOOL OALIoCtlHalDumpRegisters(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL rc = TRUE;
    
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_FUNC, (L"+OALIoCtlHalDumpRegisters\r\n"));

    // Check input parameters
    if (pInpBuffer == NULL || inpSize != sizeof(DWORD))
        {
        goto cleanUp;
        }

    // could use input parameter to select device to dump (not implemented)
    switch (*(DWORD *)pInpBuffer)
	{
        case IOCTL_HAL_DUMP_DEVICE_STATUS:
		    DumpAllDeviceStatus();
		    //RETAILMSG(1,(L"curridlelow=0x%x curridlehigh=0x%x\r\n",curridlelow,curridlehigh));
			break;
			
		default:
		    rc = FALSE;
	}

cleanUp:
    OALMSG(OAL_FUNC, (
        L"-OALIoCtlHalDumpRegisters(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetIrqCounters
//
//

extern UINT32 g_IrqCnt[MAX_IRQ_COUNT];

BOOL OALIoCtlHalGetIrqCounters(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL rc = TRUE;
    
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(code);

    OALMSG(0, (L"+OALIoCtlHalGetIrqCounters\r\n"));

    // Check input parameters
    if (pOutBuffer == NULL || outSize != (sizeof(DWORD)*MAX_IRQ_COUNT) || pOutSize == NULL)
        goto cleanUp;

	memcpy(pOutBuffer, g_IrqCnt, sizeof(DWORD)* MAX_IRQ_COUNT);
	*pOutSize = sizeof(DWORD)* MAX_IRQ_COUNT;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetHiveCleanFlag
//
//  Queries the BSP arguments to find out whether the hive registry (if present)
//  should be cleaned or not.
//
BOOL OALIoCtlHalGetHiveCleanFlag(UINT32 code, VOID *pInpBuffer, 
	UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL bRet = FALSE;
    DWORD *pdwFlags;
    BOOL  *pfClean;
//    BOOL *bHiveCleanFlag = NULL;
    UINT32 *pOALFlag = NULL;
	BOOL OALClean;
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);

    if ((!pInpBuffer) || (inpSize != sizeof(DWORD))
        || (!pOutBuffer) || (outSize != sizeof(BOOL)))
    {
        RETAILMSG(1, (L"Invalid parameter\r\n"));
        return FALSE;
    }

    pdwFlags = (DWORD *)pInpBuffer;
    pfClean  = (BOOL *)pOutBuffer;

    // This is the global shared Args flag
    pOALFlag = (UINT32 *)OALArgsQuery(OAL_ARGS_QUERY_OALFLAGS);

    if (pOALFlag != NULL) {
		OALClean = OAL_ARGS_OALFLAGS_CLEAN_REGISTRY == (*pOALFlag & OAL_ARGS_OALFLAGS_CLEAN_REGISTRY);
        *pfClean = OALClean;
        bRet = OALClean;
    }

    if (*pdwFlags == HIVECLEANFLAG_SYSTEM) {
        if(bRet) {
            OALMSG(1, (TEXT("OEM: Cleaning system hive\r\n")));
        } else {
            OALMSG(1, (TEXT("OEM: Not cleaning system hive\r\n")));
        }
    } else if (*pdwFlags == HIVECLEANFLAG_USERS) {
        if(bRet) {
            OALMSG(1, (TEXT("OEM: Cleaning user profiles\r\n")));
        } else {
            OALMSG(1, (TEXT("OEM: Not cleaning user profiles\r\n")));
        }

        // We are done checking HiveCleanFlag by now (system hive is checked before user hive).
        // Now is the time to clear the global shared Args flag if it is set by switch or software.
        if (pOALFlag != NULL) {
            *pOALFlag &= ~OAL_ARGS_OALFLAGS_CLEAN_REGISTRY;
        }
    } else {
        OALMSG(OAL_ERROR, (TEXT("OEM: Unknown flag 0x%x\r\n"), *pdwFlags));
    }

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlTable[]    
//
//  IOCTL handler table. This table includes the IOCTL code/handler pairs  
//  defined in the IOCTL configuration file. This global array is exported 
//  via oal_ioctl.h and is used by the OAL IOCTL component.
//
const OAL_IOCTL_HANDLER g_oalIoCtlTable[] = {
#include "ioctl_tab.h"
};

