// All rights reserved ADENEO EMBEDDED 2010
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


#include "omap.h"
#include "am33x_lcdc.h"
#include "_debug.h"

#ifdef DEBUG

//-----------------------------------------------------------
void
Dump_DISPC_LCD(
    LCDC_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DISPC_LCD, (L"DISPC Registers (LCD):\r\n"));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   PID            		= 0x%08x\r\n", INREG32(&pRegs->PID)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   CTRL           		= 0x%08x\r\n", INREG32(&pRegs->CTRL)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CTRL      		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CTRL)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS0_CONF  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS0_CONF)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS0_ADDR  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS0_ADDR)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS0_DATA  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS0_DATA)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS1_CONF  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS1_CONF)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS1_ADDR  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS1_ADDR)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LIDD_CS1_DATA  		= 0x%08x\r\n", INREG32(&pRegs->LIDD_CS1_DATA)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_CTRL       	= 0x%08x\r\n", INREG32(&pRegs->RASTER_CTRL)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_TIMING_0   	= 0x%08x\r\n", INREG32(&pRegs->RASTER_TIMING_0)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_TIMING_1   	= 0x%08x\r\n", INREG32(&pRegs->RASTER_TIMING_1)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_TIMING_2   	= 0x%08x\r\n", INREG32(&pRegs->RASTER_TIMING_2)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_SUBPANEL   	= 0x%08x\r\n", INREG32(&pRegs->RASTER_SUBPANEL)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   RASTER_SUBPANEL2  	= 0x%08x\r\n", INREG32(&pRegs->RASTER_SUBPANEL2)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LCDDMA_CTRL       	= 0x%08x\r\n", INREG32(&pRegs->LCDDMA_CTRL)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LCDDMA_FB0_BASE   	= 0x%08x\r\n", INREG32(&pRegs->LCDDMA_FB0_BASE)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LCDDMA_FB0_CEILING	= 0x%08x\r\n", INREG32(&pRegs->LCDDMA_FB0_CEILING)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LCDDMA_FB1_BASE   	= 0x%08x\r\n", INREG32(&pRegs->LCDDMA_FB1_BASE)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   LCDDMA_FB1_CEILING	= 0x%08x\r\n", INREG32(&pRegs->LCDDMA_FB1_CEILING)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   SYSCONFIG         	= 0x%08x\r\n", INREG32(&pRegs->SYSCONFIG)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   IRQSTATUS_RAW     	= 0x%08x\r\n", INREG32(&pRegs->IRQSTATUS_RAW)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   IRQSTATUS        	= 0x%08x\r\n", INREG32(&pRegs->IRQSTATUS)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   IRQENABLE_SET     	= 0x%08x\r\n", INREG32(&pRegs->IRQENABLE_SET)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   IRQENABLE_CLEAR   	= 0x%08x\r\n", INREG32(&pRegs->IRQENABLE_CLEAR)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   CLKC_ENABLE       	= 0x%08x\r\n", INREG32(&pRegs->CLKC_ENABLE)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"   CLKC_RESET        	= 0x%08x\r\n", INREG32(&pRegs->CLKC_RESET)));    
    DEBUGMSG(DUMP_DISPC_LCD, (L"\r\n"));
}									
									
									
#endif //DEBUG

