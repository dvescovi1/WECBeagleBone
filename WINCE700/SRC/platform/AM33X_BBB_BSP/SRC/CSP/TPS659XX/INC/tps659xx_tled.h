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
//  File:  tled.h
//
#ifndef __TLED_H
#define __TLED_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define:  MADC_DEVICE_NAME
//
#define TLED_DEVICE_NAME                    L"TLD1:"

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_TLED_GUID : {2C8C352A-2C98-415e-8839-8F4E9A4A9F33}
//
DEFINE_GUID(
    DEVICE_IFC_TLED_GUID, 0x2c8c352a, 0x2c98, 0x415e, 
    0x88, 0x39, 0x8f, 0x4e, 0x9a, 0x4a, 0x9f, 0x33
);


//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_TLED
//
//  This structure is used to obtain MADC interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    DWORD context;
    BOOL  (*pfnSetChannel)(DWORD ctx, DWORD channel);
    BOOL  (*pfnSetDutyCycle)(DWORD ctx, DWORD value);
} DEVICE_IFC_TLED;

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_TLD
//
//  This structure is used to store TLD device context
//
typedef struct {
    DEVICE_IFC_TLED ifc;
    HANDLE hDevice;
} DEVICE_CONTEXT_TLED;

//------------------------------------------------------------------------------
//
//  Functions: TLEDxxx
//
__inline
HANDLE 
TLEDOpen(
    )
{
    HANDLE hDevice;
    DEVICE_CONTEXT_TLED *pContext = NULL;

    hDevice = CreateFile(TLED_DEVICE_NAME, 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) goto cleanUp;

    // Allocate memory for our handler...
    pContext = (DEVICE_CONTEXT_TLED*)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_TLED)
        );
    if (pContext == NULL)
        {
        CloseHandle(hDevice);
        goto cleanUp;
        }

    // Get function pointers, fail when IOCTL isn't supported...
    if (!DeviceIoControl(
            hDevice, IOCTL_DDK_GET_DRIVER_IFC, (VOID*)&DEVICE_IFC_TLED_GUID,
            sizeof(DEVICE_IFC_TLED_GUID), &pContext->ifc, sizeof(DEVICE_IFC_TLED),
            NULL, NULL))
        {
        CloseHandle(hDevice);
        LocalFree(pContext);
        pContext = NULL;
        goto cleanUp;
        }

    // Save device handle
    pContext->hDevice = hDevice;

cleanUp:
    return pContext;
}

__inline
VOID
TLEDClose(
    HANDLE hContext
    )
{
    DEVICE_CONTEXT_TLED *pContext = (DEVICE_CONTEXT_TLED*)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}

__inline
BOOL
TLEDSetChannel(
    HANDLE hContext, 
    DWORD channel
    )
{
    DEVICE_CONTEXT_TLED *pContext = (DEVICE_CONTEXT_TLED*)hContext;
    return pContext->ifc.pfnSetChannel(
        pContext->ifc.context, channel
        );
}
    
__inline
BOOL 
TLEDSetDutyCycle(
    HANDLE hContext, 
    DWORD value
    )
{
    DEVICE_CONTEXT_TLED *pContext = (DEVICE_CONTEXT_TLED*)hContext;
    return pContext->ifc.pfnSetDutyCycle(
        pContext->ifc.context, value
        );
}


//------------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
