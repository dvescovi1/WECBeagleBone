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
//
//  File: SDHC.C
//  SDHC controller driver implementation (xldr and Eboot loader version)

#include "omap.h"
#include "omap_sdhc_regs.h"
#include "SDHC.h"
#include "SDHCRegs.h"
#include "bsp_cfg.h"
#include "soc_cfg.h"
#include "bsp_padcfg.h"


// set to true to enable more OALMSGs
// Caution: leave FALSE for XLOADER builds to keep size down
#define OALMSG_ENABLE   FALSE
//#define OALMSG_ENABLE   TRUE

#if OALMSG_ENABLE
    #define OALMSGX(z, m)   OALMSG(z, m)
#else
    #define OALMSGX(z, m)   {}
#endif

#ifndef OAL_FUNC
#define OAL_FUNC 1
#endif

DWORD g_bootSlot;


OMAP_MMCHS_REGS *m_pbRegisters;                     // SDHC controller registers

DWORD            m_dwSlot;
DWORD            m_dwSDIOCard;
BOOL             m_fCardInitialized;            // Card Initialized
DWORD            m_dwMaxClockRate;              // host controller's clock base
BOOL             m_fAppCmdMode;                 // if true, the controller is in App Cmd mode
BOOL             m_fCardPresent;                // a card is inserted and initialized
BOOL             m_fMMCMode;                    // if true, the controller assumed that the card inserted is MMC
UINT16           m_TransferClass;


// forward declarations
VOID SdhcSetClockRate(PDWORD pdwRate);
VOID SdhcSetInterface(DWORD mode);
static VOID MmcReset( DWORD dwResetBits );
static SD_API_STATUS GetCommandResponse(PSD_BUS_REQUEST pRequest);
static BOOL SDIPollingReceive(PBYTE pBuff, DWORD dwLen);
static BOOL SDIPollingTransmit(PBYTE pBuff, DWORD dwLen);
static SD_API_STATUS  CommandCompleteHandler(PSD_BUS_REQUEST pRequest);
#ifdef DEBUG
    VOID DumpRegisters();
#endif
    
#define MMC_INT_EN_MASK                     0x00330033

#define DEFAULT_TIMEOUT_VALUE               10000
#define START_BIT                           0x00
#define TRANSMISSION_BIT                    0x00
#define START_RESERVED                      0x3F
#define END_RESERVED                        0xFE
#define END_BIT                             0x01
#define SDIO_MAX_LOOP                       0x0080000

#define TRANSFER_SIZE(pRequest)            ((pRequest)->BlockSize * (pRequest)->NumBlocks)

#if defined(DEBUG)
    #define HEXBUFSIZE 1024
    char szHexBuf[HEXBUFSIZE];
#endif

typedef struct cmd_t
{
    BYTE Cmd;           // 1 - this is a known SD CMD; 2 - this is a known SDIO CMD
    BYTE ACmd;          // 1 - this is a known ACMD
    BYTE MMCCmd;        // 1 - this is a known MMC CMD
    DWORD flags;
} CMD;

// table of command codes...  at this time only SD/SDIO commands are implemented
const CMD gwaCMD[] =
{
    { 1, 0, 0, MMCHS_RSP_NONE }, // CMD 00
    { 0, 0, 1, MMCHS_CMD_CICE | MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 01 (known MMC command)
    { 1, 0, 1, MMCHS_RSP_LEN136 |MMCHS_CMD_CCCE | MMCHS_CMD_NORMAL }, // CMD 02
    { 1, 0, 1, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 03
    { 1, 0, 0, MMCHS_RSP_NONE  | MMCHS_CMD_NORMAL }, // CMD 04
    { 2, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 05
    { 0, 1, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 06
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 07
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL}, // CMD 08
    { 1, 0, 0, MMCHS_RSP_LEN136 | MMCHS_CMD_NORMAL }, // CMD 09
    { 1, 0, 0, MMCHS_RSP_LEN136 | MMCHS_CMD_NORMAL }, // CMD 10
    { 0, 0, 1, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_DP | MMCHS_CMD_NORMAL }, // CMD 11 (known MMC command)
    { 1, 0, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_ABORT }, // CMD 12
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 13
    { 0, 0, 0, 0 }, // CMD 14
    { 1, 0, 0, MMCHS_RSP_NONE | MMCHS_CMD_NORMAL }, // CMD 15
    { 1, 0, 0, MMCHS_CMD_CCCE | MMCHS_CMD_CICE | MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 16
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_DP | MMCHS_CMD_MSBS | MMCHS_CMD_BCE | MMCHS_CMD_NORMAL}, // CMD 17
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_MSBS | MMCHS_CMD_BCE | MMCHS_CMD_READ |MMCHS_CMD_DP|MMCHS_CMD_NORMAL }, // CMD 18
    { 0, 1, 0, 0 }, // CMD 19
    { 0, 1, 1, MMCHS_RSP_LEN48B | MMCHS_CMD_DP | MMCHS_CMD_NORMAL }, // CMD 20 (known MMC command)
    { 0, 1, 0, 0 }, // CMD 21
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_NORMAL }, // CMD 22
    { 0, 1, 0, 0 }, // CMD 23 (known MMC command)
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_DP | MMCHS_CMD_MSBS | MMCHS_CMD_BCE| MMCHS_CMD_NORMAL }, // CMD 24
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_DP | MMCHS_CMD_MSBS | MMCHS_CMD_BCE | MMCHS_CMD_NORMAL }, // CMD 25
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 26
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 27
    { 1, 0, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 28
    { 1, 0, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 29
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_NORMAL }, // CMD 30
    { 0, 0, 0, 0 }, // CMD 31
    { 1, 0, 0, 0 }, // CMD 32
    { 1, 0, 0, 0 }, // CMD 33
    { 0, 0, 0, 0 }, // CMD 34
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 35
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 36
    { 0, 0, 0, 0 }, // CMD 37
    { 1, 1, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 38
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 39 (known MMC command)
    { 1, 1, 1, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 40
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 41 (known MMC command)
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 42
    { 0, 1, 0, 0 }, // CMD 43
    { 0, 1, 0, 0 }, // CMD 44
    { 0, 1, 0, 0 }, // CMD 45
    { 0, 1, 0, 0 }, // CMD 46
    { 0, 1, 0, 0 }, // CMD 47
    { 0, 1, 0, 0 }, // CMD 48
    { 0, 1, 0, 0 }, // CMD 49
    { 0, 1, 0, 0 }, // CMD 50
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_DP| MMCHS_CMD_NORMAL }, // CMD 51 (known MMC command)
    { 2, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 52
    { 2, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_DP| MMCHS_CMD_NORMAL}, // CMD 53
    { 0, 0, 0, 0 }, // CMD 54
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 55
    { 1, 0, 0, 0 }, // CMD 56
    { 0, 0, 0, 0 }, // CMD 57
    { 0, 0, 0, 0 }, // CMD 58
    { 0, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 59
    { 0, 0, 0, 0 }, // CMD 60
    { 0, 0, 0, 0 }, // CMD 61
    { 0, 0, 0, 0 }, // CMD 62
    { 0, 0, 0, 0 }, // CMD 63
};


// Is the card present?
BOOL SdhcCardDetect()
{
	// no CD
        return TRUE;
}

static BOOL InitializeHardware(DWORD Slot)
{
    DWORD               dwCurrentTickCount;
    DWORD               dwTimeout;
    DWORD               dwCountStart;
    BOOL                fTimeoutOverflow = FALSE;

    m_pbRegisters = OALPAtoUA(GetAddressByDevice(SOCGetSDHCDeviceBySlot(Slot)));
    if(!m_pbRegisters)
    {
        OALMSGX(OAL_ERROR, (TEXT("Init HW: No SDHC device found\r\n")));
        return FALSE;
    }
             
    // Reset the controller
    OALMSG(OAL_INFO, (TEXT("Init HW: controller RST %d\r\n"),Slot));
    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_SOFTRESET);

    // calculate timeout conditions
    dwCountStart = OALGetTickCount();
    dwTimeout = dwCountStart + DEFAULT_TIMEOUT_VALUE;
    if ( dwTimeout < dwCountStart )
        fTimeoutOverflow = TRUE;

    // Verify that reset has completed.
    while (!(INREG32(&m_pbRegisters->MMCHS_SYSSTATUS) & MMCHS_SYSSTATUS_RESETDONE))
    {
        OALMSGX(OAL_INFO, (TEXT("Init HW: MMCHS_SYSSTATUS = 0x%X\r\n"), INREG32(&m_pbRegisters->MMCHS_SYSSTATUS)));

        // check for a timeout
        dwCurrentTickCount = OALGetTickCount();
        if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
            : ( dwTimeout < dwCurrentTickCount || dwCurrentTickCount < dwCountStart ) )
        {
            OALMSG(OAL_ERROR, (TEXT("InitializeHW: TIMEOUT\r\n")));
            return FALSE;
        }
    }
    return TRUE;
}


static DWORD GetBootMMCSlotConfig()
{
	DWORD pad0,pad1,bootCfg;
    AM33X_SYSC_PADCONFS_REGS *pPadRegs;

	pPadRegs = (AM33X_SYSC_PADCONFS_REGS *)OALPAtoUA(AM33X_SYSC_PADCONFS_REGS_PA);

	bootCfg = 0x1f & INREG32(OALPAtoUA(AM33X_DEVICE_BOOT_REGS_PA));

	pad0 = INREG32(&(pPadRegs->CONF_MMC0_CMD));
//	OALMSG(OAL_INFO,(TEXT("Slot 1 %d\r\n"),pad0));
	// check GPMC_CSN2 which is used as MMC1_CMD pad
	pad1 = INREG32(&(pPadRegs->CONF_GPMC_CSN2));
//	OALMSG(OAL_INFO,(TEXT("Slot 2 %d\r\n"),pad1)); 

	// user button pressed and MMC0 pads configured by firmware
	if (0x18 == bootCfg && 0x30 == pad0)
	{
		OALMSG(OAL_INFO,(TEXT("Slot 1 boot\r\n"))); 
		return 1; /* MMCSLOT_1 */
	}

	// normal boot and MMC1 pads configured by firmware
	if (0x1c == bootCfg && 0x32 == pad1)
	{
		OALMSG(OAL_INFO,(TEXT("Slot 2 boot\r\n"))); 
		return 2; /* MMCSLOT_2 */
	}

	OALMSG(OAL_INFO,(TEXT("Slot 1 boot\r\n"))); 
	return 1; /* MMCSLOT_1 */
}



static void SdhcControllerInit()
{
    DWORD dwClockRate;

    m_fAppCmdMode = FALSE;

    m_pbRegisters = NULL;
    m_fCardPresent = FALSE;

    m_fMMCMode = FALSE;

    g_bootSlot = GetBootMMCSlotConfig();
    m_dwSlot = g_bootSlot;
    m_dwSDIOCard = 0;

    // initialize dvfs variables
    m_fCardInitialized = FALSE;
    m_TransferClass = 0;

    m_dwMaxClockRate = STD_HC_MAX_CLOCK_FREQUENCY;

    InitializeHardware(m_dwSlot);

    dwClockRate = MMCSD_CLOCK_INIT;
    SdhcSetClockRate(&dwClockRate);
    
    // use 1 bit MMC mode ...to start
    SdhcSetInterface(SD_INTERFACE_SD_MMC_1BIT);

    // Initialize the slot
    MmcReset(SOFT_RESET_ALL);
    OALStall(10 * 1000); // Allow time for card to power down after a device reset

	// Enable interrupts
    OUTREG32(&m_pbRegisters->MMCHS_ISE, MMC_INT_EN_MASK);
    OUTREG32(&m_pbRegisters->MMCHS_IE,  MMC_INT_EN_MASK);

#ifdef DEBUG
    DumpRegisters();
#endif
    m_dwSDIOCard = 0;
}

//  Reset the controller.
static VOID MmcReset( DWORD dwResetBits )
{
    DWORD               dwCurrentTickCount;
    DWORD               dwTimeout;
    DWORD               dwCountStart;
    BOOL                fTimeoutOverflow = FALSE;

    OALMSGX(OAL_INFO, (TEXT("MMC Reset\r\n")));

    dwResetBits &= (MMCHS_SYSCTL_SRA | MMCHS_SYSCTL_SRC | MMCHS_SYSCTL_SRD);

    // Reset the controller
    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, dwResetBits);

    // calculate timeout conditions
    dwCountStart = OALGetTickCount();

    dwTimeout = dwCountStart + DEFAULT_TIMEOUT_VALUE;
    if ( dwTimeout < dwCountStart )
        fTimeoutOverflow = TRUE;

    // Verify that reset has completed.
    while ((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & dwResetBits))
    {
        // check for a timeout
        dwCurrentTickCount = OALGetTickCount();
        if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
            : ( dwTimeout < dwCurrentTickCount ) )
        {
            OALMSG(OAL_ERROR, (TEXT("MMC Reset timeout\r\n")));
            break;
        }
    }
    // enable autoidle, disable wakeup, enable smart-idle, ClockActivity (interface and functional clocks may be switched off)
    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_AUTOIDLE | MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_SMART));
}


// Set up the controller according to the interface parameters.
VOID SdhcSetInterface(DWORD mode)
{
    if (SD_INTERFACE_SD_4BIT == mode)
    {
        OALMSGX(OAL_INFO, (TEXT("SDHC: 4 bit mode\r\n")));
        SETREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_DTW);
    }
    else if (SD_INTERFACE_MMC_8BIT == mode)
    {
        OALMSGX(OAL_INFO, (TEXT("SDHC: 8 bit mode\r\n")));
        CLRREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_DTW);
        SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_DW8);		// MMC 8bit
    }
    else //SD_INTERFACE_SD_MMC_1BIT
    {
        OALMSGX(OAL_INFO, (TEXT("SDHC: 1 bit mode\r\n")));
        CLRREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_DTW);
    }
}

//  Set clock rate based on HC capability
VOID SdhcSetClockRate(PDWORD pdwRate)
{
    DWORD dwTimeout = 500;
    INT32 dwRegValue;
    DWORD dwDiv;
    DWORD dwClockRate = *pdwRate;

    OALMSGX(OAL_FUNC, (TEXT("SdhcSetClockRate %d\r\n"), *pdwRate));

    if (dwClockRate > m_dwMaxClockRate)
        dwClockRate = m_dwMaxClockRate;

    // calculate the register value
    dwDiv = (DWORD)((MMCSD_CLOCK_INPUT + dwClockRate - 1) / dwClockRate);

    //OALMSGX(OAL_INFO, (TEXT("actual wDiv = 0x%x  requested:0x%x"), dwDiv, *pdwRate));
    // Only 10 bits available for the divider, so mmc base clock / 1024 is minimum.
    if ( dwDiv > 0x03FF )
        dwDiv = 0x03FF;

    //OALMSGX(OAL_INFO, (TEXT("dwDiv = 0x%x 0x%x"), dwDiv, *pdwRate));

    // Program the divisor, but leave the rest of the register alone.
    dwRegValue = INREG32(&m_pbRegisters->MMCHS_SYSCTL);

    dwRegValue = (dwRegValue & ~MMCHS_SYSCTL_CLKD_MASK) | MMCHS_SYSCTL_CLKD(dwDiv);
    dwRegValue = (dwRegValue & ~MMCHS_SYSCTL_DTO_MASK) | MMCHS_SYSCTL_DTO(0x0e); // DTO
    dwRegValue &= ~MMCHS_SYSCTL_CEN;
    dwRegValue &= ~MMCHS_SYSCTL_ICE;

    CLRREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);

    OUTREG32(&m_pbRegisters->MMCHS_SYSCTL, dwRegValue);

    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_ICE); // enable internal clock

    dwTimeout = 500;
    while (((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & MMCHS_SYSCTL_ICS) != MMCHS_SYSCTL_ICS) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);
    SETREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_SDBP); // power up the card

    dwTimeout = 500;
    while (((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & MMCHS_SYSCTL_CEN) != MMCHS_SYSCTL_CEN) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    *pdwRate = MMCSD_CLOCK_INPUT / dwDiv;
    OALMSGX(OAL_INFO, (TEXT("SDHC: clock = %d\r\n"), *pdwRate));
}

static VOID SetSDVSVoltage()
{
    UINT32 val1, val2;

	// slot 1 defaults to 3.0v but can optionally be set to 1.8v
	val1 = MMCHS_CAPA_VS30;
	val2 = MMCHS_HCTL_SDVS_3V0;

#ifdef MMCHS1_LOW_VOLTAGE
	val1 = MMCHS_CAPA_VS18;
	val2 = MMCHS_HCTL_SDVS_1V8;
#endif
	if (m_dwSlot == MMCSLOT_2)	// slot 2
	{
		val1 = MMCHS_CAPA_VS18;
		val2 = MMCHS_HCTL_SDVS_1V8;
	}
		
	SETREG32(&m_pbRegisters->MMCHS_CAPA, val1);
	SETREG32(&m_pbRegisters->MMCHS_HCTL, val2);
}


// Send command without response
static SD_API_STATUS SendCmdNoResp(DWORD cmd, DWORD arg)
{
    DWORD MMC_CMD;
    DWORD dwTimeout;

    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    dwTimeout = 80000;
    while (((INREG32(&m_pbRegisters->MMCHS_PSTATE) & MMCHS_PSTAT_CMDI)) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    MMC_CMD = MMCHS_INDX(cmd);
    MMC_CMD |= gwaCMD[cmd].flags;

    // Program the argument into the argument registers
    OUTREG32(&m_pbRegisters->MMCHS_ARG, arg);
    // Issue the command.
    OUTREG32(&m_pbRegisters->MMCHS_CMD, MMC_CMD);

    dwTimeout = 5000;
    while (dwTimeout > 0)
    {
        dwTimeout --;
        if (INREG32(&m_pbRegisters->MMCHS_STAT) & (MMCHS_STAT_CC | MMCHS_STAT_CTO | MMCHS_STAT_CERR)) 
            break;
    }

    OUTREG32(&m_pbRegisters->MMCHS_STAT, INREG32(&m_pbRegisters->MMCHS_STAT));
    // always return 0 if no response needed
    return SD_API_STATUS_SUCCESS;
}

// Send init sequence to card
static VOID SendInitSequence()
{
    DWORD dwCount;

    OUTREG32(&m_pbRegisters->MMCHS_IE,  0xFFFFFEFF);
    SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_INIT);

    for (dwCount = 0; dwCount < 10; dwCount ++)
    {
        SendCmdNoResp(SD_CMD_GO_IDLE_STATE, 0xFFFFFFFF);
    }
    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    CLRREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_INIT);
}

// Issues the specified SDI command
static SD_API_STATUS SendCommand(PSD_BUS_REQUEST pRequest)
{
    DWORD MMC_CMD;
    DWORD dwTimeout;
    DWORD Cmd = pRequest->CommandCode;
    DWORD Arg = pRequest->CommandArgument;
    UINT16 respType = pRequest->CommandResponse.ResponseType;
    DWORD dwRegVal;

    m_TransferClass = pRequest->TransferClass;

    OALMSGX(OAL_IO, (TEXT("SendCommand() - Cmd = 0x%x Arg = 0x%x respType = 0x%x m_TransferClass = 0x%x\r\n"),
        Cmd, Arg, respType, m_TransferClass));

    if ((Cmd == SD_CMD_IO_RW_EXTENDED) || (Cmd == SD_CMD_IO_RW_DIRECT))
    {
        m_dwSDIOCard = 1;
    } 
    else if ((Cmd == SD_CMD_MMC_SEND_OPCOND) || (Cmd == SD_CMD_GO_IDLE_STATE))
    {
        m_dwSDIOCard = 0;
    }

    if ( m_TransferClass == SD_READ || m_TransferClass == SD_WRITE )
    {
        OALMSGX(OAL_IO, (TEXT("SendCommand RW (Cmd=0x%X, Arg=0x%x, RespType=0x%X, Data=0x%x <%dx%d>) starts\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE, pRequest->NumBlocks, pRequest->BlockSize ) );
    }
    else
    {
        OALMSGX(OAL_IO, (TEXT("SendCommand (Cmd=0x%X, Arg=0x%x, RespType=0x%X, Data=0x%x) starts\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE) );
    }

    OUTREG32(&m_pbRegisters->MMCHS_STAT,0xFFFFFFFF);
    dwTimeout = 2000;
    while (((INREG32(&m_pbRegisters->MMCHS_PSTATE) & MMCHS_PSTAT_CMDI)) && (dwTimeout>0))
    {
        dwTimeout--;
    }
    MMC_CMD = MMCHS_INDX(Cmd);

    MMC_CMD |= gwaCMD[Cmd].flags;
    if ((Cmd == SD_CMD_SELECT_DESELECT_CARD) && (respType == NoResponse))
    {
        MMC_CMD &= ~MMCHS_RSP_MASK;
        MMC_CMD |= MMCHS_RSP_NONE;
    }

    MMC_CMD &= ~MMCHS_CMD_DE;

    if (Cmd == SD_CMD_IO_RW_EXTENDED)
    {
        if (pRequest->NumBlocks > 1)
        {
           MMC_CMD |= MMCHS_CMD_MSBS | MMCHS_CMD_BCE;
        }
    }

    if ( m_TransferClass == SD_READ )
    {
        MMC_CMD |= MMCHS_CMD_DDIR;

        dwRegVal = (DWORD)(pRequest->BlockSize & 0xFFFF);
        dwRegVal += ((DWORD)(pRequest->NumBlocks & 0xFFFF)) << 16;
		OALMSGX(OAL_IO, (TEXT("SendCommand() - BlockSize = %d, NumBlocks = %d, MMCHS_BLK = 0x%x\r\n"), pRequest->BlockSize, pRequest->NumBlocks, dwRegVal));
        OUTREG32(&m_pbRegisters->MMCHS_BLK, dwRegVal);
    }

    //check for card initialization is done.
    if (!m_fCardInitialized && (Cmd == SD_CMD_READ_SINGLE_BLOCK))
        m_fCardInitialized = TRUE;

    // Program the argument into the argument registers
    OUTREG32(&m_pbRegisters->MMCHS_ARG, Arg);

    OALMSGX(OAL_IO, (TEXT("SendCommand() - registers:Command = 0x%x, MMCHS_ARG = 0x%x\r\n"), MMC_CMD, INREG32(&m_pbRegisters->MMCHS_ARG)));

    // Issue the command.
    OUTREG32(&m_pbRegisters->MMCHS_CMD, MMC_CMD);

    return SD_API_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

// Initialize the card
VOID SdhcHandleInsertion()
{
    DWORD dwClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;
    DWORD dwTimeout;

    m_fCardPresent = TRUE;
    m_dwSDIOCard = 0;

    OALMSGX(OAL_INFO, (TEXT("HandleInsertion\r\n")));

    MmcReset(SOFT_RESET_ALL);

    // Check for debounce stable
    dwTimeout = 5000;
    while (((INREG32(&m_pbRegisters->MMCHS_PSTATE) & 0x00020000)!= 0x00020000) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    OUTREG32(&m_pbRegisters->MMCHS_CON, 0x01 << 7); // CDP

    SetSDVSVoltage();

    SdhcSetClockRate(&dwClockRate);

	// Enable interrupts
    OUTREG32(&m_pbRegisters->MMCHS_ISE, MMC_INT_EN_MASK);
    OUTREG32(&m_pbRegisters->MMCHS_IE,  MMC_INT_EN_MASK);

	//TODO:??
	// set transfer for 512 bytes 1 block ... we always issue READ_SINGLE_BLOCK
    OUTREG32(&m_pbRegisters->MMCHS_BLK, (SDMMC_DEFAULT_BLOCK_LEN + (SDMMC_DEFAULT_NUM_BLOCKS<<16)));
    OALMSGX(OAL_IO, (TEXT("read back MMCHS_BLK = 0x%x\r\n"), INREG32(&m_pbRegisters->MMCHS_BLK)));
}

//#ifdef DEBUG

// Reads from SD Standard Host registers and writes them to the debugger.
VOID DumpRegisters()
{
    OALMSGX(OAL_INFO, (TEXT("+DumpStdHCRegs-------------------------\r\n")));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_CMD       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CMD)    ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_ARG       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_ARG)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_CON       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CON)   ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_PWCNT     0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_PWCNT)   ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_STAT      0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_STAT)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_PSTATE    0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_PSTATE)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_IE        0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_IE)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_ISE       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_ISE)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_BLK       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_BLK)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_REV       0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_REV)    ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_RSP10     0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP10)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_RSP32     0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP32)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_RSP54     0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP54)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_RSP76     0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP76)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_HCTL      0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_HCTL)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_SYSCTL    0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_SYSCTL)  ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_SYSCONFIG 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_SYSCONFIG) ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_CAPA      0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CAPA) ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_CUR_CAPA  0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CUR_CAPA) ));
    OALMSGX(OAL_INFO, (TEXT("MMCHS_SYSTEST   0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_SYSTEST) ));
    OALMSGX(OAL_INFO, (TEXT("-DumpStdHCRegs-------------------------\r\n")));
}

//#endif

///////////////////////////////////////////////////////////////////////////////
//  SDHCControllerIstThread - implementation of SDIO/controller IST thread
//                                for driver
//  Input:
//  Output:
//  Return: Thread exit status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
BOOL SdhcControllerIstThread(PSD_BUS_REQUEST pRequest)
{
    DWORD dwStat;
    SD_API_STATUS Status = SD_API_STATUS_PENDING;
    
    // check for interrupt pending
    dwStat = INREG32(&m_pbRegisters->MMCHS_STAT);
    dwStat &= INREG32(&m_pbRegisters->MMCHS_IE);
    if ( dwStat & (MMCHS_STAT_CC|MMCHS_STAT_CERR|MMCHS_STAT_CCRC|MMCHS_STAT_CTO|MMCHS_STAT_DTO|MMCHS_STAT_DCRC) )
    {
        Status = CommandCompleteHandler(pRequest);
    }

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDHCInitialize - Initialize the the controller
//  Input:
//  Output:
//  Return: SD_API_STATUS
//  Notes:
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SdhcInitialize()
{
    SD_API_STATUS status = SD_API_STATUS_INSUFFICIENT_RESOURCES; // intermediate status

    OALMSGX(OAL_INFO, (TEXT("SDHC init\r\n")));

    SdhcControllerInit();

    status = SD_API_STATUS_SUCCESS;

    return status;
}


///////////////////////////////////////////////////////////////////////////////
//  SdhcBusRequestHandler - bus request handler
//  Input:  pRequest - the request
//
//  Output:
//  Return: SD_API_STATUS
//  Notes:  The request passed in is marked as uncancelable, this function
//          has the option of making the outstanding request cancelable
//          returns status pending
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS SdhcBusRequestHandler(PSD_BUS_REQUEST pRequest)
{
    SD_API_STATUS   status;

    OALMSGX(OAL_IO, (TEXT("SDHCBusRequestHandler - CMD::[%d]\r\n"), pRequest->CommandCode));

    status = SendCommand(pRequest);

    if (!SD_API_SUCCESS(status))
    {
        DEBUGMSG(OAL_ERROR, (TEXT("SDHCDBusRequestHandler() - Error sending command:0x%x\r\n"), pRequest->CommandCode));
        goto cleanUp;      
    }

    // we will handle the command response interrupt on another thread
    status = SD_API_STATUS_PENDING;

cleanUp:

    return status;
}

//  CommandCompleteHandler
//  Input:
//  Output:
//  Notes:
static SD_API_STATUS CommandCompleteHandler(PSD_BUS_REQUEST pRequest)
{
    DWORD               dwCurrentTickCount;
    DWORD               dwTimeout;
    DWORD               dwCountStart;
    BOOL                fTimeoutOverflow = FALSE;
    SD_API_STATUS       status = SD_API_STATUS_PENDING;
    DWORD MMC_STAT;
    DWORD MmcPstateRegValue;
    DWORD MmcStatBits;

    OALMSGX(OAL_FUNC, (TEXT("SDHC CommandCompleteHandler\r\n")));

    MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
    MmcPstateRegValue = INREG32(&m_pbRegisters->MMCHS_PSTATE);

    if ( MmcPstateRegValue & MMCHS_PSTAT_DATI )
    {
        if ( pRequest->CommandResponse.ResponseType == ResponseR1b )
        {
            OALMSGX(OAL_WARN, (TEXT("SDHC: Card busy after command\r\n")));
            // calculate timeout conditions
            dwCountStart = OALGetTickCount();
            dwTimeout = dwCountStart + DEFAULT_TIMEOUT_VALUE;
            if ( dwTimeout < dwCountStart )
                fTimeoutOverflow = TRUE;

            MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
            MmcPstateRegValue = INREG32(&m_pbRegisters->MMCHS_PSTATE);

            while ( (MmcPstateRegValue & MMCHS_PSTAT_DATI) && !( MMC_STAT & ( MMCHS_STAT_CCRC | MMCHS_STAT_CTO | MMCHS_STAT_DCRC | MMCHS_STAT_DTO )) )
            {
                OALStall(2 * 1000);

                MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
                MmcPstateRegValue = INREG32(&m_pbRegisters->MMCHS_PSTATE);

                // check for card ejection
                if ( !SdhcCardDetect() )
                {
                    OALMSGX(OAL_ERROR, (TEXT("SDHC: Card removed!\r\n")));
                    status = SD_API_STATUS_DEVICE_REMOVED;
                    goto TRANSFER_DONE;
                }

                // check for a timeout
                dwCurrentTickCount = OALGetTickCount();
                if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
                    : ( dwTimeout < dwCurrentTickCount ) )
                {
                    OALMSGX(OAL_ERROR, (TEXT("SDHC: Card BUSY timeout!\r\n")));
                    status = SD_API_STATUS_RESPONSE_TIMEOUT;
                    goto TRANSFER_DONE;
                }
            }

            //OALMSGX(OAL_WARN, (TEXT("Card exited busy state.\r\n")));
        }
    }

    MmcStatBits = 0;

    if ( MMC_STAT & MMCHS_STAT_CCRC ) // command CRC error
    {
        status = SD_API_STATUS_CRC_ERROR;
        MmcStatBits |= MMCHS_STAT_CCRC;
        OALMSGX(OAL_ERROR, (TEXT("SDHC: command CRC error!\r\n")));
    }
    if ( MMC_STAT & MMCHS_STAT_DTO ) // data timeout
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MmcStatBits |= MMCHS_STAT_DTO;
        OALMSGX(OAL_ERROR, (TEXT("SDHC: command response timeout DTO!\r\n")));
    }
    if ( MMC_STAT & MMCHS_STAT_DCRC ) // data CRC error
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MmcStatBits |= MMCHS_STAT_DCRC;
        OALMSGX(OAL_ERROR, (TEXT("SDHC: command response timeout DCRC!\r\n")));
    }
    if ( MMC_STAT & MMCHS_STAT_CTO ) // command response timeout
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MmcStatBits |= MMCHS_STAT_CTO;
        OALMSGX(OAL_ERROR, (TEXT("SDHC: command response timeout CTO!\r\n")));
    }
    if ( MmcStatBits ) 
    {
        // clear the status error bits
        OUTREG32(&m_pbRegisters->MMCHS_STAT,MmcStatBits);
        goto TRANSFER_DONE;
    }

    // get the response information
    if (pRequest->CommandResponse.ResponseType == NoResponse)
    {
        OALMSGX(OAL_IO, (TEXT("SDHC: no response (none expected)\r\n")));
        status = SD_API_STATUS_SUCCESS;
        goto TRANSFER_DONE;
    }
    else
    {
        status =  GetCommandResponse(pRequest);
        if (!SD_API_SUCCESS(status))
        {
            OALMSGX(OAL_ERROR, (TEXT("SDHC: Error getting response for command:0x%x\r\n"), pRequest->CommandCode));
            goto TRANSFER_DONE;     
        }
    }

    if (SD_COMMAND != pRequest->TransferClass) // data transfer
    {
        DWORD cbTransfer = TRANSFER_SIZE(pRequest);
        BOOL     fRet;

        switch (pRequest->TransferClass)
        {
        case SD_READ:
            fRet = SDIPollingReceive(pRequest->pBlockBuffer, cbTransfer);
            if (!fRet)
            {
                OALMSGX(OAL_ERROR, (TEXT("SDHC: SDIPollingReceive() failed\r\n")));
                status = SD_API_STATUS_DATA_ERROR;
                goto TRANSFER_DONE;
            }
            else
            {
#ifdef DEBUG
                DWORD dwTemp = 0;
                while ( dwTemp < cbTransfer && (dwTemp < (HEXBUFSIZE / 2 - 1) ) )
                {
                    szHexBuf[dwTemp*2] = pRequest->pBlockBuffer[dwTemp] / 16;
                    szHexBuf[dwTemp*2+1] = pRequest->pBlockBuffer[dwTemp] % 16;

                    if ( szHexBuf[dwTemp*2] < 10 )
                        szHexBuf[dwTemp*2] += '0';
                    else
                        szHexBuf[dwTemp*2] += 'a' - 10;

                    if ( szHexBuf[dwTemp*2+1] < 10 )
                        szHexBuf[dwTemp*2+1] += '0';
                    else
                        szHexBuf[dwTemp*2+1] += 'a' - 10;

                    dwTemp++;
                }
                szHexBuf[dwTemp*2] = 0;
                OALMSGX(OAL_IO, (TEXT("PollingReceive succesfully received %d bytes\r\n  {%S}\r\n"), cbTransfer, szHexBuf));
#endif
            }
            break;
        }

        if (!m_fCardPresent)
            status = SD_API_STATUS_DEVICE_REMOVED;
        else
            status = SD_API_STATUS_SUCCESS;
    }

TRANSFER_DONE:

    if ( status == SD_API_STATUS_SUCCESS )
    {
        if ( m_fAppCmdMode )
        {
            m_fAppCmdMode = FALSE;
            OALMSGX(OAL_IO, (TEXT("SDHC: go to Standard Command Mode\r\n")));
        }
        else if ( pRequest && pRequest->CommandCode == 55 )
        {
            m_fAppCmdMode = TRUE;
            OALMSGX(OAL_IO, (TEXT("SDHC: go to Application Specific Command Mode\r\n")));
        }

        if ( pRequest->CommandCode == SD_CMD_MMC_SEND_OPCOND )
        {
            OALMSGX(OAL_IO, (TEXT("SDHC: Card is MMC\r\n") ) );
            m_fMMCMode = TRUE;
        }
    }

    // Clear the MMC_STAT register
    MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
    OUTREG32(&m_pbRegisters->MMCHS_STAT,MMC_STAT); 
    //UpdateSystemClock(FALSE);

    return status;
}


//Function:     GetCommandResponse()
//Description:  Retrieves the response info for the last SDI command
//              issues.
//Notes:
//Returns:      SD_API_STATUS status code.
static SD_API_STATUS GetCommandResponse(PSD_BUS_REQUEST pRequest)
{
    DWORD  dwRegVal;
    PUCHAR  respBuff;       // response buffer
    DWORD dwRSP;

    dwRegVal = INREG32(&m_pbRegisters->MMCHS_STAT);

    OALMSGX(OAL_IO, (TEXT("SDHC: MMC_STAT = 0x%X.\r\n"), dwRegVal));


    if ( dwRegVal & (MMCHS_STAT_CC | MMCHS_STAT_CERR | MMCHS_STAT_CCRC))
    {
        respBuff = pRequest->CommandResponse.ResponseBuffer;

        switch (pRequest->CommandResponse.ResponseType)
        {
            case NoResponse:
                break;

            case ResponseR1:
            case ResponseR1b:
                //--- SHORT RESPONSE (48 bits total)---
                // Format: { START_BIT(1) | TRANSMISSION_BIT(1) | COMMAND_INDEX(6) | CARD_STATUS(32) | CRC7(7) | END_BIT(1) }
                // NOTE: START_BIT and TRANSMISSION_BIT = 0, END_BIT = 1
                //
                // Dummy byte needed by calling function.
                *respBuff = (BYTE)(START_BIT | TRANSMISSION_BIT | pRequest->CommandCode);

                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP10);

                *(respBuff + 1) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 2) = (BYTE)(dwRSP >> 8);
                *(respBuff + 3) = (BYTE)(dwRSP >> 16);
                *(respBuff + 4) = (BYTE)(dwRSP >> 24);


                *(respBuff + 5) = (BYTE)(END_RESERVED | END_BIT);

                OALMSGX(OAL_IO, (TEXT("GetCommandResponse() - R1 R1b : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \r\n"), *(respBuff + 0),
                    *(respBuff + 1), *(respBuff + 2), *(respBuff + 3), *(respBuff + 4), *(respBuff + 5)));
                OALMSGX(OAL_IO, (TEXT("GetCmdResponse returned [%x %x %x %x %x]\r\n"),
                    respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
                break;

            case ResponseR3:
            case ResponseR4:
            case ResponseR7:
                OALMSGX(OAL_IO, (TEXT("ResponseR3 ResponseR4\r\n")));
                //--- SHORT RESPONSE (48 bits total)---
                // Format: { START_BIT(1) | TRANSMISSION_BIT(1) | RESERVED(6) | CARD_STATUS(32) | RESERVED(7) | END_BIT(1) }
                //
                *respBuff = (BYTE)(START_BIT | TRANSMISSION_BIT | START_RESERVED);

                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP10);

                *(respBuff + 1) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 2) = (BYTE)(dwRSP >> 8);
                *(respBuff + 3) = (BYTE)(dwRSP >> 16);
                *(respBuff + 4) = (BYTE)(dwRSP >> 24);

                *(respBuff + 5) = (BYTE)(END_RESERVED | END_BIT);

                OALMSGX(OAL_IO, (TEXT("GetCmdResponse returned [%x %x %x %x %x]\r\n"),
                    respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
                break;

            case ResponseR5:
            case ResponseR6:
                OALMSGX(OAL_IO, (TEXT("ResponseR5 ResponseR6\r\n")));
                //--- SHORT RESPONSE (48 bits total)---
                // Format: { START_BIT(1) | TRANSMISSION_BIT(1) | COMMAND_INDEX(6) | RCA(16) | CARD_STATUS(16) | CRC7(7) | END_BIT(1) }
                //
                *respBuff = (BYTE)(START_BIT | TRANSMISSION_BIT | pRequest->CommandCode);

                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP10);

                *(respBuff + 1) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 2) = (BYTE)(dwRSP >> 8);
                *(respBuff + 3) = (BYTE)(dwRSP >> 16);
                *(respBuff + 4) = (BYTE)(dwRSP >> 24);

                *(respBuff + 5) = (BYTE)(END_BIT);

                OALMSGX(OAL_IO, (TEXT("GetCommandResponse() - R5 R6 : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \r\n"), *(respBuff + 0),
                    *(respBuff + 1), *(respBuff + 2), *(respBuff + 3), *(respBuff + 4), *(respBuff + 5)));

                OALMSGX(OAL_IO, (TEXT("GetCmdResponse returned [%x %x %x %x %x]\r\n"),
                    respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
                break;

            case ResponseR2:
                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP10);

                *(respBuff + 0) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 1) = (BYTE)(dwRSP >> 8);
                *(respBuff + 2) = (BYTE)(dwRSP >> 16);
                *(respBuff + 3) = (BYTE)(dwRSP >> 24);

                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP32);

                *(respBuff + 4) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 5) = (BYTE)(dwRSP >> 8);
                *(respBuff + 6) = (BYTE)(dwRSP >> 16);
                *(respBuff + 7) = (BYTE)(dwRSP >> 24);

                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP54);

                *(respBuff + 8) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 9) = (BYTE)(dwRSP >> 8);
                *(respBuff + 10) = (BYTE)(dwRSP >> 16);
                *(respBuff + 11) = (BYTE)(dwRSP >> 24);


                dwRSP = INREG32(&m_pbRegisters->MMCHS_RSP76);

                *(respBuff + 12) = (BYTE)(dwRSP & 0xFF);
                *(respBuff + 13) = (BYTE)(dwRSP >> 8);
                *(respBuff + 14) = (BYTE)(dwRSP >> 16);
                *(respBuff + 15) = (BYTE)(dwRSP >> 24);

                OALMSGX(OAL_IO, (TEXT("GetCmdResponse returned [%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x]\r\n"),
                    respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5], respBuff[6], respBuff[7],
                    respBuff[8], respBuff[9], respBuff[10], respBuff[11], respBuff[12], respBuff[13], respBuff[14], respBuff[15]));
                break;

            default:
                OALMSGX(OAL_ERROR, (TEXT("GetCommandResponse() - Unrecognized response type!\r\n")));
                break;
        }
    }
    return SD_API_STATUS_SUCCESS;
}

//Function:     SDIPollingReceive()
//Description:
//Notes:        This routine assumes that the caller has already locked
//              the current request and checked for errors.
//Returns:      SD_API_STATUS status code.
static BOOL SDIPollingReceive(PBYTE pBuff, DWORD dwLen)
{
    DWORD fifoSizeW, blockLengthW; // Almost Full level and block length
    DWORD dwCount1, dwCount2;
    DWORD MMC_STAT;
    DWORD MmcPstateRegValue;
    //DWORD *pbuf = (DWORD *) pBuff;
    DWORD __unaligned *pbuf2 = (DWORD *) pBuff;
    DWORD dwCurrentTickCount;
    DWORD dwTimeout;
    DWORD dwCountStart;
    BOOL fTimeoutOverflow = FALSE;

    OALMSGX(OAL_IO, (TEXT("SDIPollingReceive(0x%x, %d)\r\n"), pBuff, dwLen));
    //check the parameters

    OALMSGX(OAL_IO, (TEXT("SDIPollingReceive reading MMC_STAT\r\n")));
    MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
    OALMSGX(OAL_IO, (TEXT("SDIPollingReceive reading MMCHS_PSTATE\r\n")));
    MmcPstateRegValue = INREG32(&m_pbRegisters->MMCHS_PSTATE);

    // calculate timeout conditions
    OALMSGX(OAL_IO, (TEXT("SDIPollingReceive OALGetTickCount\r\n")));
    dwCountStart = OALGetTickCount();
    dwTimeout = dwCountStart + DEFAULT_TIMEOUT_VALUE;
    if ( dwTimeout < dwCountStart )
        fTimeoutOverflow = TRUE;

    if (dwLen % SDMMC_DEFAULT_BLOCK_LEN || m_dwSDIOCard)
    {
        while ((INREG32(&m_pbRegisters->MMCHS_STAT) & MMCHS_STAT_BRR) != MMCHS_STAT_BRR)
        {
           // check for a timeout
           dwCurrentTickCount = OALGetTickCount();
           if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
             : ( dwTimeout < dwCurrentTickCount || dwCurrentTickCount < dwCountStart ) )
           {
              OALMSGX(OAL_ERROR, (TEXT("SDIPollingReceive: TIMEOUT0\r\n")));
              goto READ_ERROR;
           }
        }
        SETREG32(&m_pbRegisters->MMCHS_STAT,MMCHS_STAT_BRR);
        fifoSizeW = dwLen / sizeof(DWORD);
        if (dwLen % sizeof(DWORD)) fifoSizeW++;
        for (dwCount2 = 0; dwCount2 < fifoSizeW; dwCount2++)
        {
            *pbuf2 = INREG32(&m_pbRegisters->MMCHS_DATA);
            pbuf2++;
        }
    } 
    else
    {
      OALMSGX(OAL_IO, (TEXT("SDIPollingReceive ready to read data\r\n")));
      fifoSizeW = INREG32(&m_pbRegisters->MMCHS_BLK) & 0xFFFF;
      OALMSGX(OAL_IO, (TEXT("SDIPollingReceive fifoSizeW %d\r\n"), fifoSizeW));
      blockLengthW = dwLen / fifoSizeW;
      OALMSGX(OAL_IO, (TEXT("SDIPollingReceive blockLengthW %d\r\n"), blockLengthW));
      for (dwCount1 = 0; dwCount1 < blockLengthW; dwCount1++)
      {
        OALMSGX(OAL_IO, (TEXT("SDIPollingReceive set MMCHS_STAT BBR\r\n")));
        // Wait for Block ready for read
        while ((INREG32(&m_pbRegisters->MMCHS_STAT) & MMCHS_STAT_BRR) != MMCHS_STAT_BRR)
        {
          // check for a timeout
          dwCurrentTickCount = OALGetTickCount();
          if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
            : ( dwTimeout < dwCurrentTickCount || dwCurrentTickCount < dwCountStart ) )
          {
            OALMSGX(OAL_ERROR, (TEXT("SDIPollingReceive: TIMEOUT1\r\n")));
            goto READ_ERROR;
          }
        }
        SETREG32(&m_pbRegisters->MMCHS_STAT,MMCHS_STAT_BRR);

        // Get all data from DATA register and write in user buffer
        for (dwCount2 = 0; dwCount2 < (fifoSizeW/sizeof(DWORD)); dwCount2++)
        {
            *pbuf2 = INREG32(&m_pbRegisters->MMCHS_DATA) ;
            pbuf2++;
        }
      }
    }
    // recalculate timeout conditions
    dwCountStart = OALGetTickCount();
    dwTimeout = dwCountStart + DEFAULT_TIMEOUT_VALUE;
    if ( dwTimeout < dwCountStart )
        fTimeoutOverflow = TRUE;
    else
        fTimeoutOverflow = FALSE;

    while (((INREG32(&m_pbRegisters->MMCHS_STAT)&MMCHS_STAT_TC) != MMCHS_STAT_TC))
    {
        // check for a timeout
        dwCurrentTickCount = OALGetTickCount();
        if ( fTimeoutOverflow ? ( dwTimeout < dwCurrentTickCount && dwCurrentTickCount < dwCountStart )
           : ( dwTimeout < dwCurrentTickCount || dwCurrentTickCount < dwCountStart ) )
        {
            OALMSGX(OAL_ERROR, (TEXT("SDIPollingReceive: TIMEOUT3\r\n")));
            goto READ_ERROR;
        }
    }

    SETREG32(&m_pbRegisters->MMCHS_STAT,MMCHS_STAT_TC);
    // Check if there is no CRC error
    if (!(INREG32(&m_pbRegisters->MMCHS_STAT) & MMCHS_STAT_DCRC))
    {
        MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
        OUTREG32(&m_pbRegisters->MMCHS_STAT,MMC_STAT);
        return TRUE;
    }
    else
    {
        MMC_STAT = INREG32(&m_pbRegisters->MMCHS_STAT);
        OUTREG32(&m_pbRegisters->MMCHS_STAT,MMC_STAT);
        return FALSE;
    }

    return TRUE;

READ_ERROR:

    OALMSGX(OAL_IO, (TEXT("SDIPollingReceive error\r\n")));
    return FALSE;
}
