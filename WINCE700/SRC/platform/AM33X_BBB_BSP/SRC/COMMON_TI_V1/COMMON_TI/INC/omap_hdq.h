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
//  File: omap_hdq.h
//
#ifndef __OMAP_HDQ_H
#define __OMAP_HDQ_H

#ifdef __cplusplus
extern "C" {
#endif

// disable zero size array warning
#pragma warning(disable:4200)

//------------------------------------------------------------------------------
//
//  Define:  HDQ_DEVICE_NAME
//
#define HDQ_DEVICE_NAME        L"HDQ1:"

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_HDQ_GUID {8D6EC8E0-A631-4bdc-BCB3-CC5BBB9B624A}
//
DEFINE_GUID(
    DEVICE_IFC_HDQ_GUID, 0x8d6ec8e0, 0xa631, 0x4bdc, 
    0xbc, 0xb3, 0xcc, 0x5b, 0xbb, 0x9b, 0x62, 0x4a
);


//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_HDQ
//
//  This structure is used to obtain HDQ interface funtion pointers
//  used for in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    DWORD context;
    DWORD (*pfnWrite)(DWORD context, void* pBuffer, DWORD dwSize);
    DWORD (*pfnRead)(DWORD context, void* pBuffer, DWORD dwSize);
} DEVICE_IFC_HDQ;

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_HDQ
//
//  This structure is used to store HDQ device context.
//
typedef struct {
    DEVICE_IFC_HDQ ifc;
    HANDLE hDevice;
} DEVICE_CONTEXT_HDQ;

//------------------------------------------------------------------------------
//
//  Type:  HDQ_1WIRE_READWRITE
//
//  This structure is used during read/write operations into the hdq driver.
//
typedef struct {
    UINT8   address;
    UINT8   data[];
} HDQ_1WIRE_READWRITE;

//***********************ADDED 6/20/08  MGJ   *******************************
#ifndef SHIP_BUILD

        #define IOCTL_HDQ_READREGS          \
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

        typedef struct {
            DWORD address;
            DWORD size;
        } IOCTL_HDQ_READREGS_IN;
#endif
//***************************END ADD  **************************************


//------------------------------------------------------------------------------

__inline VOID* HdqOpen()
{
    HANDLE hDevice;
    DEVICE_CONTEXT_HDQ *pContext = NULL;

    hDevice = CreateFile(HDQ_DEVICE_NAME, 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) goto clean;

    // Allocate memory for our handler...
    if ((pContext = (DEVICE_CONTEXT_HDQ *)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_HDQ))) == NULL) 
        {
        CloseHandle(hDevice);
        goto clean;
        }

    // Get function pointers, fail when IOCTL isn't supported...
    if (!DeviceIoControl(hDevice, IOCTL_DDK_GET_DRIVER_IFC, 
        (VOID*)&DEVICE_IFC_HDQ_GUID, sizeof(DEVICE_IFC_HDQ_GUID), 
        &pContext->ifc, sizeof(DEVICE_IFC_HDQ), NULL, NULL)) 
        {
        CloseHandle(hDevice);
        LocalFree(pContext);
        pContext = NULL;
        goto clean;
        }

    // Save device handle
    pContext->hDevice = hDevice;

clean:
    return pContext;
}

__inline VOID HdqClose(HANDLE hContext)
{
    DEVICE_CONTEXT_HDQ *pContext = (DEVICE_CONTEXT_HDQ *)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}

__inline DWORD HdqWrite(HANDLE hContext, UCHAR address, UCHAR data)
{
    UINT8 szBuffer[3];
    DEVICE_CONTEXT_HDQ *pContext = (DEVICE_CONTEXT_HDQ *)hContext;

    szBuffer[0] = address;
    szBuffer[1] = data;

    return pContext->ifc.pfnWrite(pContext->ifc.context, szBuffer, 2);
}

__inline DWORD HdqRead(HANDLE hContext, UCHAR address, UCHAR *pData)
{
    DWORD dwRet;
    UINT8 szBuffer[3];
    DEVICE_CONTEXT_HDQ *pContext = (DEVICE_CONTEXT_HDQ*)hContext;
    
    szBuffer[0] = address;
    dwRet = pContext->ifc.pfnRead(pContext->ifc.context, szBuffer, 2);
    *pData = szBuffer[1];
    return dwRet;
}


//------------------------------------------------------------------------------

#pragma warning(default:4200)

#ifdef __cplusplus
}
#endif

#endif
