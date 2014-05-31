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

#include "precomp.h"


//------------------------------------------------------------------------------
//  Globals

extern DDGPE * g_pGPE;

//------------------------------------------------------------------------------
//
//  GPE-based display driver entry point
//

GPE *
GetGPE()
{
    if (!g_pGPE)
    {
        g_pGPE = new OMAPDDGPE();
    }

    return g_pGPE;
}

