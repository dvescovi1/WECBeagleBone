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


typedef struct {
    HANDLE hI2C;
    LONG refCount;
    BOOL csInitialized;
    UINT16 slaveAddress;
    CRITICAL_SECTION cs;    // keep this as last element
} Device_t;

//------------------------------------------------------------------------------
//
//  Functions: TWLxxx
//
HANDLE 
TWLOpen(
    )
{
    static  Device_t Device = {NULL,0,FALSE,0};     

    // Open i2c bus
    if (Device.refCount == 0)
    {
        Device.hI2C = I2COpen(BSPGetTritonBusID());
        if (Device.hI2C == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: TWLOpen: "
                L"Failed open I2C bus driver\r\n"
            ));
            return NULL;
        }
        
        //Device.slaveAddress = BSPGetTritonSlaveAddress();
        I2CSetSlaveAddress(Device.hI2C, BSPGetTritonSlaveAddress());  
    }
    Device.refCount++;
    return &Device;
}

VOID
TWLClose(
    HANDLE hContext
    )
{
    Device_t * pDevice = (Device_t *) hContext;
    pDevice->refCount--;
    if (pDevice->refCount == 0)
    {
        if (pDevice->hI2C) 
        {
            I2CClose(pDevice->hI2C);
            pDevice->hI2C = 0;
        }
    }
    
}

//------------------------------------------------------------------------------
//
//  Function:  ReadRegs
//
//  Internal routine to read triton registers.
//
BOOL
TWLReadRegs(
    HANDLE hContext,
    DWORD address,
    VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;   
    
    Device_t * pDevice = (Device_t *) hContext;

    if (pDevice->csInitialized) EnterCriticalSection(&pDevice->cs);
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
    if (pDevice->csInitialized) LeaveCriticalSection(&pDevice->cs);

    //if (rc)
    //{
    //    RETAILMSG(1,(TEXT("Triton ReadRegs @0x%x = 0x%x\r\n"),address,*(UCHAR*)pBuffer));
    //}
    //else
    //{
    //    RETAILMSG(1,(TEXT("Triton ReadRegs @0x%x failed\r\n"),address));
    //}
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  WriteRegs
//
//  Internal routine to write triton registers.
//
BOOL
TWLWriteRegs(
    HANDLE hContext,
    DWORD address,
    const VOID *pBuffer,
    DWORD size
    )
{
    BOOL rc = FALSE;
    
    Device_t * pDevice = (Device_t *) hContext;
    
//    RETAILMSG(1,(TEXT("Triton WriteRegs @0x%x = 0x%x\r\n"),address,*(UCHAR*)pBuffer));   
 
    if (pDevice->csInitialized) EnterCriticalSection(&pDevice->cs);

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
    if (pDevice->csInitialized) LeaveCriticalSection(&pDevice->cs);
    
 /*   
    if (!rc)
    {
        RETAILMSG(1,(TEXT("Triton WriteRegs @0x%x failed\r\n"),address));
    }
 */   
    return rc;
}


BOOL  TWLSetValue(
               HANDLE hContext,
               DWORD address,
               UCHAR value,
               UCHAR mask
               )
{
    UCHAR regval;
    BOOL rc = FALSE;
    
    Device_t * pDevice = (Device_t *) hContext;

    if (pDevice->csInitialized) EnterCriticalSection(&pDevice->cs);

    // set slave address if necessary
    if (pDevice->slaveAddress != HIWORD(address))
        {
        I2CSetSlaveAddress(pDevice->hI2C, BSPGetTritonSlaveAddress() | HIWORD(address));
        pDevice->slaveAddress = HIWORD(address);
        }

    if (I2CRead(pDevice->hI2C, (UCHAR)address, &regval, sizeof(regval)) != sizeof(regval))
        {
        goto cleanUp;
        }   

        regval = (regval & ~mask) | value;

    if (I2CWrite(pDevice->hI2C, (UCHAR)address, &regval, sizeof(regval)) != sizeof(regval))
        {
        goto cleanUp;
        }   

    // We succceded
    rc = TRUE;

cleanUp:  
    if (pDevice->csInitialized) LeaveCriticalSection(&pDevice->cs);
  
    return rc;    
}
