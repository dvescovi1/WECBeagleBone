// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  transfer.hpp
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
//     Transfer.hpp
//
// Abstract: Implements class for managing transfer for OHCI
//

#ifndef __TRANSFER_HPP_
#define __TRANSFER_HPP_
//
typedef struct STRANSFER {
// These are the IssueTransfer parameters
//
    UCHAR                     address;
    LPTRANSFER_NOTIFY_ROUTINE lpfnCallback;
    LPVOID                    lpvCallbackParameter;
    DWORD                     dwFlags;
    LPCVOID                   lpvControlHeader;
    DWORD                     paControlHeader;
    DWORD                     dwStartingFrame;
    DWORD                     dwFrames;
    LPCDWORD                  aLengths;
    DWORD                     dwBufferSize;
    LPVOID                    lpvClientBuffer;
    ULONG                     paClientBuffer;
    LPCVOID                   lpvCancelId;
    LPDWORD                   adwIsochErrors;
    LPDWORD                   adwIsochLengths;
    LPBOOL                    lpfComplete;
    LPDWORD                   lpdwBytesTransferred;
    LPDWORD                   lpdwError;
} STransfer ;
//
class CTransfer {
// Queued Transfer Managment Class
// Transfer Buffer is pointed by m_Transfer.lpvClientBuffer.
// Transfer is queued and unqueued in TD unit (it is maximum packet size). The pipe only
// can queue limited Amount of TD (Please refer CQueuePipe Companion Class TDQueue for detail).
// So m_dwNextToQueueIndex and m_dwNextToCompleteIndex is used for manage data Transfer Buffer.
//
// m_dwNextToQueueIndex: it offset to the buffer next to queue into Pipe's Companion Class TDQueue.
//    it also mean that the data before m_dwNextToQueueIndex has been queued into TDQueue.
//    Special value (-1) is indicate point to the m_Transfer.lpvControlHeader before
//    m_Transfer.lpvClientBuffer. Data size for m_Transfer.lpvControlHeader is always
//    sizeof(USB_DEVICE_REQUEST)
// m_dwNextToCompleteIndex: it ofset to the buffer that wait for the completion notification from
//    pipe. In another word, the data before m_dwNextToCompleteIndex has been completed (done ack)
//    from Pipe. Same as m_dwNextToQueueIndex, spcial value (-1) will indicate point to
//    m_Transfer.lpvControlHeader
// m_bNeedStatus && m_bNeedStatusComplete : those are use to handle Status Stage for Control
//    Transfer. TRUE is mean that it need send status packet and it need complete notification
//    for status packet.

public:
    CTransfer(CQueuedPipe* const  rPipe, STransfer sTransfer,CTransfer *lpNextTransfer=NULL );
    ~CTransfer() { ASSERT(m_Transfer.lpfnCallback == NULL );};
    BOOL ScheduleTD();
    CTransfer * SetNextTransfer (CTransfer * lpNextTransfer) {
        CTransfer * pReturn = m_lpNextTransfer;
        m_lpNextTransfer = lpNextTransfer ;
        return pReturn;
    }
    BOOL Canceled();
    BOOL DoneTD();
    BOOL IsCompleteQueue() const {
        return (    m_dwNextToQueueIndex !=-1 &&
                    m_dwNextToQueueIndex >= m_Transfer.dwBufferSize &&
                    !m_bNeedStatus);
    };
    CTransfer * GetNextTransfer () const { return m_lpNextTransfer; };
    STransfer * GetSTransfer() { return &m_Transfer; };
    CQueuedPipe* GetPipe () const { return m_rPipe ;};
    DWORD   GetFirstError() { return m_dwFirstError; };
private:
    STransfer       m_Transfer;
    // additional parameters/data
    CQueuedPipe * const    m_rPipe;            // pipe on which this transfer was issued
    CTransfer *     m_lpNextTransfer;

    DWORD           m_dwNextToQueueIndex;
    DWORD           m_dwNextToCompleteIndex;
    DWORD           m_dwQueueTD;
    DWORD           m_dwFirstError;
    BOOL            m_bNeedStatus;
    BOOL            m_bNeedStatusComplete;
};

//
class TDQueue {
//  Circular TD Queue and Its Buffer Managment Class
//  This is companion class for CQueuedPipe.
//
//   m_pTDQueue---->  TD0 -> TD1 -> TD2 ..... ... TDn -> TD0
//   m_pBuffer ---->  TB0    TB1    TB2           TBn    TB0
//
//   TD0.. TDn is TD that assigned to these Queue. The Number of TD is determined by
//   dwNumOfTD argument in constructor.
//   TB0.. TBn is memory buffer that assigned to each TD. its size is determinded by
//   dwTDBufferSize argument in constructor
//
//   m_dwHeadIndex & m_dwTailIndex:
//           m_dwHeadIndex is index to head TD that queued into "Hardware". The m_dwTailIndex
//   is the index to Tail (last) TD that Queued into "Hardware".  (Here Hardware is refer to
//   Endpoint discribtor).
//
public:
    TDQueue(IN CPhysMem* const pCPhysMem, DWORD dwNumOfTD, DWORD dwTDBufferSize) ;
    ~TDQueue();
    BOOL QueueInit();
    void QueueDeInit();

    BOOL QueueReInit( DWORD dwTDBufferSize ) {
        if (dwTDBufferSize!= m_dwTDBufferSize) {
            QueueDeInit();
            m_dwTDBufferSize=dwTDBufferSize;
        }
        return QueueInit();
    }
    DWORD GetMaxTDDataSize()const { return m_dwTDBufferSize;};
    BOOL QueueDisable();
    DWORD   IncIndex(DWORD dwIndex) const {
        dwIndex++;
        return (dwIndex<m_dwNumOfTD?dwIndex:0);
    }
    BOOL IsTDQueueFull() const {
        return ( IncIndex(m_dwTailIndex) == m_dwHeadIndex);
    }
    BOOL IsTDQueueEmpty() const { return m_dwTailIndex == m_dwHeadIndex; };
    BOOL AdvanceTDQueueTail() {
        if (!IsTDQueueFull()) {
            m_dwTailIndex = IncIndex(m_dwTailIndex);
            return TRUE;
        }
        else
            return FALSE;
    }
    PBYTE GetTDQueueTailBufferVirtAddr() const {
        return (m_pBuffer!=NULL? m_pBuffer+ m_dwTailIndex*m_dwTDBufferSize: NULL);
    }
    TD * GetTDQueueTail() const {
        return (m_pTDQueue!=NULL? m_pTDQueue+m_dwTailIndex: NULL);
    }
    DWORD GetTDQueueTailPhysAddr() const {
        return (m_pTDQueue!=NULL? m_pTDQueuePhysAddr+m_dwTailIndex*sizeof(TD): 0);
    }

    BOOL  AdvanceTDQueueHead() {
        if (!IsTDQueueEmpty()) {
            m_dwHeadIndex = IncIndex(m_dwHeadIndex);
            return TRUE;
        }
        else
            return FALSE;
    }
    PBYTE GetTDQueueHeadBufferVirtAddr() const {
        return (m_pBuffer!=NULL? m_pBuffer+m_dwHeadIndex*m_dwTDBufferSize: NULL);
    }
    TD * GetTDQueueHead() const {
        return (m_pTDQueue!=NULL? m_pTDQueue+m_dwHeadIndex: NULL);
    }
    DWORD GetTDQueueHeadPhys() const {
        return (m_pTDQueue!=NULL? m_pTDQueuePhysAddr+m_dwHeadIndex*sizeof(TD): 0);
    }


    BOOL     InitTDQueueTailTD( IN       CTransfer *pTransfer,
                              IN CQueuedPipe * pPipe,
                              IN const UCHAR InterruptOnComplete,
                              IN const DWORD PID,
                              IN const USHORT DataToggle,
                              IN const DWORD paBuffer,
                              IN const DWORD MaxLength,
                              IN const BOOL bShortPacketOk = FALSE);

private:
    CPhysMem* const m_pCPhysMem;
    const DWORD     m_dwNumOfTD  ;
    DWORD           m_dwTDBufferSize;
    TD *    m_pTDQueue ;
    DWORD   m_pTDQueuePhysAddr;
    PBYTE   m_pBuffer;
    DWORD   m_pBufferPhysAddr;

    DWORD   m_dwHeadIndex;
    DWORD   m_dwTailIndex;
};

#if ITD_STUFF

class CIsochronousPipe;
//
class CITransfer {
// Isochronous Transfer Managment Class
// m_Transfer.lpvClientBuffer: Contiguous Data Buffer
// m_Transfer.dwFrames: Number of Frame in this transfer
// m_Transfer.aLengths: Array for frame length for each frame.
//
// Transfer is queued and unqueued in TD unit (it is maximum packet size * number of frame each ITD).
// The CIsochronousPipe only can queue limited Amount of ITD (Please refer CQueuePipe Companion
// Class ITDQueue for detail).
// So m_dwNextToQueueIndex and m_dwNextToCompleteIndex is used for manage data Transfer Buffer.
// m_dwNextToQueueFrameIndex and m_dwNextToCompleteFrameIndex is used for manage frames in this transfer
//
// m_dwNextToQueueIndex: it offset to the buffer next to queue into Pipe's Companion Class TDQueue.
//    it also mean that the data before m_dwNextToQueueIndex has been queued into TDQueue.
// m_dwNextToQueueFrameIndex: it offset to the frame next to queue. it coupled with m_dwNextToQueueIndex
//
// m_dwNextToCompleteIndex: it offset to the buffer that wait for the completion notification from
//    pipe. In another word, the data before m_dwNextToCompleteIndex has been completed (done ack)
//    from Pipe.
// m_dwNextToCompleteFrameIndex: if offset to the frame next to complete. it is coupled with
//    m_dwNextToCompleteIndex.
//
public:
    CITransfer(CIsochronousPipe* const  rPipe, STransfer sTransfer,CITransfer *lpNextTransfer = NULL) ;
    ~CITransfer() {ASSERT(m_Transfer.lpfnCallback == NULL);};
    BOOL ScheduleITD();
    CITransfer * SetNextTransfer (CITransfer * lpNextTransfer) {
        CITransfer * pReturn = m_lpNextTransfer;
        m_lpNextTransfer = lpNextTransfer;
        return pReturn;
    }
    BOOL CheckFrame(WORD wMaxPacketSize) const;
    BOOL Canceled();
    BOOL DoneITD();

    BOOL IsCompleteQueue() const { return ( m_dwNextToQueueFrameIndex >= m_Transfer.dwFrames) ;   };
    BOOL IsStartQueue() const { return ( m_dwNextToQueueFrameIndex!=0 ); };

    CITransfer * GetNextTransfer () const { return m_lpNextTransfer; };
    STransfer * GetSTransfer() { return &m_Transfer; };
    CIsochronousPipe * GetPipe () const { return m_rPipe ;};
private:
    STransfer       m_Transfer;
    // additional parameters/data
    CIsochronousPipe * const    m_rPipe;            // pipe on which this transfer was issued
    CITransfer *     m_lpNextTransfer;

    DWORD           m_dwNextToQueueFrameIndex;
    DWORD           m_dwNextToCompleteFrameIndex;
    DWORD           m_dwNextToQueueBufferIndex;
    DWORD           m_dwNextToCompleteBufferIndex ;

    DWORD           m_dwQueueTD;
    DWORD           m_dwFirstError;
};

//
class ITDQueue {
//  Circular ITD Queue and Its Buffer Managment Class
//  This is companion class for CIsochronousPipe.
//
//   m_pTDQueue---->  ITD0 -> ITD1 -> ITD2 ..... ... ITDn -> ITD0
//   m_pBuffer ---->  TB0     TB1     TB2            TBn     TB0
//
//   ITD0.. ITDn is ITD that assigned to these Queue. The Number of TD is culculated by
//   constructor. ITD can have up to 8 frame of Isochronous Transfer. But it also be limited
//   to have 4K physical buffer (can not span 2 pages).
//   TB0.. TBn is memory buffer that assigned to each ITD. its size is culculated by
//   constructor. it is number of Frame multiply by Maximun Packet Size.
//
//   m_dwHeadIndex & m_dwTailIndex:
//           m_dwHeadIndex is index to head TD that queued into "Hardware". The m_dwTailIndex
//   is the index to Tail (last) TD that Queued into "Hardware".  (Here Hardware is refer to
//   Endpoint discribtor).
//

public:
    ITDQueue(IN CPhysMem* const pCPhysMem, DWORD dwNumOfFrame, DWORD dwMaxPacketSize) ;
    ~ITDQueue();
    BOOL QueueInit();
    void QueueDeInit();
    BOOL QueueDisable();
    DWORD   IncIndex(DWORD dwIndex) const {
        dwIndex++;
        return (dwIndex<m_dwNumOfTD?dwIndex:0);
    }
    BOOL IsITDQueueFull() const {  return ( IncIndex(m_dwTailIndex) == m_dwHeadIndex);  };
    BOOL IsITDQueueEmpty() const { return m_dwTailIndex == m_dwHeadIndex; };

    BOOL AdvanceITDQueueTail() {
        if (!IsITDQueueFull()) {
            m_dwTailIndex = IncIndex(m_dwTailIndex);
            return TRUE;
        }
        else
            return FALSE;
    }
    PBYTE GetITDQueueTailBufferVirtAddr() const {
        return (m_pBuffer!=NULL? m_pBuffer+ m_dwTailIndex*m_dwTDBufferSize: NULL);
    }
    ITD * GetITDQueueTail() const {
        return (m_pTDQueue!=NULL? m_pTDQueue+m_dwTailIndex: NULL);
    }
    DWORD GetITDQueueTailPhysAddr() const {
        return (m_pTDQueue!=NULL? m_pTDQueuePhysAddr+m_dwTailIndex*sizeof(ITD): 0);
    }


    BOOL  AdvanceITDQueueHead() {
        if (!IsITDQueueEmpty()) {
            m_dwHeadIndex = IncIndex(m_dwHeadIndex);
            return TRUE;
        }
        else
            return FALSE;
    }
    ITD * GetITDQueueHead() const {
        return (m_pTDQueue!=NULL? m_pTDQueue+m_dwHeadIndex: NULL);
    }
    DWORD GetITDQueueHeadPhys() const {
        return (m_pTDQueue!=NULL? m_pTDQueuePhysAddr+m_dwHeadIndex*sizeof(ITD): 0);
    }
    PBYTE GetITDQueueHeadBufferVirtAddr () const {
        return (m_pBuffer!=NULL? m_pBuffer+m_dwHeadIndex*m_dwTDBufferSize: NULL);
    }

    DWORD GetFrameNumberPerITD () const { return m_dwFramePerTD; };
    BOOL InitITDQueueTailITD( IN  CITransfer *pTransfer,
                              IN CIsochronousPipe * pPipe,
                              IN const USHORT wStartFrame,
                              IN const USHORT wNumOfFrame,
                              IN LPCDWORD     lpFrameLength,
                              IN const UCHAR InterruptOnComplete,
                              IN const DWORD paBuffer,
                              IN const DWORD MaxLength)   ;
private:
    CPhysMem* const m_pCPhysMem;
    DWORD   m_dwTDBufferSize;
    const DWORD   m_dwMaxPacketSize;
    DWORD   m_dwNumOfTD  ;
    DWORD   m_dwFramePerTD ;
    ITD *    m_pTDQueue ;
    DWORD   m_pTDQueuePhysAddr;
    PBYTE   m_pBuffer;
    DWORD   m_pBufferPhysAddr;

    DWORD   m_dwHeadIndex;
    DWORD   m_dwTailIndex;
};

#endif // ITD_STUFF

#endif
