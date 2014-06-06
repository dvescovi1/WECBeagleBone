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
/*++


Module Name:

    CameraDriver.cpp

Abstract:

    MDD Adapter implementation

Notes:
    

Revision History:

--*/

#include <windows.h>
#include <devload.h>
#include <nkintr.h>
#include <pm.h>

#include <ceddkex.h>
#include <omap_pmext.h>

#include "Cs.h"
#include "Csmedia.h"
#include "camera.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#define CAMINTERFACE
#include "CameraDebug.h"
#include "CameraDriver.h"

#pragma warning(disable:6309 6320 6387)

EXTERN_C
DWORD
CAM_Init(
    VOID * pContext
    )
{    
    CAMERADEVICE * pCamDev = NULL;
    DEBUGMSG(ZONE_INIT, (_T("CAM_Init: context %s\n"), pContext));
    
    pCamDev = new CAMERADEVICE;
    
    if ( NULL != pCamDev )
    {
        // NOTE: real drivers would need to validate pContext before dereferencing the pointer
        if ( false == pCamDev->Initialize( pContext ) )
        {
            SetLastError( ERROR_INVALID_HANDLE );
            SAFEDELETE( pCamDev );
            DEBUGMSG( ZONE_INIT|ZONE_ERROR, ( _T("CAM_Init: Initialization Failed") ) );
        }
    }
    else
    {
        SetLastError( ERROR_OUTOFMEMORY );
    }

    DEBUGMSG( ZONE_INIT, ( _T("CAM_Init: returning 0x%08x\r\n"), reinterpret_cast<DWORD>( pCamDev ) ) );

    return reinterpret_cast<DWORD>( pCamDev );
}


EXTERN_C
BOOL
CAM_Deinit(
    DWORD dwContext
    )
{
    DEBUGMSG( ZONE_INIT, ( _T("CAM_Deinit\r\n") ) );

    CAMERADEVICE * pCamDevice = reinterpret_cast<CAMERADEVICE *>( dwContext );
    SAFEDELETE( pCamDevice );

    return TRUE;
}


EXTERN_C
BOOL
CAM_IOControl(
    DWORD   dwContext,
    DWORD   Ioctl,
    UCHAR * pInBufUnmapped,
    DWORD   InBufLen, 
    UCHAR * pOutBufUnmapped,
    DWORD   OutBufLen,
    DWORD * pdwBytesTransferred
   )
{
    DEBUGMSG(ZONE_FUNCTION, (_T("CAM_IOControl(%08x): IOCTL:0x%x, InBuf:0x%x, InBufLen:%d, OutBuf:0x%x, OutBufLen:0x%x)\r\n"), dwContext, Ioctl, pInBufUnmapped, InBufLen, pOutBufUnmapped, OutBufLen));

    UCHAR * pInBuf = NULL;
    UCHAR * pOutBuf = NULL;
    DWORD dwErr = ERROR_INVALID_PARAMETER;
    BOOL  bRc   = FALSE;
    
    //All buffer accesses need to be protected by try/except
    pInBuf = pInBufUnmapped;
    pOutBuf = pOutBufUnmapped;

    CAMERAOPENHANDLE *pCamOpenHandle = reinterpret_cast<CAMERAOPENHANDLE *>(dwContext);

    if (NULL == pCamOpenHandle)
    {
        SetLastError(dwErr);
        return bRc;
    }

    CAMERADEVICE *pCamDevice = pCamOpenHandle->pCamDevice;
    
    if (NULL == pCamDevice)
    {
        SetLastError(dwErr);
        return bRc;
    }

    switch (Ioctl)
    {
        // Power Management Support.
        case IOCTL_POWER_CAPABILITIES:
#if (_WINCEOSVER<700)
        case IOCTL_POWER_QUERY:
#endif			
        case IOCTL_POWER_SET:
        case IOCTL_POWER_GET:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Power Management IOCTL\r\n"), dwContext));

            __try 
            {
                dwErr = pCamDevice->AdapterHandlePowerRequests(Ioctl, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x):Exception in Power Management IOCTL"), dwContext));
            }

            break;
        }

        case IOCTL_CS_PROPERTY:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): IOCTL_CS_PROPERTY\r\n"), dwContext));

            if ((NULL == pInBuf)
                 || (InBufLen < sizeof(CSPROPERTY))
                 || (NULL == pdwBytesTransferred))
            {
                SetLastError(dwErr);
                return bRc;
            }

            CSPROPERTY *pCsProp = reinterpret_cast<CSPROPERTY *>(pInBuf);

            __try 
            {
                dwErr = pCamDevice->AdapterHandleCustomRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);

                if (ERROR_NOT_SUPPORTED == dwErr)
                {
                    if (TRUE == IsEqualGUID(pCsProp->Set, CSPROPSETID_Pin))
                    {   
                        dwErr = pCamDevice->AdapterHandlePinRequests(pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, CSPROPSETID_VERSION))
                    {
                        dwErr = pCamDevice->AdapterHandleVersion( pOutBuf, OutBufLen, pdwBytesTransferred );
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, CSPROPSETID_Adapter))
                    {
                        dwErr = pCamDevice->AdapterHandlePropSetAdapter(pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID( pCsProp->Set, PROPSETID_VIDCAP_VIDEOPROCAMP))
                    {   
                        dwErr = pCamDevice->AdapterHandleVidProcAmpRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, PROPSETID_VIDCAP_CAMERACONTROL))
                    {   
                        dwErr = pCamDevice->AdapterHandleCamControlRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, PROPSETID_VIDCAP_VIDEOCONTROL))
                    {   
                        dwErr = pCamDevice->AdapterHandleVideoControlRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, PROPSETID_VIDCAP_DROPPEDFRAMES))
                    {   
                        dwErr = pCamDevice->AdapterHandleDroppedFramesRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                    else if (TRUE == IsEqualGUID(pCsProp->Set, CSPROPSETID_Metadata))
                    {   
                        dwErr = pCamDevice->AdapterHandleMetadataRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
                    }
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_IOCTL, ( _T("CAM_IOControl(%08x):Exception in IOCTL_CS_PROPERTY"), dwContext));
            }

            break;
        }

        default:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Unsupported IOCTL code %u\r\n"), dwContext, Ioctl));
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }
    }
    
    // pass back appropriate response codes
    SetLastError(dwErr);

    return ((dwErr == ERROR_SUCCESS) ? TRUE : FALSE);
}


EXTERN_C
DWORD
CAM_Open(
    DWORD Context, 
    DWORD Access,
    DWORD ShareMode
    )
{
    DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_Open(%x, 0x%x, 0x%x)\r\n"), Context, Access, ShareMode ) );

    UNREFERENCED_PARAMETER( ShareMode );
    UNREFERENCED_PARAMETER( Access );

    
    CAMERADEVICE     * pCamDevice     = reinterpret_cast<CAMERADEVICE *>( Context );
    CAMERAOPENHANDLE * pCamOpenHandle = NULL;
    HANDLE             hCurrentProc   = NULL;
    
    hCurrentProc = (HANDLE)GetCallerVMProcessId();

    ASSERT( pCamDevice != NULL );

    // We do a special processing for Power Manger and Device Manager.
    if ((Access &  DEVACCESS_BUSNAMESPACE) || (ShareMode & DEVACCESS_PMEXT_MODE) ) 
    {
        return NULL;
    }

    if ( pCamDevice->BindApplicationProc( hCurrentProc ) )
    {
        pCamOpenHandle = new CAMERAOPENHANDLE;

        if ( NULL == pCamOpenHandle )
        {
            DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_Open(%x): Not enought memory to create open handle\r\n"), Context ) );
        }
        else
        {
            pCamOpenHandle->pCamDevice = pCamDevice;
        }
    }
    else
    {
        SetLastError( ERROR_ALREADY_INITIALIZED );
    }

    return reinterpret_cast<DWORD>( pCamOpenHandle );
}


EXTERN_C
BOOL  
CAM_Close(
    DWORD Context
    ) 
{
    DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_Close(%x)\r\n"), Context ) );
    
    PCAMERAOPENHANDLE pCamOpenHandle = reinterpret_cast<PCAMERAOPENHANDLE>( Context );

    pCamOpenHandle->pCamDevice->UnBindApplicationProc( );
    pCamOpenHandle->pCamDevice = NULL;
    SAFEDELETE( pCamOpenHandle ) ;

    return TRUE;
}

EXTERN_C
VOID 
CAM_PowerUp(
    DWORD Context
    )
{
    DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_PowerUp(%x)\r\n"), Context ) );
    
    CAMERADEVICE* pCamDevice = reinterpret_cast<CAMERADEVICE *>( Context );
    CEDEVICE_POWER_STATE powerState = D0;
    HANDLE hCurrentProc = NULL;
    BOOL bUnbindNeeded = FALSE;
    
    hCurrentProc = (HANDLE)GetCallerVMProcessId();

    if (pCamDevice != NULL)
    {
        if (pCamDevice->BindApplicationProc( hCurrentProc ))
        {
            bUnbindNeeded = TRUE;
        }

        pCamDevice->AdapterHandlePowerRequests(IOCTL_POWER_SET, NULL, 0, (PUCHAR)&powerState, sizeof(powerState), NULL); 

        if (bUnbindNeeded)
        {
            pCamDevice->UnBindApplicationProc( );
        }
    }
}

EXTERN_C
VOID
CAM_PowerDown(
    DWORD Context
    )
{
    DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_PowerDown(%x)\r\n"), Context ) );
    
    CAMERADEVICE* pCamDevice = reinterpret_cast<CAMERADEVICE *>( Context );
    CEDEVICE_POWER_STATE powerState = D4;
    HANDLE hCurrentProc = NULL;
    BOOL bUnbindNeeded = FALSE;
    
    hCurrentProc = (HANDLE)GetCallerVMProcessId();

    if (pCamDevice != NULL)
    {
        if (pCamDevice->BindApplicationProc( hCurrentProc ))
        {
            bUnbindNeeded = TRUE;
        }

        pCamDevice->AdapterHandlePowerRequests(IOCTL_POWER_SET, NULL, 0, (PUCHAR)&powerState, sizeof(powerState), NULL);

        if (bUnbindNeeded)
        {
            pCamDevice->UnBindApplicationProc( );
        }
    }
}