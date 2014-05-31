
#include "windows.h"
#include "am33x.h"
#include "am33x_oal_prcm.h"

#define CM3_UMEM_START_ADDR			(0x44D00000u)
#define CM3_UMEM_LENGTH				(16*1024)

static BOOL LoadFirmwareFile(LPCWSTR lpszFileName )
{
	HANDLE hLocalFile = NULL;
	DWORD dwFileSize;
	BYTE *lpBuffer = NULL;    
	BOOL bResult;
	DWORD dwBytesRead;
    
    void *cm3_start_addr = NULL;
    PHYSICAL_ADDRESS pa;
    DWORD size;
    
    BOOL rc = FALSE;
    
	RETAILMSG( 0, (L"%s()\r\n", TEXT(__FUNCTION__)) );
    
    pa.QuadPart = CM3_UMEM_START_ADDR;
    size = CM3_UMEM_LENGTH;
    cm3_start_addr = (void *)MmMapIoSpace(pa, size, FALSE);
    if (cm3_start_addr == NULL)
    {
        RETAILMSG(ZONE_ERROR, (L"CM3: ERROR: LoadFirmwareFile:Failed map CM3 UMEM \r\n"));
        goto cleanup;
    }

	// safety
	if ( !lpszFileName )
	{
		RETAILMSG(1, (L"CM3: Error: invalid parameters\r\n") );
		goto cleanup;
	}

	// open the file
    hLocalFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if ( hLocalFile == INVALID_HANDLE_VALUE )
	{
		RETAILMSG( 1, (L"CM3: Error: could not open file [%s]\r\n", lpszFileName) );
		goto cleanup;
	}  

    // get file size    
    dwFileSize = GetFileSize (hLocalFile, NULL);
	SetFilePointer(hLocalFile, 0, 0, FILE_BEGIN);

    RETAILMSG( 1, (L"CM3: input file [%s] is %d bytes long\r\n", lpszFileName, dwFileSize) );
    RETAILMSG( 0, (L"CM3: reading input file..\r\n") );

    // allocate buffer
	lpBuffer = (BYTE*)malloc( dwFileSize );
	if ( lpBuffer == NULL )
	{
		RETAILMSG( 1, (L"CM3: unable to allocate %d bytes for input buffer\r\n\r\n", dwFileSize) );
		goto cleanup;
	}

	// Read file
	bResult = ReadFile( hLocalFile, lpBuffer, dwFileSize, &dwBytesRead, NULL ); 
    if ( (bResult == FALSE) || (dwBytesRead != dwFileSize) )
    {
       RETAILMSG( 1, (L"CM3: error reading firmware file\r\n") );
        goto cleanup;
    }
    RETAILMSG( 1, (L"CM3: firmware file read ok!\r\n") );
    
	// completed ok
	/*	Load CM3 SW	*/
	memcpy(cm3_start_addr, (const void *)lpBuffer, dwBytesRead);

	
	/*	Release CM3 from reset	 and handshake*/
    PrcmCM3ResetAndHandshake();
	rc=TRUE;
	
cleanup:
	if ( lpBuffer != NULL )
	{
		free( lpBuffer );
	}
	if ( hLocalFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hLocalFile );
	}
    if (cm3_start_addr != NULL) 
        MmUnmapIoSpace((VOID*)cm3_start_addr, CM3_UMEM_LENGTH);
	return rc;
}

void download_CM3()
{

    RETAILMSG(0, (L"+download_CM3: loading M3 \r\n"));
	/*	Load CM3 SW	and release CM3 from reset	*/
    //TODO: Read the firmware file name from registry ??
	LoadFirmwareFile(L"\\windows\\firmware.bin");

    RETAILMSG(0, (L"+download_CM3: done with M3 \r\n"));  
}


//------------------------------------------------------------------------------
//
//  Function:  CM3_Init
//
//  Called by device manager to initialize device.
//
DWORD
CM3_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    UNREFERENCED_PARAMETER(szContext);
	UNREFERENCED_PARAMETER(pBusContext);
	
    download_CM3();
    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  CM3_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
CM3_Deinit(
    DWORD context
    )
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
CM3_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function closes the device context.
//
BOOL
CM3_Close(
    DWORD context
    )
{
	UNREFERENCED_PARAMETER(context);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  CM3_IOControl
//
//  This function sends a command to a device.
//
BOOL
CM3_IOControl(
    DWORD context, 
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{

    UNREFERENCED_PARAMETER(context); 
    UNREFERENCED_PARAMETER(code); 
    UNREFERENCED_PARAMETER(pInBuffer); 
    UNREFERENCED_PARAMETER(inSize); 
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize); 
    UNREFERENCED_PARAMETER(pOutSize);
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
	UNREFERENCED_PARAMETER(pReserved);

    switch (reason)
        {
        case DLL_PROCESS_ATTACH:            
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//------------------------------------------------------------------------------




