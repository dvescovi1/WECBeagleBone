// All rights reserved ADENEO EMBEDDED 2010

#include <windows.h>
#include <wavedbg.h>
#include <sdk_i2c.h>
#include <soc_cfg.h>

#include "wavemain.h"

//--------------------------------------------------------------------------------

typedef struct {
	DWORD	SampleRate;
	UINT16  RegVal;
} AIC23_SAMPLE_RATE;

static const AIC23_SAMPLE_RATE SampleRatesTab[] = 
{
	{ 96000, 0x1D },
	{ 88200, 0x3F },
	{ 48000, 0x01 },
	{ 44100, 0x23 },
	{ 32000, 0x19 },
	{ 8021,  0x2F }
};

//--------------------------------------------------------------------------------

CAIC23::CAIC23():
m_hI2CDevice(NULL),
m_dwPwrDownShadowReg(0x0007),
m_dwAudioPathShadowReg(0x0010),
m_bMicrophone(FALSE)
{
}

CAIC23::~CAIC23()
{
	if (m_hI2CDevice != NULL)
	{
		I2CClose(m_hI2CDevice);
		m_hI2CDevice = NULL;
	}
}

BOOL CAIC23::InitHardware(DWORD I2CBrIndex, DWORD I2Cbus, DWORD I2Caddr)
{
	BOOL bRet = FALSE;

	// Open I2C bus
	m_hI2CDevice = I2COpen(SOCGetI2CDeviceByBus(I2Cbus));
	if (m_hI2CDevice == NULL)
	{
		ERRMSG((L"CAIC23::InitHardware: Failed to open I2C driver"));
		goto exit;
	}
	I2CSetSlaveAddress(m_hI2CDevice, (UINT16)I2Caddr);
	I2CSetSubAddressMode(m_hI2CDevice, I2C_SUBADDRESS_MODE_8);
	I2CSetBaudIndex(m_hI2CDevice, I2CBrIndex);

	// Reset Codec
	if (!ResetCodec())
	{
		ERRMSG((L"CAIC23::InitHardware: Failed to reset codec"));
		goto exit;
	}

	// Configure Codec
	if (!ConfigureCodec())
	{
        ERRMSG((L"CAIC23::InitHardware: Failed to configure codec"));
		goto exit;
	}

	// Set default sample rate
	if (!SetSampleRate(OUTPUT_SAMPLERATE))
	{
        ERRMSG((L"CAIC23::InitHardware: Failed to configure sample rate"));
		goto exit;
	}

	bRet = TRUE;

exit:

	return bRet;
}

BOOL CAIC23::OutputEnable(BOOL bEnable)
{
	UINT16 val = m_dwPwrDownShadowReg;

	if (bEnable)
	{
		val &= ~AIC23_PWR_DOWN_DAC;
	}
	else
	{
		val |= AIC23_PWR_DOWN_DAC;
	}

	if (!I2CWriteRegister(AIC23_PWR_DOWN_CTRL_ADDR, val))
	{
		return FALSE;
	}

	m_dwPwrDownShadowReg = val;

	return TRUE;
}

BOOL CAIC23::InputEnable(BOOL bEnable)
{
	UINT16 val = m_dwPwrDownShadowReg;

	if (bEnable)
	{
		val &= ~AIC23_PWR_DOWN_ADC;

		if (m_bMicrophone)
		{
			val |= AIC23_PWR_DOWN_LINE;
			val &= ~AIC23_PWR_DOWN_MIC;
		}
		else
		{
			val |= AIC23_PWR_DOWN_MIC;
			val &= ~AIC23_PWR_DOWN_LINE;
		}
	}
	else
	{
		val |= (AIC23_PWR_DOWN_ADC | AIC23_PWR_DOWN_MIC | AIC23_PWR_DOWN_LINE);
	}

	if (!I2CWriteRegister(AIC23_PWR_DOWN_CTRL_ADDR, val))
	{
		return FALSE;
	}

	m_dwPwrDownShadowReg = val;

	return TRUE;
}

BOOL CAIC23::SetLineInMux(BOOL bMic)
{
	UINT16 val = m_dwAudioPathShadowReg;

	if (bMic)
	{
		val |= AIC23_ANALOG_PATH_INSEL;
	}
	else
	{
		val &= ~AIC23_ANALOG_PATH_INSEL;
	}

	if (!I2CWriteRegister(AIC23_ANALOG_PATH_CTRL_ADDR, val))
	{
		return FALSE;
	}	

	m_dwAudioPathShadowReg = val;
	m_bMicrophone = bMic;

	return TRUE;
}

BOOL CAIC23::SetMicBoost(BOOL bMicBoost)
{
	UINT16 val = m_dwAudioPathShadowReg;

	if (bMicBoost)
	{
		val |= AIC23_ANALOG_PATH_MICB;
	}
	else
	{
		val &= ~AIC23_ANALOG_PATH_MICB;
	}

	if (!I2CWriteRegister(AIC23_ANALOG_PATH_CTRL_ADDR, val))
	{
		return FALSE;
	}	

	m_dwAudioPathShadowReg = val;

	return TRUE;
}

BOOL CAIC23::ResetCodec()
{
	return I2CWriteRegister(AIC23_RESET_REG_ADDR, 0);
}

BOOL CAIC23::ConfigureCodec()
{
	BOOL bRet = FALSE;

	// Disable ADC/DAC
	OutputEnable(FALSE);
	InputEnable(FALSE);

	// Select DAC
	if (!I2CWriteRegister(AIC23_ANALOG_PATH_CTRL_ADDR, m_dwAudioPathShadowReg))
	{
		goto exit;
	}

	// De-emphasis 44.1 kHz, High-Pass filter enable
	if (!I2CWriteRegister(AIC23_DIGITAL_PATH_CTRL_ADDR, 0x05))
	{
		goto exit;
	}

	// I2S Slave mode, 16 bit
	if (!I2CWriteRegister(AIC23_DIGITAL_FORMAT_ADDR, 0x02))
	{
		goto exit;
	}

	// Activate digital interface
	if (!I2CWriteRegister(AIC23_DIGITAL_ACT_ADDR, 0x01))
	{
		goto exit;
	}

	bRet = TRUE;

exit:

	return bRet;
}

BOOL CAIC23::SetOutputVolume(DWORD bLeft, UINT16 val)
{
	BOOL bRet = FALSE;
	BYTE reg;

	reg = (BYTE)(((val * 0x4F) / 0xFFFF) + 0x30);
	reg |= (1 << 7);	// Enable Zero-Cross detect
	
	if (!I2CWriteRegister(bLeft ? AIC23_LHEAD_VOL_CTRL_ADDR : AIC23_RHEAD_VOL_CTRL_ADDR, reg))
	{
		goto exit;
	}

	bRet = TRUE;

exit:

	return bRet;
}

BOOL CAIC23::SetInputVolume(DWORD bLeft, UINT16 val)
{
	BOOL bRet = FALSE;
	BYTE reg;

	reg = (BYTE)((val * 0x1F) / 0xFFFF);
	
	if (!I2CWriteRegister(bLeft ? AIC23_LIN_VOL_CTRL_ADDR : AIC23_RIN_VOL_CTRL_ADDR, reg))
	{
		goto exit;
	}

	bRet = TRUE;

exit:

	return bRet;
}

BOOL CAIC23::SetSampleRate(DWORD val)
{
	DWORD index = 0;
	DWORD tabSize = sizeof(SampleRatesTab)/sizeof(SampleRatesTab[0]);

	while (index < tabSize)
	{
		if (val >= SampleRatesTab[index].SampleRate)
		{
			break;
		}
		index++;
	}

	// Make sure we did not pass the last value
	if (index >= tabSize)
	{
		index = tabSize - 1;
	}

	// Configure Sample rate
	if (!I2CWriteRegister(AIC23_SAMPLE_RATE_CTRL_ADDR, SampleRatesTab[index].RegVal))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAIC23::I2CWriteRegister(BYTE reg, UINT16 val)
{
	BOOL bRet = TRUE;
	BYTE Buf = (BYTE)(val & 0xFF);

	reg = (BYTE)((reg << 1) | ((val & 0x0100) >> 8));

	if (I2CWrite(m_hI2CDevice, reg, &Buf, sizeof(Buf)) != sizeof(Buf))
	{
		bRet = FALSE;
	}

	return bRet;
}
