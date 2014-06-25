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
#define PMIC_VDD1_SR_REG    (0x23)
#define PMIC_VDD2_REG       (0x24)
#define PMIC_VDD2_OP_REG    (0x25)
#define PMIC_DEVCTRL_REG    (0x3f)
#define PMIC_DEVCTRL2_REG   (0x40)

/* VDD2  & VDD1 control register (VDD2_REG & VDD1_REG) */
#define PMIC_VGAIN_SEL_MASK                     (0x3 << 6)
#define PMIC_ILMAX_MASK                         (0x1 << 5)
#define PMIC_TSTEP_MASK                         (0x7 << 2)
#define PMIC_ST_MASK                            (0x3 << 0)

#define PMIC_REG_VGAIN_SEL_X1                   (0x0 << 6)
#define PMIC_REG_VGAIN_SEL_X1_0                 (0x1 << 6)
#define PMIC_REG_VGAIN_SEL_X3                   (0x2 << 6)
#define PMIC_REG_VGAIN_SEL_X4                   (0x3 << 6)

#define PMIC_REG_ILMAX_1_0_A                    (0x0 << 5)
#define PMIC_REG_ILMAX_1_5_A                    (0x1 << 5)

#define PMIC_REG_TSTEP_                         (0x0 << 2)
#define PMIC_REG_TSTEP_12_5                     (0x1 << 2)
#define PMIC_REG_TSTEP_9_4                      (0x2 << 2)
#define PMIC_REG_TSTEP_7_5                      (0x3 << 2)
#define PMIC_REG_TSTEP_6_25                     (0x4 << 2)
#define PMIC_REG_TSTEP_4_7                      (0x5 << 2)
#define PMIC_REG_TSTEP_3_12                     (0x6 << 2)
#define PMIC_REG_TSTEP_2_5                      (0x7 << 2)

#define PMIC_REG_ST_OFF                         (0x0 << 0)
#define PMIC_REG_ST_ON_HI_POW                   (0x1 << 0)
#define PMIC_REG_ST_OFF_1                       (0x2 << 0)
#define PMIC_REG_ST_ON_LOW_POW                  (0x3 << 0)


/* VDD2 & VDD1 voltage selection register. (VDD2_OP_REG & VDD1_OP_REG) */
#define PMIC_OP_REG_SEL			                (0x7F << 0)

#define PMIC_OP_REG_CMD_MASK                    (0x1 << 7)
#define PMIC_OP_REG_CMD_OP                      (0x0 << 7)
#define PMIC_OP_REG_CMD_SR                      (0x1 << 7)

#define PMIC_OP_REG_SEL_MASK                    (0x7F << 0)
#define PMIC_OP_REG_SEL_1_2                     (0x33 << 0)

/* Device control register . (DEVCTRL_REG) */
#define PMIC_DEVCTRL_REG_SR_CTL_I2C_MASK        (0x1 << 4)
#define PMIC_DEVCTRL_REG_SR_CTL_I2C_SEL_SR_I2C  (0x0 << 4)
#define PMIC_DEVCTRL_REG_SR_CTL_I2C_SEL_CTL_I2C (0x1 << 4)


/* Registers */
#define CHIPID				0x00
#define POWER_PATH			0x01
#define INTERRUPT			0x02
#define CHGCONFIG0			0x03
#define CHGCONFIG1			0x04
#define CHGCONFIG2			0x05
#define CHGCONFIG3			0x06
#define WLEDCTRL1			0x07
#define WLEDCTRL2			0x08
#define MUXCTRL				0x09
#define STATUS				0x0A
#define PASSWORD			0x0B
#define PGOOD				0x0C
#define DEFPG				0x0D
#define DEFDCDC1			0x0E
#define DEFDCDC2			0x0F
#define DEFDCDC3			0x10
#define DEFSLEW				0x11
#define DEFLDO1				0x12
#define DEFLDO2				0x13
#define DEFLS1				0x14
#define DEFLS2				0x15
#define ENABLE				0x16
#define DEFUVLO				0x18
#define SEQ1				0x19
#define SEQ2				0x1A
#define SEQ3				0x1B
#define SEQ4				0x1C
#define SEQ5				0x1D
#define SEQ6				0x1E

#define PROT_LEVEL_NONE			0x00
#define PROT_LEVEL_1			0x01
#define PROT_LEVEL_2			0x02

#define PASSWORD_LOCK_FOR_WRITE		0x00
#define PASSWORD_UNLOCK			0x7D

#define DCDC_GO				0x80

#define MASK_ALL_BITS			0xFF

#define USB_INPUT_CUR_LIMIT_MASK	0x03
#define USB_INPUT_CUR_LIMIT_100MA	0x00
#define USB_INPUT_CUR_LIMIT_500MA	0x01
#define USB_INPUT_CUR_LIMIT_1300MA	0x02
#define USB_INPUT_CUR_LIMIT_1800MA	0x03

#define DCDC_VOLT_SEL_1125MV		0x09
#define DCDC_VOLT_SEL_1275MV		0x0F
#define DCDC_VOLT_SEL_1325MV		0x11

#define LDO_MASK				0x1F
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
