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

#include "am33x.h"
#include "am33x_config.h"
#include "sdk_padcfg.h"
#include "soc_cfg.h"

static UINT32* g_pPadCfg = NULL;

static _inline void MapPadRegisters()
{
    if (g_pPadCfg == NULL)
        g_pPadCfg = OALPAtoUA(AM33X_SYSC_PADCONFS_REGS_PA);
}

BOOL SOCSetPadConfig(UINT16 padId, UINT16 cfg)
{    
    MapPadRegisters();
    
    //write CONTROL_PADCONF_X. don't worry about the offset it has been checked by upper layers
    OUTREG32(&g_pPadCfg[padId],cfg);   

    return TRUE;
}

BOOL SOCGetPadConfig(UINT16 padId, UINT16* pCfg)
{

    MapPadRegisters();

    //read CONTROL_PADCONF_X. don't worry about the offset it has been checked by upper layers
    *pCfg = (INREG32(&g_pPadCfg[padId]) >> 0) & 0x7F;   

    return TRUE;
}

//------------------------------------------------------------------------------

