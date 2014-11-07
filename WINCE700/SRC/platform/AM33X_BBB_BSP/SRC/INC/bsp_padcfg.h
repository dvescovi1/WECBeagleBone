// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  bsp_padcfg.c
//
#include "bsp.h"
#include "soc_cfg.h"
#include "am33x_config.h"
#include "am33x_clocks.h"

extern DWORD g_dwBoardId;

extern DWORD g_dwBoardProfile;

extern DWORD g_dwBoardHasDcard;


#define HASDCARD_SLOT1		(0x1 << 0)
#define HASDCARD_SLOT2		(0x1 << 1)
#define HASDCARD_SLOT3		(0x1 << 2)
#define HASDCARD_SLOT4		(0x1 << 3)
#define HASDCARD_DVI		(0x1 << 16)
#define HASDCARD_LCD4		(0x1 << 17)
#define HASDCARD_LCD7		(0x1 << 18)
#define HASDCARD_LCD7_4D	(0x1 << 19)


//------------------------------------------------------------------------------
//  Pad structures
//------------------------------------------------------------------------------
#define DEV_ON_BASEBOARD       0x1
#define DEV_ON_DGHTR_BRD       0x2

typedef struct {
    PAD_INFO *  padInfo;
    OMAP_DEVICE device;
    UINT16        profile;
    UINT16        device_on;
} PIN_MUX;

#define PAD_ENTRY(x,y)              {PAD_ID(x),y,0},
#define HW_DEFAULT_PAD_CONFIG       0x7FFF // read from hw regs

//------------------------------------------------------------------------------
//  Pad configuration
//------------------------------------------------------------------------------
// Redefining these to keep in sync with linux code
#define SLEWCTRL	(0x1 << 6)
#define	RXACTIVE	(0x1 << 5)
#define	PULLUP_EN	(0x1 << 4) /* Pull UP Selection */
#define PULLUDEN	(0x0 << 3) /* Pull up enabled */
#define PULLUDDIS	(0x1 << 3) /* Pull up disabled */
#define MODE(val)	val

/* Definition of output pin could have pull disabled, but
 * this has not been done due to two reasons
 * 1. AM335X_MUX will take care of it
 * 2. If pull was disabled for out macro, combining out & in pull on macros
 *    would disable pull resistor and AM335X_MUX cannot take care of the
 *    correct pull setting and unintentionally pull would get disabled
 */
#define	AM335X_PIN_OUTPUT		    (0)
#define	AM335X_PIN_OUTPUT_PULLUP	(PULLUP_EN)
#define	AM335X_PIN_INPUT		    (RXACTIVE | PULLUDDIS)
#define	AM335X_PIN_INPUT_PULLUP		(RXACTIVE | PULLUP_EN)
#define	AM335X_PIN_INPUT_PULLDOWN	(RXACTIVE)


#define ALL_ALLOWED_PADS \
{       \
    PAD_ENTRY(GPMC_AD0           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD1           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD2           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD3           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD4           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD5           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD6           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD7           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD8           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD9           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD10          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD11          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD12          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD13          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD14          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_AD15          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A0            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A1            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A2            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A3            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A4            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A5            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A6            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A7            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A8            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A9            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A10           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_A11           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_WAIT0         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_WPN           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_BE1N          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_CSN0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_CSN1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_CSN2          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_CSN3          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_CLK           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_ADVN_ALE      ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_OEN_REN       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_WEN           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(GPMC_BE0N_CLE      ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA2          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA3          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA4          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA5          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA6          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA7          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA8          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA9          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA10         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA11         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA12         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA13         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA14         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_DATA15         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_VSYNC          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_HSYNC          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_PCLK           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(LCD_AC_BIAS_EN     ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_DAT3          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_DAT2          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_DAT1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_DAT0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_CLK           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MMC0_CMD           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_COL           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_CRS           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXERR         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXEN          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXDV          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXD3          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXD2          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXD1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXD0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_TXCLK         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXCLK         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXD3          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXD2          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXD1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MII1_RXD0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(RMII1_REFCLK       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MDIO_DATA          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MDIO_CLK           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(SPI0_SCLK          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(SPI0_D0            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(SPI0_D1            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(SPI0_CS0           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(SPI0_CS1           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(ECAP0_IN_PWM0_OUT  ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART0_CTSN         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART0_RTSN         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART0_RXD          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART0_TXD          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART1_CTSN         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART1_RTSN         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART1_RXD          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(UART1_TXD          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(I2C0_SDA           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(I2C0_SCL           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_ACLKX       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_FSX         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_AXR0        ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_AHCLKR      ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_ACLKR       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_FSR         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_AXR1        ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(MCASP0_AHCLKX      ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(XDMA_EVENT_INTR0   ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(XDMA_EVENT_INTR1   ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(NRESETIN_OUT       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(PORZ               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(NNMI               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC0_IN            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC0_OUT           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC0_VSS           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(TMS                ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(TDI                ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(TDO                ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(TCK                ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(NTRST              ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(EMU0               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(EMU1               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC1_IN            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC1_OUT           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(OSC1_VSS           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(RTC_PORZ           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(PMIC_POWER_EN      ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(EXT_WAKEUP         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(ENZ_KALDO_1P8V     ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_DM            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_DP            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_CE            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_ID            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_VBUS          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB0_DRVVBUS       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_DM            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_DP            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_CE            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_ID            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_VBUS          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(USB1_DRVVBUS       ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_RESETN         ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_CSN0           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_CKE            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_NCK            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_CASN           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_RASN           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_WEN            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_BA0            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_BA1            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_BA2            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A0             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A1             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A2             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A3             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A4             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A5             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A6             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A7             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A8             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A9             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A10            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A11            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A12            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A13            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A14            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_A15            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_ODT            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D0             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D1             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D2             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D3             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D4             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D5             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D6             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D7             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D8             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D9             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D10            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D11            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D12            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D13            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D14            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_D15            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQM0           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQM1           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQS0           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQSN0          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQS1           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_DQSN1          ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_VREF           ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_VTP            ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_STRBEN0        ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(DDR_STRBEN1        ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN7               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN6               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN5               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN4               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN3               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN2               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN1               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AIN0               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(VREFP              ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(VREFN              ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AVDD               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(AVSS               ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(IFORCE             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(VSENSE             ,HW_DEFAULT_PAD_CONFIG)        \
    PAD_ENTRY(TESTOUT            ,HW_DEFAULT_PAD_CONFIG)        \
    END_OF_PAD_ARRAY     \
}

#define I2C0_PADS    \
	PAD_ENTRY(I2C0_SDA,		(MODE(0) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_DATA */  \
	PAD_ENTRY(I2C0_SCL,		(MODE(0) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_SCLK */  \

#define I2C1_PADS    \
	PAD_ENTRY(SPI0_D1,		(MODE(2) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_DATA */  \
	PAD_ENTRY(SPI0_CS0,		(MODE(2) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_SCLK */  \

#define I2C2_PADS    \
	PAD_ENTRY(UART1_CTSN,	(MODE(3) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL)) /* I2C_DATA */  \
	PAD_ENTRY(UART1_RTSN,	(MODE(3) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL)) /* I2C_SCLK */  \

#define SPI0_PADS \
    PAD_ENTRY(SPI0_SCLK,MODE(0) | PULLUDEN | RXACTIVE)              /*SPI0_SCLK */  \
	PAD_ENTRY(SPI0_D0,  MODE(0) | PULLUDEN | PULLUP_EN | RXACTIVE)  /*SPI0_D0 */    \
	PAD_ENTRY(SPI0_D1,  MODE(0) | PULLUDEN | RXACTIVE)              /*SPI0_D1 */    \
	PAD_ENTRY(SPI0_CS0, MODE(0) | PULLUDEN | PULLUP_EN | RXACTIVE)	/*SPI0_CS0 */   \

#define SPI1_PADS    \
	PAD_ENTRY(MCASP0_ACLKX, MODE(3) )               /*SPI1_SCLK */  \
	PAD_ENTRY(MCASP0_FSX,   MODE(3) )               /*SPI1_D0 */    \
	PAD_ENTRY(MCASP0_AXR0,  MODE(3) | RXACTIVE )    /*SPI1_D1 */    \
	PAD_ENTRY(MCASP0_AHCLKR,MODE(3) )               /*SPI1_CS0 */   \

#define UART0_PADS   \
	PAD_ENTRY(UART0_RXD,        (MODE(0) | PULLUP_EN | RXACTIVE))	/* UART0_RXD */ \
	PAD_ENTRY(UART0_TXD,        (MODE(0) | PULLUDEN))				/* UART0_TXD */ \

#define UART1_PADS   \
	PAD_ENTRY(UART1_RXD,        (MODE(0) | PULLUP_EN | RXACTIVE))	/* UART1_RXD */ \
	PAD_ENTRY(UART1_TXD,        (MODE(0) | PULLUDEN))				/* UART1_TXD */ \

#define UART2_PADS   \
	PAD_ENTRY(SPI0_SCLK,        (MODE(1) | PULLUP_EN | RXACTIVE))	/* UART2_RXD */ \
	PAD_ENTRY(SPI0_D0,          (MODE(1) | PULLUDEN))				/* UART2_TXD */ \

#define UART4_PADS   \
	PAD_ENTRY(GPMC_WAIT0,       (MODE(6) | PULLUP_EN | RXACTIVE))	/* UART4_RXD */ \
	PAD_ENTRY(GPMC_WPN,         (MODE(6) | PULLUDEN))				/* UART4_TXD */ \

#define USB0_PADS   \
    PAD_ENTRY(USB0_DRVVBUS,    MODE(0) | AM335X_PIN_OUTPUT)         \

#define USB1_PADS   \
    PAD_ENTRY(USB1_DRVVBUS,    MODE(0) | AM335X_PIN_OUTPUT)         \

#define MMC0_PADS    \
    PAD_ENTRY(MMC0_DAT3,      MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_DAT3 */    \
    PAD_ENTRY(MMC0_DAT2,      MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_DAT2 */    \
    PAD_ENTRY(MMC0_DAT1,      MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_DAT1 */    \
    PAD_ENTRY(MMC0_DAT0,      MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_DAT0 */    \
    PAD_ENTRY(MMC0_CLK,       MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_CLK */     \
    PAD_ENTRY(MMC0_CMD,       MODE(0) | AM335X_PIN_INPUT_PULLUP)     /* MMC0_CMD */     \
	PAD_ENTRY(SPI0_CS1,       MODE(5) | AM335X_PIN_INPUT_PULLUP)	 /* MMC0_CD */      \

#define MMC1_PADS    \
	PAD_ENTRY(GPMC_AD7,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT7 */    \
	PAD_ENTRY(GPMC_AD6,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT6 */    \
	PAD_ENTRY(GPMC_AD5,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT5 */    \
	PAD_ENTRY(GPMC_AD4,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT4 */    \
	PAD_ENTRY(GPMC_AD3,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT3 */    \
	PAD_ENTRY(GPMC_AD2,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT2 */    \
	PAD_ENTRY(GPMC_AD1,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT1 */    \
	PAD_ENTRY(GPMC_AD0,       MODE(1) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_DAT0 */    \
	PAD_ENTRY(GPMC_CSN1,      MODE(2) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_CLK */     \
	PAD_ENTRY(GPMC_CSN2,      MODE(2) | AM335X_PIN_INPUT_PULLUP)     /* MMC1_CMD */     \
	PAD_ENTRY(GPMC_A4,        MODE(7) | AM335X_PIN_INPUT_PULLDOWN)	 /* GPIO1_20 eMMC RST */   \

#define MMC2_PADS   \
    PAD_ENTRY(GPMC_AD15,      MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_DAT3 */    \
    PAD_ENTRY(GPMC_AD14,      MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_DAT2 */    \
    PAD_ENTRY(GPMC_AD13,      MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_DAT1 */    \
    PAD_ENTRY(GPMC_AD12,      MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_DAT0 */    \
    PAD_ENTRY(GPMC_CLK,       MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_CLK */     \
    PAD_ENTRY(GPMC_CSN3,      MODE(3) | AM335X_PIN_INPUT_PULLUP)     /* MMC2_CMD Note:only BBB */  \
    PAD_ENTRY(GPMC_A0,        MODE(7) | AM335X_PIN_INPUT_PULLUP)	 /* tied to MMC2_CMD */    \
	PAD_ENTRY(GPMC_WPN,       MODE(4) | AM335X_PIN_INPUT_PULLUP)	 /* MMC2_CD */      \

#define LCDC_PADS    \
	PAD_ENTRY(LCD_DATA0,        (MODE(0) | PULLUDEN))	/* LCD_DATA0 */  \
	PAD_ENTRY(LCD_DATA1,        (MODE(0) | PULLUDEN))	/* LCD_DATA1 */  \
	PAD_ENTRY(LCD_DATA2,        (MODE(0) | PULLUDEN))	/* LCD_DATA2 */  \
	PAD_ENTRY(LCD_DATA3,        (MODE(0) | PULLUDEN))	/* LCD_DATA3 */  \
	PAD_ENTRY(LCD_DATA4,        (MODE(0) | PULLUDEN))	/* LCD_DATA4 */  \
	PAD_ENTRY(LCD_DATA5,        (MODE(0) | PULLUDEN))	/* LCD_DATA5 */  \
	PAD_ENTRY(LCD_DATA6,        (MODE(0) | PULLUDEN))	/* LCD_DATA6 */  \
	PAD_ENTRY(LCD_DATA7,        (MODE(0) | PULLUDEN))	/* LCD_DATA7 */  \
	PAD_ENTRY(LCD_DATA8,        (MODE(0) | PULLUDEN))	/* LCD_DATA8 */  \
	PAD_ENTRY(LCD_DATA9,        (MODE(0) | PULLUDEN))	/* LCD_DATA9 */  \
	PAD_ENTRY(LCD_DATA10,       (MODE(0) | PULLUDEN))	/* LCD_DATA10 */ \
	PAD_ENTRY(LCD_DATA11,       (MODE(0) | PULLUDEN))	/* LCD_DATA11 */ \
	PAD_ENTRY(LCD_DATA12,       (MODE(0) | PULLUDEN))	/* LCD_DATA12 */ \
	PAD_ENTRY(LCD_DATA13,       (MODE(0) | PULLUDEN))	/* LCD_DATA13 */ \
	PAD_ENTRY(LCD_DATA14,       (MODE(0) | PULLUDEN))	/* LCD_DATA14 */ \
	PAD_ENTRY(LCD_DATA15,       (MODE(0) | PULLUDEN))	/* LCD_DATA15 */ \
	PAD_ENTRY(LCD_VSYNC,        (MODE(0)))              /* LCD_VSYNC */  \
	PAD_ENTRY(LCD_HSYNC,        (MODE(0)))              /* LCD_HSYNC */  \
	PAD_ENTRY(LCD_PCLK,         (MODE(0)))              /* LCD_PCLK */   \
	PAD_ENTRY(LCD_AC_BIAS_EN,   (MODE(0)))              /* LCD_AS_BIAS_EN */ \

#define FRAMER_PADS    \
    PAD_ENTRY(XDMA_EVENT_INTR0, (MODE(3))) /* CLKOUT 24Mhz */ \
    PAD_ENTRY(GPMC_AD9,          MODE(7) | AM335X_PIN_INPUT_PULLUP)  /* HDMI INT */ \
	PAD_ENTRY(GPMC_AD11,         MODE(7) | AM335X_PIN_OUTPUT_PULLUP) /* 24.576Mhz osc enable */ \

#define MII1_PADS    \
	PAD_ENTRY(MII1_RXERR,   MODE(0) | RXACTIVE)	            /* MII1_RXERR */ \
	PAD_ENTRY(MII1_TXEN,    MODE(0))			            /* MII1_TXEN */  \
	PAD_ENTRY(MII1_RXDV,    MODE(0) | RXACTIVE)	            /* MII1_RXDV */  \
	PAD_ENTRY(MII1_TXD3,    MODE(0))			            /* MII1_TXD3 */  \
	PAD_ENTRY(MII1_TXD2,    MODE(0))			            /* MII1_TXD2 */  \
	PAD_ENTRY(MII1_TXD1,    MODE(0))			            /* MII1_TXD1 */  \
	PAD_ENTRY(MII1_TXD0,    MODE(0))			            /* MII1_TXD0 */  \
	PAD_ENTRY(MII1_TXCLK,   MODE(0) | RXACTIVE)	            /* MII1_TXCLK */ \
	PAD_ENTRY(MII1_RXCLK,   MODE(0) | RXACTIVE)	            /* MII1_RXCLK */ \
	PAD_ENTRY(MII1_RXD3,    MODE(0) | RXACTIVE)	            /* MII1_RXD3 */  \
	PAD_ENTRY(MII1_RXD2,    MODE(0) | RXACTIVE)	            /* MII1_RXD2 */  \
	PAD_ENTRY(MII1_RXD1,    MODE(0) | RXACTIVE)	            /* MII1_RXD1 */  \
	PAD_ENTRY(MII1_RXD0,    MODE(0) | RXACTIVE)	            /* MII1_RXD0 */  \
	PAD_ENTRY(MDIO_DATA,    MODE(0) | RXACTIVE | PULLUP_EN) /* MDIO_DATA */  \
	PAD_ENTRY(MDIO_CLK,     MODE(0) | PULLUP_EN)	        /* MDIO_CLK */   \

#define MCASP1_PADS \
    PAD_ENTRY(MII1_CRS,         MODE(4) | AM335X_PIN_INPUT_PULLDOWN)            \
    PAD_ENTRY(MII1_RXERR,       MODE(4) | AM335X_PIN_INPUT_PULLDOWN)            \
    PAD_ENTRY(MII1_COL,         MODE(4) | AM335X_PIN_OUTPUT)                    \
    PAD_ENTRY(RMII1_REFCLK,     MODE(4) | AM335X_PIN_INPUT_PULLDOWN)            \

#define ADCTSC_PADS \
    PAD_ENTRY(AIN0,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN1,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN2,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN3,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(VREFP,            MODE(0) | RXACTIVE)             \
    PAD_ENTRY(VREFN,            MODE(0) | RXACTIVE)             \

#define ADC_PADS \
    PAD_ENTRY(AIN4,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN5,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN6,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(AIN7,             MODE(0) | RXACTIVE)             \
    PAD_ENTRY(VREFP,            MODE(0) | RXACTIVE)             \
    PAD_ENTRY(VREFN,            MODE(0) | RXACTIVE)             \

#define GPIO0_PADS	\
	PAD_ENTRY(ECAP0_IN_PWM0_OUT,MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_7  */   \
	PAD_ENTRY(GPMC_AD8,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_22 PWM2A */   \
	PAD_ENTRY(GPMC_AD9,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_23 PWM2B */   \
	PAD_ENTRY(GPMC_AD10,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_26 */   \
	PAD_ENTRY(GPMC_AD11,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_27 */   \
	PAD_ENTRY(UART1_RXD,        MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_14 UART1_RXD */ \
	PAD_ENTRY(UART1_TXD,        MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_15 UART1_TXD */ \
	PAD_ENTRY(SPI0_SCLK,        MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_2  UART2_RXD */  \
	PAD_ENTRY(SPI0_D0,          MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_3  UART2_TXD */  \
	PAD_ENTRY(GPMC_WAIT0,       MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_30 UART4_RXD */ \
	PAD_ENTRY(GPMC_WPN,         MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_31 UART4_TXD */ \

#define GPIO1_PADS	\
	PAD_ENTRY(GPMC_AD12,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_12 */   \
	PAD_ENTRY(GPMC_AD13,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_13 */   \
	PAD_ENTRY(GPMC_AD14,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_14 */   \
	PAD_ENTRY(GPMC_AD15,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_15 */   \
	PAD_ENTRY(GPMC_A0,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_16 */   \
	PAD_ENTRY(GPMC_A1,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_17 */   \
	PAD_ENTRY(GPMC_A2,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_18 PWM1A */   \
	PAD_ENTRY(GPMC_A3,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_19 PWM1B */   \
	PAD_ENTRY(GPMC_BE1N,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_28 */   \
	PAD_ENTRY(GPMC_CSN0,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_29 */   \

#define GPIO2_PADS	\
	PAD_ENTRY(GPMC_CLK,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO2_1  */   \
    PAD_ENTRY(GPMC_ADVN_ALE,	MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO2_2  */   \
    PAD_ENTRY(GPMC_OEN_REN,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO2_3  */   \
    PAD_ENTRY(GPMC_WEN,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO2_4  */   \
    PAD_ENTRY(GPMC_BE0N_CLE,	MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO2_5  */   \

#define GPIO3_PADS	\
	PAD_ENTRY(MCASP0_ACLKR,		MODE(7))							/* GPIO3_18 */   \
	PAD_ENTRY(MCASP0_FSR,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO3_19 */   \
	PAD_ENTRY(MCASP0_AHCLKX,	MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO3_21 */   \

#define NLED_PADS	\
	PAD_ENTRY(GPMC_A5,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_21 */  \
	PAD_ENTRY(GPMC_A6,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_22 */  \
	PAD_ENTRY(GPMC_A7,			MODE(7) | AM335X_PIN_OUTPUT)        /* GPIO1_23 */	\
	PAD_ENTRY(GPMC_A8,			MODE(7) | AM335X_PIN_OUTPUT)        /* GPIO1_24 */	\

#define KEYPAD_PADS \
	PAD_ENTRY(GPMC_A0,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_16 */   \
	PAD_ENTRY(GPMC_A1,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_17 */   \
	PAD_ENTRY(GPMC_A3,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO1_19 */   \
	PAD_ENTRY(MCASP0_AXR0,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO3_16 */   \
	PAD_ENTRY(UART1_TXD,        MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_15 UART1_TXD */ \
	PAD_ENTRY(SPI0_D0,          MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_3  UART2_TXD */ \



// Note : this function should be called by bootloaders only !
static _inline void ConfigurePadArray(const PAD_INFO* padArray)
{
    int i=0;
    if (padArray == NULL) return;
    while (padArray[i].padID != (UINT16) -1)
    {
        if (padArray[i].Cfg != HW_DEFAULT_PAD_CONFIG) 
            SOCSetPadConfig(padArray[i].padID,(UINT16) padArray[i].Cfg);
        i++;
    }
}


