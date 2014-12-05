/*
 *   Copyright (c) Texas Instruments Incorporated 2009. All Rights Reserved.
 *
 *   Use of this software is controlled by the terms and conditions found 
 *   in the license agreement under which this software has been supplied.
 * 
 *   lcdc-timing.c
 */
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include "lcdc.h"
#include "sdk_gpio.h"
#include "gpio.h"
#include "..\tda1998x\tda.h"

static HANDLE g_hGpio = NULL;

struct display_panel *pActivePanel = NULL;

struct display_panel panel_init = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	0,	// x_res 
	0,	// y_res
	0,	// pixel_clock
	0,	// hsw 
	0,	// hfp
	0,	// hbp
	0,	// vsw
	0,	// vfb
	0,	// vbp
	0,	// acb
	0,	// mono_8bit_mode
	0,	// tft_alt_mode
	COLOR_ACTIVE,	// panel_shade
	OMAP_RES_DEFAULT, // disp_res
	FALSE, // IsDVI
	NULL, // init function
	NULL  // enable/disable function
};


int LCDC_panel_320x240_init(void) 
{
	return 0;
}

int LCDC_panel_320x240_en(BOOL onoff) 
{
	return 0;
}

struct display_panel panel_320x240 = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	320,		// x_res 
	240,		// y_res
	40000000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_320W_240H,
	FALSE,
	LCDC_panel_320x240_init, // init
	LCDC_panel_320x240_en    // enable/disable
};


int LCDC_panel_480x272_init(void) 
{
    if(!g_hGpio)
    {
		g_hGpio = GPIOOpen();
 	
		if (g_hGpio == NULL)
		{
			RETAILMSG(1, (L"LCDC_panel_480x272_init:Failed to open GPIO driver\r\n"));
			return FALSE;
		}
        GPIOClrBit(g_hGpio, GPIO1_18);
        GPIOClrBit(g_hGpio, GPIO3_19);
        GPIOSetMode(g_hGpio, GPIO1_18, GPIO_DIR_OUTPUT );	// backlight enable
        GPIOSetMode(g_hGpio, GPIO3_19, GPIO_DIR_OUTPUT );	// LCD enable
    }
	return TRUE;
}

int LCDC_panel_480x272_en(BOOL onoff) 
{
	if (onoff)
	{
        GPIOSetBit(g_hGpio, GPIO1_18);
        GPIOSetBit(g_hGpio, GPIO3_19);
	}
	else
	{
        GPIOClrBit(g_hGpio, GPIO1_18);
        GPIOClrBit(g_hGpio, GPIO3_19);
	}
	return 0;
}

struct display_panel panel_480x272 = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	480,		// x_res 
	272,		// y_res
	9800000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_480W_272H,
	FALSE,
	LCDC_panel_480x272_init, // init
	LCDC_panel_480x272_en    // enable/disable
};


int LCDC_panel_640x480_init(void) 
{
	return 0;
}

int LCDC_panel_640x480_en(BOOL onoff) 
{
	return 0;
}

struct display_panel panel_640x480 = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	640,		// x_res 
	480,		// y_res
	40000000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb	
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_640W_480H,
	FALSE,
	LCDC_panel_640x480_init, // init
	LCDC_panel_640x480_en    // enable/disable
};


int LCDC_panel_800x480_init(void) 
{
	return 0;
}

int LCDC_panel_800x480_en(BOOL onoff) 
{
	return 0;
}

struct display_panel panel_800x480 = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	800,		// x_res 
	480,		// y_res
	40000000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_800W_480H,
	FALSE,
	LCDC_panel_800x480_init, // init
	LCDC_panel_800x480_en    // enable/disable
};


int LCDC_panel_800x4804D_init(void) 
{
    if(!g_hGpio)
    {
		g_hGpio = GPIOOpen();
 	
		if (g_hGpio == NULL)
		{
			RETAILMSG(1, (L"LCDC_panel_800x4804D_init:Failed to open GPIO driver\r\n"));
			return FALSE;
		}
        GPIOClrBit(g_hGpio, GPIO1_18);
        GPIOClrBit(g_hGpio, GPIO0_2);
        GPIOSetMode(g_hGpio, GPIO1_18, GPIO_DIR_OUTPUT );	// backlight enable
        GPIOSetMode(g_hGpio, GPIO0_2, GPIO_DIR_OUTPUT );	// LCD enable
    }
	return TRUE;
}

int LCDC_panel_800x4804D_en(BOOL onoff) 
{
	if (onoff)
	{
        GPIOSetBit(g_hGpio, GPIO1_18);
        GPIOSetBit(g_hGpio, GPIO0_2);
	}
	else
	{
        GPIOClrBit(g_hGpio, GPIO1_18);
        GPIOClrBit(g_hGpio, GPIO0_2);
	}
	return 0;
}

struct display_panel panel_800x4804D = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	800,		// x_res 
	480,		// y_res
	40000000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_800W_480H4D,
	FALSE,
	LCDC_panel_800x4804D_init, // init
	LCDC_panel_800x4804D_en    // enable/disable
};


int LCDC_panel_800x600_init(void) 
{
    if(!g_hGpio)
    {
		g_hGpio = GPIOOpen();
 	
		if (g_hGpio == NULL)
		{
			RETAILMSG(1, (L"LCDC_panel_800x600_init:Failed to open GPIO driver\r\n"));
			return FALSE;
		}
        GPIOSetBit(g_hGpio, GPIO0_22);
        GPIOSetMode(g_hGpio, GPIO0_22, GPIO_DIR_OUTPUT );	// Backlight PWM
    }
	return TRUE;
}

int LCDC_panel_800x600_en(BOOL onoff) 
{
	return 0;
}

struct display_panel panel_800x600 = {
	LCDC_PANEL_TFT | LCDC_INV_VSYNC | LCDC_INV_HSYNC | LCDC_HSVS_CONTROL,	// config
	DISPC_PIXELFORMAT_RGB16,	// bpp
	800,		// x_res 
	600,		// y_res
	40000000,	// pixel_clock
	47,			// hsw 
	39,			// hfp
	39,			// hbp
	2,			// vsw
	13,			// vfb
	29,			// vbp
	0,			// acb
	0,               // mono_8bit_mode
	0,               // tft_alt_mode
	COLOR_ACTIVE,    //panel_shade
	OMAP_LCD_800W_600H,
	FALSE,
	LCDC_panel_800x600_init, // init
	LCDC_panel_800x600_en    // enable/disable
};




struct drm_display_mode drm_800x600 = {
	40000, 
	800, 
	840,
	968, 
	1056, 
	0x80,		// HSKEW = hsync_end-hsync_start fixup 
	600, 
	601, 
	605, 
	628, 
	0,
	(DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC | DRM_MODE_FLAG_HSKEW)		
};

struct drm_display_mode drm_1024x768 = {
	65000, 
	1024, 
	1048,
	1184, 
	1344, 
	0x86,		// HSKEW = hsync_end-hsync_start fixup 
	768, 
	771, 
	777, 
	806, 
	0,
	(DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_HSKEW)		
};

struct drm_display_mode drm_1280x720 = {
	74250, 
	1280, 
	1390,
	1430, 
	1650, 
	0x28,		// HSKEW = hsync_end-hsync_start fixup 
	720, 
	725, 
	730, 
	750, 
	0,
	(DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC | DRM_MODE_FLAG_HSKEW)		
};


//------------------------------------------------------------------------------
//
//  Function:   dump_panel
//
//
static void dump_panel()
{
	RETAILMSG(1, (L"LCDC_panel timing\r\n"));
	RETAILMSG(1, (L"x_res = %d\r\n",panel_init.x_res));
	RETAILMSG(1, (L"y_res = %d\r\n",panel_init.y_res));
	RETAILMSG(1, (L"pixel_clock = %d\r\n",panel_init.pixel_clock));
	RETAILMSG(1, (L"hsw = %d\r\n",panel_init.hsw));
	RETAILMSG(1, (L"hfp = %d\r\n",panel_init.hfp));
	RETAILMSG(1, (L"hbp = %d\r\n",panel_init.hbp));
	RETAILMSG(1, (L"vsw = %d\r\n",panel_init.vsw));
	RETAILMSG(1, (L"vfp = %d\r\n",panel_init.vfp));
	RETAILMSG(1, (L"vbp = %d\r\n",panel_init.vbp));
}


//------------------------------------------------------------------------------
//
//  Function:   select_panel
//
//  Initialize the pActivePanel pointer to point to
//	the panel structure containing timings for the selected 
//	display
//
static BOOL select_panel()
{
enum OMAP_LCD_DVI_RES dispRes = OMAP_RES_DEFAULT;

	GetDisplayResolutionFromBootArgs(&dispRes);

	switch (dispRes) {
		case OMAP_LCD_320W_240H:
			pActivePanel = &panel_320x240;
			break;
		case OMAP_LCD_480W_272H:
			pActivePanel = &panel_480x272;
			break;
		case OMAP_LCD_640W_480H:
			pActivePanel = &panel_640x480;
			break;
		case OMAP_LCD_800W_480H:
			pActivePanel = &panel_800x480;
			break;
		case OMAP_LCD_800W_480H4D:
			pActivePanel = &panel_800x4804D;
			break;
		case OMAP_LCD_800W_600H:
			pActivePanel = &panel_800x600;
			break;
		case OMAP_DVI_800W_600H:
			tda998x_drm_to_panel(&drm_800x600, &panel_init);
			panel_init.dispRes = dispRes;
			panel_init.IsDVI = TRUE;
			pActivePanel = &panel_init;
			break;
		case OMAP_DVI_1024W_768H:
			tda998x_drm_to_panel(&drm_1024x768, &panel_init);
			panel_init.dispRes = dispRes;
			panel_init.IsDVI = TRUE;
			pActivePanel = &panel_init;
			break;
		case OMAP_DVI_1280W_720H:
			tda998x_drm_to_panel(&drm_1280x720, &panel_init);
			panel_init.dispRes = dispRes;
			panel_init.IsDVI = TRUE;
			pActivePanel = &panel_init;
			break;
		case OMAP_RES_DEFAULT:
			pActivePanel = &panel_init;
			break;
		default:
			pActivePanel = &panel_init;
			break;
	}
	return TRUE;
}


struct display_panel *get_panel(void)
{
	select_panel();
//	dump_panel();
	return (pActivePanel);
}

struct drm_display_mode *get_drm_mode(void)
{
	struct drm_display_mode *mode = NULL;

	if (NULL == pActivePanel)
		select_panel();

	switch (pActivePanel->dispRes) {
	case OMAP_DVI_800W_600H:
		mode = &drm_800x600;
		break;
	case OMAP_DVI_1024W_768H:
		mode = &drm_1024x768;
		break;
	case OMAP_DVI_1280W_720H:
		mode = &drm_1280x720;
		break;
	}
	return mode;
}



