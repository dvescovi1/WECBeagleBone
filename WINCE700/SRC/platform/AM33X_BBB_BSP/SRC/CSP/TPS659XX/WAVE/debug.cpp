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
//  Debugging support.
//


#include "omap.h"
#include "ceddkex.h"
#include "debug.h"

#ifndef SHIP_BUILD

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"TPS659XX_WAVE", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"Interrupts",  L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif