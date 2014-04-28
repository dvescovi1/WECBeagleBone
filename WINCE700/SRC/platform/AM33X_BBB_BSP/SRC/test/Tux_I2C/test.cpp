//
//  All rights reserved ADENEO EMBEDDED 2010
//  All rights reserved Texas Instruments, Inc. 2011
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_I2C TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include "platform_AM33X.h"

DWORD WriteThread(LPVOID pvarg);

BOOL g_Fail = FALSE;

////////////////////////////////////////////////////////////////////////////////
// TestProc
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
//  Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI TestI2CMultipleInstances(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE *ThreadHandles = NULL;
    DWORD *ThreadId = NULL;
    g_pKato->Log(1, TEXT("Starting I2CTest \r\n"));

    g_pKato->Log(1, TEXT("Using I2C port %d\r\n"), g_Port);
    g_pKato->Log(1, TEXT("Creating %d threads that will write %d bytes %d times\r\n"), g_NbThreads, g_BufferSize, g_Writes);

    ThreadHandles = (HANDLE*)LocalAlloc(LPTR, sizeof(HANDLE)*g_NbThreads);
    ThreadId = (DWORD*)LocalAlloc(LPTR, sizeof(DWORD)*g_NbThreads);

    for(DWORD i = 0; i < g_NbThreads; i++)
    {
        ThreadId[i] = i;
        //g_pKato->Log(1, TEXT("Starting thread %d \r\n"), i);
        ThreadHandles[i] = CreateThread(NULL, FALSE, (LPTHREAD_START_ROUTINE) WriteThread, &ThreadId[i], 0, NULL);
    }

    for(DWORD i = 0; i < g_NbThreads; i++)
    {
        DWORD dwRet = 0;
        dwRet = WaitForSingleObject(ThreadHandles[i], INFINITE);
        if(dwRet == WAIT_FAILED)
        {
            g_pKato->Log(1, TEXT("Failure in waiting for threads \r\n"), dwRet-WAIT_OBJECT_0);
            goto CLOSE;
        }
    }

CLOSE:
    g_pKato->Log(1, TEXT("Ending I2CTest \r\n"));

	return (g_Fail == TRUE)? TPR_FAIL : TPR_PASS;
}



static DWORD WriteThread(LPVOID pvarg)
{
    HANDLE hI2c = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwBytesReturned = 0;
    DWORD bThreadNumber = *(PDWORD)pvarg;
    BOOL bRet = TRUE;

    g_pKato->Log(1, TEXT("Starting I2c Thread %d\r\n"), bThreadNumber);

    pOutBuffer = (PBYTE)LocalAlloc(LPTR, sizeof(BYTE)*g_BufferSize);

    if(pOutBuffer == NULL)
    {
        g_pKato->Log(1, TEXT("Failure in Thread %d. Out of memory\r\n"), bThreadNumber);
        bRet = FALSE;
        goto CLOSE;
    }

    for(DWORD i = 0; i < g_BufferSize; i++)
    {
        pOutBuffer[i] = (BYTE)I2C_DUMMY_FILL;
    }

    //Calling platform specific Open I2C function
    hI2c = Platform_Open();
    if(hI2c == 0 || hI2c == INVALID_HANDLE_VALUE)
    {
        g_pKato->Log(1, TEXT("Could not open I2C port. Error = %d\r\n"), GetLastError());
        bRet = FALSE;
        goto CLOSE;
    }

	//Calling platform specific setSlaveAddress function
	if(g_Address != I2C_NOTSET_VARIABLE)
	{
		if( !Platform_setSlaveAddress(hI2c))
		{
			g_pKato->Log(1, TEXT("Could not set slave address. Error = %d\r\n"), GetLastError());
			bRet = FALSE;
			goto CLOSE;
		}
	}

	//Calling platform specific setBaudRateIndex function
	if(g_BaudRateIndex != I2C_NOTSET_VARIABLE)
	{
		if( !Platform_setBaudRateIndex(hI2c))
		{
			g_pKato->Log(1, TEXT("Could not set baud rate speed. Error = %d\r\n"), GetLastError());
			bRet = FALSE;
			goto CLOSE;
		}
	}

	//Calling platform specific setSubAddr function
	if(g_SubAddressMode!= I2C_NOTSET_VARIABLE)
	{
		if( !Platform_setSubAddrMode(hI2c))
		{
			g_pKato->Log(1, TEXT("Could not set sub Addr mode. Error = %d\r\n"), GetLastError());
			bRet = FALSE;
			goto CLOSE;
		}
	}

    for(DWORD i = 0; i < g_Writes; i++)
    {
        if(!Platform_Write(hI2c, I2C_SUBADDRESS, pOutBuffer, g_BufferSize))
        {
			if(g_Address != I2C_NOTSET_VARIABLE)
			{
				g_pKato->Log(1, TEXT("Could not write I2C port.\r\n"));
				bRet = FALSE;
				goto CLOSE;
			}
			else
			{
				g_pKato->Log(1, TEXT("Could not write I2C port as expected\
. No device address was specified in the command line.\r\n"));
				bRet = TRUE;
				goto CLOSE;
			}
            

        }
        Sleep(10);
    }
    
CLOSE:
    if(hI2c != 0 && hI2c != INVALID_HANDLE_VALUE)
    {
		Platform_Close(hI2c);
    }
    if(bRet == FALSE)
    {
        g_pKato->Log(1, TEXT("Thread %d failed\r\n"), bThreadNumber);
        g_Fail = TRUE;
    }

	g_pKato->Log(1, TEXT("Closing Thread %d \r\n"), bThreadNumber);
    return 0;
}
