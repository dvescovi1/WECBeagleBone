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
//      HS USB Register Map.
//      This is temporary put in here and would need to move to inc\ later on.
//
//------------------------------------------------------------------------------

#ifndef OMAP_MUSBCORE_H
#define OMAP_MUSBCORE_H

#define OMAP_MUSB_GEN_OFFSET     0x00
#define OMAP_MUSB_CSR_OFFSET     0x100
#define OMAP_MUSB_DMA_OFFSET     0x200
#define OMAP_MUSB_OTG_OFFSET     0x400

#define USBD_EP_COUNT   16

// Endpoint offset
typedef enum { EP0=0,EP1,EP2,EP3,EP4,EP5,EP6,EP7,EP8,EP9,EP10,EP11,EP12,EP13,EP14,EP15 } EP_INDEx;

enum { 
    MODE_SYSTEM_NORMAL,
    MODE_SYSTEM_SUSPEND,
    MODE_SYSTEM_RESUME
};

struct _MUSB_FUNCS {
    DWORD (*ResetIRQ)(PVOID pContext);
    DWORD (*ResumeIRQ)(PVOID pContext);
    DWORD (*ProcessEP0)(PVOID pContext);
    DWORD (*ProcessEPx_RX)(PVOID pContext, DWORD endpoint);
    DWORD (*ProcessEPx_TX)(PVOID pContext, DWORD endpoint);
    DWORD (*Connect)(PVOID pContext);
    DWORD (*Disconnect)(PVOID pContext);
    DWORD (*Suspend)(PVOID pContext);
    DWORD (*ProcessDMA)(PVOID pContext, UCHAR channel);
};

typedef struct _MUSB_FUNCS MUSB_FUNCS, * PMUSB_FUNCS;

typedef LPVOID * LPLPVOID;
typedef BOOL (* LPMUSB_ATTACH_PROC)(PMUSB_FUNCS pFuncs, int mode, LPLPVOID lppvContext);
typedef BOOL (* LPMUSB_USBCLOCK_PROC)(BOOL fStart);

#pragma pack( push, 1 )

// Indexed Register strcture for Host
typedef struct {
    UINT16 TxMaxP;      //  Max packet sizeofr host Tx endpoint (10,11)
    union {
        UINT16  CSR0;   //  Control Status Reg for ep0
        UINT16  TxCSR;  //  Control Status Reg for Tx endpoint
    } CSR;              //  (12,13)
    UINT16  RxMaxP;     //  Max packet size for host Rx endpoint (14,15)
    UINT16  RxCSR;      //  Control Status Register for Rx ep (16,17)
    union {
        UINT8   Count0; //  no of bytes received for EP0 FIFO 
        UINT16  RxCount;//  no of bytes received for EP1-15 
    } Count;            //  (18,19)
    union {
        UINT8   Type0;  //  speed of EP0
        UINT8   TxType; //  transaction protocol for Tx ep 1-15
    } Type;             //  (1A)
    union {
        UINT8   NAKLimit0;  //  NAK response timeout for EP0
        UINT8   TxInterval; //  Polling interval for Interrupt/ISOC Tx transcations
    } timeout;          //  (1B)
    UINT8   RxType;     //  transcation protocol for Rx ep 1-15 (1C)
    UINT8   RxInterval; //  Polling interval for Interrupt/ISOC Rx transactions (1D)
    UINT8   unused;     //  (1E)
    union {
        UINT8   ConfigData; // Core configuration when index reg set to EP0
        UINT8   FIFOSize;   // Configured size of the selected Rx/Tx FIFO for EP1-15
    } Config;           //  (1F)    
} H_Index;


typedef volatile struct {
    H_Index  ep[16];  // Note that endpoint 0 has been configured with different
} CSP_MUSB_CSR_REGS, *PCSP_MUSB_CSR_REGS;

// Indexed Register structure for Peripheral
typedef struct {
    UINT16  TxMaxP;     //  Max Packet size for per Tx endpoint (10,11)
    union {
        UINT16  CSR0;   //  Control Status Reg for ep 0
        UINT16  TxCSR;  //  Control Status Reg for Tx endpoint
    } CSR;              //  (12,13)
    UINT16  RxMaxP;     //  Max Packet size for per Rx endpoint (14,15)
    UINT16  RxCSR;      //  Control Status Register for Rx endpoint (16,17)
    union {
        UINT16  Count0; //  no of bytes received for EP0 FIFO 
        UINT16  RxCount;//  no of bytes received for EP1-15 
    } Count;            //  (18,19)
    UINT16  Reserved;   //  (1A,1B)
    UINT8   Unused[3];  //  (1C,1D,1E)
    union {
        UINT8   ConfigData; // Core configuration when index reg set to EP0
        UINT8   FIFOSize;   // Configured size of the selected Rx/Tx FIFO for EP1-15
    } Config;           //  (1F)
} D_Index;

typedef struct {
    UINT8   TxFuncAddr; //  Transmit EPx function address (80+8*n)
    UINT8   TxUnused;   //  (81+8*n)
    UINT8   TxHubAddr;  //  Transmit EPx hub address (82+8*n)
    UINT8   TxHubPort;  //  Transmit EPx hub port (83+8*n)
    UINT8   RxFuncAddr; //  Receive EPx function address (84+8*n)
    UINT8   RxUnused;   //  (85+8*n)
    UINT8   RxHubAddr;  //  Receive EPx Hub Address (86+8*n)
    UINT8   RxHubPort;  //  Receive EPx Hub port (87+8*n)
} TargetAddr;

typedef volatile struct {
    UINT8   ULPIVbusControl;    // (070)
    UINT8   ULPICarKitControl;  // (071) documentation shows this reg as ULPIUTMICONTROL
    UINT8   ULPIIntMask;        // (072)
    UINT8   ULPIIntSrc;         // (073)
    UINT8   ULPIRegData;        // (074)
    UINT8   ULPIRegAddr;        // (075)
    UINT8   ULPIRegControl;     // (076)
    UINT8   ULPIRawData;        // (077)
} ULPI_T;

// bits for ULPIRegControl register
#define ULPI_RD_N_WR      (1 << 2)
#define ULPI_REG_CMPLT    (1 << 1)
#define ULPI_REG_REQ      (1 << 0)

// HS USB general register (00h-FFh)
typedef volatile struct {
    // Common USB Registers
    UINT8   FAddr;      //  Function address register (00)
    UINT8   Power;      //  Power management register (01)
    UINT16  IntrTx;     //  Interrupt for EP0 + Tx for EP1-15 (02,03)
    UINT16  IntrRx;     //  Interrupt Rx for EP1-15 (04,05)
    UINT16  IntrTxE;    //  Interrupt Enable for IntrTx (06,07)
    UINT16  IntrRxE;    //  Interrupt Enable for IntrRx (08,09)
    UINT8   IntrUSB;    //  Interrupt for common USB interrupt (0A)
    UINT8   IntrUSBE;   //  Interrupt Enable for IntrUSB (0B)
    UINT16  Frame;      //  Frame number (0C,0D)
    UINT8   Index;      //  Index Register (0E)
    UINT8   Testmode;   //  Test Mode for USB 2.0 (0F)

    // Indexed Registers (10h-1Fh)
    union {
        H_Index hIndex;
        D_Index dIndex;
    } INDEX_REG;

    // FIFOs (20h-5Fh)
    UINT32 fifo[16];    //  FIFO with 4 bytes per endpoint. 

    // Additional Control & Configuration Register (60h-7Fh)
    UINT8   DevCtl;     //  OTG Device Control Reg (60)
    UINT8   unused;     //  Unused (61)
    UINT8   TxFIFOsz;   //  Tx EP FIFO size (62)
    UINT8   RxFIFOsz;   //  Rx EP FIFO size (63)
    UINT16  TxFIFOadd;  //  Tx ep FIFO address (64,65)
    UINT16  RxFIFOadd;  //  Rx ep FIFO address (66,67)
    UINT32  VCtrl_Status;   //  UTMI+PHY Vendor registers (68,69,6A,6B)
    UINT16  HWVers;         //  Hardware version (6C,6D)
    UINT16  unused2;        //  unused (6E,6F)
    ULPI_T  ulpi_regs;      //  ULPI registers (70-77)
    UINT8   EPInfo;         //  Info about no of Tx & Rx eps (78)
    UINT8   RAMInfo;        //  RAM Info (79)
    UINT8   LinkInfo;       //  Info about delays to be applied (7A)
    UINT8   VPLen;          //  VBus pulsing charge (7B)
    UINT8   HS_EOF1;        //  Time buffer available on HS transactions (7C)
    UINT8   FS_EOF1;        //  Time buffer available on FS transactions (7D)
    UINT8   LS_EOF1;        //  Time buffer available on LS transactions (7E)
    UINT8   unused3;        //  unused

    // Target address registers (80h-FFh)
    TargetAddr  ep[16];     //  Target Address register for ep0-15
} CSP_MUSB_GEN_REGS, *PCSP_MUSB_GEN_REGS;

typedef struct {
    UINT32  Cntl;   //  DMA Channel Control (204+10*(n-1))
    UINT32  Addr;   //  DMA Channel Addr    (208+10*(n-1))
    UINT32  Count;  //  DMA n-byte count    (20C+10*(n-1))    
    UINT32  unused; //  unused              (210+10*(n-1))
} DMAChannel;

//  DMA Control Register (200h-27Fh)
typedef volatile struct {
    UINT32      Intr;       //  pending DMA interrupts
    DMAChannel  dma[8];     //  DMA channel info for 8 channels
    // Note the last dma channel should not have "unused" part. But we assume it shouldn't matter
    // even if we map the extra 4 bytes.
} CSP_MUSB_DMA_REGS, *PCSP_MUSB_DMA_REGS;

// OTG Info
typedef volatile struct {
    UINT32      OTG_Rev;        //  OTG Revision
    UINT32      OTG_SYSCONFIG;  //  System Configuration
    UINT32      OTG_SYSSTATUS;  //  System Status
    UINT32      OTG_INTERFSEL;  //  Interface selection
    UINT32      OTG_SIMENABLE;  //  Simulation Enable
    UINT32      OTG_FORCESTDBY; //  Force Standby
} CSP_MUSB_OTG_REGS, *PCSP_MUSB_OTG_REGS;
#pragma pack( pop )

typedef struct {
    HANDLE  hReadyEvents[2];    // Event handle for Host Controller & Device
    PMUSB_FUNCS pFuncs[2];      // Function pointer for Host & device controller        
    PVOID       pContext[2];    // Pointer to the context storage for host & device controller
    PCSP_MUSB_OTG_REGS   pUsbOtgRegs;   //  Pointer to USB OTG Reg
    PCSP_MUSB_CSR_REGS   pUsbCsrRegs;   //  Pointer to the CSR registers (0x100 - 0x1FF)
    PCSP_MUSB_GEN_REGS   pUsbGenRegs;   //  Pointer to USB General Reg
    PCSP_MUSB_DMA_REGS   pUsbDmaRegs;   //  Pointer to USB DMA Reg  
    volatile UINT8       *pPadControlRegs;  // Pointer to Pad Control Reg
    volatile DWORD       *pSysControlRegs;  // Pointer to Control Register.
    UINT16              intr_rx;    // current interrupt RX
    UINT16              intr_tx;    // current interrupt TX
    UINT8               intr_usb;   // current control interrupt
    UINT8               operateMode;  // 0 - Idle State, 1 - Device, 2 - Host 
    UINT8               deviceType;  // 0 - A-device, 1 - B-device
    CRITICAL_SECTION    regCS; // Critical section for access control
    // connect_status is a single byte containing current connect status
    // information. Note that it should be update by OTG, not by Host nor Device
    //  D7      D6      D5      D4      D3      D2      D1
    // -----------------------------------------------------
    // |N/A     N/A     N/A     N/A    H_DC     CSC     CCS|
    // -----------------------------------------------------
    // CCS = current connect status
    // CSC = connect status change
    // H_DC = host disconnect complete
    UINT8               connect_status;    
    DWORD               dwSysIntr;
    HANDLE              hSysIntrEvent;
    HANDLE              hPowerEvent;
    HANDLE              hResumeEvent;
    BOOL                bClockStatus;
    DWORD               dwPwrMgmt; // Indicates USBOTG interface/functional clock status
    
} HSMUSB_T, *PHSMUSB_T;

// OTG_SYSCONFIG register
#define MIDLEMODE       12
#define OTG_SYSCONF_M_FORCESTDBY    (0 << MIDLEMODE)
#define OTG_SYSCONF_M_NOSTDBY       (1 << MIDLEMODE)
#define OTG_SYSCONF_M_SMARTSTDBY    (2 << MIDLEMODE)
#define OTG_SYSCONF_M_MASK          (3 << MIDLEMODE)

#define SIDLEMODE       3
#define OTG_SYSCONF_S_FORCEIDLE    (0 << SIDLEMODE)
#define OTG_SYSCONF_S_NOIDLE       (1 << SIDLEMODE)
#define OTG_SYSCONF_S_SMARTIDLE    (2 << SIDLEMODE)
#define OTG_SYSCONF_S_MASK         (3 << SIDLEMODE)

#define OTG_SYSCONF_ENABLEWAKEUP    (1 << 2)
#define OTG_SYSCONF_SOFTRESET       (1 << 1)
#define OTG_SYSCONF_AUTOIDLE        (1 << 0)

// OTG_SYSSTATUS
#define OTG_SYSSTATUS_RESETDONE     (1 << 0)

// OTG_INTERFSEL register
#define PHYSEL   0
#define OTG_INTERFSEL_UTMI          (0 << PHYSEL)
#define OTG_INTERFSEL_12_PIN_ULPI   (1 << PHYSEL)
#define OTG_INTERFSEL_8_PIN_ULPI    (2 << PHYSEL)

// OTG_FORCESTDBY register
#define OTG_FORCESTDY_ENABLEFORCE   (1 << 0)

// Power Register
#define POWER_ISO           0x80
#define POWER_SOFTCONN      0x40
#define POWER_HSENABLE      0x20
#define POWER_HSMODE        0x10
#define POWER_RESET         0x08
#define POWER_RESUME        0x04
#define POWER_SUSPENDM      0x02
#define POWER_EN_SUSPENDM   0x01

// Testmode Register
#define TESTMODE_FORCE_HOST     0x80
#define TESTMODE_FIFO_ACCESS    0x40
#define TESTMODE_FORCE_FS       0x20
#define TESTMODE_FORCES_HS      0x10
#define TESTMODE_TEST_PACKET    0x08
#define TESTMODE_TEST_K         0x04
#define TESTMODE_TEST_J         0x02
#define TESTMODE_TEST_SE0_NAK   0x01

// DevCtl Register
#define DEVCTL_B_DEVICE     0x80
#define DEVCTL_FSDEV        0x40
#define DEVCTL_LSDEV        0x20
#define DEVCTL_VBUS         0x18
#define DEVCTL_HOSTMODE     0x04
#define DEVCTL_HOSTREQ      0x02
#define DEVCTL_SESSION      0x01
// VBUS bit combinations
#define DEVCTL_VBUSVALID    0x18
#define DEVCTL_AVALID       0x10
#define DEVCTL_SESSVALID    0x08
#define DEVCTL_SESSEND      0x00

// IntrUSB
#define INTRUSB_VBUSERR     0x80
#define INTRUSB_SESSREQ     0x40
#define INTRUSB_DISCONN     0x20
#define INTRUSB_CONN        0x10
#define INTRUSB_SOF         0x08
#define INTRUSB_RESET       0x04
#define INTRUSB_BABBLE      0x04
#define INTRUSB_RESUME      0x02
#define INTRUSB_SUSPEND     0x01
#define INTRUSB_ALL         0xFF

// ENDPOINT handling
#define INTR_EP(x)          (1<<x)
#define INTR_CH(x)          INTR_EP(x)

// CSR0 Host bit set
#define CSR0_H_RxPktRdy     (1<<0)
#define CSR0_H_TxPktRdy     (1<<1)
#define CSR0_H_RxStall      (1<<2)
#define CSR0_H_SetupPkt     (1<<3)
#define CSR0_H_Error        (1<<4)
#define CSR0_H_ReqPkt       (1<<5)
#define CSR0_H_StatusPkt    (1<<6)
#define CSR0_H_NAKTimeout   (1<<7)
#define CSR0_H_FlushFIFO    (1<<8)
#define CSR0_H_DataToggle   (1<<9)
#define CSR0_H_DataTogWrEn  (1<<10)

// RxCSR Host bit set
#define RXCSR_H_RxPktRdy    (1<<0)
#define RXCSR_H_FIFOFull    (1<<1)
#define RXCSR_H_Error       (1<<2)
#define RXCSR_H_NAKTimeout  (1<<3)
#define RXCSR_H_FlushFIFO   (1<<4)
#define RXCSR_H_ReqPkt      (1<<5)
#define RXCSR_H_RxStall     (1<<6)
#define RXCSR_H_ClrDataTog  (1<<7)
#define RXCSR_H_IncompRx    (1<<8)
#define RXCSR_H_DataToggle  (1<<9)
#define RXCSR_H_DataTogWrEn (1<<10)
#define RXCSR_H_DMAReqMode  (1<<11)
#define RXCSR_H_PIDError    (1<<12)
#define RXCSR_H_DMAReqEn    (1<<13)
#define RXCSR_H_AutoReq     (1<<14)
#define RXCSR_H_AutoClr     (1<<15)

// TxCSR Host bit set
#define TXCSR_H_TxPktRdy        (1<<0)
#define TXCSR_H_FIFONotEmpty    (1<<1)
#define TXCSR_H_Error           (1<<2)
#define TXCSR_H_FlushFIFO       (1<<3)
#define TXCSR_H_SetupPkt        (1<<4)
#define TXCSR_H_RxStall         (1<<5)
#define TXCSR_H_ClrDataTog      (1<<6)
#define TXCSR_H_NAKTimeout      (1<<7)
#define TXCSR_H_DataToggle      (1<<8)
#define TXCSR_H_DataTogWrEn     (1<<9)
#define TXCSR_H_DMAReqMode      (1<<10)
#define TXCSR_H_FrcDataTog      (1<<11)
#define TXCSR_H_DMAReqEn        (1<<12)
#define TXCSR_H_Mode            (1<<13)
#define TXCSR_H_AutoSet         (1<<15)

#define RxTxTYPE(s, p, ep)  (((s<<6) & 0xc0)|((p<<4) & 0x30)|(ep & 0xf))


// CSR0 in Peripheral mode 
#define CSR0_P_RXPKTRDY     (1<<0)
#define CSR0_P_TXPKTRDY     (1<<1)
#define CSR0_P_SENTSTALL      (1<<2)
#define CSR0_P_DATAEND     (1<<3)
#define CSR0_P_SETUPEND        (1<<4)
#define CSR0_P_SENDSTALL      (1<<5)
#define CSR0_P_SERVICEDRXPKTRDY    (1<<6)
#define CSR0_P_SERVICEDSETUPEND   (1<<7)
#define CSR0_P_FLUSHFIFO    (1<<8)


/* RXCSR in Peripheral */
#define RXCSR_P_AUTOCLEAR     (1 << 15)
#define RXCSR_P_ISO           (1 << 14)
#define RXCSR_P_DMAREQENAB    (1 << 13)
#define RXCSR_P_DISNYET       (1 << 12)
#define RXCSR_P_DMAREQMODE    (1 << 11)
#define RXCSR_P_INCOMPRX      (1 <<  8)
#define RXCSR_P_CLRDATATOG    (1 <<  7)
#define RXCSR_P_SENTSTALL     (1 <<  6)
#define RXCSR_P_SENDSTALL     (1 <<  5)
#define RXCSR_P_FLUSHFIFO     (1 <<  4)
#define RXCSR_P_DATAERROR     (1 <<  3)
#define RXCSR_P_OVERRUN       (1 <<  2)
#define RXCSR_P_FIFOFULL      (1 <<  1)
#define RXCSR_P_RXPKTRDY      (1 <<  0)


/* TXCSR in Peripheral and Host mode */

#define TXCSR_P_AUTOSET       (1 << 15)
#define TXCSR_P_ISO           (1 << 14)
#define TXCSR_P_MODE          (1 << 13)
#define TXCSR_P_DMAREQENAB    (1 << 12)
#define TXCSR_P_FRCDATATOG    (1 << 11)
#define TXCSR_P_DMAREQMODE    (1 << 10)
#define TXCSR_P_INCOMPTX      (1 <<  7)
#define TXCSR_P_CLRDATATOG    (1 <<  6)
#define TXCSR_P_SENTSTALL     (1 <<  5)
#define TXCSR_P_SENDSTALL     (1 <<  4)
#define TXCSR_P_FLUSHFIFO     (1 <<  3)
#define TXCSR_P_UNDERRUN      (1 <<  2)
#define TXCSR_P_FIFONOTEMPTY  (1 <<  1)
#define TXCSR_P_TXPKTRDY      (1 <<  0)

/* TXFIFOSZ and RXFIFOSZ registers max FIFO packet size */
#define MUSB_FIFOSZ_8_BYTE           (0)
#define MUSB_FIFOSZ_16_BYTE          (1)
#define MUSB_FIFOSZ_32_BYTE          (2)
#define MUSB_FIFOSZ_64_BYTE          (3)
#define MUSB_FIFOSZ_128_BYTE         (4)
#define MUSB_FIFOSZ_256_BYTE         (5)
#define MUSB_FIFOSZ_512_BYTE         (6)
#define MUSB_FIFOSZ_1024_BYTE        (7)
#define MUSB_FIFOSZ_2048_BYTE        (8)
#define MUSB_FIFOSZ_4096_BYTE        (9)

/* Endpoint 0, max endpoint packet size */
#define MAX_EP0_PKTSIZE  (64)

/* Endpoints 1-15, max endpoint packet size */
#define MAX_EPX_PKTSIZE  (1024)

// DMA Channel Control
#define DMA_CNTL_Enable             (1<< 0)
#define DMA_CNTL_Direction          (1<< 1)
#define DMA_CNTL_DMAMode            (1<< 2) // 0 - Indiviual packet, 1 - multiple packets
#define DMA_CNTL_InterruptEnable    (1<< 3)
#define DMA_CNTL_EndpointNumber     (0xf<< 4)
#define DMA_CNTL_BusError           (1<< 8)
#define DMA_CNTL_BurstMode          (3<< 9)

#define BURSTMODE_UNSPEC            (0<<9)
#define BURSTMODE_INCR4             (1<<9)
#define BURSTMODE_INCR8             (2<<9)
#define BURSTMODE_INCR16            (3<<9)

#endif	// OMAP_MUSBCORE_H

