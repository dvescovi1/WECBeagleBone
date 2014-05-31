//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#pragma warning (push, 3)
#include <Ndis.h>
#pragma warning (pop)
#include "Am33xCpsw3gRegs.h"

PCPSW3G_MDIO_REGS f_pMdioRegs;

void MdioWaitForAccessComplete(int channel)
{
    while ((f_pMdioRegs->Useraccess[channel].access & MDIO_GO) != 0);
}

int MdioRd(int PhyAddr, int RegNum, int channel, UINT16 *pData)
{
    MdioWaitForAccessComplete(channel);

    f_pMdioRegs->Useraccess[channel].access = MDIO_GO | 
                                              MDIO_READ | 
                                              ((RegNum & 0x1F) << 21) |
                                              ((PhyAddr & 0x1F) << 16);

    MdioWaitForAccessComplete(channel);

    if(f_pMdioRegs->Useraccess[channel].access & (MDIO_ACK) )
    {
        /* Return reg value on successful ACK */
        *pData = (UINT16)(f_pMdioRegs->Useraccess[channel].access & 0xFFFF);  
        return 0;
    }
    
    return (-1);
}

//Note: +++FIXME: order of parameters is different from eboot's MdioWr 
//                too many calls to this function to fix for now
void MdioWr(UINT16 phyAddr, UINT16 regNum, UINT16 data, int channel)
{
    MdioWaitForAccessComplete(channel);
    f_pMdioRegs->Useraccess[channel].access = MDIO_GO    | 
                                              MDIO_WRITE |
                                              ((regNum  & 0x1F) << 21) |
                                              ((phyAddr & 0x1F) << 16) | 
                                              (data & 0xFFFF);
    MdioWaitForAccessComplete(channel);
}

void MdioSetBits(UINT16 PhyAddr, UINT16 RegNum, UINT16 BitMask, int channel)
{
    UINT16 val;

    if (0 == MdioRd(PhyAddr, RegNum, channel, &val))
    {
        MdioWr(PhyAddr, RegNum, (val | BitMask), channel);
    }
}


void MdioEnable(UINT32 RegsBase)
{
    f_pMdioRegs = (PCPSW3G_MDIO_REGS)RegsBase;
    f_pMdioRegs->Control = (MDIO_ENABLE|0xff) /*(MDIO_ENABLE|1<<24|0x20)*/;
}

UINT32 MdioLink(void)
{
    return (f_pMdioRegs->Link);
}

UINT32 MdioAlive(void)
{
    return (f_pMdioRegs->Alive);
}

