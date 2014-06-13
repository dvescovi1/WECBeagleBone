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
#include "omap_dss_regs.h"
#include "_debug.h"

#ifdef DEBUG

//-----------------------------------------------------------
void
Dump_DSS(
    OMAP_DSS_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DSS, (L"DSS Registers:\r\n"));
    DEBUGMSG(DUMP_DSS, (L"   DSS_REVISIONNUMBER = 0x%08x\r\n", INREG32(&pRegs->DSS_REVISIONNUMBER)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_SYSCONFIG      = 0x%08x\r\n", INREG32(&pRegs->DSS_SYSCONFIG)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_SYSSTATUS      = 0x%08x\r\n", INREG32(&pRegs->DSS_SYSSTATUS)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_CONTROL        = 0x%08x\r\n", INREG32(&pRegs->DSS_CONTROL)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_SDI_CONTROL    = 0x%08x\r\n", INREG32(&pRegs->DSS_SDI_CONTROL)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_PLL_CONTROL    = 0x%08x\r\n", INREG32(&pRegs->DSS_PLL_CONTROL)));
    DEBUGMSG(DUMP_DSS, (L"   DSS_SDI_STATUS     = 0x%08x\r\n", INREG32(&pRegs->DSS_SDI_STATUS)));
    DEBUGMSG(DUMP_DSS, (L"\r\n"));
}

//-----------------------------------------------------------
void
Dump_DISPC(
    OMAP_DISPC_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DISPC, (L"DISPC Registers:\r\n"));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_REVISION     = 0x%08x\r\n", INREG32(&pRegs->DISPC_REVISION)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_SYSCONFIG    = 0x%08x\r\n", INREG32(&pRegs->DISPC_SYSCONFIG)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_SYSSTATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_SYSSTATUS)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_IRQSTATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_IRQSTATUS)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_IRQENABLE    = 0x%08x\r\n", INREG32(&pRegs->DISPC_IRQENABLE)));
    DEBUGMSG(DUMP_DISPC, (L"\r\n"));
}

//-----------------------------------------------------------
void
Dump_DISPC_Control(
    OMAP_DISPC_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DISPC, (L"DISPC Registers (Control):\r\n"));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_CONTROL        = 0x%08x\r\n", INREG32(&pRegs->DISPC_CONTROL)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_CONFIG         = 0x%08x\r\n", INREG32(&pRegs->DISPC_CONFIG)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_CAPABLE        = 0x%08x\r\n", INREG32(&pRegs->DISPC_CAPABLE)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_DEFAULT_COLOR0 = 0x%08x\r\n", INREG32(&pRegs->DISPC_DEFAULT_COLOR0)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_DEFAULT_COLOR1 = 0x%08x\r\n", INREG32(&pRegs->DISPC_DEFAULT_COLOR1)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_TRANS_COLOR0   = 0x%08x\r\n", INREG32(&pRegs->DISPC_TRANS_COLOR0)));
    DEBUGMSG(DUMP_DISPC, (L"   DISPC_TRANS_COLOR1   = 0x%08x\r\n", INREG32(&pRegs->DISPC_TRANS_COLOR1)));
    DEBUGMSG(DUMP_DISPC, (L"\r\n"));
}

//-----------------------------------------------------------
void
Dump_DISPC_LCD(
    OMAP_DISPC_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DISPC_LCD, (L"DISPC Registers (LCD):\r\n"));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_LINE_STATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_LINE_STATUS)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_LINE_NUMBER    = 0x%08x\r\n", INREG32(&pRegs->DISPC_LINE_NUMBER)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_TIMING_H       = 0x%08x\r\n", INREG32(&pRegs->DISPC_TIMING_H)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_TIMING_V       = 0x%08x\r\n", INREG32(&pRegs->DISPC_TIMING_V)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_POL_FREQ       = 0x%08x\r\n", INREG32(&pRegs->DISPC_POL_FREQ)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_DIVISOR        = 0x%08x\r\n", INREG32(&pRegs->DISPC_DIVISOR)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_SIZE_DIG       = 0x%08x\r\n", INREG32(&pRegs->DISPC_SIZE_DIG)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"   DISPC_SIZE_LCD       = 0x%08x\r\n", INREG32(&pRegs->DISPC_SIZE_LCD)));
    DEBUGMSG(DUMP_DISPC_LCD, (L"\r\n"));
}

//-----------------------------------------------------------
void
Dump_DISPC_GFX(
    OMAP_DISPC_REGS* pRegs
    )
{
    DEBUGMSG(DUMP_DISPC_GFX, (L"DISPC Registers (GFX):\r\n"));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_BA0              = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_BA0)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_BA1              = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_BA1)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_POSITION         = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_POSITION)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_SIZE             = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_SIZE)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_ATTRIBUTES       = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_ATTRIBUTES)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_FIFO_THRESHOLD   = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_FIFO_THRESHOLD)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_FIFO_SIZE_STATUS = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_FIFO_SIZE_STATUS)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_ROW_INC          = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_ROW_INC)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_PIXEL_INC        = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_PIXEL_INC)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_WINDOW_SKIP      = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_WINDOW_SKIP)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"   DISPC_GFX_TABLE_BA         = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_TABLE_BA)));
    DEBUGMSG(DUMP_DISPC_GFX, (L"\r\n"));
}

//-----------------------------------------------------------
void
Dump_DISPC_VID(
    OMAP_VID_REGS* pRegs,
    UINT32* pVidCoeffVRegs,
    DWORD x
    )
{
    DWORD   i;
    
    DEBUGMSG(DUMP_DISPC_VID, (L"DISPC Registers (VID %d):\r\n", x));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_BA0              = 0x%08x\r\n", INREG32(&pRegs->BA0)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_BA1              = 0x%08x\r\n", INREG32(&pRegs->BA1)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_POSITION         = 0x%08x\r\n", INREG32(&pRegs->POSITION)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_SIZE             = 0x%08x\r\n", INREG32(&pRegs->SIZE)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_ATTRIBUTES       = 0x%08x\r\n", INREG32(&pRegs->ATTRIBUTES)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_FIFO_THRESHOLD   = 0x%08x\r\n", INREG32(&pRegs->FIFO_THRESHOLD)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_FIFO_SIZE_STATUS = 0x%08x\r\n", INREG32(&pRegs->FIFO_SIZE_STATUS)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_ROW_INC          = 0x%08x\r\n", INREG32(&pRegs->ROW_INC)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_PIXEL_INC        = 0x%08x\r\n", INREG32(&pRegs->PIXEL_INC)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_FIR              = 0x%08x\r\n", INREG32(&pRegs->FIR)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_PICTURE_SIZE     = 0x%08x\r\n", INREG32(&pRegs->PICTURE_SIZE)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_ACCU0            = 0x%08x\r\n", INREG32(&pRegs->ACCU0)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_ACCU1            = 0x%08x\r\n", INREG32(&pRegs->ACCU1)));

    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_FIR_COEF_Hi _HVi _Vi:\r\n"));
    for( i = 0; i < 8; i++ )
    {
        DEBUGMSG(DUMP_DISPC_VID, (L"     %d: H = 0x%08x   HV = 0x%08x   V = 0x%08x\r\n", i, INREG32(&pRegs->aFIR_COEF[i].ulH), INREG32(&pRegs->aFIR_COEF[i].ulHV), INREG32(&pVidCoeffVRegs[i])));
    }
    
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_CONV_COEF0       = 0x%08x\r\n", INREG32(&pRegs->CONV_COEF0)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_CONV_COEF1       = 0x%08x\r\n", INREG32(&pRegs->CONV_COEF1)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_CONV_COEF2       = 0x%08x\r\n", INREG32(&pRegs->CONV_COEF2)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_CONV_COEF3       = 0x%08x\r\n", INREG32(&pRegs->CONV_COEF3)));
    DEBUGMSG(DUMP_DISPC_VID, (L"   DISPC_VIDn_CONV_COEF4       = 0x%08x\r\n", INREG32(&pRegs->CONV_COEF4)));

    DEBUGMSG(DUMP_DISPC_VID, (L"\r\n"));
}

#endif //DEBUG

#if 0
//-----------------------------------------------------------
void
Retail_Dump_DSS(
    OMAP_DSS_REGS* pRegs
    )
{
    RETAILMSG(1, (L"DSS Registers:\r\n"));
    RETAILMSG(1, (L"   DSS_REVISIONNUMBER = 0x%08x\r\n", INREG32(&pRegs->DSS_REVISIONNUMBER)));
    RETAILMSG(1, (L"   DSS_SYSCONFIG      = 0x%08x\r\n", INREG32(&pRegs->DSS_SYSCONFIG)));
    RETAILMSG(1, (L"   DSS_SYSSTATUS      = 0x%08x\r\n", INREG32(&pRegs->DSS_SYSSTATUS)));
    RETAILMSG(1, (L"   DSS_CONTROL        = 0x%08x\r\n", INREG32(&pRegs->DSS_CONTROL)));
    RETAILMSG(1, (L"   DSS_SDI_CONTROL    = 0x%08x\r\n", INREG32(&pRegs->DSS_SDI_CONTROL)));
    RETAILMSG(1, (L"   DSS_PLL_CONTROL    = 0x%08x\r\n", INREG32(&pRegs->DSS_PLL_CONTROL)));
    RETAILMSG(1, (L"   DSS_SDI_STATUS     = 0x%08x\r\n", INREG32(&pRegs->DSS_SDI_STATUS)));
    RETAILMSG(1, (L"\r\n"));
}

//-----------------------------------------------------------
void
Retail_Dump_DISPC(
    OMAP_DISPC_REGS* pRegs
    )
{
    RETAILMSG(1, (L"DISPC Registers:\r\n"));
    RETAILMSG(1, (L"   DISPC_REVISION     = 0x%08x\r\n", INREG32(&pRegs->DISPC_REVISION)));
    RETAILMSG(1, (L"   DISPC_SYSCONFIG    = 0x%08x\r\n", INREG32(&pRegs->DISPC_SYSCONFIG)));
    RETAILMSG(1, (L"   DISPC_SYSSTATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_SYSSTATUS)));
    RETAILMSG(1, (L"   DISPC_IRQSTATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_IRQSTATUS)));
    RETAILMSG(1, (L"   DISPC_IRQENABLE    = 0x%08x\r\n", INREG32(&pRegs->DISPC_IRQENABLE)));
    RETAILMSG(1, (L"DISPC Registers (Control):\r\n"));
    RETAILMSG(1, (L"   DISPC_CONTROL        = 0x%08x\r\n", INREG32(&pRegs->DISPC_CONTROL)));
    RETAILMSG(1, (L"   DISPC_CONFIG         = 0x%08x\r\n", INREG32(&pRegs->DISPC_CONFIG)));
    RETAILMSG(1, (L"   DISPC_CAPABLE        = 0x%08x\r\n", INREG32(&pRegs->DISPC_CAPABLE)));
    RETAILMSG(1, (L"   DISPC_DEFAULT_COLOR0 = 0x%08x\r\n", INREG32(&pRegs->DISPC_DEFAULT_COLOR0)));
    RETAILMSG(1, (L"   DISPC_DEFAULT_COLOR1 = 0x%08x\r\n", INREG32(&pRegs->DISPC_DEFAULT_COLOR1)));
    RETAILMSG(1, (L"   DISPC_TRANS_COLOR0   = 0x%08x\r\n", INREG32(&pRegs->DISPC_TRANS_COLOR0)));
    RETAILMSG(1, (L"   DISPC_TRANS_COLOR1   = 0x%08x\r\n", INREG32(&pRegs->DISPC_TRANS_COLOR1)));
    RETAILMSG(1, (L"DISPC Registers (LCD):\r\n"));
    RETAILMSG(1, (L"   DISPC_LINE_STATUS    = 0x%08x\r\n", INREG32(&pRegs->DISPC_LINE_STATUS)));
    RETAILMSG(1, (L"   DISPC_LINE_NUMBER    = 0x%08x\r\n", INREG32(&pRegs->DISPC_LINE_NUMBER)));
    RETAILMSG(1, (L"   DISPC_TIMING_H       = 0x%08x\r\n", INREG32(&pRegs->DISPC_TIMING_H)));
    RETAILMSG(1, (L"   DISPC_TIMING_V       = 0x%08x\r\n", INREG32(&pRegs->DISPC_TIMING_V)));
    RETAILMSG(1, (L"   DISPC_POL_FREQ       = 0x%08x\r\n", INREG32(&pRegs->DISPC_POL_FREQ)));
    RETAILMSG(1, (L"   DISPC_DIVISOR        = 0x%08x\r\n", INREG32(&pRegs->DISPC_DIVISOR)));
    RETAILMSG(1, (L"   DISPC_SIZE_DIG       = 0x%08x\r\n", INREG32(&pRegs->DISPC_SIZE_DIG)));
    RETAILMSG(1, (L"   DISPC_SIZE_LCD       = 0x%08x\r\n", INREG32(&pRegs->DISPC_SIZE_LCD)));
    RETAILMSG(1, (L"DISPC Registers (GFX):\r\n"));
    RETAILMSG(1, (L"   DISPC_GFX_BA0              = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_BA0)));
    RETAILMSG(1, (L"   DISPC_GFX_BA1              = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_BA1)));
    RETAILMSG(1, (L"   DISPC_GFX_POSITION         = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_POSITION)));
    RETAILMSG(1, (L"   DISPC_GFX_SIZE             = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_SIZE)));
    RETAILMSG(1, (L"   DISPC_GFX_ATTRIBUTES       = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_ATTRIBUTES)));
    RETAILMSG(1, (L"   DISPC_GFX_FIFO_THRESHOLD   = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_FIFO_THRESHOLD)));
    RETAILMSG(1, (L"   DISPC_GFX_FIFO_SIZE_STATUS = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_FIFO_SIZE_STATUS)));
    RETAILMSG(1, (L"   DISPC_GFX_ROW_INC          = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_ROW_INC)));
    RETAILMSG(1, (L"   DISPC_GFX_PIXEL_INC        = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_PIXEL_INC)));
    RETAILMSG(1, (L"   DISPC_GFX_WINDOW_SKIP      = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_WINDOW_SKIP)));
    RETAILMSG(1, (L"   DISPC_GFX_TABLE_BA         = 0x%08x\r\n", INREG32(&pRegs->DISPC_GFX_TABLE_BA)));
    RETAILMSG(1, (L"\r\n"));
}
#endif
