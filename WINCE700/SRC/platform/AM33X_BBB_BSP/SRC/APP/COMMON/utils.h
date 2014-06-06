// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#ifndef __UTILS_H
#define __UTILS_H

//------------------------------------------------------------------------------

#define COUNT(x) (sizeof(x)/sizeof(x[0]))

BOOL GetOptionFlag(int *pargc, LPTSTR argv[], LPCTSTR szOption);
LPTSTR GetOptionString(int *pargc, LPTSTR argv[], LPCTSTR szOption);
LONG GetOptionLong(int *pargc, LPTSTR argv[], LONG lDefault, LPCTSTR szOption);
void CommandLineToArgs(TCHAR szCommandLine[], int *argc, LPTSTR argv[]);
LPTSTR GetErrorMessage(DWORD dwErrorCode) ;

DWORD RegisterDriver(WCHAR *szDriver);

//------------------------------------------------------------------------------

#endif
