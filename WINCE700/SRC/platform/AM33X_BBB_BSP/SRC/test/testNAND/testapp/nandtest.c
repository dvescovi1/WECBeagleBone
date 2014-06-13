/*
 * Copyright (c) 2011 Texas Instruments Incorporated http://www.ti.tom/
 * ALL RIGHTS RESERVED
 * 
 * File:  nandtest.c
 *
 */

#pragma warning(push)
#pragma warning(disable: 4115 4214)
#include <windows.h>
#include <Winbase.h>
#include <ceddk.h>
#pragma warning(pop)
#include <nandtest.h>

extern int  CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);
#define SECTOR_SIZE 2048

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    va_start(al, pszFormat);
    vwprintf(pszFormat, al);
    va_end(al);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    HANDLE hDevice=NULL;
    HANDLE hFile;
    TCHAR *argv[20];
    int argc;
    BOOL fSuccess = FALSE;
    DWORD sector, nbRead=0;
    UCHAR indata[SECTOR_SIZE*2], outdata[SECTOR_SIZE];

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);

    // Parse command line
    argc = CreateArgvArgc(TEXT("nandtest" ), argv, lpCmdLine);
    if (argc < 5 )
    {
        RetailPrint(TEXT("Usage: nandtest <sector> <good data file> <bad data file> <ouput file>\n"));
        return 1;
    }

     /* parse parameters */
     if((swscanf(argv[1], TEXT("%d"), &sector)) ==0)
    {
        RetailPrint(TEXT("Usage: nandtest <ecc type> <sector> <good data file> <bad data file> <ouput file>\n"));
        return 1;
    }

    RetailPrint(TEXT(" nandtest: sector=%d good_file=%s, bad file=%s, out_file=%s\n"),
		sector, argv[2], argv[3], argv[4]);

    hFile = CreateFile(argv[2],GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile == (HANDLE)0xffffffff)
    {
        RetailPrint(TEXT("(create file filed for %s, error=%d\r\n"),argv[2], GetLastError());
        goto cleanUp;
    }
		
    if (!ReadFile(hFile,&indata[0],SECTOR_SIZE,&nbRead,NULL))
    {
        RetailPrint(TEXT("(nbRead (%d), read from good data file(%s) handle=%p, failed for %d\r\n"),nbRead, argv[2], hFile, GetLastError());
        CloseHandle(hFile);
	 goto cleanUp;
    }
    CloseHandle(hFile);

    nbRead = 0;
    hFile = CreateFile(argv[3],GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile == (HANDLE)0xffffffff)
    {
        RetailPrint(TEXT("(create file filed for %s, error=%d\r\n"),argv[3], GetLastError());
	 goto cleanUp;
    }
    if (!ReadFile(hFile,&indata[SECTOR_SIZE],SECTOR_SIZE,&nbRead,NULL))
    {
        RetailPrint(TEXT("(nbRead (%d), read from bad data file failed\r\n"),nbRead);
        CloseHandle(hFile);
	 goto cleanUp;
    }  
    CloseHandle(hFile);

    hDevice = CreateFile(L"DFT1:", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        RetailPrint(TEXT("nand test: Failed to open NAND test driver\n"));
        return 1;
    }
	
    fSuccess = DeviceIoControl(hDevice, NAND_SET_SECTOR, (VOID *)&sector, sizeof(DWORD), NULL, 0, NULL, NULL);

    if(fSuccess)
    	fSuccess = DeviceIoControl(hDevice, NAND_ECC_CORRECTION, (VOID *)&indata, SECTOR_SIZE*2, (VOID *)outdata, SECTOR_SIZE, NULL, NULL);
    RetailPrint(TEXT("nand test: ecc correction is %s\n"),  (fSuccess==TRUE) ? L"successful" : L"failed");

    if(fSuccess)
    {
        hFile = CreateFile(argv[4],GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if (!WriteFile(hFile,outdata,sizeof(outdata),&nbRead,NULL))
        {
            RetailPrint(TEXT("WRITE to out data file failed\r\n"),nbRead);
        }  
        CloseHandle(hFile);
    }		
	
cleanUp:
    if(hDevice) CloseHandle(hDevice);

    return 0;
}
