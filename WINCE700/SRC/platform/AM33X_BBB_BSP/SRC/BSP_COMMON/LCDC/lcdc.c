/*
 *   Copyright (c) Texas Instruments Incorporated 2009. All Rights Reserved.
 *
 *   Use of this software is controlled by the terms and conditions found 
 *   in the license agreement under which this software has been supplied.
 * 
 *   lcdc.c
 */

#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <nkintr.h>
#include "lcdc.h"
#include "am33x_clocks.h"
#include "Image_Cfg.h"
#include "bsp_def.h"
#include "sdk_gpio.h"
#include "bsp_padcfg.h"

//
//  Defines
//
static BOOL bDVIEnabled = FALSE;

static void udelay(UINT32 delay)
{
	volatile UINT32 tmp;
	volatile UINT32 j;
	UINT32 i;
	
	for(i=0; i<delay; i++)
		for(j=0; j<10000; j++)
			tmp = j;
}

static void lcdc_dumpregs(struct lcdc *lcdc) {
    RETAILMSG(TRUE, ((L"----\r\n")));
    RETAILMSG(TRUE, ((L"lcdc->regs->PID =               0x%08x  addr=0x%08x\r\n"), lcdc->regs->PID,               &lcdc->regs->PID));
    RETAILMSG(TRUE, ((L"lcdc->regs->CTRL =              0x%08x  addr=0x%08x\r\n"), lcdc->regs->CTRL,              &lcdc->regs->CTRL));
    RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CTRL =         0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CTRL,         &lcdc->regs->LIDD_CTRL));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS0_CONF =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS0_CONF,     &lcdc->regs->LIDD_CS0_CONF));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS0_ADDR =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS0_ADDR,     &lcdc->regs->LIDD_CS0_ADDR));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS0_DATA =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS0_DATA,     &lcdc->regs->LIDD_CS0_DATA));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS1_CONF =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS1_CONF,     &lcdc->regs->LIDD_CS1_CONF));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS1_ADDR =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS1_ADDR,     &lcdc->regs->LIDD_CS1_ADDR));
    //RETAILMSG(TRUE, ((L"lcdc->regs->LIDD_CS1_DATA =  	0x%08x  addr=0x%08x\r\n"), lcdc->regs->LIDD_CS1_DATA,     &lcdc->regs->LIDD_CS1_DATA));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_CTRL     =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_CTRL,       &lcdc->regs->RASTER_CTRL));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_TIMING_0 =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_TIMING_0,   &lcdc->regs->RASTER_TIMING_0));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_TIMING_1 =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_TIMING_1,   &lcdc->regs->RASTER_TIMING_1));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_TIMING_2 =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_TIMING_2,   &lcdc->regs->RASTER_TIMING_2));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_SUBPANEL =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_SUBPANEL,   &lcdc->regs->RASTER_SUBPANEL));
    RETAILMSG(TRUE, ((L"lcdc->regs->RASTER_SUBPANEL2 =  0x%08x  addr=0x%08x\r\n"), lcdc->regs->RASTER_SUBPANEL2,  &lcdc->regs->RASTER_SUBPANEL2));
    RETAILMSG(TRUE, ((L"lcdc->regs->LCDDMA_CTRL =       0x%08x  addr=0x%08x\r\n"), lcdc->regs->LCDDMA_CTRL,       &lcdc->regs->LCDDMA_CTRL));
    RETAILMSG(TRUE, ((L"lcdc->regs->LCDDMA_FB0_BASE =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->LCDDMA_FB0_BASE,   &lcdc->regs->LCDDMA_FB0_BASE));
    RETAILMSG(TRUE, ((L"lcdc->regs->LCDDMA_FB0_CEILING =0x%08x  addr=0x%08x\r\n"), lcdc->regs->LCDDMA_FB0_CEILING,&lcdc->regs->LCDDMA_FB0_CEILING));
    RETAILMSG(TRUE, ((L"lcdc->regs->LCDDMA_FB1_BASE =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->LCDDMA_FB1_BASE,   &lcdc->regs->LCDDMA_FB1_BASE));
    RETAILMSG(TRUE, ((L"lcdc->regs->LCDDMA_FB1_CEILING =0x%08x  addr=0x%08x\r\n"), lcdc->regs->LCDDMA_FB1_CEILING,&lcdc->regs->LCDDMA_FB1_CEILING));    
    RETAILMSG(TRUE, ((L"lcdc->regs->SYSCONFIG     =     0x%08x  addr=0x%08x\r\n"), lcdc->regs->SYSCONFIG,         &lcdc->regs->SYSCONFIG));    
    RETAILMSG(TRUE, ((L"lcdc->regs->IRQSTATUS_RAW =     0x%08x  addr=0x%08x\r\n"), lcdc->regs->IRQSTATUS_RAW,     &lcdc->regs->IRQSTATUS_RAW));    
    RETAILMSG(TRUE, ((L"lcdc->regs->IRQSTATUS     =     0x%08x  addr=0x%08x\r\n"), lcdc->regs->IRQSTATUS,         &lcdc->regs->IRQSTATUS));    
    RETAILMSG(TRUE, ((L"lcdc->regs->IRQENABLE_SET =     0x%08x  addr=0x%08x\r\n"), lcdc->regs->IRQENABLE_SET,     &lcdc->regs->IRQENABLE_SET));    
    RETAILMSG(TRUE, ((L"lcdc->regs->IRQENABLE_CLEAR =   0x%08x  addr=0x%08x\r\n"), lcdc->regs->IRQENABLE_CLEAR,   &lcdc->regs->IRQENABLE_CLEAR));    
    RETAILMSG(TRUE, ((L"lcdc->regs->CLKC_ENABLE =       0x%08x  addr=0x%08x\r\n"), lcdc->regs->CLKC_ENABLE,       &lcdc->regs->CLKC_ENABLE));    
    RETAILMSG(TRUE, ((L"lcdc->regs->CLKC_RESET =        0x%08x  addr=0x%08x\r\n"), lcdc->regs->CLKC_RESET,        &lcdc->regs->CLKC_RESET));    

}


//------------------------------------------------------------------------------
DWORD lcdc_PixelFormatToPixelSize( DWORD  ePixelFormat)
{
    DWORD   dwResult = 1;
    
    //  Convert pixel format into bytes per pixel
    switch( ePixelFormat )
    {
        case DISPC_PIXELFORMAT_RGB16:
        case DISPC_PIXELFORMAT_ARGB16:
        case DISPC_PIXELFORMAT_YUV2:
        case DISPC_PIXELFORMAT_UYVY:
            //  2 bytes per pixel
            dwResult = 2;
            break;

        case DISPC_PIXELFORMAT_RGB24:
            //  3 bytes per pixel
            dwResult = 3;
            break;

        case DISPC_PIXELFORMAT_RGB32:
        case DISPC_PIXELFORMAT_ARGB32:
        case DISPC_PIXELFORMAT_RGBA32:
            //  4 bytes per pixel
            dwResult = 4;
            break;
    }

    //  Return result
    return dwResult;
}

/* Enable the Raster Engine of the LCD Controller */
static __inline void lcdc_enable_raster(struct lcdc *lcdc)
{
	lcdc->regs->RASTER_CTRL |= LCDC_RASTER_CTRL_LCD_EN;
}

/* Disable the Raster Engine of the LCD Controller */
static __inline void lcdc_disable_raster(struct lcdc *lcdc)
{
	lcdc->regs->RASTER_CTRL &= ~LCDC_RASTER_CTRL_LCD_EN;
}

static void lcdc_blit(struct lcdc *lcdc, unsigned int load_mode)
{
	UINT32 start, end;
	UINT32 reg_ras, reg_dma, reg_irq;

	/* init reg to clear PLM (loading mode) fields */
	reg_ras = lcdc->regs->RASTER_CTRL;
	reg_ras &= ~(3 << 20);

	reg_dma  = lcdc->regs->LCDDMA_CTRL;

	/* always enable underflow interrupt */
	reg_irq = lcdc->regs->IRQENABLE_SET | LCDC_IRQENABLE_UNDERFLOW_INT; 

	if (load_mode == LCDC_LOAD_FRAME)
	{
		start = lcdc->fb_pa/* + 0x40000*/;
		end = start + lcdc->fb_cur_win_size -1;

		reg_irq |= LCDC_IRQENABLE_END_OF_FRAME0_INT | LCDC_IRQENABLE_END_OF_FRAME1_INT | LCDC_IRQENABLE_RASTER_DONE_INT;
		reg_dma |= LCDC_DMA_CTRL_DUAL_FRAME_BUFFER_EN;

		lcdc->regs->LCDDMA_FB0_BASE = start;
		lcdc->regs->LCDDMA_FB0_CEILING = end;
		lcdc->regs->LCDDMA_FB1_BASE = start;
		lcdc->regs->LCDDMA_FB1_CEILING = end;
	}
	else if (load_mode == LCDC_LOAD_PALETTE) 
	{
		start = lcdc->palette_phys;
		end = start + lcdc->palette_size -1;

		reg_irq |= LCDC_IRQENABLE_PALETTE_INT;

		lcdc->regs->LCDDMA_FB0_BASE = start;
		lcdc->regs->LCDDMA_FB0_CEILING = end;
	}
	reg_ras |= load_mode <<20;
	lcdc->regs->IRQENABLE_SET = reg_irq;
	lcdc->regs->LCDDMA_CTRL = reg_dma;
	lcdc->regs->RASTER_CTRL = reg_ras;

	/*
	 * The Raster enable bit must be set after all other control fields are
	 * set.
	 */
	lcdc_enable_raster(lcdc);
}

/* Configure the Burst Size and fifo threhold of DMA */
BOOL lcdc_cfg_dma(struct lcdc *lcdc)
{
	UINT32 tmp;

	lcdc->dma_fifo_thresh = LCDC_DMA_CTRL_TH_FIFO_READY_512;

	tmp = lcdc->regs->LCDDMA_CTRL & (LCDC_DMA_CTRL_DUAL_FRAME_BUFFER_EN | 
									 LCDC_DMA_CTRL_BURST_SIZE_MASK |
									 LCDC_DMA_CTRL_TH_FIFO_READY_MASK);

	switch (lcdc->dma_burst_sz) {
	case 1:
			tmp |= LCDC_DMA_CTRL_BURST_SIZE(LCDC_DMA_CTRL_BURST_1);
		break;
	case 2:
			tmp |= LCDC_DMA_CTRL_BURST_SIZE(LCDC_DMA_CTRL_BURST_2);
		break;
	case 4:
			tmp |= LCDC_DMA_CTRL_BURST_SIZE(LCDC_DMA_CTRL_BURST_4);
		break;
	case 8:
			tmp |= LCDC_DMA_CTRL_BURST_SIZE(LCDC_DMA_CTRL_BURST_8);
		break;
	case 16:
			tmp |= LCDC_DMA_CTRL_BURST_SIZE(LCDC_DMA_CTRL_BURST_16);
		break;
	default:
		return FALSE;
	}

	tmp |= LCDC_DMA_CTRL_TH_FIFO_READY(lcdc->dma_fifo_thresh);

	lcdc->regs->LCDDMA_CTRL = tmp;
	return TRUE;
}

static void lcdc_cfg_ac_bias(struct lcdc *lcdc, int period, int transitions_per_int)
{
	UINT32 tmp;

	/* Set the AC Bias Period and Number of Transisitons per Interrupt */
	
	tmp = lcdc->regs->RASTER_TIMING_2 & 0xFFF000FF;
	tmp |= LCDC_AC_BIAS_FREQUENCY(period) |
		LCDC_AC_BIAS_TRANSITIONS_PER_INT(transitions_per_int);
	lcdc->regs->RASTER_TIMING_2 = tmp;
}

static void lcdc_cfg_horizontal_sync(struct lcdc *lcdc)
{
	UINT32 tmp;

	tmp = lcdc->regs->RASTER_TIMING_0 & 0x000003FF;
	tmp |= ((lcdc->panel->hbp & 0xff) << 24) |
		     ((lcdc->panel->hfp & 0xff) << 16) |
		     ((lcdc->panel->hsw & 0x3f) << 10);
	lcdc->regs->RASTER_TIMING_0 = tmp;

	tmp = lcdc->regs->RASTER_TIMING_2 & 0x87FFFFFF;
	tmp |= (((lcdc->panel->hsw >>6) & 0xf) << 27);
	lcdc->regs->RASTER_TIMING_2 = tmp;
}

static void lcdc_cfg_vertical_sync(struct lcdc *lcdc)
{
	UINT32 tmp;

	tmp = lcdc->regs->RASTER_TIMING_1 & 0x000003FF;
	tmp |= ((lcdc->panel->vbp & 0xff) << 24) |
		     ((lcdc->panel->vfp & 0xff) << 16) |
		     ((lcdc->panel->vsw & 0x3f) << 10) ;
	lcdc->regs->RASTER_TIMING_1 = tmp;
}

static void lcdc_cfg_display(struct lcdc *lcdc)
{
	UINT32 tmp;

	tmp = lcdc->regs->RASTER_CTRL ;
	tmp &= ~(LCDC_RASTER_CTRL_LCD_TFT | LCDC_RASTER_CTRL_MONO_8BIT_MODE| LCDC_RASTER_CTRL_MONOCHROME_MODE | LCDC_RASTER_CTRL_LCD_TFT_24 |
		 LCDC_RASTER_CTRL_TFT_24BPP_UNPACK | LCDC_RASTER_CTRL_LCD_STN_565 | LCDC_RASTER_CTRL_TFT_ALT_ENABLE);
	
	if(lcdc->panel->pixel_format == DISPC_PIXELFORMAT_RGB24 ||
	   lcdc->panel->pixel_format == DISPC_PIXELFORMAT_RGB32 ||
	   lcdc->panel->pixel_format == DISPC_PIXELFORMAT_ARGB32 ||
	   lcdc->panel->pixel_format == DISPC_PIXELFORMAT_RGBA32) 
	   	tmp |= LCDC_RASTER_CTRL_LCD_TFT_24; 
	   
	if(lcdc->panel->pixel_format == DISPC_PIXELFORMAT_RGB32 ||
	   lcdc->panel->pixel_format == DISPC_PIXELFORMAT_ARGB32 ||
	   lcdc->panel->pixel_format == DISPC_PIXELFORMAT_RGBA32)
		tmp |= LCDC_RASTER_CTRL_TFT_24BPP_UNPACK;
	   
	switch (lcdc->panel->panel_shade) {
        	case MONOCHROME:
        		tmp |= LCDC_RASTER_CTRL_MONOCHROME_MODE;
        		if (lcdc->panel->mono_8bit_mode)
        			tmp |= LCDC_RASTER_CTRL_MONO_8BIT_MODE;
        		break;
        	case COLOR_ACTIVE:
        		tmp |= LCDC_RASTER_CTRL_LCD_TFT;
        		if (lcdc->panel->tft_alt_mode)
        			tmp |= LCDC_RASTER_CTRL_TFT_ALT_ENABLE;
        		break;
        	case COLOR_PASSIVE:
        		if (lcdc->color_mode == DISPC_PIXELFORMAT_RGB16)
        			tmp |= LCDC_RASTER_CTRL_LCD_STN_565;
        		break;
        	default:
        		return ;
	}

	tmp &= ~(LCDC_RASTER_CTRL_REQDLY_MASK);

	lcdc->regs->RASTER_CTRL = tmp;

	tmp = lcdc->regs->RASTER_TIMING_2;
	tmp &= ~(LCDC_SIGNAL_MASK << 20);
	tmp |= ((lcdc->panel->config & LCDC_SIGNAL_MASK) << 20);
	lcdc->regs->RASTER_TIMING_2 = tmp;
}

static void lcdc_cfg_frame_buffer(struct lcdc *lcdc)
{
	UINT32 tmp;
       UINT32  width, height;
	   
	/* Set the Panel Width */
	/* Pixels per line = (PPL + 1)*16 */
	/*
	 * 0x7F in bits 4..10 gives max horizontal resolution = 2048
	 * pixels.
	 */

	tmp = lcdc->regs->RASTER_TIMING_0 & 0xfffffc00; 
	width = lcdc->panel->x_res & 0x7f0;
	width = (width >> 4) - 1;
	tmp |= ((width & 0x3f) << 4) | ((width & 0x40) >> 3); 
	lcdc->regs->RASTER_TIMING_0 = tmp;
	
	/* Set the Panel Height */
	/* Set bits 9:0 of Lines Per Pixel */
	tmp = lcdc->regs->RASTER_TIMING_1 & 0xfffffc00; 
	height = lcdc->panel->y_res;
	
	tmp |= ((height - 1) & 0x3ff) ;
	lcdc->regs->RASTER_TIMING_1 = tmp;
	
	/* Set bit 10 of Lines Per Pixel */
	tmp = lcdc->regs->RASTER_TIMING_2 ; 
	tmp |= ((height - 1) & 0x400) << 16;    //bit 26 in timing2 register
	lcdc->regs->RASTER_TIMING_2 = tmp;
	
	/* Set the Raster Order of the Frame Buffer */
	tmp = lcdc->regs->RASTER_CTRL ;
//	tmp |= (LCDC_RASTER_CTRL_RASTER_ORDER | LCDC_LOAD_FRAME<<20);
	tmp |= LCDC_RASTER_CTRL_PALMODE(LCDC_LOAD_FRAME);
	lcdc->regs->RASTER_CTRL = tmp;
	
	return;
}

static void lcdc_config_clk_divider(struct lcdc* lcdc)
{
	UINT32 lcd_clk, div;
	UINT32 tmp;

	lcd_clk = lcdc->clk ; //in HZ
	div = lcd_clk / lcdc->panel->pixel_clock;
     
	/* Configure the LCD clock divisor. */
	tmp = lcdc->regs->CTRL;
	tmp &= ~(LCDC_CTRL_CLKDIV_MASK);
	tmp |= LCDC_CTRL_CLKDIV(div) | LCDC_CTRL_MODESEL_RASTER;
//	udelay(10);
	lcdc->regs->CTRL = tmp;

	lcdc->regs->CLKC_ENABLE = LCDC_CLKC_ENABLE_DMA | LCDC_CLKC_ENABLE_LIDD |
				          LCDC_CLKC_ENABLE_CORE;

}

void lcdc_setup_regs(struct lcdc* lcdc) {

	lcdc_disable_raster(lcdc);

	lcdc_config_clk_divider(lcdc);
	lcdc_cfg_display(lcdc);

	/* Configure the AC bias properties. */
	lcdc_cfg_ac_bias(lcdc, 0xff, 0x0);

	/* Configure the vertical and horizontal sync properties. */
	lcdc_cfg_vertical_sync(lcdc);
	lcdc_cfg_horizontal_sync(lcdc);
	lcdc_cfg_frame_buffer(lcdc);	
	lcdc_cfg_dma(lcdc);
}

static __inline void lcdc_hw_reset(struct lcdc *lcdc) 
{
	lcdc->regs->RASTER_CTRL  = 0;

	/* take module out of reset */
	lcdc->regs->CLKC_RESET = 0xC;
	udelay(5);
	lcdc->regs->CLKC_RESET = 0x0;
	
}
static __inline void lcdc_enable(struct lcdc *lcdc) {
	lcdc_hw_reset(lcdc);
	lcdc_setup_regs(lcdc);	
	lcdc_blit	(lcdc, lcdc->load_mode);
}

static __inline void lcdc_disable_async(struct lcdc *lcdc) {

	lcdc_disable_raster(lcdc); 

   	lcdc->regs->LCDDMA_CTRL = 0x0;
	lcdc->regs->IRQENABLE_SET = 0x0;
}

static void lcdc_reset(struct lcdc *lcdc, UINT32 status) {
	lcdc_disable_async(lcdc);
	lcdc->reset_count++;

	if (lcdc->reset_count == 1) {
		ERRORMSG(1,(TEXT( "resetting (status %x,reset count %lu)\r\n"),
			              status, lcdc->reset_count));
		lcdc_dumpregs(lcdc);
	}

	if (lcdc->reset_count < 100) {
		lcdc_enable_raster(lcdc);
	} else {
		lcdc->reset_count = 0;
		ERRORMSG(1,(TEXT("too many reset attempts, giving up.\r\n")));
		lcdc_dumpregs(lcdc);
	}
}

int lcdc_change_mode(struct lcdc* lcdc, int color_mode) {

	switch (color_mode) {
		case DISPC_PIXELFORMAT_BITMAP1:
			lcdc->palette_code = 0x0000;
			lcdc->palette_size = 32;
			break;
		case DISPC_PIXELFORMAT_BITMAP2:
			lcdc->palette_code = 0x1000;
			lcdc->palette_size = 32;
			break;
		case DISPC_PIXELFORMAT_BITMAP4:
			lcdc->palette_code = 0x2000;
			lcdc->palette_size = 32;
			break;
		case DISPC_PIXELFORMAT_BITMAP8:
			lcdc->palette_code = 0x3000;
			lcdc->palette_size = 512;
			break;
		case DISPC_PIXELFORMAT_RGB16:
			lcdc->palette_code = 0x4000;
			lcdc->palette_size = 0;
			break;
		case DISPC_PIXELFORMAT_RGB12:
			lcdc->palette_code = 0x4000;
			lcdc->palette_size = 32;
			break;
		case DISPC_PIXELFORMAT_RGB24:
		case DISPC_PIXELFORMAT_RGB32:
		case DISPC_PIXELFORMAT_ARGB32:
		case DISPC_PIXELFORMAT_RGBA32:
		case DISPC_PIXELFORMAT_ARGB16:
			lcdc->palette_code = 0x4000;
			lcdc->palette_size = 0;                 
			break;
		default:
			ERRORMSG(1,(TEXT("invalid color mode %d\n"),color_mode));
			return -1;
	}

	if (lcdc->update_mode == FB_AUTO_UPDATE) {
		lcdc_disable_raster(lcdc);
		lcdc_setup_regs(lcdc);
		lcdc_blit(lcdc, LCDC_LOAD_FRAME);
		lcdc_enable_raster(lcdc);
	}
	return 0;
}

static void lcdc_load_palette(struct lcdc* lcdc)
{
	UINT16	*palette;

	if (!lcdc->palette_size) return;

	palette = (UINT16 *)lcdc->palette_virt;

	*(UINT16 *)palette &= 0x0fff;
	*(UINT16 *)palette |= lcdc->palette_code;

	lcdc->load_mode = LCDC_LOAD_PALETTE;
	lcdc_blit(lcdc,LCDC_LOAD_PALETTE);
#ifdef DEVICE
	if (WaitForSingleObject(lcdc->palette_load_complete, 500) != WAIT_OBJECT_0)
		ERRORMSG(1,(TEXT("lcdc_load_palette: timeout waiting for palette load\r\n")));
#endif	
}

int lcdc_setcolreg(struct lcdc* lcdc, UINT16 regno, 
		UINT8 red, UINT8 green, UINT8 blue, int update_hw_pal) {
	UINT16 *palette;

	if (!lcdc->palette_size)
		return 0;

	if (regno > 255)
		return -1;

	palette = (UINT16 *)lcdc->palette_virt;

      /* TAO, handles 24 bit only ? */
	palette[regno] &= ~0x0fff;
	palette[regno] |= ((red >> 4) << 8) | ((green >> 4) << 4 ) |
			   (blue >> 4);

	if (update_hw_pal) {
		lcdc_disable_raster(lcdc);
		/* at this moment, is dma/display etc configured properly? */
		lcdc_load_palette(lcdc);
		lcdc->load_mode = LCDC_LOAD_FRAME;
		lcdc_blit(lcdc, LCDC_LOAD_FRAME);
	}

	return 0;
}

int lcdc_set_update_mode(struct lcdc* lcdc,
		enum fb_update_mode mode)
{
	if (mode != lcdc->update_mode) {
		switch (mode) {
		case FB_AUTO_UPDATE:
			lcdc_setup_regs(lcdc);
			lcdc_load_palette(lcdc);
			lcdc->load_mode = LCDC_LOAD_FRAME;
			lcdc_blit(lcdc, LCDC_LOAD_FRAME);
			lcdc->update_mode = mode;
			break;
		case FB_UPDATE_DISABLED:
			lcdc_disable_async(lcdc);
			lcdc->update_mode = mode;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

enum fb_update_mode lcdc_get_update_mode(struct lcdc* lcdc) {
	return lcdc->update_mode;
}

#if 0 
void lcdc_suspend(struct lcdc* lcdc) {
	if (lcdc->update_mode == FB_AUTO_UPDATE)
		lcdc_disable(lcdc);

	EnableDeviceClocks(AM_DEVICE_LCDC, FALSE);

}

void lcdc_resume(struct lcdc* lcdc) {
	
	EnableDeviceClocks(AM_DEVICE_LCDC, TRUE);
	lcdc_hw_reset(lcdc);
	if (lcdc->update_mode == FB_AUTO_UPDATE) {
		lcdc_setup_regs(lcdc);
		lcdc_load_palette(lcdc);
		lcdc->load_mode = LCDC_LOAD_FRAME;
		lcdc_blit(lcdc, LCDC_LOAD_FRAME);
	}
}
#endif

//------------------------------------------------------------------------------
BOOL
LcdPdd_LCD_GetMode(
    DWORD   *pPixelFormat,
    DWORD   *pWidth,
    DWORD   *pHeight,
    DWORD   *pPixelClock
    )
{
    struct display_panel *pPanel;

    pPanel = get_panel();

    if( pPixelFormat )
        *pPixelFormat = pPanel->pixel_format;
	
    if( pWidth )
        *pWidth = pPanel->x_res;

    if( pHeight )
        *pHeight = pPanel->y_res;

    if(pPixelClock)
        *pPixelClock = pPanel->pixel_clock;
		
    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
LcdPdd_DVI_Enabled(void)
{
    return bDVIEnabled;
}

//------------------------------------------------------------------------------
BOOL
LcdPdd_LCD_Initialize(struct lcdc *lcdc)
{
    DWORD dwLength;
	PHYSICAL_ADDRESS pa = {0, 0};
	DWORD id = AM33X_BOARDID_BBONE_BOARD;

    lcdc->panel = get_panel();

    lcdc->fb_pa =   IMAGE_WINCE_DISPLAY_PA;
    lcdc->fb_size = IMAGE_WINCE_DISPLAY_SIZE;
	lcdc->palette_phys = IMAGE_WINCE_DISPLAY_PALETTE_PA;

    // Compute the size
    dwLength = lcdc_PixelFormatToPixelSize(lcdc->panel->pixel_format) * 
                      lcdc->panel->x_res *  
                      lcdc->panel->y_res;
	
	// set palette adresses
	pa.LowPart = lcdc->palette_phys;

	lcdc->palette_virt = MmMapIoSpace(pa, MAX_PALETTE_SIZE, FALSE);
	if (!lcdc->palette_virt) {
		ERRORMSG(1, (L"LcdPdd_LCD_Initialize: Cannot map palette\r\n" ));
		goto cleanUp;
	}
	memset(lcdc->palette_virt, 0, MAX_PALETTE_SIZE);
    lcdc->regs->RASTER_CTRL = 0;
    lcdc_hw_reset(lcdc);
    lcdc->regs->LCDDMA_CTRL = 0;
    lcdc->regs->LCDDMA_FB0_BASE = lcdc->fb_pa;
    lcdc->regs->LCDDMA_FB0_CEILING = lcdc->fb_pa + dwLength -1; 
    
    /* disable interrupt */
    lcdc->regs->IRQENABLE_CLEAR = 0x3FF;

    lcdc->clk *= 1000000;  /* change from MHz to Hz */
    lcdc->dma_burst_sz = 16;	
    lcdc_setup_regs(lcdc);
    lcdc_cfg_dma(lcdc);	

#ifdef DEVICE
    KernelIoControl(IOCTL_HAL_GET_PLATFORM_ID,
                         NULL, 0, &id, sizeof(DWORD), &dwLength);
#else
	id = g_dwBoardId;
#endif

	if (id == AM33X_BOARDID_BBONEBLACK_BOARD && lcdc->panel->IsDVI)
	{
		tda1998x_init();
		if( tda998x_connected_detect())
		{
			RETAILMSG(1, (L"HDMI Connected\r\n"));
		}
		else
			RETAILMSG(1, (L"HDMI Disconnected\r\n"));

		tda998x_encoder_mode_set(get_drm_mode());
		tda998x_encoder_dpms(TRUE);
	}

	if (lcdc->panel->init != NULL)
	{
		(*lcdc->panel->init)();
	}

	if (lcdc->panel->enable != NULL)
	{
		(*lcdc->panel->enable)(TRUE);
	}

cleanUp:
    return TRUE;
}

//------------------------------------------------------------------------------
BOOL
LcdPdd_GetMemory(
    DWORD   *pVideoMemLen,
    DWORD   *pVideoMemAddr
    )
{
    //  Return video memory parameters
    if( pVideoMemLen )
        *pVideoMemLen = IMAGE_WINCE_DISPLAY_SIZE;

    if( pVideoMemAddr )
        *pVideoMemAddr = IMAGE_WINCE_DISPLAY_PA;

    return TRUE;
}

void LcdPdd_Handle_LostofSync_int(struct lcdc *lcdc)
{
    DWORD status = 0;

    status = lcdc->regs->IRQSTATUS;

    lcdc_disable_raster(lcdc);
    /* clear interrupt*/
    lcdc->regs->IRQSTATUS = status; 
    lcdc_enable_raster(lcdc);

}

void LcdPdd_Handle_EndofPalette_int(struct lcdc *lcdc)
{
    DWORD status = 0;

    status = lcdc->regs->IRQSTATUS;

    /* Must disable raster before changing state of any control bit.
    * And also must be disabled before clearing the PL loading
    * interrupt via the following write to the status register. If
    * this is done after then one gets multiple PL done interrupts.
    */
    
    lcdc_disable_raster(lcdc);
    
    /* clear interrupt*/
    lcdc->regs->IRQSTATUS = status; 
    
    /* Disable PL completion inerrupt */
    lcdc->regs->IRQENABLE_CLEAR |= LCDC_IRQENABLE_PALETTE_INT;
    
    /* Setup and start data loading mode */
    lcdc_blit(lcdc, LCDC_LOAD_FRAME);

}

//------------------------------------------------------------------------------
BOOL
LcdPdd_SetPowerLevel(
    struct lcdc *lcdc,
    DWORD   dwPowerLevel
    )
{
    // Power display up/down
    switch(dwPowerLevel)
    {
        case D0:
        case D1:
        case D2:
			lcdc_enable_raster(lcdc);
			if (lcdc->panel != NULL && lcdc->panel->enable != NULL)
			{
				(*lcdc->panel->enable)(TRUE);
			}
			break;        
        case D3:
        case D4:
			lcdc_disable_raster(lcdc);
			if (lcdc->panel != NULL && lcdc->panel->enable != NULL)
			{
				(*lcdc->panel->enable)(FALSE);
			}
            break;
    }
    return TRUE;
}   

//------------------------------------------------------------------------------
DWORD
LcdPdd_Get_PixClkDiv(void)
{
    return 0;
}

