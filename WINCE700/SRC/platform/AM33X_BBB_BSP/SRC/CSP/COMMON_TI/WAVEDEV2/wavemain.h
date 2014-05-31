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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

#include <windows.h>
#include <mmreg.h>
#include <types.h>
#include <memory.h>
#include <string.h>
#include <nkintr.h>
#include <excpt.h>
#include <wavedbg.h>
#include <wavedev.h>
#include <waveddsi.h>
#include <mmddk.h>
#include <devload.h>

#pragma warning (push)
#pragma warning (disable: 4127)
#include <linklist.h>
#pragma warning (pop)

#include <audiosys.h>
#include <wfmtmidi.h>
#include <memtxapi.h>

#include <pm.h>

class StreamContext;
class WaveStreamContext;
class InputStreamContext;
class OutputStreamContext;
class DeviceContext;
class MixerDeviceContext;
class InputDeviceContext;
class OutputDeviceContext;
class HardwareContext;

/*
Making the driver pageable:
To make the entire driver pageable, set the following flag in the driver's registry key:

[HKEY_LOCAL_MACHINE\Drivers\BuiltIn\Audio]
    "Flags"=dword:2    <-- Add this

If performance issues in the IST thread are encountered due to paging, it is possible to make portions of the driver
pagelocked while the remainder of the driver remains pageable:

The following pragma defines a section called "PageLocked" which is set to non-pageable.

#pragma comment(linker, "/SECTION:PageLocked,ER!P")

In the source code, to make a section of code nonpageable, put the following line before the code:

#pragma code_seg("PageLocked")

Afterward, you may use the following line to force following code to be placed back in the default
text section:

#pragma code_seg()

Note that there may still be some performance issues if the IST is blocked on a critical section held by other driver
code which is itself pageable. If this is a problem, it's probably easiest to leave the entire driver as unpageable.

FWIW, the wave driver is generally fairly small, so the advantage of making it pageable is minimal.
*/

EXTERN_C void EnterMutex();
EXTERN_C void ExitMutex();

#include "wavepdd.h"
#include "oemsettings.h"
#include "devctxt.h"
#include "strmctxt.h"
#include "midistrm.h"
#include "mixerdrv.h"
#include "aic23.h"
#include "hwctxt.h"

