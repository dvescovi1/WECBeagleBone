// All rights reserved ADENEO EMBEDDED 2010
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>

#include "bsp_cfg.h"
#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include "tvp5146.h"
#include "util.h"
#include "params.h"
#include "TvpCtrl.h"

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Tvp5146ResetGpio", PARAM_DWORD, FALSE, offset(TVP5146_CONFIG, dwResetGpio),
            fieldsize(TVP5146_CONFIG, dwResetGpio), (VOID*)-1
    },
    {
        L"Tvp5146I2CAddr", PARAM_DWORD, TRUE, offset(TVP5146_CONFIG, dwI2CAddr),
            fieldsize(TVP5146_CONFIG, dwI2CAddr), 0
    },
    {
        L"Tvp5146SelComposite", PARAM_DWORD, FALSE, offset(TVP5146_CONFIG, dwSelComposite),
            fieldsize(TVP5146_CONFIG, dwSelComposite), (VOID*)0x0C
    },
    {
        L"Tvp5146SelComponent", PARAM_DWORD, FALSE, offset(TVP5146_CONFIG, dwSelComponent),
            fieldsize(TVP5146_CONFIG, dwSelComponent), (VOID*)0x94
    },
    {
        L"Tvp5146SelSVideo", PARAM_DWORD, FALSE, offset(TVP5146_CONFIG, dwSelSVideo),
            fieldsize(TVP5146_CONFIG, dwSelSVideo), (VOID*)0x46
    },
};

CTvpCtrl::CTvpCtrl()
{   
    m_hI2C=NULL;
}

CTvpCtrl::~CTvpCtrl()
{
}

//------------------------------------------------------------------------------
//
//  Function:  TVPInit
//
//  Init. TVP5146   video decoder
//
BOOL CTvpCtrl::Init(PVOID MDDContext)
{
    // Read parameters from registry
    if (GetDeviceRegistryParams(
        (LPCTSTR)MDDContext, 
        &m_pTvpConfig, 
        dimof(s_deviceRegParams), 
        s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: CTvpCtrl::Init - Error reading from Registry.\r\n")));
        return FALSE;
    }

	if (m_pTvpConfig.dwI2CAddr == 0)
	{
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: CTvpCtrl::Init - Wrong I2C address read from registry\r\n")));
        return FALSE;
	}

	if (m_pTvpConfig.dwResetGpio != (DWORD)-1)
	{
		HANDLE hGpio = GPIOOpen();
	  
		Sleep(20);
		GPIOClrBit(hGpio, m_pTvpConfig.dwResetGpio);
		GPIOSetMode(hGpio, m_pTvpConfig.dwResetGpio, GPIO_DIR_OUTPUT);
		Sleep(20);
		GPIOSetBit(hGpio, m_pTvpConfig.dwResetGpio);
		GPIOSetMode(hGpio, m_pTvpConfig.dwResetGpio, GPIO_DIR_OUTPUT);

		GPIOClose(hGpio);
	}

	I2CInit();
    
	if (m_hI2C != NULL)
	{
		for(UINT i=0;i<num_tvp_settings;i++)
		{
			WriteReg(tvpSettings[i].reg, tvpSettings[i].val);
		}
	}

	I2CDeinit();    

    return TRUE;                
}   

//------------------------------------------------------------------------------
//
//  Function:  TVPReadReg
//
//  Read data from TVP5146 register
//      
BOOL CTvpCtrl::ReadReg(UINT8 slaveaddress,UINT8* data)
{
    BOOL rc = FALSE;
    if (m_hI2C)
    {
        DWORD len = I2CRead(m_hI2C, slaveaddress, data, sizeof(UINT8));
        if ( len != sizeof(UINT8))
            ERRORMSG(ZONE_ERROR,(TEXT("TVPReadReg Failed!!\r\n")));
            else
               rc = TRUE;
        }
    return rc;

}

//------------------------------------------------------------------------------
//
//  Function:  TVPWriteReg
//
//  Read data from TVP5146 register
//      
BOOL CTvpCtrl::WriteReg(UINT8 slaveaddress,UINT8 value)
{
    BOOL rc = FALSE;
    if (m_hI2C)
    {
		DEBUGMSG(TRUE, (TEXT("CTvpCtrl::WriteReg @0x%02x=0x%02x\r\n"),slaveaddress,value));
        DWORD len = I2CWrite(m_hI2C, slaveaddress, &value, sizeof(UINT8));
        if ( len != sizeof(UINT8))
            ERRORMSG(ZONE_ERROR,(TEXT("TVPWriteReg Failed!!\r\n")));
            else
               rc = TRUE;
        }
    return rc;
}
//------------------------------------------------------------------------------
//
//  Function:  I2CInit
//
//  Init TVP5146 I2C interface
//      
BOOL CTvpCtrl::I2CInit()
{
    m_hI2C = I2COpen(BSPGetCameraI2CDevice());
    if (m_hI2C == NULL)
	{
        return FALSE;
	}

    I2CSetSlaveAddress(m_hI2C, (UINT16)m_pTvpConfig.dwI2CAddr);
    I2CSetSubAddressMode(m_hI2C, I2C_SUBADDRESS_MODE_8);    
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  I2CDeinit
//
//  Deinit TVP5146 I2C interface
//      
BOOL CTvpCtrl::I2CDeinit()
{
    if (m_hI2C)
    {
        I2CClose(m_hI2C);
        m_hI2C = NULL;
    }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectComposite
//
//  To select composite path .
//
BOOL CTvpCtrl::SelectComposite()
{       
	I2CInit();      
    WriteReg(REG_INPUT_SEL, (UINT8)m_pTvpConfig.dwSelComposite);
    I2CDeinit();        

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectSVideo
//
//  To select s-video path .
//
BOOL CTvpCtrl::SelectSVideo()
{
    I2CInit();
    WriteReg(REG_INPUT_SEL, (UINT8)m_pTvpConfig.dwSelSVideo);
    I2CDeinit();        

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectComponent
//
//  To select component path .
//
BOOL CTvpCtrl::SelectComponent()
{
    I2CInit();
    WriteReg(REG_INPUT_SEL, (UINT8)m_pTvpConfig.dwSelComponent);
    I2CDeinit();

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SetPowerState
//
//  Power on or off the TVP
//
BOOL CTvpCtrl::SetPowerState(BOOL PowerOn)
{
    I2CInit();
    WriteReg(REG_OPERATION_MODE, PowerOn ? 0x00 : 0x01);
    I2CDeinit();

    return TRUE;
}
