// All rights reserved Texas Instruments, Inc. 2011
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2009.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  am387x_usbcdma.h
//
//  AM387x USB CDMA Driver API.
//  

#ifndef __AM387X_USBCDMA_H__
#define __AM387X_USBCDMA_H__

//------------------------------------------------------------------------------

#include <windows.h>
#include <ceddk.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Queue Manager Definitions */
#define USB_CPPI_XXCMPL_QMGR         0

/* Queue Number Definitions */
#define USB_CPPI_RX_QNUM             0  // Rx Queues
#define USB_CPPI_TX_QNUM             32 // Tx Queues
#define USB_CPPI_TX0CMPL_QNUM        93 // Tx Completion Queue   port0
#define USB_CPPI_TX1CMPL_QNUM       125 // Tx Completion Queues port1
#define USB_CPPI_RX0CMPL_QNUM       109 // Rx Completion Queues port0
#define USB_CPPI_RX1CMPL_QNUM       141 // Rx Completion Queues port1
#define USB_CPPI_TDFREE_QNUM         31 // Teardown free descriptor Queue
//#define USB_CPPI_TDCMPL_QNUM        108 // Teardown free descriptor Queue

#define QUEUE_N_BITMASK(x)			 (1 << (x % 32))

#define USB_CPPI_PEND2_QMSK				0xE0000000 // 95-64
#define USB_CPPI_PEND3_QMSK				0xEFFFFFFF // 127-96
#define USB_CPPI_PEND4_QMSK				0x0FFFEFFF // 159-128

#define USB_CPPI_PEND2_QMSK_P0			0xE0000000 // 95-64
#define USB_CPPI_PEND3_QMSK_P0			0x0FFFEFFF // 127-96
#define USB_CPPI_PEND3_QMSK_P1			0xE0000000 // 127-96
#define USB_CPPI_PEND4_QMSK_P1			0x0FFFEFFF // 159-128

/* CPPI 4.1 Descriptor Macros */
#define USB_CPPI41_DESC_TYPE_SHIFT    27
#define USB_CPPI41_DESC_TYPE_MASK     (0x1f << USB_CPPI41_DESC_TYPE_SHIFT)
#define USB_CPPI41_DESC_TYPE_HOST     16
#define USB_CPPI41_DESC_TYPE_TEARDOWN 19
#define USB_CPPI41_DESC_WORDS_SHIFT   22
#define USB_CPPI41_PKT_TYPE_SHIFT     26
#define USB_CPPI41_PKT_TYPE_USB       5
#define USB_CPPI41_PKT_RETPLCY_SHIFT  15
#define USB_CPPI41_PKT_RETPLCY_FULL   0
#define USB_CPPI41_PKT_FLAGS_ZLP      (1 << 19)
#define USB_CPPI41_DESC_LOC_SHIFT     14
#define USB_CPPI41_DESC_LOC_OFFCHIP   0
#define USB_CPPI41_PKT_RETQMGR_SHIFT  12
#define USB_CPPI41_PKT_RETQ_SHIFT     0
#define USB_CPPI41_HD_BUF_LENGTH_MASK 0x003fffff
#define USB_CPPI41_HD_PKT_LENGTH_MASK 0x003fffff


#define USB_CPPI_TD_COUNT_POW2         (5)
#define USB_CPPI_TD_COUNT              (1 << USB_CPPI_TD_COUNT_POW2)
#define USB_CPPI_TD_SIZE_POW2          (5)
#define USB_CPPI_TD_SIZE               (sizeof(TEARDOWN_DESCRIPTOR))
#define USB_CPPI_TD_POOL_SIZE          (USB_CPPI_TD_COUNT * USB_CPPI_TD_SIZE)

#if USB_CPPI_TD_COUNT_POW2 < 5
#error USB_CPPI_TD_COUNT_POW2 < 5
#endif

#if USB_CPPI_TD_SIZE_POW2 < 5
#error USB_CPPI_TD_SIZE_POW2 < 5
#endif


typedef struct _TEARDOWN_DESCRIPTOR
{
    UINT32 DescInfo;
    UINT32 reserved[7];
} TEARDOWN_DESCRIPTOR;


#define CPPI_HD_COUNT_POW2          (10)                           /* 1024 host descriptors */
#define CPPI_HD_COUNT               (1 << CPPI_HD_COUNT_POW2)
#define CPPI_HD_SIZE                (sizeof(HOST_DESCRIPTOR))
#define CPPI_HD_ALIGN               ((CPPI_HD_SIZE + 0xf) & ~0xf) /* Minimum 16 byte alignment */
#define CPPI_HD_POOL_SIZE           (CPPI_HD_COUNT * CPPI_HD_SIZE)
#if CPPI_HD_COUNT_POW2 < 5
#error CPPI_HD_COUNT_POW2 must be at least 5
#endif

// CPPI host descriptor (for packet and buffer descriptors)
typedef struct _HOST_DESCRIPTOR
{
    /* Hardware Overlay */

    /* Packet Info */
    UINT32 DescInfo;            /**< Desc type, proto specific word cnt, pkt len (valid only in Host PD)*/
    UINT32 TagInfo;             /**< Source tag (31:16), Dest Tag (15:0) (valid only in Host PD)*/
    UINT32 PacketInfo;          /**< pkt err state, type, proto flags, return info, desc location */

    /* Buffer Info */
    UINT32 BuffLen;             /**< Number of valid data bytes in the buffer (21:0) */
    UINT32 BuffPtr;             /**< Pointer to the buffer associated with this descriptor */

    /* Linking Info */
    UINT32 NextPtr;             /**< Pointer to the next buffer descriptor (physical address) */

    /* Original buffer Info */
    UINT32 OrigBuffLen;         /**< Original buffer size */
    UINT32 OrigBuffPtr;         /**< Original buffer pointer */

    /* SW Data */
    UINT32 SWData[2];

    /* Protocol Specific Info */
    struct _HOST_DESCRIPTOR* next; /**<Next(software) Host Descriptor pointer*/
    UINT32                   addr; /**<Physical Address of this Host */

    UINT32 TagInfo2;            /* CPPI Channel [8:4], End-Point Number [3:0] */
    UINT32 Index;               /* Index in a set of HDs making up a single ISO transfer (1 based) */
    UINT32 pad[2];              /* Padding to Align to 16-byte boundary */
} HOST_DESCRIPTOR;

HANDLE USBCDMA_RegisterUsbModule(    /* Returns USB module handle or NULL on failure */
    UINT16 nHdCount,                 /* [IN]  Number of host descriptors to allocate (power of 2, >= 32, <= 4096 */
    UINT8 cbHdSize,                  /* [IN]  Size of a host descriptor, in bytes (power of 2 and >= 32) */
    PHYSICAL_ADDRESS *ppaHdPool,     /* [OUT] Physical address of the host descriptor pool */
    VOID **ppvHdPool,                /* [OUT] Virtual address of the descriptor pool */
    VOID (*callback)(PVOID),         /* Pointer to the completion callback function */
    VOID *param,                     /* Parameter for the completion callback parameter */
    UINT32 dwUsbPortIndex			 /* port Index 1 or 2 */
    );

BOOL USBCDMA_DeregisterUsbModule(    /* Returns TRUE on success or FALSE on failure */
    HANDLE hUsbModule                /* Handle of the USB module deregistering itself */
    );

VOID USBCDMA_KickCompletionCallback( /* Returns nothing */
    HANDLE hUsbModule                /* Handle of the USB module kicking the callback */
    );

UINT32 USBCDMA_DescriptorVAtoPA(     /* Returns the physical address of va or zero on failure */
    HANDLE hUsbModule,               /* Handle of the USB module to translate for */
    VOID *va                         /* Virtual address of descriptor to translate */
    );

VOID * USBCDMA_DescriptorPAtoVA(     /* Returns the virtual address of pa or NULL on failure */
    HANDLE hUsbModule,               /* Handle of the USB module to translate for */
    UINT32 pa                        /* Physical address of descriptor to translate */
    );

VOID USBCDMA_ConfigureScheduleRx( /* Returns nothing */
    UINT32  eP,                   /* endpoint to configure */
    BOOL enable                   /* enable endpoint in shceduler */
    );
VOID USBCDMA_ConfigureScheduleTx( /* Returns nothing */
    UINT32  eP,                   /* endpoint to configure */
    BOOL enable                   /* enable endpoint in shceduler */
    );
VOID USBCDMA_ConfigureScheduler( /* Returns nothing */
    VOID                /* Configure scheduler for Rx usage (Tx always on) */
    );

VOID USBCDMA_EnableClocks(
	UINT32 port,		/* usb port 0 or 1 */
	BOOL   enable		/* enable USB clocks */
	);

#ifdef __cplusplus
}
#endif

#endif
