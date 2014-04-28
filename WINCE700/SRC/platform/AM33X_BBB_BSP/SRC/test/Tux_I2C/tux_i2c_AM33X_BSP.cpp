//
//  All rights reserved ADENEO EMBEDDED 2010
//  All rights reserved Texas Instruments, Inc. 2011
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_I2C TUX DLL
//
//  Module: AM389X_BSP.cpp
//          Contains the platform specific functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////


#include "main.h"
#include "globals.h"
#include "platform_AM33X.h"

#include <i2cproxy.h>
#include <winioctl.h>

DWORD       g_BufferSize = I2C_BUFFER_SIZE;
DWORD       g_NbThreads = I2C_NB_THREADS;
DWORD	    g_Writes = I2C_WRITES;
DWORD       g_Address = I2C_ADDRESS;
DWORD	    g_Port = I2C_PORT;
DWORD	    g_BaudRateIndex = I2C_BAUDRATE_INDEX;
DWORD	    g_SubAddressMode = I2C_SUBADDRESS_MODE;

HANDLE Platform_Open()
{
	wchar_t DeviceName[8];
	HANDLE ret;

	//Open I2C port
    swprintf(DeviceName, TEXT("I2C%d:"), g_Port);
    ret = CreateFile(DeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	g_pKato->Log(1, TEXT("I2C: open ret value %d\r\n"), ret);
	return ret;
}

void Platform_Close(HANDLE hI2C)
{
	CloseHandle(hI2C);
}

BOOL Platform_setSlaveAddress(HANDLE hI2C)
{
    if(!DeviceIoControl(hI2C, IOCTL_I2C_SET_SLAVE_ADDRESS,&g_Address,sizeof(g_Address),NULL,0,NULL,NULL) )
    {
		g_pKato->Log(1, TEXT("Could not set slave address. \r\n"));
		return FALSE;
    }
	return TRUE;
}

BOOL Platform_setSubAddrMode(HANDLE hI2C)
{

    if(!DeviceIoControl(hI2C, IOCTL_I2C_SET_SUBADDRESS_MODE,&g_SubAddressMode,sizeof(g_SubAddressMode),NULL,0,NULL,NULL) )
    {
		g_pKato->Log(1, TEXT("Could not set sub Adress Mode. \r\n"));
		return FALSE;
    }

	return TRUE;
}

BOOL Platform_setBaudRateIndex(HANDLE hI2C)
{

    if(!DeviceIoControl(hI2C, IOCTL_I2C_SET_BAUD_INDEX,&g_BaudRateIndex,sizeof(g_BaudRateIndex),NULL,0,NULL,NULL) )
    {
		g_pKato->Log(1, TEXT("Could not set baud rate speed. \r\n"));
		return FALSE;
    }

	return TRUE;
}

UINT Platform_Write(HANDLE hI2C, LONG subAddr,void * buffer, UINT32 size)
{
	DWORD dwBytesReturned;

	if(!SetFilePointer(hI2C,subAddr,NULL,0))
	{
		g_pKato->Log(1, TEXT("Could not set the subAdress specified. \r\n"));
	}

    if(!WriteFile(hI2C, buffer, size, &dwBytesReturned, NULL))
    {
        g_pKato->Log(1, TEXT("Could not write I2C port due to NACK. \r\n"));		
    }

	return dwBytesReturned;

}
