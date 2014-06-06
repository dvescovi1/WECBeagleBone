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
//  File:  vrfb.h
//
#ifndef __VRFB_H
#define __VFRB_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define:  VRFB_DEVICE_NAME
//
#define VRFB_DEVICE_NAME        L"VRF1:"

#define VRFB_PIXELSIZE_1B       SMS_ROT_CONTROL_PIXELSIZE_1B
#define VRFB_PIXELSIZE_2B       SMS_ROT_CONTROL_PIXELSIZE_2B
#define VRFB_PIXELSIZE_4B       SMS_ROT_CONTROL_PIXELSIZE_4B

#define VRFB_ROTATE_ANGLE_0     0
#define VRFB_ROTATE_ANGLE_90    90
#define VRFB_ROTATE_ANGLE_180   180
#define VRFB_ROTATE_ANGLE_270   270


//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_VRFB_GUID
//
// {EDE6BB10-D844-4639-A7B6-CA199CD9EB34}
DEFINE_GUID(
    DEVICE_IFC_VRFB_GUID, 0xede6bb10, 0xd844, 0x4639,  
    0xa7, 0xb6, 0xca, 0x19, 0x9c, 0xd9, 0xeb, 0x34
    );


//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_VRFB
//
//  This structure is used to obtain VRFB interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    HANDLE  context;
    HANDLE (*pfnAllocateView)(HANDLE hContext, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr);
    BOOL   (*pfnReleaseView)(HANDLE hContext, HANDLE hView);
    BOOL   (*pfnGetViewInfo)(HANDLE hContext, HANDLE hView, VOID *pInfo);
    BOOL   (*pfnRotateView)(HANDLE hContext, HANDLE hView, DWORD dwRotateAngle);
    BOOL   (*pfnUpdateView)(HANDLE hContext, HANDLE hView, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr);
    DWORD  (*pfnNumDisplayViews)(HANDLE hContext);
    HANDLE (*pfnGetDisplayView)(HANDLE hContext, DWORD dwIndex);
    BOOL   (*pfnGetDisplayViewInfo)(HANDLE hContext, DWORD dwIndex, DWORD dwRotateAngle, BOOL bMirror, VOID *pInfo);
} DEVICE_IFC_VRFB;

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_VRFB
//
//  This structure is used to store VRFB device context.
//
typedef struct {
    DEVICE_IFC_VRFB ifc;
    HANDLE hDevice;
} DEVICE_CONTEXT_VRFB;


//
//  Type:  VRFB_VIEW_INFO
//
//  This structure is used to get VRFB view information
//
typedef struct {
    HANDLE  hView;
    DWORD   dwPixelSize;
    DWORD   dwPixelSizeBytes;
    DWORD   dwWidth;
    DWORD   dwHeight;
    DWORD   dwPageWidth;
    DWORD   dwPageHeight;
    DWORD   dwImageWidth;
    DWORD   dwImageHeight;
    DWORD   dwImageStride;
    DWORD   dwOriginOffset;
    DWORD   dwRotationAngle;
    DWORD   dwVirtualAddr;
    DWORD   dwPhysicalAddrInput;
    DWORD   dwPhysicalAddrOutput;
    DWORD   dwPhysicalBufferAddr;
} VRFB_VIEW_INFO;


//------------------------------------------------------------------------------

#define IOCTL_VRFB_ALLOCATEVIEW         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_RELEASEVIEW          \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_GETVIEWINFO          \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_ROTATEVIEW           \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_UPDATEVIEW           \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0704, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_NUMDISPLAYVIEWS      \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0705, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_GETDISPLAYVIEW       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0706, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VRFB_GETDISPLAYVIEWINFO   \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0707, METHOD_BUFFERED, FILE_ANY_ACCESS)


//------------------------------------------------------------------------------

typedef struct {
    DWORD       dwIndex;
    DWORD       dwRotateAngle;
    BOOL        bMirror;
} IOCTL_VRFB_GETDISPLAYVIEWINFO_IN;


//------------------------------------------------------------------------------
//
//  Functions: VRFBxxx
//
__inline HANDLE VRFBOpen()
{
    HANDLE hDevice;
    DEVICE_CONTEXT_VRFB *pContext = NULL;

    hDevice = CreateFile(VRFB_DEVICE_NAME, 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) goto clean;

    // Allocate memory for our handler...
    if ((pContext = (DEVICE_CONTEXT_VRFB *)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_VRFB)
    )) == NULL) {
        CloseHandle(hDevice);
        goto clean;
    }

    // Get function pointers.  If not possible (b/c of cross process calls), use IOCTLs instead
    if (!DeviceIoControl(
        hDevice, IOCTL_DDK_GET_DRIVER_IFC, (VOID*)&DEVICE_IFC_VRFB_GUID,
        sizeof(DEVICE_IFC_VRFB_GUID), &pContext->ifc, sizeof(DEVICE_IFC_VRFB),
        NULL, NULL
    )) {
        //  Need to use IOCTLs instead of direct function ptrs
        pContext->ifc.context = 0;
    }

    // Save device handle
    pContext->hDevice = hDevice;

clean:
    return pContext;
}

__inline VOID VRFBClose(HANDLE hContext)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}

__inline HANDLE VRFBAllocateView(HANDLE hContext, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    HANDLE              hHandle = NULL;

    if( pContext->ifc.context )
    {
        hHandle = pContext->ifc.pfnAllocateView(pContext->ifc.context, dwPixelSize, dwWidth, dwHeight, dwBufferPhysAddr);
    }
    else
    {
        VRFB_VIEW_INFO  viewInfo;
        
        viewInfo.dwPixelSize            = dwPixelSize;
        viewInfo.dwWidth                = dwWidth;
        viewInfo.dwHeight               = dwHeight;
        viewInfo.dwPhysicalBufferAddr   = dwBufferPhysAddr;

        DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_ALLOCATEVIEW, 
                        &viewInfo,
                        sizeof(VRFB_VIEW_INFO),
                        &hHandle,
                        sizeof(hHandle),
                        NULL,
                        NULL );
    }
    
    return hHandle; 
}

__inline BOOL   VRFBReleaseView(HANDLE hContext, HANDLE hView)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    BOOL                bResult = FALSE;

    if( pContext->ifc.context )
    {
        bResult = pContext->ifc.pfnReleaseView(pContext->ifc.context, hView);
    }
    else
    {
        bResult = DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_RELEASEVIEW, 
                        &hView,
                        sizeof(HANDLE*),
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
    
    return bResult; 
}
    
__inline BOOL   VRFBGetViewInfo(HANDLE hContext, HANDLE hView, VRFB_VIEW_INFO *pInfo)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    BOOL                bResult = FALSE;

    if( pContext->ifc.context )
    {
        bResult = pContext->ifc.pfnGetViewInfo(pContext->ifc.context, hView, pInfo);
    }
    else
    {
        bResult = DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_GETVIEWINFO, 
                        &hView,
                        sizeof(HANDLE*),
                        pInfo,
                        sizeof(VRFB_VIEW_INFO),
                        NULL,
                        NULL );
    }
    
    return bResult; 
}

__inline BOOL   VRFBRotateView(HANDLE hContext, HANDLE hView, DWORD dwRotateAngle)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    BOOL                bResult = FALSE;

    if( pContext->ifc.context )
    {
        bResult = pContext->ifc.pfnRotateView(pContext->ifc.context, hView, dwRotateAngle);
    }
    else
    {
        VRFB_VIEW_INFO  viewInfo;
        
        viewInfo.hView           =  hView;
        viewInfo.dwRotationAngle =  dwRotateAngle;

        bResult = DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_ROTATEVIEW, 
                        &viewInfo,
                        sizeof(VRFB_VIEW_INFO),
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
    
    return bResult; 
}

__inline BOOL   VRFBUpdateView(HANDLE hContext, HANDLE hView, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    BOOL                bResult = FALSE;

    if( pContext->ifc.context )
    {
        bResult = pContext->ifc.pfnUpdateView(pContext->ifc.context, hView, dwPixelSize, dwWidth, dwHeight, dwBufferPhysAddr);
    }
    else
    {
        VRFB_VIEW_INFO  viewInfo;
        
        viewInfo.hView                = hView;
        viewInfo.dwPixelSize          = dwPixelSize;
        viewInfo.dwWidth              = dwWidth;
        viewInfo.dwHeight             = dwHeight;
        viewInfo.dwPhysicalBufferAddr = dwBufferPhysAddr;

        bResult = DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_UPDATEVIEW, 
                        &viewInfo,
                        sizeof(VRFB_VIEW_INFO),
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
    
    return bResult; 
}

__inline DWORD  VRFBNumDisplayViews(HANDLE hContext)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    DWORD                dwResult = 0;

    if( pContext->ifc.context )
    {
        dwResult = pContext->ifc.pfnNumDisplayViews(pContext->ifc.context);
    }
    else
    {
        DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_NUMDISPLAYVIEWS, 
                        NULL,
                        0,
                        &dwResult,
                        sizeof(dwResult),
                        NULL,
                        NULL );
    }
    
    return dwResult; 
}
    
__inline HANDLE VRFBGetDisplayView(HANDLE hContext, DWORD dwIndex)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    HANDLE              hHandle = NULL;

    if( pContext->ifc.context )
    {
        hHandle = pContext->ifc.pfnGetDisplayView(pContext->ifc.context, dwIndex);
    }
    else
    {
        DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_GETDISPLAYVIEW, 
                        &dwIndex,
                        sizeof(DWORD*),
                        &hHandle,
                        sizeof(hHandle),
                        NULL,
                        NULL );
    }
    
    return hHandle; 
}
    
__inline BOOL VRFBGetDisplayViewInfo(HANDLE hContext, DWORD dwIndex, DWORD dwRotateAngle, BOOL bMirror, VRFB_VIEW_INFO *pInfo)
{
    DEVICE_CONTEXT_VRFB *pContext = (DEVICE_CONTEXT_VRFB *)hContext;
    BOOL                bResult = FALSE;

    if( pContext->ifc.context )
    {
        bResult = pContext->ifc.pfnGetDisplayViewInfo(pContext->ifc.context, dwIndex, dwRotateAngle, bMirror, pInfo);
    }
    else
    {
        IOCTL_VRFB_GETDISPLAYVIEWINFO_IN    infoIn;
        
        infoIn.dwIndex          = dwIndex;
        infoIn.dwRotateAngle    = dwRotateAngle;
        infoIn.bMirror          = bMirror;
            
        bResult = DeviceIoControl(pContext->hDevice, 
                        IOCTL_VRFB_GETDISPLAYVIEWINFO, 
                        &infoIn,
                        sizeof(infoIn),
                        pInfo,
                        sizeof(VRFB_VIEW_INFO),
                        NULL,
                        NULL );
    }
    
    return bResult; 
}
    
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
