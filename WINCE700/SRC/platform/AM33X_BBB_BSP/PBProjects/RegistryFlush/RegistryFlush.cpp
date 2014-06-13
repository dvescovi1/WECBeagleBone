// RegistryFlush.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    RegFlushKey(HKEY_LOCAL_MACHINE);
	RegFlushKey(HKEY_CURRENT_USER);
	RETAILMSG(1, (TEXT("Registry flush!\r\n")));
    return 0;
}
