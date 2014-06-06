// All rights reserved ADENEO EMBEDDED 2010
// testSerial.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{    
    HANDLE h;
    DCB dcb;
    COMMTIMEOUTS		timeouts;
    _tprintf(_T("Simple Echo application\n"));
    if (argc != 2)
    {
        _tprintf(_T("Invalid number of parameters\nusage:%s port_name (ex:%s COM1:)\n"),argv[0],argv[0]);
        return -1;
    }
    _tprintf(_T("Opening %s\n"),argv[1]);
    h = CreateFile(argv[1],GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    _tprintf(_T("File handle %08X; error %d\n"),h, GetLastError());
    GetCommState(h,&dcb);
    dcb.BaudRate = 115200;   
    dcb.fBinary          = TRUE;
    dcb.fParity          = FALSE;
    dcb.fOutxCtsFlow     = FALSE;
    dcb.fOutxDsrFlow     = FALSE;
    dcb.fDtrControl      = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity  = FALSE;
    dcb.fOutX            = FALSE;
    dcb.fInX             = FALSE;
    dcb.fErrorChar       = 0;    
    dcb.fNull            = 0;    
    dcb.fRtsControl      = RTS_CONTROL_DISABLE; //RTS_CONTROL_ENABLE;
    dcb.fAbortOnError    = FALSE;
    dcb.ByteSize         = 8;    
    dcb.Parity           = NOPARITY;
    dcb.StopBits         = ONESTOPBIT;
    SetCommState(h,&dcb);

    GetCommTimeouts(h,&timeouts);
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(h,&timeouts);

	DWORD nbW;

    WriteFile(h,"\r\nHEY !!!\r\n",11,&nbW,NULL);
    RETAILMSG(1,(TEXT("nbWritten (%d)\r\n"),nbW));
	
	for(;;)
    {
        UCHAR buffer[512];
        DWORD nbRead,nbWritten;
        if (ReadFile(h,buffer,sizeof(buffer),&nbRead,NULL))
        {
            WriteFile(h,buffer,nbRead,&nbWritten,NULL);
            if (nbWritten != nbRead)
            {
                RETAILMSG(1,(TEXT("(nbWritten (%d) != nbRead (%d))\r\n"),nbWritten,nbRead));
            }
        }
    }
    CloseHandle(h);
    return 0;
}

