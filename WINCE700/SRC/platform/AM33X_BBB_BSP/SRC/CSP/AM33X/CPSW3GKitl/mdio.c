//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2011-2012
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include <windows.h>
#include <oal.h>
#include <oal_memory.h>

#define AM33X_CPSW_MDIO_BASE          0x4A101000

void MdioWaitForAccessComplete(int channel);

UINT32 g_cpmdio_base=0;
#define CPMDIO_BASE				g_cpmdio_base
#define CPMDIO_ALIVE			(CPMDIO_BASE + 0x08)

#define MDIO_USRACCESS(channel) (*(volatile unsigned int *)(CPMDIO_BASE + 0x80 + (8*(channel))))
#define MDIO_UA_GO              0x80000000   /*(1 << 31)*/
#define MDIO_UA_READ            0x00000000
#define MDIO_UA_WRITE           0x40000000   /*(1 << 30)*/
#define MDIO_UA_ACK             0x20000000   /*(1 << 29)*/
#define MDIO_UA_DATA_MASK       0x0000ffff
#define MDIO_CTRL_ENABLE        0x40000000   /*(1 << 30)*/
#define MDIO_CTRL_IDLE          0x80000000   /*(1 << 31)*/
#define MII_STATUS_REG          1

#define MDIO_CTRL               (( volatile unsigned int *) (CPMDIO_BASE + 0x04))
#define MDIO_ACK                (*(volatile UINT32       *) (CPMDIO_BASE + 0x08))

void MdioWaitForAccessComplete(int channel)
{
    while ((MDIO_USRACCESS(channel) & MDIO_UA_GO) != 0);
}

int MdioRd(UINT16 PhyAddr, UINT16 RegNum, int channel, UINT16 *pData)
{
    MdioWaitForAccessComplete(channel);

    MDIO_USRACCESS(channel) = MDIO_UA_GO | 
                              MDIO_UA_READ | 
                              ((RegNum  & 0x1F) << 21) |
                              ((PhyAddr & 0x1F) << 16);

    MdioWaitForAccessComplete(channel);
    
    if(MDIO_USRACCESS(channel) & MDIO_UA_ACK)
    {
        /* Return reg value on successful ACK */
        *pData = (UINT16)(MDIO_USRACCESS(channel) & MDIO_UA_DATA_MASK);  
        return 0;
    }

    return (-1);
}

void MdioWr(UINT16 phyAddr, UINT16 regNum, int channel, UINT16 data)
{
    MdioWaitForAccessComplete(channel);

    MDIO_USRACCESS(channel) = MDIO_UA_GO | 
                              MDIO_UA_WRITE |
                              ((regNum & 0x1F) << 21) |
                              ((phyAddr & 0x1F) << 16) | 
                              (data & 0xFFFF);

    MdioWaitForAccessComplete(channel);
}

void MdioSetBits(UINT16 PhyAddr, UINT16 RegNum, UINT16 BitMask, int channel)
{
    UINT16 val;

    if (0 == MdioRd(PhyAddr,RegNum,channel, &val))
    {
        MdioWr((PhyAddr & 0xffff), RegNum, channel, (val | BitMask));
    }
}

void MdioEnable(void)
{
    g_cpmdio_base = (UINT32)OALPAtoUA(AM33X_CPSW_MDIO_BASE);
    *(MDIO_CTRL) = (MDIO_CTRL_ENABLE|0xff);	

	/*
	 * wait for scan logic to settle:
	 * the scan time consists of (a) a large fixed component, and (b) a
	 * small component that varies with the mii bus frequency.  These
	 * were estimated using measurements at 1.1 and 2.2 MHz on tnetv107x
	 * silicon.  Since the effect of (b) was found to be largely
	 * negligible, we keep things simple here.
	 */
	OALStall(1000);
}
