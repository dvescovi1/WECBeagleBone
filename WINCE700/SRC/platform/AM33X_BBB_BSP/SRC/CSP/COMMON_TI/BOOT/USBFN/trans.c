// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007 BSQUARE Corporation. All rights reserved.

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
//------------------------------------------------------------------------------
//
//  File:  trans.c
//
#pragma warning (push)
#pragma warning (disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>

#include "soc_cfg.h"
#pragma warning (pop)

//-------------------------------------------------------------------------
// T2 HSUSB Transceiver Register Values
//-------------------------------------------------------------------------

#define FUNC_CTRL_SUSPENDM                      (1 << 6)
#define FUNC_CTRL_RESET                         (1 << 5)
#define FUNC_CTRL_OPMODE_MASK                   (3 << 3)
#define FUNC_CTRL_OPMODE_NORMAL                 (0 << 3)
#define FUNC_CTRL_OPMODE_NONDRIVING             (1 << 3)
#define FUNC_CTRL_OPMODE_DISABLE_BIT_NRZI       (2 << 3)
#define FUNC_CTRL_TERMSELECT                    (1 << 2)
#define FUNC_CTRL_XCVRSELECT_MASK               (3 << 0)
#define FUNC_CTRL_XCVRSELECT_HS                 (0 << 0)
#define FUNC_CTRL_XCVRSELECT_FS                 (1 << 0)
#define FUNC_CTRL_XCVRSELECT_LS                 (2 << 0)
#define FUNC_CTRL_XCVRSELECT_FS4LS              (3 << 0)
 
#define OTG_CTRL_VBUS_DRV                       (1 << 5)
#define OTG_CTRL_VBUS_CHRG                      (1 << 4)
#define OTG_CTRL_VBUS_DISCHRG                   (1 << 3)
#define OTG_CTRL_DM_PULLDOWN                    (1 << 2)
#define OTG_CTRL_DP_PULLDOWN                    (1 << 1)
#define OTG_CTRL_ID_PULLUP                      (1 << 0)

#define IFC_CTRL_INTERFACE_PROTECT_DISABLE      (1 << 7)
#define IFC_CTRL_AUTORESUME                     (1 << 4)
#define IFC_CTRL_CLOCK_SUSPENDM                 (1 << 3)
#define IFC_CTRL_CARKITMODE                     (1 << 2)
#define IFC_CTRL_FSLSSERIALMODE_3PIN            (1 << 1)

#define OTHER_FUNC_CTRL_DM_PULLUP               (1 << 7)
#define OTHER_FUNC_CTRL_DP_PULLUP               (1 << 6)
#define OTHER_FUNC_CTRL_BDIS_ACON_EN            (1 << 4)
#define OTHER_FUNC_CTRL_FIVEWIRE_MODE           (1 << 2)


#define OTHER_IFC_CTRL_OE_INT_EN                (1 << 6)
#define OTHER_IFC_CTRL_CEA2011_MODE             (1 << 5)
#define OTHER_IFC_CTRL_FSLSSERIALMODE_4PIN      (1 << 4)
#define OTHER_IFC_CTRL_HIZ_ULPI_60MHZ_OUT       (1 << 3)
#define OTHER_IFC_CTRL_HIZ_ULPI                 (1 << 2)
#define OTHER_IFC_CTRL_ALT_INT_REROUTE          (1 << 0)

#define OTHER_IFC_CTRL2_ULPI_STP_LOW            (1 << 4)
#define OTHER_IFC_CTRL2_ULPI_TEXN_POL           (1 << 3)
#define OTHER_IFC_CTRL2_ULPI_4PIN_2430          (1 << 2)
#define OTHER_IFC_CTRL2_ULPI_INT_OUTSEL_INT2N   (1 << 0)

#define PHY_CLK_CTRL_CLOCKGATING_EN             (1 << 2)
#define PHY_CLK_CTRL_CLK32_EN                   (1 << 1)
#define PHY_CLK_CTRL_REQ_PHY_DPLL_CLK           (1 << 0)

#define PHY_CLK_CTRL_STS_PHY_DPLL_LOCK          (1 << 0)

#define PHY_PWR_CTRL_PHYPWD                     (1 << 0)

#define POWER_CTRL_OTG_EN                       (1 << 5)

#define CARKIT_CTRL_CARKITPWR                   (1 << 0)
#define CARKIT_CTRL_TXDEN                       (1 << 2)
#define CARKIT_CTRL_RXDEN                       (1 << 3)
#define CARKIT_CTRL_SPKLEFTEN                   (1 << 4)
#define CARKIT_CTRL_MICEN                       (1 << 6)


#define USB_INT_EN_MASK                         0xFF

// Debug message 
#define OAL_RNDIS                               FALSE


//------------------------------------------------------------------------------
//
//  Function:  RequestPHYAccess()
//

BOOL 
RequestPHYAccess( 
        HANDLE hTWL
    )
{
	UNREFERENCED_PARAMETER(hTWL);

/* TODO : Board-specific PHY init
    UCHAR value = 0;

	if (!OALTritonRead(hTWL, TWL_PHY_CLK_CTRL, &value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: RequestPHYAccess: "
            L"Failed to read PHY clock ctrl\r\n"
            ));
        return FALSE;
        }

    if (!(value & PHY_CLK_CTRL_REQ_PHY_DPLL_CLK))
    {
        value |= PHY_CLK_CTRL_REQ_PHY_DPLL_CLK;

        if (!OALTritonWrite(hTWL, TWL_PHY_CLK_CTRL, value))
            {
            OALMSG(OAL_ERROR, (L"RNDIS ERROR: RequestPHYAccess: "
                L"Failed to request PHY access\r\n"
                ));
            return FALSE;
            }

        OALStall (2000);
    }
*/
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  PowerOnT2USBTransceiver()
//

BOOL 
PowerOnT2USBTransceiver( 
        HANDLE hTWL
    )
{
    BOOL  rc = FALSE;

	UNREFERENCED_PARAMETER(hTWL);

/* TODO : Board-specific PHY init
    UCHAR value = 0;

    // Enable writing to Power Configuration registers
    value = 0xC0;
    if (!OALTritonWrite(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    value = 0x0C;
    if (!OALTritonWrite(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    value = 0x18;
    if (!OALTritonWrite(hTWL, TWL_VUSB_DEDICATED1, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to wakeup T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // Exit from sleep
    value = 0;
    if (!OALTritonWrite(hTWL, TWL_VUSB_DEDICATED2, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to wakeup T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 3.1V regulator
    value = 0xE0;
    if (!OALTritonWrite(hTWL, TWL_VUSB3V1_DEV_GRP, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 3.1V regualtor\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 1.5V regulator
    value = 0xE0;
    if (!OALTritonWrite(hTWL, TWL_VUSB1V5_DEV_GRP, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 1.5V regualtor\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 1.8V regulator
    value = 0xE0;
    if (!OALTritonWrite(hTWL, TWL_VUSB1V8_DEV_GRP, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 1.8V regualtor\r\n"
            ));
        goto cleanUp;
        }
    
    // Disable writing to Power Configuration registers
    value = 0x0;
    if (!OALTritonWrite(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // enable usb regulators 
    {
        UINT16 value16;
        
        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V5_RES_ID, TWL_RES_ACTIVE);
        OALTritonWrite(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        OALTritonWrite(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);

        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V8_RES_ID, TWL_RES_ACTIVE);
        OALTritonWrite(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        OALTritonWrite(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);

        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_3V1_RES_ID, TWL_RES_ACTIVE);
        OALTritonWrite(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        OALTritonWrite(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);
    }
   
    // Done
    rc = TRUE;

cleanUp:*/
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  InitTransceiver
//
BOOL 
InitTransceiver( 
        HANDLE hTWL
    )
{
    BOOL    rc = FALSE;

	UNREFERENCED_PARAMETER(hTWL);

/* TODO : Board-specific PHY init
    UCHAR   value = 0;

    if (OALTritonRead(hTWL, TWL_VENDOR_ID_LO, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB vendor ID LO = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB vendor ID LO 0x%x\r\n", value));
        }
    
    if (OALTritonRead(hTWL, TWL_VENDOR_ID_HI, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB vendor ID HI = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB vendor ID HI 0x%x\r\n", value));
        }
    
    if (OALTritonRead(hTWL, TWL_PRODUCT_ID_LO, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB product ID LO = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB product ID LO 0x%x\r\n", value));
        }
    
    if (OALTritonRead(hTWL, TWL_PRODUCT_ID_HI, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB product ID HI = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB product ID HI 0x%x\r\n", value));
        }


    // set for interrupt from ULPI Rx CMD to Link  
    value = OTHER_IFC_CTRL_ALT_INT_REROUTE;
    if (!OALTritonWrite(hTWL, TWL_OTHER_IFC_CTRL_SET, value))
        goto cleanUp;
    
    // Setting to IFC_CTRL register for ULPI operation mode
    value = IFC_CTRL_CARKITMODE | IFC_CTRL_FSLSSERIALMODE_3PIN;
    if (!OALTritonWrite(hTWL, TWL_IFC_CTRL_CLR, value))
        goto cleanUp;
        
    // Setting to TWL_CARKIT_CTRL_CLR register for ULPI operation mode
    value = CARKIT_CTRL_CARKITPWR |CARKIT_CTRL_TXDEN |CARKIT_CTRL_RXDEN;
    value |= CARKIT_CTRL_SPKLEFTEN |CARKIT_CTRL_MICEN;
    if (!OALTritonWrite(hTWL, TWL_CARKIT_CTRL_CLR, value))
        goto cleanUp;
        
    // Power on OTG
    value = POWER_CTRL_OTG_EN;
    if (!OALTritonWrite(hTWL, TWL_POWER_CTRL_SET, value))
        goto cleanUp;

    // Clear DM/DP PULLDOWN bits
    value = OTG_CTRL_DM_PULLDOWN | OTG_CTRL_DP_PULLDOWN;
    if (!OALTritonWrite(hTWL, TWL_OTG_CTRL_CLR, value))
        goto cleanUp;

    // Set OPMODE normal, XCVRSELECT: FS transceiver
    value = FUNC_CTRL_OPMODE_MASK | FUNC_CTRL_XCVRSELECT_MASK | FUNC_CTRL_TERMSELECT;
    if (!OALTritonWrite(hTWL, TWL_FUNC_CTRL_CLR, value))
        goto cleanUp;

    value = FUNC_CTRL_OPMODE_NORMAL | FUNC_CTRL_XCVRSELECT_FS;
    if (!OALTritonWrite(hTWL, TWL_FUNC_CTRL_SET, value))
        goto cleanUp;

    // Make sure PHY power enabled
    value = 0;
    if (!OALTritonWrite(hTWL, TWL_PHY_PWR_CTRL, value))
        goto cleanUp;

    // Done
    rc = TRUE;

cleanUp:*/
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  InitializeHardware
//

BOOL InitializeHardware()
{
    BOOL   rc = FALSE;

/* TODO : Board-specific PHY init
    HANDLE hTWL;

    // Initialize Triton
    hTWL = OALTritonOpen();
    if (!hTWL)
    {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: InitializeHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        goto cleanUp;
    }

    if(!PowerOnT2USBTransceiver(hTWL))
        {
        goto cleanUp;
        }
    
    if(!RequestPHYAccess(hTWL))
        {
        goto cleanUp;
        }

    // Configure transceiver
    if (!InitTransceiver (hTWL))
        {
        goto cleanUp;
        }

    // We are done with Triton
    OALTritonClose(hTWL);

    // Done
    rc = TRUE;

cleanUp:*/
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ConnectHardware
//

void ConnectHardware()
{
	/* TODO : Board-specific PHY init
    HANDLE hTWL;
    UCHAR   value = 0;

    OALMSG(OAL_RNDIS, (L"+ConnectHardware: "
        L"\r\n"));

    // Initialize Triton
    hTWL = OALTritonOpen();
    if (!hTWL)
    {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: ConnectHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        return;
    }

    value = FUNC_CTRL_TERMSELECT;

    // Enable D+ pullup resistor
    if (!OALTritonWrite(hTWL, TWL_FUNC_CTRL_SET, value))
    {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: ConnectHardware: "
            L"Failed to set TWL_FUNC_CTRL_SET\r\n"
            ));
    }

    // We are done with Triton
    OALTritonClose(hTWL);
	*/
}

//------------------------------------------------------------------------------
//
//  Function:  DisconnectHardware
//

void DisconnectHardware()
{
	/* TODO : Board-specific PHY init
    HANDLE hTWL;
    UCHAR   value = 0;

    OALMSG(OAL_RNDIS, (L"+DisconnectHardware: "
        L"\r\n"));

    // Initialize Triton
    hTWL = OALTritonOpen();
    if (!hTWL)
    {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: DisconnectHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        return;
    }

    value = FUNC_CTRL_TERMSELECT;

    // Disable D+ pullup resistor
    if(!OALTritonWrite(hTWL, TWL_FUNC_CTRL_CLR, value))
    {
        OALMSG(OAL_ERROR, (L"RNDIS ERROR: DisconnectHardware: "
            L"Failed to clr TWL_FUNC_CTRL_CLR\r\n"
            ));
    }

    // We are done with Triton
    OALTritonClose(hTWL);
	*/
}

//------------------------------------------------------------------------------
//
//  Function:  GetUniqueDeviceID
//

DWORD GetUniqueDeviceID()
{
    DWORD code;
    DWORD *pDieId = (DWORD *)OALPAtoUA(SOCGetIDCodeAddress());

    // Create unique part of name from SoC ID
    code  = INREG32(pDieId);

    OALMSG(1, (L"+USBFN::Device ID = 0x%x\r\n", code));

    return 0x0B5D902F;  // hardcoded id
}

//------------------------------------------------------------------------------

