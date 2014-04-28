//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
// File:  edmatest.c
//
// Calls into the EDMA test driver (via an IOControl) to perform a simple
// memory to memory transfer test.

#include <windows.h>
#include <Winbase.h>
#include <ceddk.h>
#include <nkintr.h>
#include <windev.h>

extern int  CreateArgvArgc(TCHAR *pProgName, TCHAR *argv[20], TCHAR *pCmdLine);

#define NUM_PARAMS 7

#define NUM_TEST_CASES 8

static TCHAR *aTestCases[NUM_TEST_CASES] =
{
    TEXT("mem to mem test, parameterised"),
    TEXT("mem to mem test, manual trigger, no interrupt"),
    TEXT("mem to mem test, manual trigger, with interrupt"),
    TEXT("mem to mem test, manual trigger, no interrupt, two linked channels"),
    TEXT("mem to mem test, manual trigger, with interrupt, two linked channels"),
    TEXT("mem to mem test, manual trigger, with interrupt, repeat 5000 times"),
    TEXT("mem to mem test, QDMA trigger, no interrupt"),
    TEXT("mem to mem test, QDMA trigger, with interrupt")
};

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    va_start(al, pszFormat);
    vwprintf(pszFormat, al);
    va_end(al);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    HANDLE hDevice;
    DWORD dwTestId;
    TCHAR *argv[20];
    int argc;
    int test;
    DWORD params[NUM_PARAMS];
    int i;
    BOOL fSuccess = FALSE;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmShow);

    // Parse command line
    argc = CreateArgvArgc(TEXT("edmatest" ), argv, lpCmdLine);
    if (argc < 3 || !swscanf(argv[1], TEXT("%d"), &dwTestId) ||
        (dwTestId == 0 && argc != (NUM_PARAMS+2)))
    {
        RetailPrint(TEXT("Usage: edmatest <TestId> <Instance>\n"));
        RetailPrint(TEXT("  where <TestId> is:\n"));
        for (test = 1; test < NUM_TEST_CASES; ++test)
            RetailPrint(TEXT("    %d  - %s\n"), test, aTestCases[test]);
        RetailPrint(TEXT("  and <Instance> is the channel controller instance:\n"));
        RetailPrint(TEXT("OR: edmatest 0 <Instance> <QDMA> <Int> <Linked> <Size> <Queue> <Interations>\n"));
        RetailPrint(TEXT("  where:\n"));
        RetailPrint(TEXT("    <Instance>     - EDMA channel controller instance\n"));
        RetailPrint(TEXT("    <QDMA>         - 1 for QDMA, 0 otherwise\n"));
        RetailPrint(TEXT("    <Int>          - 1 for interrupt mode, 0 otherwise\n"));
        RetailPrint(TEXT("    <Linked>       - 1 for Linked transfer, 0 otherwise\n"));
        RetailPrint(TEXT("    <Size>         - Size of transfer to perform (bytes)\n"));
        RetailPrint(TEXT("    <Queue>        - Event queue (TC) to use\n"));
        RetailPrint(TEXT("    <Interations>  - Number of tranfers to perform\n"));
        return 1;
    }

    if (dwTestId == 0)
    {
        for (i = 0; i < NUM_PARAMS; ++i)
        {
            if (!swscanf(argv[i+2], TEXT("%d"), &params[i]))
            {
                RetailPrint(TEXT("edmatest: Invalid test parameter: %d\n"), i);
                return 1;
            }
        }
    }
    else if (dwTestId < NUM_TEST_CASES)
    {   
        if (!swscanf(argv[2], TEXT("%d"), &params[0]))
        {
            RetailPrint(TEXT("edmatest: Invalid test parameter: 0\n"));
            return 1;
        }
    }
    else
    {
        RetailPrint(TEXT("edmatest: Invalid test ID: %d\n"), dwTestId);
        return 1;
    }

    hDevice = CreateFile(L"EDT1:", 0, 0, NULL, 0, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        RetailPrint(TEXT("edmatest: Failed to open EDMA test driver\n"));
        return 1;
    }

    RetailPrint(TEXT("edmatest: Start EDMA test %d, %s\n"), dwTestId, aTestCases[dwTestId]);

    fSuccess = DeviceIoControl(hDevice, dwTestId, params, sizeof(DWORD) * NUM_PARAMS, NULL, 0, NULL, NULL);

    RetailPrint(TEXT("edmatest: End EDMA test %d, test %s\n"),
                dwTestId, fSuccess ? L"successful" : L"FAILED");

    CloseHandle(hDevice);

    return 0;
}
