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
#include "omap.h"
#include <windows.h>
#include <driver.hxx>

//------------------------------------------------------------------------------

extern "C"
DWORD
Init(
   LPCWSTR activeRegName,
   VOID *pBusContext
   )
{
    Driver_t *pDevice;

    // Create bus driver
    pDevice = Driver_t::CreateDevice();
    if ((pDevice != NULL) && !pDevice->Init(activeRegName, pBusContext))
        {
        pDevice->Release();
        pDevice = NULL;
        }
    return reinterpret_cast<DWORD>(pDevice);
}

//------------------------------------------------------------------------------

extern "C"
BOOL
Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Driver_t *pDevice = reinterpret_cast<Driver_t*>(context);
    rc = pDevice->Deinit();
    pDevice->Release();
    return rc;
}

//------------------------------------------------------------------------------

extern "C"
VOID
PowerUp(
    DWORD context
    )
{
    Driver_t *pDevice = reinterpret_cast<Driver_t*>(context);
    pDevice->PowerUp();
}

//------------------------------------------------------------------------------

extern "C"
VOID
PowerDown(
    DWORD context
    )
{
    Driver_t *pDevice = reinterpret_cast<Driver_t*>(context);
    pDevice->PowerDown();
}

//------------------------------------------------------------------------------

extern "C"
DWORD
Open(
    DWORD context,
    DWORD accessCode,
    DWORD shareMode
    )
{
    Driver_t* pDevice = reinterpret_cast<Driver_t*>(context);
    DriverContext_t* pContext = pDevice->Open(accessCode, shareMode);
    return reinterpret_cast<DWORD>(pContext);
}

//------------------------------------------------------------------------------

extern "C"
BOOL
Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    DriverContext_t* pContext = reinterpret_cast<DriverContext_t*>(context);
    rc = pContext->Close();
    pContext->Release();
    return rc;
}

//------------------------------------------------------------------------------

extern "C"
BOOL
IOControl(
    DWORD context,
    DWORD code,
    UCHAR *pInBuffer,
    DWORD inSize,
    UCHAR *pOutBuffer,
    DWORD outSize,
    DWORD *pOutSize
    )
{
    DriverContext_t* pContext = reinterpret_cast<DriverContext_t*>(context);
    return pContext->IOControl(
        code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        );
}

//------------------------------------------------------------------------------

