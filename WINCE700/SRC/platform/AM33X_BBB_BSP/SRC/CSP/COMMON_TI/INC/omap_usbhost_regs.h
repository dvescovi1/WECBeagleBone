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
/*
*  File:  omap_usbhost_regs.h
*/
#ifndef __OMAP_USBHOST_REGS_H
#define __OMAP_USBHOST_REGS_H

#define OMAP_EHCI_REGS_SIZE         (1024)
#define OMAP_UHH_REGS_SIZE          (1024)

//
//  USB Host Subsystem Registers
//

typedef volatile struct
{
    UINT32 REVISION;                    
    UINT32 RESERVED_1[3];               
    UINT32 SYSCONFIG;                   
    UINT32 SYSSTATUS;                   
    UINT32 RESERVED_2[10];              
    UINT32 HOSTCONFIG;                  
    UINT32 DEBUG_CSR;                   
} OMAP_UHH_REGS;

typedef volatile struct
{
    UINT32  USBTLL_REVISION;
    UINT32  RESERVED1[3];
    UINT32  USBTLL_SYSCONFIG;
    UINT32  USBTLL_SYSSTATUS;
    UINT32  USBTLL_IRQSTATUS;
    UINT32  USBTLL_IRQENABLE;
    UINT32  RESERVED2[4];
    UINT32  TLL_SHARED_CONF;
    UINT32  RESERVED3[3];
    UINT32  TLL_CHANNEL_CONF[3];
} OMAP_USB_TLL_REGS;





// UHH_SYSCONFIG register fields

#define UHH_SYSCONFIG_AUTOIDLE                       (1 << 0)
#define UHH_SYSCONFIG_SOFTRESET                      (1 << 1)
#define UHH_SYSCONFIG_ENAWAKEUP                    (1 << 2)
#define UHH_SYSCONFIG_SIDLEMODE(mode)         ((mode) << 3)
#define UHH_SYSCONFIG_CACTIVITY                      (1 << 8)
#define UHH_SYSCONFIG_MIDLEMODE(mode)         ((mode) << 12)

#define SIDLE_FORCE                             (0)
#define SIDLE_IGNORE                            (1)
#define SIDLE_SMART                             (2)

#define MIDLE_FORCE                             (0)
#define MIDLE_IGNORE                            (1)
#define MIDLE_SMART                             (2)

// UHH_SYSSTATUS register fields

#define UHH_SYSSTATUS_RESETDONE                         (1 << 0)
#define UHH_SYSSTATUS_OHCI_RESETDONE               (1 << 1)
#define UHH_SYSSTATUS_EHCI_RESETDONE               (1 << 2)



// UHH_HOSTCONFIG register fields

#define UHH_HOSTCONFIG_P1_ULPI_BYPASS                  (1 << 0)
#define UHH_HOSTCONFIG_AUOTPPD_ON                        (1 << 1)
#define UHH_HOSTCONFIG_ENA_INCR4                            (1 << 2)
#define UHH_HOSTCONFIG_ENA_INCR8                            (1 << 3)
#define UHH_HOSTCONFIG_ENA_INCR16                           (1 << 4)
#define UHH_HOSTCONFIG_ENA_INCR_ALIGN                  (1 << 5)
#define UHH_HOSTCONFIG_P1_CONNECT_STATUS           (1 << 8)
#define UHH_HOSTCONFIG_P2_CONNECT_STATUS           (1 << 9)
#define UHH_HOSTCONFIG_P3_CONNECT_STATUS           (1 << 10)
#define UHH_HOSTCONFIG_P2_ULPI_BYPASS                   (1 << 11)
#define UHH_HOSTCONFIG_P3_ULPI_BYPASS                   (1 << 12)




// USBTLL_SYSCONFIG register fields

#define USBTLL_SYSCONFIG_AUTOIDLE                       (1 << 0)
#define USBTLL_SYSCONFIG_SOFTRESET                     (1 << 1)
#define USBTLL_SYSCONFIG_ENAWAKEUP                    (1 << 2)
#define USBTLL_SYSCONFIG_SIDLEMODE(mode)         ((mode) << 3)
#define USBTLL_SYSCONFIG_CACTIVITY                       (1 << 8)



// USBTLL_SYSSTATUS register fields

#define USBTLL_SYSSTATUS_RESETDONE                         (1 << 0)



// USBTLL_IRQSTATUS register fields

#define USBTLL_IRQSTATUS_FCLK_START                         (1 << 0)
#define USBTLL_IRQSTATUS_FCLK_END                             (1 << 1)
#define USBTLL_IRQSTATUS_ACCESS_ERROR                     (1 << 2)



// USBTLL_IRQENABLE register fields

#define USBTLL_IRQENABLE_FCLK_START_EN                         (1 << 0)
#define USBTLL_IRQENABLE_FCLK_END_EN                            (1 << 1)
#define USBTLL_IRQENABLE_ACCESS_ERROR_EN                     (1 << 2)



// USBTLL_SHARED_CONF register fields

#define USBTLL_SHARED_CONF_FCLK_IS_ON                             (1 << 0)
#define USBTLL_SHARED_CONF_FCLK_REQ                                (1 << 1)
#define USBTLL_SHARED_CONF_USB_DIVRATIO(mode)             ((mode) << 2)
#define USBTLL_SHARED_CONF_USB_180D_SDR_EN                   (1 << 5)
#define USBTLL_SHARED_CONF_USB_90D_DDR_EN                    (1 << 6)



// USBTLL_CHANNEL_CONF_i register fields

#define USBTLL_CHANNEL_CONF_i_CHANEN                                  (1 << 0)
#define USBTLL_CHANNEL_CONF_i_CHANMODE(mode)                    ((mode) << 1)
#define USBTLL_CHANNEL_CONF_i_UTMIISADEV                             (1 << 3)
#define USBTLL_CHANNEL_CONF_i_TLLATTACH                              (1 << 4)
#define USBTLL_CHANNEL_CONF_i_TLLCONNECT                             (1 << 5)
#define USBTLL_CHANNEL_CONF_i_TLLFULLSPEED                         (1 << 6)
#define USBTLL_CHANNEL_CONF_i_ULPIOUTCLKMODE                    (1 << 7)
#define USBTLL_CHANNEL_CONF_i_ULPIDDRMODE                         (1 << 8)
#define USBTLL_CHANNEL_CONF_i_UTMIAUTOIDLE                        (1 << 9)
#define USBTLL_CHANNEL_CONF_i_ULPIAUTOIDLE                         (1 << 10)
#define USBTLL_CHANNEL_CONF_i_ULPINOBITSTUFF                     (1 << 11)
#define USBTLL_CHANNEL_CONF_i_CHRGVBUS                                (1 << 15)
#define USBTLL_CHANNEL_CONF_i_DRVVBUS                                  (1 << 16)
#define USBTLL_CHANNEL_CONF_i_TESTEN                                    (1 << 17)
#define USBTLL_CHANNEL_CONF_i_TESTTXEN                                (1 << 18)
#define USBTLL_CHANNEL_CONF_i_TESTTXDAT                              (1 << 19)
#define USBTLL_CHANNEL_CONF_i_TESTTXSE0                              (1 << 20)
#define USBTLL_CHANNEL_CONF_i_FSLSMODE(mode)                     ((mode) << 24)
#define USBTLL_CHANNEL_CONF_i_FSLSLINESTATE(mode)            ((mode) << 28)



// Multiple-mode for serial interface's mode only
enum
{
    FSLSMODE_6PIN_UDIR_PHY_DAT_SE0,
    FSLSMODE_6PIN_UDIR_PHY_DP_DM,
    FSLSMODE_3PIN_BDIR_PHY,
    FSLSMODE_4PIN_BDIR_PHY,
    FSLSMODE_6PIN_UDIR_TLL_DAT_SE0,
    FSLSMODE_6PIN_UDIR_TLL_DP_DM,
    FSLSMODE_3PIN_BDIR_TLL,
    FSLSMODE_4PIN_BDIR_TLL,
    FSLSMODE_2PIN_BDIR_TLL_DAT_SE0,
    FSLSMODE_2PIN_BDIR_TLL_DP_DM
};

// Channel mode Selection
enum
{
    CHANMODE_UTMI_TO_ULPITLL,
    CHANMODE_UTMI_TO_SERIAL,
    CHANMODE_UMTI_PHY
};

// CM_FCLKEN3_CORE register fields

#define CM_FCLKEN3_CORE_EN_TS                                               (1 << 1)
#define CM_FCLKEN3_CORE_EN_USBTLL                                       (1 << 2)


// CM_USBHOST register fields
#define CM_USBHOST_FCLK_48M_EN                                          (1 << 0)
#define CM_USBHOST_FCLK_120M_EN                                         (1 << 1)
#define CM_ICLKEN_USBHOST_EN                                            (1 << 0)
#define CM_AUTOIDLE_USBHOST_EN                                          (1 << 0)


// CM_ICLKEN3_CORE register fields

#define CM_ICLKEN3_CORE_EN_USBTLL                                         (1 << 2)


// CM_AUTOIDLE3_CORE register fields

#define CM_AUTOIDLE3_CORE_AUTO_USBTLL                                (1 << 2)


// CM_IDLEST3_CORE register fields

#define CM_IDLEST3_CORE_ST_USBTLL                                         (1 << 2)

#endif //__OMAP_USBHOST_REGS_H

