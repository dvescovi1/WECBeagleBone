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

    PinDriver.cpp

Abstract:

    MDD Implementation of Pin subdevice for Sample camera driver.

Notes:


Revision History:

--*/

#include <windows.h>
#include <pm.h>
#include <devload.h>
#include <nkintr.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "CameraDebug.h"
#include <camera.h>

#include "CameraDriver.h"
#include "CameraPinDriver.h"

#pragma warning(disable:6320)

EXTERN_C
DWORD
PIN_Init(LPCTSTR Context, LPVOID lpvBusContext)
{
    DEBUGMSG(ZONE_INIT, (_T("PIN_Init: context %s\n"), Context));
    PPININITHANDLE pPinInitDev = NULL;

    pPinInitDev = new PININITHANDLE;

    if (NULL != pPinInitDev)
    {
        pPinInitDev->pCamDevice = reinterpret_cast<PCAMERADEVICE>(lpvBusContext);

        if (NULL == pPinInitDev->pCamDevice)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            SAFEDELETE(pPinInitDev);
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("PIN_Init: Initialization Failed")));
        }
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
    }

    DEBUGMSG(ZONE_INIT, (_T("PIN_Init: returning 0x%08x\r\n"), reinterpret_cast<DWORD>(pPinInitDev)));

    return reinterpret_cast<DWORD>(pPinInitDev);
}


EXTERN_C
BOOL
PIN_Deinit(
    DWORD dwContext
   )
{
    DEBUGMSG(ZONE_INIT, (_T("PIN_Deinit\r\n")));

    PPININITHANDLE pPinInitDev = reinterpret_cast<PPININITHANDLE>(dwContext);
    SAFEDELETE(pPinInitDev) ;

    return TRUE;
}


EXTERN_C
BOOL
PIN_IOControl(
    DWORD  dwContext,
    DWORD  Ioctl,
    PUCHAR pInBufUnmapped,
    DWORD  InBufLen,
    PUCHAR pOutBufUnmapped,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
  )
{
    DEBUGMSG(ZONE_FUNCTION, (_T("PIN_IOControl(%08x): IOCTL:0x%x, InBuf:0x%x, InBufLen:%d, OutBuf:0x%x, OutBufLen:0x%x)\r\n"), dwContext, Ioctl, pInBufUnmapped, InBufLen, pOutBufUnmapped, OutBufLen));

    DWORD dwErr = ERROR_INVALID_PARAMETER;
    PPINDEVICE pPinDevice = reinterpret_cast<PPINDEVICE>(dwContext);
    UCHAR * pInBuf = NULL;
    UCHAR * pOutBuf = NULL;

    pInBuf = pInBufUnmapped;

    pOutBuf = pOutBufUnmapped;

    switch (Ioctl)
    {
    case IOCTL_CS_PROPERTY:
        DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): IOCTL_CS_PROPERTY\r\n"), dwContext));

        __try
        {
            CSPROPERTY csProp = {0};

            if ((NULL == pInBuf)
                 || (InBufLen < sizeof(CSPROPERTY))
                 || (NULL == pdwBytesTransferred))
            {
                break;
            }

            if(!CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
            {
                break;
            }

            dwErr = pPinDevice->PinHandleCustomRequests(pInBuf,InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
            if (ERROR_NOT_SUPPORTED == dwErr)
            {
                if (TRUE == IsEqualGUID(csProp.Set, CSPROPSETID_Connection))
                {
                    dwErr = pPinDevice->PinHandleConnectionRequests(&csProp, pOutBuf, OutBufLen, pdwBytesTransferred);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x):Exception in IOCTL_CS_PROPERTY"), dwContext));
        }

        break;


    case IOCTL_CS_BUFFERS:
        DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): IOCTL_CS_BUFFERS\r\n"), dwContext));

        __try
        {
            CSBUFFER_INFO  csBufferInfo;

            if ((NULL == pInBuf) ||
                (InBufLen  < sizeof(CSBUFFER_INFO)) ||
                (!CeSafeCopyMemory(&csBufferInfo, pInBuf, sizeof(csBufferInfo))))
            {
                if(pdwBytesTransferred)
                {
                    *pdwBytesTransferred = 0;
                }

                DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): IOCTL_CS_BUFFERS. Invalid parameters\r\n"), dwContext));
                break;
            }

            dwErr = pPinDevice->PinHandleBufferRequest(csBufferInfo, pOutBuf, OutBufLen, pdwBytesTransferred);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x):Exception in IOCTL_CS_BUFFERS"), dwContext));
        }

        break;


    case IOCTL_STREAM_INSTANTIATE:
        __try
        {
            DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): IOCTL_STREAM_INSTANTIATE\r\n"), dwContext));

            CSPROPERTY_STREAMEX_S csPropStreamEx = {0};

            if ((NULL == pInBuf)
                 || (InBufLen < sizeof (CSPROPERTY_STREAMEX_S))
                 || (NULL == pdwBytesTransferred))
            {
                DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): IOCTL_STREAM_INSTANTIATE. Insufficient buffer\r\n"), dwContext));
                break;
            }

            CeSafeCopyMemory(&csPropStreamEx, pInBuf, sizeof(CSPROPERTY_STREAMEX_S));

            if (TRUE == IsEqualGUID(csPropStreamEx.CsPin.Property.Set, CSPROPSETID_StreamEx))
            {
                switch (csPropStreamEx.CsPin.Property.Id)
                {
                case CSPROPERTY_STREAMEX_INIT:
                    dwErr = pPinDevice->StreamInstantiate(&csPropStreamEx, pOutBuf, OutBufLen, pdwBytesTransferred);
                    break;

                default:
                    DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Request\r\n"), dwContext));
                    break;
                }
            }
            else
            {
                DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): Unsupported PropertySet Request for IOCTL_STREAM_INSTANTIATE%u\r\n"), dwContext, csPropStreamEx.CsPin.Property.Set));
                dwErr = ERROR_NOT_SUPPORTED;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x):Exception in IOCTL_STREAM_INSTANTIATE"), dwContext));
        }

        break;

    default:
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Unsupported IOCTL code %u\r\n"), dwContext, Ioctl));
        dwErr = ERROR_NOT_SUPPORTED;
        break;
    }

    // pass back appropriate response codes
    SetLastError(dwErr);
    return ((dwErr == ERROR_SUCCESS) ? TRUE : FALSE);
}


EXTERN_C
DWORD
PIN_Open(
    DWORD Context,
    DWORD Access,
    DWORD ShareMode
   )
{
    DEBUGMSG(ZONE_FUNCTION, (_T("PIN_Open(%x, 0x%x, 0x%x)\r\n"), Context, Access, ShareMode));

    UNREFERENCED_PARAMETER(ShareMode);
    UNREFERENCED_PARAMETER(Access);
   
    PPININITHANDLE pPinInit = reinterpret_cast<PPININITHANDLE>(Context);
    ASSERT(NULL != pPinInit);

    PPINDEVICE pPinDevice = new PINDEVICE;
    if (NULL == pPinDevice)
    {
        DEBUGMSG(ZONE_FUNCTION, (_T("PIN_Open(%x): Not enough memory to create pin device\r\n"), Context));
    }
    else
    {
        if (false == pPinDevice->InitializeSubDevice(pPinInit->pCamDevice))
        {
            SAFEDELETE(pPinDevice);
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("PIN_Init: Initialization Failed")));
        }

    }

    return reinterpret_cast<DWORD>(pPinDevice);
}


EXTERN_C
BOOL
PIN_Close(
    DWORD Context
   )
{
    DEBUGMSG(ZONE_FUNCTION, (_T("PIN_Close(%x)\r\n"), Context));

    PPINDEVICE pPinDev = reinterpret_cast<PPINDEVICE>(Context);
    pPinDev->CloseSubDevice();
    SAFEDELETE(pPinDev);

    return TRUE;
}

