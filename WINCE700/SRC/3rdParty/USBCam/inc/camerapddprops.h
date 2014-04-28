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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _CAMERAPDDPROPS_H
#define _CAMERAPDDPROPS_H

#define MAX_SUPPORTED_PINS        3
#define MAX_PINNAME_LENGTH       10
#define MAX_VIDEO_FORMAT         10

#define NUM_VIDEOPROCAMP_ITEMS   10
#define NUM_CAMERACONTROL_ITEMS   8
#define NUM_PROPERTY_ITEMS       NUM_VIDEOPROCAMP_ITEMS + NUM_CAMERACONTROL_ITEMS




static const WCHAR g_wszPinNames[MAX_SUPPORTED_PINS][MAX_PINNAME_LENGTH] = { L"Capture"
                                                                            ,L"Still"
                                                                            ,L"Preview"
                                                                            };

static const WCHAR g_wszPinDeviceNames[MAX_SUPPORTED_PINS][MAX_PINNAME_LENGTH] = {L"PIN1:",
                                                                                  L"PIN1:",
                                                                                  L"PIN1:"};

#endif
