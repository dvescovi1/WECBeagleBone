//-------------------------------------------------------------------------
// <copyright file="debug.h" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    MJPEG DirectShow Filter for use with USB camera driver for Windows 
//    Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#ifndef _DEBUG_H
#define _DEBUG_H

#include <windows.h>

#define ZONEID_ERROR        0
#define ZONEID_WARNING      1
#define ZONEID_INFO_MIN     2
#define ZONEID_INFO_MAX     3
#define ZONEID_INIT         4
#define ZONEID_ENTER        5
#define ZONEID_TIMING       6
#define ZONEID_MEMORY       7
#define ZONEID_LOCKING      8
#define ZONEID_MTYPES       9
#define ZONEID_LIPSYNCSTATS 10
#define ZONEID_REFCOUNT     11
#define ZONEID_PERF         12
#define ZONEID_INFO1        13
#define ZONEID_HEURISTIC    14
#define ZONEID_PAUSEBUG     15

#define ZONEMASK_ERROR      (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING    (1 << ZONEID_WARNING)
#define ZONEMASK_INFO_MIN   (1 << ZONEID_INFO_MIN)
#define ZONEMASK_INFO_MAX   (1 << ZONEID_INFO_MAX)
#define ZONEMASK_INIT       (1 << ZONEID_INIT)
#define ZONEMASK_ENTER      (1 << ZONEID_ENTER)
#define ZONEMASK_TIMING     (1 << ZONEID_TIMING)
#define ZONEMASK_MEMORY     (1 << ZONEID_MEMORY)
#define ZONEMASK_LOCKING    (1 << ZONEID_LOCKING)
#define ZONEMASK_MTYPES     (1 << ZONEID_MTYPES)
#define ZONEMASK_LIPSYNCSTATS   (1 << ZONEID_LIPSYNCSTATS)
#define ZONEMASK_REFCOUNT   (1 << ZONEID_REFCOUNT)
#define ZONEMASK_PERF       (1 << ZONEID_PERF)
#define ZONEMASK_INFO1      (1 << ZONEID_INFO1)
#define ZONEMASK_HEURISTIC  (1 << ZONEID_HEURISTIC)
#define ZONEMASK_PAUSEBUG   (1 << ZONEID_PAUSEBUG)

extern DBGPARAM dpCurSettings;

#endif // _DEBUG_H
