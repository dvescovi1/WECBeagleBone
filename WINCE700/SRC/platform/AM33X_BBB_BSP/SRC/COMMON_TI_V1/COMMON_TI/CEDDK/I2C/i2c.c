// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  i2c.cpp
//
//  This file contains DDK library implementation for platform specific
//  i2c operations.
//  
#include "omap.h"
#include "sdk_i2c.h"
#include "oalex.h"
#include "soc_cfg.h"
/*
#include <windows.h>
#include <types.h>
#include <oal.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <omap35xx.h>
#include <i2c.h>
#include <oal_i2c.h>
*/

//-----------------------------------------------------------------------------
static OAL_IFC_I2C      _i2cFnTable;
static BOOL             _bI2CFnTableInit = FALSE;


//-----------------------------------------------------------------------------
static
BOOL
I2CInitialize()
{
    if (_bI2CFnTableInit == FALSE)
    {
        // get clock ref counter table from kernel
        if (KernelIoControl(IOCTL_HAL_I2CCOPYFNTABLE, (void*)&_i2cFnTable,
            sizeof(OAL_IFC_I2C), NULL, 0, NULL))
        {
            _bI2CFnTableInit = TRUE;
        }
        else
        {
            memset(&_i2cFnTable,0,sizeof(_i2cFnTable));
        }
    }
    return _bI2CFnTableInit;
}



//-----------------------------------------------------------------------------
UINT 
I2CGetDeviceIdFromMembase(
                          UINT memBase
                          )
{
    return GetDeviceByAddress(memBase);
}


//-----------------------------------------------------------------------------
void* 
I2COpen(
        UINT devId
        )
{
    if (I2CInitialize() == FALSE) return NULL;

    return _i2cFnTable.fnI2COpen(devId);
}


//-----------------------------------------------------------------------------
BOOL
I2CSetSlaveAddress(
                   void           *hContext, 
                   UINT16          slaveAddress
                   )
{
    return _i2cFnTable.fnI2CSetSlaveAddress(hContext,slaveAddress);
}

//-----------------------------------------------------------------------------
void 
I2CSetSubAddressMode(
                     void*      hContext,
                     DWORD       subAddressMode
                     )
{
    _i2cFnTable.fnI2CSetSubAddressMode(hContext,subAddressMode);    
}

//-----------------------------------------------------------------------------
void 
I2CSetBaudIndex(
                void*      hContext,
                DWORD       baudIndex
                )
{
    _i2cFnTable.fnI2CSetBaudIndex(hContext,baudIndex);    
}

//-----------------------------------------------------------------------------
void 
I2CSetTimeout(
              void*      hContext,
              DWORD       timeOut
              )
{
    _i2cFnTable.fnI2CSetTimeout(hContext,timeOut); 
}


//-----------------------------------------------------------------------------
void
I2CClose(
         void* hContext
         )
{
    _i2cFnTable.fnI2CClose(hContext);
}

//-----------------------------------------------------------------------------
UINT
I2CRead(
        void           *hCtx,
        UINT32          subAddr,
        VOID           *pBuffer,
        UINT32          size
        )
{
    return _i2cFnTable.fnI2CRead(hCtx, subAddr, pBuffer, size);
}

//-----------------------------------------------------------------------------
UINT
I2CWrite(
         void            *hCtx,
         UINT32          subAddr,
         const VOID      *pBuffer,
         UINT32          size
         )
{
    return _i2cFnTable.fnI2CWrite(hCtx, subAddr, (VOID*)pBuffer, size);
}

void I2CLock(void           *hCtx)
{
    _i2cFnTable.fnI2CLock(hCtx);
}           

void I2CUnlock(void           *hCtx)
{
    _i2cFnTable.fnI2CUnlock(hCtx);
} 


void I2CSetManualDriveMode(VOID *hCtx,BOOL fOn)
{
    _i2cFnTable.fnSetManualDriveMode(hCtx,fOn);
}
void I2CDriveSCL(VOID *hCtx,BOOL fHigh)
{
    _i2cFnTable.fnDriveSCL(hCtx,fHigh);
}
void I2CDriveSDA(VOID *hCtx,BOOL fHigh)
{
    _i2cFnTable.fnDriveSDA(hCtx,fHigh);
}

//-----------------------------------------------------------------------------
