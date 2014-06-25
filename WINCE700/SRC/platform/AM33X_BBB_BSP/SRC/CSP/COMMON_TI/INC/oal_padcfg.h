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
//  File:  oal_padcfg.h
//
//
#ifndef __OAL_PADCFG_H
#define __OAL_PADCFG_H

#include "omap.h"
#include "sdk_padcfg.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL    (*pfnRequestPad)(UINT16 padid);
    BOOL    (*pfnReleasePad)(UINT16 padid);    
    BOOL    (*pfnConfigurePad)(UINT16 padId,UINT16 cfg);
    BOOL    (*pfnGetpadConfiguration)(UINT16 padId,UINT16* pCfg);   
    BOOL    (*pfnRequestAndConfigurePad)(UINT16 padId,UINT16 pCfg);
    BOOL    (*pfnRequestAndConfigurePadArray)(const PAD_INFO* padArray);
    BOOL    (*pfnReleasePadArray)(const PAD_INFO* padArray);
    BOOL    (*pfnRequestDevicePads)(OMAP_DEVICE device);
    BOOL    (*pfnReleaseDevicePads)(OMAP_DEVICE device);
} OAL_IFC_PADCFG;


BOOL OALPadCfgInit();
BOOL OALPadCfgPostInit();

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __OAL_PADCFG_H
