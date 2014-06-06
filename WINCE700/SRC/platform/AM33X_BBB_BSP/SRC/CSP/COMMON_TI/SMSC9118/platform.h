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
     
    File name   : gumstix.h
    Description : smsc9118 driver 

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
/*lint -save*/
/*lint -e14 -e43 -e46 -e123 -e427 -e537 -e620 -e652 -e659 -e683*/
/*lint -e726 -e760 -e761 -e762 -e763 -e767 -e773 -e783 -e793 -e806 -e828 -e912*/
/*lint -e935 -e937 -e950 -e955 -e956 -e957 -e958 -e959 -e960 -e961 -e962*/
/*lint -e973 -e1916 */
//#include <bulverde_base_regs.h>
//#include <bulverde_gpio.h>
//#include <bulverde_dma.h>
//#include <bulverde_intr.h>
//#include <bulverde_memctrl.h>
//#include <bulverde_ost.h>
/*lint -restore*/

//#define   SMSC_DIRECT_INTR    // GPIO Edge-Triggered Direct Interrupt (SMSC9118.IRQ --> GPIO, no intervening CPLD)

#define CACHE_LINE_BYTES                32UL    //Cache size = 32bytes

#define RX_END_ALIGNMENT                (CACHE_LINE_BYTES)
#define TX_BURST_DMA_END_ALIGNMENT      (CACHE_LINE_BYTES)
#define INT_DEAS                        0x16000000UL

#define DMAC_REGBASE                    (0)
#define DMAC_REGSIZE                    (0x400)

#define DCSR_RUN        (1U<<31)
#define DCSR_NODESC     (1U<<30)
#define DCSR_STOPIRQEN  (1U<<29)
#define DCSR_REQPEND    (1U<<8)
#define DCSR_STOPSTATE  (1U<<3)
#define DCSR_ENDINTR    (1U<<2)
#define DCSR_STARTINTR  (1U<<1)
#define DCSR_BUSERR     (1U<<0)

#define DCMD_INCSRCADDR (1UL<<31)
#define DCMD_INCTRGADDR (1UL<<30)
#define DCMD_FLOWSRC    (1UL<<29)
#define DCMD_FLOWTRG    (1UL<<28)
#define DCMD_STARTIRQEN (1UL<<22)
#define DCMD_ENDIRQEN   (1UL<<21)
#define DCMD_WORDSWAP   (1UL<<18)
#define DCMD_BURST8     (1UL<<16)
#define DCMD_BURST16    (2UL<<16)
#define DCMD_BURST32    (3UL<<16)
#define DCMD_WIDTH      (1UL<<14)
#define DCMD_WIDTH2     (2UL<<14)
#define DCMD_WIDTH4     (3UL<<14)
#define DCMD_LENGTH     (0x1FFFUL)

//DMA Transfer structure
typedef struct _DMA_XFER 
{
    DWORD DMABaseVA;
    DWORD dwLanReg;
    DWORD *pdwBuf;  //PA
    DWORD dwDmaCh;
    DWORD dwDwCnt;

    BOOLEAN fMemWr;
    BOOLEAN fClBurst;
    BOOLEAN fLanAddrInc;
    BOOLEAN fMemAddrInc;
    BOOLEAN fReserve;

    // Make Lint happy
    BYTE    padding[3];
} DMA_XFER, *PDMA_XFER;

#define DMA_INVALID_COUNT (0xFFFFFFFFUL)
#define DMA_RX_CH   2UL
#define DMA_TX_CH   3UL

BOOL DmaInitialize(NDIS_HANDLE hMiniportAdapterContext);
BOOL DmaStartXfer(const DMA_XFER * const dmaXfer);
DWORD DmaGetDwCnt(const DMA_XFER * const dmaXfer, const DWORD dwDmaCh);
VOID DmaDisable(const DMA_XFER * const dmaXfer, const DWORD dwDmaCh);
VOID DmaComplete(const DMA_XFER * const dmaXfer, const DWORD dwDmaCh);
VOID BufferCacheFlush(const void * const pStartAddress, const DWORD dwLengthInBytes);
void PlatformInitialize(void);
void PlatformDisplayInfo(void);
void PlatformSetBusWidth(const DWORD dwBusWidth);
void PlatformSetBusTiming(const DWORD dwChipIdReg);
void PlatformEnableGpioInterrupt(void);
