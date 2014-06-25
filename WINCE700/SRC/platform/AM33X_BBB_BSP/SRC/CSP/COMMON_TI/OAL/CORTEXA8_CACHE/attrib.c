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

//------------------------------------------------------------------------------
//
//  File:  attrib.c
//
//  This file implements OALSetMemoryAttributes function 
//
#include "omap.h"
#include <oal.h>
#include <oalex.h>

//
//  Defines
//

#define CACHE_MASK              0x000001CC          // Mask for TEX[2:0], C, and B bits for Section (4KB page)
#define CACHE_NOCACHE           0x00000000          // TEX = 000, C=0, B=0   Non-cacheable
#define CACHE_WRITECOMBINE      0x00000040          // TEX = 001, C=0, B=0   Non-cacheable, write-combine
#define CACHE_WRITETHROUGH      0x00000008          // TEX = 000, C=1, B=0   Outer and Inner Write-Through, no Write-Allocate
#define CACHE_WRITEBACK         0x0000000C          // TEX = 000, C=1, B=1   Outer and Inner Write-Back, no Write-Allocate


//
//  Functions
//

//
//  Globals
//


//------------------------------------------------------------------------------
//
//  Function:  OALSetMemoryAttributes
//
//  This function supports setting memory attributes via the CeSetMemoryAttributes
//  The only attributes supported are:
//
//      PAGE_WRITECOMBINE -     Maps to Cache Write Thru / Buffered / No write allocate
//
BOOL 
OALSetMemoryAttributes(
    LPVOID pVirtAddr,           // Virtual address of region
    LPVOID pPhysAddrShifted,    // PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,              // Size of the region
    DWORD  dwAttributes         // attributes to be set
    )
{
    BOOL    bResult = FALSE;

    OALMSG(OAL_MEMORY&&OAL_VERBOSE, (
        L"+OALSetMemoryAttributes()\r\n"
    ));

    UNREFERENCED_PARAMETER(pPhysAddrShifted);


    //  Set the specific atrributes for the given address
    switch( dwAttributes )
    {
        case TI_PAGE_NOCACHE:
            //  Set the no cache attributes
            bResult = NKVirtualSetAttributes(pVirtAddr, cbSize, CACHE_NOCACHE, CACHE_MASK, &dwAttributes );
            break;
            
        case TI_PAGE_WRITECOMBINE:
            //  Set the write combine attributes
            bResult = NKVirtualSetAttributes(pVirtAddr, cbSize, CACHE_WRITECOMBINE, CACHE_MASK, &dwAttributes );
            break;
            
        case TI_PAGE_WRITETHROUGH:
        case PAGE_WRITETHROUGH:
            //  Set the write through attributes
            bResult = NKVirtualSetAttributes(pVirtAddr, cbSize, CACHE_WRITETHROUGH, CACHE_MASK, &dwAttributes );
            break;
            
        case TI_PAGE_WRITEBACK:
            //  Set the write back attributes
            bResult = NKVirtualSetAttributes(pVirtAddr, cbSize, CACHE_WRITEBACK, CACHE_MASK, &dwAttributes );
            break;
            
        default:
            //  Ignore
            break;            
    }

    OALMSG(OAL_MEMORY&&OAL_VERBOSE, (L"-OALSetMemoryAttributes\r\n"));
    
    return bResult;
}

//------------------------------------------------------------------------------

