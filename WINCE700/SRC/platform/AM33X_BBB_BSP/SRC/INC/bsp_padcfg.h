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
//  File:  bsp_cfg.c
//
#include "bsp.h"
#include "soc_cfg.h"
//#include "oal_padcfg.h"
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
#define PULLUDEN	(0x0 << 3) /* Pull enabled */
#define PULLUDDIS	(0x1 << 3) /* Pull disabled */
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

#define NOR_PADS \
	PAD_ENTRY(LCD_DATA0,        MODE(1) | PULLUDEN) 	        /* NOR_A0 */    \
	PAD_ENTRY(LCD_DATA1,        MODE(1) | PULLUDEN)	            /* NOR_A1 */    \
	PAD_ENTRY(LCD_DATA2,        MODE(1) | PULLUDEN)	            /* NOR_A2 */    \
	PAD_ENTRY(LCD_DATA3,        MODE(1) | PULLUDEN)	            /* NOR_A3 */    \
	PAD_ENTRY(LCD_DATA4,        MODE(1) | PULLUDEN)	            /* NOR_A4 */    \
	PAD_ENTRY(LCD_DATA5,        MODE(1) | PULLUDEN)	            /* NOR_A5 */    \
	PAD_ENTRY(LCD_DATA6,        MODE(1) | PULLUDEN)	            /* NOR_A6 */    \
	PAD_ENTRY(LCD_DATA7,        MODE(1) | PULLUDEN)	            /* NOR_A7 */    \
	PAD_ENTRY(GPMC_A8,          MODE(0))			            /* NOR_A8 */    \
	PAD_ENTRY(GPMC_A9,          MODE(0))			            /* NOR_A9 */    \
	PAD_ENTRY(GPMC_A10,         MODE(0))			            /* NOR_A10 */   \
	PAD_ENTRY(GPMC_A11,         MODE(0))			            /* NOR_A11 */   \
	PAD_ENTRY(LCD_DATA8,        MODE(1) | PULLUDEN)	            /* NOR_A12 */   \
	PAD_ENTRY(LCD_DATA9,        MODE(1) | PULLUDEN)	            /* NOR_A13 */   \
	PAD_ENTRY(LCD_DATA10,       MODE(1) | PULLUDEN)	            /* NOR_A14 */   \
	PAD_ENTRY(LCD_DATA11,       MODE(1) | PULLUDEN)	            /* NOR_A15 */   \
	PAD_ENTRY(LCD_DATA12,       MODE(1) | PULLUDEN)	            /* NOR_A16 */   \
	PAD_ENTRY(LCD_DATA13,       MODE(1) | PULLUDEN)	            /* NOR_A17 */   \
	PAD_ENTRY(LCD_DATA14,       MODE(1) | PULLUDEN)	            /* NOR_A18 */   \
	PAD_ENTRY(LCD_DATA15,       MODE(1) | PULLUDEN)	            /* NOR_A19 */   \
	PAD_ENTRY(GPMC_A4,          MODE(4))			            /* NOR_A20 */   \
	PAD_ENTRY(GPMC_A5,          MODE(4))			            /* NOR_A21 */   \
	PAD_ENTRY(GPMC_A6,          MODE(4))			            /* NOR_A22 */   \
	PAD_ENTRY(GPMC_AD0,         MODE(0) | RXACTIVE)		        /* NOR_AD0 */   \
	PAD_ENTRY(GPMC_AD1,         MODE(0) | RXACTIVE)		        /* NOR_AD1 */   \
	PAD_ENTRY(GPMC_AD2,         MODE(0) | RXACTIVE)		        /* NOR_AD2 */   \
	PAD_ENTRY(GPMC_AD3,         MODE(0) | RXACTIVE)		        /* NOR_AD3 */   \
	PAD_ENTRY(GPMC_AD4,         MODE(0) | RXACTIVE)		        /* NOR_AD4 */   \
	PAD_ENTRY(GPMC_AD5,         MODE(0) | RXACTIVE)		        /* NOR_AD5 */   \
	PAD_ENTRY(GPMC_AD6,         MODE(0) | RXACTIVE)		        /* NOR_AD6 */   \
	PAD_ENTRY(GPMC_AD7,         MODE(0) | RXACTIVE)		        /* NOR_AD7 */   \
	PAD_ENTRY(GPMC_AD8,         MODE(0) | RXACTIVE)		        /* NOR_AD8 */   \
	PAD_ENTRY(GPMC_AD9,         MODE(0) | RXACTIVE)		        /* NOR_AD9 */   \
	PAD_ENTRY(GPMC_AD10,        MODE(0) | RXACTIVE)	            /* NOR_AD10 */  \
	PAD_ENTRY(GPMC_AD11,        MODE(0) | RXACTIVE)	            /* NOR_AD11 */  \
	PAD_ENTRY(GPMC_AD12,        MODE(0) | RXACTIVE)	            /* NOR_AD12 */  \
	PAD_ENTRY(GPMC_AD13,        MODE(0) | RXACTIVE)	            /* NOR_AD13 */  \
	PAD_ENTRY(GPMC_AD14,        MODE(0) | RXACTIVE)	            /* NOR_AD14 */  \
	PAD_ENTRY(GPMC_AD15,        MODE(0) | RXACTIVE)	            /* NOR_AD15 */  \
	PAD_ENTRY(GPMC_CSN0,        MODE(0) | PULLUP_EN)	        /* NOR_CE */    \
	PAD_ENTRY(GPMC_OEN_REN,     MODE(0) | PULLUP_EN)	        /* NOR_OE */    \
	PAD_ENTRY(GPMC_WEN,         MODE(0) | PULLUP_EN)	        /* NOR_WEN */   \
	PAD_ENTRY(GPMC_WAIT0,       MODE(0) | RXACTIVE | PULLUP_EN) /* NOR WAIT */  \
	PAD_ENTRY(LCD_AC_BIAS_EN,   MODE(7) | RXACTIVE | PULLUDEN)  /* NOR RESET */ \

#define RGMII1_PADS  \
	PAD_ENTRY(MII1_TXEN,    MODE(2))			            /* RGMII1_TCTL */   \
	PAD_ENTRY(MII1_RXDV,    MODE(2) | RXACTIVE)	            /* RGMII1_RCTL */   \
	PAD_ENTRY(MII1_TXD3,    MODE(2))			            /* RGMII1_TD3 */    \
	PAD_ENTRY(MII1_TXD2,    MODE(2))			            /* RGMII1_TD2 */    \
	PAD_ENTRY(MII1_TXD1,    MODE(2))			            /* RGMII1_TD1 */    \
	PAD_ENTRY(MII1_TXD0,    MODE(2))			            /* RGMII1_TD0 */    \
	PAD_ENTRY(MII1_TXCLK,   MODE(2))			            /* RGMII1_TCLK */   \
	PAD_ENTRY(MII1_RXCLK,   MODE(2) | RXACTIVE)	            /* RGMII1_RCLK */   \
	PAD_ENTRY(MII1_RXD3,    MODE(2) | RXACTIVE)	            /* RGMII1_RD3 */    \
	PAD_ENTRY(MII1_RXD2,    MODE(2) | RXACTIVE)	            /* RGMII1_RD2 */    \
	PAD_ENTRY(MII1_RXD1,    MODE(2) | RXACTIVE)	            /* RGMII1_RD1 */    \
	PAD_ENTRY(MII1_RXD0,    MODE(2) | RXACTIVE)	            /* RGMII1_RD0 */    \
	PAD_ENTRY(MDIO_DATA,    MODE(0) | RXACTIVE | PULLUP_EN) /* MDIO_DATA */     \
	PAD_ENTRY(MDIO_CLK,     MODE(0) | PULLUP_EN)	        /* MDIO_CLK */      \

#define RGMII2_PADS    \
	PAD_ENTRY(GPMC_A0,  MODE(2))			            /* RGMII2_TCTL */   \
	PAD_ENTRY(GPMC_A1,  MODE(2) | RXACTIVE)		        /* RGMII2_RCTL */   \
	PAD_ENTRY(GPMC_A2,  MODE(2))			            /* RGMII2_TD3 */    \
	PAD_ENTRY(GPMC_A3,  MODE(2))			            /* RGMII2_TD2 */    \
	PAD_ENTRY(GPMC_A4,  MODE(2))			            /* RGMII2_TD1 */    \
	PAD_ENTRY(GPMC_A5,  MODE(2))			            /* RGMII2_TD0 */    \
	PAD_ENTRY(GPMC_A6,  MODE(2))			            /* RGMII2_TCLK */   \
	PAD_ENTRY(GPMC_A7,  MODE(2) | RXACTIVE)		        /* RGMII2_RCLK */   \
	PAD_ENTRY(GPMC_A8,  MODE(2) | RXACTIVE)		        /* RGMII2_RD3 */    \
	PAD_ENTRY(GPMC_A9,  MODE(2) | RXACTIVE)		        /* RGMII2_RD2 */    \
	PAD_ENTRY(GPMC_A10, MODE(2) | RXACTIVE)		        /* RGMII2_RD1 */    \
	PAD_ENTRY(GPMC_A11, MODE(2) | RXACTIVE)		        /* RGMII2_RD0 */    \
	PAD_ENTRY(MDIO_DATA,MODE(0) | RXACTIVE | PULLUP_EN) /* MDIO_DATA */     \
	PAD_ENTRY(MDIO_CLK, MODE(0) | PULLUP_EN)	        /* MDIO_CLK */      \

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

#define I2C0_PADS    \
	PAD_ENTRY(I2C0_SDA,		(MODE(0) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_DATA */  \
	PAD_ENTRY(I2C0_SCL,		(MODE(0) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_SCLK */  \

#define I2C1_PADS    \
	PAD_ENTRY(SPI0_D1,		(MODE(2) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_DATA */  \
	PAD_ENTRY(SPI0_CS0,		(MODE(2) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL))	/* I2C_SCLK */  \

#define I2C2_PADS    \
	PAD_ENTRY(UART1_CTSN,	(MODE(3) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL)) /* I2C_DATA */  \
	PAD_ENTRY(UART1_RTSN,	(MODE(3) | AM335X_PIN_INPUT_PULLUP | SLEWCTRL)) /* I2C_SCLK */  \

#define NAND_PADS    \
	PAD_ENTRY(GPMC_AD0,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD0 */    \
	PAD_ENTRY(GPMC_AD1,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD1 */    \
	PAD_ENTRY(GPMC_AD2,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD2 */    \
	PAD_ENTRY(GPMC_AD3,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD3 */    \
	PAD_ENTRY(GPMC_AD4,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD4 */    \
	PAD_ENTRY(GPMC_AD5,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD5 */    \
	PAD_ENTRY(GPMC_AD6,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD6 */    \
	PAD_ENTRY(GPMC_AD7,         (MODE(0) | PULLUP_EN | RXACTIVE))   /* NAND AD7 */    \
	PAD_ENTRY(GPMC_WAIT0,       (MODE(0) | RXACTIVE | PULLUP_EN))   /* NAND WAIT */   \
	PAD_ENTRY(GPMC_WPN,         (MODE(7) | PULLUP_EN | RXACTIVE))	/* NAND_WPN */    \
	PAD_ENTRY(GPMC_CSN0,        (MODE(0) | PULLUDEN))	            /* NAND_CS0 */    \
	PAD_ENTRY(GPMC_ADVN_ALE,    (MODE(0) | PULLUDEN))               /* NAND_ADV_ALE */\
	PAD_ENTRY(GPMC_OEN_REN,     (MODE(0) | PULLUDEN))	            /* NAND_OE */     \
	PAD_ENTRY(GPMC_WEN,         (MODE(0) | PULLUDEN))	            /* NAND_WEN */    \
	PAD_ENTRY(GPMC_BE0N_CLE,    (MODE(0) | PULLUDEN))	            /* NAND_BE_CLE */ \

#define UART3_PADS   \
	PAD_ENTRY(SPI0_CS1,         (MODE(1) | PULLUDEN | RXACTIVE))	/* UART3_RXD */ \
	PAD_ENTRY(ECAP0_IN_PWM0_OUT,(MODE(1) | PULLUDEN))	            /* UART3_TXD */ \

#define UART0_PADS   \
	PAD_ENTRY(UART0_RXD,        (MODE(0) | PULLUP_EN | RXACTIVE))	/* UART0_RXD */ \
	PAD_ENTRY(UART0_TXD,        (MODE(0) | PULLUDEN))				/* UART0_TXD */ \


#define BKL_PADS    \
    PAD_ENTRY(ECAP0_IN_PWM0_OUT,(MODE(7) | PULLUDEN))   /* GPIO7 for LCD backlight */ \


#define FRAMER_PADS    \
    PAD_ENTRY(XDMA_EVENT_INTR0, (MODE(3))) /* CLKOUT 24Mhz */ \
    PAD_ENTRY(GPMC_AD9,          MODE(7) | AM335X_PIN_INPUT_PULLUP)  /* HDMI INT */ \
	PAD_ENTRY(GPMC_AD11,         MODE(7) | AM335X_PIN_OUTPUT_PULLUP) /* 24.576Mhz osc enable */ \


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

#define USB0_PADS   \
    PAD_ENTRY(USB0_DRVVBUS,    MODE(0) | AM335X_PIN_OUTPUT)         \

#define USB1_PADS   \
    PAD_ENTRY(USB1_DRVVBUS,    MODE(0) | AM335X_PIN_OUTPUT)         \

#define KEYPAD_PADS	\
	PAD_ENTRY(GPMC_A5,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_21 */  \
	PAD_ENTRY(GPMC_A6,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_22 */  \
	PAD_ENTRY(GPMC_A9,			MODE(7) | AM335X_PIN_INPUT_PULLDOWN)/* GPIO1_25 */	\
	PAD_ENTRY(GPMC_A10,			MODE(7) | AM335X_PIN_INPUT_PULLDOWN)/* GPIO1_26 */	\
	PAD_ENTRY(GPMC_A11,			MODE(7) | AM335X_PIN_INPUT_PULLDOWN)/* GPIO1_27 */	\
	PAD_ENTRY(SPI0_SCLK,		MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_2 */   \
	PAD_ENTRY(SPI0_D0,			MODE(7) | AM335X_PIN_INPUT_PULLUP)	/* GPIO0_3 */   \
	
#define NLED_PADS	\
	PAD_ENTRY(GPMC_A5,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_21 */  \
	PAD_ENTRY(GPMC_A6,			MODE(7) | AM335X_PIN_OUTPUT)	    /* GPIO1_22 */  \
	PAD_ENTRY(GPMC_A7,			MODE(7) | AM335X_PIN_OUTPUT)        /* GPIO1_23 */	\
	PAD_ENTRY(GPMC_A8,			MODE(7) | AM335X_PIN_OUTPUT)        /* GPIO1_24 */	\


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


