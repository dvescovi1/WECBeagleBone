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
//
//  File: SDHC.H
//
//  SDHC driver definitions
//

#ifndef __SDHC_H
#define __SDHC_H

#pragma warning(push)
#pragma warning(disable: 4100 4127 4189 4201 4214 6001 6385)
#include <windows.h>
#include <pm.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <devload.h>
#include <SDCardDDK.h>
#include <SDHCD.h>
#include <creg.hxx>
#include <Nkintr.h>
#pragma warning(pop)

#include "omap.h"
#include "omap_clocks.h"
#include "oal_clock.h"

#include "dma_utility.h"
#include "bsp_cfg.h"
#include "soc_cfg.h"
#include "omap_prcm_regs.h"
#include "am33x_sdhc_regs.h"

#define CB_DMA_BUFFER 0x20000       // 128KB buffer

#define MMC_BLOCK_SIZE     0x200
#define MIN_MMC_BLOCK_SIZE 4
#define SD_IO_BUS_CONTROL_BUS_WIDTH_MASK 0x03

#ifndef SHIP_BUILD
#define STR_MODULE _T("SDHC!")
#define SETFNAME(name) LPCTSTR pszFname = STR_MODULE name _T(":")
#else
#define SETFNAME(name)
#endif

#define EXT_MMCHS_STAT_CD_INSERT_INTR   MMCHS_STAT_CINS
#define EXT_MMCHS_STAT_CD_REMOVE_INTR   MMCHS_STAT_CREM
#define EXT_MMCHS_STAT_CD_INTR          (EXT_MMCHS_STAT_CD_INSERT_INTR  | EXT_MMCHS_STAT_CD_REMOVE_INTR)

typedef enum {
    CARD_REMOVED_STATE      = 0,
    COMMAND_TRANSFER_STATE,
    DATA_RECEIVE_STATE,
    DATA_TRANSMIT_STATE,
    CARDBUSY_STATE
} SDHCCONTROLLERIST_STATE, *PSDHCCONTROLLERIST_STATE;

typedef enum {
    SDHC_INTR_DISABLED      = 0,
    SDHC_MMC_INTR_ENABLED,
    SDHC_SDIO_INTR_ENABLED
} SDHCINTRENABLE, *PSDHCINTRENABLE;

typedef struct {
    DWORD               dwClockRate;
    SDHCINTRENABLE      eSDHCIntr;
    SD_INTERFACE_MODE   eInterfaceMode;
} SDHC_CONTROLLER_CONTEXT, *PSDHC_CONTROLLER_CONTEXT;

// SDHC hardware specific context
class CSDIOControllerBase
{
public:

    CSDIOControllerBase();
    ~CSDIOControllerBase();

    BOOL Init( LPCTSTR pszActiveKey );
    VOID FreeHostContext( BOOL fRegisteredWithBusDriver, BOOL fHardwareInitialized );
    VOID PowerDown();
    VOID PowerUp();
    // callback handlers
    static SD_API_STATUS SDHCDeinitialize(PSDCARD_HC_CONTEXT pHCContext);
    static SD_API_STATUS SDHCInitialize(PSDCARD_HC_CONTEXT pHCContext);
    static BOOLEAN SDHCCancelIoHandler( PSDCARD_HC_CONTEXT pHCContext,
                                        DWORD Slot,
                                        PSD_BUS_REQUEST pRequest );
    static SD_API_STATUS SDHCBusRequestHandler( PSDCARD_HC_CONTEXT pHCContext,
                                                DWORD Slot,
                                                PSD_BUS_REQUEST pRequest );
    static SD_API_STATUS SDHCSlotOptionHandler( PSDCARD_HC_CONTEXT pHCContext,
                                                DWORD SlotNumber,
                                                SD_SLOT_OPTION_CODE Option,
                                                PVOID pData,
                                                ULONG OptionSize );

    // SD driver callbacks implementation
    SD_API_STATUS SDHCInitializeImpl();
    SD_API_STATUS SDHCDeinitializeImpl();
    BOOLEAN SDHCCancelIoHandlerImpl( UCHAR Slot, PSD_BUS_REQUEST pRequest );
    SD_API_STATUS SDHCBusRequestHandlerImpl( PSD_BUS_REQUEST pRequest );
    SD_API_STATUS SDHCSlotOptionHandlerImpl( UCHAR SlotNumber,
                                             SD_SLOT_OPTION_CODE Option,
                                             PVOID pData,
                                             ULONG OptionSize );


    // platform specific functions
    virtual BOOL    InitializeHardware() = 0;
    virtual void    DeinitializeHardware() = 0;
    virtual BOOL    IsWriteProtected() = 0;
    virtual BOOL    SDCardDetect() = 0;
    virtual DWORD   SDHCCardDetectIstThreadImpl() = 0;
    virtual VOID    TurnCardPowerOn() = 0;
    virtual VOID    TurnCardPowerOff() = 0;
    virtual VOID    PreparePowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler) = 0;
    virtual VOID    PostPowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler) = 0;
    SD_API_STATUS   CSDIOControllerBase::SDHCBusRequestHandlerImpl_FastPath(PSD_BUS_REQUEST pRequest);
    SD_API_STATUS   CSDIOControllerBase::SDHCBusRequestHandlerImpl_NormalPath(PSD_BUS_REQUEST pRequest);

    // helper functions
    virtual BOOL    InterpretCapabilities();
    VOID    SetInterface(PSD_CARD_INTERFACE_EX pInterface);
    VOID    EnableSDHCInterrupts();
    VOID    DisableSDHCInterrupts();
    VOID    EnableSDIOInterrupts();
    VOID    AckSDIOInterrupt();
    VOID    DisableSDIOInterrupts();
    SD_API_STATUS SendCmdNoResp (DWORD cmd, DWORD arg);
    VOID    SendInitSequence();
    SD_API_STATUS SendCommand(PSD_BUS_REQUEST pRequest);
    SD_API_STATUS GetCommandResponse(PSD_BUS_REQUEST pRequest);
    SD_API_STATUS    CommandCompleteHandler_FastPath(PSD_BUS_REQUEST pRequest);
    BOOL    SDIReceive(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode);
    BOOL    SDITransmit(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode);
    BOOL    SDIDMAReceive(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode);
    BOOL    SDIDMATransmit(PBYTE pBuff, DWORD dwLen, BOOL FastPathMode);
    BOOL    SDIPollingReceive(PBYTE pBuff, DWORD dwLen);
    BOOL    SDIPollingTransmit(PBYTE pBuff, DWORD dwLen);
    VOID    SetSlotPowerState( CEDEVICE_POWER_STATE state );
    CEDEVICE_POWER_STATE GetSlotPowerState();

    VOID SoftwareReset(DWORD dwResetBits);

    // Interrupt handling methods
    VOID HandleRemoval(BOOL fCancelRequest);
    VOID HandleInsertion();

    VOID CardInterrupt(BOOL bInsert);
    BOOL HandleCardDetectInterrupt(DWORD dwStatus);

    VOID SetSDVSVoltage();
    VOID SetClockRate(PDWORD pdwRate);
    BOOL UpdateSystemClock( BOOL enable );
    VOID SystemClockOn(BOOL bInPowerHandler) {
        m_InternPowerState = D0;
        UpdateDevicePowerState(bInPowerHandler);
    }
    VOID SystemClockOff(BOOL bInPowerHandler) {
        m_InternPowerState = D4;
        UpdateDevicePowerState(bInPowerHandler);
    }
    VOID UpdateDevicePowerState(BOOL bInPowerHandler);
    BOOL SetPower(CEDEVICE_POWER_STATE dx);
    BOOL ContextRestore();

    inline DWORD Read_MMC_STAT();
    inline void Write_MMC_STAT( DWORD wVal );
    inline void Set_MMC_STAT( DWORD wVal );

    // IST functions
    static DWORD WINAPI SDHCControllerIstThread(LPVOID lpParameter);
    static DWORD WINAPI SDHCCardDetectIstThread(LPVOID lpParameter);  //NOT used in AM389X
    static DWORD WINAPI DataTransferIstThread(LPVOID lpParameter);
    DWORD SDHCControllerIstThreadImpl();
    DWORD SDHCPowerTimerThreadImpl();
    static DWORD WINAPI SDHCPowerTimerThread(LPVOID lpParameter);

    const static DWORD        m_cmdArrSize=32;
    DWORD              m_cmdArray[m_cmdArrSize];
    DWORD              m_cmdRdIndex;
    DWORD              m_cmdWrIndex;

    BOOL                 m_DmaEnable;
    DmaDataInfo_t *      m_TxDmaInfo;
    HANDLE               m_hTxDmaChannel;       // TX DMA channel allocated by the DMA lib
    DmaDataInfo_t *      m_RxDmaInfo;
    HANDLE               m_hRxDmaChannel;       // RX DMA channel allocated by the DMA lib

    // DMA functions
    BOOL SDIO_InitDMA(void);
    void SDIO_DeinitDMA(void);
    void SDIO_InitInputDMA(DWORD dwBlkCnt, DWORD dwBlkSize);
    void SDIO_InitOutputDMA(DWORD dwBlkCnt, DWORD dwBlkSize);
    void SDIO_StartInputDMA();
    void SDIO_StartOutputDMA();
    void SDIO_StopInputDMA();
    void SDIO_StopOutputDMA();
    void DumpDMARegs(int inCh);

    SD_API_STATUS CommandTransferCompleteHandler(PSD_BUS_REQUEST pRequest, DWORD dwIntrStatus, PSDHCCONTROLLERIST_STATE pNextState);
    SD_API_STATUS CardBusyCompletedHandler(PSD_BUS_REQUEST pRequest, DWORD dwIntrStatus, PSDHCCONTROLLERIST_STATE pNextState);

    SD_API_STATUS ReceiveHandler(PSD_BUS_REQUEST pRequest, PSDHCCONTROLLERIST_STATE peNextState);
    SD_API_STATUS TransmitHandler(PSD_BUS_REQUEST pRequest, PSDHCCONTROLLERIST_STATE peNextState);

    SD_API_STATUS DataReceiveCompletedHandler(PSD_BUS_REQUEST pRequest, DWORD dwIntrStatus, PSDHCCONTROLLERIST_STATE pNextState);
    SD_API_STATUS DataTransmitCompletedHandler(PSD_BUS_REQUEST pRequest, DWORD dwIntrStatus, PSDHCCONTROLLERIST_STATE pNextState);
    BOOL          StartDMATransmit(PBYTE pBuff, DWORD dwLen);
    BOOL          StartDMAReceive(PBYTE pBuff, DWORD dwLen);

    SD_API_STATUS ProcessCommandTransferStatus(PSD_BUS_REQUEST pRequest, SD_API_STATUS status, DWORD dwStatusOverwrite);
    SD_API_STATUS CheckIntrStatus(DWORD dwIntrStatus, DWORD *pOverwrite);

#ifdef DEBUG
    VOID DumpRegisters();
#else
    VOID DumpRegisters() {}
#endif

    // The following 3 functions are for Netra only.
    // Other platforms should just define a stub function in sdcontroller.cpp.
    // Netra does not use GPIO for card detect.  It is part of the 
    // SD_PSTAT register and hence these functions.
    virtual VOID    EnableSDHCCDInterrupts() = 0;
    virtual VOID    DisableSDHCCDInterrupts() = 0;
    virtual inline  void    Write_MMC_CD_STAT( DWORD wVal ) = 0;

protected:
    VOID SetSDInterfaceMode(SD_INTERFACE_MODE eSDInterfaceMode);
public:
    PSDCARD_HC_CONTEXT   m_pHCContext;                   // the host controller context
    HANDLE               m_hParentBus;                   // bus parent handle

    CRITICAL_SECTION     m_critSec;                      // used to synchronize access to SDIO controller registers
    CRITICAL_SECTION     m_powerCS;                      // used to synchronize access to SDIO controller registers
    BOOL                 m_fSDIOInterruptInService;      // TRUE - an SDIO interrupt has been detected and has
                                                        // not yet been acknowledge by the client
    CEDEVICE_POWER_STATE m_InternPowerState;            // current internal power state.
    CEDEVICE_POWER_STATE m_ActualPowerState;            // actual power state.
    BOOL                 m_bReinsertTheCard;            // force card insertion event
    DWORD                m_dwWakeupSources;             // possible wakeup sources (1 - SDIO, 2 - card insert/removal)
    DWORD                m_dwCurrentWakeupSources;      // current wakeup sources

    BOOL                 m_fCardPresent;                // a card is inserted and initialized
    BOOL                 m_fSDIOInterruptsEnabled;      // TRUE - indicates that SDIO interrupts are enabled

    BOOL                 m_fMMCMode;                    // if true, the controller assumed that the card inserted is MMC
    PBYTE                m_pDmaBuffer;                  // virtual address of DMA buffer
    PHYSICAL_ADDRESS     m_pDmaBufferPhys;              // physical address of DMA buffer
    BOOL                 m_fDMATransfer;                // TRUE - the current request will use DMA for data transfer

    BOOL                 m_fAppCmdMode;                 // if true, the controller is in App Cmd mode
    HANDLE               m_hControllerISTEvent;         // SDIO/controller interrupt event
    HANDLE               m_htControllerIST;             // SDIO/controller interrupt thread
    HANDLE               m_hCardDetectEvent;            // card detect interrupt event
    HANDLE               m_htCardDetectIST;             // card detect interrupt thread

    DWORD                m_dwSDIOPriority;              // SDIO IST priority
    BOOL                 m_fDriverShutdown;             // controller shutdown
    INT                  m_dwControllerSysIntr;         // controller interrupt
    BOOL                 m_fInitialized;                // driver initialized
    BOOL                 m_fCardInitialized;            // Card Initialized
    DWORD                m_dwMaxClockRate;              // host controller's clock base
    USHORT               m_usMaxBlockLen;               // max block length

    DWORD                m_dwMaxTimeout;                // timeout (in miliseconds) for read/write operations
    BOOL                 m_fFirstTime;                  // set to TRUE after a card is inserted to
                                                        // indicate that 80 clock cycles initialization
                                                        // must be done when the first command is issued
    BOOL                 m_fPowerDownChangedPower;      // did _PowerDown change the power state?

    AM33X_MMCHS_REGS    *m_pbRegisters;                 // SDHC controller registers

    DWORD                m_dwSlot;
	DWORD				 m_dwDeviceID;
    DWORD                m_dwSDIOCard;
    DWORD                m_fastPathSDIO;
    DWORD                m_fastPathSDMEM;
    DWORD                m_fastPathReq;
    DWORD                m_LowVoltageSlot;
    DWORD                m_Sdio4BitDisable;
    DWORD                m_SdMem4BitDisable;
    DWORD                m_dwMMC8BitMode;
    DWORD                m_dwSDHighSpeedSupport;
    DWORD                m_dwSDClockMode;

    LONG                 m_dwClockCnt;
    BOOL                 m_bExitThread;
    HANDLE               m_hTimerEvent;
    HANDLE               m_hTimerThreadIST;
    DWORD                m_nNonSDIOActivityTimeout;
    DWORD                m_nSDIOActivityTimeout;
    CRITICAL_SECTION     m_pwrThrdCS;                 // used to synchronize access to m_dwClockCnt
    BOOL                 m_bDisablePower;

    BOOL                 bRxDmaActive;
    BOOL                 bTxDmaActive;
    SD_TRANSFER_CLASS    m_TransferClass;

    VOID*                m_pCurrentRecieveBuffer;
    DWORD                m_dwCurrentRecieveBufferLength;

    DWORD                m_CardDetectInterruptStatus;
    BOOL                 m_bCommandPending;
    DWORD                m_dwDMABufferSize;
    SDHC_CONTROLLER_CONTEXT m_sContext;
};

typedef CSDIOControllerBase *PCSDIOControllerBase;

#define GET_PCONTROLLER_FROM_HCD(pHCDContext) \
    GetExtensionFromHCDContext(PCSDIOControllerBase, pHCDContext)

CSDIOControllerBase *CreateSDIOController();

#define SHC_INTERRUPT_ZONE              SDCARD_ZONE_0
#define SHC_SEND_ZONE                   SDCARD_ZONE_1
#define SHC_RESPONSE_ZONE               SDCARD_ZONE_2
#define SHC_RECEIVE_ZONE                SDCARD_ZONE_3
#define SHC_CLOCK_ZONE                  SDCARD_ZONE_4
#define SHC_TRANSMIT_ZONE               SDCARD_ZONE_5
#define SHC_SDBUS_INTERACT_ZONE         SDCARD_ZONE_6
#define SHC_BUSY_STATE_ZONE             SDCARD_ZONE_7

#define SHC_INTERRUPT_ZONE_ON           ZONE_ENABLE_0
#define SHC_SEND_ZONE_ON                ZONE_ENABLE_1
#define SHC_RESPONSE_ZONE_ON            ZONE_ENABLE_2
#define SHC_RECEIVE_ZONE_ON             ZONE_ENABLE_3
#define SHC_CLOCK_ZONE_ON               ZONE_ENABLE_4
#define SHC_TRANSMIT_ZONE_ON            ZONE_ENABLE_5
#define SHC_SDBUS_INTERACT_ZONE_ON      ZONE_ENABLE_6
#define SHC_BUSY_STATE_ZONE_ON          ZONE_ENABLE_7

#define SHC_CARD_CONTROLLER_PRIORITY    0x97

#define WAKEUP_SDIO                     1
#define WAKEUP_CARD_INSERT_REMOVAL      2

#define DMA_TX                          0
#define DMA_RX                          1
#define TIMERTHREAD_TIMEOUT_NONSDIO     1
#define TIMERTHREAD_TIMEOUT             2000
#define TIMERTHREAD_PRIORITY            252


//In Sandisk's TRM, the expected OCR register value returned from iNand should be 0xC0FF8080
//BIT31 indicates the device is ready, BIT30 indicates the High Capacity, 
//0x00FF8080 means the opertaing voltage range are VDD 2.7-3.6V and VDD 1.7-1.95V
#define SANDISK_EMMC_DEFAULT_OCR_VAL 0x40FF8080

#endif


