// VSDebugger.cpp : Defines the entry point for the application.
//

#include "windows.h"

#define STARTUP_TIMEOUT		30000
#define CMACCEPT_TIMEOUT	10000

// This functions executes an application and returns an handle to the process.
// It will close the main thread handle.
HANDLE MyCreateProcess(TCHAR* exepath)
{
	PROCESS_INFORMATION	pi;

	memset(&pi,0,sizeof(PROCESS_INFORMATION));

	// create the process
	if (!CreateProcess(exepath,NULL,NULL,NULL,FALSE,0,NULL,NULL,NULL,&pi))
	{
		return NULL;
	}

	// release main thread handle
	CloseHandle(pi.hThread);
	return pi.hProcess;
}

int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{	
	// signal startup completition
	SignalStarted(_ttoi(lpCmdLine));

	// wait to allow some time for the network connection initialization
	Sleep(STARTUP_TIMEOUT);

	HANDLE handles[2];

	PROCESS_INFORMATION	pi;

	memset(&pi,0,sizeof(PROCESS_INFORMATION));

	// executes the two applications needed to support debug connection
	handles[0]=MyCreateProcess(TEXT("ConManClient2.exe"));

	if (handles[0]==NULL)
		return -1;

	// wait to allow ConManClient2 initialization completition
	Sleep(CMACCEPT_TIMEOUT);

	handles[1]=MyCreateProcess(TEXT("CMAccept.exe"));
	
	if (handles[1]==NULL)
	{
		CloseHandle(handles[0]);
		return -1;
	}

	RETAILMSG(1, (TEXT("\r\nVisual Studio 2008 debug support enabled!\r\n")));

	// monitor the two processes and restart them if they are closed
	for (;;)
	{
		switch (WaitForMultipleObjects(2,handles,FALSE,INFINITE))
		{
		case WAIT_OBJECT_0:
			handles[0]=MyCreateProcess(TEXT("ConManClient2.exe"));
			continue;
		case WAIT_OBJECT_0+1:
			handles[1]=MyCreateProcess(TEXT("CMAccept.exe"));
			continue;
		}
		break;
	}

	// release process handles
	CloseHandle(handles[0]);
	CloseHandle(handles[1]);
    return 0;
}



