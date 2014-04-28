// All rights reserved ADENEO EMBEDDED 2010
// All rights reserved Texas Instruments, Inc. 2011

#include "stdafx.h"
#include "..\..\APP\COMMON\i2cproxy.h"

#define MESSAGE_BUFFER_SIZE     280
#define dimof(x)    (sizeof(x)/sizeof((x)[0]))

UINT32 GetHex(LPCWSTR string)
{
    UINT32 result;
    UINT32 ix = 0;

    result = 0;
    while (string != NULL){
       if ((string[ix] >= L'0') && (string[ix] <= L'9')){
            result = (result << 4) + (string[ix] - L'0');
            ix++;
       } else if ((string[ix] >= L'a') && (string[ix] <= L'f')){
            result = (result << 4) + (string[ix] - L'a' + 10);
            ix++;
       } else if (string[ix] >= L'A' && string[ix] <= L'F') {
            result = (result << 4) + (string[ix] - L'A' + 10);
            ix++;
       } else {
            break;
       }
    }
    return result;
}

BOOL SetSlaveAddress( HANDLE hI2C, DWORD address, DWORD mode)
{
    BOOL rc;

    rc = DeviceIoControl(
        hI2C, IOCTL_I2C_SET_SLAVE_ADDRESS, &address, sizeof(address), NULL, 0,NULL, NULL);

    rc = DeviceIoControl(
        hI2C, IOCTL_I2C_SET_SUBADDRESS_MODE, &mode, sizeof(mode), NULL, 0,NULL, NULL);

    return rc;
}


//-----------------------------------------------------------------------------
DWORD WriteI2C(HANDLE hI2C, UINT16 subaddr, VOID* pBuffer, DWORD count,DWORD *pWritten)
{
    SetFilePointer(hI2C, subaddr, NULL, FILE_BEGIN);
    return WriteFile(hI2C, pBuffer, count, pWritten, NULL);
}

//-----------------------------------------------------------------------------
DWORD ReadI2C(HANDLE hI2C, UINT16 subaddr, VOID* pBuffer, DWORD count, DWORD *pRead)
{
    SetFilePointer(hI2C, subaddr, NULL, FILE_BEGIN);
    return ReadFile(hI2C, pBuffer, count, pRead, NULL);
}


// I2CId address offset count mode
BOOL InI2C(ULONG argc, LPWSTR args[])
{
    BOOL rc = FALSE;
    TCHAR const szI2C1[] = _T("I2C1:");
    TCHAR const szI2C2[] = _T("I2C2:");
    TCHAR const szI2C3[] = _T("I2C3:");
    TCHAR const szI2C4[] = _T("I2C4:");

    TCHAR const *szI2C;
    int i2cIndex;
    UINT32 address, offset, count, offset1;
    UINT32 mode = 2;
    DWORD readCount;
    HANDLE hI2C = INVALID_HANDLE_VALUE;
    UCHAR buffer[MESSAGE_BUFFER_SIZE];
    WCHAR line[80];
    ULONG ix, ip;


	if (argc < 3){
        _tprintf(L"Missing i2c id, address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

	i2cIndex = GetHex(args[0]);
    switch (i2cIndex){
        case 1:szI2C = szI2C1;break;
        case 2:szI2C = szI2C2;break;
        case 3:szI2C = szI2C3;break;
        case 4:szI2C = szI2C4;break;
        default:
            _tprintf(L"Invalid i2c identifier, must be 1 or 2!\r\n");
            goto cleanUp;
    }

    address = GetHex(args[1]);
    offset  = GetHex(args[2]);
	offset1 = ((offset >> 8) & 0xff) | ((offset << 8) & 0xff00); 
	count   = (argc > 3)? GetHex(args[3]) : 1;
	mode    = (argc > 4)? GetHex(args[4]) : 2;
    if (count == 0) count = 1;

    hI2C = CreateFile(szI2C, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hI2C == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open %s: device driver!\r\n", szI2C);
        goto cleanUp;
    }

    // set slave address
    if (!SetSlaveAddress(hI2C, address, mode)){
        _tprintf(L"Failed set I2C slave address\r\n");
        goto cleanUp;
    }

    // read data
    SetFilePointer(hI2C, offset1, NULL, FILE_BEGIN);
    if (!ReadFile(hI2C, buffer, count, &readCount, NULL)){
        _tprintf(L"Failed reading value(s)\r\n");
        goto cleanUp;
    }

    _tprintf(L"Read %d of %d byte(s)\r\n", count, readCount);
    PREFAST_SUPPRESS(12008, "No offerflow/underflow possible there.");
    for (ix = 0, ip = 0; ix < readCount; ix++){
        if ((ix & 0x0F) == 0){
            StringCchPrintf(&line[ip], dimof(line) - ip, L"%04x:", offset + ix);
            ip += lstrlen(&line[ip]);
        }
        StringCchPrintf(&line[ip], dimof(line) - ip, L" %02x", buffer[ix]);
        ip += lstrlen(&line[ip]);
        if ((ix & 0x0F) == 0x0F){
            _tprintf(line);
            _tprintf(L"\r\n");
            ip = 0;
        }
    }
    if (ip > 0){
        _tprintf(line);
        _tprintf(L"\r\n");
    }

    rc = TRUE;

cleanUp:
    if (hI2C != INVALID_HANDLE_VALUE) CloseHandle(hI2C);
    return rc;
}
//-----------------------------------------------------------------------------

BOOL OutI2C(ULONG argc, LPWSTR args[])
{
    BOOL rc = FALSE;
    TCHAR const szI2C1[] = _T("I2C1:");
    TCHAR const szI2C2[] = _T("I2C2:");
    TCHAR const szI2C3[] = _T("I2C3:");
    TCHAR const szI2C4[] = _T("I2C4:");

    TCHAR const *szI2C;
    int i2cIndex;
    UINT32 address, offset, data, offset1;
    UINT32 mode = 2;
    DWORD readCount;
    HANDLE hI2C = INVALID_HANDLE_VALUE;

    if (argc < 3){
        _tprintf(L"Missing i2c id, address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

	i2cIndex = GetHex(args[0]);
    switch (i2cIndex){
        case 1: szI2C = szI2C1;break;
        case 2: szI2C = szI2C2;break;
        case 3: szI2C = szI2C3;break;
        case 4: szI2C = szI2C4;break;
        default:
            _tprintf(L"Invalid i2c identifier, must be 1 or 2!\r\n");
            goto cleanUp;
        }

    address = GetHex(args[1]);
    offset  = GetHex(args[2]);
	offset1 = ((offset >> 8) & 0xff) | ((offset << 8) & 0xff00); 
    data    = GetHex(args[3]);
	mode    = (argc > 4)? GetHex(args[4]) : 2;

    hI2C = CreateFile(szI2C, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hI2C == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open %s: device driver!\r\n", szI2C);
        goto cleanUp;
    }

    if (!SetSlaveAddress(hI2C, address, mode)){
        _tprintf(L"Failed set I2C slave address\r\n");
        goto cleanUp;
    }

    if (!WriteI2C(hI2C, (UINT16)offset1, &data, 1, &readCount)){
        _tprintf(L"Failed writing value(s)\r\n");
        goto cleanUp;
    }

    rc = TRUE;

cleanUp:
    if (hI2C != INVALID_HANDLE_VALUE) CloseHandle(hI2C);
    return rc;
}


BOOL OutI2CExt(UINT inx, UINT32 address, ULONG argc, LPWSTR args[])
{
    BOOL rc = FALSE;
    TCHAR const szI2C1[] = _T("I2C1:");
    TCHAR const szI2C2[] = _T("I2C2:");
    TCHAR const szI2C3[] = _T("I2C3:");
    TCHAR const szI2C4[] = _T("I2C4:");

    TCHAR const *szI2C;
    UINT32 data;
    DWORD readCount;
    HANDLE hI2C = INVALID_HANDLE_VALUE;

    if (argc < 1){
        _tprintf(L"Missing i2c id, address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

	data = GetHex(args[0]);
	szI2C = (inx == 1) ? szI2C1 : szI2C2; 


    hI2C = CreateFile(szI2C, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hI2C == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open %s: device driver!\r\n", szI2C);
        goto cleanUp;
    }

    if (!SetSlaveAddress(hI2C, address, 0)){
        _tprintf(L"Failed set I2C slave address\r\n");
        goto cleanUp;
    }

    if (!WriteI2C(hI2C, (UINT16)0, &data, 2, &readCount)){
        _tprintf(L"Failed writing value(s)\r\n");
        goto cleanUp;
    }

    rc = TRUE;

cleanUp:
    if (hI2C != INVALID_HANDLE_VALUE) CloseHandle(hI2C);
    return rc;
}

//================================================================================

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{    
    HANDLE hI2C;
    _tprintf(_T("Simple --I2C-- test application\n"));
    if (argc < 4 )
    {
        _tprintf(_T("Invalid number of parameters\nusage:%s <in/out> <1/2> <SA> <offset> <data/count> [<mode>]\n"),
			argv[0]);
        return -1;
    }

	if  (wcscmp(argv[1], L"in" ) == 0){
		InI2C(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"out" ) == 0){
		OutI2C(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"oex1" ) == 0){
		OutI2CExt(1, 0x20, argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"oex2" ) == 0){
		OutI2CExt(2, 0x20, argc-2, &argv[2]);
	} else {
        _tprintf(_T("%s %s\n"),argv[0],argv[1]);
	}
	
    return 0;
}

