// All rights reserved ADENEO EMBEDDED 2010

#ifndef AIC23_H
#define AIC23_H

// AIC23 register addresses
#define		AIC23_LIN_VOL_CTRL_ADDR			0x00 
#define		AIC23_RIN_VOL_CTRL_ADDR			0x01
#define		AIC23_LHEAD_VOL_CTRL_ADDR		0x02
#define		AIC23_RHEAD_VOL_CTRL_ADDR		0x03
#define		AIC23_ANALOG_PATH_CTRL_ADDR		0x04
#define		AIC23_DIGITAL_PATH_CTRL_ADDR	0x05
#define		AIC23_PWR_DOWN_CTRL_ADDR		0x06
#define		AIC23_DIGITAL_FORMAT_ADDR		0x07
#define		AIC23_SAMPLE_RATE_CTRL_ADDR		0x08
#define		AIC23_DIGITAL_ACT_ADDR			0x09
#define		AIC23_RESET_REG_ADDR			0x0F

#define		AIC23_PWR_DOWN_LINE				(1 << 0)
#define		AIC23_PWR_DOWN_MIC				(1 << 1)
#define		AIC23_PWR_DOWN_ADC				(1 << 2)
#define		AIC23_PWR_DOWN_DAC				(1 << 3)

#define		AIC23_ANALOG_PATH_MICB			(1 << 0)
#define		AIC23_ANALOG_PATH_MICM			(1 << 1)
#define		AIC23_ANALOG_PATH_INSEL			(1 << 2)

class CAIC23
{
	void*	m_hI2CDevice;
	UINT16	m_dwPwrDownShadowReg;	
	UINT16	m_dwAudioPathShadowReg;
	BOOL	m_bMicrophone;
public:
		 CAIC23();
		 ~CAIC23();

	BOOL InitHardware(DWORD I2CBrIndex, DWORD I2Cbus, DWORD I2Caddr);
	BOOL OutputEnable(BOOL bEnable);
	BOOL InputEnable(BOOL bEnable);
	BOOL SetOutputVolume(DWORD bLeft, UINT16 val);
	BOOL SetInputVolume(DWORD bLeft, UINT16 val);
	BOOL SetLineInMux(BOOL bMic);
	BOOL SetMicBoost(BOOL bMicBoost);

private:
	BOOL	ResetCodec();
	BOOL	ConfigureCodec();
	BOOL	SetSampleRate(DWORD val);
	BYTE	I2CReadRegister(BYTE reg);
	BOOL	I2CWriteRegister(BYTE reg, UINT16 val);
};

#endif  // AIC23_H