// All rights reserved ADENEO EMBEDDED 2010
/*****************************************************************************
   
    Copyright (c) 2004-2008 SMSC. All rights reserved.

    Use of this source code is subject to the terms of the SMSC Software
    License Agreement (SLA) under which you licensed this software product.  
    If you did not accept the terms of the SLA, you are not authorized to use
    this source code. 

    This code and information is provided as is without warranty of any kind,
    either expressed or implied, including but not limited to the implied
    warranties of merchantability and/or fitness for a particular purpose.
     
    File name   : lan9118.h 
    Description : lan9118 Header File

    History     :
        03-16-05 WH         First Release
        08-12-05 MDG        ver 1.01 
            - add LED1 inversion, add PHY work around
        11-07-05 WH         ver 1.02
            - Fixed middle buffer handling bug
              (Driver didn't handle middle buffers correctly if it is less than 
               4bytes size)
            - workaround for Multicast bug
            - Workaround for MAC RXEN bug
        11-17-05 WH         ver 1.03
            - 1.02 didn't have 1.01 patches
            - 1.03 is 1.02 + 1.01
        12-06-05 WH         ver 1.04
            - Fixed RX doesn't work on Silicon A1 (REV_ID = 0x011x0002)
            - Support SMSC9118x/117x/116x/115x family
        02-27-05 WH         ver 1.05
            - Fixing External Phy bug that doesn't work with 117x/115x
        03-23-05 WH         ver 1.06
            - Put the variable to avoid PHY_WORKAROUND for External PHY
            - Change product name to 9118x->9218, 9115x->9215
        07-26-06 WH, MDG, NL        ver 1.07
            - Add RXE and TXE interrupt handlers
            - Workaround Code for direct GPIO connection from 9118 family 
              Interrupt (Level Interrupt -> Edge Interrupt)
            - Change GPT interrupt interval to 200mSec from 50mSec
            - clean up un-used SH3 code
        08-25-06  WH, MDG, NL       ver 1.08
            - Fixed RXE and TXE interrupt handlers bug
            - support for direct and nondirect Interrupt
        02-15-07   NL               ver 1.09
            - First version of WinCE 6.0 driver
            - Removed Support for LAN9112
            - Added AutoMdix as modifiable parameter in the Registry
            - Fixed DMA Xmit Bug
        04-17-07   NL               ver 1.10
            - Added Support LAN9211 Chip
            - Changed Register Name ENDIAN to WORD_SWAP According to the Menual
            - Merged CE6.0 & 5.0 Drivers Together
        10-24-07   NL               ver 1.11
            - Added Support LAN9218A/LAN9217A/LAN9216A/LAN9215A Chips
        01-08-08   AH               ver 1.12
            - Added Support for LAN9210 Chip
        06-19-08   MDG              ver 1.13
            - Added IntCfg as modifiable parameter in the Registry
            - Fixed support for Fixed parameters LinkMode
            - Removed Support for early FPGA versions (OLD_REGISTERS)
        -----------------------------------------------------------------------
        09-26-08   WH
            - Move to version 2.00 intentionally
            - From version 2.00, 
               driver drops support chip ID of 0x011x0000 and 0x011x0001
        -----------------------------------------------------------------------
        09-26-08   WH               ver 2.00
            - replace TAB to SPACE
            - Reorder initialization routines to avoid 
               possible unexpect behavior
            - Fixed the issue Flow Control ignores "FlowControl" key 
               if not Auto Negotiation
            - Added "MinLinkChangeWakeUp", "MinMagicPacketWakeUp" and
               "MinPatternWakeUp" registry key for PM
            - Fixed PM issues
                - DHCP doesn't work after wakeup
                - Disable GPTimer while in sleep mode
                - Add routine to wakeup chip during Reset
            - Fix discarding Rx Frame when it is less than 16bytes
            - Enable RXE & RWT interrupt
            - Chip goes to D2(Energy Detect Power Down) when there is no link
               when POWERDOWN_AT_NO_LINK is defined (see smsc9118.h)
            - Clean up Registers which are absolete
            - See relnotes.txt for detail
        11-17-08   WH               ver 2.01
            - Lan_SetMiiRegW() and Lan_GetMiiRegW are changed to return BOOL.
              Caller checks return value to detect error
            - Tx Error Interrupt is enabled
            - Check return from NdisAllocateMemory() is 32bit aligned or not
              Technically it won't happen though
              See comments in source (smsc9118.c)
            - Fixed bug which does't go to AUTOIP when failed to get DHCP addr
		-----------------------------------------------------------------------
		07-24-09   RL
			- Added support for OMAP platform
*****************************************************************************/

#ifndef LAN9118_H
#define LAN9118_H

//#define USE_GPIO
#include "OS.h"

#ifdef USE_GPIO
#define GPIO0       0x01UL
#define GPIO1       0x02UL
#define GPIO2       0x04UL
#define GPIO3       0x08UL
#define GPIO4       0x10UL
#define SET_GPIO(dwLanBase,gpioBit)             \
{                                               \
    DWORD dwGPIO_CFG;                           \
    dwGPIO_CFG = GetRegDW(dwLanBase, GPIO_CFG); \
    dwGPIO_CFG |= gpioBit;                      \
    SetRegDW(dwLanBase, GPIO_CFG, dwGPIO_CFG);  \
}

#define CLEAR_GPIO(dwLanBase,gpioBit)           \
{                                               \
    DWORD dwGPIO_CFG;                           \
    dwGPIO_CFG = GetRegDW(dwLanBase, GPIO_CFG); \
    dwGPIO_CFG &=~ (gpioBit);                   \
    SetRegDW(dwLanBase, GPIO_CFG, dwGPIO_CFG);  \
}

#else

#define SET_GPIO(dwLanBase,gpioBit)
#define CLEAR_GPIO(dwLanBase,gpioBit)

// Can only work-around LED1 if it's NOT being used for debug purposes
#define USE_LED1_WORK_AROUND        // 10/100 LED link-state inversion
#endif
#define USE_PHY_WORK_AROUND     // output polarity inversion

#ifdef USE_LED1_WORK_AROUND
#define GPIO_CFG_LED1_DIS_ (GPIO_CFG_GPIOBUF0_|GPIO_CFG_GPIODIR0_|GPIO_CFG_GPIOD0_)
#endif

#define LINKMODE_ANEG                   0x40UL
#define LINKMODE_ASYM_PAUSE             0x20UL
#define LINKMODE_SYM_PAUSE              0x10UL
#define LINKMODE_100_FD                 0x08UL
#define LINKMODE_100_HD                 0x04UL
#define LINKMODE_10_FD                  0x02UL
#define LINKMODE_10_HD                  0x01UL
#define LINKMODE_DEFAULT                0x7FUL

#define AMDIX_DISABLE_STRAIGHT      ((WORD)0x0U)
#define AMDIX_DISABLE_CROSSOVER     ((WORD)0x01U)
#define AMDIX_ENABLE                ((WORD)0x02U)



/*
STRUCTURE: LAN9118_DATA
    This structure is used by the chip layer.
    It remembers where to access registers.
    It remembers revision IDs if there are subtle
      differences between them.
    It remembers modes of operation.
    It will start tx and rx queue information
    And it may be used for other things that
      have not been conceived yet.
    The members of this structure should be 
    considered private and only excessable from 
    the Lan_xxx functions
*/
typedef struct _LAN9118_DATA {
    DWORD dwLanBase;
    DWORD dwIdRev;
    DWORD dwFpgaRev;
    DWORD dwIntCfg;

    BOOLEAN LanInitialized;
    BOOLEAN InterruptsInitialized;
    BOOLEAN TxInitialized;
    BOOLEAN SoftwareInterruptSignal;
    BOOLEAN PhyInitialized;

    BYTE bPadding1[3];
    DWORD dwPhyId;

    BYTE bPhyAddress;
    BYTE bPhyModel;
    BYTE bPhyRev;
    BYTE bPadding2;

#define LINK_NO_LINK        (0UL)
#define LINK_10MPS_HALF     (1UL)
#define LINK_10MPS_FULL     (2UL)
#define LINK_100MPS_HALF    (3UL)
#define LINK_100MPS_FULL    (4UL)
#define LINK_AUTO_NEGOTIATE (5UL)

#define    OVERRIDE_DISABLE_STAIGHT     (0U)
#define    OVERRIDE_DISABLE_CROSSOVER   (1U)
#define    OVERRIDE_ENABLE_AMDIX        (2U)
#define    STRAP_CONTROLL               (3U)


    DWORD dwLinkMode;
    DWORD dwRemoteFaultCount;
    WORD wAutoMdix;

    DWORD dwMacAddrHigh16;
    DWORD dwMacAddrLow32;
#ifdef USE_LED1_WORK_AROUND
    DWORD dwOriginalGpioCfg;
    BOOLEAN LED1NotYetRestored;
    BYTE  bPadding3[3];     // for lint
#endif
#ifdef USE_PHY_WORK_AROUND
#define MIN_PACKET_SIZE (64UL)
    DWORD dwTxStartMargen;
    BYTE LoopBackTxPacket[MIN_PACKET_SIZE];
    DWORD dwTxEndMargen;
    DWORD dwRxStartMargen;
    BYTE LoopBackRxPacket[MIN_PACKET_SIZE];
    DWORD dwRxEndMargen;
    DWORD dwResetCount;
#endif

} LAN9118_DATA, *PLAN9118_DATA;
typedef const LAN9118_DATA * const CPCLAN9118_DATA;

/*
FUNCTION: Lan_WriteTxFifo
    This function is used to write a buffer to the
    Tx Fifo in PIO mode.
    This function is only intended to be called 
      from with in other Lan_xxx functions.
*/
void Lan_WriteTxFifo(PLAN9118_DATA const pLan9118Data, const DWORD dwLanBase, const DWORD * const pdwBuf, const DWORD dwDwordCount);

/*
FUNCTION: Lan_ReadRxFifo
    This function is used to read a buffer to the 
    Rx Fifo in PIO mode.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline void Lan_ReadRxFifo(const DWORD dwLanBase, DWORD * const pdwBuf, const DWORD dwDwordCount);

/*
FUNCTION: Lan_GetMacRegDW
    This function is used to read a Mac Register.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline DWORD Lan_GetMacRegDW(
    const DWORD dwLanBase, 
    const DWORD dwOffset);
#define LanReadMac(dwOffset)    \
        Lan_GetMacRegDW(pLan9118Data->dwLanBase, (dwOffset))

/*
FUNCTION: Lan_SetMacRegDW
    This function is used to write a Mac register.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline void Lan_SetMacRegDW(
    const DWORD dwLanBase, 
    const DWORD dwOffset, 
    const DWORD dwVal);
#define LanWriteMac(dwOffset, dwValue)  \
        Lan_SetMacRegDW(pLan9118Data->dwLanBase, (dwOffset), (dwValue))

/*
FUNCTION: Lan_GetMiiRegW, Lan_GetPhyRegW
    This function is used to read a Mii/Phy register.
    This function is only intended to be called 
      from with in other Lan_xxx functions.
*/
__inline BOOL Lan_GetMiiRegW(
    const DWORD dwLanBase,
    const DWORD dwPhyAddress,
    const DWORD dwMiiIndex,
    WORD *wValue);
#define Lan_GetPhyRegW(LanBase, PhyAddr, Index, wValue) \
        Lan_GetMiiRegW(LanBase, (DWORD)PhyAddr, Index, wValue)
#define LanReadPhy(MiiIndex, wValue) \
        Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)pLan9118Data->bPhyAddress, (MiiIndex), wValue)
#define AdapterReadPhy(MiiIndex, wValue) \
        Lan_GetPhyRegW(pAdapter->lan9118_data.dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, (MiiIndex), wValue)

/*
FUNCTION: Lan_SetMiiRegW, Lan_SetPhyRegW
    This function is used to write a Mii/Phy register.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline BOOL Lan_SetMiiRegW(
    const DWORD dwLanBase,
    const DWORD dwPhyAddress,
    const DWORD dwMiiIndex,
    const WORD wVal);
#define Lan_SetPhyRegW Lan_SetMiiRegW
#define LanWritePhy(MiiIndex, Value) \
        Lan_SetPhyRegW(pLan9118Data->dwLanBase, (DWORD)pLan9118Data->bPhyAddress, (MiiIndex), (Value))
#define AdapterWritePhy(MiiIndex, Value) \
        Lan_SetPhyRegW(pAdapter->lan9118_data.dwLanBase, (DWORD)pAdapter->lan9118_data.bPhyAddress, (MiiIndex), (WORD)(Value))

/*
FUNCTION: Lan_ShowRegs
    This function is used to display the registers. 
    Except the phy.
*/
void Lan_ShowRegs(const DWORD dwLanBase);

/*
FUNCTION: Lan_ShowMiiRegs, Lan_ShowPhyRegs
    This function is used to display the registers
    in the phy.
*/
void Lan_ShowMiiRegs(const DWORD dwLanBase, const DWORD dwPhyAddr);
#define Lan_ShowPhyRegs Lan_ShowMiiRegs

/*
FUNCTION: Lan_Initialize
  This function should be the first Lan_xxx function called
  It begins to initialize the LAN9118_DATA structure.
  It reads some ID values from the chip.
  It resets the chip.

RETURN VALUE:
    returns TRUE on Success,
    returns FALSE on Failure,
*/
BOOL Lan_Initialize(PLAN9118_DATA const pLan9118Data, const DWORD dwLanBase);

/*
FUNCTION: Lan_InitializePhy
  This function should be called after Lan_InitializeInterrupts.
  Continues to initialize the LAN9118_DATA structure.
  It reads some phy ID values from the phy
  It resets the phy.
  It initializes phy interrupts
RETURN VALUE:
  returns TRUE on Success,
  returns FALSE on Failure
*/
BOOL Lan_InitializePhy(PLAN9118_DATA pLan9118Data, BYTE bPhyAddress);

/*
FUNCTION: Lan_RequestLink
  This function should be called after Lan_InitializePhy
  It begins the interrupt driven process of establishing a link.
  It works with Lan_HandlePhyInterrupt and requires 
  Lan_HandlePhyInterrupt to be called with in the interrupt 
    handler when PHY_INT bit is set in the INT_STS register.
*/
void Lan_RequestLink(
    PLAN9118_DATA pLan9118Data,
    DWORD dwLinkRequest);

/*
FUNCTION: Lan_HandlePhyInterrupt
  This function should be called inside the interrupt handler when
    the PHY_INT bit is set in the INT_STS register.
  This function will handle and clear that interrupt status.
  It is used mainly in managing the link state.
*/
void Lan_HandlePhyInterrupt(PLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_EstablishLink
  This function similar to Lan_RequestLink except that it is
  not interrupt driven. This function will not return until 
  either the link is established or link can not be established.
RETURN VALUE:
    returns TRUE on success.
    returns FALSE on failure.
*/
BOOL Lan_EstablishLink(PLAN9118_DATA pLan9118Data, const DWORD dwLinkRequest, const BOOLEAN fFlowControl);

/*
FUNCTION: Lan_GetLinkMode
  Gets the link mode from the PHY,
  Stores it in LAN9118_DATA and also returns it.
RETURN VALUE:
  one of the following link modes
    LINK_NO_LINK
    LINK_10MPS_HALF
    LINK_10MPS_FULL
    LINK_100MPS_HALF
    LINK_100MPS_FULL
*/
DWORD Lan_GetLinkMode(const LAN9118_DATA * const pLan9118Data);


/*
FUNCTION: Lan_GetAutoMdix
  Gets the Auto Mdix from the PHY,
  Stores it in LAN9118_DATA and also returns it.
RETURN VALUE:
  one of the following link modes
    OVERRIDE_DISABLE_STAIGHT
    OVERRIDE_DISABLE_CROSSOVER
    OVERRIDE_ENABLE_AMDIX
    STRAP_CONTROLL
*/
//WORD Lan_GetAutoMdixSts(const LAN9118_DATA * const pLan9118Data);

/*
FUNCTION: Lan_SetAuotMdix
  
*/
void Lan_SetAutoMdixSts(PLAN9118_DATA pLan9118Data);



/*
FUNCTION: Lan_EnableInterrupt
  Enables bits in INT_EN according to the set bits in dwMask
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_EnableInterrupt(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask);

/*
FUNCTION: Lan_DisableInterrupt
  Disables bits in INT_EN according to the set bits in dwMask
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_DisableInterrupt(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask);

/*
FUNCTION: Lan_GetInterruptStatus
  Reads and returns the value in the INT_STS register.
*/
DWORD Lan_GetInterruptStatus(const LAN9118_DATA * const pLan9118Data);

/*
FUNCTION: Lan_ClearInterruptStatus
  Clears the bits in INT_STS according to the bits set in dwMask
*/
void Lan_ClearInterruptStatus(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask);

/*
FUNCTION: Lan_EnableIRQ
  Sets IRQ_EN in the INT_CFG registers.
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_EnableIRQ(const LAN9118_DATA * const pLan9118Data);

/*
FUNCTION: Lan_DisableIRQ
  Clears IRQ_EN in the INT_CFG registers.
  WARNING this has thread sychronization issues. Use with caution.
*/
void Lan_DisableIRQ(const LAN9118_DATA * const pLan9118Data);

/*
FUNCTION: Lan_InitializeInterrupts
  Should be called after Lan_Initialize
  Should be called before the ISR is registered.
*/
void Lan_InitializeInterrupts(LAN9118_DATA * const pLan9118Data, DWORD dwIntCfg);

/*
FUNCTION: Lan_EnableSoftwareInterrupt
  Clears a flag in the LAN9118_DATA structure
  Sets the SW_INT_EN bit of the INT_EN register
  WARNING this has thread sychronization issues. Use with caution.
*/
void Lan_EnableSoftwareInterrupt(PLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_HandleSoftwareInterrupt
  Disables the SW_INT_EN bit of the INT_EN register,
  Clears the SW_INT in the INT_STS,
  Sets a flag in the LAN9118_DATA structure
*/
void Lan_HandleSoftwareInterrupt(PLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_IsSoftwareInterruptSignaled
  returns the set/cleared state of the flag used in
    Lan_EnableSoftwareInterrupt
    Lan_HandleSoftwareInterrupt
*/
BOOLEAN Lan_IsSoftwareInterruptSignaled(const LAN9118_DATA * const pLan9118Data);

/*
FUNCTION: Lan_SetMacAddress, Lan_GetMacAddress
  gets/sets the Mac Address
*/
void Lan_SetMacAddress(PLAN9118_DATA const pLan9118Data, const DWORD dwHigh16, const DWORD dwLow32);
void Lan_GetMacAddress(PLAN9118_DATA const pLan9118Data, DWORD * const dwHigh16,DWORD * const dwLow32);

/*
FUNCTION: Lan_InitializeTx
  Prepares the LAN9118 for transmission of packets
  must be called before 
    Lan_StartTx
    Lan_SendTestPacket
    Lan_SendPacketPIO
    Lan_CompleteTx
*/
void Lan_InitializeTx(PLAN9118_DATA const pLan9118Data);

/*
FUNCTION: Lan_StartTx
  Writes the initial 2 dwords to the tx data fifo
  Must first call Lan_InitializeTx
*/
void Lan_StartTx(const LAN9118_DATA * const pLan9118Data, const DWORD dwTxCmdA, const DWORD dwTxCmdB);

/*
FUNCTION: Lan_SendTestPacket
  Sends a dummy packet out on the ethernet line.
  Must first call Lan_InitializeTx
*/
void Lan_SendTestPacket(CPCLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_SendPacketPIO
  Sends a specified packet out on the ethernet line.
  Must first call Lan_InitializeTx
  WARNING: wPacketTag must not be 0. Zero is reserved.
*/
void Lan_SendPacketPIO(CPCLAN9118_DATA pLan9118Data, const WORD wPacketTag, const WORD wPacketLength, BYTE * pbPacketData);

/*
FUNCTION: Lan_CompleteTx
  Gets the Status DWORD of a previous transmission from the TX status FIFO
  If the TX Status FIFO is empty as indicated by TX_FIFO_INF then this
    function will return 0
*/
DWORD Lan_CompleteTx(CPCLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_GetTxDataFreeSpace
  Gets the free space available in the TX fifo
*/
DWORD Lan_GetTxDataFreeSpace(CPCLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_GetTxStatusCount
  Gets the number of TX completion status' available on the TX_STATUS_FIFO
  These can be read from Lan_CompleteTx
*/
DWORD Lan_GetTxStatusCount(CPCLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_InitializeRx
  Prepares the LAN9118 for reception of packets
  Must be called After Lan_InitializeInterrupts
*/
void Lan_InitializeRx(CPCLAN9118_DATA pLan9118Data, const DWORD dwRxCfg,  const DWORD dw);

/*
FUNCTION: Lan_PopRxStatus
  If an Rx Status DWORD is available it will return it.
*/
DWORD Lan_PopRxStatus(CPCLAN9118_DATA pLan9118Data);

/*
FUNCTION: Lan_Wakeup9118
  If an Rx Status DWORD is available it will return it.
*/
BOOL Lan_Wakeup9118(CPCLAN9118_DATA pLan9118Data);

BOOLEAN Lan_AutoNegotiate(const LAN9118_DATA * const pLan9118Data);

BOOLEAN Phy_Reset(const LAN9118_DATA * const pLan9118Data);

DWORD Phy_LBT_GetTxStatus(const LAN9118_DATA * const pLan9118Data);

DWORD Phy_LBT_GetRxStatus(const LAN9118_DATA * const pLan9118Data);

BOOLEAN Phy_CheckLoopBackPacket(LAN9118_DATA * const pLan9118Data);

BOOLEAN Phy_LoopBackTest(LAN9118_DATA * const pLan9118Data);
/*
 ****************************************************************************
 ****************************************************************************
 *  TX/RX FIFO Port Register (Direct Address)
 *  Offset (from Base Address)
 *  and bit definitions
 ****************************************************************************
 ****************************************************************************
 */
#define RX_DATA_FIFO_PORT   (0x00UL)
#define TX_DATA_FIFO_PORT   (0x20UL)
    #define TX_CMD_A_INT_ON_COMP_       (0x80000000UL)
    #define TX_CMD_A_INT_BUF_END_ALGN_  (0x03000000UL)
    #define TX_CMD_A_INT_4_BYTE_ALGN_   (0x00000000UL)
    #define TX_CMD_A_INT_16_BYTE_ALGN_  (0x01000000UL)
    #define TX_CMD_A_INT_32_BYTE_ALGN_  (0x02000000UL)
    #define TX_CMD_A_INT_DATA_OFFSET_   (0x001F0000UL)
    #define TX_CMD_A_INT_FIRST_SEG_     (0x00002000UL)
    #define TX_CMD_A_INT_LAST_SEG_      (0x00001000UL)
    #define TX_CMD_A_BUF_SIZE_          (0x000007FFUL)

    #define TX_CMD_B_PKT_TAG_           (0xFFFF0000UL)
    #define TX_CMD_B_ADD_CRC_DISABLE_   (0x00002000UL)
    #define TX_CMD_B_DISABLE_PADDING_   (0x00001000UL)
    #define TX_CMD_B_PKT_BYTE_LENGTH_   (0x000007FFUL)

/*** (nw)  suspect these are wrong, try V1.2 addresses
#define RX_STSTUS_FIFO_PORT (0x3CUL)
    // NOTE: the status definitions in lan.h so that we can
    // develop SW based on 12X status format
#define RX_FIFO_PEEK        (0x40UL)
#define TX_STATUS_FIFO_PORT (0x44UL)
#define TX_FIFO_PEEK        (0x48UL)
***/
#define RX_STATUS_FIFO_PORT     (0x00000040UL)
#define RX_STATUS_PACKET_LEN    (0x3FFF0000UL)
#define RX_STS_ES               (0x00008000UL)
#define RX_STS_LENGTH_ERR       (0x00001000UL)
#define RX_STS_MULTICAST        (0x00000400UL)
#define RX_STS_FRAME_TYPE       (0x00000020UL)
#define RX_FIFO_PEEK            (0x00000044UL)

#define TX_STATUS_FIFO_PORT (0x00000048UL)
#define TX_STS_ES           (0x00008000UL)
#define TX_FIFO_PEEK        (0x0000004CUL)

/*
 ****************************************************************************
 ****************************************************************************
 *  Slave Interface Module Control and Status Register (Direct Address)
 *  Offset (from Base Address)
 *  and bit definitions
 ****************************************************************************
 ****************************************************************************
 */

#define ID_REV              (0x50UL)
    #define ID_REV_CHIP_ID_     (0xFFFF0000UL)  // RO   default 0x012X
    #define ID_REV_REV_ID_      (0x0000FFFFUL)  // RO

#define INT_CFG             (0x54UL)
    #define INT_CFG_INT_DEAS_   (0xFF000000UL)  // R/W
    #define INT_CFG_DEAS_CLR_   (0x00004000UL)  // SC
    #define INT_CFG_DEAS_STS_   (0x00002000UL)  // SC
    #define INT_CFG_IRQ_INT_    (0x00001000UL)  // RO
    #define INT_CFG_IRQ_EN_     (0x00000100UL)  // R/W
    #define INT_CFG_IRQ_POL_    (0x00000010UL)  // R/W Not Affected by SW Reset
    #define INT_CFG_IRQ_TYPE_   (0x00000001UL)  // R/W Not Affected by SW Reset
    #define INT_CFG_IRQ_RESERVED_   (0x00FFCEEEUL)  //Reserved bits mask

#define INT_STS             (0x58UL)
    #define INT_STS_SW_INT_     (0x80000000UL)  // R/WC
    #define INT_STS_PHY1_INT_   (0x00080000UL)  // RO
    #define INT_STS_PHY2_INT_   (0x00040000UL)  // RO
    #define INT_STS_TXSTOP_INT_ (0x02000000UL)  // R/WC
    #define INT_STS_RXSTOP_INT_ (0x01000000UL)  // R/WC
    #define INT_STS_RXDFH_INT_  (0x00800000UL)  // R/WC
    #define INT_STS_TX_IOC_     (0x00200000UL)  // R/WC
    #define INT_STS_RXD_INT_    (0x00100000UL)  // R/WC
    #define INT_STS_GPT_INT_    (0x00080000UL)  // R/WC
    #define INT_STS_PME_INT_    (0x00020000UL)  // R/WC
    #define INT_STS_TXSO_       (0x00010000UL)  // R/WC
    #define INT_STS_RWT_        (0x00008000UL)  // R/WC
    #define INT_STS_RXE_        (0x00004000UL)  // R/WC
    #define INT_STS_TXE_        (0x00002000UL)  // R/WC
    #define INT_STS_TDFO_       (0x00000400UL)  // R/WC
    #define INT_STS_TDFA_       (0x00000200UL)  // R/WC
    #define INT_STS_TSFF_       (0x00000100UL)  // R/WC
    #define INT_STS_TSFL_       (0x00000080UL)  // R/WC
    #define INT_STS_RDFO_       (0x00000040UL)  // R/WC
    #define INT_STS_RSFF_       (0x00000010UL)  // R/WC
    #define INT_STS_RSFL_       (0x00000008UL)  // R/WC
    #define INT_STS_GPIO2_INT_  (0x00000004UL)  // R/WC
    #define INT_STS_GPIO1_INT_  (0x00000002UL)  // R/WC
    #define INT_STS_GPIO0_INT_  (0x00000001UL)  // R/WC

#define INT_EN              (0x5CUL)
    #define INT_EN_SW_INT_EN_       (0x80000000UL)  // R/W
    #define INT_EN_PHY2_INT_EN_     (0x00080000UL)  // R/W
    #define INT_EN_PHY1_INT_EN_     (0x00040000UL)  // R/W
    #define INT_EN_TXSTOP_INT_EN_   (0x02000000UL)  // R/W
    #define INT_EN_RXSTOP_INT_EN_   (0x01000000UL)  // R/W
    #define INT_EN_RXDFH_INT_EN_    (0x00800000UL)  // R/W
    #define INT_EN_TIOC_INT_EN_     (0x00200000UL)  // R/W
    #define INT_EN_RXD_INT_EN_      (0x00100000UL)  // R/W
    #define INT_EN_GPT_INT_EN_      (0x00080000UL)  // R/W
    #define INT_EN_PHY_INT_EN_      (0x00040000UL)  // R/W
    #define INT_EN_PME_INT_EN_      (0x00020000UL)  // R/W
    #define INT_EN_TXSO_EN_         (0x00010000UL)  // R/W
    #define INT_EN_RWT_EN_          (0x00008000UL)  // R/W
    #define INT_EN_RXE_EN_          (0x00004000UL)  // R/W
    #define INT_EN_TXE_EN_          (0x00002000UL)  // R/W
    #define INT_EN_TDFO_EN_         (0x00000400UL)  // R/W
    #define INT_EN_TDFA_EN_         (0x00000200UL)  // R/W
    #define INT_EN_TSFF_EN_         (0x00000100UL)  // R/W
    #define INT_EN_TSFL_EN_         (0x00000080UL)  // R/W
    #define INT_EN_RDFO_EN_         (0x00000040UL)  // R/W
    #define INT_EN_RSFF_EN_         (0x00000010UL)  // R/W
    #define INT_EN_RSFL_EN_         (0x00000008UL)  // R/W
    #define INT_EN_GPIO2_INT_       (0x00000004UL)  // R/W
    #define INT_EN_GPIO1_INT_       (0x00000002UL)  // R/W
    #define INT_EN_GPIO0_INT_       (0x00000001UL)  // R/W

#define DMA_CFG             (0x60UL)
    #define DMA_CFG_DRQ1_DEAS_      (0xFF000000UL)  // R/W
    #define DMA_CFG_DMA1_MODE_      (0x00200000UL)  // R/W
    #define DMA_CFG_DMA1_FUNC_      (0x00180000UL)  // R/W
        #define DMA_CFG_DMA1_FUNC_DISABLED_     (0x000000000UL) // R/W
        #define DMA_CFG_DMA1_FUNC_TX_DMA_       (0x000800000UL) // R/W
        #define DMA_CFG_DMA1_FUNC_RX_DMA_       (0x001000000UL) // R/W
    #define DMA_CFG_DRQ1_BUFF_      (0x00040000UL)  // R/W
    #define DMA_CFG_DRQ1_POL_       (0x00020000UL)  // R/W
    #define DMA_CFG_DAK1_POL_       (0x00010000UL)  // R/W
    #define DMA_CFG_DRQ0_DEAS_      (0x0000FF00UL)  // R/W
    #define DMA_CFG_DMA0_MODE_      (0x00000020UL)  // R/W
    #define DMA_CFG_DMA0_FUNC_      (0x00000018UL)  // R/W
        #define DMA_CFG_DMA0_FUNC_FIFO_SEL_     (0x000000000UL) // R/W
        #define DMA_CFG_DMA0_FUNC_TX_DMA_       (0x000000008UL) // R/W
        #define DMA_CFG_DMA0_FUNC_RX_DMA_       (0x000000010UL) // R/W
    #define DMA_CFG_DRQ0_BUFF_      (0x00000004UL)  // R/W
    #define DMA_CFG_DRQ0_POL_       (0x00000002UL)  // R/W
    #define DMA_CFG_DAK0_POL_       (0x00000001UL)  // R/W

#define BYTE_TEST           (0x64UL)    // RO default 0x87654321

#define FIFO_INT            (0x68UL)
    #define FIFO_INT_TX_AVAIL_LEVEL_    (0xFF000000UL)  // R/W
    #define FIFO_INT_TX_STS_LEVEL_      (0x00FF0000UL)  // R/W
    #define FIFO_INT_RX_STS_LEVEL_      (0x000000FFUL)  // R/W

#define RX_CFG              (0x6CUL)
    #define RX_CFG_RX_END_ALGN_     (0xC0000000UL)  // R/W
        #define RX_CFG_RX_END_ALGN4_        (0x00000000UL)  // R/W
        #define RX_CFG_RX_END_ALGN16_       (0x40000000UL)  // R/W
        #define RX_CFG_RX_END_ALGN32_       (0x80000000UL)  // R/W
    #define RX_CFG_RX_DMA_CNT_      (0x0FFF0000UL)  // R/W
    #define RX_CFG_RX_DUMP_         (0x00008000UL)  // R/W
    #define RX_CFG_RXDOFF_          (0x00001F00UL)  // R/W

#define TX_CFG              (0x70UL)
    #define TX_CFG_TXS_DUMP_        (0x00008000UL)  // Self Clearing
    #define TX_CFG_TXD_DUMP_        (0x00004000UL)  // Self Clearing
    #define TX_CFG_TXSAO_           (0x00000004UL)  // R/W
    #define TX_CFG_TX_ON_           (0x00000002UL)  // R/W
    #define TX_CFG_STOP_TX_         (0x00000001UL)  // Self Clearing

#define HW_CFG              (0x74UL)
    #define HW_CFG_AMDIX_EN_STRAP_STS_  (0x01000000UL)  // R/O
    #define HW_CFG_MBO_             (0x00100000UL)  // R/W (Must Be ONE)
    #define HW_CFG_TX_FIF_SZ_       (0x000F0000UL)  // R/W
    #define HW_CFG_PHY_CLK_SEL_     (0x00000060UL)  // R/W
    #define   HW_CFG_PHY_CLK_SEL_INT_PHY_   (0x00000000UL)  // R/W
    #define   HW_CFG_PHY_CLK_SEL_EXT_PHY_   (0x00000020UL)  // R/W
    #define   HW_CFG_PHY_CLK_SEL_CLK_DIS_   (0x00000040UL)  // R/W
    #define HW_CFG_SMI_SEL_         (0x00000010UL)  // R/W
    #define HW_CFG_EXT_PHY_DET_     (0x00000008UL)  // RO
    #define HW_CFG_EXT_PHY_EN_      (0x00000004UL)  // R/W
    #define HW_CFG_SRST_TO_         (0x00000002UL)  // RO
    #define HW_CFG_SRST_            (0x00000001UL)  // Self Clearing

#define RX_DP_CTL           (0x78UL)
    #define RX_DP_CTL_FFWD_BUSY_    (0x80000000UL)  // R/?

#define RX_FIFO_INF         (0x7CUL)
    #define RX_FIFO_INF_RXSUSED_    (0x00FF0000UL)  // RO
    #define RX_FIFO_INF_RXDUSED_    (0x0000FFFFUL)  // RO

#define TX_FIFO_INF         (0x80UL)
    #define TX_FIFO_INF_TSFREE_     (0x00FF0000UL)  // RO for PAS V.1.3
    #define TX_FIFO_INF_TSUSED_     (0x00FF0000UL)  // RO for PAS V.1.4
    #define TX_FIFO_INF_TDFREE_     (0x0000FFFFUL)  // RO

#define PMT_CTRL            (0x84UL)
    #define PMT_CTRL_PM_MODE_           (0x00003000UL)  // Self Clearing
        #define PMT_CTRL_PM_MODE_GP_        (0x00003000UL)  // Self Clearing
        #define PMT_CTRL_PM_MODE_ED_        (0x00002000UL)  // Self Clearing
        #define PMT_CTRL_PM_MODE_WOL_       (0x00001000UL)  // Self Clearing
    #define PMT_CTRL_PHY_RST_           (0x00000400UL)  // Self Clearing
    #define PMT_CTRL_WOL_EN_            (0x00000200UL)  // R/W
    #define PMT_CTRL_ED_EN_             (0x00000100UL)  // R/W
    #define PMT_CTRL_PME_TYPE_          (0x00000040UL)  // R/W Not Affected by SW Reset
    #define PMT_CTRL_WUPS_              (0x00000030UL)  // R/WC
        #define PMT_CTRL_WUPS_NOWAKE_       (0x00000000UL)  // R/WC
        #define PMT_CTRL_WUPS_ED_           (0x00000010UL)  // R/WC
        #define PMT_CTRL_WUPS_WOL_          (0x00000020UL)  // R/WC
        #define PMT_CTRL_WUPS_MULTI_        (0x00000030UL)  // R/WC
    #define PMT_CTRL_PME_IND_       (0x00000008UL)  // R/W
    #define PMT_CTRL_PME_POL_       (0x00000004UL)  // R/W
    #define PMT_CTRL_PME_EN_        (0x00000002UL)  // R/W Not Affected by SW Reset
    #define PMT_CTRL_READY_         (0x00000001UL)  // RO

#define GPIO_CFG            (0x88UL)
    #define GPIO_CFG_LED3_EN_       (0x40000000UL)  // R/W
    #define GPIO_CFG_LED2_EN_       (0x20000000UL)  // R/W
    #define GPIO_CFG_LED1_EN_       (0x10000000UL)  // R/W
    #define GPIO_CFG_GPIO2_INT_POL_ (0x04000000UL)  // R/W
    #define GPIO_CFG_GPIO1_INT_POL_ (0x02000000UL)  // R/W
    #define GPIO_CFG_GPIO0_INT_POL_ (0x01000000UL)  // R/W
    #define GPIO_CFG_EEPR_EN_       (0x00700000UL)  // R/W
    #define GPIO_CFG_GPIOBUF2_      (0x00040000UL)  // R/W
    #define GPIO_CFG_GPIOBUF1_      (0x00020000UL)  // R/W
    #define GPIO_CFG_GPIOBUF0_      (0x00010000UL)  // R/W
    #define GPIO_CFG_GPIODIR2_      (0x00000400UL)  // R/W
    #define GPIO_CFG_GPIODIR1_      (0x00000200UL)  // R/W
    #define GPIO_CFG_GPIODIR0_      (0x00000100UL)  // R/W
    #define GPIO_CFG_GPIOD4_        (0x00000010UL)  // R/W
    #define GPIO_CFG_GPIOD3_        (0x00000008UL)  // R/W
    #define GPIO_CFG_GPIOD2_        (0x00000004UL)  // R/W
    #define GPIO_CFG_GPIOD1_        (0x00000002UL)  // R/W
    #define GPIO_CFG_GPIOD0_        (0x00000001UL)  // R/W

#define GPT_CFG             (0x8CUL)
    #define GPT_CFG_TIMER_EN_       (0x20000000UL)  // R/W
    #define GPT_CFG_GPT_LOAD_       (0x0000FFFFUL)  // R/W

#define GPT_CNT             (0x90UL)
    #define GPT_CNT_GPT_CNT_        (0x0000FFFFUL)  // RO

#define WORD_SWAP               (0x98UL)    // R/W Not Affected by SW Reset

#define FREE_RUN            (0x9CUL)    // RO

#define RX_DROP             (0xA0UL)    // R/WC

#define MAC_CSR_CMD         (0xA4UL)
    #define MAC_CSR_CMD_CSR_BUSY_   (0x80000000UL)  // Self Clearing
    #define MAC_CSR_CMD_R_NOT_W_    (0x40000000UL)  // R/W
    #define MAC_CSR_CMD_CSR_ADDR_   (0x000000FFUL)  // R/W

#define MAC_CSR_DATA        (0xA8UL)    // R/W

#define DEFAULT_AFC_CFG             (0x006E374FUL)

#define AFC_CFG             (0xACUL)
    #define AFC_CFG_AFC_HI_         (0x00FF0000UL)  // R/W
    #define AFC_CFG_AFC_LO_         (0x0000FF00UL)  // R/W
    #define AFC_CFG_BACK_DUR_       (0x000000F0UL)  // R/W
    #define AFC_CFG_FCMULT_         (0x00000008UL)  // R/W
    #define AFC_CFG_FCBRD_          (0x00000004UL)  // R/W
    #define AFC_CFG_FCADD_          (0x00000002UL)  // R/W
    #define AFC_CFG_FCANY_          (0x00000001UL)  // R/W

// EEPROM registers are different on chip ID 9311
#define IS_9311() ((pAdapter->lan9118_data.dwIdRev & 0xFFFF0000) == 0x93110000) 

#define E2P_CMD             (IS_9311() ? (0x1B4UL) : (0xB0UL))
    #define E2P_CMD_EPC_BUSY_       (0x80000000UL)  // Self Clearing
    #define E2P_CMD_EPC_CMD_        (0x70000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_READ_   (0x00000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_EWDS_   (0x10000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_EWEN_   (0x20000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_WRITE_  (0x30000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_WRAL_   (0x40000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_ERASE_  (0x50000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_ERAL_   (0x60000000UL)  // R/W
        #define E2P_CMD_EPC_CMD_RELOAD_ (0x70000000UL)  // R/W
    #define E2P_CMD_EPC_TIMEOUT_    (0x08000000UL)  // RO
    #define E2P_CMD_E2P_PROG_GO_    (IS_9311() ? (0x00020000UL) : (0x00000200UL)) // WO
	#define E2P_CMD_E2P_PROG_DONE_  (IS_9311() ? (0x00010000UL) : (0x00000100UL)) // RO
    #define E2P_CMD_EPC_ADDR_       (0x000000FFUL)  // R/W

#define E2P_DATA            (IS_9311() ? (0x1B8UL) : (0xB4UL))
    #define E2P_DATA_EEPROM_DATA_   (0x000000FFUL)  // R/W

#define TEST_REG_A          (0xC0UL)
    #define TEST_REG_A_FR_CNT_BYP_  (0x00000008UL)  // R/W
    #define TEST_REG_A_PME50M_BYP_  (0x00000004UL)  // R/W
    #define TEST_REG_A_PULSE_BYP_   (0x00000002UL)  // R/W
    #define TEST_REG_A_PS_BYP_      (0x00000001UL)  // R/W

#define LAN_REGISTER_EXTENT         (0x00000100UL)


/*
 ****************************************************************************
 ****************************************************************************
 *  MAC Control and Status Register (Indirect Address)
 *  Offset (through the MAC_CSR CMD and DATA port)
 ****************************************************************************
 ****************************************************************************
 *
 */
#define MAC_CR              (0x01UL)    // R/W

    /* MAC_CR - MAC Control Register */
    #define MAC_CR_RXALL_       (0x80000000UL)
    #define MAC_CR_HBDIS_       (0x10000000UL)
    #define MAC_CR_RCVOWN_      (0x00800000UL)
    #define MAC_CR_LOOPBK_      (0x00200000UL)
    #define MAC_CR_FDPX_        (0x00100000UL)
    #define MAC_CR_MCPAS_       (0x00080000UL)
    #define MAC_CR_PRMS_        (0x00040000UL)
    #define MAC_CR_INVFILT_     (0x00020000UL)
    #define MAC_CR_HFILT_       (0x00008000UL)
    #define MAC_CR_HPFILT_      (0x00002000UL)
    #define MAC_CR_LCOLL_       (0x00001000UL)
    #define MAC_CR_BCAST_       (0x00000800UL)
    #define MAC_CR_DISRTY_      (0x00000400UL)
    #define MAC_CR_PADSTR_      (0x00000100UL)
    #define MAC_CR_BOLMT_MASK_  (0x000000C0UL)
    #define MAC_CR_DFCHK_       (0x00000020UL)
    #define MAC_CR_TXEN_        (0x00000008UL)
    #define MAC_CR_RXEN_        (0x00000004UL)

#define ADDRH               (0x02UL)    // R/W mask 0x0000FFFFUL
#define ADDRL               (0x03UL)    // R/W mask 0xFFFFFFFFUL
#define HASHH               (0x04UL)    // R/W
#define HASHL               (0x05UL)    // R/W

#define MII_ACC             (0x06UL)    // R/W
    #define MII_ACC_PHY_ADDR_   (0x0000F800UL)
    #define MII_ACC_MIIRINDA_   (0x000007C0UL)
    #define MII_ACC_MII_WRITE_  (0x00000002UL)
    #define MII_ACC_MII_BUSY_   (0x00000001UL)

#define MII_DATA            (0x07UL)    // R/W mask 0x0000FFFFUL

#define FLOW                (0x08UL)    // R/W
    #define FLOW_FCPT_          (0xFFFF0000UL)
    #define FLOW_FCPASS_        (0x00000004UL)
    #define FLOW_FCEN_          (0x00000002UL)
    #define FLOW_FCBSY_         (0x00000001UL)

#define VLAN1               (0x09UL)    // R/W mask 0x0000FFFFUL
#define VLAN2               (0x0AUL)    // R/W mask 0x0000FFFFUL

#define WUFF                (0x0BUL)    // WO
    #define FILTER3_COMMAND     (0x0F000000UL)
    #define FILTER2_COMMAND     (0x000F0000UL)
    #define FILTER1_COMMAND     (0x00000F00UL)
    #define FILTER0_COMMAND     (0x0000000FUL)
        #define FILTER3_ADDR_TYPE     (0x04000000UL)
        #define FILTER3_ENABLE     (0x01000000UL)
        #define FILTER2_ADDR_TYPE     (0x00040000UL)
        #define FILTER2_ENABLE     (0x00010000UL)
        #define FILTER1_ADDR_TYPE     (0x00000400UL)
        #define FILTER1_ENABLE     (0x00000100UL)
        #define FILTER0_ADDR_TYPE     (0x00000004UL)
        #define FILTER0_ENABLE     (0x00000001UL)
    #define FILTER3_OFFSET      (0xFF000000UL)
    #define FILTER2_OFFSET      (0x00FF0000UL)
    #define FILTER1_OFFSET      (0x0000FF00UL)
    #define FILTER0_OFFSET      (0x000000FFUL)

    #define FILTER3_CRC         (0xFFFF0000UL)
    #define FILTER2_CRC         (0x0000FFFFUL)
    #define FILTER1_CRC         (0xFFFF0000UL)
    #define FILTER0_CRC         (0x0000FFFFUL)

#define WUCSR               (0x0CUL)    // R/W
    #define WUCSR_GUE_          (0x00000200UL)
    #define WUCSR_WUFR_         (0x00000040UL)
    #define WUCSR_MPR_          (0x00000020UL)
    #define WUCSR_WAKE_EN_      (0x00000004UL)
    #define WUCSR_MPEN_         (0x00000002UL)


/*
 ****************************************************************************
 *  Chip Specific MII Defines
 ****************************************************************************
 *
 *  Phy register offsets and bit definitions
 *
 */
#define LAN9118_PHY_ID  (0x00C0001CUL)

#define PHY_BCR     ((DWORD)0U)
#define PHY_BCR_RESET_              ((WORD)0x8000U)
#define PHY_BCR_SPEED_SELECT_       ((WORD)0x2000U)
#define PHY_BCR_AUTO_NEG_ENABLE_    ((WORD)0x1000U)
#define PHY_BCR_POWER_DOWN_         ((WORD)0x0800U)
#define PHY_BCR_RESTART_AUTO_NEG_   ((WORD)0x0200U)
#define PHY_BCR_DUPLEX_MODE_        ((WORD)0x0100U)

#define PHY_BSR     ((DWORD)1U)
    #define PHY_BSR_LINK_STATUS_    ((WORD)0x0004U)
    #define PHY_BSR_REMOTE_FAULT_   ((WORD)0x0010U)
    #define PHY_BSR_AUTO_NEG_COMP_  ((WORD)0x0020U)
    #define PHY_BSR_ANEG_ABILITY_   ((WORD)0x0008U)

#define PHY_ID_1    ((DWORD)2U)
#define PHY_ID_2    ((DWORD)3U)

#define PHY_ANEG_ADV    ((DWORD)4U)
#define PHY_ANEG_ADV_PAUSE_OP_      ((WORD)0x0C00)
#define PHY_ANEG_ADV_ASYM_PAUSE_    ((WORD)0x0800)
#define PHY_ANEG_ADV_SYM_PAUSE_     ((WORD)0x0400)
#define PHY_ANEG_ADV_10H_           ((WORD)0x0020)
#define PHY_ANEG_ADV_10F_           ((WORD)0x0040)
#define PHY_ANEG_ADV_100H_          ((WORD)0x0080)
#define PHY_ANEG_ADV_100F_          ((WORD)0x0100)
#define PHY_ANEG_ADV_SPEED_         ((WORD)0x01E0)

#define PHY_ANEG_LPA    ((DWORD)5U)
#define PHY_ANEG_LPA_100FDX_    ((WORD)0x0100)
#define PHY_ANEG_LPA_100HDX_    ((WORD)0x0080)
#define PHY_ANEG_LPA_10FDX_     ((WORD)0x0040)
#define PHY_ANEG_LPA_10HDX_     ((WORD)0x0020)

#define PHY_ANEG_EXP    ((DWORD)6U)
#define PHY_ANEG_EXP_PDF_           ((WORD)0x0010)
#define PHY_ANEG_EXP_LPNPA_         ((WORD)0x0008)
#define PHY_ANEG_EXP_NPA_           ((WORD)0x0004)
#define PHY_ANEG_EXP_PAGE_RX_       ((WORD)0x0002)
#define PHY_ANEG_EXP_LPANEG_ABLE_   ((WORD)0x0001)

#define PHY_MODE_CTRL_STS       ((DWORD)17) // Mode Control/Status Register
    #define MODE_CTRL_STS_EDPWRDOWN_    ((WORD)0x2000U)

#define SPECIAL_CTRL_STS                ((DWORD)27)
    #define SPECIAL_CTRL_STS_OVRRD_AMDIX_   ((WORD)0x8000U)
    #define SPECIAL_CTRL_STS_AMDIX_ENABLE_  ((WORD)0x4000U)
    #define SPECIAL_CTRL_STS_AMDIX_STATE_   ((WORD)0x2000U)

#define PHY_INT_SRC         ((DWORD)29)
#define PHY_INT_SRC_ENERGY_ON_          ((WORD)0x0080U)
#define PHY_INT_SRC_ANEG_COMP_          ((WORD)0x0040U)
#define PHY_INT_SRC_REMOTE_FAULT_       ((WORD)0x0020U)
#define PHY_INT_SRC_LINK_DOWN_          ((WORD)0x0010U)

#define PHY_INT_MASK        ((DWORD)30)
#define PHY_INT_MASK_ENERGY_ON_     ((WORD)0x0080U)
#define PHY_INT_MASK_ANEG_COMP_     ((WORD)0x0040U)
#define PHY_INT_MASK_REMOTE_FAULT_  ((WORD)0x0020U)
#define PHY_INT_MASK_LINK_DOWN_     ((WORD)0x0010U)

#define PHY_SPECIAL         ((DWORD)31)
#define PHY_SPECIAL_SPD_    ((WORD)0x001CU)
#define PHY_SPECIAL_SPD_10HALF_     ((WORD)0x0004U)
#define PHY_SPECIAL_SPD_10FULL_     ((WORD)0x0014U)
#define PHY_SPECIAL_SPD_100HALF_    ((WORD)0x0008U)
#define PHY_SPECIAL_SPD_100FULL_    ((WORD)0x0018U)

#endif

