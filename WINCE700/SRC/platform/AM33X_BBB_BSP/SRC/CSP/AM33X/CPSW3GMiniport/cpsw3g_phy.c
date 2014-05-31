//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2011-2012
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include "Am33xCpsw3gRegs.h"
#include "cpsw3g_miniport.h"
#include "cpsw3g_proto.h"
#include "winbase.h"

#define CPSW3G_PHY_DELAY			10
#define CPSW3G_PHY_LINK_WAIT_DELAY	20
#define CPSW3G_PHY_LINKED_DEAY		200

#define PHY_NOT_FOUND  0xFFFF    /*  Used in Phy Detection */
#define PHY_DRIVER_NUM 4

/****************************************************************************/
/*                                                                          */
/*         P H Y   R E G I S T E R  D E F I N I T I O N S                   */
/*                                                                          */
/****************************************************************************/
#define PHY_CONTROL_REG                       0
  #define MII_PHY_RESET           (1<<15)
  #define MII_PHY_LOOP            (1<<14)
  #define MII_PHY_100             (1<<13)
  #define MII_AUTO_NEGOTIATE_EN   (1<<12)
  #define MII_PHY_PDOWN           (1<<11)
  #define MII_PHY_ISOLATE         (1<<10)
  #define MII_RENEGOTIATE         (1<<9)
  #define MII_PHY_FD              (1<<8)
  #define MII_PHY_1000            (1<<6)

#define PHY_STATUS_REG                        1
  #define MII_NWAY_COMPLETE       (1<<5)
  #define MII_NWAY_CAPABLE        (1<<3)
  #define MII_PHY_LINKED          (1<<2)

#define PHY_IDENT_REG                         2
#define PHY_IDENT2_REG                        3
#define PHY_ADVERTIZE_REG                     4
#define PHY_ADVERTIZE_ENABLE_ALL  (0x1f<<5)

#define PHY_LINK_ABILITY_REG                  5
  #define MII_100BT4              (1<<9)
  #define MII_FD100               (1<<8)
  #define MII_HD100               (1<<7)
  #define MII_FD10                (1<<6)
  #define MII_HD10                (1<<5)
  #define MII_SEL                 (1<<0)

#define PHY_1000BT_ADVERTISE_REG              9
  #define MII_FD1000              (1<<9)
  #define MII_HD1000              (1<<8)
#define PHY_1000BT_LINK_ABILITY_REG           10
  #define MII_LINK_FD1000         (1<<11)
  #define MII_LINK_HD1000         (1<<10)

#define PHY_EXTENDED_STATUS_REG               15
  #define MII_FD1000X             (1<<15)
  #define MII_HD1000X             (1<<14)
  #define MII_FD1000T             (1<<13)
  #define MII_HD1000T             (1<<12)

#define PHY_COPPER_CONTROL                    16 /* PAGE 0 */
  #define PHY_MDI_AUTO_CROSS      (0x3 <<5)
#define PHY_COPPER_STATUS                     17
  #define MII_SPEED_DUPLEX_RESOLVED    (0x1<<11)
  #define MII_SPEED               (0x3<<14)
  #define MII_DUPLEX              (0x1<<13)
#define PHY_COPPER_INTERRUPT                  18
#define PHY_MAC_SPEC_CONTROL2                 21  /* page 2 */
  #define RGMII_TX_TIMING         (0x1 <<4)
  #define RGMII_RX_TIMING         (0x1 <<5)
  #define RGMII_MAC_SPEED_BIT6    (0x1<<6)
  #define RGMII_MAC_SPEED_BIT13   (0x1<<13)

#define PHY_MARVELL_PAGE                      22
  #define PHY_PAGE_MASK           0xff
  
#define PHY_LED_CONTROL                       24      /* PAGE 3 */
  #define MII_PHY_LED_DIRECT      0x4100

#define LSI_PHY_STATUS_REG                    26  
  #define LSI_MII_AN_COMPLETED    (0x1<<12)
  #define LSI_MII_SPEED           (0x3<<8)
  #define LSI_MII_DUPLEX          (0x1<<7)

#define LSI_PHY_LED_CONTROL_REG               28

#define ATHEROS_PHY_SPECIFIC_STATUS_REG       17
  #define ATHEROS_MII_1000         (1<<15)

/********************************************************/
/*                                                      */
/*         Function  Prototypes                         */
/*                                                      */
/********************************************************/

static UINT32 get_cpu_rev(UINT32 *pID);
static void atheros_init(void *param);
static int  atheros_config_autoneg(void *param);
static int  atheros_read_link_status(void *param);
static void atheros_reg_dump(void *param);

  
static void lsi_init(void *param);
static int lsi_config_autoneg(void *param);
static int lsi_read_link_status(void *param);
static void lsi_reg_dump(void *param);

static void marvell_init(void *param);
static int marvell_config_autoneg(void *param);
static int marvell_read_link_status(void *param);
static void marvell_reg_dump(void *param);
static void _cpsw3g_phy_register_dump(PHY_DEVICE *phy_device);

static void Cpsw3g_Phy_Timer_Handler
(
    IN  PVOID	SystemSpecific1,
    IN  PVOID	FunctionContext,
    IN  PVOID	SystemSpecific2, 
    IN  PVOID	SystemSpecific3
);


static PHY_DRIVER phy_drivers[PHY_DRIVER_NUM] = 
{
    {
        0x01410cb0,
        "Marvell 88EC046",
        TRUE,
        &marvell_init,
        &marvell_config_autoneg,
        &marvell_read_link_status,
        &marvell_reg_dump,        
    },
    {
        0x0282f010,
        "LSI ET1011C",
        TRUE,
        &lsi_init,
        &lsi_config_autoneg,
        &lsi_read_link_status,
        &lsi_reg_dump,
    },
    {
        0x004dd070,
        "Atheros AR8031",
        TRUE,
        &atheros_init,
        &atheros_config_autoneg,
        &atheros_read_link_status,
        &atheros_reg_dump,
    },
    {
        0x0007c0f0,
        "SMSC LAN8710A",
        TRUE,
        &atheros_init,
        &atheros_config_autoneg,
        &atheros_read_link_status,
        &atheros_reg_dump,
    }
} ;

static void atheros_init(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    /* LED configuration */
//    MdioWr(phydev->phy_addr, LSI_PHY_LED_CONTROL_REG, 0xF4FA, phydev->channel);
}

static int atheros_config_autoneg(void *param)
{
    UINT16 advertise, temp;
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;

    /* advertise 10, 100, 1000 */
    advertise = MII_FD100 | MII_HD100 | MII_FD10 | MII_HD10 | MII_SEL;
    MdioWr(phydev->phy_addr, PHY_ADVERTIZE_REG, advertise, phydev->channel);
    advertise = MII_FD1000 | MII_HD1000; 
    MdioWr(phydev->phy_addr, PHY_1000BT_ADVERTISE_REG, advertise, phydev->channel);

    /* enable auto negotiation*/
    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &temp);
    temp |= MII_AUTO_NEGOTIATE_EN;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);
    temp |= MII_RENEGOTIATE;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);

    return 0;
}


static int atheros_phy_is_1000base_x (PHY_DEVICE *phydev)
{
#if 1 //defined(CONFIG_PHY_GIGE)
	UINT16 val = 0;
	UINT16 reg = 0;
	UINT16 mask = 0;

    if (get_cpu_rev(NULL))
    {
        reg = ATHEROS_PHY_SPECIFIC_STATUS_REG;
        mask = ATHEROS_MII_1000;
    }
    else
    {
        DEBUGMSG(DBG_ERR, (TEXT("atheros_phy_is_1000base_x - Unknown cpu rev\r\n")));
        return 0;
    }

    // read the extended status reg
	if (MdioRd(phydev->phy_addr, reg, phydev->channel, &val)) {
        DEBUGMSG(DBG_ERR, (L"atheros_phy_is_1000base_x: PHY specific status read failed, assuming no 1000BASE-X\r\n"));
		return 0;
	}
	return 0 != (val & mask);
#else
	return 0;
#endif
}


static UINT32 atheros_phy_speed (PHY_DEVICE *phydev)
{
	UINT16 bmcr, anlpar;

#if 1  // defined(CONFIG_PHY_GIGE)
	UINT16 btsr;

	/*
	 * Check for 1000BASE-X.  If it is supported, then assume that the speed
	 * is 1000.
	 */
	if (atheros_phy_is_1000base_x (phydev)) {
		return 1000000;
	}
	/*
	 * No 1000BASE-X, so assume 1000BASE-T/100BASE-TX/10BASE-T register set.
	 */
	/* Check for 1000BASE-T. */
	if (MdioRd(phydev->phy_addr, PHY_1000BT_LINK_ABILITY_REG, phydev->channel, &btsr)) {
        DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_1000BT_LINK_ABILITY_REG\r\n"));
		goto phy_read_failed;
	}
	if (btsr != 0xFFFF &&
	    (btsr & (MII_LINK_FD1000 | MII_LINK_HD1000))) {
		return 1000000;
	}
#endif /* CONFIG_PHY_GIGE */

	/* Check Basic Management Control Register first. */
	if (MdioRd (phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &bmcr)) {
        DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_CONTROL_REG\r\n"));
		goto phy_read_failed;
	}

	/* Check if auto-negotiation is on. */
	if (bmcr & MII_AUTO_NEGOTIATE_EN) {
		/* Get auto-negotiation results. */
		if (MdioRd (phydev->phy_addr, PHY_LINK_ABILITY_REG, phydev->channel, &anlpar)) {
            DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_LINK_ABILITY_REG\r\n"));
			goto phy_read_failed;
		}

		return (anlpar & (MII_100BT4|MII_FD100|MII_HD100)) ? 100000 : 10000;
	}

	/* Get speed from basic control settings. */
	return (bmcr & MII_PHY_100) ? 100000 : 10000;

phy_read_failed:
    DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: read failed, assuming 10BASE-T\r\n"));
	return 10000;
}

static PHY_DUPLEX atheros_phy_duplex (PHY_DEVICE *phydev)
{
	UINT16 bmcr, anlpar;

#if 1  // defined(CONFIG_PHY_GIGE)
	UINT16 btsr;

	/* Check for 1000BASE-X. */
	if (atheros_phy_is_1000base_x (phydev)) {
		/* 1000BASE-X */
		if (MdioRd (phydev->phy_addr, PHY_LINK_ABILITY_REG, phydev->channel, &anlpar)) {
            DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_LINK_ABILITY_REG\r\n"));
			goto phy_read_failed;
		}
	}

	/*
	 * No 1000BASE-X, so assume 1000BASE-T/100BASE-TX/10BASE-T register set.
	 */
	/* Check for 1000BASE-T. */
	if (MdioRd (phydev->phy_addr, PHY_1000BT_LINK_ABILITY_REG, phydev->channel, &btsr)) {
        DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_1000BT_LINK_ABILITY_REG\r\n"));
		goto phy_read_failed;
	}

	if (btsr != 0xFFFF) {
		if (btsr & MII_LINK_FD1000) {
			return FULL_DUPLEX;
		} else if (btsr & MII_LINK_HD1000) {
			return HALF_DUPLEX;
		}
	}
#endif /* CONFIG_PHY_GIGE */

	/* Check Basic Management Control Register first. */
	if (MdioRd (phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &bmcr)) {
        DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_CONTROL_REG\r\n"));
		goto phy_read_failed;
	}
	/* Check if auto-negotiation is on. */
	if (bmcr & MII_AUTO_NEGOTIATE_EN) {
		/* Get auto-negotiation results. */
		if (MdioRd (phydev->phy_addr, PHY_LINK_ABILITY_REG, phydev->channel, &anlpar)) {
            DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: Error reading PHY_LINK_ABILITY_REG\r\n"));
			goto phy_read_failed;
		}
		return ((anlpar & (MII_FD10 | MII_FD100)) ?  FULL_DUPLEX : HALF_DUPLEX);
	}
	/* Get speed from basic control settings. */
	return ((bmcr & MII_PHY_FD) ? FULL_DUPLEX : HALF_DUPLEX);

phy_read_failed:
    DEBUGMSG(DBG_ERR, (L"atheros_phy_speed: read failed, assuming half duplex\r\n"));
	return HALF_DUPLEX;
}



static int atheros_read_link_status(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 temp;

    MdioRd(phydev->phy_addr, PHY_STATUS_REG, phydev->channel, &temp);
    if(!(temp & MII_PHY_LINKED))
    {
        RETAILMSG(TRUE, (L"  ** atheros_read_link_status : status is not resolved.\r\n"));   
        return -1;
    }

    phydev->link_speed = atheros_phy_speed (phydev);

    phydev->link_duplex = atheros_phy_duplex(phydev);

    DEBUGMSG(DBG_INFO, (L"  ** atheros_read_link_status : speed:%d, duplex:%s.\r\n", 
        phydev->link_speed, 
        (phydev->link_duplex == HALF_DUPLEX)? L"hd":L"fd"));   

    return 0;
}

static void atheros_reg_dump(void *param)
{
    //PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UNREFERENCED_PARAMETER(param);
    DEBUGMSG(DBG_WARN, (L"  -> atheros_reg_dump : not implemented.\r\n"));   

    return;
}


static void lsi_init(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    /* LED configuration */
    MdioWr(phydev->phy_addr, LSI_PHY_LED_CONTROL_REG, 0xF4FA, phydev->channel);
}

static int lsi_config_autoneg(void *param)
{
    UINT16 advertise, temp;
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;

    /* advertise 10, 100, 1000 */
    advertise = MII_FD100 | MII_HD100 | MII_FD10 | MII_HD10 | MII_SEL;
    MdioWr(phydev->phy_addr, PHY_ADVERTIZE_REG, advertise, phydev->channel);
    advertise = MII_FD1000 | MII_HD1000; 
    MdioWr(phydev->phy_addr, PHY_1000BT_ADVERTISE_REG, advertise, phydev->channel);

    /* enable auto negotiation*/
    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &temp);
    temp |= MII_AUTO_NEGOTIATE_EN;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);
    temp |= MII_RENEGOTIATE;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);

    return 0;
}


static int lsi_read_link_status(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 temp;

    MdioRd(phydev->phy_addr, LSI_PHY_STATUS_REG, phydev->channel, &temp);
    if(!(temp & LSI_MII_AN_COMPLETED))
    {
        RETAILMSG(TRUE, (L"  ** lsi_read_link_status : status is not resolved.\r\n"));   
        return -1;
    }

    switch((temp & LSI_MII_SPEED) >> 8)
    {
        /* 10 Mbits */
        case 0:
            phydev->link_speed = 10000;
            break;
        /* 100 Mbits */            
        case 1:
            phydev->link_speed = 100000;
            break;
        /* 1000 Mbits */                        
        case 2:
            phydev->link_speed = 1000000;            
            break;
        case 3:
        default:
            RETAILMSG(TRUE, (L"  ** lsi_read_link_status : unsupported speed:%x.\r\n", temp));   
            break;
    }

    if(temp & LSI_MII_DUPLEX) 
        phydev->link_duplex = FULL_DUPLEX;
    else 
        phydev->link_duplex = HALF_DUPLEX;

    DEBUGMSG(DBG_INFO, (L"  ** lsi_read_link_status : speed:%d, duplex:%s.\r\n", 
        phydev->link_speed, 
        (phydev->link_duplex == HALF_DUPLEX)? L"hd":L"fd"));   

    return 0;
}

static void lsi_reg_dump(void *param)
{
    //PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UNREFERENCED_PARAMETER(param);
    DEBUGMSG(DBG_WARN, (L"  -> lsi_reg_dump : not implemented.\r\n"));   

    return;
}

static void marvell_init(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 val;	

    /* Enable auto negotiation */
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);   /* select page 0*/
    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &val);
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, val, phydev->channel);

    /* auto Cross over */
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);
    MdioRd(phydev->phy_addr, PHY_COPPER_CONTROL, phydev->channel, &val);
    val |= PHY_MDI_AUTO_CROSS;
    MdioWr(phydev->phy_addr, PHY_COPPER_CONTROL, val, phydev->channel);


    /* auto-negotiation advertize */		
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);   /* select page 0*/
    MdioRd(phydev->phy_addr, PHY_ADVERTIZE_REG, phydev->channel, &val);
    val |= PHY_ADVERTIZE_ENABLE_ALL;
    MdioWr(phydev->phy_addr, PHY_ADVERTIZE_REG, val, phydev->channel);


    /* LED configuration */
    /* use page 3 */
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 3, phydev->channel);
    MdioWr(phydev->phy_addr, 16, 0x42, phydev->channel);  //0x021e

    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 2, phydev->channel);
    MdioRd(phydev->phy_addr, PHY_MAC_SPEC_CONTROL2, phydev->channel, &val);
    val |= (RGMII_TX_TIMING | RGMII_RX_TIMING) ;
    MdioWr(phydev->phy_addr, PHY_MAC_SPEC_CONTROL2, val, phydev->channel); 

    /* PHY Soft Reset */
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);
    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &val);
    val |= MII_PHY_RESET;

    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, val, phydev->channel);
}
    

static int marvell_config_autoneg(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 advertise=0, temp;

    /* advertise 10, 100 */
    if(phydev->speed_cap & (0x1<<PHY_10_FD))
    {
        advertise |= MII_FD10;
    }
    if(phydev->speed_cap & (0x1<<PHY_10_HD))
    {
        advertise |= MII_HD10;
    }
    if(phydev->speed_cap & (0x1<<PHY_100_FD))
    {
        advertise |= MII_FD100;
    }
    if(phydev->speed_cap & (0x1<<PHY_100_HD))
    {
        advertise |= MII_HD100;
    }
    
    advertise |= MII_SEL;
    MdioWr(phydev->phy_addr, PHY_ADVERTIZE_REG, advertise, phydev->channel);

    /* advertise 1000 */
    advertise =0 ;
    if(phydev->speed_cap & (0x1<<PHY_1000_FD))
    {
        advertise |= MII_FD1000;
    }
    if(phydev->speed_cap & (0x1<<PHY_1000_HD))
    {
        advertise |= MII_HD1000;
    }
    MdioWr(phydev->phy_addr, PHY_1000BT_ADVERTISE_REG, advertise, phydev->channel);

    /* enable auto negotiation*/
    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &temp);
    temp |= MII_AUTO_NEGOTIATE_EN;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);
    temp |= MII_RENEGOTIATE;
    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp, phydev->channel);

    return 0;
}

static int marvell_read_link_status(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 temp;

    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);
    MdioRd(phydev->phy_addr, PHY_COPPER_STATUS, phydev->channel, &temp);
    if(!(temp & MII_SPEED_DUPLEX_RESOLVED))
    {
        RETAILMSG(TRUE, (L"marvell_read_link_status : status is not resolved.\r\n"));   
        return -1;
    }
    switch((temp & MII_SPEED) >> 14)
    {
        /* 10 Mbits */
        case 0:
            phydev->link_speed = 10000;
            break;
        /* 100 Mbits */            
        case 1:
            phydev->link_speed = 100000;
            break;
        /* 1000 Mbits */                        
        case 2:
            phydev->link_speed = 1000000;            
            break;
        case 3:
        default:
            RETAILMSG(TRUE, (L"marvell_read_link_status : unsupported speed:%x.\r\n", temp));   
            break;
    }
    if(temp & MII_DUPLEX) phydev->link_duplex = FULL_DUPLEX;
    else phydev->link_duplex = HALF_DUPLEX;
    return 0;

}

static void marvell_reg_dump(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    int i;
    UINT16 val;

    DEBUGMSG(DBG_INFO, (L"marvell register dump for channel %d.\r\n", phydev->channel));   
    
    /* page 0 0-21 23,26 */
    DEBUGMSG(DBG_INFO,  (L"\r\nPage:0\r\n"));   
    
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 0, phydev->channel);
    for(i=0;i<27;i++)
    {
        MdioRd(phydev->phy_addr, i, phydev->channel, &val);
        DEBUGMSG(DBG_INFO,  (L"\tregister %d:0x%x\r\n", i, val));   
    }


    /* page 2 : 16, 18, 19, 21, 24, 25 */
    DEBUGMSG(DBG_INFO,  (L"\r\nPage:2\r\n"));   
    
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 2, phydev->channel);
    
    MdioRd(phydev->phy_addr, 16, phydev->channel, &val);
    DEBUGMSG(DBG_INFO,  (L"\tregister 16:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 18, phydev->channel, &val);    
    DEBUGMSG(DBG_INFO,  (L"\tregister 18:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 19, phydev->channel, &val);    
    DEBUGMSG(DBG_INFO,  (L"\tregister 19:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 21, phydev->channel, &val);    
    DEBUGMSG(DBG_INFO,  (L"\tregister 21:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 24, phydev->channel, &val);    
    DEBUGMSG(DBG_INFO,  (L"\tregister 24:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 25, phydev->channel, &val);    
    DEBUGMSG(DBG_INFO,  (L"\tregister 25:0x%x\r\n", val));   


    /* page 3: 16-18*/
    DEBUGMSG(DBG_INFO, (L"\r\nPage:3\r\n"));   
    
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 3, phydev->channel);

    for(i=16;i<19;i++)
    {
        MdioRd(phydev->phy_addr, i, phydev->channel, &val);
        DEBUGMSG(DBG_INFO, (L"\tregister %d:0x%x\r\n", i, val));   
    }

    /* page 5: 17-28*/
    DEBUGMSG(DBG_INFO, (L"\r\nPage:5\r\n"));   
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 5, phydev->channel);
    
    for(i=16;i<29;i++)
    {
        MdioRd(phydev->phy_addr, i, phydev->channel, &val);
        DEBUGMSG(DBG_INFO, (L"\tregister %d:0x%x\r\n", i, val));   
    }

     /* page 6: 16,17,26*/
    DEBUGMSG(DBG_INFO, (L"\r\nPage:6\r\n"));   
    MdioWr(phydev->phy_addr, PHY_MARVELL_PAGE, 6, phydev->channel);
    
    MdioRd(phydev->phy_addr, 16, phydev->channel, &val);
    DEBUGMSG(DBG_INFO,  (L"\tregister 16:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 17, phydev->channel, &val);
    DEBUGMSG(DBG_INFO,  (L"\tregister 17:0x%x\r\n", val));   
    MdioRd(phydev->phy_addr, 26, phydev->channel, &val);
    DEBUGMSG(DBG_INFO, (L"\tregister 26:0x%x\r\n", val));   
}

UINT32 _cpsw3g_phy_read_id(PHY_DEVICE *phydev)
{
    UINT32 id, phy_addr = phydev->phy_addr;
    UINT16 temp;

    MdioRd(phy_addr, PHY_IDENT_REG, phydev->channel, &temp);

    id = (temp & 0xffff) <<16;

    MdioRd(phy_addr, PHY_IDENT2_REG, phydev->channel, &temp);

    id = id | (UINT32)(temp & 0xffff);

    return id;   
}

VOID _cpsw3g_phy_softreset(void *param)
{
    PHY_DEVICE *phydev = (PHY_DEVICE *)param;
    UINT16 temp;

    MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &temp);

    MdioWr(phydev->phy_addr, PHY_CONTROL_REG, temp | MII_PHY_RESET, 
                     phydev->channel);

    /* Wait until reset is complete */
    while(!MdioRd(phydev->phy_addr, PHY_CONTROL_REG, phydev->channel, &temp) && (temp & MII_PHY_RESET));
}

PHY_DRIVER *_cpsw3g_find_phy_driver(UINT32 phy_id)
{
    UINT32 i;

    for(i=0;i<PHY_DRIVER_NUM;i++)
    {
        if((phy_id & 0xfffffff0) == phy_drivers[i].id)
        {
            DEBUGMSG(DBG_INFO, (L"_cpsw3g_find_phy_driver, iter:%d, id = %x\r\n",i, phy_drivers[i].id));      
        
            return &phy_drivers[i];
        }
    }
    return NULL;
}

static int  _cpsw3g_phy_update_link(PHY_DEVICE *phydev) 
{
    UINT16 val;

    /* check link status*/
    MdioRd(phydev->phy_addr, PHY_STATUS_REG, phydev->channel, &val);
    if(MII_PHY_LINKED & val)
    {
        if(phydev->link == LINK_UP) return 0;

        phydev->link = LINK_UP;       
        phydev->phy_state = LINKED;
        
        /* update link speed and duplex*/
        phydev->driver->read_status((void *)phydev);
    }
    else
    {
        if(phydev->link == LINK_DOWN) return 0;

        phydev->link = LINK_DOWN;
        phydev->phy_state = LINK_WAIT;

    }

    RETAILMSG(TRUE, (L"_cpsw3g_phy_update_link, Port:%d  LINK:%s  SPEED:%d  DUPLEX:%s\r\n",
                phydev->channel,
                (phydev->link == LINK_DOWN)? L"DOWN": L"UP",
                phydev->link_speed /1000, 
                (phydev->link_duplex == HALF_DUPLEX? L"HD":L"FD")));   

    /* Link changed , update link status */
    Cpsw3g_Update_LinkStatus(phydev);	

    /* return link changed */
    return (1);
}


static UINT32 get_cpu_rev(UINT32 *pID)
{
#if defined AM387X_BSP
    PHYSICAL_ADDRESS physicalAddress;
    volatile UINT32  *pDeviceId;
	UINT32 id;
	UINT32 rev;

    physicalAddress.HighPart = 0;
    physicalAddress.LowPart  = 0x48140600;  // DEVICE_ID  (AM387X_L4_CNTRL_MODULE_PA + 0x0600)
	pDeviceId = (volatile UINT32 *)MmMapIoSpace(physicalAddress, sizeof(UINT32), FALSE);

    if (!pDeviceId)
        return 0;  /* assume PG1_0 */

	id = (*pDeviceId);

    if (pID)
        *pID = id;

	rev = (id >> 28) & 0xF;

	/* PG2.1 devices should read 0x3 as chip rev
	 * Some PG2.1 devices have 0xc as chip rev
	 */
	if (0x3 == rev || 0xc == rev)
		return 1; /*PG2_1*/
	else
		return 0; /*PG1_0*/
#elif defined AM33X_BSP
    return 1;
#else
    return 0;
#endif

}


VOID Cpsw3g_Phy_Init(PHY_DEVICE *phy_device)
{
    UINT32 cpu_rev;
    PCPSW3G_ADAPTER pAdapter = phy_device->handle; 	

    cpu_rev = get_cpu_rev(NULL);
    if(!cpu_rev)
    {
        // Centaurus PG1.0
        pAdapter->Cfg.macCfg[phy_device->channel].PhyAddr =
            (pAdapter->Cfg.macCfg[phy_device->channel].PhyAddr ? 0:1);
    }
 
    DEBUGMSG(
       DBG_INIT, 
       (L"***** Cpsw3g_Phy_Init: port %d phy_addr:%d, state=%d.\r\n", 
       phy_device->channel, 
       pAdapter->Cfg.macCfg[phy_device->channel].PhyAddr, 
       phy_device->phy_state)
    );

    phy_device->phy_addr = pAdapter->Cfg.macCfg[phy_device->channel].PhyAddr;
    if(phy_device->phy_state == STOPPED)
    {
        phy_device->timeout = CPSW3G_PHY_DELAY;
        phy_device->phy_state = INITING;
    }
    else
    {
        phy_device->phy_state = INITING;
        phy_device->timeout = CPSW3G_PHY_DELAY;
        phy_device->link = LINK_DOWN;

        /* Ethernet Port PC port matches LAN port speed varible initialization */
        /* enable 10, 100 & 1000 */
        phy_device->speed_cap = CPSW3G_SPEED_CAP_DEFAULT;
        phy_device->speed_change_request = FALSE;
        phy_device->speed_force_default = FALSE;
        
        {
            /* Start PHY timer */
            NdisMInitializeTimer(
                                &(phy_device->Phy_timer),
                                pAdapter->AdapterHandle,
                                Cpsw3g_Phy_Timer_Handler,
                                phy_device);

            NdisMSetTimer(&(phy_device->Phy_timer), phy_device->timeout);
            DEBUGMSG(DBG_INIT, (L"Cpsw3g_Phy_Init: phy timer is started, timeout every:%d.\r\n", 
                            phy_device->timeout));
        }
    }  
}

VOID Cpsw3g_Phy_Stop(PHY_DEVICE *phy_device)
{
    phy_device->phy_state = STOPPED;
    phy_device->timeout = CPSW3G_PHY_DELAY;
    phy_device->link = LINK_DOWN;

    RETAILMSG(TRUE, (L"  ** Cpsw3g_Phy_Stop, Port %d LINK: %s   SPEED:%d,  DUPLEX:%s\r\n",
                phy_device->channel,
                (phy_device->link == LINK_DOWN)? L"DOWN": L"UP",
                phy_device->link_speed /1000, 
                (phy_device->link_duplex == HALF_DUPLEX? L"HD":L"FD")));   

    /* Link changed , update link status */
    Cpsw3g_Update_LinkStatus(phy_device);
}


LINKSTATUS Cpsw3g_Phy_Get_LinkStatus(PHY_DEVICE *phy_device)
{
    //_cpsw3g_phy_update_link(phy_device);
    return phy_device->link;
}

static void _cpsw3g_phy_register_dump(PHY_DEVICE *phy_device)
{
    /* */
    if(!(phy_device->driver))
    {
        RETAILMSG(TRUE, (L"_cpsw3g_phy_register_dump : invalid phy driver %p \r\n", phy_device->driver));
        return;
    }
    phy_device->driver->reg_dump(phy_device);
    
}
static void _cpsw3g_Phy_Init_state(PHY_DEVICE *phy_device)
{
    UINT32 phy_mask, phy_id;
    UINT32 i, j, PhyAcks, PhyNum= PHY_NOT_FOUND;

    phy_mask = 0x1 << phy_device->phy_addr;

    DEBUGMSG(DBG_INFO, (L"_cpsw3g_Phy_Init_state: phy_mask %x\r\n", phy_mask));

    PhyAcks = MdioAlive() & phy_mask;

    DEBUGMSG(
        DBG_INFO, 
        (L"_cpsw3g_Phy_Init_state: phy_mask %x, PhyAcks%x alive:%x\r\n", 
        phy_mask, PhyAcks, MdioAlive())
    );

    // Find out (if any) which phy is alive
    for(i=0, j=1;  (i<32) && ((j&PhyAcks)==0);  i++, j<<=1);

//    RETAILMSG(TRUE, (L"  *** PhyAcks=0x%x i=%d\r\n", PhyAcks, i));

    if((PhyAcks) && (i<32)) 
        PhyNum = i;

    if(PhyNum == PHY_NOT_FOUND)
    {
//        RETAILMSG(TRUE, (L"  *** PHY_NOT_FOUND\r\n"));
        phy_device->timeout = CPSW3G_PHY_DELAY; 
        return;
    }

    /* PHY found */
    if(phy_device->phy_addr != PhyNum)
    {
        DEBUGMSG(DBG_ERR, 
            (L"PHY found: phy num %d does not match configured PHY address %d. PhyAcks=0x%08X\r\n",
            PhyNum, phy_device->phy_addr, PhyAcks));
        return;
    }
    
    /* Read PHY id */
    phy_id = _cpsw3g_phy_read_id(phy_device);
    DEBUGMSG(DBG_INFO, (L"PHY id : %08X\r\n", phy_id));

    phy_device->driver =_cpsw3g_find_phy_driver(phy_id & 0xfffffff0);
    if(!(phy_device->driver))
    {
        RETAILMSG(TRUE, 
            (L"_cpsw3g_Phy_Init_state : could not found driver for PHY id %x.\r\n", 
            phy_id));
        return;
    }

    /* soft reset phy */
    _cpsw3g_phy_softreset(phy_device);
    phy_device->phy_poll = phy_device->driver->phy_poll;
    phy_device->driver->init(phy_device);

    /* Start Auto-Negotiation */
    phy_device->driver->config_autoneg(phy_device);
    phy_device->phy_state = AUTO_NEG;

    phy_device->timeout = CPSW3G_PHY_DELAY; 
    _cpsw3g_phy_register_dump(phy_device);
    
    return;
}
                    
static void  _cpsw3g_Phy_Auto_Neg_state(PHY_DEVICE *phydev)
{
    UINT16 status, local_cap, remote_cap;
    PCPSW3G_ADAPTER pAdapter = phydev->handle; 	
    
    /* Check if negotiation is completed */
    MdioRd(phydev->phy_addr, PHY_STATUS_REG, phydev->channel, &status);
    if(status & MII_NWAY_COMPLETE)
    {
        MdioRd(phydev->phy_addr, PHY_1000BT_ADVERTISE_REG, phydev->channel, &local_cap);
        MdioRd(phydev->phy_addr, PHY_1000BT_LINK_ABILITY_REG, phydev->channel, &remote_cap);

        if( remote_cap & (MII_LINK_FD1000 | MII_LINK_HD1000)) 
            DEBUGMSG(DBG_INFO, (L"_cpsw3g_Phy_Auto_Neg_state, Negotiated 1000Mb mode\r\n"));
        else
        {
            MdioRd(phydev->phy_addr, PHY_ADVERTIZE_REG, phydev->channel, &local_cap);
            MdioRd(phydev->phy_addr, PHY_LINK_ABILITY_REG, phydev->channel, &remote_cap);
            if(local_cap & remote_cap)
                DEBUGMSG(DBG_INFO, 
                    (L"_cpsw3g_Phy_Auto_Neg_state, Negotiated mode:%x\r\n", 
                    local_cap & remote_cap));
            else
                DEBUGMSG(DBG_INFO, 
                    (L"_cpsw3g_Phy_Auto_Neg_state, Negotiated failed, remote cap:%x\r\n", 
                    remote_cap));                
        }
        _cpsw3g_phy_update_link(phydev);
        phydev->retry_count = 3;
    }
    else if(pAdapter->Cfg.Speed_match)
    {
        if(phydev->speed_change_request)
        {

            /* re-Start Auto-Negotiation */
            phydev->driver->config_autoneg(phydev);
            phydev->speed_change_request = FALSE;
        }

        /* Error handling for cases when negotiation failed */
        if( !(--phydev->retry_count) && (phydev->speed_cap !=  CPSW3G_SPEED_CAP_DEFAULT))
        {
            phydev->speed_cap =  CPSW3G_SPEED_CAP_DEFAULT;         
            phydev->driver->config_autoneg(phydev);
            phydev->speed_force_default = TRUE;
        }
    }

}
static void  _cpsw3g_Phy_Link_Wait_state(PHY_DEVICE *phydev)
{
    if(!_cpsw3g_phy_update_link(phydev))
    {
        phydev->timeout = CPSW3G_PHY_LINK_WAIT_DELAY; 
        if( --phydev->retry_count == 0 )
        {
            /* Start Auto-Negotiation */
            phydev->driver->config_autoneg(phydev);
            phydev->phy_state = AUTO_NEG;
            phydev->retry_count = 3;
        }
    }
    else
        phydev->timeout = CPSW3G_PHY_DELAY; 
}
            
static void  _cpsw3g_Phy_Linked_state(PHY_DEVICE *phydev)
{
    PCPSW3G_ADAPTER pAdapter = phydev->handle; 	

    _cpsw3g_phy_update_link(phydev);       
    phydev->timeout = CPSW3G_PHY_LINKED_DEAY; 

    if(pAdapter->Cfg.Speed_match)
    {
        if(phydev->speed_change_request)
        {

            /* re-Start Auto-Negotiation */
            phydev->driver->config_autoneg(phydev);
            phydev->phy_state = AUTO_NEG;
            phydev->timeout = CPSW3G_PHY_DELAY; 
            phydev->link = LINK_DOWN; 
            phydev->retry_count = 500;
            
            phydev->speed_change_request = FALSE;
        }
    }
}

static void _cpsw3g_Phy_Stopped_state(PHY_DEVICE *phydev)
{
    phydev->timeout = CPSW3G_PHY_DELAY; 
}

static void Cpsw3g_Phy_Timer_Handler
(
    IN  PVOID	SystemSpecific1,
    IN  PVOID	FunctionContext,
    IN  PVOID	SystemSpecific2, 
    IN  PVOID	SystemSpecific3
)
{
    PHY_DEVICE *phydev = 	(PHY_DEVICE *)FunctionContext;
    PHY_STATE PhyState = phydev->phy_state;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

//    RETAILMSG(TRUE, (L"  ->Cpsw3g_Phy_Timer_Handler, port %d, state = %d\r\n", phydev->channel, PhyState));

    switch(PhyState)
    {
        case INITING:
            _cpsw3g_Phy_Init_state(phydev);
            break;
            
        case AUTO_NEG:
            _cpsw3g_Phy_Auto_Neg_state(phydev);
            
            break;
                        
        case LINK_WAIT:
            _cpsw3g_Phy_Link_Wait_state(phydev);
            break;
            
        case LINKED:
            _cpsw3g_Phy_Linked_state(phydev);
            break;

        case STOPPED:
            _cpsw3g_Phy_Stopped_state(phydev);
            break;
        default:
            break;
    }

    if(PhyState == LINKED && !phydev->phy_poll)
    {
        /* Start PHY interrupt */
        
    }
    else 
    {
        NdisMSetTimer(&phydev->Phy_timer, phydev->timeout);        
    }
}

