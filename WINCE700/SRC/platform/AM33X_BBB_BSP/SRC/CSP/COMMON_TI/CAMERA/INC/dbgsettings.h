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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _DBGSETTINGS_H
#define _DBGSETTINGS_H

//#ifndef DEBUG
//#define DEBUG                // always turn on debug output
//#endif // DEBUG

//#ifdef DEBUG

#define DEBUGMASK(bit)       (1 << (bit))

#define MASK_ERROR           DEBUGMASK(0)
#define MASK_WARN            DEBUGMASK(1)
#define MASK_INIT            DEBUGMASK(2)
#define MASK_FUNCTION        DEBUGMASK(3)
#define MASK_IOCTL           DEBUGMASK(4)
#define MASK_DEVICE          DEBUGMASK(5)
#define MASK_SCM             DEBUGMASK(6)
#define MASK_H3A             DEBUGMASK(7)

#define MASK_PDDCAM          DEBUGMASK(8)
#define MASK_IRQ             DEBUGMASK(9)
#define MASK_I2C             DEBUGMASK(10)
#define MASK_VERBOSE         DEBUGMASK(11)
#define MASK_ACTIVITY        DEBUGMASK(12)
#define MASK_DMA             DEBUGMASK(13)
#define MASK_PDDINT          DEBUGMASK(14)

#define PIN_REG_PATH         TEXT("Software\\Microsoft\\DirectX\\DirectShow\\Capture")

#undef DEBUGMSG
#define DEBUGMSG RETAILMSG
#ifdef CAMINTERFACE
extern "C" 
DBGPARAM dpCurSettings = {
    _T("Camera"), 
    {
        _T("Errors"), _T("Warnings"), _T("Init"), _T("Function"), 
        _T("Ioctl"), _T("Device"), _T("SCM"), _T("H3A"),
        _T("PDDCAM"),_T("IRQ"),_T("I2c"),_T("Verbose"),
        _T("ACTIVITY"),_T("DMA"),_T("PDDINT"),_T("") 
    },
    MASK_ERROR | MASK_WARN | MASK_DMA
}; 
#else
extern "C" DBGPARAM dpCurSettings;
#endif

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INIT
#undef ZONE_H3A
#undef ZONE_SCM

#define ZONE_ERROR           DEBUGZONE(0)
#define ZONE_WARN            DEBUGZONE(1)
#define ZONE_INIT            DEBUGZONE(2)
#define ZONE_FUNCTION        DEBUGZONE(3)
#define ZONE_IOCTL           DEBUGZONE(4)
#define ZONE_DEVICE          DEBUGZONE(5)
#define ZONE_SCM             DEBUGZONE(6)
#define ZONE_H3A             DEBUGZONE(7)

#define ZONE_PDDCAM          DEBUGZONE(8)
#define ZONE_IRQ             DEBUGZONE(9)
#define ZONE_I2C             DEBUGZONE(10)
#define ZONE_VERBOSE         DEBUGZONE(11)
#define ZONE_ACTIVITY        DEBUGZONE(12)
#define ZONE_DMA             DEBUGZONE(13)
#define ZONE_PDDINT          DEBUGZONE(14)

//#endif
#endif
