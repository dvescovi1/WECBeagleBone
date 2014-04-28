//
//  All rights reserved Texas Instruments, Inc. 2011
//
// File:  prutest.cpp
//
// Test utility for PRU driver.
//

#include <windows.h>
#include <mmsystem.h>
#include "pruioctl.h"


void RetailPrint(wchar_t *pszFormat, ...);


///////////////////////////////////////////////////////////////////////////////

void showHelp(void)
{
    RetailPrint(L"Usage: prutest [options]\n");
    RetailPrint(L"Options:\n");
//    RetailPrint(L"  -f <filename>   file name of firmware to test\n");
    RetailPrint(L"  -i <test#|?>    firmware test case #; ? to see cases\n");
}

///////////////////////////////////////////////////////////////////////////////

void ShowTestCases(void)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;
    char fname[MAX_PATH];
    size_t len;

    RetailPrint(L"Show test cases\r\n");

    hDevice = CreateFile(L"PRU1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (!DeviceIoControl(hDevice, PRUIOCTL_SHOW_TEST_CASES,
                            fname, len,
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"DeviceIoControl failed (%x)\r\n", GetLastError());
        }

        CloseHandle(hDevice);
    }
    else
    {
        RetailPrint(L"Failed to open PRU device (%d)\r\n", GetLastError());
    }
}


///////////////////////////////////////////////////////////////////////////////

void executeFirmware(PTSTR pFilename)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;
    char fname[MAX_PATH];
    size_t len;

    if (!pFilename)
    {
        RetailPrint(L"No firmware filename specified\r\n");
        return;
    }

    RetailPrint(L"executeFirmware: %s\r\n", pFilename);

    fname[0] = '\0';
    len = wcstombs(fname, pFilename, _tcslen(pFilename));
    fname[len] = '\0';

    hDevice = CreateFile(L"PRU1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (!DeviceIoControl(hDevice, PRUIOCTL_EXECUTE,
                            fname, len,
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"DeviceIoControl failed (%x)\r\n", GetLastError());
        }

        CloseHandle(hDevice);
    }
    else
    {
        RetailPrint(L"Failed to open PRU device (%d)\r\n", GetLastError());
    }

}

///////////////////////////////////////////////////////////////////////////////

void executeFirmwareCase(UINT32 case_no)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;

    RetailPrint(L"executeFirmwareCase: %d\r\n", case_no);

    hDevice = CreateFile(L"PRU1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (!DeviceIoControl(hDevice, PRUIOCTL_EXECUTE_CASE_NO,
                            &case_no, sizeof(case_no),
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"DeviceIoControl failed (%x)\n", GetLastError());
        }

        CloseHandle(hDevice);
    }
    else
    {
        RetailPrint(L"Failed to open WAV device (%d)\n", GetLastError());
    }

}
///////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, 
    int nCmdShow
)
{
    PTSTR pOpt;
    TCHAR ws[] = L" \t";
    BOOL bShowHelp = TRUE;
    UINT32 case_no, i;

    for (pOpt = _tcstok(lpCmdLine, ws); pOpt != NULL; pOpt = _tcstok(NULL, ws))
    {
        if (pOpt[0] != '/' && pOpt[0] != '-')
        {
            RetailPrint(L"Unrecognised command line option: %s\n", pOpt);
            continue;
        }
        pOpt++;
        bShowHelp = FALSE;

        if (!_tcscmp(pOpt, L"f"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                executeFirmware(pOpt);
            }
            else
            {
                RetailPrint(L"missing firmware filename\r\n");
                break;
            }
        }
        else if (!_tcscmp(pOpt, L"i"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                if (pOpt[0] == '?')
                {
                    ShowTestCases();
                    break;
                }
                else if (pOpt[0] == '0' && pOpt[1] == 'x')
                {
                    case_no = 0;

                    for(i=2; i<10; i++)
                    {
                        if (pOpt[i] >= 'A' && pOpt[i] <= 'F')
                            case_no = 16 * case_no + (pOpt[i] - 'A' + 10);
                        else
                            case_no = 16 * case_no + (pOpt[i] - '0');
                    }
                }
                else
                    case_no = _ttol(pOpt);

                executeFirmwareCase(case_no);
            }
            else
            {
                RetailPrint(L"missing firmware case_no\r\n");
                break;
            }
        }
        else
        {
            RetailPrint(L"Unrecognised command line option: %s\n", --pOpt);
        }
    }

    if (bShowHelp)
        showHelp();

    return 0;
}



///////////////////////////////////////////////////////////////////////////////

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    va_start(al, pszFormat);
    vwprintf(pszFormat, al);
    va_end(al);
}


