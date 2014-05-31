// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//      ISP1504 Register
//      This is temporary put in here and would need to move to inc\ later on.
//
//------------------------------------------------------------------------------
#ifndef OMAP35XX_ISP1504_H
#define OMAP35XX_ISP1504_H

#if __cplusplus
extern "C" {
#endif

#define ISP1504_VENDORID_LOW_R      0
#define ISP1504_VENDORID_HIGH_R     1
#define ISP1504_PRODUCT_LOW_R       2
#define ISP1504_PRODUCT_HIGH_R      3
#define ISP1504_FUNCTION_CTRL_RW    4
#define ISP1504_FUNCTION_CTRL_S     5
#define ISP1504_FUNCTION_CTRL_C     6
#define ISP1504_INTERFACE_CTRL_RW   7
#define ISP1504_INTERFACE_CTRL_S    8
#define ISP1504_INTERFACE_CTRL_C    9
#define ISP1504_OTG_CTRL_RW         0xA
#define ISP1504_OTG_CTRL_S          0xB
#define ISP1504_OTG_CTRL_C          0xC
#define ISP1504_INTR_RISING_RW      0xD
#define ISP1504_INTR_RISING_S       0xE
#define ISP1504_INTR_RISING_C       0xF
#define ISP1504_INTR_FALLING_RW     0x10
#define ISP1504_INTR_FALLING_S      0x11
#define ISP1504_INTR_FALLING_C      0x12
#define ISP1504_INTR_STATUS_R       0x13
#define ISP1504_INTR_LATCH_RC       0x14
#define ISP1504_DEBUG_R             0x15
#define ISP1504_SCRATCH_RW          0x16
#define ISP1504_SCRATCH_S           0x17
#define ISP1504_SCRATCH_C           0x18
#define ISP1504_ACCESS_EXT_W        0x2F
#define ISP1504_POWER_CTRL_RW       0x3D
#define ISP1504_POWER_CTRL_S        0x3E
#define ISP1504_POWER_CTRL_C        0x3F

//------------------------------------------------------------
//  Function: ISP1504_ReadReg
//
//  Routine Description:
//
//      Read the ULPI ISP1504 Register
//  
//  Arguments:
//
//      pOTG - PHSMUSB_T structure
//      idx  - address to write
//
//  Return
//
//      data being read

UCHAR ISP1504_ReadReg(PHSMUSB_T pOTG, UCHAR idx)
{
    DWORD dwCount = 100;
    UCHAR ucData = 0x00;
    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegAddr = idx;
    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl = ULPI_REG_REQ | ULPI_RD_N_WR;
    // May lead to deadlock
    while ((pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl & ULPI_REG_CMPLT) == 0)
    {
        Sleep(20);
        dwCount--;
        if (dwCount == 0)
        {
            RETAILMSG(1, (TEXT("####### FAIL to read ULPI Reg, Control = 0x%x\r\n"), 
                pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl));
            return 0x00;
        }
    }
    ucData = pOTG->pUsbGenRegs->ulpi_regs.ULPIRegData;

    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl = 0;

    return ucData;
}

//------------------------------------------------------------
//  Function: ISP1504_WriteReg
//
//  Routine Description:
//
//      Write the ULPI ISP1504 Register
//  
//  Arguments:
//
//      pOTG - PHSMUSB_T structure
//      idx  - address to write
//      data - data to be written to
//
//  Return
//

BOOL ISP1504_WriteReg(PHSMUSB_T pOTG, UCHAR idx, UCHAR data)
{
    DWORD dwCount = 500;
    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegAddr = idx;
    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegData = (UINT8)data;
    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl = ULPI_REG_REQ;
    // May lead to deadlock
    while ((pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl & ULPI_REG_CMPLT) == 0)
    {
        Sleep(2);
        dwCount--;
        if (dwCount == 0)
        {
            RETAILMSG(1, (TEXT("####### FAIL to write ULPI Reg, Control = 0x%x\r\n"), 
                pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl));
            return FALSE;
        }
    }

    pOTG->pUsbGenRegs->ulpi_regs.ULPIRegControl = 0;

    return TRUE;
}

//------------------------------------------------------------
//  Function: DumpIsp1504Regs
//
//  Routine Description:
//
//      Dump the value of ISP1504 ULPI register set
//  
//  Arguments:
//
//      pOTG - PHSMUSB_T structure
//
//  Return
//
void DumpIsp1504Regs(PHSMUSB_T pOTG)
{
	UNREFERENCED_PARAMETER(pOTG);

    // Don't touch the ULPI register interface
    /*
    RETAILMSG(1, (L"ULPI Register\r\n"));
    RETAILMSG(1, (L"\tULPI_VID=(%x,%x)\r\n", 
            ISP1504_ReadReg(pOTG, ISP1504_VENDORID_LOW_R),
            ISP1504_ReadReg(pOTG, ISP1504_VENDORID_HIGH_R)));
    RETAILMSG(1, (L"\tULPI_PID=(%x,%x)\r\n", 
            ISP1504_ReadReg(pOTG, ISP1504_PRODUCT_LOW_R),
            ISP1504_ReadReg(pOTG, ISP1504_PRODUCT_HIGH_R)));
    RETAILMSG(1, (L"\tFunction Control(%x)=%x\r\n", 
            ISP1504_FUNCTION_CTRL_RW,
            ISP1504_ReadReg(pOTG, ISP1504_FUNCTION_CTRL_RW)));
    RETAILMSG(1, (L"\tInterface Control(%x)=%x\r\n", 
            ISP1504_INTERFACE_CTRL_RW,
            ISP1504_ReadReg(pOTG, ISP1504_INTERFACE_CTRL_RW)));
    RETAILMSG(1, (L"\tOTG Control(%x)=%x\r\n", 
            ISP1504_OTG_CTRL_RW,
            ISP1504_ReadReg(pOTG, ISP1504_OTG_CTRL_RW)));
    RETAILMSG(1, (L"\tInterrupt Enable rising(%x)=%x\r\n", 
            ISP1504_INTR_RISING_RW,
            ISP1504_ReadReg(pOTG, ISP1504_INTR_RISING_RW)));
    RETAILMSG(1, (L"\tInterrupt Enable Falling(%x)=%x\r\n", 
            ISP1504_INTR_FALLING_RW,
            ISP1504_ReadReg(pOTG, ISP1504_INTR_FALLING_RW)));
    RETAILMSG(1, (L"\tInterrupt Status(%x)=%x\r\n", 
            ISP1504_INTR_STATUS_R,
            ISP1504_ReadReg(pOTG, ISP1504_INTR_STATUS_R)));
    // Don't read the latch, reading clears the interrupt.
    //RETAILMSG(1, (L"\tInterrupt Latch(%x)=%x\r\n", 
    //        ISP1504_INTR_LATCH_RC,
    //        ISP1504_ReadReg(pOTG, ISP1504_INTR_LATCH_RC)));
    RETAILMSG(1, (L"\tDebug(%x)=%x\r\n", 
            ISP1504_DEBUG_R,
            ISP1504_ReadReg(pOTG, ISP1504_DEBUG_R)));
    RETAILMSG(1, (L"\tScratch(%x)=%x\r\n", 
            ISP1504_SCRATCH_RW,
            ISP1504_ReadReg(pOTG, ISP1504_SCRATCH_RW)));
    RETAILMSG(1, (L"\tPower(%x)=%x\r\n", 
            ISP1504_POWER_CTRL_RW,
            ISP1504_ReadReg(pOTG, ISP1504_POWER_CTRL_RW)));
    */
}

//--------------------------------------------------------------------
// Function:    ISP1504_SetLowPowerMode
//
// Routine Description: This function is to set the PHY to lower power mode.
//
// Arguments:
//
//      pOTG - PHSMUSB_T structure
//
//  Return
//
BOOL ISP1504_SetLowPowerMode(PHSMUSB_T pOTG)
{    
    ISP1504_WriteReg(pOTG, ISP1504_FUNCTION_CTRL_C, 1<<6);
    return TRUE;
}

//--------------------------------------------------------------------
// Function:    ISP1504_WritefuncCtrlReg()
//
// Routine Description: This function is to set the PHY to lower power mode.
//
// Arguments:
//
//      pOTG - PHSMUSB_T structure
//
//  Return
//
//VOID ISP1504_WriteFuncCtrlReg(PHSMUSB_T pOTG, UCHAR mode, UCHAR data)
//{    
//    ISP1504_WriteReg(pOTG, ISP1504_FUNCTION_CTRL_RW + mode, data);
//}


#ifdef __cplusplus
}
#endif

#endif

