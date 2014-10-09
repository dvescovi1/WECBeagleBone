// All rights reserved David Vescovi 2014
//
//  File:  tda1998x.c
//
#include "bsp.h"
#include "sdk_i2c.h"
#include "lcdc.h"
#include "tda.h"
#include "tda1998x.h"


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
	UINT8 msb =0;
	UINT8 lsb =0;

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
	UINT16 i;

	memset(buf, 0, 0xff);
	OALMSG(1, (L"Page: 0x%02x\r\n", page));
	for (i=0;i<0xFF;i++)
	{
		buf[i] = reg_read(page<<8 | i);
	}
	for (i=0;i<0x00FF;i++)
	{
		if (i & 0x0F)
		{
			OALMSG(1, (L" %02x\r\n", buf[i]));
		}
		else
		{
			OALMSG(1, (L"%02x: %02x\r\n", i, buf[i]));
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

	/* Write the default value MUX register */
	reg_write(REG_MUX_VP_VIP_OUT, 0x24);

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

		reg_write(REG_ENABLE_SPACE, 0x01);
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




BOOL
tda998x_encoder_mode_set(struct drm_display_mode *mode)
{
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

	/* set HDMI HDCP mode off: */
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

	reg_write(REG_ENABLE_SPACE, 0x01);

	/* must be last register set: */
	reg_clear(REG_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_ONCE);

	reg_clear(REG_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS);
	reg_write(REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(1));
	reg_set(REG_TX33, TX33_HDMI);

//	reg_dump_page(0);
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

	// subtract one because the hardware uses a value of 0 as 1 for 
	// some registers
	if (panel->hbp)
		panel->hbp--;
	if(panel->hfp)
		panel->hfp--;
	if (panel->hsw)
		panel->hsw--;
	if (panel->vsw)
		panel->vsw--;

	panel->config = LCDC_PANEL_TFT | LCDC_HSVS_CONTROL | LCDC_INV_PIX_CLOCK | LCDC_HSVS_FALLING_EDGE;			// config
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

	switch (rev) 
	{
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

