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

#include <CSync.h>
#include <CRefCon.h>
#include <defbus.h>
#include "sdworki.hpp"

class CSDBusRequest;
class CSDHost;
class CSDSlot;

// bus driver zones
#define SDBUS_ZONE_HCD                  SDCARD_ZONE_0
#define ENABLE_SDBUS_ZONE_HCD           ZONE_ENABLE_0
#define SDBUS_ZONE_DISPATCHER           SDCARD_ZONE_1
#define ENABLE_SDBUS_ZONE_DISPATCHER    ZONE_ENABLE_1
#define SDCARD_ZONE_SHUTDOWN            SDCARD_ZONE_2
#define ENABLE_SDBUS_ZONE_SHUTDOWN      ZONE_ENABLE_2
#define SDBUS_ZONE_POWER                SDCARD_ZONE_3
#define ENABLE_SDBUS_ZONE_POWER         ZONE_ENABLE_3
#define SDBUS_ZONE_DEVICE               SDCARD_ZONE_4
#define ENABLE_SDBUS_ZONE_DEVICE        ZONE_ENABLE_4
#define SDBUS_ZONE_REQUEST              SDCARD_ZONE_5
#define ENABLE_SDBUS_ZONE_REQUEST       ZONE_ENABLE_5
#define SDBUS_ZONE_BUFFER               SDCARD_ZONE_6
#define ENABLE_SDBUS_ZONE_BUFFER        ZONE_ENABLE_6
#define SDBUS_SOFT_BLOCK                SDCARD_ZONE_7
#define ENABLE_SDBUS_SOFT_BLOCK         ZONE_ENABLE_7

#define BUS_REQUEST_FREE_SPACE_TAG 0xdaada5cc


#define SDCARD_REQUEST_RETRY_KEY        TEXT("RequestRetryCount")
#define DEFAULT_BUS_REQUEST_RETRY_COUNT     3
#define SDCARD_THREAD_PRIORITY_KEY      TEXT("ThreadPriority")
#define DEFAULT_THREAD_PRIORITY             100
#define SDCARD_PASTPATH_THRESHOLD       TEXT("Threshold")
#define DEFAULT_PASTPATH_THRESHOLD      0x800   // 2k.
#define SDCARD_FASTPATH_NODIRECTRET_KEY      TEXT("FastPathNoDirectReturn")

typedef struct _FREE_BUS_REQUEST_SPACE *PFREE_BUS_REQUEST_SPACE;
typedef struct _FREE_BUS_REQUEST_SPACE {
    PFREE_BUS_REQUEST_SPACE pNextFreeTransfer;
    DWORD dwFreeSpaceTag;
    DWORD dwSpaceSize;
    DWORD dwReserved;
} FREE_BUS_REQUEST_SPACE, *PFREE_BUS_REQUEST_SPACE;

#define BUS_VER_STR_2_0     (TEXT("VER 2.0"))
#define BUS_VER_FOR_HOST    BUS_VER_STR_2_0

#define SD_MAXIMUM_SLOT_PER_SDHOST 16 // This is limited 4 bit count in device handle.

inline SD_CARD_INTERFACE ConvertFromEx(SD_CARD_INTERFACE_EX sdInterfaceEx)
{
    SD_CARD_INTERFACE sdCardInterface = {
        sdInterfaceEx.InterfaceModeEx.bit.mmc8Bit!=0? SD_INTERFACE_MMC_8BIT :
            (sdInterfaceEx.InterfaceModeEx.bit.sd4Bit!=0?SD_INTERFACE_SD_4BIT: SD_INTERFACE_SD_MMC_1BIT),
        sdInterfaceEx.ClockRate,
        sdInterfaceEx.InterfaceModeEx.bit.sdWriteProtected!=0
    };
    return sdCardInterface;
}

// Debug Function.
extern "C" {
VOID SDOutputBuffer(__out_bcount(BufferSize) PVOID pBuffer, ULONG BufferSize) ;
BOOLEAN SDPerformSafeCopy(__out_bcount(Length)    PVOID pDestination,  __in_bcount(Length) const VOID *pSource,ULONG Length);
DWORD SDProcessException(LPEXCEPTION_POINTERS pException) ;
}
class CSDHost:public SDCARD_HC_CONTEXT, public CRefObject {
    friend class CSDDevice;
    friend class CSDSlot;
public:
    CSDHost(DWORD dwNumSlot);
    virtual ~CSDHost();
    virtual BOOL Init();
    virtual BOOL Attach();
    virtual BOOL Detach();
    BOOL    IsAttached() { return m_fHostAttached; };
// SD Host Index.
public:
    DWORD   GetIndex() {    return m_dwSdHostIndex;  };
    DWORD   SetIndex (DWORD dwIndex)  { return (m_dwSdHostIndex= dwIndex);  };
    DWORD   GetSlotCount() { return m_dwNumOfSlot; };
    CSDSlot * GetSlot(DWORD dwSlot) { return (dwSlot<m_dwNumOfSlot? m_SlotArray[dwSlot]: NULL) ; };
    SD_API_STATUS   BusRequestHandler(DWORD dwSlot, PSD_BUS_REQUEST pSdBusRequest)  {
        return pBusRequestHandler((PSDCARD_HC_CONTEXT)this,dwSlot,pSdBusRequest);
        
    }; // bus request handler
    SD_API_STATUS   SlotOptionHandler(DWORD dwSlot, SD_SLOT_OPTION_CODE sdSlotOption, PVOID pvParam, ULONG uSize) { // slot option handler
        return pSlotOptionHandler((PSDCARD_HC_CONTEXT)this,dwSlot,sdSlotOption,pvParam,uSize);
    }
    BOOL            CancelIOHandler(DWORD dwSlot, PSD_BUS_REQUEST psdBusRequest) {   // cancel request handler
        return pCancelIOHandler((PSDCARD_HC_CONTEXT)this,dwSlot,psdBusRequest);
    }
    SD_API_STATUS   InitHandler () { return pInitHandler((PSDCARD_HC_CONTEXT)this); };      // init handler       
    SD_API_STATUS   DeinitHandler() { return pDeinitHandler((PSDCARD_HC_CONTEXT)this); };     // deinit handler
    SD_API_STATUS   ChangeCardPowerHandler(DWORD dwSlot, INT iPower) { ; // Pointer to power control handler
        return pChangeCardPowerHandler((PSDCARD_HC_CONTEXT)this,dwSlot,iPower);
    }
    VOID            SDHCAccessLock() {
        EnterCriticalSection(&HCCritSection);
    }
    VOID            SDHCAccessUnlock() {
        LeaveCriticalSection(&HCCritSection);
    }
    
protected:
    DWORD   m_dwSdHostIndex;
    BOOL    m_fIntialized;
    BOOL    m_fHostAttached;
    SD_API_STATUS   SlotSetupInterface(DWORD dwSlot, PSD_CARD_INTERFACE_EX psdCardInterfaceEx );
    static  SD_API_STATUS DefaultChangeCardPower(PSDCARD_HC_CONTEXT pHCCardContext,DWORD Slot,INT CurrentDelta);
// SD Slot.
private:
    const DWORD   m_dwNumOfSlot;
    CSDSlot * m_SlotArray[ SD_MAXIMUM_SLOT_PER_SDHOST ];
    CSDHost&operator=(CSDHost&){ASSERT(FALSE);}
};

#define SD_MAXIMUM_SDHOST 16
#define SD_SUB_BUSNAME_VALNAME     TEXT("SubBusName")     // device's name on the parent bus
#define SD_SUB_BUSNAME_VALTYPE     REG_SZ
#define SD_SUB_BUSNAME_DEFAULT     TEXT("SDBus")

class CSDHostContainer :public DefaultBusDriver, protected CStaticContainer <CSDHost, SD_MAXIMUM_SDHOST > {
public:
    CSDHostContainer(LPCTSTR pszActiveKey);
    ~CSDHostContainer();
    virtual BOOL Init();
    virtual DWORD GetBusNamePrefix(LPTSTR pszReturnBusName, DWORD cchReturnBusName);
    virtual BOOL IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut);
    virtual BOOL SDC_IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut);
// Help Function
    static DWORD    GetRetryCount()  { return g_pSdContainer!=NULL? g_pSdContainer->m_BusRequestRetryCount: 0; };
protected:
    virtual DWORD   GetSlotInfo(PBYTE pslotInfoArray, DWORD Length, BOOL fInfoEx) ;
    virtual SD_API_STATUS GetSlotPowerControlInfo(DWORD slotIndexWanted,PSLOT_POWER_DATA pPCdata) ;
    virtual SD_API_STATUS GetFunctionPowerControlInfo(DWORD slotIndexWanted, DWORD FunctionIndexWanted, PFUNCTION_POWER_DATA pFPCdata) ;
    virtual SD_API_STATUS SetSlotPowerControl(DWORD slotIndexWanted, BOOL  fEnablePowerControl);
    virtual BOOL    SlotCardSelectDeselect(DWORD SlotIndexWanted, SD_SLOT_EVENT Event) ;
    
private:
    TCHAR   m_szSubBusNamePrefix[DEVNAME_LEN];
    CRegistryEdit m_deviceKey;

    PFREE_BUS_REQUEST_SPACE m_pFreeBusRequestSpace;
    DWORD   m_dwMinSize;
    DWORD   m_BusRequestRetryCount;
    PVOID AllocateBusRequestImp(size_t stSize);
    void  FreeBusRequestImp(CSDBusRequest *pBusRequest);
    void  DeleteAllTransferSpace();
public:
    LPCTSTR GetSubBusNamePrefix();
    
    static PVOID AllocateBusRequest(size_t stSize);
    static void  FreeBusRequest(CSDBusRequest *pBusRequest);
    static CSDHostContainer * GetHostContainer () { return g_pSdContainer; };
    static DWORD RegValueDWORD(LPCTSTR lpRegName,DWORD dwDefault);
    static PVOID DriverInit(LPCTSTR pszActiveKey);
    static VOID DriverDeInit(PVOID pContent);
    static CRegistryEdit& GetDeviceKey()  {
        PREFAST_ASSERT(g_pSdContainer!=NULL);
        return g_pSdContainer->m_deviceKey;
    }
    static CSDHost * GetSDHost(DWORD dwIndex) {
        PREFAST_ASSERT(g_pSdContainer!=NULL);
        return g_pSdContainer->ObjectIndex(dwIndex);
    }
    static CSDHost * GetSDHost(CSDHost * pUnknowHost);
    static CSDDevice * GetDeviceByDevice(HANDLE hDevice);
    static CSDDevice * GetDeviceByRequest(HANDLE hDevice);
private:
    BOOL GetHCandSlotbySlotIndex(CSDHost ** ppHost,CSDSlot**ppSlot,DWORD dwIndex );
public:
    static CSDHost * InsertSDHost(CSDHost * psdHost ) {
        PREFAST_ASSERT(g_pSdContainer!=NULL);
        DWORD dwIndex;
        if (psdHost) {
            CStaticContainer * pHostContainer = g_pSdContainer;
            pHostContainer->Lock();
            psdHost = pHostContainer->InsertObjectAtEmpty(&dwIndex,psdHost);
            if (psdHost)
                psdHost->SetIndex(dwIndex);
            pHostContainer->Unlock();
            return psdHost;
        }
        else
            return NULL;
            
    }
    static CSDHost * RemoveSDHostBy(DWORD dwIndex) {
        PREFAST_ASSERT(g_pSdContainer!=NULL);
        return g_pSdContainer->RemoveObjectBy(dwIndex);
    }
    static CSDHost * RemoveSDHostBy(CSDHost * pSd) {
        PREFAST_ASSERT(g_pSdContainer!=NULL);
        return g_pSdContainer->RemoveObjectBy(pSd);
    }
// Host HC Interface.
    static SD_API_STATUS SDHCDAllocateContext__X(DWORD  NumberOfSlots,  PSDCARD_HC_CONTEXT *ppExternalHCContext);
    static SD_API_STATUS SDHCDRegisterHostController__X(PSDCARD_HC_CONTEXT pExternalHCContext);
    static SD_API_STATUS SDHCDDeregisterHostController__X(PSDCARD_HC_CONTEXT pExternalHCContext);
    static VOID SDHCDDeleteContext__X(PSDCARD_HC_CONTEXT pExternalHCContext);
    static VOID SDHCDPowerUpDown__X(PSDCARD_HC_CONTEXT  pExternalHCContext,BOOL PowerUp,BOOL SlotKeepPower, DWORD SlotIndex);
    static VOID SDHCDIndicateBusRequestComplete__X(PSDCARD_HC_CONTEXT pExternalHCContext,PSD_BUS_REQUEST pRequest, SD_API_STATUS Status);
    static PSD_BUS_REQUEST SDHCDGetAndLockCurrentRequest__X(PSDCARD_HC_CONTEXT pExternalHCContext, DWORD SlotIndex);
    static VOID SDHCDUnlockRequest__X(PSDCARD_HC_CONTEXT  pExternalHCContext,PSD_BUS_REQUEST pRequest);
    static VOID SDHCDIndicateSlotStateChange__X(PSDCARD_HC_CONTEXT pExternalHCContext, DWORD SlotNumber, SD_SLOT_EVENT Event);

private:
    static CSDHostContainer * g_pSdContainer;
};


