// All rights reserved ADENEO EMBEDDED 2010
// All rights reserved Texas Instruments, Inc. 2011

#include "stdafx.h"
#include <sdk_spi.h>

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

BOOL InSPI(ULONG argc, LPWSTR args[])
// function is not completed
{
    BOOL rc = FALSE;

	DWORD config;

    UINT32 address, offset;
    HANDLE hSPI = INVALID_HANDLE_VALUE;

	if (argc < 2){
        _tprintf(L"Missing address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

    address = GetHex(args[0]);
    offset  = GetHex(args[1]);

    hSPI = SPIOpen(_T("SPI1:"));
    if (hSPI == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open SPI1: device driver!\r\n");
        goto cleanUp;
    }

	config = 0;

    rc = TRUE;

cleanUp:
    if (hSPI != INVALID_HANDLE_VALUE) SPIClose(hSPI);
    return rc;
}
//-----------------------------------------------------------------------------
BOOL OutSPI(ULONG argc, LPWSTR args[])
{
    BOOL rc = FALSE;

    UINT32 config, data;
    HANDLE hSPI = INVALID_HANDLE_VALUE;
	DWORD  len, res;

	if (argc < 2){
        _tprintf(L"Missing address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

    config = GetHex(args[0]);
    len    = GetHex(args[1]);
    data   = GetHex(args[2]);

    hSPI = SPIOpen(_T("SPI1:"));
    if (hSPI == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open SPI1: device driver!\r\n");
        goto cleanUp;
    }

	if (!SPIConfigure(hSPI, 0, config)){
		goto cleanUp;
	}

	res = SPIWrite(hSPI, len, &data);
    _tprintf(L"SPIWrite res = %d\r\n", res);

    rc = TRUE;

cleanUp:
    if (hSPI != INVALID_HANDLE_VALUE) SPIClose(hSPI);
    return rc;
}

BOOL OutInSPI(ULONG argc, LPWSTR args[])
{
    BOOL rc = FALSE;

    UINT32 config, data, rdata;
    HANDLE hSPI = INVALID_HANDLE_VALUE;
	DWORD  len, res;

	if (argc < 2){
        _tprintf(L"Missing address, and/or offset to read from!\r\n");
        goto cleanUp;
    }

    config = GetHex(args[0]);
    len    = GetHex(args[1]);
    data   = GetHex(args[2]);

    hSPI = SPIOpen(_T("SPI1:"));
    if (hSPI == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open SPI1: device driver!\r\n");
        goto cleanUp;
    }

	if (!SPIConfigure(hSPI, 0, config)){
		goto cleanUp;
	}

	res = SPIWriteRead(hSPI, len, &data, &rdata);
    _tprintf(L"SPIWrite res = %d 0x%08X\r\n", res, rdata);

    rc = TRUE;

cleanUp:
    if (hSPI != INVALID_HANDLE_VALUE) SPIClose(hSPI);
    return rc;
}


DWORD SPIWriteRead(UINT8 *outdata, DWORD outlen, UINT8 *indata, DWORD inlen )
{
    UINT32 config;
    HANDLE hSPI = INVALID_HANDLE_VALUE;
	DWORD  res = 0;

    config = 0x001123C0;

    hSPI = SPIOpen(_T("SPI1:"));
    if (hSPI == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open SPI1: device driver!\r\n");
        goto cleanUp1;
    }

	SPILockController(hSPI, 0xFFFFF);
	SPIEnableChannel(hSPI);

	if (!SPIConfigure(hSPI, 0, config))
		goto cleanUp;

	if (outlen > 256)
		outlen = 256;

	res = SPIWrite(hSPI, outlen, outdata);
    _tprintf(L"SPIWrite res = %d\r\n", res);

	if (inlen > 0){
		config = 0x001113C0;
		if (!SPIConfigure(hSPI, 0, config)) goto cleanUp;
		res = SPIRead(hSPI, inlen, indata);
		_tprintf(L"SPIRead res = %d\r\n", res);
	}

cleanUp:
	SPIDisableChannel(hSPI);
	SPIUnlockController(hSPI);

cleanUp1:

	if (hSPI != INVALID_HANDLE_VALUE) SPIClose(hSPI);

	return res;
}

UINT8 obuf[512];
UINT8 ibuf[512];

void EEStatus(){
	DWORD res;
	obuf[0] = 0x05;
	
	res = SPIWriteRead(obuf, 1, ibuf, 1);
    _tprintf(L"EEStatus res = %d; status 0x%02X \r\n", res, ibuf[0]);
}

void EEread(ULONG argc, LPWSTR args[])
{
	DWORD ilen;
	DWORD res;
	int   i;

	obuf[0] = 0x03;
	obuf[1] = GetHex(args[0]);
	obuf[2] = GetHex(args[1]);
	obuf[3] = GetHex(args[2]);

	ilen    = GetHex(args[3]);

	res = SPIWriteRead(obuf, 4, ibuf, ilen);
    _tprintf(L"EEread res = %d;\r\n", res);

	if (res > 0){
		for (i = 0; i<res; i++){
			_tprintf(L"%02X%s", ibuf[i], (((i+1)& 0xf) == 0)? L"\r\n" : L" ");
		}
	}
    _tprintf(L"\r\n");

}

void testSPI(ULONG argc, LPWSTR args[])
{
    UINT32 config, data;
	UINT8  rdata[512];
    HANDLE hSPI = INVALID_HANDLE_VALUE;
	DWORD  len, res;
	int    i;

    config = 0x001123C0;
    len    = 1;
    data   = 0x9F;

    hSPI = SPIOpen(_T("SPI1:"));
    if (hSPI == INVALID_HANDLE_VALUE){
        _tprintf(L"Can't open SPI1: device driver!\r\n");
        goto cleanUp1;
    }

	SPILockController(hSPI, 0xFFFFF);
	SPIEnableChannel(hSPI);
	if (!SPIConfigure(hSPI, 0, config)) goto cleanUp;
	res = SPIWrite(hSPI, len, &data);
    _tprintf(L"SPIWrite res = %d\r\n", res);

    config = 0x001113C0;
	len    = 3;

	if (!SPIConfigure(hSPI, 0, config)) goto cleanUp;
	res = SPIRead(hSPI, len, rdata);
    _tprintf(L"SPIRead res = %d\r\n", res);
	if (res > 0){
		for (i=0; i<res; i++){
		    _tprintf(L"%02X  ", rdata[i]);
		}
	    _tprintf(L"\r\n", res);
	}

cleanUp:
	SPIDisableChannel(hSPI);
	SPIUnlockController(hSPI);

cleanUp1:

	if (hSPI != INVALID_HANDLE_VALUE) SPIClose(hSPI);
}

//================================================================================

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{    
    HANDLE hI2C;
    _tprintf(_T("Simple --SPI-- test application\n"));
    if (argc < 2 )
    {
        _tprintf(_T("Invalid number of parameters\nusage:%s TBD\n"),argv[0]);
        return -1;
    }

	if  (wcscmp(argv[1], L"in" ) == 0){
		InSPI(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"out" ) == 0){
		OutSPI(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"oi" ) == 0){
		OutInSPI(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"status" ) == 0){
		EEStatus();
	} else if(wcscmp(argv[1], L"read" ) == 0){
		EEread(argc-2, &argv[2]);
	} else if(wcscmp(argv[1], L"test" ) == 0){
		testSPI(argc-2, &argv[2]);
	} else {
        _tprintf(_T("%s %s\n"),argv[0],argv[1]);
	}
	
    return 0;
}

