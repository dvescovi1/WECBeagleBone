//
// Copyright (c) MPC Data Limited 2009. All Rights Reserved.
// Copyright (c) Texas Instruments Inc. 2009. All Rights Reserved.
//
// File:  wavtest.cpp
//
// Test utility for WAVEDEV2.
//

#include <windows.h>
#include <mmsystem.h>
#include "wavioctl.h"


void RetailPrint(wchar_t *pszFormat, ...);


///////////////////////////////////////////////////////////////////////////////

void showHelp(void)
{
    RetailPrint(L"Usage: wavtest [options]\n");
    RetailPrint(L"Options:\n");
    RetailPrint(L"  -c           Get current configuration\n");
    RetailPrint(L"  -v           Set audio input volume, 0 (min) to 100 (max)\n");
    RetailPrint(L"  -g           Get audio input volume\n");
    RetailPrint(L"  -o <dev>     Enable output device(s):\n");
    RetailPrint(L"                 %d - high-power out\n", WAV_SET_OUTPUT_HDP);
    RetailPrint(L"                 %d - line out\n", WAV_SET_OUTPUT_LINEOUT);
    RetailPrint(L"  -i <dev>     Enable input device(s):\n");
    RetailPrint(L"                 %d - microphone\n", WAV_SET_INPUT_MIC3 >> 4);
    RetailPrint(L"                 %d - line in\n", WAV_SET_INPUT_LINE1 >> 4);
    RetailPrint(L"  -a <reg#>    show aic reg value\n");
    RetailPrint(L"  -m <regofs>  show mcasp reg value\n");
    RetailPrint(L"  -w           show W(I/O)DM_* received\n");
    RetailPrint(L"  -d <ofs> <len> show dbg dma buffer\n");
    RetailPrint(L"  -b <addr> set dma buffer base addr\n");
#if PROFILE_ENABLE
    RetailPrint(L"  -t <0|1>     show interrupt time stamps\n");
#endif
}


///////////////////////////////////////////////////////////////////////////////

void GetCurrentConfig(void)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;
    DWORD dwInputMask = 0;
    BOOL  bHasInputMask = FALSE;
    DWORD dwOutputMask = 0;
    BOOL  bHasOutputMask = FALSE;
    
    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if ( hDevice == NULL )
    {
        RetailPrint(L"Failed to open WAV device (%d)\n", GetLastError());
    }
    else
    {
        if ( !DeviceIoControl(hDevice, WAVIOCTL_GET_OUTPUT,
                              NULL, 0,
                              &dwOutputMask, sizeof(dwOutputMask),
                              &dwBytesReturned, NULL) )
        {
            RetailPrint(L"DeviceIoControl failed (%x)\n", GetLastError());
        }
        else
        {
            bHasOutputMask = TRUE;
        }

        if ( !DeviceIoControl(hDevice, WAVIOCTL_GET_INPUT,
                              NULL, 0,
                              &dwInputMask, sizeof(dwInputMask),
                              &dwBytesReturned, NULL) )
        {
            RetailPrint(L"DeviceIoControl failed (%x)\n", GetLastError());
        }
        else
        {
            bHasInputMask = TRUE;
        }
        CloseHandle(hDevice);
    }
    
    RetailPrint(L"  CURRENT STATUS\n");

    RetailPrint(L"    Input Devices ..:  ");
    if ( bHasInputMask )
    {
        if ( dwInputMask == 0 )
        {
            RetailPrint(L"None\n");
        }
        else
        {
            if ( dwInputMask & WAV_SET_INPUT_MIC3 )
            {
                RetailPrint(L"Microphone");
            }
            if ( dwInputMask & WAV_SET_INPUT_LINE1 )
            {
                RetailPrint(L"Line in");
            }
            RetailPrint(L" (%d)\n", dwInputMask >> 4);
        }
    }
    else
    {
        RetailPrint(L"unknown\n");
    }

    RetailPrint(L"    Output Devices .:  ");
    if ( bHasOutputMask )
    {
        if ( dwOutputMask == 0 )
        {
            RetailPrint(L"None\n");
        }
        else
        {
            if ( dwOutputMask & WAV_SET_OUTPUT_HDP )
            {
                RetailPrint(L"High-power out");
            }
            if ( dwOutputMask & WAV_SET_OUTPUT_LINEOUT )
            {
                RetailPrint(L"Line out");
            }
            RetailPrint(L" (%d)\n", dwOutputMask);
        }
    }
    else
    {
        RetailPrint(L"unknown\n");
    }

}


///////////////////////////////////////////////////////////////////////////////

void getInputVolume(void)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;
    DWORD dwInputVol = 0;
    
    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if ( hDevice == NULL )
    {
        RetailPrint(L"Failed to open WAV device (%d)\n", GetLastError());
        return;
    }

    
    if ( !DeviceIoControl(hDevice, WAVIOCTL_GET_INPUT_VOLUME,
                          NULL, 0,
                          &dwInputVol, sizeof(dwInputVol),
                          &dwBytesReturned, NULL) )
    {
        RetailPrint(L"DeviceIoControl failed (%x)\n", GetLastError());
    }
    else
    {
        RetailPrint(L" Input Volume: L=~%d R=~%d (%08X)\n", 
            ((((dwInputVol & 0xFFFF0000) >> 16) * 100)/0xFFFF),
            (((dwInputVol & 0x0000FFFF) * 100)/0xFFFF),
            dwInputVol);
    }

    CloseHandle(hDevice);
    return;
}


///////////////////////////////////////////////////////////////////////////////

void setOutputDevices(DWORD dwOutputMask)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;

    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (DeviceIoControl(hDevice, WAVIOCTL_SET_OUTPUT,
                            &dwOutputMask, sizeof(dwOutputMask),
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"Output set to %d\n", dwOutputMask);
        }
        else
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

void setInputDevices(DWORD dwInputMask)
{
    HANDLE hDevice;
    DWORD dwBytesReturned;

    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (DeviceIoControl(hDevice, WAVIOCTL_SET_INPUT,
                            &dwInputMask, sizeof(dwInputMask),
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"Input set to %d\n", dwInputMask >> 4);
        }
        else
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

void setInputVolume(UINT32 volume)
{
    DWORD dwVol;
    DWORD dwLeftVol;
    DWORD dwRightVol;
    HANDLE hDevice;
    DWORD dwBytesReturned;

    if (volume > 0)
    {
        dwLeftVol = (volume * (0xFFFF / 100)) + (0xFFFF % 100);
        dwRightVol = (volume * (0xFFFF / 100)) + (0xFFFF % 100);
        dwVol = (dwRightVol << 16) | dwLeftVol;
    }
    else
    {
        dwVol = 0;
    }

    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (DeviceIoControl(hDevice, WAVIOCTL_SET_INPUT_VOLUME,
                            &dwVol, sizeof(dwVol),
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"Input volume set to %x\n", dwVol);
        }
        else
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


void GetAicReg(UINT32 reg)
{
    DWORD dwOut;
    DWORD dwIn;
    HANDLE hDevice;
    DWORD dwBytesReturned;


    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        dwIn = (DWORD)reg;

        if (DeviceIoControl(hDevice, WAVIOCTL_GET_AIC_REG,
                            &dwIn, sizeof(dwIn),
                            &dwOut, sizeof(dwOut),
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"AIC reg %d val=0x%08X\n", dwIn, dwOut);
        }
        else
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


void GetMcaspReg(UINT32 reg)
{
    DWORD dwOut;
    DWORD dwIn;
    HANDLE hDevice;
    DWORD dwBytesReturned;


    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        dwIn = (DWORD)reg;

        if (DeviceIoControl(hDevice, WAVIOCTL_GET_MCASP_REG,
                            &dwIn, sizeof(dwIn),
                            &dwOut, sizeof(dwOut),
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"McASP reg offset 0x%04X val=0x%08X\n", dwIn, dwOut);
        }
        else
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


void ShowWDM(void)
{
    DWORD dwOut;
    DWORD dwIn;
    HANDLE hDevice;
    DWORD dwBytesReturned;


    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        DeviceIoControl(hDevice, WAVIOCTL_SHOW_WDM,
                            &dwIn, sizeof(dwIn),
                            &dwOut, sizeof(dwOut),
                            &dwBytesReturned, NULL);

        CloseHandle(hDevice);
    }
    else
    {
        RetailPrint(L"Failed to open WAV device (%d)\n", GetLastError());
    }

}

#if PROFILE_ENABLE
void ShowIntrTime(DWORD action)
{
    DWORD dwIn = action;
    HANDLE hDevice;
    DWORD dwBytesReturned;

    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        if (!DeviceIoControl(hDevice, WAVIOCTL_SHOW_INTR_TIME,
                            &dwIn, sizeof(dwIn),
                            NULL, 0,
                            &dwBytesReturned, NULL))
        {
            RetailPrint(L"ShowIntrTime: DeviceIoControl failed (%x)\n", GetLastError());
        }

        CloseHandle(hDevice);
    }
    else
    {
        RetailPrint(L"ShowIntrTime: Failed to open WAV device (%d)\n", GetLastError());
    }
}
#endif

void ShowDbgDmaBuf(UINT32 addr, UINT32 len)
{
    DWORD dwOut;
    DWORD dwIn;
    HANDLE hDevice;
    DWORD dwBytesReturned;


    hDevice = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
    if (hDevice)
    {
        dwIn = (DWORD)addr;
        dwOut = (DWORD)len;
        DeviceIoControl(hDevice, WAVIOCTL_SHOW_DMA_BUF,
                            &dwIn, sizeof(dwIn),
                            &dwOut, sizeof(dwOut),
                            &dwBytesReturned, NULL);

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
    UINT32 outputMask = 0;
    UINT32 inputMask = 0;
    UINT32 volume = 0;
    UINT32 reg = 0;
    UINT32 addr, len, i;
#if PROFILE_ENABLE
    UINT32 action = 1;
#endif

    for (pOpt = _tcstok(lpCmdLine, ws); pOpt != NULL; pOpt = _tcstok(NULL, ws))
    {
        if (pOpt[0] != '/' && pOpt[0] != '-')
        {
            RetailPrint(L"Unrecognised command line option: %s\n", pOpt);
            continue;
        }
        pOpt++;
        bShowHelp = FALSE;

        if (!_tcscmp(pOpt, L"o"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                outputMask = _ttoi(pOpt);
                setOutputDevices(outputMask);
            }
            else
            {
                RetailPrint(L"No output specified\n");
            }
        }
        else if (!_tcscmp(pOpt, L"v"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                volume = _ttoi(pOpt);
                if (volume > 100)
                    RetailPrint(L"Invalid volume specified\r\n");
                else
                    setInputVolume(volume);
            }
            else
            {
                RetailPrint(L"No volume specified\r\n");
            }
        }
        else if (!_tcscmp(pOpt, L"g"))
        {
            getInputVolume();
        }
        else if (!_tcscmp(pOpt, L"i"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                inputMask = _ttoi(pOpt);
                setInputDevices(inputMask << 4);
            }
            else
            {
                RetailPrint(L"No output specified\n");
            }
        }
        else if (!_tcscmp(pOpt, L"c"))
        {
            GetCurrentConfig();
        }
        else if (!_tcscmp(pOpt, L"a"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                reg = _ttoi(pOpt);
                RetailPrint(L"calling GetAicReg reg=%d\r\n", reg);
                GetAicReg(reg);
            }
            else
            {
                RetailPrint(L"No reg specified\r\n");
            }
        }
        else if (!_tcscmp(pOpt, L"m"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                reg = _ttoi(pOpt);
                RetailPrint(L"calling GetMcaspReg reg offset=0x%04X\r\n", reg);
                GetMcaspReg(reg);
            }
            else
            {
                RetailPrint(L"No reg specified\r\n");
            }
        }
        else if (!_tcscmp(pOpt, L"w"))
        {
            ShowWDM();
        }
#if PROFILE_ENABLE
        else if (!_tcscmp(pOpt, L"t"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                action = _ttoi(pOpt);
            }
            else
            {
                action = 1;
            }
            ShowIntrTime(action);
        }
#endif
#if 0
        else if (!_tcscmp(pOpt, L"d"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                addr = _ttol(pOpt);
            }
            else
            {
                RetailPrint(L"No addr offset specified\r\n");
                break;
            }

            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                len = _ttoi(pOpt);
            }
            else
            {
                RetailPrint(L"No len specified\r\n");
                break;
            }

            ShowDbgDmaBuf(addr, len);
        }
#endif
        else if (!_tcscmp(pOpt, L"d"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                if (pOpt[0] == '0' && pOpt[1] == 'x')
                {
                    addr = 0;

                    for(i=2; i<10; i++)
                    {
                        if (pOpt[i] >= 'A' && pOpt[i] <= 'F')
                            addr = 16 * addr + (pOpt[i] - 'A' + 10);
                        else
                            addr = 16 * addr + (pOpt[i] - '0');
                    }
                }
                else
                    addr = _ttol(pOpt);
            }
            else
            {
                RetailPrint(L"No addr specified\r\n");
                break;
            }

            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                len = _ttoi(pOpt);
            }
            else
            {
                RetailPrint(L"No len specified\r\n");
                break;
            }

            ShowDbgDmaBuf(addr, len);
        }
        else if (!_tcscmp(pOpt, L"b"))
        {
            pOpt = _tcstok(NULL, ws);
            if (pOpt)
            {
                if (pOpt[0] == '0' && pOpt[1] == 'x')
                {
                    addr = 0;

                    for(i=2; i<10; i++)
                    {
                        if (pOpt[i] >= 'A' && pOpt[i] <= 'F')
                            addr = 16 * addr + (pOpt[i] - 'A' + 10);
                        else
                            addr = 16 * addr + (pOpt[i] - '0');
                    }
                }
                else
                    addr = _ttol(pOpt);

                RetailPrint(L"dma buffer base addr=0x%08X\r\n", addr);
            }
            else
            {
                RetailPrint(L"No addr specified\r\n");
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


