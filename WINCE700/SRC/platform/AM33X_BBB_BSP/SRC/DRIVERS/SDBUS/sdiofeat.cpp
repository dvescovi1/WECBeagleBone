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
    SDIOFeat.cpp
Abstract:
    SDBus SDIO Setup Feature Implementation.

Notes: 
--*/

#include <windows.h>

#include <sdhcd.h>

#include "sdbus.hpp"
#include "sdslot.hpp"
//#include "sdbusreq.hpp"
#include "sddevice.hpp"

///////////////////////////////////////////////////////////////////////////////
//  SDEnableFunction  - enable/disable the device function 
//  Input:  pDevice   - the device 
//          pInfo   - enable info (required if Enable is TRUE)
//          Enable  - enable
//  Output: 
//  Return: SD_API_STATUS code
//          
//  Notes: 
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDEnableDisableFunction(PSD_IO_FUNCTION_ENABLE_INFO pInfo,BOOL Enable)
{
    UCHAR                       regValue;       // temp register value
    ULONG                       retryCount;     // ready retry count
    SD_API_STATUS               status;         // intermediate status
    BOOL                        fSkipIfMatch;   // Test for setting the same bit twice
    FUNCTION_POWER_STATE        PowerState;     // The function's power state

    if (m_FuncionIndex == 0 ) {
        DEBUGCHK(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDEnableDisableFunction: Enabling/Disabling SDIO Device Function %d \n"), m_FuncionIndex));
    

        // Get the functions power state
    status = GetFunctionPowerState(&PowerState);

    if (!SD_API_SUCCESS(status)) {
        return status;
    }

    // First check if states already match
    fSkipIfMatch = FALSE;
    if (PowerState.fFunctionEnabled == Enable) {
        if (Enable) { 
            DEBUGMSG(SDCARD_ZONE_WARN,(TEXT("SDEnableDisableFunction: Attempting to enable function that is already enabled \n")));
            fSkipIfMatch = TRUE;
        }
        else {
            DEBUGMSG(SDCARD_ZONE_WARN,(TEXT("SDEnableDisableFunction: Attempting to disable function that is already disabled \n")));
            fSkipIfMatch = TRUE;
        }
    }
        
    if (!fSkipIfMatch) {
        // Attempt to change cards power draw
        status = m_sdSlot.GetHost().ChangeCardPowerHandler(m_sdSlot.GetSlotIndex(),PowerState.EnableDelta);

        if (!SD_API_SUCCESS(status)) {
            return status;
        }

            //update the power used at the slot
        {
            INT SlotPower = m_sdSlot.GetSlotPower();
            SlotPower += PowerState.EnableDelta;
            m_sdSlot.SetSlotPower( SlotPower >= 0? (USHORT)SlotPower : 0 );
        }

        CSDDevice *pDevice0 = m_sdSlot.GetFunctionDevice(0 );
        status = SD_API_STATUS_NO_SUCH_DEVICE;
        if (pDevice0) {
                // update the parent device shadow register
            if (Enable) {     
                pDevice0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIOEnable 
                    |= (1 <<  m_SDCardInfo.SDIOInformation.Function);
            } else {
                pDevice0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIOEnable &= 
                    ~(1 <<  m_SDCardInfo.SDIOInformation.Function);
            }
            // get a copy
            regValue = pDevice0->m_SDCardInfo.SDIOInformation.pCommonInformation->CCCRShadowIOEnable;

            // update the register
            status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_WRITE, SD_IO_REG_ENABLE,FALSE, &regValue, 1);
            pDevice0->DeRef();
        }        
        if (!SD_API_SUCCESS(status)) {
            return status;
        }
    }

    // if enabling we check for I/O ready
    if (Enable) { 
        CSDDevice *pDevice0 = m_sdSlot.GetFunctionDevice(0 );
        status = SD_API_STATUS_NO_SUCH_DEVICE;
        if (pDevice0) {
            retryCount = pInfo->ReadyRetryCount;

            while (retryCount) {
                // delay the interval time
                Sleep(pInfo->Interval);

                // read the I/O ready register
                status = pDevice0->SDReadWriteRegistersDirect_I( SD_IO_READ, SD_IO_REG_IO_READY,FALSE,&regValue,1);         // one byte
                if (!SD_API_SUCCESS(status)) {
                    break;
                }

                // see if it is ready
                if (regValue & (1 << m_SDCardInfo.SDIOInformation.Function)) {
                    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDEnableDisableFunction: Card Function %d is now ready \n"),
                        m_SDCardInfo.SDIOInformation.Function));
                    break;
                }
                // decrement the count
                retryCount--; 
                DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDEnableDisableFunction: Card Function %d, Not Ready, re-checking (%d) \n"),
                    m_SDCardInfo.SDIOInformation.Function, retryCount));
            }

            if (0 == retryCount) {
                DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDEnableDisableFunction: Card Function %d, Not ready , exceeded retry count\n"),
                    m_SDCardInfo.SDIOInformation.Function));
                status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
            }
            pDevice0->DeRef();
        }
    }

    return status;
} 


///////////////////////////////////////////////////////////////////////////////
//  SDFunctionSelectPower  - switch device function to High or Low power state
//  Input:  pDevice   - the device 
//          fLowPower - High or Low Power state
//  Output: 
//  Return: SD_API_STATUS code
//          
//  Notes: 
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDFunctionSelectPower( BOOL  fLowPower)
{
    UCHAR                       regValue;       // temp register value
    SD_API_STATUS               status;         // intermediate status
    FUNCTION_POWER_STATE        PowerState;     // The function's power state
    DWORD                       FBROffset;      // calculated FBR offset


        // get the parent device
    
    if (0 == m_FuncionIndex) {
        DEBUG_ASSERT(FALSE);
        return SD_API_STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDFunctionSelectPower: SDIO Device Function %d Power Select\n"),
            m_SDCardInfo.SDIOInformation.Function));

        // Get the functions power state
    status = GetFunctionPowerState(&PowerState);

    if (!SD_API_SUCCESS(status)) {
        return status;
    }

    //check if power selection is supported
    if((!PowerState.fPowerControlSupport) || (!PowerState.fSupportsPowerSelect)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDFunctionSelectPower: Card or Function does not support Power Select.\n")));
        return SD_API_STATUS_INVALID_DEVICE_REQUEST;
    }
    // Check if states already match
    if (PowerState.fLowPower == fLowPower){
        if (fLowPower) { 
            DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDFunctionSelectPower: Attempting to select low power state when that is already enabled \n")));
        }
        else{
            DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDFunctionSelectPower: Attempting to select high power state when that is already enabled \n")));
        }
        return SD_API_STATUS_SUCCESS;
    }
        
    // Attempt to change cards power draw
    status = m_sdSlot.GetHost().ChangeCardPowerHandler(m_sdSlot.GetSlotIndex(),PowerState.SelectDelta);
    if (!SD_API_SUCCESS(status)) {
        return status;
    }

    //update the power used at the slot
    {
        INT SlotPower = m_sdSlot.GetSlotPower();
        SlotPower += PowerState.SelectDelta;
        m_sdSlot.SetSlotPower( SlotPower >= 0? (USHORT)SlotPower : 0 );
    }

    CSDDevice *pDevice0 = m_sdSlot.GetFunctionDevice(0 );
    status = SD_API_STATUS_NO_SUCH_DEVICE;
    if (pDevice0) {
        // select the function's power state
        FBROffset = SD_IO_FBR_1_OFFSET + (m_SDCardInfo.SDIOInformation.Function - 1) * SD_IO_FBR_LENGTH;
        status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_READ,FBROffset + SD_IO_FBR_POWER_SELECT,FALSE,&regValue,1); 

        if (SD_API_SUCCESS(status)) {
            if(fLowPower) {
                regValue |= SD_IO_FUNCTION_POWER_SELECT_STATE;
            }
            else {
                regValue &= ~SD_IO_FUNCTION_POWER_SELECT_STATE;
            }

            status = pDevice0->SDReadWriteRegistersDirect_I(SD_IO_WRITE,FBROffset + SD_IO_FBR_POWER_SELECT,FALSE,&regValue,1);
        }
        pDevice0->DeRef();
    }
    return status;
} 

///////////////////////////////////////////////////////////////////////////////
//  SDSetFunctionBlockSize  - set the block size of the function
//  Input:  pDevice   - the device 
//          BlockSize - block size to set
//  Output: 
//  Return: SD_API_STATUS code
//          
//  Notes: 
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDSetFunctionBlockSize(DWORD BlockSize)
{
    USHORT bytesPerBlock = (USHORT)BlockSize; // desired block size
    DWORD                  FBROffset;         // FBR offset
    SD_API_STATUS          status;
    
    DEBUGCHK(0 != m_SDCardInfo.SDIOInformation.Function);
    // calculate the FBR offset based on the function number
    FBROffset = SD_IO_FBR_1_OFFSET + (m_SDCardInfo.SDIOInformation.Function - 1) * SD_IO_FBR_LENGTH;
    CSDDevice *pDevice0 = m_sdSlot.GetFunctionDevice(0 );
    status = SD_API_STATUS_NO_SUCH_DEVICE;
    
    if (pDevice0) {
        // update the register
        status = pDevice0->SDReadWriteRegistersDirect_I( SD_IO_WRITE, FBROffset + SD_IO_FBR_IO_BLOCK_SIZE, FALSE,(PUCHAR)&bytesPerBlock,sizeof(USHORT));           // two bytes    
        pDevice0->DeRef();
    }
    
    return status;
}
SD_API_STATUS CSDDevice::SetCardFeature_Interface(SD_CARD_INTERFACE_EX& CardInterfaceEx)
{
    SD_API_STATUS               status = SD_API_STATUS_SUCCESS;  // intermediate status
    // Check if the slot can accept this interface request
    // For multifunction card or combo card, the requested interface may not be fitted
    // for other functions.
    {
        BOOL bAllFunctionsAcceptThisInterface = TRUE;

        // Start from parent device
        BOOL fContinue = TRUE; 
        for (DWORD dwIndex = 0; dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT && fContinue ; dwIndex++) {
            CSDDevice * pDevice = m_sdSlot.GetFunctionDevice(dwIndex);
            if (pDevice != NULL ) {
                if (dwIndex!=GetDeviceFuncionIndex() &&  pDevice->GetDeviceType() != Device_Unknown ) {
                    // Check if current device supports 4 bit mode request
                    if (CardInterfaceEx.InterfaceModeEx.bit.sd4Bit && !(pDevice->m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit)) {
                        bAllFunctionsAcceptThisInterface = FALSE;
                        fContinue = FALSE;
                    }
                    else
                    // Check if request clock rate is too high for this device
                    if (CardInterfaceEx.ClockRate > pDevice->m_CardInterfaceEx.ClockRate) {
                        bAllFunctionsAcceptThisInterface = FALSE;
                        fContinue = FALSE;
                    }
                    else 
                    if (!(CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed) && pDevice->m_CardInterfaceEx.InterfaceModeEx.bit.sdHighSpeed){
                        bAllFunctionsAcceptThisInterface = FALSE;
                        fContinue = FALSE;
                    }
                }
                pDevice->DeRef();
            }
        }

        if (bAllFunctionsAcceptThisInterface == FALSE) {
            DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_SET_CARD_INTERFACE - invalod interface request\n")));
            return SD_API_STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    // Changing the bus width is tricky when SDIO interrupts are 
    // enabled. In 1-bit mode, DAT[1] is used as the interrupt line.
    // In 4-bit mode, DAT[1] is used for data and interrupts. If
    // we change from 1-bit mode to 4-bit mode while interrupts are 
    // occurring (like when a BTH mouse is being moved franticly), we
    // need to disable SDIO interrupts while we are changing the mode
    // on both the host controller and the card. Otherwise an interrupt in
    // the middle could confuse the host controller.

    PSD_INTERRUPT_CALLBACK pInterruptCallBack = NULL;
    if ( (Device_SD_IO == m_DeviceType) && m_sdSlot.IsSlotInterruptOn() && 
             (m_CardInterfaceEx.InterfaceModeEx.bit.sd4Bit != CardInterfaceEx.InterfaceModeEx.bit.sd4Bit) ) {
        // Temporarily disable SDIO interrupts
        pInterruptCallBack = m_SDCardInfo.SDIOInformation.pInterruptCallBack;
        DEBUGCHK(pInterruptCallBack);
        SDIOConnectDisconnectInterrupt( NULL, FALSE);
    }
    SD_CARD_INTERFACE_EX CardInterfaceExBackup = m_CardInterfaceEx;
    BOOL isRestore = FALSE;
    for (;;) {
        SD_API_STATUS inStatus = SD_API_STATUS_SUCCESS ;
        
        // check for success
        BOOL fContinue = TRUE;
        for (DWORD dwIndex = 0; dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT && fContinue; dwIndex++) {
            CSDDevice * pDevice = m_sdSlot.GetFunctionDevice(dwIndex);
            if (pDevice != NULL) {
                inStatus = pDevice->SetCardInterface(&CardInterfaceEx);
                if (!SD_API_SUCCESS(inStatus)) {
                    fContinue = FALSE;
                }
                pDevice->DeRef();
            }
        }

        if (SD_API_SUCCESS(inStatus)) {
            // set the card interface for device's slot        
            inStatus = m_sdSlot.GetHost().SlotSetupInterface(m_sdSlot.GetSlotIndex(),&CardInterfaceEx);
            //m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(), SDHCDSetSlotInterface, &CardInterface, sizeof(CardInterface));
            fContinue = SD_API_SUCCESS(inStatus) ;
        }
            
        if (!SD_API_SUCCESS(inStatus) &&!isRestore ) {
            ASSERT(FALSE);
            status = inStatus;
            CardInterfaceEx = CardInterfaceExBackup;
            isRestore = TRUE;
        }
        else 
            break;
    }
    if (pInterruptCallBack) {
        // Re-enable SDIO interrupts
        DEBUGCHK(!m_sdSlot.IsSlotInterruptOn());
        DEBUGCHK(Device_SD_IO == m_DeviceType);
        SDIOConnectDisconnectInterrupt(pInterruptCallBack, TRUE);
    }
    return status;

}
///////////////////////////////////////////////////////////////////////////////
//  SDSetCardFeature       - Set card feature
//  Input:  hDevice        - SD Device Handle
//          CardFeature    - Card Feature to set
//          StructureSize  - size of card feature structure
//  Output: pCardInfo      - Information for the feature
//  Return: SD_API_STATUS code
//  Notes:  This function is provided to set various card features
//          in a thread safe manner.  SDIO cards utilize shared register sets
//          between functions. This requires that the 
//          register state be preserved between functions that can be 
//          controlled in separate thread contexts.
//          This function can potentially block by issuing synchronous bus 
//          request.  This function must not be called from a bus request callback
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDDevice::SDSetCardFeature_I(SD_SET_FEATURE_TYPE  CardFeature,PVOID pCardInfo,ULONG StructureSize)
{
    SD_API_STATUS               status = SD_API_STATUS_SUCCESS;  // intermediate status
    PSD_DATA_TRANSFER_CLOCKS    pClocks;                         // data transfer clocks variable  

    switch (CardFeature) {

      case SD_IO_FUNCTION_ENABLE:
        if ((sizeof(SD_IO_FUNCTION_ENABLE_INFO) != StructureSize) || (NULL == pCardInfo)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_IO_FUNCTION_ENABLE - Invalid params \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }        
        status = SDEnableDisableFunction((PSD_IO_FUNCTION_ENABLE_INFO)pCardInfo, TRUE);
        break;
        
      case SD_IO_FUNCTION_DISABLE:
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        status = SDEnableDisableFunction(NULL, FALSE);
        break;
      case SD_IO_FUNCTION_HIGH_POWER:         
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        status = SDFunctionSelectPower(FALSE);
        break;

      case SD_IO_FUNCTION_LOW_POWER:
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        status = SDFunctionSelectPower(TRUE);
        break;

      case SD_INFO_POWER_CONTROL_STATE:
        if ((sizeof(FUNCTION_POWER_STATE) != StructureSize) || (NULL == pCardInfo)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_INFO_POWER_CONTROL_STATE - Invalid params \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }

        status = GetFunctionPowerState((PFUNCTION_POWER_STATE)pCardInfo);
        break;

      case SD_IO_FUNCTION_SET_BLOCK_SIZE:
        if ((sizeof(DWORD) != StructureSize) || (NULL == pCardInfo)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_IO_FUNCTION_SET_BLOCK_SIZE - Invalid params \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        if (Device_SD_IO != m_DeviceType) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: device is not SDIO ! \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }
        status = SDSetFunctionBlockSize(*((DWORD *)pCardInfo));
        break;

      case SD_SET_DATA_TRANSFER_CLOCKS:
        if ((sizeof(SD_DATA_TRANSFER_CLOCKS) != StructureSize) || (NULL == pCardInfo)) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_SET_DATA_TRANSFER_CLOCKS - Invalid params \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }

        pClocks = (PSD_DATA_TRANSFER_CLOCKS)pCardInfo;
        m_SDCardInfo.SDMMCInformation.DataAccessReadClocks = pClocks->ReadClocks;
        m_SDCardInfo.SDMMCInformation.DataAccessWriteClocks = pClocks->WriteClocks;
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_IS_FAST_PATH_AVAILABLE:
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_FAST_PATH_DISABLE:
        //  Disable the use of Fast-Path for testing.
        m_SDCardInfo.SDIOInformation.Flags |= FSTPTH_DISABLE;
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_FAST_PATH_ENABLE:
        //  Always use Fast-Path operations.
        m_SDCardInfo.SDIOInformation.Flags &= ~ FSTPTH_DISABLE;
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_IS_SOFT_BLOCK_AVAILABLE:
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_SOFT_BLOCK_FORCE_UTILIZATION:
        //  Always use Soft-Block operations.
        m_SDCardInfo.SDIOInformation.Flags |= SFTBLK_USE_ALWAYS;
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_SOFT_BLOCK_DEFAULT_UTILIZATON:
        //  Use hardware multi-block operations if supported by the card,
        //  otherwise use Soft-Block.
        m_SDCardInfo.SDIOInformation.Flags &= ~ SFTBLK_USE_ALWAYS;
        status = SD_API_STATUS_SUCCESS;
        break;

      case SD_SET_CARD_INTERFACE: {
        if ((sizeof(SD_CARD_INTERFACE) != StructureSize) || (NULL == pCardInfo)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_SET_CARD_INTERFACE - Invalid params \n")));
                return SD_API_STATUS_INVALID_PARAMETER;
        }
        PSD_CARD_INTERFACE pCardInterface = (PSD_CARD_INTERFACE) pCardInfo ;
        SD_CARD_INTERFACE_EX sdCardInterfaceEx;
        memset (&sdCardInterfaceEx, 0, sizeof(sdCardInterfaceEx));
        sdCardInterfaceEx.ClockRate = pCardInterface->ClockRate;
        sdCardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected = (pCardInterface->WriteProtected?1:0);
        sdCardInterfaceEx.InterfaceModeEx.bit.sd4Bit = (pCardInterface->InterfaceMode == SD_INTERFACE_SD_4BIT?1:0) ;
        status =SetCardFeature_Interface(sdCardInterfaceEx);
        break;
      }
     case SD_SET_CARD_INTERFACE_EX: {
        if ((sizeof(SD_CARD_INTERFACE_EX) != StructureSize) || (NULL == pCardInfo)) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDSetCardFeature: SD_SET_CARD_INTERFACE_EX - Invalid params \n")));
                return SD_API_STATUS_INVALID_PARAMETER;
        }
        status = SetCardFeature_Interface(*(PSD_CARD_INTERFACE_EX)pCardInfo);
        break;
      }

      case SD_SET_CLOCK_STATE_DURING_IDLE:
        if ( (sizeof(BOOL) != StructureSize) || (NULL == pCardInfo) ) {
            DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDSetCardFeature: SD_SET_CLOCK_ON_DURING_IDLE - Invalid params \n")));
            return SD_API_STATUS_INVALID_PARAMETER;
        }

        // prompt the host to turn on or off the clock during the idle state based on the client's
        // request.
        status = m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(), SDHCDSetClockStateDuringIdle, pCardInfo,StructureSize);

        DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDSetCardFeature: SDHCDSetClockStateDuringIdle finished with status: %x\n"),
            status));
        break;
      case SD_CARD_FORCE_RESET:
        DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDSetCardFeature: call SD_CARD_FORCE_RESET \n")));
        m_sdSlot.PostEvent(SlotResetRequest);
        break;

      case SD_CARD_SELECT_REQUEST:
        // request made by client driver to select the card. The request will not be honored
        // until all client drivers in this slot make such request.
        {
            BOOL bAllFunctionsRequestedCardSelect  = TRUE;;
            DbgPrintZo(SDCARD_ZONE_INIT,(TEXT("SDSetCardFeature: call SD_CARD_SELECT_REQUEST \n")));
            m_bCardSelectRequest = TRUE;
            NotifyClient(SDCardSelectRequest);
            for (DWORD dwIndex = 0; dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT; dwIndex++) {
                CSDDevice * pDevice = m_sdSlot.GetFunctionDevice(dwIndex);
                if (pDevice != NULL) {
                    if (pDevice->m_bCardSelectRequest == FALSE && pDevice->GetDeviceType()!= Device_Unknown ) {
                        bAllFunctionsRequestedCardSelect = FALSE;
                    }
                    pDevice->DeRef();
                }
            }
            if (bAllFunctionsRequestedCardSelect == FALSE) {
                DbgPrintZo(SDCARD_ZONE_INFO, (TEXT("SDSetCardFeature: SD_CARD_SELECT_REQUEST - request is pending\n")));
                return SD_API_STATUS_PENDING;
            }
            DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDSetCardFeature: SD_CARD_SELECT_REQUEST - request is processing\n")));
            m_sdSlot.PostEvent(SlotSelectRequest);
        }
        break;
      case SD_CARD_DESELECT_REQUEST:
        {
            BOOL bAllFunctionsRequestedCardDeselect= TRUE;;
            DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDSetCardFeature: call SD_CARD_DESELECT_REQUEST \n")));
            if (!m_bCardDeselectRequest) {
                m_bCardDeselectRequest = TRUE;
                NotifyClient(SDCardDeselectRequest);
            }
            for (DWORD dwIndex = 0; dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT; dwIndex++) {
                CSDDevice * pDevice = m_sdSlot.GetFunctionDevice(dwIndex);
                if (pDevice != NULL) {
                    if (pDevice->m_bCardDeselectRequest == FALSE && pDevice->GetDeviceType()!= Device_Unknown) {
                        bAllFunctionsRequestedCardDeselect = FALSE ;
                    }
                    pDevice->DeRef();
                }
            }
            if (bAllFunctionsRequestedCardDeselect == FALSE) {
                DbgPrintZo(SDCARD_ZONE_INFO, (TEXT("SDSetCardFeature: SD_CARD_DESELECT_REQUEST - request is pending\n")));
                return SD_API_STATUS_PENDING;
            }
            DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDSetCardFeature: SD_CARD_DESELECT_REQUEST - request is processing\n")));
            m_sdSlot.PostEvent(SlotDeselectRequest);
        }
        break;
      case SD_SET_SWITCH_FUNCTION: {
        if (pCardInfo!=NULL && StructureSize >= sizeof(SD_CARD_SWITCH_FUNCTION)) {
            status = SwitchFunction((PSD_CARD_SWITCH_FUNCTION)pCardInfo,FALSE);
        }
      }
      break;
      case SD_DMA_ALLOC_PHYS_MEM:
        if (pCardInfo!=NULL && StructureSize >= sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER)) {
            status = m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(),SDHCAllocateDMABuffer, pCardInfo,sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER));                
        }
        break;
      case SD_DMA_FREE_PHYS_MEM:
        if (pCardInfo!=NULL && StructureSize >= sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER)) {
            status = m_sdSlot.GetHost().SlotOptionHandler(m_sdSlot.GetSlotIndex(),SDHCFreeDMABuffer, pCardInfo,sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER));                
        }
        break;
    default:
        status = SD_API_STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}



