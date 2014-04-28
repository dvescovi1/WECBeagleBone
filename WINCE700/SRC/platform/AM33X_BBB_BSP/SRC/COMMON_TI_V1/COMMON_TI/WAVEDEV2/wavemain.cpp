// All rights reserved ADENEO EMBEDDED 2010
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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include "wavemain.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("WaveDriver"), {
         TEXT("Test")           //  0
        ,TEXT("Params")         //  1
        ,TEXT("Verbose")        //  2
        ,TEXT("Interrupt")      //  3
        ,TEXT("WODM")           //  4
        ,TEXT("WIDM")           //  5
        ,TEXT("PDD")            //  6
        ,TEXT("MDD")            //  7
        ,TEXT("Regs")           //  8
        ,TEXT("Misc")           //  9
        ,TEXT("Init")           // 10
        ,TEXT("IOcontrol")      // 11
        ,TEXT("Alloc")          // 12
        ,TEXT("Function")       // 13
        ,TEXT("Warning")        // 14
        ,TEXT("Error")          // 15
    }
    ,
        (1 << 15)   // Errors
    |   (1 << 14)   // Warnings
};
#endif

//------------------------------------------------------------------------------
//
//  global mutex
//
static CRITICAL_SECTION  s_cs;

// -----------------------------------------------------------------------------
//
//  @doc    EnterMutex
//
//  @func   PVOID | EnterMutex | marks beginning of mutex
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------

EXTERN_C void
EnterMutex()
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+EnterMutex\r\n"));

    ::EnterCriticalSection(&s_cs);

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-EnterMutex\r\n"));
}

// -----------------------------------------------------------------------------
//
//  @doc    ExitMutex
//
//  @func   PVOID | ExitMutex | marks end of mutex
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------

EXTERN_C void
ExitMutex()
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+ExitMutex\r\n"));

    ::LeaveCriticalSection(&s_cs);

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-ExitMutex\r\n"));
}

BOOL CALLBACK DllMain(HANDLE hDLL,
                      DWORD dwReason,
                      LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH :
            DEBUGREGISTER((HINSTANCE)hDLL);
            DisableThreadLibraryCalls((HMODULE) hDLL);
            break;

        case DLL_PROCESS_DETACH :
            break;

        case DLL_THREAD_DETACH :
            break;

        case DLL_THREAD_ATTACH :
            break;

        default :
            break;
    }
    return TRUE;
}


// -----------------------------------------------------------------------------
//
// @doc     WDEV_EXT
//
// @topic   WAV Device Interface | Implements the WAVEDEV.DLL device
//          interface. These functions are required for the device to
//          be loaded by DEVICE.EXE.
//
// @xref                          <nl>
//          <f WAV_Init>,         <nl>
//          <f WAV_Deinit>,       <nl>
//          <f WAV_Open>,         <nl>
//          <f WAV_Close>,        <nl>
//          <f WAV_IOControl>     <nl>
//
// -----------------------------------------------------------------------------
//
// @doc     WDEV_EXT
//
// @topic   Designing a Waveform Audio Driver |
//          A waveform audio driver is responsible for processing messages
//          from the Wave API Manager (WAVEAPI.DLL) to playback and record
//          waveform audio. Waveform audio drivers are implemented as
//          dynamic link libraries that are loaded by DEVICE.EXE The
//          default waveform audio driver is named WAVEDEV.DLL (see figure).
//          The messages passed to the audio driver are similar to those
//          passed to a user-mode Windows NT audio driver (such as mmdrv.dll).
//
//          <bmp blk1_bmp>
//
//          Like all device drivers loaded by DEVICE.EXE, the waveform
//          audio driver must export the standard device functions,
//          XXX_Init, XXX_Deinit, XXX_IoControl, etc (see
//          <t WAV Device Interface>). The Waveform Audio Drivers
//          have a device prefix of "WAV".
//
//          Driver loading and unloading is handled by DEVICE.EXE and
//          WAVEAPI.DLL. Calls are made to <f WAV_Init> and <f WAV_Deinit>.
//          When the driver is opened by WAVEAPI.DLL calls are made to
//          <f WAV_Open> and <f WAV_Close>.  All
//          other communication between WAVEAPI.DLL and WAVEDEV.DLL is
//          done by calls to <f WAV_IOControl>. The other WAV_xxx functions
//          are not used.
//
// @xref                                          <nl>
//          <t Designing a Waveform Audio PDD>    <nl>
//          <t WAV Device Interface>              <nl>
//          <t Wave Input Driver Messages>        <nl>
//          <t Wave Output Driver Messages>       <nl>
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @func   PVOID | WAV_Init | Device initialization routine
//
//  @parm   DWORD | dwInfo | info passed to RegisterDevice
//
//  @rdesc  Returns a DWORD which will be passed to Open & Deinit or NULL if
//          unable to initialize the device.
//
// -----------------------------------------------------------------------------
extern "C" DWORD WAV_Init(DWORD Index)
{
	InitializeCriticalSection(&s_cs);

    return((DWORD)HardwareContext::CreateHWContext(Index));
}

// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @func   PVOID | WAV_Deinit | Device deinitialization routine
//
//  @parm   DWORD | dwData | value returned from WAV_Init call
//
//  @rdesc  Returns TRUE for success, FALSE for failure.
//
// -----------------------------------------------------------------------------
extern "C" BOOL WAV_Deinit(DWORD dwData)
{
	UNREFERENCED_PARAMETER(dwData);

    DeleteCriticalSection(&s_cs);

    return(g_pHWContext->Deinit());
}

// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @func   PVOID | WAV_Open    | Device open routine
//
//  @parm   DWORD | dwData      | Value returned from WAV_Init call (ignored)
//
//  @parm   DWORD | dwAccess    | Requested access (combination of GENERIC_READ
//                                and GENERIC_WRITE) (ignored)
//
//  @parm   DWORD | dwShareMode | Requested share mode (combination of
//                                FILE_SHARE_READ and FILE_SHARE_WRITE) (ignored)
//
//  @rdesc  Returns a DWORD which will be passed to Read, Write, etc or NULL if
//          unable to open device.
//
// -----------------------------------------------------------------------------
extern "C" PDWORD WAV_Open( DWORD dwData,
              DWORD dwAccess,
              DWORD dwShareMode)
{
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(dwAccess);
	UNREFERENCED_PARAMETER(dwShareMode);

    // allocate and return handle context to efficiently verify caller trust level
    return new DWORD(NULL); // assume untrusted. Can't tell for sure until WAV_IoControl
}

// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @func   BOOL | WAV_Close | Device close routine
//
//  @parm   DWORD | dwOpenData | Value returned from WAV_Open call
//
//  @rdesc  Returns TRUE for success, FALSE for failure
//
// -----------------------------------------------------------------------------
extern "C" BOOL WAV_Close(PDWORD pdwData)
{
    // we trust the device manager to give us a valid context to free.
    delete pdwData;
    return(TRUE);
}

BOOL HandleWaveMessage(PMMDRV_MESSAGE_PARAMS pParams, DWORD *pdwResult)
{
    //  set the error code to be no error first
    SetLastError(MMSYSERR_NOERROR);

    UINT uMsg = pParams->uMsg;
    UINT uDeviceId = pParams->uDeviceId;
    DWORD dwParam1 = pParams->dwParam1;
    DWORD dwParam2 = pParams->dwParam2;
    DWORD dwUser   = pParams->dwUser;
    StreamContext *pStreamContext = (StreamContext *)dwUser;

    DWORD dwRet;

    g_pHWContext->Lock();

    _try
    {
        switch (uMsg)
        {
        case WODM_GETNUMDEVS:
            {
                dwRet = g_pHWContext->GetNumOutputDevices();
                break;
            }

        case WIDM_GETNUMDEVS:
            {
                dwRet = g_pHWContext->GetNumInputDevices();
                break;
            }

        case WODM_GETDEVCAPS:
            {
                DeviceContext *pDeviceContext;

                if (pStreamContext)
                {
                    pDeviceContext=pStreamContext->GetDeviceContext();
                }
                else
                {
                    pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                }

                dwRet = pDeviceContext->GetDevCaps((PVOID)dwParam1,dwParam2);
                break;
            }


        case WIDM_GETDEVCAPS:
            {
                DeviceContext *pDeviceContext;

                if (pStreamContext)
                {
                    pDeviceContext=pStreamContext->GetDeviceContext();
                }
                else
                {
                    pDeviceContext = g_pHWContext->GetInputDeviceContext(uDeviceId);
                }

                dwRet = pDeviceContext->GetDevCaps((PVOID)dwParam1,dwParam2);
                break;
            }

        case WODM_GETEXTDEVCAPS:
            {
                DeviceContext *pDeviceContext;

                if (pStreamContext)
                {
                    pDeviceContext=pStreamContext->GetDeviceContext();
                }
                else
                {
                    pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                }

                dwRet = pDeviceContext->GetExtDevCaps((PVOID)dwParam1,dwParam2);
                break;
            }

        case WODM_OPEN:
            {
                // DEBUGMSG(1, (TEXT("WODM_OPEN\r\n"));
                DeviceContext *pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                dwRet = pDeviceContext->OpenStream((LPWAVEOPENDESC)dwParam1, dwParam2, (StreamContext **)dwUser);
                break;
            }

        case WIDM_OPEN:
            {
                // DEBUGMSG(1, (TEXT("WIDM_OPEN\r\n"));
                DeviceContext *pDeviceContext = g_pHWContext->GetInputDeviceContext(uDeviceId);
                dwRet = pDeviceContext->OpenStream((LPWAVEOPENDESC)dwParam1, dwParam2, (StreamContext **)dwUser);
                break;
            }

        case WODM_CLOSE:
        case WIDM_CLOSE:
            {
                // DEBUGMSG(1, (TEXT("WIDM_CLOSE/WODM_CLOSE\r\n"));
                dwRet = pStreamContext->Close();

                // Release stream context here, rather than inside StreamContext::Close, so that if someone
                // (like CMidiStream) has subclassed Close there's no chance that the object will get released
                // out from under them.
                if (dwRet==MMSYSERR_NOERROR)
                {
                    pStreamContext->Release();
                }
                break;
            }

        case WODM_RESTART:
        case WIDM_START:
            {
                dwRet = pStreamContext->Run();
                break;
            }

        case WODM_PAUSE:
        case WIDM_STOP:
            {
                dwRet = pStreamContext->Stop();
                break;
            }

        case WODM_GETPOS:
        case WIDM_GETPOS:
            {
                dwRet = pStreamContext->GetPos((PMMTIME)dwParam1);
                break;
            }

        case WODM_RESET:
        case WIDM_RESET:
            {
                dwRet = pStreamContext->Reset();
                break;
            }

        case WODM_WRITE:
        case WIDM_ADDBUFFER:
            {
                // DEBUGMSG(1, (TEXT("WODM_WRITE/WIDM_ADDBUFFER, Buffer=0x%x\r\n"),dwParam1);
                dwRet = pStreamContext->QueueBuffer((LPWAVEHDR)dwParam1);
                break;
            }

        case WODM_GETVOLUME:
            {
                PDWORD pdwGain = (PDWORD)dwParam1;

                if (pStreamContext)
                {
                    *pdwGain = pStreamContext->GetGain();
                }
                else
                {
                    *pdwGain = GetMasterVolume();
                }
                dwRet = MMSYSERR_NOERROR;
                break;
            }

        case WODM_SETVOLUME:
            {
                DWORD dwGain = dwParam1;
                if (pStreamContext)
                {
                    dwRet = pStreamContext->SetGain(dwGain);
                }
                else
                {
                    dwRet = SetMasterVolume(dwGain);
                }
                break;
            }

        case WODM_BREAKLOOP:
            {
                dwRet = pStreamContext->BreakLoop();
                break;
            }

        case WODM_SETPLAYBACKRATE:
            {
                WaveStreamContext *pWaveStream = (WaveStreamContext *)dwUser;
                dwRet = pWaveStream->SetRate(dwParam1);
                break;
            }

        case WODM_GETPLAYBACKRATE:
            {
                WaveStreamContext *pWaveStream = (WaveStreamContext *)dwUser;
                dwRet = pWaveStream->GetRate((DWORD *)dwParam1);
                break;
            }

        case MM_WOM_SETSECONDARYGAINCLASS:
            {
                if (pStreamContext)
                {
                    dwRet = pStreamContext->SetSecondaryGainClass(dwParam1);
                }
                else
                {
                    dwRet = MMSYSERR_INVALPARAM;
                }
                break;
            }

        case MM_WOM_SETSECONDARYGAINLIMIT:
            {
                DeviceContext *pDeviceContext;
                if (pStreamContext)
                {
                    pDeviceContext = pStreamContext->GetDeviceContext();
                }
                else
                {
                    pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                }
                dwRet = pDeviceContext->SetSecondaryGainLimit(dwParam1,dwParam2);
                break;
            }

        case MM_WOM_FORCESPEAKER:
            {
                if (pStreamContext)
                {
                    dwRet = pStreamContext->ForceSpeaker((BOOL)dwParam1);
                }
                else
                {
                    dwRet = g_pHWContext->ForceSpeaker((BOOL)dwParam1);
                }
                break;
            }

        case WODM_GETPROP:
            {
                // DEBUGMSG(ZONE_WODM, (TEXT("WODM_GETPROP\r\n")));

                PWAVEPROPINFO pPropInfo = (PWAVEPROPINFO) dwParam1;
                if (pStreamContext)
                {
                    dwRet = pStreamContext->GetProperty(pPropInfo);
                }
                else
                {
                    DeviceContext *pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                    dwRet = pDeviceContext->GetProperty(pPropInfo);
                }
                break;
            }

        case WIDM_GETPROP:
            {
                // DEBUGMSG(ZONE_WODM, (TEXT("WIDM_GETPROP\r\n")));

                PWAVEPROPINFO pPropInfo = (PWAVEPROPINFO) dwParam1;
                if (pStreamContext)
                {
                    dwRet = pStreamContext->GetProperty(pPropInfo);
                }
                else
                {
                    DeviceContext *pDeviceContext = g_pHWContext->GetInputDeviceContext(uDeviceId);
                    dwRet = pDeviceContext->GetProperty(pPropInfo);
                }
                break;
            }

        case WODM_SETPROP:
            {
                // DEBUGMSG(ZONE_WODM, (TEXT("WODM_SETPROP\r\n")));

                PWAVEPROPINFO pPropInfo = (PWAVEPROPINFO) dwParam1;
                if (pStreamContext)
                {
                    dwRet = pStreamContext->SetProperty(pPropInfo);
                }
                else
                {
                    DeviceContext *pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                    dwRet = pDeviceContext->SetProperty(pPropInfo);
                }
                break;
            }

        case WIDM_SETPROP:
            {
                // DEBUGMSG(ZONE_WODM, (TEXT("WIDM_SETPROP\r\n")));

                PWAVEPROPINFO pPropInfo = (PWAVEPROPINFO) dwParam1;
                if (pStreamContext)
                {
                    dwRet = pStreamContext->SetProperty(pPropInfo);
                }
                else
                {
                    DeviceContext *pDeviceContext = g_pHWContext->GetInputDeviceContext(uDeviceId);
                    dwRet = pDeviceContext->SetProperty(pPropInfo);
                }
                break;
            }

    // unsupported messages
        case WODM_GETPITCH:
        case WODM_SETPITCH:
        case WODM_PREPARE:
        case WODM_UNPREPARE:
        case WIDM_PREPARE:
        case WIDM_UNPREPARE:
            default:
            dwRet  = MMSYSERR_NOTSUPPORTED;
        }
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRet  = MMSYSERR_INVALPARAM;
    }

    g_pHWContext->Unlock();

    // Pass the return code back via pBufOut
    if (pdwResult)
    {
        *pdwResult = dwRet;
    }

    return(TRUE);
}

// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @func   BOOL | WAV_IOControl | Device IO control routine
//
//  @parm   DWORD | dwOpenData | Value returned from WAV_Open call
//
//  @parm   DWORD | dwCode |
//          IO control code for the function to be performed. WAV_IOControl only
//          supports one IOCTL value (IOCTL_WAV_MESSAGE)
//
//  @parm   PBYTE | pBufIn |
//          Pointer to the input parameter structure (<t MMDRV_MESSAGE_PARAMS>).
//
//  @parm   DWORD | dwLenIn |
//          Size in bytes of input parameter structure (sizeof(<t MMDRV_MESSAGE_PARAMS>)).
//
//  @parm   PBYTE | pBufOut | Pointer to the return value (DWORD).
//
//  @parm   DWORD | dwLenOut | Size of the return value variable (sizeof(DWORD)).
//
//  @parm   PDWORD | pdwActualOut | Unused
//
//  @rdesc  Returns TRUE for success, FALSE for failure
//
//  @xref   <t Wave Input Driver Messages> (WIDM_XXX) <nl>
//          <t Wave Output Driver Messages> (WODM_XXX)
//
// -----------------------------------------------------------------------------
extern "C" BOOL WAV_IOControl(PDWORD  pdwOpenData,
                   DWORD  dwCode,
                   PBYTE  pBufIn,
                   DWORD  dwLenIn,
                   PBYTE  pBufOut,
                   DWORD  dwLenOut,
                   PDWORD pdwActualOut)
{

    // check caller trust. if context hasn't been initialized, load from CeGetCallerTrust.
    if (*pdwOpenData != OEM_CERTIFY_TRUST) {
        /*if (OEM_CERTIFY_TRUST != (*pdwOpenData = CeGetCallerTrust())) {
            PRINTMSG(ZONE_WARN, (TEXT("WAV_IoControl: untrusted process\r\n")));
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }*/
    }

    switch (dwCode)
    {
    case IOCTL_MIX_MESSAGE:
        return HandleMixerMessage((PMMDRV_MESSAGE_PARAMS)pBufIn, (DWORD *)pBufOut);

    case IOCTL_WAV_MESSAGE:
        return HandleWaveMessage((PMMDRV_MESSAGE_PARAMS)pBufIn, (DWORD *)pBufOut);

    // Power management functions.
    case IOCTL_POWER_CAPABILITIES:
#if (_WINCEOSVER<700)
    case IOCTL_POWER_QUERY:
#endif		
    case IOCTL_POWER_SET:
    case IOCTL_POWER_GET:
        return g_pHWContext->PmControlMessage
                            (dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
    }

    return(FALSE);
}

