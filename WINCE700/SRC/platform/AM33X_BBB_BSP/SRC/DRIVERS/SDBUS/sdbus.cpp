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
/*++
Module Name:  
    SDBus.cpp
Abstract:
    SDBus Implementation.

Notes: 
--*/

#include <windows.h>
#include <bldver.h>
#include <cesdbus.h>
#include <marshal.hpp>

#include <sdhcd.h>

#include "sdbus.hpp"
#include "sdslot.hpp"
#include "sdbusreq.hpp"



///////////////////////////////////////////////////////////////////////////////
// DefaultChangeCardPower - Default power allocation/deallocation handler for the 
//                          bus driver.
//                             
// Input:   pHCCardContext  - host controller context   
//          Slot        - Slot number
//          CurrentDelta- Change in Slot current
// Output:
// Return:  SD_API_STATUS_INSUFFICIENT_HOST_POWER - the slot will exceeded is maximum
//              current limit.
//          SD_API_STATUS_SUCCESS - No current limit has been exceeded. 
// Notes:   If the Host Controller does not assign its own power allocation/deallocation 
//          handler.  This function is called each time a client requests a change that
//          affects the card's power state.
/////////////////////////////////////////////////////////////////////////////// 
SD_API_STATUS CSDHost::DefaultChangeCardPower(PSDCARD_HC_CONTEXT pHCCardContext,DWORD Slot,INT CurrentDelta)
{
    USHORT  SlotPower = 0;
    INT     NewSlotPower = 0;
    PREFAST_ASSERT(pHCCardContext!=NULL);
    CSDHost * pHost = (CSDHost *) pHCCardContext;
    DWORD dwHostIndex = MAXDWORD;
    SD_API_STATUS status = SD_API_STATUS_NO_SUCH_DEVICE;
    __try {
        dwHostIndex = pHost->m_dwSdHostIndex ;
    }
    __except (SDProcessException(GetExceptionInformation())) {
        dwHostIndex = MAXDWORD;
    }
    pHost = CSDHostContainer::GetSDHost(dwHostIndex);
    if (pHost && (PSDCARD_HC_CONTEXT)pHost == pHCCardContext) {
        CSDSlot *pSlot = pHost->GetSlot(Slot);
        if (pSlot) {
            SlotPower = pSlot->GetSlotPower();
            NewSlotPower = ((INT) SlotPower) + CurrentDelta;
            status = SD_API_STATUS_SUCCESS ;
            if(NewSlotPower > DEFAULT_MAX_SLOT_CURRENT) {
                DbgPrintZo(SDCARD_ZONE_WARN, 
                           (TEXT("SDBusDriver: Power change denied, current over limmit by %dmA\n"),
                                               NewSlotPower - DEFAULT_MAX_SLOT_CURRENT));
                status = SD_API_STATUS_INSUFFICIENT_HOST_POWER;
            }
            else if(NewSlotPower < 0) {
                DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: Power change math issue, current under limmit by %dmA\n"), NewSlotPower));
            }
        }
    }
    if (pHost)
        pHost->DeRef();
    
    // Do not change SlotPower in this function.
    // It will be tracked by the bus driver's calling function.

    return status;
}


//------------------------------------------------------------------
CSDHost::CSDHost(DWORD dwNumSlot)
:   m_dwNumOfSlot (min(dwNumSlot,SD_MAXIMUM_SLOT_PER_SDHOST)) 
{
    m_dwSdHostIndex = MAXDWORD;
    memset((SDCARD_HC_CONTEXT *)this, 00, sizeof(SDCARD_HC_CONTEXT));
    dwVersion = SDCARD_HC_BUS_INTERFACE_VERSION;
    InitializeCriticalSection(&HCCritSection);
    
    for (DWORD dwIndex = 0; dwIndex<SD_MAXIMUM_SLOT_PER_SDHOST; dwIndex ++ )
        m_SlotArray[dwIndex] = NULL ;
    m_fIntialized = FALSE;
    m_fHostAttached = FALSE;
    m_fIntialized = FALSE;
}

BOOL CSDHost::Init()
{
    if (!m_fIntialized) {
        // set the number of slots
        // such that the host can detect the version of the bus driver
        VERIFY(SUCCEEDED(StringCchCopy(HostControllerName,_countof(HostControllerName),  BUS_VER_FOR_HOST)));
        // set the default power control handler function
        pChangeCardPowerHandler = DefaultChangeCardPower;
        dwVersion = SDCARD_HC_BUS_INTERFACE_VERSION ;
        
        for (DWORD dwIndex = 0;  dwIndex<m_dwNumOfSlot; dwIndex ++) {
            m_SlotArray[dwIndex] = new CSDSlot(dwIndex,*this);
            if (!(m_SlotArray[dwIndex] && m_SlotArray[dwIndex]->Init())) {
                ASSERT(FALSE);
                return FALSE;
            }
        }
        m_fIntialized = TRUE;
        return TRUE;
    }
    return FALSE;
}
CSDHost::~CSDHost()
{
    ASSERT(!m_fHostAttached);
    for (DWORD dwIndex= 0; dwIndex< m_dwNumOfSlot; dwIndex++) {
        if (m_SlotArray[ dwIndex ]) {
            delete m_SlotArray[ dwIndex ];
        }
    };
    DeleteCriticalSection(&HCCritSection);
}

BOOL CSDHost::Attach()
{
    if (m_fIntialized && !m_fHostAttached) {
        m_fHostAttached = TRUE ;
        for (DWORD dwIndex= 0; dwIndex< m_dwNumOfSlot; dwIndex++) {
            if (m_SlotArray[ dwIndex ]) {
                m_SlotArray[ dwIndex ]->Attach();
            }
        };
        return TRUE;
    }
    return FALSE;
}
BOOL CSDHost::Detach()
{
    if (m_fHostAttached) {
        m_fHostAttached = FALSE;
        for (DWORD dwIndex= 0; dwIndex< m_dwNumOfSlot; dwIndex++) {
            if (m_SlotArray[ dwIndex ]) {
                m_SlotArray[ dwIndex ]->Detach();
            }
        };
    };
    return TRUE;
}
SD_API_STATUS CSDHost::SlotSetupInterface(DWORD dwSlot, PSD_CARD_INTERFACE_EX psdCardInterfaceEx )
{
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;
    if (psdCardInterfaceEx!= NULL && dwSlot < m_dwNumOfSlot && m_SlotArray[dwSlot]!=NULL) {
        if (psdCardInterfaceEx->InterfaceModeEx.bit.sdHighSpeed == 0 ) { // we can do this on old API.
            SD_CARD_INTERFACE sdCardInterface = ConvertFromEx(*psdCardInterfaceEx);
            status = SlotOptionHandler(dwSlot, SDHCDSetSlotInterface,&sdCardInterface,sizeof(sdCardInterface));
            if (SD_API_SUCCESS(status)){
                psdCardInterfaceEx->InterfaceModeEx.bit.sd4Bit = (sdCardInterface.InterfaceMode == SD_INTERFACE_SD_4BIT? 1: 0);
                psdCardInterfaceEx->ClockRate = sdCardInterface.ClockRate;
                psdCardInterfaceEx->InterfaceModeEx.bit.sdWriteProtected = (sdCardInterface.WriteProtected?1:0);
            }
        }
        else 
            status = SlotOptionHandler(dwSlot, SDHCDSetSlotInterfaceEx, psdCardInterfaceEx, sizeof(*psdCardInterfaceEx));
    }
    ASSERT(SD_API_SUCCESS(status));
    return status;
}

//---------------------- Bus Container ----------------------------------------
CSDHostContainer * CSDHostContainer::g_pSdContainer = NULL;
CSDHostContainer::CSDHostContainer(LPCTSTR pszActiveKey)
:   DefaultBusDriver(pszActiveKey)
,   m_deviceKey(pszActiveKey)
{
    m_pFreeBusRequestSpace = NULL ;
    m_dwMinSize=0;
    m_szSubBusNamePrefix[0] = 0 ;
    DWORD dwType;
    DWORD dwDataLen = sizeof(m_szSubBusNamePrefix) ;
    if (m_deviceKey.IsKeyOpened() && 
            m_deviceKey.RegQueryValueEx(SD_SUB_BUSNAME_VALNAME,&dwType,(PBYTE)m_szSubBusNamePrefix,&dwDataLen) &&
            dwType == SD_SUB_BUSNAME_VALTYPE ) {
        m_szSubBusNamePrefix[_countof(m_szSubBusNamePrefix)-1] = 0 ;
    }
    else 
        m_szSubBusNamePrefix[0] = 0 ;
}
CSDHostContainer::~CSDHostContainer()
{
    ((CStaticContainer *)this)->Lock();
    while (m_pFreeBusRequestSpace) {
        PFREE_BUS_REQUEST_SPACE pNext = m_pFreeBusRequestSpace->pNextFreeTransfer ;
        free(m_pFreeBusRequestSpace);
        m_pFreeBusRequestSpace = pNext;
    }
    ((CStaticContainer *)this)->Unlock();
}
BOOL CSDHostContainer::Init()
{
    if (DefaultBusDriver::Init() && m_deviceKey.IsKeyOpened()) {
        m_BusRequestRetryCount = RegValueDWORD (SDCARD_REQUEST_RETRY_KEY, DEFAULT_BUS_REQUEST_RETRY_COUNT);
        return TRUE;
    }
    else
        return FALSE;
}
#define SD_BUS_PREFIX TEXT("SDBUS")
DWORD CSDHostContainer::GetBusNamePrefix(__out_ecount(dwSizeInUnit) LPTSTR lpReturnBusName,DWORD dwSizeInUnit)
{
    DWORD dwUnitCopy = min(dwSizeInUnit,sizeof(SD_BUS_PREFIX)/sizeof(TCHAR));
    if ( lpReturnBusName && dwUnitCopy) {
        HRESULT hr = StringCchCopy( lpReturnBusName, dwSizeInUnit, SD_BUS_PREFIX);
        if ( !SUCCEEDED(hr)) {
            lpReturnBusName[0]=0; // Terminate it.
            dwUnitCopy = 0 ;
        }
        return dwUnitCopy;
    }
    else
        return 0;
}
PVOID CSDHostContainer::AllocateBusRequestImp(size_t stSize)
{
    PVOID pReturn = NULL ;
    ((CStaticContainer *)this)->Lock();
    if (stSize> m_dwMinSize) {
        DEBUGMSG(SDCARD_ZONE_WARN && m_dwMinSize!=0,(L"AllocateBusRequest Changed From %d, to %d",m_dwMinSize,stSize) );
        DeleteAllTransferSpace();
        m_dwMinSize= stSize;
    }
    if (m_pFreeBusRequestSpace==NULL) {
        m_pFreeBusRequestSpace =(PFREE_BUS_REQUEST_SPACE) malloc(sizeof(FREE_BUS_REQUEST_SPACE)+m_dwMinSize);
        if (m_pFreeBusRequestSpace) {
            m_pFreeBusRequestSpace->dwFreeSpaceTag = BUS_REQUEST_FREE_SPACE_TAG;
            m_pFreeBusRequestSpace->dwSpaceSize = m_dwMinSize;
            m_pFreeBusRequestSpace->pNextFreeTransfer = NULL;
        }
    }
    if (m_pFreeBusRequestSpace) {
        ASSERT(m_pFreeBusRequestSpace->dwFreeSpaceTag == BUS_REQUEST_FREE_SPACE_TAG);
        ASSERT(m_pFreeBusRequestSpace->dwSpaceSize>=stSize);
        pReturn = (PVOID)(m_pFreeBusRequestSpace+1);
        m_pFreeBusRequestSpace = m_pFreeBusRequestSpace->pNextFreeTransfer ;
    };
    ((CStaticContainer *)this)->Unlock();
    return pReturn;
}
CSDHost * CSDHostContainer::GetSDHost(CSDHost * pUnknowHost)
{
    DWORD dwIndex = MAXDWORD;
    if (pUnknowHost) {
        __try { dwIndex = pUnknowHost->GetIndex(); }
        __except(SDProcessException(GetExceptionInformation())) {
            dwIndex = MAXDWORD;
        }
    }
    return GetSDHost(dwIndex);
    
}

///////////////////////////////////////////////////////////////////////////////
//  GetSlotInfo - get slot information for all slots in the system
//  Input:  pBuffer - buffer to hold the slot information
//          Length  - length of the buffer
//  Output: 
//  Notes:  
//      returns the number of slots or zero on error
///////////////////////////////////////////////////////////////////////////////
DWORD CSDHostContainer::GetSlotInfo(PBYTE pslotInfoArray, DWORD Length, BOOL fInfoEx) 
{
    ULONG                   slotCount = 0;          // running slot count

    ((CStaticContainer *)this)->Lock();
    for (DWORD dwIndex = 0 ; dwIndex < m_dwArraySize; dwIndex++) {
        if (m_rgObjectArray[dwIndex]!=NULL) {
            slotCount += m_rgObjectArray[dwIndex]->GetSlotCount();
        }
    };
    if (pslotInfoArray!=NULL && Length!=0) { // We need file out the information.
        // check the buffer
        if (Length < slotCount * (fInfoEx?sizeof(BUS_DRIVER_SLOT_INFO_EX):sizeof(BUS_DRIVER_SLOT_INFO))) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: GetSlotInfo : insufficient buffer  \n")));    
            slotCount = 0;
        }
        else {
            ULONG curSlotCount = slotCount;
            PBYTE pCur = pslotInfoArray;
            for (DWORD dwHostIndex = 0; dwHostIndex < m_dwArraySize && curSlotCount ; dwHostIndex++) {
                if (m_rgObjectArray[dwHostIndex]!=NULL) {
                    for (DWORD dwSlotIndex= 0; dwSlotIndex< m_rgObjectArray[dwHostIndex]->GetSlotCount() && curSlotCount; dwSlotIndex++) {
                        CSDSlot * psdSlot = m_rgObjectArray[dwHostIndex]->GetSlot(dwSlotIndex);
                        WCHAR description[MAX_SD_DESCRIPTION_STRING];
                        VERIFY(SUCCEEDED(StringCchCopy(description,_countof(description), TEXT("Empty Slot"))));
                        if (psdSlot) {
                            BOOL fContinue = TRUE;
                            for (DWORD dwFuncIndex = 0; dwFuncIndex< SD_MAXIMUM_DEVICE_PER_SLOT && fContinue; dwFuncIndex++) {
                                CSDDevice *  pDevice = psdSlot->GetFunctionDevice(dwFuncIndex);
                                if (pDevice) {
                                    if (pDevice->IsDriverLoaded() && pDevice->IsDriverEnabled()) {
                                        VERIFY(SUCCEEDED(StringCchCopy(description,_countof(description), pDevice->GetClientName())));
                                        __try {
                                            if(fInfoEx){
                                                PBUS_DRIVER_SLOT_INFO_EX pCurSlotInfoEx = (PBUS_DRIVER_SLOT_INFO_EX)pCur;
                                                pCurSlotInfoEx->CardInterfaceEx = pDevice->GetCardInterface();
                                                pCurSlotInfoEx->CardPresent = TRUE;
                                                pCurSlotInfoEx->DeviceType = pDevice->GetDeviceType();
                                            }
                                            else{
                                                PBUS_DRIVER_SLOT_INFO pCurSlotInfo = (PBUS_DRIVER_SLOT_INFO)pCur;
                                                pCurSlotInfo->CardInterface = ConvertFromEx(pDevice->GetCardInterface());
                                                pCurSlotInfo->CardPresent = TRUE;
                                                pCurSlotInfo->DeviceType = pDevice->GetDeviceType();
                                            }
                                        }
                                        __except (SDProcessException(GetExceptionInformation())) {
                                            slotCount = 0;
                                            curSlotCount = 0;
                                        }
                                        fContinue = FALSE;
                                    }
                                    pDevice->DeRef();
                                }
                            }
                            if (fContinue) {
                                CSDDevice *  pDevice = psdSlot->GetFunctionDevice(0);// Master Device.
                                if (pDevice) {
                                    __try {
                                            if(fInfoEx){
                                                PBUS_DRIVER_SLOT_INFO_EX pCurSlotInfoEx = (PBUS_DRIVER_SLOT_INFO_EX)pCur;
                                                pCurSlotInfoEx->CardInterfaceEx = pDevice->GetCardInterface();
                                                pCurSlotInfoEx->CardPresent = TRUE;
                                                pCurSlotInfoEx->DeviceType = pDevice->GetDeviceType();
                                            }
                                            else{
                                                PBUS_DRIVER_SLOT_INFO pCurSlotInfo = (PBUS_DRIVER_SLOT_INFO)pCur;
                                                pCurSlotInfo->CardInterface = ConvertFromEx(pDevice->GetCardInterface());
                                                pCurSlotInfo->CardPresent = TRUE;
                                                pCurSlotInfo->DeviceType = pDevice->GetDeviceType();
                                            }
                                    }
                                    __except (SDProcessException(GetExceptionInformation())) {
                                        slotCount = 0;
                                        curSlotCount = 0;
                                    }
                                    
                                    pDevice->DeRef();
                                }
                            }
                        }
                        __try {
                            if(fInfoEx){
                                PBUS_DRIVER_SLOT_INFO_EX pCurSlotInfoEx = (PBUS_DRIVER_SLOT_INFO_EX)pCur;
                                VERIFY(SUCCEEDED(StringCchCopy(pCurSlotInfoEx->Description,_countof(pCurSlotInfoEx->Description),description)));
                                pCurSlotInfoEx->HostIndex = dwHostIndex;
                                pCurSlotInfoEx->SlotIndex = dwSlotIndex;
                            }
                            else{
                                PBUS_DRIVER_SLOT_INFO pCurSlotInfo = (PBUS_DRIVER_SLOT_INFO)pCur;
                                VERIFY(SUCCEEDED(StringCchCopy(pCurSlotInfo->Description,_countof(pCurSlotInfo->Description),description)));
                                pCurSlotInfo->HostIndex = dwHostIndex;
                                pCurSlotInfo->SlotIndex = dwSlotIndex;
                            }
                        }
                        __except (SDProcessException(GetExceptionInformation())) {
                            slotCount = 0;
                            curSlotCount = 0;
                        }
                        curSlotCount--;
                        if(fInfoEx)
                            pCur += sizeof(BUS_DRIVER_SLOT_INFO_EX);
                        else
                            pCur += sizeof(BUS_DRIVER_SLOT_INFO);
                    }
                }
            };
        }
    }
    ((CStaticContainer *)this)->Unlock();
    return slotCount;
}
BOOL CSDHostContainer::GetHCandSlotbySlotIndex(CSDHost ** ppHost,CSDSlot**ppSlot,DWORD dwSlotIndex )
{
    BOOL fRet = FALSE;
    if (ppHost && ppSlot) {
        ((CStaticContainer *)this)->Lock();
        for (DWORD dwIndex = 0 ; dwIndex < m_dwArraySize; dwIndex++) {
            if (m_rgObjectArray[dwIndex]!=NULL) {
                if (dwSlotIndex< m_rgObjectArray[dwIndex]->GetSlotCount()) {
                    *ppHost = m_rgObjectArray[dwIndex];
                    *ppSlot = (*ppHost)->GetSlot(dwSlotIndex);
                    (*ppHost)->AddRef();
                    fRet = TRUE;
                    break;
                }
                else {
                    dwSlotIndex -= m_rgObjectArray[dwIndex]->GetSlotCount();
                }
            }
        };
        ((CStaticContainer *)this)->Unlock();
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//  GetSlotPowerControlInfo - get slot power control information
//  Input:  slotIndexWanted - index of slot
//          
//  Output: pPCdata - data about power control at the slot
//  Notes:  
//      returns SD Api Error
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::GetSlotPowerControlInfo(DWORD slotIndexWanted, 
                                                    PSLOT_POWER_DATA pPCdata) 
{
    CSDHost *pHost = NULL;
    CSDSlot *pSlot = NULL;
    SD_API_STATUS status  = SD_API_STATUS_NO_SUCH_DEVICE;
    if (GetHCandSlotbySlotIndex(&pHost,&pSlot,slotIndexWanted) && pSlot!=NULL) {
        memset(pPCdata, 0, sizeof(SLOT_POWER_DATA));
        status = pSlot->CheckSlotReady();
        if (SD_API_SUCCESS(status)) {
            status = SD_API_STATUS_UNSUCCESSFUL;
            CSDDevice *  pDevice = pSlot->GetFunctionDevice(0);// Master Device.
            if (pDevice) {
                if (pDevice->GetCardInfo().SDIOInformation.pCommonInformation!=NULL) {
                    pPCdata->fCardSupportsPowerControl = pDevice->GetCardInfo().SDIOInformation.pCommonInformation->fCardSupportsPowerControl;  
                    pPCdata->fPowerControlEnabled      = pDevice->GetCardInfo().SDIOInformation.pCommonInformation->fPowerControlEnabled;  
                    pPCdata->CurrentDrawOfSlot = pSlot->GetSlotPower(); 
                    pPCdata->OperatingVoltage = pDevice->GetOperationVoltage();  
                    pPCdata->CurrentTupleVoltageRange = SD_MASK_FOR_33V_POWER_CONTROL_TUPLE;
                    pPCdata->NumberOfHighPowerTuples = 1;    // For SDIO 1.1 this is 1
                    pPCdata->SizeOfFunctionRecord = sizeof(FUNCTION_POWER_DATA);
                    pPCdata->Functions =(UCHAR) pSlot->GetNumOfFunctionDevice();
                    status = SD_API_STATUS_SUCCESS ;
                }
                pDevice->DeRef();
            }
        }
    }
    if (pHost)
        pHost->DeRef();

    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  GetFunctionPowerControlInfo - get function power control information
//  Input:  slotIndexWanted - index of slot
//           - index of slot
//          
//  Output: pFPCdata - data about power control at the function
//  Notes:  
//      returns SD Api Error
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::GetFunctionPowerControlInfo(DWORD slotIndexWanted, 
                                                        DWORD FunctionIndexWanted, 
                                                        PFUNCTION_POWER_DATA pFPCdata) 
{
    CSDHost *pHost = NULL;
    CSDSlot *pSlot = NULL;
    SD_API_STATUS status  = SD_API_STATUS_NO_SUCH_DEVICE;
    if (GetHCandSlotbySlotIndex(&pHost,&pSlot,slotIndexWanted) && pSlot!=NULL) {
        memset(pFPCdata, 0, sizeof(FUNCTION_POWER_DATA));
        status = pSlot->CheckSlotReady();
        if (SD_API_SUCCESS(status)) {
            status = SD_API_STATUS_UNSUCCESSFUL;
            CSDDevice *  pDevice = pSlot->GetFunctionDevice(FunctionIndexWanted);// Master Device.
            if (pDevice) {
                VERIFY(SUCCEEDED(StringCchCopy(pFPCdata->ClientName,_countof(pFPCdata->ClientName), pDevice->GetClientName() )));
                status = pDevice->GetFunctionPowerState(&pFPCdata->PowerState);
                pFPCdata->PowerTuples = pDevice->GetCardInfo().SDIOInformation.PowerDrawData;
                pDevice->DeRef();
            }
        }
    }
    if (pHost)
        pHost->DeRef();
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SetSlotPowerControl - set if a slot will enable power control on card insertion
//  Input:  slotIndexWanted - index of slot
//          fEnablePowerControl - flag set if power control is to be enabled or not
//          
//  Output: 
//  Notes:  
//      returns SD Api Error
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::SetSlotPowerControl(DWORD slotIndexWanted, BOOL  fEnablePowerControl) 
{
    CSDHost *pHost = NULL;
    CSDSlot *pSlot = NULL;
    SD_API_STATUS status  = SD_API_STATUS_NO_SUCH_DEVICE;
    if (GetHCandSlotbySlotIndex(&pHost,&pSlot,slotIndexWanted) && pSlot!=NULL) {
        pSlot->SetSlotPowerControl(fEnablePowerControl);
        status = SD_API_STATUS_SUCCESS ;
    }
    if (pHost)
        pHost->DeRef();
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SlotCardSelectDeselect - Select/Deselect the card in teh slot
//  Input:  SlotIndexWanted - index of slot
//          Event           - SlotDeselectRequest/SlotSelectRequest/SlotResetRequest
//  Output: 
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
BOOL CSDHostContainer::SlotCardSelectDeselect(DWORD SlotIndexWanted, SD_SLOT_EVENT Event) 
{
    BOOL    retStatus = FALSE;          // return status
    CSDHost *pHost = NULL;
    CSDSlot *pSlot = NULL;
    if (GetHCandSlotbySlotIndex(&pHost,&pSlot,SlotIndexWanted) && pSlot!=NULL) {
        retStatus= pSlot->SlotCardSelectDeselect(Event);
    }
    if (pHost)
        pHost->DeRef();

    return retStatus;
}

BOOL 
CSDHostContainer::SDC_IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   cbActualOut = (DWORD)-1; 

    switch (dwCode) {
      case IOCTL_SD_BUS_DRIVER_GET_SLOT_COUNT:
        if (dwLenOut != sizeof(DWORD)) {
            dwErr = ERROR_INVALID_PARAMETER;
        } else {
            *(PDWORD)pBufOut = GetSlotInfo(NULL, 0, FALSE);
            cbActualOut = sizeof(DWORD);
        }
        break;

      case IOCTL_SD_BUS_DRIVER_GET_SLOT_INFO:
        if (0 == GetSlotInfo(pBufOut, dwLenOut, FALSE)) {
            dwErr = ERROR_INVALID_PARAMETER;
        } else {
            cbActualOut = dwLenOut;
        }
        break;
      case IOCTL_SD_BUS_DRIVER_GET_SLOT_INFO_EX:
        if (0 == GetSlotInfo(pBufOut, dwLenOut, TRUE)) {
            dwErr = ERROR_INVALID_PARAMETER;
        } else {
            cbActualOut = dwLenOut;
        }
        break;
      case IOCTL_SD_BUS_DRIVER_GET_SLOT_POWER_CONTROL:
        if ((sizeof(DWORD) == dwLenIn) && pBufIn!=NULL &&
                (sizeof(SLOT_POWER_DATA) == dwLenOut) && pBufOut!=NULL) {
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            SLOT_POWER_DATA slotPowerData;
            cbActualOut = sizeof(slotPowerData);
            SD_API_STATUS status = GetSlotPowerControlInfo(dwSlotIndex,&slotPowerData);
            if (SD_API_SUCCESS(status)) {
                *(PSLOT_POWER_DATA)pBufOut = slotPowerData;
            }
            else {
                dwErr = ERROR_GEN_FAILURE;
            }
        }
        else if ( (sizeof(BUS_DRIVER_IN_SLOT_FUNCTION_POWER) == dwLenIn) && pBufIn!=NULL  &&
                  (sizeof(FUNCTION_POWER_DATA) == dwLenOut)&& pBufOut!=NULL ){
            BUS_DRIVER_IN_SLOT_FUNCTION_POWER FuncPowerIn = *(BUS_DRIVER_IN_SLOT_FUNCTION_POWER *)pBufIn ;
            FUNCTION_POWER_DATA FuncPowerData;
            cbActualOut = sizeof(FuncPowerData);
            SD_API_STATUS status = GetFunctionPowerControlInfo(
                FuncPowerIn.SlotIndex, FuncPowerIn.FunctionNumber, &FuncPowerData);
            if ( SD_API_SUCCESS(status)) {
                *(PFUNCTION_POWER_DATA)pBufOut = FuncPowerData;
            }
            else {
                dwErr = ERROR_GEN_FAILURE;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        break;
      case IOCTL_SD_BUS_DRIVER_DISABLE_SLOT_POWER_CONTROL:
        if(sizeof(DWORD) == dwLenIn && pBufIn!=NULL){
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            if (!SD_API_SUCCESS(SetSlotPowerControl(dwSlotIndex, FALSE))) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        break;
      case IOCTL_SD_BUS_DRIVER_ENABLE_SLOT_POWER_CONTROL:
        if (sizeof(DWORD) == dwLenIn && pBufIn!=NULL ) {
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            if (!SD_API_SUCCESS(SetSlotPowerControl(dwSlotIndex, TRUE))) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        break;
      case IOCTL_SD_BUS_DRIVER_GET_VERSION:
        if (dwLenOut != sizeof(DWORD) || pBufOut == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
        } 
        else {
            *(PDWORD)pBufOut = MAKELONG(CE_MINOR_VER, CE_MAJOR_VER);
            cbActualOut = sizeof(DWORD);
        }

        break;
      case IOCTL_SD_BUS_DRIVER_GET_BUS_VERSION:
        if (dwLenOut != sizeof(DWORD) || pBufOut == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
        } 
        else {
            *(PDWORD)pBufOut = MAKELONG(SD_BUS_INTERFACE_VERSION_MINOR, SD_BUS_INTERFACE_VERSION_MAJOR);
            cbActualOut = sizeof(DWORD);
        }

        break;


       case IOCTL_SD_BUS_DRIVER_SLOT_CARD_RESET:
        if (sizeof(DWORD) == dwLenIn && pBufIn) {
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            if (!SlotCardSelectDeselect(dwSlotIndex, SlotResetRequest)) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        break;

      case IOCTL_SD_BUS_DRIVER_SLOT_CARD_SELECT:
        if (sizeof(DWORD) == dwLenIn && pBufIn) {
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            if (!SlotCardSelectDeselect(dwSlotIndex, SlotSelectRequest)) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        break;
        
      case IOCTL_SD_BUS_DRIVER_SLOT_CARD_DESELECT:
        if (sizeof(DWORD) == dwLenIn && pBufIn) {
            DWORD dwSlotIndex = *(PDWORD)pBufIn;
            if (!SlotCardSelectDeselect(dwSlotIndex, SlotDeselectRequest)) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        break;
 
      default:
        dwErr = ERROR_INVALID_PARAMETER;
        break;
    }

    if (cbActualOut != -1 && pdwActualOut && dwErr == ERROR_SUCCESS) {
        *pdwActualOut = cbActualOut;
    }

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDBusDriver: -SDC_IOControl 0x%X\n"), dwErr));
    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        return FALSE;
    }
    else
        return TRUE;
}

CSDDevice * CSDHostContainer::GetDeviceByDevice(HANDLE hDevice)
{
    SDBUS_DEVICE_HANDLE deviceHandle;
    deviceHandle.hValue = hDevice;
    
    CSDDevice * pReturn = NULL;
    CSDHost *pHost = GetSDHost(deviceHandle.bit.sdBusIndex);
    if (hDevice!=NULL && deviceHandle.bit.sdF == 0xf) {
        if (pHost) {
            CSDSlot * pSlot = pHost->GetSlot(deviceHandle.bit.sdSlotIndex);
            if (pSlot) {
                pReturn = pSlot->GetFunctionDevice(deviceHandle.bit.sdFunctionIndex,deviceHandle.bit.sdRandamNumber);
                
            }
            pHost->DeRef();
        }
    }
    return pReturn;
}
CSDDevice * CSDHostContainer::GetDeviceByRequest(HANDLE hRequest)
{
    SDBUS_REQUEST_HANDLE requestHandle;
    requestHandle.hValue = hRequest;
    
    CSDDevice * pReturn = NULL;
    CSDHost *pHost = GetSDHost(requestHandle.bit.sdBusIndex);
    if (hRequest!=NULL && requestHandle.bit.sd1f == 0x1f) {
        if (pHost) {
            CSDSlot * pSlot = pHost->GetSlot(requestHandle.bit.sdSlotIndex);
            if (pSlot) {
                pReturn = pSlot->GetFunctionDevice(requestHandle.bit.sdFunctionIndex);                
            }
            pHost->DeRef();
        }
    }
    else
        ASSERT(FALSE);
    return pReturn;
}
BOOL 
CSDHostContainer::IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut,
                      PDWORD pdwActualOut)
{
    BOOL fRet = FALSE;

    switch (dwCode) {
    case IOCTL_BUS_TRANSLATE_BUS_ADDRESS:        
    case IOCTL_BUS_TRANSLATE_SYSTEM_ADDRESS:
    case IOCTL_BUS_ACTIVATE_CHILD:
    case IOCTL_BUS_DEACTIVATE_CHILD:
    case IOCTL_BUS_IS_CHILD_REMOVED:
    case IOCTL_BUS_GET_CONFIGURE_DATA:        
    case IOCTL_BUS_SET_CONFIGURE_DATA:
        SetLastError(ERROR_NOT_SUPPORTED);
        fRet = FALSE;
        break;
    case IOCTL_BUS_SD_SET_CARD_FEATURE:
        if (pBufIn && dwLenIn>=sizeof(IO_SD_SET_CARD_FEATURE) ) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_SET_CARD_FEATURE SdSetCardFeature = *(PIO_SD_SET_CARD_FEATURE) pBufIn;
            CSDDevice * pDevice = GetDeviceByDevice (SdSetCardFeature.hDevice);
            if (pDevice) {
                MarshalledBuffer_t UserBuffer((PVOID) SdSetCardFeature.pInBuf,SdSetCardFeature.nInBufSize,ARG_I_PTR, FALSE, FALSE);
                status = pDevice->SDSetCardFeature_I(SdSetCardFeature.CardFeature,UserBuffer.ptr(),SdSetCardFeature.nInBufSize) ;
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_READ_WRITE_REGISTERS_DIRECT: 
        if (pBufIn && dwLenIn>= sizeof(IO_SD_CARD_INFO_QUERY)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_READ_WRITE_REGISTERS_DIRECT ReadWriteRegisterDirect = *(PIO_SD_READ_WRITE_REGISTERS_DIRECT) pBufIn;
            CSDDevice * pDevice = GetDeviceByDevice(ReadWriteRegisterDirect.hDevice);
            if (pDevice) {
                DWORD dwArg = (ReadWriteRegisterDirect.ReadWrite==SD_IO_WRITE? ARG_I_PTR: ARG_O_PTR);
                if (ReadWriteRegisterDirect.ReadAfterWrite)
                    dwArg|= ARG_O_PTR ;
                MarshalledBuffer_t UserBuffer((PVOID)ReadWriteRegisterDirect.pBuffer,ReadWriteRegisterDirect.Length,dwArg,FALSE,FALSE);
                DEBUGMSG(SDCARD_ZONE_ERROR && (ReadWriteRegisterDirect.pBuffer!=NULL && UserBuffer.ptr()==0 ), 
                    (TEXT("IOCTL_BUS_SD_READ_WRITE_REGISTERS_DIRECT: Failed to marshal point 0x%x \n"),ReadWriteRegisterDirect.pBuffer));
                status = pDevice->SDReadWriteRegistersDirect_I(ReadWriteRegisterDirect.ReadWrite,
                            ReadWriteRegisterDirect.Address,ReadWriteRegisterDirect.ReadAfterWrite,
                            (PUCHAR)UserBuffer.ptr(),ReadWriteRegisterDirect.Length);
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;         
    case IOCTL_BUS_SD_REGISTER_CLIENT:
        if (pBufIn && dwLenIn>=sizeof(IO_SD_REGISTERCLIENTDEVICE)) {
            IO_SD_REGISTERCLIENTDEVICE RegisterClientDriver = *(PIO_SD_REGISTERCLIENTDEVICE) pBufIn;
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            CSDDevice * pDevice = GetDeviceByDevice(RegisterClientDriver.hDevice);
            if (pDevice) {
                status = pDevice->RegisterClient(RegisterClientDriver.hCallbackAPI,
                    RegisterClientDriver.pDeviceContext, &RegisterClientDriver.sdClientRegistrationInfo);
                if (status != SD_API_STATUS_SUCCESS) {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDRegisterClient: Failed to register client 0x%08X \n"),status));
                }
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
            
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_SYNCHRONOUS_BUS_REQUEST:
        if (pBufIn && dwLenIn>= sizeof(IO_SD_SYNCHRONOUS_BUS_REQUEST)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_SYNCHRONOUS_BUS_REQUEST SdSyncBusRequest = *(PIO_SD_SYNCHRONOUS_BUS_REQUEST)pBufIn;
            SD_COMMAND_RESPONSE sdResponse;
            // Mapping Embedded pointer.
            CSDDevice * pDevice = GetDeviceByDevice(SdSyncBusRequest.hDevice);
            if (pDevice) {
                MarshalledBuffer_t UserBuffer((PVOID)SdSyncBusRequest.pBuffer,
                    SdSyncBusRequest.BlockSize* SdSyncBusRequest.NumBlocks,
                    SdSyncBusRequest.TransferClass==SD_READ ? ARG_O_PTR: ARG_I_PTR, FALSE, TRUE); // no duplicate and Async(because it could call BusRequest , read & write.            
                DEBUGMSG(SDCARD_ZONE_ERROR && (SdSyncBusRequest.pBuffer!=NULL && UserBuffer.ptr()==0 ), 
                    (TEXT("IOCTL_BUS_SD_SYNCHRONOUS_BUS_REQUEST: Failed to marshal point 0x%x \n"),SdSyncBusRequest.pBuffer));
                status =  pDevice->SDSynchronousBusRequest_I(
                    SdSyncBusRequest.Command,SdSyncBusRequest.Argument,
                    SdSyncBusRequest.TransferClass,SdSyncBusRequest.ResponseType,&sdResponse,
                    SdSyncBusRequest.NumBlocks,SdSyncBusRequest.BlockSize,(PUCHAR)UserBuffer.ptr(),
                    SdSyncBusRequest.Flags &~SD_BUS_REQUEST_PHYS_BUFFER, 0, NULL);
                if (pBufOut && dwLenOut>= sizeof(SD_COMMAND_RESPONSE)) { // Response can be optional.
                    *(PSD_COMMAND_RESPONSE)pBufOut = sdResponse ;
                }
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_BUS_REQUEST:
        if (pBufIn && dwLenIn>= sizeof(IO_SD_BUS_REQUEST) && pBufOut && dwLenOut>= sizeof(HANDLE)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_BUS_REQUEST SdBusRequest = *(PIO_SD_BUS_REQUEST)pBufIn;
            HANDLE hBusRequest = NULL ;
            // Mapping Embedded pointer.
            CSDDevice * pDevice = GetDeviceByDevice(SdBusRequest.hDevice);
            if (pDevice) {
                MarshalledBuffer_t UserBuffer((PVOID)SdBusRequest.pBuffer,
                    SdBusRequest.BlockSize* SdBusRequest.NumBlocks,
                    SdBusRequest.TransferClass==SD_READ ? ARG_O_PTR: ARG_I_PTR, FALSE,FALSE); // no duplicate and no Async , read & write.
                DEBUGMSG(SDCARD_ZONE_ERROR && (SdBusRequest.pBuffer!=NULL && UserBuffer.ptr()==0 ), 
                    (TEXT("IOCTL_BUS_SD_BUS_REQUEST: Failed to marshal point 0x%x \n"),SdBusRequest.pBuffer));
                ASSERT(pDevice->GetCallbackHandle()!= NULL);
                status =  pDevice->SDBusRequest_I(SdBusRequest.Command,SdBusRequest.Argument,
                    SdBusRequest.TransferClass,SdBusRequest.ResponseType,
                    SdBusRequest.NumBlocks,SdBusRequest.BlockSize,(PUCHAR)UserBuffer.ptr(),
                    SdBusRequest.ceDriverCallbackParam.pRequestCallback, SdBusRequest.ceDriverCallbackParam.dwRequestParam,
                    &hBusRequest,SdBusRequest.Flags & ~SD_BUS_REQUEST_PHYS_BUFFER, 0, NULL);
                pDevice->DeRef();
                * (HANDLE *)pBufOut  = hBusRequest ;
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_SYNCHRONOUS_BUS_REQUEST_EX:
        if (pBufIn && dwLenIn>= sizeof(IO_SD_SYNCHRONOUS_BUS_REQUEST_EX)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_SYNCHRONOUS_BUS_REQUEST_EX SdSyncBusRequest = *(PIO_SD_SYNCHRONOUS_BUS_REQUEST_EX)pBufIn;
            SD_COMMAND_RESPONSE sdResponse;
            // Mapping Embedded pointer.
            CSDDevice * pDevice = GetDeviceByDevice(SdSyncBusRequest.hDevice);
            if (pDevice) {
                MarshalledBuffer_t UserBuffer((PVOID)SdSyncBusRequest.pBuffer,
                    SdSyncBusRequest.BlockSize* SdSyncBusRequest.NumBlocks,
                    SdSyncBusRequest.TransferClass==SD_READ ? ARG_O_PTR: ARG_I_PTR, FALSE, TRUE); // no duplicate and Async(because it could call BusRequest , read & write.            
                MarshalledBuffer_t UserDmaBuffer (SdSyncBusRequest.pPhysBuffList,SdSyncBusRequest.cbSize,ARG_I_PTR,FALSE,FALSE);
                DEBUGMSG(SDCARD_ZONE_ERROR && (SdSyncBusRequest.pBuffer!=NULL && UserBuffer.ptr()==0 ), 
                    (TEXT("IOCTL_BUS_SD_SYNCHRONOUS_BUS_REQUEST: Failed to marshal point 0x%x \n"),SdSyncBusRequest.pBuffer));
                status =  pDevice->SDSynchronousBusRequest_I(
                    SdSyncBusRequest.Command,SdSyncBusRequest.Argument,
                    SdSyncBusRequest.TransferClass,SdSyncBusRequest.ResponseType,&sdResponse,
                    SdSyncBusRequest.NumBlocks,SdSyncBusRequest.BlockSize,(PUCHAR)UserBuffer.ptr(),
                    SdSyncBusRequest.Flags,SdSyncBusRequest.cbSize, (PPHYS_BUFF_LIST)UserDmaBuffer.ptr());
                if (pBufOut && dwLenOut>= sizeof(SD_COMMAND_RESPONSE)) { // Response can be optional.
                    *(PSD_COMMAND_RESPONSE)pBufOut = sdResponse ;
                }
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_BUS_REQUEST_EX:
        if (pBufIn && dwLenIn>= sizeof(IO_SD_BUS_REQUEST_EX) && pBufOut && dwLenOut>= sizeof(HANDLE)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_BUS_REQUEST_EX SdBusRequest = *(PIO_SD_BUS_REQUEST_EX)pBufIn;
            HANDLE hBusRequest = NULL ;
            // Mapping Embedded pointer.
            CSDDevice * pDevice = GetDeviceByDevice(SdBusRequest.hDevice);
            if (pDevice) {
                MarshalledBuffer_t UserBuffer((PVOID)SdBusRequest.pBuffer,
                    SdBusRequest.BlockSize* SdBusRequest.NumBlocks,
                    SdBusRequest.TransferClass==SD_READ ? ARG_O_PTR: ARG_I_PTR, FALSE,FALSE); // no duplicate and no Async , read & write.
                MarshalledBuffer_t UserDmaBuffer (SdBusRequest.pPhysBuffList,SdBusRequest.cbSize,ARG_I_PTR,FALSE,FALSE);
                    
                DEBUGMSG(SDCARD_ZONE_ERROR && (SdBusRequest.pBuffer!=NULL && UserBuffer.ptr()==0 ), 
                    (TEXT("IOCTL_BUS_SD_BUS_REQUEST: Failed to marshal point 0x%x \n"),SdBusRequest.pBuffer));
                ASSERT(pDevice->GetCallbackHandle()!= NULL);
                status =  pDevice->SDBusRequest_I(SdBusRequest.Command,SdBusRequest.Argument,
                    SdBusRequest.TransferClass,SdBusRequest.ResponseType,
                    SdBusRequest.NumBlocks,SdBusRequest.BlockSize,(PUCHAR)UserBuffer.ptr(),
                    SdBusRequest.ceDriverCallbackParam.pRequestCallback, SdBusRequest.ceDriverCallbackParam.dwRequestParam,
                    &hBusRequest,SdBusRequest.Flags,SdBusRequest.cbSize,(PPHYS_BUFF_LIST)UserDmaBuffer.ptr() );
                pDevice->DeRef();
                * (HANDLE *)pBufOut  = hBusRequest ;
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_FREE_BUS_REQUEST: 
        if (pBufIn && dwLenIn>= sizeof(HANDLE)) {
            HANDLE hRequest = *(HANDLE *)pBufIn;
            CSDDevice * pDevice =  GetDeviceByRequest(hRequest);
            if (pDevice) {
                pDevice->SDFreeBusRequest_I(hRequest );
                pDevice->DeRef();
                fRet = TRUE;
            }
        }
        break;
    case IOCTL_BUS_SD_REQUEST_RESPONSE: 
        if (pBufIn && dwLenIn>= sizeof(HANDLE) && pBufOut && dwLenOut>=sizeof(SD_COMMAND_RESPONSE)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            HANDLE hRequest = *(HANDLE *)pBufIn;
            SD_COMMAND_RESPONSE Response;
            CSDDevice * pDevice =  GetDeviceByRequest(hRequest);
            if (pDevice) {
                status = pDevice->SDBusRequestResponse_I(hRequest,&Response );
                pDevice->DeRef();
                if (SD_API_SUCCESS(status)) {
                    *(PSD_COMMAND_RESPONSE)pBufOut = Response;
                }
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_CARD_INFO_QUERY:
        if (pBufIn && dwLenIn>= sizeof(IO_SD_CARD_INFO_QUERY)) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            PIO_SD_CARD_INFO_QUERY pSdCardInfoQuery = (PIO_SD_CARD_INFO_QUERY)pBufIn;
            CSDDevice * pDevice =  GetDeviceByDevice(pSdCardInfoQuery->hDevice);
            if (pDevice) {
                __try {
                    status = pDevice->SDCardInfoQuery_I(pSdCardInfoQuery->InfoType,pBufOut,dwLenOut);
                }
                __except(SDProcessException(GetExceptionInformation())) {
                    status = SD_API_STATUS_ACCESS_VIOLATION; 
                }
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;            
    case IOCTL_BUS_SD_CANCEL_BUS_REQUEST:
        if (pBufIn && dwLenIn>=sizeof(HANDLE)) {
            HANDLE hRequest = *(HANDLE *)pBufIn;
            CSDDevice * pDevice = GetDeviceByRequest(hRequest);
            if (pDevice) {
                fRet = pDevice->SDCancelBusRequest_I(hRequest);
            }
            else 
                SetLastError(ERROR_INVALID_PARAMETER);
        }
        else
            SetLastError(ERROR_INVALID_PARAMETER);
        break;
    case IOCTL_BUS_SD_GET_TUPLE:
        if (pBufIn && dwLenIn>=sizeof(IO_BUS_SD_GET_TUPLE) ) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_BUS_SD_GET_TUPLE BusSdGetTuple = *(PIO_BUS_SD_GET_TUPLE)pBufIn;
            DWORD dwBufferSize = dwLenOut;
            CSDDevice * pDevice = GetDeviceByDevice (BusSdGetTuple.hDevice);
            if (pDevice) {
                __try {
                    status = pDevice->SDGetTuple_I(BusSdGetTuple.TupleCode, pBufOut,&dwBufferSize, BusSdGetTuple.CommonCIS);
                }
                __except(SDProcessException(GetExceptionInformation())) {
                    status = SD_API_STATUS_ACCESS_VIOLATION; 
                }
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
            if (fRet && pdwActualOut) {
                *pdwActualOut = dwBufferSize;
            }
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;         
    case IOCTL_BUS_SD_IO_CONNECT_INTERRUPT:
        if (pBufIn && dwLenIn>=sizeof(IO_SD_IO_CONNECT_INTERRUPT) ) {
            SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER; 
            IO_SD_IO_CONNECT_INTERRUPT SdConnectInterrupt = *(PIO_SD_IO_CONNECT_INTERRUPT)pBufIn;
            CSDDevice * pDevice = GetDeviceByDevice (SdConnectInterrupt.hDevice);
            if (pDevice) {
                status = pDevice->SDIOConnectDisconnectInterrupt(SdConnectInterrupt.sdInterruptCallback.pSdInterruptCallback,TRUE);
                pDevice->DeRef();
            }
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;         
    case IOCTL_BUS_SD_IO_DISCONNECT_INTERRUPT:
        if (pBufIn && dwLenIn>=sizeof(SD_DEVICE_HANDLE) ) {
            HANDLE hDevice = *(HANDLE *)pBufIn;
            CSDDevice * pDevice = GetDeviceByDevice (hDevice);
            if (pDevice) {
                pDevice->SDIOConnectDisconnectInterrupt(NULL,FALSE);
                pDevice->DeRef();
            }
            fRet = TRUE;
        }
        else 
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;         
    case IOCTL_BUS_SD_GETCLIENT_FUNCTION:
        if (pBufOut && dwLenOut>=sizeof(SDCARD_API_FUNCTIONS)) {
            SD_API_STATUS status= SDGetClientFunctions((SDCARD_API_FUNCTIONS *)pBufOut);
            SetLastError(status);
            fRet = SD_API_SUCCESS(status);
            if (fRet && pdwActualOut) {
                *pdwActualOut = sizeof(SDCARD_API_FUNCTIONS) ;
            }
            
        }
        else
            SetLastError((DWORD)SD_API_STATUS_INVALID_PARAMETER);
        break;
    default:
        fRet = DefaultBusDriver::IOControl(dwCode, pBufIn, dwLenIn, pBufOut, 
            dwLenOut, pdwActualOut);
        break;
    };

    return fRet;
}



PVOID CSDHostContainer::AllocateBusRequest(size_t stSize)
{
    PVOID pReturn = NULL;
    if (g_pSdContainer != NULL ) {
        pReturn  = g_pSdContainer->AllocateBusRequestImp(stSize);
#ifdef DEBUG
        if (pReturn){
            memset(pReturn,0xcc,stSize);
        }
#endif
    }
    return pReturn;
}
void  CSDHostContainer::FreeBusRequestImp(CSDBusRequest *pBusRequest)
{
    if (pBusRequest) {
        ((CStaticContainer *)this)->Lock();
        PFREE_BUS_REQUEST_SPACE pBusRequestFreeSpace = (PFREE_BUS_REQUEST_SPACE)pBusRequest;
        pBusRequestFreeSpace -- ; // move back to the header.
        if (pBusRequestFreeSpace->dwFreeSpaceTag== BUS_REQUEST_FREE_SPACE_TAG) {
            if (pBusRequestFreeSpace->dwSpaceSize != m_dwMinSize) { // Size has chnaged. So delete it directory.
                DEBUGMSG(SDCARD_ZONE_WARN,(L"free space because size changed.%x ",pBusRequestFreeSpace) );
                free(pBusRequestFreeSpace);
            }
            else {
                pBusRequestFreeSpace->pNextFreeTransfer = m_pFreeBusRequestSpace;
                m_pFreeBusRequestSpace = pBusRequestFreeSpace;
            }
        }
        else {// trashed point. Do know what to do except free it direct free it
            ASSERT(FALSE);
            DEBUGMSG(SDCARD_ZONE_ERROR,(L"DeleteTransfer detect garbage pointer %x ",pBusRequestFreeSpace) );
            free(pBusRequestFreeSpace);
        }
        ((CStaticContainer *)this)->Unlock();
    }
}
void  CSDHostContainer::FreeBusRequest(CSDBusRequest *pBusRequest)
{
    if (pBusRequest) {
        if (g_pSdContainer != NULL ) {
            g_pSdContainer->FreeBusRequestImp(pBusRequest);
        }
        else { // this is best we can do.
            PFREE_BUS_REQUEST_SPACE pBusRequestFreeSpace = (PFREE_BUS_REQUEST_SPACE)pBusRequest;
            pBusRequestFreeSpace -- ; // move back to the header.
            free(pBusRequestFreeSpace);
        }
    }
    
}
void  CSDHostContainer::DeleteAllTransferSpace()
{
    ((CStaticContainer *)this)->Lock();
    while (m_pFreeBusRequestSpace) {
        PFREE_BUS_REQUEST_SPACE pRequestNextFreeSpace = m_pFreeBusRequestSpace->pNextFreeTransfer;
        free(m_pFreeBusRequestSpace);
        m_pFreeBusRequestSpace = pRequestNextFreeSpace;
    }
    ((CStaticContainer *)this)->Unlock();
}

PVOID CSDHostContainer::DriverInit(LPCTSTR pszActiveKey)
{
    if (g_pSdContainer == NULL ) {
        CSDHostContainer * pHostContainer = new CSDHostContainer(pszActiveKey);
        if (pHostContainer!=NULL && !pHostContainer->Init()) { // Failed during inti.
            delete pHostContainer;
            pHostContainer = NULL;
        }
        g_pSdContainer = pHostContainer;
        return ((PVOID)g_pSdContainer);
    }
    else {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Init called more than once\n")));
        return NULL;
    }
}
VOID CSDHostContainer::DriverDeInit(PVOID pContent)
{
    if (g_pSdContainer && g_pSdContainer == pContent ) {
        delete g_pSdContainer;
        g_pSdContainer = NULL;
    }
    else 
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: DeInit called with wrong content\n")));
};
DWORD CSDHostContainer::RegValueDWORD(LPCTSTR lpRegName,DWORD dwDefault)
{
    DWORD dwReturn = dwDefault;
    if (g_pSdContainer ) {
        if (!g_pSdContainer->GetDeviceKey().GetRegValue(lpRegName,(LPBYTE)&dwReturn,  sizeof(dwReturn))) {
            dwReturn = dwDefault;
        }
    }
    return dwReturn;
}

LPCTSTR CSDHostContainer::GetSubBusNamePrefix()
{
    if (m_szSubBusNamePrefix[0])
        return m_szSubBusNamePrefix;
    else
        return SD_SUB_BUSNAME_DEFAULT;
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDAllocateContext - Allocate an HCD Context
//
// Input: NumberOfSlots - Number of slots
// Output:
//        ppExternalHCContext - caller supplied storage for the host context
// Return: SD_API_STATUS 
// Notes:
//        Host controller drivers must allocate an HC context for the bus driver.
//        When a host controller driver is unloaded it must free this context
//        returns SD_API_STATUS
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::SDHCDAllocateContext__X(DWORD NumberOfSlots,  
                                      PSDCARD_HC_CONTEXT *ppExternalHCContext)
{
    // check parameters, at least one slot must be allocated
    if ((ppExternalHCContext == NULL) || (NumberOfSlots == 0)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDAllocateContext: invalid parameter \n")));
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    CSDHost * pNewHost = new CSDHost(NumberOfSlots);
    if (pNewHost && pNewHost->Init() && InsertSDHost(pNewHost)) {
        *ppExternalHCContext = pNewHost;
        return SD_API_STATUS_SUCCESS ;
    }
    else { 
        if (pNewHost)
            delete pNewHost;
    }
    return SD_API_STATUS_INVALID_PARAMETER;

};
///////////////////////////////////////////////////////////////////////////////
// SDHCDRegisterHostController - Register a host controller with the bus driver
//
// Input: pExternalHCContext - Allocated Host controller context
//
// Output:
// Return: SD_API_STATUS
// Notes:
//       
//      the caller must allocate a host controller context and 
//      initialize the various parameters
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::SDHCDRegisterHostController__X(PSDCARD_HC_CONTEXT pExternalHCContext)
{
    CSDHost * pHost = GetSDHost((CSDHost *)pExternalHCContext);
    SD_API_STATUS status = SD_API_STATUS_UNSUCCESSFUL ;
    if (pHost) {
        if (pExternalHCContext->dwVersion > SDCARD_HC_BUS_INTERFACE_VERSION)  {
            DEBUGMSG(SDCARD_ZONE_ERROR, 
                (TEXT("SDBusDriver: Host controller interface version (%x) does not match bus driver (%x)\n"),
                pExternalHCContext->dwVersion, SDCARD_HC_BUS_INTERFACE_VERSION));
        }
        else {
            VERIFY(pHost->Attach());
            status = pHost->InitHandler();
        }
        pHost->DeRef();
    }
    ASSERT(SD_API_SUCCESS(status));
    return status;
}

///////////////////////////////////////////////////////////////////////////////
// SDHCDDeregisterHostController - Deregister a host controller 
//
// Input: pExternalHCContext - Host controller context that was previously registered
//        
// Output:
// Return: SD_API_STATUS 
// Notes:       
//      A host controller must call this api before deleting the HC context
//      
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHostContainer::SDHCDDeregisterHostController__X(PSDCARD_HC_CONTEXT pExternalHCContext)
{
    SD_API_STATUS status = SD_API_STATUS_UNSUCCESSFUL;
    CSDHost * psdHost = GetSDHost((CSDHost *)pExternalHCContext);
    if (psdHost) {
        VERIFY(RemoveSDHostBy(psdHost->GetIndex()));
        VERIFY(psdHost->Detach());
        status = psdHost->DeinitHandler();
        psdHost->DeRef();
    }
    else {
        ASSERT(FALSE);
    }
    return status;
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDDeleteContext - Delete an HCD context
//
// Input: pExternalHCContext - Host Context to delete
// Output:
// Return: 
// Notes:
///////////////////////////////////////////////////////////////////////////////
VOID CSDHostContainer::SDHCDDeleteContext__X(PSDCARD_HC_CONTEXT pExternalHCContext)
{
    CSDHost * psdHost  = GetSDHost((CSDHost *)pExternalHCContext);
    if (psdHost) {
        ASSERT(FALSE); // This should not happens because it has been deleted from container.
        VERIFY(RemoveSDHostBy(psdHost->GetIndex()));
        psdHost->DeRef();
    }
}

///////////////////////////////////////////////////////////////////////////////
// SDHCDIndicateSlotStateChange - indicate a change in the SD Slot 
//
// Input: pExternalHCContext - Host controller context that was previously registered
//        SlotNumber - Slot Number
//        Event      - new event
// Output:
// Return: SD_API_STATUS 
// Notes:       
//      A host controller driver calls this api when the slot changes state (i.e.
//      device insertion/deletion).
///////////////////////////////////////////////////////////////////////////////
VOID CSDHostContainer::SDHCDIndicateSlotStateChange__X(PSDCARD_HC_CONTEXT pExternalHCContext, 
                                     DWORD              SlotNumber,
                                     SD_SLOT_EVENT      Event) 
{
    CSDHost * psdHost = GetSDHost((CSDHost *)pExternalHCContext);
    if (psdHost ) {
        if ((CSDHost *) pExternalHCContext == psdHost ) {
            CSDSlot * psdSlot = psdHost->GetSlot(SlotNumber) ;
            if (psdHost->IsAttached() && psdSlot) {
                psdSlot->SlotStateChange(Event);
            }
        }
        else {
            ASSERT(FALSE);
        }
        psdHost->DeRef();
    }
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDPowerUpDown - Indicate a power up/down event
//                             
// Input:   pHCContext - host controller context   
//          PowerUp    - set to TRUE if powering up
//          SlotKeepPower - set to TRUE if the slot maintains power to the
//                          slot during power down
// Output:
// Return:       
// Notes:   This function notifies the bus driver of a power up/down event.
//          The host controller driver can indicate to the bus driver that power
//          can be maintained for the slot.  If power is removed, the bus driver
//          will unload the device driver on the next power up event.
//          This function can only be called from the host controller's PowerOn
//          and PowerOff function.
///////////////////////////////////////////////////////////////////////////////
VOID CSDHostContainer::SDHCDPowerUpDown__X(PSDCARD_HC_CONTEXT  pExternalHCContext, 
                         BOOL                PowerUp, 
                         BOOL                SlotKeepPower,
                         DWORD               SlotIndex)
{
    
    CSDHost * psdHost  = GetSDHost((CSDHost *)pExternalHCContext) ;
    if (psdHost ) {
        CSDSlot * psdSlot = psdHost->GetSlot(SlotIndex) ;
        if (psdHost->IsAttached() && psdSlot) {
            if (PowerUp){
                psdSlot->PowerUp();        
            } else {
                psdSlot->PowerDown(SlotKeepPower);
            }
        }
        psdHost->DeRef();
    }
    else {
        RETAILMSG(1, (TEXT("SDBusDriver: Passed invalid SDCARD_HC_CONTEXT \n")));
        DEBUGCHK(FALSE);
        return;
    }
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDIndicateBusRequestComplete - indicate to the bus driver that
//                                   the request is complete
// Input:   pExternalHCContext - host controller context
//          pRequest   - the request to indicate
//          Status     - the ending status of the request
// Output:
// Return:  
// Notes:       
//
//      
///////////////////////////////////////////////////////////////////////////////
VOID CSDHostContainer::SDHCDIndicateBusRequestComplete__X(PSDCARD_HC_CONTEXT pExternalHCContext,
                                        PSD_BUS_REQUEST    pRequest,
                                        SD_API_STATUS      Status)
{
    CSDBusRequest * pBusRequest = (CSDBusRequest *) pRequest;
    DWORD   dwSDHCIndex = MAXDWORD;
    DWORD   dwSlotIndex = MAXDWORD;
    BOOL    fResult = FALSE;
    if (pBusRequest) {
        __try {
            dwSDHCIndex = pBusRequest->GetDevice().GetSlot().GetHost().GetIndex();
            dwSlotIndex = pBusRequest->GetDevice().GetSlot().GetSlotIndex();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)  {
        }
    }
    CSDHost * psdHost  = GetSDHost( dwSDHCIndex ) ;
    if (psdHost && (PSDCARD_HC_CONTEXT)psdHost == pExternalHCContext) {
        CSDSlot * psdSlot = psdHost->GetSlot(dwSlotIndex) ;
        if (psdSlot) {
            fResult = psdSlot->CompleteRequestFromHC(pBusRequest, Status);
        }
    }
    ASSERT(fResult);
    if (psdHost ) {
        psdHost->DeRef();
    }
    
    
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDGetAndLockCurrentRequest - get the current request in the host controller
//                                 slot and lock it to keep it from being cancelable
// Input:   pExternalHCContext - host controller context   
//          SlotIndex  - the slot number 
// Output:
// Return: current bus request       
// Notes:
//          This function retrieves the current request and marks the
//          request as NON-cancelable.  To return the request back to the
//          cancelable state the caller must call SDHCDUnlockRequest()     
//          This function returns the current request which can be NULL if 
//          the request was previously marked cancelable and the host controller's
//          cancelIo Handler completed the request 
///////////////////////////////////////////////////////////////////////////////
PSD_BUS_REQUEST CSDHostContainer::SDHCDGetAndLockCurrentRequest__X(PSDCARD_HC_CONTEXT pExternalHCContext, DWORD SlotIndex)
{
    DWORD   dwSDHCIndex = MAXDWORD;
    CSDBusRequest * pBusRequest = NULL;
    if (pExternalHCContext) {
        __try {
            dwSDHCIndex =( (CSDHost *) pExternalHCContext)->GetIndex();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)  {
        }
    }
    CSDHost * psdHost  = GetSDHost( dwSDHCIndex ) ;
    if (psdHost && (PSDCARD_HC_CONTEXT)psdHost == pExternalHCContext) {
        CSDSlot * psdSlot = psdHost->GetSlot(SlotIndex) ;
        if (psdSlot) {
            pBusRequest = psdSlot->SDHCGetAndLockCurrentRequest_I();
        }
    }
    //ASSERT(pBusRequest);
    if (psdHost ) {
        psdHost->DeRef();
    }
    return (pBusRequest!=NULL? (PSD_BUS_REQUEST)pBusRequest: NULL);
}
///////////////////////////////////////////////////////////////////////////////
// SDHCDUnlockRequest - Unlock a request that was previous locked
//                             
// Input:   pHCContext - host controller context   
//          pRequest  - the request to lock
// Output:
// Return:      
// Notes:   This function unlocks the request that was returned from the
//          function SDHCDGetAndLockCurrentRequest()
//          
//          This request can now be cancelled from any thread context
///////////////////////////////////////////////////////////////////////////////
VOID CSDHostContainer::SDHCDUnlockRequest__X(PSDCARD_HC_CONTEXT  pExternalHCContext,PSD_BUS_REQUEST pRequest)
{
    DWORD   dwSDHCIndex = MAXDWORD;
    DWORD   dwSlotIndex = MAXDWORD;
    BOOL    fSuccess = FALSE;
    if (pExternalHCContext) {
        __try {
            dwSDHCIndex =( (CSDHost *) pExternalHCContext)->GetIndex();
            dwSlotIndex = ((CSDBusRequest *)pRequest)->GetDevice().GetSlot().GetSlotIndex();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)  {
        }
    }
    CSDHost * psdHost  = GetSDHost( dwSDHCIndex ) ;
    if (psdHost && (PSDCARD_HC_CONTEXT)psdHost == pExternalHCContext) {
        CSDSlot * psdSlot = psdHost->GetSlot(dwSlotIndex) ;
        if (psdSlot) {
            psdSlot->SDHCDUnlockRequest_I(pRequest) ;
            fSuccess = TRUE;
        }
    }
    if (psdHost ) {
        psdHost->DeRef();
    }
    ASSERT(fSuccess);
}



///////////////////////////////////////////////////////////////////////////////
//  SDRegisterClient__X - register a client device
//  Input:  hDevice         - device handle
//          pDeviceContext  - device specific context allocated by driver
//          pInfo           - registration information
//          
//  Output: 
//  Notes:  
//      returns SD_API_STATUS_SUCCESS
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDRegisterClient__X(HANDLE  hDevice,PVOID pDeviceContext, PSDCARD_CLIENT_REGISTRATION_INFO pInfo)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice  = CSDHostContainer::GetDeviceByDevice(hDevice);
    if (pDevice) {
        __try {
            status = pDevice->RegisterClient(NULL,pDeviceContext, pInfo);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDRegisterClient: Failed to register client 0x%08X \n"),status));
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDSynchronousBusRequest__X - send an SD Bus request synchronously
//  Input:  hDevice -  device handle
//          Command - command to send
//          Argument - argument for command
//          TransferClass - Command only, or associated with read/write data
//          ResponseType - expected response
//          pResponse - buffer to hold response (OPTIONAL)
//          NumBlocks   - number of blocks
//          BlockSize   - block size
//          pBuffer     - block buffer
//          Flags       - bus request flags
//          
//  Output: pResponse - caller allocated storage for the return response 
//  Return: SD_API_STATUS
//  Notes:  
//        This function provides a synchronous (blocking) call to perform a 
//        bus request.  
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDSynchronousBusRequest__X(SD_DEVICE_HANDLE       hDevice, 
                                         UCHAR                  Command,
                                         DWORD                  Argument,
                                         SD_TRANSFER_CLASS      TransferClass,
                                         SD_RESPONSE_TYPE       ResponseType,
                                         PSD_COMMAND_RESPONSE   pResponse,
                                         ULONG                  NumBlocks,
                                         ULONG                  BlockSize,
                                         PUCHAR                 pBuffer,
                                         DWORD                  Flags,
                                         DWORD cbSize, PPHYS_BUFF_LIST pPhysBuffList)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDSynchronousBusRequest_I(Command,Argument,TransferClass,ResponseType,pResponse,NumBlocks,BlockSize,pBuffer,Flags,cbSize,pPhysBuffList);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDSynchronousBusRequest__X: Failed status 0x%08X \n"),status));
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDBusRequest__I - send command over SD bus
//  Input:  pHandle       - SD bus device structure
//          Command       - SD command to send over bus
//          Argument      - 32 bit argument specific to the command
//          TransferClass - Command only, or associated with read/write data
//          ResponseType  - Response Type for the command
//          NumBlocks     - Number of data blocks in pBlockArray, can be zero
//                          if transfer class is not read or write
//          BlockSize     - Size of data blocks in pBlockArray. All blocks
//                          must be same size.
//          pBuffer       - Pointer to buffer containing BlockSize*NumBlocks bytes
//          pCallback     - completion callback
//          RequestParam    - optional driver specific parameter for this request
//          Flags         - bus request flags
//          fExtern       - True for calling from external.
//  Output: ppRequest     - newly allocated request
//  Return: SD_API_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDBusRequest__X(SD_DEVICE_HANDLE         hDevice,
                              UCHAR                    Command,
                              DWORD                    Argument,
                              SD_TRANSFER_CLASS        TransferClass,
                              SD_RESPONSE_TYPE         ResponseType,
                              ULONG                    NumBlocks,
                              ULONG                    BlockSize,
                              PUCHAR                   pBuffer,
                              PSD_BUS_REQUEST_CALLBACK pCallback,
                              DWORD                    RequestParam,
                              HANDLE                   * phRequest,
                              DWORD                    Flags,
                              DWORD cbSize, PPHYS_BUFF_LIST pPhysBuffList )
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDBusRequest_I(Command,Argument,TransferClass,ResponseType,NumBlocks,BlockSize,pBuffer,
                    pCallback,RequestParam,phRequest,Flags,cbSize,pPhysBuffList);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDBusRequest__X: Failed status 0x%08X \n"),status));
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDFreeBusRequest__X - free this request
//  Input:  pRequest  - the request to free     
//  Output: 
//  Return: 
//  Notes:
//          this function returns the bus request back to the memory look aside 
//          list
///////////////////////////////////////////////////////////////////////////////
VOID SDFreeBusRequest__X(HANDLE  hRequest)
{
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByRequest((HANDLE)hRequest);
    if (pDevice) {
        pDevice->SDFreeBusRequest_I((HANDLE)hRequest);
        pDevice->DeRef();
    }
    else
        DEBUGMSG(SDCARD_ZONE_ERROR , (TEXT("SDFreeBusRequest__X: Failed:wrong Handle 0x%08X \n"),hRequest));
}
///////////////////////////////////////////////////////////////////////////////
//  SDCardInfoQuery__X - Obtain Card information
//  Input:  hHandle        - SD Device Handle
//          InfoType       - information to get
//          StructureSize  - size of info structure
//  Output: pCardInfo      - Information specific structure 
//  Return: SD_API_STATUS code
//  Notes:  pCardInfo must point to sufficient memory for the informtion type
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDCardInfoQuery__X( IN  SD_DEVICE_HANDLE hDevice,
                                 IN  SD_INFO_TYPE     InfoType,
                                 OUT PVOID            pCardInfo,
                                 IN  ULONG            StructureSize)
{

    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDCardInfoQuery_I(InfoType,pCardInfo,StructureSize);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDCardInfoQuery__X: Failed status 0x%08X \n"),status));
    return status;
};
///////////////////////////////////////////////////////////////////////////////
//  SDReadWriteRegistersDirect - Read/Write I/O register(s) direct
//  Input:  hDevice     - the device
//          ReadWrite   - read write flag 
//          Function    - Function number
//          Address     -  starting address
//          ReadAfterWrite - flag to instruct card to read after write
//          pBuffer      -   buffer to hold value of registers
//          BufferLength - number of bytes to read/write
//      
//  Output: 
//  Return: SD_API_STATUS code
//  Notes:  
//          This function can be called to read or write multiple contigous registers synchronously
//          using the SDIO RW_DIRECT command. This function issues multiple commands to transfer
//          to or from the user's buffer
//          If ReadAfterWrite is set to 1, the operation will instruct the card
//          to return the new value of the register in the response.  
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDReadWriteRegistersDirect__X(SD_DEVICE_HANDLE       hDevice,
                                            SD_IO_TRANSFER_TYPE    ReadWrite,
                                            UCHAR                  Function,
                                            DWORD                  Address,
                                            BOOLEAN                ReadAfterWrite,
                                            PUCHAR                 pBuffer,
                                            ULONG                  BufferLength)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        if (pDevice->GetDeviceFuncionIndex()!= 0 && Function == 0) { // Customer are doing function zero.
            CSDDevice*  pDevice0 =  pDevice->GetSlot().GetFunctionDevice(0);
            if (pDevice0!=NULL) {
                pDevice->DeRef();
                pDevice = pDevice0; // Switch to function 0;
            }
        }
        ASSERT(pDevice->GetDeviceFuncionIndex()== Function);
        __try {
            status = pDevice->SDReadWriteRegistersDirect_I(ReadWrite,Address,ReadAfterWrite,pBuffer,BufferLength);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDReadWriteRegistersDirect__X: Failed status 0x%08X \n"),status));
    return status;
};            
///////////////////////////////////////////////////////////////////////////////
//  SDCancelBusRequest__X - Cancel an outstanding bus request
//  Input:  
//          pRequest - request to cancel (returned from SDBusRequest)
//  Output: 
//  Return: TRUE if request was cancelled , FALSE if the request is still pending
//  Notes:
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN SDCancelBusRequest__X(HANDLE  hRequest)
{
    BOOLEAN fReturn = FALSE;
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByRequest((HANDLE)hRequest);
    if (pDevice) {
        fReturn = (BOOLEAN)pDevice->SDCancelBusRequest_I((HANDLE)hRequest);
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !fReturn , (TEXT("SDCancelBusRequest__X: Failed:wrong Handle 0x%08X \n"),hRequest));
    return fReturn;
};

///////////////////////////////////////////////////////////////////////////////
//  SDGetTuple__X  - Get tuple data from CIS
//  Input:  hDevice     - SD bus device handle
//          TupleCode   - Tuple code
//          pBufferSize - size of buffer to store Tuple Data
//          CommonCIS   - flag indicating common or function CIS
//  Output: pBuffer     - Tuple data is copied here (optional)
//          pBufferSize - if pBuffer is NULL, this will store the size of the
//                        tuple
//  Return: SD_API_STATUS code
//          
//  Notes: The caller should initially call this function with a NULL buffer
//         to determine the size of the tuple.  The variable pBufferSize points
//         to the caller supplied storage for this result.   If no bus errors occurs
//         the function returns SD_API_STATUS_SUCCESS.   The caller must check 
//         the value of the buffer size returned.  If the value is non-zero, the
//         tuple exists and can be fetched by calling this function again with a
//         non-zero buffer.
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDGetTuple__X(SD_DEVICE_HANDLE hDevice,
                            UCHAR            TupleCode,
                            PUCHAR           pBuffer,
                            PULONG           pBufferSize,
                            BOOL             CommonCIS)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDGetTuple_I(TupleCode,pBuffer,pBufferSize,CommonCIS);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDGetTuple_I: Failed status 0x%08X \n"),status));
    return status;

}
///////////////////////////////////////////////////////////////////////////////
//  SDIOConnectInterrupt  - Associate an interrupt service routine for an SDIO
//                          peripheral interrupt
//  Input:  hDevice   - SD device handle
//          pIsrFunction - the interrupt service routine
//  Output: 
//  Return: SD_API_STATUS code
//          
//  Notes: This function is provided for an SDIO peripheral driver to 
//         register an interrupt routine for the device. 
//         The interrupt function has the form of PSD_INTERRUPT_CALLBACK.
//         The caller should call SDIODisconnectInterrupt when cleaning up
//         the device. The bus driver will enable the interrupt for the function in the
//         card's CCCR area prior to returning from this function. 
//         The interrupt callback is called whenever the device function is 
//         interrupting. The bus driver will determine the interrupting function,
//         disable the interrupt on the card and call the callback.  Upon return 
//         from the callback the bus driver will reenable the interrupt in the
//         card's CCR.
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDIOConnectInterrupt__X(SD_DEVICE_HANDLE         hDevice, 
                                      PSD_INTERRUPT_CALLBACK   pIsrFunction)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDIOConnectDisconnectInterrupt( pIsrFunction,TRUE);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDIOConnectInterrupt__X: Failed status 0x%08X \n"),status));
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDIODisconnectInterrupt  - disconnect the interrupt 
//  Input:  hDevice   - SD device handle
//  Output: 
//  Return: 
//          
//  Notes: This function should be called to disconnect the interrupt
//         from the device. The bus driver will disable the interrupt in the
//         card's CCCR area
///////////////////////////////////////////////////////////////////////////////
VOID SDIODisconnectInterrupt__X(SD_DEVICE_HANDLE hDevice)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDIOConnectDisconnectInterrupt( NULL,FALSE);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDIODisconnectInterrupt__X: Failed status 0x%08X \n"),status));
}
///////////////////////////////////////////////////////////////////////////////
//  SDSetCardFeature       - Set card feature
//  Input:  hDevice        - SD Device Handle
//          CardFeature    - Card Feature to set
//          StructureSize  - size of card feature structure
//  Output: pCardInfo      - Information for the feature
//  Return: SD_API_STATUS code
//  Notes:  This function is provided to set various card features
//          in a thread safe manner.  SDIO cards utilize shared register sets
//          between functions. This requires that the 
//          register state be preserved between functions that can be 
//          controlled in separate thread contexts.
//          This function can potentially block by issuing synchronous bus 
//          request.  This function must not be called from a bus request callback
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SDSetCardFeature__X(SD_DEVICE_HANDLE     hDevice,
                                  SD_SET_FEATURE_TYPE  CardFeature,
                                  PVOID                pCardInfo,
                                  ULONG                StructureSize)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByDevice((HANDLE)hDevice);
    if (pDevice) {
        __try {
            status = pDevice->SDSetCardFeature_I(CardFeature,pCardInfo,StructureSize);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDSetCardFeature__X: Failed status 0x%08X \n"),status));
    return status;
}

SD_API_STATUS SDBusRequestResponse__X(HANDLE hRequest, PSD_COMMAND_RESPONSE pSdCmdResp)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    CSDDevice *     pDevice = CSDHostContainer::GetDeviceByRequest((HANDLE)hRequest);
    if (pDevice) {
        __try {
            status = pDevice->SDBusRequestResponse_I((HANDLE)hRequest,pSdCmdResp);
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
        pDevice->DeRef();
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status) , (TEXT("SDBusRequestResponse__X: Failed:wrong Handle 0x%08X \n"),hRequest));
    return status;
}

extern "C"
SD_API_STATUS 
SDGetClientFunctions(
                     PSDCARD_API_FUNCTIONS pFunctions
                     )
{    
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDGetClientFunctions: +Init\n")));

    if ( pFunctions && (pFunctions->dwSize == sizeof(*pFunctions)) ) {
        pFunctions->pSDRegisterClient = SDRegisterClient__X;
        pFunctions->pSDSynchronousBusRequest = SDSynchronousBusRequest__X;
        pFunctions->pSDBusRequest = (PSDBUS_REQUEST)SDBusRequest__X;
        pFunctions->pSDFreeBusRequest = (PSD_FREE_BUS_REQUEST)SDFreeBusRequest__X;
        pFunctions->pSDCardInfoQuery = SDCardInfoQuery__X;
        pFunctions->pSDReadWriteRegistersDirect = SDReadWriteRegistersDirect__X;
        pFunctions->pSDCancelBusRequest = (PSD_CANCEL_BUS_REQUEST)SDCancelBusRequest__X;
        pFunctions->pSDGetTuple = SDGetTuple__X;
        pFunctions->pSDIOConnectInterrupt  = SDIOConnectInterrupt__X;
        pFunctions->pSDIODisconnectInterrupt = (PSD_IO_DISCONNECT_INTERRUPT)SDIODisconnectInterrupt__X;
        pFunctions->pSDSetCardFeature = SDSetCardFeature__X;
        pFunctions->pSdBusRequestResponse = SDBusRequestResponse__X;

        status = SD_API_STATUS_SUCCESS;
    }
    else {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDGetClientFunctions: Invalid parameter\n")));
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDGetClientFunctions: -Init\n")));

    return status;
};



