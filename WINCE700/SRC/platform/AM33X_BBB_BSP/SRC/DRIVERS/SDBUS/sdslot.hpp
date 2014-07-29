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
// Copyright (c) Microsoft Corporation.  All rights reserved.
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
//     SdSlot.hpp
// Abstract:  
//     Definition for the sd Slot.
//
// 
// 
// Notes: 
// 
//
#pragma once

#include <CRefCon.h>
#include "sdworki.hpp"

class SDBusRequest;
class CSDDevice;
class CSDHost;

#include "sddevice.hpp"

#define SD_MAXIMUM_DEVICE_PER_SLOT 8 // This is corresponding to function SD Spec defined as 8
// typdef for the current slot state
typedef enum __SD_SLOT_STATE {
    SlotInactive = 0,    // slot is inactive
    SlotIdle,            // slot is idle (after power up)
    Ready,               // the slot is ready, the client driver now has control
    SlotDeviceEjected,   // the device is being ejected
    SlotInitFailed,      // slot initialization failed
    SlotDeselected,      // card in slot is deselected
    SlotResetting        // slot is resetting
} SD_SLOT_STATE, *PSD_SLOT_STATE;


class CSDBusReqAsyncQueue : private CLockObject{
public:
    CSDBusReqAsyncQueue();
    virtual ~CSDBusReqAsyncQueue();
    virtual BOOL    Init();
    virtual BOOL    Detach();
    BOOL    RemoveAllRequest();
    SD_API_STATUS   QueueBusRequest(CSDBusRequest * pRequest);
    CSDBusRequest * CompleteRequest(CSDBusRequest * pRequest,SD_API_STATUS Status);
    CSDBusRequest * RemoveRequest(CSDBusRequest * pRequest);
    CSDBusRequest * GetRequestFromHead();
    BOOL    IsEmpty() { return (m_pQueueListHead==NULL); };
protected:
    CSDBusRequest * m_pQueueListHead;          // list entry
    CSDBusRequest * m_pQueueListLast;
    BOOL            m_fAttached;
    DWORD           m_FastPathNoDirectRet;
    virtual SD_API_STATUS   SubmitRequestToHC(CSDBusRequest * pRequest) = 0 ;
    virtual BOOL    CancelRequestFromHC(CSDBusRequest * pRequest) = 0;
    virtual BOOL SlotHasRequest() = 0;

};

// the following bits are for flags field in the slot context
#define SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED   0x00000001

class CSDSlot: public SDCARD_HC_SLOT_INFO, public CSDWorkItem, public CSDBusReqAsyncQueue {
    friend class CSDDevice;
public:
    CSDSlot(DWORD dwSlotIndex, CSDHost& m_sdHost);
    virtual     ~CSDSlot();
    virtual BOOL Init();
    virtual BOOL Attach();
    virtual BOOL Detach();
    virtual VOID PowerUp() { ; }// I don't know what to do now,
    virtual VOID PowerDown(BOOL fKeepPower) { 
        if (!fKeepPower)
            m_SlotState = SlotDeviceEjected;
    };
    CSDHost&    GetHost() { return m_SdHost; };
// Function
    virtual VOID SlotStateChange(SD_SLOT_EVENT Event);
    virtual BOOL HandleAddDevice();
    virtual BOOL HandleRemoveDevice();
    virtual BOOL HandleDeviceInterrupting();
    virtual BOOL HandleSlotSelectDeselect(SD_SLOT_EVENT Event);
    DWORD   GetSlotIndex() { return m_dwSlotIndex; };
    USHORT  GetSlotPower() { return m_AllocatedPower; };
    USHORT  SetSlotPower(USHORT allocatedPower) { return m_AllocatedPower = allocatedPower; };
    SD_SLOT_STATE   GetSlotState() { return m_SlotState; };
protected:
    CSDHost& m_SdHost;
    DWORD   m_dwSlotIndex;
// 
    SD_SLOT_STATE               m_SlotState;         // slot state
    DWORD                       m_Flags;             // flags
    SD_CARD_INTERFACE_EX        m_SlotInterfaceEx;   // Slot interface
    BOOL                        m_fEnablePowerControl; // enable or disable power control for the slot
    USHORT                      m_AllocatedPower;    // power allocated
    // Device.
    CLockObject m_SlotLock;
    CSDDevice * m_pFuncDevice[SD_MAXIMUM_DEVICE_PER_SLOT];

    // Request Input Function.
    CLockObject m_RequestLock;
    
    // Implementation Detail Function.
//    virtual SD_API_STATUS SetOperationVoltage();
public:
    BOOL        RemoveAllDevice();
    CSDDevice * RemoveDevice(DWORD dwFunctionIndex);
    CSDDevice * InsertDevice(DWORD dwFunctionIndex,CSDDevice * pObject);
    inline SD_API_STATUS CheckSlotReady() {
        if ((SlotIdle != m_SlotState) && (Ready != m_SlotState) && (SlotResetting != m_SlotState)) {
            if (SlotDeviceEjected == m_SlotState) {
                DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDCard: Device in Slot:%d is about to be removed. Rejecting Request \n"), m_dwSlotIndex));    
                return SD_API_STATUS_DEVICE_REMOVED;
            } else {
                DEBUG_CHECK(SlotDeviceEjected == m_SlotState,(TEXT("SubmitBusRequest: Slot is not ready. Current slot state is %d \n"),m_SlotState ));
                return SD_API_STATUS_UNSUCCESSFUL;
            }
        }
        return SD_API_STATUS_SUCCESS;
    }    
    inline CSDDevice * GetFunctionDevice(DWORD dwIndex, DWORD dwReference){
        m_SlotLock.Lock();
        CSDDevice*  pReturn = dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT ? m_pFuncDevice[dwIndex] : NULL;
        if( pReturn && pReturn->GetReferenceIndex() == dwReference )  {
            pReturn->AddRef();
        }
        else
            pReturn = NULL;
        m_SlotLock.Unlock();
        return pReturn;
    }
    inline CSDDevice * GetFunctionDevice(DWORD dwIndex){
        m_SlotLock.Lock();
        CSDDevice*  pReturn = dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT ? m_pFuncDevice[dwIndex] : NULL;
        if( pReturn )  {
            pReturn->AddRef();
        }
        else
            pReturn = NULL;
        m_SlotLock.Unlock();
        return pReturn;
    }
    DWORD   GetNumOfFunctionDevice();
    void    SetSlotPowerControl(BOOL fSet) { m_fEnablePowerControl = fSet; };
    BOOL    GetSlotPowerControl() { return m_fEnablePowerControl; };
    BOOL    SlotCardSelectDeselect(SD_SLOT_EVENT Event) ;
// Interrupt Tracking;
    BOOL    IsSlotInterruptOn() { return ((m_Flags & SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED)!=0) ; };

    BOOL    SwitchtoHighSpeed();


protected:
    BOOL    EnableSDIOInterrupts(void);
    BOOL    DisableSDIOInterrupts(void);
    
protected:
    virtual SD_API_STATUS CreateChildDevice(SDCARD_DEVICE_TYPE sdCard_DeviceType, DWORD dwFunctionIndex,CSDDevice& psdDevice0);
private:
    virtual VOID    SlotStatusChangeProcessing(SD_SLOT_EVENT sdEvent); 
protected:
    VOID    DelayForPowerUp();
    SD_API_STATUS   SDSetSlotPower(DWORD Setting) {
        return m_SdHost.SlotOptionHandler(m_dwSlotIndex,SDHCDSetSlotPower,&(Setting),sizeof(Setting));
    }
    SD_API_STATUS EnumMultiFunction(CSDDevice& psdDevice,DWORD dwNumOfFunc );
    SD_API_STATUS SDSetCardInterfaceForSlot(PSD_CARD_INTERFACE_EX pSetting);
    SD_API_STATUS SelectSlotInterface();
    DWORD SDGetOperationalVoltageRange(DWORD OcrValue);
    CSDSlot&operator=(CSDSlot&){ASSERT(FALSE);}

// HC interface.
public:
    virtual SD_API_STATUS   SubmitRequestToHC(CSDBusRequest * pRequest) ;
    virtual BOOL    CancelRequestFromHC(CSDBusRequest * pRequest);
    virtual BOOL    CompleteRequestFromHC(CSDBusRequest * pRequest,SD_API_STATUS status);
    CSDBusRequest * SDHCGetAndLockCurrentRequest_I();
    void            SDHCDUnlockRequest_I(PSD_BUS_REQUEST  hReques) ;
    BOOL SDSlotEnableSDIOInterrupts() ;
    BOOL SDSlotDisableSDIOInterrupts();
    virtual BOOL SlotHasRequest(){ return (m_curHCOwned != NULL);}

protected:
    CSDBusRequest * m_curHCOwned;
    LONG            m_lcurHCLockCount;
    DWORD           m_FastPathThreshHold;   // FastPath Threshold.

};



