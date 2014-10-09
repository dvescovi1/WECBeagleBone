/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  board_detect.c
//
#include "bsp.h"
#include "oal_i2c.h"
#include "sdk_i2c.h"
#include "bsp_padcfg.h"
#include "am33x_clocks.h"


#if defined(BUILDING_XLDR_SD)
	#undef  OALMSG(cond, exp)
	#define OALMSG(cond, exp)   ((void)FALSE)
#endif


extern I2CDevice_t _rgI2CDevice[]; 

//-----------------------------------------------------------------------------
//
//  Global:  g_dwBoardId
//
//  Set during OEMInit to indicate Board ID read from EEPROM.
//

DWORD g_dwBoardId = (DWORD)AM33X_BOARDID_BBONE_BOARD;

//-----------------------------------------------------------------------------
//
//  Global:  g_dwBoardProfile
//
//  Set during OEMInit to indicate Board Profile read from CPLD.
//

DWORD g_dwBoardProfile = (DWORD)PROFILE_0;

//-----------------------------------------------------------------------------
//
//  Global:  g_dwBoardHasDcard
//
//  Set during OEMInit to indicate if board has daugther board.
//  Bit field masks:
//  

DWORD g_dwBoardHasDcard = (DWORD)FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_dwBaseBoardVersion
//
//  Set during OEMInit to indicate the hardware version of the base board.
//

DWORD g_dwBaseBoardVersion = (DWORD)AM335X_BONE_BOARD_VER_A3;



#define NO_OF_MAC_ADDR          3
#define ETH_ALEN		        6


struct am335x_baseboard_id {
	unsigned int  magic;
	char name[8];
	char version[4];
	char serial[12];
};


struct am335x_daughterBoard_id {
	UINT32  magic;
	char eepromrev[2];
	char name[32];
	char version[4];
	char manufacturer[16];
	char partnumber[16];
	UINT16 numberofpins; 
	char serial[12];
};

/*
 * Daughter board detection: All boards have i2c based EEPROM in all the
 * profiles. Base boards and daughter boards are assigned unique I2C Address.
 * We probe for daughter card and then if sucessful, read the EEPROM to find
 * daughter card type.
 */
static BOOL detect_daughter_board(void * m_hI2CDevice, UINT32 address)
{   
    static struct am335x_daughterBoard_id db_header;    
    
    if (m_hI2CDevice == NULL) goto cleanUp;
    
    /* read header from daughter board */
    I2CSetSlaveAddress(m_hI2CDevice, (UINT16)address);    
    I2CSetSubAddressMode(m_hI2CDevice,I2C_SUBADDRESS_MODE_16);    
    
    if (I2CRead(m_hI2CDevice, 0, (void *)&db_header, sizeof(db_header))!=(UINT)-1) {
        if (db_header.magic == 0xEE3355AA) {	
            OALMSG(1,(L"Daughter Board present at address %02x\r\n",address));        
            OALMSG(1,(L"\tDB Board Name: %.32S\r\n",db_header.name));
            OALMSG(1,(L"\tDB Board Manf: %.16S\r\n",db_header.manufacturer));
            OALMSG(1,(L"\tDB Board Part: %.16S\r\n",db_header.partnumber));
            OALMSG(1,(L"\tDB Board Ver : %.4S\r\n",db_header.version));
            OALMSG(1,(L"\tDB Board Ser : %.12S\r\n",db_header.serial));

			if (strncmp(db_header.partnumber,"BB-BONE-DVID-",13) == 0)
			{
				g_dwBoardHasDcard |= HASDCARD_DVI;
			}
			if (strncmp(db_header.partnumber,"BB-BONE-LCD4-",13) == 0)
			{
				g_dwBoardHasDcard |= HASDCARD_LCD4;
			}
			if (strncmp(db_header.partnumber,"BB-BONE-LCD7-",13) == 0)
			{
				if (strncmp(db_header.name,"4D",2) == 0)
					g_dwBoardHasDcard |= HASDCARD_LCD7_4D;
				else
					g_dwBoardHasDcard |= HASDCARD_LCD7;
			}

			return TRUE;
	    }
    }
cleanUp:
    OALMSG(1,(L"DAUGHTER BOARD: Cannot read DB EEPROM at address %02x\r\n",address));
    return FALSE;
}

BOOL detect_daughter_board_profiles()
{
    void * m_hI2CDevice = NULL;
    
    // open i2c device
	_rgI2CDevice[2].maxRetries = 1;	// set max retries for I2C2 to 1
    m_hI2CDevice = I2COpen(AM_DEVICE_I2C2);  
    if (m_hI2CDevice == NULL)
	{
		OALMSG(1, (L"detect_daughter_board_profile: Failed to open I2C driver"));
		return FALSE;
	}    
    
	// detect eeprom at the 4 different slots and set slot mask in g_dwBoardHasDcard
	if (detect_daughter_board(m_hI2CDevice, I2C_DAUGHTER_BOARD1_ADDR))
	{
		g_dwBoardHasDcard |= HASDCARD_SLOT1;
	}
	if (detect_daughter_board(m_hI2CDevice, I2C_DAUGHTER_BOARD2_ADDR))
	{
		g_dwBoardHasDcard |= HASDCARD_SLOT2;
	}
	if (detect_daughter_board(m_hI2CDevice, I2C_DAUGHTER_BOARD3_ADDR))
	{
		g_dwBoardHasDcard |= HASDCARD_SLOT3;
	}
	if (detect_daughter_board(m_hI2CDevice, I2C_DAUGHTER_BOARD4_ADDR))
	{
		g_dwBoardHasDcard |= HASDCARD_SLOT4;
	}

	_rgI2CDevice[2].maxRetries = BSP_I2C2_MAXRETRY_INIT;	// set max retries for I2C2 back to default

    OALI2CInit(AM_DEVICE_I2C2);	// reset it back

	return TRUE;
}

/* caller should have called OALI2CInit before calling this function */
/* Assumes that the pinmux for the I2C0 is already set before this function is called */
BOOL detect_baseboard_id_info()
{
    void * m_hI2CDevice = NULL;
    
    static struct am335x_baseboard_id header;
    DWORD readBytes=0;

    // open i2c device
    m_hI2CDevice = I2COpen(AM_DEVICE_I2C0);  
    if (m_hI2CDevice == NULL)
	{
		OALMSG(1, (L"detect_baseboard_id_info: Failed to open I2C driver"));
		return FALSE;
	}    
    
    /* read header from base board */
    I2CSetSlaveAddress(m_hI2CDevice, (UINT16)I2C_BASE_BOARD_ADDR);
    I2CSetBaudIndex(m_hI2CDevice,SLOWSPEED_MODE);
    I2CSetSubAddressMode(m_hI2CDevice,I2C_SUBADDRESS_MODE_16);
    if ((readBytes=I2CRead(m_hI2CDevice, 0, (void *)&header, sizeof(header)))==(UINT)-1) {
        OALMSG(1, (L"Cannot read base board I2C EEPROM\r\n"));
        return FALSE;
    }
    if (header.magic != 0xEE3355AA) {		
        // read using 1 byte access        
        I2CSetSubAddressMode(m_hI2CDevice,I2C_SUBADDRESS_MODE_8);
        if ((readBytes=I2CRead(m_hI2CDevice, 0, (void *)&header, sizeof(header)))==(UINT)-1) {
            OALMSG(1, (L"Cannot read base board I2C EEPROM with 1 byte access\r\n"));
            return FALSE;
        }

        if (header.magic != 0xEE3355AA) {	
            OALMSG(1, (L"I2C EEPROM returned wrong magic value 0x%x\r\n",header.magic));
		    return FALSE;
        }
	}   
    
    if (!strncmp("A335BONE", header.name, 8)) {
		g_dwBoardId = AM33X_BOARDID_BBONE_BOARD;
    }
    else if (!strncmp("A335BNLT", header.name, 8)) {
		g_dwBoardId = AM33X_BOARDID_BBONEBLACK_BOARD;
    }
    else {
        OALMSG(1, (L"IT IS A INVALID BOARD\r\n"));
        return FALSE;
    }  

    if (g_dwBoardId == AM33X_BOARDID_BBONE_BOARD)
    {
        if (!strncmp("00A1", header.version, 4)) {        
            g_dwBaseBoardVersion = AM335X_BONE_BOARD_VER_A1;     
        }
        else if (!strncmp("00A2", header.version, 4)) {        
            g_dwBaseBoardVersion = AM335X_BONE_BOARD_VER_A2;     
        }
        else if (!strncmp("00A3", header.version, 4)) {        
            g_dwBaseBoardVersion = AM335X_BONE_BOARD_VER_A3;     
        }
        else
            OALMSG(1, (L"Unrecognized BeagleBone Version!\r\n"));
        
    }
    else if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
    {
        if (!strncmp("0A5A", header.version, 4)) {    
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_A5A;
        }
        else if (!strncmp("0A5B", header.version, 4)) {    
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_A5B;
        }
        else if (!strncmp("0A5C", header.version, 4)) {    
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_A5C;
        }
        else if (!strncmp("00A6", header.version, 4)) {       
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_A6A;
        }
        else if (!strncmp("000B", header.version, 4)) {       
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_B00;
        }
        else if (!strncmp("000C", header.version, 4)) {       
            g_dwBaseBoardVersion = AM335X_BLACK_BONE_C00;
        }
        else
            OALMSG(1, (L"Unrecognized BeagleBoneBlack Version!\r\n"));
    }
    else {
        OALMSG(1, (L"IT IS A INVALID BOARD REVISION\r\n"));
        return FALSE;
    }  

    //print the Base board info    
    OALMSG(1,(L"\tBoard Name: %.8S\r\n",header.name));
    OALMSG(1,(L"\tBoard Ver : %.4S\r\n",header.version));
    OALMSG(1,(L"\tBoard Ser : %.12S\r\n",header.serial));
    
    I2CSetBaudIndex(m_hI2CDevice,FULLSPEED_MODE);
    I2CSetSubAddressMode(m_hI2CDevice,I2C_SUBADDRESS_MODE_8);       
 
    I2CClose(m_hI2CDevice);

    return TRUE;
}


