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
//  File:  oal_i2c.h
//
#ifndef __OAL_I2C_H
#define __OAL_I2C_H

#include "omap_types.h"
#include "omap_i2c_regs.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// i2c scale table, used to look-up settings of baudIndex
//
typedef struct {
    UINT16  psc;
    UINT16  scll;
    UINT16  sclh;
} I2CScaleTable_t;

//------------------------------------------------------------------------------
// i2c return value for i2c transactions
//
typedef enum {
    kI2CSuccess,
    kI2CFail,
    kI2CRetry
} I2CResult_e;

//-----------------------------------------------------------------------------
// i2c context
//
typedef struct {
    DWORD                   idI2C;  
    OMAP_DEVICE             device;
    DWORD                   baudIndex;
    DWORD                   timeOut;
    DWORD                   slaveAddress;
    DWORD                   subAddressMode;
} I2CContext_t;

//------------------------------------------------------------------------------
// structures used for i2c transactions
//
typedef enum 
{
    kI2C_Read,
    kI2C_Write
}
I2C_OPERATION_TYPE_e;

typedef struct 
{
    UINT                    size;
    UCHAR                  *pBuffer;
}
I2C_BUFFER_INFO_t;

typedef struct __I2C_PACKET_INFO_t
{
    UINT                    count;          // number of elements in array
    I2C_OPERATION_TYPE_e    opType;         // operation type (read/write)
    UINT                    result;         // number of bytes written/recieved
    I2C_BUFFER_INFO_t      *rgBuffers;      // reference to buffers    
}
I2C_PACKET_INFO_t;

typedef struct 
{
    UINT                    count;
    UINT                    con_mask;
    I2C_PACKET_INFO_t      *rgPackets;
}
I2C_TRANSACTION_INFO_t;

//-----------------------------------------------------------------------------
// i2c device
//
typedef struct {
    OMAP_DEVICE             device;
    DWORD                   ownAddress;
    DWORD                   defaultBaudIndex;
    DWORD                   maxRetries;
    DWORD                   rxFifoThreshold;
    DWORD                   txFifoThreshold;
    OMAP_I2C_REGS          *pI2CRegs;
    DWORD                   currentBaudIndex;    
    DWORD                   fifoSize;    
    CRITICAL_SECTION        cs;
} I2CDevice_t;



BOOL
OALI2CInit(
    UINT            devId 
    );

BOOL
OALI2CPostInit();

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

