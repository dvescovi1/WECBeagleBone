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
//  File: tps659xx_audio.h
//
#ifndef __TPS659XX_AUDIO_H
#define __TPS659XX_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

// defines triton audio modes
typedef enum {
    kSampleRate_8KHz = 0x0,
    kSampleRate_11KHz = 0x1,
    kSampleRate_12KHz = 0x2,
    kSampleRate_16KHz = 0x4,
    kSampleRate_22KHz = 0x5,
    kSampleRate_24KHz = 0x6,
    kSampleRate_32KHz = 0x8,
    kSampleRate_44_1KHz = 0x9,
    kSampleRate_48KHz = 0xA,
    kSampleRate_96KHz = 0xE
} AudioSampleRate_e;

//------------------------------------------------------------------------------

// TWL_CODEC_MODE
#define CODEC_MODE_APLL_RATE(rate)                      ((rate) << 4)
#define CODEC_MODE_SEL_16K                              (1 << 3)
#define CODEC_MODE_CODEC_PDZ                            (1 << 1)
#define CODEC_MODE_CODEC_OPT_MODE                       (1 << 0)

// TWL_OPTION
#define OPTION_ARXR2_EN                                 (1 << 7)
#define OPTION_ARXL2_EN                                 (1 << 6)
#define OPTION_ARXR1_EN                                 (1 << 5)
#define OPTION_ARXL1_VRX_EN                             (1 << 4)
#define OPTION_ATXR2_VTXR_EN                            (1 << 3)
#define OPTION_ATXL2_VTXL_EN                            (1 << 2)
#define OPTION_ATXR1_EN                                 (1 << 1)
#define OPTION_ATXL1_EN                                 (1 << 0)

// TWL_MICBIAS_CTL
#define MICBIAS2_CTL                                    (1 << 6)
#define MICBIAS1_CTL                                    (1 << 5)
#define HSMICBIAS_EN                                    (1 << 2)
#define MICBIAS2_EN                                     (1 << 1)
#define MICBIAS1_EN                                     (1 << 0)

// TWL_ANAMICL
#define ANAMICL_CNCL_OFFSET_START                       (1 << 7)
#define ANAMICL_OFFSET_CNCL_SEL(chnl)                   ((chnl) << 5)
#define ANAMICL_MICAMPL_EN                              (1 << 4)
#define ANAMICL_CKMIC_EN                                (1 << 3)
#define ANAMICL_AUXL_EN                                 (1 << 2)
#define ANAMICL_HSMIC_EN                                (1 << 1)
#define ANAMICL_MAINMIC_EN                              (1 << 0)

// TWL_ANAMICR
#define ANAMICR_MICAMPR_EN                              (1 << 4)
#define ANAMICR_AUXR_EN                                 (1 << 2)
#define ANAMICR_SUBMIC_EN                               (1 << 0)

// TWL_AVADC_CTL
#define AVADC_CTL_ADCL_EN                               (1 << 3)
#define AVADC_CTL_AVADC_CLK_PRIORITY                    (1 << 2)
#define AVADC_CTL_ADCR_EN                               (1 << 1)

// TWL_ADCMICSEL
#define DIGMIC1_EN                                      (1 << 3)
#define TX2IN_SEL                                       (1 << 2)
#define DIGMIC0_EN                                      (1 << 1)
#define TX1IN_SEL                                       (1 << 0)

// TWL_AUDIO_IF
#define AUDIO_IF_AIF_SLAVE_EN                           (1 << 7)
#define AUDIO_IF_DATA_WIDTH(width)                      ((width) << 5)
#define AUDIO_IF_AIF_FORMAT(fmt)                        ((fmt) << 3)
#define AUDIO_IF_AIF_TRI_EN                             (1 << 2)
#define AUDIO_IF_CLK256FS_EN                            (1 << 1)
#define AUDIO_IF_AIF_EN                                 (1 << 0)

// TWL_VOICE_IF
#define VOICE_IF_VIF_SLAVE_EN                           (1 << 7)
#define VOICE_IF_VIF_DIN_EN                             (1 << 6)
#define VOICE_IF_VIF_DOUT_EN                            (1 << 5)
#define VOICE_IF_VIF_SWAP                               (1 << 4)
#define VOICE_IF_VIF_FORMAT                             (1 << 3)
#define VOICE_IF_VIF_TRI_EN                             (1 << 2)
#define VOICE_IF_VIF_SUB_EN                             (1 << 1)
#define VOICE_IF_VIF_EN                                 (1 << 0)

// TWL_AVDAC_CTL
#define AVDAC_CTL_VDAC_EN                               (1 << 4)
#define AVDAC_CTL_ADACL2_EN                             (1 << 3)
#define AVDAC_CTL_ADACR2_EN                             (1 << 2)
#define AVDAC_CTL_ADACL1_EN                             (1 << 1)
#define AVDAC_CTL_ADACR1_EN                             (1 << 0)

// TWL_ARXL1_APGA_CTL, TWL_ARXR1_APGA_CTL,
// TWL_ARXL2_APGA_CTL, TWL_ARXR2_APGA_CTL
#define APGA_CTL_GAIN_SET(gain)                         ((gain) << 3)
#define APGA_CTL_FM_EN                                  (1 << 2)
#define APGA_CTL_DA_EN                                  (1 << 1)
#define APGA_CTL_PDZ                                    (1 << 0)

// TWL_EAR_CTL
#define EAR_OUTLOW_EN                                   (1 << 7)
#define EAR_GAIN(gain)                                  ((gain) << 4)
#define EAR_AR1_EN                                      (1 << 3)
#define EAR_AL2_EN                                      (1 << 2)
#define EAR_AL1_EN                                      (1 << 1)
#define EAR_VOICE_EN                                    (1 << 0)

// TWL_HS_SEL
#define HSR_INV_EN                                      (1 << 7)
#define HS_OUTLOW_EN                                    (1 << 6)
#define HSOR_AR2_EN                                     (1 << 5)
#define HSOR_AR1_EN                                     (1 << 4)
#define HSOR_VOICE_EN                                   (1 << 3)
#define HSOL_AL2_EN                                     (1 << 2)
#define HSOL_AL1_EN                                     (1 << 1)
#define HSOL_VOICE_EN                                   (1 << 0)

// TWL_HS_GAIN_SET
#define HSR_GAIN(gain)                                  ((gain) << 2)
#define HSL_GAIN(gain)                                  ((gain) << 0)

// TWL_HS_POPN_SET
#define VMID_EN                                         (1 << 6)
#define EXTMUTE                                         (1 << 5)
#define RAMP_DELAY(delay)                               ((delay) << 2)
#define RAMP_EN                                         (1 << 1)

// TWL_PREDL_CTL
#define PREDL_OUTLOW_EN                                 (1 << 7)
#define PREDL_GAIN(gain)                                ((gain) << 4)
#define PREDL_AR2_EN                                    (1 << 3)
#define PREDL_AL2_EN                                    (1 << 2)
#define PREDL_AL1_EN                                    (1 << 1)
#define PREDL_VOICE_EN                                  (1 << 0)

// TWL_PREDR_CTL
#define PREDR_OUTLOW_EN                                 (1 << 7)
#define PREDR_GAIN(gain)                                ((gain) << 4)
#define PREDR_AL2_EN                                    (1 << 3)
#define PREDR_AR2_EN                                    (1 << 2)
#define PREDR_AR1_EN                                    (1 << 1)
#define PREDR_VOICE_EN                                  (1 << 0)

// TWL_HFL_CTL, TWL_HFR_CTL
#define HFX_REF_EN                                      (1 << 5)
#define HFX_REF_RAMP                                    (1 << 4)
#define HFX_REF_LOOP                                    (1 << 3)
#define HFX_REF_HB                                      (1 << 2)
#define HFX_RAMP_EN                                    (1 << 4)
#define HFX_LOOP_EN                                    (1 << 3)
#define HFX_HB_EN                                      (1 << 2)
#define HFX_INPUT_SEL(input)                            ((input) << 0)

// TWL_ALC_CTL
#define ALC_MODE                                        (1 << 5)
#define SUBMIC_ALC_EN                                   (1 << 4)
#define MAINMIC_ALC_EN                                  (1 << 3)
#define ALC_WAIT(wait)                                  ((wait) < 0)

// TWL_APLL_CTL
#define APLL_EN                                         (1 << 4)
#define APLL_INFREQ(freq)                               ((freq) << 0)

// TWL_MISC_SET_1
#define MISC_SET_1_CLK65_EN                             (1 << 7)
#define MISC_SET_1_SCRAMBLE_EN                          (1 << 6)
#define MISC_SET_1_FMLOOP_EN                            (1 << 5)
#define MISC_SET_1_DIGMIC_LR_SWAP_EN                    (1 << 0)

// TWL_RX_PATH_SEL
#define RXL2_SEL                                        (1 << 5)
#define RXR2_SEL                                        (1 << 4)
#define RXL1_SEL(val)                                   ((val) << 2)
#define RXR1_SEL(val)                                   ((val) << 0)

// TWL_VDL_APGA_CTL
#define VDL_GAIN_SET(val)                               ((val) << 3)
#define VDL_FM_EN                                       (1 << 2)
#define VDL_DA_EN                                       (1 << 1)
#define VDL_PDZ                                         (1 << 0)

// TWL_ANAMIC_GAIN
#define ANAMIC_MICAMPR_GAIN(gain)                       ((gain) << 3)
#define ANAMIC_MICAMPL_GAIN(gain)                       ((gain) << 0)

// TWL_MISC_SET_2
#define MISC_SET_2_VTX_3RD_HPF_BYP                      (1 << 6)
#define MISC_SET_2_ATX_HPF_BYP                          (1 << 5)
#define MISC_SET_2_VRX_3RD_HPF_BYP                      (1 << 4)
#define MISC_SET_2_ARX_HPF_BYP                          (1 << 3)
#define MISC_SET_2_VTX_HPF_BYP                          (1 << 2)
#define MISC_SET_2_VRX_HPF_BYP                          (1 << 1)

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

