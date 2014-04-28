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
//------------------------------------------------------------------------------
//
//  File:  mcbsptypes.h
//
#ifndef __MCBSPTYPES_H__
#define __MCBSPTYPES_H__

#include <oal_io.h>
#include <memtxapi.h>
#include <omap_mcbsp_regs.h>

//------------------------------------------------------------------------------
//  defines
//
#define MAX_WAIT_FOR_RESET          100
#define DMA_SAFETY_LOOP_NUM         MAXLONG
#define MCBSP_TX_ACTIVE             0x1
#define MCBSP_RX_ACTIVE             0x2
#define MAX_CHANNEL_PER_BLOCK       16

#define DEFAULT_THREAD_PRIORITY     130
#define DEFAULT_DMA_BUFFER_SIZE     0x2000

#define MCBSP_DEVICE_COOKIE         'bspD'
#define MCBSP_INSTANCE_COOKIE       'bspI'

#define MAX_FIR_COEFF               128

//------------------------------------------------------------------------------
// McBSP data length
//

typedef enum
{
    kMcBSP_Word_Length_8    = 0,
    kMcBSP_Word_Length_12   = 1,
    kMcBSP_Word_Length_16   = 2,
    kMcBSP_Word_Length_20   = 3,
    kMcBSP_Word_Length_24   = 4,
    kMcBSP_Word_Length_32   = 5
} McBSPWordLength_e;


//------------------------------------------------------------------------------
// determines who/when the transmit Frame sync signal is
// generated
//

typedef enum
{
    kMcBSP_Tx_FS_SRC_External       = 0,     // external
    kMcBSP_Tx_FS_SRC_DSR_XSR_COPY   = 2,     // DSR->XSR data copy
    kMcBSP_Tx_FS_SRC_SRG            = 3      // sample rate generator
} McBSPTxFrameSyncMode_e;


//------------------------------------------------------------------------------
// Defines the clock source for the sample rate generator.
//

typedef enum
{
    kMcBSP_SRG_SRC_CLKS_PIN_RISE = 0,   // SRG clk derived from CLKS pin rising
    kMcBSP_SRG_SRC_CLKS_PIN_FALL = 1,   // SRG clk derived from CLKS pin falling
    kMcBSP_SRG_SRC_CPU_CLK  = 2,        // SRG clk src is ARM peripheral
    kMcBSP_SRG_SRC_CLKR_PIN = 3,        // SRG clk derived from CLKRI pin
    kMcBSP_SRG_SRC_CLKX_PIN = 4         // SRG clk derived from CLKXI pin
} McBSPSRGClkSrc_e;


//------------------------------------------------------------------------------
// Defines the partition modes for mcbsp multichannel in TDM.
//

typedef enum
{
    kMcBSP_2PartitionMode = 0,
    kMcBSP_8PartitionMode
} McBSPPartitionMode_e;


//------------------------------------------------------------------------------
// Defines the data transfer modes for mcbsp
//

typedef enum
{
    kMcBSPProfile_I2S_Slave = 0,
    kMcBSPProfile_I2S_Master,
    kMcBSPProfile_TDM
} McBSPProfile_e;

//------------------------------------------------------------------------------
// Defines the multichannel block size for mcbsp multichannel in TDM.
//

typedef enum
{
    kMcBSP_Block0 = 0,
    kMcBSP_Block1 = 16,
    kMcBSP_Block2 = 32,
    kMcBSP_Block3 = 48,
    kMcBSP_Block4 = 64,
    kMcBSP_Block5 = 80,
    kMcBSP_Block6 = 96,
    kMcBSP_Block7 = 112,
} McBSPMultiChannelBlock_e;

//------------------------------------------------------------------------------
// McBSP bus configuration structure
//
//  This structure is used to define the settings for a McBSP device
//
typedef struct
{

    // Valid values are
    //  MCBSP_SPCR1_ALB_ENABLE : enable ALB
    //  FALSE                  : disable ALB
    //
    UINT32      AnalogLoopBackMode;

    // receive configuration
    //----------------------

    // Words per receive frame, valid values are [1, 128]
    //
    UINT32      RxFrameLength;

    // Valid values are
    //  kMcBSP_Word_Length_8
    //  kMcBSP_Word_Length_12
    //  kMcBSP_Word_Length_16
    //  kMcBSP_Word_Length_20
    //  kMcBSP_Word_Length_24
    //  kMcBSP_Word_Length_32
    //
    McBSPWordLength_e RxWordLength;

    // Valid values are
    //  kMcBSP_Word_Length_8
    //  kMcBSP_Word_Length_12
    //  kMcBSP_Word_Length_16
    //  kMcBSP_Word_Length_20
    //  kMcBSP_Word_Length_24
    //  kMcBSP_Word_Length_32
    //
    McBSPWordLength_e RxWordLength2;

    // Valid values are
    //  MCBSP_REVERSE_MSB_FIRST
    //  MCBSP_REVERSE_LSB_FIRST
    //
    UINT32      RxReverse;

    // Valid values are
    //  MCBSP_DATDLY_0BIT
    //  MCBSP_DATDLY_1BIT
    //  MCBSP_DATDLY_2BIT
    //
    UINT32      RxDataDelay;

    // Valid values are
    //  MCBSP_PCR_CLKRM : CLKR pin is an output src
    //                    if DLB=0 CLKR is driven by the internal sample-rate
    //                      generator
    //                    if DLB=1 CLKR is driven by the transmit clock
    //
    //  FALSE           : if DLB=0, CLKR pin is driven by an external clock
    //                    if DLB=1, CLKR is driven by the transmit clock, which
    //                      is based on CLKXM.
    //
    UINT32      RxClockSource;

    // Valid values are
    //  MCBSP_PCR_FSRM : Frame synch. generated internally by sample-rate
    //                   generator.  FSR is an output pin except with
    //                   MCBSP_SRGR2_GSYNC is set.
    //
    //  FALSE          : Frame synch. generated externally. FSR is an input pin
    //
    UINT32      RxFrameSyncSource;

    // Valid values are
    //  MCBSP_PHASE_SINGLE  : receive is single phase (1 channel)
    //  MCBSP_PHASE_DUAL    : receive is dual phase (2 channels)
    //
    UINT32      RxPhase;

    // Valid values are
    //  MCBSP_SPCR1_RINTM_RSYNCERR
    //  FALSE
    //
    UINT32      RxSyncError;

    // Valid values are
    //  MCBSP_PCR_CLKRP     : Recieve data sampled on rising edge of CLKR
    //  FALSE               : Recieve data sampled on falling edge of CLKR
    //
    UINT32      RxClkPolarity;

    // Valid values are
    //  MCBSP_PCR_FSRP      : Frame sync pulse FSR is active high
    //  FALSE               : Frame sync pulse FSR is active low
    //
    UINT32      RxFrameSyncPolarity;

    // transmit configuration
    //----------------------

    // Words per transmit frame, valid values are [1, 128]
    //
    UINT32      TxFrameLength;

    // Valid values are
    //  kMcBSP_Word_Length_8
    //  kMcBSP_Word_Length_12
    //  kMcBSP_Word_Length_16
    //  kMcBSP_Word_Length_20
    //  kMcBSP_Word_Length_24
    //  kMcBSP_Word_Length_32
    //
    McBSPWordLength_e TxWordLength;

    // Valid values are
    //  kMcBSP_Word_Length_8
    //  kMcBSP_Word_Length_12
    //  kMcBSP_Word_Length_16
    //  kMcBSP_Word_Length_20
    //  kMcBSP_Word_Length_24
    //  kMcBSP_Word_Length_32
    //
    McBSPWordLength_e TxWordLength2;

    // Valid values are
    //  MCBSP_REVERSE_MSB_FIRST
    //  MCBSP_REVERSE_LSB_FIRST
    //
    UINT32      TxReverse;

    // Valid values are
    //  MCBSP_DATDLY_0BIT
    //  MCBSP_DATDLY_1BIT
    //  MCBSP_DATDLY_2BIT
    //
    UINT32      TxDataDelay;

    // Valid values are
    //  MCBSP_PCR_CLKXM : CLKX pin is an output src, driven by the samplre-rate
    //                      generator.
    //
    //  FALSE           : Transmitter clock is driven by an external clock with
    //                      CLKX as an input pin
    //
    UINT32      TxClockSource;

    // Valid values are
    //   kMcBSP_Tx_FS_SRC_External
    //   kMcBSP_Tx_FS_SRC_DSR_XSR_COPY
    //   kMcBSP_Tx_FS_SRC_SRG
    //
    McBSPTxFrameSyncMode_e TxFrameSyncSource;

    // Valid values are
    //  MCBSP_PHASE_SINGLE  : transmit is single phase (1 channel)
    //  MCBSP_PHASE_DUAL    : transmit is dual phase (2 channels)
    //
    UINT32      TxPhase;

    // Valid values are
    //  MCBSP_SPCR2_XINTM_XSYNCERR
    //  FALSE
    //
    UINT32      TxSyncError;

    // Valid values are
    //  MCBSP_PCR_CLKXP     : Transmit data sampled on falling edge of CLKX
    //  FALSE               : Transmit data sampled on rising edge of CLKX
    //
    UINT32      TxClkPolarity;

    // Valid values are
    //  MCBSP_PCR_FSXP      : Frame sync pulse FSX is active high
    //  FALSE               : Frame sync pulse FSX is active low
    //
    UINT32      TxFrameSyncPolarity;

    // Valid values are
    //  kMcBSP_SRG_SRC_CLKS_PIN
    //  kMcBSP_SRG_SRC_CPU_CLK
    //  kMcBSP_SRG_SRC_CLKR_PIN
    //  kMcBSP_SRG_SRC_CLKX_PIN
    //
    McBSPSRGClkSrc_e  SRGClkSrc;

    // frame width
    // Valid values are [1, 256]
    UINT32      SRGFrameWidth;

    // sample-rate generator clock divisor
    // Valid values are [0, 255]
    //
    UINT32      SRGClkDivFactor;

    // Valid values are
    //  MCBSP_SRGR2_GSYNC
    //  FALSE
    //
    UINT32      SRGClkSyncMode;

    // Valid values are
    //   MSCBS_SPCR1_RJUST_RJ_ZEROFILL
    //   MCBSP_SPCR1_RJUST_RJ_SIGNFILL
    //   MCBSP_SPCR1_RJUST_LJ_ZEROFILL
    UINT32      JustificationMode;
} McBSPDeviceConfiguration_t;


//------------------------------------------------------------------------------
//  tx/rx state information
//
typedef enum
{
    kMCBSP_Port_Uninitialized,
    kMcBSP_Port_Idle,
    kMcBSP_Port_Active,
} McBSPPortState_e;


//------------------------------------------------------------------------------
//  Data structure which shadows hardware registers
//
typedef struct McBSPShadowRegisters_t
{
    UINT32 SPCR2;
    UINT32 SPCR1;
    UINT32 RCR2;
    UINT32 RCR1;
    UINT32 XCR2;
    UINT32 XCR1;
    UINT32 SRGR2;
    UINT32 SRGR1;
    UINT32 PCR;
    UINT32 THRSH1;
    UINT32 THRSH2;
    UINT32 SYSCONFIG;
    UINT32 WAKEUPEN;
    UINT32 MCR1;
    UINT32 MCR2;
    UINT32 RCERA;
    UINT32 XCERA;
	UINT32 XCCR;
} McBSPShadowRegisters_t;

//------------------------------------------------------------------------------
//  Forward declaration
//
typedef struct McBSPDevice_t McBSPDevice_t;
class McbspProfile_t;
class DataPort_t;

//------------------------------------------------------------------------------
//  Instance Structure
//

typedef struct McBSPInstance_t
{
    DWORD                           cookie;
    McBSPDevice_t                  *pDevice;

    // define function pointers for direct data transfers
    //
    void                           *pTransferCallback;
    DataTransfer_PopulateBuffer     fnTxPopulateBuffer;
    DataTransfer_PopulateBuffer     fnRxPopulateBuffer;
    DataTransfer_Command            fnRxCommand;
    DataTransfer_Command            fnTxCommand;
    MutexLock                       fnMutexLock;
} McBSPInstance_t;


//------------------------------------------------------------------------------
//  Device Structure
//
typedef struct McBSPDevice_t
{
    DWORD                           cookie;
    LONG                            instances;

	// Bus handle
    HANDLE                          hParentBus;

    // Device info
	OMAP_DEVICE					    deviceID;
	DWORD							dwPort;
	OMAP35XX_MCBSP_REGS_t		   *pPhysAddrSidetone;	
    OMAP35XX_MCBSP_REGS_t          *pPhysAddrMcBSP;
    OMAP35XX_MCBSP_REGS_t          *pMcbspRegs;
    McBSPShadowRegisters_t          shadowRegs;
    McBSPDeviceConfiguration_t     *pConfigInfo;

    // SIDETONE Device info
    OMAP35XX_MCBSP_REGS_ST_t       *pSideToneRegs;

    // Profile info
    McbspProfile_t                 *pMcbspProfile;

    // Data ports
    DataPort_t                     *pRxPort;
    DataPort_t                     *pTxPort;

    // Activity info
    LONG                            nActivityRefCount;
    DWORD                           fMcBSPActivity;
	BOOL							bTxExitThread;
	BOOL							bRxExitThread;

    // General configuration information
    //
    DWORD                           mcbspProfile;
    DWORD                           memBase[3];
    DWORD                           memLen[3];
    DWORD                           dmaTxSyncMap;
    DWORD                           dmaRxSyncMap;
    DWORD                           priorityDmaTx;
    DWORD                           priorityDmaRx;
    DWORD                           sizeRxBuffer;
    DWORD                           sizeTxBuffer;
    DWORD                           useRegistryForMcbsp;
    // mcbsp bus configuration information
    //
    DWORD                           loopbackMode;
    DWORD                           wordsPerFrame;
    DWORD                           wordLength;
    DWORD                           wordLength2;
    DWORD                           reverseModeTx;
    DWORD                           dataDelayTx;
    DWORD                           clockModeTx;
    DWORD                           frameSyncSourceTx;
    DWORD                           phaseTx;
    DWORD                           clockPolarityTx;
    DWORD                           frameSyncPolarityTx;
    DWORD                           fifoThresholdTx;
    DWORD                           reverseModeRx;
    DWORD                           dataDelayRx;
    DWORD                           clockModeRx;
    DWORD                           frameSyncSourceRx;
    DWORD                           phaseRx;
    DWORD                           clockPolarityRx;
    DWORD                           frameSyncPolarityRx;
    DWORD                           fifoThresholdRx;
    DWORD                           clockSourceSRG;
    DWORD                           frameWidthSRG;
    DWORD                           clockDividerSRG;
    DWORD                           clockResyncSRG;
	DWORD							clksPinSrc;
    DWORD                           justificationMode;

    // mcbsp bus TDM configuration information
    //
    DWORD                           tdmWordsPerFrame;
    DWORD                           partitionMode;
    DWORD                           numOfTxChannels;
    DWORD                           requestedTxChannels[MAX_HW_CODEC_CHANNELS];
    DWORD                           numOfRxChannels;
    DWORD                           requestedRxChannels[MAX_HW_CODEC_CHANNELS];

    // SIDETONE configuration information
    //
    DWORD                           sideToneTxMapChannel0;
    DWORD                           sideToneRxMapChannel0;
    DWORD                           sideToneTxMapChannel1;
    DWORD                           sideToneRxMapChannel1;
    DWORD                           sideToneFIRCoeffWrite[MAX_FIR_COEFF];
    DWORD                           sideToneFIRCoeffRead[MAX_FIR_COEFF];
    DWORD                           sideToneGain;

} McBSPDevice_t;


//------------------------------------------------------------------------------
//  Local Functions in mcbsp.cpp
//

EXTERN_C BOOL MCP_Deinit(DWORD context);
EXTERN_C DWORD MCP_Init(LPCWSTR szContext,
                        LPCVOID pBusContext);
EXTERN_C DWORD MCP_Open(DWORD context,
                        DWORD accessCode,
                        DWORD shareMode);
EXTERN_C BOOL MCP_Close(DWORD context);
EXTERN_C BOOL MCP_IOControl(DWORD context,
                            DWORD code,
                            UCHAR *pInBuffer,
                            DWORD inSize,
                            UCHAR *pOutBuffer,
                            DWORD outSize,
                            DWORD *pOutSize);

EXTERN_C void StartTransmit(McBSPInstance_t *pInstance);
EXTERN_C void StartReceive(McBSPInstance_t *pInstance);
EXTERN_C void StopTransmit(McBSPDevice_t *pDevice);
EXTERN_C void StopReceive(McBSPDevice_t *pDevice);
EXTERN_C INT TxCommand(ExternalDrvrCommand cmd,
                       void* pData,
                       PortConfigInfo_t* pPortConfigInfo);
EXTERN_C INT RxCommand(ExternalDrvrCommand cmd,
                       void* pData,
                       PortConfigInfo_t* pPortConfigInfo);
EXTERN_C DWORD IST_TxDMA(void* pParam);
EXTERN_C DWORD IST_RxDMA(void* pParam);
EXTERN_C void CopyTransferInfo(McBSPInstance_t *pInstance,
                             EXTERNAL_DRVR_DATA_TRANSFER_IN *pTransferDataIn,
                             EXTERNAL_DRVR_DATA_TRANSFER_OUT *pTransferDataOut);
EXTERN_C void ClearTransferInfo(McBSPInstance_t *pInstance);
EXTERN_C void EnableClocks(McBSPDevice_t *pDevice,
                           BOOL bEnable);


//------------------------------------------------------------------------------
//  Local Functions in mcbspconfig.cpp
//

EXTERN_C void mcbsp_ConfigCommonRegisters(McBSPDevice_t*);
EXTERN_C void mcbsp_ClearIRQStatus(McBSPDevice_t*);
EXTERN_C void mcbsp_UpdateRegisters(McBSPDevice_t*);
EXTERN_C void mcbsp_EnableSampleRateGenerator(McBSPDevice_t*);
EXTERN_C void mcbsp_ResetSampleRateGenerator(McBSPDevice_t*);
EXTERN_C void mcbsp_ResetTransmitter(McBSPDevice_t*);
EXTERN_C void mcbsp_EnableTransmitter(McBSPDevice_t*);
EXTERN_C void mcbsp_ResetReceiver(McBSPDevice_t*);
EXTERN_C void mcbsp_EnableReceiver(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigI2SProfile(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigTDMProfile(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigTDMTxChannels(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_ConfigTDMRxChannels(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_GetRegistryValues(McBSPDevice_t*);
EXTERN_C void mcbsp_ResetShadowRegisters(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigureSampleRateGenerator(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigureTransmitter(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigureReceiver(McBSPDevice_t*);
EXTERN_C void mcbsp_ConfigureForMaster(McBSPDevice_t*);
EXTERN_C void mcbsp_SideToneInit(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneEnable(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneDisable(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneWriteFIRCoeff(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneReadFIRCoeff(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneWriteReset(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneReadReset(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneAutoIdle(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneInterruptEnable(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneInterruptStatus(McBSPDevice_t* pDevice);
EXTERN_C void mcbsp_SideToneResetInterrupt(McBSPDevice_t* pDevice);


//------------------------------------------------------------------------------

#ifndef SHIP_BUILD
EXTERN_C void mcbsp_DumpReg(McBSPDevice_t* pDevice, LPCTSTR szMsg);
#define DUMP_MCBSP_REGS(a, b) mcbsp_DumpReg(a, b)
#else
#define DUMP_MCBSP_REGS(a, b)
#endif

#endif //__MCBSPTYPES_H__

