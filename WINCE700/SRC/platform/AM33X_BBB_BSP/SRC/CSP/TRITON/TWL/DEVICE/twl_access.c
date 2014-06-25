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

#include "triton.h"
#include "twl.h"
#include "sdk_i2c.h"
#include "bsp_cfg.h"
//------------------------------------------------------------------------------
//
//  Type:  DEVICE_CONTEXT_TWL
//
//  This structure is used to store TWL device context
//
typedef struct {
    DEVICE_IFC_TWL ifc;
    HANDLE hDevice;
    DWORD seekAddress;
} DEVICE_CONTEXT_TWL;

//------------------------------------------------------------------------------
//
//  Functions: TWLxxx
//

HANDLE 
TWLOpen(
    )
{
    HANDLE hDevice;
    DEVICE_CONTEXT_TWL *pContext = NULL;

    hDevice = CreateFile(TWL_DEVICE_NAME, 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) goto cleanUp;

    // Allocate memory for our handler...
    pContext = (DEVICE_CONTEXT_TWL*)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_TWL)
        );
    if (pContext == NULL)
        {
        CloseHandle(hDevice);
        goto cleanUp;
        }

    // Get function pointers, fail when IOCTL isn't supported...
    if (!DeviceIoControl(
            hDevice, IOCTL_DDK_GET_DRIVER_IFC, (VOID*)&DEVICE_IFC_TWL_GUID,
            sizeof(DEVICE_IFC_TWL_GUID), &pContext->ifc, sizeof(DEVICE_IFC_TWL),
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


VOID
TWLClose(
    HANDLE hContext
    )
{
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;
    CloseHandle(pContext->hDevice);
    LocalFree(pContext);
}


BOOL
TWLReadRegs(
    HANDLE hContext, 
    DWORD address,
    VOID *pBuffer,
    DWORD size
    )
{
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;
    if (pContext->ifc.pfnReadRegs != NULL)
        {
        return pContext->ifc.pfnReadRegs(
            pContext->ifc.context, address, pBuffer, size
            );
        }
    else
        {
        DWORD dwCount = 0;
        if (pContext->seekAddress != address)
            {
            SetFilePointer(pContext->hDevice, address, NULL, FILE_CURRENT);
            pContext->seekAddress = address;
            }
        ReadFile(pContext->hDevice, pBuffer, size, &dwCount, NULL);
        return dwCount;
        }
}


BOOL 
TWLWriteRegs(
    HANDLE hContext, 
    DWORD address,
    const VOID *pBuffer,
    DWORD size
    )
{
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;
    if (pContext->ifc.pfnWriteRegs != NULL)
        {
        return pContext->ifc.pfnWriteRegs(
            pContext->ifc.context, address, pBuffer, size
            );
        }
    else
        {
        DWORD dwCount = 0;
        if (pContext->seekAddress != address)
            {
            SetFilePointer(pContext->hDevice, address, NULL, FILE_CURRENT);
            pContext->seekAddress = address;
            }
        return WriteFile(pContext->hDevice, pBuffer, size, &dwCount, NULL);
        }
}

BOOL 
TWLInterruptInitialize(
    HANDLE hContext, 
    DWORD intrId,
    HANDLE hEvent
    )
{
    IOCTL_TWL_INTRINIT_IN inParam;
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;

    if (pContext->ifc.pfnInterruptInitialize != NULL)
        {
        return pContext->ifc.pfnInterruptInitialize(
            pContext->ifc.context, intrId, hEvent
            );
        }
    else
        {
        inParam.intrId = intrId;
        inParam.hEvent = hEvent;
        inParam.procId = GetCurrentProcessId();
        return DeviceIoControl(pContext->hDevice,
                               IOCTL_TWL_INTRINIT,
                               &inParam,
                               sizeof(inParam),
                               NULL,
                               0,
                               NULL,
                               NULL 
                               );
        }
}


BOOL 
TWLInterruptMask(
    HANDLE hContext, 
    DWORD intrId,
    BOOL  bEnable
    )
{
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;

    if (pContext->ifc.pfnInterruptMask != NULL)
        {
        return pContext->ifc.pfnInterruptMask(
            pContext->ifc.context, intrId, bEnable
            );
        }
    else
        {
        IOCTL_TWL_INTRMASK_IN inParam;
        inParam.intrId = intrId;
        inParam.bEnable = bEnable;

        return DeviceIoControl(pContext->hDevice,
                               IOCTL_TWL_INTRMASK,
                               &inParam,
                               sizeof(inParam),
                               NULL,
                               0,
                               NULL,
                               NULL 
                               );
        }
}

BOOL 
TWLInterruptDisable(
    HANDLE hContext, 
    DWORD intrId
    )
{
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;

    if (pContext->ifc.pfnInterruptDisable != NULL)
        {
        return pContext->ifc.pfnInterruptDisable(
            pContext->ifc.context, intrId
            );
        }
    else
        {
        return DeviceIoControl(pContext->hDevice,
                               IOCTL_TWL_INTRDISABLE,
                               &intrId,
                               sizeof(DWORD),
                               NULL,
                               0,
                               NULL,
                               NULL 
                               );
        }
}


BOOL 
TWLWakeEnable(
    HANDLE hContext, 
    DWORD intrId,
    BOOL bEnable
    )
{
    IOCTL_TWL_WAKEENABLE_IN inParam;
    DEVICE_CONTEXT_TWL *pContext = (DEVICE_CONTEXT_TWL*)hContext;

    if (pContext->ifc.pfnEnableWakeup != NULL)
        {
        return pContext->ifc.pfnEnableWakeup(
            pContext->ifc.context, intrId, bEnable
            );
        }
    else
        {
        inParam.intrId = intrId;
        inParam.bEnable = bEnable;
        return DeviceIoControl(pContext->hDevice,
                               IOCTL_TWL_WAKEENABLE,
                               &inParam,
                               sizeof(inParam),
                               NULL,
                               0,
                               NULL,
                               NULL 
                               );
        }
}
