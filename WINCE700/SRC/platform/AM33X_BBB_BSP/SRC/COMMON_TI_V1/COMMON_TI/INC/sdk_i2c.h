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
//  File:  sdk_i2c.h
//
#ifndef __SDK_I2C_H
#define __SDK_I2C_H

#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------------
// i2c baud index
//
#define SLOWSPEED_MODE          0       // corresponds to 100 khz
#define FULLSPEED_MODE          1       // corresponds to 400 khz
#define HIGHSPEED_MODE_1P6      2       // corresponds to 1.6 mhz
#define HIGHSPEED_MODE_2P4      3       // corresponds to 2.4 mhz
#define HIGHSPEED_MODE_3P2      4       // corresponds to 3.2 mhz
#define MAX_SCALE_ENTRY         5       // number of entries in s_ScaleTable

//-----------------------------------------------------------------------------
// defines subaddress mode for a i2c client
//
#define I2C_SUBADDRESS_MODE_0   0
#define I2C_SUBADDRESS_MODE_8   1
#define I2C_SUBADDRESS_MODE_16  2
#define I2C_SUBADDRESS_MODE_24  3
#define I2C_SUBADDRESS_MODE_32  4

//------------------------------------------------------------------------------
//
//  Define:  IOCTL_DDK_GET_OAL_I2C_IFC
//
//  This IOCTL code is used to obtain the function pointers to the OAL
//  i2c functions.
//
#define IOCTL_DDK_GET_OAL_I2C_IFC \
    CTL_CODE(FILE_DEVICE_I2C, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    void      (*fnI2CLock)(void*);
    void      (*fnI2CUnlock)(void*);    
    HANDLE    (*fnI2COpen)(UINT devId);
    void      (*fnI2CClose)(void *pData);
    UINT      (*fnI2CWrite)(void *hCtx, UINT32 subAddr, VOID *pBuffer, UINT32 size);
    UINT      (*fnI2CRead)(void *hCtx, UINT32 subAddr, VOID *pBuffer, UINT32 size);
    BOOL      (*fnI2CSetSlaveAddress) (void *hCtx,UINT16 slaveAddress);
    void      (*fnI2CSetSubAddressMode) (void *hCtx, DWORD subAddressMode);
    void      (*fnI2CSetBaudIndex) (void *hCtx, DWORD baudIndex);
    void      (*fnI2CSetTimeout) (void *hCtx, DWORD timeOut);
    void      (*fnSetManualDriveMode) (void *hCtx, BOOL fOn);
    void      (*fnDriveSCL) (void *hCtx, BOOL fHigh);
    void      (*fnDriveSDA) (void *hCtx, BOOL fHigh);
} OAL_IFC_I2C;



//------------------------------------------------------------------------------

void 
I2CLock(
           void           *hCtx
           );
           
void 
I2CUnlock(
           void           *hCtx
           );


void*
I2COpen(
    UINT            devId    
    );

void
I2CClose(
    void           *hCtx
    );

UINT
I2CWrite(
    void            *hCtx,
    UINT32          subAddr,
    const VOID      *pBuffer,
    UINT32          size
    );

UINT
I2CRead(
    void           *hCtx,
    UINT32          subAddr,
    VOID           *pBuffer,
    UINT32          size
    );

BOOL
I2CSetSlaveAddress(
    void           *hCtx, 
    UINT16          slaveAddress
    );

//-----------------------------------------------------------------------------
void 
I2CSetSubAddressMode(
    void*       hContext,
    DWORD       subAddressMode
    );

//-----------------------------------------------------------------------------
void 
I2CSetBaudIndex(
    void*      hContext,
    DWORD       baudIndex
    );

//-----------------------------------------------------------------------------
void 
I2CSetTimeout(
    void*      hContext,
    DWORD       timeOut
    );

//------------------------------------------------------------------------------
void 
I2CSetManualDriveMode(
    void*      hContext,
    BOOL fOn
    );
//------------------------------------------------------------------------------
void
I2CDriveSCL(
    void*      hContext,
    BOOL fHigh
    );
//------------------------------------------------------------------------------
void
I2CDriveSDA(
    void*      hContext,
    BOOL fHigh
    );
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

