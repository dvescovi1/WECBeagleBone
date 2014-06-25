// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file UsbFnPdd.h
//! \brief USB Peripheral PDD Driver Header File. This header file
//!        contains the Software abstraction [typedef declarations] to
//!        denote each EndPoint and also the Context of the PDD Layer
//!
//! \version  1.00 Sept 18 2006 File Created

#ifndef __USBFNPDD_H_INCLUDED__
#define __USBFNPDD_H_INCLUDED__

/* WINDOWS CE Public Includes -------------------------------------- */
#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <usbfn.h>
#include <ceddkex.h>

/* Platform Includes ---------------------------------------------- */
#include "UsbPeripheral.h"
#include "usbfnoverlapped.h"

#ifndef SHIP_BUILD
#define STR_MODULE _T("AM33XUsbFn")
#define SETFNAME() LPCTSTR pszFname = STR_MODULE _T(__FUNCTION__) _T(":")
#else
#define SETFNAME()
#endif

/* Global:  dpCurSettings */
#ifdef DEBUG

/* The USB FN MDD Framework reserves some Debug zones for the PDD. The
 * header file usbfn.h provides further information on this aspect.
 * Zones 8 to 11 are reserved for the PDD. Zone 15 is not being
 * used by the MDD.
 */

#define ZONE_PDD_INIT           DEBUGZONE(8)
#define ZONE_PDD_EP0            DEBUGZONE(9)
#define ZONE_PDD_RX             DEBUGZONE(10)
#define ZONE_PDD_TX             DEBUGZONE(11)
#define ZONE_PDD_ISO            DEBUGZONE(14)
#define ZONE_PDD_DMA            DEBUGZONE(15)

#define DBG_PDD_INIT            (1 << 8)
#define DBG_PDD_EP0             (1 << 9)
#define DBG_PDD_RX              (1 << 10)
#define DBG_PDD_TX              (1 << 11)
#define DBG_PDD_ISO             (1 << 14)
#define DBG_PDD_DMA             (1 << 15)

#define PRINTMSG                DEBUGMSG

#else

#define PRINTMSG                RETAILMSG

#undef ZONE_ERROR
#undef ZONE_WARNING
#undef ZONE_SEND
#undef ZONE_INIT

#define ZONE_ERROR              1
#define ZONE_WARNING            1
#define ZONE_SEND               0
#define ZONE_RECEIVE            0
#define ZONE_INIT               0
#define ZONE_FUNCTION           0

#define ZONE_PDD_INIT           0
#define ZONE_PDD_EP0            0
#define ZONE_PDD_RX             0
#define ZONE_PDD_TX             0
#define ZONE_PDD_ISO            0
#define ZONE_PDD_DMA            0

#endif  /* #ifdef DEBUG */

/* Macro to find number of elements in a vector */
#define dimof(x)                (sizeof(x)/sizeof(x[0]))
#define offset(s, f)            FIELD_OFFSET(s, f)
#define fieldsize(s, f)         sizeof(((s*)0)->f)

/* Macros used for the Registry Access Routines */
#define PARAM_DWORD             1
#define PARAM_STRING            2
#define PARAM_MULTIDWORD        3
#define PARAM_BIN               4

/*  Driver -level USB Defines */

/* Maximum Buffer Area Sizes for the USB Driver DMA Buffer Areas */
#define USB_RX_EP_DMA_BUF_SIZE  N4KB
#define USB_TX_EP_DMA_BUF_SIZE  N4KB


#define INVALID_ALTSETTING_COUNT (0xffff)


/* Defines to map USB PDD States and Enums maintained by PDD */

#define LOCK_ENDPOINT(pPdd)     EnterCriticalSection (&(pPdd->epCS))
#define UNLOCK_ENDPOINT(pPdd)   LeaveCriticalSection (&(pPdd->epCS))

//******************************************************************
//
//! \typedef UsbFnEp
//! \brief Internal PDD structure which holds info about
//!        endpoint direction, max packet size and active transfer.
//
//*******************************************************************

/* peripheral side ep0 states */

typedef enum
{
    MGC_END0_STAGE_SETUP = 0,   /* waiting for setup */
    MGC_END0_START,             /* After Setup, Idle s*/
    MGC_END0_STAGE_TX,          /* IN data */
    MGC_END0_STAGE_RX,          /* OUT data */
    MGC_END0_STAGE_STATUSIN,    /* (after OUT data) */
    MGC_END0_STAGE_STATUSOUT,   /* (after IN data) */
    MGC_END0_STAGE_ACKWAIT      /* after zlp, before statusin */
} MUSB_EP0_STAGE;

typedef struct {
    UINT16          epNumber     ;  /// Physical Number of this EP
    BOOL            epInitialised;  /// EP has been initialised
    WORD            maxPacketSize;  /// max Pkt Size of EP
    WORD            fifoOffset   ;  /// Offset from which EP Fifo Starts
    UCHAR           endpointType ;  /// EP Type [CNTRL, ISO, INTR, BULK]
    BOOL            dirRx        ;  /// EP Transfer Direction
    BOOL            zeroLength   ;  /// Flag to denote Zero length Pkt
    BOOL            stalled      ;  /// Flag to Denote Stall Condition
    MUSB_EP0_STAGE  epStage      ;  /// State in which EP is operating
    STransfer       *pTransfer   ;  /// Pointer to the current Transfer
    BOOL            zlpReq       ;  /// Indicates if zero len pkt is required to terminate transfer
    BOOL            usingDma     ;  /// Using DMA for current transfer
    PVOID           pDmaChan     ;  /// Pointer to User-defined DMA Channel
    UCHAR          *pDmaBuffer   ;  /// DMA Buffer Virtual Address Pointer
    DWORD           paDmaBuffer  ;  /// DMA Buffer Physical Address
    PUSB_OVERLAPPED_INFO pOverlappedInfo;  /// Overlapped info (NULL if not overlapped)
} UsbFnEp;

/* Typedef to maintain the PDD Driver Context */

//******************************************************************
//
//! \typedef UsbFnPdd
//! \brief Internal PDD context structure. This structure is used
//!        to represent the PDD Context in the USB Function Driver
//
//******************************************************************
typedef struct {
    DWORD irq;                      /// Irqs supported by Driver.
    DWORD priority256;              /// Priority of the IST Thread
    DWORD dmaBufferSize;            /// Endpoint DMA Buffer Area Size
    DWORD disablePowerManagement;   /// Flag to control enabling/disabling of power management

    VOID *pMddContext;              /// Pointer to the MDD Context
    PFN_UFN_MDD_NOTIFY pfnNotify;   /// MDD Notify Routine

    HANDLE parentBus;               /// Handle to the Bus

    CSL_UsbRegs     		*pUsbdRegs;     /// Pointer to the USB Controller
    CSL_CppiRegs    		*pCppiRegs;     /// Pointer to the CPPI module
    volatile UINT32 		*pUsbPhyReset;  /// Pointer to USB_PHY_RESET register
//    OMAP_SYSC_GENERAL_REGS  *pSysConfRegs;  /// System Config Registers

    DWORD   sysIntr;                /// SYSINTR Assigned to the Driver
    HANDLE  hIntrEvent;             /// Handle to the IST Event
    BOOL    exitIntrThread;         /// Flag to Signal IST Exit
    BOOL    resetComplete;          /// Flag to signal USB Controller Reset
    BOOL    attachState;            /// Whether Attached or detached from Bus
    UINT8   devAddress;             /// Device Address Assigned by Host
    HANDLE  hIntrThread;            /// Handle to the IST Thread

    CEDEVICE_POWER_STATE currentPowerState;  /// Current Power State
    TCHAR szActivityEvent[MAX_PATH];
    HANDLE hActivityEvent;

    DWORD   devState;           /// Maintains the Device State
    BOOL    selfPowered;        /// Flag to signal Self-Powered USB Device
    BOOL    fUSB11Enabled;      /// Flag to indicate if USB1.1 PHY is in use

    BOOL    setupDirRx;         /// Flag to Determine SETUP Direction
    WORD    setupCount;         /// Count of the SETUP required

    HANDLE  hStartEvent;        /// Event used to indicate PDD start

    CRITICAL_SECTION epCS;      /// Critical Section to protect Access
    UsbFnEp ep[USBD_EP_COUNT];  /// Array of USB EP Structures

    USB_DEVICE_REQUEST setupRequest; /// Holding place for zero data setup req

    BOOL fWaitingForHandshake;    /// Set when waiting for MDD to call
                                  /// UfnPdd_SendControlStatusHandshake()
    BOOL fHasQueuedSetupRequest;  /// Indicates if there is a queued EP0 setup
                                  /// request in queuedSetupRequest
    USB_DEVICE_REQUEST queuedSetupRequest;

	DWORD	dwIndex;			/// USB controller number (can be 1 or 2)
#ifdef CPPI_DMA_SUPPORT
    void  *pDmaCntrl;           /// DMA Controller
    DWORD  descriptorCount;     /// Number of CPPI descriptors in the Pool
    BOOL   disableRxGenRNDIS;   /// Disable generic RNDIS mode for RX transfers
#endif /* #ifdef CPPI_DMA_SUPPORT */

} USBFNPDDCONTEXT, *PUSBFNPDDCONTEXT;

/* Function Declarations ---------------------------------------------- */
static
CEDEVICE_POWER_STATE
UpdateDevicePower(
    USBFNPDDCONTEXT *pPdd
    );

DWORD GetDeviceRegistryParams(
    LPCWSTR, VOID *, DWORD,
    const DEVICE_REGISTRY_PARAM params[]) ;

BOOL
USBPeripheralInit (
    USBFNPDDCONTEXT *pPddContext
    );

BOOL
USBPeripheralDeinit (
    USBFNPDDCONTEXT *pPddContext
    );

BOOL
USBPeripheralStart (
    USBFNPDDCONTEXT *pPddContext
    );

BOOL
USBPeripheralEndSession (
    USBFNPDDCONTEXT *pPddContext
    );

BOOL
USBPeripheralPowerDown (
    USBFNPDDCONTEXT *pPddContext
    );

BOOL
USBPeripheralPowerUp (
    USBFNPDDCONTEXT *pPddContext
    );

void
HandleUsbCoreInterrupt(
    USBFNPDDCONTEXT *pPddContext,
    UINT16           intrUsb
    );

void
UsbPddTxRxIntrHandler(
    USBFNPDDCONTEXT *pPddContext,
    UINT16           intrTxReg,
    UINT16           intrRxReg
    );

void
DumpUsbRegisters (
    USBFNPDDCONTEXT *pPdd
    );

VOID
SetupUsbRequest(
    USBFNPDDCONTEXT     *pPdd,
    PUSB_DEVICE_REQUEST pUsbRequest,
    UINT16              epNumber,
    UINT16              requestSize
    );

VOID
ContinueEp0TxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    );

VOID
ContinueTxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    );

int
TxDmaFifoComplete(
    void    *pDriverContext,
    int     endPoint
    );

VOID
ContinueEp0RxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    );

VOID
ContinueRxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    );

void
UsbPddEp0IntrHandler (
    USBFNPDDCONTEXT     *pPddContext
    );

DWORD
GetTotalAltSettingCount(
    PCUFN_CONFIGURATION pConfiguration
    );


#ifdef CPPI_DMA_SUPPORT
struct dma_controller *
cppiControllerInit (
    USBFNPDDCONTEXT *pPdd
    );

void
cppiControllerDeinit (
    USBFNPDDCONTEXT *pPdd
    );

void
cppiCompletionIsr (
    USBFNPDDCONTEXT *pPdd
    );
#endif  /* #ifdef CPPI_DMA_SUPPORT */

/* Platform specific PDD functions */
BOOL USBFNPDD_Init(void);
BOOL USBFNPDD_PowerVBUS(BOOL bPowerOn);
VOID USBFNPDD_WaitForAPIsReady();



#endif /* #ifndef __USBFNPDD_H_INCLUDED__ */
