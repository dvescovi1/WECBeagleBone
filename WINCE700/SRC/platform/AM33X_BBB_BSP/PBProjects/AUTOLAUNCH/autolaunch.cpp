///---------------------------------------------------------------------------------
// Copyright (c) David Vescovi.  All rights reserved.
//
// autolaunch - automatically launch any .exe in the storage card's or FLASH
//              startup folder.
//---------------------------------------------------------------------------------

#include <windows.h>
#include <string.h>


int WINAPI WinMain(HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPWSTR lpCmdLine,
    int nCmdShow
	)
{
	wchar_t string[80];
    HANDLE hSearch;
	HKEY hKey = NULL;
    WIN32_FIND_DATA fData;
	DWORD dwSize;
	DWORD dwWord;
    ULONG kvaluetype;

	// dump version info
	if (ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"\\Ident", 0, 0, &hKey))
		RETAILMSG(1, (TEXT("AutoLaunch: Could not open HKLM Ident key!\r\n")));
	else
	{
		dwSize = sizeof(string);
		string[0] = TEXT('\0');
		if (RegQueryValueEx (hKey, TEXT("OSDesignName"), NULL, NULL, (PBYTE)string, &dwSize) == ERROR_SUCCESS)
		{
			RETAILMSG(1, (TEXT("\r\nOSDesign: %s\r\n"),string));
		}
		dwSize = sizeof(DWORD);
		if (RegQueryValueEx (hKey, TEXT("OSDesignVersion"), NULL, &kvaluetype, (PBYTE)&dwWord, &dwSize) == ERROR_SUCCESS)
		{
			RETAILMSG(1, (TEXT("\r\nOSDesign Version: %d\r\n"),dwWord));
		}
		dwSize = sizeof(string);
		string[0] = TEXT('\0');
		if (RegQueryValueEx (hKey, TEXT("BSPName"), NULL, NULL, (PBYTE)string, &dwSize) == ERROR_SUCCESS)
		{
			RETAILMSG(1, (TEXT("\r\nBSP: %s\r\n"),string));
		}
		dwSize = sizeof(string);
		string[0] = TEXT('\0');
		if (RegQueryValueEx (hKey, TEXT("BSPVersion"), NULL, NULL, (PBYTE)string, &dwSize) == ERROR_SUCCESS)
		{
			RETAILMSG(1, (TEXT("\r\nBSP Version: %s\r\n"),string));
		}
		dwSize = sizeof(string);
		string[0] = TEXT('\0');
		if (RegQueryValueEx (hKey, TEXT("OSBuildDateTime"), NULL, NULL, (PBYTE)string, &dwSize) == ERROR_SUCCESS)
		{
			RETAILMSG(1, (TEXT("\r\nBuilt %s\r\n"),string));
		}
		RegCloseKey( hKey);
	}

	// the following code is a workaround for the activity timers not reloading when persistent hive registry is used
	// and the settings are modified by the user
	HANDLE hevReloadActivityTimeouts = OpenEvent(EVENT_ALL_ACCESS ,FALSE ,_T("PowerManager/ReloadActivityTimeouts"));

	if (hevReloadActivityTimeouts)
	{
		RETAILMSG(1, (TEXT("PM timers reload\r\n")));
		SetEvent(hevReloadActivityTimeouts);
		CloseHandle(hevReloadActivityTimeouts);
	}

	// wait for everything to stablize and cards to mount
	Sleep(3000);

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Storage Card\\startup\\*.bat"), &fData ))) 
    {
		wcscpy(string,TEXT("/C CALL \"\\Storage Card\\startup\\"));
		wcscat(string,fData.cFileName);
		wcscat(string,TEXT("\""));
        if (!CreateProcess(TEXT("cmd.exe"),string,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! cmd %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: cmd %s \r\n"),string));
    }

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Storage Card\\startup\\*.exe"), &fData ))) 
	{
		wcscpy(string,TEXT("\\Storage Card\\startup\\"));
        if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));

		while (FindNextFile(hSearch, &fData))
		{
			wcscpy(string,TEXT("\\Storage Card\\startup\\"));
			if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
				RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
			else
				RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));
		}
		goto End;		// no more processing if storage card launch!
	}

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Mounted Volume\\startup\\*.bat"), &fData ))) 
    {
		wcscpy(string,TEXT("/C CALL \"\\Mounted Volume\\startup\\"));
		wcscat(string,fData.cFileName);
		wcscat(string,TEXT("\""));
        if (!CreateProcess(TEXT("cmd.exe"),string,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! cmd %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: cmd %s \r\n"),string));
    }

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Mounted Volume\\startup\\*.exe"), &fData ))) 
	{
		wcscpy(string,TEXT("\\Mounted Volume\\startup\\"));
        if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));

		while (FindNextFile(hSearch, &fData))
		{
			wcscpy(string,TEXT("\\Mounted Volume\\startup\\"));
			if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
				RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
			else
				RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));
		}
	}

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Hard Disk\\startup\\*.bat"), &fData ))) 
    {
		wcscpy(string,TEXT("/C CALL \"\\Hard Disk\\startup\\"));
		wcscat(string,fData.cFileName);
		wcscat(string,TEXT("\""));
        if (!CreateProcess(TEXT("cmd.exe"),string,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! cmd %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: cmd %s \r\n"),string));
    }

	if (INVALID_HANDLE_VALUE != (hSearch = FindFirstFile(TEXT("\\Hard Disk\\startup\\*.exe"), &fData ))) 
	{
		wcscpy(string,TEXT("\\Hard Disk\\startup\\"));
        if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
			RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
		else
			RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));

		while (FindNextFile(hSearch, &fData))
		{
			wcscpy(string,TEXT("\\Hard Disk\\startup\\"));
			if (!CreateProcess(wcscat(string,fData.cFileName),NULL,NULL,NULL,NULL,0,NULL,NULL,NULL,NULL))
				RETAILMSG(1, (TEXT("AutoLaunch: CreateProcess error! %s %d\r\n"),string,GetLastError()));
			else
				RETAILMSG(1, (TEXT("AutoLaunch: %s \r\n"),string));
		}
	}

End:
	// for registry init launch
	SignalStarted(_wtol(lpCmdLine));

    return 0;
}
