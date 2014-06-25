// All rights reserved ADENEO EMBEDDED 2010
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
//------------------------------------------------------------------------------
//
//  File: t2transceiver.h
//
#ifndef __TPS659XXUSB_H
#define __TPS659XXUSB_H

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

#define USB_INT_EN_MASK                         0xFF

#define USB_INT_EN_RISE_IDGND                   (1 << 4)
#define USB_INT_EN_RISE_SESSEND                 (1 << 3)
#define USB_INT_EN_RISE_SESSVALID               (1 << 2)
#define USB_INT_EN_RISE_VBUSVALID               (1 << 1)
#define USB_INT_EN_RISE_HOSTDISCONNECT          (1 << 0)

#define USB_INT_EN_FALL_IDGND                   (1 << 4)
#define USB_INT_EN_FALL_SESSEND                 (1 << 3)
#define USB_INT_EN_FALL_SESSVALID               (1 << 2)
#define USB_INT_EN_FALL_VBUSVALID               (1 << 1)
#define USB_INT_EN_FALL_HOSTDISCONNECT          (1 << 0)

#define OTHER_INT_EN_RISE_VB_SESS_VLD           (1 << 7)
#define OTHER_INT_EN_RISE_DM_HI                 (1 << 6)
#define OTHER_INT_EN_RISE_DP_HI                 (1 << 5)
#define OTHER_INT_EN_RISE_BDIS_ACON             (1 << 3)
#define OTHER_INT_EN_RISE_MANU                  (1 << 1)
#define OTHER_INT_EN_RISE_ABNORMAL_STRESS       (1 << 0)


#define OTHER_INT_EN_FALL_VB_SESS_VLD           (1 << 7)
#define OTHER_INT_EN_FALL_DM_HI                 (1 << 6)
#define OTHER_INT_EN_FALL_DP_HI                 (1 << 5)
#define OTHER_INT_EN_FALL_MANU                  (1 << 1)
#define OTHER_INT_EN_FALL_ABNORMAL_STRESS       (1 << 0)

#define USB_INT_STS_IDGND                       (1 << 4)
#define USB_INT_STS_SESSEND                     (1 << 3)
#define USB_INT_STS_SESSVALID                   (1 << 2)
#define USB_INT_STS_VBUSVALID                   (1 << 1)
#define USB_INT_STS_HOSTDISCONNECT              (1 << 0)

#define USB_INT_LATCH_IDGND                     (1 << 4)
#define USB_INT_LATCH_SESSEND                   (1 << 3)
#define USB_INT_LATCH_SESSVALID                 (1 << 2)
#define USB_INT_LATCH_VBUSVALID                 (1 << 1)
#define USB_INT_LATCH_HOSTDISCONNECT            (1 << 0)

#define OTHER_INT_STS_VB_SESS_VLD               (1 << 7)
#define OTHER_INT_STS_DM_HI                     (1 << 6)
#define OTHER_INT_STS_DP_HI                     (1 << 5)
#define OTHER_INT_STS_BDIS_ACON                 (1 << 3)
#define OTHER_INT_STS_MANU                      (1 << 1)
#define OTHER_INT_STS_ABNORMAL_STRESS           (1 << 0)

#define OTHER_INT_LATCH_VB_SESS_VLD             (1 << 7)
#define OTHER_INT_LATCH_DM_HI                   (1 << 6)
#define OTHER_INT_LATCH_DP_HI                   (1 << 5)
#define OTHER_INT_LATCH_BDIS_ACON               (1 << 3)
#define OTHER_INT_LATCH_MANU                    (1 << 1)
#define OTHER_INT_LATCH_ABNORMAL_STRESS         (1 << 0)

#endif
