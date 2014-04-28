// All rights reserved ADENEO EMBEDDED 2010
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
#pragma once
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.


Module Name:    OEMSETTINGS.H

Abstract:       Platform dependent code for the mixing audio driver.

-*/

#define OUTCHANNELS (2)
#define INCHANNELS  (2)
#define MAXCHANNELS (2)

#define BITSPERSAMPLE (16)

typedef INT16 HWSAMPLE;
typedef HWSAMPLE *PHWSAMPLE;

// Set USE_MIX_SATURATE to 1 if you want the mixing code to guard against saturation
// This costs a couple of instructions in the inner loop
#define USE_MIX_SATURATE (1)

// The code will use the follwing values as saturation points
#define AUDIO_SAMPLE_MAX    (32767)
#define AUDIO_SAMPLE_MIN    (-32768)

// The following define the maximum attenuations for the SW volume controls in devctxt.cpp
// e.g. 100 => range is from 0dB to -100dB
#define STREAM_ATTEN_MAX       100
#define DEVICE_ATTEN_MAX        35
#define CLASS_ATTEN_MAX         35

// If set to 1, all gain will be mono (left volume applied to both channels)
#define MONO_GAIN 0

// If set to 1, midi code will be included in the driver (~4k)
#define ENABLE_MIDI 1

// If set to 1, the driver won't do internal mixing and will rely on the OS mixer
#define USE_OS_MIXER 1

