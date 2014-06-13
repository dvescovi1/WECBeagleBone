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
//  File:  ioctlex.c
//
//  This file This file implements additional OAL calls.
//
#pragma warning(push)
#pragma warning(disable : 4115 4127 4201 4214)
#include <ndis.h>
#pragma warning(pop)
#include "omap.h"
#include "omap_i2c_regs.h"
#include "oalex.h"
#include "oal_clock.h"
#include "oal_i2c.h"
#include "oal_padcfg.h"
#include "sdk_i2c.h"
#include "oal_alloc.h"
#include "bsp_padcfg.h"

//------------------------------------------------------------------------------
extern UINT32 OALGetSiliconIdCode();
extern UINT32 
PrcmClockGetClockRate(
    OMAP_CLOCKID clock_id
    );

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHALPostInit
//
//  
BOOL OALIoCtlHALPostInit(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *ppOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{

    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(ppOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHALPostInit\r\n"));
    OALAllocPostInit();
	
//    OALClockPostInit();
    OALI2CPostInit();
    OALPadCfgPostInit();
    
    OALPowerPostInit();

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHALPostInit(rc = %d)\r\n", TRUE));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalIrq2Sysintr
//
//
BOOL OALIoCtlHalIrq2Sysintr(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalIrq2Sysintr\r\n"));

    // Check input parameters
    if (
        pInpBuffer == NULL || inpSize < sizeof(UINT32) ||
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_IRQ2SYSINTR invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    // Call function itself
    *(UINT32*)pOutBuffer = OALIntrTranslateIrq(*(UINT32*)pInpBuffer);
    if (pOutSize != NULL) *pOutSize = sizeof(UINT32);
   rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalIrq2Sysintr(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetCpuID
//
//
BOOL OALIoCtlHalGetCpuID(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetCpuID\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_CPUID invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    //  Get CPU Silicon ID value
    ((DWORD*)pOutBuffer)[0] = OALGetSiliconIdCode();

    rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalGetCpuID(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetDieID
//
//
BOOL OALIoCtlHalGetDieID(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL                        rc = FALSE;

    
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);

    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetDieID\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(IOCTL_HAL_GET_DIEID_OUT)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_DIEID invalid parameters\r\n"
        ));
        goto cleanUp;
    }
    
#if 0	// TODO : We need to find a DIE ID to read from the CPU
    OMAP_IDCORE_REGS*           pIdRegs;    

    //  Get CPU DIE ID values
    pIdRegs = OALPAtoUA(OMAP_IDCORE_REGS_PA);

    //  Copy to passed in struct
    pDieID = (IOCTL_HAL_GET_DIEID_OUT*)pOutBuffer;
    
    pDieID->dwIdCode    = INREG32(&pIdRegs->IDCODE);
    pDieID->dwProdID_0  = INREG32(&pIdRegs->PRODUCTION_ID_0);
    pDieID->dwProdID_1  = INREG32(&pIdRegs->PRODUCTION_ID_1);
    pDieID->dwProdID_2  = INREG32(&pIdRegs->PRODUCTION_ID_2);
    pDieID->dwProdID_3  = INREG32(&pIdRegs->PRODUCTION_ID_3);
    pDieID->dwDieID_0   = INREG32(&pIdRegs->DIE_ID_0);
    pDieID->dwDieID_1   = INREG32(&pIdRegs->DIE_ID_1);
    pDieID->dwDieID_2   = INREG32(&pIdRegs->DIE_ID_2);
    pDieID->dwDieID_3   = INREG32(&pIdRegs->DIE_ID_3);
#endif
    rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalGetDieID(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetCpuFamily
//
//
BOOL OALIoCtlHalGetCpuFamily(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;

    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetCpuFamily\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_CPUFAMILY invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    //  Get CPU family
    ((DWORD*)pOutBuffer)[0] = g_dwCpuFamily;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalGetCpuFamily(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetCpuRevision
//
//
BOOL OALIoCtlHalGetCpuRevision(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;
 
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetCpuRevision\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_CPUREVISION invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    //  Get CPU revision
    ((DWORD*)pOutBuffer)[0] = g_dwCpuRevision;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalGetCpuRevision(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetCpuSpeed
//
//
BOOL OALIoCtlHalGetCpuSpeed(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;

    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);
    
    DEBUGMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetCpuSpeed\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_CPUSPEED invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    //  Get CPU speed
    ((DWORD*)pOutBuffer)[0] = PrcmClockGetClockRate(MPU_CLK);

    rc = TRUE;

cleanUp:
    DEBUGMSG(OAL_IOCTL&&OAL_FUNC, (
        L"-OALIoCtlHalGetCpuSpeed(rc = %d)\r\n", rc
    ));
    return rc;

}


//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalI2CMode
//  
//  *pInpBuffer is a pointer to DWORD which has the following values per byte
//  0 = ignore
//  1 = SS mode
//  2 = FS mode
//  3 = HS mode
//  
//
//  | 31-25(i2c4) | 24-16(i2c3) | 15-8(i2c2) | 7-0(i2c1) |
//
BOOL OALIoCtlHalI2CMode(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);

    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalI2CCopyFnTable
//
//  returns OAL i2c routines to be called directly from drivers
//
BOOL 
OALIoCtlHalI2CCopyFnTable(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OAL_IFC_I2C *pIn;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalI2CCopyFnTable\r\n"));
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);
    if (pInBuffer == NULL || inSize != sizeof(OAL_IFC_I2C))
    {
        goto cleanUp;
    }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = sizeof(OAL_IFC_I2C);
    pIn = (OAL_IFC_I2C*)pInBuffer;

    pIn->fnI2CLock      = I2CLock;
    pIn->fnI2CUnlock    = I2CUnlock;
    pIn->fnI2COpen      = I2COpen;
    pIn->fnI2CClose     = I2CClose;
    pIn->fnI2CWrite     = I2CWrite;
    pIn->fnI2CRead      = I2CRead;
    pIn->fnI2CSetSlaveAddress    = I2CSetSlaveAddress;
    pIn->fnI2CSetSubAddressMode  = I2CSetSubAddressMode;
    pIn->fnI2CSetBaudIndex       = I2CSetBaudIndex;
    pIn->fnI2CSetTimeout         = I2CSetTimeout;
    pIn->fnSetManualDriveMode    = I2CSetManualDriveMode;
    pIn->fnDriveSCL              = I2CDriveSCL;
    pIn->fnDriveSDA              = I2CDriveSDA;

    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlHalI2CCopyFnTable(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlIgnore
//
BOOL OALIoCtlIgnore(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL  rc = FALSE;
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);

    return rc;
}



//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalPadCfgCopyFnTable
//
//  returns OAL Pad configuration routines to be called directly from drivers
//
BOOL 
OALIoCtlHalPadCfgCopyFnTable(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OAL_IFC_PADCFG *pIn;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalPadCfgCopyFnTable\r\n"));
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);
    if (pInBuffer == NULL || inSize != sizeof(OAL_IFC_PADCFG))
    {
        goto cleanUp;
    }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = sizeof(OAL_IFC_PADCFG);
    pIn = (OAL_IFC_PADCFG*)pInBuffer;

    pIn->pfnRequestPad = RequestPad;
    pIn->pfnReleasePad = ReleasePad;
    pIn->pfnConfigurePad = ConfigurePad;
    pIn->pfnGetpadConfiguration = GetPadConfiguration;
    pIn->pfnRequestAndConfigurePad = RequestAndConfigurePad;
    pIn->pfnRequestAndConfigurePadArray = RequestAndConfigurePadArray;
    pIn->pfnReleasePadArray = ReleasePadArray;
    pIn->pfnRequestDevicePads = RequestDevicePads;
    pIn->pfnReleaseDevicePads = ReleaseDevicePads;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlHalPadCfgCopyFnTable(rc = %d)\r\n", rc));
    return rc;
}

BOOL OALIoctlHalGetEccType(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
)
{
    BOOL        rc = FALSE;
    DWORD     *pEccType;
    UINT8 *pData = NULL;
	
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutSize);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoctlHalGetEccType\r\n"));

    // Check parameters
    if ((pOutBuffer==NULL) && (outSize < sizeof(DWORD)))
    {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoctlHalGetEccType invalid parameters\r\n"
        ));
        goto cleanUp;
    }
	
    //  Copy to passed in struct
    pEccType  = (DWORD *)pOutBuffer;
    // This is the global shared Args flag
    pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_ECC_TYPE);

    if (pData != NULL) {
        memcpy(pEccType, pData, sizeof(DWORD));
		*pOutSize = sizeof(DWORD);
		rc = TRUE;
    }

cleanUp:
    OALMSG(OAL_IOCTL && OAL_FUNC, (
        L"-OALIoctlHalGetEccType(rc = %d)\r\n", rc
    ));
    return rc;
}


BOOL OALIoctlHalGetPlatformID(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
)
{
    BOOL  rc = FALSE;
	
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutSize);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoctlHalGetPlatformID\r\n"));

    // Check parameters
    if ((pOutBuffer==NULL) && (outSize < sizeof(DWORD)))
    {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoctlHalGetPlatformID invalid parameters\r\n"
        ));
        goto cleanUp;
    }
	
	*(DWORD *)pOutBuffer = g_dwBoardId;
	*pOutSize = sizeof(DWORD);
	rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL && OAL_FUNC, (
        L"-OALIoctlHalGetPlatformID(rc = %d)\r\n", rc
    ));
    return rc;
}


//-----------------------------------------------------------------------------
//
//  Function:  OALIoctlHalGetBootInfo
//
//  returns boot info which consists of the boot cause (lower word)
//  and the boot sdhc controller (upper word) 1=SDHC1 2=SDHC2
//
BOOL OALIoctlHalGetBootInfo(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
)
{
    BOOL  rc = FALSE;
    DWORD *pBootInfo;
    UINT8 *pData = NULL;
	
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutSize);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoctlHalGetBootInfo\r\n"));

    // Check parameters
    if ((pOutBuffer==NULL) && (outSize < sizeof(DWORD)))
    {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoctlHalGetBootInfo invalid parameters\r\n"
        ));
        goto cleanUp;
    }
	
    //  Copy to passed in struct
    pBootInfo  = (DWORD *)pOutBuffer;
    // This is the global shared Args flag
    pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_BOOT_INFO);

    if (pData != NULL) {
        memcpy(pBootInfo, pData, sizeof(DWORD));
		*pOutSize = sizeof(DWORD);
        rc = TRUE;
    }

cleanUp:
    OALMSG(OAL_IOCTL && OAL_FUNC, (
        L"-OALIoctlHalGetBootInfo(rc = %d)\r\n", rc
    ));
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function: OALIoctlGetMacAddress
//
//  Queries the BSP arguments and get the mac address
//
BOOL OALIoctlGetMacAddress(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize)
{
    BOOL bRet = FALSE;
    UINT8 *pMACAddress = NULL;
    UINT8 *pData = NULL;

	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(inpSize);
	UNREFERENCED_PARAMETER(pInpBuffer);

    OALMSG(/*OAL_IOCTL&&OAL_FUNC*/1, (L"+IOCTL_HAL_GET_MAC_ADDRESS\r\n"));

    if ((!pOutBuffer) || (outSize < ETH_LENGTH_OF_ADDRESS))
    {
        RETAILMSG(1, (L"Invalid parameter\r\n"));
        return FALSE;
    }

    pMACAddress  = (UINT8 *)pOutBuffer;

    // This is the global shared Args flag
    if (code == IOCTL_HAL_GET_MAC_ADDRESS1)
        pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_ETHADDR_CPGMAC1);
    else
        pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_ETHADDR_CPGMAC);

    if (pData != NULL) {
        memcpy(pMACAddress, pData, ETH_LENGTH_OF_ADDRESS);
        bRet = TRUE;
    }

    OALMSG(/*OAL_IOCTL&&OAL_FUNC*/1, (L"-IOCTL_HAL_GET_MAC_ADDRESS %d\r\n", bRet));

    return bRet;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function: OALIoctlGetDisplayRes
//
//  Queries the BSP arguments and get the display resolution
//
BOOL OALIoctlGetDisplayRes(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize)
{
    BOOL bRet = FALSE;
    DWORD *pDispRes = NULL;
    UINT8 *pData = NULL;

	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(inpSize);
	UNREFERENCED_PARAMETER(pInpBuffer);
	UNREFERENCED_PARAMETER(code);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+IOCTL_HAL_GET_DISPLAY_RES\r\n"));

    if ((!pOutBuffer) || (outSize < sizeof(DWORD)))
    {
        OALMSG(1, (L"Invalid parameter\r\n"));
        return FALSE;
    }

    pDispRes  = (DWORD *)pOutBuffer;

    // This is the global shared Args flag
    pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_DISP_RES);

    if (pData != NULL) {
        memcpy(pDispRes, pData, sizeof(DWORD));
		*pOutSize = sizeof(DWORD);
        bRet = TRUE;
    }

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-IOCTL_HAL_GET_DISPLAY_RES\r\n"));

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function: OALIoctlConvertCAtoPA
//
//  Queries the BSP arguments and get the display resolution
//
BOOL OALIoctlConvertCAtoPA(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize)
{    
    DWORD *pPA = NULL;    
    
	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(code);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+IOCTL_HAL_CONVERT_CA_TO_PA\r\n"));

    if ((!pOutBuffer) || (outSize < sizeof(DWORD)))
    {
        RETAILMSG(1, (L"Invalid parameter\r\n"));
        return FALSE;
    }

    if ((!pInpBuffer) || (inpSize < sizeof(DWORD)))
    {
        RETAILMSG(1, (L"Invalid parameter\r\n"));
        return FALSE;
    }
    
    pPA = (DWORD *)pOutBuffer;    
    *pPA = OALVAtoPA(pInpBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-IOCTL_HAL_CONVERT_CA_TO_PA\r\n"));

    return TRUE;
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function: OALIoctlGetEbootCfgDirtyBit
//
//  Queries the BSP arguments and get the eboot config
//
BOOL OALIoctlGetEbootCfg(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize)
{
    BOOL bRet = FALSE;
    UINT8 *pData = NULL;

	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(inpSize);
	UNREFERENCED_PARAMETER(pInpBuffer);
	UNREFERENCED_PARAMETER(code);
    
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+IOCTL_HAL_GET_EBOOT_CFG\r\n"));

    
    if (!pOutSize)
    {
        OALMSG(1, (L"IOCTL_HAL_GET_EBOOT_CFG: Invalid parameter: pOutSize\r\n"));
        goto fnExit; /* error */
    }
    *pOutSize = 0;
    
    pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_OALFLAGS);
    if (pData == NULL)
        goto fnExit; /* error */
    
    if ((*pData & OAL_ARGS_OALFLAGS_CFG_SAVE)!=0)
    {
        pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_EBOOT_CFGSZ);
        if (pData != NULL) { 
            *pOutSize = *((UINT32 *)pData);
            if (*pOutSize==0) {                
                bRet = TRUE;
                goto fnExit; /* No data to copy */
            }
        }
        else 
            goto fnExit; /* error */
        
        if ((!pOutBuffer) || (outSize < *pOutSize))
        {
            OALMSG(1, (L"IOCTL_HAL_GET_EBOOT_CFG: Invalid parameter\r\n"));
            goto fnExit; /* error */
        }
        
        pData = (UINT8 *)OALArgsQuery(OAL_ARGS_QUERY_EBOOT_CFG);
        if (pData != NULL) 
        {
            memcpy((UINT8 *)pOutBuffer,pData,*pOutSize);
            bRet = TRUE;
            goto fnExit; /* Data copied */
        }
        else 
            goto fnExit; /* error */
    }
    else 
    {
        *pOutSize = 0;
        bRet = TRUE;
        goto fnExit; /* Save bit not set */
    }

fnExit:  
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-IOCTL_HAL_GET_EBOOT_CFG\r\n"));

    return bRet;
}



