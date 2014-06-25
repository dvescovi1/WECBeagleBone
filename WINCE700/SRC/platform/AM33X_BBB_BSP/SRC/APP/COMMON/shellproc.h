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

#ifndef __SHELLPROC_H
#define __SHELLPROC_H

//------------------------------------------------------------------------------

// Macros for common test constructs
#define TEST_FUNCTION(name) \
    TESTPROCAPI name( \
        UINT msg, \
        TPPARAM tpParam, \
        LPFUNCTION_TABLE_ENTRY pFTE \
        )

// Check our message value to see why we have been called.  
#define TEST_ENTRY \
    if (msg == TPM_QUERY_THREAD_COUNT) { \
       ((LPTPS_QUERY_THREAD_COUNT)tpParam)->dwThreadCount = pFTE->dwUserData; \
       return SPR_HANDLED; \
    } else if (msg != TPM_EXECUTE) { \
       return TPR_NOT_HANDLED; \
    }

//------------------------------------------------------------------------------

// Our function table that we pass to Tux
extern "C" FUNCTION_TABLE_ENTRY g_lpFTE[];

//------------------------------------------------------------------------------

extern LPCWSTR g_szTestGroupName;

//------------------------------------------------------------------------------

BOOL ParseCommands(INT argc, LPTSTR argv[]);
BOOL PrepareToRun();
BOOL FinishRun();

//------------------------------------------------------------------------------

#endif // __SHELLPROC_H
