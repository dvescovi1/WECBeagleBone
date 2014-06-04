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
//  File: tps659xx_internals.h
//
#ifndef __TPS659XX_INTERNALS_H
#define __TPS659XX_INTERNALS_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// generates a target message to control voltage regulators on T2

#define TWL_PSM(dev_grp, res_id, res_state)  \
            ((dev_grp << 13) | (res_id << 4) | (res_state << 0)) 

#define TWL_PBM(dev_grp, res_grp, res_typ2, res_typ1, res_state) \
            (((dev_grp) << 13) | (1 << 12) | ((res_grp) << 9) | \
             ((res_typ2) << 7) | ((res_typ1) << 4) | ((res_state) << 0))

// generates a target message to control voltage regulators on T2
#define TwlTargetMessage(dev_grp, res_id, res_state)  \
                           (UINT16)((dev_grp << 13) | (res_id << 4) | res_state)

// generates a Singular message sequence to control T2 resources
#define TwlSingularMsgSequence(dev_grp, res_id, res_state, delay, next_addr)  \
                                (UINT32)((dev_grp << 29) | (res_id << 20) | \
                                         (res_state << 16) | (delay << 8) | next_addr) 

// generates a Singular message sequence to control T2 resources
#define TwlBroadcastMsgSequence(dev_grp, res_grp, res_type2, res_type, res_state, delay, next_addr)  \
                                (UINT32)((dev_grp << 29) | (1 << 28) | (res_grp << 25) | \
                                         (res_type2 << 23) | (res_type << 20) | \
										 (res_state << 16) | (delay << 8) | next_addr) 

//------------------------------------------------------------------------------
// NOTE:
// the following values correspond to the interrupt array contained in
// the triton driver.  If changes are made to the following value 
// corresponding changes must be made in the triton driver.  Values
// are masked using the following format except for usb
//
// |27 26 25 24 | 23 22 21 20 | 19 18 17 16 | 15 14 13 12 | 11 10 9 8 | 7 6 5 4 | 3 2 1 0| 
//
// |  mask bit  |     register index        | SIH index   |    event array index         |

#define TWL_ARRAYINDEX(x)                       (x & 0xFFF)
#define TWL_SIHINDEX(x)                         ((x >> 12) & 0x0F)
#define TWL_REGISTERINDEX(x)                    ((x >> 16) & 0xFF)
#define TWL_MASKBIT(x)                          ((x >> 24) & 0x0F)

// power management
#define TWL_INTR_PWRON                          0x00005000
#define TWL_INTR_CHG_PRES                       0x01005001
#define TWL_INTR_USB_PRES                       0x02005002
#define TWL_INTR_RTC_IT                         0x03005003
#define TWL_INTR_HOT_DIE                        0x04005004
#define TWL_INTR_PWROK_TIMEOUT                  0x05005005
#define TWL_INTR_MBCHG                          0x06005006
#define TWL_INTR_SC_DETECT                      0x07005007

// bci
#define TWL_INTR_WOVF                           0x00002008
#define TWL_INTR_TMOVF                          0x01002009
#define TWL_INTR_ICHGHIGH                       0x0200200A
#define TWL_INTR_ICHGLOW                        0x0300200B
#define TWL_INTR_ICHGEOC                        0x0400200C
#define TWL_INTR_TBATOR2                        0x0500200D
#define TWL_INTR_TBATOR1                        0x0600200E
#define TWL_INTR_BATSTS                         0x0700200F
#define TWL_INTR_VBATLVL                        0x00012010
#define TWL_INTR_VBATOV                         0x01012011
#define TWL_INTR_VBUSOV                         0x02012012
#define TWL_INTR_ACCHGOV                        0x03012013
#define TWL_BATT_CONNECT                        0x00000040

// madc
#define TWL_INTR_MADC_RT                        0x00003014
#define TWL_INTR_MADC_SW1                       0x01003015
#define TWL_INTR_MADC_SW2                       0x02003016
#define TWL_INTR_MADC_USB                       0x03003017

// gpio
#define TWL_INTR_GPIO_0                         0x00000018
#define TWL_INTR_GPIO_1                         0x01000019
#define TWL_INTR_GPIO_2                         0x0200001A
#define TWL_INTR_GPIO_3                         0x0300001B
#define TWL_INTR_GPIO_4                         0x0400001C
#define TWL_INTR_GPIO_5                         0x0500001D
#define TWL_INTR_GPIO_6                         0x0600001E
#define TWL_INTR_GPIO_7                         0x0700001F
#define TWL_INTR_GPIO_8                         0x00010020
#define TWL_INTR_GPIO_9                         0x01010021
#define TWL_INTR_GPIO_10                        0x02010022
#define TWL_INTR_GPIO_11                        0x03010023
#define TWL_INTR_GPIO_12                        0x04010024
#define TWL_INTR_GPIO_13                        0x05010025
#define TWL_INTR_GPIO_14                        0x06010026
#define TWL_INTR_GPIO_15                        0x07010027
#define TWL_INTR_GPIO_16                        0x00020028
#define TWL_INTR_GPIO_17                        0x01020029

// keypad
#define TWL_INTR_ITKPI                          0x0000102A
#define TWL_INTR_ITLKI                          0x0100102B
#define TWL_INTR_ITTOIS                         0x0200102C
#define TWL_INTR_ITMIS                          0x0300102D

// usb
#define TWL_INTR_USB_RISE_IDGND                 0x0400402E
#define TWL_INTR_USB_RISE_SESSEND               0x0300402F
#define TWL_INTR_USB_RISE_SESSVALID             0x02004030
#define TWL_INTR_USB_RISE_VBUSVALID             0x01004031
#define TWL_INTR_USB_RISE_HOSTDISCONNECT        0x00004032
#define TWL_INTR_USB_FALL_IDGND                 0x04014033
#define TWL_INTR_USB_FALL_SESSEND               0x03014034
#define TWL_INTR_USB_FALL_SESSVALID             0x02014035
#define TWL_INTR_USB_FALL_VBUSVALID             0x01014036
#define TWL_INTR_USB_FALL_HOSTDISCONNECT        0x00014037

// USB_other
#define TWL_INTR_OTHER_RISE_VB_SESS_VLD         0x07024038
#define TWL_INTR_OTHER_RISE_DM_HI               0x06024039
#define TWL_INTR_OTHER_RISE_DP_HI               0x0502403A
#define TWL_INTR_OTHER_RISE_BDIS_ACON           0x0302403B
#define TWL_INTR_OTHER_RISE_MANU                0x0102403C
#define TWL_INTR_OTHER_RISE_ABNORMAL_STRESS     0x0002403D
#define TWL_INTR_OTHER_FALL_VB_SESS_VLD         0x0703403E
#define TWL_INTR_OTHER_FALL_DM_HI               0x0603403F
#define TWL_INTR_OTHER_FALL_DP_HI               0x05034040
#define TWL_INTR_OTHER_FALL_MANU                0x01034042
#define TWL_INTR_OTHER_FALL_ABNORMAL_STRESS     0x00034043

// carkit
#define TWL_INTR_CARKIT_CARDP                   0x00004044
#define TWL_INTR_CARKIT_CARINTDET               0x00004045
#define TWL_INTR_CARKIT_IDFLOAT                 0x00004046

#define TWL_INTR_CARKIT_PSM_ERR                 0x00004047
#define TWL_INTR_CARKIT_PH_ACC                  0x00004048
#define TWL_INTR_CARKIT_CHARGER                 0x00004049
#define TWL_INTR_CARKIT_USB_HOST                0x0000404A
#define TWL_INTR_CARKIT_USB_OTG                 0x0000404B
#define TWL_INTR_CARKIT_CARKIT                  0x0000404C
#define TWL_INTR_CARKIT_DISCONNECTED            0x0000404D

#define TWL_INTR_CARKIT_STOP_PLS_MISS           0x0000404E
#define TWL_INTR_CARKIT_STEREO_TO_MONO          0x0000404F
#define TWL_INTR_CARKIT_PHONE_UART              0x00004050
#define TWL_INTR_CARKIT_PH_NO_ACK               0x00004051

// id
#define TWL_INTR_ID_FLOAT                       0x00004052
#define TWL_INTR_ID_RES_440K                    0x00004053
#define TWL_INTR_ID_RES_200K                    0x00004054
#define TWL_INTR_ID_RES_100K                    0x00004055

#define TWL_MAX_INTR                            0x0056
#define TWL_KEYPAD_ROWS                         8

//------------------------------------------------------------------------------
// generic interrupt masks

#define TWL_SIH_CTRL_COR                (1 << 2)
#define TWL_SIH_CTRL_PENDDIS            (1 << 1)
#define TWL_SIH_CTRL_EXCLEN             (1 << 0)

// TWL_PWR_EDR1
#define TWL_RTC_IT_RISING               (1 << 7)
#define TWL_RTC_IT_FALLING              (1 << 6)
#define TWL_USB_PRES_RISING             (1 << 5)
#define TWL_USB_PRES_FALLING            (1 << 4)
#define TWL_CHG_PRES_RISING             (1 << 3)
#define TWL_CHG_PRES_FALLING            (1 << 2)
#define TWL_PWRON_RISING                (1 << 1)
#define TWL_PWRON_FALLING               (1 << 0)

// TWL_PWR_EDR2
#define TWL_SC_DETECT_RISING            (1 << 7)
#define TWL_SC_DETECT_FALLING           (1 << 6)
#define TWL_MBCHG_RISING                (1 << 5)
#define TWL_MBCHG_FALLING               (1 << 4)
#define TWL_PWROK_TIMEOUT_RISING        (1 << 3)
#define TWL_PWROK_TIMEOUT_FALLING       (1 << 2)
#define TWL_HOTDIE_RISING               (1 << 1)
#define TWL_HOTDIE_FALLING              (1 << 0)

// TWL_RTC_STATUS_REG
#define TWL_RTC_STATUS_POWER_UP         (1 << 7)  // reset event
#define TWL_RTC_STATUS_ALARM            (1 << 6)  // alarm event
#define TWL_RTC_STATUS_ONE_D_EVENT      (1 << 5)  // day event
#define TWL_RTC_STATUS_ONE_H_EVENT      (1 << 4)  // hour event 
#define TWL_RTC_STATUS_ONE_M_EVENT      (1 << 3)  // minute event
#define TWL_RTC_STATUS_ONE_S_EVENT      (1 << 2)  // second event
#define TWL_RTC_STATUS_RUN              (1 << 1)  // RTC run 

#define TWM_RTC_STATUS_TIME_EVENT       (0xF << 2)  // Any time (day,hour,min,sec) event 

// TWL_RTC_CTRL_REG 
#define TWL_RTC_CTRL_GET_TIME           (1 << 6)
#define TWL_RTC_CTRL_SET_32_COUNTER     (1 << 5)
#define TWL_RTC_CTRL_TEST_MODE          (1 << 4)
#define TWL_RTC_CTRL_MODE_12_24         (1 << 3)
#define TWL_RTC_CTRL_AUTO_COMP          (1 << 2)
#define TWL_RTC_CTRL_ROUND_30S          (1 << 1)
#define TWL_RTC_CTRL_RUN                (1 << 0)

// TWL_RTC_INTERRUPTS_REG
#define TWL_RTC_INTERRUPTS_EVERY_SECOND (0 << 0)
#define TWL_RTC_INTERRUPTS_EVERY_MINUTE (1 << 0)
#define TWL_RTC_INTERRUPTS_EVERY_HOUR   (2 << 0)
#define TWL_RTC_INTERRUPTS_EVERY_DAY    (3 << 0)
#define TWL_RTC_INTERRUPTS_IT_TIMER     (1 << 2)
#define TWL_RTC_INTERRUPTS_IT_ALARM     (1 << 3)


// TWL_KEYP_CTRL_REG
#define TWL_KBD_CTRL_NRESET             (1 << 0)
#define TWL_KBD_CTRL_NSOFT_MODE         (1 << 1)
#define TWL_KBD_CTRL_LK_EN              (1 << 2)
#define TWL_KBD_CTRL_TOE_EN             (1 << 3)
#define TWL_KBD_CTRL_TOLE_EN            (1 << 4)
#define TWL_KBD_CTRL_RP_EN              (1 << 5)
#define TWL_KBD_CTRL_KBD_ON             (1 << 6)

// TWL_KEYP_IMR1
#define TWL_KBD_INT_EVENT               (1 << 0)
#define TWL_KBD_INT_LONG_KEY            (1 << 1)
#define TWL_KBD_INT_TIMEOUT             (1 << 2)
#define TWL_KBD_INT_MISS                (1 << 3)

// TWL_CTRL_SWx
#define TWL_MADC_CTRL_SW_BUSY           (1 << 0)
#define TWL_MADC_CTRL_SW_EOC_SW1        (1 << 1)
#define TWL_MADC_CTRL_SW_EOC_RT         (1 << 2)
#define TWL_MADC_CTRL_SW_EOC_BCI        (1 << 3)
#define TWL_MADC_CTRL_SW_EOC_USB        (1 << 4)
#define TWL_MADC_CTRL_SW_TOGGLE         (1 << 5)

// TWL_OTG_CTRL, TWL_OTG_CTRL_SET, TWL_OTG_CTRL_CLR
#define TWL_OTG_CTRL_IDPULLUP           (1<<0)
#define TWL_OTG_CTRL_DPPULLDOWN         (1<<1)
#define TWL_OTG_CTRL_DMPULLDOWN         (1<<2)
#define TWL_OTG_CTRL_DISCHRGVBUS        (1<<3)
#define TWL_OTG_CTRL_CHRGVBUS           (1<<4)
#define TWL_OTG_CTRL_DRVVBUS            (1<<5)

// TWL_ID_STATUS
#define TWL_ID_STATUS_RES_GND           (1<<0)
#define TWL_ID_STATUS_RES_FLOAT         (1<<4)

// TWL_DEV_GROUP_x
#define TWL_DEV_GROUP_NONE              (0 << 5)
#define TWL_DEV_GROUP_P1                (1 << 5)
#define TWL_DEV_GROUP_P2                (2 << 5)
#define TWL_DEV_GROUP_P1P2              (3 << 5)
#define TWL_DEV_GROUP_P3                (4 << 5)
#define TWL_DEV_GROUP_P1P3              (5 << 5)
#define TWL_DEV_GROUP_P2P3              (6 << 5)
#define TWL_DEV_GROUP_ALL               (7 << 5)

// TWL_STS_HW_CONDITIONS
#define TWL_STS_USB                     (1 << 2)
#define TWL_STS_VBUS                    (1 << 7)

// Processor Groups 
#define TWL_PROCESSOR_GRP_NULL          0x0     // GROUP NULL
#define TWL_PROCESSOR_GRP1              0x01    // Applications devices
#define TWL_PROCESSOR_GRP2              0x02    // Modem Devices - ex:- N3G 
#define TWL_PROCESSOR_GRP3              0x04    // Peripherals 
#define TWL_PROCESSOR_GRP123            0x07

// Resource Groups
#define TWL_RES_GRP_NONE                0x0
#define TWL_RES_GRP_PP                  0x1
#define TWL_RES_GRP_RC                  0x2
#define TWL_RES_GRP_PP_RC               0x3
#define TWL_RES_GRP_PR                  0x4
#define TWL_RES_GRP_PP_PR               0x5
#define TWL_RES_GRP_RC_PR               0x6
#define TWL_RES_GRP_ALL                 0x7

// VMMC1 possible voltage values 

#define TWL_VMMC1_1P85                  0x00
#define TWL_VMMC1_2P85                  0x01
#define TWL_VMMC1_3P0                   0x02
#define TWL_VMMC1_3P15                  0x03

// VMMC2 possible voltage values 

#define TWL_VMMC2_1P85                  0x06
#define TWL_VMMC2_2P60                  0x08
#define TWL_VMMC2_2P85                  0x0A
#define TWL_VMMC2_3P00                  0x0B
#define TWL_VMMC2_3P15                  0x0C    // or 0x0D or 0x0E or 0x0F 

// VPLL2 possible voltage values
#define TWL_VPLL2_0P70                  0x0     // 0.70v
#define TWL_VPLL2_1P00                  0x1     // 1.00v
#define TWL_VPLL2_1P20                  0x2     // 1.20v
#define TWL_VPLL2_1P30                  0x3     // 1.30v
#define TWL_VPLL2_1P50                  0x4     // 1.50v
#define TWL_VPLL2_1P80                  0x5     // 1.80v
#define TWL_VPLL2_1P85                  0x6     // 1.85v
#define TWL_VPLL2_2P50                  0x7     // 2.50v
#define TWL_VPLL2_2P60                  0x8     // 2.60v
#define TWL_VPLL2_2P80                  0x9     // 2.80v
#define TWL_VPLL2_2P85                  0xA     // 2.85v
#define TWL_VPLL2_3P00                  0xB     // 3.00v
#define TWL_VPLL2_3P15                  0xC     // 3.15v

// VPLL1 possible voltage values
#define TWL_VPLL1_1P00                  0x0     // 1.00v
#define TWL_VPLL1_1P20                  0x1     // 1.20v
#define TWL_VPLL1_1P30                  0x2     // 1.30v
#define TWL_VPLL1_1P80                  0x3     // 1.80v
#define TWL_VPLL1_2P80                  0x4     // 2.80v
#define TWL_VPLL1_3P00                  0x5     // 3.00v

// VSIM possible voltage values
#define TWL_VSIM_1P00                   0x0     // 1.00v
#define TWL_VSIM_1P20                   0x1     // 1.20v
#define TWL_VSIM_1P30                   0x2     // 1.30v
#define TWL_VSIM_1P80                   0x3     // 1.80v
#define TWL_VSIM_2P80                   0x4     // 2.80v
#define TWL_VSIM_3P00                   0x5     // 3.00v

// VAUX4 possible voltage values
#define TWL_VAUX4_0P70                  0x0     // 0.70v
#define TWL_VAUX4_1P00                  0x1     // 1.00v
#define TWL_VAUX4_1P20                  0x2     // 1.20v
#define TWL_VAUX4_1P30                  0x3     // 1.30v
#define TWL_VAUX4_1P50                  0x4     // 1.50v
#define TWL_VAUX4_1P80                  0x5     // 1.80v
#define TWL_VAUX4_1P85                  0x6     // 1.85v
#define TWL_VAUX4_2P50                  0x7     // 2.50v
#define TWL_VAUX4_2P60                  0x8     // 2.60v
#define TWL_VAUX4_2P80                  0x9     // 2.80v
#define TWL_VAUX4_2P85                  0xA     // 2.85v
#define TWL_VAUX4_3P00                  0xB     // 3.00v
#define TWL_VAUX4_3P15                  0xC     // 3.15v

// VAUX3 possible voltage values
#define TWL_VAUX3_1P50                  0x0     // 1.50v
#define TWL_VAUX3_1P80                  0x1     // 1.80v
#define TWL_VAUX3_2P50                  0x2     // 2.50v
#define TWL_VAUX3_2P80                  0x3     // 2.80v
#define TWL_VAUX3_3P00                  0x4     // 3.00v

// VAUX2 possible voltage values
#define TWL_VAUX2_1P00                  0x1     // 1.00v
#define TWL_VAUX2_1P20                  0x2     // 1.20v
#define TWL_VAUX2_1P30                  0x3     // 1.30v
#define TWL_VAUX2_1P50                  0x4     // 1.50v
#define TWL_VAUX2_1P80                  0x5     // 1.80v
#define TWL_VAUX2_1P85                  0x6     // 1.85v
#define TWL_VAUX2_2P50                  0x7     // 2.50v
#define TWL_VAUX2_2P60                  0x8     // 2.60v
#define TWL_VAUX2_2P80                  0x9     // 2.80v
#define TWL_VAUX2_2P85                  0xA     // 2.85v
#define TWL_VAUX2_3P00                  0xB     // 3.00v
#define TWL_VAUX2_3P15                  0xC     // 3.15v

// VAUX1 possible voltage values
#define TWL_VAUX1_1P50                  0x0     // 1.50v
#define TWL_VAUX1_1P80                  0x1     // 1.80v
#define TWL_VAUX1_2P50                  0x2     // 2.50v
#define TWL_VAUX1_2P80                  0x3     // 2.80v
#define TWL_VAUX1_3P00                  0x4     // 3.00v

// VDAC possible voltage values
#define TWL_VDAC_1P20                   0x0     // 1.20v
#define TWL_VDAC_1P30                   0x1     // 1.30v
#define TWL_VDAC_1P80                   0x2     // 1.80v

// Power Source IDs 
#define TWL_VAUX1_RES_ID                1
#define TWL_VAUX2_RES_ID                2
#define TWL_VAUX3_RES_ID                3
#define TWL_VAUX4_RES_ID                4
#define TWL_VMMC1_RES_ID                5
#define TWL_VMMC2_RES_ID                6
#define TWL_VPLL1_RES_ID                7
#define TWL_VPLL2_RES_ID                8
#define TWL_VSIM_RES_ID                 9
#define TWL_VDAC_RES_ID                 10
#define TWL_VINTANA1_RES_ID             11
#define TWL_VINTANA2_RES_ID             12
#define TWL_VINTDIG_RES_ID              13
#define TWL_VIO_RES_ID                  14
#define TWL_VDD1_RES_ID                 15
#define TWL_VDD2_RES_ID                 16
#define TWL_VUSB_1V5_RES_ID             17
#define TWL_VUSB_1V8_RES_ID             18
#define TWL_VUSB_3V1_RES_ID             19
#define TWL_REGEN_RES_ID                21
#define TWL_NRESPWRON_RES_ID            22
#define TWL_CLKEN_RES_ID                23
#define TWL_SYSEN_RES_ID                24
#define TWL_HFCLKOUT_RES_ID             25
#define TWL_32KCLK_OUT_RES_ID           26
#define TWL_TRITON_RESET_RES_ID         27
#define TWL_MAIN_REF_RES_ID             28
#define TWL_NRES_PWRON_ID               22
#define TWL_CLKEN_ID                    23
#define TWL_SYSEN_ID                    24
#define TWL_HFCLKOUT_ID                 25
#define TWL_32KCLK_OUT_ID               26
#define TWL_TRITON_RESET                27
#define TWL_MAIN_REF_ID                 28
// Power Resource States
#define TWL_RES_OFF                     0x00  // Resource in OFF 
#define TWL_RES_SLEEP                   0x08  // Resource in SLEEP 
#define TWL_RES_ACTIVE                  0x0E  // Resource in ACTIVE 
#define TWL_RES_WRST                    0x0F  // Resource in Warm Reset


// Triton2 TPS659XX General purpose i2c addressing 
//------------------------------------------------------------------------------
// address group 0x4B

#define TWL_BACKUP_REG_A                0x00030014
#define TWL_BACKUP_REG_B                0x00030015
#define TWL_BACKUP_REG_C                0x00030016
#define TWL_BACKUP_REG_D                0x00030017
#define TWL_BACKUP_REG_E                0x00030018
#define TWL_BACKUP_REG_F                0x00030019
#define TWL_BACKUP_REG_G                0x0003001A
#define TWL_BACKUP_REG_H                0x0003001B

// interrupt registers
#define TWL_PWR_ISR1                    0x0003002E
#define TWL_PWR_IMR1                    0x0003002F
#define TWL_PWR_ISR2                    0x00030030
#define TWL_PWR_IMR2                    0x00030031
#define TWL_PWR_SIR                     0x00030032
#define TWL_PWR_EDR1                    0x00030033
#define TWL_PWR_EDR2                    0x00030034
#define TWL_PWR_SIH_CTRL                0x00030035

// power registers on/off & POR mode
#define TWL_CFG_P1_TRANSITION           0x00030036
#define TWL_CFG_P2_TRANSITION           0x00030037
#define TWL_CFG_P3_TRANSITION           0x00030038
#define TWL_CFG_P123_TRANSITION         0x00030039
#define TWL_STS_BOOT                    0x0003003A
#define TWL_CFG_BOOT                    0x0003003B
#define TWL_SHUNDAN                     0x0003003C
#define TWL_BOOT_BCI                    0x0003003D
#define TWL_CFG_PWRANA1                 0x0003003E
#define TWL_CFG_PWRANA2                 0x0003003F
#define TWL_BGAP_TRIM                   0x00030040
#define TWL_BACKUP_MISC_STS             0x00030041
#define TWL_BACKUP_MISC_CFG             0x00030042
#define TWL_BACKUP_MISC_TST             0x00030043
#define TWL_PROTECT_KEY                 0x00030044
#define TWL_STS_HW_CONDITIONS           0x00030045
#define TWL_P1_SW_EVENTS                0x00030046
#define TWL_P2_SW_EVENTS                0x00030047
#define TWL_P3_SW_EVENTS                0x00030048
#define TWL_STS_P123_STATE              0x00030049
#define TWL_PB_CFG                      0x0003004A
#define TWL_PB_WORD_MSB                 0x0003004B
#define TWL_PB_WORD_LSB                 0x0003004C
#define TWL_RESERVED_A                  0x0003004D
#define TWL_RESERVED_B                  0x0003004E
#define TWL_RESERVED_C                  0x0003004F
#define TWL_RESERVED_D                  0x00030050
#define TWL_RESERVED_E                  0x00030051
#define TWL_SEQ_ADD_W2P                 0x00030052
#define TWL_SEQ_ADD_P2A                 0x00030053
#define TWL_SEQ_ADD_A2W                 0x00030054
#define TWL_SEQ_ADD_A2S                 0x00030055
#define TWL_SEQ_ADD_S2A12               0x00030056
#define TWL_SEQ_ADD_S2A3                0x00030057
#define TWL_SEQ_ADD_WARM                0x00030058
#define TWL_MEMORY_ADDRESS              0x00030059
#define TWL_MEMORY_DATA                 0x0003005A

// pm receiver (un)secure mode
#define TWL_SC_CONFIG                   0x0003005B
#define TWL_SC_DETECT1                  0x0003005C
#define TWL_SC_DETECT2                  0x0003005D
#define TWL_WATCHDOG_CFG                0x0003005E
#define TWL_IT_CHECK_CFG                0x0003005F
#define TWL_VIBRATOR_CFG                0x00030060
#define TWL_DCDC_GLOBAL_CFG             0x00030061
#define TWL_VDD1_TRIM1                  0x00030062
#define TWL_VDD1_TRIM2                  0x00030063
#define TWL_VDD2_TRIM1                  0x00030064
#define TWL_VDD2_TRIM2                  0x00030065
#define TWL_VIO_TRIM1                   0x00030066
#define TWL_VIO_TRIM2                   0x00030067
#define TWL_MISC_CFG                    0x00030068
#define TWL_LS_TST_A                    0x00030069
#define TWL_LS_TST_B                    0x0003006A
#define TWL_LS_TST_C                    0x0003006B
#define TWL_LS_TST_D                    0x0003006C
#define TWL_BB_CFG                      0x0003006D
#define TWL_MISC_TST                    0x0003006E
#define TWL_TRIM1                       0x0003006F
#define TWL_TRIM2                       0x00030070
#define TWL_DCDC_TIMEOUT                0x00030071
#define TWL_VAUX1_DEV_GRP               0x00030072
#define TWL_VAUX1_TYPE                  0x00030073
#define TWL_VAUX1_REMAP                 0x00030074
#define TWL_VAUX1_DEDICATED             0x00030075
#define TWL_VAUX2_DEV_GRP               0x00030076
#define TWL_VAUX2_TYPE                  0x00030077
#define TWL_VAUX2_REMAP                 0x00030078
#define TWL_VAUX2_DEDICATED             0x00030079
#define TWL_VAUX3_DEV_GRP               0x0003007A
#define TWL_VAUX3_TYPE                  0x0003007B
#define TWL_VAUX3_REMAP                 0x0003007C
#define TWL_VAUX3_DEDICATED             0x0003007D
#define TWL_VAUX4_DEV_GRP               0x0003007E
#define TWL_VAUX4_TYPE                  0x0003007F
#define TWL_VAUX4_REMAP                 0x00030080
#define TWL_VAUX4_DEDICATED             0x00030081
#define TWL_VMMC1_DEV_GRP               0x00030082
#define TWL_VMMC1_TYPE                  0x00030083
#define TWL_VMMC1_REMAP                 0x00030084
#define TWL_VMMC1_DEDICATED             0x00030085
#define TWL_VMMC2_DEV_GRP               0x00030086
#define TWL_VMMC2_TYPE                  0x00030087
#define TWL_VMMC2_REMAP                 0x00030088
#define TWL_VMMC2_DEDICATED             0x00030089
#define TWL_VPLL1_DEV_GRP               0x0003008A
#define TWL_VPLL1_TYPE                  0x0003008B
#define TWL_VPLL1_REMAP                 0x0003008C
#define TWL_VPLL1_DEDICATED             0x0003008D
#define TWL_VPLL2_DEV_GRP               0x0003008E
#define TWL_VPLL2_TYPE                  0x0003008F
#define TWL_VPLL2_REMAP                 0x00030090
#define TWL_VPLL2_DEDICATED             0x00030091
#define TWL_VSIM_DEV_GRP                0x00030092
#define TWL_VSIM_TYPE                   0x00030093
#define TWL_VSIM_REMAP                  0x00030094
#define TWL_VSIM_DEDICATED              0x00030095
#define TWL_VDAC_DEV_GRP                0x00030096
#define TWL_VDAC_TYPE                   0x00030097
#define TWL_VDAC_REMAP                  0x00030098
#define TWL_VDAC_DEDICATED              0x00030099
#define TWL_VINTANA1_DEV_GRP            0x0003009A
#define TWL_VINTANA1_TYPE               0x0003009B
#define TWL_VINTANA1_REMAP              0x0003009C
#define TWL_VINTANA1_DEDICATED          0x0003009D
#define TWL_VINTANA2_DEV_GRP            0x0003009E
#define TWL_VINTANA2_TYPE               0x0003009F
#define TWL_VINTANA2_REMAP              0x000300A0
#define TWL_VINTANA2_DEDICATED          0x000300A1
#define TWL_VINTDIG_DEV_GRP             0x000300A2
#define TWL_VINTDIG_TYPE                0x000300A3
#define TWL_VINTDIG_REMAP               0x000300A4
#define TWL_VINTDIG_DEDICATED           0x000300A5
#define TWL_VIO_DEV_GRP                 0x000300A6
#define TWL_VIO_TYPE                    0x000300A7
#define TWL_VIO_REMAP                   0x000300A8
#define TWL_VIO_CFG                     0x000300A9
#define TWL_VIO_MISC_CFG                0x000300AA
#define TWL_VIO_TEST1                   0x000300AB
#define TWL_VIO_TEST2                   0x000300AC
#define TWL_VIO_OSC                     0x000300AD
#define TWL_VIO_RESERVED                0x000300AE
#define TWL_VIO_VSEL                    0x000300AF
#define TWL_VDD1_DEV_GRP                0x000300B0
#define TWL_VDD1_TYPE                   0x000300B1
#define TWL_VDD1_REMAP                  0x000300B2
#define TWL_VDD1_CFG                    0x000300B3
#define TWL_VDD1_MISC_CFG               0x000300B4
#define TWL_VDD1_TEST1                  0x000300B5
#define TWL_VDD1_TEST2                  0x000300B6
#define TWL_VDD1_OSC                    0x000300B7
#define TWL_VDD1_RESERVED               0x000300B8
#define TWL_VDD1_VSEL                   0x000300B9
#define TWL_VDD1_VMODE_CFG              0x000300BA
#define TWL_VDD1_VFLOOR                 0x000300BB
#define TWL_VDD1_VROOF                  0x000300BC
#define TWL_VDD1_STEP                   0x000300BD
#define TWL_VDD2_DEV_GRP                0x000300BE
#define TWL_VDD2_TYPE                   0x000300BF
#define TWL_VDD2_REMAP                  0x000300C0
#define TWL_VDD2_CFG                    0x000300C1
#define TWL_VDD2_MISC_CFG               0x000300C2
#define TWL_VDD2_TEST1                  0x000300C3
#define TWL_VDD2_TEST2                  0x000300C4
#define TWL_VDD2_OSC                    0x000300C5
#define TWL_VDD2_RESERVED               0x000300C6
#define TWL_VDD2_VSEL                   0x000300C7
#define TWL_VDD2_VMODE_CFG              0x000300C8
#define TWL_VDD2_VFLOOR                 0x000300C9
#define TWL_VDD2_VROOF                  0x000300CA
#define TWL_VDD2_STEP                   0x000300CB
#define TWL_VUSB1V5_DEV_GRP             0x000300CC
#define TWL_VUSB1V5_TYPE                0x000300CD
#define TWL_VUSB1V5_REMAP               0x000300CE
#define TWL_VUSB1V8_DEV_GRP             0x000300CF
#define TWL_VUSB1V8_TYPE                0x000300D0
#define TWL_VUSB1V8_REMAP               0x000300D1
#define TWL_VUSB3V1_DEV_GRP             0x000300D2
#define TWL_VUSB3V1_TYPE                0x000300D3
#define TWL_VUSB3V1_REMAP               0x000300D4
#define TWL_VUSBCP_DEV_GRP              0x000300D5
#define TWL_VUSBCP_TYPE                 0x000300D6
#define TWL_VUSBCP_REMAP                0x000300D7
#define TWL_VUSB_DEDICATED1             0x000300D8
#define TWL_VUSB_DEDICATED2             0x000300D9
#define TWL_REGEN_DEV_GRP               0x000300DA
#define TWL_REGEN_TYPE                  0x000300DB
#define TWL_REGEN_REMAP                 0x000300DC
#define TWL_NRESPWRON_DEV_GRP           0x000300DD
#define TWL_NRESPWRON_TYPE              0x000300DE
#define TWL_NRESPWRON_REMAP             0x000300DF
#define TWL_CLKEN_DEV_GRP               0x000300E0
#define TWL_CLKEN_TYPE                  0x000300E1
#define TWL_CLKEN_REMAP                 0x000300E2
#define TWL_SYSEN_DEV_GRP               0x000300E3
#define TWL_SYSEN_TYPE                  0x000300E4
#define TWL_SYSEN_REMAP                 0x000300E5
#define TWL_HFCLKOUT_DEV_GRP            0x000300E6
#define TWL_HFCLKOUT_TYPE               0x000300E7
#define TWL_HFCLKOUT_REMAP              0x000300E8
#define TWL_32KCLKOUT_DEV_GRP           0x000300E9
#define TWL_32KCLKOUT_TYPE              0x000300EA
#define TWL_32KCLKOUT_REMAP             0x000300EB
#define TWL_TRITON_RESET_DEV_GRP        0x000300EC
#define TWL_TRITON_RESET_TYPE           0x000300ED
#define TWL_TRITON_RESET_REMAP          0x000300EE
#define TWL_MAINREF_DEV_GRP             0x000300EF
#define TWL_MAINREF_TYPE                0x000300F0
#define TWL_MAINREF_REMAP               0x000300F1

#define DCDC_GLOBAL_CFG_CD1_ACTIVELOW   (1<<6)
#define DCDC_GLOBAL_CFG_CD2_ACTIVELOW   (1<<7)


// rtc
#define TWL_SECONDS_REG                 0x0003001C
#define TWL_MINUTES_REG                 0x0003001D
#define TWL_HOURS_REG                   0x0003001E
#define TWL_DAYS_REG                    0x0003001F
#define TWL_MONTHS_REG                  0x00030020
#define TWL_YEARS_REG                   0x00030021
#define TWL_WEEKS_REG                   0x00030022
#define TWL_ALARM_SECONDS_REG           0x00030023
#define TWL_ALARM_MINUTES_REG           0x00030024
#define TWL_ALARM_HOURS_REG             0x00030025
#define TWL_ALARM_DAYS_REG              0x00030026
#define TWL_ALARM_MONTHS_REG            0x00030027
#define TWL_ALARM_YEARS_REG             0x00030028
#define TWL_RTC_CTRL_REG                0x00030029
#define TWL_RTC_STATUS_REG              0x0003002A
#define TWL_RTC_INTERRUPTS_REG          0x0003002B
#define TWL_RTC_COMP_LSB_REG            0x0003002C
#define TWL_RTC_COMP_MSB_REG            0x0003002D

// secure registers
#define TWL_SECURED_REG_A               0x00030000
#define TWL_SECURED_REG_B               0x00030001
#define TWL_SECURED_REG_C               0x00030002
#define TWL_SECURED_REG_D               0x00030003
#define TWL_SECURED_REG_E               0x00030004
#define TWL_SECURED_REG_F               0x00030005
#define TWL_SECURED_REG_G               0x00030006
#define TWL_SECURED_REG_H               0x00030007
#define TWL_SECURED_REG_I               0x00030008
#define TWL_SECURED_REG_J               0x00030009
#define TWL_SECURED_REG_K               0x0003000A
#define TWL_SECURED_REG_L               0x0003000B
#define TWL_SECURED_REG_M               0x0003000C
#define TWL_SECURED_REG_N               0x0003000D
#define TWL_SECURED_REG_O               0x0003000E
#define TWL_SECURED_REG_P               0x0003000F
#define TWL_SECURED_REG_Q               0x00030010
#define TWL_SECURED_REG_R               0x00030011
#define TWL_SECURED_REG_S               0x00030012
#define TWL_SECURED_REG_U               0x00030013


//------------------------------------------------------------------------------
// address group 0x4A

// interrupts
#define TWL_BCIISR1A                    0x000200B9
#define TWL_BCIISR2A                    0x000200BA
#define TWL_BCIIMR1A                    0x000200BB
#define TWL_BCIIMR2A                    0x000200BC
#define TWL_BCIISR1B                    0x000200BD
#define TWL_BCIISR2B                    0x000200BE
#define TWL_BCIIMR1B                    0x000200BF
#define TWL_BCIIMR2B                    0x000200C0
#define TWL_BCISIR1                     0x000200C1
#define TWL_BCISIR2                     0x000200C2
#define TWL_BCIEDR1                     0x000200C3
#define TWL_BCIEDR2                     0x000200C4
#define TWL_BCIEDR3                     0x000200C5
#define TWL_BCISIHCTRL                  0x000200C6

// madc
#define TWL_CTRL1                       0x00020000
#define TWL_CTRL2                       0x00020001
#define TWL_RTSELECT_LSB                0x00020002
#define TWL_RTSELECT_MSB                0x00020003
#define TWL_RTAVERAGE_LSB               0x00020004
#define TWL_RTAVERAGE_MSB               0x00020005
#define TWL_SW1SELECT_LSB               0x00020006
#define TWL_SW1SELECT_MSB               0x00020007
#define TWL_SW1AVERAGE_LSB              0x00020008
#define TWL_SW1AVERAGE_MSB              0x00020009
#define TWL_SW2SELECT_LSB               0x0002000A
#define TWL_SW2SELECT_MSB               0x0002000B
#define TWL_SW2AVERAGE_LSB              0x0002000C
#define TWL_SW2AVERAGE_MSB              0x0002000D
#define TWL_BCI_USBAVERAGE              0x0002000E
#define TWL_ACQUISITION                 0x0002000F
#define TWL_USBREF_LSB                  0x00020010
#define TWL_USBREF_MSB                  0x00020011
#define TWL_CTRL_SW1                    0x00020012
#define TWL_CTRL_SW2                    0x00020013
#define TWL_MADC_TEST                   0x00020014
#define TWL_GP_MADC_TEST1               0x00020015
#define TWL_GP_MADC_TEST2               0x00020016
#define TWL_RTCH0_LSB                   0x00020017
#define TWL_RTCH0_MSB                   0x00020018
#define TWL_RTCH1_LSB                   0x00020019
#define TWL_RTCH1_MSB                   0x0002001A
#define TWL_RTCH2_LSB                   0x0002001B
#define TWL_RTCH2_MSB                   0x0002001C
#define TWL_RTCH3_LSB                   0x0002001D
#define TWL_RTCH3_MSB                   0x0002001E
#define TWL_RTCH4_LSB                   0x0002001F
#define TWL_RTCH4_MSB                   0x00020020
#define TWL_RTCH5_LSB                   0x00020021
#define TWL_RTCH5_MSB                   0x00020022
#define TWL_RTCH6_LSB                   0x00020023
#define TWL_RTCH6_MSB                   0x00020024
#define TWL_RTCH7_LSB                   0x00020025
#define TWL_RTCH7_MSB                   0x00020026
#define TWL_RTCH8_LSB                   0x00020027
#define TWL_RTCH8_MSB                   0x00020028
#define TWL_RTCH9_LSB                   0x00020029
#define TWL_RTCH9_MSB                   0x0002002A
#define TWL_RTCH10_LSB                  0x0002002B
#define TWL_RTCH10_MSB                  0x0002002C
#define TWL_RTCH11_LSB                  0x0002002D
#define TWL_RTCH11_MSB                  0x0002002E
#define TWL_RTCH12_LSB                  0x0002002F
#define TWL_RTCH12_MSB                  0x00020030
#define TWL_RTCH13_LSB                  0x00020031
#define TWL_RTCH13_MSB                  0x00020032
#define TWL_RTCH14_LSB                  0x00020033
#define TWL_RTCH14_MSB                  0x00020034
#define TWL_RTCH15_LSB                  0x00020035
#define TWL_RTCH15_MSB                  0x00020036
#define TWL_GPCH0_LSB                   0x00020037
#define TWL_GPCH0_MSB                   0x00020038
#define TWL_GPCH1_LSB                   0x00020039
#define TWL_GPCH1_MSB                   0x0002003A
#define TWL_GPCH2_LSB                   0x0002003B
#define TWL_GPCH2_MSB                   0x0002003C
#define TWL_GPCH3_LSB                   0x0002003D
#define TWL_GPCH3_MSB                   0x0002003E
#define TWL_GPCH4_LSB                   0x0002003F
#define TWL_GPCH4_MSB                   0x00020040
#define TWL_GPCH5_LSB                   0x00020041
#define TWL_GPCH5_MSB                   0x00020042
#define TWL_GPCH6_LSB                   0x00020043
#define TWL_GPCH6_MSB                   0x00020044
#define TWL_GPCH7_LSB                   0x00020045
#define TWL_GPCH7_MSB                   0x00020046
#define TWL_GPCH8_LSB                   0x00020047
#define TWL_GPCH8_MSB                   0x00020048
#define TWL_GPCH9_LSB                   0x00020049
#define TWL_GPCH9_MSB                   0x0002004A
#define TWL_GPCH10_LSB                  0x0002004B
#define TWL_GPCH10_MSB                  0x0002004C
#define TWL_GPCH11_LSB                  0x0002004D
#define TWL_GPCH11_MSB                  0x0002004E
#define TWL_GPCH12_LSB                  0x0002004F
#define TWL_GPCH12_MSB                  0x00020050
#define TWL_GPCH13_LSB                  0x00020051
#define TWL_GPCH13_MSB                  0x00020052
#define TWL_GPCH14_LSB                  0x00020053
#define TWL_GPCH14_MSB                  0x00020054
#define TWL_GPCH15_LSB                  0x00020055
#define TWL_GPCH15_MSB                  0x00020056
#define TWL_BCICH0_LSB                  0x00020057
#define TWL_BCICH0_MSB                  0x00020058
#define TWL_BCICH1_LSB                  0x00020059
#define TWL_BCICH1_MSB                  0x0002005A
#define TWL_BCICH2_LSB                  0x0002005B
#define TWL_BCICH2_MSB                  0x0002005C
#define TWL_BCICH3_LSB                  0x0002005D
#define TWL_BCICH3_MSB                  0x0002005E
#define TWL_BCICH4_LSB                  0x0002005F
#define TWL_BCICH4_MSB                  0x00020060
#define TWL_MADC_ISR1                   0x00020061
#define TWL_MADC_IMR1                   0x00020062
#define TWL_MADC_ISR2                   0x00020063
#define TWL_MADC_IMR2                   0x00020064
#define TWL_MADC_SIR                    0x00020065
#define TWL_MADC_EDR                    0x00020066
#define TWL_MADC_SIH_CTRL               0x00020067

// main charge
#define TWL_BCIMDEN                     0x00020074
#define TWL_BCIMDKEY                    0x00020075
#define TWL_BCIMSTATEC                  0x00020076
#define TWL_BCIMSTATEP                  0x00020077
#define TWL_BCIVBAT1                    0x00020078
#define TWL_BCIVBAT2                    0x00020079
#define TWL_BCITBAT1                    0x0002007A
#define TWL_BCITBAT2                    0x0002007B
#define TWL_BCIICHG1                    0x0002007C
#define TWL_BCIICHG2                    0x0002007D
#define TWL_BCIVAC1                     0x0002007E
#define TWL_BCIVAC2                     0x0002007F
#define TWL_BCIVBUS1                    0x00020080
#define TWL_BCIVBUS2                    0x00020081
#define TWL_BCIMFSTS2                   0x00020082
#define TWL_BCIMFSTS3                   0x00020083
#define TWL_BCIMFSTS4                   0x00020084
#define TWL_BCIMFKEY                    0x00020085
#define TWL_BCIMFEN1                    0x00020086
#define TWL_BCIMFEN2                    0x00020087
#define TWL_BCIMFEN3                    0x00020088
#define TWL_BCIMFEN4                    0x00020089
#define TWL_BCIMFTH1                    0x0002008A
#define TWL_BCIMFTH2                    0x0002008B
#define TWL_BCIMFTH3                    0x0002008C
#define TWL_BCIMFTH4                    0x0002008D
#define TWL_BCIMFTH5                    0x0002008E
#define TWL_BCIMFTH6                    0x0002008F
#define TWL_BCIMFTH7                    0x00020090
#define TWL_BCIMFTH8                    0x00020091
#define TWL_BCIMFTH9                    0x00020092
#define TWL_BCITIMER1                   0x00020093
#define TWL_BCITIMER2                   0x00020094
#define TWL_BCIWDKEY                    0x00020095
#define TWL_BCIWD                       0x00020096
#define TWL_BCICTL1                     0x00020097
#define TWL_BCICTL2                     0x00020098
#define TWL_BCIVREF1                    0x00020099
#define TWL_BCIVREF2                    0x0002009A
#define TWL_BCIIREF1                    0x0002009B
#define TWL_BCIIREF2                    0x0002009C
#define TWL_BCIPWM2                     0x0002009D
#define TWL_BCIPWM1                     0x0002009E
#define TWL_BCITRIM1                    0x0002009F
#define TWL_BCITRIM2                    0x000200A0
#define TWL_BCITRIM3                    0x000200A1
#define TWL_BCITRIM4                    0x000200A2
#define TWL_BCIVREFCOMB1                0x000200A3
#define TWL_BCIVREFCOMB2                0x000200A4
#define TWL_BCIIREFCOMB1                0x000200A5
#define TWL_BCIIREFCOMB2                0x000200A6
#define TWL_BCIMNTEST1                  0x000200A7
#define TWL_BCIMNTEST2                  0x000200A8
#define TWL_BCIMNTEST3                  0x000200A9

// pwm regisers
#define TWL_LEDEN                       0x000200EE
#define TWL_PWMAON                      0x000200EF
#define TWL_PWMAOFF                     0x000200F0
#define TWL_PWMBON                      0x000200F1
#define TWL_PWMBOFF                     0x000200F2

#define TWL_PWM0ON                      0x000200F8
#define TWL_PWM0OFF                     0x000200F9
#define TWL_PWM1ON                      0x000200FB
#define TWL_PWM1OFF                     0x000200FC

// keypad 
#define TWL_KEYP_CTRL_REG               0x000200D2
#define TWL_KEY_DEB_REG                 0x000200D3
#define TWL_LONG_KEY_REG1               0x000200D4
#define TWL_LK_PTV_REG                  0x000200D5
#define TWL_TIME_OUT_REG1               0x000200D6
#define TWL_TIME_OUT_REG2               0x000200D7
#define TWL_KBC_REG                     0x000200D8
#define TWL_KBR_REG                     0x000200D9
#define TWL_KEYP_SMS                    0x000200DA
#define TWL_FULL_CODE_7_0               0x000200DB
#define TWL_FULL_CODE_15_8              0x000200DC
#define TWL_FULL_CODE_23_16             0x000200DD
#define TWL_FULL_CODE_31_24             0x000200DE
#define TWL_FULL_CODE_39_32             0x000200DF
#define TWL_FULL_CODE_47_40             0x000200E0
#define TWL_FULL_CODE_55_48             0x000200E1
#define TWL_FULL_CODE_63_56             0x000200E2
#define TWL_KEYP_ISR1                   0x000200E3
#define TWL_KEYP_IMR1                   0x000200E4
#define TWL_KEYP_ISR2                   0x000200E5
#define TWL_KEYP_IMR2                   0x000200E6
#define TWL_KEYP_SIR                    0x000200E7
#define TWL_KEYP_EDR                    0x000200E8
#define TWL_KEYP_SIH_CTRL               0x000200E9


//------------------------------------------------------------------------------
// address group 0x48

// usb
#define TWL_VENDOR_ID_LO                0x00000000
#define TWL_VENDOR_ID_HI                0x00000001
#define TWL_PRODUCT_ID_LO               0x00000002
#define TWL_PRODUCT_ID_HI               0x00000003
#define TWL_FUNC_CTRL                   0x00000004
#define TWL_FUNC_CTRL_SET               0x00000005
#define TWL_FUNC_CTRL_CLR               0x00000006
#define TWL_IFC_CTRL                    0x00000007
#define TWL_IFC_CTRL_SET                0x00000008
#define TWL_IFC_CTRL_CLR                0x00000009
#define TWL_OTG_CTRL                    0x0000000A
#define TWL_OTG_CTRL_SET                0x0000000B
#define TWL_OTG_CTRL_CLR                0x0000000C
#define TWL_USB_INT_EN_RISE             0x0000000D
#define TWL_USB_INT_EN_RISE_SET         0x0000000E
#define TWL_USB_INT_EN_RISE_CLR         0x0000000F
#define TWL_USB_INT_EN_FALL             0x00000010
#define TWL_USB_INT_EN_FALL_SET         0x00000011
#define TWL_USB_INT_EN_FALL_CLR         0x00000012
#define TWL_USB_INT_STS                 0x00000013
#define TWL_USB_INT_LATCH               0x00000014
#define TWL_DEBUG                       0x00000015
#define TWL_SCRATCH_REG                 0x00000016
#define TWL_SCRATCH_REG_SET             0x00000017
#define TWL_SCRATCH_REG_CLR             0x00000018
#define TWL_CARKIT_CTRL_SET             0x0000001A
#define TWL_CARKIT_CTRL                 0x00000019
#define TWL_CARKIT_CTRL_CLR             0x0000001B
#define TWL_CARKIT_INT_DELAY            0x0000001C
#define TWL_CARKIT_INT_EN               0x0000001D
#define TWL_CARKIT_INT_EN_SET           0x0000001E
#define TWL_CARKIT_INT_EN_CLR           0x0000001F
#define TWL_CARKIT_INT_STS              0x00000020
#define TWL_CARKIT_INT_LATCH            0x00000021
#define TWL_CARKIT_PLS_CTRL             0x00000022
#define TWL_CARKIT_PLS_CTRL_SET         0x00000023
#define TWL_CARKIT_PLS_CTRL_CLR         0x00000024
#define TWL_TRANS_POS_WIDTH             0x00000025
#define TWL_TRANS_NEG_WIDTH             0x00000026
#define TWL_RCV_PLTY_RECOVERY           0x00000027
#define TWL_MCPC_CTRL                   0x00000030
#define TWL_MCPC_CTRL_SET               0x00000031
#define TWL_MCPC_CTRL_CLR               0x00000032
#define TWL_MCPC_IO_CTRL                0x00000033
#define TWL_MCPC_IO_CTRL_SET            0x00000034
#define TWL_MCPC_IO_CTRL_CLR            0x00000035
#define TWL_MCPC_CTRL2                  0x00000036
#define TWL_MCPC_CTRL2_SET              0x00000037
#define TWL_MCPC_CTRL2_CLR              0x00000038
#define TWL_OTHER_FUNC_CTRL             0x00000080
#define TWL_OTHER_FUNC_CTRL_SET         0x00000081
#define TWL_OTHER_FUNC_CTRL_CLR         0x00000082
#define TWL_OTHER_IFC_CTRL              0x00000083
#define TWL_OTHER_IFC_CTRL_SET          0x00000084
#define TWL_OTHER_IFC_CTRL_CLR          0x00000085
#define TWL_OTHER_INT_EN_RISE           0x00000086
#define TWL_OTHER_INT_EN_RISE_SET       0x00000087
#define TWL_OTHER_INT_EN_RISE_CLR       0x00000088
#define TWL_OTHER_INT_EN_FALL           0x00000089
#define TWL_OTHER_INT_EN_FALL_SET       0x0000008A
#define TWL_OTHER_INT_EN_FALL_CLR       0x0000008B
#define TWL_OTHER_INT_STS               0x0000008C
#define TWL_OTHER_INT_LATCH             0x0000008D
#define TWL_ID_INT_EN_RISE              0x0000008E
#define TWL_ID_INT_EN_RISE_SET          0x0000008F
#define TWL_ID_INT_EN_RISE_CLR          0x00000090
#define TWL_ID_INT_EN_FALL              0x00000091
#define TWL_ID_INT_EN_FALL_SET          0x00000092
#define TWL_ID_INT_EN_FALL_CLR          0x00000093
#define TWL_ID_INT_STS                  0x00000094
#define TWL_ID_INT_LATCH                0x00000095
#define TWL_ID_STATUS                   0x00000096
#define TWL_CARKIT_SM_1_INT_EN          0x00000097
#define TWL_CARKIT_SM_1_INT_EN_SET      0x00000098
#define TWL_CARKIT_SM_1_INT_EN_CLR      0x00000099
#define TWL_CARKIT_SM_1_INT_STS         0x0000009A
#define TWL_CARKIT_SM_1_INT_LATCH       0x0000009B
#define TWL_CARKIT_SM_2_INT_EN          0x0000009C
#define TWL_CARKIT_SM_2_INT_EN_SET      0x0000009D
#define TWL_CARKIT_SM_2_INT_EN_CLR      0x0000009E
#define TWL_CARKIT_SM_2_INT_STS         0x0000009F
#define TWL_CARKIT_SM_2_INT_LATCH       0x000000A0
#define TWL_CARKIT_SM_CTRL              0x000000A1
#define TWL_CARKIT_SM_CTRL_SET          0x000000A2
#define TWL_CARKIT_SM_CTRL_CLR          0x000000A3
#define TWL_CARKIT_SM_CMD               0x000000A4
#define TWL_CARKIT_SM_CMD_SET           0x000000A5
#define TWL_CARKIT_SM_CMD_CLR           0x000000A6
#define TWL_CARKIT_SM_CMD_STS           0x000000A7
#define TWL_CARKIT_SM_STATUS            0x000000A8
#define TWL_CARKIT_SM_NEXT_STATUS       0x000000A9
#define TWL_CARKIT_SM_ERR_STATUS        0x000000AA
#define TWL_CARKIT_SM_CTRL_STATE        0x000000AB
#define TWL_POWER_CTRL                  0x000000AC
#define TWL_POWER_CTRL_SET              0x000000AD
#define TWL_POWER_CTRL_CLR              0x000000AE
#define TWL_OTHER_IFC_CTRL2             0x000000AF
#define TWL_OTHER_IFC_CTRL2_SET         0x000000B0
#define TWL_OTHER_IFC_CTRL2_CLR         0x000000B1
#define TWL_REG_CTRL_EN                 0x000000B2
#define TWL_REG_CTRL_EN_SET             0x000000B3
#define TWL_REG_CTRL_EN_CLR             0x000000B4
#define TWL_REG_CTRL_ERROR              0x000000B5
#define TWL_OTHER_FUNC_CTRL2            0x000000B8
#define TWL_OTHER_FUNC_CTRL2_SET        0x000000B9
#define TWL_OTHER_FUNC_CTRL2_CLR        0x000000BA
#define TWL_CARKIT_ANA_CTRL             0x000000BB
#define TWL_CARKIT_ANA_CTRL_SET         0x000000BC
#define TWL_CARKIT_ANA_CTRL_CLR         0x000000BD
#define TWL_VBUS_DEBOUNCE               0x000000C0
#define TWL_ID_DEBOUNCE                 0x000000C1
#define TWL_TPH_DP_CON_MIN              0x000000C2
#define TWL_TPH_DP_CON_MAX              0x000000C3
#define TWL_TCR_DP_CON_MIN              0x000000C4
#define TWL_TCR_DP_CON_MAX              0x000000C5
#define TWL_TPH_DP_PD_SHORT             0x000000C6
#define TWL_TPH_CMD_DLY                 0x000000C7
#define TWL_TPH_DET_RST                 0x000000C8
#define TWL_TPH_AUD_BIAS                0x000000C9
#define TWL_TCR_UART_DET_MIN            0x000000CA
#define TWL_TCR_UART_DET_MAX            0x000000CB
#define TWL_TPH_ID_INT_PW               0x000000CD
#define TWL_TACC_ID_INT_WAIT            0x000000CE
#define TWL_TACC_ID_INT_PW              0x000000CF
#define TWL_TPH_CMD_WAIT                0x000000D0
#define TWL_TPH_ACK_WAIT                0x000000D1
#define TWL_TPH_DP_DISC_DET             0x000000D2
#define TWL_VBAT_TIMER                  0x000000D3
#define TWL_CARKIT_4W_DEBUG             0x000000E0
#define TWL_CARKIT_5W_DEBUG             0x000000E1
#define TWL_TEST_CTRL_SET               0x000000EA
#define TWL_TEST_CTRL_CLR               0x000000EB
#define TWL_TEST_CARKIT_SET             0x000000EC
#define TWL_TEST_CARKIT_CLR             0x000000ED
#define TWL_TEST_POWER_SET              0x000000EE
#define TWL_TEST_POWER_CLR              0x000000EF
#define TWL_TEST_ULPI                   0x000000F0
#define TWL_TXVR_EN_TEST_SET            0x000000F2
#define TWL_TXVR_EN_TEST_CLR            0x000000F3
#define TWL_VBUS_EN_TEST                0x000000F4
#define TWL_ID_EN_TEST                  0x000000F5
#define TWL_PSM_EN_TEST_SET             0x000000F6
#define TWL_PSM_EN_TEST_CLR             0x000000F7
#define TWL_PHY_TRIM_CTRL               0x000000FC
#define TWL_PHY_PWR_CTRL                0x000000FD
#define TWL_PHY_CLK_CTRL                0x000000FE
#define TWL_PHY_CLK_CTRL_STS            0x000000FF


//------------------------------------------------------------------------------
// address group 0x49

// audio voice
#define TWL_CODEC_MODE                  0x00010001
#define TWL_OPTION                      0x00010002
#define TWL_MICBIAS_CTL                 0x00010004
#define TWL_ANAMICL                     0x00010005
#define TWL_ANAMICR                     0x00010006
#define TWL_AVADC_CTL                   0x00010007
#define TWL_ADCMICSEL                   0x00010008
#define TWL_DIGMIXING                   0x00010009
#define TWL_ATXL1PGA                    0x0001000A
#define TWL_ATXR1PGA                    0x0001000B
#define TWL_AVTXL2PGA                   0x0001000C
#define TWL_AVTXR2PGA                   0x0001000D
#define TWL_AUDIO_IF                    0x0001000E
#define TWL_VOICE_IF                    0x0001000F
#define TWL_ARXR1PGA                    0x00010010
#define TWL_ARXL1PGA                    0x00010011
#define TWL_ARXR2PGA                    0x00010012
#define TWL_ARXL2PGA                    0x00010013
#define TWL_VRXPGA                      0x00010014
#define TWL_VSTPGA                      0x00010015
#define TWL_VRX2ARXPGA                  0x00010016
#define TWL_AVDAC_CTL                   0x00010017
#define TWL_ARX2VTXPGA                  0x00010018
#define TWL_ARXL1_APGA_CTL              0x00010019
#define TWL_ARXR1_APGA_CTL              0x0001001A
#define TWL_ARXL2_APGA_CTL              0x0001001B
#define TWL_ARXR2_APGA_CTL              0x0001001C
#define TWL_ATX2ARXPGA                  0x0001001D
#define TWL_BT_IF                       0x0001001E
#define TWL_BTPGA                       0x0001001F
#define TWL_BTSTPGA                     0x00010020
#define TWL_EAR_CTL                     0x00010021
#define TWL_HS_SEL                      0x00010022
#define TWL_HS_GAIN_SET                 0x00010023
#define TWL_HS_POPN_SET                 0x00010024
#define TWL_PREDL_CTL                   0x00010025
#define TWL_PREDR_CTL                   0x00010026
#define TWL_PRECKL_CTL                  0x00010027
#define TWL_PRECKR_CTL                  0x00010028
#define TWL_HFL_CTL                     0x00010029
#define TWL_HFR_CTL                     0x0001002A
#define TWL_ALC_CTL                     0x0001002B
#define TWL_ALC_SET1                    0x0001002C
#define TWL_ALC_SET2                    0x0001002D
#define TWL_BOOST_CTL                   0x0001002E
#define TWL_SOFTVOL_CTL                 0x0001002F
#define TWL_DTMF_FREQSEL                0x00010030
#define TWL_DTMF_TONEXT1H               0x00010031
#define TWL_DTMF_TONEXT1L               0x00010032
#define TWL_DTMF_TONEXT2H               0x00010033
#define TWL_DTMF_TONEXT2L               0x00010034
#define TWL_DTMF_TONOFF                 0x00010035
#define TWL_DTMF_WANONOFF               0x00010036
#define TWL_I2S_RX_SCRAMBLE_H           0x00010037
#define TWL_I2S_RX_SCRAMBLE_M           0x00010038
#define TWL_I2S_RX_SCRAMBLE_L           0x00010039
#define TWL_APLL_CTL                    0x0001003A
#define TWL_DTMF_CTL                    0x0001003B
#define TWL_DTMF_PGA_CTL2               0x0001003C
#define TWL_DTMF_PGA_CTL1               0x0001003D
#define TWL_MISC_SET_1                  0x0001003E
#define TWL_PCMBTMUX                    0x0001003F
#define TWL_RX_PATH_SEL                 0x00010043
#define TWL_VDL_APGA_CTL                0x00010044
#define TWL_VIBRA_CTL                   0x00010045
#define TWL_VIBRA_SET                   0x00010046
#define TWL_VIBRA_PWM_SET               0x00010047
#define TWL_ANAMIC_GAIN                 0x00010048
#define TWL_MISC_SET_2                  0x00010049

// gpio

#define TWL_GPIODATAIN1                 0x00010098
#define TWL_GPIODATAIN2                 0x00010099
#define TWL_GPIODATAIN3                 0x0001009A
#define TWL_GPIODATADIR1                0x0001009B
#define TWL_GPIODATADIR2                0x0001009C
#define TWL_GPIODATADIR3                0x0001009D
#define TWL_GPIODATAOUT1                0x0001009E
#define TWL_GPIODATAOUT2                0x0001009F
#define TWL_GPIODATAOUT3                0x000100A0
#define TWL_CLEARGPIODATAOUT1           0x000100A1
#define TWL_CLEARGPIODATAOUT2           0x000100A2
#define TWL_CLEARGPIODATAOUT3           0x000100A3
#define TWL_SETGPIODATAOUT1             0x000100A4
#define TWL_SETGPIODATAOUT2             0x000100A5
#define TWL_SETGPIODATAOUT3             0x000100A6
#define TWL_GPIO_DEBEN1                 0x000100A7
#define TWL_GPIO_DEBEN2                 0x000100A8
#define TWL_GPIO_DEBEN3                 0x000100A9
#define TWL_GPIO_CTRL                   0x000100AA
#define TWL_GPIOPUPDCTR1                0x000100AB
#define TWL_GPIOPUPDCTR2                0x000100AC
#define TWL_GPIOPUPDCTR3                0x000100AD
#define TWL_GPIOPUPDCTR4                0x000100AE
#define TWL_GPIOPUPDCTR5                0x000100AF
#define TWL_GPIO_TEST                   0x000100B0
#define TWL_GPIO_ISR1A                  0x000100B1
#define TWL_GPIO_ISR2A                  0x000100B2
#define TWL_GPIO_ISR3A                  0x000100B3
#define TWL_GPIO_IMR1A                  0x000100B4
#define TWL_GPIO_IMR2A                  0x000100B5
#define TWL_GPIO_IMR3A                  0x000100B6
#define TWL_GPIO_ISR1B                  0x000100B7
#define TWL_GPIO_ISR2B                  0x000100B8
#define TWL_GPIO_ISR3B                  0x000100B9
#define TWL_GPIO_IMR1B                  0x000100BA
#define TWL_GPIO_IMR2B                  0x000100BB
#define TWL_GPIO_IMR3B                  0x000100BC
#define TWL_GPIO_SIR1                   0x000100BD
#define TWL_GPIO_SIR2                   0x000100BE
#define TWL_GPIO_SIR3                   0x000100BF
#define TWL_GPIO_EDR1                   0x000100C0
#define TWL_GPIO_EDR2                   0x000100C1
#define TWL_GPIO_EDR3                   0x000100C2
#define TWL_GPIO_EDR4                   0x000100C3
#define TWL_GPIO_EDR5                   0x000100C4
#define TWL_GPIO_SIH_CTRL               0x000100C5


#define TWL_GPIO_CTRL_GPIO0CD1          (1<<0)
#define TWL_GPIO_CTRL_GPIO0CD2          (1<<1)
#define TWL_GPIO_CTRL_GPIO_ON           (1<<2)


// intbr
#define TWL_IDCODE_7_0                  0x00010085
#define TWL_IDCODE_15_8                 0x00010086
#define TWL_IDCODE_23_16                0x00010087
#define TWL_IDCODE_31_24                0x00010088
#define TWL_DIEID_7_0                   0x00010089
#define TWL_DIEID_15_8                  0x0001008A
#define TWL_DIEID_23_16                 0x0001008B
#define TWL_DIEID_31_24                 0x0001008C
#define TWL_DIEID_39_32                 0x0001008D
#define TWL_DIEID_47_40                 0x0001008E
#define TWL_DIEID_55_48                 0x0001008F
#define TWL_DIEID_63_56                 0x00010090
#define TWL_GPBR1                       0x00010091
#define TWL_PMBR1                       0x00010092
#define TWL_PMBR2                       0x00010093
#define TWL_GPPUPDCTR1                  0x00010094
#define TWL_GPPUPDCTR2                  0x00010095
#define TWL_GPPUPDCTR3                  0x00010096
#define TWL_UNLOCK_TEST_REG             0x00010097

// pih
#define TWL_PIH_ISR_P1                  0x00010081
#define TWL_PIH_ISR_P2                  0x00010082
#define TWL_PIH_SIR                     0x00010083

// test
#define TWL_AUDIO_TEST_CTL              0x0001004C
#define TWL_INT_TEST_CTL                0x0001004D
#define TWL_DAC_ADC_TEST_CTL            0x0001004E
#define TWL_RXTX_TRIM_IB                0x0001004F
#define TWL_CLD_CONTROL                 0x00010050
#define TWL_CLD_MODE_TIMING             0x00010051
#define TWL_CLD_TRIM_RAMP               0x00010052
#define TWL_CLD_TESTV_CTL               0x00010053
#define TWL_APLL_TEST_CTL               0x00010054
#define TWL_APLL_TEST_DIV               0x00010055
#define TWL_APLL_TEST_CTL2              0x00010056
#define TWL_APLL_TEST_CUR               0x00010057
#define TWL_DIGMIC_BIAS1_CTL            0x00010058
#define TWL_DIGMIC_BIAS2_CTL            0x00010059
#define TWL_RX_OFFSET_VOICE             0x0001005A
#define TWL_RX_OFFSET_AL1               0x0001005B
#define TWL_RX_OFFSET_AR1               0x0001005C
#define TWL_RX_OFFSET_AL2               0x0001005D
#define TWL_RX_OFFSET_AR2               0x0001005E
#define TWL_OFFSET1                     0x0001005F
#define TWL_OFFSET2                     0x00010060


// TWL_CFG_P1_TRANSITION
// TWL_CFG_P2_TRANSITION
// TWL_CFG_P3_TRANSITION
#define STARTON_SWBUG                   (1 << 7)            
#define STARTON_VBUS                    (1 << 5)
#define STARTON_VBAT                    (1 << 4)
#define STARTON_RTC                     (1 << 3)
#define STARTON_USB                     (1 << 2)
#define STARTON_CHG                     (1 << 1)
#define STARTON_PWON                    (1 << 0)

// TWL_CFG_BOOT
#define CK32K_LOWPWR_EN                 (1 << 7)
#define BOOT_CFG_SHIFT                  (4)
#define BOOT_CFG_MASK                   (7 << BOOT_CFG_SHIFT)
#define HIGH_PERF_SQ                    (1 << 3)
#define SLICER_BYPASS                   (1 << 2)
#define HFCLK_FREQ_SHIFT                (0)
#define HFCLK_FREQ_MASK                 (3 << HFCLK_FREQ_SHIFT)
#define HFCLK_FREQ_NOTPROGRAMMED        (0)
#define HFCLK_FREQ_19_2                 (1)
#define HFCLK_FREQ_26                   (2)
#define HFCLK_FREQ_38_4                 (3)

// TWL_GPBR1
#define MADC_HFCLK_EN                   (1 << 7)
#define MADC_3MHZ_EN                    (1 << 6)
#define BAT_MON_EN                      (1 << 5)
#define DEFAULT_MADC_CLK_EN             (1 << 4)
#define PWM1_ENABLE                     (1 << 3)
#define PWM0_ENABLE                     (1 << 2)
#define PWM1_CLK_ENABLE                 (1 << 1)
#define PWM0_CLK_ENABLE                 (1 << 0)

//------------------------------------------------------------------------------
// address group 0x12

// smartflex
#define TWL_VDD1_SR_CONTROL             0x00120000
#define TWL_VDD2_SR_CONTROL             0x00120001

//------------------------------------------------------------------------------
// address group 0xFF02 (logical address group)
#define TWL_LOGADDR_FULL_CODE_7_0       0xFF0200DB

//-----------------------------------------------------------------------------
//   Union  : TRITON_PMB_MESSAGE
//      Defines the Power Management Bus Message 
//
typedef union {
    unsigned int    msgWord;
    unsigned char   msgByte[4];
} TRITON_PMB_MESSAGE;

//-----------------------------------------------------------------------------
//   Union  : TWL_PMB_ENTRY
//      entry for power message sequencer 
//
typedef struct {
    UINT16          msg;
    UINT8           delay;    
} TWL_PMB_ENTRY;

//------------------------------------------------------------------------------



#ifdef __cplusplus
}
#endif

#endif // __TPS659XX_INTERNALS_H

