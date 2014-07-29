//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
// 
// Module Name:  
//     Sdbusdef.h
// Abstract:  
//     Definition for the sd bus.
//
// 
// 
// Notes: 
// 
//


#pragma once

#define SDBUS_DEVICE_HANDLE_FLAG 0xf
typedef struct __SDBUS_DEVICE_HANDLE_BIT {
    DWORD sdBusIndex:4;
    DWORD sdSlotIndex:4;
    DWORD sdFunctionIndex:3;
    DWORD sdF:5; //set to f;
    DWORD sdRandamNumber:16;
} SDBUS_DEVICE_HANDLE_BIT;
#define SDBUS_MAX_REFERENCE_NUMBER 0x10000
#define SDBUS_DEVICE_REFERENCE_NUMBER_MASK (SDBUS_MAX_REFERENCE_NUMBER-1)

typedef union __SDBUS_DEVICE_HANDLE {
    SDBUS_DEVICE_HANDLE_BIT bit;
    HANDLE                  hValue;
} SDBUS_DEVICE_HANDLE,*PSDBUS_DEVICE_HANDLE;


#define SDBUS_REQUEST_HANDLE_FLAG 0xf
typedef struct __SDBUS_REQUEST_HANDLE_BIT {
    DWORD sdBusIndex:4;
    DWORD sdSlotIndex:4;
    DWORD sdFunctionIndex:3;
    DWORD sd1f:5; // set to 1f;
    DWORD sdRequestIndex:8;
    DWORD sdRandamNumber:8;
} SDBUS_REQUEST_HANDLE_BIT;
#define SDBUS_MAX_REQUEST_INDEX 0x100
#define SDBUS_REQUEST_INDEX_MASK (SDBUS_MAX_REQUEST_INDEX-1)

typedef union __SDBUS_REQUEST_HANDLE {
    SDBUS_REQUEST_HANDLE_BIT    bit;
    HANDLE                      hValue;
} SDBUS_REQUEST_HANDLE, *PSDBUS_REQUEST_HANDLE;



// SD Infomation.
// SDIO common information (shared by all I/O functions)
typedef struct _SDIO_COMMON_INFORMATION {
    UCHAR                   CCCRRev;              // CCCE/SDIO Revision (common for all functions)   
    UCHAR                   SDSpec;               // SD Spec version register
    UCHAR                   CardCapability;       // card capability (common for all functions)
    DWORD                   CommonCISPointer;     // common CIS pointer for this function
    USHORT                  ManufacturerID;       // 16 bit manufacturer ID  (common) 
    USHORT                  CardID;               // 16 bit cardID     (common)
    PWCHAR                  pProductInformation;  // storage for the product information string (common)
    UCHAR                   CCCRShadowIntEnable;  // shadowed interrupt enable register (Parent Only)
    UCHAR                   CCCRShadowIOEnable;   // shadowed I/O enable register (Parent Only)
    BOOL                    fCardSupportsPowerControl;  // Does the card support Power Control
    BOOL                    fPowerControlEnabled;       // Is Power Control Enabled for the Card

}SDIO_COMMON_INFORMATION, *PSDIO_COMMON_INFORMATION;

// SDIO information for each I/O function
typedef struct _SDIO_INFORMATION {
    UCHAR                    Function;             // function number
    UCHAR                    DeviceCode;           // device interface code for this function
    DWORD                    CISPointer;           // CIS pointer for this function
    DWORD                    CSAPointer;           // CSA pointer for this function
    PWCHAR                   pFunctionInformation; // storage for the function information string
    PSD_INTERRUPT_CALLBACK   pInterruptCallBack;   // for SDIO devices the interrupt callback
    PSDIO_COMMON_INFORMATION pCommonInformation;   // common information (parent only)
    BOOL                     fWUS;                 // Wake up supported?
    ULONG                    SoftBlockCount;       // number of blocks
    ULONG                    SoftBlockSize;        // size of each block
    ULONG                    SoftBlockLengthInBytes; // Size of the Soft-Blocks
    PUCHAR                   pSoftBlockBuffer;     // Data buffer starting address
    PUCHAR                   pSoftBlockEndOfBuffer;// Data buffer ending address
    PUCHAR                   pSoftBlockData;       // Current segment data buffer address
    DWORD                    SoftBlockArgument;    // Command argument for the request.
    UCHAR                    SoftBlockCommand;     // Command for the request.
    UCHAR                    Flags;                // Soft-Block control flags.
    SD_FUNCTION_POWER_DRAW   PowerDrawData;        // current draw of function.

#define SFTBLK_USE_FOR_CMD18        1       //  Use Soft-Block for read operations.
#define SFTBLK_USE_FOR_CMD25        2       //  Use Soft-Block for write operations.
#define SFTBLK_USE_FOR_CMD53_READ   4       //  Use Soft-Block for read operations.
#define SFTBLK_USE_FOR_CMD53_WRITE  8       //  Use Soft-Block for write operations.
#define SFTBLK_USE_ALWAYS           0x10    //  Use Soft-Block always.
#define FSTPTH_DISABLE              0x20    //  Disable the use of Fast-Path.

}SDIO_INFORMATION, *PSDIO_INFORMATION;

// SD/SDIO/MMC specific information
typedef struct _SDMMC_INFORMATION {
    ULONG   DataAccessWriteClocks;    // total clocks required for the write to finish  
    ULONG   DataAccessReadClocks;     // total access delay in clocks for the first byte transferred in a read
    BOOL    CardIsLocked;             // card is locked
    ULONGLONG   ullDeviceSize;          // ULONGLONG device memory size that provided from CSD to extented function.
} SDMMC_INFORMATION, *PSDMMC_INFORMATION;

// SD Card information structure
typedef struct _SDCARD_INFORMATION {
    SDIO_INFORMATION    SDIOInformation;    // SDIO information
    SDMMC_INFORMATION   SDMMCInformation;   // SD/SDIO/MMC information
} SDCARD_INFORMATION, *PSDCARD_INFORMATION;

