//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#include <windows.h>

//////////////////////////////////////////////////////////////////////////////////////
// Debug printing
void DebugPrint(wchar_t *pszFormat, ...)
{
    va_list al;
   
    va_start(al, pszFormat);
    vwprintf(pszFormat, al);
    va_end(al);

#ifdef DEBUG
    {
        wchar_t szTemp[1024];
        wchar_t szTempFormat[1024];

        va_start(al, pszFormat);
        swprintf(szTempFormat, L"wavevol: %s", pszFormat);
        vswprintf(szTemp, szTempFormat, al);
        va_end(al);
        OutputDebugString(szTemp);
    }
#endif
    
}

#define MASTER_VOLUME 999

MMRESULT SetVolume(WORD uDevice, DWORD dwVol)
{
    MMRESULT mr;
    WAVEOUTCAPS woc;
    WAVEFORMATEX wfx;
    HWAVEOUT hwo;

    if (uDevice == MASTER_VOLUME)
    {
        // Set master volume
        mr = waveOutSetVolume((HWAVEOUT)0, dwVol);
    }
    else
    {
        mr = waveOutGetDevCaps(uDevice, &woc, sizeof(woc));
        if (mr != MMSYSERR_NOERROR)
        {
            DebugPrint(TEXT("ERROR: Failed to get wave out dev caps 0x%x\n"), mr);
        }
        else
        {
            // Assume our device supports 44.1KHz stereo
            memset(&wfx, 0, sizeof(wfx));
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 2;
            wfx.wBitsPerSample = (woc.dwFormats & WAVE_FORMAT_4S16) ? 16 : 8;
            wfx.nSamplesPerSec = 44100;
            wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
            wfx.cbSize = 0;

            mr = waveOutOpen(&hwo, uDevice, &wfx, 0, NULL, 0);
            if (mr != MMSYSERR_NOERROR)
            {
                DebugPrint(TEXT("ERROR: Failed to open wave device %d, 0x%x\n"), uDevice, mr);
            }
            else
            {
                mr = waveOutSetVolume(hwo, dwVol);
                if (mr != MMSYSERR_NOERROR)
                {
                    DebugPrint(TEXT("ERROR: Failed to set playback volume 0x%x\n"), mr);
                }
                
                if (waveOutClose(hwo) != MMSYSERR_NOERROR)
                {
                    DebugPrint(TEXT("ERROR: Failed to close wave device %d, 0x%x\n"), uDevice, mr);
                }
            }
        }
    }

    return mr;
}

int
WINAPI WinMain
    ( HINSTANCE hInstance
    , HINSTANCE hPrevInstance
    , LPTSTR lpCmdLine
    , int nCmdShow
    )
{
    BOOL showHelp = false;
    UINT params;
    UINT deviceId;
    DWORD dwVolume;
    DWORD dwLeftVol;
    DWORD dwRightVol;
    
    if (lpCmdLine == NULL || *lpCmdLine == 0)
    {
        showHelp = true;
    }
    else
    {
        params = swscanf(lpCmdLine, L"%d %d", &deviceId, &dwVolume);
        if (params == 0)
        {
            showHelp = true;
        }
        else
        {
            if (params == 1)
            {
                dwVolume = deviceId;
                deviceId = MASTER_VOLUME;
            }

            if (dwVolume > 100)
            {
                showHelp = true;
            }
        }
    }

    if (showHelp)
    {
        DebugPrint(TEXT("Usage:\n"));
        DebugPrint(TEXT("  wavevol [<deviceID>] <volume>\n"));
        DebugPrint(TEXT("Where:\n"));
        DebugPrint(TEXT("  deviceID  - Output Device to set. Master volume is set if none specified.\n"));
        DebugPrint(TEXT("  volume    - Volume in range 0 (min) to 100 (max)\n"));
        DebugPrint(TEXT("\n"));
    }
    else
    {
        // Convert volume into the range 0-65535
        if (dwVolume > 0)
        {
            dwLeftVol = (dwVolume * (0xFFFF / 100)) + (0xFFFF % 100);
            dwRightVol = (dwVolume * (0xFFFF / 100)) + (0xFFFF % 100);
            dwVolume = (dwRightVol << 16) | dwLeftVol;
        }
        else
        {
            // Muted
            dwVolume = 0;
        }

        if (deviceId == MASTER_VOLUME)
            DebugPrint(TEXT("Setting master volume to 0x%x\n"), dwVolume);
        else
            DebugPrint(TEXT("Setting device %d volume to 0x%x\n"), deviceId, dwVolume);
        
        SetVolume(deviceId, dwVolume);
    }

    return 0;
}
