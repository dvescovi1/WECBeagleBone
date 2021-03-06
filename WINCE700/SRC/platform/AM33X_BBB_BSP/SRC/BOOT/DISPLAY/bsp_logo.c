// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  bsp_logo.c
//


//------------------------------------------------------------------------------
//
// includes
//

#include "bsp.h"
#include "bsp_logo.h"
#include "oalex.h"
#include "eboot.h"
#include "lcdc.h"
#include "am33x_clocks.h"
#include "am33x_prcm.h"
#include "ceddkex.h"
#include "Image_Cfg.h"
#include <oal_clock.h>

//------------------------------------------------------------------------------
//
// prototypes
//

//------------------------------------------------------------------------------
//
// defines
//

struct lcdc lcdc_device;

static void lcd_init(void)
{
	lcdc_device.regs = (LCDC_REGS*)OALPAtoUA(GetAddressByDevice(AM_DEVICE_LCDC));
	lcdc_device.prcmregs = (AM33X_PRCM_REGS*)OALPAtoUA(AM33X_PRCM_REGS_PA);
	LcdPdd_LCD_Initialize(&lcdc_device);
	lcdc_device.regs->RASTER_CTRL |= LCDC_RASTER_CTRL_LCD_EN;
}

static void lcd_deinit(void)
{
	if(lcdc_device.regs)
	{
		lcdc_device.regs->RASTER_CTRL = 0;
		lcdc_device.regs->LCDDMA_CTRL = 0;
		lcdc_device.regs->LCDDMA_FB0_BASE = 0;
		lcdc_device.regs->LCDDMA_FB0_CEILING = 0;

        /* put in reset mode */
		lcdc_device.regs->CLKC_RESET = 0xC;
	}
}

static void FlipFrameBuffer(PUCHAR fb, DWORD h, DWORD lineSize, PUCHAR temporaryBuffer)
{
    DWORD y;
    PUCHAR top;
    PUCHAR bottom;

    top = fb;
    bottom = fb + ((h-1)*lineSize);
    
    for (y=0;y<h/2;y++)
    {
        memcpy(temporaryBuffer,top,lineSize);
        memcpy(top,bottom,lineSize);
        memcpy(bottom,temporaryBuffer,lineSize);
        top += lineSize;
        bottom -= lineSize;
    }
}

//------------------------------------------------------------------------------
//
//  Function:  ShowLogo
//
//  This function shows the logo splash screen
//
VOID ShowLogo(UINT32 flashAddr, UINT32 offset, BOOL invert)
{
    HANDLE  hFlash = NULL;
    PUCHAR  pChar;
    ULONG   x, y;
    WORD    wSignature = 0;
    DWORD   dwOffset = 0;
    DWORD framebuffer = 0;
    DWORD framebufferPA = 0;
	DWORD dwPixelFormat = 0;
    DWORD dwLcdWidth = 0;
    DWORD dwLcdHeight = 0;
    DWORD dwLength = 0;

    //  Get the LCD width and height
    LcdPdd_LCD_GetMode( &dwPixelFormat, &dwLcdWidth, &dwLcdHeight, NULL );
    // Compute the size
    dwLength = lcdc_PixelFormatToPixelSize(dwPixelFormat) * dwLcdWidth * dwLcdHeight;

	// Get the frame buffer
    framebufferPA = IMAGE_WINCE_DISPLAY_PA;
    framebuffer = (DWORD) OALPAtoUA(framebufferPA);
	
    pChar = (PUCHAR)framebuffer;
    
    if (flashAddr != -1)
    {
        // Open flash storage
        hFlash = OALFlashStoreOpen(flashAddr);
        if( hFlash != NULL )
        {
            //  The LOGO reserved NAND flash region contains the BMP file
            OALFlashStoreBufferedRead( hFlash, offset, (UCHAR*) &wSignature, sizeof(wSignature), FALSE );

            //  Check for 'BM' signature
            if( wSignature == 0x4D42 )  
            {
                //  Read the offset to the pixel data
                OALFlashStoreBufferedRead( hFlash, offset + 10, (UCHAR*) &dwOffset, sizeof(dwOffset), FALSE );

                //  Read the pixel data with the given offset
                OALFlashStoreBufferedRead( hFlash, offset + dwOffset, pChar, dwLength, FALSE );
            }
           
            //  Close store
            OALFlashStoreClose(hFlash);
                    
            //As BMP are stored upside down, we need to flip the frame buffer's content
			if (!invert)
				FlipFrameBuffer((PUCHAR)framebuffer,dwLcdHeight,dwLcdWidth * lcdc_PixelFormatToPixelSize(dwPixelFormat),(PUCHAR)framebuffer + dwLength);
        }
    }

    //  If bitmap signature is valid, display the logo, otherwise fill screen with pattern
    if( wSignature != 0x4D42 )
    {
        for (y= 0; y < dwLcdHeight; y++)
        {
            for( x = 0; x < dwLcdWidth; x++ )
            {
                if( y < dwLcdHeight/2 )
                {
                    if( x < dwLcdWidth/2 )
                    {
                        *pChar++ = 0x00;    //  Blue/Green
                        *pChar++ = 0x1F;    //  Green/Red
                    }
                    else
                    {
                        *pChar++ = 0x07;    //  Blue/Green
                        *pChar++ = 0xE0;    //  Green/Red
                    }
                }
                else
                {
                    if( x < dwLcdWidth/2 )
                    {
                        *pChar++ = 0xF8;    //  Blue/Green
                        *pChar++ = 0x00;    //  Green/Red
                    }
                    else
                    {
                        *pChar++ = 0x07;    //  Blue/Green
                        *pChar++ = 0xFF;    //  Green/Red
                    }
                }
            }
        }
    }

    // Fire up the LCD
    EnableDeviceClocks(AM_DEVICE_LCDC, TRUE);
	lcd_init();
}

//------------------------------------------------------------------------------
//
//  Function:   ShowSDLogo
//
//  This function is called to display the splaschreen bitmap from the SDCard
//
//
BOOL ShowSDLogo(BOOL invert)
{
    DWORD framebuffer = 0;
    DWORD framebufferPA = 0;
	DWORD dwPixelFormat = 0;
    DWORD dwLcdWidth = 0;
    DWORD dwLcdHeight = 0;
    DWORD dwLength = 0;

	// Get the LCD width and height
    LcdPdd_LCD_GetMode( &dwPixelFormat, &dwLcdWidth, &dwLcdHeight, NULL );
    // Compute the size
    dwLength = lcdc_PixelFormatToPixelSize(dwPixelFormat) * dwLcdWidth * dwLcdHeight;

	// Get the frame buffer
    framebufferPA = IMAGE_WINCE_DISPLAY_PA;
    framebuffer = (DWORD) OALPAtoUA(framebufferPA);

	memset(framebuffer, 0xff, IMAGE_WINCE_DISPLAY_SIZE);	// fill to black

	if (invert)
	{
		if (!BLSDCardReadLogo(L"Logoi.bmp", (UCHAR*)framebuffer, &dwLength))
		{
    		return FALSE;
		}
	}
	else
	{
		if (!BLSDCardReadLogo(L"Logo.bmp", (UCHAR*)framebuffer, &dwLength))
		{
    		return FALSE;
		}
	}

    //As BMP are stored upside down, we need to flip the frame buffer's content
	FlipFrameBuffer((PUCHAR)framebuffer,dwLcdHeight,dwLcdWidth * lcdc_PixelFormatToPixelSize(dwPixelFormat),(PUCHAR)framebuffer + dwLength);

    // Fire up the LCD
    EnableDeviceClocks(AM_DEVICE_LCDC, TRUE);
	lcd_init();

	return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HideLogo
//
//  This function hides the logo splash screen
//
VOID HideLogo(VOID)
{
	lcd_deinit();
    EnableDeviceClocks(AM_DEVICE_LCDC, FALSE);
}

