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
//------------------------------------------------------------------------------
//
//  File: keypad_drv.c
//
#include "bsp.h"
#include <winuserm.h>
#include "gpio_keypad.h"
#include "gpio.h"

#ifndef dimof
#define dimof(x)            (sizeof(x)/sizeof(x[0]))
#endif

//------------------------------------------------------------------------------

const GPIO_KEY g_keypadVK[] = { 
    {1,  VK_LEFT  },
    {2,  VK_RIGHT },
    {3,  VK_UP    },
    {4,  VK_DOWN  },
    {5,  VK_RETURN},
};

//------------------------------------------------------------------------------

const UCHAR g_wakeupVKeys[]     = { VK_PERIOD };
const int g_nbWakeupVKeys = dimof(g_wakeupVKeys);


//------------------------------------------------------------------------------

static const UCHAR off[]     = { VK_PERIOD };
static const KEYPAD_REMAP_ITEM remapItems[] = {
    { VK_OFF, dimof(off),     3000, off     },
};

const KEYPAD_REMAP g_keypadRemap = { 0, /*dimof(remapItems),*/ remapItems };

//------------------------------------------------------------------------------

static const UCHAR softkeys[] = { VK_CONTROL };

static const KEYPAD_REPEAT_BLOCK softkeyBlock = { dimof(softkeys), softkeys };

static const KEYPAD_REPEAT_ITEM repeatItems[] = {
    {VK_LEFT,             500, 500, TRUE,  &softkeyBlock },
    {VK_RIGHT,            500, 500, TRUE,  &softkeyBlock },
    {VK_UP,               500, 500, TRUE,  &softkeyBlock },
    {VK_DOWN,             500, 500, TRUE,  &softkeyBlock},
    {VK_RETURN,			  500, 500, TRUE,  NULL },
};

const KEYPAD_REPEAT g_keypadRepeat = { dimof(repeatItems), repeatItems };

