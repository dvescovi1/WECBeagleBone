//
// Copyright (c) MPC Data Limited 2009. All Rights Reserved.
// Copyright (c) Texas Instruments Inc. 2009. All Rights Reserved.
//
// File:  wavioctl.h
//
// WAVEDEV2 IOCTLs definitions.
//

#include <windev.h>

#ifndef __WAVIOCTL_H__
#define __WAVIOCTL_H__

// Output ports for use with WAV_SET_OUTPUT and WAV_GET_OUTPUT
#define WAV_SET_OUTPUT_HDP          1
#define WAV_SET_OUTPUT_LINEOUT      2

// Input ports for use with WAV_SET_INPUT and WAV_GET_INPUT
#define WAV_SET_INPUT_MIC3          (1 << 4)
#define WAV_SET_INPUT_LINE1         (2 << 4)


// IOCTL to set the output port on the CODEC.
// Input parameters:
//   DWORD - WAV_SET_OUTPUT_xxx
#define WAV_SET_OUTPUT              2048

// IOCTL to get the current output port on the CODEC.
// Output parameters:
//   DWORD - returns WAV_SET_OUTPUT_xxx
#define WAV_GET_OUTPUT              2049

// IOCTL to set the input port on the CODEC.
// Input parameters:
//   DWORD - WAV_SET_INPUT_xxx
#define WAV_SET_INPUT               2050

// IOCTL to get the current input port on the CODEC.
// Output parameters:
//   DWORD - returns WAV_SET_INPUT_xxx
#define WAV_GET_INPUT               2051

// IOCTL to set the input gain on the CODEC.
// Input parameters:
//   DWORD - bits[0-15] left volume, bits[16-31] right volume
#define WAV_SET_INPUT_VOLUME        2052

// IOCTL to get the input gain on the CODEC.
// Output parameters:
//   DWORD - bits[0-15] left volume, bits[16-31] right volume
#define WAV_GET_INPUT_VOLUME        2053

// IOCTL to get the AIC reg value (for DEBUG).
// In/Out parameter:
//   [In] - reg number; [Out] - reg value
#define WAV_GET_AIC_REG             2054

#define WAV_GET_MCASP_REG           2055
#define WAV_SHOW_WDM                2056
#define WAV_SHOW_DMA_BUF            2057
#define WAV_SHOW_INTR_TIME          2058

// IOCTL defintions
#define WAVIOCTL_SET_OUTPUT         CTL_CODE(FILE_DEVICE_SOUND, WAV_SET_OUTPUT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_GET_OUTPUT         CTL_CODE(FILE_DEVICE_SOUND, WAV_GET_OUTPUT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_SET_INPUT          CTL_CODE(FILE_DEVICE_SOUND, WAV_SET_INPUT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_GET_INPUT          CTL_CODE(FILE_DEVICE_SOUND, WAV_GET_INPUT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_SET_INPUT_VOLUME   CTL_CODE(FILE_DEVICE_SOUND, WAV_SET_INPUT_VOLUME, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_GET_INPUT_VOLUME   CTL_CODE(FILE_DEVICE_SOUND, WAV_GET_INPUT_VOLUME, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_GET_AIC_REG        CTL_CODE(FILE_DEVICE_SOUND, WAV_GET_AIC_REG, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_GET_MCASP_REG      CTL_CODE(FILE_DEVICE_SOUND, WAV_GET_MCASP_REG, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_SHOW_WDM           CTL_CODE(FILE_DEVICE_SOUND, WAV_SHOW_WDM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_SHOW_DMA_BUF       CTL_CODE(FILE_DEVICE_SOUND, WAV_SHOW_DMA_BUF, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define WAVIOCTL_SHOW_INTR_TIME     CTL_CODE(FILE_DEVICE_SOUND, WAV_SHOW_INTR_TIME, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // __WAVIOCTL_H__
