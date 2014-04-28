#include <windows.h>
#include "oalex.h"
#include "bsp_cfg.h"


DWORD check_dirty_flag(UINT8 *buf, DWORD bufSz)
{
    DWORD   dataSz = 0;    
    if (!KernelIoControl(IOCTL_HAL_GET_EBOOT_CFG,
                         NULL, 0, buf, bufSz, &dataSz))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read EBOOT CFG from Boot Args\r\n")));        
    }   
    return dataSz;
    
}

DWORD get_boot_slot(DWORD *slot)
{
    DWORD   dataSz = 0;
	DWORD   data;
	

    if (!KernelIoControl(IOCTL_HAL_GET_BOOT_INFO,
                         NULL, 0, &data, sizeof(DWORD), &dataSz))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read boot info from Boot Args\r\n")));        
    }
	*slot = data>>16;
    return dataSz;

}


#define REG_SDFOLDER_PATH    TEXT("System\\StorageManager\\Profiles\\SDMemory")
#define REG_EMMCFOLDER_PATH  TEXT("System\\StorageManager\\Profiles\\eMMC\\BEAGLEBONE")


// Get the SD slot from the OS. The Ioctrl call will return the boot info which has the SD slot info from
// which we booted in the upper word. We use this info to determine the folder name of the boot slot.
// In case of multiple SD slots present on the board, the biggest assumption is that the boot SD/MMC slot 
// is the first entry in the Drivers\Builtin\SDHC(x) else the folder name will not be "Storage Card", but something
// like "Storage Card2".
void get_ebootCfg_FileName(LPWSTR fileName)
{   
    HKEY hKey = NULL;        
    DWORD dwType=REG_SZ;
    WCHAR szFolderName[50]=L"Storage Card"; //default value
    DWORD size=100;
	DWORD slot=1;
    DWORD ret;
    
	if (!get_boot_slot(&slot))
	{
        RETAILMSG(1,(L"Could not get boot info ...will use slot 1\r\n"));
		goto exit;
	}
    RETAILMSG(1,(L"Boot slot %d\r\n",slot));

	if (2 == slot)
	{
		//get the folder Name from registry in case BSP is using non-default name
		if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPWSTR)REG_EMMCFOLDER_PATH, 0, NULL, 
						REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL))
		{          
			ret=RegQueryValueEx(hKey, L"Folder", 0, &dwType, (BYTE*)szFolderName, &size);
			if (ret!=ERROR_SUCCESS)
				RETAILMSG(1,(L"RegQueryValueEx returned error %d dwType=%d size=%d bufSz=%d\r\n",ret,dwType,size,sizeof(szFolderName)));
		}
	}
	else // use 1 as default
	{
		//get the folder Name from registry in case BSP is using non-default name
		if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPWSTR)REG_SDFOLDER_PATH, 0, NULL, 
						REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL))
		{          
			ret=RegQueryValueEx(hKey, L"Folder", 0, &dwType, (BYTE*)szFolderName, &size);
			if (ret!=ERROR_SUCCESS)
				RETAILMSG(1,(L"RegQueryValueEx returned error %d dwType=%d size=%d bufSz=%d\r\n",ret,dwType,size,sizeof(szFolderName)));
		}
	}

exit:
    wcscat(fileName,szFolderName);
    wcscat(fileName,L"\\eboot.cfg");

    return;
}

void save_eboot_cfg(UINT8 *buf, DWORD bufSz)
{
    
	HANDLE hLocalFile = NULL;    
    WCHAR lpszFileName[256] = L"\\";    
    DWORD dwBytesWritten;
    BOOL bResult;

    get_ebootCfg_FileName(lpszFileName);
    
    hLocalFile = CreateFile(lpszFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);    
    if ( hLocalFile == INVALID_HANDLE_VALUE )
    {
        RETAILMSG( 1, (L"Error: could not open Eboot cfg file [%s]\r\n",lpszFileName));
        goto cleanup;
    }  

    bResult = WriteFile( hLocalFile, (void *)buf, bufSz, &dwBytesWritten, NULL ); 
    if ( (bResult == FALSE) || (dwBytesWritten != bufSz) )
    {
       RETAILMSG( 1, (L"Error: could not write Eboot cfg file [%s]\r\n",lpszFileName) );
        goto cleanup;
    }
    else
        RETAILMSG( 1, (L"eboot cfg saved to %s\r\n",lpszFileName));
      

cleanup:
    if ( hLocalFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hLocalFile );
	}
}

//------------------------------------------------------------------------------
//
//  Function:  CFG_Init
//
//  Called by device manager to initialize device.
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UINT8 buf[256];
    DWORD cfgSz = 0;
    
    UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

    memset(buf,0,sizeof(buf));
	
    cfgSz = check_dirty_flag(buf,sizeof(buf));
    if (cfgSz)
        save_eboot_cfg(buf,cfgSz);
    
    return 1;
}


