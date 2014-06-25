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
//  File:  madc.h
//
#ifndef __MADC_H
#define __MADC_H

#ifdef __cplusplus
extern "C" {
#endif


#define MADC_CHANNEL_0                      0x00000001
#define MADC_CHANNEL_1                      0x00000002
#define MADC_CHANNEL_2                      0x00000004
#define MADC_CHANNEL_3                      0x00000008
#define MADC_CHANNEL_4                      0x00000010
#define MADC_CHANNEL_5                      0x00000020
#define MADC_CHANNEL_6                      0x00000040
#define MADC_CHANNEL_7                      0x00000080
#define MADC_CHANNEL_8                      0x00000100
#define MADC_CHANNEL_9                      0x00000200
#define MADC_CHANNEL_10                     0x00000400
#define MADC_CHANNEL_11                     0x00000800
#define MADC_CHANNEL_12                     0x00001000
#define MADC_CHANNEL_13                     0x00002000
#define MADC_CHANNEL_14                     0x00004000
#define MADC_CHANNEL_15                     0x00008000
#define MADC_CHANNEL_BCI0                   0x00010000
#define MADC_CHANNEL_BCI1                   0x00020000
#define MADC_CHANNEL_BCI2                   0x00040000
#define MADC_CHANNEL_BCI3                   0x00080000
#define MADC_CHANNEL_BCI4                   0x00100000


//------------------------------------------------------------------------------
//
//  Define:  MADC_DEVICE_NAME
//
#define MADC_DEVICE_NAME                    L"ADC1:"

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_MADC_GUID : {E61CA799-8EF5-41e3-8237-2BD8879BA4AF}
//
DEFINE_GUID(
    DEVICE_IFC_MADC_GUID, 0xe61ca799, 0x8ef5, 0x41e3, 
    0x82, 0x37, 0x2b, 0xd8, 0x87, 0x9b, 0xa4, 0xaf
);

//------------------------------------------------------------------------------

#define IOCTL_MADC_READVALUE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0400, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MADC_CONVERTTOVOLTS     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0401, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_MADC
//
//  This structure is used to obtain MADC interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    DWORD context;
    DWORD (*pfnReadValue)(DWORD ctx, DWORD mask, DWORD *pdwResults, DWORD size);
    DWORD (*pfnConvertToVolts)(DWORD ctx, DWORD channel, DWORD *pdwValues, DWORD *pResults, DWORD count);
} DEVICE_IFC_MADC;

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_MADC
//
//  This structure is used to store MADC device context
//
typedef struct {
    DEVICE_IFC_MADC ifc;
    HANDLE hDevice;
} DEVICE_CONTEXT_MADC;


//------------------------------------------------------------------------------

typedef struct {
    DWORD mask;
    DWORD count;
    DWORD *pdwValues;
} IOCTL_MADC_CONVERTTOVOLTS_IN;


//------------------------------------------------------------------------------
//
//  Functions: MADCxxx
//
__inline
HANDLE 
MADCOpen(
    )
{
    HANDLE hDevice;
    DEVICE_CONTEXT_MADC *pContext = NULL;

    hDevice = CreateFile(MADC_DEVICE_NAME, 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) goto cleanUp;

    // Allocate memory for our handler...
    pContext = (DEVICE_CONTEXT_MADC*)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_MADC)
        );
    if (pContext == NULL)
        {
        CloseHandle(hDevice);
        goto cleanUp;
        }

    // Get function pointers, fail when IOCTL isn't supported...
    if (!DeviceIoControl(
            hDevice, IOCTL_DDK_GET_DRIVER_IFC, (VOID*)&DEVICE_IFC_MADC_GUID,
            sizeof(DEVICE_IFC_MADC_GUID), &pContext->ifc, sizeof(DEVICE_IFC_MADC),
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
MADCClose(
    HANDLE hContext
    )
{
    DEVICE_CONTEXT_MADC *pContext = (DEVICE_CONTEXT_MADC*)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}

__inline
DWORD
MADCReadValue(
    HANDLE hContext, 
    DWORD mask,
    DWORD *pdwResults,
    DWORD size
    )
{
    DEVICE_CONTEXT_MADC *pContext = (DEVICE_CONTEXT_MADC*)hContext;
    return pContext->ifc.pfnReadValue(
        pContext->ifc.context, mask, pdwResults, size
        );
}

    
__inline
DWORD 
MADCConvertToVolts(
    HANDLE  hContext, 
    DWORD   channel,
    DWORD  *pdwValues,
    DWORD  *pResults,
    DWORD   count
    )
{
    DEVICE_CONTEXT_MADC *pContext = (DEVICE_CONTEXT_MADC*)hContext;
    return pContext->ifc.pfnConvertToVolts(
        pContext->ifc.context, channel, pdwValues, pResults, count
        );
}


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
