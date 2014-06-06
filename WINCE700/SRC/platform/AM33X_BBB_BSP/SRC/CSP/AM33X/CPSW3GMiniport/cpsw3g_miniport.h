//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#ifndef __CPSW3G_MINIPORT_H_INCLUDED__
#define __CPSW3G_MINIPORT_H_INCLUDED__

#pragma warning (push, 3)
#include <Ndis.h>
#pragma warning (pop)
#include <windows.h>
#include "constants.h"
#include "Am33xCpsw3gRegs.h"
#include "cpsw3g_queue.h"
#include "cpgmacisr.h"

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
#define ZONE_HAL          9
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
#define NIC_MEDIA_TYPE               NdisMedium802_3

//! Update the driver version number every time you release a new driver
//! The high word is the major version. The low word is the minor version.
#define NIC_VENDOR_DRIVER_VERSION    0x00010000

//! Information about NDIS Version we are conforming
#define CPSW3G_NDIS_MAJOR_VERSION    5
#define CPSW3G_NDIS_MINOR_VERSION    0
#define CPSW3G_NDIS_DRIVER_VERSION   0x500

#define CPSW3G_DRIVER_MAJOR_VERSION  0
#define CPSW3G_DRIVER_MINOR_VERSION  0

//! Information about NDIS packets and miniport supported info.
#define CPSW3G_MAX_RX_BUFFERS        256
#define MINIPORT_RESERVED_SIZE       CPSW3G_MAX_RX_BUFFERS
#define CPSW3G_MAX_RXBUF_DESCS       CPSW3G_MAX_RX_BUFFERS 
#define NDIS_INDICATE_PKTS           CPSW3G_MAX_RX_BUFFERS

#define CPSW3G_MAX_TX_BUFFERS        256
#define CPSW3G_MAX_TXBUF_DESCS       CPSW3G_MAX_TX_BUFFERS 
#define MAX_NUM_PACKETS_PER_SEND     CPSW3G_MAX_TX_BUFFERS

#define CPSW3G_RX_CHANNEL_PER_GMAC   2
#define NIC_TAG                      ((ULONG)'003f')

//! Macro to find Minimum between two
#define MIN(a, b)                    ((a) < (b)? a : b)

//! Information about various events
#define CPSW3G_TX_TEARDOWN_EVENT(ch) BIT0 << (8 + ch)
#define CPSW3G_RX_TEARDOWN_EVENT(ch) BIT0 << ch
#define CPSW3G_TEARDOWN              0xfffffffc

#define CONFIG_CPGMAC_NOPHY          9999

#define CPSW_UNIT_SWITCH	         2
static unsigned char broadcast_mac[ETH_LENGTH_OF_ADDRESS] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static unsigned char mcast_802_1x_mac[ETH_LENGTH_OF_ADDRESS] = {
    0x01, 0x80, 0xc2, 0x00, 0x00, 0x03
};

static unsigned char g_default_mac_address[CPSW3G_NUM_MAC_PORTS][ETH_LENGTH_OF_ADDRESS] = {
    {0x00, 0xe0, 0x0e, 0xab, 0xcd, 0xee},
#if CPSW3G_NUM_MAC_PORTS == 2
    {0x00, 0xe0, 0x0e, 0xab, 0xcd, 0xef},
#endif
};
/* CPMAC Success code */
#define CPSW3G_SUCCESS               NDIS_STATUS_SUCCESS
#define CPSW3G_INTERNAL_FAILURE      -1

//***********************************
//
//! \typedef LINKSTATUS
//! \brief Enumeration of Link Status
//
//************************************
typedef enum __LINK_STATUS__
{
    LINK_DOWN,
    LINK_UP,		
    LINK_INVALID
} LINKSTATUS;

typedef enum __PHY_STATE__
{
    INITING,
    FOUND,        
    AUTO_NEG,        
    LINK_WAIT,
    LINKED,
    STOP_WAIT,
    STOPPED
}PHY_STATE;

typedef enum __phy_duplex__
{
    HALF_DUPLEX,
    FULL_DUPLEX
}PHY_DUPLEX;

typedef struct __phy_driver__
{
    UINT32 id;
    char *name;
    BOOL phy_poll;
    void (*init)(void *param);
    int (*config_autoneg)(void *param);
    int (*read_status)(void *param);
    void(*reg_dump)(void *param);
}PHY_DRIVER;

typedef struct __phy_device__
{
    UINT32				channel;
    UINT16				phy_addr;
    BOOL				phy_poll;
    PHY_STATE			phy_state;
    UINT32				timeout;
    UINT32				retry_count;
    UINT32				speed_cap;
    UINT32				remote_speed_cap;
    BOOL				speed_change_request;    
    BOOL				speed_force_default;    
    LINKSTATUS			link;
    UINT32				link_speed;
    PHY_DUPLEX			link_duplex;  
    PHY_DRIVER			*driver;

    void 				*handle;

    /* Timers */
    NDIS_MINIPORT_TIMER	Phy_timer;
        
}PHY_DEVICE, *PPHY_DEVICE;


typedef enum _phy_speed_cap_
{
    PHY_1000_FD=0,
    PHY_1000_HD,
    PHY_100_FD,
    PHY_100_HD,
    PHY_10_FD,
    PHY_10_HD
}PHY_SPEED_CAP, *PPHY_SPEED_CAP;

#define CPSW3G_SPEED_CAP_DEFAULT 0x3F

/**
 *  \brief CPDMA Init Configuration
 *
 *  Configuration information provided to DDC layer during initialization.
 *  DDA gets the config information from the OS/PAL layer and passes the relevant
 *  config to the DDC during initialization. The config info can come from
 *  various sources - static compiled in info, boot time (ENV, Flash) info etc.
 */

typedef struct
{
    UINT8	portPri;          /**< Port VLAN priority (7 is highest) */
    BOOL	portCfi;          /**< Port CFI bit */
    UINT32	portVID;          /**< Port VLAN ID */
    UINT8	bcastLimit;       /**< Broadcast Packet Rate Limit */
    UINT8	mcastLimit;       /**< Multicast Packet Rate Limit */
    UINT8	VIDIngressCheck;    
    UINT8	DropUntagged;
    
}Cpsw_PortHwConfig;

typedef struct
{
    Cpsw_PortHwConfig hw_config;
} CpdmaPortConfig;

/**
 *  \brief CPGMAC SL Init Configuration
 *
 *  Configuration information provided to DDC layer during initialization.
 *  DDA gets the config information from the OS/PAL layer and passes the relevant
 *  config to the DDC during initialization. The config info can come from
 *  various sources - static compiled in info, boot time (ENV, Flash) info etc.
 */
typedef struct
{
    Cpsw_PortHwConfig hw_config;
    
    BOOL        internalPhy;
    UINT16      PhyAddr;            /**< Phy Mask for this CPMAC Phy  */
    UINT32      LinkSpeed;
    PHY_DUPLEX  LinkDuplex;
    UINT32      rxMaxlen;           /**< Rx MaxFrame Length (default 1518, max 2047) */
    BOOL        passCrc;            /**< If set, MAC won't generate new CRC per pkt */
    
} CpgmacPortConfig;





//***********************************
//
//! \typedef EMAC_TXPKT
//! \brief Miniport Tx Packet Info. structure.
//
//************************************
typedef  struct __CPSW3G_TX_PKTS__
{
    struct __CPSW3G_TX_PKTS__*  pNext;
    PVOID                       PktHandle;
    QUEUE_T                     BufsList;

} CPSW3G_TXPKT,*PCPSW3G_TXPKT;

//***********************************
//
//! \typedef EMAC_TXBUF
//! \brief Miniport Tx Buffer Info. structure.
//
//************************************
typedef  struct __CPSW3G_TX_BUFS__
{
    struct __CPSW3G_TX_BUFS__*  pNext;
    PVOID                       BufHandle;
    UINT32                      BufLogicalAddress;
    UINT32                      BufPhysicalAddress;
    UINT32                      BufDes;
    UINT32                      BufDesPa;

} CPSW3G_TXBUF,*PCPSW3G_TXBUF;

//***********************************
//
//! \typedef EMAC_RXPKTS
//! \brief Miniport Rx Packet Info. structure.
//
//************************************
typedef  struct __CPSW3G_RX_PKTS__
{
    struct __CPSW3G_RX_PKTS__*  pNext;
    PVOID                       PktHandle;
    QUEUE_T                     BufsList;
    UINT32                      RxChannel;

} CPSW3G_RXPKTS,*PCPSW3G_RXPKTS;

//***********************************
//
//! \typedef EMAC_RXBUFS
//! \brief Miniport Rx Buffer Info. structure.
//
//************************************
typedef  struct __CPSW3G_RX_BUFS__
{
    struct __CPSW3G_RX_BUFS__*  pNext;
    PVOID                       BufHandle;
    UINT32                      BufLogicalAddress;
    UINT32                      BufPhysicalAddress;
    UINT32                      BufDes;
    UINT32                      BufDesPa;

} CPSW3G_RXBUFS,*PCPSW3G_RXBUFS;

typedef struct __VLAN_ETHHDR__
{
    UCHAR   dest[ETH_LENGTH_OF_ADDRESS];
    UCHAR   src[ETH_LENGTH_OF_ADDRESS];
    UINT16  vlan_proto;
    UINT16  vlan_tag;
}VLAN_ETHHDR, *PVLAN_ETHHDR;

//***********************************
//
//! \typedef CPSW3G_STATINFO
//! \brief Statistics Information Structure
//
//************************************

typedef  struct __CPSW3G_STATISTICS__
{
    ULONG64    TxOKFrames;
    ULONG64    RxOKFrames;
    ULONG64    TxErrorframes;
    ULONG64    RxErrorframes;
    ULONG64    RxNoBufFrames;
    ULONG64    RxAlignErrorFrames;
    ULONG64    TxOneColl;
    ULONG64    TxMoreColl;
    ULONG64    TxDeferred;
    ULONG64    TxMaxColl;
    ULONG64    RxOverRun;

    ULONG64    RxBroadcastFrame;
    ULONG64    TxBroadcastFrame;
    ULONG64    RxMulticastFrame;
    ULONG64    TxMulticastFrame;
    

} CPSW3G_STATINFO ,*PCPSW3G_STATINFO;

typedef struct __CPSW3G_SW_STATS__
{
    /* Transmit side */
    UINT32    tx_in;
    UINT32    tx_complete;
    UINT32    tx_out_of_descs;
    UINT32    tx_eoq;
    UINT32    tx_false_eoq;    
    UINT32    tx_undersized;
    UINT32    tx_teardown_discard;
    UINT32    tx_copies;
    UINT32    tx_sw_vlan_process;
    UINT32    tx_oversize_pkt;

    /* Receive side */
    UINT32    rx_in;
    UINT32    rx_out_of_descs;
    UINT32    rx_complete;
    UINT32    rx_eoq;
    UINT32    rx_teardown_discard;
    UINT32    rx_copies;
    UINT32    rx_indicated;
    UINT32    rx_vlan_tagged_pkt;    
    UINT32    rx_out_of_pkt;
    UINT32    rx_invalid_desc;    
}CPSW3G_SW_STATS, *PCPSW3G_SW_STATS;


typedef struct 
{
    BOOL intPhy;
    BOOL promiscousEnable;
    BOOL broadcastEnable;
    BOOL multicastEnable;
    UINT32 PhyMask;	
}CPSW3G_MODE;


typedef enum __CPSW3G_DRIVER_MODE__
{
    CPSW3G_ESWITCH,
    CPSW3G_CPGMAC,
    CPSW3G_CPGMAC_KITL
}CPSW3G_DRIVER_MODE;

typedef enum __CPSW3G_PORT_STATE__
{
    PORT_UNINITIALIZED = 0,       /**< Uninitialized state */
    PORT_KITL, 
    PORT_OPENED,                  /**< Port in open state */
    PORT_CLOSE_IN_PROGRESS,       /**< Port close/teardown in progress */
    PORT_CLOSED                   /**< Port is closed, resources deallocated - require this ? */
} CPSW3G_PORT_STATE;

typedef struct __CPSW3G_PORT_INFO__
{
    UINT32             PortNum;              /**< DDC_NetChInfo: Channel number */
    UINT32             RxPortChanMask;       /* Mask of channels used for this Port */
    UINT32             Tx_channel;    
} CPSW3G_PORT_INFO, *PCPSW3G_PORT_INFO;

/**
 *  \brief CPSW3G Init Configuration
 *
 *  Configuration information provided to DDC layer during initialization.
 *  DDA gets the config information from the OS/PAL layer and passes the
 *  relevant config to the DDC during initialization. The config info can come
 *  from various sources - static compiled in info, boot time (ENV, Flash) info
 *  etc.
 */
typedef struct __CPSW3G_CONFIG__
{
    USHORT				InterruptVector;

    USHORT				RxThreshInterrupt;
    USHORT				RxInterrupt;
    USHORT				TxInterrupt;
    USHORT				MiscInterrupt;

    BOOL				vlanAware;			/**< VLAN aware mode */
    CPSW3G_DRIVER_MODE	cpsw3g_mode;
    UINT32				KITL_port;
    UINT32				ALE_Agingtimer; 
    UINT8				MacAuth;
    UINT8				UnknownUntagEgress;
    UINT8				UnknownRegMcastFloodMask;
    UINT8				UnknownMcastFloodMask;
    UINT8				UnknownMemberList;    
    UINT8				Enable_8021x;
    UINT32				SysIntr;
    BOOL				rxVlanEncap;		/**< Port 0 VLAN Encapsulation */
    UINT32				rx_serviceMax;
    BOOL				sw_vlantagging;
    BOOL				ale_bypass;
    BOOL				KITL_enable;
    UINT8				stats_port_mask;
    UINT32				ale_prescale;
    BOOL				Speed_match;    
    
    /* Misc */
    UINT32				resetLine;			/**< Reset controller line # */
    UINT32				mdioBusFrequency;	/**< Bus frequency for the MII module */
    UINT32				mdioClockFrequency;	/**< Clock frequency for MDIO link */
    UINT32				mdioTickMSec;		/**< DDC MDIO Tick count in milliSeconds */

    /* Port specific items */
    CpgmacPortConfig	macCfg[CPSW3G_NUM_MAC_PORTS];     /**< CPGMAC SL Init Configuration */
    CpdmaPortConfig		dmaCfg;				/**< DMA Init Configuration */

    /* ALE specific items */
    UINT32				alePrescale;		/**< ALE Prescale */

} CPSW3G_CONFIG, *PCPSW3G_CONFIG;



//***********************************
//
//! \typedef MINIPORT_ADAPTER
//! \brief Miniport Adapter overlay structure
//
//************************************

typedef  struct __CPSW3G_ADAPTER__
{
    PCPSW3G_REGS			pCpsw3gRegsBase;
    PCPSW3G_MDIO_REGS		pMdioRegsBase;  //  Platform Specific - type might have to be c
                                            //  hanged to accomodate platform specific values
    PCPSW3G_VTP_REGS		pVtp0CtrlBase;
    PCPSW3G_VTP_REGS		pVtp1CtrlBase;
	
    NDIS_HANDLE				AdapterHandle; // Handle given by NDIS when the Adapter registered itself. 
    NDIS_HANDLE				ConfigurationContext;
    UCHAR					MACAddress[ETH_LENGTH_OF_ADDRESS];  //  Current Station address
    
    CPSW3G_CONFIG			Cfg;

    CPSW3G_PORT_INFO		Cpsw3gPort[CPSW3G_NUM_PORTS];
    PCPSW3G_PORT_INFO		curr_port_info;
    UINT32					ActiveCpgmac;
    PHY_DEVICE				PhyDevice[CPSW3G_NUM_MAC_PORTS];
    LINKSTATUS				link;	
    UINT32					link_speed;
    //UINT32                            link_duplex;
    BOOL					KITL_enable;
	
    USHORT					NumRxIndicatePkts;
    USHORT					NumRxMacBufDesc;
    USHORT					MaxPacketPerTransmit;
    USHORT					MaxTxMacBufDesc;
    
    PCPSW3G_RXPKTS			pBaseRxPkts[CPDMA_MAX_CHANNELS];
    PCPSW3G_RXBUFS			pBaseRxBufs[CPDMA_MAX_CHANNELS];
    PCPSW3G_TXPKT			pBaseTxPkts;
    PCPSW3G_TXBUF			pBaseTxBufs;
    ULONG					RxBufsBase[CPDMA_MAX_CHANNELS];
    NDIS_PHYSICAL_ADDRESS	RxBufsBasePa[CPDMA_MAX_CHANNELS];
    ULONG					TxBufBase;
    NDIS_PHYSICAL_ADDRESS	TxBufBasePa;
    ULONG					RxBDBase[CPDMA_MAX_CHANNELS];
    NDIS_PHYSICAL_ADDRESS	RxBDBasePa[CPDMA_MAX_CHANNELS];
    ULONG					TxBDBase;
    NDIS_PHYSICAL_ADDRESS	TxBDBasePa;

    NDIS_HANDLE				RecvPacketPool[CPDMA_MAX_CHANNELS];
    NDIS_HANDLE				RecvBufferPool[CPDMA_MAX_CHANNELS];

    NDIS_SPIN_LOCK        	Lock;
    NDIS_SPIN_LOCK			SendLock;
    NDIS_SPIN_LOCK			RcvLock;

    NDIS_HARDWARE_STATUS	HwStatus;


//    NDIS_MINIPORT_INTERRUPT	InterruptInfo;      //  From NdisMRegisterInterrupt(..)
    NDIS_MINIPORT_INTERRUPT	RxThreshInterruptInfo;      //  From NdisMRegisterInterrupt(..)
    NDIS_MINIPORT_INTERRUPT	RxInterruptInfo;
    NDIS_MINIPORT_INTERRUPT	TxInterruptInfo;
    NDIS_MINIPORT_INTERRUPT	MiscInterruptInfo;

    PCPSW3G_RXBUFS			pCurRxBuf;


    QUEUE_T                	RxPktPool[CPDMA_MAX_CHANNELS];
    QUEUE_T              	RxBufsPool[CPDMA_MAX_CHANNELS];
    QUEUE_T            		TxBufInfoPool;
    QUEUE_T            		TxPktsInfoPool;
    QUEUE_T					TxPostedPktPool;

    DWORD					PacketFilter;

    /*
     * List of Multicast addresses are maintained in the HAL for receive
     * configuration.
     */
    UINT8					MulticastTable[CPSW3G_MAX_MCAST_ENTRIES][ETH_LENGTH_OF_ADDRESS];
    UINT32					NumMulticastEntries;
    
    CPSW3G_STATINFO			Cpsw3gStatInfo;
    CPSW3G_SW_STATS			Cpsw3g_sw_stats;

    /* Timers */
    NDIS_MINIPORT_TIMER		ALE_timer;
    
    DWORD					Events;

    // SPF
    HANDLE					Spf_intr_event;

    //
    //  ISR handle from LoadIntChainHandler()
    //

    HANDLE					hISR;

    CPGMAC_ISR_INFO			IsrInfo;  // Shared with cpgmacisr.dll

    NDIS_DEVICE_POWER_STATE	CurrentPowerState;

    BOOL					ResetPending;

    PCPSW_SS_REGS           pSsRegsBase;

} CPSW3G_ADAPTER, *PCPSW3G_ADAPTER;


#endif /* #ifndef __CPGMAC_H_INCLUDED__*/
