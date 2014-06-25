// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//------------------------------------------------------------------------------
//
//  File:  wavext.h
//
#ifndef __WAVEXT_H
#define __WAVEXT_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Extensions to Wave driver 

#define FILE_DEVICE_AXVOICE         0x368

#define IOCTL_CMSI_AUDIO_INFO_SET   \
    CTL_CODE(FILE_DEVICE_AXVOICE,   5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CMSI_AUDIO_INFO_GET   \
    CTL_CODE(FILE_DEVICE_AXVOICE,   6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GSM_CALL_ACTIVE       \
    CTL_CODE(FILE_DEVICE_AXVOICE,   7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GSM_CALL_INACTIVE     \
    CTL_CODE(FILE_DEVICE_AXVOICE,   8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_HEADSET        \
    CTL_CODE(FILE_DEVICE_AXVOICE,  10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TOGGLE_EXT_SPEAKER    \
    CTL_CODE(FILE_DEVICE_AXVOICE,  11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_BT_HEADSET     \
    CTL_CODE(FILE_DEVICE_AXVOICE,  12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_START_AMR_CAPTURE     \
    CTL_CODE(FILE_DEVICE_AXVOICE,  13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_AMR_CAPTURE      \
    CTL_CODE(FILE_DEVICE_AXVOICE,   14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_INFO_SET      \
    CTL_CODE(FILE_DEVICE_AXVOICE,   20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_INFO_GET      \
    CTL_CODE(FILE_DEVICE_AXVOICE,   21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GSM_RESTARTED         \
    CTL_CODE(FILE_DEVICE_AXVOICE,  22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_HDMI         \
    CTL_CODE(FILE_DEVICE_AXVOICE,  23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NOTIFY_AUXHEADSET     \
    CTL_CODE(FILE_DEVICE_AXVOICE,  24, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------
// CMSI audio settings

typedef struct __CMSI_AUDIO_SETTING {
    BYTE fUpLink;       // true = uplink
    BYTE fMute;         // true = mute
    BYTE bVolume;       // volume (0-128)
} CMSI_AUDIO_SETTING, *PCMSI_AUDIO_SETTING;
 
//------------------------------------------------------------------------------
// EAC audio settings

typedef struct __EAC_AUDIO_SETTING {
    // EAC control bits            
    WORD    S;      // sidetone attenuation
    BOOL    K[12];  // switch Kn
    WORD    DMAVOL; // DMA capture volume
    WORD    M[3];   // Mixer n, att A,B
    // modem control bits

} EAC_AUDIO_SETTING, *PEAC_AUDIO_SETTING;

//------------------------------------------------------------------------------
// T2 Hardware audio codec config settings structure

typedef struct {
    DWORD    dwHwCodecInMainMicDigitalGain;
    DWORD    dwHwCodecInSubMicDigitalGain;
    DWORD    dwHwCodecInHeadsetMicDigitalGain;
    DWORD    dwHwCodecInMainMicAnalogGain;
    DWORD    dwHwCodecInSubMicAnalogGain;
    DWORD    dwHwCodecInHeadsetMicAnalogGain;
    DWORD    dwHwCodecOutStereoSpeakerDigitalGain;
    DWORD    dwHwCodecOutStereoHeadsetDigitalGain;
    DWORD    dwHwCodecOutHeadsetMicDigitalGain;
    DWORD    dwHwCodecOutStereoSpeakerAnalogGain;
    DWORD    dwHwCodecOutStereoHeadsetAnalogGain;
    DWORD    dwHwCodecOutHeadsetMicAnalogGain;
    DWORD    dwHwCodecInHeadsetAuxDigitalGain;
    DWORD    dwHwCodecInHeadsetAuxAnalogGain;
} HwCodecConfigInfo_t;

//------------------------------------------------------------------------------
// IOCTL_NOTIFY_BT_HEADSET defines

#define BT_AUDIO_NONE       0x00000000      // No audio routed to BT device
#define BT_AUDIO_SYSTEM     0x00000001      // System audio routed to BT device
#define BT_AUDIO_MODEM      0x00000002      // Modem audio routed to/from BT device

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
