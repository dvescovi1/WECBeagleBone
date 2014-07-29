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
//     SdBusReq.hpp
// Abstract:  
//     Definition for the sd request.
//
// 
// 
// Notes: 
// 
//

#pragma once


#ifdef DEBUG

// card response text look up
__declspec(selectany) extern const WCHAR  *SDCardResponseTypeLookUp[8] =
{{TEXT("NoResponse")},
{TEXT("R1")},
{TEXT("R1b")},
{TEXT("R2")},
{TEXT("R3")},
{TEXT("R4")},
{TEXT("R5")},
{TEXT("R6")}
};


// card state string look up
__declspec(selectany) extern const WCHAR  *SDCardStateStringLookUp[16] =
{{TEXT("IDLE")},
{TEXT("READY")},
{TEXT("IDENT")},
{TEXT("STBY")},
{TEXT("TRAN")},
{TEXT("DATA")},
{TEXT("RCV")},
{TEXT("PRG")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
{TEXT("UNKN")},
};

#endif

class CSDBusRequest;
// bus request System Flags
// Bits 31-28 are defined in SDCardDDK.h
#define SD_BUS_REQUEST_BUSY                 0x00000100      // bus request is in the HC
#define SD_BUS_REQUEST_NON_CANCELABLE       0x00000200      // bus request is non-cancelable
#define SD_BUS_REQUEST_COMPLETING           0x00000400      // bus request is being processed in dispatcher
#define SD_BUS_REQUEST_USING_SOFT_BLOCK     0x00000800      // Soft-block processing in use for this request
#define SD_BUS_REQUEST_SLOT_RESET           0x00001000      // bus request for resetting slot
#define SYSTEM_FLAGS_RETRY_COUNT_MASK       0x000000FF

class CSDBusRequest : public SD_BUS_REQUEST {
public:
    // Operator
    void * operator new(size_t stSize/*, CSDDevice& sdDevice*/); // We could move this to device-based operation. But, now it is global
    void operator delete (void *pointer);
public:
    CSDBusRequest(CSDDevice& sdDevice,SD_BUS_REQUEST& sdBusRequest, HANDLE hCallback = NULL, CSDBusRequest * pParentBus = NULL );
    virtual ~CSDBusRequest();
    virtual BOOL    Init();
    BOOL    IsParent() { return m_pParentBus == NULL; };
    BOOL    IsComplete() { 
        if (m_pChildListNext) 
            return (m_fCompleted && m_pChildListNext->IsComplete());
        else
            return m_fCompleted; 
    };
    CSDDevice& GetDevice() { return m_sdDevice; };
// Reference function.
    DWORD AddRef( void ) {
        return (DWORD)InterlockedIncrement(&m_lRefCount);
    };
    DWORD DeRef( void ) {
        LONG lReturn = InterlockedDecrement(&m_lRefCount);
        // Last thing to do
        if( lReturn <= 0 ) {
            delete this;
        }
        return (DWORD)lReturn;
    }
// Retry Count
    BOOL  IsRequestNeedRetry();
    DWORD GetRetryCount(){ return (SystemFlags & SYSTEM_FLAGS_RETRY_COUNT_MASK); };
    DWORD   DecRetryCount() {
        DWORD dwRetryCount = (SystemFlags & SYSTEM_FLAGS_RETRY_COUNT_MASK);
        if (dwRetryCount)
            dwRetryCount--;
        SystemFlags &= ~SYSTEM_FLAGS_RETRY_COUNT_MASK;
        SystemFlags |= (dwRetryCount & SYSTEM_FLAGS_RETRY_COUNT_MASK);
        return dwRetryCount;
    }
    
    void SetAsyncQueueNext(CSDBusRequest * pNextBusRequest) { m_pAsyncQueueNext = pNextBusRequest; };
    CSDBusRequest * GetAsyncQueueNext() { return m_pAsyncQueueNext; };
    void SetChildListNext(CSDBusRequest * pNextBusRequest) { 
        if (m_pChildListNext!=NULL) {
            m_pChildListNext->SetChildListNext(pNextBusRequest);
        }
        else 
            m_pChildListNext = pNextBusRequest; 
    };
    CSDBusRequest * GetChildListNext() { return m_pChildListNext; };
    DWORD   GetRequestRandomIndex() { return m_dwRequestIndex; };

    VOID    SetStatus(SD_API_STATUS status) { 
        Status = status; 
        m_fCompleted = (status!=SD_API_STATUS_PENDING);
    };
    BOOL CompleteBusRequest(SD_API_STATUS SdStatus) {
        // Note the response should be updaed by SDHC.
        Status  = SdStatus;
        m_fCompleted = TRUE;
        return (m_pParentBus!=NULL? m_pParentBus->CheckForCompletion():CheckForCompletion());
    }
    BOOL Cancel() {
        // Cancel it from revers order.
        if (m_pChildListNext)
            m_pChildListNext->Cancel();
        return (m_sdDevice.GetSlot().RemoveRequest(this)!=NULL) ;
    }
    SD_API_STATUS GetFirstFailedStatus() {
        if (SD_API_SUCCESS(Status) && m_pChildListNext!=NULL )
            return m_pChildListNext->GetFirstFailedStatus();
        else
            return Status;
    }
    BOOL    TerminateLink()  {
        CSDBusRequest * pCur = (CSDBusRequest *)InterlockedExchange((LONG *)&m_pChildListNext,(LONG)NULL);
        if (pCur) {
            pCur->DeRef();
        }
        pCur = (CSDBusRequest *)InterlockedExchange((LONG *)&m_pParentBus,(LONG)NULL);
        if (pCur) {
            pCur->DeRef();
        }
        return TRUE;
    }
    HANDLE SetExternalHandle(HANDLE hHandle) { return (m_ExternalHandle=hHandle); };
    HANDLE GetExternalHandle() { return m_ExternalHandle; };
    BOOL    MapAsyncBuffer();
    BOOL    IsAsyncBufferMapped() { return ((pBlockBuffer!=NULL) &&(pBlockBuffer != (PUCHAR) m_pOrinalAddr));  };
protected:
    CSDBusRequest * m_pAsyncQueueNext; // This is own by Slot Async Request Queue.

    CSDDevice& m_sdDevice;
    CSDBusRequest * m_pParentBus;
    CSDBusRequest * m_pChildListNext;
    BOOL    BuildOptionalRequest( UCHAR ucSDIOFlags );
    BOOL    BuildSoftBlock() ;
    BOOL    CheckForCompletion();
    HANDLE  m_ExternalHandle;
private:
    static DWORD g_dwRequestIndex;
    DWORD   m_dwRequestIndex;
    LONG    m_lRefCount;

    BOOL    m_fCompleted;

// Request Argument.
    HANDLE  m_hCallback ;
    PVOID   m_pOrinalAddr;
    DWORD   m_dwArguDesc;

    PPHYS_BUFF_LIST   m_pOrignalPhysAddr;
    
    CSDBusRequest&operator=(CSDBusRequest&){ASSERT(FALSE);}

};

