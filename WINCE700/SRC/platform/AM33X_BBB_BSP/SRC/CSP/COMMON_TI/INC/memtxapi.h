// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  defines platform independent data structures and api's.
//
#ifndef __MEMTXAPI_H
#define __MEMTXAPI_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// defines
//
#define MAX_HW_CODEC_CHANNELS              (4)

//------------------------------------------------------------------------------
// defines IOCTL for direct memory access to other proxy drivers
//

#define FILE_DEVICE_EXTERNAL_DRVR_DATATRANSFER      0x500
#define IOCTL_EXTERNAL_DRVR_REGISTER_TRANSFERCALLBACKS         \
    CTL_CODE(FILE_DEVICE_EXTERNAL_DRVR_DATATRANSFER,  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
#define IOCTL_EXTERNAL_DRVR_UNREGISTER_TRANSFERCALLBACKS         \
    CTL_CODE(FILE_DEVICE_EXTERNAL_DRVR_DATATRANSFER,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)

// DASF audio render path IOCTL's
//
#define IOCTL_WAV_AUDIORENDER_PORT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN,   102, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
#define IOCTL_WAV_AUDIORENDER_QUERYPORT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN,   103, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DASF_POWER_I2SCLK       \
    CTL_CODE(FILE_DEVICE_UNKNOWN,   104, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {

   BOOL bActivePort;

} IOCTL_WAV_AUDIORENDER_QUERYPORT_OUT;

typedef struct {
    
    BOOL bPortRequest;
    
} IOCTL_WAV_AUDIORENDER_QUERYPORT_IN;

//------------------------------------------------------------------------------
// direct memory access audio settings

typedef enum {
    kExternalDrvrDx_Stop = 0,
    kExternalDrvrDx_Start,
    kExternalDrvrDx_ImmediateStop,
    kExternalDrvrDx_Reconfig
} ExternalDrvrCommand;

typedef struct {
    DWORD           portProfile;
    DWORD           numOfChannels;
    DWORD           requestedChannels[MAX_HW_CODEC_CHANNELS];
} PortConfigInfo_t;

typedef int (*DataTransfer_Command)(ExternalDrvrCommand cmd, 
            void* pData, PortConfigInfo_t* pPortConfigInfo
            );

typedef int (*DataTransfer_PopulateBuffer)(void* pStart, 
            void* pData, unsigned int dwLength
            );

typedef int (*MutexLock)(BOOL bLock, DWORD dwTime, void* pData);

typedef struct __EXTERNAL_DRVR_DATA_TRANSFER_IN {
    void                           *pInData;
    DataTransfer_Command           pfnInTxCommand;
    DataTransfer_Command           pfnInRxCommand;
    DataTransfer_PopulateBuffer    pfnInTxPopulateBuffer;
    DataTransfer_PopulateBuffer    pfnInRxPopulateBuffer;
    MutexLock                      pfnMutexLock;
} EXTERNAL_DRVR_DATA_TRANSFER_IN;
    
typedef struct __EXTERNAL_DRVR_DATA_TRANSFER_OUT {
    void                           *pOutData;
    DataTransfer_Command           pfnOutTxCommand;
    DataTransfer_Command           pfnOutRxCommand;
} EXTERNAL_DRVR_DATA_TRANSFER_OUT;

#ifdef __cplusplus
}
#endif

#endif