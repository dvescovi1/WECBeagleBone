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

#ifndef __LOG_H
#define __LOG_H

//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

#define LOG_EXCEPTION           0
#define LOG_FAIL                2
#define LOG_WARNING             3
#define LOG_ABORT               4
#define LOG_SKIP                6
#define LOG_NOT_IMPLEMENTED     8
#define LOG_PASS                10
#define LOG_DETAIL              12
#define LOG_COMMENT             14

//------------------------------------------------------------------------------

typedef void (*PLOG_FUNCTION)(DWORD dwLevel, LPCTSTR sz);

void LogStartup(LPCTSTR szTestName, DWORD dwLogLevel, PLOG_FUNCTION pLogFce);
void LogCleanup();
void LogSetWATTOutput(BOOL bWATTOutput);
void LogSetLevel(DWORD dwLogLevel);
DWORD LogGetLevel();

void LogErr(LPCTSTR szFormat, ...);
void LogWrn(LPCTSTR szFormat, ...);
void LogMsg(LPCTSTR szFormat, ...);
void LogDbg(LPCTSTR szFormat, ...);
void LogVbs(LPCTSTR szFormat, ...);

void Log(DWORD dwLevel, LPCTSTR szFormat, ...);

DWORD LogGetErrorCount(void);
DWORD LogGetWarningCount(void);

//------------------------------------------------------------------------------

#define CSSD_TEST_PASS      0
#define CSSD_TEST_FAIL      1
#define CSSD_TEST_SKIP      2
#define CSSD_TEST_BLOCKED   3
#define CSSD_TEST_UNTESTED  4


typedef struct _CSSD_TEST_ENTRY {
  LPCTSTR lpTestCaseID;
  LPCTSTR lpDescription;
  LPCTSTR lpSRs;
} CSSD_TEST_ENTRY, *LPCSSD_TEST_ENTRY;


void LogCSSDTestResult(LPCSSD_TEST_ENTRY lpTestEntry, DWORD dwStatus);


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------

#endif
