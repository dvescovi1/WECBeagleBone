// All rights reserved ADENEO EMBEDDED 2010
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
// Module Name:  
//     OTG.cpp
// 
// Abstract: Provides Library for OTG Module.
// 
// Notes: 
//
#include <windows.h>
#include <types.h>
//#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include "tps659xx_musb.hpp"
#include <tps659xxusb.h>
#include <tps659xx_internals.h>
#include <initguid.h>
#include <twl.h>
#include <triton.h>
#include <oal.h>



#ifndef DEBUG
#define ZONE_OTG_ERROR DEBUGZONE(15)
#define ZONE_OTG_WARN DEBUGZONE(1)
#define ZONE_OTG_FUNCTION DEBUGZONE(2)
#define ZONE_OTG_INIT DEBUGZONE(3)
#define ZONE_OTG_INFO DEBUGZONE(4)
#define ZONE_OTG_THREAD DEBUGZONE(8)
#define ZONE_OTG_HNP DEBUGZONE(9)
#else
#define ZONE_OTG_ERROR DEBUGZONE(15)
#define ZONE_OTG_FUNCTION DEBUGZONE(2)
#define ZONE_OTG_INFO DEBUGZONE(4)
#define ZONE_OTG_HNP DEBUGZONE(9)
#endif

EXTERN_C HSUSBOTGTransceiver * CreateHSUSBOTGTransceiver(PHSMUSB_T pOTG)
{
    return new HSUSBOTGTransceiverTps659xx(pOTG);
}

BOOL 
HSUSBOTGTransceiverTps659xx::ReadOmapTriton2(
    DWORD reg, 
    BYTE *pData
) 
{
    PREFAST_ASSERT(m_hTriton2Handle != NULL);
    return (TWLReadRegs(m_hTriton2Handle, reg, pData, sizeof(BYTE)));
}

BOOL 
HSUSBOTGTransceiverTps659xx::WriteOmapTriton2(
    DWORD reg, 
    BYTE Data
) 
{
    PREFAST_ASSERT(m_hTriton2Handle != NULL);
    return (TWLWriteRegs(m_hTriton2Handle, reg, &Data, sizeof(Data)));
}


#define CARKIT_CTRL_CARKITPWR       (1 << 0)
#define CARKIT_CTRL_TXDEN           (1 << 2)
#define CARKIT_CTRL_RXDEN           (1 << 3)
#define CARKIT_CTRL_SPKLEFTEN       (1 << 4)
#define CARKIT_CTRL_MICEN           (1 << 6)



void HSUSBOTGTransceiverTps659xx::Configure()
{
    BYTE  bModeBits;

    if (!m_hTriton2Handle)
    {
        DEBUGMSG(ZONE_OTG_ERROR, (TEXT("Failed to configure the T2 board\r\n")));
        return;
    }
    
    // set for interrupt from ULPI Rx CMD to Link  
    bModeBits = OTHER_IFC_CTRL_ALT_INT_REROUTE;
    WriteOmapTriton2(TWL_OTHER_IFC_CTRL_CLR, bModeBits);
    
    // Set the DP/DM
    bModeBits = OTHER_INT_EN_RISE_DM_HI | OTHER_INT_EN_RISE_DP_HI;
    WriteOmapTriton2(TWL_OTHER_INT_EN_RISE_SET, bModeBits);
    WriteOmapTriton2(TWL_OTHER_INT_EN_FALL_SET, bModeBits);
    
    // Setting to IFC_CTRL register for ULPI operation mode
    bModeBits = IFC_CTRL_FSLSSERIALMODE_3PIN | IFC_CTRL_CARKITMODE;
    WriteOmapTriton2(TWL_IFC_CTRL_CLR, bModeBits);
    
    // Setting to TWL_CARKIT_CTRL_CLR register for ULPI operation mode
    bModeBits = CARKIT_CTRL_CARKITPWR |CARKIT_CTRL_TXDEN |CARKIT_CTRL_RXDEN;
    bModeBits |= CARKIT_CTRL_SPKLEFTEN |CARKIT_CTRL_MICEN;
    WriteOmapTriton2(TWL_CARKIT_CTRL_CLR, bModeBits);
    
    
    {
        BYTE bCarKitReg, bIfcCtrlReg;
        
        ReadOmapTriton2(TWL_CARKIT_CTRL, &bCarKitReg);
        ReadOmapTriton2(TWL_IFC_CTRL, &bIfcCtrlReg);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver: CARKIT_Reg(0x%x), IFC_CTRL_Reg(0x%x)\r\n"), 
            bCarKitReg, bIfcCtrlReg));
        
        ReadOmapTriton2(TWL_USB_INT_EN_RISE, &bCarKitReg);
        ReadOmapTriton2(TWL_USB_INT_EN_FALL, &bIfcCtrlReg);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver: TWL_USB_INT_EN_RISE(0x%x), TWL_USB_INT_EN_FALL(0x%x)\r\n"), 
            bCarKitReg, bIfcCtrlReg));
    }
    
    
    {
        UCHAR sts;
        UCHAR ctrl;
        
        // Lock PHY: start
        if (ReadOmapTriton2(TWL_PHY_CLK_CTRL_STS, &sts))
        {
            int iCount = 500;
            if (((sts & PHY_CLK_CTRL_STS_PHY_DPLL_LOCK) == 0) && ReadOmapTriton2(TWL_PHY_CLK_CTRL, &ctrl))
            {
                BOOL fSuccess;                
                do 
                {
                    ctrl |= PHY_CLK_CTRL_REQ_PHY_DPLL_CLK;                
                    WriteOmapTriton2(TWL_PHY_CLK_CTRL, ctrl);
                    sts = 0x00;
                    if (ReadOmapTriton2(TWL_PHY_CLK_CTRL_STS, &sts) &&(sts & PHY_CLK_CTRL_STS_PHY_DPLL_LOCK))
                    {
                        break;
                    }
                    iCount--;
                    StallExecution(50);
                    fSuccess = ReadOmapTriton2(TWL_PHY_CLK_CTRL, &ctrl);
                } while (fSuccess &&(iCount > 0));                                
            }        
        }
    }
    
    // Setting to TWL_USB_INT_EN_RISE_SET register 
    WriteOmapTriton2(TWL_USB_INT_EN_RISE_SET, 0x1F);
    
    // Setting to TWL_USB_INT_EN_FALL_SET register
    WriteOmapTriton2(TWL_USB_INT_EN_FALL_SET, 0x1F);

    // Configureing the debounce time as 15ms
    // Earlier it was 30ms. This is modified as sees_end is 
    // not being detected. This may have an impact on HNP/SRP.
//    WriteOmapTriton2(TWL_VBUS_DEBOUNCE,0xF); // This change affects CETK tests.
    
    
    // Enable OTG
    WriteOmapTriton2(TWL_POWER_CTRL_SET, POWER_CTRL_OTG_EN);
    
    // Setting to normal operation mode
    bModeBits = FUNC_CTRL_XCVRSELECT_MASK | FUNC_CTRL_OPMODE_MASK;
    WriteOmapTriton2(TWL_FUNC_CTRL_CLR, bModeBits);
    
    {
        BYTE bCarKitReg, bIfcCtrlReg;
        
        ReadOmapTriton2(TWL_CARKIT_CTRL, &bCarKitReg);
        ReadOmapTriton2(TWL_IFC_CTRL, &bIfcCtrlReg);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver: CARKIT_Reg(0x%x), IFC_CTRL_Reg(0x%x)\r\n"), 
            bCarKitReg, bIfcCtrlReg));
        
        ReadOmapTriton2(TWL_USB_INT_EN_RISE, &bCarKitReg);
        ReadOmapTriton2(TWL_USB_INT_EN_FALL, &bIfcCtrlReg);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver: TWL_USB_INT_EN_RISE(0x%x), TWL_USB_INT_EN_FALL(0x%x)\r\n"), 
            bCarKitReg, bIfcCtrlReg));
    }

    // configure USB presence interrupt for rising edge
    ReadOmapTriton2(TWL_PWR_EDR1, &bModeBits);
    bModeBits |=  0x20;
    bModeBits &= ~0x10;
    WriteOmapTriton2(TWL_PWR_EDR1, bModeBits);
    
    DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver:Configure success\r\n")));
}

void HSUSBOTGTransceiverTps659xx::EnableVBusDischarge(BOOL fDischarge)
{

    if (fDischarge)
    {
        WriteOmapTriton2(TWL_OTG_CTRL_SET, TWL_OTG_CTRL_DISCHRGVBUS);
    }
    else
    {
        WriteOmapTriton2(TWL_OTG_CTRL_CLR, TWL_OTG_CTRL_DISCHRGVBUS);
    }
}

BOOL HSUSBOTGTransceiverTps659xx::ResetPHY()
{
		BYTE uRegister = 0;
		BOOL rc = FALSE;
		ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
		ReadOmapTriton2(TWL_PHY_PWR_CTRL, &uRegister);
		WriteOmapTriton2(TWL_PHY_PWR_CTRL, PHY_PWR_CTRL_PHYPWD);
		ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
		WriteOmapTriton2(TWL_PHY_PWR_CTRL, 0x0);
		ReadOmapTriton2(TWL_PHY_PWR_CTRL, &uRegister);
		ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
		ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
		WriteOmapTriton2(TWL_FUNC_CTRL_SET, (0x20) );
		ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
		return rc;
}
BOOL HSUSBOTGTransceiverTps659xx::IsADeviceConnected()
{
    BYTE uRegister = 0;
    BOOL rc = FALSE;

    // Check if a B device is connected
    ReadOmapTriton2(TWL_STS_HW_CONDITIONS, &uRegister);

    // TWL_STS_USB - is set when B Device (e.g. flash drive) is connected
    // TWL_STS_VBUS - is set when a A device ( host ) is connected

    // TWL_STS_VBUS takes a while to clear after the cable is removed so
    // A device is identified by checking the VBUS states in devclt
    // register of MUSB controller
    if (uRegister & TWL_STS_VBUS)
        {
        // A device detected
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver::IsBDeviceConnected : TRUE\r\n")));
        rc = TRUE;
        }

    return rc;
}


BOOL HSUSBOTGTransceiverTps659xx::IsBDeviceConnected()
{
    BYTE uRegister = 0;
    BOOL rc = FALSE;

    // Check if a B device is connected
    ReadOmapTriton2(TWL_STS_HW_CONDITIONS, &uRegister);

    // TWL_STS_USB - is set when B Device (e.g. flash drive) is connected
    // TWL_STS_VBUS - is set when a A device ( host ) is connected
    
    // TWL_STS_VBUS takes a while to clear after the cable is removed so
    // A device is identified by checking the VBUS states in devclt
    // register of MUSB controller
    if (uRegister & TWL_STS_USB)
        {
        // B device detected
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HSUSBOTGTransceiver::IsBDeviceConnected : TRUE\r\n")));
        rc = TRUE;
        }

    return rc;
}

void HSUSBOTGTransceiverTps659xx::SetVBusSource(BOOL fVBat)
{
    BYTE bTemp =((fVBat)? 0x14: 0x18);        
    WriteOmapTriton2(TWL_VUSB_DEDICATED1, bTemp);
#if 0
    BYTE uRegister = 0;
    ReadOmapTriton2(TWL_FUNC_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_FUNC_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_IFC_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_IFC_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_OTG_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_OTG_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_OTHER_FUNC_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_OTHER_FUNC_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_OTHER_INT_STS, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_OTHER_INT_STS=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_PHY_PWR_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_PHY_PWR_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_PHY_CLK_CTRL, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_PHY_CLK_CTRL=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_VUSB3V1_REMAP, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_VUSB3V1_REMAP=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_VUSB1V5_REMAP, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_VUSB1V5_REMAP=0x%x\r\n"),uRegister));

    ReadOmapTriton2(TWL_VUSB1V8_REMAP, &uRegister);
    RETAILMSG(1, (TEXT("+TWL_VUSB1V8_REMAP=0x%x\r\n"),uRegister));
#endif
}

void    
HSUSBOTGTransceiverTps659xx::AconnNotifHandle(HANDLE hAconnEvent)
{
    m_hAconnEvent = hAconnEvent;
    DEBUGMSG(ZONE_OTG_FUNCTION, (TEXT("HSUSBOTGTransceiverTriton2::AconnNotifHandle (0x%x)\r\n"), hAconnEvent));
}

void
HSUSBOTGTransceiverTps659xx::EnableWakeupInterrupt(BOOL fEnable)
{    
    DEBUGMSG(ZONE_OTG_FUNCTION, (TEXT("HSUSBOTGTransceiverTriton2::EnableWakeupInterrupt (%d)\r\n"), fEnable));
    if (fEnable)
    {
        TWLInterruptMask(m_hTriton2Handle, TWL_INTR_USB_PRES, FALSE);
        TWLWakeEnable(m_hTriton2Handle, TWL_INTR_USB_PRES, TRUE);
    }
    else
    {
        TWLInterruptMask(m_hTriton2Handle, TWL_INTR_USB_PRES, TRUE);
        TWLWakeEnable(m_hTriton2Handle, TWL_INTR_USB_PRES, FALSE);
    }
}

#pragma warning(push)
#pragma warning(disable : 4702)
DWORD 
HSUSBOTGTransceiverTps659xx::StateChangeThread(
    void* pvData
)
{
    HSUSBOTGTransceiverTps659xx* me =(HSUSBOTGTransceiverTps659xx*)pvData;
    
    for (;;)
    {
        DWORD rc = WaitForSingleObject(me->m_hUsbPresenceEvent, INFINITE);        
        
        // usb presence interrupt
        //
#ifndef DEBUG
		UNREFERENCED_PARAMETER(rc);
#endif
        DEBUGMSG(ZONE_OTG_INFO, (L"########### kt: USB Presence interrupt ############0x%x\r\n",rc));                                    

        SetEvent(me->m_hAconnEvent);
    }
    
    return 1;
}
#pragma warning(pop)

HSUSBOTGTransceiverTps659xx::HSUSBOTGTransceiverTps659xx(PHSMUSB_T pOTG)
{
    DEBUGMSG(ZONE_OTG_FUNCTION, (TEXT("HSUSBOTGTransceiver get called\r\n")));
    m_hTriton2Handle = TWLOpen();
    m_hUsbPresenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    m_pOTG = pOTG;
    
    if (m_hUsbPresenceEvent == NULL)
    {
        DEBUGMSG(ZONE_OTG_ERROR, (L"HSUSBOTGTransceiver::HSUSBOTGTransceiver():"
            L"! unable to allocate event handle(err=0x%08X)\r\n",
            GetLastError())
            );
        return;
    }    
    SetVBusSource(TRUE);
    Configure();
    
    if (!TWLInterruptInitialize(m_hTriton2Handle, TWL_INTR_USB_PRES, m_hUsbPresenceEvent))
    {
        DEBUGMSG(ZONE_OTG_ERROR, (L"ERROR: HSUSBOTGTransceiver::HSUSBOTGTransceiver()"
            L"Failed associate event with TWL_INTR_USB_PRES interrupt\r\n"
            ));
        goto cleanUp;
    }
    
    if (!TWLInterruptMask(m_hTriton2Handle, TWL_INTR_USB_PRES, FALSE))
    {
        DEBUGMSG(ZONE_OTG_ERROR, (L"HSUSBOTGTransceiver::HSUSBOTGTransceiver() "
            L"Failed associate event with TWL_INTR_USB_PRES interrupt\r\n"
            ));
    }
    
    // Spawn transceiver state change thread
    m_hStateChangeThread = CreateThread(NULL, 0, StateChangeThread, this, 0, NULL);
    if (m_hStateChangeThread == NULL)
    {
        DEBUGMSG(ZONE_OTG_ERROR, (L"HSUSBOTGTransceiver::HSUSBOTGTransceiver():"
            L"! unable to spawn state change thread(err=0x%08X)\r\n",
            GetLastError()
            ));
        return;
    }
    
cleanUp:
    return;
}


HSUSBOTGTransceiverTps659xx::~HSUSBOTGTransceiverTps659xx()
{
    if (m_hUsbPresenceEvent)
    {
        CloseHandle(m_hUsbPresenceEvent);
        m_hUsbPresenceEvent = NULL;
    }
    
    if (m_hTriton2Handle) 
        TWLClose(m_hTriton2Handle);   
    m_hTriton2Handle = NULL;
}

//-------------------------------------------------------------------------------------------------

BOOL HSUSBOTGTransceiverTps659xx::SendSingularPBMessageT2(UCHAR power_res_id,UCHAR res_state)
{
    UINT16 pb_message;
    UCHAR regval;
    
    regval = 0x02;  // Enable I2C access to the Power Bus 
    if(!WriteOmapTriton2(TWL_PB_CFG, regval))
        goto cleanUp;
    
    // Form the message for VDAC 
    pb_message = TwlTargetMessage(TWL_PROCESSOR_GRP1, power_res_id, res_state);
    
    // Extract the Message MSB 
    regval = (UCHAR)(pb_message >> 8);
    if(!WriteOmapTriton2(TWL_PB_WORD_MSB, regval))
        goto cleanUp;
    
    // Extract the Message LSB 
    regval = (UCHAR)(pb_message & 0x00FF);
    if(!WriteOmapTriton2(TWL_PB_WORD_LSB, regval))
        goto cleanUp;
    
    return (TRUE);
    
cleanUp:
      return FALSE;    
}


BOOL HSUSBOTGTransceiverTps659xx::WakeT2USBPhyDPLLT2()
{
    BYTE sts;
    BYTE ctrl;
    BOOL fGetLock = FALSE;

    // Lock PHY: start
    if(ReadOmapTriton2(TWL_PHY_CLK_CTRL_STS, &sts))
    {
        int iCount = 500;
        if (((sts & PHY_CLK_CTRL_STS_PHY_DPLL_LOCK) == 0) && ReadOmapTriton2(TWL_PHY_CLK_CTRL, &ctrl))
        {
            BOOL fSuccess;                
            do {
                ctrl |= PHY_CLK_CTRL_REQ_PHY_DPLL_CLK;                
                WriteOmapTriton2(TWL_PHY_CLK_CTRL, ctrl);
                sts = 0x00;
                if (ReadOmapTriton2(TWL_PHY_CLK_CTRL_STS, &sts) && (sts & PHY_CLK_CTRL_STS_PHY_DPLL_LOCK))
                {
                    fGetLock = TRUE;
                    break;
                }
                iCount--;
                StallExecution(50);
                fSuccess = ReadOmapTriton2(TWL_PHY_CLK_CTRL, &ctrl);
            } while (fSuccess && (iCount > 0));                                
        }            
    }
    // Lock PHY: end

    StallExecution(10);  
#if 0
    if (ReadTwlReg(TWL_PHY_CLK_CTRL, ctrl))
    {
        // clear PHY_DPLL_LOCK bit and let PHY state depend on the SUSPENDM bit in the FUNC_CTRL register
        ctrl &= ~PHY_CLK_CTRL_STS_PHY_DPLL_LOCK;
        WriteOmapTriton2(TWL_PHY_CLK_CTRL, ctrl);
        fGetLock = TRUE;
    }     
#endif
    return fGetLock;
}


BOOL HSUSBOTGTransceiverTps659xx::SetT2USBXvcrActiveT2(BOOL fActive)
{  
    BOOL rc = TRUE;
    BYTE ctrl = 0;
    if (fActive)    
    {
        // give time for power-on
        StallExecution(100);           
        WakeT2USBPhyDPLLT2();

        WriteOmapTriton2(TWL_FUNC_CTRL_SET, FUNC_CTRL_SUSPENDM);
        StallExecution(400);           
    }
    else
    {
        // if it has been asleep, wakeup the PHY before put to sleep again.
        // this is to avoid the PHY is actually sleeping and access the FUNC_CTRL
        // would return error
        // From USB analyzer, you may see SUSPEND/ALIVE and then SUSPEND again.  The problem
        // is due to enable phy and then disable again
        // The below steps effectively stops functional clock
        ReadOmapTriton2(TWL_PHY_CLK_CTRL, &ctrl);
        ctrl &= ~PHY_CLK_CTRL_REQ_PHY_DPLL_CLK;                
        WriteOmapTriton2(TWL_PHY_CLK_CTRL, ctrl);

        WriteOmapTriton2(TWL_FUNC_CTRL_CLR, FUNC_CTRL_SUSPENDM);
    }    

    return rc;
}

BOOL HSUSBOTGTransceiverTps659xx::SetT2USBXvcrPowerOff(BOOL fActive)
{
    BOOL rc = TRUE;
    BYTE ctrl = 0;

    if(fActive)
    {
        ReadOmapTriton2(TWL_PHY_PWR_CTRL, &ctrl);
        ctrl |= PHY_PWR_CTRL_PHYPWD;                
        WriteOmapTriton2(TWL_PHY_PWR_CTRL, ctrl);
    }
    else
    {
        ReadOmapTriton2(TWL_PHY_PWR_CTRL, &ctrl);
        ctrl &= ~PHY_PWR_CTRL_PHYPWD;                
        WriteOmapTriton2(TWL_PHY_PWR_CTRL, ctrl);
    }
    return rc;
}


BOOL HSUSBOTGTransceiverTps659xx::EnableFClock(BOOL enable)
{
    if(enable)
    {
        // put LDO's in Active mode
        SendSingularPBMessageT2(TWL_VUSB_3V1_RES_ID, TWL_RES_ACTIVE);
        SendSingularPBMessageT2(TWL_VUSB_1V5_RES_ID, TWL_RES_ACTIVE);
        SendSingularPBMessageT2(TWL_VUSB_1V8_RES_ID, TWL_RES_ACTIVE);
        SetT2USBXvcrActiveT2(TRUE);

        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HS USB Enable Functional clock \r\n")));
    }
    else
    {
        SetT2USBXvcrActiveT2(FALSE);
        // put LDO's in OFF mode
        SendSingularPBMessageT2(TWL_VUSB_3V1_RES_ID, TWL_RES_OFF);
        SendSingularPBMessageT2(TWL_VUSB_1V5_RES_ID, TWL_RES_OFF);
        SendSingularPBMessageT2(TWL_VUSB_1V8_RES_ID, TWL_RES_OFF);

        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HS USB Disable Functional clock \r\n")));
    }
    return TRUE;
}

BOOL HSUSBOTGTransceiverTps659xx::UpdateUSBVBusSleepOffState(BOOL fActive)
{
    BOOL rc = TRUE;
    BYTE state =0;
    if(fActive)
    {
        state = TWL_RES_ACTIVE|(TWL_RES_ACTIVE << 4);
        WriteOmapTriton2(TWL_VINTDIG_REMAP, state);
        WriteOmapTriton2(TWL_VUSB1V8_REMAP, state);
        WriteOmapTriton2(TWL_VUSB1V5_REMAP, state);
        WriteOmapTriton2(TWL_VUSB3V1_REMAP, state);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HS USB Power ON\r\n")));
    }
    else
    {
        state = TWL_RES_OFF|(TWL_RES_OFF << 4);
        WriteOmapTriton2(TWL_VINTDIG_REMAP, state);
        WriteOmapTriton2(TWL_VUSB1V8_REMAP, state);
        WriteOmapTriton2(TWL_VUSB1V5_REMAP, state);
        WriteOmapTriton2(TWL_VUSB3V1_REMAP, state);
        DEBUGMSG(ZONE_OTG_INFO, (TEXT("HS USB Power OFF\r\n")));
    }

    return rc;
}

BOOL HSUSBOTGTransceiverTps659xx::SetLowPowerMode()
{
    return TRUE;
}

BOOL HSUSBOTGTransceiverTps659xx::IsSE0()
{
    UCHAR debug_r;
        
    ReadOmapTriton2(TWL_DEBUG, &debug_r);
    DEBUGMSG(ZONE_OTG_INFO, (TEXT("IsSE0 called with line state = 0x%x\r\n"), (debug_r & 0x3)));

    return (((debug_r & 0x3) == 0x00)? TRUE: FALSE);
}

void HSUSBOTGTransceiverTps659xx::DumpULPIRegs()
{
    BYTE Reg1, Reg2;

    RETAILMSG(1, (L"TPS659XX ULPI Register\r\n"));

    ReadOmapTriton2(TWL_VENDOR_ID_LO, &Reg1);
    ReadOmapTriton2(TWL_VENDOR_ID_HI, &Reg2);
    RETAILMSG(1, (L"\tULPI_VID=(%x,%x)\r\n", Reg1, Reg2));

    ReadOmapTriton2(TWL_PRODUCT_ID_LO, &Reg1);
    ReadOmapTriton2(TWL_PRODUCT_ID_HI, &Reg2);
    RETAILMSG(1, (L"\tULPI_PID=(%x,%x)\r\n", Reg1, Reg2));

    ReadOmapTriton2(TWL_FUNC_CTRL, &Reg1);
    RETAILMSG(1, (L"\tFunction Control=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_IFC_CTRL, &Reg1);
    RETAILMSG(1, (L"\tInterface Control=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_OTG_CTRL, &Reg1);
    RETAILMSG(1, (L"\tOTG Control(%x)=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_USB_INT_EN_RISE, &Reg1);
    RETAILMSG(1, (L"\tInterrupt Enable rising=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_USB_INT_EN_FALL, &Reg1);
    RETAILMSG(1, (L"\tInterrupt Enable Falling=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_USB_INT_STS, &Reg1);
    RETAILMSG(1, (L"\tInterrupt Status=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_USB_INT_LATCH, &Reg1);
    RETAILMSG(1, (L"\tInterrupt Latch=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_DEBUG, &Reg1);
    RETAILMSG(1, (L"\tDebug=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_SCRATCH_REG, &Reg1);
    RETAILMSG(1, (L"\tScratch=%x\r\n", Reg1));

    ReadOmapTriton2(TWL_POWER_CTRL, &Reg1);
    RETAILMSG(1, (L"\tPower=%x\r\n", Reg1));
}

