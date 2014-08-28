#ifndef _TPS65217_H_
#define _TPS65217_H_

/* I2C chip address */
#define TPS65217_CHIP_PM		0x24


typedef enum {
    kVdd1,
    kVdd2,
    kVoltageRailCount,
} VoltageRail_e;


/* PMIC Register offsets */
#define PMIC_VDD1_REG       (0x21)
#define PMIC_VDD1_OP_REG    (0x22)

#define PMIC_VDD2_REG       (0x24)
#define PMIC_VDD2_OP_REG    (0x25)


/* VDD2 & VDD1 voltage selection register. (VDD2_OP_REG & VDD1_OP_REG) */
#define PMIC_OP_REG_SEL			                (0x7F << 0)

#define PMIC_OP_REG_CMD_MASK                    (0x1 << 7)
#define PMIC_OP_REG_CMD_OP                      (0x0 << 7)
#define PMIC_OP_REG_CMD_SR                      (0x1 << 7)

#define PMIC_OP_REG_SEL_MASK                    (0x7F << 0)
#define PMIC_OP_REG_SEL_1_2                     (0x33 << 0)


/* Registers */
#define PMIC_REG_CHIPID				0x00
#define PMIC_REG_POWER_PATH			0x01
#define PMIC_REG_INTERRUPT			0x02
#define PMIC_REG_CHGCONFIG0			0x03
#define PMIC_REG_CHGCONFIG1			0x04
#define PMIC_REG_CHGCONFIG2			0x05
#define PMIC_REG_CHGCONFIG3			0x06
#define PMIC_REG_WLEDCTRL1			0x07
#define PMIC_REG_WLEDCTRL2			0x08
#define PMIC_REG_MUXCTRL			0x09
#define PMIC_REG_STATUS				0x0A
#define PMIC_REG_PASSWORD			0x0B
#define PMIC_REG_PGOOD				0x0C
#define PMIC_REG_DEFPG				0x0D
#define PMIC_REG_DEFDCDC1			0x0E
#define PMIC_REG_DEFDCDC2			0x0F
#define PMIC_REG_DEFDCDC3			0x10
#define PMIC_REG_DEFSLEW			0x11
#define PMIC_REG_DEFLDO1			0x12
#define PMIC_REG_DEFLDO2			0x13
#define PMIC_REG_DEFLS1				0x14
#define PMIC_REG_DEFLS2				0x15
#define PMIC_REG_ENABLE				0x16
#define PMIC_REG_DEFUVLO			0x18
#define PMIC_REG_SEQ1				0x19
#define PMIC_REG_SEQ2				0x1A
#define PMIC_REG_SEQ3				0x1B
#define PMIC_REG_SEQ4				0x1C
#define PMIC_REG_SEQ5				0x1D
#define PMIC_REG_SEQ6				0x1E

#define PROT_LEVEL_NONE				0x00
#define PROT_LEVEL_1				0x01
#define PROT_LEVEL_2				0x02

#define PASSWORD_LOCK_FOR_WRITE		0x00
#define PASSWORD_UNLOCK				0x7D

#define MASK_ALL_BITS						0xFF

// POWER_PATH
#define PMIC_POWER_PATH_ACSYNC				(1 << 7)
#define PMIC_POWER_PATH_USBSYNC				(1 << 6)
#define PMIC_POWER_PATH_AC_EN				(1 << 5)
#define PMIC_POWER_PATH_USB_EN				(1 << 4)
#define PMIC_POWER_PATH_IAC_LIMIT_MASK		(3 << 2)
#define PMIC_POWER_PATH_IAC_LIMIT_100MA		(0 << 2)
#define PMIC_POWER_PATH_IAC_LIMIT_500MA		(1 << 2)
#define PMIC_POWER_PATH_IAC_LIMIT_1300MA	(2 << 2)
#define PMIC_POWER_PATH_IAC_LIMIT_2500MA	(3 << 2)
#define PMIC_POWER_PATH_IUSB_LIMIT_MASK		(3 << 0)
#define PMIC_POWER_PATH_IUSB_LIMIT_100MA	(0 << 0)
#define PMIC_POWER_PATH_IUSB_LIMIT_500MA	(1 << 0)
#define PMIC_POWER_PATH_IUSB_LIMIT_1300MA	(2 << 0)
#define PMIC_POWER_PATH_IUSB_LIMIT_1800MA	(3 << 0)

// INTERRUPT
#define PMIC_INTERRUPT_PBM					(1 << 6)
#define PMIC_INTERRUPT_ACM					(1 << 5)
#define PMIC_INTERRUPT_USBM					(1 << 4)
#define PMIC_INTERRUPT_PBI					(1 << 2)
#define PMIC_INTERRUPT_ACI					(1 << 1)
#define PMIC_INTERRUPT_USBI					(1 << 0)

// CHGCONFIG0
#define PMIC_CHGCONFIG0_TREG				(1 << 7)
#define PMIC_CHGCONFIG0_DPPM				(1 << 6)
#define PMIC_CHGCONFIG0_TSUSP				(1 << 5)
#define PMIC_CHGCONFIG0_TERMI				(1 << 4)
#define PMIC_CHGCONFIG0_ACTIVE				(1 << 3)
#define PMIC_CHGCONFIG0_CHGTOUT				(1 << 2)
#define PMIC_CHGCONFIG0_PCHGTOU				(1 << 1)
#define PMIC_CHGCONFIG0_BATTEMP				(1 << 0)

// CHGCONFIG1
#define PMIC_CHGCONFIG1_TIMER_MASK			(3 << 6)
#define PMIC_CHGCONFIG1_TIMER_8HR			(3 << 6)
#define PMIC_CHGCONFIG1_TIMER_6HR			(2 << 6)
#define PMIC_CHGCONFIG1_TIMER_5HR			(1 << 6)
#define PMIC_CHGCONFIG1_TIMER_4HR			(0 << 6)
#define PMIC_CHGCONFIG1_TMR_EN				(1 << 5)
#define PMIC_CHGCONFIG1_NTC_TYPE			(1 << 4)
#define PMIC_CHGCONFIG1_RESET				(1 << 3)
#define PMIC_CHGCONFIG1_TERM				(1 << 2)
#define PMIC_CHGCONFIG1_SUSP				(1 << 1)
#define PMIC_CHGCONFIG1_CHG_EN				(1 << 0)

// CHGCONFIG2
#define PMIC_CHGCONFIG2_DYNTMR				(1 << 7)
#define PMIC_CHGCONFIG2_VPRECHG				(1 << 6)
#define PMIC_CHGCONFIG2_VOREG_MASK			(3 << 4)
#define PMIC_CHGCONFIG2_VOREG_4_25V			(3 << 4)
#define PMIC_CHGCONFIG2_VOREG_4_20V			(2 << 4)
#define PMIC_CHGCONFIG2_VOREG_4_15V			(1 << 4)
#define PMIC_CHGCONFIG2_VOREG_4_10V			(0 << 4)

// CHGCONFIG3
#define PMIC_CHGCONFIG3_ICHRG_MASK			(3 << 6)
#define PMIC_CHGCONFIG3_ICHRG_700MA			(3 << 6)
#define PMIC_CHGCONFIG3_ICHRG_500MA			(2 << 6)
#define PMIC_CHGCONFIG3_ICHRG_400MA			(1 << 6)
#define PMIC_CHGCONFIG3_ICHRG_300MA			(0 << 6)
#define PMIC_CHGCONFIG3_DPPMTH_MASK			(3 << 4)
#define PMIC_CHGCONFIG3_DPPMTH_4_25V		(3 << 4)
#define PMIC_CHGCONFIG3_DPPMTH_4_0V			(2 << 4)
#define PMIC_CHGCONFIG3_DPPMTH_3_75V		(1 << 4)
#define PMIC_CHGCONFIG3_DPPMTH_3_5V			(0 << 4)
#define PMIC_CHGCONFIG3_PCHRGT_60MIN		(1 << 3)
#define PMIC_CHGCONFIG3_PCHRGT_30MIN		(0 << 3)
#define PMIC_CHGCONFIG3_TERMIF_MASK			(3 << 1)
#define PMIC_CHGCONFIG3_TERMIF_18			(3 << 1)
#define PMIC_CHGCONFIG3_TERMIF_15			(2 << 1)
#define PMIC_CHGCONFIG3_TERMIF_7_5			(1 << 1)
#define PMIC_CHGCONFIG3_TERMIF_2_5			(0 << 1)
#define PMIC_CHGCONFIG3_TRANGE				(1 << 0)

// WLEDCTRL1
#define PMIC_WLEDCTRL1_ISINC_EN				(1 << 3)
#define PMIC_WLEDCTRL1_ISEL					(1 << 2)
#define PMIC_WLEDCTRL1_FDIM_MASK			(3 << 0)
#define PMIC_WLEDCTRL1_FDIM_1000HZ			(3 << 0)
#define PMIC_WLEDCTRL1_FDIM_500HZ			(2 << 0)
#define PMIC_WLEDCTRL1_FDIM_200HZ			(1 << 0)
#define PMIC_WLEDCTRL1_FDIM_100HZ			(0 << 0)

// WLEDCTRL2
#define PMIC_WLEDCTRL2_DUTY_MASK			(0x7F << 0)

// MUXCTRL
#define PMIC_MUXCTRL_MUX_MASK				(7 << 0)
#define PMIC_MUXCTRL_MUX_MUX_IN				(5 << 0)
#define PMIC_MUXCTRL_MUX_VICHARGE			(4 << 0)
#define PMIC_MUXCTRL_MUX_VTS				(3 << 0)
#define PMIC_MUXCTRL_MUX_VSYS				(2 << 0)
#define PMIC_MUXCTRL_MUX_VBAT				(1 << 0)
#define PMIC_MUXCTRL_MUX_HIZ				(0 << 0)

// STATUS
#define PMIC_STATUS_OFF						(1 << 7)
#define PMIC_STATUS_ACPWR					(1 << 3)
#define PMIC_STATUS_USBPWR					(1 << 2)
#define PMIC_STATUS_PB						(1 << 0)

// PGOOD
#define PMIC_PGOOD_LDO3_PG					(1 << 6)
#define PMIC_PGOOD_LDO4_PG					(1 << 5)
#define PMIC_PGOOD_DC1_PG					(1 << 4)
#define PMIC_PGOOD_DC2_PG					(1 << 3)
#define PMIC_PGOOD_DC3_PG					(1 << 2)
#define PMIC_PGOOD_LDO1_PG					(1 << 1)
#define PMIC_PGOOD_LDO2_PG					(1 << 0)

// DEFPG
#define PMIC_DEFPG_LDO1PGM_PG				(1 << 3)
#define PMIC_DEFPG_LDO2PGM_PG				(1 << 2)
#define PMIC_DEFPG_PGDLY_MASK				(3 << 0)
#define PMIC_DEFPG_PGDLY_MASK				(3 << 0)
#define PMIC_DEFPG_PGDLY_400MS				(3 << 0)
#define PMIC_DEFPG_PGDLY_200MS				(2 << 0)
#define PMIC_DEFPG_PGDLY_100MS				(1 << 0)
#define PMIC_DEFPG_PGDLY_20MS				(0 << 0)

// DEFDCDC1,2,3
#define DCDC_VOLT_SEL_1125MV				0x09
#define DCDC_VOLT_SEL_1275MV				0x0F
#define DCDC_VOLT_SEL_1325MV				0x11

// DEFSLEW
#define PMIC_DEFSLEW_GO						(1 << 7)
#define PMIC_DEFSLEW_GODSBL					(1 << 6)
#define PMIC_DEFSLEW_PFM_EN1				(1 << 5)
#define PMIC_DEFSLEW_PFM_EN2				(1 << 4)
#define PMIC_DEFSLEW_PFM_EN3				(1 << 3)
#define PMIC_DEFSLEW_SLEW_MASK				(7 << 0)
#define PMIC_DEFSLEW_SLEW_IMMED				(7 << 0)
#define PMIC_DEFSLEW_SLEW_3_5US				(6 << 0)
#define PMIC_DEFSLEW_SLEW_7US				(5 << 0)
#define PMIC_DEFSLEW_SLEW_14US				(4 << 0)
#define PMIC_DEFSLEW_SLEW_28US				(3 << 0)
#define PMIC_DEFSLEW_SLEW_56US				(2 << 0)
#define PMIC_DEFSLEW_SLEW_112US				(1 << 0)
#define PMIC_DEFSLEW_SLEW_224US				(0 << 0)

// DEFLDO1
#define PMIC_DEFLDO1_MASK					(0x0F << 0)

// DEFLDO2
#define PMIC_DEFLDO2_MASK					(0x3F << 0)
#define PMIC_DEFLDO2_TRACK					(1 << 6)

// DEFLS1/LDO3
#define PMIC_DEFLS1_MASK					(0x1F << 0)
#define PMIC_DEFLS1_LS1LDO3					(1 << 5)

// DEFLS2/LDO4
#define PMIC_DEFLS2_MASK					(0x1F << 0)
#define PMIC_DEFLS2_LS1LDO4					(1 << 5)

// ENABLE
#define PMIC_ENABLE_LS1_EN					(1 << 6)
#define PMIC_ENABLE_LS2_EN					(1 << 5)
#define PMIC_ENABLE_DC1_EN					(1 << 4)
#define PMIC_ENABLE_DC2_EN					(1 << 3)
#define PMIC_ENABLE_DC3_EN					(1 << 2)
#define PMIC_ENABLE_LDO1_EN					(1 << 1)
#define PMIC_ENABLE_LDO2_EN					(1 << 0)

// DEFUVLO
#define PMIC_DEFUVLO_MASK					(3 << 0)
#define PMIC_DEFUVLO_3_3					(3 << 0)
#define PMIC_DEFUVLO_3_18					(2 << 0)
#define PMIC_DEFUVLO_2_89					(1 << 0)
#define PMIC_DEFUVLO_2_73					(0 << 0)

// SEQ6
#define PMIC_SEQ6_SEQUP						(1 << 2)
#define PMIC_SEQ6_SEQDWN					(1 << 1)
#define PMIC_SEQ6_INSTDWN					(1 << 0)




#define LDO_VOLTAGE_OUT_1_8		0x06
#define LDO_VOLTAGE_OUT_3_3		0x1F

#define PWR_SRC_USB_BITMASK		0x4
#define PWR_SRC_AC_BITMASK		0x8


/* FUNCTIONS */
BOOL TWLSetOPVoltage(UINT voltage,UINT32 mv);
BOOL TWLUpdateVoltage(unsigned char dc_cntrl_reg, unsigned char volt_sel);
BOOL TWLProtWriteRegs(unsigned char prot_level, unsigned char dest_reg,
        unsigned char dest_val, unsigned char mask);
BOOL TWLGetStatusReg(unsigned char * buf);


#endif
