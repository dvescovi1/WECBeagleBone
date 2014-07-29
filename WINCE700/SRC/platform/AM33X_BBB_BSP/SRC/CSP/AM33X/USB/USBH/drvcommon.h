// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  drvcommon.h
//


#ifndef __DRVCOMMON_H__
#define __DRVCOMMON_H__

#include "am33x_config.h"
#include "am33x_usb.h"

/*****************************************************************************/
/* HCD register offsets */

#define USB_CTRL_REG_OFFSET           0x04
#define USB_STATR_REG_OFFSET          0x08
#define USB_AUTOREQ_REG_OFFSET        0x14
#define USB_TEARDOWN_REG_OFFSET       0x1c
#define USB_EP_INTSRCR_REG_OFFSET     0x20
#define USB_EP_INTSETR_REG_OFFSET     0x24
#define USB_EP_INTCLRR_REG_OFFSET     0x28
#define USB_EP_INTMSKR_REG_OFFSET     0x2c
#define USB_EP_INTMSKSETR_REG_OFFSET  0x30
#define USB_EP_INTMSKCLRR_REG_OFFSET  0x34
#define USB_EP_INTMASKEDR_REG_OFFSET  0x38

#define USB_CORE_INTSRCR_REG_OFFSET     0x40
#define USB_CORE_INTSETR_REG_OFFSET     0x44
#define USB_CORE_INTCLRR_REG_OFFSET     0x48
#define USB_CORE_INTMSKR_REG_OFFSET     0x4c
#define USB_CORE_INTMSKSETR_REG_OFFSET  0x50
#define USB_CORE_INTMSKCLRR_REG_OFFSET  0x54
#define USB_CORE_INTMASKEDR_REG_OFFSET  0x58

#define USB_EOIR_REG_OFFSET           0x60

#define USB_GENRNDISSZEP1_REG_OFFSET  0x80
#define USB_GENRNDISSZEP2_REG_OFFSET  0x84
#define USB_GENRNDISSZEP3_REG_OFFSET  0x88
#define USB_GENRNDISSZEP4_REG_OFFSET  0x8c
#define USB_GENRNDISSZEP5_REG_OFFSET  0x90
#define USB_GENRNDISSZEP6_REG_OFFSET  0x94
#define USB_GENRNDISSZEP7_REG_OFFSET  0x98
#define USB_GENRNDISSZEP8_REG_OFFSET  0x9c
#define USB_GENRNDISSZEP9_REG_OFFSET  0xA0
#define USB_GENRNDISSZEP10_REG_OFFSET  0xA4

#define USB_FADDR_REG_OFFSET        0x400
#define USB_POWER_REG_OFFSET        0x401
#define USB_INTRTX_REG_OFFSET       0x402
#define USB_INTRRX_REG_OFFSET       0x404
#define USB_INTRTXE_REG_OFFSET      0x406
#define USB_INTRRXE_REG_OFFSET      0x408
#define USB_INTRUSB_REG_OFFSET      0x40a
#define USB_INTRUSBE_REG_OFFSET     0x40b
#define USB_FRAME_REG_OFFSET        0x40c
#define USB_INDEX_REG_OFFSET        0x40e
#define USB_TESTMODE_REG_OFFSET     0x40f

#define USB_FIFO_REG_OFFSET         0x420

#define USB_DEVCTL_REG_OFFSET       0x460

#define MGC_O_HDRC_TXFIFOSZ         0x462   /* 8-bit (see masks) */
#define MGC_O_HDRC_RXFIFOSZ         0x463   /* 8-bit (see masks) */
#define MGC_O_HDRC_TXFIFOADD        0x464   /* 16-bit offset shifted right 3 */
#define MGC_O_HDRC_RXFIFOADD        0x466   /* 16-bit offset shifted right 3 */

/*****************************************************************************/
/* Get offset for a given FIFO */

#define MGC_FIFO_OFFSET(_bEnd)      (0x400 +0x20 + ((_bEnd) * 4))

/*****************************************************************************/
/* offsets to registers in flat model */

#define MGC_O_HDRC_TXMAXP           0x00
#define MGC_O_HDRC_TXCSR            0x02
#define MGC_O_HDRC_CSR0             MGC_O_HDRC_TXCSR    /* re-used for EP0 */
#define MGC_O_HDRC_RXMAXP           0x04
#define MGC_O_HDRC_RXCSR            0x06
#define MGC_O_HDRC_RXCOUNT          0x08
#define MGC_O_HDRC_COUNT0           MGC_O_HDRC_RXCOUNT  /* re-used for EP0 */
#define MGC_O_HDRC_TXTYPE           0x0A
#define MGC_O_HDRC_TYPE0            MGC_O_HDRC_TXTYPE   /* re-used for EP0 */
#define MGC_O_HDRC_TXINTERVAL       0x0B
#define MGC_O_HDRC_NAKLIMIT0        MGC_O_HDRC_TXINTERVAL   /* re-used for EP0 */
#define MGC_O_HDRC_RXTYPE           0x0C
#define MGC_O_HDRC_RXINTERVAL       0x0D
#define MGC_O_HDRC_FIFOSIZE         0x0F
#define MGC_O_HDRC_CONFIGDATA       MGC_O_HDRC_FIFOSIZE /* re-used for EP0 */

#define MGC_END_OFFSET(_bEnd, _bOffset) \
    (0x400 + 0x100 + (0x10*(_bEnd)) + (_bOffset))

/*****************************************************************************/

/* "bus control" registers */
#define MGC_O_HDRC_TXFUNCADDR   0x00
#define MGC_O_HDRC_TXHUBADDR    0x02
#define MGC_O_HDRC_TXHUBPORT    0x03

#define MGC_O_HDRC_RXFUNCADDR   0x04
#define MGC_O_HDRC_RXHUBADDR    0x06
#define MGC_O_HDRC_RXHUBPORT    0x07

#define MGC_BUSCTL_OFFSET(_bEnd, _bOffset) \
    (0x400 + 0x80 + (8*(_bEnd)) + (_bOffset))
/*****************************************************************************/

/* CSR0 in Host mode */
#define MGC_M_CSR0_H_DISPING      0x0800
#define MGC_M_CSR0_H_WR_DATATOGGLE   0x0400 /* set to allow setting: */
#define MGC_M_CSR0_H_DATATOGGLE     0x0200  /* data toggle control */
#define MGC_M_CSR0_FLUSHFIFO      0x0100
#define MGC_M_CSR0_H_NAKTIMEOUT   0x0080
#define MGC_M_CSR0_H_STATUSPKT    0x0040
#define MGC_M_CSR0_H_REQPKT       0x0020
#define MGC_M_CSR0_H_ERROR        0x0010
#define MGC_M_CSR0_H_SETUPPKT     0x0008
#define MGC_M_CSR0_H_RXSTALL      0x0004
#define MGC_M_CSR0_TXPKTRDY       0x0002
#define MGC_M_CSR0_RXPKTRDY       0x0001

/* TXCSR in Host mode */

#define MGC_M_TXCSR_H_WR_DATATOGGLE   0x0200
#define MGC_M_TXCSR_H_DATATOGGLE      0x0100
#define MGC_M_TXCSR_H_NAKTIMEOUT  0x0080
#define MGC_M_TXCSR_H_RXSTALL     0x0020
#define MGC_M_TXCSR_H_ERROR       0x0004

/* TXCSR in Peripheral and Host mode */

#define MGC_M_TXCSR_AUTOSET       0x8000
#define MGC_M_TXCSR_ISO           0x4000
#define MGC_M_TXCSR_MODE          0x2000
#define MGC_M_TXCSR_DMAENAB       0x1000
#define MGC_M_TXCSR_FRCDATATOG    0x0800
#define MGC_M_TXCSR_DMAMODE       0x0400
#define MGC_M_TXCSR_CLRDATATOG    0x0040
#define MGC_M_TXCSR_FLUSHFIFO     0x0008
#define MGC_M_TXCSR_FIFONOTEMPTY  0x0002
#define MGC_M_TXCSR_TXPKTRDY      0x0001

/* RXCSR in Host mode */

#define MGC_M_RXCSR_H_AUTOREQ     0x4000
#define MGC_M_RXCSR_H_WR_DATATOGGLE   0x0400
#define MGC_M_RXCSR_H_DATATOGGLE       0x0200
#define MGC_M_RXCSR_H_NAKTIMEOUT  0x0080
#define MGC_M_RXCSR_H_RXSTALL     0x0040
#define MGC_M_RXCSR_H_REQPKT      0x0020
#define MGC_M_RXCSR_H_ERROR       0x0004

#define MGC_M_RXCSR_AUTOCLEAR     0x8000
#define MGC_M_RXCSR_DMAENAB       0x2000
#define MGC_M_RXCSR_DISNYET       0x1000
#define MGC_M_RXCSR_DMAMODE       0x0800
#define MGC_M_RXCSR_INCOMPRX      0x0100
#define MGC_M_RXCSR_CLRDATATOG    0x0080
#define MGC_M_RXCSR_FLUSHFIFO     0x0010
#define MGC_M_RXCSR_DATAERROR     0x0008
#define MGC_M_RXCSR_FIFOFULL      0x0002
#define MGC_M_RXCSR_RXPKTRDY      0x0001


// Amount of memory to use for HCD buffer

#define gcTotalAvailablePhysicalMemory (0x20000) // 128K
#define gcHighPriorityPhysicalMemory (gcTotalAvailablePhysicalMemory/4)   // 1/4 as high priority

// PDD context structure

typedef struct _SOhcdPdd
{
    LPVOID lpvMemoryObject;
    LPVOID lpvOhcdMddObject;
    TCHAR szDriverRegKey[MAX_PATH];
    CSL_Usbss_Regs *usbss;
    CSL_UsbRegs    *ioPortBase;
    CSL_CppiRegs   *ioCppiBase;
    DWORD dwSysIntr;
	DWORD dwIndex;				// 
	HANDLE hParentBusHandle;
    CEDEVICE_POWER_STATE CurrentDx;
	AM33X_DEVICE_CONF_REGS *pSysConfReg;
    DWORD dwDescriptorCount;
    DWORD dwDisablePowerManagement;
    BOOL fUSB11Enabled;
} SOhcdPdd;

/* Platform specific PDD functions */
extern "C" {
    BOOL USBHPDD_Init(void);
    BOOL USBHPDD_PowerVBUS(BOOL bPowerOn);
};

extern "C" void ChwDmaGetStat(void *pCohcd, UINT32 *retBuf);
extern "C" void ChwGetInfo(void *pCohcd, UINT32 *retBuf);


#endif // __DRVCOMMON_H__
