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
#define ENABLE_TRITON_RAW_ACCESS

#include "triton.h"
#include "bsp_cfg.h"
#include "twl.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"



//------------------------------------------------------------------------------
//
//  Function:  TWLReadIDCode
//
//   Return the ID code of tthe triton
//
UINT8 TWLReadIDCode(void* hTwl)
{    
    UINT8 version;
    if (TWLReadRegs(hTwl, TWL_IDCODE_31_24, &version, sizeof(version)))
    {
        return version;
    }
    else
    {
        return (UINT8) -1;
    }
}