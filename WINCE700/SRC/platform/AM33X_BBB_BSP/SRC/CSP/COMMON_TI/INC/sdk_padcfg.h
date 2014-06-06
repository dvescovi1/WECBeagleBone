// All rights reserved ADENEO EMBEDDED 2010
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
//
//  File:  sdk_padcfg.h
//
//
#ifndef __SDK_PADCFG_H
#define __SDK_PADCFG_H

#include "omap.h"


#ifdef __cplusplus
extern "C" {
#endif


#define INPUT_ENABLED           (1<<0)
#define INPUT_DISABLED          (0<<0)
#define PULL_RESISTOR_ENABLED   (1<<1)
#define PULL_RESISTOR_DISABLED  (0<<1)
#define PULLUP_RESISTOR         (1<<2)
#define PULLDOWN_RESISTOR       (0<<2)
#define SLEWCTRL_SLOW			(1<<7)
#define SLEWCTRL_FAST			(0<<7)

/* OMAP and Netra has 3 bits for MuxMode, Centarus has 8 bits */
#define MUXMODE(x)              ((x & 0xf)<<3)

    
typedef struct {
    UINT16 padID;
    unsigned int Cfg:15;    
    unsigned int inUse:1;
} PAD_INFO;

#define END_OF_PAD_ARRAY {(UINT16) -1,0,0}


BOOL RequestPad(UINT16 padid);
BOOL ReleasePad(UINT16 padid);
BOOL ConfigurePad(UINT16 padId,UINT16 cfg);
BOOL GetPadConfiguration(UINT16 padId,UINT16* pCfg);
BOOL RequestAndConfigurePad(UINT16 padId,UINT16 cfg);
BOOL RequestAndConfigurePadArray(const PAD_INFO* padArray);
BOOL ReleasePadArray(const PAD_INFO* padArray);
BOOL RequestDevicePads(OMAP_DEVICE device);
BOOL ReleaseDevicePads(OMAP_DEVICE device);
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __SDK_PADCFG_H
