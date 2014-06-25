//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

//------------------------------------------------------------------------------
//
//  File: lan911x.c
//
//  This file implements ethernet debug driver for SMSC LAN911x/LAN9220/LAN9311 network chip.
//
#include <windows.h>
#include <halether.h>
#include "omap.h"

//------------------------------------------------------------------------------
// Types

typedef struct 
{
    UINT32  rx_fifo;            // 0x00
    UINT32 rx_fifo_alias[7];    // 0x04
    UINT32 tx_fifo;             // 0x20
    UINT32 tx_fifo_alias[7];    // 0x24
    UINT32 rx_status_fifo;      // 0x40
    UINT32 rx_status_fifo_peek; // 0x44
    UINT32 tx_status_fifo;      // 0x48
    UINT32 tx_status_fifo_peek; // 0x4c
    UINT32 id_rev;              // 0x50
    UINT32 irq_cfg;             // 0x54
    UINT32 int_sts;             // 0x58 
    UINT32 int_en;              // 0x5c
    UINT32 reserved0;           // 0x60
    UINT32 byte_test;           // 0x64
    UINT32 fifo_int;            // 0x68
    UINT32 rx_cfg;              // 0x6c
    UINT32 tx_cfg;              // 0x70
    UINT32 hw_cfg;              // 0x74
    UINT32 rx_dp_ctrl;          // 0x78
    UINT32 rx_fifo_inf;         // 0x7c 
    UINT32 tx_fifo_inf;         // 0x80 
    UINT32 pmt_ctrl;            // 0x84
    UINT32 gpio_cfg;            // 0x88
    UINT32 gpt_cfg;             // 0x8c
    UINT32 gpt_cnt;             // 0x90
    UINT32 reserved1;           // 0x94
    UINT32 endian;              // 0x98
    UINT32 free_run;            // 0x9c
    UINT32 rx_drop;             // 0xa0
    UINT32 mac_csr_cmd;         // 0xa4
    UINT32 mac_csr_data;        // 0xa8
    UINT32 afc_cfg;             // 0xac
    UINT32 e2p_cmd;             // 0xb0
    UINT32 e2p_data;            // 0xb4 
    UINT32 reserved2[0x3F];
    UINT32 e2p_cmd_9311;             // 0x1b4
    UINT32 e2p_data_9311;            // 0x1b8
} LAN911X_REGS;

//------------------------------------------------------------------------------
// Defines 
//------------------------------------------------------------------------------
// TX Cmd defines
#define TX_CMD_FIRST_SEG        (1<<13)
#define TX_CMD_LAST_SEG         (1<<12)
#define TX_CMD_DATA_OFFSET      16

// TX STATUS defines
#define TX_STATUS_FIFO_ES       (1<<15)

// RX STATUS defines
#define RX_STATUS_FIFO_ES               (1<<15)
#define RX_STATUS_FIFO_LENGTH_MASK      (0x3FFF<<16)
#define RX_STATUS_FIFO_LENGTH_OFFSET    16

// IRQ_CFG defines
#define IRQ_CFG_POL             (1<<4)
#define IRQ_CFG_TYPE            1
#define IRQ_CFG_EN              (1<<8)

// INT_STS defines
#define INT_STS_TSFL            (1<<7)
#define INT_STS_RSFL            (1<<3)

// INT_EN defines
#define INT_EN_RSFL             (1<<3)

// RX_CFG defines
#define RX_CFG_RXDOFF_OFFSET    8

// TX_CFG defines
#define TX_CFG_TX_ON            (1<<1)

// HW_CFG defines
#define HW_CFG_SRST             1
#define HW_CFG_SRST_TIMEOUT     2
#define HW_CFG_SF               (1<<20)

// RX_DP_CTRL defines
#define RX_DP_CTRL_FFWD         (1<<31)

// TX_FIFO_INF defines
#define TX_FIFO_INF_TDFREE      0xFFFF
#define TX_FIFO_INF_TXSUSED     (0xFF<<16)

// RX_FIFO_INF defines
#define RX_FIFO_INF_RXSUSED     (0xFF<<16)

// PMT_CTL defines
#define PMT_CTL_PM_MODE         0x3000
#define PMT_CTL_PHY_RESET       (1 << 10)
#define PMT_CTL_READY           1

// MAC_CSR defines
#define MAC_CSR_CMD_BUSY        (1 << 31)
#define MAC_CSR_CMD_READ        (1 << 30)

// E2P_CMD defines
#define E2P_CMD_BUSY            (1<<31)
#define E2P_CMD_TIMEOUT         (IS_9311() ? (1<<17) : (1<<9))
#define E2P_CMD_MAC_LOADED      (IS_9311() ? (1<<16) : (1<<8))
#define E2P_CMD_READ            (0<<28)
#define E2P_CMD_EWDS            (1<<28)
#define E2P_CMD_EWEN            (2<<28)
#define E2P_CMD_WRITE           (3<<28)
#define E2P_CMD_WRITEALL        (4<<28)
#define E2P_CMD_ERASE           (5<<28)
#define E2P_CMD_ERASEALL        (6<<28)
#define E2P_CMD_RELOAD          (7<<28)

// MAC register indexes
#define MAC_MAC_CR_INDEX                    1
#define MAC_ADDRH_INDEX                     2
#define MAC_ADDRL_INDEX                     3
#define MAC_HASHH_INDEX                     4
#define MAC_HASHL_INDEX                     5
#define MAC_MII_ACC_INDEX                   6
#define MAC_MII_DATA_INDEX                  7
#define MAC_FLOW_INDEX                      8
#define MAC_VLAN1_INDEX                     9
#define MAC_VLAN2_INDEX                     10
#define MAC_WUFF_INDEX                      11
#define MAC_WUCSR_INDEX                     12

// MAC_CR defines
#define MAC_CR_TXEN                         (1<<3)
#define MAC_CR_RXEN                         (1<<2)
#define MAC_CR_FDPX                         (1<<20)

// MII_ACC defines
#define MII_ACC_MIIBZY                      1

// PHY register indexes
#define PHY_CONTROL_INDEX                   0
#define PHY_STATUS_INDEX                    1
#define PHY_ID_1_INDEX                      2
#define PHY_ID_2_INDEX                      3
#define PHY_AUTO_ADVERTISE_INDEX            4
#define PHY_AUTO_PARTNER_ABILITY_INDEX      5
#define PHY_AUTO_EXPANSION_INDEX            6
#define PHY_SPECIAL_CTL_STS_INDEX           31

// PHY_STATUS defines
#define PHY_STATUS_LINKUP                   (1 << 2)


// General configuration parameters
#define LINK_TIMEOUT_SECS                   3
// Default IRQ to active low, push pull
#define IRQ_CONFIGURATION_VALUE             (IRQ_CFG_EN|IRQ_CFG_TYPE)
#define FCS_LENGTH                          4 // FCS is 4 bytes

#define IS_9311() (g_dwChipId == 0x93110000)

#define READ_EEPROM_CMD(p) READ_REGISTER_ULONG(IS_9311() ? &p->e2p_cmd_9311 : &p->e2p_cmd)
#define WRITE_EEPROM_CMD(p,val) { UINT32* t = IS_9311() ? &p->e2p_cmd_9311 : &p->e2p_cmd; RETAILMSG(0,(TEXT("t %x\r\n"),t));WRITE_REGISTER_ULONG(t,val);}
#define READ_EEPROM_DATA(p) READ_REGISTER_ULONG(IS_9311() ? &p->e2p_data_9311 : &p->e2p_data)
#define WRITE_EEPROM_DATA(p,val) WRITE_REGISTER_ULONG(IS_9311() ? &p->e2p_data_9311 : &p->e2p_data,val)
//------------------------------------------------------------------------------
// Local Variables 

static LAN911X_REGS * g_pLAN911X = NULL;
static DWORD g_dwChipId = 0;

//------------------------------------------------------------------------------
// Local Functions 

BOOL LAN911XFindController(void* pAddr,DWORD * pdwID);
BOOL LAN911XGetMacAddress(void* pAddr,UINT16 mac[3]);

//------------------------------------------------------------------------------
static UINT32 ReadMacCSR(UINT32 Register)
{
    while ( (MAC_CSR_CMD_BUSY & READ_REGISTER_ULONG(&g_pLAN911X->mac_csr_cmd)));
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->mac_csr_cmd, MAC_CSR_CMD_BUSY | MAC_CSR_CMD_READ | Register); 
    while ( (MAC_CSR_CMD_BUSY & READ_REGISTER_ULONG(&g_pLAN911X->mac_csr_cmd)));
    return READ_REGISTER_ULONG(&g_pLAN911X->mac_csr_data);
}

static void WriteMacCSR(UINT32 Register, UINT32 Data)
{
    while ( (MAC_CSR_CMD_BUSY & READ_REGISTER_ULONG(&g_pLAN911X->mac_csr_cmd)));
    WRITE_REGISTER_ULONG(&g_pLAN911X->mac_csr_data, Data);
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->mac_csr_cmd, MAC_CSR_CMD_BUSY | Register);
    while ( (MAC_CSR_CMD_BUSY & READ_REGISTER_ULONG(&g_pLAN911X->mac_csr_cmd)));
}

static UINT16 ReadPhy(UINT32 Register)
{
    while (MII_ACC_MIIBZY & ReadMacCSR(MAC_MII_ACC_INDEX));
    WriteMacCSR(MAC_MII_ACC_INDEX, 0x801 | (Register << 6));
    while (MII_ACC_MIIBZY & ReadMacCSR(MAC_MII_ACC_INDEX));
    return (UINT16) ReadMacCSR(MAC_MII_DATA_INDEX);
}

static void WritePhy(UINT32 Register, UINT16 Data)
{
    while (MII_ACC_MIIBZY & ReadMacCSR(MAC_MII_ACC_INDEX));
    WriteMacCSR(MAC_MII_DATA_INDEX, Data);
    WriteMacCSR(MAC_MII_ACC_INDEX, 0x803 | (Register << 6));
    while (MII_ACC_MIIBZY & ReadMacCSR(MAC_MII_ACC_INDEX));
}

BOOL LAN911XInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    BOOL RetCode = FALSE;
    DWORD dwTemp;
    UINT32 StartTime;

	UNREFERENCED_PARAMETER(offset);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+LAN911XInit(0x%08x, 0x%08x, 0x%08x)\r\n", pAddress, offset, mac
        ));
    // Save address
    if ((DWORD) pAddress > 0x80000000)
    {
        g_pLAN911X = (LAN911X_REGS*)pAddress;
    }
    else
    {
        g_pLAN911X = (LAN911X_REGS*)OALPAtoUA((DWORD) pAddress);
    }

    // wake chip, check for valid ID, wait for ready
    if (!LAN911XFindController(g_pLAN911X,&dwTemp))
        goto ErrorReturn;
     
    // Issue soft reset, wait for soft reset to finish or timeout
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->hw_cfg, HW_CFG_SRST | HW_CFG_SF | (5<<16));
    do 
    {
        dwTemp = READ_REGISTER_ULONG((PULONG)&g_pLAN911X->hw_cfg);
    }
    while ((dwTemp & HW_CFG_SRST) && !(dwTemp & HW_CFG_SRST_TIMEOUT));
    if (dwTemp & HW_CFG_SRST_TIMEOUT)
    {
        OALMSGS(OAL_ERROR, (L"Soft reset timeout!! (HW_CFG = 0x%x) Aborting...\r\n", dwTemp));
        goto ErrorReturn;
    }
    // wait for chip ready
    while ( !(PMT_CTL_READY & READ_REGISTER_ULONG((PULONG)&g_pLAN911X->pmt_ctrl)));    

    
    // reset the PHY, wait for reset done
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->pmt_ctrl, PMT_CTL_PHY_RESET | READ_REGISTER_ULONG((PULONG)&g_pLAN911X->pmt_ctrl));
    while ( (PMT_CTL_PHY_RESET & READ_REGISTER_ULONG((PULONG)&g_pLAN911X->pmt_ctrl)));

    // extra delay to allow PHY to enter operational state (finish reset and start RX_CLK/TX_CLK)
    OALStall(1000);
   
    

    // Default FIFO allocation (5K TX, 11K RX)
    // Set Store and Forward
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->hw_cfg, HW_CFG_SF | (5<<16));

    // set flow control high to 7KB, low to 3.5KB, backpressure duration 50us/500us (10/100)
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->afc_cfg, 0x006e3740);

    if (!(E2P_CMD_MAC_LOADED & READ_EEPROM_CMD(g_pLAN911X)))
    {
#if defined(SIMULATED_MAC_ADDRESS0) && defined(SIMULATED_MAC_ADDRESS1) && defined(SIMULATED_MAC_ADDRESS2)
        OALMSGS(OAL_ERROR, (L"MAC address not found in EEPROM, using simulated MAC address\r\n"));
        mac[0] = SIMULATED_MAC_ADDRESS0;
        mac[1] = SIMULATED_MAC_ADDRESS1;
        mac[2] = SIMULATED_MAC_ADDRESS2;
#else
        OALMSGS(OAL_ERROR, (L"MAC address not found in EEPROM!!  using the mac address from the settings\r\n"));
#endif           
        WriteMacCSR(MAC_ADDRL_INDEX, ((DWORD) mac[1]) << 16 | mac[0]);
        WriteMacCSR(MAC_ADDRH_INDEX, (DWORD) mac[2]);

    }
    else
    {        
        LAN911XGetMacAddress(g_pLAN911X,mac);
        DEBUGMSG(1,(TEXT("MAC ADDR in EEPROM: %x:%x:%x:%x:%x:%x\r\n"),mac[0] & 0xFF,(mac[0]>>8) & 0xFF,
            mac[1] & 0xFF,(mac[1]>>8) & 0xFF,
            mac[2] & 0xFF,(mac[2]>>8) & 0xFF));
    }

    // Setup GPIO pins for link status
    WRITE_REGISTER_ULONG((PULONG)&g_pLAN911X->gpio_cfg, (0x70000000));
    // Configure interrupt to fire on a single received packet, initially disabled
    WRITE_REGISTER_ULONG(&g_pLAN911X->fifo_int, 1);
    // Mask all int sources level interrupt
    WRITE_REGISTER_ULONG(&g_pLAN911X->int_en, 0);
    // Clear any potential interrupt (shouldn't be any)
    WRITE_REGISTER_ULONG(&g_pLAN911X->int_sts, 0xFFFFFFFF);

    // Configure IRQ pin
    WRITE_REGISTER_ULONG(&g_pLAN911X->irq_cfg, IRQ_CONFIGURATION_VALUE);

    // Depend on SPEED_SEL configuration for link parameters.  
    // SPEED_SEL high, autonegotiation is enabled by default at 100Mbps
    // SPEED_SEL low, autonogation is disabled and default is 10Mbps/half duplex

#if 0
    // DR - This code is failing on some hubs/switches.
    // Disable for now as it's not critical, debug and re-enable later if needed
    // Override hardware strapping, advertise all capability
    // Enable and start autonegotiation
    WritePhy(PHY_AUTO_ADVERTISE_INDEX, 0x1E0);
    WritePhy(PHY_CONTROL_INDEX, 0x1200);
#endif

    // Wait for PHY link status to be complete
    StartTime = OEMEthGetSecs();
    do
    {
        dwTemp = ReadPhy(PHY_STATUS_INDEX);
        if (PHY_STATUS_LINKUP & dwTemp)
            break;
    }
    while (OEMEthGetSecs() < (StartTime + LINK_TIMEOUT_SECS));

    if (!(dwTemp & PHY_STATUS_LINKUP))
    {
        OALMSGS(OAL_ERROR, (L"Link not detected!!  Aborting\r\n"));
        goto ErrorReturn;
    }

    // Document link parameters, set MAC CR register accordingly
    OALMSGS(OAL_ETHER, (L"Link Established: "));
    switch((0x001C & ReadPhy(PHY_SPECIAL_CTL_STS_INDEX)) >> 2)
    {
    case 1:
        OALMSGS(OAL_ETHER, (L"10Mbps, Half Duplex\r\n"));
        WriteMacCSR(MAC_MAC_CR_INDEX, MAC_CR_TXEN|MAC_CR_RXEN);
        break;
    case 5:
        OALMSGS(OAL_ETHER, (L"10Mbps, Full Duplex\r\n"));
        WriteMacCSR(MAC_MAC_CR_INDEX, MAC_CR_FDPX|MAC_CR_TXEN|MAC_CR_RXEN);
        break;
    case 2:
        OALMSGS(OAL_ETHER, (L"100Mbps, Half Duplex\r\n"));
        WriteMacCSR(MAC_MAC_CR_INDEX, MAC_CR_TXEN|MAC_CR_RXEN);
        break;
    case 6:
        OALMSGS(OAL_ETHER, (L"100Mbps, Full Duplex\r\n"));
        WriteMacCSR(MAC_MAC_CR_INDEX, MAC_CR_FDPX|MAC_CR_TXEN|MAC_CR_RXEN);
        break;
    default:
        OALMSGS(OAL_ETHER, (L"Invalid mode setting!!\r\n"));
        goto ErrorReturn;
    }

    // Enable transmitter
    WRITE_REGISTER_ULONG(&g_pLAN911X->tx_cfg, TX_CFG_TX_ON);

    RetCode = TRUE;

ErrorReturn:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"-LAN911XInit(mac = %02x:%02x:%02x:%02x:%02x:%02x, RetCode = %d)\r\n",
        mac[0]&0xFF, mac[0]>>8, mac[1]&0xFF, mac[1]>>8, mac[2]&0xFF, mac[2]>>8,
        RetCode
        ));


    return RetCode;
}

//------------------------------------------------------------------------------
// This routine should be called with a pointer to the ethernet frame data.  It is the caller's
//  responsibility to fill in all information including the destination and source addresses and
//  the frame type.  The length parameter gives the number of bytes in the ethernet frame.
// The routine will not return until the frame has been transmitted or an error has occured.  If
//  the frame transmits successfully, 0 is returned.  If an error occured, a message is sent to
//  the serial port and the routine returns non-zero.
//
UINT16 LAN911XSendFrame(UINT8 *pBuffer, UINT32 length)
{
    UINT16 RetCode = 1;
    UINT32 dwTemp;
    UINT32 dwAlignedAddr;
    UINT32 dwOffset;

    if(g_dwChipId == 0x92200000)
    {
        /* This delay is a workaround for LAN9220 chip for stress send */
        OALStall(300);
    }

    // Send complete packet in one frame
    // Note that this controller allows/requires DWORD writes to FIFO,
    // and is configured to strip off extra data at the beginning and 
    // end of the packet.

    // Verify space in TX fifo (should always be available)
    if (length > (6 + (READ_REGISTER_ULONG(&g_pLAN911X->tx_fifo_inf) & TX_FIFO_INF_TDFREE)))
    {
        OALMSGS(OAL_ETHER, (L"Not enough room in TX FIFO!\r\n"));
        goto ErrorReturn;
    }

    // TX_CMD_A
    dwOffset = (UINT32)pBuffer & 0x3;
    dwTemp = dwOffset << TX_CMD_DATA_OFFSET;
    dwTemp |= TX_CMD_FIRST_SEG | TX_CMD_LAST_SEG;
    dwTemp |= length;
    WRITE_REGISTER_ULONG(&g_pLAN911X->tx_fifo, dwTemp);

    // TX_CMD_B
    WRITE_REGISTER_ULONG(&g_pLAN911X->tx_fifo, length);

    dwAlignedAddr = (UINT32)pBuffer & 0xFFFFFFFC;

    // Copy data in DWORD quantities

    // Copy unaligned data at the beginning of the buffer
    if (dwOffset)
    {
        WRITE_REGISTER_ULONG(&g_pLAN911X->tx_fifo, *(UINT32 *)dwAlignedAddr);
        dwAlignedAddr += 4;
        length = length - (4 - dwOffset);
    }

    // Source buffer is now aligned, copy complete DWORDs
    while (length >= 4)
    {
        WRITE_REGISTER_ULONG(&g_pLAN911X->tx_fifo, *(UINT32 *)(dwAlignedAddr));
        length = length - 4;
        dwAlignedAddr += 4;
    }

    // Copy any remaining bytes
    if (length > 0)
        WRITE_REGISTER_ULONG(&g_pLAN911X->tx_fifo, *(UINT32 *)(dwAlignedAddr));

    // Wait for TX complete
    while (!(TX_FIFO_INF_TXSUSED & READ_REGISTER_ULONG(&g_pLAN911X->tx_fifo_inf)));

    // Read status, popped from tx status fifo
    dwTemp = READ_REGISTER_ULONG(&g_pLAN911X->tx_status_fifo);
    if (TX_STATUS_FIFO_ES & dwTemp)
    {
        OALMSGS(OAL_ETHER, (L"Error transmitting buffer!  0x%x\r\n", dwTemp));
        goto ErrorReturn;
    }

    RetCode = 0;

ErrorReturn:
    return RetCode;
}

//------------------------------------------------------------------------------

UINT16 LAN911XGetFrame(UINT8 *pBuffer, UINT16 *pLength)
{
    UINT32 cbLength;
    UINT32 dwTemp;
    UINT32 dwOffset;

    // Sanity check
    if (!pBuffer || !pLength)
        return 0;

    // Check to see if data is available
    if (!(RX_FIFO_INF_RXSUSED & READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo_inf)))
    {
        // No packet available
        *pLength = 0;
        return  *pLength;
    }

    // At least one packet is available, get status
    dwTemp = READ_REGISTER_ULONG(&g_pLAN911X->rx_status_fifo);
    cbLength = (dwTemp & RX_STATUS_FIFO_LENGTH_MASK) >> RX_STATUS_FIFO_LENGTH_OFFSET;
    if (RX_STATUS_FIFO_ES & dwTemp)
    {
        OALMSGS(OAL_ETHER, (L"Error in received packet!  0x%x\r\n", dwTemp));
        // Purge packet
        if (cbLength < 32)
        {
            // Should never occur, since we don't pass bad packets
            // Read appropriate number of DWORDs to clear packet
            cbLength += 3;
            cbLength >>= 2;
            for (; cbLength != 0; cbLength--)
                READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo);
        }
        else
        {
            WRITE_REGISTER_ULONG(&g_pLAN911X->rx_dp_ctrl, (DWORD) RX_DP_CTRL_FFWD);
            while (RX_DP_CTRL_FFWD & READ_REGISTER_ULONG(&g_pLAN911X->rx_dp_ctrl));
        }

        *pLength = 0;
        goto ErrorReturn;
    }

    //length returned is Total frame length minus FCS length so
    //reduce cbLength by 4 bytes
    cbLength = (cbLength > FCS_LENGTH)? (cbLength - FCS_LENGTH) : 0;
    
    // Check input buffer size
    if (*pLength < cbLength)
    {
        // Shouldn't ever occur
        OALMSGS(OAL_ETHER, (L"Input buffer too small!\r\n"));
        // Dump packet 
        WRITE_REGISTER_ULONG(&g_pLAN911X->rx_dp_ctrl, (DWORD) RX_DP_CTRL_FFWD);
        while (RX_DP_CTRL_FFWD & READ_REGISTER_ULONG(&g_pLAN911X->rx_dp_ctrl));
        *pLength = 0;
        goto ErrorReturn;
    }

    // return length in provided parameter
    *pLength = (UINT16) cbLength;

    // Check to ensure pBuffer is aligned on a DWORD boundary.
    // If not, configure controller for additional pad bytes,
    // and read initial DWORD
    dwOffset = (UINT32)pBuffer & 0x3;
    if (dwOffset)
    {
        // This should never happen
        OALMSGS(OAL_ETHER, (L"RX buffer not DWORD aligned...\r\n"));

        // Configure to add padding prior real bytes in first dword read
        // NOTE - there is a discrepancy in the docs on changing this parameter.  One
        // place indicates this can only be changed on RX is off and data purged, another
        // just says you can't change it while reading a packet.  We don't expect to ever
        // be in this condition, but code is here anyway...
        WRITE_REGISTER_ULONG(&g_pLAN911X->rx_cfg, dwOffset << RX_CFG_RXDOFF_OFFSET);

        // Read first dword, copy real bytes into user buffer 
        dwTemp = READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo);
        memcpy(pBuffer, (UINT8 *)((UINT8 *)&dwTemp + dwOffset), 4-dwOffset);
        pBuffer = pBuffer + 4-dwOffset;
        cbLength = cbLength - (4 - dwOffset);
        // user buffer should now be DWORD aligned
    }
    else
        WRITE_REGISTER_ULONG(&g_pLAN911X->rx_cfg, 0);


    // Read complete DWORDs into provided buffer
    while (cbLength >= 4)
    {
        *(ULONG *)pBuffer = READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo);
        cbLength = cbLength - 4;
        pBuffer = pBuffer + 4;
    }

    // Read any remaining bytes into buffer
    if (cbLength)
    {
        dwTemp = READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo);
        memcpy(pBuffer, &dwTemp, cbLength);

    }

    //read the four byte FCS from FIFO and discard it
    dwTemp = READ_REGISTER_ULONG(&g_pLAN911X->rx_fifo);
    
ErrorReturn:
    // Acknowledge RX interrupt
    WRITE_REGISTER_ULONG(&g_pLAN911X->int_sts, INT_STS_RSFL);
    return  *pLength;
}

//------------------------------------------------------------------------------

VOID LAN911XEnableInts()
{
    // Unmask RS status fifo interrupt
    // Global interrupt is already enabled
    WRITE_REGISTER_ULONG(&g_pLAN911X->int_en, INT_EN_RSFL);
}

//------------------------------------------------------------------------------

VOID LAN911XDisableInts()
{
    WRITE_REGISTER_ULONG(&g_pLAN911X->int_en, 0);
}

//------------------------------------------------------------------------------

VOID LAN911XCurrentPacketFilter(UINT32 filter)
{
    // Not supported
    // Must exist though because it's called through a 
    // function pointer that isn't checked 
    UNREFERENCED_PARAMETER(filter);

}

//------------------------------------------------------------------------------

BOOL LAN911XMulticastList(UINT8 *pAddresses, UINT32 count)
{
    // Not supported
    UNREFERENCED_PARAMETER(pAddresses);
    UNREFERENCED_PARAMETER(count);
    return FALSE;
}

//------------------------------------------------------------------------------

// wakes chip, checks for valid ID, waits for chip ready
BOOL LAN911XFindController(void* pAddr,DWORD * pdwID)
{
    BOOL RetCode = FALSE;
    DWORD dwTemp;
    LAN911X_REGS* pCtrl;
    pCtrl = (LAN911X_REGS*) pAddr;
    // Verify chip exists
    dwTemp = READ_REGISTER_ULONG(&pCtrl->id_rev);
    // dummy read
    dwTemp = READ_REGISTER_ULONG(&pCtrl->id_rev);

    g_dwChipId = dwTemp & 0xFFFF0000; 
    
    switch(g_dwChipId)
    {
        case 0x93110000:
        case 0x92200000:
        case 0x01180000:
        case 0x01170000:
        case 0x01160000:
        case 0x01150000:
        break;
    default:
        ERRORMSG(1, (L"Invalid Ethernet chip ID (0x%x)!\r\n", dwTemp));
        goto ErrorReturn;
    }
    //// Wake chip from sleep state - write any value to the byte register, poll PMT ready bit
    //WRITE_REGISTER_ULONG((PULONG)&pCtrl->byte_test, 0);
    //while ( !(PMT_CTL_READY & READ_REGISTER_ULONG((PULONG)&pCtrl->pmt_ctrl)));

    if (pdwID)    
        *pdwID = dwTemp;

    RetCode = TRUE;

ErrorReturn:
    return RetCode;
}


//------------------------------------------------------------------------------
static BOOL LAN911XEepromWrite(UINT8 * pValue, UINT8 Length, UINT8 Offset)
{
    UINT8 index;
    BOOL RetCode = FALSE;

    // Unlock eeprom
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(g_pLAN911X)));
    // Issue erase/write enable
    WRITE_EEPROM_CMD(g_pLAN911X, (ULONG) (E2P_CMD_EWEN | E2P_CMD_TIMEOUT | E2P_CMD_BUSY ));
    // Wait for command to complete
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(g_pLAN911X)));
    // Check for timeout
    if (E2P_CMD_TIMEOUT & READ_EEPROM_CMD(g_pLAN911X))
    {
        goto ErrorReturn;
    }
    index = 0;
    

    do 
    {
        // Erase location
        WRITE_EEPROM_CMD(g_pLAN911X, (Offset + index) | E2P_CMD_ERASE | E2P_CMD_TIMEOUT | E2P_CMD_BUSY );
        // Wait for command to complete
        while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(g_pLAN911X)));

        // Check for timeout
        if (E2P_CMD_TIMEOUT & READ_EEPROM_CMD(g_pLAN911X))
        {            
            goto ErrorReturn;
        }
        
        WRITE_EEPROM_DATA(g_pLAN911X, *pValue);
        WRITE_EEPROM_CMD(g_pLAN911X, (Offset + index) | E2P_CMD_WRITE | E2P_CMD_TIMEOUT | E2P_CMD_BUSY );
        // Wait for command to complete
        while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(g_pLAN911X)));
        // Check for timeout
        if (E2P_CMD_TIMEOUT & READ_EEPROM_CMD(g_pLAN911X))
        {
            goto ErrorReturn;
        }

        pValue++;
        index++;
    }
    while (index < Length);

    RetCode = TRUE;

ErrorReturn:

    // Issue erase/write disable
    WRITE_EEPROM_CMD(g_pLAN911X,(ULONG)( E2P_CMD_EWDS | E2P_CMD_TIMEOUT | E2P_CMD_BUSY ));
    // Wait for command to complete
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(g_pLAN911X)));

    return RetCode;
}


BOOL  LAN911XGetMacAddress(void* pAddr,UINT16 mac[3])
{
    BOOL RetCode = FALSE;
    DWORD dwTemp;
    LAN911X_REGS* pCtrl;

    if (!LAN911XFindController(pAddr,&dwTemp))
    {     
        return FALSE;
    }

    g_pLAN911X = pCtrl = (LAN911X_REGS*) pAddr;
    
	// Force update from eeprom    
    WRITE_EEPROM_CMD(pCtrl, (ULONG) (E2P_CMD_RELOAD | E2P_CMD_TIMEOUT | E2P_CMD_BUSY ));
    // Check for MAC load from EEPROM done
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(pCtrl)));
    while ( !(PMT_CTL_READY & READ_REGISTER_ULONG((PULONG)&pCtrl->pmt_ctrl)));
    
    // do it TWICE because sometimes it doesn't wor properly with only one RELOAD
    WRITE_EEPROM_CMD(pCtrl, (ULONG) (E2P_CMD_RELOAD | E2P_CMD_TIMEOUT | E2P_CMD_BUSY ));
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(pCtrl)));
    while ( !(PMT_CTL_READY & READ_REGISTER_ULONG((PULONG)&pCtrl->pmt_ctrl)));
    


    if (!(E2P_CMD_MAC_LOADED & READ_EEPROM_CMD(pCtrl)))
    {
#if defined(SIMULATED_MAC_ADDRESS0) && defined(SIMULATED_MAC_ADDRESS1) && defined(SIMULATED_MAC_ADDRESS2)
        mac[0] = SIMULATED_MAC_ADDRESS0;
        mac[1] = SIMULATED_MAC_ADDRESS1;
        mac[2] = SIMULATED_MAC_ADDRESS2;
        WriteMacCSR(MAC_ADDRL_INDEX, ((DWORD) mac[1]) << 16 | mac[0]);
        WriteMacCSR(MAC_ADDRH_INDEX, (DWORD) mac[2]);
        RetCode = TRUE;
#else
        goto ErrorReturn;
#endif
    }
    else
    {
        dwTemp = ReadMacCSR(MAC_ADDRL_INDEX);
        mac[0] = (UINT16)(dwTemp & 0xFFFF);
        mac[1] = (UINT16)(dwTemp >> 16);
        mac[2] = (UINT16)(ReadMacCSR(MAC_ADDRH_INDEX) & 0xFFFF);
        RetCode = TRUE;
    }

#if !defined(SIMULATED_MAC_ADDRESS0) || !defined(SIMULATED_MAC_ADDRESS1) || !defined(SIMULATED_MAC_ADDRESS2)
ErrorReturn:
#endif
    return RetCode;
}

BOOL LAN911XSetMacAddress(void* pAddr,UINT16 mac[3])
{
    BOOL RetCode = FALSE;
    UINT8 temp;    
    LAN911X_REGS* pCtrl;

    pCtrl = (LAN911X_REGS*) pAddr;
 
    // Write leading 0xa5
    temp = 0xA5;
    if (!LAN911XEepromWrite(&temp, 1, 0))
    {
        goto ErrorReturn;
    }

    // Write MAC address
    if (!LAN911XEepromWrite((UINT8 *)mac, 6, 1))
    {
        goto ErrorReturn;
    }
    
    // Force reload of MAC
    WRITE_EEPROM_CMD(pCtrl, (ULONG) (E2P_CMD_RELOAD | E2P_CMD_TIMEOUT | E2P_CMD_BUSY ));
    // Wait for command to complete
    while ( (E2P_CMD_BUSY & READ_EEPROM_CMD(pCtrl)));

    RetCode = TRUE;

ErrorReturn:
    return RetCode;
}
