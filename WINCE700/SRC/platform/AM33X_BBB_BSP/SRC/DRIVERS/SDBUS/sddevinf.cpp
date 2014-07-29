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
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDDevInfo.cpp
Abstract:
    SDBus Implementation.

Notes: 
--*/
#include <windows.h>
#include <safeint.hxx>
#include <sdhcd.h>
#include <image_cfg.h>

#include "sdbus.hpp"
#include "sdslot.hpp"
#include "sdbusreq.hpp"
#include "sddevice.hpp"


BOOL  CSDDevice::IsHighCapacitySDMemory()
{
    if (m_DeviceType == Device_SD_Memory || m_DeviceType == Device_MMC || m_DeviceType == Device_SD_Combo ) {
        return ( (m_CachedRegisters.OCR[3] & 0x40)!=0) ; // OCR Bit 30 is Card Capacity Status bit.
    }
    else 
        return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//  SDGetTuple__X  - Get tuple data from CIS
//  Input:  hDevice     - SD bus device handle
//          TupleCode   - Tuple code
//          pBufferSize - size of buffer to store Tuple Data
//          CommonCIS   - flag indicating common or function CIS
//  Output: pBuffer     - Tuple data is copied here (optional)
//          pBufferSize - if pBuffer is NULL, this will store the size of the
//                        tuple
//  Return: SD_API_STATUS code
//          
//  Notes: The caller should initially call this function with a NULL buffer
//         to determine the size of the tuple.  The variable pBufferSize points
//         to the caller supplied storage for this result.   If no bus errors occurs
//         the function returns SD_API_STATUS_SUCCESS.   The caller must check 
//         the value of the buffer size returned.  If the value is non-zero, the
//         tuple exists and can be fetched by calling this function again with a
//         non-zero buffer.
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDGetTuple_I(UCHAR TupleCode,PUCHAR pBuffer,PULONG pBufferSize,BOOL CommonCIS)
{
    SD_API_STATUS status = SD_API_STATUS_SUCCESS;  // intermediate status
    UCHAR                  tCode;                  // tuple code we've read so far
    UCHAR                  tupleLink;              // tuple link offset
    ULONG                  currentOffset;          // current offset

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDCard: +SDGetTuple\n")));


    if (NULL == pBufferSize) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDGetTuple: NULL buffer size \n")));
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    if (pBuffer == NULL) {
        // Initialize pBufferSize to zero to indicate that the tuple 
        // was not found
        *pBufferSize = 0; 
    }
    
    currentOffset = 0;
    // walk through the CIS
    for (;;) {

        // get 1 byte at the current position
        status = SDGetTupleBytes(currentOffset,  &tCode, 1, CommonCIS);

        if (!SD_API_SUCCESS(status)) {
            break;
        }
        // add the tCode
        currentOffset += 1;

        if(SD_CISTPL_END == tCode) {
            // this is the End of chain Tuple
            // break out of while loop
            break;

        } else  {

            // get the tuple link offset in the next byte, we always need this
            // value, so we fetch it before we compare the tuple code
            status = SDGetTupleBytes(currentOffset, &tupleLink, 1, CommonCIS);

            if (!SD_API_SUCCESS(status)) {
                break;
            }

            // add the link
            currentOffset += 1;

            // check for the end link flag, this is the alternative method to stop
            // tuple scanning
            if (SD_TUPLE_LINK_END == tupleLink) {
                // we reached an end of chain
                break;
            }
            // go back and check the tuple code
            if (tCode == TupleCode) {
                // found it

                // check to see if the caller is interested in the data
                if (NULL != pBuffer) {

                    // if the user passed a buffer, they must pass the buffer size, double check the length
                    if (*pBufferSize < tupleLink) {
                        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDCard: SDGetTuple, caller supplied buffer of size: %d bytes but tuple body (code=0x%02X) reports size of %d bytes\n"),
                            *pBufferSize, tCode, tupleLink));    
                        status = SD_API_STATUS_INVALID_PARAMETER;
                        break;
                    }

                    // fetch the tuple body
                    status = SDGetTupleBytes(currentOffset, 
                        pBuffer, 
                        tupleLink, 
                        CommonCIS);

                } else {

                    // return the size of the tuple body we just found, no need to fetch
                    *pBufferSize = tupleLink;
                }
                // break out of the while loop 
                break;

            } else {

                // add the value of the link
                currentOffset += tupleLink;
            }

        } 

    }

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDCard: -SDGetTuple\n")));
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDGetTupleBytes - Gets tuple bytes at the current tuple offset
//  Input:  hDevice       - SD bus device handle
//          Offset        - offset from the CIS pointer
//          NumberOfBytes - number of bytes to fetch
//          CommonCIS     - flag indicating that this is fetched from the common CIS
//  Output: pBuffer       - Tuple data is copied here (optional)

//  Return: SD_API_STATUS code
//          
//  Notes:
//         If the Buffer pointer is NULL, the function does not fetch the bytes
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDGetTupleBytes(DWORD Offset,PUCHAR pBuffer,ULONG NumberOfBytes,BOOL CommonCIS)
{
    SD_API_STATUS          status;          // intermediate status
    DWORD                  tupleAddress;    // calculated tuple address


    status = SD_API_STATUS_INVALID_PARAMETER;

    if (m_DeviceType == Device_SD_IO || m_DeviceType == Device_SD_Combo || CommonCIS) {
        if (CommonCIS) {
            if (m_FuncionIndex!=0) {
                CSDDevice * psdDevice0 = m_sdSlot.GetFunctionDevice(0);
                if (psdDevice0) {
                    status = psdDevice0->SDGetTupleBytes(Offset,pBuffer,NumberOfBytes, CommonCIS);
                    psdDevice0->DeRef();
                }
                return status;
            }
            else {
                DEBUGCHK(NULL != m_SDCardInfo.SDIOInformation.pCommonInformation);
                DEBUGCHK(0 != m_SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer);
                // the tuple starting address is at the common CIS pointer
                tupleAddress = m_SDCardInfo.SDIOInformation.pCommonInformation->CommonCISPointer;
            }
        } else { 
            DEBUGCHK(0 != m_SDCardInfo.SDIOInformation.CISPointer);
            // the tuple starting address is at the function CIS pointer
            tupleAddress = m_SDCardInfo.SDIOInformation.CISPointer;
        }

        // add the desired offset
        tupleAddress += Offset;

        if (NULL != pBuffer) {
            CSDDevice * psdDevice0 = m_sdSlot.GetFunctionDevice(0);
            if (psdDevice0) {
                status = psdDevice0->SDReadWriteRegistersDirect_I( SD_IO_READ,tupleAddress,FALSE,pBuffer,NumberOfBytes); 
                psdDevice0->DeRef();
            }

        }
    }
    ASSERT(SD_API_SUCCESS(status));
    return status;
}

//Structures for function 1-7.
//Note that in SDIO 2.0 protocol the data structure is the same as V1.1. 

#include <pshpack1.h>
//Structure definiton of SDIO Version 1.0 term
typedef struct _SDIO_TPLFID_FUNC17_V10 {
    UCHAR   ExtendedDataType;
    UCHAR   FunctionInfo;
    UCHAR   StandardRev;
    ULONG   CardPSN;
    ULONG   CSASize;
    UCHAR   CSAProperty;
    USHORT  MaxBlockSize;
    ULONG   OCR;
    UCHAR   OpMinPwr;
    UCHAR   OpAvgPwr;
    UCHAR   OpMaxPwr;
    UCHAR   StbyMinPwr;
    UCHAR   StbyAvgPwr;
    UCHAR   StbyMaxPwr;
    USHORT  MinBandwidth;
    USHORT  OptimalBandwidth;
}SDIO_TPLFID_FUNC17_V10, *P_SDIO_TPLFID_FUNC17_V10;

//Structure definiton of SDIO Version 1.10 added term
typedef struct _SDIO_TPLFID_FUNC17_V11 {
    USHORT  EnableTimoutVal;
    USHORT  SpAvePwr33V;
    USHORT  SpMaxPwr33V;
    USHORT  HpAvePwr33V;
    USHORT  HpMaxPwr33V;
    USHORT  LpAvePwr33V;
    USHORT  LpMaxPwr33V;
}SDIO_TPLFID_FUNC17_V11, *P_SDIO_TPLFID_FUNC17_V11;
#include <poppack.h>

///////////////////////////////////////////////////////////////////////////////
//  SDGetFunctionPowerControlTuples - Get SDIO Power control Tuples for an SDIO device function
//  Input:  pDevice         - the device context
//  Output: pPowerDrawData  - Data about function's power draw
//  Return: SD_API_STATUS code
//  Notes:  
//         This function collects pwer draw data for a SD function 
//         via the tuple CITPL_FUNCE (extended 0x01).
//         
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDGetFunctionPowerControlTuples() 
{
    SD_API_STATUS               status;             // intermediate status
    ULONG                       length;             // tuple length
    UCHAR                       buffer[SD_CISTPLE_MAX_BODY_SIZE]; //tupple info
    P_SDIO_TPLFID_FUNC17_V10    pV1FunctionTuple;   // SDIO V1.0 Function Tuple
    P_SDIO_TPLFID_FUNC17_V11    pV11FunctionTupleExt;// SDIO V1.1 Extentions to Function Tuple
    PSD_FUNCTION_POWER_DRAW     pPowerDrawData = &m_SDCardInfo.SDIOInformation.PowerDrawData;

    length = 0;

        // get the FUNCE tuple
    status = SDGetTuple_I( SD_CISTPL_FUNCE, NULL, &length, FALSE);

    if (!SD_API_SUCCESS(status)) {
         return status;
    }

    if (0 == length) {
        DbgPrintZo(SDCARD_ZONE_ERROR, 
            (TEXT("SDBusDriver: Card does not have FUNCE tuple! \n")));
           return SD_API_STATUS_DEVICE_UNSUPPORTED;
    } else {

        if (length < sizeof(SDIO_TPLFID_FUNC17_V10)) {
             DbgPrintZo(SDCARD_ZONE_ERROR, 
                (TEXT("SDBusDriver: Function tuple reports size of %d , expecting %d or greater\n"),
                length, sizeof(SDIO_TPLFID_FUNC17_V10)));
            return SD_API_STATUS_DEVICE_UNSUPPORTED;   
        }

            // get the tplIDfunction tuple 
        status = SDGetTuple_I(SD_CISTPL_FUNCE,(PUCHAR)buffer,&length,FALSE);

        if (!SD_API_SUCCESS(status)) {
             return status;
        }

        if (buffer[0] != 0x01) {
             DbgPrintZo(SDCARD_ZONE_ERROR, 
                (TEXT("SDBusDriver: Tuple is not Extended Data type: %d \n"),
                buffer[0]));
            return SD_API_STATUS_DEVICE_UNSUPPORTED;   
        }

        pV1FunctionTuple = (P_SDIO_TPLFID_FUNC17_V10)buffer;

        pPowerDrawData->OpMinPower = pV1FunctionTuple->OpMinPwr;
        pPowerDrawData->OpAvePower = pV1FunctionTuple->OpAvgPwr;
        pPowerDrawData->OpMaxPower = pV1FunctionTuple->OpMaxPwr;


        if (length < (sizeof(SDIO_TPLFID_FUNC17_V10) + sizeof(SDIO_TPLFID_FUNC17_V11))) {
            pPowerDrawData->SpAvePower33 = 0;
            pPowerDrawData->SpMaxPower33 = 0;

            pPowerDrawData->HpAvePower33 = 0;
            pPowerDrawData->HpMaxPower33 = 0;

            pPowerDrawData->LpAvePower33 = 0;
            pPowerDrawData->LpMaxPower33 = 0;
        }
        else
        {
            pV11FunctionTupleExt = (P_SDIO_TPLFID_FUNC17_V11)(&buffer[sizeof(SDIO_TPLFID_FUNC17_V10)]);

            pPowerDrawData->SpAvePower33 = pV11FunctionTupleExt->SpAvePwr33V;
            pPowerDrawData->SpMaxPower33 = pV11FunctionTupleExt->SpMaxPwr33V;

            pPowerDrawData->HpAvePower33 = pV11FunctionTupleExt->HpAvePwr33V;
            pPowerDrawData->HpMaxPower33 = pV11FunctionTupleExt->HpMaxPwr33V;

            pPowerDrawData->LpAvePower33 = pV11FunctionTupleExt->LpAvePwr33V;
            pPowerDrawData->LpMaxPower33 = pV11FunctionTupleExt->LpMaxPwr33V;
        }
    }

    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Function %d Power Draw:\r\n"),m_SDCardInfo.SDIOInformation.Function));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    OpMinPower %d mA\r\n"),   m_SDCardInfo.SDIOInformation.PowerDrawData.OpMinPower));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    OpAvePower %d mA\r\n"),   m_SDCardInfo.SDIOInformation.PowerDrawData.OpAvePower));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    OpMaxPower %d mA\r\n"),   m_SDCardInfo.SDIOInformation.PowerDrawData.OpMaxPower));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    SpAvePower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.SpAvePower33));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    SpAvePower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.SpMaxPower33));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    HpAvePower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.HpAvePower33));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    HpMaxPower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    LpAvePower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.LpAvePower33));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("    LpMaxPower33 %d mA\r\n"), m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33));


    return SD_API_STATUS_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////////
//  GetFunctionPowerState  -  Get the power state of a SDIO function
//  Input:  pDevice   - the device 
//          
//  Output: pPowerState - structure describing the power state of the device 
//  Return: SD_API_STATUS code
//          
//  Notes: 
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::GetFunctionPowerState(  PFUNCTION_POWER_STATE pPowerState)
{
    SD_API_STATUS   status = SD_API_STATUS_INVALID_PARAMETER;         // intermediate status
    DWORD           FBROffset;      // calculated FBR offset
    UCHAR           regValue;       // register value
    
    if (m_FuncionIndex == 0 || pPowerState == NULL) {
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    CSDDevice *pDevice0 = m_sdSlot.GetFunctionDevice(0);
    if (pDevice0 == NULL) {
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    

    pPowerState->fPowerControlSupport = pDevice0->GetCardInfo().SDIOInformation.pCommonInformation->fCardSupportsPowerControl;
    pPowerState->fPowerControlEnabled = pDevice0->GetCardInfo().SDIOInformation.pCommonInformation->fPowerControlEnabled;

    pPowerState->fFunctionEnabled = (pDevice0->GetCardInfo().SDIOInformation.pCommonInformation->CCCRShadowIOEnable 
                & (1 <<  m_FuncionIndex)) ? TRUE : FALSE;
    
    FBROffset = SD_IO_FBR_1_OFFSET + (m_FuncionIndex - 1) * SD_IO_FBR_LENGTH;

    status = pDevice0->SDReadWriteRegistersDirect_I( SD_IO_READ, FBROffset + SD_IO_FBR_POWER_SELECT, FALSE, &regValue, 1); 
    if (!SD_API_SUCCESS(status)) {
        return status;
    }

    pPowerState->fSupportsPowerSelect = (regValue & SD_IO_FUNCTION_POWER_SELECT_SUPPORT) ? TRUE : FALSE;
    if(pPowerState->fSupportsPowerSelect) {
        pPowerState->fLowPower = regValue & SD_IO_FUNCTION_POWER_SELECT_STATE ? TRUE : FALSE;
    }
    else {
        pPowerState->fLowPower = 0;
    }

    pPowerState->OperatingVoltage = m_OperatingVoltage;
    
    //calculate the current draw
    if(pPowerState->fPowerControlSupport) {
        if(pPowerState->fFunctionEnabled) {
            //
            // in future when more power tuples are added select the proper tuple here
            // currently only the 3.3V tuple is supported
            //
            if((pPowerState->fPowerControlEnabled) && (pPowerState->fSupportsPowerSelect)) {
                if(pPowerState->fLowPower){
                        //function enabled at low power
                    pPowerState->CurrentDrawNow = m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33;
                    pPowerState->EnableDelta = ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33) * -1;
                    pPowerState->SelectDelta = ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33)
                                               - ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33);
                }
                else {
                        //function enabled at high power
                    pPowerState->CurrentDrawNow = m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33;
                    pPowerState->EnableDelta = ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33) * -1;
                    pPowerState->SelectDelta = ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33)
                                               - ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33);
                }
            }
            else {
                    //function enabled at no power select
                pPowerState->CurrentDrawNow = m_SDCardInfo.SDIOInformation.PowerDrawData.SpMaxPower33;
                pPowerState->EnableDelta = ((INT)m_SDCardInfo.SDIOInformation.PowerDrawData.SpMaxPower33) * -1;
                pPowerState->SelectDelta = 0;
            }
        }
        else {
            if((pPowerState->fPowerControlEnabled) && (pPowerState->fSupportsPowerSelect)) {
                if(pPowerState->fLowPower) {
                        //function disabled power select set to low
                    pPowerState->CurrentDrawNow = 0;
                    pPowerState->EnableDelta = m_SDCardInfo.SDIOInformation.PowerDrawData.LpMaxPower33;
                    pPowerState->SelectDelta = 0;
                }
                else  {
                        //function disabled power select set to low
                    pPowerState->CurrentDrawNow = 0;
                    pPowerState->EnableDelta = m_SDCardInfo.SDIOInformation.PowerDrawData.HpMaxPower33;
                    pPowerState->SelectDelta = 0;
                }
            }
            else {
                    //function disabled, no power select
                pPowerState->CurrentDrawNow = 0;
                pPowerState->EnableDelta = m_SDCardInfo.SDIOInformation.PowerDrawData.SpMaxPower33;
            }
        }
    }
    else
    {
        USHORT TempMaxPower;
            //the current draw must never be greater than 200mA for a non Power Control enabled card
        if((0 == m_SDCardInfo.SDIOInformation.PowerDrawData.OpMaxPower) || (200 < m_SDCardInfo.SDIOInformation.PowerDrawData.OpMaxPower)){
            TempMaxPower = 200; 
        }
        else {
            TempMaxPower = m_SDCardInfo.SDIOInformation.PowerDrawData.OpMaxPower; 
        }

        if(pPowerState->fFunctionEnabled) {
                //function enabled, no power control
            pPowerState->CurrentDrawNow = TempMaxPower;
            pPowerState->EnableDelta = ((INT)TempMaxPower) * -1;
            pPowerState->SelectDelta = 0;
        }
        else {
                //function disabled, no power control
            pPowerState->CurrentDrawNow = 0;
            pPowerState->EnableDelta = (INT)TempMaxPower;
            pPowerState->SelectDelta = 0;
        }
    }
    return SD_API_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  GetCardStatus  - retrieve the card status
//  Input:  hDevice        - SD Device Handle
//  Output: pCardStatus    - the card status
//  Return: SD_API_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::GetCardStatus(SD_CARD_STATUS   *pCardStatus)
{
    SD_API_STATUS       status;         // status
    SD_COMMAND_RESPONSE cardResponse;   // response buffer

    // Initiate the bus transaction
    status = SDSynchronousBusRequest_I(
        SD_CMD_SEND_STATUS,
        ((DWORD)(m_RelativeAddress) << 16),
        SD_COMMAND,
        ResponseR1,
        &cardResponse,
        0,
        0,
        NULL,
        0);

    // Get the status and convert if necessary
    if (!SD_API_SUCCESS(status) ) {
        DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDGetCardStatus Failed: status 0x%X\r\n"),status));
        return status;
    }

    SDGetCardStatusFromResponse(&cardResponse, pCardStatus);

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("Status: 0x%08X, current state: %d \r\n"),
        *pCardStatus, SD_STATUS_CURRENT_STATE(*pCardStatus)));

    return status;
}



SD_API_STATUS CSDDevice::InfoQueryCID(PVOID pCardInfo, ULONG cbCardInfo)
{
    PREFAST_DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_PARSED_REGISTER_CID));
    UNREFERENCED_PARAMETER(cbCardInfo);

    PSD_PARSED_REGISTER_CID pParsedCID = (PSD_PARSED_REGISTER_CID)pCardInfo;
    UCHAR const*const            pCid  = m_CachedRegisters.CID;
    UCHAR                   Prv;

    pParsedCID->ManufacturerID = pCid[SD_CID_MID_OFFSET];  

    // get the application ID string
    pParsedCID->OEMApplicationID[0] = pCid[SD_CID_OID_OFFSET];
    pParsedCID->OEMApplicationID[1] = pCid[SD_CID_OID_OFFSET+1];
    pParsedCID->OEMApplicationID[2] = '\0';


    // MMC cards have a 1 char larger Product Name
    // and it starts 1 byte earlier in the CID data.
    // PSN and PRV are offset by 1 byte and the date
    // field has just a 4 bit year code starting at 1997.
    if( m_DeviceType == Device_MMC ) {
        pParsedCID->ProductName[0] = pCid[MMC_CID_PNM_OFFSET]; 
        pParsedCID->ProductName[1] = pCid[MMC_CID_PNM_OFFSET+1]; 
        pParsedCID->ProductName[2] = pCid[MMC_CID_PNM_OFFSET+2]; 
        pParsedCID->ProductName[3] = pCid[MMC_CID_PNM_OFFSET+3]; 
        pParsedCID->ProductName[4] = pCid[MMC_CID_PNM_OFFSET+4];
        pParsedCID->ProductName[5] = pCid[MMC_CID_PNM_OFFSET+5];
        pParsedCID->ProductName[6] = '\0';

        // get major and minor revs                                                               
        Prv = pCid[MMC_CID_PRV_OFFSET];    
        pParsedCID->MajorProductRevision = (Prv & 0xF0) >> 4;  
        pParsedCID->MinorProductRevision = Prv & 0x0F;                     

        // serial number
        pParsedCID->ProductSerialNumber = pCid[MMC_CID_PSN_OFFSET] | 
            (pCid[MMC_CID_PSN_OFFSET + 1] << 8)| 
            (pCid[MMC_CID_PSN_OFFSET + 2] << 16) | 
            (pCid[MMC_CID_PSN_OFFSET + 3] << 24); 

        // Manufacturing month
        pParsedCID->ManufacturingMonth = (pCid[MMC_CID_MDT_OFFSET] & MMC_CID_MONTH_MASK) >> MMC_CID_MONTH_SHIFT;
        // Manufacturing year
        pParsedCID->ManufacturingYear = pCid[MMC_CID_MDT_OFFSET] & MMC_CID_YEAR_MASK; 
        // Year starts at 1997
        pParsedCID->ManufacturingYear += 1997;
    } else {  
        pParsedCID->ProductName[0] = pCid[SD_CID_PNM_OFFSET]; 
        pParsedCID->ProductName[1] = pCid[SD_CID_PNM_OFFSET+1]; 
        pParsedCID->ProductName[2] = pCid[SD_CID_PNM_OFFSET+2]; 
        pParsedCID->ProductName[3] = pCid[SD_CID_PNM_OFFSET+3]; 
        pParsedCID->ProductName[4] = pCid[SD_CID_PNM_OFFSET+4];
        pParsedCID->ProductName[5] = '\0';
        pParsedCID->ProductName[6] = '\0';

        // get major and minor revs                                                               
        Prv = pCid[SD_CID_PRV_OFFSET];    
        pParsedCID->MajorProductRevision = (Prv & 0xF0) >> 4;  
        pParsedCID->MinorProductRevision = Prv & 0x0F;                     

        // serial number
        pParsedCID->ProductSerialNumber = pCid[SD_CID_PSN_OFFSET] | 
            (pCid[SD_CID_PSN_OFFSET + 1] << 8)| 
            (pCid[SD_CID_PSN_OFFSET + 2] << 16) | 
            (pCid[SD_CID_PSN_OFFSET + 3] << 24); 

        pParsedCID->ManufacturingMonth = pCid[SD_CID_MDT_OFFSET] & SD_CID_MONTH_MASK;
        // get lower 4 bits
        pParsedCID->ManufacturingYear = (pCid[SD_CID_MDT_OFFSET] & SD_CID_YEAR0_MASK) >> SD_CID_YEAR_SHIFT ; 
        // get upper 4 bits
        pParsedCID->ManufacturingYear  |= pCid[SD_CID_MDT_OFFSET+1] << SD_CID_YEAR_SHIFT;  
        // starts at year 2000
        pParsedCID->ManufacturingYear += 2000;
    }     

    memcpy(pParsedCID->RawCIDRegister, m_CachedRegisters.CID, SD_CID_REGISTER_SIZE);

    return SD_API_STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//  DumpParsedCSDRegisters- dump parsed register data to the debugger
//  Input:  pParsedCSD - the Parsed CSD structure
//  Output: 
//
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
inline VOID DumpParsedCSDRegisters(PSD_PARSED_REGISTER_CSD pParsedCSD)
{

    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("\n\n\nSDCard: Dumping parsed Registers : \n")));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" CSD Version:%d \n"),pParsedCSD->CSDVersion)); 
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Spec Version:%d \n"),pParsedCSD->SpecVersion)); 
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" TAAC: %f ns \n"),pParsedCSD->DataAccessTime.TAAC));  
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" NSAC: %d clocks \n"),pParsedCSD->DataAccessTime.NSAC)); 
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" MaxDataTransferRate: %d kb/s \n"),pParsedCSD->MaxDataTransferRate));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Card Command Classes: 0x%04X \n"),pParsedCSD->CardCommandClasses));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Max Read Block Length: %d bytes \n"),pParsedCSD->MaxReadBlockLength));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Read Block Partial? : %d  \n"),pParsedCSD->ReadBlockPartial));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Max Write Block Length: %d bytes \n"),pParsedCSD->MaxWriteBlockLength));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Write Block Partial? : %d  \n"),pParsedCSD->WriteBlockPartial));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Write Block Misaligned? : %d  \n"),pParsedCSD->WriteBlockMisalign));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Read Block Misaligned? : %d  \n"),pParsedCSD->ReadBlockMisalign));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" DSR Implemented? : %d  \n"),pParsedCSD->DSRImplemented));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Device Size : %d bytes  \n"),pParsedCSD->DeviceSize));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" VDD Read Current Min : %d mA \n"),pParsedCSD->VDDReadCurrentMin));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" VDD Read Current Max : %d mA \n"),pParsedCSD->VDDReadCurrentMax));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" VDD Write Current Min : %d mA \n"),pParsedCSD->VDDWriteCurrentMin));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" VDD Write Current Max : %d mA \n"),pParsedCSD->VDDWriteCurrentMax));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Erase Block Enabled?: %d  \n"),pParsedCSD->EraseBlockEnable));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Erase Sector Size: %d blocks \n"),pParsedCSD->EraseSectorSize));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Write Protect Group Enabled? %d \n"),pParsedCSD->WPGroupEnable));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Write Group Protect Size: %d blocks \n"),pParsedCSD->WPGroupSize));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Write Speed Factor: %d blocks \n"),pParsedCSD->WriteSpeedFactor));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Copy Flag?:  %d \n"),pParsedCSD->CopyFlag));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Permanent Write Protect?:  %d \n"),pParsedCSD->PermanentWP));
    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" Temporary Write Protect?:  %d \n"),pParsedCSD->TemporaryWP));

    switch (pParsedCSD->FileSystem ) {
        case SD_FS_FAT_PARTITION_TABLE: 
            DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" FileSystem = FAT with Partition Table \n")));
            break;
        case SD_FS_FAT_NO_PARTITION_TABLE: 
            DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" FileSystem = FAT with No Partition Table \n")));
            break;
        case SD_FS_UNIVERSAL:  
            DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" FileSystem = Universal \n")));
            break;
        default: 
            DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT(" FileSystem = Other/Unknown \n")));
    }

    DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("---------------------------------------------- \n\n\n")));
}

SD_API_STATUS CSDDevice::InfoQueryCSD(PVOID pCardInfo, ULONG cbCardInfo)
{
    PREFAST_DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_PARSED_REGISTER_CSD));
    UNREFERENCED_PARAMETER(cbCardInfo);

    PSD_PARSED_REGISTER_CSD pParsedCSD = (PSD_PARSED_REGISTER_CSD)pCardInfo;
    UCHAR const*const       pCSD  = m_CachedRegisters.CSD;
    UCHAR                   value, unit;        // Used for access time/transfer rate calculations
    DWORD                   cSize, cSizeMult;   // Used for device size calculation
    UCHAR                   fileFormatGroup, fileFormat;
    UCHAR                   rblLength;

    pParsedCSD->CSDVersion = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_VERSION_BIT_SLICE, 
        SD_CSD_VERSION_SLICE_SIZE);

    pParsedCSD->SpecVersion = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_SPECVER_BIT_SLICE, 
        SD_CSD_SPECVER_SLICE_SIZE);

    // check the CSD version code
    if (Device_SD_Memory == m_DeviceType) {
        if (pParsedCSD->CSDVersion > (SD_CSD_VERSION_CODE_SUPPORTED)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDCardInfoQuery: SD CSD version : %d currently not supported \n"),pParsedCSD->CSDVersion));
            DEBUGCHK(FALSE);
            return SD_API_STATUS_DEVICE_UNSUPPORTED;
        }
    } else if (Device_MMC == m_DeviceType) {
        // remove MMC CSD version check. To do this, we would allow MMC v4.x card to be able
        // to operate as V3.1 card in 1 bit mode.
        if (pParsedCSD->CSDVersion > MMC_CSD_VERSION_CODE_SUPPORTED) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDCardInfoQuery: MMC CSD version : %d currently not supported \n"),pParsedCSD->CSDVersion));
            DEBUGCHK(FALSE);
            return SD_API_STATUS_DEVICE_UNSUPPORTED;
        }
    } else {
        DEBUGCHK(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    // get the value
    value = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_TAAC_BIT_SLICE, 
        SD_CSD_TAAC_SLICE_SIZE);
    // extract the time units
    unit  = value & SD_CSD_TAAC_UNIT_MASK;
    // get the timee value into position
    value = (value & SD_CSD_TAAC_VALUE_MASK) >> SD_CSD_TAAC_VALUE_SHIFT;

    switch( value ) {
        case 1:  pParsedCSD->DataAccessTime.TAAC = 1.0; break;
        case 2:  pParsedCSD->DataAccessTime.TAAC = 1.2; break;
        case 3:  pParsedCSD->DataAccessTime.TAAC = 1.3; break;
        case 4:  pParsedCSD->DataAccessTime.TAAC = 1.5; break;
        case 5:  pParsedCSD->DataAccessTime.TAAC = 2.0; break;
        case 6:  pParsedCSD->DataAccessTime.TAAC = 2.5; break;
        case 7:  pParsedCSD->DataAccessTime.TAAC = 3.0; break;
        case 8:  pParsedCSD->DataAccessTime.TAAC = 3.5; break;
        case 9:  pParsedCSD->DataAccessTime.TAAC = 4.0; break;
        case 10: pParsedCSD->DataAccessTime.TAAC = 4.5; break;
        case 11: pParsedCSD->DataAccessTime.TAAC = 5.0; break;
        case 12: pParsedCSD->DataAccessTime.TAAC = 5.5; break;
        case 13: pParsedCSD->DataAccessTime.TAAC = 6.0; break;
        case 14: pParsedCSD->DataAccessTime.TAAC = 7.0; break;
        case 15: pParsedCSD->DataAccessTime.TAAC = 8.0; break;
        default: pParsedCSD->DataAccessTime.TAAC = 0.0; break;
    };

    for( ; unit; unit-- ) {
        pParsedCSD->DataAccessTime.TAAC *= 10; 
    }

    pParsedCSD->DataAccessTime.NSAC = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_NSAC_BIT_SLICE, 
        SD_CSD_NSAC_SLICE_SIZE);


    // Calculate transfer rate in kbit/s
    value = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_TRANS_SPEED_BIT_SLICE, 
        SD_CSD_TRANS_SPEED_SLICE_SIZE);

    unit  = value & SD_CSD_TRANS_SPEED_UNIT_MASK;

    // get value bits into position
    value = (value & SD_CSD_TRANS_SPEED_VALUE_MASK) >> SD_CSD_TRANS_SPEED_VALUE_SHIFT;

    switch( value ) {            
        case 1:  pParsedCSD->MaxDataTransferRate = 100; break;
        case 2:  pParsedCSD->MaxDataTransferRate = 120; break;
        case 3:  pParsedCSD->MaxDataTransferRate = 130; break;
        case 4:  pParsedCSD->MaxDataTransferRate = 150; break;
        case 5:  pParsedCSD->MaxDataTransferRate = 200; break;
        case 6:  pParsedCSD->MaxDataTransferRate = 250; break;
        case 7:  pParsedCSD->MaxDataTransferRate = 300; break;
        case 8:  pParsedCSD->MaxDataTransferRate = 350; break;
        case 9:  pParsedCSD->MaxDataTransferRate = 400; break;
        case 10: pParsedCSD->MaxDataTransferRate = 450; break;
        case 11: pParsedCSD->MaxDataTransferRate = 500; break;
        case 12: pParsedCSD->MaxDataTransferRate = 550; break;
        case 13: pParsedCSD->MaxDataTransferRate = 600; break;
        case 14: pParsedCSD->MaxDataTransferRate = 700; break;
        case 15: pParsedCSD->MaxDataTransferRate = 800; break;
        default: pParsedCSD->MaxDataTransferRate = 0;   break;
    };                              

    for( ; unit; unit-- )   
        pParsedCSD->MaxDataTransferRate *= 10;              


    pParsedCSD->CardCommandClasses = (USHORT)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_CCC_BIT_SLICE, 
        SD_CSD_CCC_SLICE_SIZE);



    rblLength = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD, SD_CSD_READ_BL_LEN_BIT_SLICE, 
        SD_CSD_READ_BL_LEN_SLICE_SIZE);

    DEBUG_CHECK((rblLength < 12), (TEXT("SDCardInfoQuery - Read Block Length %d is invalid \n"),rblLength));

    // Read Block Length
    pParsedCSD->MaxReadBlockLength = 1 << rblLength;

    // Write Block Length
    pParsedCSD->MaxWriteBlockLength = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_WRITE_BL_LEN_BIT_SLICE, 
        SD_CSD_WRITE_BL_LEN_SLICE_SIZE);             

    DEBUG_CHECK((pParsedCSD->MaxWriteBlockLength < 12), (TEXT("SDCardInfoQuery - Write Block Length Length %d is invalid \n"), 
        pParsedCSD->MaxWriteBlockLength));

    pParsedCSD->MaxWriteBlockLength = 1 << pParsedCSD->MaxWriteBlockLength;



    pParsedCSD->ReadBlockPartial =  GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_READ_BL_PARTIAL_BIT_SLICE, 
        SD_CSD_READ_BL_PARTIAL_SLICE_SIZE) 
        ? TRUE:FALSE;

    pParsedCSD->WriteBlockPartial = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_WRITE_BL_PARTIAL_BIT_SLICE, 
        SD_CSD_WRITE_BL_PARTIAL_SLICE_SIZE) 
        ? TRUE:FALSE;


    // Read/Write Block Misalign
    pParsedCSD->WriteBlockMisalign = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_WRITE_BL_MISALIGN_BIT_SLICE, 
        SD_CSD_WRITE_BL_MISALIGN_SLICE_SIZE) 
        ? TRUE:FALSE;


    pParsedCSD->ReadBlockMisalign = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_READ_BL_MISALIGN_BIT_SLICE, 
        SD_CSD_READ_BL_MISALIGN_SLICE_SIZE) 
        ? TRUE:FALSE;



    // DSR Implemented
    pParsedCSD->DSRImplemented = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_DSR_IMP_BIT_SLICE, 
        SD_CSD_DSR_IMP_SLICE_SIZE) 
        ? TRUE:FALSE;

    // Write Protect Group Enabled
    pParsedCSD->WPGroupEnable = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_WP_GRP_ENABLE_BIT_SLICE, 
        SD_CSD_WP_GRP_ENABLE_SLICE_SIZE) 
        ? TRUE:FALSE;



    // Copy Flag
    pParsedCSD->CopyFlag = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_COPY_FLAG_BIT_SLICE, 
        SD_CSD_COPY_FLAG_SLICE_SIZE) 
        ? TRUE:FALSE;

    // Permanent Write Protect
    pParsedCSD->PermanentWP = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_PERM_WR_PROT_BIT_SLICE, 
        SD_CSD_PERM_WR_PROT_SLICE_SIZE) 
        ? TRUE:FALSE;


    // Temporary Write Protect
    pParsedCSD->TemporaryWP = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_TEMP_WR_PROT_BIT_SLICE, 
        SD_CSD_TEMP_WR_PROT_SLICE_SIZE) 
        ? TRUE:FALSE;

    // Calculate the device size as per SD Spec
    cSize =  GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_CSIZE_BIT_SLICE, 
        SD_CSD_CSIZE_SLICE_SIZE);



    cSizeMult = GET_BIT_SLICE_FROM_CSD(pCSD,
        SD_CSD_CSIZE_MULT_BIT_SLICE, 
        SD_CSD_CSIZE_MULT_SLICE_SIZE); 

    DEBUG_CHECK((cSizeMult < 8), (TEXT("SDCardInfoQuery - C_SIZE %d is invalid \n"),cSizeMult));

    // Perform actual device size calculation
    pParsedCSD->DeviceSize = pParsedCSD->MaxReadBlockLength * ((cSize+1) * (1<<(cSizeMult+2)));
    m_SDCardInfo.SDMMCInformation.ullDeviceSize = pParsedCSD->DeviceSize;

    // VDD Read Current Minimum
    pParsedCSD->VDDReadCurrentMin = (USHORT)GET_BIT_SLICE_FROM_CSD(pCSD, SD_CSD_R_CURR_MIN_BIT_SLICE, 
        SD_CSD_R_CURR_MIN_SLICE_SIZE);

    switch( pParsedCSD->VDDReadCurrentMin ) {
        case 0:  pParsedCSD->VDDReadCurrentMin = 1;   break;
        case 1:  pParsedCSD->VDDReadCurrentMin = 1;   break;
        case 2:  pParsedCSD->VDDReadCurrentMin = 5;   break;
        case 3:  pParsedCSD->VDDReadCurrentMin = 10;  break;
        case 4:  pParsedCSD->VDDReadCurrentMin = 25;  break;
        case 5:  pParsedCSD->VDDReadCurrentMin = 35;  break;
        case 6:  pParsedCSD->VDDReadCurrentMin = 60;  break;
        case 7:  pParsedCSD->VDDReadCurrentMin = 100; break;
        default: pParsedCSD->VDDReadCurrentMin = 0;   break;
    }


    // VDD Write Current Minimum
    pParsedCSD->VDDWriteCurrentMin = (USHORT)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_W_CURR_MIN_BIT_SLICE, 
        SD_CSD_W_CURR_MIN_SLICE_SIZE);

    switch( pParsedCSD->VDDWriteCurrentMin ) {
        case 0:  pParsedCSD->VDDWriteCurrentMin = 1;   break;
        case 1:  pParsedCSD->VDDWriteCurrentMin = 1;   break;
        case 2:  pParsedCSD->VDDWriteCurrentMin = 5;   break;
        case 3:  pParsedCSD->VDDWriteCurrentMin = 10;  break;
        case 4:  pParsedCSD->VDDWriteCurrentMin = 25;  break;
        case 5:  pParsedCSD->VDDWriteCurrentMin = 35;  break;
        case 6:  pParsedCSD->VDDWriteCurrentMin = 60;  break;
        case 7:  pParsedCSD->VDDWriteCurrentMin = 100; break;
        default: pParsedCSD->VDDWriteCurrentMin = 0;   break;
    }


    // VDD Read Current Maximum
    pParsedCSD->VDDReadCurrentMax = (USHORT)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_R_CURR_MAX_BIT_SLICE, 
        SD_CSD_R_CURR_MAX_SLICE_SIZE);

    switch( pParsedCSD->VDDReadCurrentMax ) {
        case 0:  pParsedCSD->VDDReadCurrentMax = 1;   break;
        case 1:  pParsedCSD->VDDReadCurrentMax = 5;   break;
        case 2:  pParsedCSD->VDDReadCurrentMax = 10;  break;
        case 3:  pParsedCSD->VDDReadCurrentMax = 25;  break;
        case 4:  pParsedCSD->VDDReadCurrentMax = 35;  break;
        case 5:  pParsedCSD->VDDReadCurrentMax = 45;  break;
        case 6:  pParsedCSD->VDDReadCurrentMax = 80;  break;
        case 7:  pParsedCSD->VDDReadCurrentMax = 200; break;
        default: pParsedCSD->VDDReadCurrentMax = 0;   break;
    }


    // VDD Write Current Maximum
    pParsedCSD->VDDWriteCurrentMax = (USHORT)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_W_CURR_MAX_BIT_SLICE, 
        SD_CSD_W_CURR_MAX_SLICE_SIZE);

    switch( pParsedCSD->VDDWriteCurrentMax ) {
        case 0:  pParsedCSD->VDDWriteCurrentMax = 1;   break;
        case 1:  pParsedCSD->VDDWriteCurrentMax = 5;   break;
        case 2:  pParsedCSD->VDDWriteCurrentMax = 10;  break;
        case 3:  pParsedCSD->VDDWriteCurrentMax = 25;  break;
        case 4:  pParsedCSD->VDDWriteCurrentMax = 35;  break;
        case 5:  pParsedCSD->VDDWriteCurrentMax = 45;  break;
        case 6:  pParsedCSD->VDDWriteCurrentMax = 80;  break;
        case 7:  pParsedCSD->VDDWriteCurrentMax = 200; break;
        default: pParsedCSD->VDDWriteCurrentMax = 0;   break;
    }


    pParsedCSD->WriteSpeedFactor = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_R2W_FACTOR_BIT_SLICE, 
        SD_CSD_R2W_FACTOR_SLICE_SIZE);

    // For CSD Version 2.0 above is wrong the following code correct them.
    if (Device_SD_Memory == m_DeviceType && pParsedCSD->CSDVersion == SD_CSD_VERSION_CODE_2_0 ) { 
        // SD Physical Layer Spec(2.0) 5.3.3
        ASSERT(pParsedCSD->MaxReadBlockLength==0x200);
        ASSERT(pParsedCSD->MaxWriteBlockLength==0x200);
        // THis field no long exist in SD Memory 2.0 So we made them up.
        pParsedCSD->VDDReadCurrentMin = 0;
        pParsedCSD->VDDWriteCurrentMin = 0;
        pParsedCSD->VDDReadCurrentMax = 200;
        pParsedCSD->VDDWriteCurrentMax = 200 ;
        // C_SIZE is different from 1.0.
        ULONGLONG ullSectorSize = (ULONGLONG)GET_BIT_SLICE_FROM_CSD(pCSD, SD_CSD20_CSIZE_BIT_SLICE,  SD_CSD20_CSIZE_SLICE_SIZE); 
        m_SDCardInfo.SDMMCInformation.ullDeviceSize = (ULONGLONG)(ullSectorSize+1)*0x200*0x400; 
    }
    else  if (Device_MMC == m_DeviceType) { 
        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("CSDDevice::InfoQueryCSD: SpecVersion = %d, CSDVersion = %d"), 
                                                                                                    pParsedCSD->SpecVersion, pParsedCSD->CSDVersion));
        if(pParsedCSD->SpecVersion>= MMC_SPEC_VERSION_4_PLUS){
            // We need read extended CSD for SEC_COUNT register value
            // high capacity card relies on this register to report correct size
            SD_COMMAND_RESPONSE response;  // response
            DWORD dwExtCSDDatas[512/sizeof(DWORD)];
            SD_API_STATUS status = SDSynchronousBusRequest_I(SD_CMD_MMC_SEND_EXT_CSD, 0 , SD_READ,  ResponseR1,&response,
                1,sizeof(dwExtCSDDatas),(PBYTE)dwExtCSDDatas,0);  
            if (SD_API_SUCCESS(status) ) {
                DWORD dwSectors = dwExtCSDDatas[212/sizeof(DWORD)];
                if(dwSectors == 0){
                    // it is possible that host controller driver already swaped the bytes. 
                    // if not, then we need to do it
                    SwapByte((PBYTE)dwExtCSDDatas, sizeof(dwExtCSDDatas));
                    dwSectors = dwExtCSDDatas[212/sizeof(DWORD)];
                }
                if(dwSectors != 0) { // yes we have valid info about the number of sectors 
                    m_SDCardInfo.SDMMCInformation.ullDeviceSize = ((ULONGLONG)dwSectors)*(ULONGLONG)0x200;
                }
            }
            else
                return status;
        }
    }


    if (IsHighCapacitySDMemory()) {
        // We had no choic but provide low portion of value for BC. The Full value can get from Card Extention function.
        pParsedCSD->DeviceSize = (ULONG)(m_SDCardInfo.SDMMCInformation.ullDeviceSize/(ULONGLONG)0x200); // return as block size.

    }

    if ((Device_MMC == m_DeviceType) && (pParsedCSD->DeviceSize > EMMC_MODULE_SIZE_BOUNDARY) && (m_sdSlot.GetHost().GetIndex() == EMMC_MODULE_HOST_INDEX)) { 
       // subtract eMMC reserved area so file system can't access it
       RETAILMSG(0, (TEXT("CSDDevice::InfoQueryCSD: hostControllerIndex=%d  high capacity MMC card.  Assume it's eMMC module. Raw device size = %d\r\n"), m_sdSlot.GetHost().GetIndex(), pParsedCSD->DeviceSize));
//       pParsedCSD->DeviceSize -= IMAGE_EMMC_RESERVED_AREA_SECTOR_SIZE;
    }
	
    RETAILMSG(0, (TEXT("CSDDevice::InfoQueryCSD: hostControllerIndex=%d  DeviceType=%d	pParsedCSD->DeviceSize = %d\r\n"), 
       m_sdSlot.GetHost().GetIndex(), m_DeviceType, pParsedCSD->DeviceSize));

    pParsedCSD->WriteSpeedFactor = 1 << pParsedCSD->WriteSpeedFactor;


    fileFormatGroup = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_FILE_GROUP_BIT_SLICE, 
        SD_CSD_FILE_GROUP_SLICE_SIZE);

    fileFormat  = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_FILE_FORMAT_BIT_SLICE, 
        SD_CSD_FILE_FORMAT_SLICE_SIZE);

    if( fileFormatGroup == 0 ) {
        switch( fileFormat ) {
            case 0:  pParsedCSD->FileSystem = SD_FS_FAT_PARTITION_TABLE;    break;
            case 1:  pParsedCSD->FileSystem = SD_FS_FAT_NO_PARTITION_TABLE; break;
            case 2:  pParsedCSD->FileSystem = SD_FS_UNIVERSAL;              break;
            default: pParsedCSD->FileSystem = SD_FS_OTHER;                  break;
        }
    } else {
        pParsedCSD->FileSystem = SD_FS_OTHER;
    }

    // For MMC cards the WP Group Size is now 5 bits rather than 7, Erase sector size 
    // is calculated from 2 5bit fields. Erase block enable does not exist for MMC cards.
    if( m_DeviceType == Device_MMC ) {

        UCHAR eraseGroupSize, eraseGroupMult;

        // Set EraseBlockEnable to FALSE
        pParsedCSD->EraseBlockEnable = FALSE;

        // Calculate Erase Sector Size from the Erase Group Size and
        // Erase Group Mult fields
        eraseGroupSize = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,MMC_CSD_ER_GRP_SIZE_BIT_SLICE, 
            MMC_CSD_ER_GRP_SIZE_SLICE_SIZE);

        eraseGroupMult = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,MMC_CSD_ER_GRP_MULT_BIT_SLICE, 
            MMC_CSD_ER_GRP_MULT_SLICE_SIZE);


        pParsedCSD->EraseSectorSize = (eraseGroupSize+1)*(eraseGroupMult+1);

        pParsedCSD->WPGroupSize = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,MMC_CSD_WP_GROUP_SIZE_BIT_SLICE, 
            MMC_CSD_WP_GROUP_SIZE_SLICE_SIZE);

        pParsedCSD->WPGroupSize++;
    } else {

        // Erase by block size enabled
        pParsedCSD->EraseBlockEnable = GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_ERASE_BL_ENABLE_BIT_SLICE, 
            SD_CSD_ERASE_BL_ENABLE_SLICE_SIZE)
            ? TRUE:FALSE;

        // Erase Sector Size

        pParsedCSD->EraseSectorSize = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_ERASE_SECT_SIZE_BIT_SLICE, 
            SD_CSD_ERASE_SECT_SIZE_SLICE_SIZE);


        pParsedCSD->EraseSectorSize++;


        // Write Protect Group Size
        pParsedCSD->WPGroupSize = (UCHAR)GET_BIT_SLICE_FROM_CSD(pCSD,SD_CSD_WP_GROUP_SIZE_BIT_SLICE, 
            SD_CSD_WP_GROUP_SIZE_SLICE_SIZE);

        pParsedCSD->WPGroupSize++;
    }

    memcpy(pParsedCSD->RawCSDRegister, m_CachedRegisters.CSD, SD_CSD_REGISTER_SIZE );
    DumpParsedCSDRegisters(pParsedCSD);

    return SD_API_STATUS_SUCCESS;
}

SD_API_STATUS CSDDevice::InfoQueryRCA(PVOID pCardInfo, ULONG cbCardInfo)
{
    PREFAST_DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_CARD_RCA));
    UNREFERENCED_PARAMETER(cbCardInfo);

    memcpy(pCardInfo, &m_RelativeAddress, sizeof(SD_CARD_RCA));

    return SD_API_STATUS_SUCCESS;
}

SD_API_STATUS CSDDevice::InfoQueryCardInterface(PVOID pCardInfo,ULONG cbCardInfo)
{
    PREFAST_DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_CARD_INTERFACE_EX));
    UNREFERENCED_PARAMETER(cbCardInfo);

    m_CardInterfaceEx.InterfaceModeEx.bit.sdHighCapacity = (IsHighCapacitySDMemory()?1: 0);
    *(PSD_CARD_INTERFACE_EX)pCardInfo = m_CardInterfaceEx;
    return SD_API_STATUS_SUCCESS;
}


SD_API_STATUS CSDDevice::InfoQueryStatus(
                              PVOID                   pCardInfo,
                              ULONG                   cbCardInfo
                              )
{
    DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_CARD_STATUS));
    UNREFERENCED_PARAMETER(cbCardInfo);

    // get the card status
    return GetCardStatus((SD_CARD_STATUS *)pCardInfo);
}


SD_API_STATUS CSDDevice::InfoQuerySDIOInfo(PVOID pCardInfo, ULONG cbCardInfo )
{
    PREFAST_DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SDIO_CARD_INFO));
    UNREFERENCED_PARAMETER(cbCardInfo);
    
    if (Device_SD_IO != m_DeviceType || m_FuncionIndex==0) {
        DEBUGMSG(SDBUS_ZONE_REQUEST, (TEXT("CSDDevice::InfoQuerySDIOInfo: WARNING: Card is not a SDIO card or function index is zero!\n")));        
        return SD_API_STATUS_INVALID_PARAMETER;
    }
    
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;
    CSDDevice * device0 = m_sdSlot.GetFunctionDevice(0);
    if (device0) {
        __try {
            ((PSDIO_CARD_INFO)pCardInfo)->FunctionNumber = m_SDCardInfo.SDIOInformation.Function;
            ((PSDIO_CARD_INFO)pCardInfo)->DeviceCode = m_SDCardInfo.SDIOInformation.DeviceCode;
            ((PSDIO_CARD_INFO)pCardInfo)->CISPointer = m_SDCardInfo.SDIOInformation.CISPointer;
            ((PSDIO_CARD_INFO)pCardInfo)->CSAPointer = m_SDCardInfo.SDIOInformation.CSAPointer;

            DEBUGCHK(device0->m_SDCardInfo.SDIOInformation.pCommonInformation != NULL);

            ((PSDIO_CARD_INFO)pCardInfo)->CardCapability = 
                device0->m_SDCardInfo.SDIOInformation.pCommonInformation->CardCapability;
            status = SD_API_STATUS_SUCCESS;
        }
        __except(SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        device0->DeRef();
    }
    else {
        ASSERT(FALSE);
    }
    return status;
}


SD_API_STATUS CSDDevice::InfoQueryHostInterface(PVOID pHostInfo,ULONG cbHostInfo )
{
    PREFAST_DEBUGCHK(pHostInfo);
    DEBUGCHK(cbHostInfo == sizeof(SD_CARD_INTERFACE_EX));

    PSD_CARD_INTERFACE_EX pSlotCapabilities = (PSD_CARD_INTERFACE_EX)pHostInfo;
    
    // fetch the max clock rate
    pSlotCapabilities->ClockRate = m_sdSlot.MaxClockRate;

    // MMC 8 bit bus width support
    if((m_sdSlot.Capabilities & SD_SLOT_MMC_8BIT_CAPABLE) == SD_SLOT_MMC_8BIT_CAPABLE) {
        pSlotCapabilities->InterfaceModeEx.bit.mmc8Bit = 1;
    }
    else {
        pSlotCapabilities->InterfaceModeEx.bit.mmc8Bit = 0;
    }

    // SD/MMC 4 bit bus width support
    if( m_sdSlot.Capabilities & SD_SLOT_SD_4BIT_CAPABLE ) {
            pSlotCapabilities->InterfaceModeEx.bit.sd4Bit = 1;
    }
    else {
        pSlotCapabilities->InterfaceModeEx.bit.sd4Bit = 0;
    }

    // SD/MMC high speed support
    if( m_sdSlot.Capabilities & SD_SLOT_HIGH_SPEED_CAPABLE ) {
            pSlotCapabilities->InterfaceModeEx.bit.sdHighSpeed = 1;
    }
    else {
        pSlotCapabilities->InterfaceModeEx.bit.sdHighSpeed = 0;
    }
    
    // write protect is meaningless for a capability query, set to FALSE
    pSlotCapabilities->InterfaceModeEx.bit.sdWriteProtected = FALSE;    

    return SD_API_STATUS_SUCCESS;
}


SD_API_STATUS CSDDevice::InfoQueryBlockCaps(PVOID pCardInfo,ULONG cbCardInfo )
{
    DEBUGCHK(pCardInfo);
    DEBUGCHK(cbCardInfo == sizeof(SD_HOST_BLOCK_CAPABILITY));
    UNREFERENCED_PARAMETER(cbCardInfo);
    SD_HOST_BLOCK_CAPABILITY sdHostBLockCap = *(PSD_HOST_BLOCK_CAPABILITY)pCardInfo;
    // send the requested block transfer size to the HC, if the HC is
    // unable to support the requested block size it will return the 
    // next supported block size smaller than that requested
    SD_API_STATUS status = m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(),
        SDHCDQueryBlockCapability,
        &sdHostBLockCap,                
        sizeof(SD_HOST_BLOCK_CAPABILITY));
    if (SD_API_SUCCESS(status)) {
        *(PSD_HOST_BLOCK_CAPABILITY)pCardInfo = sdHostBLockCap;
    }
    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDCardInfoQuery__X - Obtain Card information
//  Input:  hHandle        - SD Device Handle
//          InfoType       - information to get
//          StructureSize  - size of info structure
//  Output: pCardInfo      - Information specific structure 
//  Return: SD_API_STATUS code
//  Notes:  pCardInfo must point to sufficient memory for the informtion type
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDCardInfoQuery_I( IN  SD_INFO_TYPE     InfoType,
                                 OUT PVOID            pCardInfo,
                                 IN  ULONG            StructureSize)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDCard: +SDCardInfoQuery\n")));
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;
    __try {
        switch (InfoType) {
          case SD_INFO_REGISTER_OCR:
            break;
          case SD_INFO_REGISTER_CID:
            if (StructureSize>=sizeof(SD_PARSED_REGISTER_CID) && pCardInfo!=NULL) {
                status = InfoQueryCID(pCardInfo, StructureSize);
            }
            break;
          case  SD_INFO_REGISTER_CSD: 
            if (StructureSize>=sizeof(SD_PARSED_REGISTER_CSD) && pCardInfo!=NULL ) {
                status = InfoQueryCSD(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_REGISTER_RCA:
            if (StructureSize>=sizeof(SD_CARD_RCA) && pCardInfo!=NULL) {
                status = InfoQueryRCA(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_REGISTER_IO_OCR:
            break;
          case SD_INFO_CARD_INTERFACE:
            if (StructureSize>=sizeof(SD_CARD_INTERFACE) && pCardInfo!=NULL) {
                SD_CARD_INTERFACE_EX sdCardInterfacEx;
                status = InfoQueryCardInterface(&sdCardInterfacEx, sizeof(sdCardInterfacEx));
                if (SD_API_SUCCESS(status)) {
                    *(PSD_CARD_INTERFACE)pCardInfo = ConvertFromEx(sdCardInterfacEx);
                }
            }
            break;
          case SD_INFO_CARD_INTERFACE_EX:
            if (StructureSize>=sizeof(SD_CARD_INTERFACE_EX) && pCardInfo!=NULL) {
                status = InfoQueryCardInterface((PSD_CARD_INTERFACE_EX)pCardInfo, sizeof(SD_CARD_INTERFACE_EX));
            }
            break;
          case SD_INFO_CARD_STATUS:
            if (StructureSize>=sizeof(SD_CARD_STATUS)&& pCardInfo!=NULL) {
                status = InfoQueryStatus(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_SDIO:
            if ( StructureSize>=sizeof(SDIO_CARD_INFO) && pCardInfo!=NULL) {
                status = InfoQuerySDIOInfo(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_HOST_IF_CAPABILITIES:
            if ( StructureSize>=sizeof(SD_CARD_INTERFACE_EX)  && pCardInfo!=NULL) {
                status = InfoQueryHostInterface(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_HOST_BLOCK_CAPABILITY:
            if (StructureSize >= sizeof(SD_HOST_BLOCK_CAPABILITY) && pCardInfo!=NULL) {
                status = InfoQueryBlockCaps(pCardInfo, StructureSize);
            }
            break;
          case SD_INFO_HIGH_CAPACITY_SUPPORT:
            if (StructureSize>= sizeof(DWORD) && pCardInfo!=NULL) {
                SD_CARD_INTERFACE_EX sdCardInterfacEx;
                status = InfoQueryCardInterface(&sdCardInterfacEx, sizeof(sdCardInterfacEx));
                if (SD_API_SUCCESS(status)) {
                    *(PDWORD)pCardInfo = sdCardInterfacEx.InterfaceModeEx.bit.sdHighCapacity;
                }
            };
            break;
          case SD_INFO_SWITCH_FUNCTION:
            if (StructureSize>= sizeof(SD_CARD_SWITCH_FUNCTION) && pCardInfo!=NULL) {
                status =SwitchFunction((PSD_CARD_SWITCH_FUNCTION)pCardInfo, TRUE);
            }
            break;
        }
    }__except(SDProcessException(GetExceptionInformation())) {
        status = SD_API_STATUS_ACCESS_VIOLATION;
    };

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDCard: -SDCardInfoQuery status = %x\n"),status));
    return status;
}


// Shifts pbInput down by dwBitOffset.
static
VOID
ShiftBytes(__in_bcount(cbInput) BYTE const*const pbInput, ULONG cbInput, DWORD dwBitOffset, __out_bcount(sizeof(DWORD)) PBYTE pbOutput) 
{
    PREFAST_DEBUGCHK(pbInput);
    PREFAST_DEBUGCHK(pbOutput);

    DWORD dwByteIndex = dwBitOffset / 8;
    dwBitOffset %= 8;

    DWORD dwRemainderShift = 8 - dwBitOffset;

    // Only copy 4 bytes max.
    DWORD dwEndIndex = min(dwByteIndex + sizeof(DWORD), cbInput);
    DWORD dwCurrOutputIndex = 0;
    while (dwByteIndex < dwEndIndex) {
        DEBUGCHK(dwCurrOutputIndex < sizeof(DWORD));
        DEBUGCHK(dwByteIndex < cbInput);

        pbOutput[dwCurrOutputIndex] = pbInput[dwByteIndex] >> dwBitOffset;

        ++dwByteIndex;

        if (dwByteIndex != cbInput) {
            BYTE bTemp = pbInput[dwByteIndex];
            bTemp <<= dwRemainderShift;
            pbOutput[dwCurrOutputIndex] |= bTemp;
        }

        ++dwCurrOutputIndex;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  GetBitSlice - Get a bit slice from a stream of bytes
//  Input:  pBuffer - buffer containing data stream
//          cbBuffer - size of buffer in bytes
//          dwBitOffset - bit offset from start of buffer
//          ucBitCount - number of bits (less than or equal to 32)
//  Output: 
//
//  Return: returns a DWORD contain the bit slice shifted to fill the least significant bits
//  Notes:  will raise an SEH exception if integer overflow occurs
///////////////////////////////////////////////////////////////////////////////
DWORD CSDDevice::GetBitSlice(__in_bcount(cbBuffer) UCHAR const*const pBuffer, ULONG cbBuffer, DWORD dwBitOffset, UCHAR ucBitCount)
{
    UCHAR rgbShifted[4] = { 0 };

    if (ucBitCount > 32) {
        DEBUG_CHECK(ucBitCount <= 32, (TEXT("GetBitSlice: invalid number of bits \n")));
        return 0;
    }

    typedef SafeInt<DWORD> SafeDW;
    // Exception will be raised on the next line if there is an overflow.
    if ( (SafeDW(dwBitOffset) + SafeDW(ucBitCount)) > (SafeDW(cbBuffer) * 8) ) {
        DEBUG_CHECK((SafeDW(dwBitOffset) + SafeDW(ucBitCount)) <= (SafeDW(cbBuffer) * 8),
            (TEXT("GetBitSlice: invalid bit offset given the number of bits \n")));
        return 0;
    }

    // Shift the pBuffer down by dwBitOffset bits.
    ShiftBytes(pBuffer, cbBuffer, dwBitOffset, rgbShifted);

    DWORD dwUsedBytes; // How many bytes have valid data.

    if (ucBitCount % 8 == 0) {
        // Return a byte multiple.
        dwUsedBytes = ucBitCount / 8;
    }
    else {
        // Clear the last used byte of upper bits.
        DWORD dwLastByteIndex = (ucBitCount - 1) / 8;
        DWORD dwRemainderShift = 8 - (ucBitCount % 8);
        rgbShifted[dwLastByteIndex] <<= dwRemainderShift;
        rgbShifted[dwLastByteIndex] >>= dwRemainderShift;
        dwUsedBytes = dwLastByteIndex + 1;
    }

    // Clear the unused bytes.
    if (dwUsedBytes != sizeof(rgbShifted)) {
        memset(rgbShifted + dwUsedBytes, 0, sizeof(rgbShifted) - dwUsedBytes);
    }

    DWORD dwRet;
    memcpy(&dwRet, rgbShifted, sizeof(dwRet));

    return dwRet;
}

