// All rights reserved ADENEO EMBEDDED 2010
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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//

#ifndef __PIN_DD
#define __PIN_DD

#include "marshal.hpp" //helper classes to marshal/alloc embedded/async buffer

#ifdef __cplusplus
extern "C" {
#endif

#define PIN_TIMEOUT    250

typedef struct _tagStreamDescriptorShadow
{
    PCS_STREAM_DESCRIPTOR pCsStreamDescriptorExternal;
    CS_STREAM_DESCRIPTOR  csStreamDescriptorShadow;
    BOOL                  m_fBusy;
    MarshalledBuffer_t   *m_pMarshalledDataBuffer; //Marshalled/Async allocated data buffer when the memory model is not CSPROPERTY_BUFFER_DRIVER
    MarshalledBuffer_t   *m_pMarshalledStreamDesc; //Marshalled/Async allocated stream Desc when the memory model is not CSPROPERTY_BUFFER_DRIVER
    PCS_STREAM_DESCRIPTOR m_pUnMarshalledStreamDesc; //driver maintains the unmarshalled buffer value to instruct the client where to read data from    
} CS_STREAM_DESCRIPTOR_SHADOW, * PCS_STREAM_DESCRIPTOR_SHADOW;


typedef class CPinDevice 
{
public:
    CPinDevice( );

    ~CPinDevice( );

    bool
    InitializeSubDevice(
        PCAMERADEVICE pCamDevice
        );

    DWORD
    StreamInstantiate(
        PCSPROPERTY_STREAMEX_S pCsPropStreamEx,
        __out_bcount(OutBufLen) PUCHAR pOutBuf,
        DWORD                  OutBufLen,
        PDWORD                 pdwBytesTransferred
        );

    DWORD
    PinHandleConnectionRequests(
        PCSPROPERTY pCsProp,
        __out_bcount(OutBufLen) PUCHAR pOutBuf,
        DWORD       OutBufLen,
        PDWORD      pdwBytesTransferred
        );

    DWORD
    PinHandleBufferRequest(
        CSBUFFER_INFO  csBufferInfo,
        __out_bcount(OutBufLen) PUCHAR pOutBuf,
        DWORD  OutBufLen,
        DWORD *pdwBytesTransferred
        );

    DWORD
    PinHandleCustomRequests(
        __in_bcount(InBufLen) PUCHAR pInBuf,
        DWORD  InBufLen,
        __out_bcount(OutBufLen) PUCHAR pOutBuf,
        DWORD  OutBufLen,
        PDWORD pdwBytesTransferred
        );

    DWORD
    HandlePinIO( );

    void 
    FlushBufferQueue();

    CSSTATE
    GetState( ) { return m_CsState; };
    
    void
    SetState(
        CSSTATE   CsState,
        CSSTATE * CsPrevState
        );


    DWORD
    PauseStream( );

    DWORD
    CloseSubDevice( );

    ULONG
    PictureNumber( ) const;

    ULONG
    FramesDropped( ) const;

    ULONG
    FrameSize( ) const;

    LONG
    IncrementStillCount()
    {   return InterlockedIncrement(&m_lStillCount);   };

private:

    DWORD
    AllocateBuffer( 
        PCS_STREAM_DESCRIPTOR pCsDescriptor,
        LPVOID pOutBuf,
        DWORD  OutBufLen,
        DWORD *pdwBytesTransferred
        );

    DWORD
    DeallocateBuffer(
        PCS_STREAM_DESCRIPTOR pCsDescriptor
        );

    DWORD
    EnqueueDescriptor(
        PCS_STREAM_DESCRIPTOR pCsDescriptor
        );

    bool
    RemoveBufferFromList(
        PCS_STREAM_DESCRIPTOR * ppCsStreamDesc,
        PVOID                 * ppMappedData,
        PVOID                 * ppUnmappedData
        );

    bool
    ResetBufferList( );
    
    DWORD
    PinHandleConnStateSubReqs(
        ULONG  ulReqFlags,
        __out_bcount(OutBufLen) PUCHAR pOutBuf,
        DWORD  OutBufLen,
        PDWORD pdwBytesTransferred
        );

    DWORD
    PinHandleConnDataFormatSubReqs(
        ULONG                          ulReqFlags,
        PCS_DATAFORMAT_VIDEOINFOHEADER pCsDataFormatVidInfoHdr,
        PDWORD                         pdwBytesTransferred
        );

    BOOL InitMsgQueueDescriptor(
        PCS_MSGQUEUE_BUFFER pCsMsgQBuff, 
        PCS_STREAM_DESCRIPTOR pCsStreamDesc, 
        PVOID pMappedData, 
        PVOID pUnmappedData, 
        BOOL bFillBuffer);

    LONG
    GetIndexFromHandle( 
        DWORD  dwHandle,
        LPVOID pBuffer
        );

    DWORD 
    CreateHandle( 
        DWORD  dwIndex, 
        LPVOID pBuffer 
        );

    DWORD
    SwSetupStreamDescriptor(
        DWORD                 dwIndex,
        PCS_STREAM_DESCRIPTOR pCsStreamDesc,
        LPVOID                pBuffer
        );

    DWORD
    HwSetupStreamDescriptor(
        DWORD dwIndex
        );

    bool ReadMemoryModelFromRegistry();

    LONG DecrementStillCount()
    {   return InterlockedDecrement(&m_lStillCount);   };   

    ULONG              m_ulPinId;
    // Get MAX buffer information from pdd ULONG              m_ulMaxNumOfBuffers;
    ULONG              m_ulFrameSize;
    ULONG              m_ulFramesDropped;
    ULONG              m_ulPictureNumber;
    LONG               m_lStillCount; //Count of still capture requests.

    DWORD              m_dwMemoryModel;
    DWORD              m_dwBufferCount;
    DWORD              m_ulMaxNumOfBuffers;
    DWORD              m_msStart;
    DWORD              m_msLastPT;
    
    REFERENCE_TIME     m_RtAveTimePerFrame;

    HANDLE             m_hMsgQ;

    CRITICAL_SECTION   m_csStreamBuffer;
    CRITICAL_SECTION   m_csStreamIO;

    PCAMERADEVICE      m_pCamAdapter;
    PCS_STREAM_DESCRIPTOR_SHADOW m_pStreamDescriptorList;
    CS_DATARANGE_VIDEO m_CsDataRangeVideo;

    CSSTATE            m_CsState;

    bool               m_fClientInitialized;
    bool               m_fDiscontinuity;


} PINDEVICE, * PPINDEVICE;

typedef struct CPinInitHandle
{
    PCAMERADEVICE pCamDevice;
} PININITHANDLE, * PPININITHANDLE;

#ifdef __cplusplus
}
#endif

#endif //__PIN_DD
