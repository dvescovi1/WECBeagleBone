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
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDBusReq.cpp
Abstract:
    SDBus Implementation.

Notes: 
--*/

#include <windows.h>
#include <bldver.h>
#include <cesdbus.h>
#include <marshal.hpp>

#include <sdhcd.h>

#include "sdbus.hpp"
#include "sdslot.hpp"
#include "sdbusreq.hpp"

void * CSDBusRequest::operator new(size_t stSize)
{
    return CSDHostContainer::AllocateBusRequest(stSize);
}
void CSDBusRequest::operator delete (void *pointer)
{
    CSDHostContainer::FreeBusRequest((CSDBusRequest *)pointer);
}
DWORD CSDBusRequest::g_dwRequestIndex = 0 ;
CSDBusRequest::CSDBusRequest(CSDDevice& sdDevice, SD_BUS_REQUEST& sdBusRequest, HANDLE hCallback, CSDBusRequest * pParentBus )
:   m_pParentBus(pParentBus)
,   m_hCallback(hCallback)
,   m_sdDevice(sdDevice)
{
    m_lRefCount = 0;
    m_pAsyncQueueNext = NULL;
    m_pChildListNext = NULL;

    m_dwRequestIndex = (SDBUS_REQUEST_INDEX_MASK & (DWORD)InterlockedIncrement((PLONG)&g_dwRequestIndex));

    ListEntry.Flink = NULL;
    ListEntry.Blink = NULL;
    hDevice = (m_sdDevice.GetDeviceHandle()).hValue;
    SystemFlags = sdBusRequest.SystemFlags;
    TransferClass = sdBusRequest.TransferClass;
    CommandCode = sdBusRequest.CommandCode;
    CommandArgument = sdBusRequest.CommandArgument;
    CommandResponse.ResponseType = sdBusRequest.CommandResponse.ResponseType;
    RequestParam   = sdBusRequest.RequestParam;
    NumBlocks = sdBusRequest.NumBlocks;
    BlockSize = sdBusRequest.BlockSize;
    HCParam = 0;
    pBlockBuffer = NULL ;
    pCallback    = sdBusRequest.pCallback;
    DataAccessClocks = 0; // reset data access clocks
    Flags = sdBusRequest.Flags ;
    Status = SD_API_STATUS_PENDING ;
    m_pOrinalAddr =  sdBusRequest.pBlockBuffer;
    if ((Flags & SD_BUS_REQUEST_PHYS_BUFFER) &&  sdBusRequest.cbSizeOfPhysList && sdBusRequest.pPhysBuffList) {
        cbSizeOfPhysList = sdBusRequest.cbSizeOfPhysList/sizeof(PHYS_BUFF_LIST); // Convert to unit.
        m_pOrignalPhysAddr = sdBusRequest.pPhysBuffList;
        if (!SUCCEEDED(CeAllocAsynchronousBuffer((PVOID *)&pPhysBuffList,m_pOrignalPhysAddr,cbSizeOfPhysList*sizeof(PHYS_BUFF_LIST),ARG_I_PTR))) {
            Flags &= ~SD_BUS_REQUEST_PHYS_BUFFER;
            pPhysBuffList = NULL;
            cbSizeOfPhysList = 0 ;
        }
    }
    else {
        m_pOrignalPhysAddr= pPhysBuffList = NULL;
        cbSizeOfPhysList = 0;
    }

    if (m_pParentBus)
        m_pParentBus->AddRef();
    m_dwArguDesc = 0 ;
    m_fCompleted = FALSE;
    m_ExternalHandle = NULL;
}
CSDBusRequest::~CSDBusRequest()
{
    TerminateLink();
    if (m_pOrinalAddr && pBlockBuffer && m_pOrinalAddr != pBlockBuffer) {
        VERIFY(SUCCEEDED(CeFreeAsynchronousBuffer(pBlockBuffer,m_pOrinalAddr, NumBlocks*BlockSize, m_dwArguDesc)));
    }
    if ( pPhysBuffList && cbSizeOfPhysList && m_pOrignalPhysAddr && m_pOrignalPhysAddr!=pPhysBuffList) {
        VERIFY(SUCCEEDED(CeFreeAsynchronousBuffer(pPhysBuffList,m_pOrignalPhysAddr, cbSizeOfPhysList*sizeof(PHYS_BUFF_LIST), ARG_I_PTR)));
    }
}
BOOL CSDBusRequest::Init()
{
    BOOL fRet = TRUE;
    if( TransferClass == SD_READ || TransferClass == SD_WRITE ) {
        if( NumBlocks == 0 ) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusRequest: SDBusRequest- No transfer buffers passed \n")));
            Status = SD_API_STATUS_INVALID_PARAMETER;
            fRet = FALSE;
        }
        else if( m_pOrinalAddr == NULL ) {
        // check pointer to see if block array ptr is non-NULL
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusRequest: SDBusRequest- NULL buffer pointer passed \n")));
            Status =  SD_API_STATUS_INVALID_PARAMETER;
            fRet = FALSE;
        } else {
            pBlockBuffer = (PUCHAR) m_pOrinalAddr ;
        }
    }
    if (fRet) {
        if ((Device_SD_Memory == m_sdDevice.GetDeviceType()) || (Device_MMC == m_sdDevice.GetDeviceType())) {
            if (TransferClass == SD_READ) {
                // set for read
                DataAccessClocks = m_sdDevice.GetCardInfo().SDMMCInformation.DataAccessReadClocks;
            } else if (TransferClass == SD_WRITE) {
                // set write
                DataAccessClocks = m_sdDevice.GetCardInfo().SDMMCInformation.DataAccessWriteClocks; 
            }
        }
        SystemFlags &= ~SYSTEM_FLAGS_RETRY_COUNT_MASK;
        SystemFlags |= (CSDHostContainer::GetRetryCount() & SYSTEM_FLAGS_RETRY_COUNT_MASK);
        SystemFlags |= ((Flags & SD_SLOTRESET_REQUEST)!=0?SD_BUS_REQUEST_SLOT_RESET:0);
        
        if (m_pParentBus == NULL) { // This is paranet
        
            UCHAR ucSDIOFlags =  m_sdDevice.GetCardInfo().SDIOInformation.Flags;
            if ((( 1 < NumBlocks ) && 
                       ((SD_CMD_IO_RW_EXTENDED == CommandCode) && 
                         (((SD_READ == TransferClass) && 
                           (0 != (ucSDIOFlags & (SFTBLK_USE_FOR_CMD53_READ | SFTBLK_USE_ALWAYS )))) ||
                       ((SD_WRITE == TransferClass ) && 
                           (0 != (ucSDIOFlags & (SFTBLK_USE_FOR_CMD53_WRITE | SFTBLK_USE_ALWAYS ))))))) ||
                    ((SD_CMD_READ_MULTIPLE_BLOCK == CommandCode) && 
                       (0 != (ucSDIOFlags & (SFTBLK_USE_FOR_CMD18 | SFTBLK_USE_ALWAYS )))) ||
                    ((SD_CMD_WRITE_MULTIPLE_BLOCK == CommandCode) && 
                       (0 != (ucSDIOFlags & (SFTBLK_USE_FOR_CMD25 | SFTBLK_USE_ALWAYS ))))) {
                // It is really hard to seperate physical buffer. If this happen. we disable physical buffer.
                Flags &= ~SD_BUS_REQUEST_PHYS_BUFFER;
                fRet = BuildSoftBlock();
            }
            if (((SD_CMD_READ_MULTIPLE_BLOCK != CommandCode) || 
                      (0 == (ucSDIOFlags & (SFTBLK_USE_FOR_CMD18 | SFTBLK_USE_ALWAYS)))) &&
                    ((SD_CMD_WRITE_MULTIPLE_BLOCK != CommandCode) || 
                      (0 == (ucSDIOFlags & (SFTBLK_USE_FOR_CMD25 | SFTBLK_USE_ALWAYS )))) && 
                    (0 == (ucSDIOFlags & SFTBLK_USE_ALWAYS))) {
                // check for optional request
                fRet =  BuildOptionalRequest( ucSDIOFlags );
            }        
        }
    }
    ASSERT(fRet);
    if (!fRet)
        m_fCompleted = TRUE;
    return fRet;
        
}

BOOL CSDBusRequest::CheckForCompletion()
{
    if (m_pParentBus!=NULL) {
        return m_pParentBus->CheckForCompletion();
    }
    else if (IsComplete()) {
        // This one is completed.
        if (SD_API_SUCCESS(Status) && m_pChildListNext!=NULL) {
            Status = m_pChildListNext->GetFirstFailedStatus();
        }
#ifdef DEBUG
        SD_CARD_STATUS cardStatus;

        if (SDBUS_ZONE_REQUEST) {
            DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("--- SDBusDriver: CMD%d  CMDArg: 0x%08X TransferClass:%d ResponseType: %s Complete\n"),
                CommandCode, CommandArgument,TransferClass,  SDCardResponseTypeLookUp[CommandResponse.ResponseType & 0x7]));

            if (SD_API_SUCCESS(Status)) {
                if ( (ResponseR1 == CommandResponse.ResponseType) || (ResponseR1b == CommandResponse.ResponseType)) {
                    SDGetCardStatusFromResponse(&CommandResponse, &cardStatus);
                    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("--- SDBusDriver: R1,R1b Response, Card Status: 0x%08X,  Last State: %s \n"),
                        cardStatus,
                        SDCardStateStringLookUp[((CommandResponse.ResponseBuffer[2] >> 1) & 0x0F)]));
                }

                if (NoResponse != CommandResponse.ResponseType) {
                    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("--- SDBusDriver: Response Dump: \n")));

                    if (ResponseR2 == CommandResponse.ResponseType) {
                        SDOutputBuffer(CommandResponse.ResponseBuffer, 17);
                    } else {
                        SDOutputBuffer(CommandResponse.ResponseBuffer, 6);
                    }
                }

                if (NULL != pBlockBuffer) {   
                    if (SD_READ == TransferClass) {
                        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("--- SDBusDriver: Read Data Transfered : NumBlocks:%d BytesPerBlock:%d  \n"),
                            NumBlocks, BlockSize));
                        if (SDBUS_ZONE_BUFFER) {
                            DEBUGMSG(SDBUS_ZONE_BUFFER, (TEXT("--- SDBusDriver: Read Data Dump: \n")));
                            SDOutputBuffer(pBlockBuffer, NumBlocks * BlockSize);
                        }
                    } else {
                        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("--- SDBusDriver: Write Transfer Complete:  NumBlocks:%d BytesPerBlock:%d\n"),
                            NumBlocks, BlockSize));
                    }
                }
            } 
        }
#endif
        PSD_BUS_REQUEST_CALLBACK pCallbackPtr = (PSD_BUS_REQUEST_CALLBACK)InterlockedExchange( (LPLONG)&pCallback, NULL); // Make sure only call once.
        if (pCallbackPtr) {
            __try { 
                if (m_hCallback) {
                    IO_BUS_SD_REQUEST_CALLBACK busSdRequestCallback = {
                        pCallbackPtr,  m_sdDevice.GetDeviceHandle().hValue,m_ExternalHandle,
                        m_sdDevice.GetDeviceContext(), RequestParam };
                    CeDriverPerformCallback(
                        m_hCallback, IOCTL_BUS_SD_REQUEST_CALLBACK,&busSdRequestCallback,sizeof(busSdRequestCallback),
                        NULL,0,NULL,NULL);
                    
                }
                else {
                    pCallbackPtr(m_sdDevice.GetDeviceHandle().hValue,  // device handle
                        (PSD_BUS_REQUEST)m_ExternalHandle,           // the request
                        m_sdDevice.GetDeviceContext(), // device context
                        RequestParam);  // request argument
                    }
            } __except (SDProcessException(GetExceptionInformation())) {

                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("--- SDBusDriver: Exception caught in CompleteRequest when calling callback in device %s \n"),
                    m_sdDevice.GetClientName()));     
            }
        }
        
    }
    return TRUE;
}
BOOL  CSDBusRequest::IsRequestNeedRetry()
{
    if ((m_sdDevice.GetClientFlags() & SD_CLIENT_HANDLES_RETRY)==0) { // we need retry.
        if (((SD_API_STATUS_RESPONSE_TIMEOUT == Status) || (SD_API_STATUS_DATA_TIMEOUT == Status) || (SD_API_STATUS_CRC_ERROR == Status))
                && GetRetryCount()!=0) {
            return TRUE;
        }
        
    }
    return FALSE;
}
BOOL    CSDBusRequest::BuildSoftBlock() 
{
    ASSERT(m_pParentBus == NULL);
    ASSERT(NumBlocks>1);
    ASSERT(pBlockBuffer!=NULL);

    BOOL fRetun = TRUE;
    UCHAR SoftBlockCommand = CommandCode;
    DWORD SoftBlockArgument ; 
    DWORD SoftwareBlockByteCount = NumBlocks * BlockSize; 
    DWORD SoftBlockLengthInBytes = min (BlockSize, 1 + SD_CMD53_BLOCK_COUNT);
    PBYTE pSoftBlockBuffer = pBlockBuffer;
    BOOL  fIncreasAddr ;

    // Setup Current Transfer 
    if ( SD_CMD_IO_RW_EXTENDED == SoftBlockCommand )  {
        SoftBlockArgument = (CommandArgument & ~( SD_CMD53_BLOCK_MODE | SD_CMD53_BLOCK_COUNT )) ;
        SoftBlockArgument |= SoftBlockLengthInBytes ;
        fIncreasAddr = ((CommandArgument & SD_CMD53_OPCODE) != 0);
        CommandCode = SoftBlockCommand ;
    }
    else  {
        //  Set the appropriate command.
        SoftBlockArgument = CommandArgument;
        fIncreasAddr = TRUE;
        CommandCode -= 1;
    }

    //  Turn this request into a byte mode request.
    NumBlocks = 1;
    BlockSize = SoftBlockLengthInBytes;
    CommandArgument = SoftBlockArgument ;
    pBlockBuffer = pSoftBlockBuffer;

    // Update.
    if (SoftwareBlockByteCount > SoftBlockLengthInBytes) 
        SoftwareBlockByteCount -= SoftBlockLengthInBytes;
    else
        SoftwareBlockByteCount = 0;
    pSoftBlockBuffer += SoftBlockLengthInBytes;
    if (fIncreasAddr) {
        //  Increasing address being used.
        if ( SD_CMD_IO_RW_EXTENDED == SoftBlockCommand ) {
            SoftBlockArgument = ( SoftBlockArgument & ( ~ SD_CMD53_REGISTER_ADDRESS ))
                | ((SoftBlockArgument + (SoftBlockLengthInBytes << SD_CMD53_REGISTER_ADDRESS_POS )) & SD_CMD53_REGISTER_ADDRESS );
        }
        else {
            SoftBlockArgument += SoftBlockLengthInBytes;
        }
    }

    
    while (SoftwareBlockByteCount && fRetun ) {
        SD_BUS_REQUEST sdRequest = {
            {NULL},m_sdDevice.GetDeviceHandle().hValue,0,
            TransferClass,SoftBlockCommand, SoftBlockArgument,
            {CommandResponse.ResponseType,{0}},
            NULL,
            SD_API_STATUS_UNSUCCESSFUL,
            1,SoftBlockLengthInBytes,0,
            pBlockBuffer,NULL,
            0,
            Flags
        };
        
        CSDBusRequest * pNewRequest =  new CSDBusRequest(m_sdDevice, sdRequest, NULL, this );
        if (pNewRequest && pNewRequest->Init() ) {
            // Added to Child List.
            pNewRequest->AddRef();
            SetChildListNext(pNewRequest);
            
            // Update.
            if (SoftwareBlockByteCount > SoftBlockLengthInBytes) 
                SoftwareBlockByteCount -= SoftBlockLengthInBytes;
            else
                SoftwareBlockByteCount = 0;
            pSoftBlockBuffer += SoftBlockLengthInBytes;
            if (fIncreasAddr) {
                //  Increasing address being used.
                if ( SD_CMD_IO_RW_EXTENDED == SoftBlockCommand ) {
                    SoftBlockArgument = ( SoftBlockArgument & ( ~ SD_CMD53_REGISTER_ADDRESS ))
                        | ((SoftBlockArgument + (SoftBlockLengthInBytes << SD_CMD53_REGISTER_ADDRESS_POS )) & SD_CMD53_REGISTER_ADDRESS );
                }
                else {
                    SoftBlockArgument += SoftBlockLengthInBytes;
                }
            }
            
        }
        else {
            fRetun = FALSE;
            if (pNewRequest!=NULL)
                delete pNewRequest;
        }

    }
    return fRetun;
}
BOOL  CSDBusRequest::BuildOptionalRequest( UCHAR /*ucSDIOFlags*/ )
{
    BOOL fReturn = TRUE;
    if (Flags & (SD_AUTO_ISSUE_CMD12 | SD_SDIO_AUTO_IO_ABORT)) {

        SD_BUS_REQUEST sdRequest = { // for SD_AUTO_ISSUE_CMD12
            {NULL},m_sdDevice.GetDeviceHandle().hValue,0,
            SD_COMMAND,
            SD_CMD_STOP_TRANSMISSION, 0 ,{ResponseR1b,{0}},
            NULL,
            SD_API_STATUS_UNSUCCESSFUL,
            0,0,0,
            NULL,NULL,
            0,
            0
        };
        if (Flags & SD_SDIO_AUTO_IO_ABORT) {
            DEBUGCHK( m_sdDevice.GetCardInfo().SDIOInformation.Function != 0);
            // CMD52
            sdRequest.CommandCode = SD_IO_RW_DIRECT;
            // set up argument to write the function number to the I/O abort register
            sdRequest.CommandArgument = BUILD_IO_RW_DIRECT_ARG(SD_IO_OP_WRITE,      
                SD_IO_RW_NORMAL,
                0,    // must be function 0 for access to common regs
                SD_IO_REG_IO_ABORT,  
                m_sdDevice.GetCardInfo().SDIOInformation.Function);

            sdRequest.CommandResponse.ResponseType = ResponseR5;
        }
        
        CSDBusRequest * pNewRequest =  new CSDBusRequest(m_sdDevice, sdRequest, NULL, this );
        if (pNewRequest && pNewRequest->Init() ) {
            // Added to Child List.
            pNewRequest->AddRef();
            SetChildListNext(pNewRequest);
        }
        else {
            fReturn = FALSE;
            if (pNewRequest)
                delete pNewRequest;
        }
    }
    return fReturn;
}

BOOL    CSDBusRequest::MapAsyncBuffer()
{
    BOOL fRet = TRUE;
    if (!IsAsyncBufferMapped()) {
        if ( m_pOrinalAddr && NumBlocks && BlockSize ) {
            m_dwArguDesc = (TransferClass == SD_READ?ARG_O_PTR:ARG_I_PTR);
            if (NumBlocks>=(1<<16) || BlockSize>=(1<<16) || !SUCCEEDED(CeAllocAsynchronousBuffer((PVOID *)&pBlockBuffer,m_pOrinalAddr,NumBlocks*BlockSize,m_dwArguDesc))) {
                DEBUGMSG(SDCARD_ZONE_ERROR,   (TEXT("MapAsyncBuffer CeAllocAsynchronousBuffer Error (%d)\r\n"),GetLastError()));
                ASSERT(FALSE);
                Status = SD_API_STATUS_ACCESS_VIOLATION;
                pBlockBuffer = (PUCHAR) m_pOrinalAddr ;
                fRet = FALSE;
            }         
        }
    }
    return fRet;
}

