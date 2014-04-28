//
// Copyright (c) MPC-Data Limited 2007.  All rights reserved.
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
    |   (1 << 6)    // PDD
    |   (1 << 13)   // Function
};
#endif

BOOL CALLBACK DllMain(HANDLE hDLL,
                      DWORD dwReason,
                      LPVOID lpvReserved)
{
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

//+++DBG
#define AMSG_SZ 120
UINT aMsg[AMSG_SZ];
UINT next_msg = 0;
void WaveShowDbgDmaBuf(UINT32 addr, UINT32 len)
{
#if 1
    UINT i, j, r;
    UINT32 row_start_addr;
    UINT32 *pVal;
    UINT32 n_per_row = 8;
#endif

    RETAILMSG(1, (L"WaveShowDbgDmaBuf: 0x%08X %d \r\n", addr, len));

#if 1
    r = len/n_per_row;
    row_start_addr = addr;
    for(i=0; i<r; i++)
    {
        pVal = (UINT32 *)row_start_addr;
        RETAILMSG(1, 
            (L"%08X: %08X %08X %08X %08X %08X %08X %08X %08X\r\n", 
            row_start_addr,
             *(pVal+0), *(pVal+1),  *(pVal+2),  *(pVal+3)
             , *(pVal+4), *(pVal+5),  *(pVal+6),  *(pVal+7)
/*
             , *(pVal+8), *(pVal+9),  *(pVal+10), *(pVal+11)
             , *(pVal+12), *(pVal+13), *(pVal+14), *(pVal+15) 
*/
        ));

        row_start_addr = row_start_addr + n_per_row*4;
    }
#endif
}

void WaveShowReceivedWaveMessages(void)
{
    UINT i, j, r;

    RETAILMSG(1, (L"See public\\common\\oak\\inc\\mmddk.h for msg ids. \r\n"));
    r = AMSG_SZ/10;
    for(i=0; i<r; i++)
    {
        j=10*i;
        RETAILMSG(1, (L"%03d: %02d %02d %02d %02d %02d   %02d %02d %02d %02d %02d \r\n", i*10,
            aMsg[j+0], aMsg[j+1], aMsg[j+2], aMsg[j+3], aMsg[j+4],
            aMsg[j+5], aMsg[j+6], aMsg[j+7], aMsg[j+8], aMsg[j+9]
        ));
    }
    RETAILMSG(1, (L"next msg idx %d\r\n", next_msg));
}

#if PROFILE_ENABLE
#define TIME_ARRAY_SIZE    800
extern DWORD           dma_next_time;
extern LARGE_INTEGER   dmaIntrTimeHighRes[TIME_ARRAY_SIZE];
extern LARGE_INTEGER   StartAudioOutputTimeHighRes[2];
extern ULONG           StartAudioOutputBytesTransferred[2];
extern ULONG           dmaIntrBytesTransferred[TIME_ARRAY_SIZE];

extern DWORD mcasp_next_time;
extern LARGE_INTEGER   mcaspIntrTimeHighRes[TIME_ARRAY_SIZE];

void WaveShowIntrTime(UINT32 action)
{
    UINT32 i, dur=0, total = 0, total_dur = 0;
    LARGE_INTEGER nFreq;

    QueryPerformanceFrequency(&nFreq);

    RETAILMSG(1, (L"\r\n"));
    RETAILMSG(1, (L"StartAudioOutputBytesTransferred: %d, %d\r\n",
        StartAudioOutputBytesTransferred[0],
        StartAudioOutputBytesTransferred[1]));
    RETAILMSG(1, (L"dma interrupt timestamps: \r\n"));
    for(i=0; i<dma_next_time; i++)
    {
       
        if(i == 0)
        {
			dur = ((UINT32)(dmaIntrTimeHighRes[i].QuadPart - StartAudioOutputTimeHighRes[0].QuadPart)) * 1000 / (nFreq.LowPart / 1000);
        }
        else
        {
			dur = ((UINT32)(dmaIntrTimeHighRes[i].QuadPart - dmaIntrTimeHighRes[i-1].QuadPart)) * 1000 / (nFreq.LowPart / 1000);
        }

        total += dmaIntrBytesTransferred[i];
        total_dur += dur;

        RETAILMSG(1, (L"%d: %08X %08X (%dus) %d\r\n",
            i, dmaIntrTimeHighRes[i].HighPart, dmaIntrTimeHighRes[i].LowPart, 
            dur, dmaIntrBytesTransferred[i]));
    }

    if(dma_next_time > 0)
    {
       RETAILMSG(1, (L"Total bytes transferred: %d\r\n", 
           total+StartAudioOutputBytesTransferred[0]+StartAudioOutputBytesTransferred[1]));

       RETAILMSG(1, (L"Total duration: %d us\r\n", total_dur));
    }

    RETAILMSG(1, (L"\r\n"));
    RETAILMSG(1, (L"underrun err interrupt timestamps: \r\n"));
    for(i=0; i<mcasp_next_time; i++)
        RETAILMSG(1, (L"%d: %08X %08X\r\n",i, mcaspIntrTimeHighRes[i].HighPart, mcaspIntrTimeHighRes[i].LowPart));

    if(action == 0)
    {
        dma_next_time=0;
        mcasp_next_time=0;
        StartAudioOutputBytesTransferred[0] = 0;
        StartAudioOutputBytesTransferred[1] = 0;
        StartAudioOutputTimeHighRes[0].LowPart = 0;
        StartAudioOutputTimeHighRes[0].HighPart = 0;
        StartAudioOutputTimeHighRes[1].LowPart = 0;
        StartAudioOutputTimeHighRes[1].HighPart = 0;

        RETAILMSG(1, (L"reset interrupt time arrays: dma_next_time=%d mcasp_next_time=%d\r\n",
            action, dma_next_time,mcasp_next_time ));
        RETAILMSG(1, (L"reset StartAudioOutputBytesTransferred[]: %d, %d\r\n",
            StartAudioOutputBytesTransferred[0],
            StartAudioOutputBytesTransferred[1]));
    }
}
#endif

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

//    RETAILMSG(1, (L"=> WaveMsg %d\r\n", uMsg));
//+++DBG
    aMsg[next_msg] = uMsg;
    if(next_msg >= AMSG_SZ)
        next_msg = 0;
    else
        ++next_msg;

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
            dwRet  = g_pHWContext->GetNumInputDevices();
            break;
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
                PRINTMSG(ZONE_FUNCTION, (TEXT("WAV2: WIDM_GETDEVCAPS\r\n")));

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
//                UINT NumDevs = g_pHWContext->GetNumOutputDevices();

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
                if (uDeviceId >= 0 && uDeviceId < g_pHWContext->GetNumOutputDevices())
                {
                    if (g_pHWContext->IsReady())
                    {
                        DeviceContext *pDeviceContext = g_pHWContext->GetOutputDeviceContext(uDeviceId);
                        dwRet = pDeviceContext->OpenStream((LPWAVEOPENDESC)dwParam1, dwParam2, (StreamContext **)dwUser);
                    }
                    else
                    {
                        dwRet = MMSYSERR_ERROR;
                    }
                }
                else
                {
                    dwRet = MMSYSERR_INVALPARAM;
                }
                break;
            }

        case WIDM_OPEN:
            {
                PRINTMSG(ZONE_FUNCTION, (TEXT("WAV2: WIDM_OPEN\r\n")));
                if (uDeviceId >= 0 && uDeviceId < g_pHWContext->GetNumInputDevices())
                {
                    if (g_pHWContext->IsReady())
                    {
                        DeviceContext *pDeviceContext = g_pHWContext->GetInputDeviceContext(uDeviceId);
                        dwRet = pDeviceContext->OpenStream((LPWAVEOPENDESC)dwParam1, dwParam2, (StreamContext **)dwUser);
                    }
                    else
                    {
                        dwRet = MMSYSERR_ERROR;
                    }
                }
                else
                {
                    dwRet = MMSYSERR_INVALPARAM;
                }
                break;
            }
            break;

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
                    *pdwGain = g_pHWContext->GetOutputGain(uDeviceId);                    
                }

                dwRet = MMSYSERR_NOERROR;
                break;
            }

        case WODM_SETVOLUME:
            {
                DWORD dwGain = dwParam1;
                
                if (pStreamContext)
                {
                    DEBUGMSG(ZONE_FUNCTION, (TEXT("WAV2: HandleWaveMessage: Setting output volume to 0x%x\r\n"), dwGain));
                    dwRet = pStreamContext->SetGain(dwGain);
                }
                else
                {
                    DEBUGMSG (TRUE, (TEXT("HandleWaveMessage: Setting output volume to 0x%x for device %d\r\n"), dwGain, uDeviceId));
                    dwRet = g_pHWContext->SetOutputGain(uDeviceId, dwGain);
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
                PRINTMSG(ZONE_FUNCTION, (TEXT("WAV2: WIDM_GETPROP\r\n")));

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
                PRINTMSG(ZONE_FUNCTION,(TEXT("WAV2: WIDM_SETPROP\r\n")));

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
            break;
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
    __except (EXCEPTION_EXECUTE_HANDLER)
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
        /* WEC7 does not support CeGetCallerTrust and hence commented.
        if (OEM_CERTIFY_TRUST != (*pdwOpenData = CeGetCallerTrust())) {
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
    case IOCTL_POWER_QUERY:
    case IOCTL_POWER_SET:
    case IOCTL_POWER_GET:
        return g_pHWContext->PmControlMessage
                            (dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);

    case WAVIOCTL_SET_OUTPUT:
    case WAVIOCTL_GET_OUTPUT:
    case WAVIOCTL_SET_INPUT:
    case WAVIOCTL_GET_INPUT:
    case WAVIOCTL_GET_AIC_REG:
        if(g_pHWContext->GetCodec())
            return (g_pHWContext->GetCodec())->HandleMessage(dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        else
            DEBUGMSG( ZONE_FUNCTION,
            (L"WAV_IOControl: g_pHWContext->GetCodec() returned NULL (Codec not initialized ?)\r\n"));
        break;

    case WAVIOCTL_SET_INPUT_VOLUME:
        if(pBufIn == NULL)
            return FALSE;
        if(g_pHWContext->SetInputGain(*((DWORD*)pBufIn)) == MMSYSERR_NOERROR)
            return TRUE;

        else return FALSE;
 
    case WAVIOCTL_GET_INPUT_VOLUME:
        if(pBufOut == NULL)
            return FALSE;
        *((DWORD*)pBufOut) = g_pHWContext->GetInputGain();

        return TRUE;

    case WAVIOCTL_GET_MCASP_REG:
        if(pBufOut == NULL)
            break;

        *pBufOut = *pBufIn;
        *(pBufOut+1) = *(pBufIn+1);

        if (g_pHWContext)
        {
            g_pHWContext->GetMcaspReg((DWORD*)pBufOut);
        }
        return TRUE;

    case WAVIOCTL_SHOW_WDM:
        WaveShowReceivedWaveMessages();
        return TRUE;

    case WAVIOCTL_SHOW_DMA_BUF:
        WaveShowDbgDmaBuf(*((UINT32 *)pBufIn), *((UINT32 *)pBufOut));
        return TRUE;

#if PROFILE_ENABLE
    case WAVIOCTL_SHOW_INTR_TIME:
        WaveShowIntrTime(*((UINT32 *)pBufIn));
        return TRUE;
#endif
   }

    return(FALSE);
}

