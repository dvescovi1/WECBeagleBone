// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  chw.hpp
//
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
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name:
//     CHW.hpp
//
// Abstract: Provides interface to OHCI host controller
//
// Notes:
//

#ifndef __CHW_HPP__
#define __CHW_HPP__

#include <globals.hpp>
#include <hcd.hpp>
#include "cppidma.hpp"

//vvv From desclist
#define NO_ED   32
#define NO_TD   128

typedef enum{
    TRANS_NO_ERROR=0,
    TRANS_ERROR,
    TRANS_NAK,
    TRANS_STALL,
}TransactionStatus;

#define MIN(a,b) ((a)<(b)?(a):(b))


typedef struct _USBTD USBTD;
typedef struct _USBED USBED;
typedef struct ListHead ListHead;

//^^^ From desclist

class COhcd;
class CHW;

typedef const DWORD C_DWORD;

struct HCCA {
    DWORD HccaInterruptTable[32];
    WORD  HccaFrameNumber;
    WORD  HccaPad1;             // unused
    DWORD HccaDoneHead;         // PAddr of first TD in Done Queue. 16-byte aligned. LSb special.
    // remainder is reserved so we can ignore it.
};

typedef struct
{
    UINT8 refCount;       // Refernce count
    UINT8 epType;         // Endpoint type
    UINT8 epTypeCurrent;  // Assigned Endpoint type
    UINT8 fDBMode;        // Double buffering mode
    UINT8 fSharedMode;    // Shared mode
    UINT16 fifoSize;      // Fifo size (bytes)
}
EP_CONFIG;

class CHW : public CHcd
{
public:

    // ****************************************************
    // public Functions
    // ****************************************************

    //
    // Hardware Init/Deinit routines
    //
    CHW(IN /*const*/ CSL_UsbRegs * portBase,
        IN /*const*/ CSL_CppiRegs * cppiBase,
        IN const DWORD dwSysIntr,
        IN const DWORD dwDescriptorCount,
        IN CPhysMem * const pCPhysMem,
        IN LPVOID pvOhcdPddObject );
    ~CHW();
    virtual BOOL   Initialize(void);
    virtual void   DeInitialize( void );

    void   EnterOperationalState(void);

    void   StopHostController(void);

    BOOL GetFrameNumber( OUT LPDWORD lpdwFrameNumber );
    BOOL GetFrameLength( OUT LPUSHORT lpuFrameLength );
    BOOL SetFrameLength( IN HANDLE hEvent,IN USHORT uFrameLength );
    BOOL StopAdjustingFrame(void) { return TRUE; };

    LPCTSTR GetControllerName( void ) const { return m_s_cpszName; }

	//
    // Root Hub Queries
    //
    BOOL DidPortStatusChange( IN const UCHAR port );

    BOOL GetPortStatus( IN const UCHAR port,
                               OUT USB_HUB_AND_PORT_STATUS& rStatus );

	void GetRootHubDescriptor( OUT USB_HUB_DESCRIPTOR& descriptor );

    BOOL RootHubFeature( IN const UCHAR port,
                                IN const UCHAR setOrClearFeature,
                                IN const USHORT feature );

    BOOL ResetAndEnablePort( IN const UCHAR port );

    void DisablePort( IN const UCHAR port );

    virtual BOOL WaitForPortStatusChange (HANDLE m_hHubChanged);
    void InitializeTransaction(CSL_UsbRegs * portBase, void *pED, void *pTD);
    DWORD /*CHcd::*/GetCapability(/*void*/) { return m_dwCap; }
    DWORD /*CHcd::*/SetCapability(DWORD dwCap) { return (m_dwCap |= dwCap); }

#ifdef USB_NEW_INTERRUPT_MODEL
    BOOL    HandleTxEpInterrupt (IN PVOID context, UINT16   intrTxValue, UINT16 endPoint);
    BOOL    HandleRxEpInterrupt (IN PVOID context, UINT16   intrRxValue, UINT16 endPoint);
#endif

    void LockProcessingThread() { EnterCriticalSection(&m_csUsbProcLock);	}
    void UnlockProcessingThread() { LeaveCriticalSection(&m_csUsbProcLock); }

	// From desclist
	void CreateDescriptors(void);
	USBTD* AllocateTD(void);
	USBED* AllocateED(void);
	void FreeED(USBED *);
	void FreeTD(USBTD *);
	BOOL RemoveElementFromList(ListHead **, ListHead *);
	ListHead *RemoveElementFromListEnd( ListHead **);
	ListHead *RemoveElementFromListStart( ListHead **);

private:
	void AddElementToListEnd( ListHead *pElement, ListHead **ppQueueHead );
	void AddElementToListStart(ListHead *pElement, ListHead **ppQueueHead );

public:
	USBED		*m_EDs; //[NO_ED];
	USBTD		*m_TDs; //[NO_TD];
	USBED		*m_pFreeED;
	USBTD		*m_pFreeTD;
	UINT32		m_EDUsage;
	UINT32		m_TDUsage;

public:
	// ****************************************************
    // public Variables
    // ****************************************************
    PDWORD m_pControlHead;
    PDWORD m_pBulkInHead;
    PDWORD m_pBulkOutHead;
    PDWORD m_pIntInHead;
    PDWORD m_pIntOutHead;
    PDWORD m_pIsoInHead;
    PDWORD m_pIsoOutHead;

	HANDLE m_hUsbProcessingEvent;
    /*Pointers to USBED whose transfer is currently handled
    by Host Endpoints
    NULL - Endpoint is not processg any request*/
    PDWORD m_pProcessEDControl;
    PDWORD m_pProcessEDIn[MGC_MAX_USB_ENDS - 1];
    PDWORD m_pProcessEDOut[MGC_MAX_USB_ENDS - 1];


private:
    // ****************************************************
    // private Functions
    // ****************************************************

    CHW& operator=(CHW&) { ASSERT(FALSE);}
    static DWORD CALLBACK CeResumeThreadStub( IN PVOID context );
    DWORD CeResumeThread();

    static DWORD CALLBACK UsbInterruptThreadStub( IN PVOID context );
    DWORD UsbInterruptThread( IN PVOID context );

    static DWORD CALLBACK UsbProcessingThreadStub( IN PVOID context );
    DWORD UsbProcessingThread();

    //static DWORD CALLBACK UsbAdjustFrameLengthThreadStub( IN PVOID context );
    //DWORD CALLBACK UsbAdjustFrameLengthThread();

    BOOL LoadEnpointConfiguration( void );

    void UpdateFrameCounter( void );

    void InitialiseFIFOs(void);

#ifdef DEBUG
    // Query Host Controller for registers, and prints contents
    DWORD dwTickCountLastTime;
    static void DumpAllRegisters(void);
#endif
    WORD lastFn;
    // ****************************************************
    // Private Variables
    // ****************************************************

    static const TCHAR m_s_cpszName[8];
    CSL_UsbRegs *m_portBase;    // REGISTER m_portBase;
	CSL_CppiRegs *m_cppiBase;   //REGISTER m_cppiBase;
	volatile HCCA *m_pHCCA;
    BOOL m_fConstructionStatus;

    DWORD m_dwDescriptorCount;
    // internal frame counter variables
    CRITICAL_SECTION m_csFrameCounter;
    WORD m_wFrameHigh;

    // interrupt thread variables
    DWORD    m_dwSysIntr;
    HANDLE   m_hUsbInterruptEvent;
    HANDLE   m_hUsbInterruptThread;
    HANDLE   m_hUsbProcessingThread;
    HANDLE   m_hUsbHubChangeEvent;
    BOOL     m_fUsbInterruptThreadClosing;

    // initialization parameters for the IST to support CE resume
    // (resume from fully unpowered controller).

    CPhysMem *m_pMem;
    LPVOID m_pPddContext;
    BOOL m_fPowerUpFlag;
    BOOL m_fPowerResuming;
    UINT16 m_wFifoOffset;
    UINT8 m_portStatus; /*bit 0-portConnected, 1-ConnectStatusChange*/
    UINT8 m_bHostEndPointUseageCount;
    DWORD m_dwCap;
    UINT8 m_bulkEpNum;
    UINT8 m_bulkEpUseCount;
    BOOL m_bulkEpConfigured;
    HANDLE m_Ep0ProtectSem;
    HANDLE m_EpInProtectSem[MGC_MAX_USB_ENDS - 1];
    HANDLE m_EpOutProtectSem[MGC_MAX_USB_ENDS - 1];
    BOOL m_fHighSpeed;
    CRITICAL_SECTION m_csUsbProcLock;

    DWORD       m_NumOfPort;

    // Static EP FIFO configuration
    EP_CONFIG m_EpInConfig[MGC_MAX_USB_ENDS - 1];
    EP_CONFIG m_EpOutConfig[MGC_MAX_USB_ENDS - 1];

    // Convert byte FIFO size to SZ register value (2^(m+3))
    UINT8 Byte2FifoSize(UINT16 bs)
    {
        for (UINT8 i = 0; i < 10; i ++)
        {
            if (bs == (1 << (i + 3)))
            {
                return i;
            }
        }

        return (UINT8)-1;
    }

public:

    VOID PowerMgmtCallback( IN BOOL fOff );
    INT32 AllocateHostEndPoint(UINT32 TrasnferType, DWORD MaxPktSize, BOOL IsTypeIN);
    void FreeHostEndPoint(UINT32 EndpointNum, BOOL IsDirectionIN);
    void StopHostEndpoint(DWORD HOstEndpointNumber);
    void ProgramHostEndpoint(UINT32 TrasnferType, void *);

	DWORD   GetNumOfPorts(void){return m_NumOfPort; } 

#ifdef MUSB_USEDMA
public:

    // DMA controller object
    CCppiDmaController m_dmaCrtl;

    // Static pointer for use in DMA callback
    static CHW *m_pChw[2];

    // DMA transfer completion callback
    static void DmaTransferComplete(CCppiDmaChannel *pChannel,
        UINT32 nStatus,UINT32 nLength, UINT32 nComplete, UINT32 nOptions,
        PVOID pPrivate, PVOID pCookie);

#endif // MUSB_USEDMA

#ifdef USB_STAT

public:
void GetStatInfo(UINT32 * retBuf);
#endif

};
#endif // __CHW_HPP__

