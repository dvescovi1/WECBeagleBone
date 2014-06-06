
//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
// File:  uarttest_device.c
//
// Serial driver test
//
// Usage:
// 
//         UartTest.exe -d [device name]
//                      -b [baud rate]
//                      -o [operation], w=write, r=read, l=loopback, a=async test
//                      -t [operation timeout (ms)]
//                      -l [transfer length]
//                      -c [cycle count]
//                      -x [duration (sec)], async test only 
//                      -y [max transfer size, 1-4096], async test only
//                      -f use flow control
//                      -m master mode
//                      -? usage
// 


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


// Max transfer size for async tests
#define MIN_ASYNCBUFFER_SIZE            1
#define MAX_ASYNCBUFFER_SIZE       0x1000

#define MAX_TESTPATTERN_SIZE         1024
#define MAX_PORTNAME_LENGTH           128

#define MIN_DATABLOCK_SIZE              1
#define DEFAULT_DATABLOCK_SIZE       4096
#define MAX_DATABLOCK_SIZE       (32*1024)

#define DEFAULT_TEST_DURATION          60
#define DEFAULT_BAUD_RATE          115200
#define DEFAULT_COMMS_TIMEOUT       15000
#ifdef UARTTEST_CE_VERSION
    #define DEFAULT_DEVICE_NAME   L"COM1:"
#else
	#define DEFAULT_DEVICE_NAME    L"COM1"
#endif

#define STRESS_BLOCKID_SIZE             4

typedef struct
{
    wchar_t    szName[MAX_PORTNAME_LENGTH + 8];
    DWORD      dwBaudRate;
    BYTE       nByteSize;
    BYTE       nStopBits;
    BYTE       nParity;
    BOOL       fFlowControlOn;
    DWORD      dwCommsTimeout;

	HANDLE     hDev;
#ifndef UARTTEST_CE_VERSION
    OVERLAPPED gOvpRead;
    OVERLAPPED gOvpWrite;
#endif
    
} TEST_PORT_SETUP;


typedef struct
{
    TEST_PORT_SETUP port;    
    wchar_t    op;
    DWORD      dwDataBlockSize;
    DWORD      dwNumDataBlocks;
    DWORD      dwTestDuration;
    DWORD      dwMaxfer;
    BOOL       fMaster;

    char       pTestPattern[MAX_TESTPATTERN_SIZE + 8];
    DWORD      dwTestPatternSize;

} TEST_SETUP;

typedef struct
{
    HANDLE hThread;
    HANDLE hStop;
    int nMaxfer;
    CRITICAL_SECTION csLock;
    unsigned __int64 nTransferred;
    unsigned __int64 nErrors;    
    DWORD dwTime;
	TEST_PORT_SETUP port;
   
} TEST_ASYNC_THREAD;

typedef struct
{
	char  cID1[STRESS_BLOCKID_SIZE];
    DWORD dwDataBlockSize;
    DWORD dwNumDataBlocks;
	DWORD dwBaudRate;
	BYTE  nParity;
	char  cID2[STRESS_BLOCKID_SIZE];

} TEST_STRESS_ACTION;

#define NUM_STRESS_ACTIONS    14
TEST_STRESS_ACTION StressAction[NUM_STRESS_ACTIONS] =
{ { {0}, 4096, 15, 115200, NOPARITY,   {0} },
  { {0}, 1024,  5,  38400, NOPARITY,   {0} },
  { {0}, 1024,  5,  57600, NOPARITY,   {0} },
  { {0},  256,  9,  19200, NOPARITY,   {0} },
  { {0}, 1024,  5,  14400, NOPARITY,   {0} },
  { {0}, 2048,  5,   9600, NOPARITY,   {0} },
  { {0}, 1024,  5, 115200, EVENPARITY, {0} },
  { {0},  512,  5,  19200, NOPARITY,   {0} },
  { {0},  671,  6,   4800, NOPARITY,   {0} },
  { {0}, 1024,  6, 115200, ODDPARITY,  {0} },
  { {0},  200,  5,   1200, NOPARITY,   {0} },
  { {0}, 1024,  7,  38400, NOPARITY,   {0} },
  { {0},  333,  7,   2400, EVENPARITY, {0} },
  { {0},    0,  0,      0, 0,          {0} }
};

//////////////////////////////////////////////////////////////////////////////////////
// Local functions
//
void   DebugPrint(wchar_t *pszFormat, ...);

BOOL   DecodeCommandLine(int argc, wchar_t **argv, TEST_SETUP &Setup);
void   ShowHelpInfo(void);
void   ShowTestConfiguration(TEST_SETUP &Setup);

BOOL   OpenDevice(TEST_PORT_SETUP &pSetup, BOOL PurgeBuffers=FALSE);
void   CloseDevice(TEST_PORT_SETUP &pSetup);

BOOL   WriteTest(TEST_SETUP &pSetup);
BOOL   ReadTest(TEST_SETUP &pSetup);
BOOL   LoopbackTest(TEST_SETUP &Setup);
BOOL   AsyncTest(TEST_SETUP &Setup);
BOOL   StressTest(TEST_SETUP &Setup);

DWORD WriterThread(TEST_ASYNC_THREAD *pThread);
DWORD ReaderThread(TEST_ASYNC_THREAD *pThread);

static BOOL BufferWrite(TEST_PORT_SETUP &pSetup,
						char *pData, DWORD dwDataLen,
						DWORD &dwDataSent, DWORD *TimeTaken,
						wchar_t *pszMsg);

static BOOL BufferRead(TEST_PORT_SETUP &pSetup,
					   char *pData, DWORD dwDataLen,
					   DWORD &dwDataRead, DWORD *TimeTaken,
					   wchar_t *pszMsg);



//////////////////////////////////////////////////////////////////////////////////////
// Global Variables
//
BOOL  g_bCEOutputToDebug = FALSE;
BOOL  g_dwVerboseOutput = 0;
BOOL  g_bPurgeCommsBuffers = TRUE;

//////////////////////////////////////////////////////////////////////////////////////
// Entry point
//

#ifdef UARTTEST_CE_VERSION
int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
#else
int wmain(int argc, wchar_t **argv)
#endif
{
    TEST_SETUP Setup;

#ifdef UARTTEST_CE_VERSION
	UNREFERENCED_PARAMETER(envp);
#endif 
    DebugPrint(L"\n================================== UARTTEST ===================================\n\n");

    if ( DecodeCommandLine(argc, argv, Setup) )
    {
        
        ShowTestConfiguration(Setup);
        
        switch (Setup.op)
        {
            case L'w':
                WriteTest(Setup);
                break;

            case L'r':
                ReadTest(Setup);
                break;

            case L'l':
                LoopbackTest(Setup);
                break;

            case L'a':
                AsyncTest(Setup);
                break;

            case L's':
                StressTest(Setup);
                break;

            default:
                DebugPrint(L"Unknown test operation requested\n");
                break;

        }
    
    }
 
    DebugPrint(L"\n===============================================================================\n");

	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
// Debug printing
//
void DebugPrint(wchar_t *pszFormat, ...)
{
    va_list al;
   
    va_start(al, pszFormat);

#ifndef UARTTEST_CE_VERSION

    vwprintf(pszFormat, al);

#else

    if ( g_bCEOutputToDebug )
    {
        wchar_t szTempFormat[1024];
        wchar_t szTemp[1024];

        swprintf(szTempFormat, L"UartTest: %s", pszFormat);
        pszFormat = szTempFormat;

        vswprintf(szTemp, pszFormat, al);
        OutputDebugString(szTemp);
    }
    else
    {
        vwprintf(pszFormat, al);
    }


#endif

    va_end(al);

}


//////////////////////////////////////////////////////////////////////////////////////
// 
//
BOOL DecodeCommandLine(int argc, wchar_t **argv, TEST_SETUP &Setup)
{
    int iTmp;
    BOOL fValidOptions = TRUE;
    BOOL fShowHelp = FALSE;
	BOOL fGotTestDuration = FALSE;

	// Initialise test setup data
	memset(&Setup, 0, sizeof(Setup));
	{
		char *pTmp;
		
		pTmp = "The quick brown fox jumped over the lazy dog\n";
		Setup.dwTestPatternSize = (DWORD)strlen(pTmp);
		memcpy(Setup.pTestPattern, pTmp, Setup.dwTestPatternSize);
	}
	Setup.op = L'w';
	wcscpy(Setup.port.szName, DEFAULT_DEVICE_NAME);
    Setup.port.dwBaudRate = DEFAULT_BAUD_RATE;
	Setup.port.nByteSize = 8;
	Setup.port.nStopBits = ONESTOPBIT;
	Setup.port.nParity = NOPARITY;
	Setup.port.dwCommsTimeout = DEFAULT_COMMS_TIMEOUT;
	Setup.dwDataBlockSize = DEFAULT_DATABLOCK_SIZE;
    Setup.dwNumDataBlocks = 1;
    Setup.dwTestDuration = DEFAULT_TEST_DURATION;
    Setup.dwMaxfer = 0;




    // Look for -Z option first as it decides how user output is handled
    for (iTmp = 0; iTmp < argc; iTmp++)
    {
        if ( _wcsicmp(argv[iTmp], L"-z") == 0 )
        {
            g_bCEOutputToDebug = TRUE;
        }
    }    

    DebugPrint(L"Parsing command line\n");

    while (argc)
    {
        if (argv[ 0 ][ 0 ] == L'-')
        {
            switch (argv[ 0 ][ 1 ])
            {
            case L'd': // Device
            case L'D':
                if (argc < 2)
                {
                    DebugPrint(L"ERROR: Invalid device name\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                wcscpy(Setup.port.szName, argv[ 1 ]);
                argv += 2;
                argc -= 2;
                continue;

            case L'b': // Baud
            case L'B':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.port.dwBaudRate) != 1))
                {
                    DebugPrint(L"ERROR: Invalid baud rate\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;

            case L'p': // Parity
            case L'P':
                if (argc < 2)
                {
                    DebugPrint(L"ERROR: Invalid parity name\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                if ( wcscmp(L"NONE", argv[1]) == 0 )
                {
                   Setup.port.nParity = NOPARITY; 
                }
                else if ( wcscmp(L"EVEN", argv[1]) == 0 )
                {
                    Setup.port.nParity = EVENPARITY;
                }
                else if ( wcscmp(L"ODD", argv[1]) == 0 )
                {
                    Setup.port.nParity = ODDPARITY;
                }
                else if ( wcscmp(L"MARK", argv[1]) == 0 )
                {
                    Setup.port.nParity = MARKPARITY;
                }
                else if ( wcscmp(L"SPACE", argv[1]) == 0 )
                {
                    Setup.port.nParity = SPACEPARITY;
                }
                else
                {
                    DebugPrint(L"ERROR: Invalid parity name\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                argv += 2;
                argc -= 2;
                continue;

            case L'l': // Length
            case L'L':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.dwDataBlockSize) != 1))
                {
                    DebugPrint(L"ERROR: Invalid length\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;

            case L'c': // Count
            case L'C':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.dwNumDataBlocks) != 1))
                {
                    DebugPrint(L"ERROR: Invalid count\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;

            case L't': // Timeout
            case L'T':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.port.dwCommsTimeout) != 1))
                {
                    DebugPrint(L"ERROR: Invalid timeout\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;

            case L'x': // Duration
            case L'X':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.dwTestDuration) != 1))
                {
                    DebugPrint(L"ERROR: Invalid duration\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                fGotTestDuration = TRUE;
				continue;

            case L'y': // Max transfer size for async tests
            case L'Y':
                if ((argc < 2) || (swscanf(argv[ 1 ], L"%u", &Setup.dwMaxfer) != 1))
                {
                    DebugPrint(L"ERROR: Invalid maxfer\n");
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;
                
            case L'o': // Operation
            case L'O':
                if (argc < 2)
				{
					fValidOptions = FALSE;
                    goto _done;
				}
#pragma warning (disable : 4996)
                wcslwr(argv[ 1 ]);
#pragma warning (disable : 4996)
                Setup.op = argv[ 1 ][ 0 ];
                if ((Setup.op != L'w') && // write
                    (Setup.op != L'r') && // read
                    (Setup.op != L'l') && // loopback
                    (Setup.op != L's') && // stress
                    (Setup.op != L'a'))   // async
                {
                    DebugPrint(L"ERROR: Invalid operation\n");
	                fValidOptions = FALSE;
                    goto _done;
                }
                
                argv += 2;
                argc -= 2;
                continue;

            case L'f': // Flow
            case L'F':
                Setup.port.fFlowControlOn = TRUE;
                argv ++;
                argc --;
                continue;

            case L'm': // Master
            case L'M':
                Setup.fMaster = TRUE;
                argv ++;
                argc --;
                continue;            

            case L'v': // Verbose output
            case L'V':
                g_dwVerboseOutput++;
                argv ++;
                argc --;
                continue;            

			case L'z': // Output to OutputDebugString
            case L'Z':
                // already been handled
                argv ++;
                argc --;
                continue;

            case L'?': // Usage
            case L'h':
            case L'H':
                fShowHelp = TRUE;
                argv ++;
                argc --;
                continue;

            default:
                DebugPrint(L"ERROR: Invalid flag %s found\n", argv[0]);
                argv ++;
                argc --;
                fValidOptions = FALSE;
                continue;            

            }
        }

        argc --;
        argv ++;
    }

_done:

    if ( !fValidOptions || fShowHelp )
    {
        ShowHelpInfo();
        return FALSE;
    }

    // Adjust limits
	if ( Setup.dwDataBlockSize < MIN_DATABLOCK_SIZE )
	{
		Setup.dwDataBlockSize = MIN_DATABLOCK_SIZE;
	}
	else if ( Setup.dwDataBlockSize > MAX_DATABLOCK_SIZE )
	{
		Setup.dwDataBlockSize = MAX_DATABLOCK_SIZE;
	}

	if ( Setup.dwMaxfer < MIN_ASYNCBUFFER_SIZE )
	{
		Setup.dwMaxfer = MIN_ASYNCBUFFER_SIZE;
	}
	else if ( Setup.dwMaxfer > MAX_ASYNCBUFFER_SIZE )
    {
        Setup.dwMaxfer = MAX_ASYNCBUFFER_SIZE;
    }
	if ( (Setup.op == L's') && !fGotTestDuration )
	{
		// Default stress test duration is 10 mins
		Setup.dwTestDuration = 600;
	}

    return fValidOptions;
}




//////////////////////////////////////////////////////////////////////////////////////
// 
//
void ShowHelpInfo(void)
{
    DebugPrint(L"\n");
    DebugPrint(L"Usage:\n");
    DebugPrint(L"    UartTest.exe [options]\n");
    DebugPrint(L"Options:\n");
    DebugPrint(L"    -d device_name        Default is %s\n",
		       DEFAULT_DEVICE_NAME);
    DebugPrint(L"    -b baud_rate          Default is %u\n",
		       DEFAULT_BAUD_RATE);
    DebugPrint(L"    -p parity             Use NONE or ODD or EVEN or MARK or SPACE>\n");
    DebugPrint(L"                          (default is NONE)\n");
    DebugPrint(L"    -o test_type          w=write, r=read, l=loopback, a=asynchronous, s=stress\n");
    DebugPrint(L"                          (default is write)\n");
    DebugPrint(L"    -t timeout            Operation timeout in milliSeconds [%u]\n",
		       DEFAULT_COMMS_TIMEOUT);
    DebugPrint(L"    -l block_size         Size of data blocks transferred [%u..%u]\n",
		       MIN_DATABLOCK_SIZE, MAX_DATABLOCK_SIZE);
    DebugPrint(L"    -c block_count        Number of data blocks transferred\n");
    DebugPrint(L"    -x duration           Async/Stress test duration in seconds [%u]\n",
		       DEFAULT_TEST_DURATION);
    DebugPrint(L"    -y max_buffer_size    Async test max transfer size [%u..%u]\n",
		       MIN_ASYNCBUFFER_SIZE, MAX_ASYNCBUFFER_SIZE);
    DebugPrint(L"    -f                    Use flow control\n");
    DebugPrint(L"    -m                    'Master' mode for loopback for stress tests\n");
    DebugPrint(L"    -v                    Verbose output (use multiple times to increase verbosity\n");
    DebugPrint(L"    -z                    Output messages to OutputDebugString (CE only)\n");
    DebugPrint(L"    -? usage\n");
    DebugPrint(L"Notes:\n");
    DebugPrint(L"  * During master / slave testing always start the slave first\n");
    DebugPrint(L"  * Options can be supplied in any order\n");
    DebugPrint(L"Examples:\n");
#ifdef UARTTEST_CE_VERSION
    DebugPrint(L"  uarttest -d COM2: -b 9600 -p EVEN -o l -l 45000 -t 10000 -v\n");
    DebugPrint(L"  uarttest -o w -d COM2: -b 115200 -l 45000 -t 10000 -v -v\n");
#else
    DebugPrint(L"  uarttest -d COM2 -b 9600 -p EVEN -o l -l 45000 -t 10000 -v\n");
    DebugPrint(L"  uarttest -o w -d COM2 -b 115200 -l 45000 -t 10000 -v -v\n");
#endif

}


//////////////////////////////////////////////////////////////////////////////////////
// 
//
void ShowTestConfiguration(TEST_SETUP &Setup)
{
    wchar_t *pTmp;
    
    DebugPrint(L"\n");
    DebugPrint(L"Test Strategy\n");
    switch(Setup.op)
    {
        case L'l':
            pTmp = L"Loopback";
            break;
        case L'r':
            pTmp = L"Read Data";
            break;
        case L'w':
            pTmp = L"Write Data";
            break;
        case L'a':
            pTmp = L"Asynchonous Read/Write";
            break;
        case L's':
            pTmp = L"Stress System";
            break;
        default:
            pTmp = L"Unknown";
            break;
    }
    DebugPrint(L"    Type ............ %s\n", pTmp);
	if ( (Setup.op == L'l') || (Setup.op == L's') )
    {
        DebugPrint(L"    Act As Master ... %s\n", (Setup.fMaster ? L"Yes" : L"No"));
    }
    if ( (Setup.op == L'a') || ((Setup.op != L's') && Setup.fMaster) )
    {
        DebugPrint(L"    Test Duration ... %u seconds\n", Setup.dwTestDuration);
    }

    DebugPrint(L"Test Parameters\n");
    DebugPrint(L"    Port .................... %s\n", Setup.port.szName);
    DebugPrint(L"    Baudrate ................ %u\n", Setup.port.dwBaudRate);
	DebugPrint(L"    Byte Size ............... %u bits\n", Setup.port.nByteSize);
    switch(Setup.port.nStopBits)
	{
		case ONESTOPBIT:
            pTmp = L"1";
            break;
		case ONE5STOPBITS:
            pTmp = L"1.5";
            break;
		case TWOSTOPBITS:
            pTmp = L"2";
            break;
		default:
            pTmp = L"Unknown";
            break;
	}
	DebugPrint(L"    Stop Bits ............... %s\n", pTmp);
    switch(Setup.port.nParity)
	{
		case NOPARITY:
            pTmp = L"None";
            break;
		case EVENPARITY:
            pTmp = L"Even";
            break;
		case ODDPARITY:
            pTmp = L"Odd";
            break;
		case MARKPARITY:
            pTmp = L"Mark";
            break;
		case SPACEPARITY:
            pTmp = L"Space";
            break;
		default:
            pTmp = L"Unknown";
            break;
	}
	DebugPrint(L"    Parity .................. %s\n", pTmp);
    DebugPrint(L"    Hardware Flow Control ... %s\n",
               (Setup.port.fFlowControlOn ? L"Yes" : L"No"));

    if ( ((Setup.op != L'l') && (Setup.op != L's')) || Setup.fMaster )
    {
        DebugPrint(L"    Datablock Size .......... %u bytes\n", Setup.dwDataBlockSize);
        DebugPrint(L"    Datablocks to Transfer .. %u\n", Setup.dwNumDataBlocks);
    }
    
    DebugPrint(L"    Data Transfer Timeout ... %u.%-1u seconds\n",
               (Setup.port.dwCommsTimeout / 1000),
               (Setup.port.dwCommsTimeout % 1000));
  
    DebugPrint(L"\n");

}


//////////////////////////////////////////////////////////////////////////////////////
// Show Transfer Status
//
void ShowTransferStatus(unsigned __int64 nTotal, unsigned __int64 nErrors,
                        DWORD dwTWrite, DWORD dwTRead)
{
    DebugPrint(L"  %I64u bytes transferred, %I64u errors", nTotal, nErrors);
    nTotal *= 1000;
    if (dwTWrite != 0)
    {
        DebugPrint(L", TX %u bytes/s",  (unsigned int)(nTotal / dwTWrite));
    }
    if (dwTRead != 0)
    {
        DebugPrint(L", RX %u bytes/s",  (unsigned int)(nTotal / dwTRead));
    }
    DebugPrint(L"\n");
}



//////////////////////////////////////////////////////////////////////////////////////
// Open and configure comm device
//
BOOL OpenDevice(TEST_PORT_SETUP &Port, BOOL bPurgeBuffers)
{
    DCB dcb;
	COMMTIMEOUTS ct;
    BOOL fSuccess = FALSE;


    // Make sure everything closed first
    CloseDevice(Port);

#ifdef UARTTEST_CE_VERSION

    Port.hDev = CreateFile(Port.szName, GENERIC_READ | GENERIC_WRITE,
	                       0, NULL, OPEN_EXISTING,
	                       FILE_ATTRIBUTE_NORMAL, NULL);

#else

    Port.hDev = CreateFile(Port.szName, GENERIC_READ | GENERIC_WRITE,
	                       0, NULL, OPEN_EXISTING,
	                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

#endif

    if ( Port.hDev == INVALID_HANDLE_VALUE )
    {
        DebugPrint(L"ERROR: Failed to open %s  (error %d)\n", 
                   Port.szName, GetLastError());
        
        Port.hDev = NULL;     
        goto _clean;
    }

    memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	if ( !GetCommState(Port.hDev, &dcb) )
	{
        DebugPrint(L"ERROR: Failed to retrieve comm state  (error %d)\n", 
            GetLastError());
		goto _clean;
	}

	dcb.BaudRate = Port.dwBaudRate;
	dcb.ByteSize = Port.nByteSize;
	dcb.StopBits = Port.nStopBits;
	if ( Port.nParity == NOPARITY )
	{
		dcb.fParity = 0;
		dcb.Parity = NOPARITY;
	}
	else
	{
		dcb.fParity = 1;
		dcb.Parity = Port.nParity;
	}
    if ( Port.fFlowControlOn )
    {
        dcb.fOutxCtsFlow = TRUE;
	    dcb.fOutxDsrFlow = FALSE;
	    dcb.fDtrControl = DTR_CONTROL_DISABLE;
	    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }
    else
    {
	    dcb.fOutxCtsFlow = FALSE;
	    dcb.fOutxDsrFlow = FALSE;
	    dcb.fDtrControl = DTR_CONTROL_DISABLE;
	    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    }
		
	if ( !SetCommState(Port.hDev, &dcb) )
	{
		DebugPrint(L"ERROR: Failed to update comm state  (error %d)\n", 
            GetLastError());
		goto _clean;
	}

    ct.WriteTotalTimeoutConstant = Port.dwCommsTimeout;
	ct.WriteTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = Port.dwCommsTimeout;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadIntervalTimeout = 0;

    if ( !SetCommTimeouts(Port.hDev, &ct) )
	{
        DebugPrint(L"ERROR: Failed to update comm timeouts  (error %d)\n", 
            GetLastError());
		goto _clean;
	}
    
	// Purge the comms buffers if this is the first time we are opening the port
	// This stops old data from previous test runs affecting this one
	if ( g_bPurgeCommsBuffers || bPurgeBuffers )
	{
		PurgeComm(Port.hDev, PURGE_TXCLEAR | PURGE_RXCLEAR);
		g_bPurgeCommsBuffers = FALSE;
	}

#ifndef UARTTEST_CE_VERSION

    Port.gOvpWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if ( Port.gOvpWrite.hEvent == NULL )
    {
        DebugPrint(L"ERROR: Failed to create write event  (error %d)\n", 
            GetLastError());
		Port.gOvpWrite.hEvent = NULL;
		goto _clean;
    }

    Port.gOvpRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);    
    if ( Port.gOvpRead.hEvent == NULL )
    {
        DebugPrint(L"ERROR: Failed to create read event  (error %d)\n", 
            GetLastError());
		Port.gOvpRead.hEvent = NULL;
        goto _clean;
    }

#endif

    fSuccess = TRUE;

_clean:

    if ( !fSuccess )
    {
        CloseDevice(Port);
    }    

	return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////
// Close open comm device
//
void CloseDevice(TEST_PORT_SETUP &Port)
{

#ifndef UARTTEST_CE_VERSION

    if ( Port.gOvpWrite.hEvent != NULL )
    {
        CloseHandle(Port.gOvpWrite.hEvent);
        Port.gOvpWrite.hEvent = NULL;
    }
    
    if ( Port.gOvpRead.hEvent != NULL )
    {
        CloseHandle(Port.gOvpRead.hEvent);
        Port.gOvpRead.hEvent = NULL;
    }

#endif

    if ( Port.hDev != NULL )
    {
        CloseHandle(Port.hDev);
        Port.hDev = NULL;
    }

}


//////////////////////////////////////////////////////////////////////////////////////
// Write specified amount of data from buffer to open serial device
//
static BOOL BufferWrite(TEST_PORT_SETUP &Port,
                        char *pData, DWORD dwDataLen,
                        DWORD &dwDataSent, DWORD *dwTimeTaken,
                        wchar_t *pszMsg)
{
    BOOL success = FALSE;
    DWORD dwTTime = 0;
    DWORD err;
    DWORD dwBytesDone;
	DWORD dwBytesNeeded;
	char *pDataStart;
	DWORD dwTmp;
	DWORD dwZeroCount;


    if ( pData == NULL )
    {
        DebugPrint(L"WRITE: Passed NULL pointer\r\n");
    }
    else
    {

		if ( g_dwVerboseOutput > 0  )
		{
            DebugPrint(L"WRITE: Send %u bytes\r\n", dwDataLen);
		}
		
    	dwDataSent = 0;
    	dwZeroCount = 0; 
    	do
    	{

    		pDataStart = &pData[dwDataSent];
    		dwBytesNeeded = dwDataLen - dwDataSent;
    		dwBytesDone = 0;

    	    if (dwTimeTaken != NULL)
    		{
    		    dwTTime = GetTickCount();
    		}

    #ifdef UARTTEST_CE_VERSION

    		success = ( WriteFile(Port.hDev,
    			                  pDataStart, dwBytesNeeded,
    							  &dwBytesDone, NULL) != 0 );

    #else

    		success = ( WriteFile(Port.hDev,
    			                  pDataStart, dwBytesNeeded,
    							  &dwBytesDone, &Port.gOvpWrite) != 0 );

    		if ( !success && (GetLastError() == ERROR_IO_PENDING) )
    		{
    		    success = GetOverlappedResult(Port.hDev,
    				                          &Port.gOvpWrite, &dwBytesDone, TRUE);
    	    }

    #endif
    	    if (dwTimeTaken != NULL)
    		{
    			dwTTime = (GetTickCount() - dwTTime);
    			*dwTimeTaken += dwTTime;    
    		}

    		if ( success )
    		{
    			if ( g_dwVerboseOutput > 0 )
    			{
    	            DebugPrint(L"WRITE: Sent %u bytes\r\n", dwBytesDone);
    			}
    			if ( dwBytesDone == 0 )
    			{
    				dwZeroCount++;
    				if ( dwZeroCount > 4 )
    				{
    		            DebugPrint(L"ERROR: Too many consecutive zero byte writes\r\n");
    					success = FALSE;
    				}
    			}
    			else
    			{
    				if ( g_dwVerboseOutput > 1  )
    				{
    					for (dwTmp = 0; dwTmp < dwBytesDone; dwTmp++)
    					{
    			            DebugPrint(L"%C", pDataStart[dwTmp]);
    					}
    					DebugPrint(L"\r\n");
    				}
    				dwZeroCount = 0;
    				dwDataSent += dwBytesDone;
    			}
    		}

    	} while ( success && (dwDataSent < dwDataLen) );

	
        if ( !success || (dwDataSent != dwDataLen) )
        {
            err = GetLastError();
            DebugPrint(L"ERROR: Failed to write %u byte %s  (written %u, error %d)\n",
                       dwDataLen, pszMsg, dwDataSent, err);
            success = FALSE;
        }

    	if ( g_dwVerboseOutput > 0  )
    	{
    		DebugPrint(L"WRITE: Completed\r\n");
    	}

    }
    
    return success;
}


//////////////////////////////////////////////////////////////////////////////////////
// Read specified amount of data into buffer from open serial device
//
static BOOL BufferRead(TEST_PORT_SETUP &Port,
                       char *pData, DWORD dwDataLen,
                       DWORD &dwDataRead, DWORD *dwTimeTaken,
                       wchar_t *pszMsg)
{
    BOOL success = FALSE;
    DWORD dwTTime = 0;
    DWORD err;
    DWORD dwBytesDone;
	DWORD dwBytesNeeded;
	char *pDataStart;
	DWORD dwTmp;
	DWORD dwZeroCount;

    if ( pData == NULL )
    {
        DebugPrint(L"READ: Passed NULL pointer\r\n");
    }
    else
    {
        // Clear the data buffer first
        memset(pData, 0, dwDataLen);

        dwDataRead = 0;
		dwZeroCount = 0;

		if ( g_dwVerboseOutput > 0  )
		{
            DebugPrint(L"READ: Get %u bytes\r\n", dwDataLen);
		}

		do
		{

			pDataStart = &pData[dwDataRead];
			dwBytesNeeded = dwDataLen - dwDataRead;
			dwBytesDone = 0;

			if (dwTimeTaken != NULL)
			{
				dwTTime = GetTickCount();
			}

#ifdef UARTTEST_CE_VERSION

	        success = ( ReadFile(Port.hDev, 
				                 pDataStart, dwBytesNeeded,
								 &dwBytesDone, NULL) != 0 );

#else

	        success = ( ReadFile(Port.hDev,
				                 pDataStart, dwBytesNeeded,
								 &dwBytesDone, &Port.gOvpRead) != 0 );
		    if ( !success && (GetLastError() == ERROR_IO_PENDING) )
			{
				success = GetOverlappedResult(Port.hDev, &Port.gOvpRead,
					                          &dwBytesDone, TRUE);
			}

#endif
    
		    if (dwTimeTaken != NULL)
			{
				dwTTime = (GetTickCount() - dwTTime);
				*dwTimeTaken += dwTTime;    
			}

			if ( success )
			{
				if ( g_dwVerboseOutput > 0  )
				{
		            DebugPrint(L"READ: Got %u bytes\r\n", dwBytesDone);
				}
				if ( dwBytesDone == 0 )
				{
					dwZeroCount++;
					if ( dwZeroCount > 4 )
					{
			            DebugPrint(L"ERROR: Too many consecutive zero byte reads\r\n");
						success = FALSE;
					}
				}
				else
				{
					if ( g_dwVerboseOutput > 1  )
					{
						for (dwTmp=0; dwTmp < dwBytesDone; dwTmp++)
						{
							DebugPrint(L"%C", pDataStart[dwTmp]);
						}
						DebugPrint(L"\r\n");
					}
					dwZeroCount = 0;
					dwDataRead += dwBytesDone;
				}
			}

		} while ( success && (dwDataRead < dwDataLen) );

        if ( !success || (dwDataRead != dwDataLen) )
        {
            err = GetLastError();
            DebugPrint(L"ERROR: Failed to receive %d byte %s  (read %d, error %d)\r\n",
                       dwDataLen, pszMsg, dwDataRead, err);
            success = FALSE;
        }

    	if ( g_dwVerboseOutput > 0  )
	    {
		    DebugPrint(L"READ: Completed\r\n");
    	}

    }

    return success;
}


//////////////////////////////////////////////////////////////////////////////////////
// Perform a Write test
//
BOOL WriteTest(TEST_SETUP &Setup)
{
    BOOL fSuccess;
    const char *pszPattern = "Write test. ";
    char *pszBuffer, *pszTemp;
    int nTemp, nLength; 
    DWORD dwTWrite = 0;
    unsigned __int64 nTotal = 0;
    unsigned __int64 nErrors = 0;
    DWORD dwTLoopStart;
    DWORD dwTemp;
    DWORD dwCount;

    DebugPrint(L"Starting Write Test\n");

    fSuccess = OpenDevice(Setup.port);
    if ( fSuccess )
    {
        pszBuffer = new char[ Setup.dwDataBlockSize + 8 ];
        if (!pszBuffer)
        {
            DebugPrint(L"ERROR: Failed to allocate %d byte buffer\n", Setup.dwDataBlockSize);
            nErrors++;
        }
        else
        {
            dwCount = Setup.dwNumDataBlocks;
            
            nTemp = Setup.dwDataBlockSize;
            nLength = (int)strlen(pszPattern);
            pszTemp = pszBuffer;

            while (nTemp)
            {
                int nCpy = min(nLength, nTemp);
                memcpy(pszTemp, pszPattern, nCpy);
                nTemp -= nCpy;
                pszTemp += nCpy;
            }

            dwTLoopStart = GetTickCount();

            while (dwCount --)
            {
                if ( !BufferWrite(Setup.port,
                                  pszBuffer, Setup.dwDataBlockSize,
                                  dwTemp, &dwTWrite, L"data block") )
                {
                    nErrors++;
                    break;
                }
                nTotal += dwTemp;

                // Show interim results every 5 seconds
                if (dwCount > 0)
                {
                    dwTemp = GetTickCount() - dwTLoopStart;
                    if ((dwTemp / 1000) > 5)
                    {
                        ShowTransferStatus(nTotal, nErrors, dwTWrite, 0);
                        dwTLoopStart = GetTickCount();
                    }
                }

            }   

            delete[] pszBuffer;
        }
    
        DebugPrint(L"Finished Write Test\n");
        ShowTransferStatus(nTotal, nErrors, dwTWrite, 0);
        

        CloseDevice(Setup.port);

    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////
// Read test
//
BOOL ReadTest(TEST_SETUP &Setup)
{
    BOOL fSuccess;
    char *pszBuffer;
    DWORD dwTemp;
    DWORD dwTRead = 0;
    unsigned __int64 nTotal = 0;
    DWORD dwTLoopStart;
    unsigned __int64 nErrors = 0;
    DWORD dwCount;
    
    DebugPrint(L"Starting Read Test\n");

    fSuccess = OpenDevice(Setup.port);
    if ( fSuccess )
    {

        pszBuffer = new char[ Setup.dwDataBlockSize + 8 ];
        if (!pszBuffer)
        {
            DebugPrint(L"ERROR: Failed to allocate %d byte buffer\n", Setup.dwDataBlockSize);
            nErrors++;
        }
        else
        {
            dwCount = Setup.dwNumDataBlocks;
            
            dwTLoopStart = GetTickCount();

            while (dwCount --)
            {

                if ( !BufferRead(Setup.port, 
                                 pszBuffer, Setup.dwDataBlockSize,
                                 dwTemp, &dwTRead, L"data block"))
                {
                    nErrors++;
                    break;
                }
                nTotal += dwTemp;

                // Show interim results every 5 seconds
                if (dwCount > 0)
                {
                    dwTemp = GetTickCount() - dwTLoopStart;
                    if ((dwTemp / 1000) > 5)
                    {
                        ShowTransferStatus(nTotal, nErrors, 0, dwTRead);
                        dwTLoopStart = GetTickCount();
                    }
                }

            }   

            delete[] pszBuffer;

        }

        DebugPrint(L"Finished Read Test\n");
        ShowTransferStatus(nTotal, nErrors, 0, dwTRead);

        CloseDevice(Setup.port);

    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////
// Loopback test
//
BOOL LoopbackTest(TEST_SETUP &Setup)
{
    BOOL fSuccess;
    char *pTxBuffer = NULL;
    char *pRxBuffer = NULL;
    char TmpBuffer[64];
    DWORD dwTemp, dwPos, dwSeed;
    DWORD dwTLoopStart, dwTWrite = 0, dwTRead = 0;
    unsigned __int64 nTotal = 0;
    unsigned __int64 nErrors = 0;
    char *pszTestStartMsg =  "<[==== TEST START ====]>";
    char *pszBlockStartMsg = "<[*** BLOCK START ***]>";
    wchar_t *pszMsgText1 = L"block-start message";
    wchar_t *pszMsgText2 = L"data block";
    wchar_t *pszMsgText3 = L"Test-start message";
    wchar_t *pszMsgText4 = L"NumBlocks message";
    wchar_t *pszMsgText5 = L"BlockSize message";
    DWORD dwDataBlockSize;
    DWORD dwNumDataBlocks;

    DebugPrint(L"Starting Loopback Test\n");

    fSuccess = OpenDevice(Setup.port);
    if ( fSuccess )
    {

		if ( Setup.fMaster )
        {
            // Act as a master sending data and verifying the slave returned it correctly

			// Seed the pseudo-random number genarator
            srand(GetTickCount() / 1000);

            dwNumDataBlocks = Setup.dwNumDataBlocks;
            dwDataBlockSize = Setup.dwDataBlockSize;

            // Send the test-start message (used to sync slave settings)
            dwPos = (DWORD)strlen(pszTestStartMsg);
            if ( !BufferWrite(Setup.port, pszTestStartMsg, dwPos, dwTemp, NULL, pszMsgText3) )
            {
                nErrors++;
                goto _done;
            }
            else if ( !BufferWrite(Setup.port, (char *)&dwNumDataBlocks, sizeof(DWORD), dwTemp, NULL, pszMsgText4) )
            {
                nErrors++;
                goto _done;
            }
            else if ( !BufferWrite(Setup.port, (char *)&dwDataBlockSize, sizeof(DWORD), dwTemp, NULL, pszMsgText5) )
            {
                nErrors++;
                goto _done;
            }


            pRxBuffer = new char[ dwDataBlockSize + 8 ];
            if ( pRxBuffer == NULL )
            {
                DebugPrint(L"ERROR: Failed to allocate %d byte RX buffer\n", dwDataBlockSize);
                nErrors++;
                goto _done;
            }
        
	        pTxBuffer = new char[ dwDataBlockSize + 8 ];
		    if ( pTxBuffer == NULL )
			{
				DebugPrint(L"ERROR: Failed to allocate %d byte TX buffer\n", dwDataBlockSize);
				nErrors++;
				goto _done;
			}

            
            dwTLoopStart = GetTickCount();

            while ( dwNumDataBlocks > 0 )
            {
                dwNumDataBlocks--;
                
                // Send the block-start message (used to sync slave receive timing)
                dwPos = (DWORD)strlen(pszBlockStartMsg);
                if ( !BufferWrite(Setup.port, pszBlockStartMsg, dwPos, dwTemp, NULL, pszMsgText1) )
                {
                    nErrors++;
                    break;
                }

                // Create a random set of ASCII data (will be different for each loop)
                dwSeed = (DWORD)rand();
                for (dwPos = 0; dwPos < dwDataBlockSize; dwPos ++)
                {
                    pTxBuffer[ dwPos ] = (char)((dwPos + dwSeed) % 0x100);
                }

                // Write the data buffer contents
                if ( !BufferWrite(Setup.port, pTxBuffer, dwDataBlockSize, dwTemp, &dwTWrite, pszMsgText2) )
                {
                    nErrors++;
                    break;
                }

                // Read the data back (written data echo'ed by slave)
                if ( !BufferRead(Setup.port, pRxBuffer, dwDataBlockSize, dwTemp, &dwTRead, pszMsgText2) )
                {
                    nErrors++;
                    break;
                }

                // Validate the data read back in
                for ( dwPos = 0; dwPos < dwDataBlockSize; dwPos++ )
                {
                    if ( pTxBuffer[dwPos] != pRxBuffer[dwPos] )
                    {
                        nErrors++;
                    }
                }

				nTotal += dwDataBlockSize;

                // Show interim results every 5 seconds
                if (dwNumDataBlocks > 0)
				{
                    dwTemp = GetTickCount() - dwTLoopStart;
                    if ((dwTemp / 1000) > 5)
                    {
                        ShowTransferStatus(nTotal, nErrors, dwTWrite, dwTRead);
                        dwTLoopStart = GetTickCount();
                    }
                }

            }

        }
        else
        {

            // Act as a slave echoing back all data blocks recieved.


            // Read & verify the block-start message
            dwPos = (DWORD)strlen(pszTestStartMsg);
            memset(TmpBuffer, 0, sizeof(TmpBuffer));
            if ( !BufferRead(Setup.port,
    			             TmpBuffer, dwPos,
							 dwTemp, NULL, pszMsgText3) )
            {
                nErrors ++;
                goto _done;
            }
            else if (memcmp(TmpBuffer, pszTestStartMsg, dwPos) != 0)
            {
                DebugPrint(L"ERROR: Malformed %d-byte %s received  (%S)\n",
                           dwTemp, pszMsgText3, TmpBuffer);
                nErrors ++;
                goto _done;
            }
            else if ( !BufferRead(Setup.port,
    			                  (char *)&dwNumDataBlocks, sizeof(DWORD),
							      dwTemp, NULL, pszMsgText4) )
            {
                nErrors ++;
                goto _done;
            }
            else if ( !BufferRead(Setup.port,
    			                  (char *)&dwDataBlockSize, sizeof(DWORD),
							      dwTemp, NULL, pszMsgText5) )
            {
                nErrors ++;
                goto _done;
            }


            DebugPrint(L"Performing loopback for %u data blocks of %u bytes each\r\n",
		               dwNumDataBlocks, dwDataBlockSize);

            pRxBuffer = new char[ dwDataBlockSize + 8 ];
            if ( pRxBuffer == NULL )
            {
                DebugPrint(L"ERROR: Failed to allocate %d byte RX buffer\n", dwDataBlockSize);
                nErrors++;
                goto _done;
            }

            dwTLoopStart = GetTickCount();

            while ( dwNumDataBlocks > 0 )
            {
                dwNumDataBlocks--;

                // Read & verify the block-start message
                dwPos = (DWORD)strlen(pszBlockStartMsg);
                if ( !BufferRead(Setup.port,
					             pRxBuffer, dwPos,
								 dwTemp, NULL, pszMsgText1))
                {
                    nErrors ++;
                    break;
                }
                pRxBuffer[dwTemp] = '\0';
                if (memcmp(pRxBuffer, pszBlockStartMsg, dwPos) != 0)
                {
                    DebugPrint(L"ERROR: Malformed %d-byte %s received  (%S)\n",
                               dwTemp, pszMsgText1, pRxBuffer);
                    nErrors ++;
                    break;
                }

                // Read the block of data sent by the master and echo it back
                memset(pRxBuffer, 0, dwDataBlockSize);
                if ( !BufferRead(Setup.port,
					             pRxBuffer, dwDataBlockSize,
								 dwTemp, &dwTRead, pszMsgText2) )
                { 
                    nErrors ++;
                    break;
                }
                if ( !BufferWrite(Setup.port,
					              pRxBuffer, dwDataBlockSize,
								  dwTemp, &dwTWrite, pszMsgText2) )
                {
                    nErrors ++;
                    break;
                }

                nTotal += dwDataBlockSize;

                // Show interim results every 5 seconds
                if (dwNumDataBlocks > 0)
                {
                    dwTemp = GetTickCount() - dwTLoopStart;
                    if ((dwTemp / 1000) > 5)
                    {
                        ShowTransferStatus(nTotal, nErrors, dwTWrite, dwTRead);
                        dwTLoopStart = GetTickCount();
                    }
                }

            }
        }

_done:

        DebugPrint(L"Finished Loopback Test\n");
        ShowTransferStatus(nTotal, nErrors, dwTWrite, dwTRead);

        CloseDevice(Setup.port);

    }


    if ( pRxBuffer != NULL )
    {
        delete[] pRxBuffer;
        pRxBuffer = NULL;
    }
    if ( pTxBuffer != NULL )
    {
        delete[] pTxBuffer;
        pTxBuffer = NULL;
    }

	if ( fSuccess )
	{
		if ( nErrors > 0 )
		{
			fSuccess = FALSE;
		}
	}

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////
// Async test
//


DWORD WriterThread(TEST_ASYNC_THREAD *pThread)
{
    char buffer[ MAX_ASYNCBUFFER_SIZE + 8 ];
    DWORD i, dwDataLen, dwDataSent, dwTimeTaken, dwErrors;
    char seed;

    // Give the sender on the other end enough time to 
    // open and configure the port
    Sleep(3000);
    
    srand(GetTickCount());
    seed = (char)(rand() % 0x100);

    while (WaitForSingleObject(pThread->hStop, 0) != WAIT_OBJECT_0)
    {
        dwDataLen = (rand() % pThread->nMaxfer) + 1;
        for (i = 0; i < dwDataLen; i ++)
        {
            buffer[ i ] = seed ++;
        }

        dwTimeTaken = 0;
        if (BufferWrite(pThread->port,
			            buffer, dwDataLen,
						dwDataSent, &dwTimeTaken, 
                        L"(writer thread)") && dwDataSent)
        {
            dwErrors = 0;
        }
        else
        {
            dwErrors = 1;
            dwDataSent = 0;
        }
        
        EnterCriticalSection(&pThread->csLock);
        pThread->nTransferred += dwDataSent;
        pThread->dwTime += dwTimeTaken;
        pThread->nErrors += dwErrors;
        LeaveCriticalSection(&pThread->csLock);
    }

    return 0;
}

DWORD ReaderThread(TEST_ASYNC_THREAD *pThread)
{
    char buffer[ MAX_ASYNCBUFFER_SIZE + 8 ];
    DWORD i, dwDataLen, dwDataRead, dwTimeTaken, dwErrors;
    char seed = 0, seedok = 0;
    unsigned __int64 nTransferred = 0;
        
    srand(GetTickCount());
    
    while (WaitForSingleObject(pThread->hStop, 0) != WAIT_OBJECT_0)
    {
        dwDataLen = (rand() % pThread->nMaxfer) + 1;
        dwTimeTaken = 0;

        if ( BufferRead(pThread->port,
			            buffer, dwDataLen,
			 		    dwDataRead, &dwTimeTaken,
                        L"(reader thread)") && dwDataRead )
        {
            dwErrors = 0;

            for (i = 0; i < dwDataRead; i ++)
            {
                if (!seedok)
                {
                    seed = buffer[ i ];
                    seedok = 1;                    
                }
                
                if (buffer[ i ] != seed)
                {
                    DebugPrint(L"reader:  mismatch, pos %d/%d/%I64u, exp %02X, act %02X\n",
                        i, dwDataRead, nTransferred + i, (UINT8)seed, (UINT8)buffer[ i ]);
                    
                    dwErrors ++;
                    seedok = 0;                    
                }
                else
                {
                    seed ++;
                }
            }
        }
        else
        {
            dwErrors = 1;
            dwDataRead = 0;
        }

        nTransferred += dwDataRead;
        
        EnterCriticalSection(&pThread->csLock);
        pThread->nTransferred = nTransferred;
        pThread->dwTime += dwTimeTaken;
        pThread->nErrors += dwErrors;
        LeaveCriticalSection(&pThread->csLock);
    }

    return 0;
}


BOOL AsyncTest(TEST_SETUP &Setup)
{
    BOOL fSuccess;
    int i, interval;
    unsigned __int64 nTransferred;
    unsigned __int64 nErrors; 
    DWORD dwTime;
    TEST_ASYNC_THREAD threads[ 3 ];
    DWORD dwDuration;

    dwDuration = Setup.dwTestDuration;

	DebugPrint(L"Starting Async Test: duration %d sec, maxfer [1..%d] bytes\n",
               dwDuration, Setup.dwMaxfer);

    fSuccess = OpenDevice(Setup.port);
    if ( fSuccess )
    {
        memset(threads, 0, sizeof(threads));
    
        for (i = 0; i < 2; i ++)
        {
            DebugPrint(L"Starting %s thread\n", i ? L"writer" : L"reader");

            threads[ i ].port = Setup.port;
            threads[ i ].nMaxfer = Setup.dwMaxfer;
            threads[ i ].hStop = CreateEvent(NULL, TRUE, FALSE, NULL);

            if (!threads[ i ].hStop)
            {
                DebugPrint(L"Failed to create event! Error %08X\n", GetLastError());
                goto _clean;
            }
    
            InitializeCriticalSection(&threads[ i ].csLock);

            threads[ i ].hThread = CreateThread(
                NULL, 
                0, 
                i ? (LPTHREAD_START_ROUTINE)WriterThread : (LPTHREAD_START_ROUTINE)ReaderThread,
                &threads[ i ],
                0,
                NULL);

            if (!threads[ i ].hThread)
            {
                DebugPrint(L"Failed to create thread! Error %08X\n", GetLastError());
                goto _clean;
            }
        }

        while (dwDuration)
        {
            // Use 3 second poll interval
            interval = min(dwDuration, 3);
            Sleep((DWORD)(interval * 1000));

            for (i = 0; i < 2; i ++)
            {
                EnterCriticalSection(&threads[ i ].csLock);
                nTransferred = threads[ i ].nTransferred;
                nErrors = threads[ i ].nErrors;
                dwTime = threads[ i ].dwTime;
                LeaveCriticalSection(&threads[ i ].csLock);

                DebugPrint(i ? L"writer:" : L"reader:");

                ShowTransferStatus(
                    nTransferred, 
                    nErrors, 
                    (i != 0) ? dwTime : 0, 
                    (i == 0) ? dwTime : 0);
            }        
        
            dwDuration -= interval;
        }

    _clean:

        for (i = 1; i >= 0; i --)
        {
            if (threads[ i ].hStop)
            {
                SetEvent(threads[ i ].hStop);
            
                if (threads[ i ].hThread)
                {
                    do 
                    {
                        DebugPrint(L"Waiting for %s thread termination\n",
                            i ? L"writer" : L"reader");
                    }
                    while (WaitForSingleObject(threads[ i ].hThread, 3000) != WAIT_OBJECT_0);

                    CloseHandle(threads[ i ].hThread);
                    DeleteCriticalSection(&threads[ i ].csLock);

                    DebugPrint(L"%s: %I64u bytes, %I64u errors\n",
                            i ? L"Writer" : L"Reader", 
                            threads[ i ].nTransferred, threads[ i ].nErrors);

                    if (i)
                    {
                        // Give some delay after stopping writer thread to 
                        // clean up correctly on the other end
                        Sleep(3000);
                    }
                }

                CloseHandle(threads[ i ].hStop);
            }
        }

    
        DebugPrint(L"Finished Async Test\n");

        CloseDevice(Setup.port);

    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////
// Perform a Stress test
//
BOOL StressTest(TEST_SETUP &Setup)
{
    BOOL fSuccess = FALSE;
    BOOL fFinished = FALSE;
	TEST_STRESS_ACTION nextAction;
	DWORD dwDataTransferred;
	char *pNextID1 = "==========";
	char *pNextID2 = "----------";
	char *pEndID1 =  "**********";
	char *pEndID2 =  "##########";

	memset(&nextAction, 0, sizeof(TEST_STRESS_ACTION));

	if ( Setup.fMaster )
    {
		// We are working as the master end
		DWORD dwStartTick;
		DWORD dwActionNum;
		DWORD dwTestDuration;

		// Store the user defined setup at end of the action list
		//  so it is included in the tests that will be done.
		dwActionNum = NUM_STRESS_ACTIONS - 1;
		StressAction[dwActionNum].dwDataBlockSize = Setup.dwDataBlockSize;
		StressAction[dwActionNum].dwNumDataBlocks = Setup.dwNumDataBlocks;
		StressAction[dwActionNum].dwBaudRate = Setup.port.dwBaudRate;
		StressAction[dwActionNum].nParity = Setup.port.nParity;

		// Initialise
		dwActionNum = 0;
		dwStartTick = GetTickCount();
		dwTestDuration = Setup.dwTestDuration * 1000;

		do
		{
			fSuccess = LoopbackTest(Setup);
			if ( fSuccess )
			{
			    // Add delay for port to sort itself out
				// We must open slave BEFORE the master or errors occur.
				Sleep(1500);

				// Send instruction about what to do next
				fSuccess = OpenDevice(Setup.port);
				if ( fSuccess )
				{
					memset(&nextAction, 0, sizeof(nextAction));
					if ( (GetTickCount() - dwStartTick) > dwTestDuration )
					{
						// setup block for finishing code
						memcpy(nextAction.cID1, pEndID1, sizeof(nextAction.cID1));
						memcpy(nextAction.cID2, pEndID2, sizeof(nextAction.cID2));
						fFinished = TRUE;
					}
					else
					{
						// Setup block for next test
						memcpy(nextAction.cID1, pNextID1, sizeof(nextAction.cID1));
						memcpy(nextAction.cID2, pNextID2, sizeof(nextAction.cID2));

						nextAction.dwDataBlockSize = Setup.dwDataBlockSize;
						nextAction.dwNumDataBlocks = Setup.dwNumDataBlocks;
						nextAction.dwBaudRate = Setup.port.dwBaudRate;
						nextAction.nParity = Setup.port.nParity;

						if ( dwActionNum >= NUM_STRESS_ACTIONS )
						{
							dwActionNum = 0;
						}
						nextAction.dwDataBlockSize = StressAction[dwActionNum].dwDataBlockSize;
						nextAction.dwNumDataBlocks = StressAction[dwActionNum].dwNumDataBlocks;
						nextAction.dwBaudRate = StressAction[dwActionNum].dwBaudRate;
						nextAction.nParity = StressAction[dwActionNum].nParity;
						dwActionNum++;
					}

                    // Send the ID block
					fSuccess = BufferWrite(Setup.port,
					                       (char *)&nextAction, sizeof(nextAction),
								           dwDataTransferred, NULL, 
						                   L"(block2)");
					
                    // Close the port
                    CloseDevice(Setup.port);

		            // Add time to write out the data
				    Sleep(1000);

				}
			}

			if ( !fSuccess )
			{
				fFinished = TRUE;
			}
			else if ( !fFinished )
			{
				// Setup for next test
				Setup.dwDataBlockSize = nextAction.dwDataBlockSize;
				Setup.dwNumDataBlocks = nextAction.dwNumDataBlocks;
				Setup.port.dwBaudRate = nextAction.dwBaudRate;
				Setup.port.nParity = nextAction.nParity;

			    DebugPrint(L"\n-------------------------------------------------------------------------------\n");
				ShowTestConfiguration(Setup);
			}

		} while ( !fFinished );

	}
	else
	{
		// We are working as the slave end responding to master actions

		do
		{
			fSuccess = LoopbackTest(Setup);
			if ( fSuccess )
			{
			    // Add delay for port to sort itself out
				// We must open slave BEFORE the master or errors occur.
				Sleep(500);
				
				fSuccess = OpenDevice(Setup.port);
				if ( fSuccess )
				{
					memset(&nextAction, 0, sizeof(nextAction));
					fSuccess = BufferRead(Setup.port,
  				                          (char *)&nextAction, sizeof(nextAction),
						                  dwDataTransferred, NULL, 
						                  L"(block2)");
					if ( fSuccess )
					{
						if ( memcmp(nextAction.cID1, pNextID1, sizeof(nextAction.cID1)) == 0 )
						{
							if ( memcmp(nextAction.cID2, pNextID2, sizeof(nextAction.cID2)) != 0 )
							{
						        DebugPrint(L"ERROR: Unknown ID2 for 'next' block - %C%C\n",
								           nextAction.cID2[0], nextAction.cID2[0]);
								fSuccess = FALSE;
							}
						}
						else if ( memcmp(nextAction.cID1, pEndID1, sizeof(nextAction.cID1)) == 0 )
						{
							if ( memcmp(nextAction.cID2, pEndID2, sizeof(nextAction.cID2)) != 0 )
							{
						        DebugPrint(L"ERROR: Unknown ID2 for 'end' block - %C%C\n",
								           nextAction.cID2[0], nextAction.cID2[0]);
								fSuccess = FALSE;
							}
							else
							{
								fFinished = TRUE;
							}
						}
						else
						{
					        DebugPrint(L"ERROR: Unknown ID1 - %C%C\n",
								       nextAction.cID1[0], nextAction.cID1[0]);
							fSuccess = FALSE;
						}
					}
					CloseDevice(Setup.port);
				}
			}

			if ( !fSuccess )
			{
				fFinished = TRUE;
			}
			else if ( !fFinished )
			{
			    // Setup for next test
				Setup.dwDataBlockSize = nextAction.dwDataBlockSize;
				Setup.dwNumDataBlocks = nextAction.dwNumDataBlocks;
				Setup.port.dwBaudRate = nextAction.dwBaudRate;
				Setup.port.nParity = nextAction.nParity;

			    DebugPrint(L"\n-------------------------------------------------------------------------------\n");
				ShowTestConfiguration(Setup);
			}

		} while ( !fFinished );

	}

	return fSuccess;
}

