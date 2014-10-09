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
//  File: tps65217.cpp
//
#include "bsp.h"
#include "ceddkex.h"
#include "sdk_i2c.h"

#include <initguid.h>
#include "twl.h"
#include "triton.h"

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

// disable zero size array warning
#pragma warning(disable:4200)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)
#define ZONE_RTC            DEBUGZONE(15)


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"Triton (TWL)", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"RTC"
    },
    0x8003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define TWL_DEVICE_COOKIE       'twlD'
#define TWL_INSTANCE_COOKIE     'twlI'

//------------------------------------------------------------------------------
//  Local enumerations

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD cookie;
    HANDLE hI2C;
    CRITICAL_SECTION cs;
    DWORD slaveAddress;
    CEDEVICE_POWER_STATE powerState;
} Device_t;

typedef struct {
    DWORD cookie;
    DWORD address;
    Device_t *pDevice;
} Instance_t;

//------------------------------------------------------------------------------
//  prototype

BOOL TWL_Deinit(DWORD context);
static BOOL TWL_ReadRegs(DWORD c, DWORD addr, void *pBuffer, DWORD size);
static BOOL TWL_WriteRegs(DWORD c, DWORD addr, const void *pBuffer, DWORD size);


//------------------------------------------------------------------------------
//
//  Function:  ReadRegs
//
//  Internal routine to read triton registers.
//
BOOL
ReadRegs(
    Device_t *pDevice,
    DWORD address,
    VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;

    EnterCriticalSection(&pDevice->cs);
    // set slave address if necessary
    if (pDevice->slaveAddress != HIWORD(address))
        {
        I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress() | HIWORD(address));        
        pDevice->slaveAddress = HIWORD(address);
        }

    if (I2CRead(pDevice->hI2C, (UCHAR)address, pBuffer, size) != size)
        {
        goto cleanUp;
        }
    
    // We succceded
    rc = TRUE;

cleanUp:    
    LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  WriteRegs
//
//  Internal routine to write triton registers.
//
BOOL
WriteRegs(
    Device_t *pDevice,
    DWORD address,
    const VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;

    EnterCriticalSection(&pDevice->cs);

    // set slave address if necessary
    if (pDevice->slaveAddress != HIWORD(address))
        {
        I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress() | HIWORD(address));        
        pDevice->slaveAddress = HIWORD(address);
        }

    if (I2CWrite(pDevice->hI2C, (UCHAR)address, pBuffer, size) != size)
        {
        goto cleanUp;
        }   

    // We succceded
    rc = TRUE;

cleanUp:  
    LeaveCriticalSection(&pDevice->cs);
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  TWL_Init
//
//  Called by device manager to initialize device.
//
DWORD
TWL_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = NULL;

    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+TWL_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed allocate TWL controller structure\r\n"
            ));
        goto cleanUp;
        }

    // clear memory
    memset(pDevice, 0, sizeof(Device_t));

    // Set cookie and initial power state
    pDevice->cookie = TWL_DEVICE_COOKIE;
    pDevice->powerState = D0;

    // Initalize critical section
    InitializeCriticalSection(&pDevice->cs);

    // Open i2c bus
    pDevice->hI2C = I2COpen(BSPGetTritonBusID());
    if (pDevice->hI2C == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Init: "
            L"Failed open I2C bus driver\r\n"
            ));
        goto cleanUp;
        }

	I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress());

    // Return non-null value
    rc = (DWORD)pDevice;
    
cleanUp:
    if (rc == 0) TWL_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
TWL_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;


    DEBUGMSG(ZONE_FUNCTION, (L"+TWL_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Close I2C bus
    if (pDevice->hI2C != NULL) I2CClose(pDevice->hI2C);

    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
TWL_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    Instance_t *pInstance = (Instance_t*)LocalAlloc(LPTR, sizeof(Instance_t));

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    if (pInstance == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_Open: "
            L"Failed allocate TWL instance structure\r\n"
            ));
        return NULL;
    }

    pInstance->cookie = TWL_INSTANCE_COOKIE;
    pInstance->pDevice = (Device_t*)context;
    pInstance->address = 0;

    return (DWORD)pInstance;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function closes the device context.
//
BOOL
TWL_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance != NULL && pInstance->cookie == TWL_INSTANCE_COOKIE)
        {
        LocalFree(pInstance);
        rc = TRUE;
        }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Read
//
//  This function closes the device context.
//
DWORD
TWL_Read(
    DWORD   context,
    LPVOID  pBuffer,
    DWORD   count
    )
{
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        count = 0;
        goto cleanUp;
        }

    count = TWL_ReadRegs(context, pInstance->address, pBuffer, count);

cleanUp:
    return count;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Write
//
//  This function closes the device context.
//
DWORD
TWL_Write(
    DWORD   context,
    LPCVOID pBuffer,
    DWORD   count
    )
{
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        count = 0;
        goto cleanUp;
        }

    if (TWL_WriteRegs(context, pInstance->address, pBuffer, count) == FALSE)
        {
        count = 0;
        }

cleanUp:
    return count;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Seek
//
//  This function closes the device context.
//
BOOL
TWL_Seek(
    DWORD context,
    long  amount,
    WORD  type
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;

    UNREFERENCED_PARAMETER(type);

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        goto cleanUp;
        }

    pInstance->address = amount;
    
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_PowerUp
//
//  This function wakes-up driver from power down.
//
void
TWL_PowerUp(
    DWORD context
    )
{
    Device_t *pDevice = (Device_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        return;
        }
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function prepares driver to power down.
//
void
TWL_PowerDown(
    DWORD context
    )
{
    Device_t *pDevice = (Device_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: TWL_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        return;
        }
}

//------------------------------------------------------------------------------

static
BOOL
TWL_ReadRegs(
    DWORD context, 
    DWORD address,
    void *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_ReadRegs: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (size > 0xFF) goto cleanUp;

    rc = ReadRegs(pDevice, address, pBuffer, size);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static
BOOL
TWL_WriteRegs(
    DWORD context, 
    DWORD address,
    const void *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = ((Instance_t*)context)->pDevice;
/*
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_WriteRegs: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
*/
    if (size > 0xFF) goto cleanUp;
    if (address & 0xFF000000) goto cleanUp;

    rc = WriteRegs(pDevice, address, pBuffer, size);
    
cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  TWL_IOControl
//
//  This function sends a command to a device.
//
BOOL
TWL_IOControl(
    DWORD context, 
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"+TWL_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));
    
    BOOL rc = FALSE;
    const void* pBuffer;
    Device_t *pDevice;
    DEVICE_IFC_TWL ifc;
    DWORD address, size;
    Instance_t *pInstance = (Instance_t*)context;

    if (pInstance == NULL || pInstance->cookie != TWL_INSTANCE_COOKIE)
        {
        goto cleanUp;
        }

    pDevice = pInstance->pDevice;


    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TWL_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_DDK_GET_DRIVER_IFC:
            // We can give interface only to our peer in device process
            if (GetCurrentProcessId() != (DWORD)GetCallerProcess())
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: TWL_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                    ));
                SetLastError(ERROR_ACCESS_DENIED);
                break;
                }
            // Check input parameters
            if ((pInBuffer == NULL) || (inSize < sizeof(GUID)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_TWL_GUID))
                {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_TWL);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_TWL))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                ifc.context = context;
                ifc.pfnReadRegs = TWL_ReadRegs;
                ifc.pfnWriteRegs = TWL_WriteRegs;
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_TWL)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
            
        case IOCTL_TWL_READREGS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(IOCTL_TWL_READREGS_IN)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            address = ((IOCTL_TWL_READREGS_IN*)pInBuffer)->address;
            size = ((IOCTL_TWL_READREGS_IN*)pInBuffer)->size;
            if (pOutSize != NULL) *pOutSize = size;
            if ((pOutBuffer == NULL) || (outSize < size))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            rc = TWL_ReadRegs(context, address, pOutBuffer, size);

        case IOCTL_TWL_WRITEREGS:
            if ((pInBuffer == NULL) || 
                (inSize < sizeof(IOCTL_TWL_WRITEREGS_IN)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            address = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->address;
            pBuffer = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->pBuffer;
            size = ((IOCTL_TWL_WRITEREGS_IN*)pInBuffer)->size;
            /*
            if (inSize < (sizeof(IOCTL_TWL_WRITEREGS_IN) + size))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            */	
            rc = TWL_WriteRegs(context, address, pBuffer, size);
            break;

        case IOCTL_POWER_CAPABILITIES: 
            DEBUGMSG(ZONE_INFO, (L"TWL: Received IOCTL_POWER_CAPABILITIES\r\n"));
            if (pOutBuffer && outSize >= sizeof (POWER_CAPABILITIES) && 
                pOutSize) 
                {
                    __try 
                        {
                        PPOWER_CAPABILITIES PowerCaps;
                        PowerCaps = (PPOWER_CAPABILITIES)pOutBuffer;
         
                        // Only supports D0 (permanently on) and D4(off.         
                        memset(PowerCaps, 0, sizeof(*PowerCaps));
                        PowerCaps->DeviceDx = (DX_MASK(D0) | 
                                               DX_MASK(D2) | 
                                               DX_MASK(D3) | 
                                               DX_MASK(D4));
                        *pOutSize = sizeof(*PowerCaps);                        
                        rc = TRUE;
                        }
                    __except(EXCEPTION_EXECUTE_HANDLER) 
                        {
                        RETAILMSG(ZONE_ERROR, (L"exception in ioctl\r\n"));
                        }
                }
            break;
#if (_WINCEOSVER<700)
        // deprecated
        case IOCTL_POWER_QUERY: 
            rc = TRUE;
            DEBUGMSG(ZONE_INFO,(L"TWL: Received IOCTL_POWER_QUERY\r\n"));
            break;
#endif
        // requests a change from one device power state to another
        case IOCTL_POWER_SET: 
            DEBUGMSG(ZONE_INFO,(L"TWL: Received IOCTL_POWER_SET\r\n"));
            if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pOutBuffer;
                    switch (ReqDx)
                        {
                        case D0:
                            pDevice->powerState = D0;
                            break;
                            
                        case D1:
                        case D2:
                            pDevice->powerState = D2;
                            break;

                        case D3:
                            pDevice->powerState = D3;
                            break;

                        case D4:
                            pDevice->powerState = D4;
                            break;
                        }
                    *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
                    *pOutSize = sizeof(CEDEVICE_POWER_STATE);
                    DEBUGMSG(ZONE_INFO, (L"TWL: IOCTL_POWER_SET to D%u \r\n",
                        pDevice->powerState
                        ));

                    rc = TRUE;
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
                    }
            }
            break;

        // gets the current device power state
        case IOCTL_POWER_GET: 
            DEBUGMSG(ZONE_INFO, (L"TWL: Received IOCTL_POWER_GET\r\n"));
            if (pOutBuffer != NULL && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
 
                    rc = TRUE;

                    DEBUGMSG(ZONE_INFO, (L"TWL: "
                            L"IOCTL_POWER_GET to D%u \r\n",
                            pDevice->powerState
                            ));
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
                    }
                }     
            break;
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

#pragma warning(default:4200)

//------------------------------------------------------------------------------

