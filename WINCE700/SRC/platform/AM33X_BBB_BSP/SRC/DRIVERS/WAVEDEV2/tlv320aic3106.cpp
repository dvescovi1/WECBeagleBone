//
// Copyright (c) MPC-Data Limited 2010.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  tlv320aic3106.cpp
//
//  Defines the class for the TLV320AIC3106 audio codec 
//

#include <windows.h>
//#include "i2c.h"
#include "tlv320aic3106.h"
#include <wavedbg.h>
#include <wavioctl.h>
#include <sdk_i2c.h>
#include <soc_cfg.h>
#include "sdk_padcfg.h"
#include "bsp.h"

#define R2_ADC_SMPL_RATE_DIV1  0x00
#define R2_ADC_SMPL_RATE_DIV2  0x22
#define R2_ADC_SMPL_RATE_DIV3  0x44
#define R2_ADC_SMPL_RATE_DIV4  0x66
#define R2_ADC_SMPL_RATE_DIV5  0x88
#define R2_ADC_SMPL_RATE_DIV6  0xAA

#define R7_44KHZ 0x80
#define R7_48KHZ 0x00

#define R101_CLKDIV_OUT 0x01
#define R101_PLLDIV_OUT 0x00

#define R102_CLKDIV_IN_MCLK 0x00
#define R102_CLKDIV_IN_GPIO 0x40
#define R102_CLKDIV_IN_MCLK 0x80

#define R102_PLLCLK_IN_MCLK 0x00
#define R102_PLLCLK_IN_GPIO 0x10
#define R102_PLLCLK_IN_BCLK 0x20

#define R5_VAL ((UINT8)(0xFF & ((D & 0xFFC0) >> 6)))
#define R6_VAL ((UINT8)(0xFF & ((D & 0x003F) << 2)))

// Define more #defines for other values of MCLK and FS_REF as needed
#define MCLK_12MHZ

#ifdef MCLK_12MHZ
#define  SAMPLE_RATE_DIV R2_ADC_SMPL_RATE_DIV1
#define  FS_REF          R7_48KHZ
#define  P               1
#define  R               1
#define  J               8
#define  D               1920
#endif

TLV320AIC3106CodecConfig::TLV320AIC3106CodecConfig(void)
{
    m_hI2CDevice=NULL;
    DEBUGMSG( ZONE_FUNCTION, (L"TLV320AIC3106CodecConfig\r\n"));
}

TLV320AIC3106CodecConfig::~TLV320AIC3106CodecConfig(void)
{
    DEBUGMSG( ZONE_FUNCTION, (L"~TLV320AIC3106CodecConfig\r\n"));
    
	if (m_hI2CDevice != NULL)
	{
		I2CClose(m_hI2CDevice);
		m_hI2CDevice = NULL;
	}
}

int
TLV320AIC3106CodecConfig::WriteAIC(
                                   UINT8 reg,
                                   UINT8 val
                                   )
{
    BOOL bRet = TRUE;    

    if (m_hI2CDevice==NULL) 
        RETAILMSG(1,(L"TLV320AIC3106CodecConfig::WriteAIC::m_hI2CDevice is NULL reg=%d val=%d\r\n",reg,val));

	if (I2CWrite(m_hI2CDevice, reg, &val, sizeof(val)) != sizeof(val))
	{
		bRet = FALSE;
	}

	return bRet;
}

int
TLV320AIC3106CodecConfig::ReadAIC(
                                  UINT8 reg,
                                  UINT8 *val
                                  )
{
    BOOL bRet = TRUE;    
    
    if (m_hI2CDevice==NULL) 
        RETAILMSG(1,(L"TLV320AIC3106CodecConfig::ReadAIC::m_hI2CDevice is NULL reg=%d \r\n",reg));

	if (I2CRead(m_hI2CDevice, reg, val, sizeof(UINT8)) != sizeof(UINT8))
	{
		bRet = FALSE;
	}

	return bRet;
}

BOOL
TLV320AIC3106CodecConfig::CodecInit(
                                    ULONG sampleRate,
                                    ULONG volume)
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::CodecInit\r\n"));

    //RequestDevicePads(SOCGetI2CDeviceByBus(I2C_AUDIO_BUS));

    // Open I2C bus
	m_hI2CDevice = I2COpen(SOCGetI2CDeviceByBus(I2C_AUDIO_BUS));
	if (m_hI2CDevice == NULL)
	{
		ERRMSG((L"TLV320AIC3106CodecConfig::CodecInit: Failed to open I2C driver"));
		return FALSE;
	}
	I2CSetSlaveAddress(m_hI2CDevice, (UINT16)I2C_AUDIO_ADDR);
	//I2CSetSubAddressMode(m_hI2CDevice, I2C_SUBADDRESS_MODE_8);

    EnableHPOut_PLL();

    EnableLine1();

//    DumpHPOutRegs();

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::CodecInit\r\n"));

    return TRUE;
}

void
TLV320AIC3106CodecConfig::SetMasterVolume(
                                          ULONG volume
                                          )
{
}

void
TLV320AIC3106CodecConfig::SetCodecPower(
                                        BOOL powerOn
                                        )
{
}

void
TLV320AIC3106CodecConfig::SetOutputVolume(
                                          UINT DeviceId,
                                          ULONG outputVolume
                                          )
{
    // Max volume on the AIC3106 output (DAC) is 0, min is 0x7f
    // mask the channel, shift it then apply the volume mask
    unsigned char rightVol = (unsigned char)(~(outputVolume >> 25)) & TLV320AIC3106_LEFTDACVOLCTRL_VOLMASK;
    unsigned char leftVol = (unsigned char)(~(outputVolume >> 9)) & TLV320AIC3106_RIGHTDACVOLCTRL_VOLMASK;

    DEBUGMSG(ZONE_FUNCTION,
        (TEXT("TLV320AIC3106CodecConfig::SetOutputVolume: 0x%x L=0x%x R=0x%x\r\n"), outputVolume, rightVol, leftVol));

    WriteAIC(TLV320AIC3106_LEFTDACVOLCTRL, leftVol);
    WriteAIC(TLV320AIC3106_RIGHTDACVOLCTRL, rightVol);
}

void
TLV320AIC3106CodecConfig::SetInputVolume(
                                         ULONG inputVolume
                                         )
{
    // Max volume on the AIC3106 ADC is 0x7F, min is 0
    unsigned char rightVol = (unsigned char)((inputVolume >> 25) & TLV320AIC3106_LEFTADCPGAGAINCTL_GAINMSK);
    unsigned char leftVol = (unsigned char)((inputVolume >> 9) & TLV320AIC3106_RIGHTADCPGAGAINCTL_GAINMSK);

    DEBUGMSG(ZONE_FUNCTION,
        (TEXT("TLV320AIC3106CodecConfig::SetInputVolume: 0x%x L=0x%x R=0x%x\r\n"), inputVolume, rightVol, leftVol));

    WriteAIC(TLV320AIC3106_LEFTADCPGAGAINCTL, leftVol);
    WriteAIC(TLV320AIC3106_RIGHTADCPGAGAINCTL, rightVol);
}

void
TLV320AIC3106CodecConfig::PowerOff(
                                   BOOL Enable
                                   )
{
}

INT
TLV320AIC3106CodecConfig::CheckAudioFormat(
                                           WORD Channel,
                                           LPWAVEFORMATEX pwfe
                                           )
{
    return TRUE;
}

void
TLV320AIC3106CodecConfig::EnableMic3(
                                     void
                                     )
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::EnableMic3\r\n"));

    WriteAIC( TLV320AIC3106_LEFTADCPGAGAINCTL, 0x17 );  // 15  Left ADC PGA Gain    <- [Mute=OFF][Gain=8.5dB]
    WriteAIC( TLV320AIC3106_RIGHTADCPGAGAINCTL, 0x17 ); // 16 Right ADC PGA Gain    <- [Mute=OFF][Gain=8.5dB]
    WriteAIC( TLV320AIC3106_MIC3LRLEFTADCCTRL, 0x0F );  // 17 MIC3L/R to  Left ADC  <- [MIC3L=0dBGain][MIC3R=NotConnect]
    WriteAIC( TLV320AIC3106_MIC3LRRIGHTADCCTRL, 0xF0 ); // 18 MIC3L/R to Right ADC  <- [MIC3L=NotConnect][MIC3R=0dBGain]
    WriteAIC( TLV320AIC3106_LINE1LADCLCTL, 0x7C );      // 19  LINE1L to  Left ADC  <- [SingleEnd][NotConnect][Power=ON][SoftStep=OncePerFS]
    WriteAIC( TLV320AIC3106_LINE1RADCRCTL, 0x7C );      // 22  LINE1R to Right ADC  <- [SingleEnd][NotConnect][Power=ON][SoftStep=OncePerFS]
    WriteAIC( TLV320AIC3106_MICBIAS, 0x40 );            // 25 MICBIAS               <- [MICBIAS=2.0V]

    m_InputDevice = WAV_SET_INPUT_MIC3;

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableMic3\r\n"));
}

void
TLV320AIC3106CodecConfig::EnableLine1(
                                      void
                                      )
{
    unsigned char rightVol = (unsigned char)((INPUT_VOLUME_DEFAULT >> 25) & TLV320AIC3106_LEFTADCPGAGAINCTL_GAINMSK);
    unsigned char leftVol = (unsigned char)((INPUT_VOLUME_DEFAULT >> 9) & TLV320AIC3106_RIGHTADCPGAGAINCTL_GAINMSK);

    RETAILMSG( 1,
        (L"+TLV320AIC3106CodecConfig::EnableLine1: lVol=0x%02X rVol=0x%02X\r\n", leftVol, rightVol));

    WriteAIC(TLV320AIC3106_LEFTADCPGAGAINCTL,  leftVol  );    // 15
    WriteAIC(TLV320AIC3106_RIGHTADCPGAGAINCTL, rightVol );    // 16
    WriteAIC(TLV320AIC3106_LINE1LADCLCTL,      0x04);    // 19
    WriteAIC(TLV320AIC3106_LINE1RADCRCTL,      0x04);    // 22

	/* In Centaurus board, MICBIAS is connected to some voltage, but in
	 * AM335x board, MICBIAS pin is not connected */
    WriteAIC(TLV320AIC3106_MICBIAS,            0x40);    // 25

//    WriteAIC(TLV320AIC3106_LEFTAGCCTLB, 0);
//    WriteAIC(TLV320AIC3106_RIGHTAGCCTLB, 0);

    m_InputDevice = WAV_SET_INPUT_LINE1;

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableLine1\r\n"));
}

void
TLV320AIC3106CodecConfig::EnableInput(
                                      DWORD input
                                      )
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::EnableInput (input=%d)\r\n", input));

    if(input == WAV_SET_INPUT_MIC3)
    {
        EnableMic3();
    }
    else if(input == WAV_SET_INPUT_LINE1)
    {
        EnableLine1();
    }
    else
    {
    }

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableInput\r\n"));
}
void
TLV320AIC3106CodecConfig::EnableHPOut_CLKDIV(void)
{
    UINT8 Q = 4;
    unsigned char rightVol = (unsigned char)(~(OUTPUT_VOLUME_DEFAULT >> 25)) & TLV320AIC3106_LEFTDACVOLCTRL_VOLMASK;
    unsigned char leftVol = (unsigned char)(~(OUTPUT_VOLUME_DEFAULT >> 9)) & TLV320AIC3106_RIGHTDACVOLCTRL_VOLMASK;

    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::EnableHPOut\r\n"));

    WriteAIC(TLV320AIC3106_PAGESELECT, TLV320AIC3106_PAGESELECT_ZERO);  // Select page 0
    WriteAIC(1, 0x80);                                                  // Reset AIC3106

    Sleep(2);

    WriteAIC(  TLV320AIC3106_PLLPROGA, ((Q << 0x3) | 0x02)); // 3 PLL A                  <- [PLL=OFF][Q=4][P=2]
    WriteAIC(  TLV320AIC3106_PLLPROGB,      0x20 );       // 4 PLL B                  <- [J=8]
    WriteAIC(  TLV320AIC3106_PLLPROGC,      0x6E );       // 5 PLL C                  <- [D=7075]
    WriteAIC(  TLV320AIC3106_PLLPROGD,      0x23 );       // 6 PLL D                  <- [D=7075]
    WriteAIC(  TLV3201AIC3106_CODECDATPATH, 0x0A );       // 7 Codec Datapath Setup   <- [FS=48 kHz][LeftDAC=LEFT][RightDAC=RIGHT]
    WriteAIC(  TLV320AIC3106_ASDICTRLA,     0xC0 );       // 8  Audio Interface Control A  <- [BCLK=Slave][MCLK=Slave]
	WriteAIC(  TLV320AIC3106_ASDICTRLB,     0x00 );       // 9  Audio Interface Control B  <- [I2S mode][16 bit]
	WriteAIC(  TLV320AIC3106_ASDICTRLC,     0x00 );       // 10 Audio Interface Control C  <- [Data offset=0]

    WriteAIC(  TLV320AIC3106_LEFTADCPGAGAINCTL,  0 );    // 15  Left ADC PGA Gain              <- [Mute=OFF]
    WriteAIC(  TLV320AIC3106_RIGHTADCPGAGAINCTL, 0 );    // 16 Right ADC PGA Gain              <- [Mute=OFF]
    WriteAIC(  TLV320AIC3106_LINE1LADCLCTL,      0x04 ); // 19  LINE1L to  Left ADC            <- [SingleEnd][Gain=0dB][Power=ON][SoftStep=OncePerFS]
    WriteAIC(  TLV320AIC3106_LINE1RADCRCTL,      0x04 ); // 22  LINE1R to Right ADC            <- [SingleEnd][Gain=0dB][Power=ON][SoftStep=OncePerFS]
    WriteAIC(  TLV320AIC3106_LEFTAGCCTLB,        0 );    // 27  Left AGC B                     <- [OFF]
    WriteAIC(  TLV320AIC3106_RIGHTAGCCTLB,       0 );    // 30 Right AGC B                     <- [OFF]
    WriteAIC(  TLV320AIC3106_DACPWROUTDRVCTRL,   0xE0 ); // 37 DAC Power & Output Dvr          <- [LeftDAC=ON][RightDAC=ON][HPLCOM=SingleEnd]
    WriteAIC(  TLV320AIC3106_HPRCOM_CTL,         0x10 ); // 38 High Power Output Dvr           <- [HPRCOM=SingleEnd][ShortCircuit=OFF]
    WriteAIC(  TLV320AIC3106_LEFTDACVOLCTRL,     leftVol );   // 43  Left DAC Digital Volume   <- [Mute=OFF][Gain=0dB]
    WriteAIC(  TLV320AIC3106_RIGHTDACVOLCTRL,    rightVol );   // 44 Right DAC Digital Volume  <- [Mute=OFF][Gain=0dB]
    WriteAIC(  TLV320AIC3106_DACL1TOHPLOUTCTRL,  0x80 );// 47 DAC_L1 to HPLOUT Volume         <- [Routed]
    WriteAIC(  TLV320AIC3106_HPLOUTLVLCTRL,      0x09 );// 51           HPLOUT Output         <- [Mute=OFF][Power=ON]
    WriteAIC(  TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL, 0 ); // 58           HPLCOM Output         <- []
    WriteAIC(  TLV320AIC3106_DACR1HPROUTVOL,     0x80 );// 64 DAC_R1 to HPROUT Volume         <- [Routed]
    WriteAIC(  TLV320AIC3106_HPROUTLVLCTRL,      0x09 );// 65           HPROUT Output         <- [Mute=OFF][Power=ON]
    WriteAIC(  TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL, 0 ); // 72           HPRCOM Output         <- []
    WriteAIC(  TLV320AIC3106_DACL1TOLEFTLOP,       0 ); // 82 DAC_L1 to LEFT_LOP/M Volume     <- []
    WriteAIC(  TLV320AIC3106_LEFTLOPMLVLCTRL,      0 ); // 86           LEFT_LOP/M Output     <- []
    WriteAIC(  TLV320AIC3106_DACR1TORIGHTLOP,      0 ); // 92 DAC_R1 to RIGHT_LOP/M Volume    <- []
    WriteAIC(  TLV320AIC3106_RIGHTLOPMLVLCTRL,     0 ); // 93           RIGHT_LOP/M Output    <- []

    WriteAIC(  TLV320AIC3106_ADDGPIOCTLB, 0x01 ); // 101 GPIO Control Register B  <- [CODEC_CLKIN = CLKDIV_OUT]
    WriteAIC(  TLV320AIC3106_CLKGENCTL,   0    ); // 102 Clock Generation Control <- [PLLCLK_IN and CLKDIV_IN use MCLK]

    m_OutputDevice = WAV_SET_OUTPUT_HDP;


    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableHPOut\r\n"));
}


void
TLV320AIC3106CodecConfig::EnableHPOut_PLL(void)
{
    unsigned char rightVol = (unsigned char)(~(OUTPUT_VOLUME_DEFAULT >> 25)) & TLV320AIC3106_LEFTDACVOLCTRL_VOLMASK;
    unsigned char leftVol = (unsigned char)(~(OUTPUT_VOLUME_DEFAULT >> 9)) & TLV320AIC3106_RIGHTDACVOLCTRL_VOLMASK;

    // NOte: print formula doesn't work if SAMPLE_RATE_DIV is 1.5, 2.5 etc.
    RETAILMSG(1,(L"+EnableHPOut_PLL fs_ref=%dkHz, sample_rate_div=%d (0x%02X), lVol=0x%02X, rVol=0x%02X\r\n", 
        (FS_REF==R7_44KHZ?44:48), (((SAMPLE_RATE_DIV & 0x0F)/2) + 1), SAMPLE_RATE_DIV, leftVol, rightVol));

    WriteAIC(TLV320AIC3106_PAGESELECT, TLV320AIC3106_PAGESELECT_ZERO);  // Select page 0
    WriteAIC(1, 0x80);                                                  // Reset AIC3106

    Sleep(2);

    WriteAIC(  TLV320AIC3106_CDCSAMPRATE,  SAMPLE_RATE_DIV);         // 2  fs = FS_REF/SAMPLE_RATE_DIV

    WriteAIC(  TLV320AIC3106_PLLPROGA,      (0x80 | (P & 0x07))); // 3
    WriteAIC(  TLV320AIC3106_PLLPROGB,      ((J & 0x3F) << 2));   // 4 

    WriteAIC(  TLV320AIC3106_PLLPROGC,  R5_VAL); 
    WriteAIC(  TLV320AIC3106_PLLPROGD,  R6_VAL);

    WriteAIC(  TLV3201AIC3106_CODECDATPATH, (FS_REF|0x08|0x02) ); // 7 
    WriteAIC(  TLV320AIC3106_ASDICTRLA,     (0x80|0x40|0x10));   // 8
	WriteAIC(  TLV320AIC3106_ASDICTRLB,     0x00 );        // 9
	WriteAIC(  TLV320AIC3106_ASDICTRLC,     0x00 );        // 10

	WriteAIC(  TLV320AIC3106_CDCOVRFLOWFLG,     (R & 0x0F) );    // 11

    // LINE1
    WriteAIC(  TLV320AIC3106_LEFTADCPGAGAINCTL,  0 );    // 15
    WriteAIC(  TLV320AIC3106_RIGHTADCPGAGAINCTL, 0 );    // 16
    WriteAIC(  TLV320AIC3106_LINE1LADCLCTL,      0x04 ); // 19
    WriteAIC(  TLV320AIC3106_LINE1RADCRCTL,      0x04 ); // 22

    WriteAIC(  TLV320AIC3106_LEFTAGCCTLB,        0 );    // 27
    WriteAIC(  TLV320AIC3106_RIGHTAGCCTLB,       0 );    // 30
    WriteAIC(  TLV320AIC3106_DACPWROUTDRVCTRL,   (0x80|0x40|0x20) ); // 37
    WriteAIC(  TLV320AIC3106_HPRCOM_CTL,         0x10 ); // 38
    WriteAIC(  TLV320AIC3106_LEFTDACVOLCTRL,     leftVol );   // 43
    WriteAIC(  TLV320AIC3106_RIGHTDACVOLCTRL,    rightVol );   // 44

    // HPOut
    WriteAIC(  TLV320AIC3106_DACL1TOHPLOUTCTRL,  0x80 );// 47
    WriteAIC(  TLV320AIC3106_HPLOUTLVLCTRL,      (0x08|0x01));// 51
    WriteAIC(  TLV320AIC3106_DACR1HPROUTVOL,     0x80 );// 64
    WriteAIC(  TLV320AIC3106_HPROUTLVLCTRL,      (0x08|0x01));// 65
    WriteAIC(  TLV320AIC3106_DACL1TOLEFTLOP,       0 ); // 82
    WriteAIC(  TLV320AIC3106_LEFTLOPMLVLCTRL,      0 ); // 86
    WriteAIC(  TLV320AIC3106_DACR1TORIGHTLOP,      0 ); // 92
    WriteAIC(  TLV320AIC3106_RIGHTLOPMLVLCTRL,     0 ); // 93

    WriteAIC(  TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL, 0 ); // 58
    WriteAIC(  TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL, 0 ); // 72

    WriteAIC(  TLV320AIC3106_ADDGPIOCTLB, R101_PLLDIV_OUT ); // 101
    WriteAIC(  TLV320AIC3106_CLKGENCTL,   R102_PLLCLK_IN_MCLK    ); // 102

    m_OutputDevice = WAV_SET_OUTPUT_HDP;


    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableHPOut_PLL\r\n"));
}


void
TLV320AIC3106CodecConfig::EnableLineOut(
                                        void
                                        )
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::EnableLineOut\r\n"));

    WriteAIC(  TLV320AIC3106_LEFTADCPGAGAINCTL, 0 );    // 15  Left ADC PGA Gain              <- [Mute=OFF]
    WriteAIC(  TLV320AIC3106_RIGHTADCPGAGAINCTL, 0 );   // 16 Right ADC PGA Gain              <- [Mute=OFF]
    WriteAIC(  TLV320AIC3106_LINE1LADCLCTL, 0x04 );     // 19  LINE1L to  Left ADC            <- [SingleEnd][Gain=0dB][Power=ON][SoftStep=OncePerFS]
    WriteAIC(  TLV320AIC3106_LINE1RADCRCTL, 0x04 );     // 22  LINE1R to Right ADC            <- [SingleEnd][Gain=0dB][Power=ON][SoftStep=OncePerFS]
    WriteAIC(  TLV320AIC3106_LEFTAGCCTLB, 0 );          // 27  Left AGC B                     <- [OFF]
    WriteAIC(  TLV320AIC3106_RIGHTAGCCTLB, 0 );         // 30 Right AGC B                     <- [OFF]
    WriteAIC(  TLV320AIC3106_DACPWROUTDRVCTRL, 0xE0 );  // 37 DAC Power & Output Dvr          <- [LeftDAC=ON][RightDAC=ON][HPLCOM=SingleEnd]
    WriteAIC(  TLV320AIC3106_HPRCOM_CTL, 0x10 );        // 38 High Power Output Dvr           <- [HPRCOM=SingleEnd][ShortCircuit=OFF]
    WriteAIC(  TLV320AIC3106_LEFTDACVOLCTRL, 0 );       // 43  Left DAC Digital Volume        <- [Mute=OFF][Gain=0dB]
    WriteAIC(  TLV320AIC3106_RIGHTDACVOLCTRL, 0 );      // 44 Right DAC Digital Volume        <- [Mute=OFF][Gain=0dB]
    WriteAIC(  TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL, 0 ); // 58           HPLCOM Output         <- []
    WriteAIC(  TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL, 0 ); // 72           HPRCOM Output         <- []
    WriteAIC(  TLV320AIC3106_DACL1TOLEFTLOP, 0x80 );    // 82 DAC_L1 to LEFT_LOP/M Volume     <- [Routed]
    WriteAIC(  TLV320AIC3106_LEFTLOPMLVLCTRL, 0x09 );   // 86           LEFT_LOP/M Output     <- [Mute=OFF][Power=ON]
    WriteAIC(  TLV320AIC3106_DACR1TORIGHTLOP, 0x80 );   // 92 DAC_R1 to RIGHT_LOP/M Volume    <- [Routed]
    WriteAIC(  TLV320AIC3106_RIGHTLOPMLVLCTRL, 0x09 );  // 93           RIGHT_LOP/M Output    <- [Mute=OFF][Power=ON]
    WriteAIC(  TLV320AIC3106_DACL1TOHPLOUTCTRL, 0x00 ); // 47 DAC_L1 to HPLOUT Volume         <- [Routed]
    WriteAIC(  TLV320AIC3106_HPLOUTLVLCTRL, 0x00 );     // 51           HPLOUT Output         <- [Mute=OFF][Power=ON]
    WriteAIC(  TLV320AIC3106_DACR1HPROUTVOL, 0x00 );    // 64 DAC_R1 to HPROUT Volume         <- [Routed]
    WriteAIC(  TLV320AIC3106_HPROUTLVLCTRL, 0x00 );     // 65           HPROUT Output         <- [Mute=OFF][Power=ON]

    m_OutputDevice = WAV_SET_OUTPUT_LINEOUT;

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableLineOut\r\n"));
}

void
TLV320AIC3106CodecConfig::EnableOutput(
                                       DWORD output
                                       )
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::EnableOutput (output=%d)\r\n", output));

    if(output == WAV_SET_OUTPUT_HDP)
    {
        EnableHPOut_PLL();
    }
    else if(output == WAV_SET_OUTPUT_LINEOUT)
    {
        EnableLineOut();
    }
    else
    {
    }

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::EnableInput\r\n"));
}

void
TLV320AIC3106CodecConfig::GetAicReg(
                                       DWORD *aic_reg  /* [In]=reg num; [Out]=reg val */
                                       )
{
    UINT8 reg = (UINT8)(*aic_reg);
    UINT8 data;

    ReadAIC(reg, &data);

    *aic_reg = (DWORD)data;

    RETAILMSG( 1,
        (L"-TLV320AIC3106CodecConfig::GetAicReg reg=%d data=0x%02X\r\n", reg, data));
}

BOOL
TLV320AIC3106CodecConfig::HandleMessage(
                                        DWORD  dwCode,
                                        PBYTE  pBufIn,
                                        DWORD  dwLenIn,
                                        PBYTE  pBufOut,
                                        DWORD  dwLenOut,
                                        PDWORD pdwActualOut
                                        )
{
    DEBUGMSG( ZONE_FUNCTION,
        (L"+TLV320AIC3106CodecConfig::HandleMessage (dwCode=0x%x)\r\n", dwCode));

    BOOL bRetVal = FALSE;

    switch(dwCode) {
        case WAVIOCTL_SET_OUTPUT:
            if(pBufIn == NULL)
                break;

            EnableOutput(*((DWORD*)pBufIn));
            bRetVal = TRUE;
            break;

        case WAVIOCTL_GET_OUTPUT:
            if(pBufOut == NULL)
                break;

            GetOutput((DWORD*)pBufOut);
            bRetVal = TRUE;
            break;

        case WAVIOCTL_SET_INPUT:
            if(pBufIn == NULL)
                break;

            EnableInput(*((DWORD*)pBufIn));
            bRetVal = TRUE;
            break;

        case WAVIOCTL_GET_INPUT:
            if(pBufOut == NULL)
                break;

            GetInput((DWORD*)pBufOut);
            bRetVal = TRUE;
            break;

        case WAVIOCTL_GET_AIC_REG:
            if(pBufOut == NULL)
                break;

            *pBufOut = *pBufIn;
            GetAicReg((DWORD*)pBufOut);
            bRetVal = TRUE;
            break;

    }

    DEBUGMSG( ZONE_FUNCTION,
        (L"-TLV320AIC3106CodecConfig::HandleMessage"));

    return bRetVal;
}


#if AIC_DEBUG
void TLV320AIC3106CodecConfig::DumpHPOutRegs(void)
{
    UINT8 val = 0xFF;

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_PLLPROGA,      &val);       // 3 PLL A 
    RETAILMSG(1,(L" %d PLLPROGA %02X\n\r",  TLV320AIC3106_PLLPROGA, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_PLLPROGB,      &val);       // 4 PLL B    
    RETAILMSG(1,(L" %d PLLPROGB %02X\n\r",  TLV320AIC3106_PLLPROGB, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_PLLPROGC,      &val);       // 5 PLL C    
    RETAILMSG(1,(L" %d PLLPROGC %02X\n\r",  TLV320AIC3106_PLLPROGC, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_PLLPROGD,      &val);       // 6 PLL D                 
    RETAILMSG(1,(L" %d PLLPROGD %02X\n\r",  TLV320AIC3106_PLLPROGD, val));

    val = 0xFF;
    ReadAIC(  TLV3201AIC3106_CODECDATPATH, &val);       // 7 Codec Datapath Setup  
    RETAILMSG(1,(L" %d CODECDATPATH %02X\n\r",  TLV3201AIC3106_CODECDATPATH, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_ASDICTRLA,     &val);       // 8  Audio Interface Control A 
    RETAILMSG(1,(L" %d ASDICTRLA %02X\n\r",  TLV320AIC3106_ASDICTRLA, val));

    val = 0xFF;
	ReadAIC(  TLV320AIC3106_ASDICTRLB,     &val);       // 9  Audio Interface Control B 
    RETAILMSG(1,(L" %d ASDICTRLB %02X\n\r",  TLV320AIC3106_ASDICTRLB, val));

    val = 0xFF;
	ReadAIC(  TLV320AIC3106_ASDICTRLC,     &val);       // 10 Audio Interface Control C 
    RETAILMSG(1,(L" %d ASDICTRLC %02X\n\r",  TLV320AIC3106_ASDICTRLC, val));

    val = 0xFF;
	ReadAIC(  TLV320AIC3106_CDCOVRFLOWFLG,     &val);       // 10 Audio Interface Control C 
    RETAILMSG(1,(L" %d CDCOVRFLOWFLG %02X\n\r",  TLV320AIC3106_CDCOVRFLOWFLG, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LEFTADCPGAGAINCTL,    &val );   // 15  Left ADC PGA Gain         
    RETAILMSG(1,(L" %d LEFTADCPGAGAINCTL %02X\n\r",  TLV320AIC3106_LEFTADCPGAGAINCTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_RIGHTADCPGAGAINCTL,   &val );   // 16  Right ADC PGA Gain         
    RETAILMSG(1,(L" %d RIGHTADCPGAGAINCTL %02X\n\r",  TLV320AIC3106_RIGHTADCPGAGAINCTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LINE1LADCLCTL,        &val );   // 19  LINE1L to  Left ADC       
    RETAILMSG(1,(L" %d LINE1LADCLCTL %02X\n\r",  TLV320AIC3106_LINE1LADCLCTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LINE1RADCRCTL,        &val );   // 22  LINE1R to Right ADC       
    RETAILMSG(1,(L" %d LINE1RADCRCTL %02X\n\r",  TLV320AIC3106_LINE1RADCRCTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LEFTAGCCTLB,          &val );   // 27  Left AGC B                
    RETAILMSG(1,(L" %d LEFTAGCCTLB %02X\n\r",  TLV320AIC3106_LEFTAGCCTLB, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_RIGHTAGCCTLB,         &val );   // 30  Right AGC B                
    RETAILMSG(1,(L" %d RIGHTAGCCTLB %02X\n\r",  TLV320AIC3106_RIGHTAGCCTLB, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_DACPWROUTDRVCTRL,     &val );   // 37  DAC Power & Output Dvr     
    RETAILMSG(1,(L" %d DACPWROUTDRVCTRL %02X\n\r",  TLV320AIC3106_DACPWROUTDRVCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_HPRCOM_CTL,           &val );   // 38  High Power Output Dvr      
    RETAILMSG(1,(L" %d HPRCOM_CTL %02X\n\r",  TLV320AIC3106_HPRCOM_CTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LEFTDACVOLCTRL,       &val );   // 43  Left DAC Digital Volume    
    RETAILMSG(1,(L" %d LEFTDACVOLCTRL %02X\n\r",  TLV320AIC3106_LEFTDACVOLCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_RIGHTDACVOLCTRL,      &val );   // 44 Right DAC Digital Volume    
    RETAILMSG(1,(L" %d RIGHTDACVOLCTRL %02X\n\r",  TLV320AIC3106_RIGHTDACVOLCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_DACL1TOHPLOUTCTRL,    &val );   // 47 DAC_L1 to HPLOUT Volume     
    RETAILMSG(1,(L" %d DACL1TOHPLOUTCTRL %02X\n\r",  TLV320AIC3106_DACL1TOHPLOUTCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_HPLOUTLVLCTRL,        &val );   // 51           HPLOUT Output     
    RETAILMSG(1,(L" %d HPLOUTLVLCTRL %02X\n\r",  TLV320AIC3106_HPLOUTLVLCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL, &val );   // 58           HPLCOM Output     
    RETAILMSG(1,(L" %d HPLCOM_OUTPUTLVL_CTL %02X\n\r",  TLV320AIC3106_HPLCOM_OUTPUTLVL_CTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_DACR1HPROUTVOL,       &val );   // 64 DAC_R1 to HPROUT Volume     
    RETAILMSG(1,(L" %d DACR1HPROUTVOL %02X\n\r",  TLV320AIC3106_DACR1HPROUTVOL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_HPROUTLVLCTRL,        &val );   // 65           HPROUT Output     
    RETAILMSG(1,(L" %d HPROUTLVLCTRL %02X\n\r",  TLV320AIC3106_HPROUTLVLCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL, &val );   // 72           HPRCOM Output     
    RETAILMSG(1,(L" %d HPRCOM_OUTPUTLVL_CTL %02X\n\r",  TLV320AIC3106_HPRCOM_OUTPUTLVL_CTL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_DACL1TOLEFTLOP,       &val );   // 82 DAC_L1 to LEFT_LOP/M Volume 
    RETAILMSG(1,(L" %d DACL1TOLEFTLOP %02X\n\r",  TLV320AIC3106_DACL1TOLEFTLOP, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_LEFTLOPMLVLCTRL,      &val );   // 86           LEFT_LOP/M Output 
    RETAILMSG(1,(L" %d LEFTLOPMLVLCTRL %02X\n\r",  TLV320AIC3106_LEFTLOPMLVLCTRL, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_DACR1TORIGHTLOP,      &val );   // 92 DAC_R1 to RIGHT_LOP/M Volume
    RETAILMSG(1,(L" %d DACR1TORIGHTLOP %02X\n\r",  TLV320AIC3106_DACR1TORIGHTLOP, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_RIGHTLOPMLVLCTRL,     &val );   // 93           RIGHT_LOP/M Output
    RETAILMSG(1,(L" %d RIGHTLOPMLVLCTRL %02X\n\r",  TLV320AIC3106_RIGHTLOPMLVLCTRL, val));


    val = 0xFF;
    ReadAIC(  TLV320AIC3106_ADDGPIOCTLB, &val );       // 101 GPIO Control Register B  
    RETAILMSG(1,(L" %d ADDGPIOCTLB %02X\n\r",  TLV320AIC3106_ADDGPIOCTLB, val));

    val = 0xFF;
    ReadAIC(  TLV320AIC3106_CLKGENCTL,   &val );       // 102 Clock Generation Control 
    RETAILMSG(1,(L" %d CLKGENCTL %02X\n\r",  TLV320AIC3106_CLKGENCTL, val));

}
#endif


