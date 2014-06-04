/*
 *   Copyright (c) Texas Instruments Incorporated 2009. All Rights Reserved.
 *
 *   Use of this software is controlled by the terms and conditions found 
 *   in the license agreement under which this software has been supplied.
 * 
 *   l137-lcdc.h
 */

#ifndef __LCDC_H__
#define __LCDC_H__

#include "am33x_lcdc.h"
#include "bsp_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define		MAX_PALETTE_SIZE		0x00001000

//DISPC_PIXELFORMAT (pixel formats for LCDC)

#define DISPC_PIXELFORMAT_BITMAP1               (0x0)
#define DISPC_PIXELFORMAT_BITMAP2               (0x1)
#define DISPC_PIXELFORMAT_BITMAP4               (0x2)
#define DISPC_PIXELFORMAT_BITMAP8               (0x3)
#define DISPC_PIXELFORMAT_RGB12                 (0x4)
#define DISPC_PIXELFORMAT_ARGB16                (0x5)
#define DISPC_PIXELFORMAT_RGB16                 (0x6)
#define DISPC_PIXELFORMAT_RGB32                 (0x8)
#define DISPC_PIXELFORMAT_RGB24                 (0x9)
#define DISPC_PIXELFORMAT_YUV2                  (0xA)
#define DISPC_PIXELFORMAT_UYVY                  (0xB)
#define DISPC_PIXELFORMAT_ARGB32                (0xC)
#define DISPC_PIXELFORMAT_RGBA32                (0xD)
#define DISPC_PIXELFORMAT_RGBx32                (0xE)

enum fb_update_mode {
	FB_UPDATE_DISABLED = 0,
	FB_AUTO_UPDATE,
};

enum panel_shade {
	MONOCHROME = 0,
	COLOR_ACTIVE,
	COLOR_PASSIVE,
};

// timing signal polarity etc. ..used in RASTER_TIMING_2
#define LCDC_INV_VSYNC			0x0001  //bit 20
#define LCDC_INV_HSYNC			0x0002  //bit 21
#define LCDC_INV_PIX_CLOCK		0x0004  //bit 22
#define LCDC_INV_OUTPUT_EN		0x0008  //bit 23
#define LCDC_HSVS_FALLING_EDGE	0x0010  //bit 24
#define LCDC_HSVS_CONTROL		0x0020  //bit 25
#define LCDC_SIGNAL_MASK		0x003f 

#define LCDC_PANEL_TFT			0x0100	//bit 


#define res_size(_r) (((_r)->end - (_r)->start) + 1)

struct display_panel {
	int			config;			/* TFT/STN, signal inversion */
	int			pixel_format;	/* Pixel format in fb mem */
	int			x_res, y_res;
	int			pixel_clock;	/* In kHz */
	int			hsw;			/* Horizontal synchronization pulse width */
	int			hfp;			/* Horizontal front porch */
	int			hbp;			/* Horizontal back porch */
	int			vsw;			/* Vertical synchronization	pulse width */
	int			vfp;			/* Vertical front porch */
	int			vbp;			/* Vertical back porch */
	int			acb;			/* ac-bias pin frequency */
	int			mono_8bit_mode;
	int         tft_alt_mode;
	enum panel_shade	panel_shade;   
	enum OMAP_LCD_DVI_RES	dispRes;
	BOOL		IsDVI;
	int			(*init)		(void);
};

enum lcdc_load_mode {
	LCDC_LOAD_PALETTE_AND_FRAME = 0,
	LCDC_LOAD_PALETTE,
	LCDC_LOAD_FRAME,
};


struct lcdc {
	LCDC_REGS			*regs;     // virtual address of regs
	UINT32				phys_base; // phy address of regs
	UINT32				fb_pa;     // phy address of fb
	UINT32				fb_size;   // total siz of fb
	void				*fb_va;    // virtual address of fb
	UINT32				fb_cur_win_size; // size of the FB window for current mode

	struct display_panel    *panel;
	UINT32				clk;
	void				*palette_virt;
	UINT32				palette_phys;
	int					palette_code;
	int					palette_size;
	UINT32				irq_mask;
	UINT32				reset_count;
	enum fb_update_mode	update_mode;
	enum lcdc_load_mode	load_mode;

	HANDLE				last_frame_complete;
	HANDLE				palette_load_complete;
	HANDLE				lcd_int_event;
	UINT32				lcdc_irq;
	DWORD				sys_interrupt;
	HANDLE				lcd_int_thread;

    int 				dma_burst_sz;
    int					dma_fifo_thresh;

	int 				currentBLLevel;
	int 				backlightOn;

	enum fb_pack_format pack_format;
	
	int					color_mode; //TODO ?????????????? move out of here
};

int lcdc_change_mode(struct lcdc* lcdc, int color_mode);

int lcdc_setcolreg(struct lcdc* lcdc, UINT16 regno, 
		UINT8 red, UINT8 green, UINT8 blue, int update_hw_pal);

int lcdc_set_update_mode(struct lcdc* lcdc,
		enum fb_update_mode mode);

enum fb_update_mode lcdc_get_update_mode(struct lcdc* lcdc);

struct display_panel *get_panel(void);

BOOL LcdPdd_LCD_GetMode(
    DWORD   *pPixelFormat,
    DWORD   *pWidth,
    DWORD   *pHeight,
    DWORD   *pPixelClock
    );

DWORD lcdc_PixelFormatToPixelSize( DWORD  ePixelFormat);
BOOL LcdPdd_LCD_Initialize(struct lcdc *lcdc);
BOOL LcdPdd_GetMemory(
    DWORD   *pVideoMemLen,
    DWORD   *pVideoMemAddr
    );
void LcdPdd_Handle_LostofSync_int(struct lcdc *lcdc);
void LcdPdd_Handle_EndofPalette_int(struct lcdc *lcdc);
BOOL LcdPdd_SetPowerLevel(
    struct lcdc *lcdc,
    DWORD   dwPowerLevel
    );

VOID LcdPdd_EnableLCDC(
    struct lcdc *lcdc,
    BOOL bEnable
    );

DWORD LcdPdd_Get_PixClkDiv(void);

BOOL SetBacklightLevel_GPIO(UINT level);

#ifdef __cplusplus
}
#endif

#endif
