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
     
    File name   : lan9118.c 
    Description : lan9118 driver 

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
#ifndef OS_LINUX
#pragma warning(disable: 4127 4201 4214 4115 4100 4514)
#endif

#include "lan9118.h"

/* csAccessMacReg and csAccessPhyReg are for CRITICAL_SECTION in CE.NET
 * Those are for exclusive access
 * It is multi-thread safe by APIs. (Actually for multi-thread APIs)
 */
/*lint -save*/
/*lint -e956 */
CRITICAL_SECTION    csAccessMacReg, csAccessPhyReg;
/*lint -restore*/

__inline static BOOL Lan_MacNotBusy(const DWORD dwLanBase);

#if 0
extern void DumpSIMRegs(LAN9118_DATA * const pLan9118Data);
extern void DumpPHYRegs(LAN9118_DATA * const pLan9118Data);
#endif

/*
FUNCTION: Lan_WriteTxFifo
    This function is used to write a buffer to the
    Tx Fifo in PIO mode.
    This function is only intended to be called 
      from with in other Lan_xxx functions.
*/
void Lan_WriteTxFifo(PLAN9118_DATA const pLan9118Data, const DWORD dwLanBase, const DWORD * const pdwBuf, const DWORD dwDwordCount)
{    
    WriteFifo(dwLanBase, TX_DATA_FIFO_PORT, pdwBuf, dwDwordCount);
}

/*
FUNCTION: Lan_ReadRxFifo
    This function is used to read a buffer to the 
    Rx Fifo in PIO mode.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline void Lan_ReadRxFifo(const DWORD dwLanBase, DWORD * const pdwBuf, const DWORD dwDwordCount)
{
    ReadFifo(dwLanBase, RX_DATA_FIFO_PORT, pdwBuf, dwDwordCount);
}

/*
 ****************************************************************************
 *  Indirect (MAC) Registers and Reader/Writers
 ****************************************************************************
 *
 *  Only DWORD accesses are valid on 12X
 */
__inline static BOOL Lan_MacNotBusy(const DWORD dwLanBase)
{
    int i=0;
    // wait for MAC not busy, w/ timeout
    for(i=0;i<40;i++)
    {
        if((GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)==(0UL)) {
            return TRUE;
        }
    }
    SMSC_WARNING1("timeout waiting for MAC not BUSY. MAC_CSR_CMD = 0x%08lX\n",
        (DWORD)(*(volatile DWORD *)(dwLanBase + MAC_CSR_CMD)));
    return FALSE;
}


/*
FUNCTION: Lan_GetMacRegDW
    This function is used to read a Mac Register.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline DWORD Lan_GetMacRegDW(const DWORD dwLanBase, const DWORD dwOffset)
{
    DWORD   dwRet;

    // wait until not busy, w/ timeout
    if (GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
    {
        SMSC_WARNING0("LanGetMacRegDW() failed MAC already busy at entry\n");
        return 0xFFFFFFFFUL;
    }

    EnterCriticalSection(&csAccessMacReg);
    // send the MAC Cmd w/ offset
    SetRegDW(dwLanBase, MAC_CSR_CMD, 
        ((dwOffset & MAC_CSR_CMD_CSR_ADDR_) | MAC_CSR_CMD_CSR_BUSY_ | MAC_CSR_CMD_R_NOT_W_));

    // wait for the read to happen, w/ timeout
    if (!Lan_MacNotBusy(dwLanBase))
    {
        SMSC_WARNING0("LanGetMacRegDW() failed waiting for MAC not busy after read\n");
        dwRet = 0xFFFFFFFFUL;
    }
    else
    {
        // finally, return the read data
        dwRet = GetRegDW(dwLanBase, MAC_CSR_DATA);
    }

    LeaveCriticalSection(&csAccessMacReg);
    return dwRet;
}

/*
FUNCTION: Lan_SetMacRegDW
    This function is used to write a Mac register.
    This function is only intended to be called
      from with in other Lan_xxx functions.
*/
__inline void Lan_SetMacRegDW(const DWORD dwLanBase, const DWORD dwOffset, const DWORD dwVal)
{
    if (GetRegDW(dwLanBase, MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
    {
        SMSC_WARNING0("LanSetMacRegDW() failed MAC already busy at entry\n");
        return;
    }

    EnterCriticalSection(&csAccessMacReg);

    // send the data to write
    SetRegDW(dwLanBase, MAC_CSR_DATA, dwVal);

    // do the actual write
    SetRegDW(dwLanBase, MAC_CSR_CMD, 
        ((dwOffset & MAC_CSR_CMD_CSR_ADDR_) | MAC_CSR_CMD_CSR_BUSY_));

    // wait for the write to complete, w/ timeout
    if (!Lan_MacNotBusy(dwLanBase))
    {
        SMSC_WARNING0("LanSetMacRegDW() failed waiting for MAC not busy after write\n");
    }

    LeaveCriticalSection(&csAccessMacReg);
}

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
    WORD    *wValue)
{
    DWORD dwAddr;
    BOOL  wRet = (BOOL)TRUE;
    int i=0;

    // confirm MII not busy
    if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
    {
        SMSC_WARNING0("MII is busy in MiiGetReg???\r\n");
        return (BOOL)FALSE;
    }

    EnterCriticalSection(&csAccessPhyReg);

    // set the address, index & direction (read from PHY)
    dwAddr = ((dwPhyAddress & 0x1FUL)<<11) | ((dwMiiIndex & 0x1FUL)<<6);
    Lan_SetMacRegDW(dwLanBase, MII_ACC, dwAddr);

    // wait for read to complete w/ timeout
    for(i=0;i<100;i++) 
    {
        // see if MII is finished yet
        if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
        {
            // get the read data from the MAC & return i
            *wValue = ((WORD)Lan_GetMacRegDW(dwLanBase, MII_DATA));
            break;
        }
    }
    if (i == 100) 
    {
        SMSC_WARNING0("timeout waiting for MII write to finish\n");
        wRet = (BOOL)FALSE;
    }

    LeaveCriticalSection(&csAccessPhyReg);

    return wRet;
    
}

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
    const WORD wVal)
{
    DWORD dwAddr;
    int i=0;

    // confirm MII not busy
    if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
    {
        SMSC_WARNING0("MII is busy in MiiGetReg???\n");
        return (BOOL)FALSE;
    }

    EnterCriticalSection(&csAccessPhyReg);

    // put the data to write in the MAC
    Lan_SetMacRegDW(dwLanBase, MII_DATA, (DWORD)wVal);

    // set the address, index & direction (write to PHY)
    dwAddr = ((dwPhyAddress & 0x1FUL)<<11) | ((dwMiiIndex & 0x1FUL)<<6) | MII_ACC_MII_WRITE_;
    Lan_SetMacRegDW(dwLanBase, MII_ACC, dwAddr);

    // wait for write to complete w/ timeout
    for(i=0;i<100;i++) {
        // see if MII is finished yet
        if ((Lan_GetMacRegDW(dwLanBase, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
        {
            LeaveCriticalSection(&csAccessPhyReg);
            return (BOOL)TRUE;
        }
    }

    LeaveCriticalSection(&csAccessPhyReg);
    SMSC_WARNING0("timeout waiting for MII write to finish\r\n");
    return (BOOL)FALSE;
}

/*
FUNCTION: Lan_Initialize
  This function should be the first Lan_xxx function called
  It begins to initialize the LAN9118_DATA structure.
  It reads some ID values from the chip.
  It resets the chip.
  It initialize flow control registers.

RETURN VALUE:
    returns TRUE on Success,
    returns FALSE on Failure,
*/
BOOL Lan_Initialize(PLAN9118_DATA const pLan9118Data, const DWORD dwLanBase)
{
    BOOL    result = FALSE;
    DWORD   dwTimeout, dwTemp;

    SMSC_TRACE1(DBG_INIT,"+Lan_Initialize(dwLanBase=0x%08lX)\r\n", dwLanBase);

    if (pLan9118Data==NULL) {
        SMSC_WARNING0("Lan_Initialize(pLan9118Data==NULL)\r\n");
        goto DONE;
    }

    InitializeCriticalSection(&csAccessMacReg);
    InitializeCriticalSection(&csAccessPhyReg);

    if (dwLanBase==0x0UL) 
    {
        SMSC_WARNING0("Lan_Initialize(dwLanBase==0)\r\n");
        goto DONE;
    }

    pLan9118Data->dwLanBase = dwLanBase;

    // write BYTE_TEST to wake chip up in case it is in sleep mode
    Lan_Wakeup9118(pLan9118Data);
    
    SetRegDW(dwLanBase, HW_CFG, HW_CFG_SRST_);
    dwTimeout=100000UL;
    do 
    {
        SMSC_MICRO_DELAY(10U);
        dwTemp = GetRegDW(dwLanBase,HW_CFG);
        dwTimeout--;
    } while ((dwTimeout > 0UL) && (dwTemp & HW_CFG_SRST_));

    if (dwTemp & HW_CFG_SRST_) 
    {
        SMSC_WARNING0("  Failed to complete reset.\r\n");
        goto DONE;
    }

    // Aug/4/2008 WH
    // Enable PME_EN & PME_POL to active low
    dwTemp = GetRegDW(dwLanBase, PMT_CTRL);
    dwTemp |= PMT_CTRL_PME_EN_;
    SetRegDW(dwLanBase, PMT_CTRL, dwTemp);

    result = TRUE;

    pLan9118Data->dwIdRev = GetRegDW(dwLanBase, ID_REV);
    pLan9118Data->LanInitialized = (BOOLEAN)TRUE;

DONE:
    SMSC_TRACE1(DBG_INIT,"-Lan_Initialize, result=%s\r\n",result?"TRUE":"FALSE");
    return result;
}


#ifdef USE_PHY_WORK_AROUND
BOOLEAN Phy_Reset(const LAN9118_DATA * const pLan9118Data)
{
    BOOLEAN result=(BOOLEAN)FALSE;
    WORD wTemp=(WORD)0;
    DWORD dwLoopCount=100000UL;

    if (LanReadPhy(PHY_BCR, &wTemp) == FALSE)
        return (BOOLEAN)FALSE;

    SMSC_TRACE1(DBG_PHY,"PHY_BCR:0x%04x ... ", wTemp);

    SMSC_TRACE0(DBG_PHY,"Performing PHY BCR ");
    if (LanWritePhy(PHY_BCR,PHY_BCR_RESET_) == FALSE)
    {
        result = (BOOLEAN)FALSE;
        goto DONE;
    }

    do {
        SMSC_MICRO_DELAY(10U);
        result = (BOOLEAN)LanReadPhy(PHY_BCR, &wTemp);
        if (result == FALSE)
            goto DONE;
        dwLoopCount--;
    } while((dwLoopCount>0UL)&&(wTemp&(WORD)PHY_BCR_RESET_));
    if(wTemp&PHY_BCR_RESET_) {
        SMSC_TRACE0(DBG_PHY, "Phy Reset failed to complete.\r\n");
        goto DONE;
    }
    //extra delay required because the phy may not be completed with its reset
    //  when PHY_BCR_RESET_ is cleared.
    //  They say 256 uS is enough delay but I'm using 500 here to be safe
    SMSC_MICRO_DELAY(500U);
    result=(BOOLEAN)TRUE;

DONE:
    SMSC_TRACE1(DBG_PHY,"Performing PHY BCR Reset result=%s\n\r",
        result==TRUE? "TRUE":"FALSE");
    return result;
}

DWORD Phy_LBT_GetTxStatus(const LAN9118_DATA * const pLan9118Data)
{
    DWORD result=GetRegDW(pLan9118Data->dwLanBase, TX_FIFO_INF);

    result&=TX_FIFO_INF_TSUSED_;
    if(result!=0x00000000UL) {
        result=GetRegDW(pLan9118Data->dwLanBase, TX_STATUS_FIFO_PORT);
    } else {
        result=0UL;
    }

    return result;
}

DWORD Phy_LBT_GetRxStatus(const LAN9118_DATA * const pLan9118Data)
{
    DWORD result=GetRegDW(pLan9118Data->dwLanBase, RX_FIFO_INF);
    if(result&RX_FIFO_INF_RXSUSED_) {
        //Rx status is available, read it
        result=GetRegDW(pLan9118Data->dwLanBase, RX_STATUS_FIFO_PORT);
    } else {
        result=0UL;
    }
    return result;
}


BOOLEAN Phy_CheckLoopBackPacket(LAN9118_DATA * const pLan9118Data)
{
    BOOLEAN result=(BOOLEAN)FALSE;
    DWORD tryCount=0UL;
    DWORD dwLoopCount=0UL;
    for(tryCount=0UL;tryCount<10UL;tryCount++)
    {
        DWORD dwTxCmdA=0UL;
        DWORD dwTxCmdB=0UL;
        DWORD dwStatus=0UL;
        DWORD dwPacketLength=0UL;
        
        //zero-out Rx Packet memory
        memset(pLan9118Data->LoopBackRxPacket,0,(UINT)MIN_PACKET_SIZE);
        
        //write Tx Packet to 118
        dwTxCmdA=
            ((((DWORD)(pLan9118Data->LoopBackTxPacket))&0x03UL)<<16) | //DWORD alignment adjustment
            TX_CMD_A_INT_FIRST_SEG_ | TX_CMD_A_INT_LAST_SEG_ |
            ((MIN_PACKET_SIZE));
        dwTxCmdB=
            (((DWORD)(MIN_PACKET_SIZE))<<16) |
            ((DWORD)(MIN_PACKET_SIZE));
        SetRegDW(pLan9118Data->dwLanBase,TX_DATA_FIFO_PORT,dwTxCmdA);
        SetRegDW(pLan9118Data->dwLanBase,TX_DATA_FIFO_PORT,dwTxCmdB);
        Lan_WriteTxFifo(pLan9118Data, 
            pLan9118Data->dwLanBase,
            (DWORD *)(((DWORD)(pLan9118Data->LoopBackTxPacket))&0xFFFFFFFCUL),
            (((DWORD)(MIN_PACKET_SIZE))+3UL+
            (((DWORD)(pLan9118Data->LoopBackTxPacket))&0x03UL))>>2);

        //wait till transmit is done
        dwLoopCount=60UL;
        while(((dwStatus=Phy_LBT_GetTxStatus(pLan9118Data))==0UL)&&(dwLoopCount>0UL)) {
            SMSC_MICRO_DELAY(5U);
            dwLoopCount--;
        }
        if(dwStatus==0UL) {
            SMSC_WARNING0("Failed to Transmit during Loop Back Test\r\n");
            continue;
        }
        if(dwStatus&0x00008000UL) {
            SMSC_WARNING0("Transmit encountered errors during Loop Back Test\r\n");
            continue;
        }

        //wait till receive is done
        dwLoopCount=60UL;
        while(((dwStatus=Phy_LBT_GetRxStatus(pLan9118Data))==0UL)&&(dwLoopCount>0UL))
        {
             SMSC_MICRO_DELAY(5U);
             dwLoopCount--;
        }
        if(dwStatus==0UL) {
            SMSC_WARNING0("Failed to Receive during Loop Back Test\r\n");
            continue;
        }
        if(dwStatus&RX_STS_ES)
        {
            SMSC_WARNING0("Receive encountered errors during Loop Back Test\r\n");
            continue;
        }

        dwPacketLength=((dwStatus&0x3FFF0000UL)>>16);

        Lan_ReadRxFifo(
            pLan9118Data->dwLanBase,
            ((DWORD *)(pLan9118Data->LoopBackRxPacket)),
            (dwPacketLength+3UL+(((DWORD)(pLan9118Data->LoopBackRxPacket))&0x03UL))>>2);
        
        if(dwPacketLength!=(MIN_PACKET_SIZE+4UL)) {
            SMSC_TRACE1(DBG_INIT, "Unexpected packet size during loop back test, size=%ld, will retry",dwPacketLength);
        } else {
            DWORD byteIndex=0UL;
            BOOLEAN foundMissMatch=(BOOLEAN)FALSE;
            for(byteIndex=0UL;byteIndex<MIN_PACKET_SIZE;byteIndex++) {
                if(pLan9118Data->LoopBackTxPacket[byteIndex]!=pLan9118Data->LoopBackRxPacket[byteIndex])
                {
                    foundMissMatch=(BOOLEAN)TRUE;
                    break;
                }         
            }
            if(foundMissMatch != TRUE) {
                SMSC_TRACE0(DBG_PHY, "Successfully verified Loop Back Packet\n\r");
                result=(BOOLEAN)TRUE;
                goto DONE;
            } else {
                SMSC_WARNING0("Data miss match during loop back test, will retry.\r\n");
            }
        }
    }
DONE:
    return result;
}

BOOLEAN Phy_LoopBackTest(LAN9118_DATA * const pLan9118Data)
{
    BOOLEAN result=(BOOLEAN)FALSE;
    DWORD byteIndex=0UL;
    DWORD tryCount=0UL;
//  DWORD failed=0;
    //Initialize Tx Packet
    for(byteIndex=0UL;byteIndex<6UL;byteIndex++) {
        //use broadcast destination address
        pLan9118Data->LoopBackTxPacket[byteIndex]=(BYTE)0xFF;
    }
    for(byteIndex=6UL;byteIndex<12UL;byteIndex++) {
        //use incrementing source address
        pLan9118Data->LoopBackTxPacket[byteIndex]=(BYTE)byteIndex;
    }
    //Set length type field
    pLan9118Data->LoopBackTxPacket[12]=(BYTE)0x00;
    pLan9118Data->LoopBackTxPacket[13]=(BYTE)0x00;
    for(byteIndex=14UL;byteIndex<MIN_PACKET_SIZE;byteIndex++)
    {
        pLan9118Data->LoopBackTxPacket[byteIndex]=(BYTE)byteIndex;
    }
//TRY_AGAIN:
    {
        DWORD dwRegVal=GetRegDW(pLan9118Data->dwLanBase, HW_CFG);
        dwRegVal&=HW_CFG_TX_FIF_SZ_;
        dwRegVal|=HW_CFG_MBO_;
        SetRegDW(pLan9118Data->dwLanBase,HW_CFG,dwRegVal);
    }
    SetRegDW(pLan9118Data->dwLanBase,TX_CFG,TX_CFG_TX_ON_);

    SetRegDW(pLan9118Data->dwLanBase,RX_CFG,(((DWORD)(pLan9118Data->LoopBackRxPacket))&0x03UL)<<8);
    
    //Set Phy to 10/FD, no ANEG,
    if (LanWritePhy(PHY_BCR,(WORD)0x0100) == FALSE)
    {
        result = (BOOLEAN)FALSE;
        goto DONE;
    }

    //enable MAC Tx/Rx, FD
    LanWriteMac(MAC_CR,MAC_CR_FDPX_|MAC_CR_TXEN_|MAC_CR_RXEN_);

//  Phy_TransmitTestPacket(privateData);
    
    //set Phy to loopback mode
    if (LanWritePhy(PHY_BCR,(WORD)0x4100) == FALSE)
    {
        result = (BOOLEAN)FALSE;
        goto DONE;
    }

    for(tryCount=0UL;tryCount<10UL;tryCount++) {
        if(Phy_CheckLoopBackPacket(pLan9118Data) == TRUE)
        {
            result=(BOOLEAN)TRUE;
            goto DONE;
        }
        pLan9118Data->dwResetCount++;

        //disable MAC rx
        LanWriteMac(MAC_CR,0UL);
        result = Phy_Reset(pLan9118Data);

        //Set Phy to 10/FD, no ANEG, and Loopbackmode
        if (LanWritePhy(PHY_BCR,(WORD)0x4100) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        //enable MAC Tx/Rx, FD
        LanWriteMac(MAC_CR,MAC_CR_FDPX_|MAC_CR_TXEN_|MAC_CR_RXEN_);
    }
DONE:
    //disable MAC
    LanWriteMac(MAC_CR,0UL);

    //Cancel Phy loopback mode
    if (LanWritePhy(PHY_BCR,(WORD)0U) == FALSE)
    {
        return FALSE;
    }

    SetRegDW(pLan9118Data->dwLanBase,TX_CFG,0UL);
    SetRegDW(pLan9118Data->dwLanBase,RX_CFG,0UL);
    
    return result;
}

#endif //USE_PHY_WORK_AROUND

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
BOOL Lan_InitializePhy(PLAN9118_DATA pLan9118Data, BYTE bPhyAddress)
{
    BOOL    result = TRUE;
    BOOL    ExtPhy = FALSE;
    DWORD   dwTemp = 0UL;
    DWORD   dwLoopCount = 0UL;
    DWORD   dwPhyId = 0UL;
    BYTE    bPhyModel = (BYTE)0;
    BYTE    bPhyRev = (BYTE)0;
    WORD    wPhyID1 = 0, wPhyID2 = 0;

    SMSC_TRACE2(DBG_INIT, "+Lan_InitializePhy(pLan9118Data=0x%08lX, bPhyAddress=%d)\r\n", (DWORD)pLan9118Data,bPhyAddress);
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data == NULL) 
    {
        return FALSE;
    }

    SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
    SMSC_ASSERT(pLan9118Data->dwIdRev != 0UL);
    SMSC_ASSERT(pLan9118Data->LanInitialized == (BOOLEAN)TRUE);
    SMSC_ASSERT(pLan9118Data->InterruptsInitialized == (BOOLEAN)TRUE);

    if (bPhyAddress == 0xFF) 
    {
        /* Use Internal Phy */
        RETAILMSG(1, (TEXT("Use IntPhy\r\n")));
        /* verify phy ID */
        bPhyAddress = (BYTE)1;  // internal address
        ExtPhy = FALSE;
    }
    else 
    {
        /* Using External Phy */
        /* Check ID */
        switch (pLan9118Data->dwIdRev & 0xFFFF0000UL)
        {
            /*
             * NOTE
             *  0x01150000, 0x01150001, 0x01170000 and 0x01170001 
             *   will NOT reach here
             */
            case    0x01150000UL:
            case    0x01170000UL:
            case    0x115A0000UL:
            case    0x117A0000UL:
                dwTemp = GetRegDW(pLan9118Data->dwLanBase, HW_CFG);
                if (dwTemp & HW_CFG_EXT_PHY_DET_)
                {
                    dwTemp &= ~HW_CFG_PHY_CLK_SEL_;
                    dwTemp |= HW_CFG_PHY_CLK_SEL_CLK_DIS_;
                    SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);
                    SMSC_MICRO_DELAY(10U);

                    dwTemp |= HW_CFG_EXT_PHY_EN_;
                    SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);

                    dwTemp &= ~HW_CFG_PHY_CLK_SEL_;
                    dwTemp |= HW_CFG_PHY_CLK_SEL_EXT_PHY_;
                    SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);
                    SMSC_MICRO_DELAY(10U);

                    dwTemp |= HW_CFG_SMI_SEL_;
                    SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);

                    if (bPhyAddress < 32)
                    {
                        // Use specified PhyAddress
                        SMSC_TRACE1(DBG_PHY, "Use 0x%x ExtPhy\r\n", bPhyAddress);
                        RETAILMSG(1, (TEXT("Use 0x%x ExtPhy\r\n"), bPhyAddress));
                        result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_ID_1, &wPhyID1);
                        if (result == FALSE)
                            goto DONE;
                        result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_ID_2, &wPhyID2);
                        if (result == FALSE)
                            goto DONE;
                        ExtPhy = TRUE;
                    }
                    else
                    {
                        DWORD   dwAddr;
                        for (dwAddr = 0UL;dwAddr < 32UL;dwAddr++)
                        {
                            result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, dwAddr, PHY_ID_1, &wPhyID1);
                            if (result == FALSE)
                                goto DONE;
                            result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, dwAddr, PHY_ID_2, &wPhyID2);
                            if (result == FALSE)
                                goto DONE;
                            if ((wPhyID1 != (WORD)0xFFFFU) || 
                                (wPhyID2 != (WORD)0xFFFFU))
                            {
                                SMSC_TRACE1(DBG_PHY, "Detect Phy at Address 0x%x\r\n", dwAddr);
                                RETAILMSG(1, (TEXT("Detect Phy at Address 0x%x\r\n"), dwAddr));
                                bPhyAddress = (BYTE)dwAddr;
                                ExtPhy = TRUE;
                                break;
                            }
                        }
                        if (dwAddr == 32UL)
                        {
                            SMSC_WARNING0("Error! Failed to detect External Phy\r\n");
                            ExtPhy = FALSE;
                        }
                    }
                    if ((wPhyID1 == (WORD)0xFFFF) && 
                        (wPhyID2 == (WORD)0xFFFF))
                    {
                        SMSC_WARNING0("Error! External Phy is not accessible. Switch to Internal Phy\r\n");
                        // revert back to Internal Phy
                        bPhyAddress = (BYTE)1;  // internal address
                        dwTemp &= ~HW_CFG_PHY_CLK_SEL_;
                        dwTemp |= HW_CFG_PHY_CLK_SEL_CLK_DIS_;
                        SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);
                        SMSC_MICRO_DELAY(10U);

                        dwTemp &= ~HW_CFG_EXT_PHY_EN_;
                        SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);

                        dwTemp &= ~HW_CFG_PHY_CLK_SEL_;
                        dwTemp |= HW_CFG_PHY_CLK_SEL_EXT_PHY_;
                        SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);
                        SMSC_MICRO_DELAY(10U);

                        dwTemp &= ~HW_CFG_SMI_SEL_;
                        SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwTemp);
                        ExtPhy = FALSE;
                    }
                }
                else
                {
                    /* Use Internal Phy */
                    SMSC_WARNING0("ExtPhy is not detected. Switch to Internal Phy\r\n");
                    bPhyAddress = (BYTE)1;  // internal address
                    ExtPhy = FALSE;
                }
                break;

            case    0x93110000UL:
                /* Use Internal Phy */
                SMSC_TRACE0(DBG_PHY, "Use IntPhy\r\n");
                RETAILMSG(1, (L"Use IntPhy : address=%d\r\n",bPhyAddress));
                ExtPhy = FALSE;
                break;

            default:
                /* Use Internal Phy */
                SMSC_TRACE0(DBG_PHY, "Use IntPhy\r\n");
                RETAILMSG(1, (TEXT("Use IntPhy\r\n")));
                bPhyAddress = (BYTE)1;  // internal address
                ExtPhy = FALSE;
                break;
        }
    }

    result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_ID_2, (WORD *)&dwTemp);
    if (result == FALSE)
        goto DONE;
    bPhyRev = ((BYTE)(dwTemp & (0x0FUL)));
    bPhyModel = ((BYTE)((dwTemp>>4) & (0x3FUL)));
    dwPhyId = dwTemp << 16;
    result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_ID_1, (WORD *)&dwTemp);
    if (result == FALSE)
        goto DONE;
    dwPhyId |= ((dwTemp & (0x0000FFFFUL))<<2);

    pLan9118Data->bPhyAddress = bPhyAddress;
    pLan9118Data->dwPhyId = dwPhyId;
    pLan9118Data->bPhyModel = bPhyModel;
    pLan9118Data->bPhyRev = bPhyRev;
    pLan9118Data->dwLinkMode = LINK_NO_LINK;

    /* reset the PHY */
    if (Lan_SetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_BCR, PHY_BCR_RESET_) == FALSE)
    {
        result = FALSE;
        goto DONE;
    }

    dwLoopCount = 100000UL;
    do {
        SMSC_MICRO_DELAY(10U);
        result = Lan_GetPhyRegW(pLan9118Data->dwLanBase, (DWORD)bPhyAddress, PHY_BCR, (WORD *)&dwTemp);
        if (result == FALSE)
            goto DONE;
        dwLoopCount--;
    } while ((dwLoopCount>0UL) && (dwTemp & (DWORD)PHY_BCR_RESET_));
    if (dwTemp & (DWORD)PHY_BCR_RESET_) {
        SMSC_WARNING0("PHY reset failed to complete.\r\n");
        goto DONE;
    }

#ifdef USE_PHY_WORK_AROUND  // on internal PHY use only
    if (ExtPhy == FALSE)    // 031305 WH
    {
        // workaround for 118/117/116/115 family
        if (((pLan9118Data->dwIdRev & 0xFFF0FFFFUL) == 0x01100001UL) ||
            ((pLan9118Data->dwIdRev & 0xFFF0FFFFUL) == 0x01100002UL))
        {
            if(!Phy_LoopBackTest(pLan9118Data) && (pLan9118Data->bPhyAddress==(BYTE)1)) {
                SMSC_WARNING1("Failed Loop Back Test, reset %d times\n\r",
                    pLan9118Data->dwResetCount);
                goto DONE;
            } else {
                SMSC_WARNING1("Passed Loop Back Test, reset %d times\n\r",
                    pLan9118Data->dwResetCount);
            }   
        }
    }
#endif //USE_PHY_WORK_AROUND    // on internal PHY use only


    if (ExtPhy == FALSE) 
    {
        if (((pLan9118Data->dwIdRev & 0x000F0000UL) == 0x000A0000UL) | 
            ((pLan9118Data->dwIdRev & 0xFFF00000UL) == 0x92200000UL) |
            ((pLan9118Data->dwIdRev & 0xFFF00000UL) == 0x92210000UL) |
            ((pLan9118Data->dwIdRev & 0xFFF00000UL) == 0x92100000UL) |
            ((pLan9118Data->dwIdRev & 0xFFF00000UL) == 0x92110000UL) |
            ((pLan9118Data->dwIdRev & 0xFF0F0000UL) == 0x210A0000UL))
        {
            Lan_SetAutoMdixSts(pLan9118Data);
        }
        else
        {
            RETAILMSG(1, (TEXT("This chip doesn't support Auto Mdix!!!\r\n")));
        }
    }

    pLan9118Data->PhyInitialized = (BOOLEAN)TRUE;

DONE:
    SMSC_TRACE1(DBG_INIT,"-Lan_InitializePhy, result=%s\r\n",result?TEXT("TRUE"):TEXT("FALSE"));
    return result;
}

/*
FUNCTION: Lan_AutoNegotiate
RETURN VALUE:
    returns TRUE on success.
    returns FALSE on failure.
*/
BOOLEAN Lan_AutoNegotiate(const LAN9118_DATA * const pLan9118Data)
{
    BOOL result;
    DWORD dwTimeout=0UL;
    WORD  wTemp, wBreaknow=(WORD)0;

    SMSC_TRACE0(DBG_INIT, "+Lan_AutoNegotiate(...)\r\n");

    wTemp = (WORD)(PHY_BCR_AUTO_NEG_ENABLE_ | PHY_BCR_RESTART_AUTO_NEG_);
    if (LanWritePhy(PHY_BCR, wTemp) == FALSE)
    {
        SMSC_TRACE0(DBG_INIT, "-Lan_AutoNegotiate(...) Failed\r\n");
        return (BOOLEAN)FALSE;
    }

Restart_AutoNegotiation:
    wBreaknow++;
    dwTimeout = 100000UL;
    // Check for the completion and the remote fault
    do {
        result = LanReadPhy(PHY_BSR, &wTemp);
        if (result == FALSE)
            return (BOOLEAN)FALSE;
    } while((dwTimeout-- > 0UL) && 
            !((wTemp & (WORD)PHY_BSR_REMOTE_FAULT_) || 
             (wTemp & (WORD)PHY_BSR_AUTO_NEG_COMP_)));

    if (dwTimeout == 0)
    {
        SMSC_TRACE0(DBG_INIT, "-Lan_AutoNegotiate(...) Failed\r\n");
        return (BOOLEAN)FALSE;
    }

    if(wTemp & (WORD)PHY_BSR_REMOTE_FAULT_) {
        SMSC_TRACE0(DBG_INIT,"Autonegotiation Remote Fault\r\n");
        if(wBreaknow < 10)
        {
            goto Restart_AutoNegotiation;
        }
        SMSC_TRACE0(DBG_INIT, "-Lan_AutoNegotiate(...) Failed\r\n");
        return (BOOLEAN)FALSE;
    }

    SMSC_TRACE0(DBG_INIT, "-Lan_AutoNegotiate(...)\r\n");
    return (BOOLEAN)TRUE;
}

/*
FUNCTION: Lan_EstablishLink
  This function similar to Lan_RequestLink except that it is
  not interrupt driven. This function will not return until 
  either the link is established or link can not be established.
RETURN VALUE:
    returns TRUE on success.
    returns FALSE on failure.
*/
BOOL Lan_EstablishLink(PLAN9118_DATA pLan9118Data, const DWORD dwLinkRequest, const BOOLEAN fFlowControl)
{
    WORD    wTemp, wRegVal = (WORD)0;
    DWORD   dwRegVal = 0UL;
    BOOL    result = TRUE;

    SMSC_TRACE3(DBG_INIT, "+Lan_EstablishLink(pLan9118Data==0x%08lX,dwLinkRequest==%ld,fFlowControl=%d)\r\n", (DWORD)pLan9118Data,dwLinkRequest,fFlowControl);

    if (dwLinkRequest & LINKMODE_ANEG) 
    {
        // Enable ANEG
        if (LanReadPhy(PHY_BCR, &wTemp) == FALSE)
            return FALSE;

        wTemp = (WORD)(wTemp | PHY_BCR_AUTO_NEG_ENABLE_);
        if (LanWritePhy(PHY_BCR, wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        // Set ANEG Advertise
        if (LanReadPhy(PHY_ANEG_ADV, &wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        wTemp = (WORD)(wTemp & ~(PHY_ANEG_ADV_PAUSE_OP_ | PHY_ANEG_ADV_SPEED_));
        if (dwLinkRequest & LINKMODE_ASYM_PAUSE)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_ASYM_PAUSE_);
        }
        if (dwLinkRequest & LINKMODE_SYM_PAUSE)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_SYM_PAUSE_);
        }
        if (dwLinkRequest & LINKMODE_100_FD)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_100F_);
        }
        if (dwLinkRequest & LINKMODE_100_HD)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_100H_);
        }
        if (dwLinkRequest & LINKMODE_10_FD)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_10F_);
        }
        if (dwLinkRequest & LINKMODE_10_HD)
        {
            wTemp = (WORD)(wTemp | PHY_ANEG_ADV_10H_);
        }
        if (LanWritePhy(PHY_ANEG_ADV, wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        if(!Lan_AutoNegotiate(pLan9118Data))
        {
            pLan9118Data->dwLinkMode = LINK_NO_LINK;
            SMSC_TRACE0(DBG_INIT,"Auto Negotiation Failed !\r\n");
            result = FALSE;
        } 
        else 
        {
            SMSC_TRACE0(DBG_INIT,"Auto Negotiation Complete\r\n");
        }

        //Clear any pending interrupts
        if (LanReadPhy(PHY_INT_SRC, &wRegVal) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }
        // avoid lint error
        wRegVal = wRegVal;

        //CheckForLink
        pLan9118Data->dwLinkMode = Lan_GetLinkMode(pLan9118Data);

        dwRegVal = Lan_GetMacRegDW(pLan9118Data->dwLanBase, MAC_CR);
        dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);

        switch(pLan9118Data->dwLinkMode) 
        {
            case LINK_NO_LINK:
                SMSC_TRACE0(DBG_INIT,"There is no Link\r\n");
                //TODO: consider auto linking to a specified link state.
                break;

            case LINK_10MPS_HALF:
                SMSC_TRACE0(DBG_INIT,"Link is 10Mbps Half Duplex\r\n");
                dwRegVal|=MAC_CR_RCVOWN_;
                break;

            case LINK_10MPS_FULL:
                SMSC_TRACE0(DBG_INIT,"Link is 10Mbps Full Duplex\r\n");
                dwRegVal|=MAC_CR_FDPX_;
                break;

            case LINK_100MPS_HALF:
                SMSC_TRACE0(DBG_INIT,"Link is 100Mbps Half Duplex\r\n");
                dwRegVal|=MAC_CR_RCVOWN_;
                break;

            case LINK_100MPS_FULL:
                SMSC_TRACE0(DBG_INIT,"Link is 100Mbps Full Duplex\r\n");
                dwRegVal|=MAC_CR_FDPX_;
                break;

            default:
                SMSC_TRACE0(DBG_INIT,"Unknown LinkMode\r\n");
                break;
        }

        Lan_SetMacRegDW(pLan9118Data->dwLanBase, MAC_CR, dwRegVal);
    }
    else
    {
        // Non-ANEG
        // If multiple mode bits were set, it uses following priority,
        //   100FD->100HD->10FD->10HD
        if (LanReadPhy(PHY_BCR, &wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }
        wTemp &= ~(PHY_BCR_AUTO_NEG_ENABLE_ | PHY_BCR_RESTART_AUTO_NEG_);
        if (dwLinkRequest & LINKMODE_100_FD) {
            SMSC_TRACE0(DBG_INIT,"Link fixed at 100Mbps Full Duplex\r\n");
            wTemp |= (PHY_BCR_SPEED_SELECT_ | PHY_BCR_DUPLEX_MODE_);
        }
        else if (dwLinkRequest & LINKMODE_100_HD) {
            SMSC_TRACE0(DBG_INIT,"Link fixed at 100Mbps Half Duplex\r\n");
            wTemp |= (PHY_BCR_SPEED_SELECT_);
            wTemp &= ~(PHY_BCR_DUPLEX_MODE_);
        }
        else if (dwLinkRequest & LINKMODE_10_FD) {
            SMSC_TRACE0(DBG_INIT,"Link fixed at 10Mbps Full Duplex\r\n");
            wTemp &= ~(PHY_BCR_SPEED_SELECT_);
            wTemp |= ( PHY_BCR_DUPLEX_MODE_);
        }
        else if (dwLinkRequest & LINKMODE_10_HD) {
            SMSC_TRACE0(DBG_INIT,"Link fixed at 10Mbps Half Duplex\r\n");
            wTemp &= ~(PHY_BCR_SPEED_SELECT_ | PHY_BCR_DUPLEX_MODE_);
        }
        else {
            RETAILMSG(1, (TEXT("Error! No Link Mode was Set.\r\n")));
            return FALSE;
        }
        if (LanWritePhy(PHY_BCR, wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        //Clear any pending interrupts
        if (LanReadPhy(PHY_INT_SRC, &wTemp) == FALSE)
        {
            result = FALSE;
            goto DONE;
        }

        //CheckForLink
        dwRegVal = Lan_GetMacRegDW(pLan9118Data->dwLanBase, MAC_CR);
        dwRegVal &= ~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);
        if (dwLinkRequest & (LINKMODE_100_FD | LINKMODE_10_FD)) 
        {
            dwRegVal |= MAC_CR_FDPX_;
        }
        else if (dwLinkRequest & (LINKMODE_100_HD | LINKMODE_10_HD)) 
        {
            dwRegVal |= MAC_CR_RCVOWN_;
        }
        Lan_SetMacRegDW(pLan9118Data->dwLanBase, MAC_CR, dwRegVal);

        if (dwLinkRequest & (LINKMODE_100_FD | LINKMODE_10_FD)) 
        {
            DWORD dwTemp;

            // Mac Initialize
            if (fFlowControl == (BOOLEAN)TRUE)
            {
                DWORD dwTimeout=1000UL;

                SetRegDW(pLan9118Data->dwLanBase, AFC_CFG, DEFAULT_AFC_CFG);

                //Wait for busy bit to clear.
                while ((Lan_GetMacRegDW(pLan9118Data->dwLanBase, FLOW) & FLOW_FCBSY_)  && (dwTimeout > 0UL)) 
                {
                    SMSC_MICRO_DELAY(1U);
                    dwTimeout--;
                }

                if(!dwTimeout)
                {
                    SMSC_WARNING0("Timeout waiting flow busy bit.\r\n");
                }

                // Set FLOW register in MAC to send out a pause frame.
                Lan_SetMacRegDW(pLan9118Data->dwLanBase, 
                                FLOW, 
                                (FLOW_FCEN_ | (0xFFFFUL<<16UL) | FLOW_FCBSY_));
            }
            else
            {
                // Disable Flow Control
                dwTemp = LanReadMac(FLOW);
                LanWriteMac(FLOW, dwTemp & ~FLOW_FCEN_);
                dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
                SetRegDW(pLan9118Data->dwLanBase,AFC_CFG, dwTemp & ~AFC_CFG_FCANY_);
            }
        }
        else if (dwLinkRequest & (LINKMODE_100_HD | LINKMODE_10_HD)) 
        {
            DWORD dwTemp;

            // Disable Flow Control
            dwTemp = LanReadMac(FLOW);
            LanWriteMac(FLOW, dwTemp & ~FLOW_FCEN_);
            dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
            SetRegDW(pLan9118Data->dwLanBase,AFC_CFG, dwTemp & ~AFC_CFG_FCANY_);
        }
    }

DONE:
    SMSC_TRACE1(DBG_INIT,"-Lan_EstablishLink, result=%s\r\n",result?TEXT("TRUE"):TEXT("FALSE"));
    return result;
}


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
DWORD Lan_GetLinkMode(const LAN9118_DATA * const pLan9118Data)
{
    DWORD result = LINK_NO_LINK;
    WORD wRegVal;
    WORD wRegBSR;

    if (LanReadPhy(PHY_BSR, &wRegBSR) == FALSE)
    {
        SMSC_WARNING0("Lan_GetLiniMode() : error to read PHY_BSR\n");
        return LINK_NO_LINK;
    }

    if (LanReadPhy(PHY_BCR, &wRegVal) == FALSE)
    {
        SMSC_WARNING0("Lan_GetLiniMode() : error to read PHY_BCR\n");
        return LINK_NO_LINK;
    }

    if (wRegVal & PHY_BCR_AUTO_NEG_ENABLE_) 
    {
        if(wRegBSR & PHY_BSR_LINK_STATUS_) 
        {
            WORD wTemp;
            WORD wRegADV;
            WORD wRegLPA;

            if (LanReadPhy(PHY_ANEG_ADV, &wRegADV) == FALSE)
            {
                SMSC_WARNING0("Lan_GetLiniMode() : error to read PHY_ANEG_ADV\n");
                return LINK_NO_LINK;
            }
            if (LanReadPhy(PHY_ANEG_LPA, &wRegLPA) == FALSE)
            {
                SMSC_WARNING0("Lan_GetLiniMode() : error to read PHY_ANEG_LPA\n");
                return LINK_NO_LINK;
            }


            wTemp = (WORD)(wRegLPA & wRegADV);

            if(wTemp & PHY_ANEG_LPA_100FDX_) 
            {
                result = LINK_100MPS_FULL;
            } 
            else if(wTemp & PHY_ANEG_LPA_100HDX_) 
            {
                result = LINK_100MPS_HALF;
            } 
            else if(wTemp & PHY_ANEG_LPA_10FDX_) 
            {
                result = LINK_10MPS_FULL;
            } 
            else if(wTemp & PHY_ANEG_LPA_10HDX_) 
            {
                result = LINK_10MPS_HALF;
            }
            else 
            {
                // do nothing except making lint happy
            }
            // check Flow Control
            if (wTemp & (WORD) (PHY_ANEG_LPA_100FDX_ | PHY_ANEG_LPA_10FDX_)) {
                DWORD dwTemp;
                if (wTemp & (WORD)0x0400U) {
                    // both support Symmetric Flow Control
                    dwTemp = LanReadMac(FLOW);
                    LanWriteMac(FLOW, dwTemp | FLOW_FCEN_);
                    dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
                    SetRegDW(pLan9118Data->dwLanBase, AFC_CFG, dwTemp | AFC_CFG_FCANY_);
                }
                else if (((wRegADV & 0x0C00) == 0x0C00) && 
                         ((wRegLPA & 0x0C00) == 0x0800)) {
                    // Partner is Asymmetric Flow Control
                    // Enable Rx Only (Enable FC on MAC, Disable at AFC_CFG)
                    dwTemp = LanReadMac(FLOW);
                    LanWriteMac(FLOW, dwTemp | FLOW_FCEN_);
                    dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
                    SetRegDW(pLan9118Data->dwLanBase, AFC_CFG, dwTemp & ~AFC_CFG_FCANY_);
                }
                else {
                    // Disable Flow Control
                    dwTemp = LanReadMac(FLOW);
                    LanWriteMac(FLOW, dwTemp & ~FLOW_FCEN_);
                    dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
                    SetRegDW(pLan9118Data->dwLanBase, AFC_CFG, dwTemp & ~AFC_CFG_FCANY_);
                }
            }
            else {
                // Half-Duplex
                // Disable Flow Control
                DWORD dwTemp;
                dwTemp = LanReadMac(FLOW);
                LanWriteMac(FLOW, dwTemp & ~FLOW_FCEN_);
                dwTemp = GetRegDW(pLan9118Data->dwLanBase, AFC_CFG);
                SetRegDW(pLan9118Data->dwLanBase, AFC_CFG, dwTemp & ~AFC_CFG_FCANY_);
            }
        }
    } 
    else 
    {
        if (wRegBSR & PHY_BSR_LINK_STATUS_) 
        {
            if (wRegVal & PHY_BCR_SPEED_SELECT_) 
            {
                if (wRegVal & PHY_BCR_DUPLEX_MODE_) 
                {
                    result = LINK_100MPS_FULL;
                } 
                else 
                {
                    result = LINK_100MPS_HALF;
                }
            } 
            else 
            {
                if (wRegVal & PHY_BCR_DUPLEX_MODE_) 
                {
                    result = LINK_10MPS_FULL;
                } 
                else 
                {
                    result = LINK_10MPS_HALF;
                }
            }
        }
    }

    return result;
}

void Lan_SetAutoMdixSts(PLAN9118_DATA pLan9118Data)
{
    WORD wAutoMdixSts = pLan9118Data->wAutoMdix;
    WORD SpecialCtrlSts=0U;

    if (wAutoMdixSts > 2)
    {

        if (LanReadPhy(SPECIAL_CTRL_STS, &SpecialCtrlSts) == FALSE)
        {
            RETAILMSG(1, (TEXT("Error to read SPECIAL_STRL_STS\r\n")));
            return;
        }

        SpecialCtrlSts = (SpecialCtrlSts&0x1FFF);
        if (LanWritePhy(SPECIAL_CTRL_STS,SpecialCtrlSts) == FALSE)
        {
            RETAILMSG(1, (TEXT("Error to write to SPECIAL_STRL_STS\r\n")));
            return;
        }

        if (GetRegDW(pLan9118Data->dwLanBase, HW_CFG) & HW_CFG_AMDIX_EN_STRAP_STS_)
        {
            RETAILMSG(1, (TEXT("Auto-MDIX Enable by default!!!\r\n")));
        }
        else {
            RETAILMSG(1, (TEXT("Auto-MDIX Disable by default!!!\r\n")));
        }
        pLan9118Data->wAutoMdix=3;
    }
    else 
    {

        if (LanReadPhy(SPECIAL_CTRL_STS, &SpecialCtrlSts) == FALSE)
        {
            RETAILMSG(1, (TEXT("Error to read SPECIAL_STRL_STS\r\n")));
            return;
        }

        SpecialCtrlSts = (((wAutoMdixSts+4) << 13) | (SpecialCtrlSts&0x1FFF));
        if (LanWritePhy(SPECIAL_CTRL_STS,SpecialCtrlSts) == FALSE)
        {
            RETAILMSG(1, (TEXT("Error to write to SPECIAL_STRL_STS\r\n")));
            return;
        }

        if (wAutoMdixSts & AMDIX_ENABLE) {
            RETAILMSG(1, (TEXT("Override Strap, Enable Auto-MDIX\r\n")));
        } else if (wAutoMdixSts & AMDIX_DISABLE_CROSSOVER) {
            RETAILMSG(1, (TEXT("Override Strap, Disable Auto-MDIX, CrossOver Cable\r\n")));     
        } else {
            RETAILMSG(1, (TEXT("Override Strap, Disable Auto-MDIX, Straight Cable\r\n")));  
        }
        pLan9118Data->wAutoMdix=wAutoMdixSts;
        
    }
}


/*
FUNCTION: Lan_EnableInterrupt
  Enables bits in INT_EN according to the set bits in dwMask
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_EnableInterrupt(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask)
{
    DWORD dwTemp;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        dwTemp = GetRegDW(pLan9118Data->dwLanBase, INT_EN);
        dwTemp |= dwMask;
        SetRegDW(pLan9118Data->dwLanBase, INT_EN, dwTemp);
    }
}

/*
FUNCTION: Lan_DisableInterrupt
  Disables bits in INT_EN according to the set bits in dwMask
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_DisableInterrupt(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask)
{
    DWORD dwTemp;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        dwTemp = GetRegDW(pLan9118Data->dwLanBase, INT_EN);
        dwTemp &= (~dwMask);
        SetRegDW(pLan9118Data->dwLanBase, INT_EN, dwTemp);
    }
}

/*
FUNCTION: Lan_GetInterruptStatus
  Reads and returns the value in the INT_STS register.
*/
DWORD Lan_GetInterruptStatus(const LAN9118_DATA * const pLan9118Data)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        return GetRegDW(pLan9118Data->dwLanBase, INT_STS);
    }
    return 0xFFFFFFFFUL;
}

/*
FUNCTION: Lan_EnableIRQ
  Sets IRQ_EN in the INT_CFG registers.
  WARNING this has thread synchronization issues. Use with caution.
*/
void Lan_EnableIRQ(const LAN9118_DATA * const pLan9118Data)
{
    DWORD dwRegVal = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        dwRegVal = GetRegDW(pLan9118Data->dwLanBase, INT_CFG);
        dwRegVal |= INT_CFG_IRQ_EN_;
        SetRegDW(pLan9118Data->dwLanBase, INT_CFG, dwRegVal);
    }
}

/*
FUNCTION: Lan_DisableIRQ
  Clears IRQ_EN in the INT_CFG registers.
  WARNING this has thread sychronization issues. Use with caution.
*/
void Lan_DisableIRQ(const LAN9118_DATA * const pLan9118Data)
{
    DWORD dwRegVal = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        dwRegVal = GetRegDW(pLan9118Data->dwLanBase, INT_CFG);
        dwRegVal &= ~(INT_CFG_IRQ_EN_);
        SetRegDW(pLan9118Data->dwLanBase, INT_CFG, dwRegVal);
    }
}

/*
FUNCTION: Lan_ClearInterruptStatus
  Clears the bits in INT_STS according to the bits set in dwMask
*/
void Lan_ClearInterruptStatus(const LAN9118_DATA * const pLan9118Data, const DWORD dwMask)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        SetRegDW(pLan9118Data->dwLanBase, INT_STS, dwMask);
    }
}

/*
FUNCTION: Lan_InitializeInterrupts
  Should be called after Lan_Initialize
  Should be called before the ISR is registered.
*/
void Lan_InitializeInterrupts(LAN9118_DATA * const pLan9118Data, DWORD dwIntCfg)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        SMSC_ASSERT(pLan9118Data->LanInitialized == TRUE);

        SetRegDW(pLan9118Data->dwLanBase, INT_EN, 0UL);
        SetRegDW(pLan9118Data->dwLanBase, INT_STS, 0xFFFFFFFFUL);
        dwIntCfg |= INT_CFG_IRQ_EN_;
        SetRegDW(pLan9118Data->dwLanBase, INT_CFG, dwIntCfg);

        pLan9118Data->InterruptsInitialized = (BOOLEAN)TRUE;
    }
}

/*
FUNCTION: Lan_EnableSoftwareInterrupt
  Clears a flag in the LAN9118_DATA structure
  Sets the SW_INT_EN bit of the INT_EN register
  WARNING this has thread sychronization issues. Use with caution.
*/
void Lan_EnableSoftwareInterrupt(PLAN9118_DATA pLan9118Data)
{
    DWORD dwTemp = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        pLan9118Data->SoftwareInterruptSignal = (BOOLEAN)FALSE;
        dwTemp = GetRegDW(pLan9118Data->dwLanBase, INT_EN);
        dwTemp |= INT_EN_SW_INT_EN_;
        SetRegDW(pLan9118Data->dwLanBase, INT_EN, dwTemp);
    }
}

/*
FUNCTION: Lan_HandleSoftwareInterrupt
  Disables the SW_INT_EN bit of the INT_EN register,
  Clears the SW_INT in the INT_STS,
  Sets a flag in the LAN9118_DATA structure
*/
void Lan_HandleSoftwareInterrupt(PLAN9118_DATA pLan9118Data)
{
    DWORD dwTemp = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        dwTemp = GetRegDW(pLan9118Data->dwLanBase, INT_EN);
        dwTemp &= ~(INT_EN_SW_INT_EN_);
        SetRegDW(pLan9118Data->dwLanBase, INT_EN, dwTemp);
        SetRegDW(pLan9118Data->dwLanBase, INT_STS, INT_STS_SW_INT_);
        pLan9118Data->SoftwareInterruptSignal = (BOOLEAN)TRUE;
    }
}

/*
FUNCTION: Lan_IsSoftwareInterruptSignaled
  returns the set/cleared state of the flag used in
    Lan_EnableSoftwareInterrupt
    Lan_HandleSoftwareInterrupt
*/
BOOLEAN Lan_IsSoftwareInterruptSignaled(const LAN9118_DATA * const pLan9118Data)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        return pLan9118Data->SoftwareInterruptSignal;
    }
    else
    {
        return (BOOLEAN)FALSE;
    }
}

void Lan_SetMacAddress(PLAN9118_DATA const pLan9118Data, const DWORD dwHigh16, const DWORD dwLow32)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        Lan_SetMacRegDW(pLan9118Data->dwLanBase, ADDRH, dwHigh16);
        Lan_SetMacRegDW(pLan9118Data->dwLanBase, ADDRL, dwLow32);
        pLan9118Data->dwMacAddrHigh16 = dwHigh16;
        pLan9118Data->dwMacAddrLow32 = dwLow32;
    }
}

void Lan_GetMacAddress(PLAN9118_DATA const pLan9118Data, DWORD * const dwHigh16,DWORD * const dwLow32)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        *dwHigh16 = pLan9118Data->dwMacAddrHigh16=
                        Lan_GetMacRegDW(pLan9118Data->dwLanBase, ADDRH);
        *dwLow32 = pLan9118Data->dwMacAddrLow32=
                        Lan_GetMacRegDW(pLan9118Data->dwLanBase, ADDRL);
    }
}

void Lan_InitializeTx(PLAN9118_DATA const pLan9118Data)
{
    DWORD dwRegVal = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        SMSC_ASSERT(pLan9118Data->InterruptsInitialized == TRUE);
        SMSC_ASSERT(pLan9118Data->PhyInitialized == TRUE);

        // setup MAC for TX
        dwRegVal = Lan_GetMacRegDW(pLan9118Data->dwLanBase, MAC_CR);
        dwRegVal |= (MAC_CR_TXEN_ | MAC_CR_HBDIS_);
        Lan_SetMacRegDW(pLan9118Data->dwLanBase, MAC_CR, dwRegVal);

        //setup TLI store-and-forward, and preserve TxFifo size
        dwRegVal = GetRegDW(pLan9118Data->dwLanBase, HW_CFG);
        // some chips (may) use bit 11~0 
        dwRegVal &= (HW_CFG_TX_FIF_SZ_ | 0xFFFUL);
        dwRegVal |= HW_CFG_MBO_;
        SetRegDW(pLan9118Data->dwLanBase, HW_CFG, dwRegVal);

        SetRegDW(pLan9118Data->dwLanBase, TX_CFG, TX_CFG_TX_ON_);

        SetRegDW(pLan9118Data->dwLanBase, FIFO_INT, 0xFF000000UL);
        Lan_EnableInterrupt(pLan9118Data, INT_EN_TDFO_EN_ | INT_EN_TDFA_EN_);

        pLan9118Data->TxInitialized = (BOOLEAN)TRUE;
    }
}

void Lan_StartTx(const LAN9118_DATA * const pLan9118Data, const DWORD dwTxCmdA, const DWORD dwTxCmdB)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        
        SMSC_ASSERT(pLan9118Data->TxInitialized == TRUE);
        SetRegDW(pLan9118Data->dwLanBase, TX_DATA_FIFO_PORT ,dwTxCmdA);
        SetRegDW(pLan9118Data->dwLanBase, TX_DATA_FIFO_PORT, dwTxCmdB);
    }
}

void Lan_SendPacketPIO(const LAN9118_DATA * const pLan9118Data, const WORD wPacketTag, const WORD wPacketLength, BYTE * pbPacketData)
{
    DWORD dwTxCmdA;
    DWORD dwTxCmdB;

    SMSC_ASSERT(pLan9118Data);
    
    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        SMSC_ASSERT(pLan9118Data->TxInitialized == TRUE);
        if(wPacketTag == 0)
        {
            SMSC_WARNING0("Lan_SendPacketPIO(wPacketTag==0) Zero is reserved\n");
        }

        dwTxCmdA=(
            ((((DWORD)pbPacketData)&0x03UL)<<16) | //DWORD alignment adjustment
            TX_CMD_A_INT_FIRST_SEG_ | TX_CMD_A_INT_LAST_SEG_ | 
            ((DWORD)wPacketLength));
        dwTxCmdB=
            (((DWORD)wPacketTag)<<16) | 
            ((DWORD)wPacketLength);
        SetRegDW(pLan9118Data->dwLanBase,TX_DATA_FIFO_PORT,dwTxCmdA);
        SetRegDW(pLan9118Data->dwLanBase,TX_DATA_FIFO_PORT,dwTxCmdB);
        Lan_WriteTxFifo((PLAN9118_DATA const)pLan9118Data,
            pLan9118Data->dwLanBase,
            (DWORD *)(((DWORD)pbPacketData)&0xFFFFFFFCUL),
            ((DWORD)wPacketLength+3UL+
            (((DWORD)pbPacketData)&0x03UL))>>2);
    }
}

DWORD Lan_CompleteTx(const LAN9118_DATA * const pLan9118Data)
{
    DWORD result = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase!=0UL);
        SMSC_ASSERT(pLan9118Data->TxInitialized==TRUE);

        result = GetRegDW(pLan9118Data->dwLanBase,TX_FIFO_INF);
        result &= TX_FIFO_INF_TSUSED_;
        if(result != 0x00000000UL) {
            result = GetRegDW(pLan9118Data->dwLanBase,TX_STATUS_FIFO_PORT);
        } else {
            result = 0UL;
        }
    }
    return result;
}

DWORD Lan_GetTxDataFreeSpace(const LAN9118_DATA * const pLan9118Data)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        return GetRegDW(pLan9118Data->dwLanBase,TX_FIFO_INF) & TX_FIFO_INF_TDFREE_;
    }
    else
    {
        return 0x0UL;
    }
}

DWORD Lan_GetTxStatusCount(const LAN9118_DATA * const pLan9118Data)
{
    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        return ((GetRegDW(pLan9118Data->dwLanBase,TX_FIFO_INF) & (TX_FIFO_INF_TSUSED_)) >> 16) & 0xFFFFUL;
    }
    else
    {
        return 0x0UL;
    }
}


void Lan_InitializeRx(CPCLAN9118_DATA pLan9118Data, const DWORD dwRxCfg, const DWORD dw)
{
    DWORD dwRegVal = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);
        SMSC_ASSERT(pLan9118Data->InterruptsInitialized == TRUE);
        SMSC_ASSERT(pLan9118Data->PhyInitialized == TRUE);

        //set receive configuration
        SetRegDW(pLan9118Data->dwLanBase, RX_CFG, dwRxCfg);

        //set the interrupt levels to zero
        dwRegVal = GetRegDW(pLan9118Data->dwLanBase, FIFO_INT);
        dwRegVal &= 0xFFFF0000UL;
        dwRegVal |= (dw & 0x0000FFFFUL);
        SetRegDW(pLan9118Data->dwLanBase, FIFO_INT, dwRegVal);

        //enable interrupt
        Lan_EnableInterrupt(pLan9118Data, INT_EN_RSFL_EN_);
    }
}


DWORD Lan_PopRxStatus(CPCLAN9118_DATA pLan9118Data)
{
    DWORD result = 0UL;

    SMSC_ASSERT(pLan9118Data);

    if (pLan9118Data)
    {
        SMSC_ASSERT(pLan9118Data->dwLanBase != 0UL);

        result = GetRegDW(pLan9118Data->dwLanBase, RX_FIFO_INF);
        if(result & 0x00FF0000UL) {
            //Rx status is available, read it
            result = GetRegDW(pLan9118Data->dwLanBase,RX_STATUS_FIFO_PORT);
        } else {
            result = 0UL;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
    Lan_Wakeup9118
*/
BOOL Lan_Wakeup9118(CPCLAN9118_DATA pLan9118Data)
{
    DWORD dwPmeCtrlReg, timeout;
    const DWORD dwLanBase = pLan9118Data->dwLanBase;

    SMSC_TRACE0(DBG_POWER, "+Lan_Wakeup9118\r\n");
    
    // Assert nCS and nWR to wakeup device
    SetRegDW(dwLanBase, BYTE_TEST, 0UL);

    timeout=0x10000000UL;   //good timeout value?
    dwPmeCtrlReg = GetRegDW(dwLanBase, PMT_CTRL);
    while (((dwPmeCtrlReg & PMT_CTRL_READY_) == 0UL) && timeout)
    {
        dwPmeCtrlReg = GetRegDW(dwLanBase, PMT_CTRL);
        timeout--;
    }

    if (!timeout)
    {
        //Timed out.
        SMSC_WARNING0("Setting device power state (D0) timed out.\r\n");
        SMSC_TRACE0(DBG_POWER, "-Lan_Wakeup9118\r\n");
        return FALSE;
    }
    else
    {
        if (dwPmeCtrlReg & PMT_CTRL_PM_MODE_) 
        {
            SMSC_WARNING0("PMT_CTRL_PM_MODE_ still set when device is woken up.\r\n");
        }

        SMSC_TRACE0(DBG_POWER, "-Lan_Wakeup9118\r\n");
        return TRUE;
    }

}

