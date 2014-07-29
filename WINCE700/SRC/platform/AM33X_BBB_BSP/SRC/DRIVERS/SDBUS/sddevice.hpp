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
// Module Name:  
//     Sdbus.hpp
// Abstract:  
//     Definition for the sd bus.
//
// 
// 
// Notes: 
// 
//
#pragma once

#include <CRefCon.h>
#include "Sdbusdef.h"

#include "sdbusdef.h"
// the card registers
typedef struct _SDCARD_CARD_REGISTERS {
    union { 
        UCHAR   OCR[SD_OCR_REGISTER_SIZE];          // SD OCR
        UCHAR   IO_OCR[SD_IO_OCR_REGISTER_SIZE];    // IO OCR
    };
    UCHAR   CID[SD_CID_REGISTER_SIZE];          // CID
    UCHAR   CSD[SD_CSD_REGISTER_SIZE];          // CSD
    UCHAR   SCR[SD_SCR_REGISTER_SIZE];          // SCR
}SDCARD_CARD_REGISTERS, *PSDCARD_CARD_REGISTERS;

// default powerup wait while polling for NOT busy
#define DEFAULT_POWER_UP_TOTAL_WAIT_TIME  2000
// macro to get the function number from an R4 response 
#define SD_GET_NUMBER_OF_FUNCTIONS(pResponse) (((pResponse)->ResponseBuffer[4] >> 4) & 0x7)
// macro to get the memory present bit from an R4 response
#define SD_MEMORY_PRESENT_WITH_IO(pResponse) (((pResponse)->ResponseBuffer[4] >> 3) &  0x1)
// macro to get the I/O ready flag from an R4 response
#define SD_IS_IO_READY(pResponse)             ((pResponse)->ResponseBuffer[4] & 0x80)
// macro to get the Memory card ready flag from an R3 response
#define SD_IS_MEM_READY(pResponse)            ((pResponse)->ResponseBuffer[4] & 0x80)

// as per spec , 50 MS interval 
#define DEFAULT_POWER_UP_SD_POLL_INTERVAL 50
// MMC cards don't really specify this, so we'll just borrow the SD one.
#define DEFAULT_POWER_UP_MMC_POLL_INTERVAL DEFAULT_POWER_UP_SD_POLL_INTERVAL
// SDIO doesn't specify this either, but in case some one
// needs clocks during CMD5 polling (which is a spec violation)
// we provide a clock train 
#define DEFAULT_POWER_UP_SDIO_POLL_INTERVAL 2 // SDIO WiFi cards need this to be 2

#define POWER_UP_POLL_TIME_KEY          TEXT("PowerUpPollingTime")
#define POWER_UP_POLL_TIME_INTERVAL_KEY TEXT("PowerUpPollingInterval")

#define SDCARD_CLOCK_RATE_OVERRIDE     TEXT("SDClockRateOverride")
#define SDCARD_INTERFACE_MODE_OVERRIDE TEXT("SDInterfaceOverride")
#define SDCARD_INTERFACE_OVERRIDE_1BIT 0
#define SDCARD_INTERFACE_OVERRIDE_4BIT 1


// base registry key for the SDCARD driver
#define SDCARD_SDMEMORY_CLASS_REG_PATH  TEXT("\\Drivers\\SDCARD\\ClientDrivers\\Class\\SDMemory_Class")
#define SDCARD_MMC_CLASS_REG_PATH       TEXT("\\Drivers\\SDCARD\\ClientDrivers\\Class\\MMC_Class")
#define SDCARD_CUSTOM_DEVICE_REG_PATH   TEXT("\\Drivers\\SDCARD\\ClientDrivers\\Custom")
#define SDCARD_SDIO_CLASS_REG_PATH      TEXT("\\Drivers\\SDCARD\\ClientDrivers\\Class\\SDIO_Class")

// eMMC registry entry
#define SDCARD_eMMC_CLASS_REG_PATH       TEXT("\\Drivers\\SDCARD\\ClientDrivers\\Class\\eMMC_Class")

#define SDCARD_HIGH_CAPACITY_REG_PATH   TEXT("\\High_Capacity")

// if MMC device is larger than 26MB ((26 * 1000 * 1000 * (2 * 512))/ 512), assume it's eMMC module
#define EMMC_MODULE_SIZE_BOUNDARY	 (26 * 1000 * 1000 * (2))
// eMMC module connects to MMC1 controller
#define EMMC_MODULE_HOST_INDEX	 1

#define CIS_CSA_BYTES (SD_IO_CIS_PTR_BYTES + SD_IO_CSA_PTR_BYTES)
#define CIS_OFFSET_BYTE_0 0
#define CIS_OFFSET_BYTE_1 1
#define CIS_OFFSET_BYTE_2 2
#define CSA_OFFSET_BYTE_0 3
#define CSA_OFFSET_BYTE_1 4
#define CSA_OFFSET_BYTE_2 5
#define UNKNOWN_PRODUCT_INFO_STRING_LENGTH 64

#define GET_BIT_SLICE_FROM_CSD(pCSD, Slice, Size) \
    GetBitSlice(pCSD, SD_CSD_REGISTER_SIZE, Slice, Size)

#define SYNC_EVENTS_ARRAYSIZE 4

typedef struct _SYNCEVENTSARRAY{
    volatile BOOL  fEventsAvailability[SYNC_EVENTS_ARRAYSIZE];  //indicates whether the event is being used or not
    HANDLE hSyncEvents[SYNC_EVENTS_ARRAYSIZE]; //event array for sync transfers
}SYNCEVENTSARRAY, *PSYNCEVENTSARRAY;


// typdef for the current slot state
typedef enum _SD_DEVICE_SPEEDMODE {
    UndefinedSpeedMode = 0,
    LowSpeed,
    FullSpeed,     
    HighSpeed
} SD_DEVICE_SPEEDMODE, *PSD_DEVICE_SPEEDMODE;

#define TRAN_SPEED_FULL_SPEED_VALUE     0x32      //25Mhz, Spec 2.0, section 5.3.2
#define TRAN_SPEED_HIGH_SPEED_VALUE     0x5A      //50Mhz, Spec 2.0, section 5.3.2

class CSDBusRequest;
class CSDHost;
class CSDSlot;
class CSDDevice : public CRefObject, public CStaticContainer <CSDBusRequest, SDBUS_MAX_REQUEST_INDEX > {
public:
    friend class SDBusRequest;
    CSDDevice(DWORD dwFunctionIndex, CSDSlot& sdSlot);
    virtual ~CSDDevice();
    virtual BOOL Init();
    virtual BOOL Attach();
    virtual BOOL Detach();
// public function.
    SDBUS_DEVICE_HANDLE GetDeviceHandle();
    DWORD   GetDeviceFuncionIndex() { return m_FuncionIndex; };
    DWORD   GetReferenceIndex(){ return m_FuncRef; };
    SD_API_STATUS SetCardInterface( PSD_CARD_INTERFACE_EX pInterfaceEx) ;
    BOOL  MMCSwitchto8BitBusWidth();
    SD_API_STATUS SwitchCardSpeedMode(SD_DEVICE_SPEEDMODE SpeedMode) ;
    BOOL GetUpdatedTranSpeedValue(UCHAR& Value);
    SDCARD_DEVICE_TYPE SetDeviceType(SDCARD_DEVICE_TYPE deviceType) { return m_DeviceType = deviceType; };
    SDCARD_DEVICE_TYPE GetDeviceType() { return m_DeviceType; };
    
    SD_API_STATUS DetectSDCard( DWORD& dwNumOfFunct);    
    SD_API_STATUS GetCardRegisters();
    SD_API_STATUS DeactivateCardDetect();
    SD_API_STATUS SelectCardInterface();
    SD_API_STATUS SDGetSDIOPnpInformation(CSDDevice& psdDevice0);
    SD_API_STATUS GetFunctionPowerState(PFUNCTION_POWER_STATE pPowerState);
    SD_API_STATUS HandleDeviceSelectDeselect(SD_SLOT_EVENT SlotEvent,BOOL fSelect);

    HANDLE  GetCallbackHandle() { return m_hCallbackHandle; };
    SDCARD_INFORMATION& GetCardInfo() { return m_SDCardInfo; };
    CSDSlot& GetSlot() { return m_sdSlot; };    
    PVOID   GetDeviceContext() { return m_pDeviceContext; };
// API    
    virtual SD_API_STATUS SDReadWriteRegistersDirect_I(SD_IO_TRANSFER_TYPE ReadWrite, DWORD Address,
        BOOLEAN ReadAfterWrite,PUCHAR pBuffer,ULONG BufferLength);
    virtual SD_API_STATUS SDSynchronousBusRequest_I(UCHAR Command, DWORD Argument,SD_TRANSFER_CLASS TransferClass,
        SD_RESPONSE_TYPE ResponseType,PSD_COMMAND_RESPONSE  pResponse,ULONG NumBlocks,ULONG BlockSize,PUCHAR pBuffer, DWORD  Flags, DWORD cbSize = 0, PPHYS_BUFF_LIST pPhysBuffList =NULL );
    virtual SD_API_STATUS SDBusRequest_I(UCHAR Command,DWORD Argument,SD_TRANSFER_CLASS TransferClass, SD_RESPONSE_TYPE ResponseType,
        ULONG NumBlocks,ULONG BlockSize, PUCHAR pBuffer, PSD_BUS_REQUEST_CALLBACK pCallback, DWORD RequestParam,
        HANDLE *phRequest,DWORD Flags,DWORD cbSize=0, PPHYS_BUFF_LIST pPhysBuffList=NULL );
    virtual VOID SDFreeBusRequest_I(HANDLE hRequest);
    virtual SD_API_STATUS SDBusRequestResponse_I(HANDLE hRequest, PSD_COMMAND_RESPONSE pSdCmdResp);
    virtual BOOL SDCancelBusRequest_I(HANDLE hRequest);
    virtual SD_API_STATUS SDIOConnectDisconnectInterrupt(PSD_INTERRUPT_CALLBACK pIsrFunction, BOOL Connect);
    virtual void HandleDeviceInterrupt();
    VOID NotifyClient(SD_SLOT_EVENT_TYPE Event);

    VOID UpdateClockRate(ULONG ClockRate){m_CardInterfaceEx.ClockRate = ClockRate;};
    
// Load & Unload Driver.
public:
    SD_API_STATUS SDLoadDevice();
    BOOL SDUnloadDevice();
    BOOL IsDriverLoaded() { return (m_pDriverFolder!=NULL? m_pDriverFolder->IsDriverLoaded() : FALSE ); };
    BOOL IsDriverEnabled();
    SD_API_STATUS RegisterClient(HANDLE hCallbackHandle,PVOID  pContext,PSDCARD_CLIENT_REGISTRATION_INFO pInfo);
    LPCTSTR GetClientName() { return m_ClientName; };
    DWORD   GetClientFlags() { return m_ClientFlags; };
    SD_CARD_INTERFACE_EX& GetCardInterface() { return m_CardInterfaceEx;};
    BOOL    IsHighCapacitySDMemory(); 
protected:
    DeviceFolder * m_pDriverFolder;
// Tuple & Info.
public:
    virtual SD_API_STATUS SDGetTuple_I(UCHAR TupleCode,PUCHAR pBuffer,PULONG pBufferSize,BOOL CommonCIS);
    virtual SD_API_STATUS SDCardInfoQuery_I(IN SD_INFO_TYPE InfoType,OUT PVOID pCardInfo, IN ULONG StructureSize);
protected:
    SD_API_STATUS GetCardStatus(SD_CARD_STATUS   *pCardStatus);
    SD_API_STATUS SDGetTupleBytes(DWORD Offset,PUCHAR pBuffer,ULONG NumberOfBytes,BOOL CommonCIS);
    SD_API_STATUS SDGetFunctionPowerControlTuples();
    SD_API_STATUS InfoQueryCID(PVOID pCardInfo,ULONG cbCardInfo);
    SD_API_STATUS InfoQueryCSD(PVOID pCardInfo,ULONG cbCardInfo);
    SD_API_STATUS InfoQueryRCA(PVOID pCardInfo,ULONG cbCardInfo);
    SD_API_STATUS InfoQueryCardInterface(PVOID pCardInfo,ULONG cbCardInfo);
    SD_API_STATUS InfoQueryStatus(PVOID pCardInfo,ULONG  cbCardInfo);
    SD_API_STATUS InfoQuerySDIOInfo(PVOID pCardInfo, ULONG cbCardInfo);
    SD_API_STATUS InfoQueryHostInterface(PVOID pHostInfo,ULONG cbHostInfo);
    SD_API_STATUS InfoQueryBlockCaps(PVOID pCardInfo,ULONG cbCardInfo);
    
// Setup Feature
public:
    SD_API_STATUS SDSetCardFeature_I(SD_SET_FEATURE_TYPE  CardFeature,PVOID pCardInfo,ULONG StructureSize);
protected:
    SD_API_STATUS SDEnableDisableFunction(PSD_IO_FUNCTION_ENABLE_INFO pInfo,BOOL Enable);
    SD_API_STATUS SDSetFunctionBlockSize(DWORD BlockSize);
    SD_API_STATUS SDFunctionSelectPower( BOOL  fLowPower);
    SD_API_STATUS SwitchFunction(PSD_CARD_SWITCH_FUNCTION pSwitchData,BOOL fReadOnly);
    SD_API_STATUS MMCSwitch(DWORD dwAccessMode,DWORD dwAccessIndex, DWORD dwAccessValue, DWORD dwCmdSet);
    SD_API_STATUS SetCardFeature_Interface(SD_CARD_INTERFACE_EX& sdCardInterfaceEx);
protected:
    // BusRequest Queue
    CSDBusRequest *  InsertRequestAtEmpty(PDWORD pdwIndex, CSDBusRequest * pRequest);
    DWORD           m_dwCurSearchIndex;
    DWORD           m_dwCurFunctionGroup; // 6*4 bits.
// Internal
    void SwapByte(__inout_bcount(dwLength) PBYTE pPtr, DWORD dwLength);
    VOID FormatProductString(CHAR const*const pAsciiString, __out_ecount(cchString) PWCHAR pString, DWORD cchString ) const;

// Send Cmd to the Card.
    virtual SD_API_STATUS SendSDAppCmd(UCHAR Command,DWORD Argument,SD_TRANSFER_CLASS TransferClass,SD_RESPONSE_TYPE ResponseType,
                           PSD_COMMAND_RESPONSE pResponse,DWORD NumberOfBlocks,ULONG BlockSize,PUCHAR pBlockBuffer);
    SD_API_STATUS SendSDAppCommand(UCHAR AppCommand,DWORD Argument,SD_RESPONSE_TYPE ResponseType, PSD_COMMAND_RESPONSE pResponse) {
        return SendSDAppCmd( AppCommand, Argument, SD_COMMAND, ResponseType,pResponse,0,0,0);
    }
    SD_API_STATUS SendSDCommand(UCHAR Command, DWORD Argument,SD_RESPONSE_TYPE ResponseType, PSD_COMMAND_RESPONSE pResponse) {
        return SDSynchronousBusRequest_I(Command,Argument,SD_COMMAND,ResponseType,pResponse,0,0,NULL,(DWORD)SD_SLOTRESET_REQUEST);
    }
    // RCA
    SD_CARD_RCA GetRelativeAddress() { return m_RelativeAddress; };
public:
    // Operational Voltage.
    SD_API_STATUS  SetOperationVoltage(SDCARD_DEVICE_TYPE DeviceType, BOOL SetHCPower); // For function Zero.
    DWORD   GetOperationVoltage() { return m_OperatingVoltage; };
    // Card Regiser
    SDCARD_CARD_REGISTERS   GetCachedRegisters() { return m_CachedRegisters; };
    void SetupWakeupSupport();
    void CopyContentFromParent(CSDDevice& psdDevice0);
protected:    
// Sync Request Property
    static void SDSyncRequestCallback(HANDLE /*hDevice*/,PSD_BUS_REQUEST /*hRequest*/,PVOID /*pContext*/,DWORD BusRequestParam) {
        VERIFY(SetEvent((HANDLE)BusRequestParam));
    }
    
    
protected:
    CSDSlot&    m_sdSlot;
    DWORD       m_FuncionIndex;
    static  DWORD   g_FuncRef;
    DWORD       m_FuncRef;
    BOOL        m_fAttached;
    
    BOOL                    m_fIsHandleCopied;      // Indicate whether handle is dupplicated or not.
    HANDLE                  m_hCallbackHandle;      // callback handle. It needed for CeDriverPerformCallback
    TCHAR                   m_ClientName[MAX_SDCARD_CLIENT_NAME]; // client name
    DWORD                   m_ClientFlags;          // flags set by the client driver
    SDCARD_DEVICE_TYPE      m_DeviceType;           // device type
    PVOID                   m_pDeviceContext;       // device specific context
    PSD_SLOT_EVENT_CALLBACK m_pSlotEventCallBack;   // slot event callback
    SD_CARD_RCA             m_RelativeAddress;      // card's relative address
    SDCARD_CARD_REGISTERS   m_CachedRegisters;      // cached registers
    DWORD                   m_OperatingVoltage;     // current operating voltage
    PVOID                   m_pSystemContext;       // system context
    SDCARD_INFORMATION      m_SDCardInfo;           // information for SD Card (based on type)
    SD_CARD_INTERFACE_EX    m_CardInterfaceEx;      // card's current interface
    BOOL                    m_bCardSelectRequest;   // request the card to be selected
    BOOL                    m_bCardDeselectRequest; // request the card to be deselected
    CRITICAL_SECTION        m_csChildRequests;      // guarantee the atomic operation of submitting all child requests
    SYNCEVENTSARRAY         m_SyncEventArray;       //provide an array of events for sync reqeusts to use (avoid creating events on the fly every time)
    SD_DEVICE_SPEEDMODE     m_CurSpeedMode;         //the speed class that the card is at
    BOOL                    m_fMMCSectorAccessMode; //indicate MMC card can use sector mode or not

protected:
    SD_API_STATUS SetCardPower(SDCARD_DEVICE_TYPE DeviceType,DWORD OperatingVoltageMask,BOOL SetHCPower);
    VOID GetInterfaceOverrides();
    SD_API_STATUS GetCustomRegPath(__out_ecount(cchPath) LPTSTR  pPath, DWORD cchPath, BOOL BasePath);
    VOID UpdateCachedRegisterFromResponse(SD_INFO_TYPE  Register,PSD_COMMAND_RESPONSE pResponse) ;
    DWORD GetBitSlice(__in_bcount(cbBuffer) UCHAR const*const pBuffer, ULONG cbBuffer, DWORD dwBitOffset, UCHAR ucBitCount);
    BOOL    IsValid20Card();
    VOID LockForChildRequests(){EnterCriticalSection(&m_csChildRequests);};
    VOID UnlockForChildRequests(){LeaveCriticalSection(&m_csChildRequests);};

    HANDLE TryGetSyncEventHandle(int &iEventIndex ){
        int index = 0;
        HANDLE hRet = NULL;
        
        LockForChildRequests();
        while((index < SYNC_EVENTS_ARRAYSIZE) && (m_SyncEventArray.fEventsAvailability[index] == FALSE))
            index ++;
        
        if(index < SYNC_EVENTS_ARRAYSIZE){
            m_SyncEventArray.fEventsAvailability[index] = FALSE;
            hRet = m_SyncEventArray.hSyncEvents[index];
            iEventIndex = index; 
        }
        else
            iEventIndex = -1;

        UnlockForChildRequests();
        return hRet; 
            
    };


    inline void ReturnSyncEventHandle(int iEventIndex){
        if (iEventIndex >= 0 && iEventIndex < SYNC_EVENTS_ARRAYSIZE) {
            ResetEvent(m_SyncEventArray.hSyncEvents[iEventIndex]);
            m_SyncEventArray.fEventsAvailability[iEventIndex] = TRUE;
        } else {
            ASSERT(iEventIndex >= 0 && iEventIndex < SYNC_EVENTS_ARRAYSIZE);
        }
    };
    CSDDevice&operator=(CSDDevice&){ASSERT(FALSE);}
};


