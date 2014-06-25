//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Adapter.h
//! \brief  Miniport required Adapter related header file.
//!
//! This header File contains the various structures implementation and various
//! macros being used by miniport.
//!
//! \version  1.00 Aug 22nd 2006 File Created

#ifndef __EMAC_ADAPTER_H_INCLUDED__
#define __EMAC_ADAPTER_H_INCLUDED__

#include <windows.h>
#include "Emac_Queue.h"
#include "omap.h"
#include "omap_cpgmac_regs.h"
#include "ceddkex.h"

#ifdef DEBUG
//
// These defines must match the ZONE_* defines
//
#define ZONE_INIT         0
#define ZONE_CRITICAL     1
#define ZONE_INTERRUPT    2
#define ZONE_MESSAGE      3
#define ZONE_SEND         4
#define ZONE_RECEIVE      5
#define ZONE_INFO         6
#define ZONE_FUNCTION     7
#define ZONE_OID          8
#define ZONE_WARN         14
#define ZONE_ERRORS       15

//! Debug zones

#define DBG_INIT        DEBUGZONE(ZONE_INIT)
#define DBG_CRITICAL    DEBUGZONE(ZONE_CRITICAL)
#define DBG_INT         DEBUGZONE(ZONE_INTERRUPT)
#define DBG_MSG         DEBUGZONE(ZONE_MESSAGE)
#define DBG_TX          DEBUGZONE(ZONE_SEND)
#define DBG_RX          DEBUGZONE(ZONE_RECEIVE)
#define DBG_INFO        DEBUGZONE(ZONE_INFO)
#define DBG_FUNC        DEBUGZONE(ZONE_FUNCTION)
#define DBG_OID         DEBUGZONE(ZONE_OID)
#define DBG_WARN        DEBUGZONE(ZONE_WARN)
#define DBG_ERR         DEBUGZONE(ZONE_ERRORS)

#endif  // DEBUG

//! Media type, we use ethernet as Medium,
#define NIC_MEDIA_TYPE                NdisMedium802_3

//! Update the driver version number every time you release a new driver
//! The high word is the major version. The low word is the minor version.
#define NIC_VENDOR_DRIVER_VERSION       0x00010000

//! Information about NDIS Version we are conforming
#define EMAC_NDIS_MAJOR_VERSION          5
#define EMAC_NDIS_MINOR_VERSION          0
#define EMAC_NDIS_DRIVER_VERSION         0x500

//! Information about NDIS packets and miniport supported info.
#define MINIPORT_RESERVED_SIZE          256
#define NDIS_INDICATE_PKTS              256
#define EMAC_MAX_RXBUF_DESCS            256
#define EMAC_MAX_TX_BUFFERS             256
#define EMAC_MAX_TXBUF_DESCS            256
#define MAX_NUM_PACKETS_PER_SEND        256


//! Macro to find Minimum between two
#define MIN(a, b)                  ((a) < (b)? a : b)

//! Information about various events
#define EMAC_TX_TEARDOWN_EVENT   BIT(0)
#define EMAC_RX_TEARDOWN_EVENT   BIT(1)


//***********************************
//
//! \typedef EMAC_TXPKT
//! \brief Miniport Tx Packet Info. structure.
//
//************************************
typedef  struct __EMAC_TX_PKTS__
{
    struct __EMAC_TX_PKTS__*    m_pNext;
    PVOID                       m_PktHandle;
    QUEUE_T                     m_BufsList;

} EMAC_TXPKT,*PEMAC_TXPKT;

//***********************************
//
//! \typedef EMAC_TXBUF
//! \brief Miniport Tx Buffer Info. structure.
//
//************************************
typedef  struct __EMAC_TX_BUFS__
{
    struct __EMAC_TX_BUFS__*    m_pNext;
    PVOID                       m_BufHandle;
    UINT32                      m_BufLogicalAddress;
    UINT32                      m_BufPhysicalAddress;
    UINT32                      m_EmacBufDes;
    UINT32                      m_EmacBufDesPa;

} EMAC_TXBUF,*PEMAC_TXBUF;

//***********************************
//
//! \typedef EMAC_RXPKTS
//! \brief Miniport Rx Packet Info. structure.
//
//************************************
typedef  struct __EMAC_RX_PKTS__
{
    struct __EMAC_RX_PKTS__*    m_pNext;
    PVOID                       m_PktHandle;
    QUEUE_T                     m_BufsList;

} EMAC_RXPKTS,*PEMAC_RXPKTS;

//***********************************
//
//! \typedef EMAC_RXBUFS
//! \brief Miniport Rx Buffer Info. structure.
//
//************************************
typedef  struct __EMAC_RX_BUFS__
{
    struct __EMAC_RX_BUFS__*    m_pNext;
    PVOID                       m_BufHandle;
    UINT32                      m_BufLogicalAddress;
    UINT32                      m_BufPhysicalAddress;
    UINT32                      m_EmacBufDes;
    UINT32                      m_EmacBufDesPa;

} EMAC_RXBUFS,*PEMAC_RXBUFS;

//***********************************
//
//! \typedef EMAC_STATINFO
//! \brief Statistics Information Structure
//
//************************************

typedef  struct __EMAC_STATISTICS__
{
    UINT32    m_TxOKFrames;
    UINT32    m_RxOKFrames;
    UINT32    m_TxErrorframes;
    UINT32    m_RxErrorframes;
    UINT32    m_RxNoBufFrames;
    UINT32    m_RxAlignErrorFrames;
    UINT32    m_TxOneColl;
    UINT32    m_TxMoreColl;
    UINT32    m_TxDeferred;
    UINT32    m_TxMaxColl;
    UINT32    m_RxOverRun;

} EMAC_STATINFO ,*PEMAC_STATINFO;

//***********************************
//
//! \typedef LINKSTATUS
//! \brief Enumeration of Link Status
//
//************************************
typedef enum _LINK_STATUS
{
    UP,
    DOWN,
    INVALID
} LINKSTATUS;

//***********************************
//
//! \typedef MINIPORT_ADAPTER
//! \brief Miniport Adapter overlay structure
//
//************************************

typedef  struct __MINIPORT_ADAPTER__
{
    NDIS_HANDLE             m_AdapterHandle;      /// Handle given by NDIS when the Adapter registered itself.
    PEMACREGS               m_pEmacRegsBase;
    PEMACCTRLREGS           m_pEmacCtlRegs;
    ULONG                   m_EmacIRamBase;
    PEMACMDIOREGS           m_pMdioRegsBase;             //  Platform Specific - type might have to be changed to accomodate platform specific values
    UCHAR                   m_MACAddress[ETH_LENGTH_OF_ADDRESS];  //  Current Station address
    NDIS_HANDLE             m_RecvPacketPool;
    NDIS_HANDLE             m_RecvBufferPool;
    USHORT                  m_NumRxIndicatePkts;
    USHORT                  m_NumEmacRxBufDesc;
    USHORT                  m_MaxPacketPerTransmit;
    USHORT                  m_MaxTxEmacBufs;
    PEMAC_RXPKTS            m_pBaseRxPkts;
    PEMAC_RXBUFS            m_pBaseRxBufs;
    PEMAC_TXPKT             m_pBaseTxPkts;
    PEMAC_TXBUF             m_pBaseTxBufs;
    ULONG                   m_RxBufsBase;
    NDIS_PHYSICAL_ADDRESS   m_RxBufsBasePa;
    ULONG                   m_TxBufBase;
    NDIS_PHYSICAL_ADDRESS   m_TxBufBasePa;
    NDIS_SPIN_LOCK          m_Lock;
    NDIS_SPIN_LOCK          m_SendLock;
    NDIS_SPIN_LOCK          m_RcvLock;
    OMAP_DEVICE             m_device;
    NDIS_MINIPORT_INTERRUPT m_RxIntrInfo;      //  From NdisMRegisterInterrupt(..)
    USHORT                  m_RxIntrVector;
    NDIS_MINIPORT_INTERRUPT m_TxIntrInfo;      //  From NdisMRegisterInterrupt(..)
    USHORT                  m_TxIntrVector;
    NDIS_MINIPORT_INTERRUPT m_LinkIntrInfo;      //  From NdisMRegisterInterrupt(..)
    USHORT                  m_LinkIntrVector;
    BOOLEAN                 m_IsInterruptSet;     //  Attached to interrupt using NdisMRegisterInterrupt(...)
    PEMACDESC               m_pCurEmacRcvBufDesc;
    PNDIS_PACKET            m_pCurRcvPktDesc;
    PEMAC_RXBUFS            m_pCurEmacRxBuf;
    NDIS_HARDWARE_STATUS    m_HwStatus;
    LINKSTATUS              m_LinkStatus;
    DWORD                   m_ActivePhy;
    DWORD                   m_PacketFilter;

    QUEUE_T                 m_RxPktPool;
    QUEUE_T                 m_RxBufsPool;
    QUEUE_T                 m_TxBufInfoPool;
    QUEUE_T                 m_TxPktsInfoPool;
    QUEUE_T                 m_TxPostedPktPool;

    /*
     * List of Multicast addresses are maintained in the HAL for receive
     * configuration.
     */

    UINT8                 m_MulticastTable[EMAC_MAX_MCAST_ENTRIES][ETH_LENGTH_OF_ADDRESS];
    UINT32                m_NumMulticastEntries;
    EMAC_STATINFO         m_EmacStatInfo;
    DWORD                 m_Events;

} MINIPORT_ADAPTER, *PMINIPORT_ADAPTER, *PEMAC_ADAPTER;

void EmacEnableInterrupts(PEMAC_ADAPTER  pAdapter);
void EmacDisableInterrupts(PEMAC_ADAPTER  pAdapter);

/* External HW init - must be defined in BSP */
BOOL EthHwInit(void);
void SocAckInterrupt(DWORD flag);
#define ACK_MISC    (0x1)
#define ACK_RX      (0x2)
#define ACK_TX      (0x8)

extern const EMAC_MEM_LAYOUT g_EmacMemLayout;


// EMAC_RX/TX
// RX/TX descriptor related defines 
#define EMAC_RX_DESC_BASE           (g_EmacMemLayout.EMAC_RAM_OFFSET + 0)
#define EMAC_TX_DESC_BASE           (EMAC_RX_DESC_BASE + (4*1024))



#if 0
#undef DEBUGMSG
#define DEBUGMSG(x,y) RETAILMSG(TRUE,y)
#endif

#endif /* #ifndef __EMAC_ADAPTER_H_INCLUDED__*/
