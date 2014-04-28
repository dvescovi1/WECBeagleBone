//-------------------------------------------------------------------------
// <copyright file="PDD_Intf.cpp" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#include <windows.h>
#include <pkfuncs.h>
#include <pm.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include <camera.h>
#include "CameraDriver.h"
#include "PinDriver.h"

#include <ceddk.h>
#include <USBdi.h>                  // USB includes
#include <usb100.h>                 // USB includes
#include <usbclient.h>              // USB client driver helper code
#include "USBCode.h"
#include "CameraPDD.h"
#include "USBPdd.h"


PVOID PDD_Init( PVOID MDDContext, PPDDFUNCTBL pPDDFuncTbl )
{
    DWORD dwRet = ERROR_SUCCESS;

    CUSBPdd *pDD = new CUSBPdd();
    if ( pDD == NULL )
    {
        return NULL;
    }

    dwRet = pDD->PDDInit( MDDContext, pPDDFuncTbl );
    if( ERROR_SUCCESS != dwRet )
    {
        return NULL;
    }

    return pDD;    
}

DWORD PDD_DeInit( LPVOID PDDContext )
{
    DWORD rc = ERROR_SUCCESS;
    
    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD != NULL )
    {
        rc = pDD->PDDDeInit();
        delete pDD;
    }
    
    return rc;
}

DWORD PDD_GetAdapterInfo( LPVOID PDDContext, PADAPTERINFO pAdapterInfo )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if( NULL == pDD )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->GetAdapterInfo( pAdapterInfo );
    }

    return(rc); 
}

DWORD PDD_HandleVidProcAmpChanges( LPVOID PDDContext, DWORD dwPropId, LONG lFlags, LONG lValue)
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->HandleVidProcAmpChanges( dwPropId, lFlags, lValue );
    }

    return(rc); 
}

DWORD PDD_HandleCamControlChanges( LPVOID PDDContext, DWORD dwPropId, LONG lFlags, LONG lValue )
{    
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->HandleCamControlChanges( dwPropId, lFlags, lValue );
    }

    return(rc); 
}

DWORD PDD_HandleVideoControlCapsChanges( LPVOID PDDContext, LONG lModeType ,ULONG ulCaps )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->HandleVideoControlCapsChanges( lModeType, ulCaps );
    }

    return(rc); 
}

DWORD PDD_SetPowerState( LPVOID PDDContext, CEDEVICE_POWER_STATE PowerState )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->SetPowerState( PowerState );
    }

    return(rc); 
}


DWORD PDD_HandleAdapterCustomProperties( LPVOID PDDContext, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->HandleAdapterCustomProperties( pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred );
    }

    return(rc); 
}


DWORD PDD_InitSensorMode( LPVOID PDDContext, ULONG ulModeType, LPVOID ModeContext )
{

    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->InitSensorMode( ulModeType, ModeContext );
    }

    return(rc); 
}

DWORD PDD_DeInitSensorMode( LPVOID PDDContext, ULONG ulModeType )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->DeInitSensorMode( ulModeType );
    }

    return(rc); 
}

DWORD PDD_SetSensorState( LPVOID PDDContext, ULONG ulModeType, CSSTATE CsState )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->SetSensorState( ulModeType, CsState );
    }

    return(rc); 
}

DWORD PDD_TakeStillPicture( LPVOID PDDContext, LPVOID pBurstModeInfo )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->TakeStillPicture( pBurstModeInfo );
    }

    return(rc); 
}

DWORD PDD_GetSensorModeInfo( LPVOID PDDContext, ULONG ulModeType, PSENSORMODEINFO pSensorModeInfo )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->GetSensorModeInfo( ulModeType, pSensorModeInfo );
    }

    return(rc); 
}

DWORD PDD_SetSensorModeFormat( LPVOID PDDContext, ULONG ulModeType, PCS_DATARANGE_VIDEO pCsDataRangeVideo )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->SetSensorModeFormat( ulModeType, pCsDataRangeVideo );
    }

    return(rc); 
}

PVOID PDD_AllocateBuffer( LPVOID PDDContext, ULONG ulModeType )
{

    return NULL;
}

DWORD PDD_DeAllocateBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer )
{

    return ERROR_SUCCESS;
}

DWORD PDD_RegisterClientBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->RegisterClientBuffer( ulModeType, pBuffer );
    }

    return(rc); 
}

DWORD PDD_UnRegisterClientBuffer( LPVOID PDDContext, ULONG ulModeType, PVOID pBuffer )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->UnRegisterClientBuffer( ulModeType, pBuffer );
    }

    return(rc); 
}

DWORD PDD_FillBuffer( LPVOID PDDContext, ULONG ulModeType, PUCHAR pImage )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->FillBuffer( ulModeType, pImage );
    }

    return(rc); 
}

DWORD PDD_HandleModeCustomProperties( LPVOID PDDContext, ULONG ulModeType, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DWORD rc;

    CUSBPdd *pDD = (CUSBPdd *)PDDContext;
    if ( pDD == NULL )
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        rc = pDD->HandleSensorModeCustomProperties( ulModeType, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred );
    }

    return(rc); 
}
