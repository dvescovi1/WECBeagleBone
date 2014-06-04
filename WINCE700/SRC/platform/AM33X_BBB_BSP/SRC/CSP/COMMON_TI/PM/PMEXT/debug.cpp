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
//  File:  debug.cpp
//
//  This file defines the debug zone data structure.
//
#include <windows.h>
#include <ceddk.h>

#ifndef SHIP_BUILD

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings =
{
    L"OMAP_PMEXT", 
        {
        L"Error",       L"Warning",     L"Function",    L"Info",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined" 
        },
    0x0003
};

#endif