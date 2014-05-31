// All rights reserved Texas Instruments, Inc 2011
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//

//////////////////////////////////////////////////////////////////////////////////////////
//

#pragma warning(push)
#pragma warning(disable: 4510 4512 4610)
#include <windows.h>
#include <ceddk.h>
#include <usbtypes.h>

#include "am33x.h"
#include "drvcommon.h"
#include "cppidma.hpp"
#pragma warning(pop)

#ifdef MUSB_USEDMA

#pragma warning(disable:4068)  // disable warning for unknown pragmas

// Private debug zones
#define CPPI_DBG_ERROR      1
#define CPPI_DBG_CHANNEL    0
#define CPPI_DBG_CHAIN      0
#define CPPI_DBG_TX         0
#define CPPI_DBG_RX         0

#define ZONE_VERBOSE        0
//#define ZONE_WARNING        1
//#define ZONE_ERROR          1
//#define ZONE_INIT           0
#define ZONE_PDD_DMA        0

extern "C" DWORD g_IstThreadPriority;

UINT32 comp_q_tx0[] = { 93, 94, 95, 96, 97, 98, 99,100,101,102,103,104,105,106,107};
UINT32 comp_q_rx0[] = {109,110,111,112,113,114,115,116,117,118,119,120,121,122,123};
UINT32 comp_q_tx1[] = {125,126,127,128,129,130,131,132,133,134,135,136,137,138,139};
UINT32 comp_q_rx1[] = {141,142,143,144,145,146,147,148,149,150,151,152,153,154,155};


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CCppiDmaChannel class

// Constructor
CCppiDmaChannel::CCppiDmaChannel(
    CCppiDmaController* pController,
    UINT8               epNum,
    PfnTransferComplete pCallback)
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+CCppiDmaChannel: 0x%08x, EP %u, 0x%08x\r\n",
        pController, epNum, pCallback));

    if ((pController == NULL) || (pController->PvUsbRegs() == NULL) ||
        (pController->PvCppiRegs() == NULL) || (epNum == 0)){
        ERRORMSG(1, (L"Invalid args!\r\n"));
        DEBUGCHK(FALSE);
        return;
    }

    InitializeCriticalSection(&m_csLock);

    // Save channel info
    m_nRefCount = 0;
    m_pController = pController;
    m_pUsbRegs = pController->PvUsbRegs();
    m_pCppiRegs = pController->PvCppiRegs();
    m_nCppiChannelOffset = pController->m_nCppiChannelOffset;
    m_pvBuffer = NULL;
    m_paBuffer.QuadPart = 0;
    m_cbBuffer = CPPI_MAX_BUFFER;
    m_fIsTeardownPending = FALSE;
    m_fIsOut = FALSE;
    m_chNum = epNum - 1;
    m_epNum = epNum;
    m_qNum = 0;
    m_pCallback = pCallback;
    m_pUsbEpcsrRegs = &m_pUsbRegs->EPCSR[m_epNum];
    m_pCppiChannelRegs = &m_pCppiRegs->CDMACHANNEL[m_chNum + m_nCppiChannelOffset];

    // Defaults
    m_epAddr = 0;
    m_fnAddr = 0;
    m_epType = 0;
    m_epMaxPkt = 0;
    m_nMaxBD = 0;
    m_pvTransferBuffer = NULL;
    m_paTransferBuffer = 0;
    m_nTransferFrames = 0;
    m_pTransferFrameLengths = NULL;
    m_pTransferFrameLengthsActual = NULL;
    m_pdwIsochErrors = NULL;
    m_pdwIsochLengths = NULL;
    m_nTransferLength = 0;
    m_nTransferOptions = 0;
    m_pTransferCookie1 = NULL;
    m_pTransferCookie2 = NULL;
    m_nTransferComplete = 0;
    m_nTransferFramesComplete = 0;
    m_nSegmentPending = 0;
    m_nSegmentFramesPending = 0;
    m_nSegmentFramesComplete = 0;
    m_nSegmentComplete = 0;
    m_fIsRndisMode = FALSE;
    m_fIsCancelPending = FALSE;
    m_nCancelStatus = USB_CANCELED_ERROR;

    // Allocate DMA buffer
    DMA_ADAPTER_OBJECT Adapter;
    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;
    Adapter.BusNumber = 0;

    m_pvBuffer = (UINT8*)HalAllocateCommonBuffer(&Adapter, m_cbBuffer, &m_paBuffer, FALSE);
    DEBUGCHK(m_pvBuffer != NULL);
    if (!m_pvBuffer){
        ERRORMSG(1, (L"Failed to allocate buffer!\r\n"));
        return;
    }

    // Allocate frame lengths array
    m_pTransferFrameLengthsActual = (UINT32*)LocalAlloc(LPTR, CPPI_MAX_DESCR * sizeof(UINT32));
    DEBUGCHK(m_pTransferFrameLengthsActual != NULL);
    if (!m_pTransferFrameLengthsActual){
        ERRORMSG(1, (L"Failed to alloc frame lengths array!\r\n"));
        return;
    }

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"-CCppiDmaChannel: Ch %u, EP %u\r\n", m_chNum, m_epNum));
}

// Desctructor
CCppiDmaChannel::~CCppiDmaChannel()
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL,(L"+~CCppiDmaChannel: %s Ch %u, EP %u\r\n",
              IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum));

    // Make sure we are cleaned up correctly
    DEBUGCHK(!IsInUse());

    // Free frame lengths array
    m_pTransferFrameLengthsActual = (UINT32*)LocalFree((HLOCAL)m_pTransferFrameLengthsActual);
    DEBUGCHK(m_pTransferFrameLengthsActual == NULL);

    // Free DMA buffer
    DMA_ADAPTER_OBJECT Adapter;
    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;
    Adapter.BusNumber = 0;
    HalFreeCommonBuffer(&Adapter, m_cbBuffer, m_paBuffer, m_pvBuffer, FALSE);
    m_pvBuffer = NULL;

    m_pCppiRegs = NULL;
    m_pUsbRegs = NULL;

    DeleteCriticalSection(&m_csLock);

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"-~CCppiDmaChannel\r\n"));
}

// Build next DMA descriptor chain
void CCppiDmaChannel::NextSegment()
{
    DEBUGMSG(CPPI_DBG_CHAIN, (L"+CCppiDmaChannel::NextSegment: %s Ch %u (EP %u/0x%02x/%u/%u)\r\n",
        IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum, m_epAddr, m_epType, m_epMaxPkt));

    DEBUGCHK(m_nSegmentPending == 0);
    DEBUGCHK(m_nSegmentFramesPending == 0);
    DEBUGCHK(m_nMaxBD != 0);

    UINT32 nBytesLeft  = m_nTransferLength - m_nTransferComplete;
    UINT32 nFramesLeft = m_nTransferFrames - m_nTransferFramesComplete;
    UINT32 cmplQNum = IsOut() ? 
		((m_chwInd == 0) ? comp_q_tx0[m_chNum] : comp_q_tx1[m_chNum]) :
		((m_chwInd == 0) ? comp_q_rx0[m_chNum] : comp_q_rx1[m_chNum]);
    
    DEBUGMSG(CPPI_DBG_CHAIN, (L" CCppiDmaChannel::NextSegment: TransferLength %u, BytesLeft %u, FramesLeft %u\r\n",
        m_nTransferLength, nBytesLeft, nFramesLeft));

    // ZLP packets are special cased
    if (m_nTransferLength == 0) {
        // Only OUT direction please
        DEBUGCHK(USB_ENDPOINT_DIRECTION_OUT(m_epAddr));

        HOST_DESCRIPTOR* pHd = m_pController->HdAlloc();
        DEBUGCHK(pHd != NULL);
        if (pHd == NULL)
            return;

        /* Initialise the descriptor */
        pHd->DescInfo    = (USB_CPPI41_DESC_TYPE_HOST   << USB_CPPI41_DESC_TYPE_SHIFT) |
                           (((sizeof(*pHd) - 40) / 4)   << USB_CPPI41_DESC_WORDS_SHIFT) |
                           0;
        pHd->TagInfo     = 0;
        pHd->PacketInfo  = (USB_CPPI41_PKT_TYPE_USB     << USB_CPPI41_PKT_TYPE_SHIFT) |
                           (USB_CPPI41_PKT_RETPLCY_FULL << USB_CPPI41_PKT_RETPLCY_SHIFT) |
                           (USB_CPPI41_DESC_LOC_OFFCHIP << USB_CPPI41_DESC_LOC_SHIFT) |
                           (USB_CPPI_XXCMPL_QMGR        << USB_CPPI41_PKT_RETQMGR_SHIFT) |
                           (cmplQNum                    << USB_CPPI41_PKT_RETQ_SHIFT);
        pHd->BuffLen     = 0;
        pHd->BuffPtr     = m_paBuffer.LowPart;
        pHd->NextPtr     = 0;
        pHd->OrigBuffLen = pHd->BuffLen | 0x80000000;
        pHd->OrigBuffPtr = pHd->BuffPtr;
        pHd->TagInfo2    = (/* isTx */ (m_fIsOut ? 1 : 0) << 9) |
                           (/* Ch */ m_chNum << 4) |
                           (/* EP */ m_epNum);
        pHd->Index       = 0;

        // Special handling for ZLP TX - set length to 1 and set ZLP bit
        if (m_fIsOut) {
            pHd->PacketInfo |= USB_CPPI41_PKT_FLAGS_ZLP;
            pHd->DescInfo |= 1;
            pHd->BuffLen = 1;
            pHd->OrigBuffLen = 1 | 0x80000000;
        }

        DEBUGMSG(CPPI_DBG_CHAIN, (L" CCppiDmaChannel::NextSegment: ZLP BD %u - Next 0x%08x, Buffer 0x%08x, Length %u\r\n",
            0, pHd->NextPtr, pHd->BuffPtr, pHd->BuffLen));

        m_nSegmentPending = 0;
        m_nSegmentFramesPending = 1;

        QueuePush(pHd);
    } else {
        UINT32 i, paBuff, nLength, nThisBD;

        // Build descriptor chain based on transfer length and RNDIS switch.

        // Either virtual or physcal buffer must be supplied
        DEBUGCHK(m_pvTransferBuffer || m_paTransferBuffer);

        if (IsIso()){
            m_nSegmentPending = 0;
            m_nSegmentFramesPending = min(nFramesLeft, CPPI_MAX_DESCR);

            if (m_paTransferBuffer) {
                // For physical buffers the only limit is the number of
                // available descriptors
                for (i = 0; i < m_nSegmentFramesPending; i ++){
                    m_nSegmentPending += m_pTransferFrameLengths[i];
                }
                paBuff = m_paTransferBuffer;
            } else {
                // For virtual buffers we are limited by both the
                // descriptors and the buffer size
                for (i = 0; i < m_nSegmentFramesPending; i ++) {
                    if ((m_nSegmentPending + m_pTransferFrameLengths[i]) > CPPI_MAX_BUFFER) {
                        m_nSegmentFramesPending = i + 1;
                        break;
                    }
                    m_nSegmentPending += m_pTransferFrameLengths[i];
                }
                paBuff = m_paBuffer.LowPart;
            }
            DEBUGCHK(m_nSegmentPending > 0);
        } else {
            // For BULK and INTR perform single packet transfers.
            // When RNDIS mode is in use this will be the entire transfer.
            // For transparent mode we stop on short packets (RX) or stalls (RX/TX).
            m_nSegmentFramesPending = 1;
            m_nSegmentPending = min(nBytesLeft, m_nMaxBD);

            if (m_paTransferBuffer){
                paBuff = m_paTransferBuffer;
            } else {
                // Limited by buffer size
                if (m_nSegmentPending > CPPI_MAX_BUFFER)
                    m_nSegmentPending = CPPI_MAX_BUFFER;

                paBuff = m_paBuffer.LowPart;
            }
            DEBUGCHK(m_nSegmentPending > 0);
        }

        if (!m_paTransferBuffer && IsOut()) {
            memcpy(m_pvBuffer, m_pvTransferBuffer, m_nSegmentPending);
        }

        nLength = m_nSegmentPending;

        for (i = 0; i < m_nSegmentFramesPending; i ++) {
            HOST_DESCRIPTOR* pHd = m_pController->HdAlloc();
            DEBUGCHK(pHd != NULL);
            if (pHd == NULL)
                return;

			nThisBD = (IsIso()) ? m_pTransferFrameLengths[i] : min(m_nMaxBD, nLength);

            nLength -= nThisBD;

            /* Initialise the descriptor */
            pHd->DescInfo    = (USB_CPPI41_DESC_TYPE_HOST   << USB_CPPI41_DESC_TYPE_SHIFT) |
                               (((sizeof(*pHd) - 40) / 4)   << USB_CPPI41_DESC_WORDS_SHIFT) |
                               nThisBD;
            pHd->TagInfo     = 0;
            pHd->PacketInfo  = (USB_CPPI41_PKT_TYPE_USB     << USB_CPPI41_PKT_TYPE_SHIFT) |
                               (USB_CPPI41_PKT_RETPLCY_FULL << USB_CPPI41_PKT_RETPLCY_SHIFT) |
                               (USB_CPPI41_DESC_LOC_OFFCHIP << USB_CPPI41_DESC_LOC_SHIFT) |
                               (USB_CPPI_XXCMPL_QMGR        << USB_CPPI41_PKT_RETQMGR_SHIFT) |
                               (cmplQNum                    << USB_CPPI41_PKT_RETQ_SHIFT);
            pHd->BuffLen     = nThisBD;
            pHd->BuffPtr     = paBuff;
            pHd->NextPtr     = 0;
            pHd->OrigBuffLen = pHd->BuffLen | 0x80000000;
            pHd->OrigBuffPtr = pHd->BuffPtr;
            pHd->TagInfo2    = (/* isTx */ (m_fIsOut ? 1 : 0) << 9) |
                               (/* Ch */ m_chNum << 4) |
                               (/* EP */ m_epNum);
            pHd->Index       = 0;

            DEBUGMSG(CPPI_DBG_CHAIN, 
				(L" CCppiDmaChannel::NextSegment: BD %u - Next 0x%08x, Buffer 0x%08x, Length %u\r\n",
                i, pHd->NextPtr, pHd->BuffPtr, pHd->BuffLen));

            QueuePush(pHd);
            paBuff += nThisBD;
        }
        DEBUGCHK(nLength == 0);
    }

    m_nSegmentFramesComplete = 0;
    m_nSegmentComplete = 0;

    DEBUGCHK(m_nSegmentFramesPending > 0);
    DEBUGMSG(CPPI_DBG_CHAIN, (L"-CCppiDmaChannel::NextSegment: %u BDs in the chain\r\n", m_nSegmentFramesPending));
}

// Perform any post-transfer operations, e.g. advance
// counters/pointers and copy data to the user buffer.
void CCppiDmaChannel::ReleaseSegment()
{
    DEBUGCHK(m_nSegmentFramesPending > 0);

    if (m_nSegmentPending){
        DEBUGCHK(m_pvTransferBuffer || m_paTransferBuffer);

        if (m_paTransferBuffer){
            if (IsIn()){
                if (IsIso()){
                    // For ISO we may need to compress the buffer
                    if (m_nTransferOptions & USB_COMPRESS_ISOCH){
                        PHYSICAL_ADDRESS pa;
                        UINT8 *pvIBuffer;

                        pa.QuadPart = m_paTransferBuffer;
                        pvIBuffer = (UINT8 *)MmMapIoSpace(pa, m_nSegmentPending, FALSE);

                        if (pvIBuffer){
                            // Compress the buffer starting with frame #1
                            UINT8 *pvDesctBuffer = pvIBuffer + m_pTransferFrameLengthsActual[0];
                            UINT8 *pvSrcBuffer = pvIBuffer + m_pTransferFrameLengths[0];

                            for (UINT32 i = 1; i < m_nSegmentFramesComplete; i ++) {
                                memmove(pvDesctBuffer, pvSrcBuffer, m_pTransferFrameLengthsActual[i]);
                                pvDesctBuffer += m_pTransferFrameLengthsActual[i];
                                pvSrcBuffer += m_pTransferFrameLengths[0];
                            }
                            MmUnmapIoSpace(pvIBuffer, m_nSegmentPending);
                        } else {
                            // There is nothing we can do about it!
                            DEBUGCHK(FALSE);
                        }

                        m_paTransferBuffer += m_nSegmentComplete;
					} else {// Uncompressed
                        m_paTransferBuffer += m_nSegmentPending;
                    }
				} else { // Non-ISO
                    m_paTransferBuffer += m_nSegmentComplete;
                }
			} else {// Out
                m_paTransferBuffer += m_nSegmentPending;
            }
		} else {// Virtual buffer
            if (IsIn()) {
                if (IsIso()) {
                    // For ISO the received frames could be smaller than requested,
                    // thus we have to copy the whole buffer unless the client opted
                    // for a compressed transfer.
                    if (m_nTransferOptions & USB_COMPRESS_ISOCH) {
                        UINT8 *pvBuffer = m_pvBuffer;

                        for (UINT32 i = 0; i < m_nSegmentFramesComplete; i ++){
                            memcpy(m_pvTransferBuffer, pvBuffer, m_pTransferFrameLengthsActual[i]);
                            m_pvTransferBuffer += m_pTransferFrameLengthsActual[i];
                            pvBuffer += m_pTransferFrameLengths[i];
                        }
                    } else {
                        // Copy the whole segment
                        memcpy(m_pvTransferBuffer, m_pvBuffer, m_nSegmentPending);
                        m_pvTransferBuffer += m_nSegmentPending;
                    }
                } else {
                    // Copy completed segment to the user buffer
                    memcpy(m_pvTransferBuffer, m_pvBuffer, m_nSegmentComplete);
                    m_pvTransferBuffer += m_nSegmentComplete;
                }
			} else {// Out
                m_pvTransferBuffer += m_nSegmentPending;
            }
        }

        if (IsIso()) {
            // Copy ISO frame lengths
            if (m_pdwIsochLengths) {
                for (UINT32 i = 0; i < m_nSegmentFramesComplete; i ++) {
                    m_pdwIsochLengths[i + m_nTransferFramesComplete] = m_pTransferFrameLengthsActual[i];
                    //RETAILMSG(1, (L"IsochLen[%d] = %d", i+m_nTransferFramesComplete, m_pdwIsochLengths[i]));
                }
            }

            // Set ISO error flags
            if (m_pdwIsochErrors) {
                for (UINT32 i = 0; i < m_nSegmentFramesComplete; i ++) {
                    // Short packets not allowed?
                    if (((m_nTransferLength % (UINT32)m_epMaxPkt) == 0) &&
                        ((m_nTransferOptions & USB_SHORT_TRANSFER_OK) == 0) &&
                        ((m_pTransferFrameLengthsActual[i] % (UINT32)m_epMaxPkt) != 0))
                    {
                        // Indicate underrun error
                        m_pdwIsochErrors[i + m_nTransferFramesComplete] = USB_DATA_UNDERRUN_ERROR;
                        //RETAILMSG(1, (L"IsochErr[%d] = UNDERRUN", i + m_nTransferFramesComplete));
                    } else {
                        m_pdwIsochErrors[i + m_nTransferFramesComplete] = USB_NO_ERROR;
                        //RETAILMSG(1, (L"IsochErr[%d] = NO_ERROR", i + m_nTransferFramesComplete));
                    }
                }
            }
        }

        m_nTransferComplete += m_nSegmentComplete;
        m_nSegmentComplete = 0;
    }

    if (m_pTransferFrameLengths) {
        m_pTransferFrameLengths += m_nSegmentFramesComplete;
    }

    m_nTransferFramesComplete += m_nSegmentFramesComplete;
    m_nSegmentFramesComplete = 0;

    m_nSegmentPending = 0;
    m_nSegmentFramesPending = 0;
}

void CCppiDmaChannel::ProcessCompletedTeardown(TEARDOWN_DESCRIPTOR* pTd)
{
#ifndef SHIP_BUILD
    WCHAR* szDir = m_fIsOut ? L"OUT" : L"IN";
#endif
    DEBUGMSG(ZONE_PDD_DMA, (L"+CCppiDmaChannel::ProcessCompletedTeardown: %s Ch %u\r\n", szDir, m_chNum));

    if (pTd == NULL) { 
		ERRORMSG(TRUE, (L" CCppiDmaChannel::ProcessCompletedTeardown: %s Ch %u, NULL TD\r\n", szDir, m_chNum));
    } else {
        ERRORMSG(m_fIsTeardownPending == FALSE,
            (L" CCppiDmaChannel::ProcessCompletedTeardown: %s Ch %u - Not requested for this channel\r\n",
            szDir, m_chNum));

        m_pController->QueuePush(USB_CPPI_TDFREE_QNUM, pTd);
        m_fIsTeardownPending = FALSE;
    }
    DEBUGMSG(ZONE_PDD_DMA, (L"-CCppiDmaChannel::ProcessCompletedTeardown: %s Ch %u\r\n", szDir, m_chNum));
}

void CCppiDmaChannel::UpdateRndisMode(BOOL fIsRndisMode)
{
	volatile UINT32* pRndisReg = NULL;
    UINT32 rndisRegVal;
    UINT32 mode;

    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE, (L"+CCppiDmaChannel::UpdateRndisMode: %s\n", 
		     fIsRndisMode ? L"TRUE" : L"FALSE"));

    /* Mode
       00: Transparent
       01: RNDIS
       10: CDC
       11: Generic RNDIS
    */
	pRndisReg = (m_fIsOut) ? &m_pUsbRegs->TXMODE : &m_pUsbRegs->RXMODE;

	rndisRegVal = *pRndisReg;
    mode = 0x3 << (m_chNum * 2);

    if (fIsRndisMode)
        rndisRegVal |= mode;
    else
        rndisRegVal &= ~mode;

    if (*pRndisReg != rndisRegVal)
        *pRndisReg = rndisRegVal;

    m_fIsRndisMode = fIsRndisMode;

    // The generic RNDIS size register is only used for receive channels
    if (fIsRndisMode && !m_fIsOut) {
        // Round transfer size up to multiple of EP size
        UINT32 rndisSize = ((m_nTransferLength + m_epMaxPkt - 1) / m_epMaxPkt) * m_epMaxPkt;
        if (rndisSize == 0)
            rndisSize = m_epMaxPkt;

        // Set the Generic RNDIS EP reg
        switch (m_chNum) {
			case 0  : m_pUsbRegs->GENRNDISSZ1  = rndisSize; break;
			case 1  : m_pUsbRegs->GENRNDISSZ2  = rndisSize; break;
			case 2  : m_pUsbRegs->GENRNDISSZ3  = rndisSize; break;
			case 3  : m_pUsbRegs->GENRNDISSZ4  = rndisSize; break;
			case 4  : m_pUsbRegs->GENRNDISSZ5  = rndisSize; break;
			case 5  : m_pUsbRegs->GENRNDISSZ6  = rndisSize; break;
			case 6  : m_pUsbRegs->GENRNDISSZ7  = rndisSize; break;
			case 7  : m_pUsbRegs->GENRNDISSZ8  = rndisSize; break;
			case 8  : m_pUsbRegs->GENRNDISSZ9  = rndisSize; break;
			case 9  : m_pUsbRegs->GENRNDISSZ10 = rndisSize; break;
			case 10 : m_pUsbRegs->GENRNDISSZ11 = rndisSize; break;
			case 11 : m_pUsbRegs->GENRNDISSZ12 = rndisSize; break;
			case 12 : m_pUsbRegs->GENRNDISSZ13 = rndisSize; break;
			case 13 : m_pUsbRegs->GENRNDISSZ14 = rndisSize; break;
			case 14 : m_pUsbRegs->GENRNDISSZ15 = rndisSize; break;
			default:
				ERRORMSG(TRUE,
					(L" CCppiDmaChannel::UpdateRndisMode: ERROR - Invalid channel number %u!\r\n", m_chNum));
				break;
        }
    }

    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE,
        (L"-CCppiDmaChannel::UpdateRndisMode: Ch %u MODE RegVal 0x%08x\n", m_chNum, *pRndisReg));
}

void CCppiDmaChannel::KickCompletionCallback()
{
    USBCDMA_KickCompletionCallback(m_pController->m_hUsbCdma);
}

void CCppiDmaChannel::QueuePush(void* pD)
{
    m_pController->QueuePush(m_qNum, pD);
}

void* CCppiDmaChannel::QueuePop()
{
    return m_pController->QueuePop(m_qNum);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CCppiDmaRxChannel class

class CCppiDmaRxChannel : public CCppiDmaChannel
{
public:
    CCppiDmaRxChannel(
        CCppiDmaController* pController,
        UINT8 epNum,
        PfnTransferComplete pCallback);
   ~CCppiDmaRxChannel();

    BOOL IssueTransfer(UINT8 epAddr, UINT8 fnAddr, UINT8 epType, UINT16 epMaxPkt,
        PVOID pvBuffer, UINT32 paBuffer, UINT32 nLength, UINT32 nFrames, UINT32 *pnFrameLengths,
        DWORD *pdwIsochErrors, DWORD *pdwIsochLengths,
        UINT32 nOptions, PVOID pCookie1, PVOID pCookie2);

    BOOL CancelTransfer();
    BOOL ScheduleTransfer();
    BOOL ValidateTransferState();

    void ProcessCompletedPacket(HOST_DESCRIPTOR* pHd);
};

// Construction
CCppiDmaRxChannel::CCppiDmaRxChannel(CCppiDmaController *pController,
    UINT8 epNum, PfnTransferComplete pCallback)
    :
    CCppiDmaChannel(pController, epNum, pCallback)
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+CCppiDmaRxChannel\r\n"));

    m_fIsOut = FALSE;
    m_qNum = pController->m_nRxQueueOffset + m_chNum;

    // Disable EP interrupt
    m_pUsbRegs->IRQ_EN_CLR_0 = (1 << (m_epNum + USB_OTG_RXINT_SHIFT));

    // Enable Rx for this channel in the DMA scheduler
//    USBCDMA_ConfigureScheduleRx(m_chNum + m_nCppiChannelOffset, TRUE);

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"-CCppiDmaRxChannel\r\n"));
}

// Destruction
CCppiDmaRxChannel::~CCppiDmaRxChannel()
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL,
        (L"+~CCppiDmaRxChannel: %s Ch %u, EP %u\r\n", IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum));

//    USBCDMA_ConfigureScheduleRx(m_chNum + m_nCppiChannelOffset, FALSE); // Disable Rx for this channel in the DMA scheduler

    // Flush FIFO and clear status bits
    m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR |
                             MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;
    
    m_pUsbRegs->IRQ_STATUS_0 = 1 << (m_epNum + USB_OTG_RXINT_SHIFT); // Clear EP interrupt
    m_pUsbRegs->IRQ_EN_SET_0 = 1 << (m_epNum + USB_OTG_RXINT_SHIFT); // Restore EP interrupt

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+~CCppiDmaRxChannel\r\n"));
}

// Initiate RX DMA transfer
BOOL CCppiDmaRxChannel::IssueTransfer(UINT8 epAddr, UINT8 fnAddr, UINT8 epType, UINT16 epMaxPkt,
    PVOID pvBuffer, UINT32 paBuffer, UINT32 nLength, UINT32 nFrames, UINT32 *pnFrameLengths,
    DWORD *pdwIsochErrors, DWORD *pdwIsochLengths,
    UINT32 nOptions, PVOID pCookie1, PVOID pCookie2)
{
    BOOL fResult = FALSE;

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE,
        (L"+CCppiDmaRxChannel::IssueTransfer: RX Ch %u (EP %u) - %u bytes\r\n", m_chNum, m_epNum, nLength));

    if (!nLength) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Zero length\r\n"));
        goto _doneUnsafe;
    }

    // Validate input
    if (!pvBuffer && !paBuffer) {
        RETAILMSG(CPPI_DBG_ERROR,
            (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Invalid args, pv %08X, pa %08X\r\n",
            pvBuffer, paBuffer));

        goto _doneUnsafe;
    }

    // Validate ISO frames
    if (epType == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        if (!nFrames || !pnFrameLengths) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Invalid iso args, frames %08X, lengths %08X\r\n",
                nFrames, pnFrameLengths));

            goto _doneUnsafe;
        }

        nLength = 0;
        for (UINT32 n = 0; n < nFrames; n ++) {
            if (!pnFrameLengths[n] || (pnFrameLengths[n] > (UINT32)epMaxPkt)) {
                RETAILMSG(CPPI_DBG_ERROR,
                    (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Invalid iso args, frame %08X, size %08X\r\n",
                    n, pnFrameLengths[n]));

                goto _doneUnsafe;
            }

            nLength += pnFrameLengths[n];
        }
    }

    if (!nLength) {
        RETAILMSG(CPPI_DBG_ERROR,
            (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Invalid args, zero transfer length\r\n"));
        goto _doneUnsafe;
    }

    ChLock();

    // We are not supporting overlapped I/O by now
    if (IsInUse()) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaRxChannel::IssueTransfer: ERROR - Pending transfer, length %u\r\n",
            m_nTransferLength));

        goto _done;
    }

    // Extra debug checks
    DEBUGCHK(m_pvTransferBuffer == NULL);
    DEBUGCHK(m_paTransferBuffer == 0);
    DEBUGCHK(m_nTransferLength == 0);
    DEBUGCHK(m_nSegmentPending == 0);
    DEBUGCHK(m_nSegmentFramesPending == 0);

    // Save transfer info
    m_epAddr = epAddr;
    m_fnAddr = fnAddr;
    m_epType = epType;
    m_epMaxPkt = epMaxPkt;

    m_pvTransferBuffer = (UINT8 *)pvBuffer;
    m_paTransferBuffer = paBuffer;
    m_nTransferLength = nLength;
    m_nTransferFrames = nFrames;
    m_pTransferFrameLengths = pnFrameLengths;
    m_pdwIsochErrors = pdwIsochErrors;
    m_pdwIsochLengths = pdwIsochLengths;
    m_nTransferOptions = nOptions;
    m_pTransferCookie1 = pCookie1;
    m_pTransferCookie2 = pCookie2;
    m_nTransferComplete = 0;
    m_nTransferFramesComplete = 0;
    m_nSegmentPending = 0;
    m_nSegmentFramesPending = 0;

    BOOL fIsRndisMode;

	fIsRndisMode = (!IsIso()) ? ((m_epMaxPkt & 0x3f) == 0) && (m_nTransferLength <= 0x10000) : FALSE;

	m_nMaxBD = (fIsRndisMode) ? 0x10000 : m_epMaxPkt;

    DEBUGMSG(CPPI_DBG_RX,
        (L" CCppiDmaRxChannel::IssueTransfer: RndisMode %d, epMaxPkt %u, MaxBD %u\r\n",
         fIsRndisMode, m_epMaxPkt, m_nMaxBD));

    UpdateRndisMode(fIsRndisMode);

    if (!IsIso()){
        m_nTransferFrames = m_nTransferLength / m_nMaxBD;
        if (m_nTransferLength % m_nMaxBD)
            m_nTransferFrames ++;
    }

    DEBUGMSG(CPPI_DBG_RX, (L" CCppiDmaRxChannel::IssueTransfer: Length %u, %u frames, options 0x%08x\r\n",
        m_nTransferLength, m_nTransferFrames, m_nTransferOptions));

    m_pCppiChannelRegs->HPCRA = ((m_chNum + m_nCppiChannelOffset) | ((m_chNum + m_nCppiChannelOffset) << 16));
    m_pCppiChannelRegs->HPCRB = ((m_chNum + m_nCppiChannelOffset) | ((m_chNum + m_nCppiChannelOffset) << 16));

    m_pCppiChannelRegs->RXGCR =
        BIT31 | /* Enable */
        BIT24 | /* Retry on starvation */
        BIT14 | /* Host descriptor type (default) */ /*
        qmgr |
        qnum? */ ((m_chwInd == 0) ? comp_q_rx0[m_chNum] : comp_q_rx1[m_chNum]);

    // Schedule next transfer
    fResult = ScheduleTransfer();

_done:

    ChUnlock();

_doneUnsafe:
    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE, (L"-CCppiDmaRxChannel::IssueTransfer\r\n"));

    return fResult;
}

// Cancel pending RX transfer if any
BOOL CCppiDmaRxChannel::CancelTransfer()
{
    BOOL fResult = FALSE;

#ifndef SHIP_BUILD
    WCHAR* szDir = m_fIsOut ? L"OUT" : L"IN";
#endif

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE,
        (L"+CCppiDmaRxChannel::CancelTransfer: %s Ch %u (EP %u)\r\n", szDir, m_chNum, m_epNum));

    ChLock();

    // Only if there is a pending DMA transfer
    if (IsInUse()) {
        m_fIsTeardownPending = TRUE;

        // 1. Disable receiver, flush RX FIFO and clear status bits
        m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_H_RXSTALL |
            MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;

        // 2. Set USB controller TD bit
        m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_RXTD_SHIFT);

        // 3. Initiate channel tear-down
        m_pCppiChannelRegs->RXGCR =
            BIT31 | /* Enable */
            BIT30 | /* Tear-down */
            BIT24 | /* Retry on starvation */
            BIT14 | /* Host descriptor type (default) */ /*
            qmgr |
            qnum */ ((m_chwInd == 0) ? comp_q_rx0[m_chNum] : comp_q_rx1[m_chNum]);

        // 4. Wait for CDMA completion and retry (due to TX disconnect issue)
        if (m_fIsTeardownPending)
            Sleep(10);

        DWORD tc0 = GetTickCount();
        while ((m_fIsTeardownPending) && ((GetTickCount() - tc0) < 2000)) {
            m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_RXTD_SHIFT);
            KickCompletionCallback();
            Sleep(10);
        }

        /* 5. Set USB controller TD bit */
        m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_RXTD_SHIFT);

        /* 6. Flush the EP's FIFO */
        m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR |
            MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;

        if (m_fIsTeardownPending) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaRxChannel::CancelTransfer: %s Ch %u (EP %u) - Timed-out (lost teardown descriptor)!\r\n",
                szDir, m_chNum, m_epNum));
        }

        // Pop all aborted BDs from the channel's queue and free them
        for(;;) {
            HOST_DESCRIPTOR* pHd = (HOST_DESCRIPTOR*)QueuePop();
            if (pHd != NULL) {
                m_pController->HdFree(pHd);
                DEBUGMSG(ZONE_WARNING,
                    (L" CCppiDmaRxChannel::CancelTransfer: Recovered %s BD on queue %u after tear-down\r\n",
                    szDir, m_qNum));
            }
            else
                break;
        }

        DEBUGMSG(CPPI_DBG_RX, (L" CCppiDmaRxChannel::CancelTransfer: %s - Cancelled %u/%u\r\n",
            szDir, m_nTransferComplete, m_nTransferLength));

        // Cancel complete
        m_fIsCancelPending = FALSE;

        // Let the client know we are canceling this transfer
        if (m_pCallback) {
            m_pCallback( this, m_nCancelStatus, m_nTransferLength, m_nTransferComplete,
                0, m_pTransferCookie1, m_pTransferCookie2);
        }

        // Cleanup
        m_pvTransferBuffer = NULL;
        m_paTransferBuffer = 0;
        m_nTransferLength = 0;
        m_nTransferFrames = 0;
        m_nSegmentPending = 0;
        m_nSegmentFramesPending = 0;
        m_nCancelStatus = USB_CANCELED_ERROR;

        // Done
        fResult = TRUE;
    }

    ChUnlock();

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE, (L"-CCppiDmaRxChannel::CancelTransfer: %s Ch %u (EP %u) - %s\r\n",
        szDir, m_chNum, m_epNum, m_fIsTeardownPending ? L"FAILED!" : L"SUCCEEDED"));

    return fResult;
}

// Schedule next RX DMA segment
BOOL CCppiDmaRxChannel::ScheduleTransfer()
{
    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE, (L"+CCppiDmaRxChannel::ScheduleTransfer: %s Ch %u (EP %u)\r\n",
        IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum));

    if (!m_nTransferLength || (!m_pvTransferBuffer && !m_paTransferBuffer)) {
        RETAILMSG(CPPI_DBG_ERROR,
            (L" CCppiDmaRxChannel::ScheduleTransfer: ERROR - Invalid state, pv 0x%08x, pa 0x%08x, length %u\r\n",
            m_pvTransferBuffer, m_paTransferBuffer, m_nTransferLength));

        return FALSE;
    }

    DEBUGCHK(m_nSegmentPending == 0);
    DEBUGCHK(m_nSegmentFramesPending == 0);

    // A channel that has been torn down must be re-enabled before re-use
    if ((m_pCppiChannelRegs->RXGCR & BIT31) == 0)
        m_pCppiChannelRegs->RXGCR =
            BIT31 | /* Enable */
            BIT24 | /* Retry on starvation */
            BIT14 | /* Host descriptor type (default) */ /*
            qmgr |
            qnum? */ ((m_chwInd == 0) ? comp_q_rx0[m_chNum] : comp_q_rx1[m_chNum]);

    // See if we can use AUTOREQ mode
    UINT32 nAutoreqMask = 0;
    if (IsRndisMode()) {
        // In RNDIS mode we can use autoreq for all packets by using the "all but EOP" option.
        nAutoreqMask = (0x1 << (m_chNum * 2));
    }

    // Update Autoreq reg
    UINT32 nAutoreq = m_pUsbRegs->AUTOREQR & ~(0x3 << (m_chNum * 2));
    nAutoreq |= nAutoreqMask;
    m_pUsbRegs->AUTOREQR = nAutoreq;

    // Create HDs for next segment
    NextSegment();

    // Enable DMA mode, trigger IN token
    UINT16 nCsr = MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_H_REQPKT;

    if (m_nTransferComplete == 0) {
        if (m_nTransferOptions & USB_TOGGLE_CARRY)
            nCsr |= (MGC_M_RXCSR_H_WR_DATATOGGLE | MGC_M_RXCSR_H_DATATOGGLE);
        else
            nCsr |= MGC_M_RXCSR_CLRDATATOG;
    }

    m_pUsbEpcsrRegs->RXCSR = nCsr;

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE, (L"-CCppiDmaRxChannel::ScheduleTransfer\r\n"));

    return TRUE;
}

// Validate transfer state
BOOL CCppiDmaRxChannel::ValidateTransferState()
{
    BOOL fStateOK = TRUE;

    if (IsInUse()) {
        // Check for errors
        UINT16 nCsr = m_pUsbEpcsrRegs->RXCSR;
        if (nCsr & (MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERROR)) {
            DEBUGMSG(CPPI_DBG_ERROR, (L" CCppiDmaRxChannel:: ERROR - Ch %u, EP %u, CSR 0x%08x\r\n",
                m_chNum, m_epNum, nCsr));

            // Simply disable receiver, flush RX FIFO and leave status bits set
            m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_FLUSHFIFO |
                MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;

			m_nCancelStatus = (nCsr & MGC_M_RXCSR_H_RXSTALL) ? USB_STALL_ERROR : USB_DEVICE_NOT_RESPONDING_ERROR;

            // Need to perform a cancel (teardown) to clear out all HDs.  This is picked up
            // from the processing thread in CCppiDmaController::ValidateTransferState().
            m_fIsCancelPending = TRUE;
            fStateOK = FALSE;
        } else if ((nCsr & MGC_M_RXCSR_DMAENAB) != MGC_M_RXCSR_DMAENAB) {
            DEBUGMSG(ZONE_WARNING,
                (L" CCppiDmaRxChannel::ValidateTransferState: WARNING - RX DMA reset, EP %u, CSR 0x%08x\r\n",
                m_epNum, nCsr));
            m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_H_RXSTALL |
                MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;
        }
    }

    return fStateOK;
}

void CCppiDmaRxChannel::ProcessCompletedPacket(HOST_DESCRIPTOR* pHd)
{
    BOOL fTransferComplete = FALSE;
    BOOL fSegmentComplete = FALSE;
    UINT32 nStatus = USB_NO_ERROR;
    UINT32 nTransferred = 0;
    UINT16 nCsr;

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE,(L"+CCppiDmaRxChannel::ProcessCompletedPacket: %s Ch %u (EP %u)\r\n",
        m_fIsOut ? L"OUT" : L"IN", m_chNum, m_epNum));

    if (pHd == NULL) {
        DEBUGMSG(ZONE_WARNING, (L"-CCppiDmaRxChannel::ProcessCompletedPacket: ERROR - NULL HD\r\n"));
        return;
    }

    if (pHd->NextPtr != 0) {
        DEBUGMSG(ZONE_WARNING, (L" CCppiDmaRxChannel::ProcessCompletedPacket: WARNING - Linked HD\r\n"));
    }

    if (m_fIsTeardownPending) {
        DEBUGMSG(ZONE_WARNING,
            (L"-CCppiDmaRxChannel::ProcessCompletedPacket: WARNING - Dropped (pending teardown)\r\n"));
        // Free the HD
        m_pController->HdFree(pHd);
        return;
    }

    ChLock();

    if (!IsInUse()) {
        DEBUGMSG(ZONE_WARNING,
            (L"-CCppiDmaRxChannel::ProcessCompletedPacket: WARNING - Dropped (transfer cancelled)\r\n"));
        goto _done;
    }

    if (!ValidateTransferState())
        goto _done;

    // Some extra debug validation
    DEBUGCHK(m_pvTransferBuffer || m_paTransferBuffer);
    DEBUGCHK(m_nTransferLength > 0);
    DEBUGCHK(m_nSegmentPending > 0);
    DEBUGCHK(m_nSegmentComplete < m_nSegmentPending);
    DEBUGCHK(m_nSegmentFramesComplete < m_nSegmentFramesPending);

    // Count the frames in this packet (and their sizes)
    m_nSegmentFramesComplete++;
    nTransferred = pHd->BuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK;

    // Check for zero length packet
    if (pHd->PacketInfo & USB_CPPI41_PKT_FLAGS_ZLP)
        nTransferred = 0;

    m_nSegmentComplete += nTransferred;

    DEBUGMSG(CPPI_DBG_RX,
             (L" CCppiDmaRxChannel::ProcessCompletedPacket: Packet %u (of %u), bytes %u\r\n",
              m_nSegmentFramesComplete, m_nSegmentFramesPending, nTransferred));

    if (IsIso()) {
        // Store the actual frame length for ISO
        m_pTransferFrameLengthsActual[m_nSegmentFramesComplete-1] = nTransferred;
    }

    // Check for segment complete
    if (IsRndisMode()) {
        // In Rndis mode, there is only one BD so segment has completed
        fSegmentComplete = TRUE;
	} else  {// Transparent mode
        // Check for short packet. Isochronous endpoints handled elsewhere.
        if (!IsIso() && (nTransferred < m_epMaxPkt)) {
            fSegmentComplete = TRUE;
            fTransferComplete = TRUE;
            if (((m_nTransferLength % (UINT32)m_epMaxPkt) == 0) &&
                ((m_nTransferOptions & USB_SHORT_TRANSFER_OK) == 0)) {
                // Indicate underrun error if short packets are not accepted
                nStatus = USB_DATA_UNDERRUN_ERROR;
            }
        } else {
            // Check to see if the segment is complete because
            // all the expected frames have been received
            if (m_nSegmentFramesComplete == m_nSegmentFramesPending)
                fSegmentComplete = TRUE;
        }
    }        

    // Check for transfer complete
    if ((m_nTransferFramesComplete + m_nSegmentFramesComplete) == m_nTransferFrames){
        // Finished transfer
        fTransferComplete = TRUE;
    }

    DEBUGMSG(CPPI_DBG_RX,
        (L" CCppiDmaRxChannel::ProcessCompletedPacket: Received %u/%u, %u/%u (%u of %u frames)\r\n",
        m_nSegmentComplete, m_nSegmentPending, m_nTransferComplete,
        m_nTransferLength, m_nSegmentFramesComplete, m_nSegmentFramesPending));

    if (fSegmentComplete) {
        nCsr = m_pUsbEpcsrRegs->RXCSR;

        // Turn things off and clear sticky bits
        m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR |
                                 MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY;

        // Update user buffer
        ReleaseSegment();

        if (fTransferComplete) {
            DEBUGMSG(CPPI_DBG_RX, (L" CCppiDmaRxChannel::ProcessCompletedPacket: Completed %u/%u, status %x\r\n",
                m_nTransferComplete, m_nTransferLength, nStatus));

            // Let the client know the transfer has been completed
            if (m_pCallback){
                // Save information for the callback below, before cleaning-up
                UINT32 nTransferLength = m_nTransferLength;
                UINT32 nTransferComplete = m_nTransferComplete;

                // Cleanup *before* the callback (to prevent IssueTransfer calls while
                // m_nTransferLength and m_nTransferFrames are still non-zero)
                m_pvTransferBuffer = NULL;
                m_paTransferBuffer = 0;
                m_nTransferLength = 0;
                m_nTransferFrames = 0;
                m_nSegmentPending = 0;
                m_nSegmentFramesPending = 0;

                // Let the client know the transfer has been completed
                m_pCallback( this, nStatus, nTransferLength, nTransferComplete,
                    (nCsr & MGC_M_RXCSR_H_DATATOGGLE) ? USB_TOGGLE_CARRY : 0,
                    m_pTransferCookie1, m_pTransferCookie2);
            }
        } else {
            // Schedule next transfer
            ScheduleTransfer();
        }
    } else {
        // Trigger next IN token
        m_pUsbEpcsrRegs->RXCSR = MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_H_REQPKT;
    }

_done:
    // Free the HD
    m_pController->HdFree(pHd);

    ChUnlock();

    DEBUGMSG(CPPI_DBG_RX || ZONE_VERBOSE, (L"-CCppiDmaRxChannel::ProcessCompletedPacket\r\n"));
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CCppiDmaTxChannel class

class CCppiDmaTxChannel : public CCppiDmaChannel
{
public:
    CCppiDmaTxChannel(
        CCppiDmaController* pController,
        UINT8 epNum,
        PfnTransferComplete pCallback);
   ~CCppiDmaTxChannel();

    BOOL IssueTransfer(UINT8 epAddr, UINT8 fnAddr, UINT8 epType, UINT16 epMaxPkt,
        PVOID pvBuffer, UINT32 paBuffer, UINT32 nLength, UINT32 nFrames, UINT32 *pnFrameLengths,
        DWORD *pdwIsochErrors, DWORD *pdwIsochLengths,
        UINT32 nOptions, PVOID pCookie1, PVOID pCookie2);

    BOOL  CancelTransfer();
    BOOL  ScheduleTransfer();
    BOOL  ValidateTransferState();

    void  ProcessCompletedPacket(HOST_DESCRIPTOR* pHd);

    void  OnTransferComplete();
    BOOL  IsFifoEmpty() const;
    void  KickDrainThread();
    DWORD DrainThread();

    static DWORD WINAPI DrainThreadStub(LPVOID lpParameter);

    BOOL m_fDrainThreadClosing;
    HANDLE m_hDrainEvent;
    HANDLE m_hDrainThread;
    UINT32 m_nStatus;
};

// Constructor
CCppiDmaTxChannel::CCppiDmaTxChannel(CCppiDmaController *pController,
    UINT8 epNum, PfnTransferComplete pCallback)
    :
    CCppiDmaChannel(pController, epNum, pCallback)
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+CCppiDmaTxChannel\r\n"));

    m_fIsOut = TRUE;
    m_qNum = pController->m_nTxQueueOffset + (2 * m_chNum);

    m_fDrainThreadClosing = FALSE;
    m_hDrainEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hDrainThread = CreateThread(NULL, 0, DrainThreadStub, this, 0, NULL);
    CeSetThreadPriority(m_hDrainThread, g_IstThreadPriority + 1);
    m_nStatus = USB_NO_ERROR;

    // Disable EP interrupt
    m_pUsbRegs->IRQ_EN_CLR_0 = (1 << (m_epNum + USB_OTG_TXINT_SHIFT));

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"-CCppiDmaTxChannel\r\n"));
}

// Destructor
CCppiDmaTxChannel::~CCppiDmaTxChannel()
{
    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+~CCppiDmaTxChannel: %s Ch %u, EP %u\r\n",
             IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum));

    m_fDrainThreadClosing = TRUE;
    SetEvent(m_hDrainEvent);
    if (WaitForSingleObject(m_hDrainThread, 5000) != WAIT_OBJECT_0 ) {
        DEBUGCHK(0);
#pragma warning(push)
#pragma warning(disable: 6258)
        TerminateThread(m_hDrainThread, (DWORD)-1);
#pragma warning(pop)
    }
    CloseHandle(m_hDrainThread);
    m_hDrainThread = NULL;
    CloseHandle(m_hDrainEvent);
    m_hDrainEvent = NULL;

    // Flush FIFO and clear status bits
    m_pUsbEpcsrRegs->TXCSR = MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_H_NAKTIMEOUT |
        MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY;

    // Clear EP interrupt
    m_pUsbRegs->IRQ_STATUS_0 = (1 << (m_epNum + USB_OTG_TXINT_SHIFT));

    // Restore EP interrupt
    m_pUsbRegs->IRQ_EN_SET_0 = (1 << (m_epNum + USB_OTG_TXINT_SHIFT));

    DEBUGMSG(ZONE_INIT || CPPI_DBG_CHANNEL, (L"+~CCppiDmaTxChannel\r\n"));
}

// Initiate TX DMA transfer
BOOL CCppiDmaTxChannel::IssueTransfer(UINT8 epAddr, UINT8 fnAddr, UINT8 epType, UINT16 epMaxPkt,
    PVOID pvBuffer, UINT32 paBuffer, UINT32 nLength, UINT32 nFrames, UINT32 *pnFrameLengths,
    DWORD *pdwIsochErrors, DWORD *pdwIsochLengths,
    UINT32 nOptions, PVOID pCookie1, PVOID pCookie2)
{
    BOOL fResult = FALSE;

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"+CCppiDmaTxChannel::IssueTransfer: TX Ch %u (EP %u/0x%02x/%u/%u) - %u bytes\r\n",
        m_chNum, m_epNum, epAddr, epType, epMaxPkt, nLength));

    // Validate input
    if (nLength && (!pvBuffer && !paBuffer)) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaTxChannel::IssueTransfer: ERROR - Invalid args, pv %08X, pa %08X, length %08X",
            pvBuffer, paBuffer, nLength));
        goto _doneUnsafe;
    }

    // Validate ISO frames
    if (epType == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        if (!nFrames || !pnFrameLengths) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaTxChannel::IssueTransfer: ERROR - Invalid iso args, frames %08X, lengths %08X\r\n",
                nFrames, pnFrameLengths));
            goto _doneUnsafe;
        }

        nLength = 0;
        for (UINT32 n = 0; n < nFrames; n ++) {
            if (!pnFrameLengths[n] || (pnFrameLengths[n] > (UINT32)epMaxPkt)) {
                RETAILMSG(CPPI_DBG_ERROR,
                    (L" CCppiDmaTxChannel::IssueTransfer: ERROR - Invalid iso args, frame %08X, size %08X\r\n",
                    n, pnFrameLengths[n]));
                goto _doneUnsafe;
            }
            nLength += pnFrameLengths[n];
        }

        if (!nLength) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaTxChannel::IssueTransfer: ERROR - Invalid ISO args, zero transfer length\r\n"));
            goto _doneUnsafe;
        }
    }

    ChLock();

    // We are not supporting overlapped I/O by now
    if (IsInUse()) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaTxChannel::IssueTransfer: ERROR - Pending transfer, length %u",
            m_nTransferLength));
        goto _done;
    }

    // Extra debug checks
    DEBUGCHK(m_pvTransferBuffer == NULL);
    DEBUGCHK(m_paTransferBuffer == 0);
    DEBUGCHK(m_nTransferLength == 0);
    DEBUGCHK(m_nSegmentPending == 0);
    DEBUGCHK(m_nSegmentFramesPending == 0);

    // Save transfer info
    m_epAddr = epAddr;
    m_fnAddr = fnAddr;
    m_epType = epType;
    m_epMaxPkt = epMaxPkt;

    m_pvTransferBuffer = (UINT8 *)pvBuffer;
    m_paTransferBuffer = paBuffer;
    m_nTransferLength = nLength;
    m_nTransferFrames = nFrames;
    m_pTransferFrameLengths = pnFrameLengths;
    m_pdwIsochErrors = pdwIsochErrors;
    m_pdwIsochLengths = pdwIsochLengths;
    m_nTransferOptions = nOptions;
    m_pTransferCookie1 = pCookie1;
    m_pTransferCookie2 = pCookie2;
    m_nTransferComplete = 0;
    m_nTransferFramesComplete = 0;
    m_nSegmentPending = 0;
    m_nSegmentFramesPending = 0;

    BOOL fIsRndisMode;
	fIsRndisMode = (!IsIso()) ? ((m_epMaxPkt & 0x3f) == 0) && (m_nTransferLength <= 0x10000) : FALSE;
	m_nMaxBD = (fIsRndisMode) ? 0x10000 : m_epMaxPkt;

    DEBUGMSG(CPPI_DBG_TX, (L" CCppiDmaTxChannel::IssueTransfer: RndisMode %d, epMaxPkt %u, MaxBD %u\r\n",
         fIsRndisMode, m_epMaxPkt, m_nMaxBD));

    UpdateRndisMode(fIsRndisMode);

    if (!IsIso()) {
        if (m_nTransferLength) {
            m_nTransferFrames = m_nTransferLength / m_nMaxBD;

            if (m_nTransferLength % m_nMaxBD)
                m_nTransferFrames ++;
        } else {
            m_nTransferFrames = 1;
        }
    }

    DEBUGMSG(CPPI_DBG_TX,
        (L" CCppiDmaTxChannel::IssueTransfer: Length %u, %u frames, options 0x%08x\r\n",
        m_nTransferLength, m_nTransferFrames, m_nTransferOptions));

    m_pCppiChannelRegs->TXGCR =
        BIT31 | /* Enable */ /*
        qmgr |
        qnum? */ ((m_chwInd == 0) ? comp_q_tx0[m_chNum] : comp_q_tx1[m_chNum]);

    // Schedule next transfer
    fResult = ScheduleTransfer();

_done:
    ChUnlock();

_doneUnsafe:
    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::IssueTransfer\r\n"));

    return fResult;
}

// Cancel pending TX transfer if any
BOOL CCppiDmaTxChannel::CancelTransfer()
{
    BOOL fResult = FALSE;

#ifndef SHIP_BUILD
    WCHAR* szDir = m_fIsOut ? L"OUT" : L"IN";
#endif

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE,(L"+CCppiDmaTxChannel::CancelTransfer: %s Ch %u (EP %u)\r\n",
        szDir, m_chNum, m_epNum));

    ChLock();

    // Only if there is a pending DMA transfer
    if (IsInUse()){
        m_fIsTeardownPending = TRUE;

        // 1. Initiate channel tear-down
        m_pCppiChannelRegs->TXGCR =
            BIT31 | /* Enable */
            BIT30 | /* Tear-down */ /*
            qmgr |
            qnum */ ((m_chwInd == 0) ? comp_q_tx0[m_chNum] : comp_q_tx1[m_chNum]);

        // 2. Set USB controller TD bit
        m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_TXTD_SHIFT);

        // 3. Wait for CDMA completion and retry (due to TX disconnect issue)
        if (m_fIsTeardownPending)
            Sleep(10);

        DWORD tc0 = GetTickCount();
        while ((m_fIsTeardownPending) && ((GetTickCount() - tc0) < 2000)) {
            m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_TXTD_SHIFT);
            KickCompletionCallback();
            Sleep(10);
        }

        // 4. Set USB controller TD bit
        m_pUsbRegs->TEARDOWN |= ((1 << m_epNum) << USB_OTG_TXTD_SHIFT);

        // 5. Flush FIFO and clear status bits
        m_pUsbEpcsrRegs->TXCSR = MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_H_NAKTIMEOUT |
            MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY;

        if (m_fIsTeardownPending) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaTxChannel::CancelTransfer: %s Ch %u (EP %u) - Timed-out (lost teardown descriptor)!\r\n",
                szDir, m_chNum, m_epNum));
        }

        // Pop all aborted BDs from the channel's queue and free them
        for(;;) {
            HOST_DESCRIPTOR* pHd = (HOST_DESCRIPTOR*)QueuePop();
            if (pHd != NULL) {
                m_pController->HdFree(pHd);
                DEBUGMSG(ZONE_WARNING/*0*/,
                    (L" CCppiDmaTxChannel::CancelTransfer: Recovered %s BD on queue %u after tear-down\r\n",
                    szDir, m_qNum));
            }
            else
                break;
        }

        DEBUGMSG(CPPI_DBG_TX, (L" CCppiDmaTxChannel::CancelTransfer: %s - Cancelled %u/%u\r\n",
            szDir, m_nTransferComplete, m_nTransferLength));

        // Cancel complete
        m_fIsCancelPending = FALSE;

        // Let the client know we are canceling this transfer
        if (m_pCallback) {
            m_pCallback( this, m_nCancelStatus, m_nTransferLength, m_nTransferComplete,
                0, m_pTransferCookie1, m_pTransferCookie2);
        }

        // Cleanup
        m_pvTransferBuffer = NULL;
        m_paTransferBuffer = 0;
        m_nTransferLength = 0;
        m_nTransferFrames = 0;
        m_nSegmentPending = 0;
        m_nSegmentFramesPending = 0;
        m_nCancelStatus = USB_CANCELED_ERROR;
        
        fResult = TRUE; // Done 
    }

    ChUnlock();

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::CancelTransfer: %s Ch %u (EP %u) - %s\r\n",
        szDir, m_chNum, m_epNum, m_fIsTeardownPending ? L"FAILED!" : L"SUCCEEDED"));

    return fResult;
}

// Schedule next TX DMA segment
BOOL CCppiDmaTxChannel::ScheduleTransfer()
{
    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"+CCppiDmaTxChannel::ScheduleTransfer: %s Ch %u (EP %u)\r\n",
        IsOut() ? L"OUT" : L"IN", m_chNum, m_epNum));

    if (m_nTransferLength) {
        if (!m_pvTransferBuffer && !m_paTransferBuffer) {
            RETAILMSG(CPPI_DBG_ERROR,
                (L" CCppiDmaTxChannel::ScheduleTransfer: ERROR - Invalid state, pv 0x%08x, pa 0x%08x, length %u\r\n",
                m_pvTransferBuffer, m_paTransferBuffer, m_nTransferLength));
            return FALSE;
        }
    }

    DEBUGCHK(m_nSegmentPending == 0);
    DEBUGCHK(m_nSegmentFramesPending == 0);

    // A channel that has been torn down must be re-enabled before re-use
    if ((m_pCppiChannelRegs->TXGCR & BIT31) == 0)
        m_pCppiChannelRegs->TXGCR =
            BIT31 | /* Enable */ /*
            qmgr |
            qnum? */ ((m_chwInd == 0) ? comp_q_tx0[m_chNum] : comp_q_tx1[m_chNum]);

    // Enable DMA mode
    UINT16 nCsr = MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB;

    if (m_nTransferComplete == 0) {
        if (m_nTransferOptions & USB_TOGGLE_CARRY)
            nCsr |= (MGC_M_TXCSR_H_WR_DATATOGGLE | MGC_M_TXCSR_H_DATATOGGLE);
        else
            nCsr |= MGC_M_TXCSR_CLRDATATOG;
    }

    m_pUsbEpcsrRegs->TXCSR = nCsr;

    // Create HDs for next segment
    NextSegment();

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::ScheduleTransfer\r\n"));

    return TRUE;
}

// Validate transfer state
BOOL CCppiDmaTxChannel::ValidateTransferState()
{
    BOOL fStateOK = TRUE;

    if (IsInUse()){
        // Check for errors
        UINT16 nCsr = m_pUsbEpcsrRegs->TXCSR;
        if (nCsr &(MGC_M_TXCSR_H_NAKTIMEOUT | MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR)){
            DEBUGMSG(CPPI_DBG_ERROR,(L" CCppiDmaTxChannel:: ERROR - Ch %u, EP %u, CSR 0x%08x\r\n",
                m_chNum, m_epNum, nCsr));

            // Flush FIFO and leave status bits set
            m_pUsbEpcsrRegs->TXCSR = MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_FLUSHFIFO |
                MGC_M_TXCSR_H_NAKTIMEOUT | MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY;

			m_nCancelStatus = (nCsr & MGC_M_TXCSR_H_RXSTALL) ? USB_STALL_ERROR : USB_DEVICE_NOT_RESPONDING_ERROR;

            // Need to perform a cancel (teardown) to clear out all HDs.  This is picked up
            // from the processing thread in CCppiDmaController::ValidateTransferState().
            m_fIsCancelPending = TRUE;
            fStateOK = FALSE;
        } else if ((nCsr & (MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB)) != (MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB)){
            DEBUGMSG(ZONE_WARNING,
                (L" CCppiDmaTxChannel::ValidateTransferState: WARNING - TX MODE/DMA reset, EP %u, CSR 0x%08x\r\n",
                m_epNum, nCsr));

            m_pUsbEpcsrRegs->TXCSR = MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_H_NAKTIMEOUT |
                MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY;
        }
    }

    return fStateOK;
}

void CCppiDmaTxChannel::ProcessCompletedPacket(HOST_DESCRIPTOR* pHd)
{
    BOOL fTransferComplete = FALSE;
    BOOL fSegmentComplete = FALSE;
    UINT32 nTransferred;

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE,
        (L"+CCppiDmaTxChannel::ProcessCompletedPacket: %s Ch %u (EP %u)\r\n",
        m_fIsOut ? L"OUT" : L"IN", m_chNum, m_epNum));

    if (pHd == NULL) {
        DEBUGMSG(ZONE_WARNING, (L"-CCppiDmaTxChannel::ProcessCompletedPacket: ERROR - NULL HD\r\n"));
        return;
    }

    if (pHd->NextPtr != 0) {
        DEBUGMSG(ZONE_WARNING, (L" CCppiDmaTxChannel::ProcessCompletedPacket: WARNING - Linked HD\r\n"));
    }

    if (m_fIsTeardownPending) {
        DEBUGMSG(ZONE_WARNING, (L"-CCppiDmaTxChannel::ProcessCompletedPacket: WARNING - Dropped (pending teardown)\r\n"));
        // Free the HD
        m_pController->HdFree(pHd);
        return;
    }

    ChLock();

    if (!IsInUse()) {
        DEBUGMSG(ZONE_WARNING,(L"-CCppiDmaTxChannel::ProcessCompletedPacket: WARNING - Dropped (transfer cancelled)\r\n"));
        goto _done;
    }

    if (!ValidateTransferState())
        goto _done;

    // Some extra debug validation
    if (m_nTransferLength) {
        DEBUGCHK(m_pvTransferBuffer || m_paTransferBuffer);
        DEBUGCHK(m_nSegmentPending > 0);
        DEBUGCHK(m_nSegmentComplete < m_nSegmentPending);
        DEBUGCHK(m_nSegmentFramesComplete < m_nSegmentFramesPending);
    } else {
        DEBUGCHK(m_nSegmentFramesPending == 1);
    }

    m_nStatus = USB_NO_ERROR;

    // Count the frames in this packet (and their sizes)
    m_nSegmentFramesComplete++;
    nTransferred = pHd->BuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK;
    m_nSegmentComplete += nTransferred;

    DEBUGMSG(CPPI_DBG_TX, (L" CCppiDmaTxChannel::ProcessCompletedPacket: Packet %u (of %u), bytes %u\r\n",
              m_nSegmentFramesComplete, m_nSegmentFramesPending, nTransferred));

    if (IsIso()){
        // Store the actual frame length for ISO
        m_pTransferFrameLengthsActual[m_nSegmentFramesComplete-1] = nTransferred;
    }

    // Check for segment complete
    if (IsRndisMode()){
        // In Rndis mode, there is only one BD so segment has completed
        fSegmentComplete = TRUE;
	} else {// Transparent mode
        // Check to see if the segment is complete because
        // all the expected frames have been transferred
        if (m_nSegmentFramesComplete == m_nSegmentFramesPending)
            fSegmentComplete = TRUE;
    }

    // Check for transfer complete
    if ((m_nTransferFramesComplete + m_nSegmentFramesComplete) == m_nTransferFrames){
        // Finished transfer
        fTransferComplete = TRUE;
    }

    DEBUGMSG(CPPI_DBG_TX, (L" CCppiDmaTxChannel::ProcessCompletedPacket: Sent %u/%u, %u/%u\r\n",
        m_nSegmentComplete,m_nSegmentPending, m_nTransferComplete, m_nTransferLength));

    if (fSegmentComplete) {
        // Update user buffer
        ReleaseSegment();

        if (fTransferComplete){
            DEBUGMSG(CPPI_DBG_TX,
                (L" CCppiDmaTxChannel::ProcessCompletedPacket: Completed %u/%u, status 0x%x\r\n",
                m_nTransferComplete, m_nTransferLength, m_nStatus));

            if (IsFifoEmpty())
                OnTransferComplete();
            else
                KickDrainThread();
        } else {
            // Schedule next transfer
            ScheduleTransfer();
        }
    } else {
        // Nothing to do for TX transfers - wait for next HD
    }

_done:
    // Free the HD
    m_pController->HdFree(pHd);

    ChUnlock();

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::ProcessCompletedPacket\r\n"));
}

void CCppiDmaTxChannel::OnTransferComplete()
{
    UINT32 nCsr;

    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"+CCppiDmaTxChannel::OnTransferComplete: %s Ch %u (EP %u)\r\n",
        m_fIsOut ? L"OUT" : L"IN", m_chNum, m_epNum));

	StallExecution(20);
    // Need to check for errors (e.g. stalls) AFTER packet written 
    // out to bus.  Transfer will be cancelled on error.
    if (!ValidateTransferState())
        goto _done;

    nCsr = m_pUsbEpcsrRegs->TXCSR;

    // Ensure TX MODE and DMAENAB remain set (they are sometimes reset)
    m_pUsbRegs->TXCSR = MGC_M_TXCSR_MODE | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_H_NAKTIMEOUT |
		MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY;

    // Save information for the callback below, before cleaning-up
    UINT32 nTransferLength = m_nTransferLength;
    UINT32 nTransferComplete = m_nTransferComplete;

    // Cleanup *before* the callback (to prevent IssueTransfer calls while m_nTransferLength and
    // m_nTransferFrames are still non-zero)
    m_pvTransferBuffer = NULL;
    m_paTransferBuffer = 0;
    m_nTransferLength = 0;
    m_nTransferFrames = 0;
    m_nSegmentPending = 0;
    m_nSegmentFramesPending = 0;

    // Let the client know the transfer has been completed
    if (m_pCallback) {
        m_pCallback( this, m_nStatus, nTransferLength, nTransferComplete,
            (nCsr & MGC_M_TXCSR_H_DATATOGGLE) ? USB_TOGGLE_CARRY : 0,
            m_pTransferCookie1, m_pTransferCookie2);
    }

_done:
    DEBUGMSG(CPPI_DBG_TX || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::OnTransferComplete\r\n"));
}

BOOL CCppiDmaTxChannel::IsFifoEmpty() const
{
    return ((m_pUsbEpcsrRegs->TXCSR & MGC_M_TXCSR_TXPKTRDY) == 0);
}

void CCppiDmaTxChannel::KickDrainThread()
{
    SetEvent(m_hDrainEvent);
}

DWORD CCppiDmaTxChannel::DrainThread()
{
    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaTxChannel::DrainThread\n"));

    while (!m_fDrainThreadClosing) {
        DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L" CCppiDmaTxChannel::DrainThread: Waiting...\r\n"));

        WaitForSingleObject(m_hDrainEvent, INFINITE);

        DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L" CCppiDmaTxChannel::DrainThread: Released\r\n"));

        while ((!m_fDrainThreadClosing) && (!IsFifoEmpty()))
            Sleep(1);

        if (!m_fDrainThreadClosing)
            OnTransferComplete();
    }

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaTxChannel::DrainThread\n"));

    return 0;
}

DWORD WINAPI CCppiDmaTxChannel::DrainThreadStub(LPVOID lpParameter)
{
    return ((CCppiDmaTxChannel*)lpParameter)->DrainThread();
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CCppiDmaController class

// Constructor
CCppiDmaController::CCppiDmaController(DWORD dwDescriptorCount) :
    m_dwDescriptorCount(dwDescriptorCount)
{
    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::CCppiDmaController\r\n"));

    InitializeCriticalSection(&m_csLock);
    m_paUsbRegs = 0;
    m_pUsbRegs = NULL;
    m_pCppiRegs = NULL;
    m_hUsbCdma = NULL;

    m_nCppiChannelOffset = 0;
    m_nRxQueueOffset = USB_CPPI_RX_QNUM;
    m_nTxQueueOffset = USB_CPPI_TX_QNUM;

    memset(m_pTxChannels, 0, sizeof(m_pTxChannels));
    memset(m_pRxChannels, 0, sizeof(m_pRxChannels));

#ifdef USB_STAT
	memset(m_TxChaAllocCnt,   0, sizeof(m_TxChaAllocCnt));
	memset(m_TxChaReleaseCnt, 0, sizeof(m_TxChaReleaseCnt));
	memset(m_RxChaAllocCnt,   0, sizeof(m_RxChaAllocCnt));
	memset(m_RxChaReleaseCnt, 0, sizeof(m_RxChaReleaseCnt));
		
	m_hdAllocCnt = 0;
	m_hdFreeCnt  = 0;
#endif	

    m_pvPool = NULL;
    m_paPool.QuadPart = 0;
    m_cbPoolSize = 0;
    InitializeCriticalSection(&m_csPoolLock);

    m_pvHdPool = NULL;
    m_pvHdPoolHead = NULL;

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::CCppiDmaController\r\n"));
}

// Destructor
CCppiDmaController::~CCppiDmaController()
{
    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+~CCppiDmaController::CCppiDmaController\r\n"));

    DEBUGCHK(m_paUsbRegs == 0);
    DEBUGCHK(m_pvPool == 0);
    DEBUGCHK(m_pvHdPoolHead == 0);

    DeleteCriticalSection(&m_csPoolLock);
    DeleteCriticalSection(&m_csLock);

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-~CCppiDmaController::CCppiDmaController\r\n"));
}

// Initialize CPPI controller
BOOL CCppiDmaController::Initialize(UINT32 paUdcBase, PVOID pvUdcBase, PVOID pvCppiBase, UINT8 chwInd)
{
    BOOL fResult = FALSE;

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::Initialize\r\n"));

	m_chwInd = chwInd;

	if (m_chwInd == 0){	
		m_nCppiChannelOffset = 0;
		m_nRxQueueOffset = USB_CPPI_RX_QNUM;
		m_nTxQueueOffset = USB_CPPI_TX_QNUM;
	} else {
    	m_nCppiChannelOffset = 15;
    	m_nRxQueueOffset = USB_CPPI_RX_QNUM + 15 ;
    	m_nTxQueueOffset = USB_CPPI_TX_QNUM + 15 * 2;
	}
	
    PoolInit();
    Lock();

    // Validate input
    if (!paUdcBase || !pvUdcBase || !pvCppiBase){
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaController::Initialize: ERROR - Invalid parametrs\r\n"));
        goto _done;
    }

    // Do not allow multiple instances
    if (m_paUsbRegs) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaController::Initialize: ERROR - Already initialized\r\n"));
        goto _done;
    }

    // Save input parameters
    m_paUsbRegs = paUdcBase;
    m_pUsbRegs  = (CSL_UsbRegs*)pvUdcBase;
    m_pCppiRegs = (CSL_CppiRegs*)pvCppiBase;

    // Disable RNIDS and AUTOREQ modes
    m_pUsbRegs->AUTOREQR = 0;
    m_pUsbRegs->TXMODE  = 0;
    m_pUsbRegs->RXMODE  = 0;

    fResult = TRUE;

_done:
    Unlock();

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::Initialize\r\n"));

    return fResult;
}

// Deinitialize CPPI controller and release
// any used resources
void CCppiDmaController::Deinitialize()
{
    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::Deinitialize\r\n"));

    Lock();

    if (m_paUsbRegs) {
        UINT32 i;

        // Abort any pending OUT transfers and release TX DMA channels
        for (i = 0; i < USB_CPPI_MAX_TX_CHANNELS; i ++) {
            if (m_pTxChannels[i]) {
                DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"CCppiDmaController::Deinitialize: Closing TX channel\r\n", i));
                m_pTxChannels[i]->CancelTransfer();
                delete m_pTxChannels[i];
                m_pTxChannels[i] = NULL;
            }
        }

        // Abort any pending IN transfers and release RX DMA channels
        for (i = 0; i < USB_CPPI_MAX_RX_CHANNELS; i ++) {
            if (m_pRxChannels[i]) {
                DEBUGMSG(ZONE_INIT || ZONE_VERBOSE,
                    (L"CCppiDmaController::Deinitialize: Closing RX channel\r\n", i));
                m_pRxChannels[i]->CancelTransfer();
                delete m_pRxChannels[i];
                m_pRxChannels[i] = NULL;
            }
        }

        m_paUsbRegs = 0;
    }

    Unlock();

    PoolDeinit();

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::Deinitialize\r\n"));
}

// Allocate CPPI DMA channel if available
CCppiDmaChannel *CCppiDmaController::AllocChannel( UINT8 epNum, UINT8 epAddr, PfnTransferComplete pCallback)
{
    UINT8 chNum = epNum - 1;
    CCppiDmaChannel* pChannel = NULL;

    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE, (L"+CCppiDmaController::AllocChannel: EP %u/0x%02x\r\n",
        epNum, epAddr));

    if (chNum >= USB_CPPI_MAX_CHANNELS/2) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaController::AllocChannel: ERROR - Channel out of range %u/%u\r\n",
            chNum, USB_CPPI_MAX_CHANNELS/2));
        goto _doneUnsafe;
    }

    Lock();

    if (!m_paUsbRegs) {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaController::AllocChannel: ERROR - DMA controller not initialized\r\n"));
        goto _doneSafe;
    }

    if (USB_ENDPOINT_DIRECTION_OUT(epAddr)) {
        // Allocate OUT channel
        if (!m_pTxChannels[chNum])
            m_pTxChannels[chNum] = new CCppiDmaTxChannel(this, epNum, pCallback);

        pChannel = m_pTxChannels[chNum];
#ifdef USB_STAT
		m_TxChaAllocCnt[chNum]++;
#endif
    } else {
        // Allocate IN channel
        if (!m_pRxChannels[chNum])
            m_pRxChannels[chNum] = new CCppiDmaRxChannel(this, epNum, pCallback);

        pChannel = m_pRxChannels[chNum];
#ifdef USB_STAT
		m_RxChaAllocCnt[chNum]++;
#endif
    }

	pChannel->m_chwInd = m_chwInd;
	
    if (pChannel) {
        pChannel->AddRef();

        DEBUGMSG(CPPI_DBG_CHANNEL, (L" CCppiDmaController::AllocChannel: AddRef(%u) %s Ch %u (EP %u/0x%02x)\r\n",
            pChannel->GetRefCount(), USB_ENDPOINT_DIRECTION_OUT(epAddr) ? L"OUT" : L"IN",
            chNum, epNum, epAddr));
    } else {
        RETAILMSG(CPPI_DBG_ERROR, (L" CCppiDmaController::AllocChannel: ERROR - Out of memory\r\n"));
    }

_doneSafe:
    Unlock();

_doneUnsafe:
    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE, (L"-CCppiDmaController::AllocChannel: EP %u/0x%02x\r\n", epNum, epAddr));

    return pChannel;
}

// Release CPPI DMA channel
void CCppiDmaController::ReleaseChannel(CCppiDmaChannel *pChannel)
{
    UINT8 chNum;
    BOOL isOut;

    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE,(L"+CCppiDmaController::ReleaseChannel\r\n"));

    DEBUGCHK(pChannel != NULL);
    if(pChannel != NULL) {
        Lock();

        DEBUGCHK(pChannel->GetRefCount());
        DEBUGCHK(m_paUsbRegs != 0);

        chNum = pChannel->ChannelNumber();
        isOut = pChannel->IsOut();

        DEBUGCHK(chNum < USB_CPPI_MAX_CHANNELS/2);
        
        pChannel->CancelTransfer();  // Cancel pending operation if any
        pChannel->Release();         // Release channel ref

#ifdef USB_STAT
		if (isOut)
			m_TxChaReleaseCnt[chNum]++;
		else		
			m_RxChaReleaseCnt[chNum]++;
#endif

        if (!pChannel->GetRefCount()){
            if (isOut){
                if(chNum < USB_CPPI_MAX_CHANNELS/2) {
                    DEBUGCHK(pChannel == m_pTxChannels[chNum]);
                    m_pTxChannels[chNum] = NULL;
                }
            } else {
                if(chNum < USB_CPPI_MAX_CHANNELS/2) {
                    DEBUGCHK(pChannel == m_pRxChannels[chNum]);
                    m_pRxChannels[chNum] = NULL;
                }
            }

            delete pChannel;
        }

        Unlock();
    }

    DEBUGMSG(ZONE_PDD_DMA || ZONE_VERBOSE, (L"-CCppiDmaController::ReleaseChannel\r\n"));
}

void CCppiDmaController::QueuePush(BYTE qNum, void* pD)
{
    UINT32 value = 0;
    UINT32 type = 0;

    if (pD != NULL){
        UINT32 addr  = DescriptorVAtoPA(pD);
        UINT32 size  = 0;

        type = (*(UINT32*)pD & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;
        switch (type) {
			case USB_CPPI41_DESC_TYPE_HOST:     size = (CPPI_HD_SIZE - 24) / 4;     break;
			case USB_CPPI41_DESC_TYPE_TEARDOWN: size = (USB_CPPI_TD_SIZE - 24) / 4; break;
			default: ERRORMSG(TRUE, (L"Invalid descriptor type %u\r\n", type));
        }

        value = ((addr & QMGR_QUEUE_N_REG_D_DESC_ADDR_MASK) | (size & QMGR_QUEUE_N_REG_D_DESCSZ_MASK   ));
    }

    m_pCppiRegs->QMQUEUEMGMT[qNum].QCTRLD = value;

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE,
        (L" CCppiDmaController::QueuePush: type %d, queue %u, value 0x%08x\r\n", type, qNum, value));
}

void* CCppiDmaController::QueuePop(BYTE qNum)
{
    UINT32 value = m_pCppiRegs->QMQUEUEMGMT[qNum].QCTRLD & QMGR_QUEUE_N_REG_D_DESC_ADDR_MASK;

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE,
        (L" CCppiDmaController::QueuePop: queue %u, value 0x%08x\r\n", qNum, value));

    return DescriptorPAtoVA(value);
}

BOOL CCppiDmaController::ValidateTransferState()
{
    BOOL fStateOK = TRUE;

    for (unsigned i = 0; i < USB_CPPI_MAX_TX_CHANNELS; i ++){
        if (m_pRxChannels[i]){
            if (!m_pRxChannels[i]->ValidateTransferState() || m_pRxChannels[i]->IsCancelPending()){
                m_pRxChannels[i]->CancelTransfer();
                fStateOK = FALSE;
            }
        }

        if (m_pTxChannels[i]){
            if (!m_pTxChannels[i]->ValidateTransferState() || m_pTxChannels[i]->IsCancelPending()){
                m_pTxChannels[i]->CancelTransfer();
                fStateOK = FALSE;
            }
        }
    }

    return fStateOK;
}

void CCppiDmaController::ProcessCompletionEvent(void* pD)
{
	UINT32 bufflen = 0;
	UINT32 timeout = 50;
    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L"+CCppiDmaController::ProcessCompletionEvent: 0x%08x\r\n", pD));

    DEBUGCHK(pD != NULL);
    if (pD == NULL)
        return;

    UINT32 type = (*(UINT32*)pD & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L" CCppiDmaController::ProcessCompletionEvent: type %u\r\n", type));

    switch (type) {
		case USB_CPPI41_DESC_TYPE_HOST: /* Host Descriptor */
        {
            HOST_DESCRIPTOR* pHd = (HOST_DESCRIPTOR*)pD;
            CCppiDmaChannel* pChannel = NULL;
            BYTE chanNum  = (BYTE)((pHd->TagInfo2 & 0x1f0) >> 4);
            BOOL transmit = (pHd->TagInfo2 & 0x200) ? TRUE : FALSE;
			pChannel = (transmit) ? m_pTxChannels[chanNum] : m_pRxChannels[chanNum];

			if (!transmit){
				while(--timeout){
					bufflen = pHd->BuffLen & USB_CPPI41_HD_BUF_LENGTH_MASK;
					if (bufflen != 0)
						break;
					StallExecution(1);
				}
				if (bufflen==0){
					ERRORMSG(1,(L"USBH: Race condition!!! rxBD read before updated by DMA\r\n"));
					break;
				}
			}

            if (pChannel)
                pChannel->ProcessCompletedPacket(pHd);
            else
                DEBUGMSG(ZONE_ERROR, (L" CCppiDmaController::ProcessCompletionEvent(HD): ERROR - Ch %u deleted\r\n",
                    chanNum));
            break;
        }

		case USB_CPPI41_DESC_TYPE_TEARDOWN: /* Teardown Descriptor */
        {
            TEARDOWN_DESCRIPTOR* pTd = (TEARDOWN_DESCRIPTOR*)pD;
            CCppiDmaChannel* pChannel = NULL;
            BYTE chanNum  = (BYTE)((pTd->DescInfo & 0x0001f) >> 0);
            BOOL transmit = (pTd->DescInfo & 0x10000) ? FALSE : TRUE;
			pChannel = (transmit)? m_pTxChannels[chanNum - m_nCppiChannelOffset] :
								   m_pRxChannels[chanNum - m_nCppiChannelOffset];

            if (pChannel)
                pChannel->ProcessCompletedTeardown(pTd);
            else
                DEBUGMSG(ZONE_ERROR,
                    (L" CCppiDmaController::ProcessCompletionEvent(TD): ERROR - Ch %u deleted\r\n",
                    chanNum - m_nCppiChannelOffset));
            break;
        }

    default:
        ERRORMSG(TRUE, (L"Unknown descriptor type %u\r\n", type));
    }

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L"-CCppiDmaController::ProcessCompletionEvent\r\n"));
}

void CCppiDmaController::OnCompletionEvent()
{
    UINT32 pending2, pending3, pending4;
	BYTE qnum;
	void*  pD;

	UINT32 pendmask2 = 0;
	UINT32 pendmask3 = 0;
	UINT32 pendmask4 = 0;
	

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE,
        (L"+CCppiDmaController::OnCompletionEvent\r\n"));

	if (m_chwInd == 0){
		pendmask2 = USB_CPPI_PEND2_QMSK_P0;
		pendmask3 = USB_CPPI_PEND3_QMSK_P0;
	} else {
		pendmask3 = USB_CPPI_PEND3_QMSK_P1;
		pendmask4 = USB_CPPI_PEND4_QMSK_P1;
	}

	pending2 = m_pCppiRegs->PEND2;
	pending3 = m_pCppiRegs->PEND3;
	pending4 = m_pCppiRegs->PEND4;
	while ((pending2 & pendmask2) || (pending3 & pendmask3) ||
		   (pending4 & pendmask4))
	{
		if (m_chwInd == 0){
			// TX from port0
			for (qnum = 93; qnum < 96; qnum++){
				if (pending2 & QUEUE_N_BITMASK(qnum)){ 
            		pD = QueuePop(qnum);
            		ProcessCompletionEvent(pD);
				}
        	}
			// TX from port0 (continue)
			for (qnum = 96; qnum < 108; qnum++){
				if (pending3 & QUEUE_N_BITMASK(qnum)){ 
					void* pD = QueuePop(qnum);
					ProcessCompletionEvent(pD);
				}
			}
		}
		if (m_chwInd == 1){
			// TX from port1
			for (qnum = 125; qnum < 128; qnum++){
				if (pending3 & QUEUE_N_BITMASK(qnum)){ 
            		pD = QueuePop(qnum);
            		ProcessCompletionEvent(pD);
				}
			}
			// TX from port1 (continue)
			for (qnum = 128; qnum < 140; qnum++){
				if (pending3 & QUEUE_N_BITMASK(qnum)){ 
					void* pD = QueuePop(qnum);
            ProcessCompletionEvent(pD);
        }
			}
		}

		if (m_chwInd == 0){
			// RX from port0
			for (qnum = 109; qnum < 124; qnum++){
				if (pending3 & QUEUE_N_BITMASK(qnum)){ 
            		pD = QueuePop(qnum);
            ProcessCompletionEvent(pD);
        }
			}
		}

		if (m_chwInd == 1){
			// RX from port1
			for (qnum = 141; qnum < 156; qnum++){
				if (pending4 & QUEUE_N_BITMASK(qnum)){ 
					void* pD = QueuePop(qnum);
            ProcessCompletionEvent(pD);
        }
			}
		}
		pending2 = m_pCppiRegs->PEND2;
		pending3 = m_pCppiRegs->PEND3;
		pending4 = m_pCppiRegs->PEND4;
    }

    DEBUGMSG(ZONE_PDD_DMA && ZONE_VERBOSE, (L"-CCppiDmaController::OnCompletionEvent\r\n"));
}

VOID CCppiDmaController::CompletionCallback(PVOID param)
{
    if (param == NULL) {
        ERRORMSG(TRUE, (L" CCppiDmaController::CompletionCallback: ERROR - Invalid param 0x%08x\r\n", param));
        return;
    }

    ((CCppiDmaController*)param)->OnCompletionEvent();
}

BOOL CCppiDmaController::PoolInit()
{
    // Only initialise once
    if (m_pvPool != NULL)
        return TRUE;

    UINT16 nHdCount = (UINT16)m_dwDescriptorCount;

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::PoolInit: %u HDs\r\n", nHdCount));

    PoolLock();

    m_hUsbCdma = USBCDMA_RegisterUsbModule( nHdCount, CPPI_HD_SIZE, &m_paPool, &m_pvPool,
        CompletionCallback, this, m_chwInd + 1);

    m_cbPoolSize = nHdCount * CPPI_HD_SIZE;
    m_pvHdPool = NULL;
    m_pvHdPoolHead = NULL;

    PoolUnlock();

    ERRORMSG(m_pvPool == NULL, (L"ERROR: Failed to allocate %u bytes for the descriptor pool\r\n", m_cbPoolSize));

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::PoolInit: %s - Allocated space for %u HDs\r\n",
        (m_pvPool != NULL) ? L"SUCCEEDED" : L"FALIED", nHdCount));

    if (m_pvPool)
        HdPoolInit();

    return (m_pvPool != NULL);
}

void CCppiDmaController::PoolDeinit()
{
    // Only deinitialise once
    if (m_pvPool == NULL)
        return;

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::PoolDeinit\r\n"));

    PoolLock();

    m_pvHdPool = NULL;
    m_pvHdPoolHead = NULL;

    USBCDMA_DeregisterUsbModule(m_hUsbCdma);
    m_hUsbCdma = NULL;

    m_cbPoolSize = 0;
    m_paPool.QuadPart = 0;
    m_pvPool = NULL;

    PoolUnlock();

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::PoolDeinit\r\n"));
}

void CCppiDmaController::HdPoolInit()
{
    unsigned nHdCount = m_dwDescriptorCount;
    unsigned n = 0;

    // Only initialise once
    if (m_pvHdPool != NULL)
        goto done;

    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"+CCppiDmaController::HdPoolInit: %u HDs\r\n", nHdCount));

    // The descriptor pool must be initialised before the host descriptor pool
    PoolInit();
    DEBUGCHK(m_pvPool != NULL);

    // The host descriptor pool begins at the base of the descriptor pool
    HOST_DESCRIPTOR* pHd = (HOST_DESCRIPTOR*)m_pvPool;
    m_pvHdPool = (HOST_DESCRIPTOR*)&pHd[0];

    PoolLock();

    // Build the host descriptor free list
    m_pvHdPoolHead = NULL;
    for (n = 0; n < nHdCount; n++, pHd++) {
        // Set descriptor type to 'host'
        pHd->DescInfo = (UINT32)(USB_CPPI41_DESC_TYPE_HOST << USB_CPPI41_DESC_TYPE_SHIFT);

        // Host descriptors have their physical address in their 'addr' member
        pHd->addr = DescriptorVAtoPA(pHd);

        HdFree(pHd);
        DEBUGMSG(0 /*ZONE_INIT && ZONE_VERBOSE*/, (L"HD %04u: PAddr 0x%08x VAddr 0x%08x\r\n", n, pHd->addr, pHd));
    }
	m_hdFreeCnt = 0;
    PoolUnlock();

done:
    DEBUGMSG(ZONE_INIT || ZONE_VERBOSE, (L"-CCppiDmaController::HdPoolInit: Allocated %u HDs\r\n", n));
}

HOST_DESCRIPTOR* CCppiDmaController::HdAlloc()
{
    HOST_DESCRIPTOR* pHd;

    PoolLock();
    pHd = m_pvHdPoolHead;
    if (pHd) {
        m_pvHdPoolHead = pHd->next;
        pHd->NextPtr = NULL;
    }
    PoolUnlock();

    if (pHd == NULL) {
        ERRORMSG(1,
            (L" CCppiDmaController::HdAlloc: FAILED - Out of descriptors (increase DescriptorCount registry setting)!\r\n"));
    } else {
        ERRORMSG(((UINT32)pHd & (CPPI_HD_ALIGN - 1)), 
			(L" CCppiDmaController::HdAlloc: ERROR - Misaligned descriptor - 0x%08x!\r\n", pHd));
    }

#ifdef USB_STAT
	if (pHd) m_hdAllocCnt++;
#endif
    return pHd;
}

HOST_DESCRIPTOR* CCppiDmaController::HdFree(HOST_DESCRIPTOR* pHd)
{
    UINT32 type;

    DEBUGCHK(pHd != NULL);
    if (!pHd)
        return pHd;

    type = (pHd->DescInfo & USB_CPPI41_DESC_TYPE_MASK) >> USB_CPPI41_DESC_TYPE_SHIFT;
    if (type != USB_CPPI41_DESC_TYPE_HOST) {
        ERRORMSG(1, (L" CCppiDmaController::HdFree: Not a Host descriptor type %u\r\n", type));
        return pHd;
    }

    PoolLock();
    pHd->next = m_pvHdPoolHead;
    m_pvHdPoolHead = pHd;
    PoolUnlock();

#ifdef USB_STAT
	if (pHd) m_hdFreeCnt++;
#endif

    return NULL;
}

UINT32 CCppiDmaController::DescriptorVAtoPA(void *va)
{
    UINT32 pa = 0;

    if (va != NULL) {
        UINT32 vaPoolBase  = (UINT32)m_pvPool;
        UINT32 vaPoolLimit = (UINT32)m_pvPool + m_cbPoolSize;

        if (((UINT32)va < vaPoolBase) || ((UINT32)va > vaPoolLimit))
            pa = USBCDMA_DescriptorVAtoPA(m_hUsbCdma, va);
        else
            pa = m_paPool.LowPart + ((UINT32)va - vaPoolBase);
    }

    return pa;
}

void* CCppiDmaController::DescriptorPAtoVA(UINT32 pa)
{
    void *va = NULL;

    if (pa != 0) {
        UINT32 paPoolBase  = m_paPool.LowPart;
        UINT32 paPoolLimit = m_paPool.LowPart + m_cbPoolSize;

        if ((pa < paPoolBase) || (pa > paPoolLimit))
			va = USBCDMA_DescriptorPAtoVA(m_hUsbCdma, pa);
		else
            va = (void *)(((UINT32)m_pvPool) + (pa - paPoolBase));
    }

    return va;
}

#ifdef USB_STAT
void CCppiDmaController::GetStat(UINT32 *retBuf)
{
	int j;
	for (j=0; j<USB_CPPI_MAX_TX_CHANNELS; j++){
		*retBuf++ = (UINT32)m_pTxChannels[j];
		*retBuf++ = m_TxChaAllocCnt[j];
		*retBuf++ = m_TxChaReleaseCnt[j];
		*retBuf++ = (UINT32)m_pRxChannels[j];
		*retBuf++ = m_RxChaAllocCnt[j];
		*retBuf++ = m_RxChaReleaseCnt[j];
	}
	*retBuf++ = m_hdAllocCnt;
	*retBuf++ = m_hdFreeCnt;
}
#endif

#endif // MUSB_USEDMA

//////////////////////////////////////////////////////////////////////////////////////////
