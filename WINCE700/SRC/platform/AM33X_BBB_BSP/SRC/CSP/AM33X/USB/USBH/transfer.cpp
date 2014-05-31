// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  transfer.cpp
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
//     Transfer.cpp

#pragma warning(push)
#pragma warning(disable: 4201 4510 4512 4610)
#include "cpipe.hpp"
#include "cphysmem.hpp"
#include "chw.hpp"
#include "cohcd.hpp"
#pragma warning(pop)

#ifndef _PREFAST_
#pragma warning(disable: 4068 6320 6322) // Disable pragma warnings
#endif

#pragma warning(disable: 6320 6322)

CTransfer::CTransfer(CQueuedPipe* const  rPipe, STransfer sTransfer,CTransfer *lpNextTransfer)
: m_rPipe(rPipe)
, m_lpNextTransfer(lpNextTransfer)
{
    m_Transfer = sTransfer;
    if ( m_Transfer.lpvControlHeader!=NULL ) { // Control Transfer
        m_dwNextToQueueIndex = m_dwNextToCompleteIndex=(DWORD)-1; // Need Setup.
        m_bNeedStatus = m_bNeedStatusComplete = TRUE;
    }
    else {
        m_dwNextToQueueIndex=m_dwNextToCompleteIndex=0; // Need Setup.
        m_bNeedStatus = m_bNeedStatusComplete = FALSE;
    };
    m_dwQueueTD =0;
    m_dwFirstError=USB_NO_ERROR;
#pragma prefast(disable: 322, "Recover gracefully from hardware failure")
    __try { // initializing transfer status parameters
        *m_Transfer.lpfComplete = FALSE;
        *m_Transfer.lpdwBytesTransferred = 0;
        *m_Transfer.lpdwError = USB_NOT_COMPLETE_ERROR;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
    }
#pragma prefast(pop)
};
// ******************************************************************
// Scope: public
BOOL CTransfer::ScheduleTD()
//
// Purpose: Schedule TD for this transfer.
//
// return : TRUE: success schedule TD, FALSE: Error happens.
//
// Parameters: Nothing
//
// ******************************************************************
{
    return TRUE;
}
// ******************************************************************
// Scope: public
BOOL CTransfer::DoneTD()
//
// Purpose: One TD Done for this Transfer..
//
// return : TRUE: success Done TD, FALSE: Error happens.
//
// Parameters: Nothing
//
// ******************************************************************
{
    return FALSE;
};

BOOL CTransfer::Canceled()
{
    __try { // initializing transfer status parameters
        m_dwNextToQueueIndex=m_dwNextToCompleteIndex=m_Transfer.dwBufferSize;
        *m_Transfer.lpfComplete = TRUE;
        *m_Transfer.lpdwError = USB_CANCELED_ERROR;
        if (m_Transfer.lpfnCallback ) {
            ( *m_Transfer.lpfnCallback )( m_Transfer.lpvCallbackParameter );
            m_Transfer.lpfnCallback =NULL;
        }
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::DoneTD()(%s)::exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
    }

    return TRUE;

}

TDQueue::TDQueue(IN CPhysMem* const pCPhysMem, DWORD dwNumOfTD, DWORD dwTDBufferSize)
: m_dwNumOfTD (dwNumOfTD)
, m_dwTDBufferSize( dwTDBufferSize)
, m_pCPhysMem( pCPhysMem)
{
    m_pTDQueue = NULL;
    m_pBuffer = NULL;
};
TDQueue::~TDQueue()
{
    QueueDeInit();
}
void TDQueue::QueueDeInit()
{
    if (m_pBuffer) {
        m_pCPhysMem->FreeMemory(m_pBuffer,m_pBufferPhysAddr,CPHYSMEM_FLAG_NOBLOCK);
        m_pBuffer=NULL;
        m_pBufferPhysAddr = 0;
    }
    if (m_pTDQueue) {
        m_pCPhysMem->FreeMemory((PBYTE)m_pTDQueue,m_pTDQueuePhysAddr,CPHYSMEM_FLAG_NOBLOCK);
        m_pTDQueue = NULL;
        m_pTDQueuePhysAddr = 0;
    }
}
BOOL TDQueue::QueueInit()
{
    if (m_pTDQueue==NULL) {
        if (!m_pCPhysMem->AllocateMemory( DEBUG_PARAM( TEXT("IssueTransfer TDs") )
                                m_dwNumOfTD *sizeof(TD),
                                (PUCHAR *) &m_pTDQueue,
                                CPHYSMEM_FLAG_NOBLOCK))  {
            DEBUGMSG( ZONE_WARNING, (TEXT("TDQueue::TDQueue - no memory for TD list\n") ) );
            m_pTDQueue= NULL;
        }
        else {
            m_pTDQueuePhysAddr = m_pCPhysMem->VaToPa((PBYTE) m_pTDQueue );
            memset (  m_pTDQueue , 0, m_dwNumOfTD *sizeof(TD));
        }
    }
    if (m_pBuffer==NULL) {
        if (!m_pCPhysMem->AllocateMemory( DEBUG_PARAM( TEXT("IssueTransfer TDs") )
                                m_dwNumOfTD * m_dwTDBufferSize ,
                                (PUCHAR *) &m_pBuffer,
                                CPHYSMEM_FLAG_NOBLOCK))  {
            DEBUGMSG( ZONE_WARNING, (TEXT("TDQueue::TDQueue - no memory for TD list\n") ) );
            m_pBuffer= NULL;
        }
        else
            m_pBufferPhysAddr =  m_pCPhysMem->VaToPa(m_pBuffer);
    }
    m_dwHeadIndex = m_dwTailIndex = 0;
    if  ( m_pTDQueue!=NULL && m_pBuffer!=NULL ) {
        for (DWORD dwIndex=0;dwIndex< m_dwNumOfTD;dwIndex++) {
            (m_pTDQueue+dwIndex)->bfDiscard=1;
            (m_pTDQueue+dwIndex)->paNextTd.phys =  m_pTDQueuePhysAddr + sizeof(TD) * IncIndex(dwIndex);
            (m_pTDQueue+dwIndex)->paCurBuffer = m_pBufferPhysAddr + m_dwTDBufferSize * m_dwTailIndex;
        }
        m_dwHeadIndex = m_dwTailIndex = 0;
        return TRUE;
    }
    return FALSE;
}
BOOL TDQueue::QueueDisable()
{
    if  ( m_pTDQueue!=NULL && m_pBuffer!=NULL ) {
        for (DWORD dwIndex=0;dwIndex< m_dwNumOfTD;dwIndex++) {
            (m_pTDQueue+dwIndex)->bfDiscard=1;
        }
    }
    return TRUE;
}


BOOL TDQueue::InitTDQueueTailTD( IN       CTransfer *pTransfer,
                              IN CQueuedPipe * pPipe,
                              IN const UCHAR InterruptOnComplete,
                              IN const DWORD PID,
                              IN const USHORT DataToggle,
                              IN const DWORD paBuffer,
                              IN const DWORD MaxLength,
                              IN const BOOL bShortPacketOk )

{
    if (IsTDQueueFull() || m_pTDQueue==NULL || m_pBuffer == NULL)
        return FALSE;
    TD * pTD = m_pTDQueue + m_dwTailIndex;

    // not really part of the TD
    pTD->pTransfer = pTransfer;
    pTD->pNextTd =  m_pTDQueue + IncIndex(m_dwTailIndex);
    pTD->pPipe = pPipe;
    pTD->bfIsIsoch = 0;
    pTD->bfDiscard = 0;

    // the actual TD (null is legal for the last TD)
    pTD->paNextTd.phys = m_pTDQueuePhysAddr + sizeof(TD) * IncIndex(m_dwTailIndex);

//    DEBUGCHK( InterruptOnComplete == 0 || InterruptOnComplete == 7 );
    pTD->bfShortPacketOk = bShortPacketOk;
    pTD->bfDelayInterrupt = InterruptOnComplete ? gcTdInterruptOnComplete : gcTdNoInterrupt;
    pTD->bfDataToggle = DataToggle;
    pTD->bfErrorCount = 0;
    pTD->bfConditionCode = USB_NOT_ACCESSED_ERROR;

    DEBUGCHK( PID == TD_IN_PID ||
              PID == TD_OUT_PID ||
              PID == TD_SETUP_PID );
    pTD->bfPID = PID;
    DWORD phAddr = (paBuffer==0? (m_pBufferPhysAddr + m_dwTDBufferSize * m_dwTailIndex):paBuffer);
    if (MaxLength == 0 ) {
        // zero-length transfer
        pTD->paCurBuffer = 0;
        pTD->paBufferEnd = 0;
    } else {
        DEBUGCHK( MaxLength <= 0x2000 /*8K*/ );
        pTD->paCurBuffer = phAddr;
        pTD->paBufferEnd = phAddr+MaxLength-1;
    }
    return TRUE;
};

#if ITD_STUFF

CITransfer::CITransfer(CIsochronousPipe* const  rPipe, STransfer sTransfer,CITransfer *lpNextTransfer)
: m_rPipe(rPipe)
, m_lpNextTransfer(lpNextTransfer)
{
    m_Transfer = sTransfer;
    m_dwNextToQueueFrameIndex= m_dwNextToCompleteFrameIndex = 0;
    m_dwNextToQueueBufferIndex = m_dwNextToCompleteBufferIndex = 0;

    m_dwQueueTD =0;
    m_dwFirstError = USB_NO_ERROR;
#pragma prefast(disable: 322, "Recover gracefully from hardware failure")
    __try { // initializing transfer status parameters
        *m_Transfer.lpfComplete = FALSE;
        *m_Transfer.lpdwBytesTransferred = 0;
        *m_Transfer.lpdwError = USB_NOT_COMPLETE_ERROR;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
    }
#pragma prefast(pop)

};
// ******************************************************************
// Scope: public
BOOL CITransfer::ScheduleITD()
//
// Purpose: Schedule ITD for this transfer.
//
// return : TRUE: success schedule TD, FALSE: Error happens.
//
// Parameters: Nothing
//
// ******************************************************************
{
    if (m_rPipe->IsITDQueueFull())
        return FALSE;

    while (m_rPipe->IsITDQueueFull()==FALSE && m_dwNextToQueueFrameIndex < m_Transfer.dwFrames ) { // if this is TD not full yet.
        DWORD dwFrame = min(  m_rPipe->GetFrameNumberPerITD(),m_Transfer.dwFrames - m_dwNextToQueueFrameIndex);
        DWORD dwDataBufferSize =0;
        for (DWORD dwIndex =0;dwIndex< dwFrame; dwIndex ++) {
            dwDataBufferSize += m_Transfer.aLengths[m_dwNextToQueueFrameIndex + dwIndex];
        }
        if ( m_Transfer.paClientBuffer == 0 && (m_Transfer.dwFlags & USB_IN_TRANSFER) ==0 ) { // We need Copy data
            PBYTE pUserData = m_rPipe->GetITDQueueTailBufferVirtAddr();
            __try { // setting transfer status and executing callback function
                memcpy( pUserData, (PBYTE)(m_Transfer.lpvClientBuffer) +  m_dwNextToQueueBufferIndex ,dwDataBufferSize );
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::ScheduleTD()(%s)::CheckForDoneTransfers - exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
            }
        };
        BOOL bReturn =m_rPipe->InitITDQueueTailITD(this,
                              m_rPipe,
                              (WORD)(m_Transfer.dwStartingFrame + m_dwNextToQueueFrameIndex),
                              (WORD)dwFrame,
                              m_Transfer.aLengths+m_dwNextToQueueFrameIndex,
                              gcTdInterruptOnComplete,
                              ((m_Transfer.paClientBuffer!=0)?m_Transfer.paClientBuffer +  m_dwNextToQueueBufferIndex:0),
                              dwDataBufferSize)   ;
        ASSERT(bReturn==TRUE);
        if (bReturn){
            m_dwNextToQueueFrameIndex += dwFrame ;
            m_dwNextToQueueBufferIndex += dwDataBufferSize;
            m_rPipe->AdvanceITDQueueTail();
        }
        else
            return FALSE;
    }
    return TRUE;
}
// ******************************************************************
// Scope: public
BOOL CITransfer::DoneITD()
//
// Purpose: One ITD Done for this Transfer..
//
// return : TRUE: success Done TD, FALSE: Error happens.
//
// Parameters: Nothing
//
// ******************************************************************
{
    P_ITD pCur= m_rPipe->GetITDQueueHead();
    if (m_dwNextToCompleteFrameIndex < m_Transfer.dwFrames ) {
        DWORD dwFrame =min ( pCur->bfFrameCount +1, m_rPipe->GetFrameNumberPerITD());
        ASSERT(dwFrame<=m_Transfer.dwFrames - m_dwNextToCompleteFrameIndex );
        dwFrame = min (dwFrame,m_Transfer.dwFrames - m_dwNextToCompleteFrameIndex); // This is for protection.
        DWORD dwDataBufferSize =0;
        for (DWORD dwIndex =0;dwIndex< dwFrame; dwIndex ++) {
            DWORD cc = pCur->offsetPsw[dwIndex].uConditionCode;
            DWORD sz = pCur->offsetPsw[dwIndex].uSize;
            if (cc == USB_NOT_ACCESSED_ALT)
                cc = USB_NOT_ACCESSED_ERROR;
            __try { // initializing transfer status parameters
                if (m_Transfer.adwIsochErrors!=NULL)
                    m_Transfer.adwIsochErrors[m_dwNextToCompleteFrameIndex + dwIndex] = cc;
                // OHCI 4.3.2.3.3
                if (m_Transfer.adwIsochLengths!=NULL)
                    m_Transfer.adwIsochLengths[m_dwNextToCompleteFrameIndex + dwIndex] =
                        ((m_Transfer.dwFlags & USB_IN_TRANSFER)!=0?sz:m_Transfer.aLengths[m_dwNextToCompleteFrameIndex + dwIndex]);
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::DoneTD()(%s)::exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
            }
            dwDataBufferSize += m_Transfer.aLengths[m_dwNextToCompleteFrameIndex + dwIndex];
        }
        if (m_Transfer.paClientBuffer == 0 && (m_Transfer.dwFlags & USB_IN_TRANSFER)!=0) { // In transfer , we need copy.
            PBYTE pUserData = m_rPipe->GetITDQueueHeadBufferVirtAddr();
            __try { // initializing transfer status parameters
                memcpy ((PBYTE)(m_Transfer.lpvClientBuffer) + m_dwNextToCompleteBufferIndex , pUserData,dwDataBufferSize);
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::DoneTD()(%s)::exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
            }
        }
        m_dwNextToCompleteBufferIndex += dwDataBufferSize;
        m_dwNextToCompleteFrameIndex += dwFrame;
    }
    // Update the TD status.
    if (m_dwFirstError== USB_NO_ERROR) {
        m_dwFirstError = pCur->bfConditionCode;
    }

    m_rPipe->AdvanceITDQueueHead();
    DWORD bReturn=FALSE;
    if (m_dwNextToCompleteFrameIndex>=m_Transfer.dwFrames) { // We are completed.
        bReturn=TRUE;
        __try { // initializing transfer status parameters
            *m_Transfer.lpfComplete = TRUE;
            *m_Transfer.lpdwBytesTransferred = m_dwNextToCompleteFrameIndex;
            *m_Transfer.lpdwError = m_dwFirstError;
            if (m_Transfer.lpfnCallback ) {
                ( *m_Transfer.lpfnCallback )( m_Transfer.lpvCallbackParameter );
                m_Transfer.lpfnCallback =NULL;
            }

        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::DoneTD()(%s)::exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
        }

    }
    return bReturn;
};

BOOL CITransfer::Canceled()
{
    __try { // initializing transfer status parameters
        m_dwNextToQueueFrameIndex=m_dwNextToCompleteFrameIndex=m_Transfer.dwFrames;
        *m_Transfer.lpfComplete = TRUE;
        *m_Transfer.lpdwError = USB_CANCELED_ERROR;
        if (m_Transfer.lpfnCallback ) {
            ( *m_Transfer.lpfnCallback )( m_Transfer.lpvCallbackParameter );
            m_Transfer.lpfnCallback =NULL;
        }
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        DEBUGMSG( ZONE_ERROR, (TEXT("CTransfer::Canceled()(%s)::exception setting transfer status to complete\n"), m_rPipe->GetPipeType() ) );
    }

    return TRUE;
}
// ******************************************************************
// Scope: public
BOOL CITransfer::CheckFrame(WORD wMaxPacketSize)const
//
// Purpose: Check Each frame against buffer that used.
//
// return : TRUE: MATCH, FALSE: Error happens.
//
// Parameters: Packet Size.
//
{
    BOOL fValid = TRUE;
    __try {
        DWORD dwTotalData = 0;
        for ( DWORD frame = 0; frame < m_Transfer.dwFrames; frame++ ) {
            if ( m_Transfer.aLengths[ frame ] == 0 ||
                 m_Transfer.aLengths[ frame ] > wMaxPacketSize ) {
                fValid = FALSE;
                break;
            }
            dwTotalData += m_Transfer.aLengths[ frame ];
        }
        fValid = ( fValid &&
                   dwTotalData == m_Transfer.dwBufferSize );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        fValid = FALSE;
    }
    return fValid;

}




ITDQueue::ITDQueue(IN CPhysMem* const pCPhysMem, DWORD dwNumOfFrame, DWORD dwMaxPacketSize)
: m_pCPhysMem( pCPhysMem)
, m_dwMaxPacketSize( dwMaxPacketSize)
{
    m_dwFramePerTD = (dwMaxPacketSize >512?4:8);
    m_dwNumOfTD =  (dwNumOfFrame + m_dwFramePerTD -1)/m_dwFramePerTD ;
    m_dwTDBufferSize = dwMaxPacketSize * m_dwFramePerTD ;
    m_pTDQueue = NULL;
    m_pBuffer = NULL;

};
ITDQueue::~ITDQueue()
{
    QueueDeInit();
}
void ITDQueue::QueueDeInit()
{
    if (m_pBuffer) {
        m_pCPhysMem->FreeMemory(m_pBuffer,m_pBufferPhysAddr,CPHYSMEM_FLAG_NOBLOCK);
        m_pBuffer=NULL;
        m_pBufferPhysAddr = 0;
    }
    if (m_pTDQueue) {
        m_pCPhysMem->FreeMemory((PBYTE)m_pTDQueue,m_pTDQueuePhysAddr,CPHYSMEM_FLAG_NOBLOCK);
        m_pTDQueue = NULL;
        m_pTDQueuePhysAddr = 0;
    }
}
BOOL ITDQueue::QueueInit()
{
    if (m_pTDQueue==NULL) {
        if (!m_pCPhysMem->AllocateMemory( DEBUG_PARAM( TEXT("IssueTransfer ITDs") )
                                m_dwNumOfTD *sizeof(ITD),
                                (PUCHAR *) &m_pTDQueue,
                                CPHYSMEM_FLAG_NOBLOCK))  {
            DEBUGMSG( ZONE_WARNING, (TEXT("TDQueue::TDQueue - no memory for ITD list\n") ) );
            m_pTDQueue= NULL;
        }
        else {
            m_pTDQueuePhysAddr = m_pCPhysMem->VaToPa( (PBYTE)m_pTDQueue );
            memset (  m_pTDQueue , 0, m_dwNumOfTD *sizeof(ITD));
        }
    }
    if (m_pBuffer==NULL) {
        if (!m_pCPhysMem->AllocateMemory( DEBUG_PARAM( TEXT("IssueTransfer ITDs") )
                                m_dwNumOfTD * m_dwTDBufferSize ,
                                (PUCHAR *) &m_pBuffer,
                                CPHYSMEM_FLAG_NOBLOCK))  {
            DEBUGMSG( ZONE_WARNING, (TEXT("TDQueue::TDQueue - no memory for TD list\n") ) );
            m_pBuffer= NULL;
        }
        else
            m_pBufferPhysAddr =  m_pCPhysMem->VaToPa(m_pBuffer);
    }
    m_dwHeadIndex = m_dwTailIndex = 0;
    if  ( m_pTDQueue!=NULL && m_pBuffer!=NULL ) {
        for (DWORD dwIndex=0;dwIndex< m_dwNumOfTD;dwIndex++) {
            (m_pTDQueue+dwIndex)->bfDiscard=1;
            (m_pTDQueue+dwIndex)->paNextTd.phys =  m_pTDQueuePhysAddr + sizeof(ITD) * IncIndex(dwIndex);
        }
        m_dwHeadIndex = m_dwTailIndex = 0;
        return TRUE;
    }
    return FALSE;
}
BOOL ITDQueue::QueueDisable()
{
    if  ( m_pTDQueue!=NULL && m_pBuffer!=NULL ) {
        for (DWORD dwIndex=0;dwIndex< m_dwNumOfTD;dwIndex++) {
            (m_pTDQueue+dwIndex)->bfDiscard=1;
        }
    }
    return TRUE;
}
BOOL ITDQueue::InitITDQueueTailITD( IN  CITransfer  *pTransfer,
                              IN CIsochronousPipe * pPipe,
                              IN const USHORT wStartFrame,
                              IN const USHORT wNumOfFrame,
                              IN LPCDWORD     lpFrameLength,
                              IN const UCHAR InterruptOnComplete,
                              IN const DWORD paBuffer,
                              IN const DWORD /*MaxLength*/)
{
    if (IsITDQueueFull() || m_pTDQueue==NULL || m_pBuffer == NULL)
        return FALSE;
    ITD * pTD = m_pTDQueue + m_dwTailIndex;
    memset((PUCHAR)  pTD, 0, sizeof(ITD));
    // not really part of the ITD
    pTD->pTransfer = pTransfer;
    pTD->pNextTd =  m_pTDQueue + IncIndex(m_dwTailIndex);
    pTD->pPipe = pPipe;
    pTD->bfIsIsoch = 0;
    pTD->bfDiscard = 0;

    // the actual TD (null is legal for the last TD)
    pTD->paNextTd.phys = m_pTDQueuePhysAddr + sizeof(ITD) * IncIndex(m_dwTailIndex);

    pTD->bfStartFrame = (wStartFrame) & 0xFFFF;
    pTD->bfIsIsoch = 1;
    pTD->bfDiscard = 0;
    pTD->bfDelayInterrupt = InterruptOnComplete;
    pTD->bfConditionCode = USB_NOT_ACCESSED_ERROR;
    DWORD dwStartPhysAddr = (paBuffer!=0?paBuffer:(m_pBufferPhysAddr + m_dwTDBufferSize * m_dwTailIndex));
    pTD->paBufferPage0 = dwStartPhysAddr;
    WORD    wPageSelect =0;

    for (DWORD dwIndex=0; dwIndex < wNumOfFrame && dwIndex< m_dwFramePerTD; dwIndex++) {
        pTD->offsetPsw[dwIndex].uHiBits = ~0; // init to NOT_ACCESSED
        pTD->offsetPsw[dwIndex].uOffset = ((dwStartPhysAddr) & (gcTdPageSize - 1)) | wPageSelect;
        dwStartPhysAddr += lpFrameLength[dwIndex];

        // Check to see if the next frame is on the same OHCI page as this one
        if ((pTD->paBufferPage0 ^ dwStartPhysAddr ) & ~(gcTdPageSize - 1))
            wPageSelect = gcTdPageSize;
    }
    ASSERT(dwIndex!=0);
    pTD->paBufferEnd = dwStartPhysAddr  - 1;
    pTD->bfFrameCount = dwIndex - 1; // as described in table 4-3
    return TRUE;
};

#endif // ITD_STUFF


