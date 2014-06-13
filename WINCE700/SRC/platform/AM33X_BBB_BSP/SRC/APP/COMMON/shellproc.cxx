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
#include <tux.h>
#include <kato.h>
#include "Log.h"
#include "Utils.h"
#include "ShellProc.h"

//------------------------------------------------------------------------------

static INT ProcessBeginGroup(FUNCTION_TABLE_ENTRY *pFTE);
static INT ProcessEndGroup(FUNCTION_TABLE_ENTRY *pFTE);
static INT ProcessBeginScript();
static INT ProcessEndScript();
static INT ProcessBeginTest(SPS_BEGIN_TEST *pBT);
static INT ProcessEndTest(SPS_END_TEST *pET);
static INT ProcessException(SPS_EXCEPTION *pException);
static VOID LogKato(DWORD verbosity, LPCWSTR sz);

//------------------------------------------------------------------------------

// Global shell info structure
static SPS_SHELL_INFO* g_pShellInfo = NULL;

// Global logging object
static CKato* g_pKato = NULL;

// Global critical section to be used by threaded tests if necessary.
CRITICAL_SECTION g_csProcess;

//------------------------------------------------------------------------------
// ShellProc()
//------------------------------------------------------------------------------

SHELLPROCAPI 
ShellProc(
    UINT msg, 
    SPPARAM spParam
    )
{
    INT rc = SPR_NOT_HANDLED;

    switch (msg)
        {
        case SPM_LOAD_DLL:
            // If we are UNICODE
            ((SPS_LOAD_DLL*)spParam)->fUnicode = TRUE;
            // Get/create a global logging object
            g_pKato = (CKato*)KatoGetDefaultObject();
            // Initialize our global critical section
            InitializeCriticalSection(&g_csProcess);
            rc = SPR_HANDLED;
            break;

       case SPM_UNLOAD_DLL:
            // Destroy our global critical section
            DeleteCriticalSection(&g_csProcess);
            rc = SPR_HANDLED;
            break;

        case SPM_SHELL_INFO:
            // Store a pointer to our shell info for later use.
            g_pShellInfo = (SPS_SHELL_INFO*)spParam;
            // This should be there because we can cancel test only there
            rc = ProcessBeginScript();
            break;

        case SPM_REGISTER:
            ((SPS_REGISTER*)spParam)->lpFunctionTable = g_lpFTE;
            rc = SPR_HANDLED | SPF_UNICODE;
            break;

        case SPM_START_SCRIPT:
            rc = SPR_HANDLED;
            break;

        case SPM_STOP_SCRIPT:
            rc = ProcessEndScript();
            break;

        case SPM_BEGIN_GROUP:
            rc = ProcessBeginGroup((FUNCTION_TABLE_ENTRY*)spParam);
            break;

        case SPM_END_GROUP:
            rc = ProcessEndGroup((FUNCTION_TABLE_ENTRY*)spParam);
            break;

        case SPM_BEGIN_TEST:
            rc = ProcessBeginTest((SPS_BEGIN_TEST*)spParam);
            break;

        case SPM_END_TEST:
            rc = ProcessEndTest((SPS_END_TEST*)spParam);
            break;

        case SPM_EXCEPTION:
            rc = ProcessException((SPS_EXCEPTION*)spParam);
            break;
        }

   return rc;
}

//------------------------------------------------------------------------------
// ProcessBeginScript()
//------------------------------------------------------------------------------

INT 
ProcessBeginScript(
    )
{
    INT rc = SPR_FAIL;
    WCHAR cmdLine[512];
    LPWSTR argv[64];
    int argc = 64;
    DWORD outputLevel = LOG_PASS;

    // Make a local copy of command line
    if (lstrlen(g_pShellInfo->szDllCmdLine) >= sizeof(cmdLine)) goto cleanUp;
    lstrcpy(cmdLine, g_pShellInfo->szDllCmdLine);

    // Divide command line to token
    CommandLineToArgs(cmdLine, &argc, argv);

    // Parse for option string
    outputLevel = GetOptionLong(&argc, argv, outputLevel, L"v");

    // Initialize the Logwrapper
    LogStartup(g_szTestGroupName, outputLevel, LogKato);

    // Let parse test specific options
    if (!ParseCommands(argc, argv)) goto cleanUp;

    // Prepare enviroment to run a test suite
    if (!PrepareToRun()) goto cleanUp;
   
    // When we are there it's success
    rc = SPR_HANDLED;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
// ProcessEndScript()
//------------------------------------------------------------------------------

INT 
ProcessEndScript(
    )
{
    // Clean enviroment after test suite
    FinishRun();

    // Finish up log
    LogCleanup();

    return SPR_HANDLED;
}

//------------------------------------------------------------------------------
// ProcessBeginGroup()
//------------------------------------------------------------------------------

INT
ProcessBeginGroup(
    FUNCTION_TABLE_ENTRY *pFTE
    )
{
	UNREFERENCED_PARAMETER(pFTE);

    g_pKato->BeginLevel(0, L"BEGIN GROUP: \"%s\"\r\n", g_szTestGroupName);
    return SPR_HANDLED;
}

//------------------------------------------------------------------------------
// ProcessEndGroup()
//------------------------------------------------------------------------------

INT
ProcessEndGroup(
    FUNCTION_TABLE_ENTRY *pFTE
    )
{
	UNREFERENCED_PARAMETER(pFTE);

    g_pKato->EndLevel(L"END GROUP: \"%s\"\r\n", g_szTestGroupName);
    return SPR_HANDLED;
}

//------------------------------------------------------------------------------
// ProcessBeginTest()
//------------------------------------------------------------------------------

INT
ProcessBeginTest(
    SPS_BEGIN_TEST *pBT
    )
{
    // Start our logging level.
    g_pKato->BeginLevel(
        pBT->lpFTE->dwUniqueID, L"BEGIN TEST: \"%s\", Threads=%u, Seed=%u\r\n",
        pBT->lpFTE->lpDescription, pBT->dwThreadCount, pBT->dwRandomSeed
        );
    return SPR_HANDLED;
}

//------------------------------------------------------------------------------
// ProcessEndTest()
//------------------------------------------------------------------------------

INT 
ProcessEndTest(
    SPS_END_TEST *pET
    )
{  
    LPWSTR szResult = NULL;

    switch (pET->dwResult)
        {
        case TPR_SKIP:
            szResult = L"SKIPPED";
            break;
        case TPR_PASS:
            szResult = L"PASSED";
            break;
        case TPR_FAIL:
            szResult = L"FAILED";
            break;
        default:
            szResult = L"ABORTED";
            break;
        }
    g_pKato->EndLevel(
        L"END TEST: \"%s\", %s, Time=%u.%03u\r\n", pET->lpFTE->lpDescription,
        szResult, pET->dwExecutionTime/1000, pET->dwExecutionTime%1000
        );
    return SPR_HANDLED;
}

//------------------------------------------------------------------------------
// ProcessException()
//------------------------------------------------------------------------------

INT
ProcessException(
    SPS_EXCEPTION *pException
    )
{
	UNREFERENCED_PARAMETER(pException);

    g_pKato->Log(LOG_EXCEPTION, L"An exception occurred...\r\n");
    return SPR_HANDLED;
}

//------------------------------------------------------------------------------

VOID LogKato(
    DWORD verbosity, 
    LPCWSTR sz
    )
{
    g_pKato->Log(verbosity, sz);
}

//------------------------------------------------------------------------------
// DllMain()
//------------------------------------------------------------------------------

BOOL WINAPI
DllMain(
    HANDLE hInstance, 
    ULONG reason, 
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(reason);
	UNREFERENCED_PARAMETER(pReserved);

   return TRUE;
}

//------------------------------------------------------------------------------
 

