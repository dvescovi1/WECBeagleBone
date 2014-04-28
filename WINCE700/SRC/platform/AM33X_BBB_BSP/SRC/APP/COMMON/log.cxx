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
#include <windows.h>
#include <tchar.h>
#include <kato.h>
#include "Log.h"

//------------------------------------------------------------------------------

#define BUFFER_SIZE           2048
#define MILLISEC_PER_HOUR     (1000*60*60)
#define MILLISEC_PER_MINUTE   (1000*60)

//------------------------------------------------------------------------------

typedef struct {
   BOOL   m_bStartup;
   BOOL   m_bWATTOutput;
   DWORD  m_dwLogLevel;
   DWORD  m_dwStartTick;
   TCHAR  m_szBuffer[BUFFER_SIZE];
   LPTSTR m_szBufferTail;
   DWORD  m_cbBufferTail;
   LONG   m_nErrorCount;
   LONG   m_nWarningCount;
   PLOG_FUNCTION m_pLogFce;
   CRITICAL_SECTION m_cs;
} LOG_INFO;

//------------------------------------------------------------------------------

static LOG_INFO g_LogInfo = {
   FALSE, FALSE, LOG_PASS, 0, _T(""), NULL, 0, 0, 0, NULL
};

//------------------------------------------------------------------------------

static LPCTSTR g_LogPrefix[] = {
   _T("ERROR: "), _T("ERROR: "), _T("ERROR: "), _T("Wrn: "), 
   _T("Wrn: "), _T("Wrn: "), _T("Wrn: "), _T("Wrn: "), 
   _T("Wrn: "), _T("Wrn: "), _T("Msg: "), _T("Msg: "), 
   _T("Dbg: "), _T("Dbg: "), _T("Vbs: "), _T("Vbs: ")
};

//------------------------------------------------------------------------------

static LPCTSTR g_CSSDTestResults[] = {
   _T("P"), _T("F"), _T("S"), _T("B"), _T("U")
};

//------------------------------------------------------------------------------

void LogStartup(LPCTSTR szTestName, DWORD dwLogLevel, PLOG_FUNCTION pLogFce)
{
   DWORD nLength = 0;

   ASSERT(!g_LogInfo.m_bStartup);

   InitializeCriticalSection(&g_LogInfo.m_cs);
   EnterCriticalSection(&g_LogInfo.m_cs);
   
   g_LogInfo.m_bStartup = TRUE;
   g_LogInfo.m_bWATTOutput = FALSE;
   g_LogInfo.m_dwLogLevel = dwLogLevel;
   g_LogInfo.m_dwStartTick = GetTickCount();
   g_LogInfo.m_pLogFce = pLogFce;
   
   lstrcpy(g_LogInfo.m_szBuffer, _T("00000000: "));
   lstrcpy(g_LogInfo.m_szBuffer + 10, szTestName);
   lstrcat(g_LogInfo.m_szBuffer, _T(" "));

   nLength = lstrlen(g_LogInfo.m_szBuffer);
   g_LogInfo.m_szBufferTail = g_LogInfo.m_szBuffer + nLength;
   g_LogInfo.m_cbBufferTail = BUFFER_SIZE - nLength - 3;

   LeaveCriticalSection(&g_LogInfo.m_cs);
}

//------------------------------------------------------------------------------

void LogCleanup()
{
   DWORD dwTime, dw;
   UINT uiHours, uiMinutes, uiSeconds;
   TCHAR szBuffer[41] = _T("");

   ASSERT(g_LogInfo.m_bStartup);
   
   EnterCriticalSection(&g_LogInfo.m_cs);

   if (g_LogInfo.m_bWATTOutput) {
      dw = dwTime = GetTickCount() - g_LogInfo.m_dwStartTick;
      uiHours = dw / MILLISEC_PER_HOUR;
      dw -= uiHours * MILLISEC_PER_HOUR;
      uiMinutes = dw / MILLISEC_PER_MINUTE;
      dw -= uiMinutes * MILLISEC_PER_MINUTE;
      uiSeconds = dw / 1000;

      Log(LOG_PASS, _T(""), uiHours, uiMinutes, uiSeconds, dwTime);
      Log(
         LOG_PASS, _T(""), 
         LogGetErrorCount() ? _T("FAILED") : _T("PASSED"), 
         LogGetErrorCount(), LogGetWarningCount()
      );

      wsprintf(szBuffer, _T("@@@@@@%d\r\n"), LogGetErrorCount());
      OutputDebugString(szBuffer);
   }

   g_LogInfo.m_bStartup = FALSE;
   LeaveCriticalSection(&g_LogInfo.m_cs);        
   DeleteCriticalSection(&g_LogInfo.m_cs);
}

//------------------------------------------------------------------------------

void LogV(DWORD dwLevel, LPCTSTR szFormat, va_list pArgs)
{
	#pragma warning(push)
	#pragma warning(disable:6385)
   LONG nCount = 0;

   ASSERT(g_LogInfo.m_bStartup);

   EnterCriticalSection(&g_LogInfo.m_cs);

   if (dwLevel > g_LogInfo.m_dwLogLevel) goto cleanUp;

   if (dwLevel <= LOG_FAIL) g_LogInfo.m_nErrorCount++;
   else if (dwLevel <= LOG_WARNING) g_LogInfo.m_nWarningCount++;

   wsprintf(g_LogInfo.m_szBuffer, _T("%08x"), GetCurrentThreadId());
   g_LogInfo.m_szBuffer[8] = _T(':');

   nCount = lstrlen(g_LogPrefix[dwLevel]);
   lstrcpy(g_LogInfo.m_szBufferTail, g_LogPrefix[dwLevel]);

   wvsprintf(g_LogInfo.m_szBufferTail + nCount, szFormat, pArgs);
   if (g_LogInfo.m_pLogFce == NULL) {
      OutputDebugString(g_LogInfo.m_szBuffer);
   } else {
      g_LogInfo.m_pLogFce(dwLevel, g_LogInfo.m_szBuffer);
   }

cleanUp:
   LeaveCriticalSection(&g_LogInfo.m_cs);
   #pragma warning(pop)
}

//------------------------------------------------------------------------------

void Log(DWORD dwLevel, LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(dwLevel, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

void LogErr(LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(LOG_FAIL, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

void LogWrn(LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(LOG_WARNING, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

void LogMsg(LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(LOG_PASS, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

void LogDbg(LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(LOG_DETAIL, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

void LogVbs(LPCTSTR szFormat, ...)
{
   va_list pArgs;
   
   va_start(pArgs, szFormat);
   LogV(LOG_COMMENT, szFormat, pArgs);
   va_end(pArgs);
}

//------------------------------------------------------------------------------

DWORD LogGetErrorCount(void)
{
   return g_LogInfo.m_nErrorCount;
}

//------------------------------------------------------------------------------

DWORD LogGetWarningCount(void)
{
   return g_LogInfo.m_nWarningCount;
}

//------------------------------------------------------------------------------

void LogSetWATTOutput(BOOL bWATTOutput)
{
   g_LogInfo.m_bWATTOutput = bWATTOutput;
}

//------------------------------------------------------------------------------

void LogSetLevel(DWORD dwLogLevel)
{
   g_LogInfo.m_dwLogLevel = dwLogLevel;
}

//------------------------------------------------------------------------------

DWORD LogGetLevel()
{
   return g_LogInfo.m_dwLogLevel;
}

//------------------------------------------------------------------------------

void LogCSSDTestResult(LPCSSD_TEST_ENTRY lpTestEntry, DWORD dwStatus)
{
    //  Log the CSSD test result
    //  Format:
    //
    //      CSSD Test: TestCaseID, Descrip,, Status
    //
    LogMsg(TEXT("CSSD Test: %s, %s,, %s"), 
        lpTestEntry->lpTestCaseID,
        lpTestEntry->lpDescription,
        g_CSSDTestResults[dwStatus]);
}

//------------------------------------------------------------------------------


