// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file UsbPeripheral.h
//! \brief USB Peripheral defines Header File

#ifndef __USBPERIPHERAL_H_INCLUDED__
#define __USBPERIPHERAL_H_INCLUDED__

/* Defines for Min-max Values of some important Register values */
#define USB_EP_INTR_MASK_ALL        0xFFFEFFFF
#define USB_CORE_INTR_MASK_ALL      0xFFFF03FF

#define USBD_EP_COUNT               MGC_MAX_USB_ENDS

/* macro for identifying the FIFO Sizes from the TXFIFOSZ and RXFIFOSZ Regs*/
#define MUSB_FIFO_GET_SIZE(sz)      ((USHORT)(1 << ((sz) + 3) ) )

/* MUSBHDRC Register bit masks */

/* POWER */
#define MGC_M_POWER_ISOUPDATE       0x80
#define MGC_M_POWER_SOFTCONN        0x40
#define MGC_M_POWER_HSENAB          0x20
#define MGC_M_POWER_HSMODE          0x10
#define MGC_M_POWER_RESET           0x08
#define MGC_M_POWER_RESUME          0x04
#define MGC_M_POWER_SUSPENDM        0x02
#define MGC_M_POWER_ENSUSPEND       0x01

/* INTRUSB */
#define MGC_M_INTR_SUSPEND          0x01
#define MGC_M_INTR_RESUME           0x02
#define MGC_M_INTR_RESET            0x04
#define MGC_M_INTR_BABBLE           0x04
#define MGC_M_INTR_SOF              0x08
#define MGC_M_INTR_CONNECT          0x10
#define MGC_M_INTR_DISCONNECT       0x20
#define MGC_M_INTR_SESSREQ          0x40
#define MGC_M_INTR_VBUSERROR        0x80  /* FOR SESSION END */
#define MGC_M_INTR_EP0              0x01  /* FOR EP0 INTERRUPT */

/* DEVCTL */
#define MGC_M_DEVCTL_BDEVICE        0x80
#define MGC_M_DEVCTL_FSDEV          0x40
#define MGC_M_DEVCTL_LSDEV          0x20
#define MGC_M_DEVCTL_VBUS           0x18
#define MGC_S_DEVCTL_VBUS           3
#define MGC_M_DEVCTL_HM             0x04
#define MGC_M_DEVCTL_HR             0x02
#define MGC_M_DEVCTL_SESSION        0x01

/* TESTMODE */
#define MGC_M_TEST_FORCE_HOST       0x80
#define MGC_M_TEST_FIFO_ACCESS      0x40
#define MGC_M_TEST_FORCE_FS         0x20
#define MGC_M_TEST_FORCE_HS         0x10
#define MGC_M_TEST_PACKET           0x08
#define MGC_M_TEST_K                0x04
#define MGC_M_TEST_J                0x02
#define MGC_M_TEST_SE0_NAK          0x01

/* allocate for double-packet buffering (effectively doubles assigned _SIZE) */
#define MGC_M_FIFOSZ_DPB            0x10
/* allocation size (8, 16, 32, ... 4096) */
#define MGC_M_FIFOSZ_SIZE           0x0f

/* CSR0 */
#define MGC_M_CSR0_FLUSHFIFO        0x0100
#define MGC_M_CSR0_TXPKTRDY         0x0002
#define MGC_M_CSR0_RXPKTRDY         0x0001

/* CSR0 in Peripheral mode */
#define MGC_M_CSR0_P_SVDSETUPEND    0x0080
#define MGC_M_CSR0_P_SVDRXPKTRDY    0x0040
#define MGC_M_CSR0_P_SENDSTALL      0x0020
#define MGC_M_CSR0_P_SETUPEND       0x0010
#define MGC_M_CSR0_P_DATAEND        0x0008
#define MGC_M_CSR0_P_SENTSTALL      0x0004
#define MGC_EP0_STALL_BITS          (MGC_M_CSR0_P_SENDSTALL | MGC_M_CSR0_P_SENTSTALL)

/* CSR0 in Host mode */
#define MGC_M_CSR0_H_WR_DATATOGGLE  0x0400  /* set to allow setting: */
#define MGC_M_CSR0_H_DATATOGGLE     0x0200  /* data toggle control   */
#define MGC_M_CSR0_H_NAKTIMEOUT     0x0080
#define MGC_M_CSR0_H_STATUSPKT      0x0040
#define MGC_M_CSR0_H_REQPKT         0x0020
#define MGC_M_CSR0_H_ERROR          0x0010
#define MGC_M_CSR0_H_SETUPPKT       0x0008
#define MGC_M_CSR0_H_RXSTALL        0x0004

/* CSR0 bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MGC_M_CSR0_P_WZC_BITS       (MGC_M_CSR0_P_SENTSTALL)
#define MGC_M_CSR0_H_WZC_BITS       (MGC_M_CSR0_H_NAKTIMEOUT | MGC_M_CSR0_H_RXSTALL \
                                        | MGC_M_CSR0_RXPKTRDY )

/* TxType/RxType */
#define MGC_M_TYPE_SPEED            0xc0
#define MGC_S_TYPE_SPEED            6
#define MGC_TYPE_SPEED_HIGH         1
#define MGC_TYPE_SPEED_FULL         2
#define MGC_TYPE_SPEED_LOW          3
#define MGC_M_TYPE_PROTO            0x30
#define MGC_S_TYPE_PROTO            4
#define MGC_M_TYPE_REMOTE_END       0xf

/* TXCSR in Peripheral and Host mode */
#define MGC_M_TXCSR_AUTOSET         0x8000
#define MGC_M_TXCSR_ISO             0x4000
#define MGC_M_TXCSR_MODE            0x2000
#define MGC_M_TXCSR_DMAENAB         0x1000
#define MGC_M_TXCSR_FRCDATATOG      0x0800
#define MGC_M_TXCSR_DMAMODE         0x0400
#define MGC_M_TXCSR_CLRDATATOG      0x0040
#define MGC_M_TXCSR_FLUSHFIFO       0x0008
#define MGC_M_TXCSR_FIFONOTEMPTY    0x0002
#define MGC_M_TXCSR_TXPKTRDY        0x0001

/* TXCSR in Peripheral mode */
#define MGC_M_TXCSR_P_INCOMPTX      0x0080
#define MGC_M_TXCSR_P_SENTSTALL     0x0020
#define MGC_M_TXCSR_P_SENDSTALL     0x0010
#define MGC_M_TXCSR_P_UNDERRUN      0x0004

/* TXCSR in Host mode */
#define MGC_M_TXCSR_H_WR_DATATOGGLE 0x0200
#define MGC_M_TXCSR_H_DATATOGGLE    0x0100
#define MGC_M_TXCSR_H_NAKTIMEOUT    0x0080
#define MGC_M_TXCSR_H_RXSTALL       0x0020
#define MGC_M_TXCSR_H_ERROR         0x0004

/* TXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MGC_M_TXCSR_P_WZC_BITS  \
    ( MGC_M_TXCSR_P_INCOMPTX | MGC_M_TXCSR_P_SENTSTALL \
    | MGC_M_TXCSR_P_UNDERRUN | MGC_M_TXCSR_FIFONOTEMPTY )
#define MGC_M_TXCSR_H_WZC_BITS  \
    ( MGC_M_TXCSR_H_NAKTIMEOUT | MGC_M_TXCSR_H_RXSTALL \
    | MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_FIFONOTEMPTY )

/* RXCSR in Peripheral and Host mode */
#define MGC_M_RXCSR_AUTOCLEAR       0x8000
#define MGC_M_RXCSR_DMAENAB         0x2000
#define MGC_M_RXCSR_DISNYET         0x1000
#define MGC_M_RXCSR_DMAMODE         0x0800
#define MGC_M_RXCSR_INCOMPRX        0x0100
#define MGC_M_RXCSR_CLRDATATOG      0x0080
#define MGC_M_RXCSR_FLUSHFIFO       0x0010
#define MGC_M_RXCSR_DATAERROR       0x0008
#define MGC_M_RXCSR_FIFOFULL        0x0002
#define MGC_M_RXCSR_RXPKTRDY        0x0001

/* RXCSR in Peripheral mode */
#define MGC_M_RXCSR_P_ISO           0x4000
#define MGC_M_RXCSR_P_SENTSTALL     0x0040
#define MGC_M_RXCSR_P_SENDSTALL     0x0020
#define MGC_M_RXCSR_P_OVERRUN       0x0004

/* RXCSR in Host mode */
#define MGC_M_RXCSR_H_AUTOREQ       0x4000
#define MGC_M_RXCSR_H_WR_DATATOGGLE 0x0400
#define MGC_M_RXCSR_H_DATATOGGLE    0x0200
#define MGC_M_RXCSR_H_RXSTALL       0x0040
#define MGC_M_RXCSR_H_REQPKT        0x0020
#define MGC_M_RXCSR_H_ERROR         0x0004

/* RXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define MGC_M_RXCSR_P_WZC_BITS  \
    ( MGC_M_RXCSR_P_SENTSTALL | MGC_M_RXCSR_P_OVERRUN \
    | MGC_M_RXCSR_RXPKTRDY )
#define MGC_M_RXCSR_H_WZC_BITS  \
    ( MGC_M_RXCSR_H_RXSTALL | MGC_M_RXCSR_H_ERROR \
    | MGC_M_RXCSR_DATAERROR | MGC_M_RXCSR_RXPKTRDY )

/* HUBADDR */
#define MGC_M_HUBADDR_MULTI_TT      0x80

/* TXCSR in Peripheral and Host mode */
#define MGC_M_TXCSR2_AUTOSET        0x80
#define MGC_M_TXCSR2_ISO            0x40
#define MGC_M_TXCSR2_MODE           0x20
#define MGC_M_TXCSR2_DMAENAB        0x10
#define MGC_M_TXCSR2_FRCDATATOG     0x08
#define MGC_M_TXCSR2_DMAMODE        0x04

#define MGC_M_TXCSR1_CLRDATATOG     0x40
#define MGC_M_TXCSR1_FLUSHFIFO      0x08
#define MGC_M_TXCSR1_FIFONOTEMPTY   0x02
#define MGC_M_TXCSR1_TXPKTRDY       0x01

/* TXCSR in Peripheral mode */
#define MGC_M_TXCSR1_P_INCOMPTX     0x80
#define MGC_M_TXCSR1_P_SENTSTALL    0x20
#define MGC_M_TXCSR1_P_SENDSTALL    0x10
#define MGC_M_TXCSR1_P_UNDERRUN     0x04

/* RXCSR in Peripheral and Host mode */
#define MGC_M_RXCSR2_AUTOCLEAR      0x80
#define MGC_M_RXCSR2_DMAENAB        0x20
#define MGC_M_RXCSR2_DISNYET        0x10
#define MGC_M_RXCSR2_DMAMODE        0x08
#define MGC_M_RXCSR2_INCOMPRX       0x01

#define MGC_M_RXCSR1_CLRDATATOG     0x80
#define MGC_M_RXCSR1_FLUSHFIFO      0x10
#define MGC_M_RXCSR1_DATAERROR      0x08
#define MGC_M_RXCSR1_FIFOFULL       0x02
#define MGC_M_RXCSR1_RXPKTRDY       0x01

/* RXCSR in Peripheral mode */
#define MGC_M_RXCSR2_P_ISO          0x40
#define MGC_M_RXCSR1_P_SENTSTALL    0x40
#define MGC_M_RXCSR1_P_SENDSTALL    0x20
#define MGC_M_RXCSR1_P_OVERRUN      0x04


#endif /* #ifndef __USBPERIPHERAL_H_INCLUDED__ */
