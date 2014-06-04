//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#ifndef _CPSW3G_CFG_H_
#define _CPSW3G_CFG_H_
#include <ndis.h>


// delay used for link detection to minimize the init time
// change this value to match your hardware 
#define CPSW3G_AGE_OUT_DELAY					200000  /* in milli-seconds*/

//CPSW3G default config
//#define CPSW3G_DEFAULT_INTERRUPT_VECTOR			19

#if defined(AM387X_BSP) || defined(AM33X_BSP)
#define CPSW3G_DEFAULT_RX_THRESH_INTERRUPT      40
#define CPSW3G_DEFAULT_RX_INTERRUPT             41
#define CPSW3G_DEFAULT_TX_INTERRUPT             42
#define CPSW3G_DEFAULT_MISC_INTERRUPT           43
#endif

#define CPSW3G_DEFAULT_VLANAWARE				1
#define CPSW3G_DEFAULT_MODE						2
#define CPSW3G_DEFAULT_VID_INGRESS_CHECK		0
#define CPSW3G_DEFAULT_DROP_UNTAGGED			0 
#define CPSW3G_DEFAULT_MAC_AUTH					0 
#define CPSW3G_DEFAULT_UNKNOWN_UNTAG_EGRESS		0x7
#define CPSW3G_DEFAULT_UNKNOWN_REG_MCAST_MASK	0x3
#define CPSW3G_DEFAULT_UNKNOWN_MCAST_MASK		0x3
#define CPSW3G_DEFAULT_UNKNOWN_MEMBER_LIST		0x7
#define CPSW3G_DEFAULT_8021X					0
#define CPSW3G_DEFAULT_RX_SERVICE				200
#define CPSW3G_DEFAULT_RX_SERVICE_MAX			256
#define CPSW3G_DEFAULT_ALE_BYPASS				0
#define CPSW3G_DEFAULT_KITL_ENABLE				0
#define CPSW3G_DEFAULT_SW_VLANTAGGING			0
#define CPSW3G_DEFAULT_STATS_PORT_MASK			7
#define CPSW3G_DEFAULT_ALE_PRESCALE				0xF4240

#define CPSW3G_DEFAULT_BCAST_RATELIMIT			0
#define CPSW3G_DEFAULT_MCAST_RATELIMIT			0

// Default Dma config
#define CPSW3G_DEFAULT_DMA_PORTPRI              0
#define CPSW3G_DEFAULT_DMA_PORTCFI              0
#define CPSW3G_DEFAULT_DMA_PORTVID              0 
#define CPSW3G_DEFAULT_TXPTYPE                  1

// Default MAC sliver config
#define CPSW3G_DEFAULT_MAC_PORTPRI              0
#define CPSW3G_DEFAULT_MAC_PORTCFI              0
#define CPSW3G_DEFAULT_MAC_PORTVID              0 
#define CPSW3G_DEFAULT_PASSCRC                  0
#define CPSW3G_DEFAULT_RXCMFEN                  0
#define CPSW3G_DEFAULT_RXCSFEN                  0
#define CPSW3G_DEFAULT_EXTEN                    1 
#define CPSW3G_DEFAULT_GIGFORCE                 0
#define CPSW3G_DEFAULT_IFCTLB                   0
#define CPSW3G_DEFAULT_IFCTLA                   1
#define CPSW3G_DEFAULT_MAC_CMDIDLE              0
#define CPSW3G_DEFAULT_TXSHORTGAPEN             0
#define CPSW3G_DEFAULT_GIGABITEN                0
#define CPSW3G_DEFAULT_TXPACINGEN               0
#define CPSW3G_DEFAULT_GMIIEN                   1
#define CPSW3G_DEFAULT_TXFLOWEN                 1
#define CPSW3G_DEFAULT_RXFLOWEN                 1
#define CPSW3G_DEFAULT_LOOPBKEN                 0
#define CPSW3G_DEFAULT_RXMAXLEN                 1518
#define CPSW3G_DEFAULT_FD                       1
#define AVALANCHE_DEFAULT_MDIOBUSFREQ           0
#define AVALANCHE_DEFAULT_MDIOCLOCKFREQ         0
#define AVALANCHE_DEFAULT_CPMACBUSFREQ          125000000
#define AVALANCHE_DEFAULT_MDIOTICK              1000

typedef enum __CFG_TYPE__
{

    CFG_OPTIONAL,
    CFG_REQUIRED,
    CFG_NOT_SUPPORTED
}CFG_TYPE;

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif

typedef struct __CPSW3G_REG_CFG__
{
    NDIS_STRING         keyword;    // key word in .reg file
    CFG_TYPE            cfg_type;   // 1: required, 0: optional, 2: not currently supported
    NDIS_PARAMETER_TYPE type;
    UINT32              Cfg_field;  // offset of cfg field in struct CPSW3G_CONFIG
    UINT32              FieldSize;              
    UINT32              Default;
    UINT32              Min;                    
    UINT32              Max;                    
    
}CPSW3G_REG_CFG;

typedef struct __CPSW3G_REG_ALE_CFG__
{
    NDIS_STRING   keyword;
    UINT32        Cfg_field;            
    UINT32        FieldSize; 
}CPSW3G_REG_ALE_CFG;

typedef struct __CPSW3G_REG_VLAN_CFG__
{
    NDIS_STRING   keyword;
    UINT32        Cfg_field;            
    UINT32        FieldSize; 
    UINT32        Min;                    
    UINT32        Max;                        
}CPSW3G_REG_VLAN_CFG;

#endif /* _CPSW3G_CFG_H_ */



