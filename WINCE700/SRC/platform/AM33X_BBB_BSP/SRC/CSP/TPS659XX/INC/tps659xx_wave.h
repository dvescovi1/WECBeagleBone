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
//
//  File: tps659xx_wave.h
//
#ifndef __TPS659XX_WAVE_H
#define __TPS659XX_WAVE_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//  Local enumerations

// defines triton audio modes
typedef enum {
    kAudioModeDisabled,
    kAudioModeSpeaker,
    kAudioModeMicrophone,
    kAudioModeStereoHeadset,
    kAudioModeSubMic,
    kAudioModeHeadset,
    kAudioModeAuxHeadset,
    kAudioModeMicHeadset,
    kAudioModeClockOnly,
    kAudioModeHwCodecDisable
} AudioMode_e;


// defines triton audio profiles
typedef enum {
    kAudioI2SProfile=0,
    kAudioTDMProfile,
    kAudioProfileUnknown
} AudioProfile_e;

// defines triton mode
typedef enum {
    kT2ModeSlave=0,
    kT2ModeMaster
} T2Mode_e;

// Prototypes
//
static void SetHwCodecMode_DisableCodecPower(HANDLE hTritonDriver);
static void SetHwCodecMode_EnableCodecPower(HANDLE hTritonDriver);
static void SetHwCodecMode_DisableCodec(HANDLE hTritonDriver);
static void SetHwCodecMode_EnableCodec(HANDLE hTritonDriver, UINT sampleRate);
static void SetHwCodecMode_DisableOutputRouting(HANDLE hTritonDriver);
static void SetHwCodecMode_DisableOutputGain(HANDLE hTritonDriver);
static void SetHwCodecMode_EnableOutputRouting(HANDLE hTritonDriver, AudioMode_e type);
static void SetHwCodecMode_EnableOutputGain(HANDLE hTritonDriver, AudioMode_e type);
static void SetHwCodecMode_DisableInputRouting(HANDLE hTritonDriver);
static void SetHwCodecMode_DisableInputGain(HANDLE hTritonDriver);
static void SetHwCodecMode_EnableInputRouting(HANDLE hTritonDriver, AudioMode_e type);
static void SetHwCodecMode_EnableInputGain(HANDLE hTritonDriver, AudioMode_e type);
extern void SetHwCodecMode_Disabled(HANDLE hTritonDriver);
extern void SetHwCodecMode_Microphone(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_Speaker(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_MicHeadset(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_Headset(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_AuxHeadset(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_StereoHeadset(HANDLE hTritonDriver, AudioProfile_e audioProfile);
extern void SetHwCodecMode_EnableT2AudioClkOnly(HANDLE hTritonDriver);
extern HANDLE OpenHwCodecHandle(HwCodecConfigInfo_t *pHwCodecConfigInfo);
extern BOOL SetHwCodecMode_Gain(HANDLE hTritonDriver, DWORD dwGain, DWORD *pdwGlobalGain);
extern void SetAudioI2SProfile(HANDLE s_hTritonDriver);
extern void SetAudioTDMProfile(HANDLE s_hTritonDriver);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

