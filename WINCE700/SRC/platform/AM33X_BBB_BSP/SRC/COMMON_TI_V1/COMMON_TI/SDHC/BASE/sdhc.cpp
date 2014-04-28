// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: SDHC.CPP
//  SDHC controller driver implementation

#include "SDHC.h"
#include "SDHCRegs.h"
#include <oalex.h>
#include <nkintr.h>

#ifndef SD_CMD_MMC_SEND_EXT_CSD
 #define SD_CMD_MMC_SEND_EXT_CSD     8       // CMD8
#endif

// BusIoControl definition is not correct, unused parameters may be NULL/0
#pragma warning (disable: 6309)
#pragma warning (disable: 6387)

#define LOOKUP_CMD(cmd) ((cmd) == 60 && m_dwSDHighSpeedSupport ? gwaCMD60_HS : gwaCMD[(cmd)])
#define IS_SDHC_SWITCH_COMMAND(trans,cmd)     (m_dwSDHighSpeedSupport && (trans->TransferClass == SD_READ && cmd == SD_CMD_SWITCH_FUNCTION) ? TRUE : FALSE)
#define ADJUST_FOR_SDHC_SWITCH_CMD(cmd)       (LOOKUP_CMD(cmd*10).flags)

#define MMC_INT_EN_MASK                     0x00330033

#define DEFAULT_TIMEOUT_VALUE               10000
#define DATACMD_TIMEOUT_VALUE               10000
#define START_BIT                           0x00
#define TRANSMISSION_BIT                    0x00
#define START_RESERVED                      0x3F
#define END_RESERVED                        0xFE
#define END_BIT                             0x01
#define SDIO_MAX_LOOP                       0x0080000

#define IndicateBusRequestComplete(pRequest, status) \
    SDHCDIndicateBusRequestComplete(m_pHCContext, \
    (pRequest), (status))

#define IndicateSlotStateChange(event) \
    SDHCDIndicateSlotStateChange(m_pHCContext, \
    (UCHAR) 0, (event))

#define GetAndLockCurrentRequest() \
    SDHCDGetAndLockCurrentRequest(m_pHCContext, (UCHAR) 0)

#define PowerUpDown(fPowerUp, fKeepPower) \
    SDHCDPowerUpDown(m_pHCContext, (fPowerUp), (fKeepPower), \
    (UCHAR) 0)

#define TRANSFER_IS_WRITE(pRequest)        (SD_WRITE == (pRequest)->TransferClass)
#define TRANSFER_IS_READ(pRequest)         (SD_READ == (pRequest)->TransferClass)
#define TRANSFER_IS_COMMAND_ONLY(pRequest) (SD_COMMAND == (pRequest)->TransferClass)

#define TRANSFER_SIZE(pRequest)            ((pRequest)->BlockSize * (pRequest)->NumBlocks)

#define READ_MMC_STATUS()                  ((m_ActualPowerState<D3) ? (Read_MMC_STAT() & INREG32(&m_pbRegisters->MMCHS_IE)) : 0)
#define READ_EXT_MMC_STATUS()              (m_CardDetectInterruptStatus ? m_CardDetectInterruptStatus : READ_MMC_STATUS())


static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
	{
        L"NonSDIOActivityTimeout", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_nNonSDIOActivityTimeout),
        fieldsize(CSDIOControllerBase, m_nNonSDIOActivityTimeout), (VOID*)TIMERTHREAD_TIMEOUT_NONSDIO
    }, {
        L"SDIOActivityTimeout", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_nSDIOActivityTimeout),
        fieldsize(CSDIOControllerBase, m_nSDIOActivityTimeout), (VOID*)TIMERTHREAD_TIMEOUT
    }, {
        L"FastPath_SDMEM", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_fastPathSDMEM),
        fieldsize(CSDIOControllerBase, m_fastPathSDMEM), (VOID*)0
    }, {
        L"FastPath_SDIO", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_fastPathSDIO),
        fieldsize(CSDIOControllerBase, m_fastPathSDIO), (VOID*)1
    }, {
        L"LowVoltageSlot", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_LowVoltageSlot),
        fieldsize(CSDIOControllerBase, m_LowVoltageSlot), (VOID*)0
    }, {
        L"Sdio4BitDisable", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_Sdio4BitDisable),
        fieldsize(CSDIOControllerBase, m_Sdio4BitDisable), (VOID*)0
    }, {
        L"SdMem4BitDisable", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_SdMem4BitDisable),
        fieldsize(CSDIOControllerBase, m_SdMem4BitDisable), (VOID*)0
    }, {
        L"MMC8BitMode", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwMMC8BitMode),
        fieldsize(CSDIOControllerBase, m_dwMMC8BitMode), (VOID*)0
    }, {
        L"CTOTimeout", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_wCTOTimeout),
        fieldsize(CSDIOControllerBase, m_wCTOTimeout), (VOID*)MMC_CTO_CONTROL_DEFAULT
    }, {
        L"DTOTimeout", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_wDTOTimeout),
        fieldsize(CSDIOControllerBase, m_wDTOTimeout), (VOID*)MMC_DTO_CONTROL_DEFAULT
    }, {
        L"SDIOPriority", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwSDIOPriority),
        fieldsize(CSDIOControllerBase, m_dwSDIOPriority), (VOID*)SHC_CARD_CONTROLLER_PRIORITY
    }, {
        L"CDPriority", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwCDPriority),
        fieldsize(CSDIOControllerBase, m_dwCDPriority), (VOID*)SHC_CARD_DETECT_PRIORITY
    }, {
        L"BaseClockFrequency", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwMaxClockRate),
        fieldsize(CSDIOControllerBase, m_dwMaxClockRate), (VOID*)STD_HC_MAX_CLOCK_FREQUENCY
    }, {
        L"ReadWriteTimeout", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwMaxTimeout),
        fieldsize(CSDIOControllerBase, m_dwMaxTimeout), (VOID*)DEFAULT_TIMEOUT_VALUE
    }, {
        L"WakeupSources", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwWakeupSources),
        fieldsize(CSDIOControllerBase, m_dwWakeupSources), (VOID*)0
    }, {
        L"CardDetectGPIO", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwCardDetectGPIO),
        fieldsize(CSDIOControllerBase, m_dwCardDetectGPIO), (VOID*)-1
    }, {
        L"CardInsertedState", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwCardInsertedGPIOState),
        fieldsize(CSDIOControllerBase, m_dwCardInsertedGPIOState), (VOID*)0
    }, {
        L"CardWPGPIO", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwCardWPGPIO),
        fieldsize(CSDIOControllerBase, m_dwCardWPGPIO), (VOID*)-1
    }, {
        L"CardWriteProtectedState", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwCardWriteProtectedState),
        fieldsize(CSDIOControllerBase, m_dwCardWriteProtectedState), (VOID*)0
    }, {
        L"DMABufferSize", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwDMABufferSize),
        fieldsize(CSDIOControllerBase, m_dwDMABufferSize), (VOID*)CB_DMA_BUFFER
    }, {
        L"SDHighSpeedSupport", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwSDHighSpeedSupport),
        fieldsize(CSDIOControllerBase, m_dwSDHighSpeedSupport), (VOID*)0
    }, {
        L"SDClockMode", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwSDClockMode),
        fieldsize(CSDIOControllerBase, m_dwSDClockMode), (VOID*)0
    }, {
        L"SlotNumber", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_dwSlot),
        fieldsize(CSDIOControllerBase, m_dwSlot), (VOID*)MMCSLOT_1
    },
    {
        L"DmaEnable", PARAM_DWORD, FALSE,
        offset(CSDIOControllerBase, m_DmaEnable),
        fieldsize(CSDIOControllerBase, m_DmaEnable), (VOID*)1
    },
};

#ifdef DEBUG
// dump the current request info to the debugger
static
VOID
DumpRequest(PSD_BUS_REQUEST pRequest)
{
    DEBUGCHK(pRequest);

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DumpCurrentRequest: 0x%08X\r\n"), pRequest));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("\t Command: %d\r\n"),  pRequest->CommandCode));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("\t ResponseType: %d\r\n"),  pRequest->CommandResponse.ResponseType));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("\t NumBlocks: %d\r\n"),  pRequest->NumBlocks));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("\t BlockSize: %d\r\n"),  pRequest->BlockSize));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("\t HCParam: %d\r\n"),    pRequest->HCParam));

}
#else
#define DumpRequest(ptr)
#endif

#if defined(DEBUG) || defined(ENABLE_RETAIL_OUTPUT)
#define HEXBUFSIZE 1024
char szHexBuf[HEXBUFSIZE];
#endif

extern "C" void SocSdhcDevconf(DWORD dwSlot);

struct CMD
{
    BYTE Cmd;           // 1 - this is a known SD CMD; 2 - this is a known SDIO CMD
    BYTE ACmd;          // 1 - this is a known ACMD
    BYTE MMCCmd;        // 1 - this is a known MMC CMD
    DWORD flags;
};

// table of command codes...  at this time only SD/SDIO commands are implemented
CMD gwaCMD[] =
{
    { 1, 0, 0, MMCHS_RSP_NONE }, // CMD 00
    { 0, 0, 1, MMCHS_CMD_CICE | MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 01 (known MMC command)
    { 1, 0, 1, MMCHS_RSP_LEN136 |MMCHS_CMD_CCCE | MMCHS_CMD_NORMAL }, // CMD 02
    { 1, 0, 1, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 03
    { 1, 0, 0, MMCHS_RSP_NONE  | MMCHS_CMD_NORMAL }, // CMD 04
    { 2, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 05
    { 0, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 06
    { 1, 0, 0, MMCHS_CMD_CICE|MMCHS_CMD_CCCE|MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 07
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL}, // CMD 08
    { 1, 0, 0, MMCHS_RSP_LEN136 | MMCHS_CMD_NORMAL }, // CMD 09
    { 1, 0, 0, MMCHS_RSP_LEN136 | MMCHS_CMD_NORMAL }, // CMD 10
    { 0, 0, 1, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_DP | MMCHS_CMD_NORMAL }, // CMD 11 (known MMC command)
    { 1, 0, 0, MMCHS_RSP_LEN48B | MMCHS_CMD_ABORT }, // CMD 12
    { 1, 1, 0, MMCHS_CMD_CICE|MMCHS_CMD_CCCE| MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 13
    { 0, 0, 0, 0 }, // CMD 14
    { 1, 0, 0, MMCHS_RSP_NONE | MMCHS_CMD_NORMAL }, // CMD 15
    { 1, 0, 0, MMCHS_CMD_CCCE | MMCHS_CMD_CICE | MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 16
    { 1, 1, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_DP | MMCHS_CMD_NORMAL}, // CMD 17
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
    { 1, 0, 0, MMCHS_CMD_CICE|MMCHS_CMD_CCCE|MMCHS_RSP_LEN48| MMCHS_CMD_NORMAL }, // CMD 32
    { 1, 0, 0, MMCHS_CMD_CICE|MMCHS_CMD_CCCE|MMCHS_RSP_LEN48| MMCHS_CMD_NORMAL }, // CMD 33
    { 0, 0, 0, 0 }, // CMD 34
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 35
    { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_NORMAL }, // CMD 36
    { 0, 0, 0, 0 }, // CMD 37
    { 1, 1, 0, MMCHS_CMD_CICE|MMCHS_CMD_CCCE|MMCHS_RSP_LEN48B | MMCHS_CMD_NORMAL }, // CMD 38
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

CMD gwaCMD60_HS = { 1, 0, 0, MMCHS_RSP_LEN48 | MMCHS_CMD_READ | MMCHS_CMD_DP | MMCHS_CMD_MSBS | MMCHS_CMD_BCE | MMCHS_CMD_NORMAL};

//-----------------------------------------------------------------------------
CSDIOControllerBase::CSDIOControllerBase()
{
    InitializeCriticalSection( &m_critSec );
    InitializeCriticalSection( &m_powerCS );
    m_fSDIOInterruptInService = FALSE;
    m_fFirstTime = TRUE;
    m_hControllerISTEvent = NULL;
    m_htControllerIST = NULL;
    m_dwControllerSysIntr = (DWORD)SYSINTR_UNDEFINED;
    m_hCardDetectEvent = NULL;
    m_htCardDetectIST = NULL;
    m_fAppCmdMode = FALSE;

    m_pbRegisters = NULL;
    m_fCardPresent = FALSE;
    m_fSDIOInterruptsEnabled = FALSE;
    m_pDmaBuffer = NULL;
    m_pCachedDmaBuffer = NULL;
    m_pDmaBufferPhys.QuadPart = 0;

    m_dwMaxTimeout = DEFAULT_TIMEOUT_VALUE;
    m_bReinsertTheCard = FALSE;
    m_dwWakeupSources = 0;
    m_dwCurrentWakeupSources = 0;
    m_fMMCMode = FALSE;

    m_InternPowerState = D4;
    m_ActualPowerState = D4;
    m_hParentBus = NULL;
    m_hGPIO = NULL;

    m_dwSlot = MMCSLOT_1;
	m_dwDeviceID = OMAP_DEVICE_NONE;
    m_dwSDIOCard = 0;
    m_fDriverShutdown = FALSE;

    bRxDmaActive = FALSE;
    bTxDmaActive = FALSE;
    m_dwClockCnt = 0;
    m_fCardInitialized = FALSE;
    m_bExitThread = FALSE;
	m_hTimerThreadIST = NULL;
	m_hTimerEvent = NULL;
    //m_TransferClass = NULL;

    m_pCurrentRecieveBuffer = NULL;
    m_dwCurrentRecieveBufferLength = 0;
    m_CardDetectInterruptStatus = 0;
    m_bCommandPending = FALSE;

    m_sContext.dwClockRate = MMCSD_CLOCK_INIT;
    m_sContext.eSDHCIntr = SDHC_INTR_DISABLED;
    m_sContext.eInterfaceMode = SD_INTERFACE_SD_MMC_1BIT;

    m_DmaEnable = FALSE;
    m_TxDmaInfo = NULL;
    m_RxDmaInfo = NULL;
    m_hTxDmaChannel = NULL;
    m_hRxDmaChannel = NULL;
}

//-----------------------------------------------------------------------------
DWORD CSDIOControllerBase::Read_MMC_STAT()
{
    DWORD dwVal;
    EnterCriticalSection( &m_critSec );
    dwVal = INREG32(&m_pbRegisters->MMCHS_STAT);
    LeaveCriticalSection( &m_critSec );
    return dwVal;
}

//-----------------------------------------------------------------------------
void CSDIOControllerBase::Write_MMC_STAT( DWORD dwVal )
{
    EnterCriticalSection( &m_critSec );

    // For omap, bit-6 and bit-7 are Rsvd
    //
    // For netr, bit-6 (CINS) and bit-7 (CREM) are handled 
    // separately in Write_MMC_CD_STAT().
    //
    // So clearing these 2 bits here before writing to MMCHS_STAT 
    // is required and does not harm either.
    dwVal &= ~(MMCHS_STAT_CREM|MMCHS_STAT_CINS);

    OUTREG32(&m_pbRegisters->MMCHS_STAT, dwVal);
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
void CSDIOControllerBase::Set_MMC_STAT( DWORD dwVal )
{
    EnterCriticalSection( &m_critSec );
    OUTREG32(&m_pbRegisters->MMCHS_STAT, dwVal);
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
//  Reset the controller.
VOID CSDIOControllerBase::SoftwareReset( DWORD dwResetBits )
{
    DWORD               dwCountStart;

    DEBUGCHK(sizeof(OMAP_MMCHS_REGS) % sizeof(DWORD) == 0);

    dwResetBits &= (MMCHS_SYSCTL_SRA | MMCHS_SYSCTL_SRC | MMCHS_SYSCTL_SRD);

    // Reset the controller
    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, dwResetBits);

    // get starting tick count for timeout
    dwCountStart = GetTickCount();

    // Verify that reset has completed.
    while ((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & dwResetBits))
    {
        // check for timeout (see CE Help to understand how this calculation works)
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SoftwareReset() - exit: TIMEOUT.\r\n")));
            break;
        }

        Sleep(0);
    }
}

//-----------------------------------------------------------------------------
// Set up the controller according to the SD interface parameters.
VOID CSDIOControllerBase::SetSDInterfaceMode(SD_INTERFACE_MODE eSDInterfaceMode)
{
    if (SD_INTERFACE_SD_MMC_1BIT == eSDInterfaceMode)
    {
        CLRREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_DTW);
        DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SetInterface SD/MMC 1bit MMCHS_HCTL value = %X\r\n"), m_pbRegisters->MMCHS_HCTL ));
		RETAILMSG(1,(TEXT("Slot:%d SetInterface SD/MMC 1bit MMCHS_HCTL value = %X\r\n"), m_dwSlot, m_pbRegisters->MMCHS_HCTL ));
    }
    else if (SD_INTERFACE_SD_4BIT == eSDInterfaceMode)
    {
        SETREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_DTW);
        DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SetInterface SD 4bit MMCHS_HCTL value = %X\r\n"), m_pbRegisters->MMCHS_HCTL ));
        RETAILMSG(1,(TEXT("Slot:%d SetInterface SD 4bit MMCHS_HCTL value = %X\r\n"), m_dwSlot, m_pbRegisters->MMCHS_HCTL ));
    }
    else if (SD_INTERFACE_MMC_8BIT== eSDInterfaceMode)
        {
        SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_DW8);
        DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SetSDInterfaceMode MMC 8bit, MMCHS_CON value = %X\r\n"), m_pbRegisters->MMCHS_CON ));
        RETAILMSG(1,(TEXT("Slot:%d SetSDInterfaceMode MMC 8bit, MMCHS_CON value = %X\r\n"), m_dwSlot, m_pbRegisters->MMCHS_CON ));
        }
    else
    {
        RETAILMSG(SDCARD_ZONE_ERROR, 
            (L"SDHC!ERROR - Unexpected SD interface(%d)\r\n",
            eSDInterfaceMode)
            );
    }
}

//-----------------------------------------------------------------------------
// Set up the controller according to the interface parameters.
VOID CSDIOControllerBase::SetInterface(PSD_CARD_INTERFACE_EX pInterface)
{
    if(m_ActualPowerState == D4) return;

    UpdateSystemClock(TRUE);

    Sleep(2);

    m_sContext.eInterfaceMode = (pInterface->InterfaceModeEx.bit.sd4Bit) ? SD_INTERFACE_SD_4BIT : SD_INTERFACE_SD_MMC_1BIT;
    m_sContext.eInterfaceMode = (pInterface->InterfaceModeEx.bit.mmc8Bit) ? SD_INTERFACE_MMC_8BIT : m_sContext.eInterfaceMode;
    SetSDInterfaceMode(m_sContext.eInterfaceMode);

    SetClockRate(&pInterface->ClockRate);
    UpdateSystemClock(FALSE);
}

//-----------------------------------------------------------------------------
// Enable SDHC Interrupts.
VOID CSDIOControllerBase::EnableSDHCInterrupts()
{
    EnterCriticalSection( &m_critSec );
    OUTREG32(&m_pbRegisters->MMCHS_ISE, MMC_INT_EN_MASK);
    OUTREG32(&m_pbRegisters->MMCHS_IE,  MMC_INT_EN_MASK);
    m_sContext.eSDHCIntr = SDHC_MMC_INTR_ENABLED;
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
// Disable SDHC Interrupts.
void CSDIOControllerBase::DisableSDHCInterrupts()
{
    EnterCriticalSection( &m_critSec );
    OUTREG32(&m_pbRegisters->MMCHS_ISE, 0);
    OUTREG32(&m_pbRegisters->MMCHS_IE,  0);
    m_sContext.eSDHCIntr = SDHC_INTR_DISABLED;
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
// Enable SDIO Interrupts.
VOID CSDIOControllerBase::EnableSDIOInterrupts()
{
    ASSERT( !m_fSDIOInterruptsEnabled );
    m_fSDIOInterruptsEnabled = TRUE;

    DEBUGMSG(SHC_INTERRUPT_ZONE, (TEXT("CSDHCSlot::EnableSDIOInterrupts\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("CSDHCSlot::EnableSDIOInterrupts\r\n")));
#endif
    EnterCriticalSection( &m_critSec );
    SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_CTPL);
    if (!m_Sdio4BitDisable && (m_sContext.eInterfaceMode == SD_INTERFACE_SD_4BIT))
    {
        SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_CLKEXTFREE);
    }

    // enable exit from smart idle mode on SD/SDIO card interrupt
    SETREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_CIRQ);
    // enable SD/SDIO card interrupt
    SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_CIRQ);

    m_sContext.eSDHCIntr = SDHC_SDIO_INTR_ENABLED;
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
// Acknowledge an SDIO Interrupt.
VOID CSDIOControllerBase::AckSDIOInterrupt(
    )
{
    ASSERT( m_fSDIOInterruptsEnabled );
    DEBUGMSG(SHC_INTERRUPT_ZONE, (TEXT("CSDHCSlot::AckSDIOInterrupt\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("CSDHCSlot::AckSDIOInterrupt\r\n")));
#endif
    DWORD dwRegValue = Read_MMC_STAT();
    if( dwRegValue & MMCHS_STAT_CIRQ )
    {
        Set_MMC_STAT(MMCHS_STAT_CIRQ);
        SDHCDIndicateSlotStateChange(m_pHCContext, 0, DeviceInterrupting);
    }
    else
    {
        EnterCriticalSection( &m_critSec );
        SETREG32(&m_pbRegisters->MMCHS_IE,  MMCHS_IE_CIRQ);
        LeaveCriticalSection( &m_critSec );
        m_fSDIOInterruptInService = FALSE;
    }
}

//-----------------------------------------------------------------------------
// Disable SDIO Interrupts.
VOID CSDIOControllerBase::DisableSDIOInterrupts()
{
    ASSERT( m_fSDIOInterruptsEnabled );
    m_fSDIOInterruptsEnabled = FALSE;

    DEBUGMSG(SHC_INTERRUPT_ZONE, (TEXT("CSDHCSlot::DisableSDIOInterrupts\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("CSDHCSlot::DisableSDIOInterrupts\r\n")));
#endif
    EnterCriticalSection( &m_critSec );
    CLRREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_CIRQ);
    CLRREG32(&m_pbRegisters->MMCHS_IE,  MMCHS_IE_CIRQ);
    m_sContext.eSDHCIntr = SDHC_INTR_DISABLED;
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
//  Set clock rate based on HC capability
VOID CSDIOControllerBase::SetClockRate(PDWORD pdwRate)
{
    DWORD dwClockRate = *pdwRate;

    if(dwClockRate > m_dwMaxClockRate) dwClockRate = m_dwMaxClockRate;

    // calculate the register value
    DWORD dwDiv = (DWORD)((MMCSD_CLOCK_INPUT + dwClockRate - 1) / dwClockRate);

    DEBUGMSG(SHC_CLOCK_ZONE, (TEXT("actual wDiv = 0x%x  requested:0x%x\r\n"), dwDiv, *pdwRate));
    // Only 10 bits available for the divider, so mmc base clock / 1024 is minimum.
    if ( dwDiv > 0x03FF )
        dwDiv = 0x03FF;

    DEBUGMSG(SHC_CLOCK_ZONE, (TEXT("dwDiv = 0x%x 0x%x\r\n"), dwDiv, *pdwRate));

    // Program the divisor, but leave the rest of the register alone.
    INT32 dwRegValue = INREG32(&m_pbRegisters->MMCHS_SYSCTL);

    dwRegValue = (dwRegValue & ~MMCHS_SYSCTL_CLKD_MASK) | MMCHS_SYSCTL_CLKD(dwDiv);
    dwRegValue = (dwRegValue & ~MMCHS_SYSCTL_DTO_MASK) | MMCHS_SYSCTL_DTO(0x0e); // DTO
    dwRegValue &= ~MMCHS_SYSCTL_CEN;
    dwRegValue &= ~MMCHS_SYSCTL_ICE;

    CLRREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);

    OUTREG32(&m_pbRegisters->MMCHS_SYSCTL, dwRegValue);

    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_ICE); // enable internal clock

    DWORD dwTimeout = 500;
    while(((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & MMCHS_SYSCTL_ICS) != MMCHS_SYSCTL_ICS) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    if(0==dwTimeout)
    {
        DEBUGMSG(ZONE_ENABLE_INFO, (TEXT("Timeout for ICS \r\n")));
    }

    SETREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);
    SETREG32(&m_pbRegisters->MMCHS_HCTL, MMCHS_HCTL_SDBP); // power up the card

    dwTimeout = 500;
    while(((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & MMCHS_SYSCTL_CEN) != MMCHS_SYSCTL_CEN) && (dwTimeout>0))
    {
        dwTimeout--;
    }
	
    if(0==dwTimeout)
    {
        DEBUGMSG(ZONE_ENABLE_INFO, (TEXT("Timeout for CEN \r\n")));
    }

    *pdwRate = MMCSD_CLOCK_INPUT / dwDiv;
    m_sContext.dwClockRate = MMCSD_CLOCK_INPUT / dwDiv;

    DEBUGMSG(SHC_CLOCK_ZONE,(TEXT("SDHCSetRate - Actual clock rate = 0x%x, MMCHS_SYSCTL = 0x%x\r\n"), *pdwRate, INREG32(&m_pbRegisters->MMCHS_SYSCTL)));
	RETAILMSG(SDCARD_ZONE_INFO,(TEXT("Slot:%d SDHCSetRate - Actual clock rate = %d, MMCHS_SYSCTL = 0x%x\r\n"),m_dwSlot, *pdwRate, INREG32(&m_pbRegisters->MMCHS_SYSCTL)));
}

BOOL CSDIOControllerBase::IsMultipleBlockReadSupported()
{
    BOOL bVal = TRUE;
    // work around for a OMAP35XX silicon issue (data CRC error on READ_MULTIPLE_BLOCK command)
    // This is present in earley processor revisions
    if(m_dwCPURev < CPU_FAMILY_35XX_REVISION_ES_3_0)
    {
        bVal = FALSE;
    }
    return bVal;
}

//-----------------------------------------------------------------------------
//
VOID CSDIOControllerBase::SetSDVSVoltage()
{
    UINT32 val1, val2;

	val1 = MMCHS_CAPA_VS33;
	val2 = MMCHS_HCTL_SDVS_3V3;
    if ( m_dwSlot == MMCSLOT_1 )
    {
        if (m_LowVoltageSlot)
        {
            val1 = MMCHS_CAPA_VS18;
            val2 = MMCHS_HCTL_SDVS_1V8;
        }
    }
    else if (m_dwSlot == MMCSLOT_2)
    {
        if (m_LowVoltageSlot)
        {
            val1 = MMCHS_CAPA_VS18;
            val2 = MMCHS_HCTL_SDVS_1V8;
        }
    }
    else
    {
        RETAILMSG(SDCARD_ZONE_WARN, (L"MMC Slot number is not Valid\r\n"));
        return;
    }
    SETREG32(&m_pbRegisters->MMCHS_CAPA, val1);
    SETREG32(&m_pbRegisters->MMCHS_HCTL, val2);
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  SDHCPowerTimerThread - SDIO power timer thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSDIOControllerBase::SDHCPowerTimerThread(LPVOID lpParameter)
{
    CSDIOControllerBase *pController = (CSDIOControllerBase*)lpParameter;
    return pController->SDHCPowerTimerThreadImpl();
}

//------------------------------------------------------------------------------
//
//  Function:  SDHCPowerTimerThread
//
//  timeout thread, checks to see if the power can be disabled.
//
DWORD CSDIOControllerBase::SDHCPowerTimerThreadImpl()
{
    DWORD nTimeout = INFINITE;
    RETAILMSG(0, (TEXT("*** start SDHCPowerTimerThreadImpl()\r\n")));

    CeSetThreadPriority(GetCurrentThread(), TIMERTHREAD_PRIORITY);

    for(;;)
    {
        WaitForSingleObject(m_hTimerEvent, nTimeout);

        if (m_bExitThread == TRUE) 
		    break;

        // serialize access to power state changes
        EnterCriticalSection(&m_critSec);

        // by the time this thread got the cs hTimerEvent may
        // have gotten resignaled.  Clear the event to  make
        // sure the activity timer thread isn't awaken prematurely
        //
        ResetEvent(m_hTimerEvent);

        // check if we need to reset the timer
        if (m_dwClockCnt == 0)
        {
            // We disable the clocks only when this thread
            // wakes-up twice in a row with no power state
            // change to D0.  This is achieved by using the
            // bDisablePower flag to determine if power state
            // changed since the last time this thread woke-up
            //
            if ((m_bDisablePower == TRUE) || (m_fCardPresent == FALSE))
            {
                if (m_ActualPowerState < D3)
                {
                    // update clock control
                    if (!m_dwSDClockMode || !m_fCardPresent /*|| !(m_dwWakeupSources & WAKEUP_SDIO)*/)
					{
                        OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, 
                            MMCHS_SYSCONFIG_CLOCKACTIVITY(0) | 
                            MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_FORCE) |
                            MMCHS_SYSCONFIG_AUTOIDLE 
                            );
                    }
					else
					{
    	                OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, 
	    				    MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_IGNORE) | 
		    				MMCHS_SYSCONFIG_CLOCKACTIVITY(3)
			    			);
                    }
                    UpdateDevicePowerState(FALSE);
                 }
                nTimeout = INFINITE;
            }
            else
            {
                // wait for activity time-out before shutting off power.
                m_bDisablePower = TRUE;
                nTimeout = (m_fCardInitialized && !m_dwSDIOCard) ? m_nNonSDIOActivityTimeout : m_nSDIOActivityTimeout;
            }
        }
        else
        {
            nTimeout = INFINITE;
        }
        LeaveCriticalSection(&m_critSec);
    }

    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  UpdateSystemClock
//
//  This function enable/disable AutoIdle Mode
//
//
BOOL
CSDIOControllerBase::UpdateSystemClock(BOOL enable)
{
    LONG   lClockCount;

    DEBUGMSG(SDCARD_ZONE_FUNC, (L"+UpdateSystemClock()\r\n"));

    if (enable)
    {
        lClockCount = InterlockedIncrement(&m_dwClockCnt);

        EnterCriticalSection( &m_critSec );
        m_InternPowerState = D0;
        if(lClockCount == 1)
        {
            if (m_ActualPowerState >= D3)
            {
                UpdateDevicePowerState(FALSE);
                
                if (m_dwSDClockMode)
				{
                    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, 
                        MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_IGNORE) | 
                        MMCHS_SYSCONFIG_CLOCKACTIVITY(3)
                        );
                }
				else
				{
                    // update clock control
                    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, 
                        MMCHS_SYSCONFIG_CLOCKACTIVITY(0) | 
                        MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_SMART) |
                        MMCHS_SYSCONFIG_ENAWAKEUP |
                        MMCHS_SYSCONFIG_AUTOIDLE 
                        );
                }
            }
        }
        m_bDisablePower = FALSE;
        LeaveCriticalSection( &m_critSec );
    }
    else
    {
        lClockCount = InterlockedDecrement(&m_dwClockCnt);
        if(lClockCount < 0)
            m_dwClockCnt = 0;

        if(lClockCount <= 0)
        {
            m_InternPowerState = D4;
            if (m_hTimerEvent != NULL)
            {
                SetEvent(m_hTimerEvent);
            }
            else
            {
                if (m_ActualPowerState < D3)
                {
                    // update clock control
                    if (!m_dwSDClockMode || m_InternPowerState == D4)
		    		{
                        OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, 
                            MMCHS_SYSCONFIG_AUTOIDLE | 
                            MMCHS_SYSCONFIG_CLOCKACTIVITY(0) | 
                            MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_FORCE)
                            );
					}
                }
                UpdateDevicePowerState(FALSE);
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
VOID CSDIOControllerBase::UpdateDevicePowerState(BOOL bInPowerHandler)
{
    CEDEVICE_POWER_STATE curPowerState = D4;

    // if card is present then lowest power state is D3
    if (m_fCardPresent)
        curPowerState = D3;

    curPowerState = min(curPowerState, m_InternPowerState);

    if((m_ActualPowerState == D4 && curPowerState < D4 ) || (m_ActualPowerState < D4 && curPowerState == D4 ))
        PreparePowerChange(curPowerState, bInPowerHandler);

    SetDevicePowerState( m_hParentBus, curPowerState, NULL );

    if((m_ActualPowerState == D4 && curPowerState < D4 ) || (m_ActualPowerState < D4 && curPowerState == D4 ))
        PostPowerChange(curPowerState, bInPowerHandler);

    m_ActualPowerState = curPowerState;
}

//-----------------------------------------------------------------------------
// Send command without response
SD_API_STATUS CSDIOControllerBase::SendCmdNoResp (DWORD cmd, DWORD arg)
{
    DWORD MMC_CMD;
    DWORD dwTimeout;

    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    dwTimeout = 80000;
    while(((INREG32(&m_pbRegisters->MMCHS_PSTATE) & MMCHS_PSTAT_CMDI)) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    MMC_CMD = MMCHS_INDX(cmd);
    MMC_CMD |= LOOKUP_CMD(cmd).flags;

    // Program the argument into the argument registers
    OUTREG32(&m_pbRegisters->MMCHS_ARG, arg);
    // Issue the command.
    OUTREG32(&m_pbRegisters->MMCHS_CMD, MMC_CMD);

    dwTimeout = 5000;
    DWORD dwVal;
    while(dwTimeout > 0)
    {
        dwTimeout --;
        dwVal = INREG32(&m_pbRegisters->MMCHS_STAT);
        if(dwVal & (MMCHS_STAT_CC | MMCHS_STAT_CTO | MMCHS_STAT_CERR)) break;
    }

    dwVal = INREG32(&m_pbRegisters->MMCHS_STAT);
    OUTREG32(&m_pbRegisters->MMCHS_STAT, dwVal);

    // always return 0 if no response needed
    return SD_API_STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Send init sequence to card
VOID CSDIOControllerBase::SendInitSequence()
{
    EnterCriticalSection( &m_critSec );
    OUTREG32(&m_pbRegisters->MMCHS_IE,  0xFFFFFEFF);
    SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_INIT);

    DWORD dwCount;
    for(dwCount = 0; dwCount < 10; dwCount ++)
    {
        SendCmdNoResp(SD_CMD_GO_IDLE_STATE, 0xFFFFFFFF);
    }
    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    CLRREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_INIT);
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
// Issues the specified SDI command
SD_API_STATUS CSDIOControllerBase::SendCommand( PSD_BUS_REQUEST pRequest )
{
    DWORD MMC_CMD;
    DWORD dwCountStart;
    DWORD Cmd = pRequest->CommandCode;
    DWORD Arg = pRequest->CommandArgument;
    UINT16 respType = (UINT16)pRequest->CommandResponse.ResponseType;
    DWORD dwRegVal;

    m_TransferClass = pRequest->TransferClass;

    DEBUGMSG(SHC_SEND_ZONE, (TEXT("SendCommand() - Cmd = 0x%x Arg = 0x%x respType = 0x%x m_TransferClass = 0x%x\r\n"),
        Cmd, Arg, respType, m_TransferClass));

    if ((Cmd == SD_CMD_IO_RW_EXTENDED) || (Cmd == SD_CMD_IO_RW_DIRECT))
    {
        m_dwSDIOCard = 1;
    } 
    else if ((Cmd == SD_CMD_MMC_SEND_OPCOND) || (Cmd == SD_CMD_GO_IDLE_STATE))
    {
        m_dwSDIOCard = 0;
    }

    if( m_TransferClass == SD_READ || m_TransferClass == SD_WRITE )
    {
        DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("SendCommand (Cmd=0x%08X, Arg=0x%08x, RespType=0x%08X, Data=0x%x <%dx%d>) starts\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE, pRequest->NumBlocks, pRequest->BlockSize ) );
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("SendCommand (Cmd=0x%08X, Arg=0x%08x, RespType=0x%08X, Data=0x%x <%dx%d>) starts\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE, pRequest->NumBlocks, pRequest->BlockSize ) );
#endif

        // get starting tick count for timeout
        dwCountStart = GetTickCount();

        while((INREG32(&m_pbRegisters->MMCHS_PSTATE) & MMCHS_PSTAT_DATI))
        {
            if (GetTickCount() - dwCountStart > DATACMD_TIMEOUT_VALUE)
            {
                DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("WARNING- SendCommand (Cmd=0x%08X, Arg=0x%08x, RespType=0x%08X, Data=0x%x) Data Line in use!\r\n"),
                Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE) );
                break;
            }
        }

    }
    else
    {
        DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("SendCommand (Cmd=0x%08X, Arg=0x%08x, RespType=0x%08X, Data=0x%x) starts\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE) );
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, 
            (TEXT("SendCommand (Cmd=%d, Arg=0x%08x, RespType=0x%08X, XferCl=%d, Data=0x%x) starts\r\n"),
            Cmd, Arg, respType, m_TransferClass, (m_TransferClass==SD_COMMAND)?FALSE:TRUE) );
#endif
    }

    Write_MMC_STAT(0xFFFFFFFF);
    dwCountStart = GetTickCount();
    while(((INREG32(&m_pbRegisters->MMCHS_PSTATE) & MMCHS_PSTAT_DATI)) )
    {
        if (GetTickCount() - dwCountStart > DATACMD_TIMEOUT_VALUE)
        {
            DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("WARNING- SendCommand (Cmd=0x%08X, Arg=0x%08x, RespType=0x%08X, Data=0x%x) Data Line in use!\r\n"),
            Cmd, Arg, respType, (m_TransferClass==SD_COMMAND)?FALSE:TRUE) );
            break;
        }
    }
    MMC_CMD = MMCHS_INDX(Cmd);

    // CMD6 is defined differently in MMC and SD specifications, try to identify them here 
    // and use appropriate controller settings. 
    if (IS_SDHC_SWITCH_COMMAND(pRequest,Cmd))
    {
        DEBUGMSG(SHC_SDBUS_INTERACT_ZONE,(TEXT("SendCommand:: branch for switch command\r\n")));
        MMC_CMD |= ADJUST_FOR_SDHC_SWITCH_CMD(Cmd);
    }
    else
        MMC_CMD |= LOOKUP_CMD(Cmd).flags;
	
    if ((Cmd == SD_CMD_SELECT_DESELECT_CARD) && (respType == NoResponse))
    {
        MMC_CMD &= ~MMCHS_RSP_MASK;
        MMC_CMD |= MMCHS_RSP_NONE;
    }

    m_fDMATransfer = FALSE;
    MMC_CMD &= ~MMCHS_CMD_DE;

    if (Cmd == SD_CMD_IO_RW_EXTENDED)
    {
        if(pRequest->NumBlocks > 1)
        {
           MMC_CMD |= MMCHS_CMD_MSBS | MMCHS_CMD_BCE;
        }
    }

    if( m_TransferClass == SD_READ )
    {
        MMC_CMD |= MMCHS_CMD_DDIR;

    if(m_DmaEnable)
    {
         // if we can use the DMA for transfer...
         if( ((( pRequest->NumBlocks > 1) && !(pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)) || 
         (( pRequest->NumBlocks > 0) && (pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE))) &&
              ( TRANSFER_SIZE(pRequest) % MIN_MMC_BLOCK_SIZE == 0 ) &&
              ( TRANSFER_SIZE(pRequest) <= m_dwDMABufferSize ) )
          {
              MMC_CMD |= MMCHS_CMD_DE;
              m_fDMATransfer = TRUE;
              // program the DMA controller
              EnterCriticalSection( &m_critSec );
              SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_BRR);
              SETREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_IE_BRR);
              LeaveCriticalSection( &m_critSec );
              SDIO_InitInputDMA( pRequest->NumBlocks,  pRequest->BlockSize);
          }
    	}
        dwRegVal = (DWORD)(pRequest->BlockSize & 0xFFFF);
        dwRegVal += ((DWORD)(pRequest->NumBlocks & 0xFFFF)) << 16;
        OUTREG32(&m_pbRegisters->MMCHS_BLK, dwRegVal);
    }
    else if( m_TransferClass == SD_WRITE )
    {
        MMC_CMD &= ~MMCHS_CMD_DDIR;

        if(m_DmaEnable)
        {
            // if we can use the DMA for transfer...
            if( ((( pRequest->NumBlocks > 1) && !(pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)) || 
                 (( pRequest->NumBlocks > 0) && (pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE))) &&
                ( TRANSFER_SIZE(pRequest) % MIN_MMC_BLOCK_SIZE == 0 ) &&
                ( TRANSFER_SIZE(pRequest) <= m_dwDMABufferSize ) )
            {
                MMC_CMD |= MMCHS_CMD_DE;
                m_fDMATransfer = TRUE;
                // program the DMA controller
                EnterCriticalSection( &m_critSec );
                SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_BWR);
                SETREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_BWR);
                LeaveCriticalSection( &m_critSec );
                SDIO_InitOutputDMA( pRequest->NumBlocks,  pRequest->BlockSize );
            }
        }
        dwRegVal = (DWORD)(pRequest->BlockSize & 0xFFFF);
        dwRegVal += ((DWORD)(pRequest->NumBlocks & 0xFFFF)) << 16;
        OUTREG32(&m_pbRegisters->MMCHS_BLK, dwRegVal);
    }

    //ACMD13 has associated data, but the CMD13 doesnt have data
    //Hence we need to set the Data Present Select (CMD:DP- bit 21) for ACMD13 alone
    if(m_fAppCmdMode && (pRequest->CommandCode == SD_ACMD_SD_STATUS))
    {
       MMC_CMD |= MMCHS_CMD_DP;
    }

    //Both SD_CMD_MMC_SEND_EXT_CSD, and SD_CMD_SEND_IF_COND maps to CMD 8
    //SD_CMD_MMC_SEND_EXT_CSD (which is MMC v4.x specific) has associated data, and SD_CMD_SEND_IF_COND (which is SD specific) doesnt have data
    //Hence we need to set the Data Present Select (CMD:DP- bit 21) for CMD 8 for MMC.
    if(m_fMMCMode&& (pRequest->CommandCode == SD_CMD_MMC_SEND_EXT_CSD))
    {
       MMC_CMD |= MMCHS_CMD_DP;
    }

    //check for card initialization is done.
    if(!m_fCardInitialized && (Cmd == SD_CMD_READ_SINGLE_BLOCK))
        m_fCardInitialized = TRUE;

    // Program the argument into the argument registers
    OUTREG32(&m_pbRegisters->MMCHS_ARG, Arg);

    DEBUGMSG(SHC_SEND_ZONE, (TEXT("SendCommand() - registers:Command = 0x%x, MMCHS_ARG = 0x%x%x\r\n"), MMC_CMD, INREG32(&m_pbRegisters->MMCHS_ARG)));

    // Issue the command.
    OUTREG32(&m_pbRegisters->MMCHS_CMD, MMC_CMD);

    return SD_API_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Remove the device instance in the slot
VOID CSDIOControllerBase::CardInterrupt(BOOL bInsert)
{
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("CardInterrupt(%d)\r\n"), bInsert));

    EnterCriticalSection( &m_critSec );
    m_CardDetectInterruptStatus |= (bInsert ? EXT_MMCHS_STAT_CD_INSERT_INTR : EXT_MMCHS_STAT_CD_REMOVE_INTR);
    LeaveCriticalSection( &m_critSec );

    SetEvent(m_hControllerISTEvent);
}

//-----------------------------------------------------------------------------
// Handle Card Detect interrupt. Return true if card is inserted.
// In this function, 
//   dwWriteValue (local) 
//       bits are set.  This is used for netra and does not hurt omap.
//   m_CardDetectInterruptStatus 
//       bits are cleared.  This is used for omap, and as long 
//       as it remains 0 it does not hurt netra. 
BOOL CSDIOControllerBase::HandleCardDetectInterrupt(DWORD dwStatus)
{
    BOOL    bRetValue = FALSE;
    DWORD   dwWriteValue = 0;  // for netra

#ifdef ENABLE_RETAIL_OUTPUT
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("CardDetectInterrupt\r\n")));
#endif
		
    if (dwStatus & EXT_MMCHS_STAT_CD_REMOVE_INTR)
    {
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("CardDetectInterrupt: Card removed!\r\n")));
#endif

        HandleRemoval(TRUE);

        EnterCriticalSection( &m_critSec );
        m_CardDetectInterruptStatus &= ~EXT_MMCHS_STAT_CD_REMOVE_INTR;
        dwWriteValue |= EXT_MMCHS_STAT_CD_REMOVE_INTR;
        LeaveCriticalSection( &m_critSec );

        bRetValue = FALSE;
    }

    if (dwStatus & EXT_MMCHS_STAT_CD_INSERT_INTR)
    {
        EnterCriticalSection( &m_critSec );
        m_CardDetectInterruptStatus &= ~EXT_MMCHS_STAT_CD_INSERT_INTR;
        dwWriteValue |= EXT_MMCHS_STAT_CD_INSERT_INTR;
        LeaveCriticalSection( &m_critSec );

#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (TEXT("CardDetectInterrupt: Card inserted!\r\n")));
#endif
			
        HandleInsertion();

        bRetValue = TRUE;
    }

    // Clear the CD interrupt bits for Netra.
    // For OMAP, calling this function should not matter because 
    // Write_MMC_CD_STAT is just a stub function there.
    Write_MMC_CD_STAT(dwWriteValue);

    return bRetValue;
}

//-----------------------------------------------------------------------------
// Remove the device instance in the slot
VOID CSDIOControllerBase::HandleRemoval(BOOL fCancelRequest)
{
    PSD_BUS_REQUEST pRequest = NULL;

    m_fCardPresent = FALSE;
    m_fMMCMode = FALSE;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("HandleRemoval\r\n")));

    TurnCardPowerOn();  // try to turn slot power on

    IndicateSlotStateChange(DeviceEjected);
    SystemClockOn(FALSE);

    // turn off SDIO interrupts
    if( m_fSDIOInterruptsEnabled )
    {
        DisableSDIOInterrupts();
    }

    if (fCancelRequest)
    {
        // get the current request
        pRequest = GetAndLockCurrentRequest();

        if (pRequest != NULL)
        {
            DEBUGMSG(SDCARD_ZONE_WARN,
                (TEXT("Card Removal Detected - Canceling current request: 0x%08X, command: %d\r\n"),
                pRequest, pRequest->CommandCode));
            DumpRequest(pRequest);
            IndicateBusRequestComplete(pRequest, SD_API_STATUS_DEVICE_REMOVED);
        }
    }
    
    if(m_ActualPowerState == D4) return;
    SoftwareReset(SOFT_RESET_ALL);
    DisableSDHCInterrupts();

    /*
        DisableSDHCInterrupts() writes 0 to MMCHS_IE & MMCHS_IES.
        For Netra this disables the CD intrs also.  For OMAP, it 
        has no effect on CD intrs as they come from GPIO.
 
        So EnableSDHCCDInterrupts() is called for Netra to restore
        the CD intrs while it is a stub for OMAP.
    */
    EnableSDHCCDInterrupts();

    if (m_dwSDClockMode)
    {
        // turn clock off and remove power from the slot
        OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_AUTOIDLE | MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_FORCE));
    }
    
    ResetEvent(m_hControllerISTEvent);
    Sleep(100);

    m_fCardInitialized = FALSE;

    CLRREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_CLKEXTFREE);

#if 0
    // get and lock the current bus request
    while(SDHCDGetAndLockCurrentRequest(m_pHCContext, 0) != NULL)
    {
        CommandCompleteHandler();
    }
#else
    // get and lock the current bus request
    while((pRequest = SDHCDGetAndLockCurrentRequest(m_pHCContext, 0)) != NULL)
    {
        IndicateBusRequestComplete(pRequest, SD_API_STATUS_DEVICE_REMOVED);
    }
#endif

    TurnCardPowerOff();  // try to turn slot power off
}

//-----------------------------------------------------------------------------
// Initialize the card
VOID CSDIOControllerBase::HandleInsertion()
{
    DWORD dwClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("HandleInsertion\r\n")));
    m_fCardPresent = TRUE;
    m_dwSDIOCard = 0;

    // turn power to the card on
    TurnCardPowerOn();

    SoftwareReset(SOFT_RESET_ALL);

    // Check for debounce stable
    DWORD dwTimeout = 5000;
    while(((INREG32(&m_pbRegisters->MMCHS_PSTATE) & 0x00020000)!= 0x00020000) && (dwTimeout>0))
    {
        dwTimeout--;
    }

    // For OMAP, the original code is moved to SetCDPolarity(): OUTREG32(&m_pbRegisters->MMCHS_CON, 0x01 << 7); // CDP
    SetCDPolarity();

    SetSDVSVoltage();

    SetClockRate(&dwClockRate);
    if (m_LowVoltageSlot && m_dwSlot == MMCSLOT_1 && m_dwCPURev == CPU_FAMILY_35XX_REVISION_ES_2_1)
    {
        SendInitSequence();
    }
    EnableSDHCInterrupts();
    EnableSDHCCDInterrupts();

    // indicate device arrival
    IndicateSlotStateChange(DeviceInserted);

    TurnCardPowerOff();
}

//-----------------------------------------------------------------------------
CSDIOControllerBase::~CSDIOControllerBase()
{
    DeleteCriticalSection( &m_critSec );
    DeleteCriticalSection( &m_powerCS );
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::Init( LPCTSTR pszActiveKey )
{
    SD_API_STATUS      status;              // SD status
    CReg               regDevice;           // encapsulated device key
    DWORD              dwRet = 0;           // return value

    PHYSICAL_ADDRESS PortAddress;

    DEBUGMSG(SDCARD_ZONE_INFO, (L"SDHC +Init\r\n"));
    DEBUGMSG(SDCARD_ZONE_INFO, (L"SDHC Active RegPath: %s \r\n",pszActiveKey));

    CalibrateStallCounter();

    m_dwCPURev = (DWORD)CPU_REVISION_UNKNOWN;
    KernelIoControl(IOCTL_HAL_GET_CPUREVISION, NULL, 0, &m_dwCPURev, sizeof(m_dwCPURev), NULL);

    // open the parent bus driver handle
    m_hParentBus = CreateBusAccessHandle(pszActiveKey);
    if ( m_hParentBus == NULL )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, 
            (L"SDHC: Failed to obtain parent bus handle\r\n")
            );
        goto cleanUp;
    }

    // allocate the context - we only support one slot
    status = SDHCDAllocateContext(1, &m_pHCContext);
    if (!SD_API_SUCCESS(status))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (L"SDHC Failed to allocate context: 0x%08X \r\n", status));
        goto cleanUp;
    }

    // Set our extension
    m_pHCContext->pHCSpecificContext = this;
    if (GetDeviceRegistryParams(pszActiveKey, this, dimof(s_deviceRegParams), 
        s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (L"ERROR: CSDIOControllerBase:Init: "
            L"Failed read SDHC driver registry parameters\r\n"
            ));
        goto cleanUp;
    }

    // get the Command and Data timeouts
    m_dwCurrentWakeupSources = m_dwWakeupSources & (~WAKEUP_SDIO);
    m_wCTOTimeout = min(m_wCTOTimeout, MMC_CTO_CONTROL_MAX);
    m_wDTOTimeout = min(m_wDTOTimeout, MMC_DTO_CONTROL_MAX);
    m_dwMaxClockRate = m_dwMaxClockRate == 0 ? STD_HC_MAX_CLOCK_FREQUENCY : 
                                               min(m_dwMaxClockRate, STD_HC_MAX_CLOCK_FREQUENCY);

    DEBUGMSG(SDCARD_ZONE_INFO, 
        (L"SDHC_%d host controller initialize: m_fastPathSDIO:%d m_fastPathSDMEM:%d\r\n",
        m_dwSlot, m_fastPathSDIO, m_fastPathSDMEM)
        );
    
	m_dwDeviceID = SOCGetSDHCDeviceBySlot(m_dwSlot);
	
    if (!RequestDevicePads(m_dwDeviceID))
    {
	    ERRORMSG(1, (_T("SDHCInitialize:: Error requesting pads  id %d\r\n"),m_dwDeviceID));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    if (m_dwCardDetectGPIO == -1)
    {
        m_dwCardDetectGPIO = BSPGetSDHCCardDetect(m_dwSlot);
    }

    // Open the GPIO driver if Card Detection uses GPIO
    if (m_dwCardDetectGPIO != DWORD(-1))
    {
        m_hGPIO = GPIOOpen();
	
        if( m_hGPIO == NULL )
        {
            ERRORMSG(1, (_T("SDHCInitialize:: Error opening the GPIO driver!\r\n")));
            status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
            goto cleanUp;
        }
    }
	
    // map hardware memory space
    PortAddress.QuadPart = GetAddressByDevice(m_dwDeviceID);
    m_pbRegisters = (OMAP_MMCHS_REGS *)MmMapIoSpace(PortAddress, sizeof(OMAP_MMCHS_REGS), FALSE );
    if ( !m_pbRegisters )
    {
        ERRORMSG(1, (_T("SDHCInitialize:: Error allocating SDHC controller register\r\n")));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // turn power and system clocks on
    TurnCardPowerOn();
    if( !InitializeHardware() )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("InitializeHardware:: Error configuring SDHC hardware\r\n")));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        TurnCardPowerOff();
        goto cleanUp;
    }

    // Perform SOC-specific configuration
    SocSdhcDevconf(m_dwSlot);

    // Read SD Host Controller Info from register.
    if (!InterpretCapabilities())
    {
        TurnCardPowerOff();
        goto cleanUp;
    }

    // now register the host controller
    status = SDHCDRegisterHostController(m_pHCContext);
    if (!SD_API_SUCCESS(status))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to register host controller: %0x08X \r\n"),status));
        TurnCardPowerOff();
        goto cleanUp;
    }

    EnableSDHCInterrupts();
    EnableSDHCCDInterrupts();
    TurnCardPowerOff();

    // create the card detect IST thread
    m_htCardDetectIST = CreateThread(NULL, 0, SDHCCardDetectIstThread, this, 0, NULL);

    if (NULL == m_htCardDetectIST)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCCardDetectIstThread create Failed \r\n")));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // return the controller context
    dwRet = (DWORD) this;

cleanUp:
    if ( (dwRet == 0) && m_pHCContext )
    {
        FreeHostContext(FALSE, TRUE);
    }

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHC -Init\r\n")));
    return dwRet;
}

//-----------------------------------------------------------------------------
// Free the host context and associated resources.
VOID CSDIOControllerBase::FreeHostContext( BOOL fRegisteredWithBusDriver, BOOL fHardwareInitialized )
{
    UNREFERENCED_PARAMETER(fHardwareInitialized);
    DEBUGCHK(m_pHCContext);

    if (fRegisteredWithBusDriver)
    {
        // deregister the host controller
        SDHCDDeregisterHostController(m_pHCContext);
    }

    // unmap hardware memory space

    //DeinitializeHardware();
    if (m_pbRegisters)  
	    MmUnmapIoSpace((PVOID)m_pbRegisters, sizeof(OMAP_MMCHS_REGS));

    if( m_hParentBus != NULL )
    {
        TurnCardPowerOff();
        CloseBusAccessHandle( m_hParentBus );
        m_hParentBus = NULL;
    }

    if( m_hGPIO != NULL )
    {
        GPIOClose( m_hGPIO );
        m_hGPIO = NULL;
    }

    // cleanup the host context
    SDHCDDeleteContext(m_pHCContext);
}

//-----------------------------------------------------------------------------
// Process the capabilities register
BOOL CSDIOControllerBase::InterpretCapabilities()
{
    BOOL fRet = TRUE;

    // set the host controller name
    SDHCDSetHCName(m_pHCContext, TEXT("SDHC"));

    // set init handler
    SDHCDSetControllerInitHandler(m_pHCContext, CSDIOControllerBase::SDHCInitialize);

    // set deinit handler
    SDHCDSetControllerDeinitHandler(m_pHCContext, CSDIOControllerBase::SDHCDeinitialize);

    // set the Send packet handler
    SDHCDSetBusRequestHandler(m_pHCContext, CSDIOControllerBase::SDHCBusRequestHandler);

    // set the cancel I/O handler
    SDHCDSetCancelIOHandler(m_pHCContext, CSDIOControllerBase::SDHCCancelIoHandler);

    // set the slot option handler
    SDHCDSetSlotOptionHandler(m_pHCContext, CSDIOControllerBase::SDHCSlotOptionHandler);

    // set maximum block length
    m_usMaxBlockLen = STD_HC_MAX_BLOCK_LENGTH;

    return fRet;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  SDHCControllerIstThread - SDIO/controller IST thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSDIOControllerBase::SDHCControllerIstThread(LPVOID lpParameter)
{
    CSDIOControllerBase *pController = (CSDIOControllerBase*)lpParameter;
    return pController->SDHCControllerIstThreadImpl();
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  SDHCControllerIstThreadImpl - implementation of SDIO/controller IST thread
//                                for driver
//  Input:
//  Output:
//  Return: Thread exit status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD CSDIOControllerBase::SDHCControllerIstThreadImpl()
{
    DWORD code;
    DWORD dwStat;

    SDHCCONTROLLERIST_STATE eCurrentState   = CARD_REMOVED_STATE; // COMMAND_TRANSFER_STATE;
    PSD_BUS_REQUEST         pRequest = NULL;
    SDHCCONTROLLERIST_STATE eNextState = CARD_REMOVED_STATE;
    SD_API_STATUS           status = 0;

    if (!CeSetThreadPriority(GetCurrentThread(), m_dwSDIOPriority))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, 
            (L"CSDIOControllerBase::SDHCControllerIstThreadImpl!ERROR - "
             L"failed to set CEThreadPriority\r\n")
            );
    }

    for(;;)
    {

        // wait for the SDIO/controller interrupt
        code = WaitForSingleObject(m_hControllerISTEvent, INFINITE);

        // check for shutdown
        if (m_fDriverShutdown) break;

        DEBUGMSG(SHC_INTERRUPT_ZONE, 
            (L"CSDIOControllerBase::SDHCControllerIstThreadImpl(): IST\r\n")
            );       

        // request clocks        
        UpdateSystemClock(TRUE);

        EnterCriticalSection( &m_critSec );
        dwStat = READ_EXT_MMC_STATUS();
        LeaveCriticalSection( &m_critSec );

        DEBUGMSG(SHC_INTERRUPT_ZONE, 
            (L"CSDIOControllerBase::SDHCControllerIstThreadImpl(): dwStat=0x%08X\r\n", 
            dwStat)
            );

#ifdef ENABLE_RETAIL_OUTPUT
        if(dwStat > 0x00000001)
            // has bits other than Cmd Cmplt
            RETAILMSG(1,  (L"SDHCControllerIstThreadImpl: dwStat=0x%08X\r\n", dwStat));
#endif

        if (dwStat & EXT_MMCHS_STAT_CD_INTR)
        {
            eCurrentState = (HandleCardDetectInterrupt(dwStat) ? COMMAND_TRANSFER_STATE : CARD_REMOVED_STATE);
            UpdateSystemClock(FALSE);
            if (m_bCommandPending == TRUE)
            {
                m_bCommandPending = FALSE;
                UpdateSystemClock(FALSE);
            }

            /*  
                For OMAP, if dwStat contains EXT_MMCHS_STAT_CD_INTR bits, then it only 
                contains EXT_MMCHS_STAT_CD_INTR bits and nothing else.  
                This is because READ_EXT_MMC_STATUS() returns either 
                m_CardDetectInterruptStatus or READ_MMC_STATUS().  And READ_MMC_STATUS()
                does not include CD intr status bits as they are not used in OMAP.  CD 
                intr status bits are set in SDHCCardDetectIstThreadImpl() when a GPIO intr
                occurs.  So clearing the EXT_MMCHS_STAT_CD_INTR bits in dwStat will always
                result in 0 and hence InterruptDone() is always called, which is the orignal
                OMAP processing.

                However, for Netra, dwStat value returned by READ_EXT_MMC_STATUS()
                (which is always from READ_MMC_STATUS() because m_CardDetectInterruptStatus
                is always 0 for Netra, see comment for #define READ_EXT_MMC_STATUS above) 
                may contain intr bit status other than the EXT_MMCHS_STAT_CD_INTR bits.  
                So the condition !(dwStat & ~EXT_MMCHS_STAT_CD_INTR) is added 
                before calling InterruptDone().

                Notice that adding the condition !(dwStat & ~EXT_MMCHS_STAT_CD_INTR) does
                not change the original processing logic for OMAP but is needed for Netra.
            */
            if (!(dwStat & ~EXT_MMCHS_STAT_CD_INTR))
            {
                InterruptDone( m_dwControllerSysIntr );
                continue;
            }
        }

        if(m_ActualPowerState == D4)
        {
            DEBUGMSG(SHC_INTERRUPT_ZONE, 
                (L"SDHCControllerIstThreadImpl: Register access at D4\r\n")
                );
            
            UpdateSystemClock(FALSE);
            InterruptDone( m_dwControllerSysIntr );
            continue;
        }

        switch (eCurrentState)
        {
            case CARD_REMOVED_STATE:
                DEBUGMSG(SHC_INTERRUPT_ZONE, 
                    (TEXT("ERROR: CSDIOControllerBase::SDHCControllerIstThread: Internal error. No SD card in the slot!!!\r\n"))
                    );
                eNextState = CARD_REMOVED_STATE;
                break;
            case COMMAND_TRANSFER_STATE:
                // get and lock the current bus request
                pRequest = SDHCDGetAndLockCurrentRequest(m_pHCContext, 0);
                status = CommandTransferCompleteHandler(pRequest, dwStat, &eNextState);
                break;
            case DATA_RECEIVE_STATE:
                if( m_DmaEnable )		
                    status = DataReceiveCompletedHandler(pRequest, dwStat, &eNextState);
                break;
            case DATA_TRANSMIT_STATE:
                if(m_DmaEnable)		
                    status = DataTransmitCompletedHandler(pRequest, dwStat, &eNextState);
                break;
            case CARDBUSY_STATE:
                pRequest = SDHCDGetAndLockCurrentRequest(m_pHCContext, 0);
                DEBUGMSG(SHC_INTERRUPT_ZONE, 
                    (TEXT("ERROR: CSDIOControllerBase::SDHCControllerIstThread: CARDBUSY_STATE, pRequest is null!!!\r\n"))
                    );
				
                status = CardBusyCompletedHandler(pRequest, dwStat, &eNextState);
                break;
            default:
                RETAILMSG(SDCARD_ZONE_WARN, 
                    (TEXT("ERROR: CSDIOControllerBase::SDHCControllerIstThread: Internal error. Should get to here!!!\r\n"))
                    );
                break;
        }

        if( ((eNextState == COMMAND_TRANSFER_STATE)) && (dwStat & MMCHS_STAT_CIRQ))
        {

            ASSERT( m_fSDIOInterruptsEnabled );
            // indicate that the card is interrupting
            DEBUGMSG(SHC_INTERRUPT_ZONE, (TEXT("CSDIOControllerBase::SDHCControllerIstThread: got SDIO interrupt!\r\n")));
            DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("Received SDIO interrupt\r\n")));

            // disable the SDIO interrupt
            EnterCriticalSection( &m_critSec );
            CLRREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_CIRQ);
            LeaveCriticalSection( &m_critSec );

            // notify the SDBusDriver of the SDIO interrupt
            m_fSDIOInterruptInService = TRUE;
            SDHCDIndicateSlotStateChange(m_pHCContext, 0, DeviceInterrupting);
        }

        UpdateSystemClock(FALSE);
        InterruptDone( m_dwControllerSysIntr );

        if (eNextState == COMMAND_TRANSFER_STATE)
        {
            // We need to release the clock once more when a command is completed because the 
            // we requested the clock before sending out a command the via normal path and didn't 
            // release the clock there. We also request the clock again when interrupt occurs, to make
            // sure the clock is on to avoid crash, so we need to release the clock twice when
            // a command is completed.
            //RETAILMSG(1, (TEXT("=2===========> %d %d\r\n"), eNextState, m_dwClockCnt ));
            UpdateSystemClock(FALSE);
            m_bCommandPending = FALSE;
            if (pRequest != NULL) 
            {
                SDHCDIndicateBusRequestComplete(m_pHCContext, pRequest, status);
                pRequest = NULL;
            }
        }

        eCurrentState = eNextState;
    }

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHCCardDetectIstThread: Thread Exiting\r\n")));
    return 0;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  CheckIntrStatus -             Check interrupt status.
//  Input:  dwIntrStatus:         Interrupt status
//  Output: pOverwrite:           Override flag
//  Return:                       Interrupt process status.
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::CheckIntrStatus(
    DWORD dwIntrStatus,
    DWORD *pOverwrite
    )
{
    SD_API_STATUS status        = SD_API_STATUS_PENDING;  
    DWORD         dwOverwrite   = 0;

    if( dwIntrStatus & MMCHS_STAT_CCRC ) // command CRC error
    {
        status = SD_API_STATUS_CRC_ERROR;
        dwOverwrite |= MMCHS_STAT_CCRC;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command CRC error!\r\n")));
    }
    if( dwIntrStatus & MMCHS_STAT_CTO ) // command response timeout
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        dwOverwrite |= MMCHS_STAT_CTO;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command response timeout!\r\n")));
    }
    if( dwIntrStatus & MMCHS_STAT_DTO ) // data timeout
    {
        status = SD_API_STATUS_DATA_TIMEOUT;
        dwOverwrite |= MMCHS_STAT_DTO;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got data timeout!\r\n")));
    }
    if( dwIntrStatus & MMCHS_STAT_DCRC ) // data CRC error
    {
        status = SD_API_STATUS_DATA_ERROR;
        dwOverwrite |= MMCHS_STAT_DCRC;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got data CRC error!\r\n")));
    }
    if( dwOverwrite ) // clear the status error bits
    {
        Write_MMC_STAT(dwOverwrite);
    }

    if (pOverwrite != NULL)
    {
        *pOverwrite = dwOverwrite;
    }

    return status;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  CommandTransferCompleteHandler -  Process command complete.
//  Input:  pRequest:             pointer to the current request.
//          dwIntrStatus:         Current interrupt status.
//  Output: pNextState:           pointer to next IST state.
//  Return:                       Interrupt process status.
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::CommandTransferCompleteHandler(
    PSD_BUS_REQUEST             pRequest,
    DWORD                       dwIntrStatus,
    PSDHCCONTROLLERIST_STATE    pNextState
    )
{
    SDHCCONTROLLERIST_STATE eNextControllerState = COMMAND_TRANSFER_STATE;
    DWORD           dwStatusOverwrite = 0;
    SD_API_STATUS   status            = SD_API_STATUS_PENDING;
    DWORD           MMC_STAT          = Read_MMC_STAT();

    UNREFERENCED_PARAMETER(dwIntrStatus);

    DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("+CSDIOControllerBase::CommandTransferCompleteHandler\r\n")));

    // get and lock the current bus request
    if (pRequest == NULL)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDIOControllerBase::CommandTransferCompleteHandler - Unable to get/lock current request!\r\n")));
        status = SD_API_STATUS_INVALID_DEVICE_REQUEST;

        Write_MMC_STAT(MMC_STAT);

        goto TRANSFER_DONE;
    }

    DWORD MMC_PSTAT = INREG32(&m_pbRegisters->MMCHS_PSTATE);
 
    if ((MMC_PSTAT & MMCHS_PSTAT_DATI) && (pRequest->CommandResponse.ResponseType == ResponseR1b))
    {
        if (!( MMC_STAT & ( MMCHS_STAT_CCRC | MMCHS_STAT_CTO | MMCHS_STAT_DCRC | MMCHS_STAT_DTO) ))
        {
            eNextControllerState = CARDBUSY_STATE;

            Write_MMC_STAT(MMC_STAT);

            goto cleanUp;
        }
    }

    status = CheckIntrStatus(MMC_STAT, &dwStatusOverwrite);

    if (dwStatusOverwrite != 0)
    {
        goto TRANSFER_DONE;
    }

    // get the response information
    if(pRequest->CommandResponse.ResponseType == NoResponse)
    {
        DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned no response (no response expected)\r\n")));
        goto TRANSFER_DONE;
    }
    else
    {
        status =  GetCommandResponse(pRequest);
        if(!SD_API_SUCCESS(status))
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDBusRequestHandler() - Error getting response for command:0x%02x\r\n"), pRequest->CommandCode));
            goto TRANSFER_DONE;
        }
    }

    switch(pRequest->TransferClass)
    {
        case SD_READ:
            status = ReceiveHandler(pRequest, &eNextControllerState);
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("+CSDIOControllerBase::CommandTransferCompleteHandler ReceiveHandler returns %d, %d\r\n"), status, eNextControllerState));
            break;
        case SD_WRITE:
            status = TransmitHandler(pRequest, &eNextControllerState);
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("+CSDIOControllerBase::CommandTransferCompleteHandler calling TransmitHandler returns %d, %d\r\n"), status, eNextControllerState));
            break;
        default:
            MMC_STAT = Read_MMC_STAT();
            Write_MMC_STAT(MMC_STAT);
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("+CSDIOControllerBase::CommandTransferCompleteHandler: command completed.\r\n")));
            break;
    }

    if(!m_fCardPresent)
    {
        status = SD_API_STATUS_DEVICE_REMOVED;
    }

TRANSFER_DONE:

    if (eNextControllerState == COMMAND_TRANSFER_STATE)
    {
        ProcessCommandTransferStatus(pRequest, status, dwStatusOverwrite);
    }

cleanUp:

    if (pNextState != NULL)
    {
        *pNextState = eNextControllerState;
    }
    
    return status;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  ReceiveHandler -  Initialize data receive operation..
//  Input:  pRequest:             pointer to the current request.
//  Output: pNextState:           pointer to next IST state.
//  Return:                       Interrupt process status.
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS   CSDIOControllerBase::ReceiveHandler(
    PSD_BUS_REQUEST             pRequest, 
    PSDHCCONTROLLERIST_STATE    peNextState
    )
{
    SD_API_STATUS   status          = SD_API_STATUS_SUCCESS ;
    BOOL            FastPathMode    = FALSE;
    BOOL            fRet;

    FastPathMode = ((pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)) ? TRUE : FALSE;

    __try
        {
        DWORD cbTransfer = TRANSFER_SIZE(pRequest);

        RETAILMSG (0, (TEXT("CSDIOControllerBase::ReceiveHandler: cbTransfer=%d\r\n"),cbTransfer));
        if(FastPathMode)
            {
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::ReceiveHandler: calling SDIReceive\r\n")));
            fRet = SDIReceive(pRequest->pBlockBuffer, cbTransfer, FastPathMode);
            }
        else
            {
            if( m_fDMATransfer && m_DmaEnable)
                {
                fRet = StartDMAReceive(pRequest->pBlockBuffer, cbTransfer);
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::ReceiveHandler: StartDMAReceive returns %d\r\n"), fRet));
                if (fRet == TRUE)
                    {
                    status = SD_API_STATUS_PENDING;
                    if (peNextState != NULL)
                        {
                        *peNextState = DATA_RECEIVE_STATE;
                        }
                    }
                }
            else
                {
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::ReceiveHandler: calling SDIPollingReceive\r\n")));
                fRet = SDIPollingReceive(pRequest->pBlockBuffer, cbTransfer);
                }
            }
        }
    __except(SDProcessException(GetExceptionInformation())) 
        {
        fRet = FALSE;
        }

    if (!fRet)
        {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDIOControllerBase::ReceiveHandler - SDIPollingReceive() failed\r\n")));
        status = SD_API_STATUS_DATA_ERROR;
        }

    return status;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  TransmitHandler -  Initialize data transmit operation..
//  Input:  pRequest:             pointer to the current request.
//  Output: pNextState:           pointer to next IST state.
//  Return:                       Interrupt process status.
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS    CSDIOControllerBase::TransmitHandler(
    PSD_BUS_REQUEST             pRequest, 
    PSDHCCONTROLLERIST_STATE    peNextState
    )
{
    SD_API_STATUS   status            = SD_API_STATUS_PENDING;
    DWORD cbTransfer = TRANSFER_SIZE(pRequest);

    BOOL            FastPathMode    = FALSE;
    BOOL            fRet;

    FastPathMode = ((pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)) ? TRUE : FALSE;

    __try 
        {
        if(FastPathMode)
            {
            fRet = SDITransmit(pRequest->pBlockBuffer, cbTransfer, FastPathMode);
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::TransmitHandler: SDITransmit returns %d\r\n"), fRet));
            status = SD_API_STATUS_PENDING;
            if (peNextState != NULL)
                {
                *peNextState = DATA_TRANSMIT_STATE;
                }
            }
        else
            {
            if( m_fDMATransfer && m_DmaEnable)
                {
                fRet = StartDMATransmit(pRequest->pBlockBuffer, cbTransfer);
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::TransmitHandler: StartDMATransmit returns %d\r\n"), fRet));
                if (fRet == TRUE)
                    {
                    status = SD_API_STATUS_PENDING;
                    if (peNextState != NULL)
                        {
                        *peNextState = DATA_TRANSMIT_STATE;
                        }
                    }
                }
            else
                {
                fRet = SDIPollingTransmit(pRequest->pBlockBuffer, cbTransfer);
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::TransmitHandler: SDIPollingTransmit returns %d\r\n"), fRet));
                }
            }
        }
    __except(SDProcessException(GetExceptionInformation())) 
        {
        fRet = FALSE;
        }

    if( !fRet )
        {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDIOControllerBase::TransmitHandler - SDIPollingTransmit() failed\r\n")));
        DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("PollingTransmit failed to send %d bytes\r\n"), cbTransfer));
        status = SD_API_STATUS_DATA_ERROR;
        }
    else
        {
        DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("PollingTransmit succesfully sent %d bytes\r\n"), cbTransfer));
        }

    return status;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  ProcessCommandTransferStatus -  post process for comannd transfer complete operation
//  Input:  pRequest:             pointer to the current request.
//          status:               command transfer complete process status
//          dwStatusOverwrite:    status overwrite status.
//  Return:                       Interrupt process status.
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS   CSDIOControllerBase::ProcessCommandTransferStatus(
    PSD_BUS_REQUEST         pRequest, 
    SD_API_STATUS           status, 
    DWORD                   dwStatusOverwrite
    )
{
    UNREFERENCED_PARAMETER(dwStatusOverwrite);

    if( status == SD_API_STATUS_SUCCESS )
        {
        if( m_fAppCmdMode )
            {
            m_fAppCmdMode = FALSE;
            DEBUGMSG(SHC_SEND_ZONE, (TEXT("CSDIOControllerBase::ProcessCommandTransferStatus - Switched to Standard Command Mode\r\n")));
            }
        else if( pRequest && pRequest->CommandCode == 55 )
            {
            m_fAppCmdMode = TRUE;
            DEBUGMSG(SHC_SEND_ZONE, (TEXT("CSDIOControllerBase::ProcessCommandTransferStatus - Switched to Application Specific Command Mode\r\n")));
            }

        if( pRequest->CommandCode == SD_CMD_MMC_SEND_OPCOND )
            {
            DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("CSDIOControllerBase::ProcessCommandTransferStatus Card is recognized as a MMC\r\n") ) );
            m_fMMCMode = TRUE;
            }
        }

    if(m_ActualPowerState == D4)
        {
        if( pRequest != NULL )
            SDHCDIndicateBusRequestComplete(m_pHCContext, pRequest, status);
        return TRUE;
        }


    // Clear the MMC_STAT register
    DWORD MMC_STAT = Read_MMC_STAT();
    Write_MMC_STAT(MMC_STAT);

    if (MMC_STAT)
        {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("+CSDIOControllerBase::ProcessCommandTransferStatus: status = %x\r\n"), MMC_STAT));
        }

    return status;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  StartDMAReceive -  Initialize DMA data receive operation..
//  Input:  pBuff:                pointer receive buffer.
//          dwLen:                size of receiver buffer.
//  Output: None.
//  Return:                       Success or failed
//  Notes:
///////////////////////////////////////////////////////////////////////////////
BOOL    CSDIOControllerBase::StartDMAReceive(PBYTE pBuff, DWORD dwLen)
{
    BOOL    bRet = TRUE;

    DWORD MMC_STAT = Read_MMC_STAT();

    Write_MMC_STAT(MMC_STAT);

    if( MMC_STAT & (MMCHS_STAT_DTO | MMCHS_STAT_DCRC))
        {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DTO or MMC_STAT_DCRC error (%x).\r\n"), MMC_STAT));
        bRet = FALSE;
        }
    else
        {
        EnterCriticalSection( &m_critSec );
        OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
        LeaveCriticalSection( &m_critSec );

        m_pCurrentRecieveBuffer = pBuff;
        m_dwCurrentRecieveBufferLength = dwLen;

        // start the DMA
        SDIO_StartInputDMA();

        if (m_pCachedDmaBuffer != NULL)
            {
            CacheRangeFlush(m_pCachedDmaBuffer, m_dwCurrentRecieveBufferLength, TI_CACHE_SYNC_INVALIDATE);
            }
        }

    return bRet;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  StartDMATransmit -  Initialize DMA data transmit operation..
//  Input:  pBuff:                pointer transmit buffer.
//          dwLen:                size of transmit buffer.
//  Output: None.
//  Return:                       Success or failed
//  Notes:
///////////////////////////////////////////////////////////////////////////////
BOOL    CSDIOControllerBase::StartDMATransmit(PBYTE pBuff, DWORD dwLen)
{
    BOOL    bRet;

    // first copy the data to the DMA buffer, then to the user buffer, which maybe SG buffer,
#if 1
    // We use uncached DMA buffer instead of cached DMA for write operation because it give
    // better performance. 
    bRet = SDPerformSafeCopy( m_pDmaBuffer, pBuff, dwLen );
#else
    if (m_pCachedDmaBuffer == NULL)
        {
        bRet = SDPerformSafeCopy( m_pDmaBuffer, pBuff, dwLen );
        }
    else
        {
        bRet = SDPerformSafeCopy( m_pCachedDmaBuffer, pBuff, dwLen );
        CacheRangeFlush(m_pCachedDmaBuffer, dwLen, CACHE_SYNC_WRITEBACK);
        }
#endif

    if( bRet == FALSE)
        {
        goto cleanUp;
        }

    DWORD MMC_STAT = Read_MMC_STAT();

    Write_MMC_STAT(MMC_STAT);

    if( MMC_STAT & (MMCHS_STAT_DTO | MMCHS_STAT_DCRC))
        {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_STAT_DTO or MMC_STAT_DCRC error(%x).\r\n"), MMC_STAT));
        bRet = FALSE;
        }
    else
        {
        // start the DMA
        EnterCriticalSection( &m_critSec );
        SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_TC);
        SETREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_TC);
        OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
        LeaveCriticalSection( &m_critSec );

        SDIO_StartOutputDMA();
        }

cleanUp:

    return bRet;
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  DataReceiveCompletedHandler -  process data receive complete.
//  Input:  pRequest:             point to the current request
//          dwIntrStatus:         current interrupt status.
//  Output: pNextState.           pointer to next IST state.
//  Return:                       process status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::DataReceiveCompletedHandler(
    PSD_BUS_REQUEST             pRequest,
    DWORD                       dwIntrStatus,
    PSDHCCONTROLLERIST_STATE    pNextState
    )
{
    SD_API_STATUS   status      = SD_API_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pRequest);

    static DWORD           dwSize      = 0;

    if(dwIntrStatus & (MMCHS_STAT_BRR | MMCHS_STAT_TC))
        {
        Read_MMC_STAT();
   
        // stop DMA
        SDIO_StopInputDMA();

        Set_MMC_STAT(MMCHS_STAT_TC);

        if ((m_pCurrentRecieveBuffer != NULL) && (m_dwCurrentRecieveBufferLength != 0))
            {
            // finally, copy the data from DMA buffer to the user buffer, which maybe SG buffer,
            SDPerformSafeCopy(m_pCurrentRecieveBuffer, (m_pCachedDmaBuffer!=NULL) ? m_pCachedDmaBuffer : m_pDmaBuffer, m_dwCurrentRecieveBufferLength);

            m_pCurrentRecieveBuffer = NULL;
            m_dwCurrentRecieveBufferLength = 0;
            }
        }
    else
        {
        if( dwIntrStatus & MMCHS_STAT_DTO )
            {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
            }
        if( dwIntrStatus & MMCHS_STAT_DCRC ) // DATA CRC Error
            {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DCRC error.\r\n")));
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
            }
        }

    if (pNextState != NULL)
        {
        *pNextState = COMMAND_TRANSFER_STATE;
        }

    return status;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  DataTransmitCompletedHandler -  process data transmit complete.
//  Input:  pRequest:             point to the current request
//          dwIntrStatus:         current interrupt status.
//  Output: pNextState.           pointer to next IST state.
//  Return:                       process status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::DataTransmitCompletedHandler(
    PSD_BUS_REQUEST             pRequest,
    DWORD                       dwIntrStatus,
    PSDHCCONTROLLERIST_STATE    pNextState
    )
{
    SD_API_STATUS   status = SD_API_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pRequest);

    if (dwIntrStatus & MMCHS_STAT_TC)
        {
        Read_MMC_STAT();

        // stop DMA
        SDIO_StopOutputDMA();

        Set_MMC_STAT(MMCHS_STAT_TC);
        }
    else
        {
        // stop DMA
        SDIO_StopOutputDMA();

        if( dwIntrStatus & MMCHS_STAT_DTO )
            {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("DataTransmitCompletedHandler() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
            }
        if( dwIntrStatus & MMCHS_STAT_DCRC ) // DATA CRC Error
            {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("DataTransmitCompletedHandler() - exit: STAT register indicates MMC_CRC_ERROR_RCVD error.\r\n")));
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
            }
        }

    if (pNextState != NULL)
        {
        *pNextState = COMMAND_TRANSFER_STATE;
        }

    return status;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  CardBusyCompletedHandler -  process card busy complete.
//  Input:  pRequest:             point to the current request
//          dwIntrStatus:         current interrupt status.
//  Output: pNextState.           pointer to next IST state.
//  Return:                       process status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::CardBusyCompletedHandler(
    PSD_BUS_REQUEST             pRequest,
    DWORD                       dwIntrStatus,
    PSDHCCONTROLLERIST_STATE    pNextState
    )
{
    // when card busy is complete, process it as a normal command complete interrupt.
    return CommandTransferCompleteHandler(pRequest, dwIntrStatus, pNextState);
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  SDHCCardDetectIstThread - card detect IST thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSDIOControllerBase::SDHCCardDetectIstThread(LPVOID lpParameter)
{
    CSDIOControllerBase *pController = (CSDIOControllerBase*)lpParameter;
    return pController->SDHCCardDetectIstThreadImpl();
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  CSDIOControllerBase::SDHCDeinitializeImpl - Deinitialize the SDHC Controller
//  Input:
//  Output:
//  Return: SD_API_STATUS
//  Notes:
//
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::SDHCDeinitializeImpl()
{
    DEBUGMSG(SDCARD_ZONE_INFO, (L"CSDIOControllerBase::SDHCDeinitializeImpl++\r\n"));
    
    // mark for shutdown
    m_fDriverShutdown = TRUE;

    if (m_fInitialized)
    {
        if( m_dwControllerSysIntr != SYSINTR_UNDEFINED )
        {
            // disable wakeup on SDIO interrupt
            if ( m_dwCurrentWakeupSources & WAKEUP_SDIO )
            {
                KernelIoControl( IOCTL_HAL_DISABLE_WAKE,
                    &m_dwControllerSysIntr,
                    sizeof( m_dwControllerSysIntr ),
                    NULL,
                    0,
                    NULL );
            }

            // disable controller interrupt
            InterruptDisable(m_dwControllerSysIntr);

            // release the SYSINTR value
            KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &m_dwControllerSysIntr, sizeof(DWORD), NULL, 0, NULL);
            m_dwControllerSysIntr = SYSINTR_UNDEFINED;
        }


        if (m_fCardPresent)
        {
           // remove device
           HandleRemoval(FALSE);
        }
        DeinitializeHardware();
    }

    // clean up controller IST
    if (NULL != m_htControllerIST)
    {
        // wake up the IST
        SetEvent(m_hControllerISTEvent);
        // wait for the thread to exit
        WaitForSingleObject(m_htControllerIST, INFINITE);
        CloseHandle(m_htControllerIST);
        m_htControllerIST = NULL;
    }

    // free controller interrupt event
    if (NULL != m_hControllerISTEvent)
    {
        CloseHandle(m_hControllerISTEvent);
        m_hControllerISTEvent = NULL;
    }

    // clean up card detect IST
    if (NULL != m_htCardDetectIST)
    {
        // wake up the IST
        SetEvent(m_hCardDetectEvent);
        // wait for the thread to exit
        WaitForSingleObject(m_htCardDetectIST, INFINITE);
        CloseHandle(m_htCardDetectIST);
        m_htCardDetectIST = NULL;
    }

    // free card detect interrupt event
    if (NULL != m_hCardDetectEvent)
    {
        CloseHandle(m_hCardDetectEvent);
        m_hCardDetectEvent = NULL;
    }
    // clean up power thread
    if (NULL != m_hTimerThreadIST)
    {
        m_bExitThread = TRUE;
        SetEvent(m_hTimerEvent);
        WaitForSingleObject(m_hTimerThreadIST, INFINITE);
        CloseHandle(m_hTimerThreadIST);
        m_hTimerThreadIST = NULL;
    }

    if (m_hTimerEvent != NULL)
    {
        CloseHandle(m_hTimerEvent);
    }

    if(m_DmaEnable)
        SDIO_DeinitDMA();

    // Release pads
    ReleaseDevicePads(SOCGetSDHCDeviceBySlot(m_dwSlot));

    return SD_API_STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
//  CSDIOControllerBase::SDHCInitialize - Initialize the the controller
//  Input:
//  Output:
//  Return: SD_API_STATUS
//  Notes: Assume card power is already enabled.
//
SD_API_STATUS CSDIOControllerBase::SDHCInitializeImpl()
{
    SD_API_STATUS status = SD_API_STATUS_INSUFFICIENT_RESOURCES; // intermediate status
    DMA_ADAPTER_OBJECT dmaAdapter;
    DWORD         threadID;
    DWORD *pdwSDIOIrq;
    DWORD dwSDIOIrqLen;
    DWORD dwClockRate;

    DEBUGMSG(SDCARD_ZONE_INFO, (L"CSDIOControllerBase::SDHCInitializeImpl++\r\n"));

    // allocate the DMA buffer
    dmaAdapter.ObjectSize = sizeof(dmaAdapter);
    dmaAdapter.InterfaceType = Internal;
    dmaAdapter.BusNumber = 0;
    m_pDmaBuffer = (PBYTE)HalAllocateCommonBuffer( &dmaAdapter, m_dwDMABufferSize, &m_pDmaBufferPhys, FALSE );
    ASSERT(m_pDmaBuffer);

    if( m_pDmaBuffer == NULL )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("InitializeInstance:: Error allocating DMA buffer!\r\n")));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    //  Change the attributes of the buffer for cache write combine to improve write performance.
    if( !CeSetMemoryAttributes(m_pDmaBuffer, (void *)(m_pDmaBufferPhys.LowPart >> 8), m_dwDMABufferSize, PAGE_WRITECOMBINE))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (L"InitializeInstance:: Error failed CeSetMemoryAttributes for SDHC DMA buffer\r\n"));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // map DMA buffer to cached memory so we can get better performance in the read operations. The will need to invalidate 
    // (not writeback) the cahced memory before copying data from the cached DMA to the MDD buffer!!!  
    m_pCachedDmaBuffer = (PBYTE)VirtualAlloc(NULL, m_dwDMABufferSize, MEM_RESERVE, PAGE_READWRITE);

    if (m_pCachedDmaBuffer == NULL)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("InitializeInstance:: Error allocating DMA cached buffer!\r\n")));
    }
    else
    {
        if (!VirtualCopy(m_pCachedDmaBuffer, (PVOID)(m_pDmaBufferPhys.LowPart>>8), m_dwDMABufferSize, PAGE_READWRITE | PAGE_PHYSICAL))
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("InitializeInstance:: Error mapping DMA cached buffer!\r\n")));
            VirtualFree(m_pCachedDmaBuffer, m_dwDMABufferSize, MEM_RELEASE);
            m_pCachedDmaBuffer = NULL;
        }
    }

    SoftwareReset(SOFT_RESET_ALL);

    dwClockRate = MMCSD_CLOCK_INIT;
    SetClockRate(&dwClockRate);

    // convert the SDIO hardware IRQ into a logical SYSINTR value
    DWORD rgdwSDIOIrq[] = {
        GetIrqByDevice(SOCGetSDHCDeviceBySlot(MMCSLOT_1), NULL),
        GetIrqByDevice(SOCGetSDHCDeviceBySlot(MMCSLOT_2), NULL)
        };
    if(m_dwSlot == MMCSLOT_1)
      pdwSDIOIrq = &rgdwSDIOIrq[0];
    else
      pdwSDIOIrq = &rgdwSDIOIrq[1];
    dwSDIOIrqLen = sizeof(DWORD);

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, pdwSDIOIrq, dwSDIOIrqLen, &m_dwControllerSysIntr, sizeof(DWORD), NULL))
    {
        // invalid SDIO SYSINTR value!
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("Error obtaining SDIO SYSINTR value!\r\n")));
        m_dwControllerSysIntr = SYSINTR_UNDEFINED;
        status = SD_API_STATUS_UNSUCCESSFUL;
        goto cleanUp;
    }

    // allocate the interrupt event for the SDIO/controller interrupt
    m_hControllerISTEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if (NULL == m_hControllerISTEvent)
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    if ( !InterruptInitialize( m_dwControllerSysIntr, m_hControllerISTEvent, NULL, 0 ) )
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // allocate the interrupt event for card detection
    m_hCardDetectEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

    if (NULL == m_hCardDetectEvent)
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // create the Controller IST thread
    m_htControllerIST = CreateThread(NULL,
        0,
        CSDIOControllerBase::SDHCControllerIstThread,
        this,
        0,
        &threadID);

    if (NULL == m_htControllerIST) 
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

#ifdef SDIO_PRINT_THREAD

    m_cmdRdIndex = m_cmdWrIndex = 0;
    m_hPrintEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

    if (NULL == m_hPrintEvent)
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    m_hPrintIST = CreateThread(NULL,
        0,
        CSDIOControllerBase::SDHCPrintThread,
        this,
        0,
        &threadID);

    if (NULL == m_hPrintIST)
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

#endif

    // start timer thread
    m_bDisablePower = FALSE;
    m_hTimerThreadIST = NULL;
    m_hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hTimerEvent != NULL)
    {
        m_hTimerThreadIST = CreateThread(NULL,
            0,
            CSDIOControllerBase::SDHCPowerTimerThread,
            this,
            0,
            &threadID);
    }

    if (NULL == m_hTimerThreadIST)
    {
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    m_fInitialized = TRUE;

    // on start we need the IST to check the slot for a card
    SetEvent(m_hCardDetectEvent);

    if(m_DmaEnable)
        SDIO_InitDMA();

    status = SD_API_STATUS_SUCCESS;

cleanUp:

    if (!SD_API_SUCCESS(status))
    {
        // just call the deinit handler directly to cleanup
        SDHCDeinitializeImpl();
    }
    return status;
}

//-----------------------------------------------------------------------------
//  CSDIOControllerBase::SDHCSDCancelIoHandlerImpl - io cancel handler
//  Input:  Slot - slot the request is going on
//          pRequest - the request to be cancelled
//
//  Output:
//  Return: TRUE if I/O was cancelled
//  Notes:
//          the HC lock is taken before entering this cancel handler
//
BOOLEAN CSDIOControllerBase::SDHCCancelIoHandlerImpl(UCHAR Slot, PSD_BUS_REQUEST pRequest)
{
    // for now, we should never get here because all requests are non-cancelable
    // the hardware supports timeouts so it is impossible for the controller to get stuck
    DEBUGCHK(FALSE);

    UNREFERENCED_PARAMETER(Slot);
    UNREFERENCED_PARAMETER(pRequest);

    return TRUE;
}

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCBusRequestHandlerImpl_FastPath(PSD_BUS_REQUEST pRequest)
{
    DEBUGCHK(pRequest);

    SD_API_STATUS   status = SD_API_STATUS_FAST_PATH_SUCCESS;
    DWORD            wIe;
    m_fastPathReq = 1;

    // turn the clock on
    UpdateSystemClock(TRUE);

    // acquire the device lock to protect from device removal
    SDHCDAcquireHCLock(m_pHCContext);
    //m_fastPathSDMEM = 1;

    // ??? check register handling, seems odd
    // Disable SDIO interrupt for Fast path
    wIe = (DWORD)INREG32(&(m_pbRegisters->MMCHS_IE));
    EnterCriticalSection( &m_critSec );
    SETREG32(&m_pbRegisters->MMCHS_CON, MMCHS_CON_CTPL);

    SETREG32(&(m_pbRegisters->MMCHS_IE) , MMC_INT_EN_MASK);
    CLRREG32(&(m_pbRegisters->MMCHS_ISE) , MMC_INT_EN_MASK);
    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    LeaveCriticalSection( &m_critSec );

    status = SendCommand(pRequest);
    if (!SD_API_SUCCESS(status))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDBusRequestHandler() - Error sending command:0x%02x\r\n"), pRequest->CommandCode));
        SDHCDReleaseHCLock(m_pHCContext);
        goto cleanUp;      
    }

    {
       DWORD retries = 0;
       status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
 
       // polling end-of-command
       while (!(Read_MMC_STAT() & MMCHS_STAT_CC))
       {
           if (retries > SDIO_MAX_LOOP) {
                SDHCDReleaseHCLock(m_pHCContext);
                if(Read_MMC_STAT() & MMCHS_STAT_CTO)
                {
                    status = SD_API_STATUS_RESPONSE_TIMEOUT;
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCBusRequestHandler() - MMCHS_STAT_CTO\r\n")));
                }
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCBusRequestHandler() - Timeout waiting for MMCHS_STAT_CC\r\n")));
                goto cleanUp;      
           }
           retries++;
       }

       status = CommandCompleteHandler_FastPath(pRequest);

       SDHCDReleaseHCLock(m_pHCContext);

            }
cleanUp:
	
    // Restore SDIO interrupts
    EnterCriticalSection( &m_critSec );
    // ??? check register handling, seems odd...
    CLRREG32(&(m_pbRegisters->MMCHS_IE) , MMC_INT_EN_MASK);
    SETREG32(&(m_pbRegisters->MMCHS_IE), (wIe & (MMCHS_IE_CIRQ | MMC_INT_EN_MASK)));
    SETREG32(&(m_pbRegisters->MMCHS_ISE), (wIe & (MMCHS_ISE_CIRQ | MMC_INT_EN_MASK)));
    OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
    LeaveCriticalSection( &m_critSec );

    UpdateSystemClock(FALSE);

    return status;

                }

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCBusRequestHandlerImpl_NormalPath(PSD_BUS_REQUEST pRequest)
{
    DEBUGCHK(pRequest);

    SD_API_STATUS   status = SD_API_STATUS_FAST_PATH_SUCCESS;
    DEBUGCHK(pRequest);

    if (m_fCardPresent == FALSE)
    {
         status = SD_API_STATUS_DEVICE_REMOVED;
         return status;
    }

    UpdateSystemClock(TRUE);

    SDHCDAcquireHCLock(m_pHCContext);

    status = SendCommand(pRequest);
    m_bCommandPending = TRUE;

    if(!SD_API_SUCCESS(status))
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDBusRequestHandler() - Error sending command:0x%02x\r\n"), pRequest->CommandCode));
        SDHCDReleaseHCLock(m_pHCContext);
        goto cleanUp;      
    }
    // we will handle the command response interrupt on another thread
    status = SD_API_STATUS_PENDING;
    SDHCDReleaseHCLock(m_pHCContext);   // really needed?

cleanUp:

    // No, we don't call UpdateSystemClock(FALSE) here because we don't want to cause
    // the timer thread to timeout while the SDHC controller is processing command,
    // or transferring data. The Controller IST will call the UpdateSystemClock(FALSE) 
    // when the process is is completed or the data transfer is complete.
    if (status != SD_API_STATUS_PENDING)
    {
        UpdateSystemClock(FALSE);
    }

    return status;
}

//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  CSDIOControllerBase::SDHCBusRequestHandlerImpl - bus request handler
//  Input:  pRequest - the request
//
//  Output:
//  Return: SD_API_STATUS
//  Notes:  The request passed in is marked as uncancelable, this function
//          has the option of making the outstanding request cancelable
//          returns status pending
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDIOControllerBase::SDHCBusRequestHandlerImpl(PSD_BUS_REQUEST pRequest)
{
    DEBUGCHK(pRequest);
    
    DEBUGMSG(SDCARD_ZONE_FUNC, 
        (L"CSDIOControllerBase::SDHCBusRequestHandlerImpl-SDHC_%d"
         L"hDevice=0x%08X, SystemFlags=0x%08X, TransferClass=0x%08X, CommandCode=0x%08X, "
         L"CommandArgument=0x%08X, ResponseType=0x%08X, RequestParam=0x%08X, Status=0x%08X, "
         L"NumBlocks=0x%08X, BlockSize=0x%08X, HCParam=0x%08X, pBlockBuffer=0x%08X, "
         L"DataAccessClocks=0x%08X, Flags=0x%08X\r\n",
         m_dwSlot,
         pRequest->hDevice,
         pRequest->SystemFlags,
         pRequest->TransferClass,
         pRequest->CommandCode,
         pRequest->CommandArgument,
         pRequest->CommandResponse.ResponseType,
         pRequest->RequestParam,
         pRequest->Status,
         pRequest->NumBlocks,
         pRequest->BlockSize,
         pRequest->HCParam,
         pRequest->pBlockBuffer,
         pRequest->DataAccessClocks,
         pRequest->Flags)
         );

    SD_API_STATUS   status = SD_API_STATUS_FAST_PATH_SUCCESS;

    if(pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)
    {
       m_fastPathReq = 1;
    }

	DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHCBusRequestHandler: SDHC_%d - CMD::[%d]  FastPath %d \r\n"), m_dwSlot, pRequest->CommandCode , m_fastPathReq));

    /* Choose fastpath or not based on registry settings for SDIO and SD memory cards */
    if ((pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE) && 
        ((!m_fastPathSDIO && m_dwSDIOCard) || (!m_fastPathSDMEM && !m_dwSDIOCard)))
    {
       pRequest->SystemFlags &= ~SD_FAST_PATH_AVAILABLE;
       m_fastPathReq = 0;
    }

    if(pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)
       status = SDHCBusRequestHandlerImpl_FastPath(pRequest);
    else
       status = SDHCBusRequestHandlerImpl_NormalPath(pRequest);

    return status;
}

//-----------------------------------------------------------------------------
//  CommandCompleteHandler_FastPath
//  Input:
//  Output:
//  Notes:
SD_API_STATUS CSDIOControllerBase::CommandCompleteHandler_FastPath(PSD_BUS_REQUEST pRequest)
{
    DWORD               dwCountStart;
    SD_API_STATUS       status = SD_API_STATUS_PENDING;
    DWORD MMC_STAT_OVERWRITE = 0;

    DEBUGMSG(SDCARD_ZONE_FUNC, 
        (L"CSDIOControllerBase::CommandCompleteHandler_FastPath-"
         L"hDevice=0x%08X, SystemFlags=0x%08X, TransferClass=0x%08X, CommandCode=0x%08X, "
         L"CommandArgument=0x%08X, ResponseType=0x%08X, RequestParam=0x%08X, Status=0x%08X, "
         L"NumBlocks=0x%08X, BlockSize=0x%08X, HCParam=0x%08X, pBlockBuffer=0x%08X, "
         L"DataAccessClocks=0x%08X, Flags=0x%08X\r\n",
         pRequest->hDevice,
         pRequest->SystemFlags,
         pRequest->TransferClass,
         pRequest->CommandCode,
         pRequest->CommandArgument,
         pRequest->CommandResponse.ResponseType,
         pRequest->RequestParam,
         pRequest->Status,
         pRequest->NumBlocks,
         pRequest->BlockSize,
         pRequest->HCParam,
         pRequest->pBlockBuffer,
         pRequest->DataAccessClocks,
         pRequest->Flags)
         );

    DWORD MMC_STAT = Read_MMC_STAT();
    DWORD MMC_PSTAT = INREG32(&m_pbRegisters->MMCHS_PSTATE);

    if( MMC_PSTAT & MMCHS_PSTAT_DATI )
    {
        if( pRequest->CommandResponse.ResponseType == ResponseR1b )
        {
            DEBUGMSG(SHC_BUSY_STATE_ZONE, (TEXT("Card in busy state after command!  Delaying...\r\n")));
            // get starting tick count for timeout
            dwCountStart = GetTickCount();

            do 
            {
                MMC_STAT = Read_MMC_STAT();
                MMC_PSTAT = INREG32(&m_pbRegisters->MMCHS_PSTATE);

                // check for card ejection
                if( !SDCardDetect() )
                {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Card removed!\r\n")));
                    status = SD_API_STATUS_DEVICE_REMOVED;
                    goto TRANSFER_DONE;
                }

                // check for timeout
                if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
                {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Card BUSY timeout!\r\n")));
                    status = SD_API_STATUS_RESPONSE_TIMEOUT;
                    goto TRANSFER_DONE;
                }
            } while( (MMC_PSTAT & MMCHS_PSTAT_DATI) && !( MMC_STAT & ( MMCHS_STAT_CCRC | MMCHS_STAT_CTO | MMCHS_STAT_DCRC | MMCHS_STAT_DTO )) );

            DEBUGMSG(SHC_BUSY_STATE_ZONE, (TEXT("Card exited busy state.\r\n")));
        }
    }


    if((MMC_STAT & MMCHS_STAT_CCRC ) && (MMC_STAT & MMCHS_STAT_CTO) )// command CRC &Command timeout
    {

        DWORD               dwCountStart;
        DEBUGCHK(sizeof(OMAP_MMCHS_REGS) % sizeof(DWORD) == 0);

        if(MMC_STAT & MMCHS_STAT_CCRC)
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got control command CRC and timeout!\r\n")));
            MMC_STAT_OVERWRITE |= MMCHS_STAT_CCRC;
        }
        else
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got No control command CRC and timeout!\r\n")));
        }

        status = SD_API_STATUS_CRC_ERROR;
        MMC_STAT_OVERWRITE |= MMCHS_STAT_CTO;
        Write_MMC_STAT(MMC_STAT_OVERWRITE);

        // Reset the controller
        SETREG32(&m_pbRegisters->MMCHS_SYSCTL, MMCHS_SYSCTL_SRC);

        // get starting tick count for timeout
        dwCountStart = GetTickCount();

        // Verify that reset has completed.
        while ((INREG32(&m_pbRegisters->MMCHS_SYSCTL) & MMCHS_SYSCTL_SRC))
        {
            if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
            {
                DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SoftwareReset() - exit: TIMEOUT.\n")));
                break;
            }
            Sleep(0);
        }

        goto TRANSFER_DONE;
    }



    if( MMC_STAT & MMCHS_STAT_CCRC ) // command CRC error
    {
        status = SD_API_STATUS_CRC_ERROR;
        MMC_STAT_OVERWRITE |= MMCHS_STAT_CCRC;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command CRC error!\r\n")));
    }
    
    if( MMC_STAT & MMCHS_STAT_CTO ) // command response timeout
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MMC_STAT_OVERWRITE |= MMCHS_STAT_CTO;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command response timeout!\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::CommandCompleteHandler(MMCHS_STAT_CTO)\r\n"));
#endif        
    }
    
    if( MMC_STAT & MMCHS_STAT_DTO ) // data timeout
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MMC_STAT_OVERWRITE |= MMCHS_STAT_DTO;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command response timeout!\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::CommandCompleteHandler(MMCHS_STAT_DTO)\r\n"));
#endif        
    }
    
    if( MMC_STAT & MMCHS_STAT_DCRC ) // data CRC error
    {
        status = SD_API_STATUS_RESPONSE_TIMEOUT;
        MMC_STAT_OVERWRITE |= MMCHS_STAT_DCRC;
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHControllerIstThread() - Got command response timeout!\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::CommandCompleteHandler(MMCHS_STAT_DCRC)\r\n"));
#endif        
    }
    
    if( MMC_STAT_OVERWRITE ) // clear the status error bits
    {
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::CommandCompleteHandler(MMC_STAT_OVERWRITE)\r\n"));
#endif        
        Write_MMC_STAT(MMC_STAT_OVERWRITE);
        goto TRANSFER_DONE;
    }

    // get the response information
    if(pRequest->CommandResponse.ResponseType == NoResponse)
    {
        DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned no response (no response expected)\r\n")));
        status = SD_API_STATUS_SUCCESS;
        goto TRANSFER_DONE;
    }
    else
    {
        status =  GetCommandResponse(pRequest);
        if(!SD_API_SUCCESS(status))
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCDBusRequestHandler() - Error getting response for command:0x%02x\r\n"), pRequest->CommandCode));
            goto TRANSFER_DONE;     
        }
    }

    if (SD_COMMAND != pRequest->TransferClass) // data transfer
    {
        DWORD cbTransfer = TRANSFER_SIZE(pRequest);
        BOOL     fRet;
        BOOL     FastPathMode = FALSE;

        FastPathMode = (pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE) ? TRUE : FALSE; 

        switch(pRequest->TransferClass)
        {
        case SD_READ:
                __try {
                       fRet = SDIReceive(pRequest->pBlockBuffer, cbTransfer, FastPathMode);
                    }
                __except(SDProcessException(GetExceptionInformation())) {
                    fRet = FALSE;
                }
            if(!fRet)
            {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCBusRequestHandler() - SDIPollingReceive() failed\r\n")));
                status = SD_API_STATUS_DATA_ERROR;
                goto TRANSFER_DONE;
            }
            else
            {
#ifdef DEBUG
                DWORD dwTemp = 0;
                while( dwTemp < cbTransfer && (dwTemp < (HEXBUFSIZE / 2 - 1) ) )
                {
                    szHexBuf[dwTemp*2] = pRequest->pBlockBuffer[dwTemp] / 16;
                    szHexBuf[dwTemp*2+1] = pRequest->pBlockBuffer[dwTemp] % 16;

                    if( szHexBuf[dwTemp*2] < 10 )
                        szHexBuf[dwTemp*2] += '0';
                    else
                        szHexBuf[dwTemp*2] += 'a' - 10;

                    if( szHexBuf[dwTemp*2+1] < 10 )
                        szHexBuf[dwTemp*2+1] += '0';
                    else
                        szHexBuf[dwTemp*2+1] += 'a' - 10;

                    dwTemp++;
                }
                szHexBuf[dwTemp*2] = 0;
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("PollingReceive succesfully received %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#ifdef ENABLE_RETAIL_OUTPUT
                RETAILMSG (1,(TEXT("PollingReceive succesfully received %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#endif
#endif
            }
            break;

        case SD_WRITE:
            {
#ifdef DEBUG
                DWORD dwTemp = 0;
                while( dwTemp < cbTransfer && (dwTemp < (HEXBUFSIZE / 2 - 1) ) )
                {
                    szHexBuf[dwTemp*2] = pRequest->pBlockBuffer[dwTemp] / 16;
                    szHexBuf[dwTemp*2+1] = pRequest->pBlockBuffer[dwTemp] % 16;

                    if( szHexBuf[dwTemp*2] < 10 )
                        szHexBuf[dwTemp*2] += '0';
                    else
                        szHexBuf[dwTemp*2] += 'a' - 10;

                    if( szHexBuf[dwTemp*2+1] < 10 )
                        szHexBuf[dwTemp*2+1] += '0';
                    else
                        szHexBuf[dwTemp*2+1] += 'a' - 10;

                    dwTemp++;
                }
                szHexBuf[dwTemp*2] = 0;
#endif
            }

                __try {
                       fRet = SDITransmit(pRequest->pBlockBuffer, cbTransfer, FastPathMode);
                    }
                __except(SDProcessException(GetExceptionInformation())) {
                    fRet = FALSE;
                }

            if( !fRet )
            {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHCBusRequestHandler() - SDIPollingTransmit() failed\r\n")));
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("PollingTransmit failed to send %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#ifdef ENABLE_RETAIL_OUTPUT
                RETAILMSG (1,(TEXT("PollingTransmit failed to send %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#endif
                status = SD_API_STATUS_DATA_ERROR;
                goto TRANSFER_DONE;
            }
            else
            {
                DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("PollingTransmit succesfully sent %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#ifdef ENABLE_RETAIL_OUTPUT
                RETAILMSG (1,(TEXT("PollingTransmit succesfully sent %d bytes\r\n                                     {%S}\r\n"),
                    cbTransfer, szHexBuf ));
#endif
            }

            break;
        }
        if(!m_fCardPresent)
            status = SD_API_STATUS_DEVICE_REMOVED;
        else
            status = SD_API_STATUS_SUCCESS;
    }

TRANSFER_DONE:

    if( status == SD_API_STATUS_SUCCESS )
    {
        if( m_fAppCmdMode )
        {
            m_fAppCmdMode = FALSE;
            DEBUGMSG(SHC_SEND_ZONE, (TEXT("SDHCBusRequestHandler - Switched to Standard Command Mode\r\n")));
        }
        else if( pRequest && pRequest->CommandCode == 55 )
        {
            m_fAppCmdMode = TRUE;
            DEBUGMSG(SHC_SEND_ZONE, (TEXT("SDHCBusRequestHandler - Switched to Application Specific Command Mode\r\n")));
        }

        if( pRequest->CommandCode == SD_CMD_MMC_SEND_OPCOND )
        {
            DEBUGMSG(SHC_SDBUS_INTERACT_ZONE, (TEXT("SendCommand: Card is recognized as a MMC\r\n") ) );
            m_fMMCMode = TRUE;
        }
    }

    // Clear the MMC_STAT register
    MMC_STAT = Read_MMC_STAT();
    Write_MMC_STAT(MMC_STAT); 

    if( pRequest != NULL )
    {
      if( MMC_STAT_OVERWRITE ) // clear the status error bits
      {
        if( !SDCardDetect() )
        {
          SetEvent( m_hCardDetectEvent );
#ifdef ENABLE_RETAIL_OUTPUT
          RETAILMSG(1, (L"CSDIOControllerBase::SDHCDIndicateBusRequestComplete(%x)\r\n", status));
#endif
          // Update status according to the request type
          if((status == SD_API_STATUS_SUCCESS) && (pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)) 
          {
            status = SD_API_STATUS_FAST_PATH_SUCCESS;
          }
        }
      }
    }

    return status;
}


//-----------------------------------------------------------------------------
VOID CSDIOControllerBase::SetSlotPowerState(CEDEVICE_POWER_STATE state)
{
    UNREFERENCED_PARAMETER(state);
}

//-----------------------------------------------------------------------------
CEDEVICE_POWER_STATE CSDIOControllerBase::GetSlotPowerState()
{
    return D3;
}


//-----------------------------------------------------------------------------
//  CSDIOControllerBase::SDHCSlotOptionHandler - handler for slot option changes
//  Input:  SlotNumber   - the slot the change is being applied to
//          Option       - the option code
//          pData        - data associaSHC with the option
//  Output:
//  Return: SD_API_STATUS
//  Notes:
SD_API_STATUS CSDIOControllerBase::SDHCSlotOptionHandlerImpl(
    UCHAR                 SlotNumber,
    SD_SLOT_OPTION_CODE   Option,
    PVOID                 pData,
    ULONG                 OptionSize)
{
    SD_API_STATUS status = SD_API_STATUS_SUCCESS;   // status

    UNREFERENCED_PARAMETER(SlotNumber);

    SDHCDAcquireHCLock(m_pHCContext);

#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (TEXT(">>>>> SDHCSlotOptionHandlerImpl: Option=%d\r\n"), Option));
#endif

    switch (Option)
    {
        case SDHCDSetSlotInterface:
        {
            PSD_CARD_INTERFACE pInterface = (PSD_CARD_INTERFACE) pData;

            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotInterface(slot=%d, "
                 L"interface=0x%08X, clockrate=%d, writeprotect=%d)\r\n",
                 SlotNumber, pInterface->InterfaceMode, 
                 pInterface->ClockRate, pInterface->WriteProtected)
                );
            
            // set/get internal capabilities
            SD_CARD_INTERFACE_EX sdCardInterfaceEx;
            memset(&sdCardInterfaceEx,0, sizeof(sdCardInterfaceEx));
            sdCardInterfaceEx.InterfaceModeEx.bit.sd4Bit = (pInterface->InterfaceMode == SD_INTERFACE_SD_4BIT? 1: 0);
            sdCardInterfaceEx.InterfaceModeEx.bit.mmc8Bit = (pInterface->InterfaceMode == SD_INTERFACE_MMC_8BIT? 1: 0);			
            sdCardInterfaceEx.ClockRate = pInterface->ClockRate;
            sdCardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected = (pInterface->WriteProtected?1:0);
            SetInterface(&sdCardInterfaceEx);

            // return internal capabilities
            pInterface->InterfaceMode = (sdCardInterfaceEx.InterfaceModeEx.bit.sd4Bit!=0?SD_INTERFACE_SD_4BIT:SD_INTERFACE_SD_MMC_1BIT);
            pInterface->InterfaceMode = (sdCardInterfaceEx.InterfaceModeEx.bit.mmc8Bit!=0?SD_INTERFACE_MMC_8BIT:pInterface->InterfaceMode);
            pInterface->ClockRate =  sdCardInterfaceEx.ClockRate;
            pInterface->WriteProtected = (sdCardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected!=0?TRUE:FALSE);

            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotInterface(out)(slot=%d, "
                 L"interface=0x%08X, clockrate=%d, writeprotect=%d)\r\n",
                 SlotNumber, pInterface->InterfaceMode, 
                 pInterface->ClockRate, pInterface->WriteProtected)
                );
        }
        break;

        case SDHCDSetSlotInterfaceEx: 
        {
            PSD_CARD_INTERFACE_EX pInterface = (PSD_CARD_INTERFACE_EX) pData;

            DEBUGMSG(SDCARD_ZONE_INFO,
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotInterfaceEx(slot=%d, "
                 L"clockrate=%d, 4bit=%d, 8bit=%d, highspeed=%d, writeprotect=%d, "
                 L"highcapacity=%d)\r\n",
                 SlotNumber, pInterface->ClockRate, 
                 pInterface->InterfaceModeEx.bit.sd4Bit,
                 pInterface->InterfaceModeEx.bit.mmc8Bit,
                 pInterface->InterfaceModeEx.bit.sdHighSpeed,
                 pInterface->InterfaceModeEx.bit.sdWriteProtected,
                 pInterface->InterfaceModeEx.bit.sdHighCapacity)
                ); 
#ifdef ENABLE_RETAIL_OUTPUT		
            RETAILMSG(1,
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotInterfaceEx(slot=%d, "
                 L"clockrate=%d, 4bit=%d, 8bit=%d, highspeed=%d, writeprotect=%d, "
                 L"highcapacity=%d)\r\n",
                 SlotNumber, pInterface->ClockRate, 
                 pInterface->InterfaceModeEx.bit.sd4Bit,
                 pInterface->InterfaceModeEx.bit.mmc8Bit,                
                 pInterface->InterfaceModeEx.bit.sdHighSpeed,
                 pInterface->InterfaceModeEx.bit.sdWriteProtected,
                 pInterface->InterfaceModeEx.bit.sdHighCapacity)
                );
			
#endif
            // set/get internal capabilities
            SetInterface((PSD_CARD_INTERFACE_EX)pInterface);

            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotInterfaceEx(out)(slot=%d, "
                 L"clockrate=%d, 4bit=%d, 8bit=%d, highspeed=%d, writeprotect=%d, "
                 L"highcapacity=%d)\r\n",
                 SlotNumber, pInterface->ClockRate, 
                 pInterface->InterfaceModeEx.bit.sd4Bit,
                 pInterface->InterfaceModeEx.bit.mmc8Bit,                
                 pInterface->InterfaceModeEx.bit.sdHighSpeed,
                 pInterface->InterfaceModeEx.bit.sdWriteProtected,
                 pInterface->InterfaceModeEx.bit.sdHighCapacity)
                );
        }
        break;

        case SDHCDSetSlotPower:
        {            
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotPower(slot=%d, power=0x%08X)\r\n",
                SlotNumber, *(DWORD*)pData)
                );

            // this platform has 3.2V tied directly to the slot
            // UNDONE:
            //  We should save this value and propogate to subclass.
        }
        break;

        case SDHCDSetSlotPowerState:
        {
            // validate parameters
            PCEDEVICE_POWER_STATE pcps = (PCEDEVICE_POWER_STATE) pData;
            if( pData == NULL || OptionSize != sizeof(CEDEVICE_POWER_STATE) )
            {
                status = SD_API_STATUS_INVALID_PARAMETER;
                break;
            }
            
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDSetSlotPowerState(slot=%d, powestate=0x%08X)\r\n",
                SlotNumber, *pcps)
                );
            
            SetSlotPowerState( *pcps );
        }
        break;

        case SDHCDGetSlotPowerState:
        {
            // validate parameters
            PCEDEVICE_POWER_STATE pcps = (PCEDEVICE_POWER_STATE) pData;
            if( pData == NULL || OptionSize != sizeof(CEDEVICE_POWER_STATE) )
            {
                status = SD_API_STATUS_INVALID_PARAMETER;
                break;
            }
            
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDGetSlotPowerState(slot=%d, powestate=0x%08X)\r\n",
                SlotNumber, GetSlotPowerState())
                );
            
            *pcps = GetSlotPowerState();
        }
        break;

        case SDHCDWakeOnSDIOInterrupts:
        {
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDWakeOnSDIOInterrupts(slot=%d, enable=0x%08X)\r\n",
                SlotNumber, *(BOOL*)pData)
                );

            // only enable SDIO wake interrupt if controller is able to
            BOOL bEnable = *(BOOL*)pData;
            if ( m_dwWakeupSources & WAKEUP_SDIO )
            {
                DWORD dwCurrentWakeupSources = m_dwWakeupSources & ~WAKEUP_SDIO;
                if (bEnable) 
                {
                    m_dwCurrentWakeupSources |= WAKEUP_SDIO;
                }

                if( m_dwCurrentWakeupSources != dwCurrentWakeupSources )
                {
                    KernelIoControl( IOCTL_HAL_ENABLE_WAKE,
                        &m_dwControllerSysIntr,
                        sizeof( m_dwControllerSysIntr ),
                        NULL,
                        0,
                        NULL );
                }
                else
                {
                    KernelIoControl( IOCTL_HAL_DISABLE_WAKE,
                        &m_dwControllerSysIntr,
                        sizeof( m_dwControllerSysIntr ),
                        NULL,
                        0,
                        NULL );
                }
    
                m_dwCurrentWakeupSources = dwCurrentWakeupSources;
            }
            else
            {
                status = SD_API_STATUS_UNSUCCESSFUL;
            }
        }
        break;

        case SDHCDAckSDIOInterrupt:
        {
            DEBUGMSG(SDCARD_ZONE_INFO, 
                    (L"SHCSDSlotOptionHandler-SDHCDAckSDIOInterrupt(slot=%d)\r\n",
                    SlotNumber)
                    );
            
            AckSDIOInterrupt();
        }
        break;

        case SDHCDEnableSDIOInterrupts:
        {
            // this platform has 3.2V tied directly to the slot
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDEnableSDIOInterrupts(slot=%d)\r\n",
                SlotNumber)
                );
            
            EnableSDIOInterrupts();
        }
        break;

        case SDHCDDisableSDIOInterrupts:
        {
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDDisableSDIOInterrupts(slot=%d)\r\n",
                SlotNumber)
                );

            // UNDONE: should we disable SDIO interrupts???
        }
        break;

        case SDHCDGetWriteProtectStatus:
        {
            PSD_CARD_INTERFACE pInterface = (PSD_CARD_INTERFACE) pData;
                
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDGetWriteProtectStatus(slot=%d, protected=%d)\r\n",
                SlotNumber, IsWriteProtected())
                );
            
            pInterface->WriteProtected = IsWriteProtected();

#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (TEXT(">>>>> SDHCSlotOptionHandlerImpl:SDHCDGetWriteProtectStatus - WP=%d\r\n"), 
        pInterface->WriteProtected));
#endif

        }
        break;

        case SDHCDQueryBlockCapability:
        {
            PSD_HOST_BLOCK_CAPABILITY pBlockCaps = (PSD_HOST_BLOCK_CAPABILITY)pData;

            DEBUGMSG(SDCARD_ZONE_INFO,
                (L"SHCSDSlotOptionHandler-SDHCDQueryBlockCapability(slot=%d, "
                 L"readlength=%d, readblocks=%d, writelength=%d, writeblocks=%d)\r\n",
                 SlotNumber, 
            pBlockCaps->ReadBlockSize,
                 pBlockCaps->ReadBlocks,
            pBlockCaps->WriteBlockSize,
                 pBlockCaps->WriteBlocks)
                );            

            if (pBlockCaps->ReadBlockSize < STD_HC_MIN_BLOCK_LENGTH) 
            {
                pBlockCaps->ReadBlockSize = STD_HC_MIN_BLOCK_LENGTH;
            }

            if (pBlockCaps->ReadBlockSize > m_usMaxBlockLen) 
            {
                pBlockCaps->ReadBlockSize = m_usMaxBlockLen;
            }

            if (pBlockCaps->WriteBlockSize < STD_HC_MIN_BLOCK_LENGTH) 
            {
                pBlockCaps->WriteBlockSize = STD_HC_MIN_BLOCK_LENGTH;
            }

            if (pBlockCaps->WriteBlockSize > m_usMaxBlockLen) 
            {
                pBlockCaps->WriteBlockSize = m_usMaxBlockLen;
            }
        }
        break;

        case SDHCDGetSlotInfo:
        {
            DWORD dwSlotCapabilities = SD_SLOT_SD_1BIT_CAPABLE | SD_SLOT_SDIO_CAPABLE;
            PSDCARD_HC_SLOT_INFO pSlotInfo = (PSDCARD_HC_SLOT_INFO)pData;

            // validate parameters
            if( OptionSize != sizeof(SDCARD_HC_SLOT_INFO) || pData == NULL )
            {
                status = SD_API_STATUS_INVALID_PARAMETER;
                break;                  
            }

            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDGetSlotInfo(slot=%d)\r\n",
                 SlotNumber)
                ); 

            // SDIO 1 bit or 4 bit.
            if (!m_Sdio4BitDisable)
                dwSlotCapabilities |= SD_SLOT_SD_4BIT_CAPABLE;
  
            if (!m_SdMem4BitDisable)
                dwSlotCapabilities |= SD_SLOT_SDMEM_4BIT_CAPABLE;
  
            if (m_dwMMC8BitMode != FALSE)
               dwSlotCapabilities |= SD_SLOT_MMC_8BIT_CAPABLE;

			if (m_dwSDHighSpeedSupport)
                  dwSlotCapabilities |= SD_SLOT_HIGH_SPEED_CAPABLE;
  
            if (!IsMultipleBlockReadSupported())
                dwSlotCapabilities |= SD_SLOT_USE_SOFT_BLOCK_CMD53_READ;
  
            SDHCDSetSlotCapabilities( pSlotInfo,dwSlotCapabilities);
  
            SDHCDSetVoltageWindowMask(pSlotInfo,(SD_VDD_WINDOW_3_1_TO_3_2 |
                                                 SD_VDD_WINDOW_3_2_TO_3_3 |
                                                 SD_VDD_WINDOW_3_3_TO_3_4 |
                                                 SD_VDD_WINDOW_3_4_TO_3_5 |
                                                 SD_VDD_WINDOW_3_5_TO_3_6));
            // Set optimal voltage
            SDHCDSetDesiredSlotVoltage(pSlotInfo, SD_VDD_WINDOW_2_9_TO_3_0);
  
            // Set maximum supported clock rate
            SDHCDSetMaxClockRate(pSlotInfo, m_dwMaxClockRate);
  
            // set power up delay
            SDHCDSetPowerUpDelay(pSlotInfo, 100);
   
            DEBUGMSG(SDCARD_ZONE_INFO, 
                (L"SHCSDSlotOptionHandler-SDHCDGetSlotInfo[out](slot=%d, "
                 L"Capabilities=%d, VoltageWindowMask=0x%08X, "
                 L"DesiredVoltageMask=0x%08X, MaxClockRate=0x%08X, "
                 L"PowerUpDelay=%d)\r\n",
                 SlotNumber, 
                 pSlotInfo->Capabilities,
                 pSlotInfo->VoltageWindowMask,
                 pSlotInfo->DesiredVoltageMask,
                 pSlotInfo->MaxClockRate,
                 pSlotInfo->PowerUpDelay)
                ); 
        }
        break;

        default:
            status = SD_API_STATUS_INVALID_PARAMETER;
        break;
    }

    SDHCDReleaseHCLock(m_pHCContext);
    return status;
}

//-----------------------------------------------------------------------------
BOOLEAN CSDIOControllerBase::SDHCCancelIoHandler(
    PSDCARD_HC_CONTEXT pHCContext,
    DWORD Slot,
    PSD_BUS_REQUEST pRequest )
{
    // get our extension
    CSDIOControllerBase *pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    return pController->SDHCCancelIoHandlerImpl((UCHAR)Slot, pRequest);
}

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCBusRequestHandler(
    PSDCARD_HC_CONTEXT pHCContext,
    DWORD Slot,
    PSD_BUS_REQUEST pRequest )
{
    UNREFERENCED_PARAMETER(Slot);

    // get our extension
    CSDIOControllerBase *pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    return pController->SDHCBusRequestHandlerImpl(pRequest);
}

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCSlotOptionHandler(
    PSDCARD_HC_CONTEXT pHCContext,
    DWORD SlotNumber,
    SD_SLOT_OPTION_CODE Option,
    PVOID pData,
    ULONG OptionSize )
{
    // get our extension
    CSDIOControllerBase *pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    return pController->SDHCSlotOptionHandlerImpl((UCHAR)SlotNumber,
        Option,
        pData,
        OptionSize );
}

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCDeinitialize(
    PSDCARD_HC_CONTEXT pHCContext )
{
    // get our extension
    CSDIOControllerBase *pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    return pController->SDHCDeinitializeImpl();
}

//-----------------------------------------------------------------------------
SD_API_STATUS CSDIOControllerBase::SDHCInitialize(
    PSDCARD_HC_CONTEXT pHCContext )
{
    // get our extension
    CSDIOControllerBase *pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    return pController->SDHCInitializeImpl();
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SetPower(CEDEVICE_POWER_STATE dx)
{
    BOOL rc = FALSE;

    EnterCriticalSection(&m_powerCS);
    if(m_ActualPowerState != dx)
	{ 
	    switch(dx)
        {
          case D0:
    		EnableDeviceClocks(m_dwDeviceID, TRUE);
    		// Notify the SD Bus driver of the PowerUp event
            SDHCDPowerUpDown(m_pHCContext, TRUE, FALSE, 0);
            if(!m_fCardPresent)
                SetEvent(m_hCardDetectEvent);
    
            rc = TRUE;
            break;
    
          case D4:
            // Notify the SD Bus driver of the PowerDown event
            SDHCDPowerUpDown(m_pHCContext, FALSE, FALSE, 0);
    		EnableDeviceClocks(m_dwDeviceID, FALSE);

            rc = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&m_powerCS);
    return rc;
}

//-----------------------------------------------------------------------------
void CSDIOControllerBase::PowerDown()
{
    // Notify bus driver
    IndicateSlotStateChange(DeviceEjected);

    SystemClockOn(TRUE);

    // Simulate Device removal for Suspend/Resume
    m_fCardPresent = FALSE;
    // get the current request
    PSD_BUS_REQUEST pRequest = GetAndLockCurrentRequest();

    if (pRequest != NULL)
    {
        DEBUGMSG(SDCARD_ZONE_WARN,
            (TEXT("PowerDown - Canceling current request: 0x%08X, command: %d\n"),
             pRequest, pRequest->CommandCode));
        DumpRequest(pRequest);
        IndicateBusRequestComplete(pRequest, SD_API_STATUS_DEVICE_REMOVED);
    }

    if (m_dwSDClockMode)
    {
        // turn clock off and remove power from the slot
        OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_AUTOIDLE | MMCHS_SYSCONFIG_SIDLEMODE(SIDLE_FORCE));
    }

    SetPower(D4);

    // go to D4 right away
    ResetEvent(m_hTimerEvent);
    m_dwClockCnt = 0;
    m_bDisablePower = TRUE;
    m_InternPowerState = D4;
    UpdateDevicePowerState(TRUE);
}

//-----------------------------------------------------------------------------
void CSDIOControllerBase::PowerUp()
{
}

//------------------------------------------------------------------------------
//
//  Function:  ContextRestore
//
//  This function restores the context.
//
BOOL 
CSDIOControllerBase::ContextRestore()
{
    SD_API_STATUS      status;              // SD status
    BOOL               fRegisteredWithBusDriver = FALSE;
    BOOL               fHardwareInitialized = FALSE;
    BOOL               bRet = FALSE;
    DWORD              dwClockRate;

    DEBUGMSG(SHC_RESPONSE_ZONE, (L"CSDIOControllerBase:+ContextRestore\r\n"));
	
	if( !InitializeHardware() )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("InitializeHardware:: Error allocating CD/RW GPIO registers\r\n")));
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
        goto cleanUp;
    }

    // Perform SOC-specific configuration
    SocSdhcDevconf(m_dwSlot);

    fHardwareInitialized = TRUE;

    // Initialize the slot
    SoftwareReset(SOFT_RESET_ALL);

	// Allow time for card to power down after a device reset
    StallExecution(10000);

    dwClockRate = m_sContext.dwClockRate;
    
    SetCDPolarity();
    SetSDVSVoltage();
    SetSDInterfaceMode(m_sContext.eInterfaceMode);
    SetClockRate(&dwClockRate);

    switch (m_sContext.eSDHCIntr)
    {
        case SDHC_MMC_INTR_ENABLED:
            EnableSDHCInterrupts();
            break;

        case SDHC_SDIO_INTR_ENABLED:
            EnableSDIOInterrupts();
            break;
    }

    fRegisteredWithBusDriver = TRUE;

    bRet = TRUE;

cleanUp:
    if ( (bRet == FALSE) && (m_pHCContext) )
    {
        FreeHostContext( fRegisteredWithBusDriver, fHardwareInitialized );
    }
    DEBUGMSG(SHC_RESPONSE_ZONE, (L"CSDIOControllerBase:-ContextRestore\r\n"));

    return bRet;
}

//-----------------------------------------------------------------------------
//Function:     GetCommandResponse()
//Description:  Retrieves the response info for the last SDI command
//              issues.
//Notes:
//Returns:      SD_API_STATUS status code.
SD_API_STATUS CSDIOControllerBase::GetCommandResponse(PSD_BUS_REQUEST pRequest)
{
    DWORD  dwRegVal;
    PUCHAR  respBuff;       // response buffer
    DWORD dwRSP;

    dwRegVal = Read_MMC_STAT();

    DEBUGMSG(SHC_RESPONSE_ZONE, (TEXT("GetCommandResponse() - MMC_STAT = 0x%08X.\r\n"), dwRegVal));

#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (TEXT("GetCommandResponse: Cmd=%d, MMC_STAT = 0x%08X.\r\n"), pRequest->CommandCode, dwRegVal));
#endif

    if ( dwRegVal & (MMCHS_STAT_CC | MMCHS_STAT_CERR | MMCHS_STAT_CCRC))
    {
        respBuff = pRequest->CommandResponse.ResponseBuffer;

        switch(pRequest->CommandResponse.ResponseType)
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

            DEBUGMSG(SHC_RESPONSE_ZONE, (TEXT("GetCommandResponse() - R1 R1b : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \r\n"), *(respBuff + 0),
                *(respBuff + 1), *(respBuff + 2), *(respBuff + 3), *(respBuff + 4), *(respBuff + 5)));
            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG (1,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#endif
            break;

        case ResponseR3:
        case ResponseR4:
        case ResponseR7:
            DEBUGMSG(SHC_RESPONSE_ZONE, (TEXT("ResponseR3 ResponseR4\r\n")));
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

            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG (1,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#endif
            break;

        case ResponseR5:
        case ResponseR6:
            DEBUGMSG(SHC_RESPONSE_ZONE, (TEXT("ResponseR5 ResponseR6\r\n")));
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

            DEBUGMSG(SHC_RESPONSE_ZONE, (TEXT("GetCommandResponse() - R5 R6 : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n"), *(respBuff + 0),
                *(respBuff + 1), *(respBuff + 2), *(respBuff + 3), *(respBuff + 4), *(respBuff + 5)));

            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG (1,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5] ));
#endif
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

            DEBUGMSG (SHC_SDBUS_INTERACT_ZONE,(TEXT("GetCmdResponse returned [%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5], respBuff[6], respBuff[7],
                respBuff[8], respBuff[9], respBuff[10], respBuff[11], respBuff[12], respBuff[13], respBuff[14], respBuff[15]));

#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG (1, (TEXT("GetCmdResponse returned [%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x]\r\n"),
                respBuff[0], respBuff[1], respBuff[2], respBuff[3], respBuff[4], respBuff[5], respBuff[6], respBuff[7],
                respBuff[8], respBuff[9], respBuff[10], respBuff[11], respBuff[12], respBuff[13], respBuff[14], respBuff[15]));
#endif
            break;

        default:
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("GetCommandResponse() - Unrecognized response type!\r\n")));
            break;
        }
    }
    return SD_API_STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SDIReceive(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode)
{
    if( m_fDMATransfer && m_DmaEnable)
    {
        return SDIDMAReceive( pBuff, dwLen, FastPathMode );
    }
    else
    {
        return SDIPollingReceive( pBuff, dwLen );
    }
}


//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SDITransmit(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode)
{
    if( m_fDMATransfer && m_DmaEnable)
    {
        return SDIDMATransmit( pBuff, dwLen, FastPathMode );
    }
    else
    {
        return SDIPollingTransmit( pBuff, dwLen );
    }
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SDIDMAReceive(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode)
{
    DWORD dwCountStart;
    DWORD __unaligned *pbuf2 = (DWORD *) pBuff;

    DWORD MMC_STAT = Read_MMC_STAT(); 

    UNREFERENCED_PARAMETER(FastPathMode);

    if( MMC_STAT & MMCHS_STAT_DTO )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DTO error.\n")));
        goto cleanUp;
    }
    if( MMC_STAT & MMCHS_STAT_DCRC ) // DATA CRC Error
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_CRC_ERROR_RCVD error.\n")));
        goto cleanUp;
    }

    // Clear interrupt status
        EnterCriticalSection( &m_critSec );
        SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_TC);
        CLRREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_TC);
        OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
        LeaveCriticalSection( &m_critSec );

    SDIO_StartInputDMA();

    StallExecution(1);

    // calculate timeout conditions
    dwCountStart = GetTickCount();

    // wait for the SDIO/controller interrupt
    for(;;)
    {
        // check for a timeout
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: TIMEOUT.\n")));
            // stop DMA
            SDIO_StopInputDMA();
            goto cleanUp;
        }

           DWORD retries = 0;
           // polling end-of-command
           while (!(Read_MMC_STAT() & MMCHS_STAT_TC)) {
               if (retries > SDIO_MAX_LOOP) {
                   RETAILMSG(1, (TEXT("A.S. Timeout while polling DMA Receive complete.\n")));
                   break;      
               }
               retries++;
           }

        if(!m_fCardPresent)
        {
#ifdef ENABLE_RETAIL_OUTPUT
           RETAILMSG(1, (L"Card has been Removed stopping Input DMA\r\n"));
#endif
           break;
        }

        MMC_STAT = Read_MMC_STAT(); 
        if(MMC_STAT & MMCHS_STAT_BRR)
        {
            break;
        }
        if(MMC_STAT & MMCHS_STAT_TC)
        {
            break;
        }
        if( MMC_STAT & MMCHS_STAT_DTO )
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
            // stop DMA
            SDIO_StopInputDMA();
            goto cleanUp;
        }
        if( MMC_STAT & MMCHS_STAT_DCRC ) // DATA CRC Error
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMAReceive() - exit: STAT register indicates MMC_STAT_DCRC error.\r\n")));
            // stop DMA
            SDIO_StopInputDMA();
            goto cleanUp;
        }

    }

    MMC_STAT = Read_MMC_STAT(); 

    // stop DMA
    SDIO_StopInputDMA();

    Set_MMC_STAT(MMCHS_STAT_TC);

    // finally, copy the data from DMA buffer to the user buffer, which maybe SG buffer, 
    if( !SDPerformSafeCopy( pbuf2, m_pDmaBuffer, dwLen ) )
    {
        goto cleanUp;
    }

#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"SDIDMAReceive(dwLen 0x%x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x)+\r\n", 
      dwLen, pbuf2[0], pbuf2[1], pbuf2[2], pbuf2[3], pbuf2[4], pbuf2[5], pbuf2[6], pbuf2[7])); 
#endif      
    return TRUE;  

cleanUp:
#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"SDIDMAReceive(%08X, %08X, %08X)-\r\n", INREG32(&m_pbRegisters->MMCHS_STAT), INREG32(&m_pbRegisters->MMCHS_PSTATE), INREG32(&m_pbRegisters->MMCHS_DATA)));
#endif
    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SDIDMATransmit(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode)
{
    DWORD dwCountStart;
    DWORD __unaligned *pbuf2 = (DWORD *) pBuff;

    UNREFERENCED_PARAMETER(FastPathMode);

    // first copy the data to the DMA buffer, then to the user buffer, which maybe SG buffer,
    if( !SDPerformSafeCopy( m_pDmaBuffer, pbuf2, dwLen ) )
    {
        goto cleanUp;
    }

    DWORD MMC_STAT = Read_MMC_STAT(); 
#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"SDIDMATransmit(dwLen 0x%x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x)+\r\n", 
      dwLen, pbuf2[0], pbuf2[1], pbuf2[2], pbuf2[3], pbuf2[4], pbuf2[5], pbuf2[6], pbuf2[7])); 
#endif      
    if( MMC_STAT & MMCHS_STAT_DTO )
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
        goto cleanUp;
    }
    if( MMC_STAT & MMCHS_STAT_DCRC ) // DATA CRC Error
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_STAT_DCRC error.\r\n")));
        goto cleanUp;
    }

    // Clear interrupt status
        EnterCriticalSection( &m_critSec );
        SETREG32(&m_pbRegisters->MMCHS_IE, MMCHS_IE_TC);
        CLRREG32(&m_pbRegisters->MMCHS_ISE, MMCHS_ISE_TC);
        OUTREG32(&m_pbRegisters->MMCHS_STAT, 0xFFFFFFFF);
        LeaveCriticalSection( &m_critSec );

    SDIO_StartOutputDMA();

    StallExecution(1);

    // calculate timeout conditions
    dwCountStart = GetTickCount();

    // wait for the SDIO/controller interrupt
    for(;;)
    {
        // check for a timeout
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIDMATransmit() - exit: TIMEOUT.\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG(1, (TEXT("SDIDMATransmit() - exit: TIMEOUT.\r\n")));
#endif            
            goto cleanUp;
        }

            DWORD retries = 0;
            // polling end-of-command
            while (!(Read_MMC_STAT() & MMCHS_STAT_TC)) {
                if (retries > SDIO_MAX_LOOP) {
                    RETAILMSG(1, (TEXT("A.S. Timeout while polling DMA Transmit complete.\r\n")));
                    break;      
                }
                retries++;
            }


        if(!m_fCardPresent)
        {
#ifdef ENABLE_RETAIL_OUTPUT
           RETAILMSG(1, (L"Card has been Removed stopping Output DMA\r\n"));
#endif
           break;
        }

        MMC_STAT = Read_MMC_STAT(); 
        if(MMC_STAT & MMCHS_STAT_TC)
        {
            break;
        }
        if( MMC_STAT & MMCHS_STAT_DTO )
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG(1, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_STAT_DTO error.\r\n")));
#endif            
            // stop DMA
            SDIO_StopInputDMA();
            goto cleanUp;
        }
        if( MMC_STAT & MMCHS_STAT_DCRC ) // DATA CRC Error
        {
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_CRC_ERROR_RCVD error.\r\n")));
#ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG(1, (TEXT("SDIDMATransmit() - exit: STAT register indicates MMC_CRC_ERROR_RCVD error.\r\n")));
#endif
            // stop DMA
            SDIO_StopInputDMA();
            goto cleanUp;
        }
    }
    MMC_STAT = Read_MMC_STAT();

    // stop DMA
    SDIO_StopOutputDMA();
    Set_MMC_STAT(MMCHS_STAT_TC);

    return TRUE;

cleanUp:
#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"CSDIOControllerBase::SDIDMATransmit(%08X, %08X)-\r\n", INREG32(&m_pbRegisters->MMCHS_STAT), INREG32(&m_pbRegisters->MMCHS_PSTATE)));
#endif
    return FALSE;
}


//-----------------------------------------------------------------------------
//Function:     SDIPollingReceive()
//Description:
//Notes:        This routine assumes that the caller has already locked
//              the current request and checked for errors.
//Returns:      SD_API_STATUS status code.
BOOL CSDIOControllerBase::SDIPollingReceive(PBYTE pBuff, DWORD dwLen)
{
    DWORD fifoSizeW, blockLengthW; // Almost Full level and block length
    DWORD dwCount1, dwCount2;

    DWORD *pbuf = (DWORD *) pBuff;
    DWORD __unaligned *pbuf2 = (DWORD *) pBuff;
    DWORD dwCountStart;

    DEBUGMSG(SHC_RECEIVE_ZONE, (TEXT("R(0x%x)\r\n"), dwLen));
    //check the parameters

    DWORD MMC_STAT = Read_MMC_STAT();
    INREG32(&m_pbRegisters->MMCHS_PSTATE);

    // calculate timeout conditions
    dwCountStart = GetTickCount();


    if(dwLen % MMC_BLOCK_SIZE || m_dwSDIOCard)
    {
        while ((Read_MMC_STAT() & MMCHS_STAT_BRR) != MMCHS_STAT_BRR)
        {
            // check for a timeout
            if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
            {
                DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingReceive() - exit: TIMEOUT.\r\n")));
                goto READ_ERROR;
            }
        }

        Set_MMC_STAT(MMCHS_STAT_BRR);
        fifoSizeW = dwLen / sizeof(DWORD);
        if(dwLen % sizeof(DWORD)) fifoSizeW++;
        for (dwCount2 = 0; dwCount2 < fifoSizeW; dwCount2++)
        {
               *pbuf2 = INREG32(&m_pbRegisters->MMCHS_DATA) ;
               pbuf2++;
        }
    } 
    else
    {
        fifoSizeW = INREG32(&m_pbRegisters->MMCHS_BLK) & 0xFFFF;
        blockLengthW = dwLen / fifoSizeW;
        for (dwCount1 = 0; dwCount1 < blockLengthW; dwCount1++)
        {
            // Wait for Block ready for read
            while((Read_MMC_STAT() & MMCHS_STAT_BRR) != MMCHS_STAT_BRR)
            {
                // check for a timeout
                if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
                {
                    DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingReceive() - exit: TIMEOUT.\r\n")));
                    goto READ_ERROR;
                }
            }
            Set_MMC_STAT(MMCHS_STAT_BRR);
    
            // Get all data from DATA register and write in user buffer
            for (dwCount2 = 0; dwCount2 < (fifoSizeW / sizeof(DWORD)); dwCount2++)
            {
                *pbuf = INREG32(&m_pbRegisters->MMCHS_DATA) ;
                pbuf++;
            }
        }
    }
    // recalculate timeout conditions
    dwCountStart = GetTickCount();

    while (((Read_MMC_STAT()&MMCHS_STAT_TC) != MMCHS_STAT_TC))
    {
        // check for a timeout
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingReceive() - exit: TIMEOUT.\r\n")));
            goto READ_ERROR;
        }
    }

    Set_MMC_STAT(MMCHS_STAT_TC);
    // Check if there is no CRC error
    if (!(Read_MMC_STAT() & MMCHS_STAT_DCRC))
    {
        MMC_STAT = Read_MMC_STAT();
        Write_MMC_STAT(MMC_STAT);
        return TRUE;
    }
    else
    {
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::SDIPollingReceive(ERROR:%08X)\r\n", INREG32(&m_pbRegisters->MMCHS_STAT)));
#endif
        MMC_STAT = Read_MMC_STAT();
        Write_MMC_STAT(MMC_STAT);
        return FALSE;
    }

READ_ERROR:

#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"CSDIOControllerBase::SDIPollingReceive(BUSY:%08X)\r\n", INREG32(&m_pbRegisters->MMCHS_STAT)));
#endif
    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CSDIOControllerBase::SDIPollingTransmit(PBYTE pBuff, DWORD dwLen)
{
    DWORD fifoSizeW, blockLengthW; // Almost Full level and block length
    DWORD dwCount1, dwCount2;
    DWORD *pbuf = (DWORD *) pBuff; // short* of buffer
    DWORD __unaligned *pbuf2 = (DWORD *) pBuff;
#ifdef ENABLE_RETAIL_OUTPUT
    UINT16 __unaligned *pbuf3 = (UINT16 *) pBuff;
#endif
    DWORD dwCountStart;

#ifdef ENABLE_RETAIL_OUTPUT
    if (dwLen == 2)
        RETAILMSG(1, (TEXT("SDIPollingTransmit([%d: %04x])\n"), dwLen, pbuf3[0]));
    else
        RETAILMSG(1, (TEXT("SDIPollingTransmit([%d: %08x,%08x])\n"), dwLen, pbuf2[0], pbuf2[1]));
#endif

    // calculate timeout conditions
    dwCountStart = GetTickCount();

    if(dwLen % MMC_BLOCK_SIZE || m_dwSDIOCard)
    {
        while((Read_MMC_STAT() & MMCHS_STAT_BWR) != MMCHS_STAT_BWR)
        {
            if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
            {
                DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingTransmit() - exit: TIMEOUT.\r\n")));
                goto WRITE_ERROR;
            }
        }
        Set_MMC_STAT(MMCHS_STAT_BWR);

        fifoSizeW = dwLen / sizeof(DWORD);
        if(dwLen % sizeof(DWORD)) fifoSizeW++;
        for (dwCount1 = 0; dwCount1 < fifoSizeW; dwCount1++)
        {
            OUTREG32(&m_pbRegisters->MMCHS_DATA, *pbuf2++) ;
        }
    } 
    else
    {
        fifoSizeW = INREG32(&m_pbRegisters->MMCHS_BLK) & 0xFFFF;
        blockLengthW = dwLen / fifoSizeW;
        for (dwCount1 = 0; dwCount1 < blockLengthW; dwCount1++)
        {
            // poll on write ready here
            while((Read_MMC_STAT() & MMCHS_STAT_BWR) != MMCHS_STAT_BWR)
            {
                if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
                {
                    DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingTransmit() - exit: TIMEOUT.\r\n")));
                    goto WRITE_ERROR;
                }
            }
            Set_MMC_STAT(MMCHS_STAT_BWR);
  
            for (dwCount2 = 0; dwCount2 < (fifoSizeW /sizeof(DWORD)); dwCount2++) // write data to DATA buffer
            {
                OUTREG32(&m_pbRegisters->MMCHS_DATA, *pbuf++);
            }
        }
    }

    // recalculate timeout conditions
    dwCountStart = GetTickCount();

    while (((Read_MMC_STAT()&MMCHS_STAT_TC) != MMCHS_STAT_TC))
    {
        // check for a timeout
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("SDIPollingTransmit() - exit: TIMEOUT.\r\n")));
            goto WRITE_ERROR;
        }
    }

    // Check if there is no CRC error
    if (!(Read_MMC_STAT() & MMCHS_STAT_DCRC))
    {
        return TRUE;
    }
    else
    {
#ifdef ENABLE_RETAIL_OUTPUT
        RETAILMSG(1, (L"CSDIOControllerBase::SDIPollingTransmit(ERROR:%08X)\r\n", INREG32(&m_pbRegisters->MMCHS_STAT)));
#endif
        return FALSE;
    }

WRITE_ERROR:
#ifdef ENABLE_RETAIL_OUTPUT
    RETAILMSG(1, (L"CSDIOControllerBase::SDIPollingTransmit(BUSY:%08X)\r\n", INREG32(&m_pbRegisters->MMCHS_STAT)));
#endif
    return FALSE;
}


#ifdef SDIO_PRINT_THREAD

DWORD WINAPI CSDIOControllerBase::SDHCPrintThread(LPVOID lpParameter)
{
    CSDIOControllerBase *pController = (CSDIOControllerBase*)lpParameter;
    return pController->SDHCPrintThreadImpl();
}


//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//  SDHCPrintThread - SDIO print thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSDIOControllerBase::SDHCPrintThreadImpl()
{
    DWORD dwEventVal;

    while (TRUE)
    {
        WaitForSingleObject(m_hPrintEvent, INFINITE);

        while (m_cmdRdIndex != m_cmdWrIndex)
        {

            dwEventVal = m_cmdArray[m_cmdRdIndex];

            if (dwEventVal & SD_FAST_PATH_AVAILABLE)
            {
                RETAILMSG(1, (TEXT("SD_FAST_PATH - CMD::[%d]\r\n"),
                dwEventVal & 0xFFFF));
            }
            else
            {
                RETAILMSG(1, (TEXT("no SD_FAST_PATH - CMD::[%d]\r\n"),
                dwEventVal));
            }

            m_cmdRdIndex++;
            if (m_cmdRdIndex >= m_cmdArrSize)
                m_cmdRdIndex = 0;

        }
    }

    return 1;
}
#endif

#ifdef DEBUG

//-----------------------------------------------------------------------------
// Reads from SD Standard Host registers and writes them to the debugger.
VOID CSDIOControllerBase::DumpRegisters()
{
    EnterCriticalSection( &m_critSec );
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("+DumpStdHCRegs-------------------------\r\n")));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_CMD 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CMD)    ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_ARG 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_ARG)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_CON  0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CON)   ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_PWCNT  0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_PWCNT)   ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_STAT 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_STAT)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_PSTATE 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_PSTATE)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_IE 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_IE)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_ISE 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_ISE)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_BLK 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_BLK)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_REV 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_REV)    ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_RSP10 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP10)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_RSP32 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP32)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_RSP54 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP54)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_RSP76 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_RSP76)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_HCTL 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_HCTL)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_SYSCTL 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_SYSCTL)  ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_SYSCONFIG 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_SYSCONFIG) ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_CAPA 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CAPA) ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("MMCHS_CUR_CAPA 0x%08X \r\n"), INREG32(&m_pbRegisters->MMCHS_CUR_CAPA) ));
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("-DumpStdHCRegs-------------------------\r\n")));
    LeaveCriticalSection( &m_critSec );
}

#endif
//-----------------------------------------------------------------------------
