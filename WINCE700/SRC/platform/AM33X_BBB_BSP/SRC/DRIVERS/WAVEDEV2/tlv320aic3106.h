//
// Copyright (c) MPC-Data Limited 2010.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  tlv320aic3106.h
//
//  Defines the class for the TLV320AIC3106 audio codec 
//

#ifndef __TLV320AIC3106_H__
#define __TLV320AIC3106_H__

#ifdef __cplusplus
extern "C" {
#endif

// Volume values
// Reduced max volume to reduce audio clipping and distortion due to amplifiers.
#define TLV320AIC3106_VOLUME_MAX_VAL             0xF0     // Maximum volume
#define TLV320AIC3106_VOLUME_MIN_VAL             0x80     // Minimum volume

// Supported sample rates

#define SAMPLE_32K                 32000
#define SAMPLE_44K                 44100
#define SAMPLE_48K                 48000
#define SAMPLE_96K                 96000

// Output volume values
#define OUTPUT_VOLUME_MIN            TLV320AIC3106_VOLUME_MIN_VAL
#define OUTPUT_VOLUME_MAX            TLV320AIC3106_VOLUME_MAX_VAL
#define OUTPUT_VOLUME_RANGE          (OUTPUT_VOLUME_MAX - OUTPUT_VOLUME_MIN)

// Low word = left channel, high word = right channel
//#define OUTPUT_VOLUME_DEFAULT        0xCCCCCCCC  // 80% of 0xFFFF
//#define INPUT_VOLUME_DEFAULT         0xA000A000  // ~63% of 0xFFFF
#define OUTPUT_VOLUME_DEFAULT        0xFFFFFFFF  // 100% of 0xFFFF
#define INPUT_VOLUME_DEFAULT         0x60006000  // ~38% of 0xFFFF

/* ------------------------------------------------------------------------ *
 * CodecConfig
 * 
 * This class represents the AIC23 Codec. This pointer of this class
 * type is maintained by the WaveDev Driver's PDD Layer to maintain the
 * data and the control interface to the Codec. 
 *
 * Depending on which Codec is being used, the same class module 
 * can be modified and the PDD Layer can be used to complete the Driver
 * for any given Audio Codec Chip.
 * ------------------------------------------------------------------------ */ 
 
#define TLV320AIC3106_PAGESELECT                0
#define TLV320AIC3106_PAGESELECT_ZERO           0
#define TLV320AIC3106_PAGESELECT_ONE            1

#define TLV320AIC3106_CDCSAMPRATE               2
#define TLV320AIC3106_CDCSAMPRATE_FSREF_1       0
#define TLV320AIC3106_CDCSAMPRATE_FSREF_1_5     1
#define TLV320AIC3106_CDCSAMPRATE_FSREF_2       2
#define TLV320AIC3106_CDCSAMPRATE_FSREF_2_5     3
#define TLV320AIC3106_CDCSAMPRATE_FSREF_3       4
#define TLV320AIC3106_CDCSAMPRATE_FSREF_3_5     5
#define TLV320AIC3106_CDCSAMPRATE_FSREF_4       6
#define TLV320AIC3106_CDCSAMPRATE_FSREF_4_5     7
#define TLV320AIC3106_CDCSAMPRATE_FSREF_5       8
#define TLV320AIC3106_CDCSAMPRATE_FSREF_5_5     9
#define TLV320AIC3106_CDCSAMPRATE_FSREF_6       10
#define TLV320AIC3106_CDCSAMPRATE_DAC_POS       4
#define TLV320AIC3106_CDCSAMPRATE_DAC_MASK      (0xf << TLV320AIC3106_CDCSAMPRATE_DAC_POS)
#define TLV320AIC3106_CDCSAMPRATE_ADC_POS       0
#define TLV320AIC3106_CDCSAMPRATE_ADC_MASK      (0xf << TLV320AIC3106_CDCSAMPRATE_ADC_POS)


#define TLV320AIC3106_PLLPROGA                  3
#define TLV320AIC3106_PLLPROGA_PVALMASK         7
#define TLV320AIC3106_PLLPROGA_PLLEN            (1 << 7)

#define TLV320AIC3106_PLLPROGB                  4
#define TLV320AIC3106_PLLPROGB_JVALPOS          2
#define TLV320AIC3106_PLLPROGB_JVALMASK         (0x3f << TLV320AIC3106_PLLPROGB_JVALPOS)

#define TLV320AIC3106_PLLPROGC                  5
#define TLV320AIC3106_PLLPROGC_DVALMASKHIGH     0xff

#define TLV320AIC3106_PLLPROGD                  6
#define TLV320AIC3106_PLLPROGD_DVALMASKLOW      0xfc

#define TLV3201AIC3106_CODECDATPATH             7
#define TLV3201AIC3106_CODECDATPATH_FSREF_POS   7
#define TLV3201AIC3106_CODECDATPATH_FSREF_MASK  (1 << TLV3201AIC3106_CODECDATPATH_FSREF_POS)
#define TLV3201AIC3106_CODECDATPATH_FSREF44_1   (1 << TLV3201AIC3106_CODECDATPATH_FSREF_POS)

#define TLV3201AIC3106_CODECDATPATH_LEFT_POS    3
#define TLV3201AIC3106_CODECDATPATH_LEFT_MASK   (0x3 << TLV3201AIC3106_CODECDATPATH_LEFT_POS)
#define TLV3201AIC3106_CODECDATPATH_LEFT_LEFT   (1 << TLV3201AIC3106_CODECDATPATH_LEFT_POS)
#define TLV3201AIC3106_CODECDATPATH_LEFT_RIGHT  (2 << TLV3201AIC3106_CODECDATPATH_LEFT_POS)
#define TLV3201AIC3106_CODECDATPATH_LEFT_MONO   (3 << TLV3201AIC3106_CODECDATPATH_LEFT_POS)

#define TLV3201AIC3106_CODECDATPATH_RIGHT_POS   1
#define TLV3201AIC3106_CODECDATPATH_RIGHT_MASK  (0x3 << TLV3201AIC3106_CODECDATPATH_RIGHT_POS)
#define TLV3201AIC3106_CODECDATPATH_RIGHT_RIGHT (1 << TLV3201AIC3106_CODECDATPATH_RIGHT_POS)
#define TLV3201AIC3106_CODECDATPATH_RIGHT_LEFT  (2 << TLV3201AIC3106_CODECDATPATH_RIGHT_POS)
#define TLV3201AIC3106_CODECDATPATH_RIGHT_MONO  (3 << TLV3201AIC3106_CODECDATPATH_RIGHT_POS)

#define TLV320AIC3106_ASDICTRLA                 8
#define TLV320AIC3106_ASDICTRLA_BCLKDIR_OUT     (1 << 7)
#define TLV320AIC3106_ASDICTRLA_WCLKDIR_OUT     (1 << 6)

#define TLV320AIC3106_ASDICTRLB                 9

#define TLV320AIC3106_ASDICTRLC                 10

#define TLV320AIC3106_CDCOVRFLOWFLG             11
#define TLV320AIC3106_CDCOVRFLOWFLG_RMASK       0xf

#define TLV320AIC3106_LEFTADCPGAGAINCTL         15
#define TLV320AIC3106_LEFTADCPGAGAINCTL_GAINMSK (0x7f << 0)

#define TLV320AIC3106_RIGHTADCPGAGAINCTL        16
#define TLV320AIC3106_RIGHTADCPGAGAINCTL_GAINMSK (0x7f << 0)

#define TLV320AIC3106_MIC3LRLEFTADCCTRL         17

#define TLV320AIC3106_MIC3LRRIGHTADCCTRL        18

#define TLV320AIC3106_LINE1LADCLCTL             19
#define TLV320AIC3106_LINE1LADCLCTL_PWR_POS     2
#define TLV320AIC3106_LINE1LADCLCTL_PWR_EN      (1 << TLV320AIC3106_LINE1LADCLCTL_PWR_POS)

#define TLV320AIC3106_LINE1RADCRCTL             22
#define TLV320AIC3106_LINE1RADCRCTL_PWR_POS     2
#define TLV320AIC3106_LINE1RADCRCTL_PWR_EN      (1 << TLV320AIC3106_LINE1RADCRCTL_PWR_POS)

#define TLV320AIC3106_MICBIAS                   25

#define TLV320AIC3106_LEFTAGCCTLB               27
#define TLV320AIC3106_LEFTAGCCTLB_POS           1
#define TLV320AIC3106_LEFTAGCCTLB_MASK          (0x7f << TLV320AIC3106_LEFTAGCCTLB_POS)

#define TLV320AIC3106_RIGHTAGCCTLB              30
#define TLV320AIC3106_RIGHTAGCCTLB_POS          1
#define TLV320AIC3106_RIGHTAGCCTLB_MASK         (0x7f << TLV320AIC3106_LEFTAGCCTLB_POS)

#define TLV320AIC3106_DACPWROUTDRVCTRL          37
#define TLV320AIC3106_DACPWROUTDRVCTRL_LEFTON   (1 << 7)
#define TLV320AIC3106_DACPWROUTDRVCTRL_RIGHTON  (1 << 6)

#define TLV320AIC3106_HPRCOM_CTL                38

#define TLV320AIC3106_LEFTDACVOLCTRL            43
#define TLV320AIC3106_LEFTDACVOLCTRL_MUTE       (1 << 7)
#define TLV320AIC3106_LEFTDACVOLCTRL_VOLMASK    0x7f

#define TLV320AIC3106_RIGHTDACVOLCTRL           44
#define TLV320AIC3106_RIGHTDACVOLCTRL_MUTE      (1 << 7)
#define TLV320AIC3106_RIGHTDACVOLCTRL_VOLMASK   0x7f

#define TLV320AIC3106_DACL1TOHPLOUTCTRL         47
#define TLV320AIC3106_DACL1TOHPLOUTCTRL_ROUTED  (1 << 7)
#define TLV320AIC3106_DACL1TOHPLOUTCTRL_VOL_MASK    (0x7f)

#define TLV320AIC3106_HPLOUTLVLCTRL             51

#define TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL      58

#define TLV320AIC3106_DACR1HPROUTVOL            64

#define TLV320AIC3106_HPROUTLVLCTRL             65

#define TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL      72

#define TLV320AIC3106_DACL1TOLEFTLOP            82
#define TLV320AIC3106_DACL1TOLEFTLOP_ROUTED     (1 << 7)
#define TLV320AIC3106_DACL1TOLEFTLOP_ANVOLMASK  0x3f

#define TLV320AIC3106_LEFTLOPMLVLCTRL           86
#define TLV320AIC3106_LEFTLOPMLVLCTRL_LVLMASK   (0xf << 4)
#define TLV320AIC3106_LEFTLOPMLVLCTRL_UNMUTE    (1 << 3)
#define TLV320AIC3106_LEFTLOPMLVLCTRL_VOLSTATUS (1 << 1)
#define TLV320AIC3106_LEFTLOPMLVLCTRL_PWRSTATUS (1 << 0)

#define TLV320AIC3106_DACR1TORIGHTLOP           92
#define TLV320AIC3106_DACR1TORIGHTLOP_ROUTED    (1 << 7)
#define TLV320AIC3106_DACR1TORIGHTLOP_ANVOLMASK 0x3f

#define TLV320AIC3106_RIGHTLOPMLVLCTRL          93
#define TLV320AIC3106_RIGHTLOPMLVLCTRL_LVLMASK   (0xf << 4)
#define TLV320AIC3106_RIGHTLOPMLVLCTRL_UNMUTE    (1 << 3)
#define TLV320AIC3106_RIGHTLOPMLVLCTRL_VOLSTATUS (1 << 1)
#define TLV320AIC3106_RIGHTLOPMLVLCTRL_PWRSTATUS (1 << 0)

#define TLV320AIC3106_ADDGPIOCTLB               101

#define TLV320AIC3106_CLKGENCTL                 102

class TLV320AIC3106CodecConfig
{
    private:

        DWORD  m_CodecClock;                /* Codec Clock Value */
        ULONG  m_DacSampleRate;             /* Sampling Rate for DAC      */ 

        USHORT m_OutVolume;                 /* Audio Output Volume        */
        DWORD m_OutputDevice;
        DWORD m_InputDevice;
        void* m_hI2CDevice;

        BOOL WriteAIC (UINT8 Addr, UINT8 Data);
        BOOL ReadAIC  (UINT8 Addr, UINT8 *pData);
        void EnableMic3( void );
        void EnableLine1( void );

        void EnableHPOut_CLKDIV(void);
        void EnableHPOut_PLL(void);
        void EnableLineOut(void);

        void GetOutput(DWORD* pOut) {*pOut = m_OutputDevice;}
        void GetInput(DWORD* pIn) {*pIn = m_InputDevice;}

        void GetAicReg(DWORD *aic_reg);  /* aic_reg: [In]=ptr to reg num; [Out]=ptr to reg val */

#if AIC_DEBUG
        void DumpHPOutRegs();
#endif

    public:
        TLV320AIC3106CodecConfig  ();
        ~TLV320AIC3106CodecConfig ();

        BOOL CodecInit  (ULONG sampleRate, ULONG volume);

        void SetMasterVolume  (ULONG volume);

        void SetCodecPower    (BOOL powerOn);   
        void SetOutputVolume  (UINT DeviceId, ULONG outputVolume);
        void SetInputVolume  (ULONG outputVolume);
        
        void PowerOff (BOOL Enable);
        INT CheckAudioFormat(WORD Channel, LPWAVEFORMATEX pwfe);

        void EnableInput(DWORD input);
        void EnableOutput(DWORD output);

        BOOL HandleMessage(
            DWORD  dwCode,
            PBYTE  pBufIn,
            DWORD  dwLenIn,
            PBYTE  pBufOut,
            DWORD  dwLenOut,
            PDWORD pdwActualOut
            );
};




#ifdef __cplusplus
}
#endif

#endif // __TLV320AIC3106_H__
