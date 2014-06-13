// All rights reserved ADENEO EMBEDDED 2010
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
//
//  File:  spi.h
//
#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_SPI_GUID
//
// {2E559225-C95E-4300-86E9-6A5CBC07328F}
DEFINE_GUID(
	DEVICE_IFC_SPI_GUID, 0x2e559225, 0xc95e, 0x4300,
	0x86, 0xe9, 0x6a, 0x5c, 0xbc, 0x7, 0x32, 0x8f
	);


//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_SPI
//
//  This structure is used to obtain SPI interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    DWORD context;
    BOOL  (*pfnConfigure)(DWORD context, DWORD address, DWORD config);
    DWORD (*pfnRead)(DWORD context, VOID *pBuffer, DWORD size);
    DWORD (*pfnWrite)(DWORD context, VOID *pBuffer, DWORD size);
    DWORD (*pfnWriteRead)(DWORD context, DWORD size, VOID *pInBuffer, VOID *pOutBuffer);
    BOOL  (*pfnLockController)(DWORD context, DWORD timeout);
    BOOL  (*pfnUnlockController)(DWORD context);
    BOOL  (*pfnEnableChannel)(DWORD context);
    BOOL  (*pfnDisableChannel)(DWORD context);
    DWORD (*pfnAsyncWriteRead)(DWORD context, DWORD size, VOID *pInBuffer, VOID *pOutBuffer);
    DWORD (*pfnWaitForAsyncWriteReadComplete)(DWORD context, DWORD, VOID*);
    BOOL (*pfnSetSlaveMode)(DWORD context);
} DEVICE_IFC_SPI;

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_SPI
//
//  This structure is used to store SPI device context.
//
typedef struct {
    DEVICE_IFC_SPI ifc;
    HANDLE hDevice;
} DEVICE_CONTEXT_SPI;

//------------------------------------------------------------------------------

#define IOCTL_SPI_CONFIGURE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_WRITEREAD     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0201, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_ASYNC_WRITEREAD     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0202, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_ASYNC_WRITEREAD_COMPLETE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_SET_SLAVEMODE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0204, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_LOCK_CTRL     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0205, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_UNLOCK_CTRL     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0206, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_ENABLE_CHANNEL     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0207, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SPI_DISABLE_CHANNEL     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0208, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD address;
    DWORD config;
} IOCTL_SPI_CONFIGURE_IN;


HANDLE SPIOpen(LPCTSTR pSpiName);
VOID SPIClose(HANDLE hContext);
BOOL SPILockController(HANDLE hContext, DWORD dwTimeout);
BOOL SPIUnlockController(HANDLE hContext);
BOOL SPIEnableChannel(HANDLE hContext);
BOOL SPIDisableChannel(HANDLE hContext);
BOOL SPIConfigure(HANDLE hContext, DWORD address, DWORD config);
BOOL SPISetSlaveMode(HANDLE hContext);
DWORD SPIRead(HANDLE hContext, DWORD size, VOID *pBuffer);
DWORD SPIWrite(HANDLE hContext, DWORD size, VOID *pBuffer);
DWORD SPIWriteRead(HANDLE hContext, DWORD size, VOID *pOutBuffer, VOID *pInBuffer);
DWORD SPIAsyncWriteRead(HANDLE hContext, DWORD size, VOID *pOutBuffer, VOID *pInBuffer);
DWORD SPIWaitForAsyncWriteReadComplet(HANDLE hContext, DWORD size, VOID *pOutBuffer);
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
