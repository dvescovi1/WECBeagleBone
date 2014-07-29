//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File:  mcasp.h
//
//  This header contains type definitions and function prototypes
//  for the McASP lower layer audio routines.
//

#ifndef __MCASP_H_
#define __MCASP_H_

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of serializers
#define MCASP_MAX_SERS 16

/** Enumeration for the serializer numbers */
typedef enum {
    /** SRCTL0 */
    SERIALIZER_0 = 0,
    /** SRCTL1 */
    SERIALIZER_1 = 1,
    /** SRCTL2 */
    SERIALIZER_2 = 2,
    /** SRCTL3 */
    SERIALIZER_3 = 3,
    /** SRCTL4 */
    SERIALIZER_4 = 4,
    /** SRCTL5 */
    SERIALIZER_5 = 5,
    /** SRCTL6 */
    SERIALIZER_6 = 6,
    /** SRCTL7 */
    SERIALIZER_7 = 7,
    /** SRCTL8 */
    SERIALIZER_8 = 8,
    /** SRCTL9 */
    SERIALIZER_9 = 9,
    /** SRCTL10 */
    SERIALIZER_10 = 10,
    /** SRCTL11 */
    SERIALIZER_11 = 11,
    /** SRCTL12 */
    SERIALIZER_12 = 12,
    /** SRCTL13 */
    SERIALIZER_13 = 13,
    /** SRCTL14 */
    SERIALIZER_14 = 14,
    /** SRCTL15 */
    SERIALIZER_15 = 15
} McaspSerializerNum;

/**
 *  Enumeration for DIT channel index
 */
typedef enum {
    /* 1st DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_0 = 0,
    /* 2nd DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_1 = 1,
    /* 3rd DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_2 = 2,
    /* 4th DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_3 = 3,
    /* 5th DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_4 = 4,
    /* 6th DIT (channel/user data), (left/right) Register */
    DIT_REGISTER_5 = 5
} McaspDITRegIndex;


/** Enumeration for the serializer mode */
typedef enum {
    /** Serializer is inactive */
    SERIALIZER_INACTIVE = 0,
    /** Serializer is transmitter */
    SERIALIZER_XMT = 1,
    /** Serializer is receiver */
    SERIALIZER_RCV = 2
} McaspSerMode;

/** Hardware setup global structure */
typedef struct _McaspHwSetupGbl {
    /** Pin function register */
    UINT32 pfunc;
    /** Pin direction register */
    UINT32 pdir;
    /** Global control register - GBLCTL*/
    UINT32 ctl;
    /** Decides whether McASP operates in DIT mode - DITCTL */
    UINT32 ditCtl;
    /** Digital loopback mode setup - DLBEN */
    UINT32 dlbMode;
    /** Mute control register - AMUTE */
    UINT32 amute;
    /** Setup serializer control register (SRCTL0-15) */
    UINT32 serSetup[MCASP_MAX_SERS];
} McaspHwSetupGbl;

/** Hardware setup data clock structure */
typedef struct _McaspHwSetupDataClk {
    /** Clock details ACLK(R/X)CTL */
    UINT32  clkSetupClk;
    /** High clock details AHCLK(R/X)CTL */
    UINT32  clkSetupHiClk;
    /** Configures receive/transmit clock failure detection R/XCLKCHK */
    UINT32  clkChk;
} McaspHwSetupDataClk;

/** Hardware setup data structure */
typedef struct _McaspHwSetupData {
    /** To mask or not to mask - R/XMASK */
    UINT32                      mask;
    /** Format details as per  - R/XFMT */
    UINT32                      fmt;
    /** Configure the rcv/xmt frame sync - AFSR/XCTL */
    UINT32                      frSyncCtl;
    /** Specifies which TDM slots are active - R/XTDM */
    UINT32                      tdm;
    /** Controls generation of McASP interrupts - R/XINTCTL */
    UINT32                      intCtl;
    /** Status register (controls writable fields of STAT register)-R/XSTAT */
    UINT32                      stat;
    /** Event control register - R/XEVTCTL */
    UINT32                      evtCtl;
    /** Clock settings for rcv/xmt */
    McaspHwSetupDataClk     clk;
} McaspHwSetupData;

/** DIT channel status register structure */
typedef struct _McaspChStatusRam {
    /** Left channel status registers (DITCSRA0-5) */
    UINT32 chStatusLeft[6];
    /** Right channel status register (DITCSRB0-5) */
    UINT32 chStatusRight[6];
} McaspChStatusRam;

/** DIT channel user data register structure */
typedef struct _McaspUserDataRam {
    /** Left channel user data registers (DITUDRA0-5) */
    UINT32 userDataLeft[6];
    /** Right channel user data registers (DITUDRB0-5) */
    UINT32 userDataRight[6];
} McaspUserDataRam;

/** Hardware setup structure */
typedef struct _McaspHwSetup {
    /** Value to be loaded in global control register (GLBCTL) */
    McaspHwSetupGbl glb;
    /** Receiver settings */
    McaspHwSetupData rx;
    /** Transmitter settings */
    McaspHwSetupData tx;
    /** Power down emulation mode params - PWRDEMU */
    UINT32 emu;
} McaspHwSetup;

UINT16 mcaspGetCurrentXSlot (PMCASPREGS pMcASPRegs);
UINT16 mcaspGetCurrentRSlot (PMCASPREGS pMcASPRegs);
BOOL mcaspGetXmtErr (PMCASPREGS pMcASPRegs);
BOOL mcaspGetXmtClkFail (PMCASPREGS pMcASPRegs);
BOOL mcaspGetXmtSyncErr (PMCASPREGS pMcASPRegs);
BOOL mcaspGetXmtUnderrun (PMCASPREGS pMcASPRegs);
BOOL mcaspGetXmtDataReady (PMCASPREGS pMcASPRegs);
BOOL mcaspGetRcvErr (PMCASPREGS pMcASPRegs);
BOOL mcaspGetRcvClkFail (PMCASPREGS pMcASPRegs);
BOOL mcaspGetRcvSyncErr (PMCASPREGS pMcASPRegs);
BOOL mcaspGetSerRcvReady (PMCASPREGS pMcASPRegs, BOOL *serRcvReady,
                              McaspSerializerNum  serNum);
BOOL mcaspGetSerXmtReady (PMCASPREGS pMcASPRegs,
                                      BOOL *serXmtReady,
                                      McaspSerializerNum  serNum);
BOOL mcaspGetSerMode (PMCASPREGS pMcASPRegs, McaspSerMode *serMode,
                                  McaspSerializerNum  serNum);
UINT16 mcaspGetXmtStat (PMCASPREGS pMcASPRegs);
UINT8 mcaspGetSmFsXmt (PMCASPREGS pMcASPRegs);
UINT8 mcaspGetSmFsRcv (PMCASPREGS pMcASPRegs);
BOOL mcaspGetDitMode (PMCASPREGS pMcASPRegs);
void mcaspSetXmtGbl (PMCASPREGS pMcASPRegs, UINT32 xmtGbl);
void mcaspSetRcvGbl (PMCASPREGS pMcASPRegs, UINT32 rcvGbl);
void mcaspResetXmtFSRst (PMCASPREGS pMcASPRegs);
void mcaspResetRcvFSRst (PMCASPREGS pMcASPRegs);
void mcaspConfigAudioMute (PMCASPREGS pMcASPRegs, UINT32 audioMute);
void mcaspConfigLoopBack (PMCASPREGS pMcASPRegs, BOOL loopBack, UINT8 numSerializers);
void mcaspConfigRcvSlot (PMCASPREGS pMcASPRegs, UINT32 rcvSlot);
void mcaspConfigXmtSlot (PMCASPREGS pMcASPRegs, UINT32 xmtSlot);
void mcaspConfigRcvInt (PMCASPREGS pMcASPRegs, UINT32 rcvInt);
void mcaspConfigXmtInt (PMCASPREGS pMcASPRegs, UINT32 xmtInt);
void mcaspResetRcvClk (PMCASPREGS pMcASPRegs);
void mcaspResetXmtClk (PMCASPREGS pMcASPRegs);
void mcaspSetRcvClk (PMCASPREGS pMcASPRegs, McaspHwSetupDataClk *rcvClkSet);
void mcaspSetXmtClk (PMCASPREGS pMcASPRegs, McaspHwSetupDataClk *xmtClkSet);
void mcaspConfigXmtSection (PMCASPREGS pMcASPRegs, McaspHwSetupData  *xmtData);
void mcaspConfigRcvSection (PMCASPREGS pMcASPRegs, McaspHwSetupData  *rcvData);
void mcaspSetSerXmt (PMCASPREGS pMcASPRegs, McaspSerializerNum  serNum);
void mcaspSetSerRcv (PMCASPREGS pMcASPRegs, McaspSerializerNum  serNum);
void mcaspSetSerIna (PMCASPREGS pMcASPRegs, McaspSerializerNum serNum);
void mcaspWriteChanStatRam (PMCASPREGS pMcASPRegs, McaspChStatusRam *chanStatRam);
void mcaspWriteUserDataRam (PMCASPREGS pMcASPRegs, McaspUserDataRam *userDataRam);
void mcaspResetXmt (PMCASPREGS pMcASPRegs);
void mcaspResetRcv (PMCASPREGS pMcASPRegs);
void mcaspResetSmFsXmt (PMCASPREGS pMcASPRegs);
void mcaspResetSmFsRcv (PMCASPREGS pMcASPRegs);
void mcaspActivateXmtClkSer (PMCASPREGS pMcASPRegs);
void mcaspActivateRcvClkSer (PMCASPREGS pMcASPRegs);
void mcaspActivateSmRcvXmt (PMCASPREGS pMcASPRegs);
void mcaspActivateFsRcvXmt (PMCASPREGS pMcASPRegs);
void mcaspActivateSmFsXmt  (PMCASPREGS pMcASPRegs);
void mcaspActivateSmFsRcv  (PMCASPREGS pMcASPRegs);
void mcaspSetDitMode (PMCASPREGS pMcASPRegs, BOOL ditFlag);
UINT16 mcaspGetAmute (PMCASPREGS pMcASPRegs);
void mcaspActivateClkRcvXmt(PMCASPREGS pMcASPRegs);
void mcaspRegReset (PMCASPREGS pMcASPRegs, UINT8 numSerializers);
BOOL mcaspHwSetup(PMCASPREGS pMcASPRegs, McaspHwSetup *myHwSetup, UINT8 numSerializers);
void mcaspConfigGpio(PMCASPREGS pMcASPRegs, UINT32 setVal);
UINT32 mcaspGpioRead (PMCASPREGS pMcASPRegs);
void mcaspGpioWrite (PMCASPREGS pMcASPRegs, UINT32 setVal);
void mcaspReadXmtConfig (PMCASPREGS pMcASPRegs, McaspHwSetupData *xmtData);
void mcaspReadRcvConfig (PMCASPREGS pMcASPRegs, McaspHwSetupData *rcvData);
void mcaspSerXmtEn(PMCASPREGS pMcASPRegs);
void mcaspSerRcvEn(PMCASPREGS pMcASPRegs);

void mcaspTestAudio(PMCASPREGS pMcASPRegs);

#ifdef __cplusplus
}
#endif

#endif /* __MCASP_H_ */
