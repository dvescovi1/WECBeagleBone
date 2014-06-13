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

//------------------------------------------------------------------------------
//
//  File:  init.c
//
//  This file implements OALCacheGlobalsInit function 
//
#include "omap.h"
#include <oal.h>
#include <oalex.h>


//
//  Defines
//

#define ARMV7_CACHEID_MASK                  0x7
#define ARMV7_CACHEID_NO_CACHE              0x0
#define ARMV7_CACHEID_ICACHE                0x1
#define ARMV7_CACHEID_DCACHE                0x2
#define ARMV7_CACHEID_UCACHE                0x4

#define ARMV7_CACHESIZE_LINESIZE_MASK       0x00000007
#define ARMV7_CACHESIZE_LINESIZE_SHIFT      0

#define ARMV7_CACHESIZE_ASSOCIATIVITY_MASK  0x00001FF8
#define ARMV7_CACHESIZE_ASSOCIATIVITY_SHIFT 3

#define ARMV7_CACHESIZE_NUMSETS_MASK        0x0FFFE000
#define ARMV7_CACHESIZE_NUMSETS_SHIFT       13

#define ARMV7_CACHESIZE_SEL_DATA            0
#define ARMV7_CACHESIZE_SEL_INSTRUCTION     1

#define ARMV7_CACHESIZE_SEL_L1              (0 << 1)
#define ARMV7_CACHESIZE_SEL_L2              (1 << 1)
#define ARMV7_CACHESIZE_SEL_L3              (2 << 1)
#define ARMV7_CACHESIZE_SEL_L4              (3 << 1)
#define ARMV7_CACHESIZE_SEL_L5              (4 << 1)
#define ARMV7_CACHESIZE_SEL_L6              (5 << 1)
#define ARMV7_CACHESIZE_SEL_L7              (6 << 1)
#define ARMV7_CACHESIZE_SEL_L8              (7 << 1)


//
//  Functions
//

DWORD   OALGetCacheType();
DWORD   OALGetCacheLevelID();
DWORD   OALGetCacheSizeID(DWORD cacheID);

//
//  Globals
//

DWORD   g_dwRangeLimit;     //  Threshold on buffer size between clean/flush line-by-line or whole cache


//------------------------------------------------------------------------------
//
//  Function:  OALCacheGlobalsInit
//
//  This function initializes globals variables which holds cache parameters.
//  It must be called before any other cache/TLB function.
//
VOID OALCacheGlobalsInit()
{
    DWORD   dwCacheType,
            dwCacheLevelID,
            dwCacheSize;

    OALMSG(OAL_CACHE&&OAL_VERBOSE, (
        L"+OALCacheGlobalsInit()\r\n"
    ));


    //  Initialize WinCE cache info struct
    memset( &g_oalCacheInfo, 0 , sizeof(g_oalCacheInfo) );


    //  Get cache type
    dwCacheType = OALGetCacheType();

    //  Get cache info
    dwCacheLevelID = OALGetCacheLevelID();


    //  Set L1 cache properties
    g_oalCacheInfo.L1Flags |= ((dwCacheLevelID & ARMV7_CACHEID_MASK) == ARMV7_CACHEID_UCACHE) ? CF_UNIFIED : 0;

    //  L1 data
    dwCacheSize = OALGetCacheSizeID(ARMV7_CACHESIZE_SEL_L1|ARMV7_CACHESIZE_SEL_DATA);

    g_oalCacheInfo.L1DLineSize    = 1 << (((dwCacheSize & ARMV7_CACHESIZE_LINESIZE_MASK) >> ARMV7_CACHESIZE_LINESIZE_SHIFT) + 4);
    g_oalCacheInfo.L1DNumWays     = ((dwCacheSize & ARMV7_CACHESIZE_ASSOCIATIVITY_MASK) >> ARMV7_CACHESIZE_ASSOCIATIVITY_SHIFT) + 1;
    g_oalCacheInfo.L1DSetsPerWay  = ((dwCacheSize & ARMV7_CACHESIZE_NUMSETS_MASK) >> ARMV7_CACHESIZE_NUMSETS_SHIFT) + 1;
    g_oalCacheInfo.L1DSize        = g_oalCacheInfo.L1DLineSize * g_oalCacheInfo.L1DNumWays * g_oalCacheInfo.L1DSetsPerWay;

    //  L1 instruction
    dwCacheSize = OALGetCacheSizeID(ARMV7_CACHESIZE_SEL_L1|ARMV7_CACHESIZE_SEL_INSTRUCTION);

    g_oalCacheInfo.L1ILineSize    = 1 << (((dwCacheSize & ARMV7_CACHESIZE_LINESIZE_MASK) >> ARMV7_CACHESIZE_LINESIZE_SHIFT) + 4);
    g_oalCacheInfo.L1INumWays     = ((dwCacheSize & ARMV7_CACHESIZE_ASSOCIATIVITY_MASK) >> ARMV7_CACHESIZE_ASSOCIATIVITY_SHIFT) + 1;
    g_oalCacheInfo.L1ISetsPerWay  = ((dwCacheSize & ARMV7_CACHESIZE_NUMSETS_MASK) >> ARMV7_CACHESIZE_NUMSETS_SHIFT) + 1;
    g_oalCacheInfo.L1ISize        = g_oalCacheInfo.L1ILineSize * g_oalCacheInfo.L1INumWays * g_oalCacheInfo.L1ISetsPerWay;



    //  Set L2 cache properties
    dwCacheLevelID >>= 3;
    g_oalCacheInfo.L2Flags |= ((dwCacheLevelID & ARMV7_CACHEID_MASK) == ARMV7_CACHEID_UCACHE) ? CF_UNIFIED : 0;

    //  L2 unified cache
    dwCacheSize = OALGetCacheSizeID(ARMV7_CACHESIZE_SEL_L2|ARMV7_CACHESIZE_SEL_DATA);

    g_oalCacheInfo.L2DLineSize    = 1 << (((dwCacheSize & ARMV7_CACHESIZE_LINESIZE_MASK) >> ARMV7_CACHESIZE_LINESIZE_SHIFT) + 4);
    g_oalCacheInfo.L2DNumWays     = ((dwCacheSize & ARMV7_CACHESIZE_ASSOCIATIVITY_MASK) >> ARMV7_CACHESIZE_ASSOCIATIVITY_SHIFT) + 1;
    g_oalCacheInfo.L2DSetsPerWay  = ((dwCacheSize & ARMV7_CACHESIZE_NUMSETS_MASK) >> ARMV7_CACHESIZE_NUMSETS_SHIFT) + 1;
    g_oalCacheInfo.L2DSize        = g_oalCacheInfo.L2DLineSize * g_oalCacheInfo.L2DNumWays * g_oalCacheInfo.L2DSetsPerWay;

    g_oalCacheInfo.L2ILineSize    = g_oalCacheInfo.L2DLineSize;
    g_oalCacheInfo.L2INumWays     = g_oalCacheInfo.L2DNumWays;
    g_oalCacheInfo.L2ISetsPerWay  = g_oalCacheInfo.L2DSetsPerWay;
    g_oalCacheInfo.L2ISize        = g_oalCacheInfo.L2DSize;


    //  Compute cache range limit
    /*
    g_dwRangeLimit = g_oalCacheInfo.L1DLineSize*(
                        g_oalCacheInfo.L1DSetsPerWay*g_oalCacheInfo.L1DNumWays +
                        g_oalCacheInfo.L2DSetsPerWay*g_oalCacheInfo.L2DNumWays);
    */
    g_dwRangeLimit = g_oalCacheInfo.L1DSize;


    OALMSG(OAL_CACHE&&OAL_VERBOSE,
            (TEXT("CacheType = %x   CacheLevelID = %x\r\n"),
             dwCacheType,
             dwCacheLevelID));

    OALMSG(OAL_CACHE&&OAL_VERBOSE, 
            (TEXT("L1 cache details:\r\n flags %x\r\nI: %d sets/way, %d ways, %d line size, %d size\r\nD: %d sets/way, %d ways, %d line size, %d size\r\n"),
             g_oalCacheInfo.L1Flags,
             g_oalCacheInfo.L1ISetsPerWay, g_oalCacheInfo.L1INumWays,
             g_oalCacheInfo.L1ILineSize, g_oalCacheInfo.L1ISize,
             g_oalCacheInfo.L1DSetsPerWay, g_oalCacheInfo.L1DNumWays,
             g_oalCacheInfo.L1DLineSize, g_oalCacheInfo.L1DSize));

    OALMSG(OAL_CACHE&&OAL_VERBOSE, 
            (TEXT("L2 cache details:\r\n flags %x\r\nI: %d sets/way, %d ways, %d line size, %d size\r\nD: %d sets/way, %d ways, %d line size, %d size\r\n"),
             g_oalCacheInfo.L2Flags,
             g_oalCacheInfo.L2ISetsPerWay, g_oalCacheInfo.L2INumWays,
             g_oalCacheInfo.L2ILineSize, g_oalCacheInfo.L2ISize,
             g_oalCacheInfo.L2DSetsPerWay, g_oalCacheInfo.L2DNumWays,
             g_oalCacheInfo.L2DLineSize, g_oalCacheInfo.L2DSize));


    OALMSG(OAL_CACHE&&OAL_VERBOSE, (L"-OALCacheGlobalsInit\r\n"));
}

//------------------------------------------------------------------------------

