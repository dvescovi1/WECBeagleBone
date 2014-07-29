// All rights reserved Texas Instrumens, Inc. 2011
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//

#ifndef __CPPI_DMA_HPP__
#define __CPPI_DMA_HPP__

//////////////////////////////////////////////////////////////////////////////////////////
// CPPI USB DMA controller

#include <usb200.h>
#include "am33x_usb.h"
#include "am33x_usbcdma.h"

// Custom transfer flags
#define USB_TOGGLE_CARRY            0x80000000

// Per-channel Limitations
#define CPPI_MAX_BUFFER             0x10000
#define CPPI_MAX_DESCR              256

#if DEBUG
#define ISO_MIN_ADVANCED_FRAME      6
#define ISO_MAX_FRAME_ERROR         12
#else
#define ISO_MIN_ADVANCED_FRAME      6
#define ISO_MAX_FRAME_ERROR         3
#endif

// Forward declarations
class CCppiDmaChannel;
class CCppiDmaController;

// USB transfer completion callback
typedef void (*PfnTransferComplete) (
    CCppiDmaChannel *pChannel,
    UINT32 nStatus,
    UINT32 nLength,
    UINT32 nComplete,
    UINT32 nOptions,
    PVOID pCookie1,
    PVOID pCookie2);


////////////////////////////////////////////////////////////////////////////////////////////////////
// CPPI DMA channel class. Note the class is abstract and therefore can not be instantiated.
// Instead, use derived channel objects returned by the CCppiDmaController::AllocChannel() method.
//
class CCppiDmaChannel
{
public:
             CCppiDmaChannel(
                 CCppiDmaController *pController,
                 UINT8 epNum,
                 PfnTransferComplete pCallback);

    virtual ~CCppiDmaChannel();

    UINT32 AddRef      () { return ++m_nRefCount; }
    UINT32 Release     () { return --m_nRefCount; }
    UINT32 GetRefCount () { return   m_nRefCount; }

    virtual BOOL IssueTransfer(
        UINT8   epAddr,
        UINT8   fnAddr,
        UINT8   epType,
        UINT16  epMaxPkt,
        PVOID   pvBuffer,
        UINT32  paBuffer,
        UINT32  nLength,
        UINT32  nFrames,
        UINT32* pnFrameLengths,
        DWORD*  pdwIsochErrors,
        DWORD*  pdwIsochLengths,
        UINT32  nOptions,
        PVOID   pCookie1,
        PVOID   pCookie2) = 0;

    virtual BOOL CancelTransfer() = 0;

protected:
    void ChLock   () { EnterCriticalSection(&m_csLock); }
    void ChUnlock () { LeaveCriticalSection(&m_csLock); }

    virtual BOOL ValidateTransferState    () = 0;
    virtual void ProcessCompletedPacket   (HOST_DESCRIPTOR*     pHd) = 0;
            void ProcessCompletedTeardown (TEARDOWN_DESCRIPTOR* pTd);

    void NextSegment();
    void ReleaseSegment();
    void UpdateRndisMode(BOOL fIsRndisMode);
    void KickCompletionCallback();

    void  QueuePush (void* pD);
    void* QueuePop  ();

protected:
    UINT8  ChannelNumber()     const { return m_chNum; }
    UINT8  EndpointNumber()    const { return m_epNum; }
    UINT8  EndpointAddress()   const { return m_epAddr; }
    UINT8  EndpointType()      const { return m_epType; }
    UINT16 EndpointMaxPacket() const { return m_epMaxPkt; }
    BOOL   IsTeardownPending() const { return m_fIsTeardownPending; }
    BOOL   IsOut()             const { return m_fIsOut; }
    BOOL   IsIn()              const { return !m_fIsOut; }
    BOOL   IsInUse()           const { return m_nTransferFrames > 0; }
    BOOL   IsBulk()            const { return m_epType == USB_ENDPOINT_TYPE_BULK; }
    BOOL   IsInterrupt()       const { return m_epType == USB_ENDPOINT_TYPE_INTERRUPT; }
    BOOL   IsIso()             const { return m_epType == USB_ENDPOINT_TYPE_ISOCHRONOUS; }
    BOOL   IsRndisMode()       const { return m_fIsRndisMode != 0; }
    BOOL   IsCancelPending()   const { return m_fIsCancelPending; }

protected:
    friend class CCppiDmaController;

    // Channel data
    UINT32                m_nRefCount;
    CCppiDmaController*   m_pController;
    CSL_UsbRegs*          m_pUsbRegs;
    CSL_UsbEpcsrRegs*     m_pUsbEpcsrRegs;
    CSL_CppiRegs*         m_pCppiRegs;
    CSL_ChannelRegs*      m_pCppiChannelRegs;
    UINT8                 m_nCppiChannelOffset;
    UINT8*                m_pvBuffer;
    PHYSICAL_ADDRESS      m_paBuffer;
    UINT32                m_cbBuffer;
    BOOL                  m_fIsOut;
    BOOL volatile         m_fIsTeardownPending;
    UINT8                 m_chNum;
    UINT8                 m_epNum;
    UINT8                 m_qNum;
    PfnTransferComplete   m_pCallback;
    CRITICAL_SECTION      m_csLock;
    BOOL                  m_fIsCancelPending;
    UINT32                m_nCancelStatus;

    // Transfer parameters
    UINT8   m_epAddr;
    UINT8   m_fnAddr;
    UINT8   m_epType;
    UINT16  m_epMaxPkt;
    UINT32  m_nMaxBD;
    UINT8*  m_pvTransferBuffer;
    UINT32  m_paTransferBuffer;
    UINT32  m_nTransferFrames;
    UINT32* m_pTransferFrameLengths;
    UINT32* m_pTransferFrameLengthsActual;
    DWORD*  m_pdwIsochErrors;
    DWORD*  m_pdwIsochLengths;
    UINT32  m_nTransferLength;
    UINT32  m_nTransferOptions;
    PVOID   m_pTransferCookie1;
    PVOID   m_pTransferCookie2;
    UINT32  m_nTransferComplete;
    UINT32  m_nTransferFramesComplete;
    UINT32  m_nSegmentPending;
    UINT32  m_nSegmentFramesPending;
    UINT32  m_nSegmentFramesComplete;
    UINT32  m_nSegmentComplete;
    BOOL    m_fIsRndisMode;

public:
	UINT32  m_chwInd;

};


////////////////////////////////////////////////////////////////////////////////////////////////////
// CPPI DMA controller class
//
class CCppiDmaController
{
public:
    CCppiDmaController(DWORD dwDescriptorCount);
   ~CCppiDmaController();

    BOOL Initialize   (UINT32 paUdcBase, PVOID pvUdcBase, PVOID pvCppiBase, UINT8 chwInd);
    void Deinitialize ();

    CCppiDmaChannel* AllocChannel   (UINT8 epNum, UINT8 epAddr, PfnTransferComplete pCallback);
    void             ReleaseChannel (CCppiDmaChannel *pChannel);

#ifdef USB_STAT
	void GetStat(UINT32 *retBuf);
#endif

protected:
    void Lock   () { EnterCriticalSection(&m_csLock); }
    void Unlock () { LeaveCriticalSection(&m_csLock); }

    UINT32        PaUsbRegs  () { return m_paUsbRegs; }
    CSL_UsbRegs*  PvUsbRegs  () { return m_pUsbRegs;  }
    CSL_CppiRegs* PvCppiRegs () { return m_pCppiRegs; }

protected:
    friend class CHW; // This could be removed (if the we validate our own state instead)
    friend class CCppiDmaChannel;
    friend class CCppiDmaRxChannel;
    friend class CCppiDmaTxChannel;

    void  QueuePush (BYTE qNum, void* pD);
    void* QueuePop  (BYTE qNum);

    BOOL  ValidateTransferState();
    void  ProcessCompletionEvent(void* pD);
    void  OnCompletionEvent();

    static VOID CompletionCallback(PVOID param);

    BOOL PoolInit   ();
    void PoolDeinit ();
    void PoolLock   () { EnterCriticalSection(&m_csPoolLock); }
    void PoolUnlock () { LeaveCriticalSection(&m_csPoolLock); }

    void HdPoolInit ();

    HOST_DESCRIPTOR* HdAlloc ();
    HOST_DESCRIPTOR* HdFree  (HOST_DESCRIPTOR* hd);

    UINT32 DescriptorVAtoPA(void* va);
    void*  DescriptorPAtoVA(UINT32 pa);

private:
    CRITICAL_SECTION m_csLock;
    UINT32           m_paUsbRegs;
    CSL_UsbRegs*     m_pUsbRegs;
    CSL_CppiRegs*    m_pCppiRegs;
    HANDLE           m_hUsbCdma;
    DWORD            m_dwDescriptorCount;
    UINT8            m_nCppiChannelOffset;
    UINT8            m_nRxQueueOffset;
    UINT8            m_nTxQueueOffset;

	UINT8			 m_chwInd;
    CCppiDmaChannel* m_pTxChannels[USB_CPPI_MAX_TX_CHANNELS];
    CCppiDmaChannel* m_pRxChannels[USB_CPPI_MAX_RX_CHANNELS];

    void*                     m_pvPool;           /**< Virtual Base of Descriptor Pool */
    PHYSICAL_ADDRESS          m_paPool;           /**< Physical Base of Descriptor Pool */
    UINT32                    m_cbPoolSize;       /**< Size of Descriptor Pool (Host + Teardown) */
    CRITICAL_SECTION          m_csPoolLock;       /**< Critical Section to Protect Pool Access */

    HOST_DESCRIPTOR*          m_pvHdPool;         /**< Virtual Base of Host-Descriptor Pool */
    HOST_DESCRIPTOR*          m_pvHdPoolHead;     /**< Free Host-Descriptor Head Pointer */

#ifdef USB_STAT
	UINT32			m_TxChaAllocCnt[USB_CPPI_MAX_TX_CHANNELS];
	UINT32			m_TxChaReleaseCnt[USB_CPPI_MAX_TX_CHANNELS];
	UINT32			m_RxChaAllocCnt[USB_CPPI_MAX_RX_CHANNELS];
	UINT32			m_RxChaReleaseCnt[USB_CPPI_MAX_RX_CHANNELS];
	UINT32			m_hdAllocCnt;
	UINT32			m_hdFreeCnt;
#endif	
};


////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
