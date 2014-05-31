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

#define PININTERFACE
#include <windows.h>
#include <pm.h>
#include <Msgqueue.h>
#include <pwinbase.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "CameraDebug.h"
#include <camera.h>
#include "CameraDriver.h"
#include "CameraPinDriver.h"
#include "wchar.h"

#pragma warning(disable:6320)

CPinDevice :: CPinDevice()
{
    m_dwMemoryModel             = CSPROPERTY_BUFFER_CLIENT_LIMITED;
    m_fClientInitialized        = false;
    m_fDiscontinuity            = true;
    m_ulPinId                   = 0xFFFFFFFF; // Invalid Pin Id
    m_ulMaxNumOfBuffers         = 0;
    m_ulFrameSize               = 0;
    m_ulFramesDropped           = 0;
    m_ulPictureNumber           = 0;
    m_RtAveTimePerFrame         = 0;
    m_hMsgQ                     = NULL;
    m_CsState                   = CSSTATE_STOP;
    m_msStart                   = 0xFFFFFFFF;
    m_msLastPT                  = 0;
    m_pStreamDescriptorList     = NULL;
    m_dwBufferCount             = 0;
    m_lStillCount               = 0;

    InitializeCriticalSection(&m_csStreamBuffer);
    InitializeCriticalSection(&m_csStreamIO);
}

CPinDevice :: ~CPinDevice()
{
    ResetBufferList();

    if (NULL != m_hMsgQ)
    {
        CloseMsgQueue(m_hMsgQ);
    }

    if (NULL != m_pStreamDescriptorList)
    {
        LocalFree(m_pStreamDescriptorList);
        m_pStreamDescriptorList = NULL;
    }

    m_CsState = CSSTATE_STOP;
    DeleteCriticalSection(&m_csStreamBuffer);
    DeleteCriticalSection(&m_csStreamIO);
}

bool CPinDevice :: InitializeSubDevice(PCAMERADEVICE pCamDevice)
{
    m_pCamAdapter = pCamDevice;
    if (NULL == m_pCamAdapter)
    {
        return false;
    }

    return true;
}

DWORD CPinDevice :: CloseSubDevice()
{
    DWORD dwRet = FALSE;
    dwRet = m_pCamAdapter->DecrCInstances(m_ulPinId);
    if (dwRet)
    {
        dwRet = m_pCamAdapter->PDDClosePin(m_ulPinId);
    }

    return dwRet;
}


DWORD CPinDevice :: StreamInstantiate(PCSPROPERTY_STREAMEX_S pCsPropStreamEx, PUCHAR /*pOutBuf*/, DWORD /*OutBufLen*/, PDWORD /*pdwBytesTransferred*/)
{
    DWORD   dwError  = ERROR_INVALID_PARAMETER;
    HANDLE  hProcess = NULL;
    PCS_DATARANGE_VIDEO pCsDataRangeVid = NULL;

    if (-1 != m_ulPinId)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Pin %d is already instantiated.\r\n"), this, m_ulPinId));
        return dwError;
    }

    if (NULL == m_pCamAdapter)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Initialization incomplete.\r\n"), this, m_ulPinId));
        return dwError;
    }

    if (false == m_pCamAdapter->IsValidPin(pCsPropStreamEx->CsPin.PinId))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Pin Id\r\n"), this));
        return dwError;
    }

    m_ulPinId = pCsPropStreamEx->CsPin.PinId;

    SENSORMODEINFO SensorModeInfo;
    if (ERROR_SUCCESS != m_pCamAdapter->PDDGetPinInfo(m_ulPinId, &SensorModeInfo))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to retrieve sub-device information\r\n"), this));
        return dwError;
    }

    m_dwMemoryModel     = SensorModeInfo.MemoryModel;
    m_ulMaxNumOfBuffers = SensorModeInfo.MaxNumOfBuffers;

    // Let us set a default format for this pin

    if (false == m_pCamAdapter->GetPinFormat(m_ulPinId, 1, &pCsDataRangeVid))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): No Pin Format provided for pin\r\n"), this));
        return dwError;
    }

    memcpy(&m_CsDataRangeVideo,pCsDataRangeVid, sizeof(CS_DATARANGE_VIDEO));

    if (NULL == pCsPropStreamEx->hMsgQueue)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): NULL Handle provided for msgqueue\r\n"), this));
        return dwError;
    }

    //TODO : Check whether the client created msgqueue with enough buffersize and number of buffers.

    MSGQUEUEOPTIONS msgQueueOptions = {0};
    msgQueueOptions.bReadAccess = FALSE; // we need write-access to msgqueue
    msgQueueOptions.dwSize      = sizeof(MSGQUEUEOPTIONS);

    hProcess = OpenProcess(NULL, FALSE, GetCallerVMProcessId());
    if (NULL == hProcess)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to open Process\r\n"), this));
        return dwError;
    }

    ASSERT(m_hMsgQ == NULL);

    if (NULL == (m_hMsgQ = OpenMsgQueue(hProcess, pCsPropStreamEx->hMsgQueue, &msgQueueOptions)))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to open MsgQueue\r\n"), this));
        CloseHandle(hProcess);
        return dwError;
    }

    CloseHandle(hProcess);

    if (false == m_pCamAdapter->IncrCInstances(m_ulPinId, this))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Pin %d is already instantiated.\r\n"), this, m_ulPinId));
        return dwError;
    }

    return ERROR_SUCCESS;
}

void CPinDevice :: SetState(CSSTATE CsState, CSSTATE *CsPrevState)
{
    EnterCriticalSection(&m_csStreamIO);

    if (NULL != CsPrevState)
    {
        *CsPrevState = m_CsState;
    }

     //Check if we are not already in the target state
    if (m_CsState != CsState)
    {
        m_CsState = CsState;

        if (STILL != m_ulPinId || CSSTATE_RUN != CsState)
        {
            m_pCamAdapter->PDDSetPinState(m_ulPinId, CsState);
        }

        if (STILL == m_ulPinId && CSSTATE_RUN == CsState)
        {
            m_fDiscontinuity = true;

            // If we have pending still images, we need to restart the sensor
            if (0 != GetStillCount())
            {
                m_pCamAdapter->PDDSetPinState(m_ulPinId, CsState);
            }
        }

        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("Pin: %d Setting State to 0x%X\r\n"), m_ulPinId, CsState));
    }
    LeaveCriticalSection(&m_csStreamIO);
    return;
}

BOOL
CPinDevice::InitMsgQueueDescriptor(
    PCS_MSGQUEUE_BUFFER pCsMsgQBuff,
    PCS_STREAM_DESCRIPTOR pCsStreamDesc,
    PVOID pMappedData,
    PVOID pUnmappedData,
    DWORD dwIndex,
    BOOL bBufferFill
   )
{
    PCSSTREAM_HEADER pCsStreamHeader = &pCsStreamDesc->CsStreamHeader;
    PCS_FRAME_INFO   pCsFrameInfo    = &pCsStreamDesc->CsFrameInfo;

    if ((pCsStreamHeader == NULL) || (pCsFrameInfo == NULL))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("InitMsgQueueDescriptor(%08x): Invalid Stream Descriptor\r\n"), this));
        return false;
    }

    if (bBufferFill)
    {
        // The buffer fill function must use the pointer that's been mapped into this process.
        pCsStreamHeader->Data = pMappedData;

        EnterCriticalSection(&m_csStreamBuffer);
        pCsStreamHeader->DataUsed = m_pCamAdapter->PDDFillPinBuffer(m_ulPinId, (PUCHAR) pMappedData);
        LeaveCriticalSection(&m_csStreamBuffer);

        pCsFrameInfo->PictureNumber = (LONGLONG)++m_ulPictureNumber;
        pCsFrameInfo->DropCount     = (LONGLONG)m_ulFramesDropped;
    }

    // The message queue requires the original pointer value.
    pCsStreamHeader->Data = pUnmappedData;

    // Init the flags to zero
    pCsStreamHeader->OptionsFlags = 0;

    // Set the discontinuity flag if frames have been previously
    // dropped, and then reset our internal flag

    if (true == m_fDiscontinuity)
    {
        pCsStreamHeader->OptionsFlags |= CSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
        m_fDiscontinuity = false;
    }

    DWORD msNow = GetTickCount();

    if (m_msStart == 0xFFFFFFFF)
    {
        m_msStart = msNow;
    }

    //
    // Return the timestamp for the frame
    //
    pCsStreamHeader->PresentationTime.Numerator   = 1;
    pCsStreamHeader->PresentationTime.Denominator = 1;
    pCsStreamHeader->Duration                     = m_RtAveTimePerFrame;
#ifdef DEBUG
    DWORD prevPT = m_msLastPT;
	UNREFERENCED_PARAMETER(prevPT);	// DEBUG is always turned on in the Camera driver, even for release builds
#endif

    m_msLastPT = msNow - m_msStart;
    pCsStreamHeader->PresentationTime.Time        = (LONGLONG) m_msLastPT * 10000;  // presentation time stamp in 100s of ns

    DEBUGMSG(ZONE_FUNCTION, (_T("InitMsgQueueDescriptor: LastPT = %d, elapsed = %d\n"), m_msLastPT, m_msLastPT - prevPT));

    // clear the timestamp valid flags
    pCsStreamHeader->OptionsFlags &= ~(CSSTREAM_HEADER_OPTIONSF_TIMEVALID | CSSTREAM_HEADER_OPTIONSF_DURATIONVALID);

    // Every frame we generate is a key frame (aka SplicePoint)
    // Delta frames (B or P) should not set this flag

    pCsStreamHeader->OptionsFlags |= CSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

    pCsMsgQBuff->CsMsgQueueHeader.Size    = sizeof(CS_MSGQUEUE_HEADER);
    pCsMsgQBuff->CsMsgQueueHeader.Flags   = FLAG_MSGQ_FRAME_BUFFER;
    pCsMsgQBuff->CsMsgQueueHeader.Context = NULL;

    // Get the unmarshalled descriptor.
    pCsMsgQBuff->pStreamDescriptor = m_pStreamDescriptorList[dwIndex].m_pUnmarshalledStreamDesc;

    DEBUGMSG(ZONE_FUNCTION, (
        _T("InitMsgQueueDescriptor(%08x): Frame buf queued: %d (dropped %d), start %d, time %d\n"),
        this,
        (LONG)pCsFrameInfo->PictureNumber,
        (LONG)pCsFrameInfo->DropCount,
        (LONG)m_msStart,
        (LONG)(pCsStreamHeader->PresentationTime.Time / 10000)));

    // Flush client data buffer.
    FlushClientBuffer(dwIndex);

    // Unmarshal client descriptor.
    UnmarshalClientDescriptor(dwIndex);

    return TRUE;
}

void  CPinDevice::FlushBufferQueue()
{
    PCS_STREAM_DESCRIPTOR pCsStreamDesc = NULL;
    PVOID                 pMappedData   = NULL;
    PVOID                 pUnmappedData = NULL;
    CS_MSGQUEUE_BUFFER    CsMsgQBuff;
    DWORD                 dwIndex;

    while ((RemoveBufferFromList(&pCsStreamDesc, &pMappedData, &pUnmappedData, &dwIndex)) && (NULL != pCsStreamDesc) && (m_hMsgQ != NULL))
    {
        if (!InitMsgQueueDescriptor (&CsMsgQBuff, pCsStreamDesc, pMappedData, pUnmappedData, dwIndex, FALSE))
        {
            continue;
        }

        if (!WriteMsgQueue(m_hMsgQ, reinterpret_cast<LPVOID>(&CsMsgQBuff),  sizeof(CS_MSGQUEUE_BUFFER), PIN_TIMEOUT, 0))
        {
            DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false\r\n"), this));
        }
    }

    return;
}

void CPinDevice::ReleaseBuffers()
{
    DWORD dwIndex = m_dwBufferCount;

    EnterCriticalSection(&m_csStreamBuffer);
    while (dwIndex--)
    {
        DeallocateBuffer_I(dwIndex, TRUE);
    }
    LeaveCriticalSection(&m_csStreamBuffer);
}

DWORD CPinDevice :: HandlePinIO()
{
    DWORD dwRet = ERROR_SUCCESS;
    BOOL bOK = TRUE;

    EnterCriticalSection(&m_csStreamIO);
    if (CSSTATE_RUN != m_CsState)
    {
        LeaveCriticalSection(&m_csStreamIO);
        return ERROR_INVALID_STATE;
    }

    PCS_STREAM_DESCRIPTOR pCsStreamDesc = NULL;
    PVOID                 pMappedData   = NULL;
    PVOID                 pUnmappedData = NULL;
    CS_MSGQUEUE_BUFFER    CsMsgQBuff;
    DWORD                 dwIndex;

    if (!RemoveBufferFromList(&pCsStreamDesc, &pMappedData, &pUnmappedData, &dwIndex) || (NULL == pCsStreamDesc))
    {
        // We dropped a frame
        m_ulFramesDropped++;
        m_fDiscontinuity = true;
        bOK = FALSE;
        dwRet = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!InitMsgQueueDescriptor (&CsMsgQBuff, pCsStreamDesc, pMappedData, pUnmappedData, dwIndex, TRUE))
    {
        bOK = FALSE;
        dwRet = ERROR_INVALID_DATA;
        goto exit;
    }

    if (NULL == m_hMsgQ)
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): MsgQueue is not opened\r\n"), this));
        bOK = FALSE;
        goto exit;
    }

    if (false == WriteMsgQueue(m_hMsgQ, reinterpret_cast<LPVOID>(&CsMsgQBuff),  sizeof(CS_MSGQUEUE_BUFFER), PIN_TIMEOUT, 0))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false\r\n"), this));
        dwRet = ERROR_WRITE_FAULT;
    }

exit:

    if (STILL == m_ulPinId)
    {
        if ((bOK) && (0 == DecrementStillCount()))
        {
            PauseStream();
        }
    }
    LeaveCriticalSection(&m_csStreamIO);
    return dwRet;
}

DWORD CPinDevice ::PauseStream()
{

    if (m_CsState == CSSTATE_STOP)
    {
        // Let's allocate our resources
        if (m_pStreamDescriptorList == NULL)
        {
            m_pStreamDescriptorList = (PCS_STREAM_DESCRIPTOR_SHADOW) LocalAlloc(LMEM_ZEROINIT, sizeof(CS_STREAM_DESCRIPTOR_SHADOW) * m_ulMaxNumOfBuffers);
            if (NULL == m_pStreamDescriptorList)
                return ERROR_OUTOFMEMORY;
        }

//        m_dwBufferCount = 0;
    }

    if (false == m_fClientInitialized)
    {
        // By this time the buffers must be allocated
        if (ERROR_SUCCESS == m_pCamAdapter->PDDInitPin(m_ulPinId, this))
        {
            m_fClientInitialized = true;
        }
    }

    if (m_fClientInitialized == false)
    {
        return ERROR_INTERNAL_ERROR;
    }

    m_CsState    = CSSTATE_PAUSE;
    m_pCamAdapter->PDDSetPinState(m_ulPinId, m_CsState);

    return ERROR_SUCCESS;
}


DWORD CPinDevice :: PinHandleConnectionRequests(
        PCSPROPERTY pCsProp,
        PUCHAR pOutBuf,                 // Unsafe, use with caution
        DWORD  OutBufLen,
        PDWORD pdwBytesTransferred      // Unsafe, use with caution
       )
{
    DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): PinHandleConnectionRequests\r\n"), this));

    DWORD                           dwError                 = ERROR_INVALID_PARAMETER;
    PCSALLOCATOR_FRAMING            pCsAllocatorFraming     = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER  pCsDataFormatVidInfoHdr = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER  pCsDataFormatVidInfoHdrCopy = NULL;

    if (NULL == pCsProp)
    {
        return dwError;
    }

    __try
    {
        *pdwBytesTransferred = 0;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // we support PROPSETID_Pin, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == pCsProp->Flags)
    {
        return ERROR_SUCCESS;
    }

    switch (pCsProp->Id)
    {
    case CSPROPERTY_CONNECTION_STATE:
        dwError = PinHandleConnStateSubReqs(pCsProp->Flags, pOutBuf, OutBufLen, pdwBytesTransferred);
        break;

    case CSPROPERTY_CONNECTION_DATAFORMAT:

        pCsDataFormatVidInfoHdr = (PCS_DATAFORMAT_VIDEOINFOHEADER) pOutBuf;
        DWORD dwStructSize;
        if ((OutBufLen < sizeof(PCS_DATAFORMAT_VIDEOINFOHEADER)) || (pOutBuf == NULL))
        {
            break;
        }

        if (m_CsState != CSSTATE_STOP)
        {
            dwError = ERROR_INVALID_STATE;
            break;
        }

        // The video info header can be modified by the caller while it's being accessed in the subroutine.
        // The Subroutine needs to make a copy of the video info header before accessing it.

        __try
        {
            dwStructSize = sizeof(CS_DATAFORMAT_VIDEOINFOHEADER) + pCsDataFormatVidInfoHdr->VideoInfoHeader.bmiHeader.biSize - sizeof(CS_BITMAPINFOHEADER);
            pCsDataFormatVidInfoHdrCopy = (PCS_DATAFORMAT_VIDEOINFOHEADER)LocalAlloc(LMEM_ZEROINIT, dwStructSize);
            if (pCsDataFormatVidInfoHdrCopy == NULL)
            {
                return ERROR_INVALID_PARAMETER;
            }

            if (CeSafeCopyMemory(pCsDataFormatVidInfoHdrCopy, pCsDataFormatVidInfoHdr, dwStructSize))
            {
                dwError = PinHandleConnDataFormatSubReqs(pCsProp->Flags, pCsDataFormatVidInfoHdrCopy, pdwBytesTransferred);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwError = ERROR_INVALID_PARAMETER;
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): CSPROPERTY_CONNECTION_DATAFORMAT: Exception occured\r\n"), this));
        }

        LocalFree(pCsDataFormatVidInfoHdrCopy);
        break;

    case CSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        switch (pCsProp->Flags)
        {
        case CSPROPERTY_TYPE_GET:
        case CSPROPERTY_TYPE_BASICSUPPORT:
            CSALLOCATOR_FRAMING csAllocatorFraming;

            if ((OutBufLen < sizeof(CSALLOCATOR_FRAMING)) || (pOutBuf == NULL))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            {
                csAllocatorFraming.RequirementsFlags = m_dwMemoryModel;
                csAllocatorFraming.PoolType          = PagedPool;
                csAllocatorFraming.Frames            = m_ulMaxNumOfBuffers;
                csAllocatorFraming.FrameSize         = m_CsDataRangeVideo.VideoInfoHeader.bmiHeader.biSizeImage;
                csAllocatorFraming.FileAlignment     = FILE_BYTE_ALIGNMENT;
                csAllocatorFraming.Reserved          = 0;
            }

            __try
            {
                memcpy(pOutBuf, &csAllocatorFraming, sizeof(CSALLOCATOR_FRAMING));
                *pdwBytesTransferred = sizeof(CSALLOCATOR_FRAMING);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            dwError = ERROR_SUCCESS;
            break;

        case CSPROPERTY_TYPE_SET:

            if (OutBufLen < sizeof(CSALLOCATOR_FRAMING))
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            if (m_CsState != CSSTATE_STOP)
            {
                dwError = ERROR_INVALID_STATE;
                break;
            }

            pCsAllocatorFraming = (PCSALLOCATOR_FRAMING) pOutBuf;
            if (m_dwMemoryModel != pCsAllocatorFraming->RequirementsFlags)
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }
            if (m_ulMaxNumOfBuffers != pCsAllocatorFraming->Frames)
            {
                ResetBufferList();
                if (NULL != m_pStreamDescriptorList)
                {
                    LocalFree(m_pStreamDescriptorList);
                    m_pStreamDescriptorList = NULL;
                }
            }
            m_ulMaxNumOfBuffers = pCsAllocatorFraming->Frames;
            dwError = ERROR_SUCCESS;
            break;

        default :
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): CSPROPERTY_CONNECTION_ALLOCATORFRAMING Invalid Request\r\n"), this));
        }

        break;

    case CSPROPERTY_CONNECTION_PROPOSEDATAFORMAT :
        // I don't want to support dynamic format changes for this test driver
        break;

    default :
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Request\r\n"), this));

    }

    return dwError;
}

DWORD
CPinDevice::PinHandleBufferRequest(
    CSBUFFER_INFO  csBufferInfo,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    DWORD *pdwBytesTransferred
   )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCommand = csBufferInfo.dwCommand;
    PCS_STREAM_DESCRIPTOR pUnmarshalledCsDescriptor = (PCS_STREAM_DESCRIPTOR) csBufferInfo.pStreamDescriptor;

    if (pdwBytesTransferred)
    {
        *pdwBytesTransferred = 0;
    }

    // The pOutBuf argument has already been probed with MapCallerPointer, and the upper layer
    // has already checked for the size of the buffer to be at least sizeof(CS_STREAM_DESCRIPTOR)

    switch(dwCommand)
    {
        case CS_ALLOCATE:
            if ((NULL == pOutBuf) || (OutBufLen < sizeof(CS_STREAM_DESCRIPTOR)))
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            // Let's allocate our resources
            if (m_pStreamDescriptorList == NULL)
            {
                m_pStreamDescriptorList = (PCS_STREAM_DESCRIPTOR_SHADOW) LocalAlloc(LMEM_ZEROINIT, sizeof(CS_STREAM_DESCRIPTOR_SHADOW) * m_ulMaxNumOfBuffers);
                if (NULL == m_pStreamDescriptorList)
                {
                    dwError = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }
            }

            dwError = AllocateBuffer(pUnmarshalledCsDescriptor, (PCS_STREAM_DESCRIPTOR) pOutBuf, pdwBytesTransferred);
            break;

        case CS_ENQUEUE:
            dwError = EnqueueDescriptor(pUnmarshalledCsDescriptor);
            break;

        case CS_DEALLOCATE:
            dwError = DeallocateBuffer(pUnmarshalledCsDescriptor);
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }

Cleanup:

    return dwError;
}


DWORD
CPinDevice::PinHandleConnStateSubReqs(
    ULONG  ulReqFlags,
    PUCHAR pOutBuf,                 // Unsafe, use with caution
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    DWORD dwError  = ERROR_INVALID_PARAMETER;

    DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): PinHandleConnStateSubReqs\r\n"), this));

    switch(ulReqFlags)
    {
    case CSPROPERTY_TYPE_GET:
        if (OutBufLen < sizeof (CSSTATE))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        if (NULL == pOutBuf)
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        __try
        {
            memcpy(pOutBuf, &m_CsState, sizeof (CSSTATE));
            *pdwBytesTransferred = sizeof (CSSTATE);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        dwError = ERROR_SUCCESS;
        break;

    case CSPROPERTY_TYPE_SET:

        CSSTATE csState;
        if (OutBufLen < sizeof(CSSTATE))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        if (NULL == pOutBuf)
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        if (!CeSafeCopyMemory(&csState, pOutBuf, sizeof(CSSTATE)))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        if (csState == m_CsState)
        {
            dwError = ERROR_SUCCESS;
            break;
        }

        EnterCriticalSection(&m_csStreamIO);
        switch (csState)
        {
        case CSSTATE_STOP:

			DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): Set PIN %d to CSSTATE_STOP\r\n"), this, m_ulPinId));

            m_ulPictureNumber = 0;
            m_ulFramesDropped = 0;
            m_msLastPT        = 0;

            // We can get to the CSSTATE_STOP state from any other state.
            if (CSSTATE_STOP == m_CsState)
            {
                DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): State to set = CSSTATE_STOP but we are already Stopped.\r\n"), this));
                dwError = ERROR_SUCCESS;
                break;
            }

            m_CsState = CSSTATE_STOP;
            m_pCamAdapter->PDDSetPinState(m_ulPinId, m_CsState);

            // The buffer queue needs to be emptied if the driver is not allocating the buffers
            FlushBufferQueue();
            dwError = ERROR_SUCCESS;

            break;

        case CSSTATE_PAUSE:

			DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): Set PIN %d to CSSTATE_PAUSE\r\n"), this, m_ulPinId));

            if (CSSTATE_PAUSE == m_CsState)
            {
                DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): State to set = CSSTATE_PAUSE but we are already Paused.\r\n"), this));
                dwError = ERROR_SUCCESS;
                break;
            }

            dwError = PauseStream();
            break;

        case CSSTATE_RUN:

			DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): Set PIN %d to CSSTATE_RUN\r\n"), this, m_ulPinId));

            if (CSSTATE_STOP == m_CsState)
            {
                DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): CSSTATE_STOP to CSSTATE_RUN is not a supported transition .\r\n"), this));
                dwError = ERROR_INVALID_STATE;
                break;
            }

            // We only allow Still Pin to goto Run state through PROPSETID_VIDCAP_VIDEOCONTROL, or if there's a pending still count
            if (STILL == m_ulPinId)
            {
                // If we have pending still images, we need to restart the sensor
                if (0 != GetStillCount())
                {
                    SetState(CSSTATE_RUN, NULL);
                }
                dwError = ERROR_SUCCESS;
                break;
            }

            m_CsState = CSSTATE_RUN;
            m_msStart = 0xFFFFFFFF;
            m_pCamAdapter->PDDSetPinState(m_ulPinId, m_CsState);


            dwError = ERROR_SUCCESS;

            break;

        default :
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Incorrect State\r\n"), this));
            dwError = ERROR_INVALID_PARAMETER;
        }
        LeaveCriticalSection(&m_csStreamIO);
        break;

    default:
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Request\r\n"), this));

        break;
    }

    return dwError;
}


DWORD
CPinDevice::PinHandleCustomRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
   )
{
    return m_pCamAdapter->PDDHandlePinCustomProperties(m_ulPinId, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
}

DWORD
CPinDevice::PinHandleConnDataFormatSubReqs(
    ULONG                          ulReqFlags,
    PCS_DATAFORMAT_VIDEOINFOHEADER pCsDataFormatVidInfoHdr,  // Warning: this buffer is unsafe, use with caution
    PDWORD                         pdwBytesTransferred
   )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    PCS_DATARANGE_VIDEO pCsDataRangeVideoMatched = NULL;

    // We must have called IOCTL_STREAM_INSTANTIATE before setting format
    if (-1 == m_ulPinId)
    {
        return dwError;
    }

    // The incoming video info header is unsafe. The data might change on a separate thread
    // while it's being accessed. For security purposes, let's make a copy of the data
    // before any attempt to access them is done, and then work off the copy

    switch(ulReqFlags)
    {
    case CSPROPERTY_TYPE_SET:
        if (true == m_pCamAdapter->AdapterCompareFormat(m_ulPinId, pCsDataFormatVidInfoHdr, &pCsDataRangeVideoMatched, true))
        {
            // We found our format
            memcpy(&m_CsDataRangeVideo, pCsDataRangeVideoMatched, sizeof (CS_DATARANGE_VIDEO));
            memcpy(&m_CsDataRangeVideo, &pCsDataFormatVidInfoHdr->DataFormat, sizeof (CSDATARANGE));
            memcpy(&m_CsDataRangeVideo.VideoInfoHeader, &pCsDataFormatVidInfoHdr->VideoInfoHeader, sizeof (CS_VIDEOINFOHEADER));

            m_RtAveTimePerFrame = m_CsDataRangeVideo.VideoInfoHeader.AvgTimePerFrame;

            dwError = m_pCamAdapter->PDDSetPinFormat(m_ulPinId, &m_CsDataRangeVideo);
            *pdwBytesTransferred = 0;
        }

        break;

    default:
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Request\r\n"), this));
    }

    return dwError;
}

DWORD
CPinDevice::AllocateBuffer(
    PCS_STREAM_DESCRIPTOR pUnmarshalledCsDescriptor,
    PCS_STREAM_DESCRIPTOR pCsDescriptorOut,
    DWORD *pdwBytesTransferred
   )
{
    DWORD dwError = ERROR_SUCCESS;

    if ((NULL == pUnmarshalledCsDescriptor) || (NULL == pCsDescriptorOut))
    {
        return ERROR_INVALID_PARAMETER;
    }

    MarshalledBuffer_t MarshalledStreamDesc(pUnmarshalledCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_I_PTR, FALSE, FALSE);
    PCS_STREAM_DESCRIPTOR pCsDescriptorIn = reinterpret_cast<PCS_STREAM_DESCRIPTOR>(MarshalledStreamDesc.ptr());
    if (NULL == pCsDescriptorIn)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // There are 2 cases here: the buffer comes from the hardware or from the software.
    // If the buffer comes from the software, we generate a new entry in the table up to the maximum allowed.
    // If the buffer comes from the hardware, we setup the application stream descriptor

    EnterCriticalSection(&m_csStreamBuffer);

    //Check the BufferCount after the critical section is entered.
    //This prevents synch issues with validating the BufferCount
    if (m_dwBufferCount >= m_ulMaxNumOfBuffers)
    {
        // No more buffers.
        dwError = ERROR_NO_SYSTEM_RESOURCES;
        goto Cleanup;
    }

    DWORD iAllocBuffer = 0xFFFFFFFF;

    switch (m_dwMemoryModel)
    {
    case CSPROPERTY_BUFFER_DRIVER:
        {
            // Use next buffer.
            iAllocBuffer = m_dwBufferCount;

            ASSERT(!m_pStreamDescriptorList[iAllocBuffer].m_fBusy);

            // Get one of the hardware buffers, and setup the descriptor.  The
            // buffer is allocated in shared heap.  The shared heap is writable
            // from kernel and readable from user mode, and does not need to
            // be marshalled.
            dwError = HwSetupStreamDescriptor(iAllocBuffer);
            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            // Copy shadow to output.
            if (!CeSafeCopyMemory(
                    pCsDescriptorOut,
                    &(m_pStreamDescriptorList[iAllocBuffer].csStreamDescriptorShadow),
                    sizeof(CS_STREAM_DESCRIPTOR)))
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pCsDescriptorOut->CsStreamHeader.Data = m_pStreamDescriptorList[iAllocBuffer].m_pvUnmarshalledDataBuffer;
        }
        break;

    case CSPROPERTY_BUFFER_CLIENT_LIMITED:
    case CSPROPERTY_BUFFER_CLIENT_UNLIMITED:
        {
            // Default to next buffer.
            iAllocBuffer = m_dwBufferCount;

            // Find available slot for client unlimited.
            if (CSPROPERTY_BUFFER_CLIENT_UNLIMITED == m_dwMemoryModel)
            {
                BOOL fFoundBuffer = FALSE;
                for (DWORD iBuffer = 0; iBuffer < m_ulMaxNumOfBuffers; iBuffer++)
                {
                    if (!m_pStreamDescriptorList[iBuffer].m_fBusy)
                    {
                        iAllocBuffer = iBuffer;
                        fFoundBuffer = TRUE;
                        break;
                    }
                }

                if (!fFoundBuffer)
                {
                    dwError = ERROR_NO_SYSTEM_RESOURCES;
                    goto Cleanup;
                }
            }

            ASSERT(!m_pStreamDescriptorList[iAllocBuffer].m_fBusy);

            // Copy input descriptor to output if different buffers.
            if (pCsDescriptorOut != pCsDescriptorIn)
            {
                if (!CeSafeCopyMemory(
                        pCsDescriptorOut,
                        pCsDescriptorIn,
                        sizeof(CS_STREAM_DESCRIPTOR)))
                {
                    dwError = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }

            // Copy input descriptor to shadow.
            if (!CeSafeCopyMemory(
                    &(m_pStreamDescriptorList[iAllocBuffer].csStreamDescriptorShadow),
                    pCsDescriptorIn,
                    sizeof(CS_STREAM_DESCRIPTOR)))
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            // Populate the handle and the buffer field of the descriptor.
            dwError = SwSetupStreamDescriptor(
                            iAllocBuffer,
                            pCsDescriptorIn->CsStreamHeader.Data,   // Unmarshalled client buffer
                            pCsDescriptorOut);
            if (dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
        break;

    default:
        dwError = ERROR_INTERNAL_ERROR;
        goto Cleanup;
        break;
    }

    ASSERT(iAllocBuffer != -1);
    m_pStreamDescriptorList[iAllocBuffer].m_pvUnmarshalledDataBuffer = pCsDescriptorOut->CsStreamHeader.Data;
    m_pStreamDescriptorList[iAllocBuffer].m_fBusy = TRUE;
    m_pStreamDescriptorList[iAllocBuffer].pCsStreamDescriptorExternal = NULL;
    m_dwBufferCount++;

    if (pdwBytesTransferred)
    {
        *pdwBytesTransferred = sizeof(CS_STREAM_DESCRIPTOR);
    }

Cleanup:

    LeaveCriticalSection(&m_csStreamBuffer);
    return dwError;
}

DWORD
CPinDevice::DeallocateBuffer(
    PCS_STREAM_DESCRIPTOR pUnmarshalledCsDescriptor
   )
{
    DWORD dwError = ERROR_SUCCESS;

    MarshalledBuffer_t MarshalledStreamDesc(
                            pUnmarshalledCsDescriptor,
                            sizeof(CS_STREAM_DESCRIPTOR),
                            ARG_IO_PTR,
                            FALSE,
                            FALSE);
    PCS_STREAM_DESCRIPTOR pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>(MarshalledStreamDesc.ptr());
    if (NULL == pCsDescriptor)
    {
        return ERROR_INVALID_PARAMETER;
    }

    PVOID pvBuffer = pCsDescriptor->CsStreamHeader.Data;
    DWORD dwHandle = pCsDescriptor->CsStreamHeader.Handle;

    EnterCriticalSection(&m_csStreamBuffer);

    //Check if there are any buffers to deallocate
    if (0 == m_dwBufferCount)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Get the entry for this buffer in the internal list
    DWORD dwIndex = GetIndexFromHandle(dwHandle, pvBuffer);
    if (-1 == dwIndex)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Internal deallocate
    dwError = DeallocateBuffer_I(dwIndex, FALSE);
    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    // For driver allocated, clear the buffer address in the client descriptor.
    if (CSPROPERTY_BUFFER_DRIVER == m_dwMemoryModel)
    {
        pCsDescriptor->CsStreamHeader.Data = NULL;
    }

Cleanup:

    LeaveCriticalSection(&m_csStreamBuffer);
    return dwError;
}

DWORD
CPinDevice::DeallocateBuffer_I(
    DWORD dwIndex,
    BOOL fForce
   )
{
    // Note: Assume m_csStreamBuffer is taken.
    DWORD dwError = ERROR_SUCCESS;
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    // If the row is not in use or if fForce is set, let's release the buffer
    if (!((NULL == pDescriptorListEntry->pCsStreamDescriptorExternal) || fForce))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    PCS_STREAM_DESCRIPTOR pStreamDescriptor = &pDescriptorListEntry->csStreamDescriptorShadow;

    if (pDescriptorListEntry->m_fBusy)
    {
        pDescriptorListEntry->m_fBusy = FALSE;
        m_dwBufferCount--;

        if (CSPROPERTY_BUFFER_DRIVER == m_dwMemoryModel)
        {
            VirtualFreeEx((HANDLE)GetCallerVMProcessId(), pDescriptorListEntry->m_pvUnmarshalledDataBuffer, 0, MEM_RELEASE);
            dwError = m_pCamAdapter->PDDDeAllocatePinBuffer(
                            m_ulPinId,
                            pStreamDescriptor->CsStreamHeader.Data);
        }
        else
        {
            dwError = m_pCamAdapter->PDDUnRegisterClientBuffer(
                            m_ulPinId,
                            pDescriptorListEntry->m_pvUnmarshalledDataBuffer);

            // Unmarshal buffer.
            UnmarshalClientBuffer(dwIndex);
        }
    }

    pStreamDescriptor->CsStreamHeader.Data = NULL;
    pDescriptorListEntry->pCsStreamDescriptorExternal = NULL;
    pDescriptorListEntry->m_pvUnmarshalledDataBuffer = NULL;

    // Unmarshal descriptor.
    UnmarshalClientDescriptor(dwIndex);

Cleanup:

    return dwError;
}

DWORD
CPinDevice::MarshalClientBuffer(
    DWORD dwIndex,
    PVOID pvUnmarshalledClientBuffer
   )
{
    DWORD dwError = ERROR_SUCCESS;
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    // Should not have stale client buffer.  Make sure to unmarshal it if we do have one.
    ASSERT((NULL == pDescriptorListEntry->m_pMarshalledDataBuffer) ||
           (NULL == pDescriptorListEntry->m_pMarshalledDataBuffer->ptr()));
    UnmarshalClientBuffer(dwIndex);

    // Alloc marshal helper.
    if (NULL == pDescriptorListEntry->m_pMarshalledDataBuffer)
    {
        pDescriptorListEntry->m_pMarshalledDataBuffer = new MarshalledBuffer_t();
        if (NULL == pDescriptorListEntry->m_pMarshalledDataBuffer)
        {
            dwError = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // Marshal the client buffer.
    DWORD dwSize = m_CsDataRangeVideo.VideoInfoHeader.bmiHeader.biSizeImage;
    if (FAILED(pDescriptorListEntry->m_pMarshalledDataBuffer->Marshal(
            pvUnmarshalledClientBuffer,
            dwSize,
            ARG_O_PTR | MARSHAL_FORCE_ALIAS,
            FALSE,
            TRUE))) //Enable async access
    {
        DEBUGMSG(ZONE_IOCTL | ZONE_ERROR, (
            _T("PIN_IOControl(%08x): Unable to marshal client data buffer for async access.\r\n"),
            this));
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Set the marshalled buffer in shadow.
    pDescriptorListEntry->csStreamDescriptorShadow.CsStreamHeader.Data =
        pDescriptorListEntry->m_pMarshalledDataBuffer->ptr();

Cleanup:

    return dwError;
}

DWORD
CPinDevice::UnmarshalClientBuffer(
    DWORD dwIndex
   )
{
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    if (pDescriptorListEntry->m_pMarshalledDataBuffer != NULL)
    {
        pDescriptorListEntry->m_pMarshalledDataBuffer->Unmarshal();
    }

    return ERROR_SUCCESS;
}

DWORD
CPinDevice::FlushClientBuffer(
    DWORD dwIndex
   )
{
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    if (pDescriptorListEntry->m_pMarshalledDataBuffer != NULL)
    {
        pDescriptorListEntry->m_pMarshalledDataBuffer->Flush();
    }

    return ERROR_SUCCESS;
}

DWORD
CPinDevice::MarshalClientDescriptor(
    DWORD dwIndex,
    PCS_STREAM_DESCRIPTOR pUnmarshalledCsDescriptor
   )
{
    DWORD dwError = ERROR_SUCCESS;
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    // Make sure there is no stale descriptor.
    UnmarshalClientDescriptor(dwIndex);

    // Alloc marshal helper.
    if (NULL == pDescriptorListEntry->m_pMarshalledStreamDesc)
    {
        pDescriptorListEntry->m_pMarshalledStreamDesc = new MarshalledBuffer_t();
        if (NULL == pDescriptorListEntry->m_pMarshalledStreamDesc)
        {
            dwError = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // Marshal the descriptor
    if (FAILED(pDescriptorListEntry->m_pMarshalledStreamDesc->Marshal(
            pUnmarshalledCsDescriptor,
            sizeof(CS_STREAM_DESCRIPTOR),
            ARG_IO_PTR | MARSHAL_FORCE_ALIAS,
            FALSE,
            TRUE))) //Enable async access
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (
            _T("PIN_IOControl(%08x): Unable to marshal client descriptor for async access.\r\n"),
            this));
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    PCS_STREAM_DESCRIPTOR pMarshalledCsDescriptor = (PCS_STREAM_DESCRIPTOR) pDescriptorListEntry->m_pMarshalledStreamDesc->ptr();
    ASSERT(pMarshalledCsDescriptor != NULL);
    pDescriptorListEntry->pCsStreamDescriptorExternal = pMarshalledCsDescriptor;
    pDescriptorListEntry->m_pUnmarshalledStreamDesc = pUnmarshalledCsDescriptor;

Cleanup:

    return dwError;
}

DWORD
CPinDevice::UnmarshalClientDescriptor(
    DWORD dwIndex
   )
{
    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[dwIndex];

    if (pDescriptorListEntry->m_pMarshalledStreamDesc != NULL)
    {
        pDescriptorListEntry->m_pMarshalledStreamDesc->Unmarshal();
    }

    return ERROR_SUCCESS;
}

DWORD
CPinDevice::SwSetupStreamDescriptor(
    DWORD dwIndex,
    PVOID pvUnmarshalledClientBuffer,
    PCS_STREAM_DESCRIPTOR pCsStreamDescOut
   )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fMarshalled = FALSE;
    DWORD dwHandle;

    if ((NULL == pvUnmarshalledClientBuffer) ||
        (NULL == pCsStreamDescOut))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwHandle = MakeHandle(dwIndex, pCsStreamDescOut);

    __try
    {
        pCsStreamDescOut->CsStreamHeader.Handle = dwHandle;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return ERROR_INVALID_PARAMETER;
    }

    PCS_STREAM_DESCRIPTOR pCsStreamDescShadow = &m_pStreamDescriptorList[dwIndex].csStreamDescriptorShadow;
    pCsStreamDescShadow->CsStreamHeader.Handle = dwHandle;

    // Marshal the client buffer.
    dwError = MarshalClientBuffer(
                    dwIndex,
                    pvUnmarshalledClientBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    // Mark as marshalled for cleanup.
    fMarshalled = TRUE;

    // Call the PDD to register the client buffer.  If the PDD wants to
    // use this buffer, it will have to marshal it.
    dwError =  m_pCamAdapter->PDDRegisterClientBuffer(
                    m_ulPinId,
                    pvUnmarshalledClientBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

Cleanup:

    if (dwError != ERROR_SUCCESS)
    {
        if (fMarshalled)
        {
            UnmarshalClientBuffer(dwIndex);
        }
    }

    return dwError;
}


DWORD
CPinDevice::HwSetupStreamDescriptor(
    DWORD   dwIndex
)
{
    PCSSTREAM_HEADER      pCsStreamHeader;
    PCS_FRAME_INFO        pCsFrameInfo;

    if (dwIndex > m_dwBufferCount)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_ulFrameSize = CS_DIBSIZE (m_CsDataRangeVideo.VideoInfoHeader.bmiHeader);

    pCsStreamHeader = &(m_pStreamDescriptorList[ dwIndex ].csStreamDescriptorShadow.CsStreamHeader);
    pCsFrameInfo = &(m_pStreamDescriptorList[ dwIndex ].csStreamDescriptorShadow.CsFrameInfo);

    pCsStreamHeader->Size                         = sizeof(CSSTREAM_HEADER);
    pCsStreamHeader->TypeSpecificFlags            = 0;
    pCsStreamHeader->PresentationTime.Time        = 0;
    pCsStreamHeader->PresentationTime.Numerator   = 1;
    pCsStreamHeader->PresentationTime.Denominator = 1;
    pCsStreamHeader->Duration                     = 0;
    pCsStreamHeader->FrameExtent                  = m_ulFrameSize;
    pCsStreamHeader->DataUsed                     = m_ulFrameSize;
    pCsStreamHeader->OptionsFlags                 = CSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;

    pCsFrameInfo->ExtendedHeaderSize = sizeof(CS_FRAME_INFO);
    pCsFrameInfo->dwFrameFlags       = CS_VIDEO_FLAG_FRAME;
    pCsFrameInfo->PictureNumber      = 0;
    pCsFrameInfo->DropCount          = 0;

    // Note: The __try/__except block is here to highlight the fact that this call has to be
    // protected in the case of a hardware access.
    __try
    {
        pCsStreamHeader->Data = m_pCamAdapter->PDDAllocatePinBuffer(m_ulPinId);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return ERROR_INTERNAL_ERROR;
    }
    
    if (NULL == pCsStreamHeader->Data)
    {
        return ERROR_OUTOFMEMORY;
    }

    // Since we are aliasing this buffer, the PDD needs to ensure that the allocation is both
    // page aligned and granular with respect to the page size.
    PVOID pUnmarshalledData = VirtualAllocCopyEx((HANDLE)GetCurrentProcessId(), (HANDLE)GetCallerVMProcessId(), pCsStreamHeader->Data, m_ulFrameSize, PAGE_READWRITE);

    if (NULL == pUnmarshalledData)
    {
        m_pCamAdapter->PDDDeAllocatePinBuffer(m_ulPinId, pCsStreamHeader->Data);
        pCsStreamHeader->Data = NULL;
        return GetLastError();
    }

    m_pStreamDescriptorList[dwIndex].m_pvUnmarshalledDataBuffer = pUnmarshalledData;

    // And setup the handle
    pCsStreamHeader->Handle = MakeHandle(dwIndex, pCsStreamHeader->Data);

    return ERROR_SUCCESS;
}


DWORD
CPinDevice::MakeHandle(
    DWORD  dwIndex,
    LPVOID pBuffer
)
{
    DWORD  dwHandle;
    DWORD dwProcessId = GetCallerVMProcessId();

    dwHandle = dwProcessId ^ ((dwIndex << 16) + ((DWORD)pBuffer & 0xFFFF));

    return dwHandle;
}


LONG
CPinDevice::GetIndexFromHandle(
    DWORD  dwHandle,
    LPVOID pvBuffer      // Warning: This is an unsafe buffer, use with caution
)
{
    LONG   lIndex = -1;
    DWORD dwProcessId = GetCallerVMProcessId();

    // let's retrieve the index from the handle table and make sure we have a match
    lIndex = (dwHandle ^ dwProcessId) >> 16;
    if (lIndex >= (LONG)m_ulMaxNumOfBuffers || lIndex < 0)
    {
        // Invalid index, bail out
        return -1;
    }

    PCS_STREAM_DESCRIPTOR_SHADOW pDescriptorListEntry = &m_pStreamDescriptorList[lIndex];
    PCS_STREAM_DESCRIPTOR pCsStreamDescriptor = &m_pStreamDescriptorList[lIndex].csStreamDescriptorShadow;

    if ((pDescriptorListEntry->m_pvUnmarshalledDataBuffer != pvBuffer) ||
        (pCsStreamDescriptor->CsStreamHeader.Handle != dwHandle))
    {
        // Something's wrong, bail out
        return -1;
    }

    return lIndex;
}


DWORD
CPinDevice::EnqueueDescriptor(
    PCS_STREAM_DESCRIPTOR pUnmarshalledCsDescriptor
   )
{
    DWORD dwError = ERROR_SUCCESS;
    PCS_STREAM_DESCRIPTOR pCsDescriptor = NULL;

    DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): EnqueueDescriptor\r\n"), this));
    if (m_CsState == CSSTATE_STOP)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    MarshalledBuffer_t  MarshalledStreamDesc(pUnmarshalledCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_I_PTR, FALSE, FALSE);
    pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>(MarshalledStreamDesc.ptr());

    if (NULL == pCsDescriptor)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // First, let's use the handle and the buffer to retrieve the shadow copy
    // If an exception happens during the following 2 lines, it will be trapped by the upper level
    PVOID pvBuffer = pCsDescriptor->CsStreamHeader.Data;
    DWORD dwHandle = pCsDescriptor->CsStreamHeader.Handle;

    EnterCriticalSection(&m_csStreamBuffer);

    // Get the entry for this buffer in the internal list
    DWORD dwIndex = GetIndexFromHandle(dwHandle, pvBuffer);
    if (dwIndex == -1)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Is the row in use?
    if (!m_pStreamDescriptorList[ dwIndex ].m_fBusy)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): The buffer has not be prepared. Call CS_ALLOCATE first.\r\n"), this));
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (m_pStreamDescriptorList[ dwIndex ].pCsStreamDescriptorExternal != NULL)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): This buffer has already be enqueued.\r\n"), this));
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Marshal the client descriptor.
    dwError = MarshalClientDescriptor(dwIndex, pUnmarshalledCsDescriptor);
    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (!m_fClientInitialized && (CSSTATE_PAUSE == m_CsState))
    {
        m_fClientInitialized = TRUE;
    }

Cleanup:

    LeaveCriticalSection(&m_csStreamBuffer);
    return dwError;
}


bool
CPinDevice::RemoveBufferFromList(
    PCS_STREAM_DESCRIPTOR * ppCsStreamDesc,
    PVOID                 * ppMappedData,
    PVOID                 * ppUnmappedData,
    PDWORD                  pdwIndex
   )
{
    DWORD iBuffer = 0;
    bool  RetVal = true;

    // Let's look in the list of buffers for the first buffer that has a non null external stream descriptor
    DEBUGMSG(ZONE_IOCTL, (_T("PIN_IOControl(%08x): RemoveBufferFromList\r\n"), this));

    if ((NULL == ppCsStreamDesc) || (NULL == ppMappedData) || (NULL == ppUnmappedData) || (NULL == pdwIndex))
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): RemoveBufferFromList - Null pointer has been passed in.\r\n"), this));
        return RetVal;
    }

    //Initialize arguments
    *ppCsStreamDesc = NULL;
    *ppMappedData = NULL;
    *ppUnmappedData = NULL;
    *pdwIndex = 0xFFFFFFFF;

    EnterCriticalSection(&m_csStreamBuffer);
    while ((iBuffer < m_dwBufferCount) && (*ppCsStreamDesc == NULL))
    {
        if (m_pStreamDescriptorList[ iBuffer ].pCsStreamDescriptorExternal != NULL)
        {
            // We found one registered buffer. Let's return it.
            *pdwIndex = iBuffer;
            *ppCsStreamDesc = m_pStreamDescriptorList[ iBuffer ].pCsStreamDescriptorExternal;
            *ppMappedData   = m_pStreamDescriptorList[ iBuffer ].csStreamDescriptorShadow.CsStreamHeader.Data;
            *ppUnmappedData = m_pStreamDescriptorList[ iBuffer ].pCsStreamDescriptorExternal->CsStreamHeader.Data;
            m_pStreamDescriptorList[ iBuffer ].pCsStreamDescriptorExternal = NULL;
            break;
        }

        iBuffer++;
    }
    LeaveCriticalSection(&m_csStreamBuffer);
    if (NULL == *ppMappedData)
    {
        RetVal = false;
    }

    return RetVal;
}


bool
CPinDevice::ResetBufferList()
{
    EnterCriticalSection(&m_csStreamBuffer);
    if (m_pStreamDescriptorList)
    {
        for(DWORD i = 0; i < m_ulMaxNumOfBuffers; i++)
        {
            if (m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer != NULL)
            {
                delete m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer;
                m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer = NULL;
            }

            if (m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc != NULL)
            {
                delete m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc;
                m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc = NULL;
            }

            m_pStreamDescriptorList[ i ].pCsStreamDescriptorExternal = NULL;
            m_pStreamDescriptorList[ i ].m_fBusy = FALSE;
        }
    }

    LeaveCriticalSection(&m_csStreamBuffer);

    return true;
}



ULONG CPinDevice :: PictureNumber() const
{
    return m_ulPictureNumber;
}

ULONG CPinDevice :: FramesDropped() const
{
    return m_ulFramesDropped;
}

ULONG CPinDevice :: FrameSize() const
{
    return m_ulFrameSize;
}

DWORD CPinDevice :: WriteMessage(ULONG flag, LPVOID Context)
{

    if (flag != FLAG_MSGQ_SAMPLE_SCANNED)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == m_hMsgQ)
    {
        return ERROR_INVALID_STATE;
    }

    CS_MSGQUEUE_BUFFER csMsgQueueBuf;
    csMsgQueueBuf.CsMsgQueueHeader.Size     = sizeof(CS_MSGQUEUE_HEADER);
    csMsgQueueBuf.CsMsgQueueHeader.Flags    = flag;
    csMsgQueueBuf.CsMsgQueueHeader.Context  = NULL;
    csMsgQueueBuf.pStreamDescriptor         = NULL;

    if (false == WriteMsgQueue(m_hMsgQ, reinterpret_cast<LPVOID>(&csMsgQueueBuf),  sizeof(CS_MSGQUEUE_BUFFER), PIN_TIMEOUT, 0))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false while sending notification\r\n"), this));
        return ERROR_WRITE_FAULT;
    }

    if (false == WriteMsgQueue(m_hMsgQ, reinterpret_cast<LPVOID>(Context),  sizeof(CAM_NOTIFICATION_CONTEXT), PIN_TIMEOUT, 0))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false while sending notification\r\n"), this));
        return ERROR_WRITE_FAULT;
    }

    return ERROR_SUCCESS;
}

