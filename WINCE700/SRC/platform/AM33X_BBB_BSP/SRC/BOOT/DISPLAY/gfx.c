/*
 *  Copyright MPC Data Limited 2010, All Rights Reserved.
 *
 *  File:		gfx.c
 *  Purpose:	Bootloader Graphics routines
 *
 *  Notes:		
 *
 *  Author:		Christopher Peerman
 *  Date:		
 *
 *  Modifications:
 *  MM/DD/YYYY       Initials		Change description 
 *  
 */
#include <bsp.h>
#include "lcdc.h"

//------------------------------------------------------------------------------
//
//  Function: DrawProgressBar
//
//  Draw a progress bar.
// Percentage is a fixed point 8bit number
//
//
VOID DrawProgressBar(UINT32 total, UINT32 pos, BOOL invert)
{
	static DWORD dwLastPos = (DWORD)-1;
	DWORD dwLcdWidth = 0;
	DWORD dwLcdHeight = 0;
	DWORD framebuffer = 0;

    UINT8 *pPixel;
	UINT32 bar_height, bar_width;
    UINT32 xpos=0, ypos;
    UINT32 xoffset, yoffset;
    UINT32 bar_pos;

	// Read the LCD parameters
	if (LcdPdd_LCD_GetMode( 0, &dwLcdWidth, &dwLcdHeight, NULL ) == FALSE)
		return;

	if (LcdPdd_GetMemory( NULL, &framebuffer ) == FALSE)
		return;

	// Setup the position of the progress bar
	bar_height = 12;
	bar_width = (dwLcdWidth / 8) * 6;

    xoffset = (dwLcdWidth - bar_width) / 2;

	if (invert)
	{
		yoffset = (dwLcdHeight / 8);
		xoffset += bar_width;
	}
	else
	{
	    yoffset = (dwLcdHeight / 8) * 7;
	}


    if (pos < 1)
        bar_pos = 1;
    else if (pos >= total)
        bar_pos = bar_width;
    else 
        bar_pos = ((pos >> 8) * bar_width) / (total >> 8);

	if (dwLastPos == bar_pos)
		return;
	dwLastPos = bar_pos;

	for (ypos = 0; ypos <= bar_height; ypos ++) {
		pPixel = (UINT8 *)(framebuffer + ((ypos + yoffset) * (dwLcdWidth  * 2)) + (xoffset * 2));
		for (xpos = 0; xpos <= bar_width; xpos++)
		{
			if (invert)
			{
				if ((xpos < 2) || (xpos > (bar_width - 2))|| (ypos < 2) || (ypos > (bar_height - 2))) {
					*pPixel-- = 0xff;
					*pPixel-- = 0xff;
				} else {
					if (xpos < bar_pos) {
						*pPixel-- = 0x00;
						*pPixel-- = 0x1F;
					} else {
						*pPixel-- = 0x00;
						*pPixel-- = 0x00;
					}
				}
			}
			else
			{
				if ((xpos < 2) || (xpos > (bar_width - 2))|| (ypos < 2) || (ypos > (bar_height - 2))) {
					*pPixel++ = 0xff;
					*pPixel++ = 0xff;
				} else {
					if (xpos < bar_pos) {
						*pPixel++ = 0x00;
						*pPixel++ = 0x1F;
					} else {
						*pPixel++ = 0x00;
						*pPixel++ = 0x00;
					}
				}
			}
		}		
	}
}