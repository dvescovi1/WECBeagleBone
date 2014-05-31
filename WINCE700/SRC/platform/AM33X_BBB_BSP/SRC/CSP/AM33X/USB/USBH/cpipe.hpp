// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  cpipe.hpp
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
//     CPipe.hpp
//
// Abstract: Implements class for managing open pipes for OHCI
//
//                             CPipe (ADT)
//                           /             \
//                  CQueuedPipe (ADT)       CIsochronousPipe
//                /         |       \
//              /           |         \
//   CControlPipe    CInterruptPipe    CBulkPipe
//
// Notes:
//
//       Revised pipe classes hierarchy for Mentor USB
//       controller. The original ISO implementation highly
//       depends on xHCI ...
//
//
//                             CPipe (ADT)
//                           /
//                  CQueuedPipe (ADT)-------------
//                /         |       \             \
//              /           |         \            \
//   CControlPipe    CInterruptPipe    CBulkPipe   CIsochronousPipe
//


#ifndef __CPIPE_HPP__
#define __CPIPE_HPP__

#include <globals.hpp>
#include <cphysmem.hpp>
#include "chw.hpp"

class CHCCArea;
class COhcd;
class CPipe;
class CIsochronousPipe;
class CQueuedPipe;
class CControlPipe;
class CInterruptPipe;
class CBulkPipe;
typedef struct STRANSFER STransfer;
class CTransfer ;
class CITransfer ;
typedef struct _TD TD, *P_TD;
typedef struct _ITD ITD, *P_ITD;
typedef struct _ED ED, *P_ED;


// Not real PIDs like in UHCI, these codes are defined in the OHCI spec.
#define TD_IN_PID    2
#define TD_OUT_PID   1
#define TD_SETUP_PID 0

typedef enum { TYPE_UNKNOWN =0, TYPE_CONTROL, TYPE_BULK, TYPE_INTERRUPT, TYPE_ISOCHRONOUS, TYPE_ANY } PIPE_TYPE;
const UINT gcTdOffsetBits = 12; // number of bits in an offset within an OHCI page
const UINT gcTdPageSize = 1<<gcTdOffsetBits; // size of an OHCI page (4K) in bytes
const UINT gcTdMaxPages = 2;    // maximum contiguous pages referenceable by one TD
const UINT gcTdMaxBuffer = gcTdPageSize*gcTdMaxPages;

const UINT gcTdIsochMaxFrames = 60000; // leave some room - 60 seconds is plenty.

const UINT gcTdInterruptOnComplete = 0; // interrupt immediately when this TD completes
const UINT gcTdNoInterrupt = 7;    // do not interrupt when this TD completes

typedef union {
    DWORD phys;                 // used when preparing to give the TD to the HC
    P_TD td;                    // for back-linking done TDs
    P_ITD itd;                  // ditto, but eases type-casting
} TDLINK;

typedef struct _TD {
    DWORD :16;                  // reserved, bits 0-15
    DWORD bfDiscard:1;          // reserved, bit 16 (used by HCD, matches ITD))
    DWORD bfIsIsoch:1;          // reserved, bit 17 (used by HCD, matches ITD))
    DWORD bfShortPacketOk:1;    // bit 18
    DWORD bfPID:2;              // bits 19-20
    DWORD bfDelayInterrupt:3;   // bits 21-23
    DWORD bfDataToggle:2;       // bits 24-25
        enum { DATATOG_ED=0, DATATOG_0=2, DATATOG_1=3 };
    DWORD bfErrorCount:2;       // bits 26-27
    DWORD bfConditionCode:4;    // bits 28-31
    ULONG paCurBuffer;
    TDLINK paNextTd;
    ULONG paBufferEnd;

    // These add-ons are for the driver to use
    CTransfer *pTransfer;       // transfer object owning this TD (until scheduled)
    TD *pNextTd;                // next TD in this transfer (paNextTd is clobbered by the HC)
    // This is temporary until we stop copying transfer objects around
    CQueuedPipe *pPipe;
    // These are unused but force proper alignment
    DWORD _reserved[1];
} TD;

typedef union {
    struct {
        // bit 12 is really a page select, but we can pretend it's an address bit
        WORD uOffset:13;
        WORD uHiBits:3;  // only the most significant 3 bits, for initting.
    };
    struct {
        WORD uSize:12;
        WORD uConditionCode:4;
    };
} ITDFrame;
const UINT gcITdNumOffsets                  = 8;

typedef struct _ITD {
    DWORD   bfStartFrame:16;    // bits 0-15
    DWORD   bfDiscard:1;        // reserved, bit 16 (used by HCD, matches TD)
    DWORD   bfIsIsoch:1;        // reserved, bit 17 (used by HCD, matches TD)
    DWORD   :3;                 // reserved, bits 18-20
    DWORD   bfDelayInterrupt:3; // bits 21-23
    DWORD   bfFrameCount:3;     // bits 24-26
    DWORD   :1;                 // undefined, bit 27
    DWORD   bfConditionCode:4;  // bits 28-31
    DWORD   paBufferPage0;      // And with gcITdPageMask
    TDLINK  paNextTd;
    DWORD   paBufferEnd;
    ITDFrame offsetPsw[gcITdNumOffsets];

    // These add-ons are for the driver to use
    CITransfer *pTransfer;       // transfer object owning this TD
    P_ITD pNextTd;               // next TD in this transfer (paNextTd is clobbered by the HC)
    // This is temporary until we stop copying transfer objects around
    CIsochronousPipe *pPipe;
    // These are unused but force proper alignment
    DWORD _reserved[5];
} ITD;

typedef struct _ED {
    DWORD   bfFunctionAddress:7; // bits 0-6
    DWORD   bfEndpointNumber:4; // bits 7-10
    DWORD   bfDirection:2;      // bits 11-12
    DWORD   bfIsLowSpeed:1;     // bit 13
    DWORD   bfSkip:1;           // bit 14
    DWORD   bfIsIsochronous:1;  // bit 15
    DWORD   bfMaxPacketSize:11; // bits 16-26
    DWORD   :5;                 // undefined, bits 27-31
    ULONG   paTdQueueTail;
    union {
        ULONG paTdQueueHead;
        struct {
            DWORD bfHalted:1;
            DWORD bfToggleCarry:1;
            DWORD :30;
        };
    };
    ULONG   paNextEd;
} ED;

// This is an instantaneous check; a stable check requires that the ED be disabled first.
// The LSNs of the queue pointers are not actually part of the pointers themselves.
#define IsEDQEmpty(p) (((p)->paTdQueueHead & ~0xF) == ((p)->paTdQueueTail & ~0xF))

#include "Transfer.hpp"

typedef struct ListHead {
    struct ListHead *next;
}ListHead;

#ifdef MUSB_USEDMA
// Forward declaration
class CCppiDmaChannel;
#endif // MUSB_USEDMA

typedef enum{
    STATUS_IDLE=0, /*endpoint is not being serviced currently*/
    STATUS_PROGRESS, /*being serviced currently*/
    STATUS_COMPLETE, /* servicing complete*/
    STATUS_NO_RESPONSE,
    STATUS_NAK,
    STATUS_CANCEL
}TransferStatus;

typedef struct _USBED {
    ListHead  NextED;
#if 0
    DWORD   bfFunctionAddress:7; // bits 0-6
    DWORD   bfEndpointNumber:4; // bits 7-10
    DWORD   bfDirection:2;      // bits 11-12
    DWORD   bfIsLowSpeed:1;     // bit 13
    DWORD   bfSkip:1;           // bit 14
    DWORD   bfIsIsochronous:1;  // bit 15
    DWORD   bfMaxPacketSize:11; // bits 16-26
    DWORD   bHostEndPointNum:5; // bits 27-31
#else
    UINT8   bfFunctionAddress;
    UINT8   bfEndpointNumber;
    UINT8   bfDirection;
    UINT8   bfAttributes;
    UINT8   bfIsLowSpeed;
    UINT8   bfIsHighSpeed;
    UINT8   bfHubAddress;
    UINT8   bfHubPort;
    /*indicates whether the processing thread should service
    this descriptor or not*/
    UINT8   bfSkip;
    UINT8   bfIsIsochronous;
    UINT16  bfMaxPacketSize;
    UINT8   bHostEndPointNum;
    UINT8   bInterval;
#endif
    ListHead   *HeadTD;
    ListHead   *TailTD;
    /*indicates the status/phase of endpoint descriptor*/
    TransferStatus TransferStatus;
    PIPE_TYPE   epType; /* Denotes the Type of Endpoint CONTROL, BULK, INTERRUPT or ISOCHRONOUS */
    BOOL bfHalted;
    UINT16 bfToggleCarry;
    BOOL bSemaphoreOwner;
    HANDLE hSemaphore;

#ifdef MUSB_USEDMA
    // DMA channel assigned to this ED
    CCppiDmaChannel *pDmaChannel;
#endif // MUSB_USEDMA

} USBED;

typedef enum{
    STAGE_SETUP=0,
    STAGE_DATAIN=1,
    STAGE_DATAOUT=2,
    STAGE_STATUSIN=3,
    STAGE_STATUSOUT=4,
}TransferStage;

typedef struct _USBTD {
    ListHead NextTD;            //compulsaraly this should be the first element in the structure
#if 0
    DWORD :16;                  // reserved, bits 0-15
    DWORD bfDiscard:1;          // reserved, bit 16 (used by HCD, matches ITD))
    DWORD bfIsIsoch:1;          // reserved, bit 17 (used by HCD, matches ITD))
    DWORD bfShortPacketOk:1;    // bit 18
    DWORD bfPID:2;              // bits 19-20
    DWORD bfDelayInterrupt:3;   // bits 21-23
    DWORD bfDataToggle:2;       // bits 24-25
        enum { DATATOG_ED=0, DATATOG_0=2, DATATOG_1=3 };
    DWORD bfErrorCount:2;       // bits 26-27
    DWORD bfConditionCode:4;    // bits 28-31
#endif
    STransfer sTransfer;
    UINT32 dwCurrentPermissions;
    /* indicates the type of transaction to initiate*/
    TransferStage  TransferStage;
    /*number of bytes transferred so far*/
    DWORD BytesTransferred;
    /*number of bytes yet to be transferred*/
    DWORD BytesToTransfer;

    DWORD TXCOUNT;

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO

    BOOL bAwaitingFrame; // Wait on frame flag

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA

} USBTD;

class CHCCArea {
    friend class CPipe;
    friend class CIsochronousPipe;
    friend class CQueuedPipe;
    friend class CControlPipe;
    friend class CInterruptPipe;
    friend class CBulkPipe;
public:
    CHCCArea(IN CPhysMem* const pCPhysMem);
    ~CHCCArea() ;
    BOOL Initialize(IN COhcd *  pCOhcd);
    void         DeInitialize( void );
    BOOL        WaitForPipes(DWORD dwTimeout);
    void        IncrementPipeCount() {InterlockedIncrement(&m_dwPipeCount);};
    void        DecrementPipeCount() {InterlockedDecrement(&m_dwPipeCount);};
private:
    static ULONG CALLBACK CheckForDoneTransfersThreadStub( IN PVOID context );
    ULONG CheckForDoneTransfersThread();

    // ****************************************************
    // Private Variables for CPipe
    // ****************************************************
    volatile LONG m_dwPipeCount;
public:

    void         HandleReclamationLoadChange( IN const BOOL fAddingTransfer  );

    inline ULONG GetQHPhysAddr( IN P_ED virtAddr ) {
        DEBUGCHK( virtAddr != NULL &&
                  m_pCPhysMem->VaToPa( PUCHAR(virtAddr) ) % 16 == 0 );
        return m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
    }

    inline ULONG GetTDPhysAddr( IN const P_TD virtAddr ) {
        DEBUGCHK( virtAddr != NULL );
        return m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
    };

    inline ULONG GetTDPhysAddr( IN const P_ITD virtAddr ) {
        DEBUGCHK( virtAddr != NULL );
        return m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
    };
    // Return the index of the m_interruptQHTree that this frame should point to
#define OHCD_MAX_INTERRUPT_INTERVAL 32
    inline UCHAR QHTreeEntry( IN const DWORD frame ) const
    {
        DEBUGCHK( frame >= 0 );
        // return interval + (frame % interval)
        DEBUGCHK( frame % OHCD_MAX_INTERRUPT_INTERVAL == (frame & (OHCD_MAX_INTERRUPT_INTERVAL - 1)) );
        return UCHAR( OHCD_MAX_INTERRUPT_INTERVAL + (frame & (OHCD_MAX_INTERRUPT_INTERVAL - 1)) );
    }
    CPhysMem *   const   m_pCPhysMem;            // memory object for our TD/QH and Frame list alloc
private:
    // ****************************************************
    // Protected Variables for CPipe
    // ****************************************************

    // schedule related vars
    CRITICAL_SECTION m_csQHScheduleLock;    // crit sec for QH section of schedule
    P_ED m_interruptQHTree[ 2 * OHCD_MAX_INTERRUPT_INTERVAL ];
                                            // array to keep track of the binary tree containing
                                            // the interrupt queue heads.
    P_ED m_pFinalQH; // final QH in the schedule
    P_TD m_pDoneHead; // virt ptr to head of [cached] done queue

#ifdef DEBUG
    int              m_debug_TDMemoryAllocated;
    int              m_debug_QHMemoryAllocated;
    int              m_debug_BufferMemoryAllocated;
    int              m_debug_ControlExtraMemoryAllocated;
#endif // DEBUG
};


class CPipe : public CPipeAbs
{
    friend class CHCCArea;
public:
    // ****************************************************
    // Public Functions for CPipe
    // ****************************************************

    CPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
           IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
           IN const UCHAR bDeviceAddress,
           IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
           IN COhcd * const pCOhcd);

    virtual ~CPipe();

    virtual PIPE_TYPE GetType () { return TYPE_UNKNOWN; };

    virtual HCD_REQUEST_STATUS  OpenPipe( void ) = 0;

    virtual HCD_REQUEST_STATUS  ClosePipe( void ) = 0;

    virtual HCD_REQUEST_STATUS AbortTransfer(
                                IN const LPTRANSFER_NOTIFY_ROUTINE lpCancelAddress,
                                IN const LPVOID lpvNotifyParameter,
                                IN LPCVOID lpvCancelId ) = 0;

    virtual BOOL    CheckForDoneTransfers( TDLINK pCurTD ) = 0;

    virtual void    UpdateListControl(BOOL bEnable, BOOL bFill) = 0;

    HCD_REQUEST_STATUS IsPipeHalted( OUT LPBOOL const lpbHalted );

    void ClearHaltedFlag( void );

    P_ED GetED() { return m_pED; };
    USBED* GetUSBED() { return m_pUSBED; };

    // ****************************************************
    // Public Variables for CPipe
    // ****************************************************
    UCHAR const m_bEndpointAddress;
    UCHAR const& m_bBusAddress;
	
    // Private instance variables

//#ifdef DEBUG
    virtual const TCHAR*  GetPipeType( void ) const = 0;
//#endif // DEBUG
    UCHAR const m_bHubAddress;
    UCHAR const m_bHubPort;

    COhcd * const m_pCOhcd;
    PVOID const m_TTContext;

protected:
    UCHAR m_private_address;    // maintains the USB address of this pipe's device
                                //  (default zero; settable once during enumeration)
    // ****************************************************
    // Protected Functions for CPipe
    // ****************************************************
    virtual const int     GetTdSize( void ) const = 0;

    virtual BOOL    AreTransferParametersValid( const STransfer *pTransfer ) const = 0;

    virtual DWORD   GetMemoryAllocationFlags( void ) const;

    virtual HCD_REQUEST_STATUS  ScheduleTransfer( void ) = 0;

    void            FreeTransferMemory( STransfer *pTransfer = NULL );

    virtual ULONG * GetListHead(const BOOL fEnable ) = 0;

    //COhcd * const m_pCOhcd;

	LPCTSTR GetControllerName( void ) const;

    inline ULONG GetQHPhysAddr( IN P_ED virtAddr );

    inline ULONG GetTDPhysAddr( IN const P_TD virtAddr ) ;

    inline ULONG GetTDPhysAddr( IN const P_ITD virtAddr );


    P_ED           m_pED;                // virt ptr to OHCI ED for this pipe
    USBED*     m_pUSBED;

    // pipe specific variables
    CRITICAL_SECTION        m_csPipeLock;           // crit sec for this specific pipe's variables
    USB_ENDPOINT_DESCRIPTOR m_usbEndpointDescriptor; // descriptor for this pipe's endpoint
    BOOL                    m_fIsLowSpeed;          // indicates speed of this pipe
    BOOL                    m_fIsHighSpeed;         // Indicates speed of this Pipe;
    BOOL                    m_fIsHalted;            // indicates pipe is halted
    BOOL                    m_fTransferInProgress;  // indicates if this pipe is currently executing a transfer
    // WARNING! These parameters are treated as a unit. They
    // can all be wiped out at once, for example when a
    // transfer is aborted.

	CPipe& operator=(CPipe&) {ASSERT(FALSE);}
};

class CQueuedPipe : public CPipe
{

public:
    // ****************************************************
    // Public Functions for CQueuedPipe
    // ****************************************************
    CQueuedPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
                 IN COhcd * const pCOhcd);
    virtual ~CQueuedPipe();

    inline const int GetTdSize( void ) const { return sizeof(TD); };

    HCD_REQUEST_STATUS ClosePipe( void );

    virtual HCD_REQUEST_STATUS  IssueTransfer(
                                IN const UCHAR address,
                                IN LPTRANSFER_NOTIFY_ROUTINE const lpfnCallback,
                                IN LPVOID const lpvCallbackParameter,
                                IN const DWORD dwFlags,
                                IN LPCVOID const lpvControlHeader,
                                IN const DWORD dwStartingFrame,
                                IN const DWORD dwFrames,
                                IN LPCDWORD const aLengths,
                                IN const DWORD dwBufferSize,
                                IN_OUT LPVOID const lpvBuffer,
                                IN const ULONG paBuffer,
                                IN LPCVOID const lpvCancelId,
                                OUT LPDWORD const adwIsochErrors,
                                OUT LPDWORD const adwIsochLengths,
                                OUT LPBOOL const lpfComplete,
                                OUT LPDWORD const lpdwBytesTransferred,
                                OUT LPDWORD const lpdwError );

    HCD_REQUEST_STATUS AbortTransfer(
                                IN const LPTRANSFER_NOTIFY_ROUTINE lpCancelAddress,
                                IN const LPVOID lpvNotifyParameter,
                                IN LPCVOID lpvCancelId );

    virtual BOOL    CheckForDoneTransfers( TDLINK pCurTD );
    // ****************************************************
    // Public Variables for CQueuedPipe
    // ****************************************************

private:
    // ****************************************************
    // Private Functions for CQueuedPipe
    // ****************************************************
    void  AbortQueue( void );

    // ****************************************************
    // Private Variables for CQueuedPipe
    // ****************************************************

protected:
    // ****************************************************
    // Protected Functions for CQueuedPipe
    // ****************************************************

    BOOL CheckForDoneTransfers( void );

    virtual void UpdateInterruptQHTreeLoad( IN const UCHAR branch,
                                            IN const int   deltaLoad );

    // ****************************************************
    // Protected Variables for CQueuedPipe
    // ****************************************************
    CTransfer *m_pTransfer; // Parameters for transfer on pipe
    CTransfer *m_pFirstTransfer; // Ptr to first transfer in queue
    CTransfer *m_pLastTransfer; // ptr to last transfer in queue

	CQueuedPipe operator=(CQueuedPipe&);
};

class CBulkPipe : public CQueuedPipe
{
public:
    // ****************************************************
    // Public Functions for CBulkPipe
    // ****************************************************
    CBulkPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
               IN COhcd * const pCOhcd);
    ~CBulkPipe();

    virtual PIPE_TYPE GetType () { return TYPE_BULK; };

    virtual void    UpdateListControl(BOOL bEnable, BOOL bFill) ;

    HCD_REQUEST_STATUS    OpenPipe( void );
    // ****************************************************
    // Public variables for CBulkPipe
    // ****************************************************

//#ifdef DEBUG
    const TCHAR*  GetPipeType( void ) const
    {
        return TEXT("Bulk");
    }
//#endif // DEBUG
//    USHORT              GetNumTDsNeeded( const STransfer *pTransfer = NULL ) const;

private:
    // ****************************************************
    // Private Functions for CBulkPipe
    // ****************************************************

    virtual ULONG * GetListHead( IN const BOOL fEnable );

    BOOL                AreTransferParametersValid( const STransfer *pTransfer  ) const;

    HCD_REQUEST_STATUS  ScheduleTransfer( void );

    // ****************************************************
    // Private variables for CBulkPipe
    // ****************************************************

    CBulkPipe operator=(CBulkPipe&);
};

class CControlPipe : public CQueuedPipe
{
public:
    // ****************************************************
    // Public Functions for CControlPipe
    // ****************************************************
    CControlPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
               IN COhcd * const pCOhcd);
    ~CControlPipe();

    virtual PIPE_TYPE GetType () { return TYPE_CONTROL; };

    HCD_REQUEST_STATUS  OpenPipe( void );

    virtual void    UpdateListControl(BOOL bEnable, BOOL bFill) ;

    void                ChangeMaxPacketSize( IN const USHORT wMaxPacketSize );

    // ****************************************************
    // Public variables for CControlPipe
    // ****************************************************

//#ifdef DEBUG
    const TCHAR*  GetPipeType( void ) const
    {
        static const TCHAR* cszPipeType = TEXT("Control");
        return cszPipeType;
    }
//#endif // DEBUG
private:
    // ****************************************************
    // Private Functions for CControlPipe
    // ****************************************************

    BOOL                AreTransferParametersValid( const STransfer *pTransfer ) const;

    HCD_REQUEST_STATUS  ScheduleTransfer( void );

    virtual ULONG * GetListHead( IN const BOOL fEnable );

    // ****************************************************
    // Private variables for CControlPipe
    // ****************************************************
};

class CInterruptPipe : public CQueuedPipe
{
public:
    // ****************************************************
    // Public Functions for CInterruptPipe
    // ****************************************************
    CInterruptPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
               IN COhcd * const pCOhcd);
    ~CInterruptPipe();

    HCD_REQUEST_STATUS    OpenPipe( void );

    virtual PIPE_TYPE GetType () { return TYPE_INTERRUPT; };

    virtual void    UpdateListControl(BOOL bEnable, BOOL bFill);

    // ****************************************************
    // Public variables for CInterruptPipe
    // ****************************************************

//#ifdef DEBUG
    const TCHAR*  GetPipeType( void ) const
    {
        static const TCHAR* cszPipeType = TEXT("Interrupt");
        return cszPipeType;
    }
//#endif // DEBUG
private:
    // ****************************************************
    // Private Functions for CInterruptPipe
    // ****************************************************


    // ComputeLoad caller must already hold the QHSchedule critsec!
    //int                 ComputeLoad( IN const int branch ) const;

    void                UpdateInterruptQHTreeLoad( IN const UCHAR branch,
                                                   IN const int   deltaLoad );

    //USHORT              GetNumTDsNeeded( const STransfer *pTransfer = NULL ) const;

    virtual ULONG * GetListHead( IN const BOOL fEnable );

    BOOL                AreTransferParametersValid( const STransfer *pTransfer  ) const;

    HCD_REQUEST_STATUS  ScheduleTransfer( void );

    // ****************************************************
    // Private variables for CInterruptPipe
    // ****************************************************

    int m_iListHead;         // index of the list head for the list on which this pipe resides

    CInterruptPipe operator=(CInterruptPipe&);
};

class CIsochronousPipe : public CQueuedPipe
{
public:
    // ****************************************************
    // Public Functions for CIsochronousPipe
    // ****************************************************
    CIsochronousPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
               IN COhcd *const pCOhcd);
    ~CIsochronousPipe();

    virtual PIPE_TYPE GetType () { return TYPE_ISOCHRONOUS; };

    virtual void UpdateListControl(BOOL bEnable, BOOL bFill) ;

    HCD_REQUEST_STATUS  OpenPipe( void );


    // ****************************************************
    // Public variables for CIsochronousPipe
    // ****************************************************


//#ifdef DEBUG
    const TCHAR*  GetPipeType( void ) const
    {
        static const TCHAR* cszPipeType = TEXT("Isochronous");
        return cszPipeType;
    }
//#endif // DEBUG

private:

    // ****************************************************
    // Private Functions for CIsochronousPipe
    // ****************************************************
    virtual ULONG * GetListHead( IN const BOOL fEnable );

    BOOL AreTransferParametersValid( const STransfer *pTransfer  ) const;

    HCD_REQUEST_STATUS  ScheduleTransfer( void );
};

#endif // __CPIPE_HPP__
