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
//  File: twl.h
//
#ifndef __TWL_H
#define __TWL_H

#include "ceddkex.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define:  TWL_DEVICE_NAME
//
#define TWL_DEVICE_NAME         L"TWL1:"

//------------------------------------------------------------------------------
//
//  Functions: TWLxxx
//

BOOL
TWLReadRegs(
    HANDLE hContext, 
    DWORD address,
    VOID *pBuffer,
    DWORD size
    );
    
BOOL 
TWLWriteRegs(
    HANDLE hContext, 
    DWORD address,
    const VOID *pBuffer,
    DWORD size
    );

static __inline BOOL
TWLWriteByteReg(
    void* hTWL,
    DWORD address,
    BYTE data
    )
{
    return TWLWriteRegs(hTWL,address,&data,sizeof(data));
}

static __inline BOOL
TWLReadByteReg(
    void* hTWL,
    DWORD address,
    BYTE* data
    )
{
    return TWLReadRegs(hTWL,address,data,sizeof(*data));
}


BOOL 
TWLWakeEnable(
    HANDLE hContext, 
    DWORD intrId,
    BOOL bEnable
    );

BOOL  TWLSetValue(
               HANDLE hContext,
               DWORD address,
               UCHAR value,
               UCHAR mask
               );
BOOL 
TWLInterruptInitialize(
    HANDLE hContext, 
    DWORD intrId,
    HANDLE hEvent
    );

BOOL 
TWLInterruptMask(
    HANDLE hContext, 
    DWORD intrId,
    BOOL  bEnable
    );

BOOL 
TWLInterruptDisable(
    HANDLE hContext, 
    DWORD intrId
    );

//------------------------------------------------------------------------------
//
//  GUID:  DEVICE_IFC_TWL_GUID
//
// {DEF0A04B-B967-43db-959E-D9FC6225CDEB}
DEFINE_GUID(
    DEVICE_IFC_TWL_GUID, 0xdef0a04b, 0xb967, 0x43db, 
    0x95, 0x9e, 0xd9, 0xfc, 0x62, 0x25, 0xcd, 0xeb
    );


//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_TWL
//
//  This structure is used to obtain TWL interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    DWORD context;
    BOOL (*pfnReadRegs)(DWORD ctx, DWORD address, VOID *pBuffer, DWORD size);
    BOOL (*pfnWriteRegs)(DWORD ctx, DWORD address, const void *pBuffer, DWORD size);
    BOOL (*pfnInterruptInitialize)(DWORD ctx, DWORD intrId, HANDLE hEvent);
    BOOL (*pfnInterruptDisable)(DWORD ctx, DWORD intrId);
    BOOL (*pfnInterruptMask)(DWORD ctx, DWORD intrId, BOOL bEnable);
    BOOL (*pfnEnableWakeup)(DWORD ctx, DWORD intrId, BOOL bEnable);
} DEVICE_IFC_TWL;

//------------------------------------------------------------------------------

#define IOCTL_TWL_READREGS          \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD address;
    DWORD size;
} IOCTL_TWL_READREGS_IN;

#define IOCTL_TWL_WRITEREGS         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD address;
    const void* pBuffer;
    DWORD size;
} IOCTL_TWL_WRITEREGS_IN;

#define IOCTL_TWL_SETINTREVENT      \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0302, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT   procId;
    DWORD  intrId;
    HANDLE hEvent;
} IOCTL_TWL_INTRINIT_IN;

typedef struct {
    UINT   procId;
    DWORD  intrId;
    BOOL   bEnable;
} IOCTL_TWL_INTRMASK_IN;

#define IOCTL_TWL_INTRINIT         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0303, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TWL_INTRDISABLE       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TWL_INTRMASK       \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0305, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TWL_WAKEENABLE        \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0306, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct {
    DWORD intrId;
    BOOL bEnable;
} IOCTL_TWL_WAKEENABLE_IN;



#ifdef __cplusplus
}
#endif

#endif
