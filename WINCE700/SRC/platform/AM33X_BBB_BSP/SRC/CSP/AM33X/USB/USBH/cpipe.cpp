// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  cpipe.cpp
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
//     CPipe.cpp
// Abstract:
//     Implements the Pipe class for managing open pipes for OHCI
//
//                             CPipe (ADT)
//                           /             \
//                  CQueuedPipe (ADT)       CIsochronousPipe
//                /         |       \
//              /           |         \
//   CControlPipe    CInterruptPipe    CBulkPipe
//
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

#pragma warning(push)
#pragma warning(disable: 4201 4510 4512 4610)
#include "cpipe.hpp"
#include <cphysmem.hpp>
#include <cohcd.hpp>
#include "chw.hpp"
#pragma warning(pop)

// Macros to enable extra logging
#define ITMSG     //RETAILMSG
#define ABORTMSG  //RETAILMSG

#ifndef _PREFAST_
#pragma warning(disable: 4068) // Disable pragma warnings
#endif


CHCCArea::CHCCArea(IN CPhysMem* const pCPhysMem)
:m_pCPhysMem(pCPhysMem)
{
    // Initialise parameters
    m_pFinalQH = NULL;
#ifdef DEBUG
    m_debug_TDMemoryAllocated = 0;
    m_debug_QHMemoryAllocated = 0;
    m_debug_BufferMemoryAllocated = 0;
    m_debug_ControlExtraMemoryAllocated = 0;
#endif // DEBUG
    m_dwPipeCount = 0;
    m_pDoneHead = 0;
    InitializeCriticalSection( &m_csQHScheduleLock );
}
CHCCArea::~CHCCArea()
{
    DeInitialize();
    DeleteCriticalSection( &m_csQHScheduleLock );
}

// *****************************************************************
// Scope: public static
BOOL CHCCArea::Initialize(IN COhcd * pCOhcd)
//
// Purpose: Initialize CPipe's static variables. This
//          also sets up the original empty schedule
//          with the frame list, and interrupt Queue Head tree.
//          We also set up a thread for processing done transfers
//
// Parameters: pCPhysMem - pointer to memory manager object
//
// Returns: TRUE - if everything initialized ok
//          FALSE - in case of failure
//
// Notes: This function is only called from the COhcd::Initialize routine.
//        It should only be called once, to initialize the static variables
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE,(L"+CHCCArea::Initialize\n\r"));
    UNREFERENCED_PARAMETER(pCOhcd);
    DEBUGCHK(m_dwPipeCount == 0);
    DEBUGMSG( ZONE_PIPE,(L"-CHCCArea::Initialize\n\r"));
    return TRUE;
}

// *****************************************************************
// Scope: public static
void CHCCArea::DeInitialize( void )
//
// Purpose: DeInitialize any static variables
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: This function should only be called from the ~COhcd()
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE,(L"+CHCCArea::DeInitialize\n\r"));

    // wait for pipe count to reduce to zero...
    WaitForPipes(3000);

    DEBUGMSG( ZONE_PIPE,(L"-CHCCArea::DeInitialize\n\r"));
}

// *****************************************************************
// Scope: public static
BOOL CHCCArea::WaitForPipes(DWORD dwTimeout)
//
// Purpose: Wait for pipe count to reach zero
//
// Parameters: None
//
// Returns: TRUE if pipe count hit 0 within dwTimeout milliseconds
//
// Notes: 
// ******************************************************************
{
    BOOL rc = FALSE;
    DWORD tc0 = GetTickCount();
    DEBUGMSG( ZONE_PIPE,(L"+CHCCArea::WaitForPipes(%d)\n\r", dwTimeout));

    while ((m_dwPipeCount) && ((GetTickCount() - tc0) < dwTimeout)) 
    {
        DEBUGMSG( ZONE_PIPE,(L"CHCCArea::WaitForPipes: m_dwPipeCount = %d\n\r", m_dwPipeCount));
        Sleep(10);
    }

    if(m_dwPipeCount == 0)
        rc = TRUE;
    else
        DEBUGMSG(ZONE_WARNING, (L"CHCCArea::WaitForPipes: timed out waiting for pipes to close\n\r"));

    DEBUGMSG( ZONE_PIPE,(L"-CHCCArea::WaitForPipes(%d)\n\r", rc));
    return rc;
}

#if defined(MIPSIV)
#define __OPT_PLAT_FLAG TRUE
#else   // defined(MIPSIV)
#define __OPT_PLAT_FLAG FALSE
#endif  // defined(MIPSIV)
#define __OPT_BUGNUMSTRING "27332"
#define __OPT_VER_RESTORE
#include "optimizer.h"

// ******************************************************************
// Scope: protected static
void CHCCArea::HandleReclamationLoadChange( IN const BOOL fAddingTransfer  )
//
// Purpose: This function is called whenever transfers which use bandwidth
//          reclamation (high speed Bulk/Control) are added/removed.
//          If there are transfers for which reclamation is needed, this
//          function will clear the termination bit of m_pFinalQH. Otherwise,
//          it will set the termination bit to prevent infinite QH processing.
//
// Parameters: fAddingTransfer - if TRUE, a reclamation transfer is being added
//                               to the schedule. If FALSE, a reclamation transfer
//                               has just finished/aborted
//
// Returns: Nothing
//
// Notes:
// ******************************************************************
{
    // important that this be static - this variable tracks the total
    // number of reclamation transfers in progress on the USB
    DEBUGMSG( ZONE_TRANSFER, (TEXT("+CHCCArea::HandleReclamationLoadChange - %s\n\r"), ((fAddingTransfer) ? TEXT("Add Reclamation Transfer") : TEXT("Remove Reclamation Transfer")) ) );
    UNREFERENCED_PARAMETER(fAddingTransfer);
    DEBUGMSG( ZONE_TRANSFER, (TEXT("-CHCCArea::HandleReclamationLoadChange - %s\n\r"), ((fAddingTransfer) ? TEXT("Add Reclamation Transfer") : TEXT("Remove Reclamation Transfer")) ) );
}


// ******************************************************************
// Scope: public
CPipe::CPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
           IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
           IN const UCHAR bDeviceAddress,
           IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
              IN COhcd * const pCOhcd)
//
// Purpose: constructor for CPipe
//
// Parameters: lpEndpointDescriptor - pointer to endpoint descriptor for
//                                    this pipe (assumed non-NULL)
//
//             fIsLowSpeed - indicates if this pipe is low speed
//
// Returns: Nothing.
//
// Notes: Most of the work associated with setting up the pipe
//        should be done via OpenPipe. The constructor actually
//        does very minimal work.
//
//        Do not modify static variables here!!!!!!!!!!!
// ******************************************************************
: CPipeAbs(lpEndpointDescriptor->bEndpointAddress )
, m_usbEndpointDescriptor( *lpEndpointDescriptor )
, m_pCOhcd(pCOhcd)
, m_fIsLowSpeed( !!fIsLowSpeed ) // want to ensure m_fIsLowSpeed is 0 or 1
, m_fIsHighSpeed( !!fIsHighSpeed)
, m_fIsHalted( FALSE )
, m_bEndpointAddress( lpEndpointDescriptor->bEndpointAddress )
, m_private_address( bDeviceAddress )
, m_bBusAddress( m_private_address )
, m_pED( NULL )
, m_bHubAddress (bHubAddress)
, m_bHubPort (bHubPort)
, m_TTContext(ttContext)
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CPipe::CPipe\n\r")) );
    // CPipe::Initialize should already have been called by now
    // to set up the schedule and init static variables

    InitializeCriticalSection( &m_csPipeLock );
    m_fTransferInProgress= FALSE;

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CPipe::CPipe\n\r")) );
}

// ******************************************************************
// Scope: public virtual
CPipe::~CPipe( )
//
// Purpose: Destructor for CPipe
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes:   Most of the work associated with destroying the Pipe
//          should be done via ClosePipe
//
//          Do not delete static variables here!!!!!!!!!!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CPipe::~CPipe\n\r")) );

    // transfers should be aborted or closed before deleting object
    DEBUGCHK( m_fTransferInProgress == FALSE );
    DeleteCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CPipe::~CPipe\n\r")) );
}

inline ULONG CPipe::GetQHPhysAddr( IN P_ED virtAddr ) {
        DEBUGCHK( virtAddr != NULL &&
                  m_pCOhcd->CHCCArea::m_pCPhysMem->VaToPa( PUCHAR(virtAddr) ) % 16 == 0 );
        return m_pCOhcd->m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
}

inline ULONG CPipe::GetTDPhysAddr( IN const P_TD virtAddr ) {
        DEBUGCHK( virtAddr != NULL );
        return m_pCOhcd->CHCCArea::m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
    };

inline ULONG CPipe::GetTDPhysAddr( IN const P_ITD virtAddr ) {
        DEBUGCHK( virtAddr != NULL );
        return m_pCOhcd->CHCCArea::m_pCPhysMem->VaToPa( PUCHAR(virtAddr) );
};


// ******************************************************************
// Scope: public
HCD_REQUEST_STATUS CPipe::IsPipeHalted( OUT LPBOOL const lpbHalted )
//
// Purpose: Return whether or not this pipe is halted (stalled)
//
// Parameters: lpbHalted - pointer to BOOL which receives
//                         TRUE if pipe halted, else FALSE
//
// Returns: requestOK
//
// Notes:  Caller should check for lpbHalted to be non-NULL
// ******************************************************************
{
    USBED *pED = GetUSBED();
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CPipe(%s)::IsPipeHalted\n\r"), GetPipeType()) );

    DEBUGCHK( lpbHalted ); // should be checked by COhcd

    EnterCriticalSection( &m_csPipeLock );
    if (lpbHalted!=NULL)
        *lpbHalted = pED->bfHalted;
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CPipe(%s)::IsPipeHalted, *lpbHalted = %d, returning HCD_REQUEST_STATUS %d\n\r"), GetPipeType(), *lpbHalted, requestOK) );
    return requestOK;
}

// ******************************************************************
// Scope: public
void CPipe::ClearHaltedFlag( void )
//
// Purpose: Clears the pipe is halted flag
//
// Parameters: None
//
// Returns: Nothing
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CPipe(%s)::ClearHaltedFlag\n\r"), GetPipeType() ) );

    EnterCriticalSection( &m_csPipeLock );
    m_pUSBED->bfHalted = FALSE;
    m_pUSBED->bfToggleCarry = FALSE;
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CPipe(%s)::ClearHaltedFlag\n\r"), GetPipeType()) );
}

// ******************************************************************
// Scope: protected virtual
DWORD CPipe::GetMemoryAllocationFlags( void ) const
//
// Purpose: Get flags for allocating memory from the CPhysMem class.
//          Descended pipes can over-ride this if they want to
//          specify different memory alloc flags (i.e. block or not,
//          high priority or not, etc)
//
// Parameters: None
//
// Returns: DWORD representing memory allocation flags
//
// Notes:
// ******************************************************************
{
    // default choice - not high priority, no blocking
    return CPHYSMEM_FLAG_NOBLOCK;
}

LPCTSTR CPipe::GetControllerName( void ) const
{
    return m_pCOhcd->GetControllerName();
}

// ******************************************************************
// Scope: public
CQueuedPipe::CQueuedPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
                          IN COhcd * const pCOhcd)
//
// Purpose: Constructor for CQueuedPipe
//
// Parameters: See CPipe::CPipe
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
: CPipe( lpEndpointDescriptor, fIsLowSpeed,fIsHighSpeed, bDeviceAddress,bHubAddress,bHubPort,ttContext, pCOhcd )   // constructor for base class
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CQueuedPipe::CQueuedPipe\n\r")) );
    m_pTransfer = NULL;
    m_pLastTransfer = m_pFirstTransfer = NULL;
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CQueuedPipe::CQueuedPipe\n\r")) );
}

// ******************************************************************
// Scope: public virtual
CQueuedPipe::~CQueuedPipe( )
//
// Purpose: Destructor for CQueuedPipe
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CQueuedPipe::~CQueuedPipe\n\r")) );
    // queue should be freed via ClosePipe before calling destructor
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CQueuedPipe::~CQueuedPipe\n\r")) );
}

// ******************************************************************
// Scope: public (Implements CPipe::ClosePipe = 0)
HCD_REQUEST_STATUS CQueuedPipe::ClosePipe( void )
//
// Purpose: Abort any transfers associated with this pipe, and
//          remove its data structures from the schedule
//
// Parameters: None
//
// Returns: requestOK
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS status = requestOK;
    USBED *pED;
    USBTD *pTD;

    DEBUGMSG( ZONE_PIPE, (TEXT("+CQueuedPipe(%s)::ClosePipe\n\r"), GetPipeType() ) );
    // Delete outstanding transfer first.
    EnterCriticalSection( &m_csPipeLock );
    m_pCOhcd->LockProcessingThread();

    pED = GetUSBED();
    /*stop processing transfer request on this endpoint*/
    pED->bfSkip = 1;

#ifdef MUSB_USEDMA

    // Abort any pending DMA operations and release
    // DMA channel
    if (pED->pDmaChannel)
    {
        m_pCOhcd->m_dmaCrtl.ReleaseChannel(pED->pDmaChannel);
        pED->pDmaChannel = NULL;
    }

#endif // MUSB_USEDMA

    /*free the NULL TD*/
    m_pCOhcd->RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pED->TailTD);
    m_pCOhcd->FreeTD((USBTD*)pED->TailTD);


    /*free all the TD except the first TD of that endpoint*/
    if(pED->HeadTD){
        pTD = (USBTD*)pED->HeadTD->next;
        while(pTD){
            m_pCOhcd->RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD);
            *pTD->sTransfer.lpfComplete = TRUE;
            *pTD->sTransfer.lpdwError = USB_CANCELED_ERROR;
            if (pTD->sTransfer.lpfnCallback ) {
                ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                pTD->sTransfer.lpfnCallback =NULL;
            }
            m_pCOhcd->FreeTD(pTD);
            pTD = (USBTD*)pED->HeadTD->next;
        }
    }

    LeaveCriticalSection( &m_csPipeLock );

    /*free the first TD of that endpoint*/
    LPVOID lpCancelId;
    do {
        EnterCriticalSection( &m_csPipeLock );
        pTD = (USBTD*)pED->HeadTD;

        if ( pTD) {
            lpCancelId = (LPVOID)(pTD->sTransfer.lpvCancelId);
        }
        else
            lpCancelId = NULL;

        LeaveCriticalSection( &m_csPipeLock );
        if (lpCancelId!=NULL)
            AbortTransfer( NULL, // callback function
                           NULL, // callback param
                           lpCancelId);
    }while (lpCancelId!=NULL);

    if(pTD)
        m_pCOhcd->FreeTD(pTD);

    /*free ED*/

    switch(GetType())
    {
    case TYPE_CONTROL:
        m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pControlHead, (ListHead*)pED);
        break;
    case TYPE_BULK:
        if(pED->bfDirection == TD_IN_PID)
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pBulkInHead, (ListHead*)pED);
        else
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pBulkOutHead, (ListHead*)pED);
        break;

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO

    case TYPE_ISOCHRONOUS:
        if(pED->bfDirection == TD_IN_PID)
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pIsoInHead, (ListHead*)pED);
        else
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pIsoOutHead, (ListHead*)pED);
        break;

#endif MUSB_USEDMA_FOR_ISO
#endif MUSB_USEDMA

    case TYPE_INTERRUPT:
        if(pED->bfDirection == TD_IN_PID)
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pIntInHead, (ListHead*)pED);
        else
            m_pCOhcd->RemoveElementFromList((ListHead**)&m_pCOhcd->m_pIntOutHead, (ListHead*)pED);
        break;
    }

    m_pCOhcd->FreeED(pED);

    m_pCOhcd->FreeHostEndPoint(pED->bHostEndPointNum, USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress));

    m_pCOhcd->UnlockProcessingThread();
    
    m_pCOhcd->DecrementPipeCount();

    DEBUGMSG( ZONE_PIPE, (TEXT("-CQueuedPipe(%s)::ClosePipe\n\r"), GetPipeType() ) );
    return status;
}
// ******************************************************************
// Scope: public
HCD_REQUEST_STATUS CQueuedPipe::IssueTransfer(
                                    IN const UCHAR address,
                                    IN LPTRANSFER_NOTIFY_ROUTINE const lpStartAddress,
                                    IN LPVOID const lpvNotifyParameter,
                                    IN const DWORD dwFlags,
                                    IN LPCVOID const lpvControlHeader,
                                    IN const DWORD dwStartingFrame,
                                    IN const DWORD dwFrames,
                                    IN LPCDWORD const aLengths,
                                    IN const DWORD dwBufferSize,
                                    IN_OUT LPVOID const lpvClientBuffer,
                                    IN const ULONG paBuffer,
                                    IN LPCVOID const lpvCancelId,
                                    OUT LPDWORD const adwIsochErrors,
                                    OUT LPDWORD const adwIsochLengths,
                                    OUT LPBOOL const lpfComplete,
                                    OUT LPDWORD const lpdwBytesTransferred,
                                    OUT LPDWORD const lpdwError )
//
// Purpose: Issue a Transfer on this pipe
//
// Parameters: address - USB address to send transfer to
//
//             OTHER PARAMS - see comment in COhcd::IssueTransfer
//
// Returns: requestOK if transfer issued ok, else requestFailed
//
// Notes:
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER, (TEXT("+CQueuedPipe(%s)::IssueTransfer, address = %d\n\r"), GetPipeType(), address) );

    HCD_REQUEST_STATUS status = requestFailed;
    USBTD *pTD;
    USBED *pED;
    STransfer *psTransfer;

    // Go safe
    EnterCriticalSection( &m_csPipeLock );

    m_pCOhcd->LockProcessingThread();

    // Allocate TD
    pTD = m_pCOhcd->AllocateTD();
    if (pTD)
    {
        DWORD dwActStartingFrame = dwStartingFrame;

        // Get ED for this endpoint
        pED = GetUSBED();
        if (!pED || !pED->HeadTD || !pED->TailTD)
        {
            RETAILMSG(1, (L"\r\nCorrupted queue!!!\r\n"));
            SetLastError(ERROR_CAN_NOT_COMPLETE);
            m_pCOhcd->FreeTD(pTD);
            goto _done;
        }

        ITMSG(1, (L"IT: EP%d %s %s, len %d\r\n",
                  pED->bHostEndPointNum,
                  GetType()==TYPE_CONTROL ? L"CTRL" :
                  GetType()==TYPE_ISOCHRONOUS ? L"ISO" :
                  GetType()==TYPE_BULK ? L"BULK" : L"INT",
                  pED->bfDirection == TD_IN_PID ? L"IN" : L"OUT",
                  dwBufferSize));

        // Any EP type specific code goes here
        switch (GetType())
        {
        // Control transfers
        case TYPE_CONTROL:

            // Control transfers start in setup stage
            ((_USBTD*)pED->TailTD)->TransferStage = STAGE_SETUP;
            break;

        case TYPE_ISOCHRONOUS:

            DEBUGCHK(dwFrames > 0);
            DEBUGCHK(aLengths != NULL);

            {
                DWORD dwCurrentFrame;

                // Grab current frame number
                m_pCOhcd->GetFrameNumber(&dwCurrentFrame);

                // Adjust frame number for ISO ASAP transfers
                if (dwFlags & USB_START_ISOCH_ASAP)
                {
                    // See if there are another pending TDs
                    _USBTD *pTemp = (USBTD *)pED->HeadTD;

                    if (pTemp != (USBTD *)pED->TailTD)
                    {
                        // There is another pending TD on the list
                        while (pTemp->NextTD.next != pED->TailTD)
                        {
                            pTemp = (USBTD *)pTemp->NextTD.next;
                        }

                        // Calc start frame based on the previous TD
                        dwActStartingFrame = pTemp->sTransfer.dwStartingFrame +
                            pTemp->sTransfer.dwFrames;
                    }
                    else
                    {
                        // Calc start frame based on current frame number
                        dwActStartingFrame = dwCurrentFrame;
                    }

                    // Ensure there is enough set up time
                    if (dwActStartingFrame < dwCurrentFrame + ISO_MIN_ADVANCED_FRAME)
                        dwActStartingFrame = dwCurrentFrame + ISO_MIN_ADVANCED_FRAME;
                }

                // Make sure we are not behind current frame
                if (dwActStartingFrame < dwCurrentFrame)
                {
                    RETAILMSG( 1, (TEXT("\r\nCQueuedPipe::IssueTransfer: ISO transfer cannot meet the schedule! CurFrame %08X, StartFrame %08X\r\n"),
                        dwCurrentFrame, dwActStartingFrame));

                    SetLastError(ERROR_CAN_NOT_COMPLETE);
                    m_pCOhcd->FreeTD(pTD);
                    goto _done;
                }
            }

            // Don't break here, fall down to the bulk & intr

        // Bulk and interrupt transfers
        case TYPE_BULK:
        case TYPE_INTERRUPT:

            if(pED->bfDirection == TD_IN_PID)
                ((_USBTD*)pED->TailTD)->TransferStage = STAGE_DATAIN;
            else
                ((_USBTD*)pED->TailTD)->TransferStage = STAGE_DATAOUT;

            break;
        }

        // Copy all transfer parameters to the NULL TD
        psTransfer = &((_USBTD*)pED->TailTD)->sTransfer;
        psTransfer->address = address;
        psTransfer->lpfnCallback = lpStartAddress;
        psTransfer->lpvCallbackParameter = lpvNotifyParameter;
        psTransfer->dwFlags = dwFlags;
        psTransfer->lpvControlHeader = lpvControlHeader;
        psTransfer->dwStartingFrame = dwActStartingFrame;
        psTransfer->dwFrames = dwFrames;
        psTransfer->aLengths = aLengths;
        psTransfer->dwBufferSize = dwBufferSize;
        psTransfer->lpvClientBuffer = lpvClientBuffer;
        psTransfer->paClientBuffer = paBuffer;
        psTransfer->lpvCancelId = lpvCancelId;
        psTransfer->adwIsochErrors = adwIsochErrors;
        psTransfer->adwIsochLengths = adwIsochLengths;
        psTransfer->lpfComplete = lpfComplete;
        psTransfer->lpdwBytesTransferred = lpdwBytesTransferred;
        psTransfer->lpdwError = lpdwError;
        *lpfComplete = FALSE;
        *lpdwBytesTransferred = 0;
        *lpdwError = USB_NOT_COMPLETE_ERROR;
        ((_USBTD*)pED->TailTD)->BytesToTransfer = dwBufferSize;
        ((_USBTD*)pED->TailTD)->BytesTransferred = 0;
        // Make the newely allocated TD as the NULL TD
        pED->TailTD->next = (ListHead*)pTD;
        pED->TailTD = (ListHead*)pTD;
        status = requestOK;
        SetEvent(m_pCOhcd->m_hUsbProcessingEvent);
    }
    else
    {
        DEBUGMSG( ZONE_TRANSFER, (TEXT("CQueuedPipe::IssueTransfer: failed to allocate TD!\n\r")));
        SetLastError(ERROR_CAN_NOT_COMPLETE);
    }


_done:

    // Go unsafe

    m_pCOhcd->UnlockProcessingThread();

    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_TRANSFER, (TEXT("-CQueuedPipe(%s)::IssueTransfer - address = %d, returning STATUS %d\n\r"), GetPipeType(), address, status) );

    return status;
}

// ******************************************************************
// Scope: public (implements CPipe::AbortTransfer = 0)
HCD_REQUEST_STATUS CQueuedPipe::AbortTransfer(
                                IN const LPTRANSFER_NOTIFY_ROUTINE lpCancelAddress,
                                IN const LPVOID lpvNotifyParameter,
                                IN LPCVOID lpvCancelId )
//
// Purpose: Abort any transfer on this pipe if its cancel ID matches
//          that which is passed in.
//
// Parameters: lpCancelAddress - routine to callback after aborting transfer
//
//             lpvNotifyParameter - parameter for lpCancelAddress callback
//
//             lpvCancelId - identifier for transfer to abort
//
// Returns: requestOK if transfer aborted
//          requestFailed if lpvCancelId doesn't match currently executing
//                 transfer, or if there is no transfer in progress
//
// Notes:
// ******************************************************************

{
    DEBUGMSG( ZONE_TRANSFER, (TEXT("+CQueuedPipe(%s)::AbortTransfer - lpvCancelId = 0x%x\n\r"), GetPipeType(), lpvCancelId) );

    HCD_REQUEST_STATUS status = requestFailed;
    //CTransfer         *pTransfer = NULL;
    USBTD *pTD=NULL;
    USBED *pED = GetUSBED();

    EnterCriticalSection( &m_csPipeLock );

    m_pCOhcd->LockProcessingThread();

    pED->bfSkip = 1;

//redo:
    if (pED->HeadTD )
    {
        if(((USBTD*)pED->HeadTD)->sTransfer.lpvCancelId == lpvCancelId)
        {
            ABORTMSG(1, (L"AB: EP%d %s %s\r\n",
                         pED->bHostEndPointNum,
                         GetType()==TYPE_CONTROL ? L"CTRL" :
                         GetType()==TYPE_ISOCHRONOUS ? L"ISO" :
                         GetType()==TYPE_BULK ? L"BULK" : L"INT",
                         pED->bfDirection == TD_IN_PID ? L"IN" : L"OUT"
                         ));

#ifdef MUSB_USEDMA

            // Abort any pending DMA I/O
            if (pED->pDmaChannel)
            {
                pED->pDmaChannel->CancelTransfer();
            }

#endif // MUSB_USEDMA

            /*ID matches with the ID of the first TD of ED and
            therefore may be getting processed by UsbProcessingThread
            that way we need to stop the Endpoint*/
            if(GetType() == TYPE_BULK || GetType() == TYPE_ISOCHRONOUS)
            {
                if(pED->bSemaphoreOwner)
                {
                    m_pCOhcd->StopHostEndpoint(pED->bHostEndPointNum);
                    ReleaseSemaphore(pED->hSemaphore, 1, NULL );
                    pED->bSemaphoreOwner = FALSE;
                }
            }
            else
            {
                m_pCOhcd->StopHostEndpoint(pED->bHostEndPointNum);
            }

            pTD = (USBTD*)pED->HeadTD;
            //pED->TransferStatus = STATUS_CANCEL;
            pED->TransferStatus = STATUS_IDLE;
            SetEvent(m_pCOhcd->m_hUsbProcessingEvent);
        }
        else
        {
            /*search*/
            pTD = (USBTD*)(((USBTD*)pED->HeadTD)->NextTD.next);
            while(pTD)
            {
                if (pTD->sTransfer.lpvCancelId == lpvCancelId)
                {
                    break;
                }
                else
                {
                    pTD = (USBTD*)pTD->NextTD.next;
                }
            }
        }
    }


    if (pTD)
    {
        RETAILMSG(0, (L"P0 ABT %08X\r\n", pED, pTD));

        m_pCOhcd->RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD);
        *pTD->sTransfer.lpfComplete = TRUE;
        *pTD->sTransfer.lpdwError = USB_CANCELED_ERROR;

        if (pTD->sTransfer.lpfnCallback )
        {
            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
            pTD->sTransfer.lpfnCallback =NULL;
        }

        m_pCOhcd->FreeTD(pTD);

        if ( lpCancelAddress )
        {
            ( *lpCancelAddress )( lpvNotifyParameter );
        }

        status = requestOK;
    }
    else
    {
        // Too late; the transfer already completed.
        RETAILMSG(0, (L"P0 %08X\r\n", pED));
    }

    pED->bfSkip = 0;

    m_pCOhcd->UnlockProcessingThread();

    LeaveCriticalSection( &m_csPipeLock );
    DEBUGMSG( ZONE_TRANSFER, (TEXT("-CQueuedPipe(%s)::AbortTransfer - lpvCancelId = 0x%x, returning HCD_REQUEST_STATUS %d\n\r"), GetPipeType(), lpvCancelId, status) );
    return status;
}

// ******************************************************************
// Scope: private
void  CQueuedPipe::AbortQueue( void )
//
// Purpose: Abort the current transfer (i.e., queue of TDs).
//
// Parameters: pQH - pointer to Queue Head for transfer to abort
//
// Returns: Nothing
//
// Notes: not used for OHCI
// ******************************************************************
{
    DEBUGCHK(0);
}

// ******************************************************************
// Scope: protected (Implements CPipe::CheckForDoneTransfers = 0)
BOOL CQueuedPipe::CheckForDoneTransfers( TDLINK pCurTD )
//
// Purpose: Check if the transfer on this pipe is finished, and
//          take the appropriate actions - i.e. remove the transfer
//          data structures and call any notify routines
//
// Parameters: None
//
// Returns: TRUE if transfer is done, else FALSE
//
// Notes:
// ******************************************************************
{
    BOOL fTransferDone = FALSE;
    DEBUGMSG( ZONE_TRANSFER, (TEXT("+CQueuedPipe(%s)::CheckForDoneTransfers\n\r"), GetPipeType() ) );

    UNREFERENCED_PARAMETER(pCurTD);

    DEBUGMSG( ZONE_TRANSFER, (TEXT("-CQueuedPipe(%s)::CheckForDoneTransfers, returning BOOL %d\n\r"), GetPipeType(), fTransferDone) );
    return fTransferDone;
}


// ******************************************************************
// Scope: protected virtual
void CQueuedPipe::UpdateInterruptQHTreeLoad( IN const UCHAR, // branch - ignored
                                             IN const int ) // deltaLoad - ignored
//
// Purpose: Change the load counts of all members in m_interruptQHTree
//          which are on branch "branch" by deltaLoad. This needs
//          to be done when interrupt QHs are added/removed to be able
//          to find optimal places to insert new queues
//
// Parameters: branch - indicates index into m_interruptQHTree array
//                      after which the branch point occurs (i.e. if
//                      a new queue is inserted after m_interruptQHTree[ 8 ],
//                      branch will be 8)
//
//             deltaLoad - amount by which to alter the Load counts
//                         of affected m_interruptQHTree members
//
// Returns: Nothing
//
// Notes: Currently, there is nothing to do for BULK/CONTROL queues.
//        INTERRUPT should override this function
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CQueuedPipe(%s)::UpdateInterruptQHTreeLoad\n\r"), GetPipeType() ) );
    DEBUGMSG( ZONE_PIPE && ZONE_WARNING, (TEXT("CQueuedPipe(%s)::UpdateInterruptQHTreeLoad - doing nothing\n\r"), GetPipeType()) );
    DEBUGCHK( (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) != USB_ENDPOINT_TYPE_INTERRUPT );
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CQueuedPipe(%s)::UpdateInterruptQHTreeLoad\n\r"), GetPipeType() ) );
}

// ******************************************************************
// Scope: public
CBulkPipe::CBulkPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
                      IN COhcd * const pCOhcd)
//
// Purpose: Constructor for CBulkPipe
//
// Parameters: See CQueuedPipe::CQueuedPipe
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
: CQueuedPipe(lpEndpointDescriptor,fIsLowSpeed, fIsHighSpeed,bDeviceAddress, bHubAddress, bHubPort, ttContext, pCOhcd ) // constructor for base class
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CBulkPipe::CBulkPipe\n\r")) );
    DEBUGCHK( m_usbEndpointDescriptor.bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE &&
              m_usbEndpointDescriptor.bLength >= sizeof( USB_ENDPOINT_DESCRIPTOR ) &&
              (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK );

    DEBUGCHK( !fIsLowSpeed ); // bulk pipe must be high speed

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CBulkPipe::CBulkPipe\n\r")) );
}

// ******************************************************************
// Scope: public
CBulkPipe::~CBulkPipe( )
//
// Purpose: Destructor for CBulkPipe
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CBulkPipe::~CBulkPipe\n\r")) );
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CBulkPipe::~CBulkPipe\n\r")) );
}


// ******************************************************************
// Scope: public (Implements CPipe::OpenPipe = 0)
HCD_REQUEST_STATUS CBulkPipe::OpenPipe( void )
//
// Purpose: Create the data structures necessary to conduct
//          transfers on this pipe
//
// Parameters: None
//
// Returns: requestOK - if pipe opened
//
//          requestFailed - if pipe was not opened
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS RetVal;
    INT32 EndpointNum;
    DEBUGMSG( ZONE_PIPE, (TEXT("+CBulkPipe::OpenPipe\n\r") ) );

    EnterCriticalSection( &m_csPipeLock );

    // if this fails, we have a low speed Bulk device
    // which is not allowed by the USB spec
    DEBUGCHK( !m_fIsLowSpeed );

    EndpointNum = m_pCOhcd->AllocateHostEndPoint(
        (UINT32)GetType(),
        m_usbEndpointDescriptor.wMaxPacketSize,
        USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TRUE : FALSE);

    if(EndpointNum >= 0)
    {
        m_pUSBED = m_pCOhcd->AllocateED();

        if (m_pUSBED)
        {
            m_pUSBED->bHostEndPointNum = (UINT8)EndpointNum;
            m_pUSBED->bfFunctionAddress = m_bBusAddress;
            m_pUSBED->bfEndpointNumber = m_bEndpointAddress;
            m_pUSBED->bfDirection = USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TD_IN_PID : TD_OUT_PID;;
            m_pUSBED->bfIsLowSpeed = 0;
            m_pUSBED->bfIsHighSpeed = m_fIsHighSpeed ? 1 :0;
            m_pUSBED->bfHubAddress = m_bHubAddress;
            m_pUSBED->bfHubPort = m_bHubPort;
            m_pUSBED->bfIsIsochronous =  0;
            m_pUSBED->bfMaxPacketSize = m_usbEndpointDescriptor.wMaxPacketSize;
            m_pUSBED->bfAttributes = m_usbEndpointDescriptor.bmAttributes;
            m_pUSBED->TransferStatus = STATUS_IDLE;
            m_pUSBED->bfHalted = FALSE;
            m_pUSBED->bfToggleCarry = FALSE;
            // USB controller datasheet says interval value must be in range 1-16.
            // Values > 16 result in USB controller hangup.
            m_pUSBED->bInterval = m_usbEndpointDescriptor.bInterval > 0x10 ? 0x00 : m_usbEndpointDescriptor.bInterval;

            m_pCOhcd->ProgramHostEndpoint((UINT32)GetType(), (void *)m_pUSBED);

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_BULK

            // Allocate DMA channel for Bulk transfers
            m_pUSBED->pDmaChannel = m_pCOhcd->m_dmaCrtl.AllocChannel(
                m_pUSBED->bHostEndPointNum,
                m_pUSBED->bfEndpointNumber,
                (PfnTransferComplete)m_pCOhcd->DmaTransferComplete);

#endif // MUSB_USEDMA_FOR_BULK
#endif // MUSB_USEDMA

            if(m_pUSBED->bfDirection == TD_IN_PID)
            {
                m_pUSBED->NextED.next = (ListHead*)(m_pCOhcd->CHW::m_pBulkInHead);
                (m_pCOhcd->CHW::m_pBulkInHead) = (PDWORD)m_pUSBED;
            }
            else
            {
                m_pUSBED->NextED.next = (ListHead*)(m_pCOhcd->CHW::m_pBulkOutHead);
                (m_pCOhcd->CHW::m_pBulkOutHead) = (PDWORD)m_pUSBED;
            }

            RetVal = requestOK;
            m_pCOhcd->IncrementPipeCount();
        }
        else
        {
            DEBUGMSG( ZONE_PIPE, (TEXT("CBulkPipe::OpenPipe: no free EDs!\n\r")));
            RetVal = requestFailed;
        }
    }
    else
    {
        DEBUGMSG( ZONE_PIPE, (TEXT("CBulkPipe::OpenPipe: no free endpoints!\n\r")));
        RetVal = requestFailed;
    }

    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE, (TEXT("-CBulkPipe::OpenPipe, returning HCD_REQUEST_STATUS %d\n\r"), RetVal) );
    return RetVal;
}

ULONG * CBulkPipe::GetListHead( const BOOL fEnable )
{
    UNREFERENCED_PARAMETER(fEnable);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_BULK, fEnable, FALSE);

    // return m_pCOhcd->CHW::m_pBulkHead;
    return NULL;
}

void CBulkPipe::UpdateListControl(BOOL bEnable, BOOL bFill)
{
    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(bFill);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_BULK, bEnable, bFill);
}


// ******************************************************************
// Scope: private (Implements CPipe::AreTransferParametersValid = 0)
BOOL CBulkPipe::AreTransferParametersValid( const STransfer *pTransfer ) const
//
// Purpose: Check whether this class' transfer parameters are valid.
//          This includes checking m_transfer, m_pPipeQH, etc
//
// Parameters: None (all parameters are vars of class)
//
// Returns: TRUE if parameters valid, else FALSE
//
// Notes: Assumes m_csPipeLock already held
// ******************************************************************
{
    if (pTransfer == NULL)
        return FALSE;

    //DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CBulkPipe::AreTransferParametersValid\n\r")) );

    // these parameters aren't used by CBulkPipe, so if they are non NULL,
    // it doesn't present a serious problem. But, they shouldn't have been
    // passed in as non-NULL by the calling driver.
    DEBUGCHK( pTransfer->adwIsochErrors == NULL && // ISOCH
              pTransfer->adwIsochLengths == NULL && // ISOCH
              pTransfer->aLengths == NULL && // ISOCH
              pTransfer->lpvControlHeader == NULL ); // CONTROL
    // this is also not a serious problem, but shouldn't happen in normal
    // circumstances. It would indicate a logic error in the calling driver.
    DEBUGCHK( !(pTransfer->lpfnCallback == NULL && pTransfer->lpvCallbackParameter != NULL) );
    // DWORD                     pTransfer->dwStartingFrame (ignored - ISOCH)
    // DWORD                     pTransfer->dwFrames (ignored - ISOCH)

    BOOL fValid = (
                    pTransfer->address > 0 &&
                    pTransfer->address <= USB_MAX_ADDRESS &&
                    (pTransfer->lpvClientBuffer != NULL || pTransfer->dwBufferSize == 0) &&
                    // paClientBuffer could be 0 or !0
                    pTransfer->lpfComplete != NULL &&
                    pTransfer->lpdwBytesTransferred != NULL &&
                    pTransfer->lpdwError != NULL );

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE && !fValid, (TEXT("!CBulkPipe::AreTransferParametersValid, returning BOOL %d\n\r"), fValid) );
    return fValid;
}

// ******************************************************************
// Scope: private (Implements CPipe::ScheduleTransfer = 0)
HCD_REQUEST_STATUS CBulkPipe::ScheduleTransfer( void )
//
// Purpose: Schedule a USB Transfer on this pipe
//
// Parameters: None (all parameters are in m_transfer)
//
// Returns: requestOK if transfer issued ok, else requestFailed
//
// Notes:
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CBulkPipe::ScheduleTransfer %08x\n\r"), m_pTransfer) );


    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CBulkPipe::ScheduleTransfer, returning HCD_REQUEST_STATUS %d\n\r"), requestOK) );
    return requestOK;
}


// ******************************************************************
// Scope: public
CControlPipe::CControlPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
                            IN COhcd * const pCOhcd)
//
// Purpose: Constructor for CControlPipe
//
// Parameters: See CQueuedPipe::CQueuedPipe
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
: CQueuedPipe( lpEndpointDescriptor, fIsLowSpeed, fIsHighSpeed, bDeviceAddress,bHubAddress, bHubPort, ttContext, pCOhcd ) // constructor for base class
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CControlPipe::CControlPipe\n\r")) );
    DEBUGCHK( m_usbEndpointDescriptor.bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE &&
              m_usbEndpointDescriptor.bLength >= sizeof( USB_ENDPOINT_DESCRIPTOR ) &&
              (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_CONTROL );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CControlPipe::CControlPipe\n\r")) );
}

// ******************************************************************
// Scope: public
CControlPipe::~CControlPipe( )
//
// Purpose: Destructor for CControlPipe
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CControlPipe::~CControlPipe\n\r")) );
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CControlPipe::~CControlPipe\n\r")) );
}

// ******************************************************************
// Scope: public (Implements CPipe::OpenPipe = 0)
HCD_REQUEST_STATUS CControlPipe::OpenPipe( void )
//
// Purpose: Create the data structures necessary to conduct
//          transfers on this pipe
//
// Parameters: None
//
// Returns: requestOK - if pipe opened
//
//          requestFailed - if pipe was not opened
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS RetVal;
    INT32 EndpointNum;
    DEBUGMSG( ZONE_PIPE, (TEXT("+CControlPipe::OpenPipe\n\r") ) );

    EnterCriticalSection( &m_csPipeLock );
    //m_pCOhcd->LockProcessingThread();

    /*last two parameters not valied*/
    EndpointNum = m_pCOhcd->AllocateHostEndPoint(
        (UINT32)GetType(),
        m_usbEndpointDescriptor.wMaxPacketSize,
        TRUE);

    if(EndpointNum >= 0){
        m_pUSBED = m_pCOhcd->AllocateED();
        if (m_pUSBED)
        {
            m_pUSBED->bHostEndPointNum = (UINT8)EndpointNum;
            m_pUSBED->bfFunctionAddress = m_bBusAddress;
            m_pUSBED->bfEndpointNumber = m_bEndpointAddress;
            m_pUSBED->bfDirection = TD_SETUP_PID;
            m_pUSBED->bfIsLowSpeed = m_fIsLowSpeed ? 1 : 0;
            m_pUSBED->bfIsHighSpeed = m_fIsHighSpeed ? 1 :0;
            m_pUSBED->bfHubAddress = m_bHubAddress;
            m_pUSBED->bfHubPort = m_bHubPort;
            m_pUSBED->bfIsIsochronous =  0;
            m_pUSBED->bfMaxPacketSize = m_usbEndpointDescriptor.wMaxPacketSize;
             m_pUSBED->bfAttributes = m_usbEndpointDescriptor.bmAttributes;
            m_pUSBED->TransferStatus = STATUS_IDLE;
            // Control endpoint does not have interval control register
            m_pUSBED->bInterval = 0; // m_usbEndpointDescriptor.bInterval;

            m_pCOhcd->ProgramHostEndpoint((UINT32)GetType(), (void *)m_pUSBED);

            m_pUSBED->NextED.next = (ListHead *)(m_pCOhcd->CHW::m_pControlHead);
            (m_pCOhcd->CHW::m_pControlHead) = (PDWORD)m_pUSBED;

#ifdef MUSB_USEDMA
            // Control pipes do not support DMA transfers
            m_pUSBED->pDmaChannel = NULL;
#endif // MUSB_USEDMA

            RetVal = requestOK;
            m_pCOhcd->IncrementPipeCount();
        }
        else
        {
            DEBUGMSG( ZONE_PIPE, (TEXT("CControlPipe::OpenPipe: no free EDs!\n\r")));
            RetVal = requestFailed;
        }
    }
    else
    {
        DEBUGMSG( ZONE_PIPE, (TEXT("CControlPipe::OpenPipe: no free endpoints!\n\r")));
        RetVal = requestFailed;
    }

    //m_pCOhcd->UnlockProcessingThread();
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE, (TEXT("-CControlPipe::OpenPipe %d\n\r"), RetVal));
    return RetVal;
}

// ******************************************************************
// Scope: public
void CControlPipe::ChangeMaxPacketSize( IN const USHORT wMaxPacketSize )
//
// Purpose: Update the max packet size for this pipe. This should
//          ONLY be done for control endpoint 0 pipes. When the endpoint0
//          pipe is first opened, it has a max packet size of
//          ENDPOINT_ZERO_MIN_MAXPACKET_SIZE. After reading the device's
//          descriptor, the device attach procedure can update the size.
//
// Parameters: wMaxPacketSize - new max packet size for this pipe
//
// Returns: Nothing
//
// Notes:   This function should only be called by the Hub AttachDevice
//          procedure
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CControlPipe::ChangeMaxPacketSize - new wMaxPacketSize = %d\n\r"), wMaxPacketSize) );

    EnterCriticalSection( &m_csPipeLock );
    m_pCOhcd->LockProcessingThread();

    // this pipe should be for endpoint 0, control pipe
    DEBUGCHK( (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_CONTROL &&
              (m_usbEndpointDescriptor.bEndpointAddress & TD_ENDPOINT_MASK) == 0 );
    // update should only be called if the old address was ENDPOINT_ZERO_MIN_MAXPACKET_SIZE
    DEBUGCHK( m_usbEndpointDescriptor.wMaxPacketSize == ENDPOINT_ZERO_MIN_MAXPACKET_SIZE );
    // this function should only be called if we are increasing the max packet size.
    // in addition, the USB spec 1.0 section 9.6.1 states only the following
    // wMaxPacketSize are allowed for endpoint 0
    DEBUGCHK( wMaxPacketSize > ENDPOINT_ZERO_MIN_MAXPACKET_SIZE &&
              (wMaxPacketSize == 16 ||
               wMaxPacketSize == 32 ||
               wMaxPacketSize == 64) );

    if (m_usbEndpointDescriptor.wMaxPacketSize != wMaxPacketSize) {
        m_pUSBED->bfSkip = 1;
        m_usbEndpointDescriptor.wMaxPacketSize = wMaxPacketSize;
        m_pUSBED->bfMaxPacketSize = wMaxPacketSize;
        m_pUSBED->bfSkip = 0;
    }
    ASSERT(m_pUSBED->bfMaxPacketSize == wMaxPacketSize);

    m_pCOhcd->UnlockProcessingThread();
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CControlPipe::ChangeMaxPacketSize - new wMaxPacketSize = %d\n\r"), wMaxPacketSize) );
}
ULONG * CControlPipe::GetListHead( IN const BOOL fEnable )
{
    UNREFERENCED_PARAMETER(fEnable);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_CONTROL, fEnable, FALSE);
    return m_pCOhcd->CHW::m_pControlHead;
};
void CControlPipe::UpdateListControl(BOOL bEnable, BOOL bFill)
{
    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(bFill);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_CONTROL, bEnable, bFill);
}
// ******************************************************************
// Scope: private (Implements CPipe::AreTransferParametersValid = 0)
BOOL CControlPipe::AreTransferParametersValid( const STransfer *pTransfer ) const
//
// Purpose: Check whether this class' transfer parameters are valid.
//          This includes checking m_transfer, m_pPipeQH, etc
//
// Parameters: None (all parameters are vars of class)
//
// Returns: TRUE if parameters valid, else FALSE
//
// Notes: Assumes m_csPipeLock already held
// ******************************************************************
{
    if (pTransfer == NULL)
        return FALSE;

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CControlPipe::AreTransferParametersValid\n\r")) );

    // these parameters aren't used by CControlPipe, so if they are non NULL,
    // it doesn't present a serious problem. But, they shouldn't have been
    // passed in as non-NULL by the calling driver.
    DEBUGCHK( pTransfer->adwIsochErrors == NULL && // ISOCH
              pTransfer->adwIsochLengths == NULL && // ISOCH
              pTransfer->aLengths == NULL ); // ISOCH
    // this is also not a serious problem, but shouldn't happen in normal
    // circumstances. It would indicate a logic error in the calling driver.
    DEBUGCHK( !(pTransfer->lpfnCallback == NULL && pTransfer->lpvCallbackParameter != NULL) );
    // DWORD                     pTransfer->dwStartingFrame; (ignored - ISOCH)
    // DWORD                     pTransfer->dwFrames; (ignored - ISOCH)

    BOOL fValid = (
                    pTransfer->address <= USB_MAX_ADDRESS &&
                    pTransfer->lpvControlHeader != NULL &&
                    pTransfer->lpfComplete != NULL &&
                    pTransfer->lpdwBytesTransferred != NULL &&
                    pTransfer->lpdwError != NULL );
    if ( fValid ) {
        if ( pTransfer->dwFlags & USB_IN_TRANSFER ) {
            fValid = (pTransfer->lpvClientBuffer != NULL &&
                      // paClientBuffer could be 0 or !0
                      pTransfer->dwBufferSize > 0);
        } else {
            fValid = ( (pTransfer->lpvClientBuffer == NULL &&
                        pTransfer->paClientBuffer == 0 &&
                        pTransfer->dwBufferSize == 0) ||
                       (pTransfer->lpvClientBuffer != NULL &&
                        // paClientBuffer could be 0 or !0
                        pTransfer->dwBufferSize > 0) );
        }
    }

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CControlPipe::AreTransferParametersValid, returning BOOL %d\n\r"), fValid) );
    return fValid;
}

// ******************************************************************
// Scope: private (Implements CPipe::ScheduleTransfer = 0)
HCD_REQUEST_STATUS CControlPipe::ScheduleTransfer( void )
//
// Purpose: Schedule a USB Transfer on this pipe
//
// Parameters: None (all parameters are in m_transfer)
//
// Returns: requestOK if transfer issued ok, else requestFailed
//
// Notes:
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CControlPipe::ScheduleTransfer\n\r")) );

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CControlPipe::ScheduleTransfer\n\r")) );
    return requestOK;
}

// ******************************************************************
// Scope: public
CInterruptPipe::CInterruptPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,IN const PVOID ttContext,
                                IN COhcd * const pCOhcd)
//
// Purpose: Constructor for CInterruptPipe
//
// Parameters: See CQueuedPipe::CQueuedPipe
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
: CQueuedPipe( lpEndpointDescriptor, fIsLowSpeed, fIsHighSpeed, bDeviceAddress, bHubAddress,bHubPort, ttContext, pCOhcd ) // constructor for base class
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CInterruptPipe::CInterruptPipe\n\r")) );
    DEBUGCHK( m_usbEndpointDescriptor.bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE &&
              m_usbEndpointDescriptor.bLength >= sizeof( USB_ENDPOINT_DESCRIPTOR ) &&
              (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT );

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CInterruptPipe::CInterruptPipe\n\r")) );
}

// ******************************************************************
// Scope: public
CInterruptPipe::~CInterruptPipe( )
//
// Purpose: Destructor for CInterruptPipe
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CInterruptPipe::~CInterruptPipe\n\r")) );
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CInterruptPipe::~CInterruptPipe\n\r")) );
}
#if 0
// ******************************************************************
// Scope: private
int CInterruptPipe::ComputeLoad( IN const int branch ) const
//
// Purpose: Calculates the bandwidth load on the specified
//          branch of the interruptQHTree
//
// Parameters: branch to examine
//
// Returns: the number of active EDs on the branch.
//
// Notes: Caller must hold the QHScheduleLock critsec.
// ******************************************************************
{
    // Since we know that this method exists solely for the purpose of placing
    // a new interrupt pipe ED into the tree, and since the new pipe's service
    // interval cannot change over the course of said operation, we really
    // only need to find the load up to the next service level. Following that
    // node, nothing can change because the QHScheduleLock is being held.

    DEBUGCHK( branch >= 1 && branch < OHCD_MAX_INTERRUPT_INTERVAL*2 );

    // The head of the next service level is the link as calculated
    // in CPipe::Initialize. To make that calculation, we have to find
    // the largest power of two that is less than or equal to branch.
    int pow = OHCD_MAX_INTERRUPT_INTERVAL;
    while (pow > branch)
        pow >>= 1;
    int link = (pow ^ branch) | (pow >> 1);

    DEBUGCHK( link >= 0 && link < OHCD_MAX_INTERRUPT_INTERVAL );
    P_ED pEndED = m_pCOhcd->CHCCArea::m_interruptQHTree[link];

#pragma prefast(suppress:2000, "This is already checked before, suppressing.")
    DEBUGCHK( m_pCOhcd->CHCCArea::m_interruptQHTree[branch] != NULL && pEndED != NULL );
    int load = 0;
    for ( P_ED pED = (P_ED) m_pCOhcd->CHCCArea::m_pCPhysMem->PaToVa(m_pCOhcd->CHCCArea::m_interruptQHTree[branch]->paNextEd);
          pED != pEndED;
          pED = (P_ED) m_pCOhcd->CHCCArea::m_pCPhysMem->PaToVa(pED->paNextEd) ) {
        ++load;
    }
    return load;
}
#endif

// ******************************************************************
// Scope: public (Implements CPipe::OpenPipe = 0)
HCD_REQUEST_STATUS CInterruptPipe::OpenPipe( void )
//
// Purpose: Create the data structures necessary to conduct
//          transfers on this pipe
//
// Parameters: None
//
// Returns: requestOK - if pipe opened
//
//          requestFailed - if pipe was not opened
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS RetVal;
    INT32 EndpointNum;
    DEBUGMSG( ZONE_PIPE, (TEXT("+CInterruptPipe::OpenPipe\n\r") ) );

    //HCD_REQUEST_STATUS status = requestFailed;

    EnterCriticalSection( &m_csPipeLock );
    //m_pCOhcd->LockProcessingThread();

    EndpointNum = m_pCOhcd->AllocateHostEndPoint(
        (UINT32)GetType(),
        m_usbEndpointDescriptor.wMaxPacketSize,
        USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TRUE : FALSE);

    if(EndpointNum >= 0)
    {
        m_pUSBED = m_pCOhcd->AllocateED();

        if (m_pUSBED)
        {
            m_pUSBED->bHostEndPointNum = (UINT8)EndpointNum;
            m_pUSBED->bfFunctionAddress = m_bBusAddress;
            m_pUSBED->bfEndpointNumber = m_bEndpointAddress;
            m_pUSBED->bfDirection = USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TD_IN_PID : TD_OUT_PID;
            m_pUSBED->bfIsLowSpeed = m_fIsLowSpeed ? 1 : 0;
            m_pUSBED->bfIsHighSpeed = m_fIsHighSpeed ? 1 :0;
            m_pUSBED->bfHubAddress = m_bHubAddress;
            m_pUSBED->bfHubPort = m_bHubPort;
            m_pUSBED->bfIsIsochronous =  0;
            m_pUSBED->bfMaxPacketSize = m_usbEndpointDescriptor.wMaxPacketSize;
             m_pUSBED->bfAttributes = m_usbEndpointDescriptor.bmAttributes;
            m_pUSBED->TransferStatus = STATUS_IDLE;
            m_pUSBED->bfHalted = FALSE;
            m_pUSBED->bfToggleCarry = 0;
            // USB controller datasheet says interval value must be in range 1-16 for high
            // speed and 1-255 for low and full speed.
            m_pUSBED->bInterval = m_usbEndpointDescriptor.bInterval;
            if (m_pUSBED->bfIsHighSpeed && m_pUSBED->bInterval > 0x10)
                m_pUSBED->bInterval = 0;

            m_pCOhcd->ProgramHostEndpoint((UINT32)GetType(), (void *)m_pUSBED);

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_INTR

            // Allocate DMA channel for Interrupt transfers
            m_pUSBED->pDmaChannel = m_pCOhcd->m_dmaCrtl.AllocChannel(
                m_pUSBED->bHostEndPointNum,
                m_pUSBED->bfEndpointNumber,
                /*USB_ENDPOINT_TYPE_INTERRUPT,
                m_pUSBED->bfMaxPacketSize,*/
                (PfnTransferComplete)m_pCOhcd->DmaTransferComplete);

#endif // MUSB_USEDMA_FOR_INTR
#endif // MUSB_USEDMA

            if (m_pUSBED->bfDirection == TD_IN_PID)
            {
                m_pUSBED->NextED.next = (ListHead *)(m_pCOhcd->CHW::m_pIntInHead);
                (m_pCOhcd->CHW::m_pIntInHead) = (PDWORD)m_pUSBED;
            }
            else
            {
                m_pUSBED->NextED.next = (ListHead *)(m_pCOhcd->CHW::m_pIntOutHead);
                (m_pCOhcd->CHW::m_pIntOutHead) = (PDWORD)m_pUSBED;
            }

            RetVal = requestOK;
            m_pCOhcd->IncrementPipeCount();
        }
        else
        {
            DEBUGMSG( ZONE_PIPE, (TEXT("CInterruptPipe::OpenPipe: no free EDs!\n\r")));
            RetVal = requestFailed;
        }
    }
    else
    {
        DEBUGMSG( ZONE_PIPE, (TEXT("CInterruptPipe::OpenPipe: no free endpoints!\n\r")));
        RetVal = requestFailed;
    }

    //m_pCOhcd->UnlockProcessingThread();
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE, (TEXT("-CInterruptPipe::OpenPipe, returning HCD_REQUEST_STATUS %d\n\r"), RetVal) );
    return RetVal;
}

// ******************************************************************
// Scope: private (over-rides CQueuedPipe::UpdateInterruptQHTreeLoad)
void CInterruptPipe::UpdateInterruptQHTreeLoad( IN const UCHAR branch,
                                                IN const int   deltaLoad )
//
// Purpose: Change the load counts of all members in m_pCOhcd->CHCCArea::m_interruptQHTree
//          which are on branch "branch" by deltaLoad. This needs
//          to be done when interrupt QHs are added/removed to be able
//          to find optimal places to insert new queues
//
// Parameters: See CQueuedPipe::UpdateInterruptQHTreeLoad
//
// Returns: Nothing
//
// Notes:
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CInterruptPipe::UpdateInterruptQHTreeLoad - branch = %d, deltaLoad = %d\n\r"), branch, deltaLoad) );
    DEBUGCHK( branch >= 1 && branch < 2 * OHCD_MAX_INTERRUPT_INTERVAL );

#ifdef JEFFRO
    // first step - need to find the greatest power of 2 which is
    // <= branch
    UCHAR pow = OHCD_MAX_INTERRUPT_INTERVAL;
    while ( pow > branch ) {
        pow >>= 1;
    }

    EnterCriticalSection( &m_pCOhcd->CHCCArea::m_csQHScheduleLock );

    // In the reverse direction, any queue which will eventually
    // point to m_pCOhcd->CHCCArea::m_interruptQHTree[ branch ] needs to get
    // its dwInterruptTree.Load incremented. These are the queues
    // branch + n * pow, n = 1, 2, 3, ...
    for ( UCHAR link = branch + pow; link < 2 * OHCD_MAX_INTERRUPT_INTERVAL; link += pow ) {
        DEBUGCHK( m_pCOhcd->CHCCArea::m_interruptQHTree[ link ]->dwInterruptTree.Load <= m_pCOhcd->CHCCArea::m_interruptQHTree[ branch ]->dwInterruptTree.Load );
        m_pCOhcd->CHCCArea::m_interruptQHTree[ link ]->dwInterruptTree.Load += deltaLoad;
    }
    // In the forward direction, any queue that
    // m_pCOhcd->CHCCArea::m_interruptQHTree[ branch ] eventually points to
    // needs its dwInterruptTree.Load incremented. These queues
    // are found using the same algorithm as CPipe::Initialize();
    link = branch;
    while ( link >= 1 ) {
        DEBUGCHK( ( link & pow ) &&
                  ( (link ^ pow) | (pow / 2) ) < link );
        DEBUGCHK( link == branch ||
                  m_pCOhcd->CHCCArea::m_interruptQHTree[ link ]->dwInterruptTree.Load + deltaLoad >= m_pCOhcd->CHCCArea::m_interruptQHTree[ branch ]->dwInterruptTree.Load );
        m_pCOhcd->CHCCArea::m_interruptQHTree[ link ]->dwInterruptTree.Load += deltaLoad;
        link ^= pow;
        pow >>= 1;
        link |= pow;
    }

    LeaveCriticalSection( &m_pCOhcd->CHCCArea::m_csQHScheduleLock );
#else
    UNREFERENCED_PARAMETER(deltaLoad );
    UNREFERENCED_PARAMETER(branch);
#endif //JEFFRO

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CInterruptPipe::UpdateInterruptQHTreeLoad - branch = %d, deltaLoad = %d\n\r"), branch, deltaLoad) );
}

// ******************************************************************
// USHORT CInterruptPipe::GetNumTDsNeeded( const STransfer *pTransfer ) const
//
// CInterruptPipe inherits this method from CQueuedPipe.
//
ULONG * CInterruptPipe::GetListHead( IN const BOOL fEnable )
{
    UNREFERENCED_PARAMETER(fEnable);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_INTERRUPT, fEnable, FALSE);
    // return the address of the QH beginning the list for this pipe
    return &(m_pCOhcd->CHCCArea::m_interruptQHTree[m_iListHead]->paNextEd);
};
void CInterruptPipe::UpdateListControl(BOOL bEnable, BOOL bFill)
{
    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(bFill);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_INTERRUPT, bEnable, bFill);
}

// ******************************************************************
// Scope: private (Implements CPipe::AreTransferParametersValid = 0)
BOOL CInterruptPipe::AreTransferParametersValid( const STransfer *pTransfer ) const
//
// Purpose: Check whether this class' transfer parameters are valid.
//          This includes checking m_transfer, m_pPipeQH, etc
//
// Parameters: None (all parameters are vars of class)
//
// Returns: TRUE if parameters valid, else FALSE
//
// Notes: Assumes m_csPipeLock already held
// ******************************************************************
{
    if (pTransfer == NULL)
        return FALSE;

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CInterruptPipe::AreTransferParametersValid\n\r")) );

    // these parameters aren't used by CInterruptPipe, so if they are non NULL,
    // it doesn't present a serious problem. But, they shouldn't have been
    // passed in as non-NULL by the calling driver.
    DEBUGCHK( pTransfer->adwIsochErrors == NULL && // ISOCH
              pTransfer->adwIsochLengths == NULL && // ISOCH
              pTransfer->aLengths == NULL && // ISOCH
              pTransfer->lpvControlHeader == NULL ); // CONTROL
    // this is also not a serious problem, but shouldn't happen in normal
    // circumstances. It would indicate a logic error in the calling driver.
    DEBUGCHK( !(pTransfer->lpfnCallback == NULL && pTransfer->lpvCallbackParameter != NULL) );
    // DWORD                     pTransfer->dwStartingFrame (ignored - ISOCH)
    // DWORD                     pTransfer->dwFrames (ignored - ISOCH)

    BOOL fValid = ( m_pED != NULL &&
                    pTransfer->address > 0 &&
                    pTransfer->address <= USB_MAX_ADDRESS &&
                    (pTransfer->lpvClientBuffer != NULL || pTransfer->dwBufferSize == 0) &&
                    // paClientBuffer could be 0 or !0
                    pTransfer->lpfComplete != NULL &&
                    pTransfer->lpdwBytesTransferred != NULL &&
                    pTransfer->lpdwError != NULL );

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CInterruptPipe::AreTransferParametersValid, returning BOOL %d\n\r"), fValid) );
    return fValid;
}

// ******************************************************************
// Scope: private (Implements CPipe::ScheduleTransfer = 0)
HCD_REQUEST_STATUS CInterruptPipe::ScheduleTransfer( void )
//
// Purpose: Schedule a USB Transfer on this pipe
//
// Parameters: None (all parameters are in m_transfer)
//
// Returns: requestOK if transfer issued ok, else requestFailed
//
// Notes:
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CInterruptPipe::ScheduleTransfer\n\r")) );

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CInterruptPipe::ScheduleTransfer, returning HCD_REQUEST_STATUS %d\n\r"), 0) );
    return requestOK;
}

// ******************************************************************
// Scope: public
CIsochronousPipe::CIsochronousPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
                 IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
                 IN const UCHAR bDeviceAddress,
                 IN const UCHAR bHubAddress,IN const UCHAR bHubPort,const PVOID ttContext,
                                    IN COhcd * const pCOhcd)
//
// Purpose: Constructor for CIsochronousPipe
//
// Parameters: See CPipe::CPipe
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
: CQueuedPipe(lpEndpointDescriptor,fIsLowSpeed, fIsHighSpeed,bDeviceAddress, bHubAddress, bHubPort, ttContext,   pCOhcd ) // constructor for base class
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CIsochronousPipe::CIsochronousPipe\n\r")) );
    DEBUGCHK( m_usbEndpointDescriptor.bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE &&
              m_usbEndpointDescriptor.bLength >= sizeof( USB_ENDPOINT_DESCRIPTOR ) &&
              (m_usbEndpointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS );

    DEBUGCHK( !fIsLowSpeed ); // iso pipe must be high speed

    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CIsochronousPipe::CIsochronousPipe\n\r")) );
}

// ******************************************************************
// Scope: public
CIsochronousPipe::~CIsochronousPipe( )
//
// Purpose: Destructor for CIsochronousPipe
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: Do not modify static variables here!!
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("+CIsochronousPipe::~CIsochronousPipe\n\r")) );
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("-CIsochronousPipe::~CIsochronousPipe\n\r")) );
}

// ******************************************************************
// Scope: public (Implements CPipe::OpenPipe = 0)
HCD_REQUEST_STATUS CIsochronousPipe::OpenPipe( void )
//
// Purpose: Inserting the necessary (empty) items into the
//          schedule to permit future transfers
//
// Parameters: None
//
// Returns: requestOK if pipe opened successfuly
//          requestFailed if pipe not opened
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS RetVal;
    INT32 EndpointNum;
    DEBUGMSG( ZONE_PIPE, (TEXT("+CIsochronousPipe::OpenPipe\n\r") ) );

    EnterCriticalSection( &m_csPipeLock );
    //m_pCOhcd->LockProcessingThread();

    // if this fails, we have a low speed Bulk device
    // which is not allowed by the USB spec
    DEBUGCHK( !m_fIsLowSpeed );

    EndpointNum = m_pCOhcd->AllocateHostEndPoint(
        (UINT32)GetType(),
        m_usbEndpointDescriptor.wMaxPacketSize,
        USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TRUE : FALSE);

    if(EndpointNum >= 0){
        m_pUSBED = m_pCOhcd->AllocateED();

        if (m_pUSBED)
        {
            m_pUSBED->bHostEndPointNum = (UINT8)EndpointNum;
            m_pUSBED->bfFunctionAddress = m_bBusAddress;
            m_pUSBED->bfEndpointNumber = m_bEndpointAddress;
            m_pUSBED->bfDirection = USB_ENDPOINT_DIRECTION_IN(m_bEndpointAddress) ? TD_IN_PID : TD_OUT_PID;;
            m_pUSBED->bfIsLowSpeed = 0;
            m_pUSBED->bfIsHighSpeed = m_fIsHighSpeed ? 1 :0;
            m_pUSBED->bfHubAddress = m_bHubAddress;
            m_pUSBED->bfHubPort = m_bHubPort;
            m_pUSBED->bfIsIsochronous = 1;
            m_pUSBED->bfMaxPacketSize = m_usbEndpointDescriptor.wMaxPacketSize;
            m_pUSBED->bfAttributes = m_usbEndpointDescriptor.bmAttributes;
            m_pUSBED->TransferStatus = STATUS_IDLE;
            m_pUSBED->bfHalted = FALSE;
            m_pUSBED->bfToggleCarry = FALSE;
            // USB spec says interval value must be in range 1-16.
            // Values > 16 result in USB controller hangup.
            m_pUSBED->bInterval = m_usbEndpointDescriptor.bInterval > 0x10 ? 0x00 : m_usbEndpointDescriptor.bInterval;

            m_pCOhcd->ProgramHostEndpoint((UINT32)GetType(), (void *)m_pUSBED);

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO

            // Allocate DMA channel
            m_pUSBED->pDmaChannel = m_pCOhcd->m_dmaCrtl.AllocChannel(
                m_pUSBED->bHostEndPointNum,
                m_pUSBED->bfEndpointNumber,
                (PfnTransferComplete)m_pCOhcd->DmaTransferComplete);

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA

            if(m_pUSBED->bfDirection == TD_IN_PID)
            {
                m_pUSBED->NextED.next = (ListHead*)(m_pCOhcd->CHW::m_pIsoInHead);
                (m_pCOhcd->CHW::m_pIsoInHead) = (PDWORD)m_pUSBED;
            }
            else
            {
                m_pUSBED->NextED.next = (ListHead*)(m_pCOhcd->CHW::m_pIsoOutHead);
                (m_pCOhcd->CHW::m_pIsoOutHead) = (PDWORD)m_pUSBED;
            }

            RetVal = requestOK;
            m_pCOhcd->IncrementPipeCount();
        }
        else
        {
            DEBUGMSG( ZONE_PIPE, (TEXT("CIsochronousPipe::OpenPipe: no free EDs!\n\r")));
            RetVal = requestFailed;
        }
    }
    else
    {
        DEBUGMSG( ZONE_PIPE, (TEXT("CIsochronousPipe::OpenPipe: no free endpoints!\n\r")));
        RetVal = requestFailed;
    }

    //m_pCOhcd->UnlockProcessingThread();
    LeaveCriticalSection( &m_csPipeLock );

    DEBUGMSG( ZONE_PIPE, (TEXT("-CIsochronousPipe::OpenPipe, returning HCD_REQUEST_STATUS %d\n\r"), RetVal) );
    return RetVal;
}

ULONG * CIsochronousPipe::GetListHead( const BOOL fEnable )
{
    UNREFERENCED_PARAMETER(fEnable);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_ISOCH, fEnable, FALSE);

    // return m_pCOhcd->CHW::m_pIsoHead;
    return NULL;
}

void CIsochronousPipe::UpdateListControl(BOOL bEnable, BOOL bFill)
{
    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(bFill);
    //m_pCOhcd->CHW::ListControl(CHW::LIST_ISOCH, bEnable, bFill);
}


// ******************************************************************
// Scope: private (Implements CPipe::AreTransferParametersValid = 0)
BOOL CIsochronousPipe::AreTransferParametersValid( const STransfer *pTransfer ) const
//
// Purpose: Check whether this class' transfer parameters, stored in
//          m_transfer, are valid.
//
// Parameters: None (all parameters are vars of class)
//
// Returns: TRUE if parameters valid, else FALSE
//
// Notes: Assumes m_csPipeLock already held
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("+CIsochronousPipe::AreTransferParametersValid\n\r")) );

    if (pTransfer == NULL)
        return FALSE;

    // these parameters aren't used by CIsochronousPipe, so if they are non NULL,
    // it doesn't present a serious problem. But, they shouldn't have been
    // passed in as non-NULL by the calling driver.
    DEBUGCHK( pTransfer->lpvControlHeader == NULL ); // CONTROL
    // this is also not a serious problem, but shouldn't happen in normal
    // circumstances. It would indicate a logic error in the calling driver.
    DEBUGCHK( !(pTransfer->lpfnCallback == NULL && pTransfer->lpvCallbackParameter != NULL) );

    BOOL fValid = (
                    pTransfer->address > 0 &&
                    pTransfer->address <= USB_MAX_ADDRESS &&
                    pTransfer->lpvClientBuffer != NULL &&
                    // paClientBuffer could be 0 or !0
                    pTransfer->dwBufferSize > 0 &&
                    pTransfer->adwIsochErrors != NULL &&
                    pTransfer->adwIsochLengths != NULL &&
                    pTransfer->aLengths != NULL &&
                    pTransfer->dwFrames > 0 &&
                    pTransfer->dwFrames < gcTdIsochMaxFrames &&
                    pTransfer->lpfComplete != NULL &&
                    pTransfer->lpdwBytesTransferred != NULL &&
                    pTransfer->lpdwError != NULL );

    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("-CIsochronousPipe::AreTransferParametersValid, returning BOOL %d\n\r"), fValid) );
    return fValid;
}

// ******************************************************************
// Scope: private (Implements CPipe::ScheduleTransfer = 0)
HCD_REQUEST_STATUS CIsochronousPipe::ScheduleTransfer( void )
//
// Purpose: Schedule a USB Transfer on this pipe
//
// Parameters: None (all parameters are in m_transfer)
//
// Returns: requestOK if transfer issued ok, else requestFailed
//
// Notes:
// ******************************************************************
{
    HCD_REQUEST_STATUS status = requestOK;

    DEBUGMSG( ZONE_TRANSFER, (TEXT("+CIsochronousPipe::ScheduleTransfer\n\r")) );

    DEBUGMSG( ZONE_TRANSFER, (TEXT("-CIsochronousPipe::ScheduleTransfer, returning HCD_REQUEST_STATUS %d\n\r"), status) );
    return status;
}

////////////////////////////////////////////////////////////////////////////////
//

CPipeAbs * CreateBulkPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,const PVOID ttContext,
               IN CHcd * const pChcd)
{
    return new CBulkPipe(lpEndpointDescriptor,fIsLowSpeed,fIsHighSpeed,
		           bDeviceAddress,bHubAddress,bHubPort,ttContext,(COhcd * const)pChcd);
}
CPipeAbs * CreateControlPipe(IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,const PVOID ttContext,
               IN CHcd * const pChcd)
{
    return new CControlPipe(lpEndpointDescriptor,fIsLowSpeed,fIsHighSpeed,
		           bDeviceAddress,bHubAddress,bHubPort,ttContext,(COhcd * const)pChcd);
}

CPipeAbs * CreateInterruptPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,const PVOID ttContext,
               IN CHcd * const pChcd)
{
    return new CInterruptPipe(lpEndpointDescriptor,fIsLowSpeed,fIsHighSpeed,
		           bDeviceAddress,bHubAddress,bHubPort,ttContext,(COhcd * const)pChcd);
}

CPipeAbs * CreateIsochronousPipe( IN const LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDescriptor,
               IN const BOOL fIsLowSpeed,IN const BOOL fIsHighSpeed,
               IN const UCHAR bDeviceAddress,
               IN const UCHAR bHubAddress,IN const UCHAR bHubPort,const PVOID ttContext,
               IN CHcd * const pChcd)
{
    CPipeAbs *pPipe = NULL;

    UNREFERENCED_PARAMETER(lpEndpointDescriptor);
    UNREFERENCED_PARAMETER(fIsLowSpeed);
    UNREFERENCED_PARAMETER(fIsHighSpeed);
    UNREFERENCED_PARAMETER(bDeviceAddress);
    UNREFERENCED_PARAMETER(bHubAddress);
    UNREFERENCED_PARAMETER(bHubPort);
    UNREFERENCED_PARAMETER(ttContext);
    UNREFERENCED_PARAMETER(pChcd);

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO

    // ISO is currently supported in DMA mode only!

    pPipe = new CIsochronousPipe(lpEndpointDescriptor,fIsLowSpeed,fIsHighSpeed,bDeviceAddress,bHubAddress,bHubPort,ttContext,
		(COhcd * const)pChcd);

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA

    return pPipe;
}
