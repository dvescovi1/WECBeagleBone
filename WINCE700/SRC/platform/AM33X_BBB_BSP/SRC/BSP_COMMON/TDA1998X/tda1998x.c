// All rights reserved David Vescovi 2014
//
//  File:  tda1998x.c
//
#include "bsp.h"
#include "sdk_i2c.h"
#include "lcdc.h"
#include "tda1998x.h"

/* The TDA19988 series of devices use a paged register scheme.. to simplify
 * things we encode the page # in upper bits of the register #.  To read/
 * write a given register, we need to make sure CURPAGE register is set
 * appropriately.  Which implies reads/writes are not atomic.  Fun!
 */

#define REG(page, addr) (((page) << 8) | (addr))
#define REG2ADDR(reg)   ((reg) & 0xff)
#define REG2PAGE(reg)   (((reg) >> 8) & 0xff)

#define REG_CURPAGE               0xff                /* write */


/* Page 00h: General Control */
#define REG_VERSION_LSB           REG(0x00, 0x00)     /* read */
#define REG_MAIN_CNTRL0           REG(0x00, 0x01)     /* read/write */
# define MAIN_CNTRL0_SR           (1 << 0)
# define MAIN_CNTRL0_DECS         (1 << 1)
# define MAIN_CNTRL0_DEHS         (1 << 2)
# define MAIN_CNTRL0_CECS         (1 << 3)
# define MAIN_CNTRL0_CEHS         (1 << 4)
# define MAIN_CNTRL0_SCALER       (1 << 7)
#define REG_VERSION_MSB           REG(0x00, 0x02)     /* read */
#define REG_SOFTRESET             REG(0x00, 0x0a)     /* write */
# define SOFTRESET_AUDIO          (1 << 0)
# define SOFTRESET_I2C_MASTER     (1 << 1)
#define REG_DDC_DISABLE           REG(0x00, 0x0b)     /* read/write */
#define REG_CCLK_ON               REG(0x00, 0x0c)     /* read/write */
#define REG_I2C_MASTER            REG(0x00, 0x0d)     /* read/write */
# define I2C_MASTER_DIS_MM        (1 << 0)
# define I2C_MASTER_DIS_FILT      (1 << 1)
# define I2C_MASTER_APP_STRT_LAT  (1 << 2)
#define REG_INT_FLAGS_0           REG(0x00, 0x0f)     /* read/write */
#define REG_INT_FLAGS_1           REG(0x00, 0x10)     /* read/write */
#define REG_INT_FLAGS_2           REG(0x00, 0x11)     /* read/write */
# define INT_FLAGS_2_EDID_BLK_RD  (1 << 1)
#define REG_ENA_VP_0              REG(0x00, 0x18)     /* read/write */
#define REG_ENA_VP_1              REG(0x00, 0x19)     /* read/write */
#define REG_ENA_VP_2              REG(0x00, 0x1a)     /* read/write */
#define REG_ENA_AP                REG(0x00, 0x1e)     /* read/write */
#define REG_VIP_CNTRL_0           REG(0x00, 0x20)     /* write */
# define VIP_CNTRL_0_MIRR_A       (1 << 7)
# define VIP_CNTRL_0_SWAP_A(x)    (((x) & 7) << 4)
# define VIP_CNTRL_0_MIRR_B       (1 << 3)
# define VIP_CNTRL_0_SWAP_B(x)    (((x) & 7) << 0)
#define REG_VIP_CNTRL_1           REG(0x00, 0x21)     /* write */
# define VIP_CNTRL_1_MIRR_C       (1 << 7)
# define VIP_CNTRL_1_SWAP_C(x)    (((x) & 7) << 4)
# define VIP_CNTRL_1_MIRR_D       (1 << 3)
# define VIP_CNTRL_1_SWAP_D(x)    (((x) & 7) << 0)
#define REG_VIP_CNTRL_2           REG(0x00, 0x22)     /* write */
# define VIP_CNTRL_2_MIRR_E       (1 << 7)
# define VIP_CNTRL_2_SWAP_E(x)    (((x) & 7) << 4)
# define VIP_CNTRL_2_MIRR_F       (1 << 3)
# define VIP_CNTRL_2_SWAP_F(x)    (((x) & 7) << 0)
#define REG_VIP_CNTRL_3           REG(0x00, 0x23)     /* write */
# define VIP_CNTRL_3_X_TGL        (1 << 0)
# define VIP_CNTRL_3_H_TGL        (1 << 1)
# define VIP_CNTRL_3_V_TGL        (1 << 2)
# define VIP_CNTRL_3_EMB          (1 << 3)
# define VIP_CNTRL_3_SYNC_DE      (1 << 4)
# define VIP_CNTRL_3_SYNC_HS      (1 << 5)
# define VIP_CNTRL_3_DE_INT       (1 << 6)
# define VIP_CNTRL_3_EDGE         (1 << 7)
#define REG_VIP_CNTRL_4           REG(0x00, 0x24)     /* write */
# define VIP_CNTRL_4_BLC(x)       (((x) & 3) << 0)
# define VIP_CNTRL_4_BLANKIT(x)   (((x) & 3) << 2)
# define VIP_CNTRL_4_CCIR656      (1 << 4)
# define VIP_CNTRL_4_656_ALT      (1 << 5)
# define VIP_CNTRL_4_TST_656      (1 << 6)
# define VIP_CNTRL_4_TST_PAT      (1 << 7)
#define REG_VIP_CNTRL_5           REG(0x00, 0x25)     /* write */
# define VIP_CNTRL_5_CKCASE       (1 << 0)
# define VIP_CNTRL_5_SP_CNT(x)    (((x) & 3) << 1)

#define REG_MUX_AP                REG(0x00, 0x26)
# define MUX_AP_SELECT_I2S        (0x64)
#define REG_MAT_CONTRL            REG(0x00, 0x80)     /* write */
# define MAT_CONTRL_MAT_SC(x)     (((x) & 3) << 0)
# define MAT_CONTRL_MAT_BP        (1 << 2)
#define REG_VIDFORMAT             REG(0x00, 0xa0)     /* write */
#define REG_REFPIX_MSB            REG(0x00, 0xa1)     /* write */
#define REG_REFPIX_LSB            REG(0x00, 0xa2)     /* write */
#define REG_REFLINE_MSB           REG(0x00, 0xa3)     /* write */
#define REG_REFLINE_LSB           REG(0x00, 0xa4)     /* write */
#define REG_NPIX_MSB              REG(0x00, 0xa5)     /* write */
#define REG_NPIX_LSB              REG(0x00, 0xa6)     /* write */
#define REG_NLINE_MSB             REG(0x00, 0xa7)     /* write */
#define REG_NLINE_LSB             REG(0x00, 0xa8)     /* write */
#define REG_VS_LINE_STRT_1_MSB    REG(0x00, 0xa9)     /* write */
#define REG_VS_LINE_STRT_1_LSB    REG(0x00, 0xaa)     /* write */
#define REG_VS_PIX_STRT_1_MSB     REG(0x00, 0xab)     /* write */
#define REG_VS_PIX_STRT_1_LSB     REG(0x00, 0xac)     /* write */
#define REG_VS_LINE_END_1_MSB     REG(0x00, 0xad)     /* write */
#define REG_VS_LINE_END_1_LSB     REG(0x00, 0xae)     /* write */
#define REG_VS_PIX_END_1_MSB      REG(0x00, 0xaf)     /* write */
#define REG_VS_PIX_END_1_LSB      REG(0x00, 0xb0)     /* write */
#define REG_VS_LINE_STRT_2_MSB    REG(0x00, 0xb1)     /* write */
#define REG_VS_LINE_STRT_2_LSB    REG(0x00, 0xb2)     /* write */
#define REG_VS_PIX_STRT_2_MSB     REG(0x00, 0xb3)     /* write */
#define REG_VS_PIX_STRT_2_LSB     REG(0x00, 0xb4)     /* write */
#define REG_VS_LINE_END_2_MSB     REG(0x00, 0xb5)     /* write */
#define REG_VS_LINE_END_2_LSB     REG(0x00, 0xb6)     /* write */
#define REG_VS_PIX_END_2_MSB      REG(0x00, 0xb7)     /* write */
#define REG_VS_PIX_END_2_LSB      REG(0x00, 0xb8)     /* write */
#define REG_HS_PIX_START_MSB      REG(0x00, 0xb9)     /* write */
#define REG_HS_PIX_START_LSB      REG(0x00, 0xba)     /* write */
#define REG_HS_PIX_STOP_MSB       REG(0x00, 0xbb)     /* write */
#define REG_HS_PIX_STOP_LSB       REG(0x00, 0xbc)     /* write */
#define REG_VWIN_START_1_MSB      REG(0x00, 0xbd)     /* write */
#define REG_VWIN_START_1_LSB      REG(0x00, 0xbe)     /* write */
#define REG_VWIN_END_1_MSB        REG(0x00, 0xbf)     /* write */
#define REG_VWIN_END_1_LSB        REG(0x00, 0xc0)     /* write */
#define REG_VWIN_START_2_MSB      REG(0x00, 0xc1)     /* write */
#define REG_VWIN_START_2_LSB      REG(0x00, 0xc2)     /* write */
#define REG_VWIN_END_2_MSB        REG(0x00, 0xc3)     /* write */
#define REG_VWIN_END_2_LSB        REG(0x00, 0xc4)     /* write */
#define REG_DE_START_MSB          REG(0x00, 0xc5)     /* write */
#define REG_DE_START_LSB          REG(0x00, 0xc6)     /* write */
#define REG_DE_STOP_MSB           REG(0x00, 0xc7)     /* write */
#define REG_DE_STOP_LSB           REG(0x00, 0xc8)     /* write */
#define REG_TBG_CNTRL_0           REG(0x00, 0xca)     /* write */
# define TBG_CNTRL_0_TOP_TGL      (1 << 0)
# define TBG_CNTRL_0_TOP_SEL      (1 << 1)
# define TBG_CNTRL_0_DE_EXT       (1 << 2)
# define TBG_CNTRL_0_TOP_EXT      (1 << 3)
# define TBG_CNTRL_0_FRAME_DIS    (1 << 5)
# define TBG_CNTRL_0_SYNC_MTHD    (1 << 6)
# define TBG_CNTRL_0_SYNC_ONCE    (1 << 7)
#define REG_TBG_CNTRL_1           REG(0x00, 0xcb)     /* write */
# define TBG_CNTRL_1_H_TGL        (1 << 0)
# define TBG_CNTRL_1_V_TGL        (1 << 1)
# define TBG_CNTRL_1_TGL_EN       (1 << 2)
# define TBG_CNTRL_1_X_EXT        (1 << 3)
# define TBG_CNTRL_1_H_EXT        (1 << 4)
# define TBG_CNTRL_1_V_EXT        (1 << 5)
# define TBG_CNTRL_1_DWIN_DIS     (1 << 6)
#define REG_ENABLE_SPACE          REG(0x00, 0xd6)     /* write */
#define REG_HVF_CNTRL_0           REG(0x00, 0xe4)     /* write */
# define HVF_CNTRL_0_SM           (1 << 7)
# define HVF_CNTRL_0_RWB          (1 << 6)
# define HVF_CNTRL_0_PREFIL(x)    (((x) & 3) << 2)
# define HVF_CNTRL_0_INTPOL(x)    (((x) & 3) << 0)
#define REG_HVF_CNTRL_1           REG(0x00, 0xe5)     /* write */
# define HVF_CNTRL_1_FOR          (1 << 0)
# define HVF_CNTRL_1_YUVBLK       (1 << 1)
# define HVF_CNTRL_1_VQR(x)       (((x) & 3) << 2)
# define HVF_CNTRL_1_PAD(x)       (((x) & 3) << 4)
# define HVF_CNTRL_1_SEMI_PLANAR  (1 << 6)
#define REG_RPT_CNTRL             REG(0x00, 0xf0)     /* write */
#define REG_I2S_FORMAT            REG(0x00, 0xfc)

#define REG_AIP_CLKSEL            REG(0x00, 0xfd)
# define SEL_AIP_I2S              (1 << 3)  /* I2S Clk */

/* Page 02h: PLL settings */
#define REG_PLL_SERIAL_1          REG(0x02, 0x00)     /* read/write */
# define PLL_SERIAL_1_SRL_FDN     (1 << 0)
# define PLL_SERIAL_1_SRL_IZ(x)   (((x) & 3) << 1)
# define PLL_SERIAL_1_SRL_MAN_IZ  (1 << 6)
#define REG_PLL_SERIAL_2          REG(0x02, 0x01)     /* read/write */
# define PLL_SERIAL_2_SRL_NOSC(x) (((x) & 3) << 0)
# define PLL_SERIAL_2_SRL_PR(x)   (((x) & 0xf) << 4)
#define REG_PLL_SERIAL_3          REG(0x02, 0x02)     /* read/write */
# define PLL_SERIAL_3_SRL_CCIR    (1 << 0)
# define PLL_SERIAL_3_SRL_DE      (1 << 2)
# define PLL_SERIAL_3_SRL_PXIN_SEL (1 << 4)
#define REG_SERIALIZER            REG(0x02, 0x03)     /* read/write */
#define REG_BUFFER_OUT            REG(0x02, 0x04)     /* read/write */
#define REG_PLL_SCG1              REG(0x02, 0x05)     /* read/write */
#define REG_PLL_SCG2              REG(0x02, 0x06)     /* read/write */
#define REG_PLL_SCGN1             REG(0x02, 0x07)     /* read/write */
#define REG_PLL_SCGN2             REG(0x02, 0x08)     /* read/write */
#define REG_PLL_SCGR1             REG(0x02, 0x09)     /* read/write */
#define REG_PLL_SCGR2             REG(0x02, 0x0a)     /* read/write */
#define REG_AUDIO_DIV             REG(0x02, 0x0e)     /* read/write */
# define AUDIO_DIV_SERCLK_1       0
# define AUDIO_DIV_SERCLK_2       1
# define AUDIO_DIV_SERCLK_4       2
# define AUDIO_DIV_SERCLK_8       3
# define AUDIO_DIV_SERCLK_16      4
# define AUDIO_DIV_SERCLK_32      5
#define REG_SEL_CLK               REG(0x02, 0x11)     /* read/write */
# define SEL_CLK_SEL_CLK1         (1 << 0)
# define SEL_CLK_SEL_VRF_CLK(x)   (((x) & 3) << 1)
# define SEL_CLK_ENA_SC_CLK       (1 << 3)
#define REG_ANA_GENERAL           REG(0x02, 0x12)     /* read/write */


/* Page 09h: EDID Control */
#define REG_EDID_DATA_0           REG(0x09, 0x00)     /* read */
/* next 127 successive registers are the EDID block */
#define REG_EDID_CTRL             REG(0x09, 0xfa)     /* read/write */
#define REG_DDC_ADDR              REG(0x09, 0xfb)     /* read/write */
#define REG_DDC_OFFS              REG(0x09, 0xfc)     /* read/write */
#define REG_DDC_SEGM_ADDR         REG(0x09, 0xfd)     /* read/write */
#define REG_DDC_SEGM              REG(0x09, 0xfe)     /* read/write */


/* Page 10h: information frames and packets */
#define REG_AVI_IF                REG(0x10, 0x40)   /* AVI Infoframe packet */
#define REG_AUDIO_IF              REG(0x10, 0x80)   /* AVI Infoframe packet */

/* Page 11h: audio settings and content info packets */
#define REG_AIP_CNTRL_0           REG(0x11, 0x00)     /* read/write */
# define AIP_CNTRL_0_RST_FIFO     (1 << 0)
# define AIP_CNTRL_0_SWAP         (1 << 1)
# define AIP_CNTRL_0_LAYOUT       (1 << 2)
# define AIP_CNTRL_0_ACR_MAN      (1 << 5)
# define AIP_CNTRL_0_RST_CTS      (1 << 6)
#define REG_ACR_CTS_0             REG(0x11, 0x05)
#define REG_ACR_CTS_1             REG(0x11, 0x06)
#define REG_ACR_CTS_2             REG(0x11, 0x07)
#define REG_ACR_N_0               REG(0x11, 0x08)
#define REG_ACR_N_1               REG(0x11, 0x09)
#define REG_ACR_N_2               REG(0x11, 0x0a)
#define REG_GC_AVMUTE             REG(0x11, 0x0b)
# define GC_AVMUTE_CLRMUTE        (1 << 0)
# define GC_AVMUTE_SETMUTE        (1 << 1)
#define REG_CTS_N                 REG(0x11, 0x0c)
#define REG_ENC_CNTRL             REG(0x11, 0x0d)     /* read/write */
# define ENC_CNTRL_RST_ENC        (1 << 0)
# define ENC_CNTRL_RST_SEL        (1 << 1)
# define ENC_CNTRL_CTL_CODE(x)    (((x) & 3) << 2)
#define REG_DIP_FLAGS             REG(0x11, 0x0e)
# define DIP_FLAGS_ACR            (1 << 0)
#define REG_DIP_IF_FLAGS          REG(0x11, 0x0f)     /* read/write */
#define DIP_IF_FLAGS_IF1          (1 << 1)
#define DIP_IF_FLAGS_IF2          (1 << 2)
#define DIP_IF_FLAGS_IF3          (1 << 3)
#define DIP_IF_FLAGS_IF4          (1 << 4)
#define DIP_IF_FLAGS_IF5          (1 << 5)

/* Page 12h: HDCP and OTP */
#define REG_TX3                   REG(0x12, 0x9a)     /* read/write */
#define REG_TX33                  REG(0x12, 0xb8)     /* read/write */
# define TX33_HDMI                (1 << 1)


/* Page 13h: Gamut related metadata packets */



/* CEC registers: (not paged)
 */
#define REG_CEC_FRO_IM_CLK_CTRL   0xfb                /* read/write */
# define CEC_FRO_IM_CLK_CTRL_GHOST_DIS (1 << 7)
# define CEC_FRO_IM_CLK_CTRL_ENA_OTP   (1 << 6)
# define CEC_FRO_IM_CLK_CTRL_IMCLK_SEL (1 << 1)
# define CEC_FRO_IM_CLK_CTRL_FRO_DIV   (1 << 0)
#define REG_CEC_RXSHPDLEV         0xfe                /* read */
# define CEC_RXSHPDLEV_RXSENS     (1 << 0)
# define CEC_RXSHPDLEV_HPD        (1 << 1)

#define REG_CEC_ENAMODS           0xff                /* read/write */
# define CEC_ENAMODS_DIS_FRO      (1 << 6)
# define CEC_ENAMODS_DIS_CCLK     (1 << 5)
# define CEC_ENAMODS_EN_RXSENS    (1 << 2)
# define CEC_ENAMODS_EN_HDMI      (1 << 1)
# define CEC_ENAMODS_EN_CEC       (1 << 0)


/* Device versions: */
#define TDA9989N2                 0x0101
#define TDA19989                  0x0201
#define TDA19989N2                0x0202
#define TDA19988                  0x0301

//------------------------------------------------------------------------------

#define CECWrite(subAddr, pBuffer, size)		I2CSetSlaveAddress(g_hI2CDevice, (UINT16)I2C_CEC_ADDR); \
													I2CWrite(g_hI2CDevice, subAddr, pBuffer, size)

#define CECRead(subAddr, pBuffer, size)			I2CSetSlaveAddress(g_hI2CDevice, (UINT16)I2C_CEC_ADDR); \
													I2CRead(g_hI2CDevice, subAddr, pBuffer, size)

#define REG(page, addr) (((page) << 8) | (addr))
#define REG2ADDR(reg)   ((reg) & 0xff)
#define REG2PAGE(reg)   (((reg) >> 8) & 0xff)

#define REG_CURPAGE               0xff

struct tda998x_encoder_params {
	UINT8 swap_b:3;
	UINT8 mirr_b:1;
	UINT8 swap_a:3;
	UINT8 mirr_a:1;
	UINT8 swap_d:3;
	UINT8 mirr_d:1;
	UINT8 swap_c:3;
	UINT8 mirr_c:1;
	UINT8 swap_f:3;
	UINT8 mirr_f:1;
	UINT8 swap_e:3;
	UINT8 mirr_e:1;

	UINT8 audio_cfg;
	UINT8 audio_clk_cfg;
	UINT8 audio_frame[6];

	enum {
		AFMT_SPDIF,
		AFMT_I2S
	} audio_format;

	unsigned audio_sample_rate;
};


static UINT8	g_current_page;
static HANDLE   g_hI2CDevice = NULL;
    



static void 
set_page(UINT16 reg)
{
	UINT8 page = REG2PAGE(reg);

	I2CSetSlaveAddress(g_hI2CDevice, (UINT16)I2C_HDMI_ADDR);
	if (page != g_current_page) 
	{
		I2CWrite(g_hI2CDevice, REG_CURPAGE, &page, sizeof(page));
		g_current_page = page;
	}
}


static UINT
reg_read_range(UINT16 reg, VOID *buf, UINT32 size)
{
	set_page(reg);
	return I2CRead(g_hI2CDevice, REG2ADDR(reg), &buf, size);
}


static UINT 
reg_write_range(UINT16 reg, VOID *pBuffer, UINT32 size)
{
	set_page(reg);
	return I2CWrite(g_hI2CDevice, REG2ADDR(reg), pBuffer, size);
}


static UINT8 
reg_read(UINT16 reg)
{
	UINT8 val = 0;

	set_page(reg);
	I2CRead(g_hI2CDevice, REG2ADDR(reg), &val, sizeof(val));
	return val;
}


static void 
reg_write(UINT16 reg, UINT8 val)
{
	set_page(reg);
	I2CWrite(g_hI2CDevice, REG2ADDR(reg), &val, 1);
}


static void 
reg_write16(UINT16 reg, UINT16 val)
{
	UINT8 msb,lsb;

	msb = (UINT8)(val >> 8);
	lsb = (UINT8)(val & 0xff);
	set_page(reg);
	I2CWrite(g_hI2CDevice, REG2ADDR(reg), &msb, 1);
	I2CWrite(g_hI2CDevice, REG2ADDR(reg+1), &lsb, 1);
}


static void 
reg_set( UINT16 reg, UINT8 val)
{
	reg_write(reg, reg_read(reg) | val);
}


static void 
reg_clear(UINT16 reg, UINT8 val)
{
	reg_write(reg, reg_read(reg) & ~val);
}



static void
reg_dump_page(UINT8 page)
{
	UINT8 buf[0xff];
	DWORD i;

	memset(buf, 0, 0xff);
	OALMSG(1, (L"\r\nPage: 0x%02x", page));
	for (i=0;i<0xFF;i++)
	{
		buf[i] = reg_read(page<<8 | i);
	}
	for (i=0;i<0xFF;i++)
	{
		if (i & 0x0F)
		{
			OALMSG(1, (L" %02x", buf[i]));
		}
		else
		{
			OALMSG(1, (L"\r\n%02x: %02x", i, buf[i]));
		}
	}
	OALMSG(1, (L"\r\n"));
	OALMSG(1, (L"\r\n"));
}




BOOL
tda998x_reset()
{
    if (g_hI2CDevice == NULL)
	{
		OALMSG(1, (L"tda1998x_init: Failed to open I2C driver\r\n"));
		return FALSE;
	}    

	/* reset audio and i2c master: */
	reg_set(REG_SOFTRESET, SOFTRESET_AUDIO | SOFTRESET_I2C_MASTER);
	OALStall(50000);
	reg_clear(REG_SOFTRESET, SOFTRESET_AUDIO | SOFTRESET_I2C_MASTER);
	OALStall(50000);

	/* reset transmitter: */
	reg_set(REG_MAIN_CNTRL0, MAIN_CNTRL0_SR);
	reg_clear(REG_MAIN_CNTRL0, MAIN_CNTRL0_SR);

	/* PLL registers common configuration */
	reg_write(REG_PLL_SERIAL_1, 0x00);
	reg_write(REG_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(1));
	reg_write(REG_PLL_SERIAL_3, 0x00);
	reg_write(REG_SERIALIZER,   0x00);
	reg_write(REG_BUFFER_OUT,   0x00);
	reg_write(REG_PLL_SCG1,     0x00);
	reg_write(REG_AUDIO_DIV,    AUDIO_DIV_SERCLK_8);
	reg_write(REG_SEL_CLK,      SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);
	reg_write(REG_PLL_SCGN1,    0xfa);
	reg_write(REG_PLL_SCGN2,    0x00);
	reg_write(REG_PLL_SCGR1,    0x5b);
	reg_write(REG_PLL_SCGR2,    0x00);
	reg_write(REG_PLL_SCG2,     0x10);
	return TRUE;
}

/* DRM encoder functions */

BOOL
tda998x_encoder_dpms(BOOL enable)
{
    if (g_hI2CDevice == NULL)
	{
		OALMSG(1, (L"tda1998x_init: Failed to open I2C driver\r\n"));
		return FALSE;
	}    

	if (enable)
	{
		/* enable audio and video ports */
		reg_write(REG_ENA_AP, 0x03);
		reg_write(REG_ENA_VP_0, 0xff);
		reg_write(REG_ENA_VP_1, 0xff);
		reg_write(REG_ENA_VP_2, 0xff);
		/* set muxing after enabling ports: */
		reg_write(REG_VIP_CNTRL_0,
				VIP_CNTRL_0_SWAP_A(2) | VIP_CNTRL_0_SWAP_B(3));
		reg_write(REG_VIP_CNTRL_1,
				VIP_CNTRL_1_SWAP_C(0) | VIP_CNTRL_1_SWAP_D(1));
		reg_write(REG_VIP_CNTRL_2,
				VIP_CNTRL_2_SWAP_E(4) | VIP_CNTRL_2_SWAP_F(5));
//reg_dump_page(0);
//reg_dump_page(2);
	}
	else
	{
		/* disable audio and video ports */
		reg_write(REG_ENA_AP, 0x00);
		reg_write(REG_ENA_VP_0, 0x00);
		reg_write(REG_ENA_VP_1, 0x00);
		reg_write(REG_ENA_VP_2, 0x00);
	}
	return TRUE;
}

/* lookup table for CEA values to VIDFORMAT values taken from NXP datasheet */
static char cea_to_nxp_mode[34] = {-1, 0, 1, 1, 2, 3, 4, 4, 5, 5, -1, -1, 
		-1, -1, -1, -1, 6, 7, 7, 8, 9, 10, 10,
		11, 11, -1, -1, -1, -1, -1, -1, 12, 13};

static char tda998x_cea_to_vidformat(unsigned char cea_mode)
{
	if(cea_mode > (sizeof(cea_to_nxp_mode) -1) ) {
		return -1;
	}
	return cea_to_nxp_mode[cea_mode];
}

//static void 
//tda998x_write_if(void *hCtx, UINT8 bit, UINT16 addr,
//		 UINT8 *buf, size_t size)
//{
//	buf[PB(0)] = tda998x_cksum(buf, size);
//
//	reg_clear(hCtx, REG_DIP_IF_FLAGS, bit);
//	reg_write_range(hCtx, addr, buf, size);
//	reg_set(hCtx, REG_DIP_IF_FLAGS, bit);
//}
//
//static void 
//tda998x_write_aif(void *hCtx, struct tda998x_encoder_params *p)
//{
//	UINT8 buf[PB(5) + 1];
//
//	buf[HB(0)] = 0x84;
//	buf[HB(1)] = 0x01;
//	buf[HB(2)] = 10;
//	buf[PB(0)] = 0;
//	buf[PB(1)] = p->audio_frame[1] & 0x07; /* CC */
//	buf[PB(2)] = p->audio_frame[2] & 0x1c; /* SF */
//	buf[PB(4)] = p->audio_frame[4];
//	buf[PB(5)] = p->audio_frame[5] & 0xf8; /* DM_INH + LSV */
//
//	tda998x_write_if(hCtx, DIP_IF_FLAGS_IF4, REG_IF4_HB0, buf,
//			 sizeof(buf));
//}
//
//static void
//tda998x_write_avi(void *hCtx, struct drm_display_mode *mode)
//{
//	UINT8 buf[PB(13) + 1];
//
//	memset(buf, 0, sizeof(buf));
//	buf[HB(0)] = 0x82;
//	buf[HB(1)] = 0x02;
//	buf[HB(2)] = 13;
//	buf[PB(1)] = HDMI_SCAN_MODE_UNDERSCAN;
//	buf[PB(3)] = HDMI_QUANTIZATION_RANGE_FULL << 2;
//	buf[PB(4)] = drm_match_cea_mode(mode);
//
//	tda998x_write_if(hCtx, DIP_IF_FLAGS_IF2, REG_IF2_HB0, buf,
//			 sizeof(buf));
//}
//
//static void 
//tda998x_audio_mute(void *hCtx, BOOL on)
//{
//	if (on) {
//		reg_set(hCtx, REG_SOFTRESET, SOFTRESET_AUDIO);
//		reg_clear(hCtx, REG_SOFTRESET, SOFTRESET_AUDIO);
//		reg_set(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO);
//	} else {
//		reg_clear(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO);
//	}
//}
//
//static void
//tda998x_configure_audio(void *hCtx,	struct drm_display_mode *mode, struct tda998x_encoder_params *p)
//{
//	UINT8 buf[6], clksel_aip, clksel_fs, ca_i2s, cts_n, adiv;
//	UINT32 n;
//
//	/* Enable audio ports */
//	reg_write(hCtx, REG_ENA_AP, p->audio_cfg);
//	reg_write(hCtx, REG_ENA_ACLK, p->audio_clk_cfg);
//
//	/* Set audio input source */
//	switch (p->audio_format) {
//	case AFMT_SPDIF:
//		reg_write(hCtx, REG_MUX_AP, 0x40);
//		clksel_aip = AIP_CLKSEL_AIP(0);
//		/* FS64SPDIF */
//		clksel_fs = AIP_CLKSEL_FS(2);
//		cts_n = CTS_N_M(3) | CTS_N_K(3);
//		ca_i2s = 0;
//		break;
//
//	case AFMT_I2S:
//		reg_write(hCtx, REG_MUX_AP, 0x64);
//		clksel_aip = AIP_CLKSEL_AIP(1);
//		/* ACLK */
//		clksel_fs = AIP_CLKSEL_FS(0);
//		cts_n = CTS_N_M(3) | CTS_N_K(3);
//		ca_i2s = CA_I2S_CA_I2S(0);
//		break;
//
//	default:
//		return;
//	}
//
//	reg_write(hCtx, REG_AIP_CLKSEL, clksel_aip);
//	reg_clear(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_LAYOUT);
//
//	/* Enable automatic CTS generation */
//	reg_clear(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_ACR_MAN);
//	reg_write(hCtx, REG_CTS_N, cts_n);
//
//	/*
//	 * Audio input somehow depends on HDMI line rate which is
//	 * related to pixclk. Testing showed that modes with pixclk
//	 * >100MHz need a larger divider while <40MHz need the default.
//	 * There is no detailed info in the datasheet, so we just
//	 * assume 100MHz requires larger divider.
//	 */
////TODO:DV fix!
//	//if (mode->clock > 100000)
//	//	adiv = AUDIO_DIV_SERCLK_16;
//	//else
//		adiv = AUDIO_DIV_SERCLK_8;
//	reg_write(hCtx, REG_AUDIO_DIV, adiv);
//
//	/*
//	 * This is the approximate value of N, which happens to be
//	 * the recommended values for non-coherent clocks.
//	 */
//	n = 128 * p->audio_sample_rate / 1000;
//
//	/* Write the CTS and N values */
//	buf[0] = 0x44;
//	buf[1] = 0x42;
//	buf[2] = 0x01;
//	buf[3] = (UINT8)n;
//	buf[4] = (UINT8)(n >> 8);
//	buf[5] = (UINT8)(n >> 16);
//	reg_write_range(hCtx, REG_ACR_CTS_0, buf, 6);
//
//	/* Set CTS clock reference */
//	reg_write(hCtx, REG_AIP_CLKSEL, clksel_aip | clksel_fs);
//
//	/* Reset CTS generator */
//	reg_set(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_CTS);
//	reg_clear(hCtx, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_CTS);
//
//	/* Write the channel status */
//	buf[0] = 0x04;
//	buf[1] = 0x00;
//	buf[2] = 0x00;
//	buf[3] = 0xf1;
//	reg_write_range(hCtx, REG_CH_STAT_B(0), buf, 4);
//
//	tda998x_audio_mute(hCtx, TRUE);
//	OALStall(20000);
//	tda998x_audio_mute(hCtx, FALSE);
//
//	/* Write the audio information packet */
//	tda998x_write_aif(hCtx, p);
//}


//void tda998x_encoder_mode_set(void *hCtx,
//			struct drm_display_mode *mode,
//			struct drm_display_mode *adjusted_mode)
BOOL
tda998x_encoder_mode_set(struct drm_display_mode *mode)
{
//	struct tda998x_priv *priv = to_tda998x_priv(encoder);
	UINT16 ref_pix, ref_line, n_pix, n_line;
	UINT16 hs_pix_s, hs_pix_e;
	UINT16 vs1_pix_s, vs1_pix_e, vs1_line_s, vs1_line_e;
	UINT16 vs2_pix_s, vs2_pix_e, vs2_line_s, vs2_line_e;
	UINT16 vwin1_line_s, vwin1_line_e;
	UINT16 vwin2_line_s, vwin2_line_e;
	UINT16 de_pix_s, de_pix_e;
	UINT8 reg, div, rep;

    if (g_hI2CDevice == NULL)
	{
		OALMSG(1, (L"tda1998x_init: Failed to open I2C driver\r\n"));
		return FALSE;
	}    
	/*
	 * Internally TDA998x is using ITU-R BT.656 style sync but
	 * we get VESA style sync. TDA998x is using a reference pixel
	 * relative to ITU to sync to the input frame and for output
	 * sync generation.
	 *
	 * Now there is some issues to take care of:
	 * - HDMI data islands require sync-before-active
	 * - TDA998x register values must be > 0 to be enabled
	 * - REFLINE needs an additional offset of +1
	 * - REFPIX needs an addtional offset of +1 for UYUV and +3 for RGB
	 *
	 * So we add +1 to all horizontal and vertical register values,
	 * plus an additional +3 for REFPIX as we are using RGB input only.
	 */

	n_pix        = mode->htotal;
	n_line       = mode->vtotal;

	ref_pix      = 3 + mode->hsync_start - mode->hdisplay;

	/*
	 * handle issue on TILCDC where it is outputing
	 * non-VESA compliant sync signals the workaround
	 * forces us to invert the HSYNC, so need to adjust display to
	 * the left by hskew pixels, provided by the tilcdc driver
	 */
	if (mode->flags & DRM_MODE_FLAG_HSKEW)
		ref_pix += mode->hskew;

	de_pix_s     = mode->htotal - mode->hdisplay;
	de_pix_e     = de_pix_s + mode->hdisplay;
	hs_pix_s     = mode->hsync_start - mode->hdisplay;
	hs_pix_e     = hs_pix_s + mode->hsync_end - mode->hsync_start;

	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) == 0) {
		ref_line     = 1 + mode->vsync_start - mode->vdisplay;
		vwin1_line_s = mode->vtotal - mode->vdisplay - 1;
		vwin1_line_e = vwin1_line_s + mode->vdisplay;
		vs1_pix_s    = vs1_pix_e = hs_pix_s;
		vs1_line_s   = mode->vsync_start - mode->vdisplay;
		vs1_line_e   = vs1_line_s +
			       mode->vsync_end - mode->vsync_start;
		vwin2_line_s = vwin2_line_e = 0;
		vs2_pix_s    = vs2_pix_e  = 0;
		vs2_line_s   = vs2_line_e = 0;
	} else {
		ref_line     = 1 + (mode->vsync_start - mode->vdisplay)/2;
		vwin1_line_s = (mode->vtotal - mode->vdisplay)/2;
		vwin1_line_e = vwin1_line_s + mode->vdisplay/2;
		vs1_pix_s    = vs1_pix_e = hs_pix_s;
		vs1_line_s   = (mode->vsync_start - mode->vdisplay)/2;
		vs1_line_e   = vs1_line_s +
			       (mode->vsync_end - mode->vsync_start)/2;
		vwin2_line_s = vwin1_line_s + mode->vtotal/2;
		vwin2_line_e = vwin2_line_s + mode->vdisplay/2;
		vs2_pix_s    = vs2_pix_e = hs_pix_s + mode->htotal/2;
		vs2_line_s   = vs1_line_s + mode->vtotal/2 ;
		vs2_line_e   = vs2_line_s +
			       (mode->vsync_end - mode->vsync_start)/2;
	}

	div = 148500 / mode->clock;

	/* mute the audio FIFO: */
	reg_set(REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO);

	/*  HDMI/HDCP mode off... for now...: */
	reg_set(REG_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS);
	reg_clear(REG_TX33, TX33_HDMI);

	reg_write(REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(0));
	/* no pre-filter or interpolator: */
	reg_write(REG_HVF_CNTRL_0, HVF_CNTRL_0_PREFIL(0) |
			HVF_CNTRL_0_INTPOL(0));
	reg_write(REG_VIP_CNTRL_5, VIP_CNTRL_5_SP_CNT(0));
	reg_write(REG_VIP_CNTRL_4, VIP_CNTRL_4_BLANKIT(0) |
			VIP_CNTRL_4_BLC(0));
	reg_clear(REG_PLL_SERIAL_3, PLL_SERIAL_3_SRL_CCIR);

	reg_clear(REG_PLL_SERIAL_1, PLL_SERIAL_1_SRL_MAN_IZ);
	reg_clear(REG_PLL_SERIAL_3, PLL_SERIAL_3_SRL_DE);
	reg_write(REG_SERIALIZER, 0);
	reg_write(REG_HVF_CNTRL_1, HVF_CNTRL_1_VQR(0));

	/* TODO enable pixel repeat for pixel rates less than 25Msamp/s */
	rep = 0;
	reg_write(REG_RPT_CNTRL, 0);
	reg_write(REG_SEL_CLK, SEL_CLK_SEL_VRF_CLK(0) |
			SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);

	reg_write(REG_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(div) |
			PLL_SERIAL_2_SRL_PR(rep));

	/* set color matrix bypass flag: */
	reg_set(REG_MAT_CONTRL, MAT_CONTRL_MAT_BP);

	/* set BIAS tmds value: */
	reg_write(REG_ANA_GENERAL, 0x09);

	reg_clear(REG_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_MTHD);

	/*
	 * Sync on rising HSYNC/VSYNC
	 */
	reg_write(REG_VIP_CNTRL_3, 0);
	reg_set(REG_VIP_CNTRL_3, VIP_CNTRL_3_SYNC_HS);

	/*
	 * TDA19988 requires high-active sync at input stage,
	 * so invert low-active sync provided by master encoder here
	 */
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		reg_set(REG_VIP_CNTRL_3, VIP_CNTRL_3_H_TGL);
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		reg_set(REG_VIP_CNTRL_3, VIP_CNTRL_3_V_TGL);

	/*
	 * Always generate sync polarity relative to input sync and
	 * revert input stage toggled sync at output stage
	 */
	reg = TBG_CNTRL_1_TGL_EN;
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		reg |= TBG_CNTRL_1_H_TGL;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		reg |= TBG_CNTRL_1_V_TGL;
	reg_write(REG_TBG_CNTRL_1, reg);

	reg_write(REG_VIDFORMAT, 0x00);
	reg_write16(REG_REFPIX_MSB, ref_pix);
	reg_write16(REG_REFLINE_MSB, ref_line);
	reg_write16(REG_NPIX_MSB, n_pix);
	reg_write16(REG_NLINE_MSB, n_line);
	reg_write16(REG_VS_LINE_STRT_1_MSB, vs1_line_s);
	reg_write16(REG_VS_PIX_STRT_1_MSB, vs1_pix_s);
	reg_write16(REG_VS_LINE_END_1_MSB, vs1_line_e);
	reg_write16(REG_VS_PIX_END_1_MSB, vs1_pix_e);
	reg_write16(REG_VS_LINE_STRT_2_MSB, vs2_line_s);
	reg_write16(REG_VS_PIX_STRT_2_MSB, vs2_pix_s);
	reg_write16(REG_VS_LINE_END_2_MSB, vs2_line_e);
	reg_write16(REG_VS_PIX_END_2_MSB, vs2_pix_e);
	reg_write16(REG_HS_PIX_START_MSB, hs_pix_s);
	reg_write16(REG_HS_PIX_STOP_MSB, hs_pix_e);
	reg_write16(REG_VWIN_START_1_MSB, vwin1_line_s);
	reg_write16(REG_VWIN_END_1_MSB, vwin1_line_e);
	reg_write16(REG_VWIN_START_2_MSB, vwin2_line_s);
	reg_write16(REG_VWIN_END_2_MSB, vwin2_line_e);
	reg_write16(REG_DE_START_MSB, de_pix_s);
	reg_write16(REG_DE_STOP_MSB, de_pix_e);

//	if (priv->rev == TDA19988) {
		/* let incoming pixels fill the active space (if any) */
		reg_write(REG_ENABLE_SPACE, 0x01);
//	}

//	if(tda998x_is_monitor_hdmi(encoder) == 1) {
//		char vidformat;
//		vidformat = tda998x_cea_to_vidformat(drm_match_cea_mode(mode));
//		if(vidformat == (char)-1) {
//			dev_err(encoder->dev->dev, "Not sure which CEA mode to set, leaving as DVI");
//			goto out;
//		}
//		dev_info(encoder->dev->dev, "Connected to an HDMI monitor with cea mode %d", vidformat);
//			
//		/* this is an HDMI monitor, so set things up a bit differently */
//		reg_write(REG_TBG_CNTRL_1, 0);
//		reg_write(encoder, REG_VIDFORMAT, vidformat);
//		/* get the infoframes pumping */
//		tda998x_avi_infoframe_enable(encoder, mode);
//		tda998x_audio_infoframe_enable(encoder);
		reg_set(REG_TX33, TX33_HDMI);
//
//		/* set up audio registers */
//		reg_write(encoder, REG_ACR_CTS_0, 0x0);
//		reg_write(encoder, REG_ACR_CTS_1, 0x0);
//		reg_write(encoder, REG_ACR_CTS_2, 0x0);
//
//		reg_write(encoder, REG_ACR_N_0, 0x0);
//		reg_write(encoder, REG_ACR_N_1, 0x18);
//		reg_write(encoder, REG_ACR_N_2, 0x0);
//
//		reg_set(encoder, REG_DIP_FLAGS, DIP_FLAGS_ACR);
//
//		reg_write(encoder, REG_ENC_CNTRL, 0x04);
//		reg_write(encoder, REG_CTS_N, 0x33);
//		/* Set 2 channel I2S mode */
//		reg_write(encoder, REG_ENA_AP, 0x3);
//
//		/* set audio divider in pll settings */
//		reg_write(encoder, REG_AUDIO_DIV, 0x2);
//
//		/* select the audio input port clock */
//		reg_write(encoder, REG_AIP_CLKSEL, SEL_AIP_I2S);
//		reg_write(encoder, REG_MUX_AP, MUX_AP_SELECT_I2S);
//
//		/* select I2S format, and datasize */
//		reg_write(encoder, REG_I2S_FORMAT, 0x0a);
//
//		/* enable the audio FIFO: */
//		reg_clear(encoder, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO);
//
//		/* mute and then unmute, to get audio going */
//		reg_write(encoder, REG_GC_AVMUTE, GC_AVMUTE_SETMUTE);
//		reg_write(encoder, REG_GC_AVMUTE, GC_AVMUTE_CLRMUTE);
//	}
//out:
	/* must be last register set: */
	reg_clear(REG_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_ONCE);

	return TRUE;
}


BOOL
tda998x_drm_to_panel(struct drm_display_mode *mode, struct display_panel *panel)
{
	panel->x_res = mode->hdisplay;
	panel->y_res = mode->vdisplay;

	panel->pixel_clock = mode->clock * 1000;
	panel->hbp = mode->htotal - mode->hsync_end;
	panel->hfp = mode->hsync_start - mode->hdisplay;
	panel->hsw = mode->hsync_end - mode->hsync_start;
	panel->vbp = mode->vtotal - mode->vsync_end;
	panel->vfp = mode->vsync_start - mode->vdisplay;
	panel->vsw = mode->vsync_end - mode->vsync_start;

	// subtract one because the hardware uses a value of 0 as 1
	if (panel->hbp)
		panel->hbp--;
	if(panel->hfp)
		panel->hfp--;
	if (panel->hsw)
		panel->hsw--;
	//if (panel->vbp)
	//	panel->vbp--;
	//if (panel->vfp)
	//	panel->vfp--;
	if (panel->vsw)
		panel->vsw--;

	panel->config = LCDC_PANEL_TFT | LCDC_HSVS_CONTROL | LCDC_HSVS_FALLING_EDGE | LCDC_INV_PIX_CLOCK;			// config
	// fixup HSYNC by inverting	
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		panel->config |= LCDC_INV_HSYNC;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		panel->config |= LCDC_INV_VSYNC;

	return TRUE;
}



BOOL
tda998x_connected_detect(void)
{
	UINT8 val;
	
    if (g_hI2CDevice == NULL)
	{
		OALMSG(1, (L"tda998x_connected_detect: Failed to open I2C driver\r\n"));
		return FALSE;
	}    
    
	CECRead(REG_CEC_RXSHPDLEV,&val,sizeof(val));

	return (val & CEC_RXSHPDLEV_HPD);
}



BOOL
tda1998x_init()
{
    UINT8 val;
	UINT written;
	UINT16 rev;

    // open i2c device
    g_hI2CDevice = I2COpen(AM_DEVICE_I2C0);  
    if (g_hI2CDevice == NULL)
	{
		OALMSG(1, (L"tda1998x_init: Failed to open I2C driver\r\n"));
		return FALSE;
	}    
    
    I2CSetBaudIndex(g_hI2CDevice,FULLSPEED_MODE);
    I2CSetSubAddressMode(g_hI2CDevice,I2C_SUBADDRESS_MODE_8);

	// wakeup
    val = CEC_ENAMODS_EN_RXSENS | CEC_ENAMODS_EN_HDMI;
    written = CECWrite(REG_CEC_ENAMODS, &val, sizeof(val));
	if (written != sizeof(val))
	{
		OALMSG(1, (L"tda1998x_init: Failed to write wakeup\r\n"));
		return FALSE;
	}    

	tda998x_reset();

	// read version
	rev = reg_read(REG_VERSION_LSB) |
			reg_read(REG_VERSION_MSB) << 8;

	// mask off feature bits
	rev &= ~0x30; /* not-hdcp and not-scalar bit */

	switch (rev) {
	case TDA9989N2:  OALMSG(1, (L"TDA9989 n2 found\r\n"));  break;
	case TDA19989:   OALMSG(1, (L"TDA19989 found\r\n"));    break;
	case TDA19989N2: OALMSG(1, (L"TDA19989 n2 found\r\n")); break;
	case TDA19988:   OALMSG(1, (L"TDA19988 found\r\n"));    break;
	default:
		OALMSG(1, (L"found unsupported device: %04x\r\n", rev));
		goto fail;
	}

	/* after reset, enable DDC: */
	reg_write(REG_DDC_DISABLE, 0x00);

	/* set clock on DDC channel: */
	reg_write(REG_TX3, 39);

	/* if necessary, disable multi-master: */
	if (rev == TDA19989)
		reg_set(REG_I2C_MASTER, I2C_MASTER_DIS_MM);

    val = CEC_FRO_IM_CLK_CTRL_GHOST_DIS | CEC_FRO_IM_CLK_CTRL_IMCLK_SEL;
	CECWrite(REG_CEC_FRO_IM_CLK_CTRL, &val, sizeof(val));

	return TRUE;

fail:
	I2CClose(g_hI2CDevice);
	return FALSE;
}


