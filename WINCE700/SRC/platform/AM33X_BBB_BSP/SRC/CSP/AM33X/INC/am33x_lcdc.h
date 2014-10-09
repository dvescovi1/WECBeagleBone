//------------------------------------------------------------------------------
//
//  Header:  am33x_lcdc.h
//
//  Provides definitions for LCDC module based on Am33x.
//
//------------------------------------------------------------------------------
#ifndef __AM33X_LCDC_H
#define __AM33X_LCDC_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// INCLUDE FILES  
//------------------------------------------------------------------------------
#include "omap_types.h"

//------------------------------------------------------------------------------
// GENERAL MODULE CONSTANTS
//------------------------------------------------------------------------------
#define LCDC_PCD_VALUE(refclk, pixelclk)    \
    ( ((refclk) % (pixelclk) == 0)? (((refclk) / (pixelclk)) - 1) : ((refclk) / (pixelclk)) )
#define LCDC_PIXEL_SIZE_BYTES(bpp)    \
    ( ((bpp) < 8)? 1 : ( ((bpp) > 16)? 4 : ( (bpp) >> 3 ) ))
#define LCDC_GW_SIZE_X(x)               (x >> 4)
#define LCDC_GW_SIZE_STRIDE(x)          (x >> 2)
#define LCDC_GW_TRANSPARENCY(x)         (0xFF - x)
	

//Escape codes
#define LCDC_ESC_REQUEST_WINDOW      0x10001 // Request hardware resources
#define LCDC_ESC_RELEASE_WINDOW      0x10002 // Release hardware resources
#define LCDC_ESC_ENABLE_WINDOW       0x10003 // Configure and enable graphic window
#define LCDC_ESC_DISABLE_WINDOW      0x10004 // Disable graphic window
#define LCDC_ESC_FLIP_WINDOW         0x10005 // Vertical flip graphic window
#define LCDC_ESC_GET_TRANSPARENCY    0x10006 // Get current transparency
#define LCDC_ESC_SET_TRANSPARENCY    0x10007 // Set transparency of the window

//------------------------------------------------------------------------------
// REGISTER LAYOUT
//------------------------------------------------------------------------------
typedef struct {
	REG32 PID;
	REG32 CTRL;
	REG32 NOTUSED1;
    REG32 LIDD_CTRL;
    REG32 LIDD_CS0_CONF;
    REG32 LIDD_CS0_ADDR;
    REG32 LIDD_CS0_DATA;
    REG32 LIDD_CS1_CONF;
    REG32 LIDD_CS1_ADDR;
    REG32 LIDD_CS1_DATA;
    REG32 RASTER_CTRL;
    REG32 RASTER_TIMING_0;
    REG32 RASTER_TIMING_1;
    REG32 RASTER_TIMING_2;
    REG32 RASTER_SUBPANEL;
    REG32 RASTER_SUBPANEL2;
    REG32 LCDDMA_CTRL;
    REG32 LCDDMA_FB0_BASE;
    REG32 LCDDMA_FB0_CEILING;
    REG32 LCDDMA_FB1_BASE;
    REG32 LCDDMA_FB1_CEILING;
    REG32 SYSCONFIG;
    REG32 IRQSTATUS_RAW;
    REG32 IRQSTATUS;
    REG32 IRQENABLE_SET;
    REG32 IRQENABLE_CLEAR;
	REG32 NOTUSED2;
    REG32 CLKC_ENABLE;
    REG32 CLKC_RESET;
} AM33X_LCDC_REGS;

#define LCDC_REGS AM33X_LCDC_REGS

// Register definitions
// CTRL register
#define LCDC_CTRL_MODESEL_RASTER			(1 << 0)
#define LCDC_CTRL_AUTO_UF_RESTART			(1 << 1)
#define LCDC_CTRL_CLKDIV(x)					((x) << 8)
#define LCDC_CTRL_CLKDIV_MASK				(0xff << 8)


// RASTER_CTRL register
#define LCDC_RASTER_CTRL_LCD_EN				(1 << 0)
#define LCDC_RASTER_CTRL_MONOCHROME_MODE	(1 << 1)
#define LCDC_RASTER_CTRL_LCD_TFT			(1 << 7)
#define LCDC_RASTER_CTRL_RASTER_ORDER		(1 << 8)
#define LCDC_RASTER_CTRL_MONO_8BIT_MODE		(1 << 9)
#define LCDC_RASTER_CTRL_LINE_IRQ_CLR_SEL	(1 << 10)
#define LCDC_RASTER_CTRL_PALMODE(x)			((x) << 20)
#define LCDC_RASTER_CTRL_TFT_ALT_ENABLE		(1 << 23)
#define LCDC_RASTER_CTRL_LCD_STN_565		(1 << 24)
#define LCDC_RASTER_CTRL_LCD_TFT_24			(1 << 25)
#define LCDC_RASTER_CTRL_TFT_24BPP_UNPACK	(1 << 26)
#define LCDC_RASTER_CTRL_REQDLY(x)			((x) << 12)
#define LCDC_RASTER_CTRL_REQDLY_MASK		(0xff << 12)

// RASTER_TIMING_2 register
#define LCDC_RASTER_RASTER_TIMING_2


// LCD DMA Control Register
#define LCDC_DMA_CTRL_DUAL_FRAME_BUFFER_EN	(1 << 0)
#define LCDC_DMA_CTRL_BURST_SIZE(x)			((x) << 4)
#define LCDC_DMA_CTRL_BURST_1				0x0
#define LCDC_DMA_CTRL_BURST_2				0x1
#define LCDC_DMA_CTRL_BURST_4				0x2
#define LCDC_DMA_CTRL_BURST_8				0x3
#define LCDC_DMA_CTRL_BURST_16				0x4
#define LCDC_DMA_CTRL_BURST_SIZE_MASK		(0x7 << 4)
#define LCDC_DMA_CTRL_TH_FIFO_READY(x)		((x) << 8)
#define LCDC_DMA_CTRL_TH_FIFO_READY_8		0x0
#define LCDC_DMA_CTRL_TH_FIFO_READY_16		0x1
#define LCDC_DMA_CTRL_TH_FIFO_READY_32		0x2
#define LCDC_DMA_CTRL_TH_FIFO_READY_64		0x3
#define LCDC_DMA_CTRL_TH_FIFO_READY_128		0x4
#define LCDC_DMA_CTRL_TH_FIFO_READY_256		0x5
#define LCDC_DMA_CTRL_TH_FIFO_READY_512		0x6
#define LCDC_DMA_CTRL_TH_FIFO_READY_MASK	(0x7 << 8)

// IRQ Status register
#define LCDC_IRQSTATUS_DONE					(1 << 0)
#define LCDC_IRQSTATUS_RASTER_DONE			(1 << 1)
#define LCDC_IRQSTATUS_SYNC_LOST			(1 << 2)
#define LCDC_IRQSTATUS_AC_BIAS				(1 << 3)
#define LCDC_IRQSTATUS_FUF					(1 << 5)
#define LCDC_IRQSTATUS_PALETTE_LOADED		(1 << 6)
#define LCDC_IRQSTATUS_FRAME_DONE0			(1 << 8)
#define LCDC_IRQSTATUS_FRAME_DONE1			(1 << 9)

// LCD IRQ Enable Set/Clear Register
#define LCDC_IRQENABLE_DONE_INT				(1 << 0)
#define LCDC_IRQENABLE_RASTER_DONE_INT		(1 << 1)
#define LCDC_IRQENABLE_UNDERFLOW_INT		(1 << 5)
#define LCDC_IRQENABLE_PALETTE_INT			(1 << 6)
#define LCDC_IRQENABLE_END_OF_FRAME0_INT	(1 << 8)
#define LCDC_IRQENABLE_END_OF_FRAME1_INT	(1 << 9)

// CLKC_ENABLE register
#define LCDC_CLKC_ENABLE_CORE				(1 << 0)
#define LCDC_CLKC_ENABLE_LIDD				(1 << 1)
#define LCDC_CLKC_ENABLE_DMA				(1 << 2)


#define LCDC_AC_BIAS_TRANSITIONS_PER_INT(x)	((x) << 16)
#define LCDC_AC_BIAS_FREQUENCY(x)			((x) << 8)


#ifdef __cplusplus
}
#endif

#endif // __AM33X_LCDC_H
