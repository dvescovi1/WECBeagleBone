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
//  File: omap_mcspi.cpp
//
#include "omap.h"
#include "ceddkex.h"
#include <initguid.h>
#include "sdk_spi.h"

//------------------------------------------------------------------------------
//
//  Functions: SPIxxx
//
HANDLE SPIOpen(LPCTSTR pSpiName)
{
    HANDLE hDevice;
    DEVICE_CONTEXT_SPI *pContext = NULL;
   
    hDevice = CreateFile(pSpiName, GENERIC_READ | GENERIC_WRITE,    
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);    
    if (hDevice == INVALID_HANDLE_VALUE) goto clean;

    // Allocate memory for our handler...
    if ((pContext = (DEVICE_CONTEXT_SPI *)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_SPI)
    )) == NULL) {
        CloseHandle(hDevice);
        goto clean;
    }

    // Get function pointers.  If not possible (b/c of cross process calls), use IOCTLs instead
    if (!DeviceIoControl(
        hDevice, IOCTL_DDK_GET_DRIVER_IFC, (VOID*)&DEVICE_IFC_SPI_GUID,
        sizeof(DEVICE_IFC_SPI_GUID), &pContext->ifc, sizeof(DEVICE_IFC_SPI),
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

VOID SPIClose(HANDLE hContext)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}

BOOL SPILockController(HANDLE hContext, DWORD dwTimeout)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnLockController(pContext->ifc.context, dwTimeout);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_LOCK_CTRL,
                        &dwTimeout,
                        sizeof(dwTimeout),
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}

BOOL SPIUnlockController(HANDLE hContext)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnUnlockController(pContext->ifc.context);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_UNLOCK_CTRL,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}

BOOL SPIEnableChannel(HANDLE hContext)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnEnableChannel(pContext->ifc.context);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_ENABLE_CHANNEL,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}

BOOL SPIDisableChannel(HANDLE hContext)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnDisableChannel(pContext->ifc.context);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_DISABLE_CHANNEL,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}


BOOL SPIConfigure(HANDLE hContext, DWORD address, DWORD config)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
	    return pContext->ifc.pfnConfigure(pContext->ifc.context, address, config);
	}
    else
    {
        IOCTL_SPI_CONFIGURE_IN  dwIn;

        dwIn.address = address;
        dwIn.config = config;

        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_CONFIGURE,
                        &dwIn,
                        sizeof(dwIn),
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}

BOOL SPISetSlaveMode(HANDLE hContext)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnSetSlaveMode(pContext->ifc.context);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_SET_SLAVEMODE,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL );
    }
}

DWORD SPIRead(HANDLE hContext, DWORD size, VOID *pBuffer)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
    	return pContext->ifc.pfnRead(pContext->ifc.context, pBuffer, size);
	}
    else
    {
        DWORD   dwCount = 0;
		BOOL ret; 
		ret = ReadFile( pContext->hDevice, pBuffer, size, &dwCount, NULL );
        return dwCount;
    }
}

DWORD SPIWrite(HANDLE hContext, DWORD size, VOID *pBuffer)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
	    return pContext->ifc.pfnWrite(pContext->ifc.context, pBuffer, size);
	}
    else
    {
        DWORD   dwCount = 0;

        WriteFile( pContext->hDevice, pBuffer, size, &dwCount, NULL );
        return dwCount;
    }
}

DWORD SPIWriteRead(HANDLE hContext, DWORD size, VOID *pOutBuffer, VOID *pInBuffer)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
	    return pContext->ifc.pfnWriteRead(pContext->ifc.context, size, pOutBuffer, pInBuffer);
	}
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_WRITEREAD,
                        pInBuffer,
                        size,
                        pOutBuffer,
                        size,
                        NULL,
                        NULL );
    }
}

DWORD SPIAsyncWriteRead(HANDLE hContext, DWORD size, VOID *pOutBuffer, VOID *pInBuffer)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnAsyncWriteRead(pContext->ifc.context, size, pOutBuffer, pInBuffer);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_ASYNC_WRITEREAD,
                        pInBuffer,
                        size,
                        pOutBuffer,
                        size,
                        NULL,
                        NULL );
    }
}

DWORD SPIWaitForAsyncWriteReadComplet(HANDLE hContext, DWORD size, VOID *pOutBuffer)
{
    DEVICE_CONTEXT_SPI *pContext = (DEVICE_CONTEXT_SPI *)hContext;
    if( pContext->ifc.context )
    {
        return pContext->ifc.pfnWaitForAsyncWriteReadComplete(pContext->ifc.context, size, pOutBuffer);
    }
    else
    {
        return DeviceIoControl(pContext->hDevice,
                        IOCTL_SPI_ASYNC_WRITEREAD_COMPLETE,
                        NULL,
                        0,
                        pOutBuffer,
                        size,
                        NULL,
                        NULL );
    }
}
