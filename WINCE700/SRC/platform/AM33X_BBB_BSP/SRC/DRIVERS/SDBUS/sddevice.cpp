/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
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
Module Name:
    SDDevice.cpp
Abstract:
    SDBus Implementation.

Notes:
--*/
#include <windows.h>
#include <creg.hxx>

#include <sdhcd.h>
#include <cesdbus.h>
#include <sdcard.h>

#include "sdbus.hpp"
#include "sdslot.hpp"
#include "sdbusreq.hpp"
#include "sddevice.hpp"


#define MMC_8BIT_SEND_TEST_PATTERN 0

#define SD_GET_IO_RW_DIRECT_RESPONSE_FLAGS(pResponse) (pResponse)->ResponseBuffer[SD_IO_R5_RESPONSE_FLAGS_BYTE_OFFSET]
#define SD_GET_IO_RW_DIRECT_DATA(pResponse)           (pResponse)->ResponseBuffer[SD_IO_R5_RESPONSE_DATA_BYTE_OFFSET]


DWORD CSDDevice::g_FuncRef = 0 ;
CSDDevice::CSDDevice(DWORD dwFunctionIndex, CSDSlot& sdSlot)
:   m_sdSlot(sdSlot)
,   m_FuncionIndex(dwFunctionIndex)
{
    m_DeviceType = Device_Unknown;
    m_RelativeAddress = 0 ;
    m_FuncRef = (DWORD)InterlockedIncrement((LPLONG)&g_FuncRef);


    m_fAttached = FALSE;
    m_pDriverFolder = NULL;

    m_hCallbackHandle = NULL;
    m_fIsHandleCopied = FALSE;
    m_ClientName[0] = 0;
    m_ClientFlags = 0;
    m_DeviceType=Device_Unknown;
    m_pDeviceContext = NULL;
    m_pSlotEventCallBack = NULL;
    m_RelativeAddress = 0;
    m_OperatingVoltage =0 ;
    m_pSystemContext = NULL;
    // set default access clocks
    memset(&m_SDCardInfo,0,sizeof(m_SDCardInfo));
    m_SDCardInfo.SDMMCInformation.DataAccessReadClocks = SD_UNSPECIFIED_ACCESS_CLOCKS;
    m_SDCardInfo.SDMMCInformation.DataAccessWriteClocks = SD_UNSPECIFIED_ACCESS_CLOCKS;
    m_SDCardInfo.SDIOInformation.Function = (UCHAR)dwFunctionIndex;
    m_bCardSelectRequest = FALSE;
    m_bCardDeselectRequest = FALSE;

    m_dwCurSearchIndex = 0;
    m_dwCurFunctionGroup = 0 ;

    m_CurSpeedMode = UndefinedSpeedMode;

    InitializeCriticalSection(&m_csChildRequests);

    for(int i = 0; i < SYNC_EVENTS_ARRAYSIZE; i++){
        m_SyncEventArray.hSyncEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        ASSERT(m_SyncEventArray.hSyncEvents[i] != NULL);
        m_SyncEventArray.fEventsAvailability[i] = TRUE;
    }


}
CSDDevice::~CSDDevice()
{
    Detach();
    if (m_SDCardInfo.SDIOInformation.pCommonInformation!=NULL) {
        if (m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation)
            delete[] m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation;
        if (m_SDCardInfo.SDIOInformation.pFunctionInformation!=NULL)
            delete [] m_SDCardInfo.SDIOInformation.pFunctionInformation;
        delete m_SDCardInfo.SDIOInformation.pCommonInformation;
    }

    DeleteCriticalSection(&m_csChildRequests);

    for(int i = 0; i < SYNC_EVENTS_ARRAYSIZE; i++){
        CloseHandle(m_SyncEventArray.hSyncEvents[i]);
    }

}

BOOL CSDDevice::Init()
{
    memset(&m_CardInterfaceEx,0,sizeof(m_CardInterfaceEx));
    m_CardInterfaceEx.ClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;
    m_CurSpeedMode = LowSpeed;
    return TRUE;
}
BOOL CSDDevice::Attach()
{
    m_fAttached = TRUE;
    return TRUE;
}

BOOL CSDDevice::Detach()
{
    Lock();
    if (m_fAttached) {
        m_fAttached = FALSE;
        SDIOConnectDisconnectInterrupt(NULL,FALSE);
        SDUnloadDevice();
        for (DWORD dwIndex=0; dwIndex<m_dwArraySize ; dwIndex++ ) {
            if( m_rgObjectArray[m_dwCurSearchIndex] ) {
                // After this point the status should return during the completion.
                SDBUS_REQUEST_HANDLE sdBusRequestHandle ;
                sdBusRequestHandle.bit.sdBusIndex = m_sdSlot.GetHost().GetIndex();
                sdBusRequestHandle.bit.sdSlotIndex = m_sdSlot.GetSlotIndex();
                sdBusRequestHandle.bit.sdFunctionIndex = m_FuncionIndex;
                sdBusRequestHandle.bit.sdRequestIndex = dwIndex;
                sdBusRequestHandle.bit.sd1f = 0x1f;
                sdBusRequestHandle.bit.sdRandamNumber =RawObjectIndex(m_dwCurSearchIndex)->GetRequestRandomIndex();

                SDFreeBusRequest_I(sdBusRequestHandle.hValue);
            }
        }
    }
    Unlock();
    return FALSE;
}

SDBUS_DEVICE_HANDLE CSDDevice::GetDeviceHandle()
{
    SDBUS_DEVICE_HANDLE retHandle ;
    if (m_fAttached) {
        retHandle.bit.sdBusIndex = m_sdSlot.GetHost().GetIndex();
        retHandle.bit.sdSlotIndex = m_sdSlot.GetSlotIndex();
        retHandle.bit.sdFunctionIndex = m_FuncionIndex;
        retHandle.bit.sdRandamNumber = m_FuncRef;
        retHandle.bit.sdF = SDBUS_DEVICE_HANDLE_FLAG;
    }
    else
        retHandle.hValue = NULL;
    return retHandle;
}
BOOL    CSDDevice::IsValid20Card()
{
    BOOL fValid20Card = FALSE;
    if (m_DeviceType!= Device_SD_IO && m_DeviceType != Device_SD_Combo ) {
        SD_API_STATUS status = SD_API_STATUS_DEVICE_UNSUPPORTED;
        SD_COMMAND_RESPONSE  response;                       // response buffer
        // Supported Voltage.
        const BYTE fCheckFlags = 0x5a;
        BYTE fVHS = (((m_sdSlot.VoltageWindowMask & 0x00f80000)!=0)? 1: 2); // 2.0 / 4.3.13
        status = SendSDCommand(SD_CMD_SEND_IF_COND,((DWORD)fVHS<<8)| fCheckFlags , ResponseR7, &response);
        if (SD_API_SUCCESS(status)) {
            if (response.ResponseBuffer[1]== fCheckFlags && response.ResponseBuffer[2]== fVHS) {
                fValid20Card = TRUE;
            }
            else {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDDevice::DetectSDCard: unusable device found in slot %d , CMD8 response %x, %x \n"),
                    m_sdSlot.GetSlotIndex(),response.ResponseBuffer[1],response.ResponseBuffer[2]));
                status = SD_API_STATUS_DEVICE_UNSUPPORTED ;
            }
        }
        else {//if (SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status)) // This may cause by SDHC. we have know way to tell.
            ASSERT(SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status));
            DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status),
                (TEXT("CSDDevice::DetectSDCard:  found CMD8 failed in slot %d, please check SDHC make sure to support Response7, SD error =0x%x \n"),m_sdSlot.GetSlotIndex(),status));
            status = SD_API_STATUS_SUCCESS;
        }
    }
    return fValid20Card;
}

SD_API_STATUS CSDDevice::DetectSDCard( DWORD& dwNumOfFunct)
{
    if (m_FuncionIndex!=0) {
        ASSERT(FALSE);
        return SD_API_STATUS_DEVICE_UNSUPPORTED;
    }
    SD_API_STATUS status = SD_API_STATUS_DEVICE_UNSUPPORTED;
    SD_COMMAND_RESPONSE  response;                       // response buffer
    dwNumOfFunct = 1;
    SDCARD_DEVICE_TYPE deviceType = Device_Unknown;

    // Detect SD IO Card.
    status = SendSDCommand(SD_CMD_IO_OP_COND, 0,ResponseR4,&response);
    if (SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status)) {
        if (SD_API_SUCCESS(status)) {
            UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_IO_OCR,&response);
            DWORD   dwNumFunctions = SD_GET_NUMBER_OF_FUNCTIONS(&response);
            BOOL    fMemoryPresent = SD_MEMORY_PRESENT_WITH_IO(&response);
            // Note. dwNumOfFunc does not include function 0. SDIO 3.3
            dwNumOfFunct = dwNumFunctions + 1;
            if (dwNumFunctions ) {
                SetDeviceType(deviceType = (fMemoryPresent? Device_SD_Combo: Device_SD_IO));
                status = SetOperationVoltage(Device_SD_IO, TRUE);
                if (!SD_API_SUCCESS(status) && fMemoryPresent) {
                    SetDeviceType(deviceType = Device_Unknown); // Try Memory
                    status = SD_API_STATUS_SUCCESS;
                }
            }
            else // Only one function.
                if (!fMemoryPresent) { // This is unsported.
                    status = SD_API_STATUS_DEVICE_UNSUPPORTED ;
                }

        }
        else {
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: SDIO Card check timeout, moving on \n")));
            status = SD_API_STATUS_SUCCESS; // Timeout is OK . we will continue to skan memory.
        }
    }

    // Detect SD MMC
    if (SD_API_SUCCESS(status) && deviceType == Device_Unknown) { // We have to do other than SD_IO.
        // Intiaize
        status = SendSDCommand( SD_CMD_GO_IDLE_STATE,  0x00000000,  NoResponse, NULL);
        if (SD_API_SUCCESS(status)) {
            DWORD dwVoltage = ((m_sdSlot.VoltageWindowMask & (SD_VDD_WINDOW_1_8_TO_1_9 | SD_VDD_WINDOW_1_7_TO_1_8))!=0?
                0x80 : (m_sdSlot.VoltageWindowMask & 0xff8000));
            DWORD dwArg = dwVoltage | 0x40000000;
            m_fMMCSectorAccessMode = FALSE;
            status = SendSDCommand( SD_CMD_MMC_SEND_OPCOND,dwArg, ResponseR3, &response);
            if (SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status)) {
                if (SD_API_SUCCESS(status)) {
                    UpdateCachedRegisterFromResponse( SD_INFO_REGISTER_OCR, &response);
                    m_fMMCSectorAccessMode = TRUE;
                    SetDeviceType(deviceType = Device_MMC);
                    status = SetOperationVoltage(Device_MMC,TRUE);
                    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("MMC card: sector access mode is used \n")));
                }
                else {
                    //if failed, ignore it
                    status = SD_API_STATUS_SUCCESS;
                }
            }
            else { // now try byte access mode
                status = SendSDCommand( SD_CMD_GO_IDLE_STATE,  0x00000000,  NoResponse, NULL);
                if (SD_API_SUCCESS(status)) {
                    status = SendSDCommand( SD_CMD_MMC_SEND_OPCOND,dwVoltage, ResponseR3, &response);
                    if (SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(status)) {
                        if (SD_API_SUCCESS(status)) {
                            UpdateCachedRegisterFromResponse( SD_INFO_REGISTER_OCR, &response);
                            SetDeviceType(deviceType = Device_MMC);
                            status = SetOperationVoltage(Device_MMC,TRUE);
                            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("MMC card: byte access mode is used \n")));
                        }
                        else {
                            //if failed, ignore it
                            status = SD_API_STATUS_SUCCESS;
                        }
                    }
                }
            }
        }
    }

    // Detect SD Memory
    if (SD_API_SUCCESS(status) && (deviceType == Device_Unknown || deviceType == Device_SD_Combo )) { // We have to do other than SD_IO.
        // Intiaize
        status = SendSDCommand( SD_CMD_GO_IDLE_STATE,  0x00000000,  NoResponse, NULL);
        if (SD_API_SUCCESS(status)) {
            BOOL fValid20Card = IsValid20Card();
            // PhiscalLayer 2.0 Table 4-27.
            status = SendSDAppCommand( SD_ACMD_SD_SEND_OP_COND, (fValid20Card? (m_sdSlot.VoltageWindowMask | 0x40000000): 0), ResponseR3,  &response);
            if (SD_API_SUCCESS(status)) {
                UpdateCachedRegisterFromResponse( SD_INFO_REGISTER_OCR, &response);
                if (deviceType == Device_Unknown)
                    SetDeviceType(deviceType = Device_SD_Memory);
                status = SetOperationVoltage(Device_SD_Memory, deviceType == Device_SD_Memory );
            }
        }
        if (!SD_API_SUCCESS(status)){
            // check to see what we discovered and post the appropriate error message
            if (Device_SD_Combo == deviceType) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: SDIOCombo, Memory portion in slot %d not responsing to ACMD41 \n"), m_sdSlot.GetSlotIndex()));
                status = SD_API_STATUS_SUCCESS;
                SetDeviceType(deviceType = Device_SD_IO);
            } else {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Unknown device found in slot %d \n"),m_sdSlot.GetSlotIndex()));
                status = SD_API_STATUS_DEVICE_UNSUPPORTED;
            }
        }
    }
    ASSERT(SD_API_SUCCESS(status));
    ASSERT(deviceType != Device_Unknown);
    return status;
}


CSDBusRequest* CSDDevice::InsertRequestAtEmpty( PDWORD pdwIndex, CSDBusRequest* pObject )
{
    if( pObject )
    {
        Lock();
        CSDBusRequest*  pReturn = NULL;
        DWORD dwStopIndex = m_dwCurSearchIndex;
        do  {
            if( m_rgObjectArray[m_dwCurSearchIndex] == NULL )
                break;
            else if (++m_dwCurSearchIndex>=m_dwArraySize)
                m_dwCurSearchIndex = 0 ;
        } while (dwStopIndex != m_dwCurSearchIndex);

        ASSERT(m_dwCurSearchIndex<m_dwArraySize);
        if( m_dwCurSearchIndex < m_dwArraySize && m_rgObjectArray[m_dwCurSearchIndex] == NULL )  {
            pReturn = m_rgObjectArray[m_dwCurSearchIndex] = pObject;
            pReturn->AddRef();
            if( pdwIndex )
                *( pdwIndex ) = m_dwCurSearchIndex;
        };
        Unlock();
        return pReturn;
    }
    else
    {
        return NULL;
    }
}
///////////////////////////////////////////////////////////////////////////////
//  GetCardRegisters - Get card registers
//  Input:  pBaseDevice - the base device
//          BaseDeviceType - base device type
//          pSlot - the slot
//  Output:
//  Return:  SD_API_STATUS code
//  Notes:  This function retreives important card registers for the device type
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::GetCardRegisters()
{
    if (m_FuncionIndex!=0) {
        ASSERT(FALSE);
        return SD_API_STATUS_DEVICE_UNSUPPORTED;
    }
    SD_API_STATUS               status = SD_API_STATUS_DEVICE_UNSUPPORTED;    // status
    SD_COMMAND_RESPONSE         response;  // response
    UCHAR                       scrReg[SD_SCR_REGISTER_SIZE]; // temporary buffer
    USHORT                      oidValue;  // oid value
    SD_CARD_STATUS              cardStatus; // card status

    // must get CID first in order to get the cards into the IDENT state.
    // Check for SD I/O - only cards; will not have  CID.
    if (Device_SD_IO != m_DeviceType) {
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice:  Getting registers from slot %d \n"), m_sdSlot.GetSlotIndex()));

        // for MMC, SD Memory and SD Combo cards, retreive the CID
        status = SendSDCommand(SD_CMD_ALL_SEND_CID,0x00000000,ResponseR2,&response);

        if (!SD_API_SUCCESS(status)){
            return status;
        }

        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Got CID from device in slot %d \n"), m_sdSlot.GetSlotIndex()));
        // update shadow registers
        UpdateCachedRegisterFromResponse( SD_INFO_REGISTER_CID, &response);
    }

    // fetch/set the RCA
    if (Device_MMC != m_DeviceType) {
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Getting RCA from SD card in  slot %d .... \n"),m_sdSlot.GetSlotIndex()));
        // get the RCA
        status = SendSDCommand(SD_CMD_SEND_RELATIVE_ADDR, 0x00000000,ResponseR6,&response);

        if (!SD_API_SUCCESS(status)){
            return status;
        }
        // update shadow registers
        UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_RCA, &response);

        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Got RCA (0x%04X) from SD card in slot %d \n"),
            m_RelativeAddress, m_sdSlot.GetSlotIndex()));
    } else {
        // get OEM ID from the CID
        oidValue = m_CachedRegisters.CID[SD_CID_OID_OFFSET];
        oidValue |= (((USHORT)m_CachedRegisters.CID[SD_CID_OID_OFFSET + 1]) << 8);

        // for MMC cards set the RCA
        // take the unique OEM ID and add the slot number to it to form a system unique address
        m_RelativeAddress = (SD_CARD_RCA)(oidValue + m_sdSlot.GetSlotIndex());

        // add 1 if this is zero
        if (m_RelativeAddress == 0) {
            m_RelativeAddress++;
        }
        // for MMC cards, we must set the RCA
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Setting MMC RCA to 0x%04X .... \n"), m_RelativeAddress));

        // set the RCA
        status = SendSDCommand(SD_CMD_MMC_SET_RCA,((DWORD)m_RelativeAddress) << 16, ResponseR1,&response);

        if (!SD_API_SUCCESS(status)){
            return status;
        }

        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: RCA set for MMC device in slot %d \n"), m_sdSlot.GetSlotIndex()));
    }

    // now that the RCA has been fetched/set, we can move on to do other things.........

    // check for SD I/O - Only cards. They will not have a CSD or card status
    if (Device_SD_IO != m_DeviceType) {
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Getting CSD in slot %d .... \n"), m_sdSlot.GetSlotIndex()));

        // get the CSD
        status = SendSDCommand(SD_CMD_SEND_CSD,
            ((DWORD)m_RelativeAddress) << 16,
            ResponseR2,
            &response);

        if (!SD_API_SUCCESS(status)){
            return status;
        }

        // update shadow registers
        UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_CSD, &response);
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: Got CSD from device in slot %d \n"), m_sdSlot.GetSlotIndex()));
        // get the card status
        status = SendSDCommand( SD_CMD_SEND_STATUS,((DWORD)m_RelativeAddress) << 16,ResponseR1, &response);
        if (!SD_API_SUCCESS(status)){
            return status;
        }


        SDGetCardStatusFromResponse(&response, &cardStatus);

        if (cardStatus & SD_STATUS_CARD_IS_LOCKED) {
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: card in slot %d is locked\n"), m_sdSlot.GetSlotIndex()));
            m_SDCardInfo.SDMMCInformation.CardIsLocked = TRUE;
        }

    }

    // now in order to get the SCR register we must be in the trans state
    // also in order to do a few other things, so lets select the card now and leave it
    // selected
    // send CMD 7 to select the card and keep it selected, this is required  for SDIO cards
    // too as mentioned in I/O working group newsgroup
    status = SendSDCommand( SD_CMD_SELECT_DESELECT_CARD,((DWORD)m_RelativeAddress) << 16, ResponseR1b, &response);

    if (!SD_API_SUCCESS(status)){
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDDevice: Failed to select card in slot %d \n"), m_sdSlot.GetSlotIndex()));
        return status;
    }

    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice:Card in slot %d is now selected \n"), m_sdSlot.GetSlotIndex()));

    // only SD Memory and Combo cards have an SCR
    if ((Device_SD_Memory == m_DeviceType) || (Device_SD_Combo == m_DeviceType)) {
        // if the card is unlocked, get the SCR
        if (!m_SDCardInfo.SDMMCInformation.CardIsLocked) {
            // get the SD Configuration register
            status = SendSDAppCmd(SD_ACMD_SEND_SCR, 0, SD_READ,  ResponseR1,&response,
                1,                    // 1 block
                SD_SCR_REGISTER_SIZE, // 64 bits
                scrReg);

            // If the memory card is locked then the SCR becomes inaccessable. If we fail
            // to read the SCR then just set up the cached SCR register to default to
            // a 1bit access mode.

            if (!SD_API_SUCCESS(status)){
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDDevice: Failed to get SCR from device in slot %d \n"), m_sdSlot.GetSlotIndex()));
                memset(m_CachedRegisters.SCR, 0, sizeof(m_CachedRegisters.SCR));
            } else {
                // this is a spec discrepency, since the SCR register is not associated with
                // an address, the byte order it ambiguous.  All the cards we have seen store the data
                // most significant byte first as it arrives.
                for (ULONG ii = 0 ; ii < sizeof(m_CachedRegisters.SCR); ii++) {
                    m_CachedRegisters.SCR[ii] = scrReg[(SD_SCR_REGISTER_SIZE - 1) - ii];
                }

                DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Got SCR from device in slot %d \n"), m_sdSlot.GetSlotIndex()));
            }
        }
    }

    return SD_API_STATUS_SUCCESS;
}
SD_API_STATUS CSDDevice::HandleDeviceSelectDeselect(SD_SLOT_EVENT SlotEvent,BOOL fSelect)
{
    SD_API_STATUS          status = SD_API_STATUS_SUCCESS;
    UCHAR                  regValue;
    SD_COMMAND_RESPONSE    response;
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("CSDDevice: HandleSlotSelectDeselect++: %d \n"),SlotEvent));
    BOOL fSDIO = FALSE;
    BOOL fSDMemory = FALSE;

    if (m_DeviceType ==Device_SD_IO ||  m_DeviceType == Device_SD_Combo ) {
        fSDIO = TRUE;
    }
    if ((m_DeviceType == Device_MMC) || (m_DeviceType == Device_SD_Memory) ||(m_DeviceType == Device_SD_Combo)) {
        fSDMemory = TRUE;
    }

    if (fSelect) {
        if (m_FuncionIndex == 0 ) {
            // Select I/O controller first
            if (fSDIO) {
                if ( m_sdSlot.GetSlotState()!= SlotDeselected ) {
                    // reset I/O controller
                    regValue = SD_IO_REG_IO_ABORT_RES;
                    status = SDSynchronousBusRequest_I(SD_IO_RW_DIRECT,
                        (DWORD)BUILD_IO_RW_DIRECT_ARG((UCHAR)SD_IO_WRITE,FALSE,0,(SD_IO_REG_IO_ABORT),SD_IO_REG_IO_ABORT_RES),
                        SD_COMMAND,ResponseR5,&response,0,0,NULL,(DWORD)SD_SLOTRESET_REQUEST);
                    m_RelativeAddress = 0;
                }

                // Query OCR
                if (SD_API_SUCCESS(status)) {
                    status = SendSDCommand(SD_CMD_IO_OP_COND,0,ResponseR4,&response);
                }

                // Re-init I/O controller by powering it up
                if (SD_API_SUCCESS(status)) {
                    status = SetCardPower(Device_SD_IO,m_OperatingVoltage,FALSE);
                }

                if (SD_API_SUCCESS(status)) {
                    status = GetCardRegisters();
                }
                else {
                    DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("CSDSlot: Failed to power up SDIO card \n")));
                }
            }
            if (fSDMemory) {

                status = SDSynchronousBusRequest_I(SD_CMD_GO_IDLE_STATE,0x00000000,SD_COMMAND,NoResponse,
                        NULL,0,0,NULL,(DWORD)SD_SLOTRESET_REQUEST);

                if (SD_API_SUCCESS(status)) {
                    m_RelativeAddress = 0;
                }
                else{
                    DbgPrintZo(SDCARD_ZONE_ERROR,
                        (TEXT("CSDSlot: Go Idle Failed during selection: Status: 0x%08X on slot:%d \n"),status,m_sdSlot.GetSlotIndex()));
                }
                if (SD_API_SUCCESS(status)) {
                    if ((m_DeviceType == Device_SD_Memory) || (m_DeviceType == Device_SD_Combo)) {
                        status = SendSDAppCommand(SD_ACMD_SD_SEND_OP_COND,0,ResponseR3,&response);
                    }
                }

                // Now it is ready to re-initialize memory controller.
                // Re-initialize memory controller by powering it up
                // SetCardPower will send CMD0 first before issuing ACMD41
                if (SD_API_SUCCESS(status)) {
                    status = SetCardPower(m_DeviceType, m_OperatingVoltage,FALSE);
                }
                // For SD Memory, GetCardRegisters will do the following things:
                //  Issue CMD2 to make memory state trans to IDENT
                //  Issue CMD3 to get or set RCA
                //  Issue CMD7 to select the card again
                //  For SD Memory card, additional ACMD51 would be issued too, but
                //  should not have impact for init process.
                if (SD_API_SUCCESS(status)) {
                    status = GetCardRegisters();
                }
            }
        }
        else {
            CSDDevice * pDevice0 = m_sdSlot.GetFunctionDevice(0);
            if (pDevice0) {
                CopyContentFromParent(*pDevice0);
                pDevice0->DeRef();
            }
        }
    }
    else {// De select Card.
        if ((SlotEvent == SlotResetRequest) || (SlotEvent == SlotDeselectRequest)) {
            DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("CSDSlot: Deselect the card\n")));

            if (m_FuncionIndex == 0 ) {
                // deselect I/O controller first, see SDIO Spec v1.1 Section 3.4.4
                if (fSDIO) {
                    regValue = SD_IO_REG_IO_ABORT_RES;
                    status = SDReadWriteRegistersDirect_I(SD_IO_WRITE,SD_IO_REG_IO_ABORT,FALSE,&regValue,1);
                    if (SD_API_SUCCESS(status))  {
                        m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIOEnable = 0;
                    }
                    else {
                        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to deselect SDIO card \n")));
                    }
                }
                if (fSDMemory && SD_API_SUCCESS(status)) {
                    status = SDSynchronousBusRequest_I(SD_CMD_GO_IDLE_STATE,
                        0x00000000,SD_COMMAND,NoResponse,NULL,0,0,NULL,(DWORD)SD_SLOTRESET_REQUEST);
                    if (!SD_API_SUCCESS(status))  {
                        DbgPrintZo(SDCARD_ZONE_ERROR,(TEXT("CSDSlot: Go Idle Failed during deselection: Status: 0x%08X \n"),
                            status));
                    }

                }
            }
            m_RelativeAddress = 0;
            m_bCardDeselectRequest = FALSE;
        }
    }
    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  DeactivateCardDetect - Deativate Card Detect resistors on SD devices
//  Input:  pBaseDevice - the base device
//          BaseDeviceType - the base device type
//          pSlot - the slot
//  Output:
//  Return : SD_API_STATUS code
//  Notes:  This function deactivates the card detect resistor for SDIO,
//          SD and or Combo cards
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::DeactivateCardDetect()
{
    UCHAR                   regValue;       // intermediate value
    SD_API_STATUS           status;         // status
    SD_COMMAND_RESPONSE     response;       // response
    BOOL                    fDisableSDIO;   // disable SDIO portion?

    ASSERT(m_FuncionIndex == 0);
    if (Device_SD_IO == m_DeviceType ) {
        fDisableSDIO = TRUE;
    }
    else if (Device_SD_Combo == m_DeviceType) {
        fDisableSDIO = TRUE;

        // *** Combo Card Issue ***
        // SDIO 1.00 requires that both memory and SDIO portions of combo
        // cards be deactivated.
        // SDIO 1.10 only requires one of memory or SDIO combo to be disabled. (4.6)
        // Some 1.10 cards have a problem where only one can be disabled--disabling
        // both simultaneously will fail.
        //
        // Our solution is to disable both for < 1.10 and only disable memory
        // for >= 1.10.

        BYTE bSDIORevision; // SDIO spec version
        status = SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_CCCR, FALSE, &bSDIORevision, sizeof(bSDIORevision));
        if (!SD_API_SUCCESS(status)) {
            DEBUGMSG(SDCARD_ZONE_ERROR,
                (TEXT("SDBusDriver: Failed to read SDIO revision in slot %d. We will treat it as a 1.0 combo card \n"),
                m_sdSlot.GetSlotIndex()));
        }
        else {
            bSDIORevision >>= 4; // SDIO revision is in the upper four bits.
            if (bSDIORevision != 0x0) {
                fDisableSDIO = FALSE;
            }
        }
    }
    else {
        DEBUGCHK(Device_SD_Memory == m_DeviceType ||
            Device_MMC == m_DeviceType);
        fDisableSDIO = FALSE;
    }

    // for SD memory or combo, send ACMD42 to turn off card detect resistor
    if ( (Device_SD_Memory == m_DeviceType) ||
         (Device_SD_Combo == m_DeviceType) ) {
        // send ACMD42
        status = SendSDAppCommand(SD_ACMD_SET_CLR_CARD_DETECT,
            0x00000000,  // bit 0 - cleared to disconnect pullup resistor
            ResponseR1,
            &response);

        if (!SD_API_SUCCESS(status)){
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to turn off CD resistor in slot %d \n"), m_sdSlot.GetSlotIndex()));
            return status;
        }
    }

    // for SD I/O or SD Combo cards, write to the Bus Interface control register and clear the CD bit
    if (fDisableSDIO) {
        regValue = SD_IO_BUS_CONTROL_CD_DETECT_DISABLE;

        status = SDReadWriteRegistersDirect_I(SD_IO_WRITE,
            SD_IO_REG_BUS_CONTROL,
            TRUE,        // read after write
            &regValue,
            1);

        if (!SD_API_SUCCESS(status)){
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to turn off CD resistor for SDIO Device in slot %d \n"), m_sdSlot.GetSlotIndex()));
            return status;
        }
    }

    return SD_API_STATUS_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////////
//  UpdateCachedRegisterFromResponse - update the device shadow registers from the
//                                     information in the response buffer
//  Input:  pDevice - the device who's shadowed register should be updated
//          Register - the information to update
//          pResponse - the response containing the register data
//  Output:
//  Return
//  Notes:
//
///////////////////////////////////////////////////////////////////////////////
VOID CSDDevice::UpdateCachedRegisterFromResponse(SD_INFO_TYPE  Register,PSD_COMMAND_RESPONSE     pResponse)
{

    switch (Register) {

        case SD_INFO_REGISTER_CID:
            memcpy(m_CachedRegisters.CID, pResponse->ResponseBuffer, SD_CID_REGISTER_SIZE);
            break;
        case SD_INFO_REGISTER_RCA:
            // RCA is in bytes 3,4
            m_RelativeAddress = (SD_CARD_RCA)pResponse->ResponseBuffer[3];
            m_RelativeAddress |= ((SD_CARD_RCA)pResponse->ResponseBuffer[4]) << 8;
            break;
        case SD_INFO_REGISTER_OCR:
            m_CachedRegisters.OCR[3] = pResponse->ResponseBuffer[4];
            m_CachedRegisters.OCR[2] = pResponse->ResponseBuffer[3];
            m_CachedRegisters.OCR[1] = pResponse->ResponseBuffer[2];
            m_CachedRegisters.OCR[0] = pResponse->ResponseBuffer[1];
            break;
        case SD_INFO_REGISTER_CSD:
            memcpy(m_CachedRegisters.CSD, pResponse->ResponseBuffer, SD_CSD_REGISTER_SIZE);
            break;
        case SD_INFO_REGISTER_IO_OCR:
            m_CachedRegisters.IO_OCR[2] = pResponse->ResponseBuffer[3];
            m_CachedRegisters.IO_OCR[1] = pResponse->ResponseBuffer[2];
            m_CachedRegisters.IO_OCR[0] = pResponse->ResponseBuffer[1];
            break;
        default:
            DEBUGCHK(FALSE);
    }

}
SD_API_STATUS  CSDDevice::SetOperationVoltage(SDCARD_DEVICE_TYPE DeviceType,BOOL SetHCPower)
{
    SD_API_STATUS status = SD_API_STATUS_DEVICE_UNSUPPORTED;
    if (SetHCPower) {
        DWORD ocrValue = 0;
        ASSERT(m_FuncionIndex == 0 );
        if (m_DeviceType == Device_SD_Memory || m_DeviceType == Device_MMC) {
            ocrValue = (DWORD)m_CachedRegisters.OCR[0] & ~0xF;
            ocrValue |= ((DWORD)m_CachedRegisters.OCR[1]) << 8;
            ocrValue |= ((DWORD)m_CachedRegisters.OCR[2]) << 16;

            if (m_DeviceType == Device_MMC && (ocrValue &0x80)!=0 ) { // Low Voltage Support.
                ocrValue |= SD_VDD_WINDOW_1_8_TO_1_9 | SD_VDD_WINDOW_1_7_TO_1_8;
                ocrValue &= ~0x80;
            }

        }
        else if (m_DeviceType == Device_SD_IO || m_DeviceType == Device_SD_Combo) {
            ocrValue = (DWORD)m_CachedRegisters.IO_OCR[0] & ~0xF;
            ocrValue |= ((DWORD)m_CachedRegisters.IO_OCR[1]) << 8;
            ocrValue |= ((DWORD)m_CachedRegisters.IO_OCR[2]) << 16;
        }
        else
            ASSERT(FALSE);

        m_OperatingVoltage = m_sdSlot.SDGetOperationalVoltageRange(ocrValue);
    }

    // check to see if the voltages can be supported
    if (0 != m_OperatingVoltage ) {
        // power up the card
        status = SetCardPower(DeviceType, m_OperatingVoltage, SetHCPower);
    }
    return status;

}
///////////////////////////////////////////////////////////////////////////////
//  SetCardPower - Set the card power
//  Input:  DeviceType - the device type
//          OperatingVoltageMask - operating voltage mask
//          pSlot - the slot
//          SetHCPower - flag to indicate whether the power should be set in the HC
//                       (combo cards do not need to reset power if the I/O portion
//                        was already powered).
//
//  Output:
//  Return: SD_API_STATUS code
//  Notes:
//          This function sets the card power and polls the card until it is
//          not busy
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SetCardPower(SDCARD_DEVICE_TYPE DeviceType,DWORD OperatingVoltageMask,BOOL SetHCPower)
{
    SD_API_STATUS       status = SD_API_STATUS_SUCCESS;  // intermediate status
    SD_COMMAND_RESPONSE response;                        // response
    UCHAR               command;                         // command
    SD_RESPONSE_TYPE    responseType;                    // response type
    ULONG               powerUpRetries;                  // power up retries
    BOOL                appcmd = FALSE;                  // command is an app cmd
    ULONG               powerUpInterval;                 // powerup interval
    ULONG               totalPowerUpTime;                // total powerup time
    ULONG               powerUpIntervalByDevice;         // powerup interval by device

    switch (DeviceType) {

        case Device_MMC:
            command = SD_CMD_MMC_SEND_OPCOND;
            responseType = ResponseR3;
            powerUpIntervalByDevice = DEFAULT_POWER_UP_MMC_POLL_INTERVAL;
            break;
        case Device_SD_IO:
            command = SD_CMD_IO_OP_COND;
            responseType = ResponseR4;
            powerUpIntervalByDevice = DEFAULT_POWER_UP_SDIO_POLL_INTERVAL;
            break;
        case Device_SD_Memory:
            command = SD_ACMD_SD_SEND_OP_COND;
            responseType = ResponseR3;
            appcmd = TRUE;
            powerUpIntervalByDevice = DEFAULT_POWER_UP_SD_POLL_INTERVAL;
            break;
        default:
            DEBUGCHK(FALSE);
            return SD_API_STATUS_INVALID_PARAMETER;
    }

    // check to see if we need to apply power
    if (SetHCPower) {
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Setting slot %d power to 0x%08X \n"),
            m_sdSlot.GetSlotIndex(), OperatingVoltageMask));
        // set slot power
        status = m_sdSlot.SDSetSlotPower(OperatingVoltageMask);

        if (!SD_API_SUCCESS(status)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Host failed to set slot power 0x%08X on slot:%d \n"),
                status, m_sdSlot.GetSlotIndex()));
            return status;
        }

        m_sdSlot.DelayForPowerUp();
    }

    if (Device_SD_IO != DeviceType) {
        // put the card into idle again
        status = SendSDCommand(SD_CMD_GO_IDLE_STATE, 0x00000000, NoResponse, NULL);
        if (!SD_API_SUCCESS(status)){
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Go Idle Failed during powerup: Status: 0x%08X on slot:%d \n"),status,
                m_sdSlot.GetSlotIndex()));
            return status;
        }
        if (Device_SD_Memory == DeviceType  && IsValid20Card()) {
            OperatingVoltageMask |= 0x40000000; // HCS
        }
        else if ( Device_MMC==DeviceType ) {
            if(m_fMMCSectorAccessMode)
                OperatingVoltageMask |= 0x40000000; // Set sector access mode bit
            if ((OperatingVoltageMask & (SD_VDD_WINDOW_1_8_TO_1_9 | SD_VDD_WINDOW_1_7_TO_1_8))!=0) {
                OperatingVoltageMask |= 0x80;
                OperatingVoltageMask &= ~0x7f;
            }

        }
    }

    totalPowerUpTime = CSDHostContainer::RegValueDWORD(POWER_UP_POLL_TIME_KEY, DEFAULT_POWER_UP_TOTAL_WAIT_TIME);
    powerUpInterval = CSDHostContainer::RegValueDWORD(POWER_UP_POLL_TIME_INTERVAL_KEY, powerUpIntervalByDevice);

    powerUpRetries = totalPowerUpTime/powerUpInterval;

    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Power Set, checking card in slot %d, MaxRetries: %d, Time: %d MS, Interval: %d MS \n"),
        m_sdSlot.GetSlotIndex(), powerUpRetries, totalPowerUpTime , powerUpInterval));

    while (powerUpRetries != 0) {

        // sleep the powerup interval
        Sleep(powerUpInterval);

        if (appcmd) {
            // send it as an APP cmd
            status = SendSDAppCommand(command,
                OperatingVoltageMask,
                responseType,
                &response);
        } else {

            // send the command to get the ready bit
            status = SendSDCommand(command,
                OperatingVoltageMask,
                responseType,
                &response);
        }

        if (!SD_API_SUCCESS(status)){
            break;
        }

        if (Device_SD_IO == DeviceType) {
            // check to see if the I/O is ready
            if (SD_IS_IO_READY(&response)) {
                UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_IO_OCR, &response);
                // we're done
                break;
            }
        } else {
            // check to see if the MMC or Memory card is ready
            if (SD_IS_MEM_READY(&response)) {
                // card is ready
                UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_OCR, &response);
                break;
            }
        }


        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Card in slot %d not ready, countdown: %d \n"),m_sdSlot.GetSlotIndex(), powerUpRetries));
        powerUpRetries--;
    }

    if (0 == powerUpRetries) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Device failed to powerup after voltage setting \n")));
        status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
    }

    if (SD_API_SUCCESS(status)) {
        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Card in slot %d ready \n"),m_sdSlot.GetSlotIndex()));
    }

    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SetCardInterface - sets the interface for the card
//  Input:  pDevice - the device
//          pInterface - alternate interface, can be NULL
//  Output:
//  Return: SD_API_STATUS code
//  Notes:  This function sets the card's interface by issuing appropriate SD commands
//          to the card.
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SetCardInterface(PSD_CARD_INTERFACE_EX pInterfaceEx)
{
    UCHAR                   regValue;                           // register value
    SD_API_STATUS           status = SD_API_STATUS_SUCCESS;     // intermediate status
    SD_COMMAND_RESPONSE     response;                           // response
    PSD_CARD_INTERFACE_EX   pInterfaceToUse;                    // interface to use

    if (NULL != pInterfaceEx) {
        pInterfaceToUse = pInterfaceEx;
    } else {
        // otherwise use the one set in the device
        pInterfaceToUse = &m_CardInterfaceEx;
    }

        //  Determine if Soft-Block should be used.
    if (( NULL != m_SDCardInfo.SDIOInformation.pCommonInformation )
        && ( 0 == ( m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability
                    & SD_IO_CARD_CAP_SUPPORTS_MULTI_BLOCK_TRANS )))
    {
            //  The SDIO card does not support block mode.  Use Soft-Block instead.
        m_SDCardInfo.SDIOInformation.Flags = SFTBLK_USE_FOR_CMD53_READ | SFTBLK_USE_FOR_CMD53_WRITE;
        DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The SDIO card does not support block mode.  Use Soft-Block instead. \n")));
    }

    //  Some host controllers can not properly support multi-block operations.
    //  Enable Soft-Block for these operations.
    if ( 0 != (m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD18 ))
    {
        if ( 0 != ( m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD18 )) {
                //  The host controller needs to use Soft-Block for CMD18 read operations.
            m_SDCardInfo.SDIOInformation.Flags |= SFTBLK_USE_FOR_CMD18;
            DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD18 read operations. \n")));
        }
        if ( 0 != ( m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD25 )) {
                //  The host controller needs to use Soft-Block for CMD25 write operations.
            m_SDCardInfo.SDIOInformation.Flags |= SFTBLK_USE_FOR_CMD25;
            DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD25 write operations. \n")));
        }
        if ( 0 != (m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD53_READ )) {
            //  The host controller needs to use Soft-Block for CMD53 multi-block read operations.
            m_SDCardInfo.SDIOInformation.Flags |= SFTBLK_USE_FOR_CMD53_READ;
            DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD53 multi-block read operations. \n")));
        }
        if ( 0 != ( m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD53_WRITE )) {
                //  The host controller needs to use Soft-Block for CMD53 multi-block write operations.
            m_SDCardInfo.SDIOInformation.Flags |= SFTBLK_USE_FOR_CMD53_WRITE;
            DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD53 multi-block write operations. \n")));
        }
    }
    if ( 0 != ( m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD25 )) {
            //  The host controller needs to use Soft-Block for CMD25 write operations.
        m_SDCardInfo.SDIOInformation.Flags = SFTBLK_USE_FOR_CMD25;
        DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD25 write operations. \n")));
    }
    if ( 0 != ( m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD53_READ )) {
            //  The host controller needs to use Soft-Block for CMD53 multi-block read operations.
        m_SDCardInfo.SDIOInformation.Flags = SFTBLK_USE_FOR_CMD53_READ;
        DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD53 multi-block read operations. \n")));
    }
    if ( 0 != (m_sdSlot.Capabilities & SD_SLOT_USE_SOFT_BLOCK_CMD53_WRITE )) {
            //  The host controller needs to use Soft-Block for CMD53 multi-block write operations.
        m_SDCardInfo.SDIOInformation.Flags = SFTBLK_USE_FOR_CMD53_WRITE;
        DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: The host controller needs to use Soft-Block for CMD53 multi-block write operations. \n")));
    }
    // set the card interface


    if (Device_SD_IO == m_DeviceType) {
        CSDDevice * pDevice0 = m_sdSlot.GetFunctionDevice(0);
        status = SD_API_STATUS_DEVICE_REMOVED;

        if (pDevice0) {
            // read the bus control register to keep its current bits
            status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_BUS_CONTROL,FALSE,&regValue,1);

            if (SD_API_SUCCESS(status)) {
                // write the bus control register to set for 4 bit mode
                if (pInterfaceToUse->InterfaceModeEx.bit.sd4Bit) {
                    regValue |= SD_IO_BUS_CONTROL_BUS_WIDTH_4BIT;
                    status = pDevice0->SDReadWriteRegistersDirect_I( SD_IO_WRITE, SD_IO_REG_BUS_CONTROL,FALSE,&regValue,1);

                    if (SD_API_SUCCESS(status)) {
                        if (m_sdSlot.Capabilities & SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK) {

                            // get the card capabilities register
                            status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_READ,SD_IO_REG_CARD_CAPABILITY,FALSE,&regValue,1);

                            if (SD_API_SUCCESS(status)) {
                                // check the bit
                                if (regValue & SD_IO_CARD_CAP_SUPPORTS_INTS_4_BIT_MB_MODE) {
                                    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Host and Card supports interrupts in 4bit Multi-block mode\n")));

                                    // set the bit, it's in the same register
                                    regValue |= SD_IO_CARD_CAP_ENABLE_INTS_4_BIT_MB_MODE;
                                    // write out the card capabilities register
                                    status = pDevice0->SDReadWriteRegistersDirect_I( SD_IO_WRITE,SD_IO_REG_CARD_CAPABILITY,FALSE,&regValue,1);

                                    DEBUGMSG(SDBUS_ZONE_DEVICE && SD_API_SUCCESS(status),
                                        (TEXT("SDBusDriver: 4 Bit multi-block interrupts capability enabled \n")));
                                }
                            }
                        }
                    }
                }
                else {
                    regValue &= ~SD_IO_BUS_CONTROL_BUS_WIDTH_4BIT;
                    status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_WRITE,SD_IO_REG_BUS_CONTROL,FALSE,&regValue,1);
                    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDBusDriver: Failed to set I/O Card Bus Width \n")));

                }
            }
            pDevice0->DeRef();
        }
    }
    else if (Device_SD_Memory == m_DeviceType || Device_SD_Combo == m_DeviceType ) {

        // bus width commands are only allowed if the card is unlocked
        if (!m_SDCardInfo.SDMMCInformation.CardIsLocked) {

            // send the SET_BUS_WIDTH command to the device if 4 bit mode is used
            if (pInterfaceToUse->InterfaceModeEx.bit.sd4Bit) {
                // send command
                status = SendSDAppCommand(SD_ACMD_SET_BUS_WIDTH, SD_ACMD_ARG_SET_BUS_4BIT, ResponseR1, &response);
                DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDBusDriver: Failed to set Memory Card Bus Width in Slot \n")));
            } else {

                // send command
                status = SendSDAppCommand(SD_ACMD_SET_BUS_WIDTH,0x00,ResponseR1,&response);
                DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (TEXT("SDBusDriver: Failed to set Memory Card Bus Width in Slot \n")));
            }

        }

    }
    else if (Device_MMC == m_DeviceType) {
        UCHAR const*const pCSD  = m_CachedRegisters.CSD;
        UCHAR uSpecVer = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_SPECVER_BIT_SLICE,  SD_CSD_SPECVER_SLICE_SIZE);
        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("CSDDevice::SetCardInterface: SpecVersion = %d"), uSpecVer));

        //set 4/8 bit mode if neccessary
        if (uSpecVer >= MMC_SPEC_VERSION_4_PLUS) {
            DWORD dwBitValue = (pInterfaceToUse->InterfaceModeEx.bit.mmc8Bit!=0? 2:
                (pInterfaceToUse->InterfaceModeEx.bit.sd4Bit!=0?1:0));
            status = MMCSwitch(3, // Write Byte
                            0xb7, // Offset to  BUS_WIDTH
                            dwBitValue, // BUS_WIDTH value.
                            0);
        }
        else {
            pInterfaceToUse->InterfaceModeEx.bit.mmc8Bit = 0;
            pInterfaceToUse->InterfaceModeEx.bit.sd4Bit = 0;
        }
    }
    else {
        //DEBUGCHK(FALSE);
    }
    if (SD_API_SUCCESS(status) && (m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed != pInterfaceToUse->InterfaceModeEx.bit.sdHighSpeed) &&
            pInterfaceToUse->InterfaceModeEx.bit.sdHighSpeed!=0) { // We need switch to Hight speed.
        if ((Device_SD_Memory == m_DeviceType) && ((m_sdSlot.Capabilities &SD_SLOT_HIGH_SPEED_CAPABLE)!=0)) {
            // switch SD card to high speed.
            SD_CARD_SWITCH_FUNCTION switchData = {
                0x00000001, // Group 1 set to function 1 High Speed Table 4.7, SD Spec 2.0
                MAXDWORD,
                2*1000,     // let use try 2 second maximun.
            };
            status = SwitchFunction(&switchData,FALSE);
            if (SD_API_SUCCESS(status)) {
                m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed = 1;
                m_CurSpeedMode = HighSpeed;
            }
            else {
                m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed = 0;
            }

        }
        else if ((Device_MMC == m_DeviceType) && ((m_sdSlot.Capabilities &SD_SLOT_HIGH_SPEED_CAPABLE)!=0)) {
            UCHAR const*const pCSD  = m_CachedRegisters.CSD;
            UCHAR uSpecVer = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_SPECVER_BIT_SLICE,  SD_CSD_SPECVER_SLICE_SIZE);
            //switch MMC card to high speed mode. it is supported only on 4.0+ version MMC cards
            status = SD_API_STATUS_DEVICE_UNSUPPORTED;
            if (uSpecVer >= MMC_SPEC_VERSION_4_PLUS) {
                status = MMCSwitch(3, // Write Byte
                                0xb9, // Offset to  HS_TIMING
                                1, // enable high speed.
                                0);
            }
            if (SD_API_SUCCESS(status)) {
                m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed = 1;
                m_CurSpeedMode = HighSpeed;
            }
            else {
                m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed = 0;
            }

        }
        else {
            status = SD_API_STATUS_DEVICE_UNSUPPORTED;
            m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed = 0;
        }
    }
    if (SD_API_SUCCESS(status)) {
        m_CardInterfaceEx = *pInterfaceToUse;
    }


    ASSERT(SD_API_SUCCESS(status));
    return status;

}


///////////////////////////////////////////////////////////////////////////////
//  SelectCardInterface - select card interface based on information from the card,
//                        the host controller.
//
//  Output: pDevice->CardInterface contains ideal interface for this function
//  Return: SD_API_STATUS code
//  Notes:  This function sets the card's default interface in the device structure
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SelectCardInterface()
{
    DWORD                   bitSlice;              // bit slice
    SD_API_STATUS           status;                // intermediate status

    status = SD_API_STATUS_SUCCESS;

    SD_PARSED_REGISTER_CSD CSDRegister =  {0};

    // for MMC and SD cards allocate storage for the parsed CSD
    if ( (Device_SD_Memory == m_DeviceType) || (Device_MMC == m_DeviceType)) {
        // get the parsed CSD registers
        status = SDCardInfoQuery_I(SD_INFO_REGISTER_CSD,&CSDRegister,sizeof(SD_PARSED_REGISTER_CSD));

        if (!SD_API_SUCCESS(status)) {
            return status;
        }
    }

    // Set default interface for the current device. SelectSlotInterface()

    // By default we use 4 bit bus width for MMC, we will determine in the later
    // stage on whether we can use 8 bit bus width
    m_CardInterfaceEx.InterfaceModeEx.bit.mmc8Bit = 0 ;
    if (Device_MMC != m_DeviceType) {
        m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = ((m_sdSlot.Capabilities & SD_SLOT_SD_4BIT_CAPABLE)!=0 ? 1 : 0);
        // deal with special cases
        // 1 bit SD memory + 4 bit SDIO
        // SD_SLOT_SD_1BIT_CAPABLE | SD_SLOT_SDIO_CAPABLE | SD_SLOT_SDIO_4BIT_CAPABLE
        if (Device_SD_IO == m_DeviceType && (m_sdSlot.Capabilities & SD_SLOT_SDIO_4BIT_CAPABLE)!=0) {
            m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 1 ;
        }

        // 1 bit SDIO + 4 bit SD Memory
        // SD_SLOT_SD_1BIT_CAPABLE | SD_SLOT_SDIO_CAPABLE | SD_SLOT_SDMEM_4BIT_CAPABLE
        if ((Device_SD_Memory == m_DeviceType  || Device_SD_Combo == m_DeviceType ) && (m_sdSlot.Capabilities & SD_SLOT_SDMEM_4BIT_CAPABLE)!=0) {
            m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 1;
        }

        // try all cards at full speed
        m_CardInterfaceEx.ClockRate = SD_FULL_SPEED_RATE;

    } else {
        m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit =
            (((m_sdSlot.Capabilities & SD_SLOT_SD_4BIT_CAPABLE)!=0 || (m_sdSlot.Capabilities & SD_SLOT_SDMEM_4BIT_CAPABLE)!=0) ?
            1 : 0);
        m_CardInterfaceEx.ClockRate = MMC_FULL_SPEED_RATE;

    }

    // select the actual interface speed and type based on the card type
    if (Device_SD_IO == m_DeviceType) {
        CSDDevice * pParentDevice = m_sdSlot.GetFunctionDevice( 0 );
        if (pParentDevice) {
            // check for a low speed device
            if (pParentDevice->m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability & SD_IO_CARD_CAP_LOW_SPEED) {
                // drop it down to low speed
                m_CardInterfaceEx.ClockRate = SD_LOW_SPEED_RATE;
                // check for 4 bit support at low speed
                if (!(pParentDevice->m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability & SD_IO_CARD_CAP_4_BIT_LOW_SPEED)) {
                    // drop to 1 bit mode if the low speed device doesn't support
                    // 4 bit operation
                    m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 0 ;
                }
            }
            pParentDevice->DeRef();
        }
        else {
            ASSERT(FALSE);
            status = SD_API_STATUS_UNSUCCESSFUL;
        }

    }
    else if (Device_SD_Memory == m_DeviceType || Device_SD_Combo == m_DeviceType) {

        // get the bus width bits from the SCR to check for 4 bit
        // all cards must minimally support 1 bit mode
        bitSlice = GetBitSlice(m_CachedRegisters.SCR, sizeof(m_CachedRegisters.SCR), SD_SCR_BUS_WIDTH_BIT_SLICE , SD_SCR_BUS_WIDTH_SLICE_SIZE);

        if (!(bitSlice & SD_SCR_BUS_WIDTH_4_BIT)) {
            // card only supports 1 bit mode
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: SD Memory Card only supports 1 bit mode! \n"),
                m_FuncionIndex));
            m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 0 ;
        }

    // SD cards have their Max transfer rate set to 25 Mhz, we do not need to check the CSD register

    } else if (Device_MMC == m_DeviceType) {

        // for MMC cards we need to check the Max Transfer Rate reported by the card
        // the data rate is in kbits/sec
        if ((CSDRegister.MaxDataTransferRate * 1000) < MMC_FULL_SPEED_RATE) {
                // set for a lower rate
            m_CardInterfaceEx.ClockRate = CSDRegister.MaxDataTransferRate * 1000;
        }

        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: MMC Card Max Transfer Rate: %d Hz \n"),
                m_CardInterfaceEx.ClockRate));
    } else {
        DEBUGCHK(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    if (SD_API_SUCCESS(status)) {
        //the card should in full speed class now
        m_CurSpeedMode = FullSpeed;

        // get any interface overrides via the registry
        GetInterfaceOverrides();
        // sanity check the overrides
        if (m_CardInterfaceEx.InterfaceModeEx.bit.mmc8Bit) {
            if (m_DeviceType!= Device_MMC || (m_sdSlot.Capabilities &SD_SLOT_MMC_8BIT_CAPABLE) == 0) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: override to 8 bit on non-8-bit capable host or not MMC card, not allowed! \n")));
                return SD_API_STATUS_INVALID_PARAMETER;
            }

        }
        if (m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit) {
            if ((m_sdSlot.Capabilities & (SD_SLOT_SD_4BIT_CAPABLE|SD_SLOT_SDMEM_4BIT_CAPABLE|SD_SLOT_SDIO_4BIT_CAPABLE))== 0) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: override to 4 bit on non-4-bit capable host, not allowed! \n")));
                return SD_API_STATUS_INVALID_PARAMETER;
            }
        }

            // get the write protect status
        if (Device_SD_Memory == m_DeviceType) {
            SD_CARD_INTERFACE sdCardInterface;
            status = m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(), SDHCDGetWriteProtectStatus, &sdCardInterface, sizeof(sdCardInterface));
            m_CardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected = (sdCardInterface.WriteProtected?1:0);
            if (m_CardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected) {
                DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Card in slot %d is write protected \n"),m_sdSlot.GetSlotIndex()));
            }

        }
    }
    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Interface for slot %d , Mode :%x, Clock:%d \n"),
        m_sdSlot.GetSlotIndex(),m_CardInterfaceEx.InterfaceModeEx.uInterfaceMode,m_CardInterfaceEx.ClockRate));
    ASSERT(SD_API_SUCCESS(status));
    return status;

}

///////////////////////////////////////////////////////////////////////////////
//  SwitchCardSpeedMode - switch the speed mode of the card
//  Input:  TargetSpeedMode - the desired speed mode
//  Output:
//  Return: SD_API_STATUS code
//  Notes:  This function puts the card to desired speed mode. currently this function only
//              supports switching to high speed mode
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SwitchCardSpeedMode(SD_DEVICE_SPEEDMODE TargetSpeedMode)
{
    SD_API_STATUS    status = SD_API_STATUS_SUCCESS;

    if(TargetSpeedMode == m_CurSpeedMode)
        return status; // no need to do anything

    if(HighSpeed ==TargetSpeedMode){

        if (Device_SD_Memory == m_DeviceType ) {
            // Switch card to high speed mode.
            SD_CARD_SWITCH_FUNCTION switchData = {
                0x00000001, // Group 1 set to function 1 High Speed Table 4.7, SD Spec 2.0
                MAXDWORD,
                2*1000,     // 2 second time out for this request.
            };
            status = SwitchFunction(&switchData,FALSE);
        }
        else if (Device_MMC == m_DeviceType ) {
            UCHAR const*const pCSD  = m_CachedRegisters.CSD;
            UCHAR uSpecVer = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_SPECVER_BIT_SLICE,  SD_CSD_SPECVER_SLICE_SIZE);

            //switch MMC card to high speed mode. it is supported
            // on 4.0+ versions
            if (uSpecVer >= MMC_SPEC_VERSION_4_PLUS) {
                status = MMCSwitch(3, // Write Byte
                                0xb9, // Offset to  HS_TIMING
                                1, // enable high speed.
                                0);
            }
            else {
                status = SD_API_STATUS_DEVICE_UNSUPPORTED;
            }
        }
        else {
            //will deal with SDIO and combo card later
            status = SD_API_STATUS_DEVICE_UNSUPPORTED;
        }
    }
    else {
        //will deal with other speed modes later
        status = SD_API_STATUS_DEVICE_UNSUPPORTED;
    }

    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SwitchCardSpeedMode:  return status is: 0x%x\n"), status));

    return status;

}

///////////////////////////////////////////////////////////////////////////////
//  GetUpdatedTranSpeedValue - Get updated SPEED_TRAN register value from card
//  Input:  Value
//  Output: Value - Value read out from TRAN_SPEED register
//  Return: BOOL indicates the operation succeeds or not
 //  Notes:  This function will update the CSD values to the cached data structure, and slice
//               out the SPEED_TRAN register value and return it. This function will deselect the card
//               and reselect the card(if needed) because CSD command can only be executed in
//               deselected state
///////////////////////////////////////////////////////////////////////////////
BOOL CSDDevice::GetUpdatedTranSpeedValue(UCHAR& Value)
{
    SD_COMMAND_RESPONSE  response;  // response

    // SDIO cards does not have CSD
    if (Device_SD_IO == m_DeviceType)
        return FALSE;

    //deselect the card first
    SD_API_STATUS deselectstatus = SendSDCommand( SD_CMD_SELECT_DESELECT_CARD, 0, NoResponse, NULL);

    // get the CSD
    SD_API_STATUS status = SendSDCommand(SD_CMD_SEND_CSD,
        ((DWORD)m_RelativeAddress) << 16,
        ResponseR2,
        &response);

    if (!SD_API_SUCCESS(status)){
        goto Done;
    }

    // update shadow registers
    UpdateCachedRegisterFromResponse(SD_INFO_REGISTER_CSD, &response);

    // Calculate transfer rate in kbit/s
    Value = (UCHAR)GET_BIT_SLICE_FROM_CSD(m_CachedRegisters.CSD, SD_CSD_TRANS_SPEED_BIT_SLICE,
        SD_CSD_TRANS_SPEED_SLICE_SIZE);

Done:

    //restore the card into selected mode if needed
    if(SD_API_SUCCESS(deselectstatus)){
       status = SendSDCommand( SD_CMD_SELECT_DESELECT_CARD,
                                            ((DWORD)m_RelativeAddress) << 16,
                                            ResponseR1b,
                                            &response);
    }

    DEBUGMSG(SDBUS_ZONE_DEVICE,
                    (TEXT("GetUpdatedTranSpeedValue: return status is: 0x%x, value is: 0x%x\n"),
                    status, Value));

    return (SD_API_SUCCESS(status));
}


///////////////////////////////////////////////////////////////////////////////
//  GetInterfaceOverrides - get card interface overrides from the user
//  Input:  pDevice - the device
//
//  Output:
//  Return:
//  Notes:  This function gets the overrides from the device path in the
//          registry.  This function only uses a path built from the card's
//          OEM, MANF or CARDIDs.  It will not search a class path. For multi-
//          function and combo devices, overrides detected in the MANF and CARDID
//          path ex:[HKLM\Drivers\SDCARD\ClientDrivers\Custom\MANF-xxxx-CARDID-xxxx]
//          will override the interface settings for the entire card (all functions).
//
///////////////////////////////////////////////////////////////////////////////
VOID CSDDevice::GetInterfaceOverrides()
{
    WCHAR regPath[MAX_KEY_PATH_LENGTH]; // regpath
    DWORD clockRate;                    // clockrate override
    DWORD interfaceMode;                // interface mode override

    // get the card registry path
    if (!SD_API_SUCCESS(GetCustomRegPath(regPath, _countof(regPath), TRUE))) {
        return;
    }

    CReg regDevice(HKEY_LOCAL_MACHINE, regPath);
    if (regDevice.IsOK()) {
        // check for clock rate override
        clockRate = regDevice.ValueDW(SDCARD_CLOCK_RATE_OVERRIDE, (DWORD)-1);
        if (clockRate != -1) {
            DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: RegPath: %s overrides clock rate to : %d\n"),
                 regPath,clockRate));
            m_CardInterfaceEx.ClockRate = clockRate;
        }

        // check for interface mode override
        interfaceMode = regDevice.ValueDW(SDCARD_INTERFACE_MODE_OVERRIDE, (DWORD)-1);
        if (interfaceMode != -1) {
            if (interfaceMode == SDCARD_INTERFACE_OVERRIDE_1BIT) {
                m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 0 ;
                DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: RegPath: %s overrides interface mode to 1 bit \n"),regPath));
            } else if (interfaceMode == SDCARD_INTERFACE_OVERRIDE_4BIT) {
                m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit = 1;
                DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: RegPath: %s overrides interface mode to 4 bit \n"),regPath));
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//  GetCustomRegPath - get a device's custom registry path
//  Input:  pDevice - the device instance
//          cchPath - number of characters in pPath
//          BasePath - if TRUE return only the card's base path not
//                     its function path (single or multifunction I/O cards)
//                     this parameter is ignored for memory cards
//  Output: pPath   - the path returned
//  Notes:
//      returns SD_API_STATUS
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::GetCustomRegPath(__out_ecount(cchPath) LPTSTR                 pPath,
                                             DWORD                  cchPath,
                                             BOOL                   BasePath)
{
    SD_PARSED_REGISTER_CID  ParsedRegisters;                // parsed registers
    SD_API_STATUS           status = SD_API_STATUS_SUCCESS; // intermediate status

    DEBUGCHK(pPath);

    // for Memory or MMC get the CID
    if ( (Device_SD_Memory == m_DeviceType) ||(Device_MMC == m_DeviceType) || (Device_SD_Combo==m_DeviceType) ) {
        // get the parsed CID registers
        status = SDCardInfoQuery_I( SD_INFO_REGISTER_CID,&ParsedRegisters, sizeof(ParsedRegisters));
#define TEMP_CHAR_BUFFER_SIZE 3
        if (SD_API_SUCCESS(status)) {
            TCHAR szOEMID0[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szOEMID1[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szProductName0[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szProductName1[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szProductName2[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szProductName3[TEMP_CHAR_BUFFER_SIZE];
            TCHAR szProductName4[TEMP_CHAR_BUFFER_SIZE];

            // the OEM application ID and product name should only contain 7 bit ASCII characters,
            // but some cards use other characters in the 127-255 range).  This causes a problem because
            // the registry path is made up of these characters, and should not contain any characters
            // outside of the 32-126 range.
            // the following code will check each of the characters, and replace them with two digit
            // hexadecimal string representation if necessary

            if( ParsedRegisters.OEMApplicationID[0] >= 32 && ParsedRegisters.OEMApplicationID[0] < 127 )
                StringCchPrintf( szOEMID0, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.OEMApplicationID[0] );
            else
                StringCchPrintf( szOEMID0, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.OEMApplicationID[0] );

            if( ParsedRegisters.OEMApplicationID[1] >= 32 && ParsedRegisters.OEMApplicationID[1] < 127 )
                StringCchPrintf( szOEMID1, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.OEMApplicationID[1] );
            else
                StringCchPrintf( szOEMID1, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.OEMApplicationID[1] );

            if( ParsedRegisters.ProductName[0] >= 32 && ParsedRegisters.ProductName[0] < 127 )
                StringCchPrintf( szProductName0, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.ProductName[0] );
            else
                StringCchPrintf( szProductName0, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.ProductName[0] );

            if( ParsedRegisters.ProductName[1] >= 32 && ParsedRegisters.ProductName[1] < 127 )
                StringCchPrintf( szProductName1, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.ProductName[1] );
            else
                StringCchPrintf( szProductName1, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.ProductName[1] );

            if( ParsedRegisters.ProductName[2] >= 32 && ParsedRegisters.ProductName[2] < 127 )
                StringCchPrintf( szProductName2, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.ProductName[2] );
            else
                StringCchPrintf( szProductName2, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.ProductName[2] );

            if( ParsedRegisters.ProductName[3] >= 32 && ParsedRegisters.ProductName[3] < 127 )
                StringCchPrintf( szProductName3, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.ProductName[3] );
            else
                StringCchPrintf( szProductName3, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.ProductName[3] );

            if( ParsedRegisters.ProductName[4] >= 32 && ParsedRegisters.ProductName[4] < 127 )
                StringCchPrintf( szProductName4, TEMP_CHAR_BUFFER_SIZE, TEXT("%c"), (TCHAR)ParsedRegisters.ProductName[4] );
            else
                StringCchPrintf( szProductName4, TEMP_CHAR_BUFFER_SIZE, TEXT("%02x"), (TCHAR)ParsedRegisters.ProductName[4] );

            // build up the string
            VERIFY(SUCCEEDED(StringCchPrintf(pPath, cchPath, TEXT("%s\\CID-%d-%s%s-%s%s%s%s%s"),
                SDCARD_CUSTOM_DEVICE_REG_PATH,
                ParsedRegisters.ManufacturerID,
                szOEMID0, szOEMID1,
                szProductName0, szProductName1, szProductName2, szProductName3, szProductName4 )));

            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: built custom MMC/SD path: %s \n"),pPath));
        }
    }
    else if (Device_SD_IO == m_DeviceType) {
        if (BasePath) {
            // build the custom registry path for the card as a whole
            VERIFY(SUCCEEDED(StringCchPrintf(pPath, cchPath, TEXT("%s\\MANF-%04X-CARDID-%04X"),
                SDCARD_CUSTOM_DEVICE_REG_PATH,
                m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CardID)));
        } else {
            // build the custom registry path for the SDIO card function
            VERIFY(SUCCEEDED(StringCchPrintf(pPath, cchPath, TEXT("%s\\MANF-%04X-CARDID-%04X-FUNC-%d"),
                SDCARD_CUSTOM_DEVICE_REG_PATH,
                m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CardID,
                m_SDCardInfo.SDIOInformation.Function)));
        }

        DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: built custom SDIO path: %s \n"),pPath));
    }
    else {
        DEBUG_ASSERT(FALSE);
        status = SD_API_STATUS_INVALID_PARAMETER;
    }

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDLoadDevice - load the associated device driver
//  Input:  pDevice - the device instance requiring it's device driver to be loaded
//  Output:
//  Notes:
//      returns SD_API_STATUS
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDLoadDevice()
{
    TCHAR                   loadPath[MAX_KEY_PATH_LENGTH];  // string buffer
    BOOLEAN                 deviceFound = FALSE;            // device found flag
    SD_API_STATUS           status;                         // intermediate status
    HKEY                    hKey;                           // the key

    // get the custom path for the card or function
    status = GetCustomRegPath(loadPath, _countof(loadPath), FALSE);

    if (!SD_API_SUCCESS(status)) {
        return status;
    }

    // attempt to open the path
    if ( (RegOpenKeyEx(HKEY_LOCAL_MACHINE, loadPath, 0, KEY_ALL_ACCESS,&hKey) == ERROR_SUCCESS) ) {
        RegCloseKey(hKey);
        deviceFound = TRUE;
    } else {
        // no CUSTOM driver path was found so set the default path
        if (Device_SD_Memory == m_DeviceType || Device_SD_Combo == m_DeviceType ) {
            // set the default load path for SD Memory Devices
            HRESULT hr = StringCchCopy(loadPath, _countof(loadPath),SDCARD_SDMEMORY_CLASS_REG_PATH);
            if ( SUCCEEDED(hr)) {
                if (IsHighCapacitySDMemory()) {
                    hr = StringCchCat(loadPath, _countof(loadPath),SDCARD_HIGH_CAPACITY_REG_PATH);
                }
            }
            if (SUCCEEDED(hr))
                deviceFound = TRUE;
        } else if (Device_MMC == m_DeviceType) {
            // set the default load path for MMC devices
            HRESULT hr = StringCchCopy(loadPath, _countof(loadPath), SDCARD_MMC_CLASS_REG_PATH);
            RETAILMSG(0, (TEXT("SDBusDriver: Device class is MMC.  Load path is %s\n"), loadPath));
            if ( SUCCEEDED(hr)) {
                if (IsHighCapacitySDMemory()) {
                    // assume eMMC, use eMMC registry path
                    loadPath[0] = TEXT('\0');
                    StringCchCopy(loadPath, _countof(loadPath), SDCARD_eMMC_CLASS_REG_PATH);
                    hr = StringCchCat(loadPath, _countof(loadPath),SDCARD_HIGH_CAPACITY_REG_PATH);
                    RETAILMSG(0, (TEXT("SDBusDriver: Device class is high-capacity eMMC.  Load path is %s\n"), loadPath));
                }
            }
            if (SUCCEEDED(hr))
                deviceFound = TRUE;
        } else if (Device_SD_IO == m_DeviceType) {
            if (SD_IO_NON_STANDARD_DEVICE_CODE == m_SDCardInfo.SDIOInformation.DeviceCode) {
                // can't support this device, it has the non-standard device code and the registry does not
                // contain the settings for the driver
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: SDIO Device class is of type UNKNOWN, no custom driver found at %s\n"), loadPath));
            } else {
                // put back in the default device code reg path
                VERIFY(SUCCEEDED(StringCchPrintf(loadPath, _countof(loadPath),
                    TEXT("%s\\%d"),SDCARD_SDIO_CLASS_REG_PATH,
                    m_SDCardInfo.SDIOInformation.DeviceCode)));
                deviceFound = TRUE;
            }
        }
    }
    ASSERT(deviceFound);
    CSDHostContainer * pHostContainer = CSDHostContainer::GetHostContainer();

    if (deviceFound && pHostContainer && m_pDriverFolder == NULL  ) {
        status = SD_API_STATUS_UNSUCCESSFUL;
        Lock();

        // the reason we pass a regini structure is to make client drivers
        // source code compatible with CE 3.0 where the card handle has to be
        // retrieved from the registry. ActivateDeviceEx does not create the
        // context key.
        // this little task sets up the context key in the active path
        HANDLE hDeviceHandle = GetDeviceHandle().hValue;
        REGINI riDevice = {
            DEVLOAD_CLIENTINFO_VALNAME,
                (PBYTE) &hDeviceHandle,
                sizeof(hDeviceHandle),
                DEVLOAD_CLIENTINFO_VALTYPE
        };
        m_pDriverFolder = new DeviceFolder(pHostContainer->GetSubBusNamePrefix(),loadPath,Internal,
            m_sdSlot.GetHost().GetIndex(),m_sdSlot.GetSlotIndex(),m_FuncionIndex,
            pHostContainer->GetDeviceHandle());
        if (m_pDriverFolder && m_pDriverFolder->Init() && m_pDriverFolder->AddInitReg(1, &riDevice) && pHostContainer->InsertChild(m_pDriverFolder)) {
            m_pDriverFolder->AddRef();
            m_sdSlot.m_SlotState = Ready;
/*
            // Testing Code
            if (m_DeviceType == Device_SD_Memory) {
                SD_CARD_SWITCH_FUNCTION sdSwitchData = {
                    0x00000001, // Group 1 set to function 1 High Speed Table 4.7, SD Spec 2.0
                    MAXDWORD,
                    INFINITE,
                };
                SD_API_STATUS status = SwitchFunction(&sdSwitchData,FALSE);
            }
            // Endof Test.
*/
            m_pDriverFolder->LoadDevice();
            if ( (m_DeviceType == Device_SD_IO) &&
                 m_SDCardInfo.SDIOInformation.fWUS ) {
                // This SDIO function supports wake up. Tell the host to
                // allow waking on SDIO interrupts if the client has a
                // registry entry enabling this. It is okay if the host fails
                // this call.
                DWORD fWakeOnSDIOInterrupts;
                if (!m_pDriverFolder->GetRegValue(SDCARD_WAKE_ON_SDIO_INTERRUPTS_VALNAME,(LPBYTE)&fWakeOnSDIOInterrupts,sizeof(fWakeOnSDIOInterrupts)))
                    fWakeOnSDIOInterrupts = 0 ;
                if (fWakeOnSDIOInterrupts) {
                    SD_API_STATUS statusHandler =
                        m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.m_dwSlotIndex, SDHCDWakeOnSDIOInterrupts,
                            &fWakeOnSDIOInterrupts, sizeof(fWakeOnSDIOInterrupts));

                    if (!SD_API_SUCCESS(statusHandler)) {
                        DEBUGMSG(SDCARD_ZONE_WARN, (_T("SD: Could not enable waking on SDIO interrupts\r\n")));
                    }
                }
            }
            if (IsDriverLoaded() && IsDriverEnabled())
                status = SD_API_STATUS_SUCCESS;
            else { // Driver loading failed. So, it mail Register Client callback. We need take it out becaose code is gone.
                m_pSlotEventCallBack = NULL;
            }

            m_pDriverFolder->DeRef();
        }
        else if (m_pDriverFolder) {
            delete m_pDriverFolder;
            m_pDriverFolder = NULL;
        }
        Unlock();

        if (!SD_API_SUCCESS(status)) {
            //m_sdSlot.m_SlotState = SlotInitFailed;
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to load driver with path: %s \n"), loadPath));
            return status;
        }

        return SD_API_STATUS_SUCCESS;
    }

    return SD_API_STATUS_NO_SUCH_DEVICE;

}


///////////////////////////////////////////////////////////////////////////////
//  SDUnloadDevice - unload the client device
//  Input:  pDevice - the device instance requiring it's device driver to be unloaded
//  Output:
//  Notes:
//      returns SD_API_STATUS
///////////////////////////////////////////////////////////////////////////////
BOOL CSDDevice::SDUnloadDevice()
{
    // notify the client
    BOOL fResult = FALSE;
    NotifyClient( SDCardEjected);
    CSDHostContainer * pHostContainer = CSDHostContainer::GetHostContainer();
    if (pHostContainer) {
        Lock();
        if (m_pDriverFolder ) {
            fResult = pHostContainer->RemoveChildByFolder(m_pDriverFolder);
            ASSERT(fResult);
            m_pDriverFolder = NULL;
        }
        Unlock();
    };
    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDUnloadDevice: Client Device deleted \n")));
    return fResult;
}
///////////////////////////////////////////////////////////////////////////////
//  RegisterClientDevice - register a client device
//  Input:  pDevice - the device being registered
//          pContext - device specific context
//          pInfo   - client registration information
//
//  Output: pDevice
//  Notes:
//      returns SD_API_STATUS
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::RegisterClient(HANDLE hCallbackHandle,PVOID  pContext,PSDCARD_CLIENT_REGISTRATION_INFO pInfo)
{
    PREFAST_DEBUGCHK(pInfo);
    SD_API_STATUS status  = SD_API_STATUS_SUCCESS ;
    if (m_hCallbackHandle) { // we already called. We can trash the the handle now
        status = SD_API_STATUS_ACCESS_VIOLATION ;
    }
    else  if (hCallbackHandle) {
        if (CeDriverGetDirectCaller() == GetCurrentProcessId()) { // We can use it directly.
            m_hCallbackHandle = hCallbackHandle;
            m_fIsHandleCopied = FALSE;
        }
        else {
            m_hCallbackHandle = CeDriverDuplicateCallerHandle(hCallbackHandle,
                0,FALSE,DUPLICATE_SAME_ACCESS);
            m_fIsHandleCopied = TRUE;
        }
        if (m_hCallbackHandle == NULL) {
            status = SD_API_STATUS_INVALID_HANDLE;
        }
    }
    else
        m_hCallbackHandle = NULL ;

    if (SD_API_SUCCESS(status)) {
    // save the registration information
        __try {
            VERIFY(SUCCEEDED(StringCchCopy(  m_ClientName , _countof(m_ClientName), pInfo->ClientName)));
            m_pDeviceContext = pContext;
            m_pSlotEventCallBack = pInfo->pSlotEventCallBack;
            m_ClientFlags = pInfo->ClientFlags;
        }
        __except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_ACCESS_VIOLATION;
        }
    }
    ASSERT(SD_API_SUCCESS(status));
    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SetupWakeupSupport - Look at CISTPL_FUNCE for the function to see if
//                       wakeup is supported
//  Input:  pDevice - the device
//
//  Return: TRUE if wakeup is supported
///////////////////////////////////////////////////////////////////////////////

void CSDDevice::SetupWakeupSupport()
{
    BOOL fRet = FALSE;
    BYTE rgbFunce[SD_CISTPLE_MAX_BODY_SIZE];
    PSD_CISTPL_FUNCE_FUNCTION pFunce = (PSD_CISTPL_FUNCE_FUNCTION) rgbFunce;
    DWORD cbTuple;

    SD_API_STATUS status = SDGetTuple_I(SD_CISTPL_FUNCE, NULL, &cbTuple, FALSE);

    if ( SD_API_SUCCESS(status) && (cbTuple <= sizeof(rgbFunce)) ) {
        status = SDGetTuple_I(SD_CISTPL_FUNCE, rgbFunce, &cbTuple, FALSE);
        if ( SD_API_SUCCESS(status) && (pFunce->bType == SD_CISTPL_FUNCE_FUNCTION_TYPE) ) {
            // Valid FUNCE tuple. Check the wake up support bit.
            if (pFunce->FN_WUS) {
                fRet = TRUE;
            }
        }
    }
    m_SDCardInfo.SDIOInformation.fWUS = fRet;

}

///////////////////////////////////////////////////////////////////////////////
//  SDGetSDIOPnpInformation - Get SDIO PnP information for an SDIO device function
//  Input:  pDevice - the device context
//          Parent - this is the parent device
//  Output:
//  Return: SD_API_STATUS code
//  Notes:
//         This function collects SDIO Pnp information for the device.
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDGetSDIOPnpInformation(CSDDevice& psdDevice0)
{

    SD_API_STATUS          status;           // intermediate status
    UCHAR                  regValue[4];      // most tuples are 4 bytes
    UCHAR                  CSA_CISBuffer[CIS_CSA_BYTES]; // CIS and CSA are in contiguous locations
    DWORD                  FBROffset;        // calculated FBR offset
    DWORD                  manFid;           // manufacturer ID
    ULONG                  length;           // buffer length
    BOOL                   found;            // CIS found flag
    PCHAR                  pVersionBuffer;   // tuple buffer for vers1 info

    status = SD_API_STATUS_SUCCESS;
    ASSERT(m_SDCardInfo.SDIOInformation.pFunctionInformation == NULL);
    ASSERT(m_SDCardInfo.SDIOInformation.pCommonInformation == NULL);

    // allocate the common information
    ASSERT(m_SDCardInfo.SDIOInformation.pCommonInformation == NULL);
    m_SDCardInfo.SDIOInformation.pCommonInformation = new SDIO_COMMON_INFORMATION;

    if (NULL == m_SDCardInfo.SDIOInformation.pCommonInformation) {
        return SD_API_STATUS_INSUFFICIENT_RESOURCES;
    }
    memset(m_SDCardInfo.SDIOInformation.pCommonInformation, 0, sizeof(SDIO_COMMON_INFORMATION));

    if ((Device_SD_IO == m_DeviceType) || (Device_SD_Combo == m_DeviceType)) {
        // for the parent device save the common information
        if (m_FuncionIndex == 0 ) { // Function zero is parent.


            // get the CCCR
            status = SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_CCCR,FALSE,regValue,1);

            if (!SD_API_SUCCESS(status)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to get CCCR \n")));
                return status;
            }

            m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev = regValue[0];

            // get the card capabilities register
            status = SDReadWriteRegistersDirect_I( SD_IO_READ, SD_IO_REG_CARD_CAPABILITY, FALSE, regValue,1);

            if (!SD_API_SUCCESS(status)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to get CARD CAPABILITY register \n")));
                return status;
            }

            m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability = regValue[0];

            // get the SD Spec rev
            status = SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_SPEC_REV,FALSE,regValue,1);

            if (!SD_API_SUCCESS(status)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to get SD Spec revision \n")));
                return status;
            }

            m_SDCardInfo.SDIOInformation.pCommonInformation->SDSpec = regValue[0];

            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: SDIO Card Function %d, CCCR 0x%02X , Card Caps: 0x%02X, Spec Ver: %d \n"),
                m_SDCardInfo.SDIOInformation.Function,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability,
                m_SDCardInfo.SDIOInformation.pCommonInformation->SDSpec));


            // get the CIS pointer
            // function 0 only has a CIS, no CSA
            status = SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_COMMON_CIS_POINTER,FALSE,CSA_CISBuffer,SD_IO_CIS_PTR_BYTES);
            if (!SD_API_SUCCESS(status)) {
                return status;
            }

            m_SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer = CSA_CISBuffer[CIS_OFFSET_BYTE_0] |
                (CSA_CISBuffer[CIS_OFFSET_BYTE_1] << 8) |
                (CSA_CISBuffer[CIS_OFFSET_BYTE_2] << 16);

#if 0
            // for Debugging a bad CIS only
            {
                UCHAR tupleBuffer[256];
                status = SDReadWriteRegistersDirect((SD_DEVICE_HANDLE)pDevice,
                    SD_IO_READ,
                    0,      // all from function 0
                    pDevice->SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer,
                    FALSE,
                    tupleBuffer,
                    256);
                if (SD_API_SUCCESS(status)) {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver:  Common CIS Pointer: 0x%08X \n"),
                        pDevice->SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer));
                    SDOutputBuffer(tupleBuffer, 256);
                }

            }

#endif
            length = SD_CISTPL_MANFID_BODY_SIZE;

            // for function 0 get the Manufacturer ID (this is not the same as the VER1 string)
            status = SDGetTuple_I(SD_CISTPL_MANFID, (PUCHAR)&manFid, &length,TRUE);

            if (!SD_API_SUCCESS(status)) {
                return status;
            }

            if (0 == length) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver:  Card is missing CISTPL_MANFID \n")));
                status = SD_API_STATUS_DEVICE_UNSUPPORTED;
                return status;
            }
            // set Manufacturer and CardID
            m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID = (USHORT)manFid;
            m_SDCardInfo.SDIOInformation.pCommonInformation->CardID = (USHORT)(manFid >> 16);

            // retrieve the ver_1 tuple to retrieve the manufacturer string
            length = 0;


            // query the size of the tuple from the common CIS
            // the VERS_1 tuple is a variable length tuple
            status = SDGetTuple_I(SD_CISTPL_VERS_1, NULL,&length, TRUE);

            if (!SD_API_SUCCESS(status)) {
                return status;
            }

            if (0 != length ) {
                found = TRUE;
            } else {
                // the VER_1 tuple is optional, so if we could not find it, we
                // allocate the string identifying it as unknown
                found = FALSE;
                length = UNKNOWN_PRODUCT_INFO_STRING_LENGTH;
            }
            // allocate the string (include NULL) even if no tuple
            m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation = new WCHAR[length+1];

            if (NULL == m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver:  Failed to allocate product information string \n")));
                return SD_API_STATUS_INSUFFICIENT_RESOURCES;
            }

            if (found) {

                pVersionBuffer = new CHAR [length + 1];

                if (NULL == pVersionBuffer) {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver:  Failed to allocate product information string \n")));
                    return SD_API_STATUS_INSUFFICIENT_RESOURCES;
                }

                // retrieve the tuple from the common CIS
                status = SDGetTuple_I( SD_CISTPL_VERS_1,(PUCHAR)pVersionBuffer,&length, TRUE);


                if (!SD_API_SUCCESS(status)) {
                    delete[] pVersionBuffer ;
                    return status;
                }

                // make sure the string is null terminated
                pVersionBuffer[length] = NULL;

                // bump past the binary version info,
                // and format the string to wide char
                FormatProductString(&pVersionBuffer[2],
                    m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation, length);
                // free the version buffer
                delete[] pVersionBuffer;

            } else {
                // form the product name based on the required MANF and CARDID instead of the
                // ver1 string
                VERIFY(SUCCEEDED(StringCchPrintf( m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation,
                    length+1,
                    TEXT("Manufacturer ID:0x%04X, Card ID:0x%04X"),
                    m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID,
                    m_SDCardInfo.SDIOInformation.pCommonInformation->CardID)));

            }

                // enable power control on the card if available
            m_SDCardInfo.SDIOInformation.pCommonInformation->fCardSupportsPowerControl = FALSE;
            m_SDCardInfo.SDIOInformation.pCommonInformation->fPowerControlEnabled = FALSE;

            UCHAR uCCCRRev = m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev & SDIO_CCCR_SPEC_REV_MASK;
            //based on the SDIO protocol, only setup power controle for if card's CCCR version is 1.1 or 1.2
            if(uCCCRRev == SDIO_CCCR_SPEC_REV_1_1 || uCCCRRev == SDIO_CCCR_SPEC_REV_1_2)  {
                status = SDReadWriteRegistersDirect_I(SD_IO_READ, SD_IO_REG_POWER_CONTROL,FALSE, regValue,1);

                if (SD_API_SUCCESS(status) &&
                        (*regValue & SD_IO_CARD_POWER_CONTROL_SUPPORT)) {
                    m_SDCardInfo.SDIOInformation.pCommonInformation->fCardSupportsPowerControl = TRUE;

                    if(m_sdSlot.GetSlotPowerControl()) {
                        *regValue |= SD_IO_CARD_POWER_CONTROL_ENABLE;
                        status = SDReadWriteRegistersDirect_I( SD_IO_WRITE,SD_IO_REG_POWER_CONTROL, FALSE,regValue, 1);
                        if (SD_API_SUCCESS(status)) {
                            DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Power Control Enabled for SDIO Card.\n")));
                            m_SDCardInfo.SDIOInformation.pCommonInformation->fPowerControlEnabled = TRUE;
                        }
                    }
                }
            }
            else if(uCCCRRev > SDIO_CCCR_SPEC_REV_1_2) {
                DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: WARNING: Unsupported CCCR revision 0x%x!.\n"), uCCCRRev));
            }
        }
        else {
            // Copy Common Info
            m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev = psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev;
            m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability = psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability;
            m_SDCardInfo.SDIOInformation.pCommonInformation->SDSpec = psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->SDSpec;

            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: SDIO Card Function %d, CCCR 0x%02X , Card Caps: 0x%02X, Spec Ver: %d \n"),
                m_SDCardInfo.SDIOInformation.Function,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev,
                m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability,
                m_SDCardInfo.SDIOInformation.pCommonInformation->SDSpec));

            m_SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer = NULL;
            m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID = psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->ManufacturerID;
            m_SDCardInfo.SDIOInformation.pCommonInformation->CardID = psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->CardID;
            m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation = NULL;

            // calculate the FBR offset
            FBROffset = SD_IO_FBR_1_OFFSET + (m_SDCardInfo.SDIOInformation.Function - 1) *
                SD_IO_FBR_LENGTH;

            // fetch the device code
            status = psdDevice0.SDReadWriteRegistersDirect_I(SD_IO_READ,FBROffset + SD_IO_FBR_DEVICE_CODE, FALSE,regValue,1);
            if (!SD_API_SUCCESS(status)) {
                return status;
            }

            // save the device code, 1.0 style
            m_SDCardInfo.SDIOInformation.DeviceCode = (regValue[0] & SDIO_DEV_CODE_MASK);

            // check to see if we are using the special device code extension token
            if (SDIO_DEV_CODE_USES_EXTENSION == m_SDCardInfo.SDIOInformation.DeviceCode) {

                UCHAR uCCCRRev = m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRRev & SDIO_CCCR_SPEC_REV_MASK;
                //based on the SDIO protocol, only do this if CCCR version is 1.1 or 1.2
                if(uCCCRRev == SDIO_CCCR_SPEC_REV_1_1 || uCCCRRev == SDIO_CCCR_SPEC_REV_1_2)  {

                    // fetch the device code extension
                    status = psdDevice0.SDReadWriteRegistersDirect_I(SD_IO_READ, FBROffset + SD_IO_FBR_DEVICE_CODE_EXT,
                        FALSE, regValue,1);

                    if (!SD_API_SUCCESS(status)) {
                        return status;
                    }

                    // now 0x10-0xFF are available as device code extensions
                    m_SDCardInfo.SDIOInformation.DeviceCode = regValue[0];

                }
                else if (uCCCRRev > SDIO_CCCR_SPEC_REV_1_2) {
                    DbgPrintZo(SDCARD_ZONE_WARN, (TEXT("SDBusDriver: WARNING: Unsupported CCCR revision 0x%x!.\n"), uCCCRRev));
                }
            }

            // get the CIS and CSA pointers, we do a multi-byte read here
            status = psdDevice0.SDReadWriteRegistersDirect_I(SD_IO_READ, FBROffset + SD_IO_FBR_CISP_BYTE_0, FALSE,
                CSA_CISBuffer, CIS_CSA_BYTES);
            if (!SD_API_SUCCESS(status)) {
                return status;
            }

            m_SDCardInfo.SDIOInformation.CISPointer = CSA_CISBuffer[CIS_OFFSET_BYTE_0] |
                (CSA_CISBuffer[CIS_OFFSET_BYTE_1] << 8) |
                (CSA_CISBuffer[CIS_OFFSET_BYTE_2] << 16);

            m_SDCardInfo.SDIOInformation.CSAPointer = CSA_CISBuffer[CSA_OFFSET_BYTE_0] |
                (CSA_CISBuffer[CSA_OFFSET_BYTE_1] << 8) |
                (CSA_CISBuffer[CSA_OFFSET_BYTE_2] << 16);

            // allocate a product information tuple and fill it will some sort of
            // friendly name string
            ASSERT(m_SDCardInfo.SDIOInformation.pFunctionInformation == NULL);
            m_SDCardInfo.SDIOInformation.pFunctionInformation = new WCHAR[UNKNOWN_PRODUCT_INFO_STRING_LENGTH + 1];

            if (NULL == m_SDCardInfo.SDIOInformation.pFunctionInformation) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver:  Failed to allocate product information string \n")));
                status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
                return status;
            }

            // get the power control infomation via the Function FUNCE Tuple
            status = SDGetFunctionPowerControlTuples();


            // form the product name using the device class
            VERIFY(SUCCEEDED(StringCchPrintf(m_SDCardInfo.SDIOInformation.pFunctionInformation,
                UNKNOWN_PRODUCT_INFO_STRING_LENGTH + 1,
                TEXT("Device of Class Type: %d"),
                m_SDCardInfo.SDIOInformation.DeviceCode)));


            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Device 0x%08X , Function String: %s \n"),
                this,m_SDCardInfo.SDIOInformation.pFunctionInformation));

            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Common Product String: %s \n"),
                psdDevice0.m_SDCardInfo.SDIOInformation.pCommonInformation->pProductInformation));
        }
    }
    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  FormatProductString - format a raw CISPTPL_VERS_1 data buffer into a
//                        displayable string
//  Input:  pAsciiString  - raw product string buffer retreived from VERS_1 tuple
//                    the tuple is stored as ASCII characters
//          pString - the wide char string to store the version 1 tuple
//
//  Output:
//  Return:
//  Notes:
//         This function converts the raw version tuple data buffer
//         into a wide char displayable string
//         All non-displayable characters and spaces are converted to
//         an underscore.
///////////////////////////////////////////////////////////////////////////////
VOID CSDDevice::FormatProductString(CHAR const*const pAsciiString, __out_ecount(cchString) PWCHAR pString, DWORD cchString) const
{
    ULONG  ii;  // loop variable

    for (ii = 0; ii < strlen(pAsciiString) && ii < cchString - 1; ii++) {
        // convert to Wide char
        pString[ii] = (WCHAR)pAsciiString[ii];

        // if the character is outside the range of displayable characters
        // then substitute with an under score
        if ((pAsciiString[ii] <= 0x20) || (pAsciiString[ii] > 0x7E)) {
            pString[ii] = TEXT('_');
        }
    }

    // terminate
    pString[ii] = NULL;
}
///////////////////////////////////////////////////////////////////////////////
//  SendSDAppCmd - send an SD App Command synchronously (sends CMD55 followed by command)
//  Input:  hDevice - device handle
//          Command - command to send
//          Argument - argument for command
//          TransferClass - transfer class
//          ResponseType - expected response
//          NumberOfBlocks - number of blocks
//          BlockSize  - number of blocks
//          pBlockBuffer - buffer size
//  Output:
//          pResponse - response buffer
//  Return: SD_API_STATUS code
//  Notes:
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SendSDAppCmd( UCHAR                Command,
                           DWORD                Argument,
                           SD_TRANSFER_CLASS    TransferClass,
                           SD_RESPONSE_TYPE     ResponseType,
                           PSD_COMMAND_RESPONSE pResponse,
                           DWORD                NumberOfBlocks,
                           ULONG                BlockSize,
                           PUCHAR               pBlockBuffer)
{
    SD_API_STATUS          status;          // intermediate status
    ULONG                  retryCount;      // retry count
    ULONG                  i;               // loop variable


    // send the APP Command primer (CMD 55)
    status = SendSDCommand(SD_CMD_APP_CMD,((DWORD)GetRelativeAddress()) << 16,ResponseR1, pResponse);

    if (!SD_API_SUCCESS(status)){
        return status;
    }

    // CMD55 did not timeout so send the command
    if ((m_ClientFlags & SD_CLIENT_HANDLES_RETRY)!=0 ) {
        status = SDSynchronousBusRequest_I( Command,
            Argument,
            TransferClass,
            ResponseType,
            pResponse,
            NumberOfBlocks,
            BlockSize,
            pBlockBuffer,
            SD_SLOTRESET_REQUEST);
    }
    else {

        // save original ClientFlags
        DWORD clientFlags = m_ClientFlags;
        m_ClientFlags |= SD_CLIENT_HANDLES_RETRY;

        status = SDSynchronousBusRequest_I( Command,
            Argument,
            TransferClass,
            ResponseType,
            pResponse,
            NumberOfBlocks,
            BlockSize,
            pBlockBuffer,
            SD_SLOTRESET_REQUEST);

        if (!SD_API_SUCCESS(status)) {
            retryCount = CSDHostContainer::GetRetryCount();

            for (i = 0; i < retryCount; i++) {
                // check for retry
                if ( !( (SD_API_STATUS_RESPONSE_TIMEOUT == status) ||
                        (SD_API_STATUS_DATA_TIMEOUT == status) ||
                        (SD_API_STATUS_CRC_ERROR == status) ) ) {
                    break;
                }

                // send the APP Command primer (CMD55)
                status = SendSDCommand(SD_CMD_APP_CMD,((DWORD)GetRelativeAddress()) << 16, ResponseR1, pResponse);

                if (!SD_API_SUCCESS(status)){
                     break;
                }

                // send the command again
                status = SDSynchronousBusRequest_I( Command,
                    Argument,
                    TransferClass,
                    ResponseType,
                    pResponse,
                    NumberOfBlocks,
                    BlockSize,
                    pBlockBuffer,
                    SD_SLOTRESET_REQUEST);

            }
        }

        // restore client flags
        m_ClientFlags = clientFlags;
    }

    return status;
}


void CSDDevice::CopyContentFromParent(CSDDevice& psdDevice0)
{
    m_RelativeAddress = psdDevice0.m_RelativeAddress;
    m_OperatingVoltage = psdDevice0.m_OperatingVoltage;
    m_CachedRegisters = psdDevice0.m_CachedRegisters;

}
VOID CSDDevice::NotifyClient(SD_SLOT_EVENT_TYPE Event)
{
    PVOID pEventData = NULL;    // event data (defaults to NULL)
    DWORD eventDataLength = 0;  // event data length

    switch (Event) {
        case SDCardEjected :
        case SDCardBeginSelectDeselect:
        case SDCardDeselected:
        case SDCardSelected:
        case SDCardDeselectRequest:
        case SDCardSelectRequest:

            break;
        default:
            DEBUGCHK(FALSE);
    }
    Lock();
        // make sure this callback can be called
    if (NULL != m_pDriverFolder ) {
        __try {
            if (NULL != m_hCallbackHandle && m_pSlotEventCallBack!=NULL ) {
                IO_BUS_SD_SLOT_EVENT_CALLBACK busSdSlogEventCallback = {
                    m_pSlotEventCallBack,(SD_DEVICE_HANDLE)GetDeviceHandle().hValue,
                    m_pDeviceContext,
                    Event,  0, 0  };
                CeDriverPerformCallback(m_hCallbackHandle,IOCTL_BUS_SD_SLOT_EVENT_CALLBACK,&busSdSlogEventCallback,sizeof(busSdSlogEventCallback),
                    NULL,0,NULL,NULL);
            }
            else if (NULL != m_pSlotEventCallBack ) {
                m_pSlotEventCallBack ((SD_DEVICE_HANDLE)GetDeviceHandle().hValue,
                    m_pDeviceContext, Event, pEventData, eventDataLength);
            }

        } __except (SDProcessException(GetExceptionInformation())) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: slot event callback resulted in an exception \n")));
        }
    }
    Unlock();

}
SD_API_STATUS CSDDevice::SDReadWriteRegistersDirect_I(SD_IO_TRANSFER_TYPE ReadWrite, DWORD Address,
        BOOLEAN ReadAfterWrite,PUCHAR pBuffer,ULONG BufferLength)
{
    SD_API_STATUS   status = SD_API_STATUS_UNSUCCESSFUL;
    HANDLE  hSynchEvent = NULL;
    int iEventIndex = 0;

    if(pBuffer && BufferLength){
        hSynchEvent = TryGetSyncEventHandle(iEventIndex);
        if(hSynchEvent == NULL)
            hSynchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    if (hSynchEvent) {
        LockForChildRequests();
        for (DWORD dwCount = 0; dwCount < BufferLength ; dwCount ++ ) {
            DWORD dwArg = 0;
            BOOL fArgSuccess = FALSE;
            __try {
                dwArg = BUILD_IO_RW_DIRECT_ARG((UCHAR)ReadWrite,ReadAfterWrite,m_FuncionIndex,(Address + dwCount),ReadWrite==SD_IO_READ?0: pBuffer[dwCount]);
                fArgSuccess = TRUE;

            }
            __except(SDProcessException(GetExceptionInformation())) {
                fArgSuccess = FALSE;
            }
            if (!fArgSuccess) {
                status = SD_API_STATUS_ACCESS_VIOLATION;
                break;
            }
            SD_BUS_REQUEST sdRequest = {
                {NULL},(SD_DEVICE_HANDLE)GetDeviceHandle().hValue,0,
                SD_COMMAND,SD_IO_RW_DIRECT,dwArg ,
                {ResponseR5,{0}},
                NULL,
                SD_API_STATUS_UNSUCCESSFUL,
                0,0,0,
                NULL,NULL,
                0
            };

            if ((m_SDCardInfo.SDIOInformation.Flags & FSTPTH_DISABLE ) == 0 ) {
                sdRequest.Flags |= SD_SYNCHRONOUS_REQUEST ;
                sdRequest.SystemFlags |= SD_FAST_PATH_AVAILABLE;
            }
            sdRequest.pCallback = SDSyncRequestCallback ;
            sdRequest.RequestParam = (DWORD)hSynchEvent;
            ResetEvent(hSynchEvent);
            CSDBusRequest * pNewRequest =  new CSDBusRequest(*this, sdRequest );
            if (pNewRequest && pNewRequest->Init() ) {
                pNewRequest->AddRef();
                CSDBusRequest * pCur = pNewRequest;
                while (pCur) {
                    status = m_sdSlot.QueueBusRequest(pNewRequest);
                    if (!SD_API_SUCCESS(status)) {
                        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("SDReadWriteRegistersDirect_I: queue request failed(0x%x),TransferClass(%x), CommandCode(%x),CommandArgument(%x)\r\n"),
                            status, pCur->TransferClass, pCur->CommandCode,pCur->CommandArgument));
                        break;
                    }
                    else {
                        pCur = pCur->GetChildListNext();
                    }
                }
                while (pCur) { // This failed. So we complete the rest of them.
                    pCur->CompleteBusRequest(status );
                    pCur = pCur->GetChildListNext();
                }
                // wait for the I/O to complete
                if (!pNewRequest->IsComplete()) {
                    VERIFY(WAIT_OBJECT_0 == WaitForSingleObject(hSynchEvent, INFINITE));
                }
                ASSERT(pNewRequest->IsComplete());
                sdRequest = *(SD_BUS_REQUEST *)pNewRequest;
                status = pNewRequest->GetFirstFailedStatus();
                pNewRequest->DeRef();
            }
            else if (pNewRequest){
                delete pNewRequest;
            }

            if (SD_API_SUCCESS(status)) {
                UCHAR responseStatus = SD_GET_IO_RW_DIRECT_RESPONSE_FLAGS(&sdRequest.CommandResponse);
                // mask out the previous CRC error, the command is retried on CRC errors
                // so this bit will reflect the previous command instead
                responseStatus &= ~SD_IO_COM_CRC_ERROR;

                if (!SD_IO_R5_RESPONSE_ERROR(responseStatus)) {
                    status = SD_API_STATUS_SUCCESS;
                    // check to see if read after write or just a read
                    if (ReadAfterWrite || (SD_IO_READ == ReadWrite)) {
                        pBuffer[dwCount] = SD_GET_IO_RW_DIRECT_DATA(&sdRequest.CommandResponse);
                    }

                } else {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDCard: SDReadWriteRegistersDirect- R5 response returned error code : 0x%02X \n"),responseStatus));
                    status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
                    break;
                }
            }
            else
                break;
        };
        UnlockForChildRequests();
    }
    ASSERT(SD_API_SUCCESS(status));
    if(hSynchEvent){
        if(iEventIndex != -1)
            ReturnSyncEventHandle(iEventIndex);
        else
            CloseHandle(hSynchEvent);
        hSynchEvent = NULL;
    }

    return (status);
};
SD_API_STATUS CSDDevice::SDSynchronousBusRequest_I(UCHAR Command, DWORD Argument,SD_TRANSFER_CLASS TransferClass,
        SD_RESPONSE_TYPE ResponseType,PSD_COMMAND_RESPONSE  pResponse,ULONG NumBlocks,ULONG BlockSize,PUCHAR pBuffer, DWORD  Flags,DWORD cbSize, PPHYS_BUFF_LIST pPhysBuffList )
{
    SD_API_STATUS   status = SD_API_STATUS_UNSUCCESSFUL;
    HANDLE  hSynchEvent = NULL;
    int iEventIndex = 0;

    hSynchEvent = TryGetSyncEventHandle(iEventIndex);
    if(hSynchEvent == NULL)
        hSynchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(hSynchEvent == NULL)
        return (status);

    SD_BUS_REQUEST sdRequest = {
        {NULL},GetDeviceHandle().hValue,0,
        TransferClass,Command, Argument,
        {ResponseType,{0}},
        NULL,
        SD_API_STATUS_UNSUCCESSFUL,
        NumBlocks,BlockSize,0,
        pBuffer,NULL,
        0, // DATA Clock
        Flags,
        cbSize, pPhysBuffList
    };

    BOOL fFastPathSuitable = (Command == SD_COMMAND || (NumBlocks*BlockSize)<= m_sdSlot.m_FastPathThreshHold );

    if (fFastPathSuitable && (m_SDCardInfo.SDIOInformation.Flags & FSTPTH_DISABLE ) == 0  ) {
        sdRequest.Flags |= SD_SYNCHRONOUS_REQUEST ;
        sdRequest.SystemFlags |= SD_FAST_PATH_AVAILABLE;
    }
    sdRequest.pCallback = SDSyncRequestCallback ;
    sdRequest.RequestParam = (DWORD)hSynchEvent;
    ResetEvent(hSynchEvent);
    CSDBusRequest * pNewRequest =  new CSDBusRequest(*this, sdRequest );
    if (pNewRequest && pNewRequest->Init() ) {
        pNewRequest->AddRef();
        CSDBusRequest * pCur = pNewRequest;

        LockForChildRequests();
        while (pCur) {
            status = m_sdSlot.QueueBusRequest(pCur);
            if (!SD_API_SUCCESS(status)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (_T("SDSynchronousBusRequest_I: queue request failed(0x%x),TransferClass(%x), CommandCode(%x),CommandArgument(%x)\r\n"),
                    status, pCur->TransferClass, pCur->CommandCode,pCur->CommandArgument));
                break;
            }
            else {
                pCur = pCur->GetChildListNext();
            }
        }
        while (pCur) { // This failed. So we complete the rest of them.
            pCur->CompleteBusRequest(status );
            pCur = pCur->GetChildListNext();
        }
        UnlockForChildRequests();

        // wait for the I/O to complete
        if (!pNewRequest->IsComplete()) {
            VERIFY(WAIT_OBJECT_0 == WaitForSingleObject(hSynchEvent, INFINITE));
        }
        ASSERT(pNewRequest->IsComplete());
        sdRequest = *(SD_BUS_REQUEST *)pNewRequest;
        if (SD_API_SUCCESS(status)) { // If Request Submit Correct and Get Answered. We should get STatus.
            status = sdRequest.Status;
        }
        pNewRequest->TerminateLink();
        pNewRequest->DeRef();
    }
    else if (pNewRequest)
        delete pNewRequest;

    if (SD_API_SUCCESS(status) && NULL != pResponse) {
        __try {
            *pResponse = sdRequest.CommandResponse ;
        } __except (SDProcessException(GetExceptionInformation())) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("SDCard: Access violation while copying command response to user buffer\r\n")));
            status = SD_API_STATUS_ACCESS_VIOLATION;
        };

    }

    if(hSynchEvent){
        if(iEventIndex != -1)
            ReturnSyncEventHandle(iEventIndex);
        else
            CloseHandle(hSynchEvent);
        hSynchEvent = NULL;
    }

    return (status);
}
SD_API_STATUS CSDDevice::SDBusRequest_I(UCHAR Command,DWORD Argument,SD_TRANSFER_CLASS TransferClass, SD_RESPONSE_TYPE ResponseType,
        ULONG NumBlocks,ULONG BlockSize, PUCHAR pBuffer, PSD_BUS_REQUEST_CALLBACK pCallback, DWORD RequestParam,
        HANDLE *phRequest,DWORD Flags,DWORD cbSize, PPHYS_BUFF_LIST pPhysBuffList )
{
    SD_API_STATUS   status = SD_API_STATUS_UNSUCCESSFUL;

    SD_BUS_REQUEST sdRequest = {
        {NULL},GetDeviceHandle().hValue,0,
        TransferClass,Command, Argument,
        {ResponseType,{0}},
        RequestParam,
        SD_API_STATUS_UNSUCCESSFUL,
        NumBlocks,BlockSize,0,
        pBuffer,pCallback,
        0, // DATA Clock
        Flags,
        cbSize,
        pPhysBuffList
    };

    CSDBusRequest * pNewRequest =  new CSDBusRequest(*this, sdRequest, m_hCallbackHandle );// This is from external
    if (phRequest && pNewRequest && pNewRequest->Init() ) {
        DWORD dwIndex = 0;
        pNewRequest->AddRef();
        if (InsertRequestAtEmpty(&dwIndex,pNewRequest )) {
            // After this point the status should return during the completion.
            SDBUS_REQUEST_HANDLE sdBusRequestHandle;
            sdBusRequestHandle.bit.sdBusIndex = m_sdSlot.GetHost().GetIndex();
            sdBusRequestHandle.bit.sdSlotIndex = m_sdSlot.GetSlotIndex();
            sdBusRequestHandle.bit.sdFunctionIndex = m_FuncionIndex;
            sdBusRequestHandle.bit.sdRequestIndex = dwIndex;
            sdBusRequestHandle.bit.sd1f = 0x1f;
            sdBusRequestHandle.bit.sdRandamNumber = pNewRequest->GetRequestRandomIndex();

            status = SD_API_STATUS_PENDING ;
            __try {
                *phRequest = sdBusRequestHandle.hValue;
            }
            __except (SDProcessException(GetExceptionInformation())) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (_T("SDCard: Access violation while copying command response to user buffer\r\n")));
                status = SD_API_STATUS_ACCESS_VIOLATION;
            }
            if (SD_API_SUCCESS(status)) {
                pNewRequest->SetExternalHandle(sdBusRequestHandle.hValue);
                CSDBusRequest * pCur = pNewRequest;
                LockForChildRequests();
                while (pCur) {
                    status = m_sdSlot.QueueBusRequest(pCur);
                    if (!SD_API_SUCCESS(status)) {
                        ASSERT(FALSE);
                        break;
                    }
                    else {
                        pCur = pCur->GetChildListNext();
                    }
                }
                while (pCur) { // This failed. So we complete the rest of them.
                    pCur->CompleteBusRequest(status );
                    pCur = pCur->GetChildListNext();
                }
                UnlockForChildRequests();
                status = SD_API_STATUS_PENDING; // Real status should be return from callback from this point.
            }
            else {
                ASSERT(FALSE);
                SDFreeBusRequest_I(sdBusRequestHandle.hValue);
            }
        }
        else
            status = SD_API_STATUS_NO_MEMORY;
        pNewRequest->DeRef();
        pNewRequest = NULL;

    }
    else {
        status = SD_API_STATUS_NO_MEMORY;
        if (pNewRequest ) {
            delete pNewRequest;
        }
    }

    ASSERT(SD_API_SUCCESS(status));
    return (status);


}
VOID CSDDevice::SDFreeBusRequest_I(HANDLE hRequest)
{
    SDBUS_REQUEST_HANDLE sdBusRequestHandle;
    sdBusRequestHandle.hValue = hRequest;
    BOOL fFreedRequest = FALSE;
    if (hRequest && sdBusRequestHandle.bit.sd1f == 0x1f) {
        CSDBusRequest * pCurRequest = ObjectIndex(sdBusRequestHandle.bit.sdRequestIndex);
        if (pCurRequest ) {
            if (pCurRequest->GetRequestRandomIndex() == sdBusRequestHandle.bit.sdRandamNumber) { // this is right requerst.
                CSDBusRequest * pCurRequest2 = pCurRequest ;
                while (pCurRequest2 ) {
                    if (!pCurRequest2->IsComplete()) {
                        m_sdSlot.RemoveRequest(pCurRequest2);
                    }
                    pCurRequest2 = pCurRequest2->GetChildListNext();
                }
                fFreedRequest = TRUE;
            }
            else
                ASSERT(FALSE);
            VERIFY(RemoveObjectBy(sdBusRequestHandle.bit.sdRequestIndex ));
            pCurRequest->TerminateLink();
            pCurRequest->DeRef();
        }
        else
            ASSERT(FALSE);
    }
    ASSERT(fFreedRequest);
    DEBUGMSG(SDCARD_ZONE_ERROR && !fFreedRequest, (_T("SDFreeBusRequest_I: Invalid request h=0x%x\r\n"),hRequest));
}
SD_API_STATUS CSDDevice::SDBusRequestResponse_I(HANDLE hRequest, PSD_COMMAND_RESPONSE pSdCmdResp)
{
    SDBUS_REQUEST_HANDLE sdBusRequestHandle;
    sdBusRequestHandle.hValue = hRequest;
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;  // intermediate status
    if (hRequest && sdBusRequestHandle.bit.sd1f == 0x1f) {
        CSDBusRequest * pCurRequest = ObjectIndex(sdBusRequestHandle.bit.sdRequestIndex);
        if (pCurRequest ) {
            if ( pSdCmdResp && pCurRequest->GetRequestRandomIndex() == sdBusRequestHandle.bit.sdRandamNumber) { // this is right requerst.
                __try {
                    *pSdCmdResp = pCurRequest->CommandResponse;
                    status = pCurRequest->Status;
                }__except(EXCEPTION_EXECUTE_HANDLER){
                    status = SD_API_STATUS_INVALID_PARAMETER;
                }
            }
            pCurRequest->DeRef();
        }
    }
    DEBUGMSG(SDCARD_ZONE_ERROR && !SD_API_SUCCESS(status), (_T("SDBusRequestResponse_I:  Invalid request h=0x%x\r\n"),hRequest));
    return status;
}
BOOL CSDDevice::SDCancelBusRequest_I(HANDLE hRequest)
{
    SDBUS_REQUEST_HANDLE sdBusRequestHandle;
    sdBusRequestHandle.hValue = hRequest;
    BOOL fCanceledRequest = FALSE;
    if (hRequest && sdBusRequestHandle.bit.sd1f == 0x1f) {
        CSDBusRequest * pCurRequest = ObjectIndex(sdBusRequestHandle.bit.sdRequestIndex);
        if (pCurRequest ) {
            if (pCurRequest->GetRequestRandomIndex() == sdBusRequestHandle.bit.sdRandamNumber) { // this is right requerst.
                CSDBusRequest * pCurRequest2 = pCurRequest ;
                while (pCurRequest2 ) {
                    if (!pCurRequest2->IsComplete()) {
                        m_sdSlot.RemoveRequest(pCurRequest2);
                    }
                    pCurRequest2 = pCurRequest2->GetChildListNext();
                }
                fCanceledRequest = TRUE;
            }
            pCurRequest->DeRef();
        }
    }
    ASSERT(fCanceledRequest);
    DEBUGMSG(SDCARD_ZONE_ERROR && !fCanceledRequest, (_T("SDCancelBusRequest_I:  Invalid request h=0x%x\r\n"),hRequest));
    return fCanceledRequest;
}


void CSDDevice::HandleDeviceInterrupt()
{
    SD_API_STATUS status = SD_API_STATUS_INVALID_HANDLE;
    if (m_SDCardInfo.SDIOInformation.pInterruptCallBack!=NULL) {
        DEBUGMSG(SDBUS_ZONE_DEVICE,(TEXT("HandleDeviceInterrupting - Calling interrupt handler for %s \n"),
            m_ClientName));
        if (m_SDCardInfo.SDIOInformation.pInterruptCallBack!=NULL) { // We need callback.
            __try {
                if (m_hCallbackHandle!=NULL) { // Callback between proc.
                    IO_BUS_SD_INTERRUPT_CALLBACK sdInterruptCallback = {
                        m_SDCardInfo.SDIOInformation.pInterruptCallBack, GetDeviceHandle().hValue,m_pDeviceContext
                    };
                    CeDriverPerformCallback(m_hCallbackHandle, IOCTL_BUS_SD_INTERRUPT_CALLBACK, &sdInterruptCallback, sizeof(sdInterruptCallback),
                        NULL,0,
                        NULL,NULL);
                    status = SD_API_STATUS_SUCCESS ;

                }
                else { // In-proc
                    status = m_SDCardInfo.SDIOInformation.pInterruptCallBack(GetDeviceHandle().hValue,m_pDeviceContext);
                }
            }
            __except(SDProcessException(GetExceptionInformation())) {
                status = SD_API_STATUS_ACCESS_VIOLATION ;
            }
        }
    }
    ASSERT(SD_API_SUCCESS(status));
}

SD_API_STATUS CSDDevice::SDIOConnectDisconnectInterrupt(PSD_INTERRUPT_CALLBACK   pIsrFunction, BOOL Connect)
{
    UCHAR                       regValue;           // intermediate register value
    SD_API_STATUS               status;             // intermediate status

    if (Device_SD_IO != m_DeviceType) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIOConnectDisconnectInterrupt: device is not SDIO ! \n")));
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    if ( Connect && (NULL == pIsrFunction) ) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDCard: SDIOConnectInterrupt: callback function is NULL \n")));
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    if (m_FuncionIndex == 0 ) { // Function zero.
        DEBUGCHK(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    CSDDevice * device0 = m_sdSlot.GetFunctionDevice(0 );
    if (device0 == NULL) {
        DEBUGCHK(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    Lock();
    if (Connect) {
        m_SDCardInfo.SDIOInformation.pInterruptCallBack = pIsrFunction;
        // update shadow register , we automatically enable the master interrupt enable bit
        device0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIntEnable |=
            ((1 << m_SDCardInfo.SDIOInformation.Function) | SD_IO_INT_ENABLE_MASTER_ENABLE);
    } else {
        DEBUGCHK(pIsrFunction == NULL);
        m_SDCardInfo.SDIOInformation.pInterruptCallBack = NULL;
        // mask out this function
        device0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIntEnable &=
            ~(1 << m_SDCardInfo.SDIOInformation.Function);
    }

    // get a copy
    regValue = device0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIntEnable;

    // check to see if there are interrupts to keep enabled
    if (!(regValue & SD_IO_INT_ENABLE_ALL_FUNCTIONS)) {
        // if none, then clear out master enable
        regValue &= ~SD_IO_INT_ENABLE_MASTER_ENABLE;
        device0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIntEnable = regValue;
    }

    // set status
    status = SD_API_STATUS_SUCCESS;

    // now check and see if we need to enable/disable interrupts in the host controller
    if (Connect) {
        m_sdSlot.SDSlotEnableSDIOInterrupts();
    } else {
        // check to see if all the interrupts have been turned off
        if (0 == regValue) {
            // disable interrupts, we don't really care about the result
            m_sdSlot.SDSlotDisableSDIOInterrupts();
        }
    }
    Unlock();

    // update the INT Enable register
    status = device0->SDReadWriteRegistersDirect_I(SD_IO_WRITE, SD_IO_REG_INT_ENABLE,FALSE,&regValue, 1);

    if (!SD_API_SUCCESS(status)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIOConnectDisconnectInterrupt: failed to write INT_ENABLE register for function %d \n"),
            m_SDCardInfo.SDIOInformation.Function));
        m_SDCardInfo.SDIOInformation.pInterruptCallBack = NULL;
    } else {
        if (Connect) {
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDIOConnectDisconnectInterrupt: Interrupt enabled for function %d \n"),
                m_SDCardInfo.SDIOInformation.Function));
        } else {
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDIOConnectDisconnectInterrupt: Interrupt disabled for function %d \n"),
                m_SDCardInfo.SDIOInformation.Function));
        }
    }
    device0->DeRef();

    return status;

}

SD_API_STATUS CSDDevice::SwitchFunction(PSD_CARD_SWITCH_FUNCTION pSwitchData, BOOL fReadOnly)
{

    SD_API_STATUS  status = SD_API_STATUS_DEVICE_UNSUPPORTED ;
    DWORD dwOutputFunctioGroup = 0xffffff;
    if (m_DeviceType == Device_SD_Memory && m_sdSlot.GetNumOfFunctionDevice()<=1 && pSwitchData!=NULL ) {
        DWORD dwInputFunctionGroup = pSwitchData->dwSelectedFunction & 0xffffff;
        DWORD dwSCRVersion = GetBitSlice(m_CachedRegisters.SCR, sizeof(m_CachedRegisters.SCR), SD_SCR_VERSION_BIT_SLICE , SD_SCR_VERSION_SLICE_SIZE);
        DWORD dwSDSpecVersion = 0 ;
        if (dwSCRVersion == 0 ) {
            dwSDSpecVersion = GetBitSlice(m_CachedRegisters.SCR, sizeof(m_CachedRegisters.SCR), SD_SCR_SD_SPEC_BIT_SLICE , SD_SCR_SD_SPEC_SLICE_SIZE);
        }
        if (dwSDSpecVersion >=1 ) { // Spec 1.1 and above.
            if (fReadOnly) {
                UCHAR SwitchStatus[512/8];
                dwOutputFunctioGroup = m_dwCurFunctionGroup ;
                SD_COMMAND_RESPONSE response;  // response
                status = SDSynchronousBusRequest_I(SD_CMD_SWITCH_FUNCTION, dwInputFunctionGroup, SD_READ,  ResponseR1,&response,
                    1,sizeof(SwitchStatus),SwitchStatus,SD_SLOTRESET_REQUEST);
                ASSERT(sizeof(SwitchStatus) == sizeof(pSwitchData->clientData));
                if (SD_API_SUCCESS(status)) {
                    SwapByte(SwitchStatus,sizeof(SwitchStatus));
                    dwOutputFunctioGroup = GetBitSlice(SwitchStatus,sizeof(SwitchStatus),376,4*6);
                    pSwitchData->dwMaxCurrentAllowed = GetBitSlice(SwitchStatus,sizeof(SwitchStatus),496,16);
                    memcpy(pSwitchData->clientData , SwitchStatus, sizeof(pSwitchData->clientData));
                }
            }
            else {
                m_sdSlot.m_RequestLock.Lock();
                DWORD dwStartTick = GetTickCount();
                BOOL fContinue = TRUE;
                status = SD_API_STATUS_DEVICE_BUSY;
                while (fContinue && GetTickCount()- dwStartTick  < pSwitchData->dwTimeOut ) {
                    // Send out CMD6 Mode 0 to read it.
                    UCHAR SwitchStatus[512/8];
                    SD_COMMAND_RESPONSE response;  // response
                    status = SDSynchronousBusRequest_I(SD_CMD_SWITCH_FUNCTION, dwInputFunctionGroup, SD_READ,  ResponseR1,&response,
                        1,sizeof(SwitchStatus),SwitchStatus,SD_SLOTRESET_REQUEST);
                    if (SD_API_SUCCESS(status)) {
                        SwapByte(SwitchStatus,sizeof(SwitchStatus));
                        dwOutputFunctioGroup = GetBitSlice(SwitchStatus,sizeof(SwitchStatus),376,4*6);
                        DWORD dwCurrentRequired = GetBitSlice(SwitchStatus,sizeof(SwitchStatus),496,16);
                        if (dwOutputFunctioGroup == dwInputFunctionGroup && dwCurrentRequired<= pSwitchData->dwMaxCurrentAllowed) { // it is ready to set.
                            status = SDSynchronousBusRequest_I(SD_CMD_SWITCH_FUNCTION, dwInputFunctionGroup|0x80000000, SD_READ,
                                ResponseR1,&response, 1,sizeof(SwitchStatus),SwitchStatus,SD_SLOTRESET_REQUEST);
                            if (SD_API_SUCCESS(status)) {
                                Sleep(1);
                                SwapByte(SwitchStatus,sizeof(SwitchStatus));
                                m_dwCurFunctionGroup = dwOutputFunctioGroup = GetBitSlice(SwitchStatus,sizeof(SwitchStatus),376,4*6);
                                if (dwOutputFunctioGroup == dwInputFunctionGroup) {
                                    ASSERT(sizeof(SwitchStatus) == sizeof(pSwitchData->clientData));
                                    memcpy(pSwitchData->clientData , SwitchStatus, sizeof(pSwitchData->clientData));
                                    status = SD_API_STATUS_SUCCESS;
                                    fContinue = FALSE;
                                }
                                else
                                    status = SD_API_STATUS_DEVICE_BUSY;
                            }
                            else{ // This is serious error. device now is in unknow state. we need reset to recover.
                                ASSERT(FALSE);
                                m_sdSlot.m_SlotState = SlotInitFailed;
                                m_dwCurFunctionGroup = 0 ; // Back to default.
                                fContinue = FALSE;
                                status = SD_API_STATUS_ACCESS_VIOLATION ;
                            }

                        }
                        else {
                            DWORD dwCheck = dwOutputFunctioGroup;
                            for (DWORD dwIndex=0; dwIndex<6; dwIndex++) {
                                if ((dwCheck & 0xf) == 0xf) { // unsported function.
                                    break;
                                }
                                else {
                                    dwCheck >>= 4;
                                }
                            }
                            if (dwIndex<6 || dwCurrentRequired>pSwitchData->dwMaxCurrentAllowed) { // failed.
                                status = SD_API_STATUS_DEVICE_UNSUPPORTED ;
                                fContinue = FALSE;
                            }
                        }
                    }
                    else
                        fContinue = FALSE;
                }
                m_sdSlot.m_RequestLock.Unlock();
            }

        }
    }
    if (SD_API_SUCCESS(status) && pSwitchData)
        pSwitchData->dwSelectedFunction = dwOutputFunctioGroup ;

    return status;
}

BOOL CSDDevice::MMCSwitchto8BitBusWidth()
{
    SD_API_STATUS  status = SD_API_STATUS_DEVICE_UNSUPPORTED;
    BOOL fSwitchto8Bit =  FALSE;

    //if host does not support 8bit mode, then return
    if((m_sdSlot.Capabilities & SD_SLOT_MMC_8BIT_CAPABLE) !=
                                SD_SLOT_MMC_8BIT_CAPABLE) {

        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("CSDDevice::MMCSwithto8BitBusWidth: Host does not support 8 bit mode")));
        return FALSE;
    }


    PUCHAR pCSD  = m_CachedRegisters.CSD;
    UCHAR uSpecVer = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_SPECVER_BIT_SLICE,  SD_CSD_SPECVER_SLICE_SIZE);

    //Spec version must be 4 or beyond to support bus width test mechanism
    if (uSpecVer < MMC_SPEC_VERSION_4_PLUS) {
        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("CSDDevice::MMCSwithto8BitBusWidth: SpecVersion = %d, can not perform bus detection"), uSpecVer));
        return FALSE;
    }

    SD_COMMAND_RESPONSE response;  // response
    BYTE TestBuffer[512];

    //let slot go into 8bit mode first
    m_CardInterfaceEx.InterfaceModeEx.bit.mmc8Bit = 1;
    status = m_sdSlot.SDSetCardInterfaceForSlot(&m_CardInterfaceEx);

#if MMC_8BIT_SEND_TEST_PATTERN  
    if(SD_API_SUCCESS(status)){
        memset(TestBuffer, 0, sizeof(TestBuffer));
        TestBuffer[0] = 0x55;
        TestBuffer[1] = 0xAA;

        // issue CMD 19 to write 4 bytes of test pattern (only the first two bytes matter)
        status = SDSynchronousBusRequest_I(SD_CMD_MMC_BUSTEST_W,
                                                                    0, SD_WRITE,
                                                                    ResponseR1, &response,
                                                                    1, sizeof(TestBuffer),
                                                                    TestBuffer, SD_SLOTRESET_REQUEST);
        if(!SD_API_SUCCESS(status) && status != SD_API_STATUS_CRC_ERROR){
            DEBUGMSG(SDCARD_ZONE_ERROR,
                (_T("Issue CMD19 for bustest write failed!status = 0x%x"), status));
            goto Done;
        }

        // issue CMD 14 to read 4 bytes of returned test pattern (only the first two bytes matter)
        TestBuffer[0] = TestBuffer[1] = 0;
        status = SDSynchronousBusRequest_I(SD_CMD_MMC_BUSTEST_R,
                                                                    0, SD_READ,
                                                                    ResponseR1, &response,
                                                                    1, sizeof(TestBuffer),
                                                                    TestBuffer, SD_SLOTRESET_REQUEST);
        if( SD_API_SUCCESS(status)  || status == SD_API_STATUS_CRC_ERROR) {
            // If successful, the read data will be the reverse of what was sent
           if(TestBuffer[0] == 0xAA && TestBuffer[1] == 0x55){
                //yes, it supports 8 bit bus width
                DEBUGMSG(SDCARD_ZONE_INFO, (_T("MMC Card: 8 bit bus width is supported!")));
                fSwitchto8Bit = TRUE;
            }
            else{
                DEBUGMSG(SDCARD_ZONE_INFO, (_T("MMC Card: 8bit bus width is not supported!")));
            }
        }
    }
    else{
        DEBUGMSG(SDCARD_ZONE_INFO, (_T("MMC Card: Can not set host controller to 8bit mode!")));
    }
#else

    // switch to 8 bit mode without sending test pattern
    fSwitchto8Bit = TRUE;
	TestBuffer[0] = 0x55;
    goto Done;

#endif

Done:

    //card does not support 8 bit, go back to 4 bit
    if(fSwitchto8Bit == FALSE){
        m_CardInterfaceEx.InterfaceModeEx.bit.mmc8Bit = 0;
        m_sdSlot.SDSetCardInterfaceForSlot(&m_CardInterfaceEx);
    }
    else {
        //set card to the right bus width
        status = MMCSwitch(3, // Write Byte
                        0xb7, // Offset to  BUS_WIDTH
                        2, // 8bit BUS_WIDTH value.
                        0);
        DEBUGMSG(SDCARD_ZONE_INFO, (_T("MMC card set bus width to 8bit, status = 0x%x"), status));

        //preivous action failed, try a simple recovery by deselecting and reselecting card
        if(!SD_API_SUCCESS(status)){
        //deselect the card
            SendSDCommand( SD_CMD_SELECT_DESELECT_CARD,0, ResponseR1b, &response);
            Sleep(200);
            //select the card
            SendSDCommand( SD_CMD_SELECT_DESELECT_CARD,((DWORD)m_RelativeAddress) << 16, ResponseR1b, &response);
            Sleep(200);

            //back to 4 bit mode
            m_CardInterfaceEx.InterfaceModeEx.bit.mmc8Bit = 0;
            m_sdSlot.SDSetCardInterfaceForSlot(&m_CardInterfaceEx);
            fSwitchto8Bit = FALSE;
        }
    }


    return fSwitchto8Bit;
}

SD_API_STATUS CSDDevice::MMCSwitch(DWORD dwAccessMode,DWORD dwAccessIndex, DWORD dwAccessValue, DWORD dwCmdSet)
{
    SD_API_STATUS  status = SD_API_STATUS_DEVICE_UNSUPPORTED ;
    if (m_DeviceType == Device_MMC && m_sdSlot.GetNumOfFunctionDevice()<=1 ) {
        DWORD dwArg = ((dwAccessMode & 3)<<24) | ((dwAccessIndex & 0xff)<<16) | ((dwAccessValue & 0xff)<<8) | (dwCmdSet&0x7);
        SD_COMMAND_RESPONSE response;  // response
        status = SDSynchronousBusRequest_I(SD_CMD_SWITCH_FUNCTION,dwArg , SD_COMMAND, ResponseR1b,&response,
            0,0,NULL,0);
    }
    ASSERT(SD_API_SUCCESS(status));
    if(SD_API_SUCCESS(status)){
        // Some cards need a little bit of extra time to settle down after the switch, especially after bus-width changes
        // Without this the next command 
        Sleep(1);
    }
    return (status);
}
void CSDDevice::SwapByte(__inout_bcount(dwLength) PBYTE pPtr, DWORD dwLength)
{
    ASSERT(pPtr!=NULL);
    ASSERT(dwLength!=0);
    if (pPtr!=NULL && dwLength) {
        DWORD dwLeft = 0;
        DWORD dwRight = dwLength-1;
        while (dwLeft < dwRight) {
            BYTE chTemp = *(pPtr + dwRight);
            *(pPtr + dwRight) = *(pPtr + dwLeft);
            *(pPtr + dwLeft) = chTemp;
            dwLeft ++;
            dwRight --;
        }
    }
}

BOOL CSDDevice::IsDriverEnabled()
{
    if (m_pDriverFolder!=NULL) {
        DWORD dwResult = 0 ;
        return (m_pDriverFolder->IsDeviceDriverEnabled(&dwResult) && dwResult!= 0);
    }
    else
        return FALSE;
}


