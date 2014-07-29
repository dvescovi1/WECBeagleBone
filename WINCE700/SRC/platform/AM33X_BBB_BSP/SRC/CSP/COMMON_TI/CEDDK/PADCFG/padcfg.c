// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  padcfg.cpp
//
//  This file contains DDK library implementation for platform specific
//  pad config operations.
//  
#include "omap.h"
#include "oalex.h"
#include "oal_padcfg.h"
#include "sdk_padcfg.h"

//-----------------------------------------------------------------------------
static OAL_IFC_PADCFG _FnTable;
static BOOL           _init = FALSE;


//-----------------------------------------------------------------------------
static
BOOL
__InitPadCfgfnTable()
{
    if (_init == FALSE)
    {
        // get clock ref counter table from kernel
        if (KernelIoControl(IOCTL_HAL_PADCFGCOPYFNTABLE, (void*)&_FnTable,
            sizeof(OAL_IFC_PADCFG), NULL, 0, NULL))
        {
            _init = TRUE;
        }
        else
        {
            memset(&_FnTable,0,sizeof(_FnTable));
        }
    }
    return _init;
}


BOOL RequestPad(UINT16 padid)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnRequestPad(padid);
}

BOOL ReleasePad(UINT16 padid)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnReleasePad(padid);
}

BOOL ConfigurePad(UINT16 padId,UINT16 cfg)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnConfigurePad(padId,cfg);
}

BOOL GetPadConfiguration(UINT16 padId,UINT16* pcfg)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnGetpadConfiguration(padId,pcfg);
}

BOOL RequestAndConfigurePad(UINT16 padId,UINT16 cfg)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnRequestAndConfigurePad(padId,cfg);
}

BOOL RequestAndConfigurePadArray(const PAD_INFO* padArray)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnRequestAndConfigurePadArray(padArray);
}


BOOL ReleasePadArray(const PAD_INFO* padArray)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnReleasePadArray(padArray);
}

BOOL ReleaseDevicePads(OMAP_DEVICE device)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnReleaseDevicePads(device);
}


BOOL RequestDevicePads(OMAP_DEVICE device)
{    
    if (__InitPadCfgfnTable() == FALSE) return FALSE;
    return _FnTable.pfnRequestDevicePads(device);
}


//-----------------------------------------------------------------------------
