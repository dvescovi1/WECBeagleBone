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
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//

#include "omap.h"
#include "ceddkex.h"
#include "debug.h"
#include "wavext.h"
/*#include <windows.h>

#include <memtxapi.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <tps659xx.h>
#include <gpio.h>
*/
#include "tps659xx.h"
#include "tps659xx_internals.h"
#include "tps659xx_wave.h"
#include "tps659xx_audio.h"
#include "debug.h"

#include <initguid.h>
#include "twl.h"
#include "triton.h"

//------------------------------------------------------------------------------
//  Local

// defines current triton audio mode
static AudioMode_e s_currentAudioMode = kAudioModeDisabled;

// HwCodec Config info
static HwCodecConfigInfo_t *s_pHwCodecConfigInfo = NULL;

// Prototype
//
#ifdef DUMP_TRITON_AUDIO_REGS
static void DumpTritonAudioRegs(HANDLE hTritonDriver);
#endif
//------------------------------------------------------------------------------
//  Functions


// -----------------------------------------------------------------------------
//
//  @doc    OpenHwCodecHandle
//
//  @func   HANDLE | OpenHwCodecHandle | Opens triton device handle.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
HANDLE
OpenHwCodecHandle(
    HwCodecConfigInfo_t *pHwCodecConfigInfo
    )
{
    HANDLE hDevice = NULL;

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    // Populate the global variable with T2 Hw codec configurations obtained
    // from registry
    //
    s_pHwCodecConfigInfo = pHwCodecConfigInfo;

    hDevice = TWLOpen();
    if (hDevice == NULL)
    {
        RETAILMSG(hDevice == INVALID_HANDLE_VALUE,
            (L"WAV:!ERROR Can't load Triton Driver err=0x%08X\r\n",
            GetLastError()
            ));
    }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));

    return (hDevice);
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableCodecPower
//
//  @func   VOID | SetHwCodecMode_DisableCodecPower | Disable codec power.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableCodecPower(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
             __FUNCTION__
             ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableCodecPower
//
//  @func   VOID | SetHwCodecMode_EnableCodecPower | enables codec power.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableCodecPower(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        regVal = (CODEC_MODE_CODEC_PDZ);
        TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableCodec
//
//  @func   VOID | SetHwCodecMode_DisableCodec | Diable codec.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableCodec(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        regVal = APLL_INFREQ(6);
        TWLWriteRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_VOICE_IF, &regVal, sizeof(regVal));

        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableCodec
//
//  @func   VOID | SetHwCodecMode_EnableCodec | enables codec.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableCodec(
    HANDLE hTritonDriver,
    UINT apllRate
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        // init
        regVal = APLL_INFREQ(6);
        TWLWriteRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));

        TWLReadRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        regVal |= (CODEC_MODE_CODEC_PDZ | CODEC_MODE_APLL_RATE(apllRate));
        TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));

        regVal = (APLL_EN | APLL_INFREQ(6));
        TWLWriteRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));

        regVal = AUDIO_IF_AIF_EN;
        TWLWriteRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_VOICE_IF, &regVal, sizeof(regVal));
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}


// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableOutputRouting
//
//  @func   VOID | SetHwCodecMode_DisableOutputRouting | Disable output routing.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableOutputRouting(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));
        regVal &= ~(OPTION_ARXL2_EN | OPTION_ARXR2_EN |
            OPTION_ARXR1_EN | OPTION_ARXL1_VRX_EN);
        TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_EAR_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_HS_SEL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_HS_GAIN_SET, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_PREDL_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_PREDR_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_PRECKL_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_PRECKR_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_HFR_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_AVDAC_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_RX_PATH_SEL, &regVal, sizeof(regVal));

        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableOutputGain
//
//  @func   VOID | SetHwCodecMode_DisableOutputGain | Disable output gain.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableOutputGain(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXL1PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXR1PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXL2PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXR2PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXL1_APGA_CTL, &regVal,
            sizeof(regVal)
            );

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXR1_APGA_CTL, &regVal,
            sizeof(regVal)
            );

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXL2_APGA_CTL, &regVal,
            sizeof(regVal)
            );

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ARXR2_APGA_CTL, &regVal,
            sizeof(regVal)
            );
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableOutputRouting
//
//  @func   VOID | SetHwCodecMode_EnableOutputRouting | Enables output routing.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableOutputRouting(
    HANDLE hTritonDriver,
    AudioMode_e type
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        switch (type)
            {
            case kAudioModeSpeaker: // Handsfree Speaker Out
                regVal = (PREDL_OUTLOW_EN | PREDL_GAIN(2) | PREDL_AL2_EN);
                TWLWriteRegs(hTritonDriver, TWL_PREDL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (PREDR_OUTLOW_EN | PREDR_GAIN(2) | PREDR_AR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_PREDR_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = HFX_REF_EN;
                TWLWriteRegs(hTritonDriver, TWL_HFR_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |= HFX_REF_RAMP;
                TWLWriteRegs(hTritonDriver, TWL_HFR_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |= (HFX_REF_LOOP | HFX_REF_HB);
                TWLWriteRegs(hTritonDriver, TWL_HFR_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |=  HFX_INPUT_SEL(2);
                TWLWriteRegs(hTritonDriver, TWL_HFR_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = HFX_REF_EN;
                TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |= HFX_REF_RAMP;
                TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |= HFX_REF_LOOP;
                TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |= HFX_REF_HB;
                TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal |=  HFX_INPUT_SEL(2);
                TWLWriteRegs(hTritonDriver, TWL_HFL_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (AVDAC_CTL_ADACL2_EN | AVDAC_CTL_ADACR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_AVDAC_CTL, &regVal,
                    sizeof(regVal)
                    );

                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= (OPTION_ARXL2_EN | OPTION_ARXR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeStereoHeadset: // Stereo Headset
                regVal = VMID_EN;
                TWLWriteRegs(hTritonDriver, TWL_HS_POPN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (VMID_EN | RAMP_EN);
                TWLWriteRegs(hTritonDriver, TWL_HS_POPN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (HSOR_AR2_EN | HSOL_AL2_EN);
                TWLWriteRegs(hTritonDriver, TWL_HS_SEL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (HSR_GAIN(2) | HSL_GAIN(2));
                TWLWriteRegs(hTritonDriver, TWL_HS_GAIN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (AVDAC_CTL_ADACL2_EN | AVDAC_CTL_ADACR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_AVDAC_CTL, &regVal,
                    sizeof(regVal)
                    );

                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= (OPTION_ARXL2_EN | OPTION_ARXR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                     sizeof(regVal)
                     );

                break;

            case kAudioModeHeadset: // Mic Headset
                regVal = VMID_EN;
                TWLWriteRegs(hTritonDriver, TWL_HS_POPN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (VMID_EN | RAMP_EN);
                TWLWriteRegs(hTritonDriver, TWL_HS_POPN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (HSOR_AR2_EN | HSOL_AL2_EN);
                TWLWriteRegs(hTritonDriver, TWL_HS_SEL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (HSR_GAIN(2) | HSL_GAIN(2));
                TWLWriteRegs(hTritonDriver, TWL_HS_GAIN_SET, &regVal,
                    sizeof(regVal)
                    );

                regVal = (AVDAC_CTL_ADACL2_EN | AVDAC_CTL_ADACR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_AVDAC_CTL, &regVal,
                    sizeof(regVal)
                    );

                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= (OPTION_ARXL2_EN | OPTION_ARXR2_EN);
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );

                break;

            default:
                break;
            }
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableOutputGain
//
//  @func   VOID | SetHwCodecMode_EnableOutputGain | Enables output gain.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableOutputGain(
    HANDLE hTritonDriver,
    AudioMode_e type
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        switch (type)
            {
            case kAudioModeSpeaker: // Handsfree Speaker out

                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoSpeakerDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXL2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoSpeakerDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXR2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (UINT8) (APGA_CTL_PDZ | APGA_CTL_DA_EN |
                    APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoSpeakerAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXL2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (UINT8)(APGA_CTL_PDZ | APGA_CTL_DA_EN |
                    APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoSpeakerAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXR2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );
                break;

            case kAudioModeStereoHeadset: // Stereo Headset
                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoHeadsetDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXL2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoHeadsetDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXR2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (APGA_CTL_PDZ | APGA_CTL_DA_EN |
                (UINT8) APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoHeadsetAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXL2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (APGA_CTL_PDZ | APGA_CTL_DA_EN |
                (UINT8) APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutStereoHeadsetAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXR2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeHeadset: // Mic Headset
                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutHeadsetMicDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXL2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecOutHeadsetMicDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ARXR2PGA, &regVal,
                    sizeof(regVal)
                    );

                regVal = (APGA_CTL_PDZ | APGA_CTL_DA_EN |
                (UINT8) APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutHeadsetMicAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXL2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );

                regVal = (APGA_CTL_PDZ | APGA_CTL_DA_EN |
                (UINT8) APGA_CTL_GAIN_SET(s_pHwCodecConfigInfo->
                    dwHwCodecOutHeadsetMicAnalogGain)
                    );
                TWLWriteRegs(hTritonDriver, TWL_ARXR2_APGA_CTL, &regVal,
                    sizeof(regVal)
                    );

                break;

            default:
                break;
            }
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableInputRouting
//
//  @func   VOID | SetHwCodecMode_DisableInputRouting | Disables input routing.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableInputRouting(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));
        regVal &= ~(OPTION_ATXR2_VTXR_EN | OPTION_ATXL2_VTXL_EN |
            OPTION_ATXR1_EN | OPTION_ATXL1_EN
            );
        TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ANAMICL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ANAMICR, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_AVADC_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ADCMICSEL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_DIGMIXING, &regVal, sizeof(regVal));
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_DisableInputGain
//
//  @func   VOID | SetHwCodecMode_DisableInputGain | Disables input gain.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_DisableInputGain(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        regVal = ALC_WAIT(5); // default
        TWLWriteRegs(hTritonDriver, TWL_ALC_CTL, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ATXL1PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_ATXR1PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_AVTXL2PGA, &regVal, sizeof(regVal));

        regVal = 0;
        TWLWriteRegs(hTritonDriver, TWL_AVTXR2PGA, &regVal, sizeof(regVal));
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableInputRouting
//
//  @func   VOID | SetHwCodecMode_EnableInputRouting | Enables input routing.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableInputRouting(
    HANDLE hTritonDriver,
    AudioMode_e type
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        switch (type)
            {
            case kAudioModeMicrophone: // Main Mic
                /* The active channel is TxPath channel 1 - left channel */
                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= OPTION_ATXL1_EN;
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );

                /* Set the MICBIAS for analog MIC and enable it */
                regVal = MICBIAS1_EN;
                TWLWriteRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal,
                    sizeof(regVal)
                    );

                /* Enable the main MIC and select ADCL input */
                regVal = (ANAMICL_MICAMPL_EN | ANAMICL_MAINMIC_EN);
                TWLWriteRegs(hTritonDriver, TWL_ANAMICL, &regVal,
                    sizeof(regVal)
                    );

                /* Enable Left ADC */
                regVal = AVADC_CTL_ADCL_EN;
                TWLWriteRegs(hTritonDriver, TWL_AVADC_CTL, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeSubMic: // Sub Mic
                /* The active channel is TxPath channel 1 - right channel */
                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= OPTION_ATXR1_EN;
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );

                /* Set the MICBIAS for analog MIC and enable it */
                regVal = MICBIAS2_EN;
                TWLWriteRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal,
                    sizeof(regVal)
                    );

                /* Enable the sub MIC and select ADCL input */
                regVal = (ANAMICR_MICAMPR_EN | ANAMICR_SUBMIC_EN);
                TWLWriteRegs(hTritonDriver, TWL_ANAMICR, &regVal,
                    sizeof(regVal)
                    );

                /* Enable Right ADC */
                regVal = AVADC_CTL_ADCR_EN;
                TWLWriteRegs(hTritonDriver, TWL_AVADC_CTL, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeMicHeadset: // Mic Headset
                /* The active channel is TxPath channel 1 - left channel */
                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );
                regVal |= OPTION_ATXL1_EN;
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal,
                    sizeof(regVal)
                    );

                /* Set the HSMICBIAS for analog MIC and enable it */
                regVal = HSMICBIAS_EN;
                TWLWriteRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal,
                    sizeof(regVal)
                    );

                /* Enable the HS MIC and select ADCL input */
                regVal = (ANAMICL_MICAMPL_EN | ANAMICL_HSMIC_EN);
                TWLWriteRegs(hTritonDriver, TWL_ANAMICL, &regVal,
                    sizeof(regVal)
                    );

                /* Enable Left ADC */
                regVal = AVADC_CTL_ADCL_EN;
                TWLWriteRegs(hTritonDriver, TWL_AVADC_CTL, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeAuxHeadset: // Aux In, Headset Out
                /* The active channel is TxPath channel 1 - left and right channel */
                TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));
                regVal |= OPTION_ATXL1_EN | OPTION_ATXR1_EN;
                TWLWriteRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));

                /* Set the HSMICBIAS for analog MIC and enable it */
                //regVal = HSMICBIAS_EN;
                //TWLWriteRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal, sizeof(regVal));

                /* select the AUXL input, enable MICAMPL */
                //regVal = (ANAMICL_MICAMPL_EN | ANAMICL_HSMIC_EN);
                regVal = ANAMICL_AUXL_EN | ANAMICL_MICAMPL_EN;
                TWLWriteRegs(hTritonDriver, TWL_ANAMICL, &regVal, sizeof(regVal));

                /* select the AUXR input, enable MICAMPR */
                regVal = ANAMICR_AUXR_EN | ANAMICR_MICAMPR_EN;
                TWLWriteRegs(hTritonDriver, TWL_ANAMICR, &regVal, sizeof(regVal));
                
                /* Enable Left and Right ADC */
                regVal = AVADC_CTL_ADCL_EN | AVADC_CTL_ADCR_EN;
                TWLWriteRegs(hTritonDriver, TWL_AVADC_CTL, &regVal, sizeof(regVal));

                break;

            default:
                break;
            }
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableInputGain
//
//  @func   VOID | SetHwCodecMode_EnableInputGain | Enables input gain.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
SetHwCodecMode_EnableInputGain(
    HANDLE hTritonDriver,
    AudioMode_e type
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        switch (type)
            {
            case kAudioModeMicrophone: // Main Mic
                /* Configure gain on input */
                regVal = (UINT8)ANAMIC_MICAMPL_GAIN(s_pHwCodecConfigInfo->
                    dwHwCodecInMainMicAnalogGain
                    );
                TWLWriteRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal,
                    sizeof(regVal)
                    );

                /* Set Tx path volume control */
                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecInMainMicDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ATXL1PGA, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeSubMic: // Sub Mic
                /* Configure gain on input */
                regVal = (UINT8)ANAMIC_MICAMPR_GAIN(s_pHwCodecConfigInfo->
                    dwHwCodecInSubMicAnalogGain
                    );
                TWLWriteRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal,
                    sizeof(regVal)
                    );

                /* Set Tx path volume control */
                regVal = (UINT8)s_pHwCodecConfigInfo->
                    dwHwCodecInSubMicDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ATXR1PGA, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeMicHeadset: // Mic Headset
                /* Configure gain on input */
                regVal = (UINT8)ANAMIC_MICAMPL_GAIN(s_pHwCodecConfigInfo->
                    dwHwCodecInHeadsetMicAnalogGain
                    );
                TWLWriteRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal,
                    sizeof(regVal)
                    );

                /* Set Tx path volume control */
                regVal =(UINT8) s_pHwCodecConfigInfo->
                    dwHwCodecInHeadsetMicDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ATXL1PGA, &regVal,
                    sizeof(regVal)
                    );

                break;

            case kAudioModeAuxHeadset: // Aux in, Headset out
                /* Configure gain on input */
                regVal = (UINT8) ( ANAMIC_MICAMPL_GAIN(s_pHwCodecConfigInfo->dwHwCodecInHeadsetAuxAnalogGain) | 
                         ANAMIC_MICAMPR_GAIN(s_pHwCodecConfigInfo->dwHwCodecInHeadsetAuxAnalogGain) );
                TWLWriteRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal, sizeof(regVal));

                /* Set Tx Left path volume control */
                regVal = (UINT8) s_pHwCodecConfigInfo->dwHwCodecInHeadsetAuxDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ATXL1PGA, &regVal, sizeof(regVal));

                /* Set Tx Right path volume control */
                regVal = (UINT8) s_pHwCodecConfigInfo->dwHwCodecInHeadsetAuxDigitalGain;
                TWLWriteRegs(hTritonDriver, TWL_ATXR1PGA, &regVal, sizeof(regVal));

                break;

            default:
                break;
            }
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetAudioI2SProfile
//
//  @func   VOID | SetAudioI2SProfile | Enables I2S profile mode.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetAudioI2SProfile(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
    {
        UINT8 regVal;

        TWLReadRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        regVal &= ~(CODEC_MODE_CODEC_OPT_MODE);
        TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));

        TWLReadRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));
        regVal &= ~( AUDIO_IF_AIF_FORMAT(3) | AUDIO_IF_DATA_WIDTH(0));
        TWLWriteRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));

        // Side tone gain
        //
        regVal = 0x0;      // Mute
        TWLWriteRegs(hTritonDriver, TWL_VSTPGA, &regVal, sizeof(regVal));

    }
    else
    {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
    }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetAudioTDMProfile
//
//  @func   VOID | SetAudioTDMProfile | Enables TDM profile mode.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetAudioTDMProfile(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
    {
        UINT8 regVal;

        TWLReadRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        regVal |= CODEC_MODE_CODEC_OPT_MODE;
        TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));

        TWLReadRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));
        regVal |= (AUDIO_IF_DATA_WIDTH(0) | AUDIO_IF_AIF_FORMAT(3));
        TWLWriteRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));

        // Side tone gain
        //
        regVal = 0x29;      // -10db
        TWLWriteRegs(hTritonDriver, TWL_VSTPGA, &regVal, sizeof(regVal));

    }
    else
    {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
    }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}


// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_Disabled
//
//  @func   VOID | SetHwCodecMode_Disabled | Disables T2 audio.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_Disabled(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        SetHwCodecMode_DisableOutputRouting(hTritonDriver);
        SetHwCodecMode_DisableOutputGain(hTritonDriver);
        SetHwCodecMode_DisableInputRouting(hTritonDriver);
        SetHwCodecMode_DisableInputGain(hTritonDriver);
        SetHwCodecMode_DisableCodec(hTritonDriver);
        SetHwCodecMode_DisableCodecPower(hTritonDriver);

        s_currentAudioMode = kAudioModeDisabled;
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_EnableT2AudioClkOnly
//
//  @func   VOID | SetHwCodecMode_EnableT2AudioClkOnly | Enable T2 audio clock
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_EnableT2AudioClkOnly(
    HANDLE hTritonDriver
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            goto code_exit;
            }

        if (s_currentAudioMode != kAudioModeDisabled)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            UINT8 regVal;

            // Power on codec
            regVal = (CODEC_MODE_CODEC_PDZ);
            TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal,
                sizeof(regVal)
                );

            // init
            regVal = APLL_INFREQ(6);
            TWLWriteRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));

            // Power on codec; Fs = 44.1 KHz, o/p 256 * fs KHz
            TWLReadRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
            regVal |= (CODEC_MODE_CODEC_PDZ |
                CODEC_MODE_APLL_RATE(kSampleRate_44_1KHz));
            TWLWriteRegs(hTritonDriver, TWL_CODEC_MODE, &regVal,
                sizeof(regVal)
                );

            // Enalbe APLL; Input clock frq - 26Mhz and APLL is enabled
            regVal = (APLL_EN | APLL_INFREQ(6));
            TWLWriteRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));

            // Enable CLK256FS
            regVal = AUDIO_IF_AIF_SLAVE_EN | AUDIO_IF_AIF_TRI_EN |
                AUDIO_IF_CLK256FS_EN | AUDIO_IF_AIF_EN;
            TWLWriteRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));

            }

        s_currentAudioMode = kAudioModeClockOnly;
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__)
            );
        }

code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_Microphone
//
//  @func   VOID | SetHwCodecMode_Microphone | Set Microphone path.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_Microphone(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (s_currentAudioMode == kAudioModeMicrophone)
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {
        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);

            // disable input if transitioning directly from different input
            SetHwCodecMode_DisableInputRouting(hTritonDriver);
            SetHwCodecMode_DisableInputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeSpeaker);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeSpeaker);

        // enable input
        SetHwCodecMode_EnableInputRouting(hTritonDriver, kAudioModeMicrophone);
        SetHwCodecMode_EnableInputGain(hTritonDriver, kAudioModeMicrophone);

        s_currentAudioMode = kAudioModeMicrophone;
        //RETAILMSG(1, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_Speaker
//
//  @func   VOID | SetHwCodecMode_Speaker | set speaker path.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_Speaker(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if ((s_currentAudioMode == kAudioModeSpeaker) ||
        (s_currentAudioMode == kAudioModeMicrophone))
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {
        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeSpeaker);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeSpeaker);

        s_currentAudioMode = kAudioModeSpeaker;
        //RETAILMSG(1, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_MicHeadset
//
//  @func   VOID | SetHwCodecMode_MicHeadset | sets headset microphone path.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_MicHeadset(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (s_currentAudioMode == kAudioModeMicHeadset)
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {
        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);

            // disable input if transitioning directly from different input
            SetHwCodecMode_DisableInputRouting(hTritonDriver);
            SetHwCodecMode_DisableInputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeHeadset);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeHeadset);

        // enable input
        SetHwCodecMode_EnableInputRouting(hTritonDriver, kAudioModeMicHeadset);
        SetHwCodecMode_EnableInputGain(hTritonDriver, kAudioModeMicHeadset);

        s_currentAudioMode = kAudioModeMicHeadset;
        //RETAILMSG(1, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_Headset
//
//  @func   VOID | SetHwCodecMode_Headset | sets headset path.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_Headset(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if ((s_currentAudioMode == kAudioModeHeadset) ||
        (s_currentAudioMode == kAudioModeMicHeadset))
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {
        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeHeadset);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeHeadset);

        s_currentAudioMode = kAudioModeHeadset;
        //RETAILMSG(ZONE_FUNCTION, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_AuxHeadset
//
//  @func   VOID | SetHwCodecMode_AuxHeadset | sets AUX in path, stereo headset out path.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_AuxHeadset(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (s_currentAudioMode == kAudioModeAuxHeadset)
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {

        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);

            // disable input if transitioning directly from different input
            SetHwCodecMode_DisableInputRouting(hTritonDriver);
            SetHwCodecMode_DisableInputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeStereoHeadset);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeStereoHeadset);

        // enable input
        SetHwCodecMode_EnableInputRouting(hTritonDriver, kAudioModeAuxHeadset);
        SetHwCodecMode_EnableInputGain(hTritonDriver, kAudioModeAuxHeadset);

        s_currentAudioMode = kAudioModeAuxHeadset;
        //RETAILMSG(1, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecMode_StereoHeadset
//
//  @func   VOID | SetHwCodecMode_StereoHeadset | sets stereo headset path.
//          Note: Same levels are used for headset as for AuxHeadset mode.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
void
SetHwCodecMode_StereoHeadset(
    HANDLE hTritonDriver,
    AudioProfile_e audioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    if (s_currentAudioMode == kAudioModeStereoHeadset)
        {
        goto code_exit;
        }

    if (hTritonDriver)
        {

        if (s_currentAudioMode == kAudioModeClockOnly)
            {
            SetHwCodecMode_Disabled(hTritonDriver);
            }

        if (s_currentAudioMode == kAudioModeDisabled)
            {
            // init
            SetHwCodecMode_EnableCodecPower(hTritonDriver);
            SetHwCodecMode_EnableCodec(hTritonDriver, kSampleRate_44_1KHz);

            if (audioProfile == kAudioI2SProfile)
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            else if (audioProfile == kAudioTDMProfile)
                {
                SetAudioTDMProfile(hTritonDriver);
                }
            else
                {
                SetAudioI2SProfile(hTritonDriver);
                }
            }
        else
            {
            // disable output if transitioning directly from different output
            SetHwCodecMode_DisableOutputRouting(hTritonDriver);
            SetHwCodecMode_DisableOutputGain(hTritonDriver);

            // disable input if transitioning directly from different input
            SetHwCodecMode_DisableInputRouting(hTritonDriver);
            SetHwCodecMode_DisableInputGain(hTritonDriver);
            }

        // enable output
        SetHwCodecMode_EnableOutputRouting(hTritonDriver, kAudioModeStereoHeadset);
        SetHwCodecMode_EnableOutputGain(hTritonDriver, kAudioModeStereoHeadset);

        s_currentAudioMode = kAudioModeStereoHeadset;
        //RETAILMSG(1, (L"WAV:%S\r\n",__FUNCTION__));
#ifdef DUMP_TRITON_AUDIO_REGS
        DumpTritonAudioRegs(hTritonDriver);
#endif
        }
    else
        {
        DEBUGMSG(ZONE_TPS659XX, (L"WAV:%S - hTritonDriver is NULL!!!\r\n",
            __FUNCTION__
            ));
        }
code_exit:
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
}

// -----------------------------------------------------------------------------
//
//  @doc    SetHwCodecGain
//
//  @func   BOOL | SetHwCodecGain | sets hradware codec gain.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
BOOL
SetHwCodecMode_Gain(
    HANDLE hTritonDriver,
    DWORD dwGain,
    DWORD *pdwGlobalGain)
{
    BOOL returnValue = TRUE;

    UNREFERENCED_PARAMETER(hTritonDriver);

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+%S\r\n",__FUNCTION__));

    // if gain levels are identical then nothing needs to be done
    if (*pdwGlobalGain == dwGain)
        {
        returnValue = FALSE;
        }
    else
        {
        *pdwGlobalGain = dwGain;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-%S\r\n",__FUNCTION__));
    return returnValue;
}
#ifdef DUMP_TRITON_AUDIO_REGS
// -----------------------------------------------------------------------------
//
//  @doc    DumpTritonAudioRegs
//
//  @func   VOID | DumpTritonAudioRegs | Dumps T2 Registers.
//
//  @rdesc  none
//
// -----------------------------------------------------------------------------
static void
DumpTritonAudioRegs(
    HANDLE hTritonDriver
    )
{
    RETAILMSG(1, (L"WAV:+%S\r\n",__FUNCTION__));

    if (hTritonDriver)
        {
        UINT8 regVal;

        TWLReadRegs(hTritonDriver, TWL_CODEC_MODE, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_CODEC_MODE = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_OPTION, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_OPTION = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_MICBIAS_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_MICBIAS_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ANAMICL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ANAMICL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ANAMICR, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ANAMICR = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_AVADC_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_AVADC_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ADCMICSEL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ADCMICSEL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DIGMIXING, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DIGMIXING = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ATXL1PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ATXL1PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ATXR1PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ATXR1PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_AVTXL2PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_AVTXL2PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_AVTXR2PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_AVTXR2PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_AUDIO_IF, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_AUDIO_IF = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VOICE_IF, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VOICE_IF = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXR1PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXR1PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXL1PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXL1PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXR2PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXR2PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXL2PGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXL2PGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VRXPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VRXPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VSTPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VSTPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VRX2ARXPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VRX2ARXPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_AVDAC_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_AVDAC_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARX2VTXPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARX2VTXPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXL1_APGA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXL1_APGA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXR1_APGA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXR1_APGA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXL2_APGA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXL2_APGA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ARXR2_APGA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ARXR2_APGA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ATX2ARXPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ATX2ARXPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_BT_IF, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_BT_IF = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_BTPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_BTPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_BTSTPGA, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_BTSTPGA = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_EAR_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_EAR_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_HS_SEL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_HS_SEL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_HS_GAIN_SET, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_HS_GAIN_SET = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_HS_POPN_SET, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_HS_POPN_SET = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_PREDL_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_PREDL_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_PREDR_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_PREDR_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_PRECKL_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_PRECKL_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_PRECKR_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_PRECKR_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_HFL_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_HFL_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_HFR_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_HFR_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ALC_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ALC_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ALC_SET1, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ALC_SET1 = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ALC_SET2, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ALC_SET2 = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_BOOST_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_BOOST_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_SOFTVOL_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_SOFTVOL_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_FREQSEL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_FREQSEL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_TONEXT1H, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_TONEXT1H = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_TONEXT1L, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_TONEXT1L = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_TONEXT2H, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_TONEXT2H = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_TONEXT2L, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_TONEXT2L = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_TONOFF, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_TONOFF = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_WANONOFF, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_WANONOFF = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_I2S_RX_SCRAMBLE_H, &regVal,
            sizeof(regVal)
            );
        RETAILMSG(1, (L"WAV: TWL_I2S_RX_SCRAMBLE_H = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_I2S_RX_SCRAMBLE_M, &regVal,
            sizeof(regVal)
            );
        RETAILMSG(1, (L"WAV: TWL_I2S_RX_SCRAMBLE_M = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_I2S_RX_SCRAMBLE_L, &regVal,
            sizeof(regVal)
            );
        RETAILMSG(1, (L"WAV: TWL_I2S_RX_SCRAMBLE_L = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_APLL_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_APLL_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_PGA_CTL2, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_PGA_CTL2 = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_DTMF_PGA_CTL1, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_DTMF_PGA_CTL1 = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_MISC_SET_1, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_MISC_SET_1 = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_PCMBTMUX, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_PCMBTMUX = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_RX_PATH_SEL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_RX_PATH_SEL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VDL_APGA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VDL_APGA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VIBRA_CTL, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VIBRA_CTL = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VIBRA_SET, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VIBRA_SET = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_VIBRA_PWM_SET, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_VIBRA_PWM_SET = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_ANAMIC_GAIN, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_ANAMIC_GAIN = 0x%x\r\n", regVal));

        TWLReadRegs(hTritonDriver, TWL_MISC_SET_2, &regVal, sizeof(regVal));
        RETAILMSG(1, (L"WAV: TWL_MISC_SET_2 = 0x%x\r\n", regVal));

        }
    RETAILMSG(1, (L"WAV:-%S\r\n",__FUNCTION__));
}

#endif