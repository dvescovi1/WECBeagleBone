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
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#include <windows.h>
#include <oal.h>
#include <oalex.h>

//------------------------------------------------------------------------------

#define dimof(x)    (sizeof(x)/sizeof((x)[0]))

//------------------------------------------------------------------------------

typedef enum {
    ARCH_4 = 1,
    ARCH_4T,
    ARCH_5,
    ARCH_5T,
    ARCH_5TE,
    ARCH_5TEJ,
    ARCH_6,
    ARCH_7
} ArmArchitecture_e;

// Layout of the ID Code register
typedef union {
    UINT32 code;
    struct {
        UINT32 revision :  4;
        UINT32 partNumber : 12;
        UINT32 architecture : 4;
        UINT32 variant : 4;
        UINT32 implementor : 8;
    };
} IdCode_t;

// Map a feature to a bitfield of architectures that have that feature.
typedef struct {
    UINT32 feature;
    UINT32 mask;
}  FeatureToArchitectureMask_t;

static const FeatureToArchitectureMask_t s_featureToArchitectureMask[] = {
    { 
        PF_ARM_V4,        
        (1 << ARCH_4) | (1 << ARCH_4T) |
        (1 << ARCH_5) | (1 << ARCH_5T) | (1 << ARCH_5TE) | (1 << ARCH_5TEJ) |
        (1 << ARCH_6) | (1 << ARCH_7)
    }, { 
        PF_ARM_V5,
        (1 << ARCH_5) | (1 << ARCH_5T) | (1 << ARCH_5TE) | (1 << ARCH_5TEJ) |
        (1 << ARCH_6) | (1 << ARCH_7)
    }, {
        PF_ARM_V6,
        (1 << ARCH_6) | (1 << ARCH_7) 
    }, {
        PF_ARM_V7,
        (1 << ARCH_7) 
    }, {
        PF_ARM_THUMB,
        (1 << ARCH_4T) | (1 << ARCH_5T) | (1 << ARCH_5TE) | (1 << ARCH_5TEJ) |
        (1 << ARCH_6)  | (1 << ARCH_7) 
    }, { 
        PF_ARM_JAZELLE,   
        (1 << ARCH_5TEJ) | (1 << ARCH_6)
    }, { 
        PF_ARM_WRITE_BACK_CACHE,   
        (1 << ARCH_6) | (1 << ARCH_7)
    }, { 
        PF_ARM_VFP_SINGLE_PRECISION,   
        (1 << ARCH_6) | (1 << ARCH_7)
    }, { 
        PF_ARM_VFP_DOUBLE_PRECISION,   
        (1 << ARCH_6) | (1 << ARCH_7)
    }, { 
        PF_ARM_L2CACHE_COPROC,   
        (1 << ARCH_7)
    }, { 
        PF_ARM_NEON,   
        (1 << ARCH_7)
    }
};

//------------------------------------------------------------------------------

extern UINT32 OALGetIdCode();

//------------------------------------------------------------------------------
//
//  Function:  OALIsProcessorFeaturePresent
//
//  Called to determine the processor's supported feature set.
//
BOOL
OALIsProcessorFeaturePresent(
    DWORD feature
    )
{
    BOOL rc = FALSE;
    IdCode_t idCode;
    ULONG idx;

    // Cortex-A8 doesn't support feature query by architecture ID anymore
    // Returning ARM features supported by OMAP35XX
    idCode.code = OALGetIdCode();
    idCode.architecture = ARCH_7;

    OALMSG(OAL_INFO&&OAL_VERBOSE, (
        L"OALIsProcessorFeaturePresent: (0x%08x) %c 0x%02x 0x%03x 0x%03x 0x%02x\r\n",
        idCode.code, idCode.implementor, idCode.variant, idCode.architecture, 
        idCode.partNumber, idCode.revision
        ));
    
    // Is it possible that this feature can be supported?
    for (idx = 0; idx < dimof(s_featureToArchitectureMask); ++idx)
        {
        if (s_featureToArchitectureMask[idx].feature == feature) break;
        }

    if (idx < dimof(s_featureToArchitectureMask))
        {
        // Yes, so now see if this CPU supports this feature.
        UINT32 mask = (1 << idCode.architecture);
        rc = (s_featureToArchitectureMask[idx].mask & mask) != 0;
        }
    return rc;
}

//------------------------------------------------------------------------------

